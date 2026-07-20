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

/* ITER 2: record to a DVR-VOLUME destination and tail 0000.ts.
 *
 * Root cause established from source + serial:
 *   - the DVB record produces a SECURE hardware TSW buffer (log: "tsw - hwsec 1,
 *     ptr_en 1 ... tswmem"), which the CPU cannot read.
 *   - ringmem:// (stream_ringmem, software mem) has no DVR/secure path -> the
 *     record is REFUSED at open (RET=-1).
 *   - a flat "*.ts" falls to plain stream_file, whose write() can't read the
 *     secure buffer -> "[Record]: Disk Access Failed !.. wsize:-1".
 *   - "*.ts.dvr" is mapped by url_rec_proto() to the "dvr" protocol -> stream_dvr,
 *     the secure/DVR-capable writer (gxsecure DMA) that PVR uses to produce a
 *     real 12 MB recording. So memory-direct is impossible for a secure DVB
 *     record; the TS only becomes CPU-readable once stream_dvr writes it to disk.
 *
 * volume_open_dvr_file() (fileops_volume.c) splits the dsturl on ".dvr",
 * GxCore_Mkdir()s <dir>, and writes volumes as "<dir>/%04d.ts" (MAKEVOL).
 *   dsturl "/media/sda1/ristcap.ts.dvr" -> volume "/media/sda1/ristcap/0000.ts"
 * We record there (proven-good path, no wsize:-1) and TAIL 0000.ts from
 * userspace. PSI injection + UDP sendto come in Iter 3; this iteration only
 * proves the DVR-dir record runs clean and the live tail reads real ES. */
#define RIST_DEST               "/media/sda1/ristcap.ts.dvr"
#define RIST_VOL0               "/media/sda1/ristcap/0000.ts"
#define RIST_START_DELAY_MS     2000                    /* let the live player settle */
#define RIST_READ_CHUNK         (188 * 350)             /* ~65 KB per MediaRead */
#define RIST_DUMP_PATH          "/media/sda1/rist_dump.ts"  /* ITER 1: file sink */

#define RIST_LOG(fmt, ...)      printf("[RIST] " fmt, ##__VA_ARGS__)

/* reused, globally-linked helpers (defined in app_play_control.c / app_pvr.c) */
extern void app_player_url_get(char *url, GxBusPmDataProg *prog, uint16_t ts_src, uint16_t dmx_id);
extern void app_player_record_config_get(int prog_id, PlayerRecordConfig *config);
/* Direct record call so we can SEE the real return (the GXMSG_PLAYER_RECORD
 * message path returns success on delivery and discards MediaRecord's -1). */
extern status_t GxPlayer_MediaRecord(const char *name, const char *srcurl, const char *file);
extern status_t GxPlayer_MediaGetStatus(const char *name, PlayerStatusInfo *info);

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
/* ITER 2 reader: live-tail the DVR volume 0000.ts that stream_dvr is writing.
 * The record writes CLEAR TS to disk (secure DMA handled by stream_dvr); the
 * CPU can read the file back, so we fopen it and follow its growth (clearerr
 * after each EOF so the next fread sees appended data). This proves the tail
 * delivers real ES before we add PSI + UDP in Iter 3. */
static void _rist_reader(void *arg)
{
    uint8_t   *buf   = NULL;
    FILE      *in    = NULL;
    uint64_t   total = 0, since = 0;
    time_t     last  = time(NULL);
    int        first = 1;
    int        waited = 0;

    buf = (uint8_t *)GxCore_Mallocz(RIST_READ_CHUNK);
    if (buf == NULL) {
        RIST_LOG("reader: malloc(%d) FAILED\n", RIST_READ_CHUNK);
        return;
    }

    /* the record creates the volume asynchronously; wait for 0000.ts to appear */
    while (s_rist.reader_run && in == NULL) {
        in = fopen(RIST_VOL0, "rb");
        if (in == NULL) {
            if ((++waited % 20) == 0)
                RIST_LOG("reader: waiting for %s to appear (%d x100ms)...\n", RIST_VOL0, waited);
            GxCore_ThreadDelay(100);
        }
    }
    if (in == NULL) {
        RIST_LOG("reader: stop requested before %s appeared\n", RIST_VOL0);
        GxCore_Free(buf);
        return;
    }
    RIST_LOG("reader: tailing %s\n", RIST_VOL0);

    while (s_rist.reader_run) {
        size_t n = fread(buf, 1, RIST_READ_CHUNK, in);
        if (n > 0) {
            if (first) {
                RIST_LOG("reader: FIRST tail read = %d bytes (%d pkts)\n", (int)n, (int)(n / 188));
                first = 0;
            }
            /* Iter 3 will inject PAT/PMT here and sendto() the UDP socket. */
            total += (uint64_t)n;
            since += (uint64_t)n;
        } else {
            clearerr(in);              /* clear EOF so we re-read appended data */
            GxCore_ThreadDelay(20);
        }

        {
            time_t now = time(NULL);
            if (now != last) {
                RIST_LOG("reader: tail %llu B/s  total=%llu B (%llu pkts)%s\n",
                         (unsigned long long)since,
                         (unsigned long long)total,
                         (unsigned long long)(total / 188),
                         (since == 0) ? "   <-- not growing yet" : "");
                since = 0;
                last  = now;
            }
        }
    }

    fclose(in);
    GxCore_Free(buf);
    RIST_LOG("reader: stopped, tailed total=%llu B (%llu pkts)\n",
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

    /* 4) fire the record DIRECTLY (not via GXMSG_PLAYER_RECORD) so we see the
     *    real return. The message path's DEF_APP_SEND_MSG_SYNC returns success
     *    on delivery and throws away MediaRecord's status -- which is why the
     *    previous run logged "ACTIVE" even though the ring never delivered a
     *    byte. sat2ip calls MediaRecord2 the same way (app-thread, direct). */
    {
        status_t rr;
        PlayerStatusInfo si;

        /* Log player3's status BEFORE the call -> directly tests the "Busy"
         * gate in gxplayer_media_record (it refuses unless status is one of
         * STOPPED(0)/ERROR/PLAY_END/RECORD_END/RECORD_FULL). player3 was
         * created by GxPlayer_MediaRecordConfig above, so Find/GetStatus works. */
        memset(&si, 0, sizeof(si));
        if (GxPlayer_MediaGetStatus(RIST_PLAYER, &si) == GXCORE_SUCCESS)
            RIST_LOG("start: player3 status=%d error=%d BEFORE record (0=STOPPED wanted)\n",
                     (int)si.status, (int)si.error);
        else
            RIST_LOG("start: MediaGetStatus BEFORE record FAILED (player3 not created?)\n");

        RIST_LOG("start: calling GxPlayer_MediaRecord(%s, <dvbs...>, %s)\n", RIST_PLAYER, RIST_DEST);
        rr = GxPlayer_MediaRecord(RIST_PLAYER, url, RIST_DEST);
        RIST_LOG("start: GxPlayer_MediaRecord RET = %d  (0=OK, <0 = refused)\n", (int)rr);
        if (rr != GXCORE_SUCCESS) {
            RIST_LOG("start: record REFUSED at dest '%s'. Compare to the known-good "
                     "PVR-to-file result: if FILE is ALSO refused here, the direct-call "
                     "CONTEXT is the problem (route via GXMSG/PVR sequence); if FILE works, "
                     "it is SCHEME-specific (ringmem).\n", RIST_DEST);
            goto cleanup;
        }

        memset(&si, 0, sizeof(si));
        if (GxPlayer_MediaGetStatus(RIST_PLAYER, &si) == GXCORE_SUCCESS)
            RIST_LOG("start: player3 status=%d error=%d AFTER record (6=RECORD_RUNNING wanted)\n",
                     (int)si.status, (int)si.error);
        RIST_LOG("start: record ACCEPTED -> expect NO 'wsize:-1'; volume %s should grow\n", RIST_VOL0);
    }

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
