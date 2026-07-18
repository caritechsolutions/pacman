/*****************************************************************************
 * app_rist_capture.c  --  RIST satellite capture -> userspace TS
 *
 * ITER 1 SCOPE (this file):
 *   selected program --> player3(REC) record --> ringmem:// --> reader thread
 *   --> MediaRead(PLAYER_READ_DUMPER) --> dump to a file on USB.
 *
 * The single question Iter 1 answers: does the player record pipeline deliver
 * real ES (video+audio) from the tuner into a ringmem sink on this chip? PVR
 * proves the pipeline delivers ES to a FILE; here we prove it to MEMORY.
 *
 * Run this against the WORKING baseline (NORMAL still playing dvbs:// live, so
 * the tuner is already locked). That isolates the ringmem tap as the only
 * variable. Later iterations layer on:
 *   Iter 1.5 - NORMAL plays udp://, capture tunes/holds the tuner itself
 *   Iter 2   - marker PID via ext_pids (is_rawts=1 diagnostic first)
 *   Iter 3   - own PAT/PMT injection (self-contained TS, no ts_encrypt trust)
 *   Iter 4   - sink file -> UDP sendto()
 *
 * Design rules honored here:
 *   - Capture runs ONLY on player3 (PLAYER_FOR_REC); the only slot that
 *     coexists with live NORMAL per player_logic. Mutually exclusive with
 *     USB-PVR (also player3) -- we refuse to start if PVR is active.
 *   - Every successful app_player_open(REC) is matched by app_player_close(REC)
 *     on EVERY exit path (single cleanup label). Never return between open and
 *     its matching close -- that is exactly what wedged player3 before
 *     ({NORMAL,REC,REC} -> PLAYER_OPS_FORBID -> PVR dead until reboot).
 *   - Reader thread is stopped+joined BEFORE app_player_close (which frees the
 *     dumper+ringmem), so no MediaRead ever touches a freed dumper.
 *   - Heavy logging via printf (serial console), tagged [RIST].
 *****************************************************************************/

#include "gxcore.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app.h"                 /* APP_TIMER_ADD / APP_TIMER_REMOVE, create_timer */
#include "gxplayer.h"            /* GxPlayer_MediaRecordConfig / MediaRead, PlayerRecordConfig */
#include "module/app_pvr.h"      /* g_AppPvrOps, pvr_state, PVR_DUMMY */
#include "module/app_nim.h"      /* AppFrontend_Config (ts_src / dmx_id) */
#include "module/app_ioctl.h"    /* app_ioctl, FRONTEND_CONFIG_GET */

#include <stdio.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ config */
#define RIST_PLAYER             PLAYER_FOR_REC          /* "player3" */
#define RIST_RING_ID            7                       /* ringmem instance id */
#define RIST_RING_SIZE          (188 * 3 * 1000)        /* ~564 KB, multiple of 188 */
#define RIST_START_DELAY_MS     2000                    /* let the live player settle */
#define RIST_READ_CHUNK         (188 * 350)             /* ~65 KB per MediaRead */
#define RIST_DUMP_PATH          "/media/sda1/rist_dump.ts"  /* ITER 1: file sink */

#define RIST_LOG(fmt, ...)      printf("[RIST] " fmt, ##__VA_ARGS__)

/* reused, globally-linked helpers (defined in app_play_control.c / app_pvr.c) */
extern void app_player_url_get(char *url, GxBusPmDataProg *prog, uint16_t ts_src, uint16_t dmx_id);
extern void app_player_record_config_get(int prog_id, PlayerRecordConfig *config);

/* ------------------------------------------------------------------- state */
static struct {
    int             active;         /* capture fully up */
    int             opened;         /* app_player_open(REC) succeeded -> owes a close */
    volatile int    reader_run;     /* reader loop flag */
    handle_t        reader_thread;  /* GxCore thread handle */
    event_list     *start_timer;    /* deferred-start one-shot timer */
    GxBusPmDataProg prog;           /* program to capture */
    uint32_t        marker_pid;     /* ITER 1: unused (0) */
} s_rist = { 0, 0, 0, -1, NULL, {0}, 0 };

/* --------------------------------------------------------------- reader thr */
static void _rist_reader(void *arg)
{
    uint8_t   *buf   = NULL;
    FILE      *fp    = NULL;
    uint64_t   total = 0, since = 0;
    time_t     last  = time(NULL);
    int        first = 1;

    buf = (uint8_t *)GxCore_Mallocz(RIST_READ_CHUNK);
    if (buf == NULL) {
        RIST_LOG("reader: malloc(%d) FAILED\n", RIST_READ_CHUNK);
        return;
    }
    fp = fopen(RIST_DUMP_PATH, "wb");
    if (fp == NULL) {
        RIST_LOG("reader: fopen(%s) FAILED (USB mounted?)\n", RIST_DUMP_PATH);
        GxCore_Free(buf);
        return;
    }
    RIST_LOG("reader: started, dumping -> %s (ring id:%d size:%d)\n",
             RIST_DUMP_PATH, RIST_RING_ID, RIST_RING_SIZE);

    while (s_rist.reader_run) {
        int n = GxPlayer_MediaRead(RIST_PLAYER, PLAYER_READ_DUMPER, buf, RIST_READ_CHUNK);
        if (n > 0) {
            if (first) {
                RIST_LOG("reader: FIRST read = %d bytes (%d pkts)\n", n, n / 188);
                first = 0;
            }
            fwrite(buf, 1, (size_t)n, fp);
            total += (uint64_t)n;
            since += (uint64_t)n;
        } else {
            GxCore_ThreadDelay(10);
        }

        {
            time_t now = time(NULL);
            if (now != last) {
                RIST_LOG("reader: %llu B/s  total=%llu B (%llu pkts)%s\n",
                         (unsigned long long)since,
                         (unsigned long long)total,
                         (unsigned long long)(total / 188),
                         (since == 0) ? "   <-- ZERO PAYLOAD!" : "");
                since = 0;
                last  = now;
            }
        }
    }

    fflush(fp);
    fclose(fp);
    GxCore_Free(buf);
    RIST_LOG("reader: stopped, total=%llu B (%llu pkts)\n",
             (unsigned long long)total, (unsigned long long)(total / 188));
}

/* ----------------------------------------------------------------- teardown */
void app_rist_capture_stop(void)
{
    /* cancel a pending deferred start (safe if NULL) */
    APP_TIMER_REMOVE(s_rist.start_timer);

    if (!s_rist.active && !s_rist.opened && s_rist.reader_thread <= 0)
        return;

    RIST_LOG("stop: tearing down capture\n");

    /* 1) stop reader FIRST: no MediaRead against a dumper we are about to free */
    if (s_rist.reader_run || s_rist.reader_thread > 0) {
        s_rist.reader_run = 0;
        if (s_rist.reader_thread > 0) {
            GxCore_ThreadJoin(s_rist.reader_thread);
            s_rist.reader_thread = -1;
        }
    }

    /* 2) close REC: GXMSG_PLAYER_STOP -> MediaStop -> frees dumper+ringmem and
     *    unwinds player_logic_record so the {NORMAL,REC} prefix is restored. */
    if (s_rist.opened) {
        app_player_close(RIST_PLAYER);
        s_rist.opened = 0;
    }

    s_rist.active = 0;
    RIST_LOG("stop: done, player3 released\n");
}

/* ------------------------------------------------------------- deferred start */
static int _rist_start_cb(void *arg)
{
    PlayerRecordConfig         *cfg = NULL;
    GxMsgProperty_PlayerRecord *rec = NULL;
    AppFrontend_Config          fe  = {0};
    char                        url[PLAYER_URL_LONG + 1] = {0};

    /* TIMER_ONCE auto-removes itself after firing; drop our stale handle now
     * so a later APP_TIMER_REMOVE in stop() never touches freed memory. */
    s_rist.start_timer = NULL;

    if (s_rist.active) {
        RIST_LOG("start: already active, skip\n");
        return 0;
    }

    /* product guard: do not fight USB-PVR for the single REC slot */
    if (g_AppPvrOps.state != PVR_DUMMY) {
        RIST_LOG("start: PVR active (state=%d) -> skip capture\n", g_AppPvrOps.state);
        return 0;
    }

    /* 1) open the dedicated REC player. mutates player_logic -> owes a close. */
    if (app_player_open(RIST_PLAYER) != PLAYER_OPEN_OK) {
        RIST_LOG("start: app_player_open(%s) FAILED (player_logic busy?)\n", RIST_PLAYER);
        return 0;                          /* nothing opened -> nothing to close */
    }
    s_rist.opened = 1;

    /* 2) config: real PSI (PAT+PMT userdata, have_pmt=1) + ext PIDs, same as PVR */
    cfg = (PlayerRecordConfig *)GxCore_Calloc(1, sizeof(PlayerRecordConfig));
    if (cfg == NULL) {
        RIST_LOG("start: cfg calloc FAILED\n");
        goto cleanup;
    }
    app_player_record_config_get((int)s_rist.prog.id, cfg);
    RIST_LOG("start: cfg have_pmt=%u userdata_len=%u ext_num=%u\n",
             cfg->ext_data.have_pmt, cfg->ext_data.userdata_len, cfg->ext_info.ext_num);
    /* ITER 2 will append the marker PID here (ext_pids/ext_types, ext_num++). */
    GxPlayer_MediaRecordConfig(RIST_PLAYER, cfg);
    GxCore_Free(cfg);
    cfg = NULL;

    /* 3) source URL. NORMAL dvb variant (no tsbuff:/tscache:) -> tunes + low latency.
     *    ITER 1: share the live tuner's ts_src/dmx (tuner already locked by NORMAL). */
    app_ioctl(s_rist.prog.tuner, FRONTEND_CONFIG_GET, &fe);
    app_player_url_get(url, &s_rist.prog, fe.ts_src, fe.dmx_id);
    RIST_LOG("start: srcurl=%s\n", url);

    /* 4) fire the record via the PROVEN message path, dest = ringmem */
    rec = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerRecord));
    if (rec == NULL) {
        RIST_LOG("start: rec malloc FAILED\n");
        goto cleanup;
    }
    rec->player = RIST_PLAYER;
    strncpy((char *)rec->url, url, PLAYER_URL_LONG);
    snprintf((char *)rec->file, PLAYER_URL_LONG, "ringmem://id:%d&size:%d&",
             RIST_RING_ID, RIST_RING_SIZE);
    RIST_LOG("start: dst=%s\n", (char *)rec->file);

    if (app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)rec) != GXCORE_SUCCESS) {
        RIST_LOG("start: GXMSG_PLAYER_RECORD FAILED\n");
        GXCORE_FREE(rec);
        goto cleanup;
    }
    GXCORE_FREE(rec);

    /* 5) spawn the reader */
    s_rist.reader_run = 1;
    if (GxCore_ThreadCreate("app_rist_reader", &s_rist.reader_thread,
                            _rist_reader, NULL, 32 * 1024,
                            GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS) {
        RIST_LOG("start: reader thread create FAILED\n");
        s_rist.reader_run    = 0;
        s_rist.reader_thread = -1;
        goto cleanup;          /* record is up -> cleanup MediaStops it via close */
    }

    s_rist.active = 1;
    RIST_LOG("start: capture ACTIVE (prog_id=%d ts_id=%d svc_id=%d marker=%u)\n",
             s_rist.prog.id, s_rist.prog.tp_id, s_rist.prog.service_id, s_rist.marker_pid);
    return 0;

cleanup:
    if (cfg) {
        GxCore_Free(cfg);
        cfg = NULL;
    }
    if (s_rist.reader_run || s_rist.reader_thread > 0) {
        s_rist.reader_run = 0;
        if (s_rist.reader_thread > 0) {
            GxCore_ThreadJoin(s_rist.reader_thread);
            s_rist.reader_thread = -1;
        }
    }
    if (s_rist.opened) {
        app_player_close(RIST_PLAYER);   /* matches the successful open above */
        s_rist.opened = 0;
    }
    s_rist.active = 0;
    RIST_LOG("start: cleaned up after failure\n");
    return 0;
}

/* --------------------------------------------------------------- entry hook */
/* Called from app_normal_play() after the live player is set up. */
int app_rist_play_change(GxBusPmDataProg *prog)
{
    if (prog == NULL)
        return -1;

    /* always fully tear down the previous capture before re-arming */
    app_rist_capture_stop();

    memcpy(&s_rist.prog, prog, sizeof(GxBusPmDataProg));
    s_rist.marker_pid = 0;      /* ITER 1: marker fetch not wired yet */

    RIST_LOG("play_change: prog_id=%d ts_id=%d svc_id=%d -> start in %dms\n",
             prog->id, prog->tp_id, prog->service_id, RIST_START_DELAY_MS);

    APP_TIMER_ADD(s_rist.start_timer, _rist_start_cb, RIST_START_DELAY_MS, TIMER_ONCE);
    return 0;
}
