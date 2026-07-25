/*****************************************************************************
 * app_rist_capture.c  --  zap-driven dmx2 clear-TS -> UDP output
 *
 * PATH (this build):
 *   channel zap (app_normal_play) -> app_rist_play_change(prog)
 *     -> deferred _rist_start_cb  (APP_TIMER_ADD; does NOT block the zap thread)
 *        -> app_ts_record_start({prog_id, user_pmt=true})   [dvb2ip capture]
 *           captures the selected program on DEMUX/DVR instance 2 (the
 *           UNPROTECTED instance that yields CLEAR TS -- runtime-selectable via
 *           /tmp/ristdmx, default 2) and injects PAT/PMT.
 *        -> reader thread: app_ts_record_read() -> sendto() 1316-byte
 *           (7 x 188) TS-over-UDP datagrams to a runtime destination
 *           (/tmp/ristcap "ip:port", default below).
 *
 * The dmx2 capture already emits a self-contained CLEAR transport stream
 * (app-injected PAT/PMT + clear video/audio), proven byte-correct on the
 * dvb2ip HTTP path, so we forward those bytes VERBATIM -- no decrypt, no
 * separate PSI injection here.
 *
 * The UDP output is the feed for RIST later (rist_watchdog ./ristsender_marker
 * -i udp://addr:port).
 *
 * SCREEN SWITCH (Step C, default OFF): when /tmp/ristscreen == "1", app_normal_play
 * suppresses the live-tuner decode (app_player_close(PLAYER_FOR_NORMAL) -- frees
 * the single video decoder) and we play the box's OWN loopback udp:// stream on
 * screen via player_av. This proves the full local path (dmx2 -> UDP -> player ->
 * screen) with no RIST and no external test source. Point /tmp/ristcap at
 * 127.0.0.1:<port> for the loopback test. Flag absent/0 => normal TV, untouched.
 *****************************************************************************/

#include "gxcore.h"
#include "app_config.h"                        /* DVB2IP_SERVER_SUPPORT */
#include "app_module.h"
#include "app_send_msg.h"
#include "app.h"
#include "module/pm/gxpm_manage.h"             /* GxBusPmDataProg */
#include "gxplayer.h"                          /* umbrella -> GxPlayer_MediaPlay/MediaStop */
#include "../dvb2ip_server/app_ts_record.h"    /* app_ts_record_* + TsRecConfig (DVB2IP-gated) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if DVB2IP_SERVER_SUPPORT

/* ------------------------------------------------------------------ config */
#define RIST_DGRAM              (188 * 7)      /* 1316: one TS-over-UDP datagram */
#define RIST_READ               (188 * 256)    /* 48128: read chunk from the capture fifo */
#define RIST_START_DELAY_MS     2000           /* let the tuner lock before we tap dmx2 */
#define RIST_RESOLVE_SECS       2              /* re-read /tmp/ristcap this often */
#define RIST_FIFO_SIZE          (20 * 256 * 188)
#define RIST_MAX_PROG           4

#define RIST_CTRL_FILE          "/tmp/ristcap"            /* runtime dest: "ip:port" */
#define RIST_UDP_DEFAULT_IP     "239.6.6.6"
#define RIST_UDP_DEFAULT_PORT   6000

/* Screen switch (Step C): when /tmp/ristscreen == "1", app_normal_play suppresses
 * the live-tuner decode (frees the single video decoder) and we play the box's
 * own loopback udp:// stream on screen via player_av. Default OFF -- normal TV.
 * /tmp/ristscreenurl optionally overrides the whole player URL (so the receive
 * form or low-latency params can be tuned without a reflash). */
#define RIST_SCREEN_FLAG_FILE   "/tmp/ristscreen"
#define RIST_SCREEN_URL_FILE    "/tmp/ristscreenurl"
#define RIST_SCREEN_PLAYER      "player_av"               /* PMP_PLAYER_AV: local media player slot */
#define RIST_SCREEN_DELAY_MS    1500                      /* after capture start, let data flow before player_av probes */

#define RIST_LOG(fmt, ...)      printf("[RIST] " fmt, ##__VA_ARGS__)
#define ULL(x)                  ((unsigned long long)(x))

/* ------------------------------------------------------------------- state */
static struct {
    int                 active;
    volatile int        reader_run;
    handle_t            reader_thread;
    handle_t            rec_handle;      /* from app_ts_record_start (0 = none) */
    event_list         *start_timer;
    event_list         *screen_timer;   /* deferred player_av start */
    int                 screen_started;  /* player_av running on the loopback udp:// */
    GxBusPmDataProg     prog;

    int                 udp_fd;
    struct sockaddr_in  dst;
    char                dst_ip[24];
    int                 dst_port;
    uint64_t            sent;
    uint64_t            senderr;
} s_rist = { 0, 0, -1, 0, NULL, NULL, 0, {0}, -1, {0}, {0}, 0, 0, 0 };

/* ------------------------------------------------------------------- UDP */
/* Resolve the destination from /tmp/ristcap ("ip:port"); fall back to the
 * compiled default. Returns 1 if the destination changed, 0 otherwise. Unicast
 * is the intended use (multicast does not cross an AP-isolated WiFi) -- just put
 * the laptop IP in the file, e.g.  echo 192.168.1.50:6000 > /tmp/ristcap  */
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

/* --------------------------------------------------------------- screen */
/* Runtime switch, queried both here and by app_normal_play's decode suppression:
 * "1" in /tmp/ristscreen => suppress live decode + play the loopback stream on
 * screen. Anything else / absent => normal TV (default OFF). */
int app_rist_screen_enabled(void)
{
    FILE *f = fopen(RIST_SCREEN_FLAG_FILE, "r");
    int on = 0;
    if (f) {
        int c = fgetc(f);
        on = (c == '1');
        fclose(f);
    }
    return on;
}

/* player_av receive URL: default "udp://@:<capture-dest-port>" (listen on the
 * same port the capture sends to on loopback); overridable verbatim via
 * /tmp/ristscreenurl if the receive form / options need tuning without reflash. */
static void _rist_screen_url(char *out, int outsz)
{
    FILE *f = fopen(RIST_SCREEN_URL_FILE, "r");
    if (f) {
        char line[128] = {0};
        if (fgets(line, sizeof(line) - 1, f)) {
            size_t n = strlen(line);
            while (n && (line[n-1] == '\n' || line[n-1] == '\r' || line[n-1] == ' '))
                line[--n] = '\0';
            if (n) {
                strncpy(out, line, outsz - 1);
                out[outsz - 1] = '\0';
                fclose(f);
                return;
            }
        }
        fclose(f);
    }
    snprintf(out, outsz, "udp://@:%d", s_rist.dst_port);
}

static void _rist_screen_stop(void)
{
    if (s_rist.screen_started) {
        RIST_LOG("screen: stopping player_av\n");
        GxPlayer_MediaStop(RIST_SCREEN_PLAYER);
        s_rist.screen_started = 0;
    }
}

/* Deferred so the capture is already producing before player_av probes the
 * loopback socket (an empty port would make avformat_open_input time out). Runs
 * on the timer thread, so the reader keeps pumping datagrams during the probe. */
static int _rist_screen_cb(void *arg)
{
    char url[160];

    (void)arg;
    s_rist.screen_timer = NULL;

    if (!s_rist.active) {
        RIST_LOG("screen: capture not active -> not starting player_av\n");
        return 0;
    }
    if (!app_rist_screen_enabled()) {
        RIST_LOG("screen: /tmp/ristscreen off -> player_av not started\n");
        return 0;
    }
    if (s_rist.screen_started)
        return 0;

    _rist_screen_url(url, sizeof(url));
    RIST_LOG("screen: starting player_av on \"%s\" (decode the box's own loopback stream)\n", url);
    if (GxPlayer_MediaPlay(RIST_SCREEN_PLAYER, url, 0, 0, NULL) != GXCORE_SUCCESS) {
        RIST_LOG("screen: GxPlayer_MediaPlay(player_av) FAILED\n");
        return 0;
    }
    s_rist.screen_started = 1;
    RIST_LOG("screen: player_av STARTED\n");
    return 0;
}

/* --------------------------------------------------------------- reader */
static void _rist_reader(void *arg)
{
    uint8_t  *rbuf = NULL, *sbuf = NULL;
    int       sfill = 0, first = 1;
    uint64_t  total = 0, since = 0;
    time_t    last = time(NULL), last_resolve = last;

    (void)arg;

    rbuf = (uint8_t *)GxCore_Mallocz(RIST_READ);
    sbuf = (uint8_t *)GxCore_Mallocz(RIST_READ + 2 * RIST_DGRAM);
    if (!rbuf || !sbuf) {
        RIST_LOG("reader: buffer alloc FAILED (rbuf=%p sbuf=%p)\n", rbuf, sbuf);
        goto done;
    }

    RIST_LOG("reader: started -- reading dmx2 clear TS via app_ts_record\n");

    while (s_rist.reader_run) {
        int off = 0;
        int n = app_ts_record_read(s_rist.rec_handle, rbuf, RIST_READ);
        if (n <= 0) {
            GxCore_ThreadDelay(10);
            continue;
        }

        /* One-shot classification of the captured stream: at 188 boundaries a
         * sync byte (0x47) on every packet plus 00-00-01 start codes means CLEAR
         * decodable TS. No framing => wrong demux instance (check /tmp/ristdmx). */
        if (first) {
            int i, nsync = 0, npkt = 0, nsc = 0;
            for (i = 0; i + 187 < n; i += 188) { npkt++; if (rbuf[i] == 0x47) nsync++; }
            for (i = 0; i + 2 < n; i++)
                if (rbuf[i] == 0 && rbuf[i + 1] == 0 && rbuf[i + 2] == 1) nsc++;
            RIST_LOG("DIAG first read len=%d  head: %02x %02x %02x %02x  sync47=%d/%d  startcodes=%d  -> %s\n",
                     n, rbuf[0], rbuf[1], rbuf[2], rbuf[3], nsync, npkt, nsc,
                     (npkt && nsync >= npkt && nsc > 0) ? "CLEAR TS (decodable)" :
                     (npkt && nsync >= npkt)            ? "188-framed, no start codes (PSI only so far)" :
                                                          "NOT 188-framed (check dmx2 / /tmp/ristdmx)");
            first = 0;
        }

        /* accumulate then emit whole 1316-byte datagrams; carry the remainder */
        memcpy(sbuf + sfill, rbuf, n);
        sfill += n;
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
        total += n;
        since += n;

        {
            time_t now = time(NULL);
            if (now != last) {
                RIST_LOG("reader: %llu B/s  total=%llu B  sent=%llu err=%llu -> %s:%d\n",
                         ULL(since), ULL(total), ULL(s_rist.sent), ULL(s_rist.senderr),
                         s_rist.dst_ip, s_rist.dst_port);
                since = 0;
                last  = now;
            }
            if (now - last_resolve >= RIST_RESOLVE_SECS) {
                _rist_resolve_dest();          /* dest override without a re-zap */
                last_resolve = now;
            }
        }
    }

done:
    if (rbuf) GxCore_Free(rbuf);
    if (sbuf) GxCore_Free(sbuf);
    RIST_LOG("reader: stopped, total=%llu B  udp sent=%llu err=%llu\n",
             ULL(total), ULL(s_rist.sent), ULL(s_rist.senderr));
}

/* ----------------------------------------------------------------- teardown */
void app_rist_capture_stop(void)
{
    APP_TIMER_REMOVE(s_rist.start_timer);
    APP_TIMER_REMOVE(s_rist.screen_timer);
    _rist_screen_stop();                     /* release the video decoder for the next player */

    if (!s_rist.active && s_rist.reader_thread <= 0 && s_rist.udp_fd < 0 &&
        s_rist.rec_handle == 0)
        return;

    RIST_LOG("stop: tearing down capture\n");

    if (s_rist.reader_run || s_rist.reader_thread > 0) {
        s_rist.reader_run = 0;
        if (s_rist.reader_thread > 0) {
            GxCore_ThreadJoin(s_rist.reader_thread);
            s_rist.reader_thread = -1;
        }
    }
    if (s_rist.rec_handle != 0 && s_rist.rec_handle != (handle_t)-1) {
        app_ts_record_stop(s_rist.rec_handle);   /* releases DVR+slots on instance 2 */
        s_rist.rec_handle = 0;
    }
    if (s_rist.udp_fd >= 0) {
        close(s_rist.udp_fd);
        s_rist.udp_fd = -1;
    }

    s_rist.active = 0;
    RIST_LOG("stop: done (dmx2 capture released)\n");
}

/* ------------------------------------------------------------- deferred start */
static int _rist_start_cb(void *arg)
{
    TsRecConfig cfg;

    (void)arg;
    s_rist.start_timer = NULL;

    if (s_rist.active) {
        RIST_LOG("start: already active, skip\n");
        return 0;
    }

    /* dmx2 capture pipeline. Idempotent: if the dvb2ip HTTP server already
     * inited it, this is a no-op; otherwise it inits and spawns the dumpfilter
     * thread that fills each prog's fifo from DEMUX/DVR instance 2. */
    if (app_ts_record_init(RIST_FIFO_SIZE, RIST_MAX_PROG) < 0) {
        RIST_LOG("start: app_ts_record_init FAILED\n");
        return 0;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.prog_id  = (uint16_t)s_rist.prog.id;
    cfg.user_pmt = true;                 /* inject PAT/PMT -> self-contained TS */

    s_rist.rec_handle = app_ts_record_start(&cfg);
    if (s_rist.rec_handle == 0 || s_rist.rec_handle == (handle_t)-1) {
        RIST_LOG("start: app_ts_record_start(prog=%d) FAILED\n", s_rist.prog.id);
        s_rist.rec_handle = 0;
        return 0;
    }

    if (_rist_udp_open() < 0) {
        RIST_LOG("start: UDP open FAILED -> aborting\n");
        app_ts_record_stop(s_rist.rec_handle);
        s_rist.rec_handle = 0;
        return 0;
    }

    s_rist.reader_run = 1;
    if (GxCore_ThreadCreate("app_rist_reader", &s_rist.reader_thread,
                            _rist_reader, NULL, 64 * 1024,
                            GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS) {
        RIST_LOG("start: reader thread create FAILED\n");
        s_rist.reader_run    = 0;
        s_rist.reader_thread = -1;
        close(s_rist.udp_fd);
        s_rist.udp_fd = -1;
        app_ts_record_stop(s_rist.rec_handle);
        s_rist.rec_handle = 0;
        return 0;
    }

    s_rist.active = 1;
    RIST_LOG("start: capture ACTIVE prog_id=%d ts_id=%d svc_id=%d -> udp %s:%d "
             "(dmx via /tmp/ristdmx, default 2)\n",
             s_rist.prog.id, s_rist.prog.tp_id, s_rist.prog.service_id,
             s_rist.dst_ip, s_rist.dst_port);

    /* Screen switch: if enabled, start player_av on the loopback udp:// a bit
     * later, once the reader is pumping datagrams. Default OFF -> screen keeps
     * showing the live tuner (app_normal_play did not suppress the decode). */
    if (app_rist_screen_enabled()) {
        RIST_LOG("start: /tmp/ristscreen ON -> player_av in %dms on loopback\n", RIST_SCREEN_DELAY_MS);
        APP_TIMER_ADD(s_rist.screen_timer, _rist_screen_cb, RIST_SCREEN_DELAY_MS, TIMER_ONCE);
    }
    return 0;
}

/* --------------------------------------------------------------- entry hook */
/* Called from app_normal_play on every channel change. Non-blocking: it tears
 * down the previous program's capture and arms a short-delay start so the tuner
 * has time to lock before we tap dmx2. */
int app_rist_play_change(GxBusPmDataProg *prog)
{
    if (prog == NULL)
        return -1;

    app_rist_capture_stop();

    memcpy(&s_rist.prog, prog, sizeof(GxBusPmDataProg));
    s_rist.sent    = 0;
    s_rist.senderr = 0;

    RIST_LOG("play_change: prog_id=%d ts_id=%d svc_id=%d -> start in %dms\n",
             prog->id, prog->tp_id, prog->service_id, RIST_START_DELAY_MS);

    APP_TIMER_ADD(s_rist.start_timer, _rist_start_cb, RIST_START_DELAY_MS, TIMER_ONCE);
    return 0;
}

#else  /* !DVB2IP_SERVER_SUPPORT -- app_ts_record is not built; provide stubs */

int  app_rist_play_change(GxBusPmDataProg *prog) { (void)prog; return 0; }
void app_rist_capture_stop(void) { }

#endif /* DVB2IP_SERVER_SUPPORT */
