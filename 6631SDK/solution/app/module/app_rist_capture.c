/*****************************************************************************
 * app_rist_capture.c  --  RIST satellite capture -> userspace TS -> UDP
 *
 * PROVEN PATH (Iter 1c/2):
 *   selected program --> player3(REC) record to a DVR-volume dest
 *   ("/media/sda1/ristcap.ts.dvr") --> stream_dvr DMAs the secure hardware TSW
 *   buffer to disk as CLEAR TS --> we live-tail "/media/sda1/ristcap/0000.ts".
 *   (ringmem:// is impossible for a secure DVB record; a flat .ts hits
 *    wsize:-1; only the .dvr/stream_dvr path works -- see git history.)
 *
 * ITER 3 (this file): wire the tail to UDP.
 *   reader tails 0000.ts (live, from EOF) and for every block:
 *     1. injects our own PAT/PMT (app_pat_generate/app_pmt_generate -- the
 *        same generators PVR uses; PMT declares real stream_types e.g. HEVC
 *        0x24 / AAC) ahead of the ES, so the stream is self-describing;
 *     2. sendto()s 1316-byte (7x188) datagrams to a UDP dest (default
 *        multicast 239.6.6.6:6000, overridable via /tmp/ristcap "ip:port").
 *   The record keeps running -- it is the source of the tail.
 *
 * Later: Iter 4 = volume rotation/recycling + the marker PID (add to the
 * record config's ext_pids AND declare it in the generated PMT -- hook left
 * in _rist_build_psi).
 *
 * Discipline: capture only on player3 (sole slot that coexists with live
 * NORMAL); refuses while USB-PVR is active; every app_player_open(REC) is
 * matched by app_player_close(REC) on all exits; reader stopped+joined before
 * the player close; gate on /media/sda1 being mounted (USB attaches ~65-140s).
 *****************************************************************************/

#include "gxcore.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app.h"                 /* APP_TIMER_ADD / APP_TIMER_REMOVE */
#include "gxplayer.h"            /* GxPlayer_Media*, PlayerRecordConfig, PlayerPVRConfig */
#include "module/app_pvr.h"      /* g_AppPvrOps, PVR_DUMMY */
#include "module/app_nim.h"      /* AppFrontend_Config (ts_src / dmx_id) */
#include "module/app_ioctl.h"    /* app_ioctl, FRONTEND_CONFIG_GET */
#include "module/app_psi.h"      /* app_pat_generate/app_pmt_generate/app_ts_pack, AppTsData */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ------------------------------------------------------------------ config */
#define RIST_PLAYER             PLAYER_FOR_REC          /* "player3" */
#define RIST_DEST               "/media/sda1/ristcap.ts.dvr"
#define RIST_VOL0               "/media/sda1/ristcap/0000.ts"
#define RIST_USB_MOUNT          "/media/sda1"
#define RIST_START_DELAY_MS     2000                    /* let the live player settle */
#define RIST_READ_CHUNK         (188 * 350)             /* 65800 = 50 x 1316, packet-aligned */
#define RIST_DGRAM              (188 * 7)               /* 1316: standard TS-over-UDP datagram */
#define RIST_PSI_MAX            (188 * 12)              /* room for PAT + a large PMT */
#define RIST_PSI_PIDS           4                       /* distinct PIDs in the PSI burst */
#define RIST_VOL_SIZEMB         2000                    /* one big 0000.ts; avoid rotation (Iter 4) */

#define RIST_CTRL_FILE          "/tmp/ristcap"          /* optional "ip:port" override */
#define RIST_UDP_DEFAULT_IP     "239.6.6.6"
#define RIST_UDP_DEFAULT_PORT   6000

#define RIST_LOG(fmt, ...)      printf("[RIST] " fmt, ##__VA_ARGS__)
#define ULL(x)                  ((unsigned long long)(x))

/* reused, globally-linked helpers (defined in app_play_control.c / app_pvr.c) */
extern void app_player_url_get(char *url, GxBusPmDataProg *prog, uint16_t ts_src, uint16_t dmx_id);
extern void app_player_record_config_get(int prog_id, PlayerRecordConfig *config);
extern status_t GxPlayer_MediaRecord(const char *name, const char *srcurl, const char *file);
extern status_t GxPlayer_MediaGetStatus(const char *name, PlayerStatusInfo *info);

/* ------------------------------------------------------------------- state */
static struct {
    int             active;
    int             opened;         /* app_player_open(REC) succeeded -> owes a close */
    volatile int    reader_run;
    handle_t        reader_thread;
    event_list     *start_timer;
    GxBusPmDataProg prog;
    uint32_t        marker_pid;     /* Iter 4 */

    /* PSI generated once per capture (program is stable) */
    unsigned char   psi[RIST_PSI_MAX];
    int             psi_len;
    int             pmt_pid;
    int             cc_pid[RIST_PSI_PIDS];
    int             cc_val[RIST_PSI_PIDS];

    /* UDP out */
    int             udp_fd;
    struct sockaddr_in dst;
    char            dst_ip[24];
    int             dst_port;
    uint64_t        sent;
    uint64_t        senderr;
} s_rist = { 0, 0, 0, -1, NULL, {0}, 0,
             {0}, 0, 0, {-1,-1,-1,-1}, {0,0,0,0},
             -1, {0}, {0}, 0, 0, 0 };

/* --------------------------------------------------------------- USB gate */
static int _rist_usb_ready(void)
{
    return (access(RIST_USB_MOUNT, W_OK) == 0);   /* mounted + writable */
}

/* --------------------------------------------------------------- PSI build */
/* Generate PAT + PMT TS packets once, exactly like app_pvr.c's
 * _pvr_pat_ts_pack / _pvr_pmt_ts_pack. Runs on the app thread while the
 * captured program is the current one, so app_pmt_generate() reads the right
 * program info (real video/audio stream_types). */
static void _rist_build_psi(uint16_t prog_id)
{
    AppTsData ts = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    char *pat = NULL, *pmt = NULL;
    int dlen, i;

    s_rist.psi_len = 0;
    s_rist.pmt_pid = 0;
    for (i = 0; i < RIST_PSI_PIDS; i++) { s_rist.cc_pid[i] = -1; s_rist.cc_val[i] = 0; }

    /* PAT on PID 0 */
    pat = app_pat_generate(&prog_id, 1);
    if (pat) {
        dlen = ((pat[1] & 0x0f) << 8) | pat[2];
        dlen += 3;
        if (app_ts_pack(0, pat, dlen, &ts) == GXCORE_SUCCESS) {
            for (i = 0; i < ts.pack_num; i++) {
                if (s_rist.psi_len + 188 > RIST_PSI_MAX) break;
                memcpy(s_rist.psi + s_rist.psi_len, ts.pack_data[i], 188);
                s_rist.psi_len += 188;
            }
        }
        app_ts_data_free(&ts);
        GxCore_Free(pat);
    }

    /* PMT pid from the program node */
    node.node_type = NODE_PROG;
    node.id = prog_id;
    if (app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
        s_rist.pmt_pid = node.prog_data.pmt_pid;

    /* PMT on pmt_pid. No private descriptor for RIST (PVR passes its lock tag;
     * we want a clean, standard PMT). Iter 4: append the marker PID's
     * elementary-stream entry here so the stream stays self-describing. */
    memset(&ts, 0, sizeof(ts));
    pmt = app_pmt_generate(prog_id, NULL);
    if (pmt) {
        dlen = ((pmt[1] & 0x0f) << 8) | pmt[2];
        dlen += 3;
        if (app_ts_pack((unsigned short)s_rist.pmt_pid, pmt, dlen, &ts) == GXCORE_SUCCESS) {
            for (i = 0; i < ts.pack_num; i++) {
                if (s_rist.psi_len + 188 > RIST_PSI_MAX) break;
                memcpy(s_rist.psi + s_rist.psi_len, ts.pack_data[i], 188);
                s_rist.psi_len += 188;
            }
        }
        app_ts_data_free(&ts);
        GxCore_Free(pmt);
    }

    RIST_LOG("psi: built %d bytes (%d pkts), pmt_pid=%d\n",
             s_rist.psi_len, s_rist.psi_len / 188, s_rist.pmt_pid);
}

/* Patch per-PID continuity counters (like the SDK's _modify_hdr_cc), then send
 * the PAT+PMT burst as one datagram ahead of the ES. */
static void _rist_psi_send(void)
{
    int off, k;

    if (s_rist.psi_len <= 0 || s_rist.udp_fd < 0)
        return;

    for (off = 0; off + 188 <= s_rist.psi_len; off += 188) {
        unsigned char *p = s_rist.psi + off;
        int pid = ((p[1] & 0x1f) << 8) | p[2];
        for (k = 0; k < RIST_PSI_PIDS; k++) {
            if (s_rist.cc_pid[k] == pid || s_rist.cc_pid[k] < 0) {
                s_rist.cc_pid[k] = pid;
                p[3] = (unsigned char)((p[3] & 0xf0) | (s_rist.cc_val[k] & 0x0f));
                s_rist.cc_val[k] = (s_rist.cc_val[k] + 1) & 0x0f;
                break;
            }
        }
    }

    if (sendto(s_rist.udp_fd, s_rist.psi, s_rist.psi_len, 0,
               (struct sockaddr *)&s_rist.dst, sizeof(s_rist.dst)) < 0)
        s_rist.senderr++;
    else
        s_rist.sent++;
}

/* ------------------------------------------------------------------- UDP */
static int _rist_udp_open(void)
{
    char ip[24];
    int  port, fo;
    FILE *f;

    strncpy(ip, RIST_UDP_DEFAULT_IP, sizeof(ip) - 1);
    ip[sizeof(ip) - 1] = '\0';
    port = RIST_UDP_DEFAULT_PORT;

    f = fopen(RIST_CTRL_FILE, "r");
    if (f) {
        char line[64] = {0};
        if (fgets(line, sizeof(line) - 1, f)) {
            char tip[24];
            int  tport;
            if (sscanf(line, "%23[^:\n]:%d", tip, &tport) == 2 && tport > 0 && tport < 65536) {
                strncpy(ip, tip, sizeof(ip) - 1);
                ip[sizeof(ip) - 1] = '\0';
                port = tport;
            }
        }
        fclose(f);
        RIST_LOG("udp: %s -> dest %s:%d\n", RIST_CTRL_FILE, ip, port);
    } else {
        RIST_LOG("udp: no %s -> default dest %s:%d\n", RIST_CTRL_FILE, ip, port);
    }

    s_rist.udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_rist.udp_fd < 0) {
        RIST_LOG("udp: socket() FAILED\n");
        return -1;
    }

    memset(&s_rist.dst, 0, sizeof(s_rist.dst));
    s_rist.dst.sin_family      = AF_INET;
    s_rist.dst.sin_addr.s_addr = inet_addr(ip);
    s_rist.dst.sin_port        = htons((unsigned short)port);
    strncpy(s_rist.dst_ip, ip, sizeof(s_rist.dst_ip) - 1);
    s_rist.dst_ip[sizeof(s_rist.dst_ip) - 1] = '\0';
    s_rist.dst_port = port;

    fo = atoi(ip);   /* first octet */
    if (fo >= 224 && fo <= 239) {
        unsigned char ttl = 8;
        setsockopt(s_rist.udp_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
        RIST_LOG("udp: multicast dest -> IP_MULTICAST_TTL=8\n");
    }
    return 0;
}

/* --------------------------------------------------------------- reader */
static void _rist_reader(void *arg)
{
    uint8_t  *rbuf  = NULL;
    uint8_t  *sbuf  = NULL;   /* accumulator for 1316-byte datagram alignment */
    int       sfill = 0;
    FILE     *in    = NULL;
    uint64_t  total = 0, since = 0;
    time_t    last  = time(NULL);
    int       first = 1, waited = 0;

    rbuf = (uint8_t *)GxCore_Mallocz(RIST_READ_CHUNK);
    sbuf = (uint8_t *)GxCore_Mallocz(RIST_READ_CHUNK + 2 * RIST_DGRAM);
    if (rbuf == NULL || sbuf == NULL) {
        RIST_LOG("reader: malloc FAILED\n");
        goto done;
    }

    /* the record creates the volume asynchronously; wait for 0000.ts */
    while (s_rist.reader_run && in == NULL) {
        in = fopen(RIST_VOL0, "rb");
        if (in == NULL) {
            if ((++waited % 20) == 0)
                RIST_LOG("reader: waiting for %s (%d x100ms)...\n", RIST_VOL0, waited);
            GxCore_ThreadDelay(100);
        }
    }
    if (in == NULL) {
        RIST_LOG("reader: stop requested before %s appeared\n", RIST_VOL0);
        goto done;
    }
    fseek(in, 0, SEEK_END);   /* start LIVE: skip the on-disk backlog */
    RIST_LOG("reader: tailing %s live -> udp %s:%d\n", RIST_VOL0, s_rist.dst_ip, s_rist.dst_port);

    while (s_rist.reader_run) {
        size_t n = fread(rbuf, 1, RIST_READ_CHUNK, in);
        if (n > 0) {
            int off = 0;
            if (first) {
                RIST_LOG("reader: FIRST live read = %d bytes (%d pkts)\n", (int)n, (int)(n / 188));
                first = 0;
            }
            _rist_psi_send();                       /* PAT/PMT ahead of this ES block */

            memcpy(sbuf + sfill, rbuf, n);
            sfill += (int)n;
            while (sfill - off >= RIST_DGRAM) {
                if (sendto(s_rist.udp_fd, sbuf + off, RIST_DGRAM, 0,
                           (struct sockaddr *)&s_rist.dst, sizeof(s_rist.dst)) < 0)
                    s_rist.senderr++;
                else
                    s_rist.sent++;
                off += RIST_DGRAM;
            }
            if (off > 0) {
                if (sfill - off > 0)
                    memmove(sbuf, sbuf + off, sfill - off);
                sfill -= off;
            }
            total += (uint64_t)n;
            since += (uint64_t)n;
        } else {
            clearerr(in);              /* clear EOF so the next fread sees appended data */
            GxCore_ThreadDelay(20);
        }

        {
            time_t now = time(NULL);
            if (now != last) {
                RIST_LOG("reader: tail %llu B/s  total=%llu B  udp sent=%llu err=%llu%s\n",
                         ULL(since), ULL(total), ULL(s_rist.sent), ULL(s_rist.senderr),
                         (since == 0) ? "   (idle)" : "");
                since = 0;
                last  = now;
            }
        }
    }

done:
    if (in)   fclose(in);
    if (rbuf) GxCore_Free(rbuf);
    if (sbuf) GxCore_Free(sbuf);
    RIST_LOG("reader: stopped, total=%llu B  udp sent=%llu err=%llu\n",
             ULL(total), ULL(s_rist.sent), ULL(s_rist.senderr));
}

/* ----------------------------------------------------------------- teardown */
void app_rist_capture_stop(void)
{
    APP_TIMER_REMOVE(s_rist.start_timer);

    if (!s_rist.active && !s_rist.opened && s_rist.reader_thread <= 0 && s_rist.udp_fd < 0)
        return;

    RIST_LOG("stop: tearing down capture\n");

    /* 1) stop reader FIRST */
    if (s_rist.reader_run || s_rist.reader_thread > 0) {
        s_rist.reader_run = 0;
        if (s_rist.reader_thread > 0) {
            GxCore_ThreadJoin(s_rist.reader_thread);
            s_rist.reader_thread = -1;
        }
    }

    /* 2) close UDP socket */
    if (s_rist.udp_fd >= 0) {
        close(s_rist.udp_fd);
        s_rist.udp_fd = -1;
    }

    /* 3) close REC -> MediaStop -> stops stream_dvr, unwinds player_logic */
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
    AppFrontend_Config fe  = {0};
    char               url[PLAYER_URL_LONG + 1] = {0};

    s_rist.start_timer = NULL;   /* TIMER_ONCE auto-removed */

    if (s_rist.active) {
        RIST_LOG("start: already active, skip\n");
        return 0;
    }
    if (g_AppPvrOps.state != PVR_DUMMY) {
        RIST_LOG("start: PVR active (state=%d) -> skip capture\n", g_AppPvrOps.state);
        return 0;
    }

    /* USB may attach late (~65-140s). Retry until /media/sda1 is mounted. */
    if (!_rist_usb_ready()) {
        RIST_LOG("start: %s not mounted yet -> retry in %dms\n", RIST_USB_MOUNT, RIST_START_DELAY_MS);
        APP_TIMER_ADD(s_rist.start_timer, _rist_start_cb, RIST_START_DELAY_MS, TIMER_ONCE);
        return 0;
    }

    /* 1) open the dedicated REC player (owes a close) */
    if (app_player_open(RIST_PLAYER) != PLAYER_OPEN_OK) {
        RIST_LOG("start: app_player_open(%s) FAILED\n", RIST_PLAYER);
        return 0;
    }
    s_rist.opened = 1;

    /* 2) one big volume so 0000.ts doesn't rotate during the run (rotation = Iter 4) */
    {
        PlayerPVRConfig pc = {0};
        GxPlayer_GetPVRConfig(&pc);
        pc.volume_sizemb = RIST_VOL_SIZEMB;
        pc.volume_maxnum = 0;
        GxPlayer_SetPVRConfig(&pc);
    }

    /* 3) bind record config: which PIDs the record captures (video+audio+...).
     * Same call PVR/Iter 2 used. Its have_pmt/userdata PSI goes to the .dvr
     * sidecar (secure record), NOT into 0000.ts -- so 0000.ts stays bare ES and
     * we inject our OWN PAT/PMT in the reader. Iter 4: append marker_pid to
     * cfg->ext_info.ext_pids/ext_types (+ ext_num++) here. */
    {
        PlayerRecordConfig *cfg = (PlayerRecordConfig *)GxCore_Calloc(1, sizeof(PlayerRecordConfig));
        if (cfg) {
            app_player_record_config_get((int)s_rist.prog.id, cfg);
            RIST_LOG("start: cfg have_pmt=%u userdata_len=%u ext_num=%u\n",
                     cfg->ext_data.have_pmt, cfg->ext_data.userdata_len, cfg->ext_info.ext_num);
            GxPlayer_MediaRecordConfig(RIST_PLAYER, cfg);
            GxCore_Free(cfg);
        }
    }

    /* 4) source URL: NORMAL dvb variant (no tsbuff/tscache) */
    app_ioctl(s_rist.prog.tuner, FRONTEND_CONFIG_GET, &fe);
    app_player_url_get(url, &s_rist.prog, fe.ts_src, fe.dmx_id);
    RIST_LOG("start: srcurl=%s\n", url);

    /* 5) record to the DVR-volume dest (proven-good path, no wsize:-1) */
    {
        status_t rr;
        PlayerStatusInfo si;

        memset(&si, 0, sizeof(si));
        if (GxPlayer_MediaGetStatus(RIST_PLAYER, &si) == GXCORE_SUCCESS)
            RIST_LOG("start: player3 status=%d BEFORE record (0=STOPPED wanted)\n", (int)si.status);

        RIST_LOG("start: GxPlayer_MediaRecord(%s, <dvbs...>, %s)\n", RIST_PLAYER, RIST_DEST);
        rr = GxPlayer_MediaRecord(RIST_PLAYER, url, RIST_DEST);
        RIST_LOG("start: GxPlayer_MediaRecord RET = %d\n", (int)rr);
        if (rr != GXCORE_SUCCESS) {
            RIST_LOG("start: record REFUSED -> aborting\n");
            goto cleanup;
        }
        memset(&si, 0, sizeof(si));
        if (GxPlayer_MediaGetStatus(RIST_PLAYER, &si) == GXCORE_SUCCESS)
            RIST_LOG("start: player3 status=%d AFTER record (6=RECORD_RUNNING wanted)\n", (int)si.status);
    }

    /* 6) build PSI (program is current here) and open the UDP socket */
    _rist_build_psi((uint16_t)s_rist.prog.id);
    if (_rist_udp_open() < 0) {
        RIST_LOG("start: UDP open FAILED -> aborting\n");
        goto cleanup;
    }

    /* 7) spawn the tail->UDP reader */
    s_rist.reader_run = 1;
    if (GxCore_ThreadCreate("app_rist_reader", &s_rist.reader_thread,
                            _rist_reader, NULL, 64 * 1024,
                            GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS) {
        RIST_LOG("start: reader thread create FAILED\n");
        s_rist.reader_run    = 0;
        s_rist.reader_thread = -1;
        goto cleanup;
    }

    s_rist.active = 1;
    RIST_LOG("start: capture ACTIVE (prog_id=%d ts_id=%d svc_id=%d) -> udp %s:%d\n",
             s_rist.prog.id, s_rist.prog.tp_id, s_rist.prog.service_id,
             s_rist.dst_ip, s_rist.dst_port);
    return 0;

cleanup:
    if (s_rist.reader_run || s_rist.reader_thread > 0) {
        s_rist.reader_run = 0;
        if (s_rist.reader_thread > 0) {
            GxCore_ThreadJoin(s_rist.reader_thread);
            s_rist.reader_thread = -1;
        }
    }
    if (s_rist.udp_fd >= 0) {
        close(s_rist.udp_fd);
        s_rist.udp_fd = -1;
    }
    if (s_rist.opened) {
        app_player_close(RIST_PLAYER);
        s_rist.opened = 0;
    }
    s_rist.active = 0;
    RIST_LOG("start: cleaned up after failure\n");
    return 0;
}

/* --------------------------------------------------------------- entry hook */
int app_rist_play_change(GxBusPmDataProg *prog)
{
    if (prog == NULL)
        return -1;

    app_rist_capture_stop();   /* fully tear down the previous capture */

    memcpy(&s_rist.prog, prog, sizeof(GxBusPmDataProg));
    s_rist.marker_pid = 0;     /* Iter 4 */
    s_rist.sent = 0;
    s_rist.senderr = 0;

    RIST_LOG("play_change: prog_id=%d ts_id=%d svc_id=%d -> start in %dms\n",
             prog->id, prog->tp_id, prog->service_id, RIST_START_DELAY_MS);

    APP_TIMER_ADD(s_rist.start_timer, _rist_start_cb, RIST_START_DELAY_MS, TIMER_ONCE);
    return 0;
}
