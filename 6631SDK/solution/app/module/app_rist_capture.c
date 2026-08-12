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
#include "module/app_ioctl.h"                  /* app_ioctl + FRONTEND_LOCK_STATE_GET */
#include "module/app_nim.h"                    /* AppFrontend_LockState */
#include "module/app_rist_api.h"               /* recovery API cache lookup (Step E) */
#include "module/app_rist_stats.h"             /* per-view statistics */
#include "gxplayer.h"                          /* umbrella -> GxPlayer_MediaPlay/MediaStop */
#include "../dvb2ip_server/app_ts_record.h"    /* app_ts_record_* + TsRecConfig (DVB2IP-gated) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if DVB2IP_SERVER_SUPPORT

/* ------------------------------------------------------------------ config */
#define RIST_DGRAM              (188 * 7)      /* 1316: one TS-over-UDP datagram */
#define RIST_READ               (188 * 256)    /* 48128: read chunk from the capture fifo */
#define RIST_START_DELAY_MS     2000           /* default capture-start delay (tuner lock) */
#define RIST_RESOLVE_SECS       2              /* re-read /tmp/ristcap this often */
#define RIST_FIFO_SIZE          (20 * 256 * 188)
#define RIST_MAX_PROG           4

/* Latency tuning (Step: reduce zap-to-picture). Both re-read per zap from /tmp so
 * the floor can be swept without a reflash; milliseconds; default = the old fixed
 * value. /tmp/ristdelay1 = wait before capture starts (tuner lock); acts as the
 * SAFETY-TIMEOUT ceiling under lock-triggered start. /tmp/ristdelay2 = wait before
 * player_av opens (so the reader is already pumping datagrams). */
#define RIST_DELAY1_FILE        "/tmp/ristdelay1"
#define RIST_DELAY2_FILE        "/tmp/ristdelay2"
#define RIST_LOCK_POLL_MS       50             /* lock-poll granularity for capture start */
#define RIST_PROBE_POLL_MS      100            /* player_av "first frame" poll granularity */
#define RIST_PROBE_MAX_MS       20000          /* stop probing after this long */

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

/* player_av URL "-H" options appended to the default receive URL to cut ffmpeg
 * bring-up on our known live TS (consumed in demuxer/demux_lavf.c):
 *   net_stream_live_mode:1 -> AVFMT_FLAG_QUICK_START: early-exit find_stream_info
 *                             once the program map + streams are found (big win).
 *   no_cache_flag:1        -> AVFMT_FLAG_NOBUFFER: drop the udp cache thread and
 *                             probe packet buffering (low-latency live).
 * Space-separated "key:value" after a leading " -H". Override the whole URL live
 * via /tmp/ristscreenurl to sweep these (add ffprobesize:KB, ffanalyzeduration:S,
 * ffnonblock_flag:1, etc.) without a reflash. */
#define RIST_SCREEN_URL_OPTS    " -H net_stream_live_mode:1 no_cache_flag:1"

/* ---------------------------------------------------------- RIST chain (D) */
/* On zap, if the tuned service_id is in the recovery API cache AND the chain
 * kill switch is on, run the full product path:
 *
 *   dmx2 capture -> udp://127.0.0.1:6000
 *     -> rist_watchdog stb_part7_receiver -i udp://@127.0.0.1:6000
 *                                        -u rist://@127.0.0.1:6100?buffer=8000
 *     -> ristreceiver -i "<local sat peer weight=0>,<API recovery peer weight=1000>"
 *                     -o udp://127.0.0.1:6200
 *     -> player_av on udp://@:6200 -> screen
 *
 * A service NOT in the API list leaves the factory path completely alone.
 * /tmp/ristchain defaults OFF so a bad chain is one file away from a working box.
 *
 * NOTE: librist here is OUR modified build (VSF TR-06-4 Part 6 program selection
 * + Part 7 FSR). The binaries are invoked by absolute path from the rootfs; do
 * not substitute an upstream/packaged librist -- FSR would silently never fire. */
#define RIST_CAP_PORT           6000    /* capture -> sender (UDP)      */
#define RIST_LOCAL_PORT         6100    /* sender  -> receiver (RIST)   */
#define RIST_OUT_PORT           6200    /* receiver -> player_av (UDP)  */

#define RIST_CHAIN_FLAG_FILE    "/tmp/ristchain"
#define RIST_DELAY3_FILE        "/tmp/ristdelay3"   /* player delay when the chain is up */
#define RIST_DELAY3_MS          3000                /* receiver buffer needs longer than loopback */

#define RIST_BIN_WATCHDOG       "/usr/bin/rist_watchdog"
/* STB-side Part 7 receiver: validates the headend's markers, counts elementary
 * streams only, rebuilds each block to 35 packets and re-emits as RIST. Named
 * for where it runs and what it does; the legacy ristsender_marker stays
 * installed alongside, so reverting is a one-line change here plus a reflash.
 * Cross-built and staged into the rootfs by install.sh section 6a. */
#define RIST_BIN_SENDER         "/usr/bin/stb_part7_receiver"
#define RIST_BIN_RECEIVER       "/usr/bin/ristreceiver"

#define RIST_PID_WATCHDOG       "/tmp/rist_watchdog.pid"
#define RIST_PID_RECEIVER       "/tmp/rist_receiver.pid"

/* A marker PID must be a real elementary PID: not 0 (PAT) and not the 0x1FFF null PID. */
#define VALID_MARKER_PID(p)     ((p) > 0 && (p) < 0x1FFF)

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
    int                 delay1;          /* capture-start safety-timeout ceiling (ms) */
    unsigned            waited_ms;       /* elapsed lock-poll time this zap */
    GxBusPmDataProg     prog;

    /* Startup timing: every stage is logged as T+<ms> from the zap */
    unsigned            t0_ms;
    event_list         *probe_timer;    /* polls player_av until it reports RUNNING */
    unsigned            probe_t0;        /* when the probe started (real elapsed base) */
    int                 probe_done;      /* probe resolved: got a frame, or gave up */
    const char         *probe_player;    /* which player carries the picture this zap */

    /* Step D: RIST chain for this program */
    int                 chain_active;    /* kill switch ON *and* service has recovery */
    int                 chain_running;   /* children actually spawned */
    AppRistRecovery     rec;             /* API entry for the tuned service */
    pid_t               pid_watchdog;    /* rist_watchdog (owns ristsender_marker) */
    pid_t               pid_receiver;    /* ristreceiver (standalone, not watchdogged) */

    int                 udp_fd;
    struct sockaddr_in  dst;
    char                dst_ip[24];
    int                 dst_port;
    uint64_t            sent;
    uint64_t            senderr;
} s_rist = { 0, 0, -1, 0, NULL, NULL, 0, 0, 0, {0},
             0, NULL, 0, 0, NULL,
             0, 0, {0}, -1, -1,
             -1, {0}, {0}, 0, 0, 0 };

/* Milliseconds since an arbitrary epoch; only differences are used, so the
 * 32-bit wrap (~49 days) is harmless. */
static unsigned _rist_now_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (unsigned)tv.tv_sec * 1000u + (unsigned)(tv.tv_usec / 1000);
}

/* Stage marker: every chain-startup step logs its offset from the zap, so the
 * real breakdown is readable on serial instead of inferred. librist's own lines
 * (peer authenticated / FSR enabled / recovery data flowing) interleave with
 * these from the child processes and can be correlated against them. */
#define RIST_T(fmt, ...) \
    RIST_LOG("T+%-5u " fmt, _rist_now_ms() - s_rist.t0_ms, ##__VA_ARGS__)

static int s_swept = 0;     /* stale-child pidfile sweep done once per app run */

/* Read a non-negative integer from a /tmp control file; return defval if the
 * file is absent/unparseable. Used for the per-zap latency knobs. */
static int _rist_read_int_file(const char *path, int defval)
{
    FILE *f = fopen(path, "r");
    int v = defval, t = 0;
    if (f) {
        if (fscanf(f, "%d", &t) == 1 && t >= 0)
            v = t;
        fclose(f);
    }
    return v;
}

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

    /* Chain up: the capture MUST feed ristsender_marker on loopback, so the
     * destination is fixed in code and /tmp/ristcap is ignored (the reader
     * re-resolves every 2s and would otherwise drag it back to the file value). */
    if (s_rist.chain_active) {
        strncpy(ip, "127.0.0.1", sizeof(ip) - 1);
        ip[sizeof(ip) - 1] = '\0';
        port = RIST_CAP_PORT;
        goto apply;
    }

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

apply:
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
             s_rist.chain_active ? "RIST chain input (fixed)" :
             from_file           ? RIST_CTRL_FILE : "default, no /tmp/ristcap");
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

/* ----------------------------------------------------------- RIST chain */
/* RIST is the normal behaviour now that it is proven, so the chain is ON unless
 * explicitly disabled:
 *   file absent -> ENABLED (default)
 *   "0"         -> DISABLED (escape hatch: factory path everywhere)
 *   "1"         -> ENABLED (accepted so existing habits keep working)
 * Enabling by default is safe because every failure downstream already falls
 * back: no API response, an empty list, or a service not in the list all take
 * the factory tuner path, so a box that cannot reach the API behaves exactly
 * like a stock box. *why is passed back for the per-zap log line. */
static int _rist_chain_flag(const char **why)
{
    FILE *f = fopen(RIST_CHAIN_FLAG_FILE, "r");
    int on = 1;
    const char *reason = "default, no " RIST_CHAIN_FLAG_FILE;

    if (f) {
        int c = fgetc(f);
        fclose(f);
        if (c == '0') {
            on     = 0;
            reason = RIST_CHAIN_FLAG_FILE "=0, forced off";
        } else if (c == '1') {
            reason = RIST_CHAIN_FLAG_FILE "=1, forced on";
        } else {
            reason = RIST_CHAIN_FLAG_FILE " unreadable, using default";
        }
    }

    if (why)
        *why = reason;
    return on;
}

/* Kill a pid recorded in a pidfile, but only if it really is one of ours --
 * check /proc/<pid>/cmdline for the expected binary so a recycled pid belonging
 * to an unrelated process is never killed. Used to sweep survivors of an app
 * restart (where PR_SET_PDEATHSIG did not get the chance to fire). */
static void _rist_pid_sweep(const char *pidfile, const char *binary)
{
    FILE *f = fopen(pidfile, "r");
    int pid = 0;

    if (!f)
        return;

    if (fscanf(f, "%d", &pid) == 1 && pid > 1) {
        char path[64], cmd[256] = {0};
        FILE *cf;
        int n = 0;

        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        cf = fopen(path, "r");
        if (cf) {
            n = (int)fread(cmd, 1, sizeof(cmd) - 1, cf);
            fclose(cf);
        }
        /* cmdline is NUL-separated; argv[0] is enough to identify it. */
        if (n > 0 && strstr(cmd, binary) != NULL) {
            kill(pid, SIGKILL);
            RIST_LOG("chain: swept stale %s pid=%d\n", binary, pid);
        }
    }

    fclose(f);
    unlink(pidfile);
}

static pid_t _rist_spawn(char *const argv[], const char *pidfile, const char *label)
{
    pid_t pid;
    int i;

    RIST_LOG("chain: exec %s", label);
    for (i = 0; argv[i]; i++)
        printf(" %s", argv[i]);
    printf("\n");

    pid = fork();
    if (pid < 0) {
        RIST_LOG("chain: fork FAILED for %s\n", label);
        return -1;
    }

    if (pid == 0) {
        /* Child: keep this async-signal-safe -- no printf, no malloc.
         * PDEATHSIG makes the kernel kill us if the app dies, so a crashed or
         * restarted app can never leave these running against a stale chain. */
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        if (getppid() == 1)
            _exit(127);                 /* parent died between fork and prctl */
        execv(argv[0], argv);
        _exit(127);                     /* execv only returns on failure */
    }

    if (pidfile) {
        FILE *f = fopen(pidfile, "w");
        if (f) {
            fprintf(f, "%d\n", (int)pid);
            fclose(f);
        }
    }
    RIST_LOG("chain: started %s pid=%d\n", label, (int)pid);
    return pid;
}

static void _rist_reap(pid_t *pid, const char *pidfile, const char *label)
{
    int i, st = 0;

    if (pid && *pid > 0) {
        kill(*pid, SIGTERM);
        for (i = 0; i < 20; i++) {              /* up to ~1s for a clean exit */
            if (waitpid(*pid, &st, WNOHANG) == *pid) {
                *pid = -1;
                goto done;
            }
            GxCore_ThreadDelay(50);
        }
        kill(*pid, SIGKILL);
        waitpid(*pid, &st, 0);
        *pid = -1;
done:
        RIST_LOG("chain: stopped %s\n", label);
    }

    if (pidfile)
        unlink(pidfile);
}

static void _rist_chain_stop(void)
{
    if (!s_rist.chain_running &&
        s_rist.pid_watchdog <= 0 && s_rist.pid_receiver <= 0)
        return;

    /* Receiver first: it is the consumer, so stopping it first avoids a burst of
     * "peer gone" churn in the sender during teardown. */
    _rist_reap(&s_rist.pid_receiver, RIST_PID_RECEIVER, "ristreceiver");
    _rist_reap(&s_rist.pid_watchdog, RIST_PID_WATCHDOG, "rist_watchdog(+sender)");
    s_rist.chain_running = 0;
}

static int _rist_chain_start(void)
{
    static char in_url[96], out_url[96], recv_in[512], recv_out[96];
    char *wd_argv[8];
    char *rx_argv[6];

    /* sender: UDP in from our capture, RIST out listening for the local receiver */
    snprintf(in_url,  sizeof(in_url),  "udp://@127.0.0.1:%d", RIST_CAP_PORT);
    snprintf(out_url, sizeof(out_url), "rist://@127.0.0.1:%d?buffer=8000", RIST_LOCAL_PORT);

    /* receiver: two peers, comma separated (stock tools/ristreceiver splits on ',').
     * The local satellite peer is weight=0; the API recovery URL MUST carry
     * weight=1000 or librist classifies it as a second satellite peer and FSR can
     * never activate. The API returns no weight, so append it here.
     *
     * timing-mode=1 is RIST_TIMING_MODE_ARRIVAL, and it is REQUIRED ON BOTH
     * PEERS. The two peers are fed by two independent senders on two different
     * machines -- our stb_part7_receiver here, and the headend's ristsender --
     * yet they deliberately share one flow so FSR can substitute one for the
     * other. librist's default (RIST_TIMING_MODE_SOURCE) derives source_time
     * from each sender's RTP timestamp, so one flow ends up carrying two
     * unrelated CLOCK_MONOTONIC epochs. It calibrates f->time_offset once from
     * whichever peer arrives first, then classifies the other peer's packets as
     * out-of-order -- which suppresses receiver_mark_missing() AND freezes
     * last_seq_found, so NACKs stop entirely. Observed on hardware as
     * reordered=~180/s (every packet from peer 1) with missing=0 retries=0
     * while lost climbed. In ARRIVAL mode the receiver discards the sender's
     * timestamp and stamps arrival locally, so both peers share one clock.
     *
     * Setting it on only one peer would be worse than neither: local-arrival
     * time on one and headend-derived time on the other guarantees the mismatch
     * instead of merely risking it. */
    {
        int need = snprintf(recv_in, sizeof(recv_in),
                 "rist://127.0.0.1:%d?weight=0&buffer=8000&timing-mode=1,%s%sweight=1000&timing-mode=1",
                 RIST_LOCAL_PORT,
                 s_rist.rec.rist_url,
                 strchr(s_rist.rec.rist_url, '?') ? "&" : "?");
        /* A clipped URL would silently lose the trailing weight/timing-mode and
         * present as "FSR never activates" or "every packet reordered" -- two
         * bugs we have already spent runs on. Fail loudly instead. */
        if (need < 0 || (size_t)need >= sizeof(recv_in)) {
            RIST_LOG("chain: recovery URL too long (%d >= %d) -- NOT starting the chain\n",
                     need, (int)sizeof(recv_in));
            return -1;
        }
    }
    snprintf(recv_out, sizeof(recv_out), "udp://127.0.0.1:%d", RIST_OUT_PORT);

    RIST_LOG("chain: ports cap=%d local=%d out=%d  (svc_id=%d \"%s\")\n",
             RIST_CAP_PORT, RIST_LOCAL_PORT, RIST_OUT_PORT,
             s_rist.rec.service_id, s_rist.rec.name);

    /* execv, not system(): the URLs contain '&' and '?' which a shell would
     * mangle; as argv elements they need no quoting at all. */
    wd_argv[0] = (char *)RIST_BIN_WATCHDOG;
    wd_argv[1] = (char *)RIST_BIN_SENDER;
    wd_argv[2] = (char *)"-i";
    wd_argv[3] = in_url;
    wd_argv[4] = (char *)"-u";
    wd_argv[5] = out_url;
    wd_argv[6] = NULL;

    rx_argv[0] = (char *)RIST_BIN_RECEIVER;
    rx_argv[1] = (char *)"-i";
    rx_argv[2] = recv_in;
    rx_argv[3] = (char *)"-o";
    rx_argv[4] = recv_out;
    rx_argv[5] = NULL;

    RIST_T("chain: spawning children\n");
    s_rist.pid_watchdog = _rist_spawn(wd_argv, RIST_PID_WATCHDOG, "rist_watchdog(+sender)");
    s_rist.pid_receiver = _rist_spawn(rx_argv, RIST_PID_RECEIVER, "ristreceiver");
    RIST_T("chain: children spawned (watchdog=%d receiver=%d)\n",
           (int)s_rist.pid_watchdog, (int)s_rist.pid_receiver);

    if (s_rist.pid_watchdog <= 0 || s_rist.pid_receiver <= 0) {
        RIST_LOG("chain: start FAILED -> tearing down (screen falls back on next zap)\n");
        _rist_chain_stop();
        return -1;
    }

    s_rist.chain_running = 1;
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

    /* Two independent reasons to take the screen off the live tuner:
     *   /tmp/ristscreen  -> Step C loopback test, any channel;
     *   chain_active     -> Step D, THIS service has recovery and the chain is on.
     * chain_active is computed in app_rist_play_change(), which runs before the
     * decode gate in app_normal_play, so it is already correct when this is
     * called for the suppression decision. A service with no recovery entry
     * leaves chain_active 0 -> factory decode, untouched. */
    return on || s_rist.chain_active;
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
    /* Chain up -> decode the RECEIVER's corrected output, not the raw capture. */
    snprintf(out, outsz, "udp://@:%d%s",
             s_rist.chain_active ? RIST_OUT_PORT : s_rist.dst_port,
             RIST_SCREEN_URL_OPTS);
}

/* "Is RIST genuinely putting a picture on screen right now?"
 *
 * Used by the full-screen arbiter to decide whether an UNLOCKED tuner really
 * means "no signal". In the FSR case the tuner is legitimately unlocked while
 * the recovery path carries the service, so the no-signal tip would be wrong.
 *
 * Deliberately gated on the player actually RUNNING (i.e. decoding the
 * receiver's output), not merely on chain_active: if the chain is up but the
 * recovery peer is unreachable or silent, this returns 0 and the user still
 * gets the honest no-signal indication instead of an unexplained black screen. */
int app_rist_screen_delivering(void)
{
    PlayerStatusInfo si;

    if (!s_rist.chain_active || !s_rist.screen_started)
        return 0;

    memset(&si, 0, sizeof(si));
    if (GxPlayer_MediaGetStatus(RIST_SCREEN_PLAYER, &si) != GXCORE_SUCCESS)
        return 0;

    return (si.status == PLAYER_STATUS_PLAY_RUNNING);
}

/* "Is a RIST channel still coming up?"
 *
 * True from the moment a chain zap starts until the picture appears or the
 * first-frame probe gives up (RIST_PROBE_MAX_MS). The full-screen arbiter shows
 * a neutral "Waiting..." while this is true instead of "No signal", which would
 * otherwise be displayed for the whole ~10s startup even though nothing is wrong.
 *
 * Bounded on purpose: once the probe resolves, this goes false and the real
 * no-signal/error path takes over, so a genuinely failed chain is never masked
 * behind an indefinite "Waiting...". */
int app_rist_screen_connecting(void)
{
    if (!s_rist.chain_active || s_rist.probe_done)
        return 0;

    return !app_rist_screen_delivering();
}

static void _rist_screen_stop(void)
{
    if (s_rist.screen_started) {
        RIST_LOG("screen: stopping player_av\n");
        GxPlayer_MediaStop(RIST_SCREEN_PLAYER);
        s_rist.screen_started = 0;
    }
}

/* Poll player_av after start until it reports RUNNING, logging when the picture
 * actually appears. This is measurement only -- it changes no behaviour. */
static int _rist_probe_cb(void *arg)
{
    PlayerStatusInfo si;

    (void)arg;
    s_rist.probe_timer = NULL;

    if (!s_rist.probe_player)
        return 0;

    memset(&si, 0, sizeof(si));
    if (GxPlayer_MediaGetStatus(s_rist.probe_player, &si) == GXCORE_SUCCESS &&
        si.status == PLAYER_STATUS_PLAY_RUNNING) {
        /* Measure real elapsed time, not accumulated poll ticks: each timer
         * fires at >= the requested period, so counting ticks under-reported
         * the wait (the reason the old figure disagreed with the T+ base). */
        RIST_T("%s RUNNING -- FIRST FRAME (%ums after the play call)\n",
               s_rist.probe_player, _rist_now_ms() - s_rist.probe_t0);
        app_rist_stats_first_frame(_rist_now_ms() - s_rist.t0_ms);
        s_rist.probe_done = 1;
        return 0;
    }

    if ((_rist_now_ms() - s_rist.probe_t0) >= RIST_PROBE_MAX_MS) {
        RIST_T("%s still not RUNNING after %ums (status=%d) -- giving up\n",
               s_rist.probe_player, _rist_now_ms() - s_rist.probe_t0, (int)si.status);
        s_rist.probe_done = 1;      /* stop showing "Waiting..." -> real error state */
        return 0;
    }

    APP_TIMER_ADD(s_rist.probe_timer, _rist_probe_cb, RIST_PROBE_POLL_MS, TIMER_ONCE);
    return 0;
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
    RIST_T("screen: starting player_av on \"%s\"\n", url);
    if (GxPlayer_MediaPlay(RIST_SCREEN_PLAYER, url, 0, 0, NULL) != GXCORE_SUCCESS) {
        RIST_LOG("screen: GxPlayer_MediaPlay(player_av) FAILED\n");
        return 0;
    }
    s_rist.screen_started = 1;
    RIST_T("screen: player_av STARTED (call returned)\n");

    /* Poll until the player reports RUNNING, i.e. it has actually decoded the
     * stream -- that is the "first frame" edge, and the tail of the startup
     * budget that neither delay2/delay3 nor the librist logs expose. */
    s_rist.probe_t0     = _rist_now_ms();
    s_rist.probe_done   = 0;
    s_rist.probe_player = RIST_SCREEN_PLAYER;
    APP_TIMER_ADD(s_rist.probe_timer, _rist_probe_cb, RIST_PROBE_POLL_MS, TIMER_ONCE);
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
    APP_TIMER_REMOVE(s_rist.probe_timer);
    _rist_screen_stop();                     /* release the video decoder for the next player */
    _rist_chain_stop();                      /* SIGTERM -> wait -> SIGKILL, both children */

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
/* The actual capture bring-up, invoked once the tuner is confirmed locked (or the
 * delay1 ceiling is hit). Leaves s_rist.active = 1 on success. */
static void _rist_capture_begin(void)
{
    TsRecConfig cfg;

    /* dmx2 capture pipeline. Idempotent: if the dvb2ip HTTP server already
     * inited it, this is a no-op; otherwise it inits and spawns the dumpfilter
     * thread that fills each prog's fifo from DEMUX/DVR instance 2. */
    if (app_ts_record_init(RIST_FIFO_SIZE, RIST_MAX_PROG) < 0) {
        RIST_LOG("start: app_ts_record_init FAILED\n");
        return;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.prog_id  = (uint16_t)s_rist.prog.id;
    cfg.user_pmt = true;                 /* inject PAT/PMT -> self-contained TS */

    /* Step F: the API-supplied marker PID. Without it ristsender_marker never
     * sees a marker, so rist_start() (only reached inside "if (!first_marker_seen)")
     * is never called, the satellite peer never materialises, and the receiver
     * sits in recovery-only FSR.
     *
     * Taken from the API rather than the broadcast PMT deliberately: the uplink
     * declares the marker as stream_type 0x05, which the box's PMT parser
     * (_app_pmt_get_si_info) rejects as STREAM_UNKNOWN_TYPE and skips -- hence
     * "can't support stream type!!type = 5" on tune. The API value is
     * authoritative and the mux does not remap it. */
    if (s_rist.chain_active && VALID_MARKER_PID(s_rist.rec.marker_pid)) {
        cfg.ext_info.ext_pids[0] = (uint32_t)s_rist.rec.marker_pid;
        cfg.ext_info.ext_num     = 1;
        RIST_T("capture: marker pid %d (0x%04X) from the API -> ext_pids\n",
               s_rist.rec.marker_pid, s_rist.rec.marker_pid);
    } else if (s_rist.chain_active) {
        RIST_LOG("capture: no usable marker_pid from the API (%d) -- sender will "
                 "wait for a first marker that never comes\n", s_rist.rec.marker_pid);
    }

    s_rist.rec_handle = app_ts_record_start(&cfg);
    if (s_rist.rec_handle == 0 || s_rist.rec_handle == (handle_t)-1) {
        RIST_LOG("start: app_ts_record_start(prog=%d) FAILED\n", s_rist.prog.id);
        s_rist.rec_handle = 0;
        return;
    }

    if (_rist_udp_open() < 0) {
        RIST_LOG("start: UDP open FAILED -> aborting\n");
        app_ts_record_stop(s_rist.rec_handle);
        s_rist.rec_handle = 0;
        return;
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
        return;
    }

    s_rist.active = 1;
    RIST_T("capture ACTIVE prog_id=%d ts_id=%d svc_id=%d -> udp %s:%d\n",
           s_rist.prog.id, s_rist.prog.tp_id, s_rist.prog.service_id,
           s_rist.dst_ip, s_rist.dst_port);

    /* Step D: bring up the RIST chain now that the capture is producing, so the
     * sender has data waiting the moment it binds. If it fails to start we clear
     * chain_active, which makes the screen fall back to the raw capture rather
     * than waiting on a receiver output that will never come. */
    /* The RIST chain and the screen player are NOT started here: on a chain zap
     * they are already up from T+0 (app_rist_play_change), since the children and
     * the receiver do not depend on the tuner at all. This path only feeds the
     * satellite peer once the tuner locks -- if it never locks, the chain simply
     * stays recovery-only. */
    if (s_rist.chain_active) {
        RIST_T("capture feeding the chain (satellite peer now has a source)\n");
        app_rist_stats_sat_source(1);
        return;
    }

    /* Loopback (Step C) screen switch only: here the player consumes the capture
     * directly, so it must wait for the capture to be producing. */
    if (app_rist_screen_enabled()) {
        int d2 = _rist_read_int_file(RIST_DELAY2_FILE, RIST_SCREEN_DELAY_MS);
        RIST_T("screen ON -> player_av in %dms (delay2/loopback) on udp://@:%d\n",
               d2, s_rist.dst_port);
        APP_TIMER_ADD(s_rist.screen_timer, _rist_screen_cb, d2, TIMER_ONCE);
    }
}

/* Lock-triggered capture start: poll the frontend lock and begin the moment it
 * reports LOCKED, or when the delay1 ceiling is reached (safety timeout). A
 * same-transponder zap never unlocks, so the first poll (~RIST_LOCK_POLL_MS)
 * already reads LOCKED -> near-zero wait; a genuine retune waits for the real
 * lock instead of a blind 2s. Re-arms itself as a fresh TIMER_ONCE (auto-freed). */
static int _rist_start_cb(void *arg)
{
    AppFrontend_LockState lock = FRONTEND_UNLOCK;

    (void)arg;
    s_rist.start_timer = NULL;

    if (s_rist.active)
        return 0;

    app_ioctl(s_rist.prog.tuner, FRONTEND_LOCK_STATE_GET, &lock);

    if (lock != FRONTEND_LOCKED && s_rist.waited_ms < (unsigned)s_rist.delay1) {
        s_rist.waited_ms += RIST_LOCK_POLL_MS;
        APP_TIMER_ADD(s_rist.start_timer, _rist_start_cb, RIST_LOCK_POLL_MS, TIMER_ONCE);
        return 0;
    }

    RIST_T("tuner %s (lock-poll %ums, delay1 ceiling %dms) -> begin capture\n",
           (lock == FRONTEND_LOCKED) ? "LOCKED" : "TIMEOUT(unlocked)",
           s_rist.waited_ms, s_rist.delay1);
    _rist_capture_begin();
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

    /* One-time sweep of children left behind by a previous app instance (a crash
     * or restart, where PR_SET_PDEATHSIG never got the chance to fire). */
    if (!s_swept) {
        s_swept = 1;
        _rist_pid_sweep(RIST_PID_WATCHDOG, "rist_watchdog");
        _rist_pid_sweep(RIST_PID_RECEIVER, "ristreceiver");
    }

    memcpy(&s_rist.prog, prog, sizeof(GxBusPmDataProg));
    s_rist.sent      = 0;
    s_rist.senderr   = 0;
    s_rist.delay1    = _rist_read_int_file(RIST_DELAY1_FILE, RIST_START_DELAY_MS);
    s_rist.waited_ms = 0;
    s_rist.t0_ms     = _rist_now_ms();      /* T0 = the zap */
    s_rist.probe_t0  = 0;
    s_rist.probe_done = 0;
    s_rist.probe_player = NULL;   /* set below per path; NULL = probe inert */

    /* Step D decision, taken here so it is already correct when app_normal_play
     * asks app_rist_screen_enabled() for the suppression decision a few lines
     * later. A miss (no recovery for this service, or the chain switch off)
     * leaves the factory tuner path completely alone. */
    memset(&s_rist.rec, 0, sizeof(s_rist.rec));
    s_rist.chain_active = 0;
    {
        const char *why = NULL;
        int chain_on = _rist_chain_flag(&why);

        RIST_LOG("play_change: chain=%s (%s)\n", chain_on ? "ENABLED" : "DISABLED", why);

        if (chain_on) {
            if (app_rist_api_lookup(prog->service_id, &s_rist.rec) == 0) {
                s_rist.chain_active = 1;
                RIST_T("svc_id=%d IS in the recovery list (\"%s\") -> RIST chain\n",
                       prog->service_id, s_rist.rec.name);
            } else {
                RIST_LOG("play_change: svc_id=%d not in the recovery list (%d cached) -> factory path\n",
                         prog->service_id, app_rist_api_count());
            }
        } else {
            RIST_LOG("play_change: svc_id=%d -> factory path (chain disabled)\n", prog->service_id);
        }
    }

    /* CHAIN FIRST, at T+0, in parallel with the lock poll.
     *
     * ristsender_marker and ristreceiver do not depend on the tuner: the receiver
     * pulls the recovery peer over IP, so waiting up to delay1 for a lock that may
     * never come was pure dead time in exactly the case that matters (no satellite).
     * The capture still waits for lock below and joins later; if lock never comes
     * the chain simply runs recovery-only.
     *
     * Started synchronously (not on a timer) so a spawn failure clears chain_active
     * BEFORE app_normal_play reads app_rist_screen_enabled() a few lines later --
     * otherwise the decode would be suppressed for a chain that never came up. */
    if (s_rist.chain_active) {
        if (_rist_chain_start() < 0) {
            s_rist.chain_active = 0;
            RIST_T("chain start FAILED -> factory decode for this zap\n");
        } else {
            int d3 = _rist_read_int_file(RIST_DELAY3_FILE, RIST_DELAY3_MS);
            RIST_T("screen: player_av in %dms (delay3/chain) on udp://@:%d\n",
                   d3, RIST_OUT_PORT);
            APP_TIMER_ADD(s_rist.screen_timer, _rist_screen_cb, d3, TIMER_ONCE);
        }
    }

    /* Open the view record now that the path for this zap is settled.
     * Name: prefer the API's, fall back to the DVB service name so factory-path
     * views (which have no API entry) are not recorded blank. */
    {
        /* prog_name is a fixed 32-byte field that is not guaranteed to be
         * NUL-terminated, so copy it bounded rather than handing a bare pointer
         * to something that will strlen() it. */
        char dvbname[MAX_PROG_NAME + 1];
        const char *nm;

        memcpy(dvbname, prog->prog_name, MAX_PROG_NAME);
        dvbname[MAX_PROG_NAME] = '\0';

        nm = (s_rist.chain_active && s_rist.rec.name[0]) ? s_rist.rec.name : dvbname;
        app_rist_stats_view_start(prog->service_id, prog->tp_id, nm,
                                  s_rist.chain_active ? RIST_PATH_RIST : RIST_PATH_TUNER);
    }

    /* First-frame probe for the FACTORY path. The chain path arms its own probe
     * once player_av starts; a factory zap decodes on PLAYER_FOR_NORMAL, which
     * the probe never watched -- so every tuner view was recorded as
     * first_frame_ms = -1 (i.e. "never showed a picture") even though it did.
     * Probing the right player makes the failed-view metric meaningful again and
     * gives us normal zap-to-picture timing for free. app_normal_play issues the
     * decode a few lines after this hook returns, so the first poll lands after
     * it has been kicked off. */
    if (!s_rist.chain_active) {
        s_rist.probe_t0     = _rist_now_ms();
        s_rist.probe_done   = 0;
        s_rist.probe_player = PLAYER_FOR_NORMAL;
        APP_TIMER_ADD(s_rist.probe_timer, _rist_probe_cb, RIST_PROBE_POLL_MS, TIMER_ONCE);
    }

    RIST_T("prog_id=%d ts_id=%d svc_id=%d tuner=%d -> capture lock-poll (delay1 ceiling %dms)\n",
           prog->id, prog->tp_id, prog->service_id, prog->tuner, s_rist.delay1);

    /* First lock poll fires soon (RIST_LOCK_POLL_MS); _rist_start_cb re-arms until
     * the frontend reports lock or delay1 elapses, then begins the capture. */
    APP_TIMER_ADD(s_rist.start_timer, _rist_start_cb, RIST_LOCK_POLL_MS, TIMER_ONCE);
    return 0;
}

#else  /* !DVB2IP_SERVER_SUPPORT -- app_ts_record is not built; provide stubs */

int  app_rist_play_change(GxBusPmDataProg *prog) { (void)prog; return 0; }
void app_rist_capture_stop(void) { }

#endif /* DVB2IP_SERVER_SUPPORT */
