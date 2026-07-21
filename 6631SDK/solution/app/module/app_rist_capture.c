/*****************************************************************************
 * app_rist_capture.c  --  RIST satellite capture -> userspace TS -> UDP
 *
 * PATH:
 *   selected program --> player3(REC) record to "/tmp/ristcap_rec.ts.dvr"
 *   --> stream_dvr writes DEVICE-ENCRYPTED TS to a tmpfs volume 0000.ts
 *   --> we live-tail the volume, DECRYPT each block, inject PAT/PMT, sendto UDP.
 *
 * KEY FACT (Iter "decrypt"): the recorded volume is NOT clear TS. stream_dvr
 * encrypts every block at rest with the device key (AES-128-ECB, TFM_KEY_SSUK,
 * TS-packet mode, HW_PVR_MODE) via _dvr_tfm_copy() -- that is why the .dvr
 * plays clean on the box (stream_dvr's READ decrypts) but raw 0000.ts is
 * high-entropy garbage. GxStream_* is not app-linkable, but GxTfm_Decrypt IS
 * (used by app_des_descrambler.c / app_system_init.c), so we replicate the
 * exact _dvr_tfm_copy DECRYPT on each block before sending.
 *
 * This build folds in a one-shot diagnostic: on the first block it logs the
 * raw vs decrypted 00-00-01 start-code counts, so one flash confirms the
 * transform AND (if it works) already streams clean TS.
 *
 * Robustness (Iter 4): tmpfs volume (no USB fragility), bounded circular
 * volume rotation (RAM cap), reader follows the incrementing volumes,
 * /tmp/ristcap re-read every ~2s (dest override without a re-zap).
 *
 * DEFERRED (Iter 5): marker PID (record ext_pids + PMT declaration); send
 * pacing; re-disable dvb2ip. Hooks marked below.
 *****************************************************************************/

#include "gxcore.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app.h"
#include "gxplayer.h"
#include "module/app_pvr.h"
#include "module/app_nim.h"
#include "module/app_ioctl.h"
#include "module/app_psi.h"
#include "gxsecure/gxtfm_api.h"   /* GxTfm_Decrypt, GxTfmCrypto, TFM_KEY_SSUK, flags */
#include "common/memhole.h"       /* GxCore_MemholeMalloc/Free -- DMA-capable buffers */
#include "common/gx_hw_malloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/statvfs.h>

/* ------------------------------------------------------------------ config */
#define RIST_PLAYER             PLAYER_FOR_REC
/* Dedicated size-capped tmpfs ramdisk for the volumes, so "Disk Full" can only
 * ever hit our own budget, never the rest of /tmp. Small volumes + the reader
 * deleting each one the instant it's consumed keeps ~2 volumes live at a time. */
#define RIST_RD_MOUNT           "/tmp/ristcap_rd"
#define RIST_RD_SIZE            "16m"
#define RIST_DEST               "/tmp/ristcap_rd/rec.ts.dvr"
#define RIST_VOL_DIR            "/tmp/ristcap_rd/rec"
#define RIST_VOL_SIZEMB         1          /* 1 MB per volume */
#define RIST_VOL_MAXNUM         0          /* DVR never deletes; the READER recycles */
#define RIST_RESERVE_MB         1          /* shrink DVR free-space reserve (default 20MB
                                            * > our whole ramdisk) so record can start */

#define RIST_START_DELAY_MS     2000
#define RIST_BLOCK              (188 * 256)             /* 48128 = DVR flush block (0xbc00) */
#define RIST_DGRAM              (188 * 7)               /* 1316: TS-over-UDP datagram */
#define RIST_PSI_MAX            (188 * 12)
#define RIST_PSI_PIDS           4
#define RIST_RESOLVE_SECS       2

#define RIST_MARKER_PID         0                       /* Iter 5 */

#define RIST_CTRL_FILE          "/tmp/ristcap"
#define RIST_UDP_DEFAULT_IP     "239.6.6.6"
#define RIST_UDP_DEFAULT_PORT   6000

#define RIST_LOG(fmt, ...)      printf("[RIST] " fmt, ##__VA_ARGS__)
#define ULL(x)                  ((unsigned long long)(x))

extern void app_player_url_get(char *url, GxBusPmDataProg *prog, uint16_t ts_src, uint16_t dmx_id);
extern void app_player_record_config_get(int prog_id, PlayerRecordConfig *config);
extern status_t GxPlayer_MediaRecord(const char *name, const char *srcurl, const char *file);
extern status_t GxPlayer_MediaGetStatus(const char *name, PlayerStatusInfo *info);

/* ------------------------------------------------------------------- state */
static struct {
    int             active;
    int             opened;
    volatile int    reader_run;
    handle_t        reader_thread;
    event_list     *start_timer;
    GxBusPmDataProg prog;
    uint32_t        marker_pid;

    unsigned char   psi[RIST_PSI_MAX];
    int             psi_len;
    int             pmt_pid;
    int             cc_pid[RIST_PSI_PIDS];
    int             cc_val[RIST_PSI_PIDS];

    int             udp_fd;
    struct sockaddr_in dst;
    char            dst_ip[24];
    int             dst_port;
    uint64_t        sent;
    uint64_t        senderr;
} s_rist = { 0, 0, 0, -1, NULL, {0}, 0,
             {0}, 0, 0, {-1,-1,-1,-1}, {0,0,0,0},
             -1, {0}, {0}, 0, 0, 0 };

/* ------------------------------------------------------------- DVR decrypt */
/* Verbatim of stream_dvr.c _dvr_tfm_copy(..., encrypt=0): reverse the
 * device-key encryption stream_dvr applied at record time. Position-
 * independent (per-TS-packet ECB), so it works on any 188-aligned run. */
static int _rist_dvr_decrypt(unsigned char *src, unsigned char *dst, unsigned int size)
{
    GxTfmCrypto param;

    memset(&param, 0, sizeof(param));
    param.module        = TFM_MOD_M2M;
    param.alg           = TFM_ALG_AES128;
    param.opt           = TFM_OPT_ECB;
    param.src.id        = TFM_SRC_MEM;
    param.dst.id        = TFM_DST_MEM;
    param.input.buf     = src;
    param.input.length  = size;
    param.output.buf    = dst;
    param.output.length = size;
    param.even_key.id   = TFM_KEY_SSUK;
    param.odd_key.id    = TFM_KEY_SSUK;
    param.flags         = TFM_FLAG_CRYPT_EVEN_KEY_VALID | TFM_FLAG_CRYPT_ODD_KEY_VALID |
                          TFM_FLAG_CRYPT_TS_PACKET_MODE | TFM_FLAG_CRYPT_SWITCH_CLR |
                          TFM_FLAG_CRYPT_HW_PVR_MODE;
    param.output_paddr  = (unsigned int)dst;

    return GxTfm_Decrypt(&param);
}

static int _rist_count_sc(const unsigned char *b, int len)
{
    int i, c = 0;
    for (i = 0; i + 2 < len; i++)
        if (b[i] == 0 && b[i + 1] == 0 && b[i + 2] == 1)
            c++;
    return c;
}

/* --------------------------------------------------------------- PSI build */
static void _rist_build_psi(uint16_t prog_id)
{
    AppTsData ts = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    char *pat = NULL, *pmt = NULL;
    int dlen, i;

    s_rist.psi_len = 0;
    s_rist.pmt_pid = 0;
    for (i = 0; i < RIST_PSI_PIDS; i++) { s_rist.cc_pid[i] = -1; s_rist.cc_val[i] = 0; }

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

    node.node_type = NODE_PROG;
    node.id = prog_id;
    if (app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
        s_rist.pmt_pid = node.prog_data.pmt_pid;

    /* Iter 5: append the marker PID's ES entry to the PMT here (+ CRC fix). */
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
static int _rist_resolve_dest(void)
{
    char ip[24];
    int  port, from_file = 0;
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
            if (sscanf(line, " %23[^: \t\r\n]:%d", tip, &tport) == 2 &&
                tport > 0 && tport < 65536) {
                strncpy(ip, tip, sizeof(ip) - 1);
                ip[sizeof(ip) - 1] = '\0';
                port = tport;
                from_file = 1;
            }
        }
        fclose(f);
    }

    if (strcmp(ip, s_rist.dst_ip) == 0 && port == s_rist.dst_port)
        return 0;

    strncpy(s_rist.dst_ip, ip, sizeof(s_rist.dst_ip) - 1);
    s_rist.dst_ip[sizeof(s_rist.dst_ip) - 1] = '\0';
    s_rist.dst_port = port;
    memset(&s_rist.dst, 0, sizeof(s_rist.dst));
    s_rist.dst.sin_family      = AF_INET;
    s_rist.dst.sin_addr.s_addr = inet_addr(ip);
    s_rist.dst.sin_port        = htons((unsigned short)port);

    RIST_LOG("udp: dest = %s:%d  (%s)\n", ip, port,
             from_file ? RIST_CTRL_FILE : "default, no /tmp/ristcap");
    return 1;
}

static int _rist_udp_open(void)
{
    unsigned char ttl = 8;

    s_rist.udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_rist.udp_fd < 0) {
        RIST_LOG("udp: socket() FAILED\n");
        return -1;
    }
    setsockopt(s_rist.udp_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    s_rist.dst_ip[0] = '\0';
    s_rist.dst_port  = 0;
    _rist_resolve_dest();
    return 0;
}

/* --------------------------------------------------------------- volumes */
static int _rist_vol_exists(int vol)
{
    char p[80];
    snprintf(p, sizeof(p), "%s/%04d.ts", RIST_VOL_DIR, vol);
    return (access(p, R_OK) == 0);
}

static FILE *_rist_vol_open(int vol)
{
    char p[80];
    snprintf(p, sizeof(p), "%s/%04d.ts", RIST_VOL_DIR, vol);
    return fopen(p, "rb");
}

static void _rist_vol_unlink(int vol)
{
    char p[80];
    snprintf(p, sizeof(p), "%s/%04d.ts", RIST_VOL_DIR, vol);
    unlink(p);          /* frees the tmpfs RAM immediately once our fd is closed */
}

/* --------------------------------------------------------------- reader */
static void _rist_reader(void *arg)
{
    /* DMA-capable memhole buffers: the M2M cipher DMAs to output_paddr, so an
     * ordinary heap/BSS pointer faults ("sirius_m2m space err"). GxCore_Memhole
     * memory has a valid physical address (same allocator dvb2ip hands the DVR
     * hardware), and is CPU-readable so we can send the decrypted result. */
    unsigned char *din  = NULL;
    unsigned char *dout = NULL;
    uint8_t  *sbuf  = NULL;
    int       sfill = 0;
    FILE     *in    = NULL;
    int       cur   = 0;
    uint64_t  total = 0, since = 0, decfail = 0;
    time_t    last  = time(NULL), last_resolve = last;
    int       first = 1, waited = 0, seen_data = 0;

    din  = (unsigned char *)GxCore_MemholeMalloc(RIST_BLOCK, NULL);
    dout = (unsigned char *)GxCore_MemholeMalloc(RIST_BLOCK, NULL);
    sbuf = (uint8_t *)GxCore_Mallocz(RIST_BLOCK + 2 * RIST_DGRAM);
    if (din == NULL || dout == NULL || sbuf == NULL) {
        RIST_LOG("reader: buffer alloc FAILED (memhole din=%p dout=%p)\n", din, dout);
        goto done;
    }

    while (s_rist.reader_run && !_rist_vol_exists(0)) {
        if ((++waited % 20) == 0)
            RIST_LOG("reader: waiting for %s/0000.ts (%d x100ms)...\n", RIST_VOL_DIR, waited);
        GxCore_ThreadDelay(100);
    }
    if (!s_rist.reader_run)
        goto done;
    in = _rist_vol_open(0);
    if (in == NULL) {
        RIST_LOG("reader: open 0000.ts FAILED\n");
        goto done;
    }
    cur = 0;                  /* read from the START of 0000 (not EOF) */
    RIST_LOG("reader: tailing %s/%04d.ts (decrypt on) -> udp %s:%d\n",
             RIST_VOL_DIR, cur, s_rist.dst_ip, s_rist.dst_port);

    while (s_rist.reader_run) {
        int got = 0, rotated = 0, dret, off;

        /* fill one full DVR block, following growth / volume rotation */
        while (s_rist.reader_run && got < RIST_BLOCK) {
            size_t r = fread(din + got, 1, RIST_BLOCK - got, in);
            if (r > 0) {
                if (!seen_data) {
                    RIST_LOG("reader: first data on vol %04d (r=%d)\n", cur, (int)r);
                    seen_data = 1;
                }
                got += (int)r;
                continue;
            }
            /* fread==0: EOF on cur. If the next volume exists, cur is complete
             * -> free it and rotate (volumes are 48128-aligned in steady state,
             * so a nonzero partial here only ever happens at teardown). */
            if (_rist_vol_exists(cur + 1)) {
                fclose(in);
                _rist_vol_unlink(cur);
                cur++;
                in = _rist_vol_open(cur);
                if (in == NULL) {
                    RIST_LOG("reader: rotate open %04d.ts FAILED -> stop\n", cur);
                    goto done;
                }
                RIST_LOG("reader: rotate -> vol %04d.ts (freed %04d)\n", cur, cur - 1);
                got = 0;
                rotated = 1;
                break;
            }
            clearerr(in);      /* cur is the live volume; wait for it to grow */
            GxCore_ThreadDelay(20);
        }
        if (rotated) continue;
        if (got < RIST_BLOCK) continue;   /* reader stopping */

        /* decrypt this block (same transform stream_dvr applies on read) */
        dret = _rist_dvr_decrypt(din, dout, RIST_BLOCK);

        if (first) {
            int scr = _rist_count_sc(din, RIST_BLOCK);
            int scd = _rist_count_sc(dout, RIST_BLOCK);
            RIST_LOG("DIAG raw: %02x %02x %02x %02x  sync47@0=%d  startcodes=%d\n",
                     din[0], din[1], din[2], din[3], (din[0] == 0x47), scr);
            RIST_LOG("DIAG dec: ret=%d  %02x %02x %02x %02x  sync47@0=%d  startcodes=%d\n",
                     dret, dout[0], dout[1], dout[2], dout[3], (dout[0] == 0x47), scd);
            RIST_LOG("DIAG verdict: %s\n",
                     (dret == 0 && scd > scr + 8)
                         ? "DECRYPT -> CLEAN TS"
                         : "decrypt FAILED (no clean TS) -- NOT sending; check memhole/key/flags");
            first = 0;
        }

        /* only send successfully-decrypted TS; never emit ciphertext */
        if (dret != 0) {
            decfail++;
        } else {
            _rist_psi_send();
            memcpy(sbuf + sfill, dout, RIST_BLOCK);
            sfill += RIST_BLOCK;
            off = 0;
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
            total += RIST_BLOCK;
            since += RIST_BLOCK;
        }

        {
            time_t now = time(NULL);
            if (now != last) {
                RIST_LOG("reader: %llu B/s  total=%llu B  vol=%04d  sent=%llu err=%llu decfail=%llu\n",
                         ULL(since), ULL(total), cur, ULL(s_rist.sent), ULL(s_rist.senderr), ULL(decfail));
                since = 0;
                last  = now;
            }
            if (now - last_resolve >= RIST_RESOLVE_SECS) {
                if (_rist_resolve_dest())
                    RIST_LOG("udp: dest updated -> %s:%d\n", s_rist.dst_ip, s_rist.dst_port);
                last_resolve = now;
            }
        }
    }

done:
    if (in)   fclose(in);
    if (sbuf) GxCore_Free(sbuf);
    if (din)  GxCore_MemholeFree(din);
    if (dout) GxCore_MemholeFree(dout);
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
    RIST_LOG("stop: done, player3 released\n");
}

/* ------------------------------------------------------------- deferred start */
static int _rist_start_cb(void *arg)
{
    AppFrontend_Config fe  = {0};
    char               url[PLAYER_URL_LONG + 1] = {0};

    s_rist.start_timer = NULL;

    if (s_rist.active) {
        RIST_LOG("start: already active, skip\n");
        return 0;
    }
    if (g_AppPvrOps.state != PVR_DUMMY) {
        RIST_LOG("start: PVR active (state=%d) -> skip capture\n", g_AppPvrOps.state);
        return 0;
    }

    /* mount the size-capped tmpfs (idempotent) and clear it for a fresh capture.
     * guard the mount: an unconditional mount stacks a new tmpfs over the old
     * one every zap, wasting RAM and hiding the previous contents. */
    system("mkdir -p " RIST_RD_MOUNT " 2>/dev/null; "
           "grep -q " RIST_RD_MOUNT " /proc/mounts || "
           "mount -t tmpfs -o size=" RIST_RD_SIZE " tmpfs " RIST_RD_MOUNT " 2>/dev/null; "
           "rm -rf " RIST_VOL_DIR " " RIST_DEST " 2>/dev/null");

    if (app_player_open(RIST_PLAYER) != PLAYER_OPEN_OK) {
        RIST_LOG("start: app_player_open(%s) FAILED\n", RIST_PLAYER);
        return 0;
    }
    s_rist.opened = 1;

    {
        PlayerPVRConfig pc = {0};
        GxPlayer_GetPVRConfig(&pc);
        pc.volume_sizemb   = RIST_VOL_SIZEMB;
        pc.volume_maxnum   = RIST_VOL_MAXNUM;
        pc.volume_fullstop = 0;
        /* the DVR refuses to start if free space <= reserve (default 20MB), which
         * is bigger than our whole ramdisk -> keep the reserve tiny (1MB). */
        pc.reserve_sizemb  = RIST_RESERVE_MB;
        GxPlayer_SetPVRConfig(&pc);
    }

    {
        PlayerRecordConfig *cfg = (PlayerRecordConfig *)GxCore_Calloc(1, sizeof(PlayerRecordConfig));
        if (cfg) {
            app_player_record_config_get((int)s_rist.prog.id, cfg);
            RIST_LOG("start: cfg have_pmt=%u userdata_len=%u ext_num=%u\n",
                     cfg->ext_data.have_pmt, cfg->ext_data.userdata_len, cfg->ext_info.ext_num);
            /* Iter 5: append marker_pid to cfg->ext_info.ext_pids/ext_types here */
            GxPlayer_MediaRecordConfig(RIST_PLAYER, cfg);
            GxCore_Free(cfg);
        }
    }

    app_ioctl(s_rist.prog.tuner, FRONTEND_CONFIG_GET, &fe);
    app_player_url_get(url, &s_rist.prog, fe.ts_src, fe.dmx_id);
    RIST_LOG("start: srcurl=%s\n", url);

    {
        status_t rr;
        PlayerStatusInfo si;

        {   /* prove whether the reserve-space check can be the cause of a -1:
             * log the tmpfs free space (MB) vs the reserve we requested (1 MB) */
            struct statvfs vfs;
            if (statvfs(RIST_RD_MOUNT, &vfs) == 0) {
                unsigned long freemb =
                    (unsigned long)((vfs.f_bavail * (unsigned long long)vfs.f_bsize) >> 20);
                RIST_LOG("start: tmpfs free=%luMB reserve_req=%dMB\n", freemb, RIST_RESERVE_MB);
            }
        }
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

    _rist_build_psi((uint16_t)s_rist.prog.id);
    if (_rist_udp_open() < 0) {
        RIST_LOG("start: UDP open FAILED -> aborting\n");
        goto cleanup;
    }

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

    app_rist_capture_stop();

    memcpy(&s_rist.prog, prog, sizeof(GxBusPmDataProg));
    s_rist.marker_pid = RIST_MARKER_PID;
    s_rist.sent = 0;
    s_rist.senderr = 0;

    RIST_LOG("play_change: prog_id=%d ts_id=%d svc_id=%d -> start in %dms\n",
             prog->id, prog->tp_id, prog->service_id, RIST_START_DELAY_MS);

    APP_TIMER_ADD(s_rist.start_timer, _rist_start_cb, RIST_START_DELAY_MS, TIMER_ONCE);
    return 0;
}
