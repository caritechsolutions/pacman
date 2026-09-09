/*****************************************************************************
 * app_rist_capture.c  --  zap-driven dmx2 clear-TS -> UDP output
 *
 * PATH (this build):
 *   channel zap (app_normal_play) -> app_rist_play_change(prog)
 *     -> deferred _rist_start_cb  (APP_TIMER_ADD; does NOT block the zap thread)
 *        -> app_ts_record_start({prog_id, user_pmt})   [dvb2ip capture]
 *           captures the selected program on DEMUX/DVR instance 2 (the
 *           UNPROTECTED instance that yields CLEAR TS -- runtime-selectable via
 *           /tmp/ristdmx, default 2).
 *           PART 7: user_pmt=true  -- the box injects its own PAT/PMT.
 *           PART 8: user_pmt=false -- the BROADCAST PSI is captured instead,
 *             because the headend cuts the broadcast stream and the two ends
 *             must number the same bytes. Not a mode; see _rist_capture_begin.
 *        -> reader thread: app_ts_record_read() -> sendto() 1316-byte
 *           (7 x 188) TS-over-UDP datagrams to a runtime destination
 *           (/tmp/ristcap "ip:port", default below).
 *
 * Either way the dmx2 capture emits CLEAR TS, proven byte-correct on the dvb2ip
 * HTTP path, and we forward those bytes VERBATIM -- no decrypt, no separate PSI
 * injection here.
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
#include "module/player/gxmedia_api.h"        /* Part 8 dmx3 tail: memory-fed demux */
#include "module/si/si_filter.h"             /* U1 probe: section filters on dmx3 */
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
#include <errno.h>

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
#define RIST_PCRCUT_FILE        "/tmp/ristpcrcut"   /* 1 = PCR-boundary cutting */

/* ---------------------------------------------------------------- Part 8
 * Step 1: the video path only. Capture -> box cutter -> local RIST receiver ->
 * player_av, on loopback, with NO recovery peer, no NACK, no FSR and no
 * sync-to-server. The question it answers is whether the box can decode its own
 * PCR-cut-and-reassembled stream; alignment with the headend only starts to
 * matter once there is a peer to NACK, and there is not one yet.
 *
 * ITS OWN PORTS. A Part 8 chain must never be able to half-attach to a Part 7
 * one -- same capture port would mean two senders reading one stream, same out
 * port would mean two writers into player_av -- and both are the kind of fault
 * that looks like "the picture is wrong" rather than "the ports collided".
 * Separate hundreds, same pattern.
 *
 * DEFAULT OFF, unlike /tmp/ristchain which defaults ON. A box that has never
 * heard of Part 8 boots to exactly what it does today. */
#define RIST_P8_FLAG_FILE       "/tmp/ristp8"       /* 1 = Part 8 video path */
/* /tmp/ristp8psi is GONE. Broadcast PSI is not a mode -- see the capture block
 * in _rist_capture_begin(). A knob here could only ever select "cut different
 * bytes from the headend", which is never a thing anyone wants. */
/* Bench override for the Part 8 keep-list, as a comma-separated PID list. The
 * production source is the headend's part8_filter_pids -- see the keep-list
 * block in _rist_p8_chain_start() for why theirs is authoritative and this is
 * only for a box with no headend to ask. */
#define RIST_P8_PIDS_FILE       "/tmp/ristp8pids"
#define RIST_P8_CAP_PORT        6300    /* capture -> p8 sender  (UDP)  */
#define RIST_P8_LOCAL_PORT      6400    /* p8 sender -> receiver (RIST) */
#define RIST_P8_OUT_PORT        6500    /* receiver -> player_av (UDP)  */
#define RIST_BIN_P8_SENDER      "/usr/bin/stb_part8_receiver"

/* Tail selector. Absent or "player_av" keeps the Step 1 path, which is proven
 * on hardware; "dmx3" runs the Step 2 reinjection experiment; "dmx0" is the
 * target architecture -- demux 0 fed from the repaired stream full time, with
 * every consumer left exactly where it is.
 *
 * WHY dmx0 IS NOT "JUST SET THE SOURCE AND WALK AWAY". A demux instance is
 * effectively keyed on (dmx_id, ts_source) by the layers above it:
 *
 *   - dmx_sub_system.c:249-262 refuses outright ("dmx sub already open!",
 *     return -1) when a second opener asks for the same dmx_id with a
 *     DIFFERENT ts. Two owners disagreeing about the source is not a race that
 *     resolves; it is an error return that kills whoever asked second.
 *   - si_filter.c's ts_demux_connect() (:213) writes config_demux.source ONLY
 *     on the first open -- s_Demux[id].handle is never zeroed, because the
 *     close inside ts_demux_disconnect() is #if 0'd out (:201-209). So after
 *     boot the SI layer's opinion of the source is frozen and its ts_src
 *     argument is ignored.
 *
 * What DOES write the source on every single play is the player's own DVB
 * access module: dvbsource_normal.c:33-46 reads &tsid: and &dmxid: straight
 * off the play URL and does cfg_dmx.source = tsid unconditionally. And the URL
 * is built from g_AppPlayOps.normal_play.ts_src (app_play_control.c:962), for
 * which app_play_control.c:1089-1098 ALREADY has a "ts_src = 3" branch -- the
 * pdmx one, live on this build (PDMX_SUPPORT 1).
 *
 * So the dmx0 tail does not touch the demux at all. It sets that one variable,
 * lets the player point demux 0 at DEMUX_SDRAM the way the SDK already does
 * for pdmx and timeshift, and feeds the SDRAM side itself. Consumers keep
 * their compiled-in dmx_id and never learn anything changed; slots and filters
 * sit downstream of the source selector, so they follow it. */
/* Cutter on/off. Absent or 1 = PCR-boundary framing (the default and the only
 * shape that can ever align with the headend); 0 = plain greedy-7, for isolating
 * the cutter during a box-only bring-up. */
#define RIST_P8_CUT_FILE        "/tmp/ristp8cut"
#define RIST_P8_TAIL_FILE       "/tmp/ristp8tail"
/* dmx0/dmx1 are the AV path, dmx2 is our capture, TS_REC_DEMUX_MOD_MAX is 4 --
 * so 3 is the one free demux instance on this chip. */
#define RIST_P8_DMX_MODID       3
/* How long the dmx3 tail gets to produce a first frame before it is declared
 * a failure and player_av takes over. Generous: the receiver holds a full
 * buffer (4000ms default) before it emits anything at all. */
#define RIST_P8_TAIL_FIRSTFRAME_MS  12000
#define RIST_P8_TAIL_POLL_MS        500

/* U1 PROBE. Does a MEMORY-FED demux run hardware SECTION FILTERS?
 *
 * This is the single unknown the whole SI/EPG/CAS-off-the-repaired-stream
 * architecture rests on, and nothing in this SDK demonstrates it. Three
 * subsystems ASSUME it -- gxfrontend_net.c feeds demux 0 from SDRAM and is
 * registered as a frontend, app_pdmx.c makes the consumers' demux SDRAM-fed
 * while moving the tuner elsewhere, dvbsource_tscache.c does the same -- but
 * all three are compiled out or config-gated off on this build, so not one of
 * them has ever run here.
 *
 * The probe uses the PLATFORM'S OWN SI filter API rather than a hand-rolled
 * slot+filter, because what has to be proven is not "can I get bytes out of
 * the hardware" -- it is "would app_epg and app_time get their tables". Those
 * go through GxBus_SiFilterCreate()/GxBus_SiFilterRead(), so the probe does
 * too, on the same demux id and with ts_src = DEMUX_SDRAM.
 *
 * The tables are always in the stream now -- the Part 8 capture takes the
 * broadcast PSI unconditionally -- so this needs one knob, not two:
 *
 *   echo 1 > /tmp/ristp8sec
 */
#define RIST_P8_SECPROBE_FILE   "/tmp/ristp8sec"
#define RIST_P8_SECPROBE_MAX    4
#define RIST_P8_SECPROBE_BUF    4096

/* ---- the dmx0 tail ---------------------------------------------------- *
 *
 * SAME MECHANISM AS THE dmx3 TAIL, DIFFERENT INSTANCE. Not a second
 * implementation: GxMediaApi_ModuleOpen3() takes the demux modid as a
 * parameter, so "memory-fed demux 3" and "memory-fed demux 0" are one code
 * path with one number changed. Two implementations of this would drift.
 *
 * Why instance 0 is the one that matters: GxAvdev_OpenModule() is a REFCOUNTED
 * SINGLETON per (type, id) -- gxavdev.c:62-86, `if (-1 == module_handle[type][id])
 * module_handle[type][id] = GxAVOpenModule(...)`, and every later opener gets
 * that same handle back. So every consumer that opens demux 0 by instance
 * number shares one hardware module, and configuring it memory-fed configures
 * it for all of them at once. That shared handle IS the process-wide demux
 * object; it just lives as an array slot in the avdev layer rather than as a
 * named global. */
#define RIST_P8_DMX0_MODID      0
#define RIST_PID_P8_SENDER      "/tmp/rist_p8_sender.pid"
#define RIST_DELAY3_FILE        "/tmp/ristdelay3"   /* player delay when the chain is up */
#define RIST_DELAY3_MS          3000                /* receiver buffer needs longer than loopback */

/* librist receive buffer, milliseconds. This is the single biggest term in
 * zap-to-picture: the receiver holds each packet for the full buffer depth
 * before emitting it, so first frame lands roughly one buffer after the capture
 * starts feeding. Measured on hardware at 8000: capture active T+111, first
 * frame T+8998.
 *
 * 4000 is a measured choice, not a guess. Through an RF-pull switchover the
 * deepest drawdown was avg_buffer_time 8003 -> 5504, i.e. ~2.5s consumed, so
 * 8000 was carrying ~5.5s that never got used. What the remainder does buy is
 * retransmission depth -- that same window recovered 114 packets needing more
 * than four NACK round trips, with the recovery peer's RTT spiking to ~985ms --
 * so this cannot go much lower without turning those recoveries into visible
 * loss. Roughly: ~2.5s switchover gap + ~1-3s of multi-NACK recovery.
 *
 * Overridable per boot for sweeping without a reflash:  echo 3000 > /tmp/ristbuffer
 * Note the value also sets NACK cadence: rist-common.c derives the rtt_min
 * floor as recovery_length_min / max_retries (8000/20 = 400ms was in the log),
 * so halving the buffer halves that floor too. */
#define RIST_BUFFER_FILE        "/tmp/ristbuffer"
#define RIST_BUFFER_MS          4000

/* Part 8's local pair gets its own default. The 4000 above is sized for the
 * Part 7 recovery peer over the public internet -- multi-NACK depth against a
 * ~985ms RTT. Part 8's Step-1/dmx0 hop is 127.0.0.1 to 127.0.0.1: there is no
 * recovery peer, no NACK round trip worth budgeting for, and the buffer is
 * paid for twice over in zap-to-picture, because first frame lands roughly one
 * buffer after the capture starts feeding. /tmp/ristbuffer still overrides. */
#define RIST_P8_BUFFER_MS       2000

/* Send buffer for the capture -> sender hop. 4 MB is ~540ms at 59 Mb/s, which
 * covers a reader burst without the kernel dropping on our behalf. The kernel
 * halves nothing and doubles this, then clamps to net.core.wmem_max -- the
 * achieved value is what gets logged. */
/* 1MB asked, ~135ms at 59 Mb/s. Not 4MB: the kernel doubles what it accepts,
 * and on 54MB of managed RAM an 8MB socket buffer is how the sender got
 * OOM-killed. Two full-rate runs at the clamped 327680 had senderr=0. */
#define RIST_UDP_SNDBUF         (1024 * 1024)

/* Chain report cadence. 5s: often enough that a shedding stage is obvious
 * within a zap, rare enough that a minute of serial stays readable. */
#define RIST_P8_CHAIN_REPORT_MS (5000)

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
    int                 ts_rec_held;     /* we hold a app_ts_record_init() reference */
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

    /* Fallback to factory decode when the chain never delivers a picture.
     * failed_svc_id latches the service we gave up on so the retry cannot loop:
     * falling back replays the channel, which re-enters app_rist_play_change,
     * which would start the chain again and fail again. Cleared as soon as a
     * different service is selected, so zapping away and back retries. */
    int                 screen_failed;   /* gave up on the chain for this zap */
    int                 failed_svc_id;   /* service we fell back on (0 = none) */

    /* Step D: RIST chain for this program */
    int                 chain_active;    /* kill switch ON *and* service has recovery */
    int                 chain_running;   /* children actually spawned */
    int                 p8_active;       /* Part 8 video path (Step 1) for this zap */
    /* A PID keep-list went out on the sender's input URL this zap. The chain
     * report needs to know: with the filter on, (b)->(c) SHRINKS by design --
     * the whole transponder goes into the sender and one service comes out --
     * and the gap check would otherwise call the thing working as intended a
     * fault, every single interval. */
    int                 p8_filtering;

    /* Part 8 Step 2: the dmx3 reinjection tail. Everything here is inert unless
     * /tmp/ristp8tail says dmx3. */
    int                 p8_tail_on;      /* a memory-fed tail is up this zap */
    handle_t            p8_mod;          /* GxMediaApi_ModuleOpen3 handle, 0 = none */
    int                 p8_mod_started;
    int                 p8_tail_fd;      /* UDP socket reading the receiver output */
    int                 p8_tail_run;
    handle_t            p8_tail_thread;
    event_list         *p8_tail_timer;   /* first-frame watchdog */
    /* Four-stage chain report. Separate timer from the first-frame watchdog
     * because that one stops once a frame arrives, and the whole question here
     * is what happens after a minute, not whether it starts. */
    event_list         *p8_chain_timer;
    uint32_t            p8_chain_t0;
    uint32_t            p8_chain_last;
    uint64_t            p8_cr_a, p8_cr_b, p8_cr_c, p8_cr_d;  /* previous sample */
    uint32_t            p8_tail_t0;
    uint64_t            p8_inj_bytes;    /* accepted by GxMediaApi_ModuleInjectData */
    uint64_t            p8_inj_calls;
    uint64_t            p8_inj_busy;     /* returned 0: ES fifos above the gate */
    uint64_t            p8_inj_err;      /* returned <0 */
    uint64_t            p8_rx_bytes;     /* read off the socket */
    int                 p8_saw_bytes;    /* logged the first accepted inject */
    int                 p8_saw_frame;    /* vpts moved / decoder reported RUNNING */

    /* Which demux instance the tail feeds this zap: 3 for the Step 2
     * experiment, 0 for the target architecture. Latched at chain start and
     * never re-read from the knob mid-zap, because the teardown has to release
     * the same instance the setup opened. */
    int                 p8_dmx;
    int                 p8_dmx0_asked;   /* app_normal_play was told ts_src=3 this zap */
    int                 p8_dmx0_want;    /* play_change confirmed the dmx0 tail this zap */

    /* U1 section-filter probe */
    int                 p8_sec_on;
    /* P2: how many sections of each EIT table_id arrived. Indexed
     * table_id - 0x40, so 0x4E -> 14 (now/next) and 0x50..0x5F -> 16..31
     * (schedule). One counter per id is what turns "EPG works" into "now/next
     * works, schedule does/does not". */
    uint32_t            p8_sec_eit_tid[32];
    int16_t             p8_sec_id[RIST_P8_SECPROBE_MAX];
    uint32_t            p8_sec_sections[RIST_P8_SECPROBE_MAX];
    uint32_t            p8_sec_bytes[RIST_P8_SECPROBE_MAX];
    AppRistRecovery     rec;             /* API entry for the tuned service */
    pid_t               pid_watchdog;    /* rist_watchdog (owns ristsender_marker) */
    pid_t               pid_receiver;    /* ristreceiver (standalone, not watchdogged) */

    int                 udp_fd;
    struct sockaddr_in  dst;
    char                dst_ip[24];
    int                 dst_port;
    uint64_t            sent;
    uint64_t            senderr;
}
/*
 * DESIGNATED, not positional. The positional list this replaces had drifted out
 * of step with the struct: its three -1 values were landing on chain_active,
 * chain_running and p8_active rather than on the descriptors they were written
 * for, which left udp_fd initialised to 0 -- so a teardown before any capture
 * had opened would have run close(0) on stdin. Nothing had tripped it, because
 * every path that reaches the teardown has opened the socket first, but adding
 * fields in the middle is exactly how a latent one becomes a live one.
 *
 * Everything not named here is zero-initialised, which is what these want.
 */
s_rist = {
    .reader_thread  = -1,
    .udp_fd         = -1,
    .pid_watchdog   = -1,
    .pid_receiver   = -1,
    .p8_tail_fd     = -1,
    .p8_tail_thread = -1,
    /* Default to the Step 2 instance so a stale read before the tail is
     * selected can never name demux 0 -- the one instance whose teardown would
     * disturb the consumers. */
    .p8_dmx         = RIST_P8_DMX_MODID,
    /* -1 is "no filter". Guarded by p8_sec_on everywhere, but a sentinel array
     * that reads as "filter 0 exists" at boot is the kind of thing that only
     * bites once someone removes the guard. */
    .p8_sec_id      = { -1, -1, -1, -1 },
};

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

/* One line of a knob file into dst, trimmed. Returns 1 if something was read.
 *
 * Used for the Part 8 PID list, where a TRUNCATED read is worse than no read:
 * a short keep-list filters out packets the headend kept, and every repair from
 * then on splices the wrong bytes while the channel still looks alive. So a
 * line that does not fit is refused outright rather than clipped. */
static int _rist_read_str_file(const char *path, char *dst, int dstlen)
{
    FILE *f = fopen(path, "r");
    int  n;

    if (!f)
        return 0;
    dst[0] = '\0';
    if (!fgets(dst, dstlen, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);

    n = (int)strlen(dst);
    /* fgets fills the buffer exactly when the line was too long for it. */
    if (n == dstlen - 1 && dst[n - 1] != '\n') {
        RIST_LOG("knob %s: line longer than %d bytes -- ignoring it rather than "
                 "using a truncated value\n", path, dstlen - 1);
        dst[0] = '\0';
        return 0;
    }
    while (n > 0 && (dst[n - 1] == '\n' || dst[n - 1] == '\r' || dst[n - 1] == ' '))
        dst[--n] = '\0';

    return dst[0] ? 1 : 0;
}

/* Is pid named in this comma/space separated keep-list?
 *
 * Parses the same shapes librist's rist_pcr_cut_set_filter() accepts, so a list
 * this says is fine is one the sender will also accept. Deliberately does NOT
 * validate the whole list: that is the sender's job and it has the one parser
 * both ends of a repair share. This only answers the single question the box
 * needs to ask before spawning anything. */
static int _rist_pids_contains(const char *list, int pid)
{
    const char *p = list;

    if (!list || pid < 0)
        return 0;

    while (*p) {
        char *end;
        long  v;

        while (*p == ',' || *p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;

        v = strtol(p, &end, 0);
        if (end == p)
            return 0;               /* malformed; the sender will refuse it */
        if (v == pid)
            return 1;
        p = end;
    }
    return 0;
}

/* Copy src to dst with any "<key>=..." query parameter removed.
 *
 * Needed because the recovery URL comes from the API already carrying its own
 * buffer= (rist://<host>:<port>?buffer=8000), so the box cannot simply append
 * one: a URL with the parameter twice has no defined precedence in librist's
 * parser. The box owns its own latency budget, so we strip theirs and add ours.
 *
 * This also closes a race. librist sets the flow's buffer from whichever peer
 * CREATES the flow (flow.c: f->recovery_buffer_ticks = p->recovery_buffer_ticks),
 * and the max-across-peers rescaling only runs when recovery_length_min differs
 * from max -- which a single buffer= value never produces. So with the two peers
 * disagreeing, the effective buffer would depend on which one connected first.
 * Rewriting both keeps them identical and the outcome deterministic.
 *
 * Leaves no trailing separator: if every parameter is dropped the '?' goes too,
 * so the caller's usual  strchr(url,'?') ? "&" : "?"  logic stays correct.
 *
 * Returns 0, or -1 if the result would not fit -- in which case dst may hold a
 * partial result, so the caller must overwrite it rather than use it. (Output is
 * never longer than input, so with dst sized like src this cannot actually fire;
 * it is kept as a guard against a future caller passing a smaller buffer.) */
static int _rist_url_drop_param(char *dst, size_t dstsz, const char *src, const char *key)
{
    const char *q = strchr(src, '?');
    size_t keylen = strlen(key);
    size_t n;
    int first = 1;

    if (!q) {                                   /* no query string at all */
        if (strlen(src) >= dstsz) return -1;
        strcpy(dst, src);
        return 0;
    }

    n = (size_t)(q - src);                      /* base, excluding the '?' */
    if (n >= dstsz) return -1;
    memcpy(dst, src, n);
    dst[n] = '\0';

    for (q++; *q; ) {
        const char *amp  = strchr(q, '&');
        size_t      plen = amp ? (size_t)(amp - q) : strlen(q);

        /* Match "<key>=" exactly -- not "<key>" alone, not "<key>foo=". */
        if (!(plen > keylen && strncmp(q, key, keylen) == 0 && q[keylen] == '=')) {
            if (n + 1 + plen + 1 > dstsz) return -1;
            dst[n++] = first ? '?' : '&';
            memcpy(dst + n, q, plen);
            n += plen;
            dst[n] = '\0';
            first = 0;
        }
        if (!amp) break;
        q = amp + 1;
    }
    return 0;
}

/* ------------------------------------------------------------------- UDP */
/* Resolve the destination from /tmp/ristcap ("ip:port"); fall back to the
 * compiled default. Returns 1 if the destination changed, 0 otherwise. Unicast
 * is the intended use (multicast does not cross an AP-isolated WiFi) -- just put
 * the laptop IP in the file, e.g.  echo 192.168.1.50:6000 > /tmp/ristcap  */
/* Which chain owns the ports this zap. Two accessors rather than two copies of
 * the ternary at every call site: getting one of them wrong would cross the two
 * chains' wiring in a way that presents as a picture fault. */
static int _rist_cap_port(void)
{
    return s_rist.p8_active ? RIST_P8_CAP_PORT : RIST_CAP_PORT;
}

static int _rist_out_port(void)
{
    return s_rist.p8_active ? RIST_P8_OUT_PORT : RIST_OUT_PORT;
}

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
        port = _rist_cap_port();
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

    /* SEND BUFFER, and it is not decoration at this rate.
     *
     * Part 7 ran this hop at ~2.5 Mb/s and still lost datagrams on LOOPBACK
     * when the reader burst ahead of the sender. Whole-TP is 59 Mb/s -- 23x the
     * load -- and a loopback UDP drop here is indistinguishable downstream from
     * satellite loss, which is exactly the confusion Part 8 exists to remove.
     *
     * The kernel doubles what it accepts and clamps to net.core.wmem_max, so the
     * ACHIEVED value is logged rather than the asked one: on a box where wmem_max
     * is small this silently becomes a 64KB buffer (~9ms at this rate) and the
     * log is the only place that would say so. */
    {
        int want = RIST_UDP_SNDBUF, got = 0;
        socklen_t gl = sizeof(got);

        /* NO net.core.wmem_max RAISE HERE, AND THAT IS DELIBERATE.
         *
         * A previous version wrote the sysctl so the 4MB request would not be
         * clamped. It worked -- and the granted buffer went from 327680 to
         * 8388608, which is 8MB of kernel socket memory on a box whose OOM dump
         * says managed:54472kB with free:3060kB against min:3072kB. Together
         * with the other buffers raised in the same build it added ~17MB to a
         * box that had 3MB, and the OOM killer took stb_part8_receiver mid-run.
         *
         * The evidence says the clamp was never the problem: two full-rate runs
         * at 327680 (44ms) sent 7.4 MB/s with senderr=0 throughout. Ask for a
         * modest buffer, let the sysctl clamp it, and log what was granted. */
        setsockopt(s_rist.udp_fd, SOL_SOCKET, SO_SNDBUF, &want, sizeof(want));
        if (getsockopt(s_rist.udp_fd, SOL_SOCKET, SO_SNDBUF, &got, &gl) == 0) {
            RIST_LOG("udp: SO_SNDBUF asked %d got %d (%d ms at 59 Mb/s)\n",
                     want, got, got / 7400);
            if (got < 262144)
                RIST_LOG("udp:   under 256KB (%d ms) -- raise net.core.wmem_max "
                         "only if senderr starts climbing; it is a real cost on "
                         "%dMB of RAM\n", got / 7400, 54);
        }
        else
            RIST_LOG("udp: SO_SNDBUF asked %d, achieved value unreadable\n", want);
    }

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
    /* pid_watchdog holds the Part 7 watchdog OR the Part 8 sender -- one slot,
     * one chain at a time -- so the pidfile has to follow the mode or a Part 8
     * run leaves /tmp/rist_p8_sender.pid behind for the next boot's sweep to
     * act on. */
    if (s_rist.p8_active)
        _rist_reap(&s_rist.pid_watchdog, RIST_PID_P8_SENDER, "stb_part8_receiver");
    else
        _rist_reap(&s_rist.pid_watchdog, RIST_PID_WATCHDOG, "rist_watchdog(+sender)");
    s_rist.chain_running = 0;
}

/* ===================================================================== *
 *  Part 8, Step 2: the dmx3 reinjection tail.
 *
 *  THIS IS AN EXPERIMENT AND IT IS BUILT TO REPORT, NOT TO ASSERT. Feeding a
 *  demux instance from memory and having it run its own filter+decode chain has
 *  never been demonstrated on this box. What the SDK does give us is the
 *  mechanism, and it is not ambiguous:
 *
 *    GxMedia_DemuxOpen() with source_type GXMEDIA_SOURCE_TS opens a DVR module
 *    configured src = DVR_INPUT_MEM, dst = DVR_OUTPUT_DMX (gxmedia_demux.c), and
 *    GxMedia_ModuleConfig() forces tsid = -1, which GxMedia_DemuxConfig() turns
 *    into dmx_config.source = DEMUX_SDRAM. So the memory-fed path is wired by
 *    the SDK itself; what is unproven is whether it decodes here.
 *
 *  ONE CORRECTION TO THE PLAN, and it matters: this demux does NOT lock a
 *  service from PSI. GxMedia_DemuxConfig() takes AudPid/VidPid/PcrPid and
 *  allocates slots for exactly those three -- it never reads a PAT or a PMT.
 *  So there is no "it might pick program 0 of 34" risk on this path, and
 *  broadcast PSI passthrough is not required for it to decode: PSI packets we
 *  send simply have no slot and are dropped inside the demux. The PIDs come
 *  from the box's own PMT read, the same record the capture's slot set and the
 *  cutter's PCR PID come from, so all three cannot disagree.
 *
 *  Decoder contention needs nothing new. app_rist_screen_enabled() is already
 *  true whenever chain_active is set, and app_play_control.c already responds by
 *  calling app_player_close(PLAYER_FOR_NORMAL) -- which is exactly what releases
 *  the single video decoder. The dmx3 module then opens video/audio module 0,
 *  the same hardware player_av would have taken.
 * ===================================================================== */

/* Tail selection. Anything unrecognised -- including the file being absent --
 * keeps the proven Step 1 player_av tail, so a typo degrades to the path that
 * works rather than to a black screen. */
enum { RIST_P8_TAIL_PLAYER_AV = 0, RIST_P8_TAIL_DMX3, RIST_P8_TAIL_DMX0 };

static int _rist_p8_tail_mode(void)
{
    FILE *f = fopen(RIST_P8_TAIL_FILE, "r");
    char  buf[16] = {0};
    int   mode = RIST_P8_TAIL_PLAYER_AV;

    if (!f)
        return RIST_P8_TAIL_PLAYER_AV;
    if (fgets(buf, sizeof(buf) - 1, f)) {
        if (strncmp(buf, "dmx3", 4) == 0)
            mode = RIST_P8_TAIL_DMX3;
        else if (strncmp(buf, "dmx0", 4) == 0)
            mode = RIST_P8_TAIL_DMX0;
    }
    fclose(f);
    return mode;
}

static const char *_rist_p8_tail_name(int mode)
{
    switch (mode) {
    case RIST_P8_TAIL_DMX3: return "dmx3";
    case RIST_P8_TAIL_DMX0: return "dmx0";
    default:                return "player_av";
    }
}

static VideoCodecType _rist_p8_vcodec(uint32_t t)
{
    switch (t) {
    case GXBUS_PM_PROG_MPEG:  return VIDEO_CODEC_MPEG12;
    case GXBUS_PM_PROG_AVS:   return VIDEO_CODEC_AVS;
    case GXBUS_PM_PROG_H264:  return VIDEO_CODEC_H264;
    case GXBUS_PM_PROG_H265:  return VIDEO_CODEC_H265;
    case GXBUS_PM_PROG_MPEG4: return VIDEO_CODEC_MPEG4;
    default:                  return VIDEO_CODEC_UNKNOWN;
    }
}

static AudioCodecType _rist_p8_acodec(uint32_t t)
{
    switch (t) {
    case GXBUS_PM_AUDIO_MPEG1:    return AUDIO_CODEC_MPEG1;
    case GXBUS_PM_AUDIO_MPEG2:    return AUDIO_CODEC_MPEG2;
    case GXBUS_PM_AUDIO_AAC_LATM: return AUDIO_CODEC_AAC_LATM;
    case GXBUS_PM_AUDIO_AAC_ADTS: return AUDIO_CODEC_AAC_ADTS;
    case GXBUS_PM_AUDIO_AC3:      return AUDIO_CODEC_AC3;
    case GXBUS_PM_AUDIO_EAC3:     return AUDIO_CODEC_EAC3;
    case GXBUS_PM_AUDIO_DTS:      return AUDIO_CODEC_DTS;
    case GXBUS_PM_AUDIO_DRA:      return AUDIO_CODEC_DRA1;
    default:                      return AUDIO_CODEC_UNKNOWN;
    }
}

/* ------------------------------------------------ U1: section filter probe */
/*
 * The four tables the target architecture needs, and who would consume each.
 * PAT is included because it is the cheapest positive control: if PAT sections
 * arrive and SDT/EIT do not, the filters work and the STREAM is short of tables;
 * if nothing arrives at all, the filters do not fire on a memory-fed demux and
 * the architecture is dead.
 */
static const struct {
    const char *name;
    uint16_t    pid;
    uint8_t     tid;
    uint8_t     tid_mask;
    const char *consumer;
} s_p8_sec_tab[RIST_P8_SECPROBE_MAX] = {
    { "PAT", 0x0000, 0x00, 0xFF, "positive control"        },
    { "SDT", 0x0011, 0x42, 0xFF, "app_sdt / service names" },
    /* EIT p/f actual is 0x4E; SCHEDULE actual is 0x50-0x5F. Mask 0xE0 accepts
     * 0x40-0x5F so BOTH land in this one filter.
     *
     * FULL SCHEDULE EPG IS EXPECTED TO WORK, and nothing in this chain argues
     * otherwise. The RIST buffer is a DELAY LINE, not a lossy cache: every
     * packet that enters leaves 2000ms later, in order. Nothing is evicted for
     * being large or low-rate, sections are never held or reassembled inside
     * it, and app_epg assembles 0x50 off dmx0 exactly as it always did off the
     * tuner -- it was already assembling sections spread over many seconds.
     * Buffer depth governs how much recovery time there is, never which data
     * survives.
     *
     * The breakdown below is INSTRUMENTATION, not a predicted limitation: this
     * box filters 0x4E and 0x50 (gxepg.c:66-76 epg_day=3 cur_tp_only=1 ->
     * gxepg_table.c:169-192 over c_EpgTableId[]), and counting them separately
     * is what turns "EPG works" into an answer rather than an impression. */
    { "EIT", 0x0012, 0x4E, 0xE0, "app_epg (0x4E now/next + 0x50 schedule)" },
    { "TDT", 0x0014, 0x70, 0xFF, "app_time"                },
};

static void _rist_p8_sec_stop(void)
{
    int i;

    if (!s_rist.p8_sec_on)
        return;
    for (i = 0; i < RIST_P8_SECPROBE_MAX; i++) {
        if (s_rist.p8_sec_id[i] >= 0) {
            GxBus_SiFilterStop(s_rist.p8_dmx, s_rist.p8_sec_id[i]);
            GxBus_SiFilterDestroy(s_rist.p8_dmx, s_rist.p8_sec_id[i]);
            s_rist.p8_sec_id[i] = -1;
        }
    }
    s_rist.p8_sec_on = 0;
    RIST_LOG("p8sec: probe filters released\n");
}

static void _rist_p8_sec_start(void)
{
    int i, made = 0;

    if (_rist_read_int_file(RIST_P8_SECPROBE_FILE, 0) != 1)
        return;

    for (i = 0; i < RIST_P8_SECPROBE_MAX; i++) {
        GxSiFilter f;

        s_rist.p8_sec_id[i]       = -1;
        s_rist.p8_sec_sections[i] = 0;
        s_rist.p8_sec_bytes[i]    = 0;
        if (i == 0)
            memset(s_rist.p8_sec_eit_tid, 0, sizeof(s_rist.p8_sec_eit_tid));

        memset(&f, 0, sizeof(f));
        f.pid         = s_p8_sec_tab[i].pid;
        f.match_depth = 1;
        f.eq_or_neq   = EQ_MATCH;
        f.match[0]    = s_p8_sec_tab[i].tid;
        f.mask[0]     = s_p8_sec_tab[i].tid_mask;
        /* CRC off: a table that fails CRC still proves the filter fired, and
         * proving the filter fired is the entire point of this probe. */
        f.crc         = CRC_OFF;
        f.soft_filter = SOFT_OFF;

        /* ts_src = DEMUX_SDRAM (3), on whichever instance the tail took. On
         * the dmx0 tail that is the SAME instance app_epg and app_time filter,
         * so this probe reads the very demux the real consumers read -- which
         * is what makes its per-table counts an answer about them and not just
         * about itself. ts_demux_connect() is a no-op here because si_filter
         * opened demux 0 at boot and never re-writes source (si_filter.c:227,
         * and the close at :201-209 is #if 0'd out). */
        s_rist.p8_sec_id[i] = GxBus_SiFilterCreate(3, s_rist.p8_dmx, &f);
        if (s_rist.p8_sec_id[i] < 0) {
            RIST_LOG("p8sec: %s (pid 0x%04X) FilterCreate FAILED on dmx%d\n",
                     s_p8_sec_tab[i].name, s_p8_sec_tab[i].pid, s_rist.p8_dmx);
            continue;
        }
        if (GxBus_SiFilterStart(s_rist.p8_dmx, s_rist.p8_sec_id[i]) != GXCORE_SUCCESS) {
            RIST_LOG("p8sec: %s FilterStart FAILED\n", s_p8_sec_tab[i].name);
            GxBus_SiFilterDestroy(s_rist.p8_dmx, s_rist.p8_sec_id[i]);
            s_rist.p8_sec_id[i] = -1;
            continue;
        }
        RIST_LOG("p8sec: %s pid 0x%04X tid 0x%02X/0x%02X -> filter %d (%s)\n",
                 s_p8_sec_tab[i].name, s_p8_sec_tab[i].pid, s_p8_sec_tab[i].tid,
                 s_p8_sec_tab[i].tid_mask, s_rist.p8_sec_id[i], s_p8_sec_tab[i].consumer);
        made++;
    }

    if (!made) {
        RIST_LOG("p8sec: NO filters could be created on dmx%d -- U1 answered NO "
                 "at the allocation step\n", s_rist.p8_dmx);
        return;
    }
    s_rist.p8_sec_on = 1;
    RIST_LOG("p8sec: %d/%d filters armed on dmx%d (ts_src=DEMUX_SDRAM). "
             "Sections below or nothing.\n", made, RIST_P8_SECPROBE_MAX, s_rist.p8_dmx);
}

/* Polled, from the tail's existing watchdog. GxBus_SiFilterRead() is a read of
 * the filter's fifo -- a non-zero return IS a section, which is the answer. */
static void _rist_p8_sec_poll(void)
{
    static uint8_t buf[RIST_P8_SECPROBE_BUF];
    int i;

    if (!s_rist.p8_sec_on)
        return;

    for (i = 0; i < RIST_P8_SECPROBE_MAX; i++) {
        size_t n;

        if (s_rist.p8_sec_id[i] < 0)
            continue;
        n = GxBus_SiFilterRead(s_rist.p8_dmx, s_rist.p8_sec_id[i],
                               buf, sizeof(buf));
        if (n == 0)
            continue;

        /* P2 histogram. buf[0] is the section's table_id. */
        if (s_p8_sec_tab[i].pid == 0x0012 && buf[0] >= 0x40 && buf[0] <= 0x5F)
            s_rist.p8_sec_eit_tid[buf[0] - 0x40]++;

        if (s_rist.p8_sec_sections[i] == 0) {
            /* The first section of each table, with its head bytes, so the log
             * shows a real table and not merely a non-zero length. */
            RIST_LOG("p8sec: *** %s SECTION on dmx%d, %u bytes: "
                     "%02X %02X %02X %02X %02X %02X %02X %02X ***\n",
                     s_p8_sec_tab[i].name, s_rist.p8_dmx, (unsigned)n,
                     buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        }
        s_rist.p8_sec_sections[i]++;
        s_rist.p8_sec_bytes[i] += (uint32_t)n;
    }
}

static void _rist_p8_sec_report(uint32_t elapsed_ms)
{
    int i, any = 0;

    if (!s_rist.p8_sec_on)
        return;

    for (i = 0; i < RIST_P8_SECPROBE_MAX; i++) {
        if (s_rist.p8_sec_id[i] < 0)
            continue;
        RIST_LOG("p8sec:   %s pid 0x%04X: %u sections, %u bytes  (%s)\n",
                 s_p8_sec_tab[i].name, s_p8_sec_tab[i].pid,
                 s_rist.p8_sec_sections[i], s_rist.p8_sec_bytes[i],
                 s_p8_sec_tab[i].consumer);
        if (s_rist.p8_sec_sections[i])
            any = 1;

        /* P2, spelled out: which EIT actually made it. "EPG works" is two
         * answers, not one, and the schedule half is the one that can fail on
         * its own. */
        if (s_p8_sec_tab[i].pid == 0x0012) {
            uint32_t pf = s_rist.p8_sec_eit_tid[0x4E - 0x40]
                        + s_rist.p8_sec_eit_tid[0x4F - 0x40];
            uint32_t sch = 0;
            int t;

            for (t = 0x50 - 0x40; t < 32; t++)
                sch += s_rist.p8_sec_eit_tid[t];

            RIST_LOG("p8sec:     EIT breakdown: now/next (0x4E,0x4F) %u "
                     "sections, SCHEDULE (0x50-0x5F) %u sections\n", pf, sch);
            if (pf && !sch)
                RIST_LOG("p8sec:     -> 0x50 absent. NOT a buffer effect -- the "
                         "delay line drops nothing. Look at the dmx2 slot for "
                         "0x0012, or at whether this TP carries schedule EIT.\n");
        }
    }
    RIST_LOG("p8sec: U1 VERDICT at T+%ums: section filters on a memory-fed "
             "demux %s\n", elapsed_ms,
             any ? "DO FIRE -- the SI/CAS architecture is buildable"
                 : "produced NOTHING -- see the note below");
    if (!any) {
        RIST_LOG("p8sec:   Before concluding they cannot: check ts_in/inj_ok "
                 "above are non-zero (bytes reached dmx%d at all), and that\n",
                 s_rist.p8_dmx);
        RIST_LOG("p8sec:   the capture logged its BROADCAST PSI line above "
                 "(pid 0x0000 slotted).\n");
    }
}

/* THE FOUR-STAGE CHAIN REPORT.
 *
 * At 59 Mb/s there are three hops that can shed and they fail identically from
 * the outside -- a picture that breaks up. One line, four counters, so a gap
 * names the stage that dropped it rather than leaving three suspects:
 *
 *   (a) capture out    app_ts_record's own read accounting (DVR -> our buffer)
 *   (b) into sender    datagrams we put on udp/6300
 *   (c) out of receiver bytes read off udp/6500
 *   (d) into dmx0      bytes GxMediaApi_ModuleInjectData accepted
 *
 * (a)->(b) is our reader plus SO_SNDBUF. (b)->(c) is the RIST pair, and a
 * shortfall there is the sender's cutter dropping pre-first-PCR packets (a
 * one-off ~30) or genuine loopback loss. (c)->(d) is demux back-pressure, which
 * shows as inj_busy rather than loss.
 *
 * ONE STAGE BOUNDARY IS NOT A LOSS BOUNDARY. The PID keep-list is applied inside
 * the sender, so (a) and (b) carry the whole transponder and (c) carries one
 * service. (b)->(c) is therefore MEANT to shrink, by roughly the share this
 * service occupies, and the report says so rather than calling it a gap. The
 * reading worth alarming on is the inverse -- a ratio near 100%, meaning the
 * list did not take and the full 59 Mb/s is inside a RIST pair this box does
 * not have the memory to hold.
 *
 * Rates are per interval; totals are cumulative. A stage that is fine for a
 * minute and then sheds is the signature this exists to catch. */
static int _rist_p8_chain_report_cb(void *arg)
{
    uint64_t a_by = 0, a_pk = 0, a_bad = 0, a_cc = 0;
    uint32_t a_pid = 0;
    uint64_t b_by, c_by, d_by;
    uint64_t d_a, d_b, d_c, d_d;      /* per-interval deltas */
    uint32_t now, iv, elapsed;

    (void)arg;
    s_rist.p8_chain_timer = NULL;

    if (!s_rist.p8_active || !s_rist.chain_active)
        return 0;

    now     = _rist_now_ms();
    elapsed = now - s_rist.p8_chain_t0;
    iv      = now - s_rist.p8_chain_last;
    if (iv == 0)
        iv = 1;

    app_ts_record_health_get(&a_by, &a_pk, &a_bad, &a_cc, &a_pid);
    b_by = s_rist.sent * (uint64_t)RIST_DGRAM;
    c_by = s_rist.p8_rx_bytes;
    d_by = s_rist.p8_inj_bytes;

    d_a = a_by - s_rist.p8_cr_a;
    d_b = b_by - s_rist.p8_cr_b;
    d_c = c_by - s_rist.p8_cr_c;
    d_d = d_by - s_rist.p8_cr_d;

    RIST_LOG("p8chain T+%us  (a)cap %llu kb/s  (b)->snd %llu kb/s  "
             "(c)rcv-> %llu kb/s  (d)->dmx%d %llu kb/s\n",
             elapsed / 1000,
             (unsigned long long)((d_a * 8ULL) / iv),
             (unsigned long long)((d_b * 8ULL) / iv),
             (unsigned long long)((d_c * 8ULL) / iv),
             s_rist.p8_dmx,
             (unsigned long long)((d_d * 8ULL) / iv));
    RIST_LOG("p8chain   totals a=%llu b=%llu c=%llu d=%llu  senderr=%llu "
             "inj_busy=%llu inj_err=%llu\n",
             (unsigned long long)a_by, (unsigned long long)b_by,
             (unsigned long long)c_by, (unsigned long long)d_by,
             (unsigned long long)s_rist.senderr,
             (unsigned long long)s_rist.p8_inj_busy,
             (unsigned long long)s_rist.p8_inj_err);
    RIST_LOG("p8chain   capture health: pkts=%llu pids=%u badsync=%llu CC=%llu\n",
             (unsigned long long)a_pk, a_pid,
             (unsigned long long)a_bad, (unsigned long long)a_cc);

    /* Name the stage rather than leaving three numbers to be compared by eye.
     * 2% tolerance: (b) is quantised to whole 1316-byte datagrams and (c)/(d)
     * lag by the receiver buffer, so small steady gaps are expected. */
    /* CUMULATIVE totals must not be compared across stages -- they have
     * different time origins. (a) starts at app_ts_record_start, (b) when the
     * reader thread comes up some hundreds of ms later, and (c)/(d) a further
     * buffer-depth behind. The first run flagged "GAP a->b" at 84.7% with
     * senderr=0: 3.8MB, which is exactly the reader's head start, not loss.
     *
     * Compare the PER-INTERVAL deltas instead. Once every stage is running they
     * share the same interval, and a stage that is genuinely shedding shows a
     * lower RATE, which is the thing worth waking someone for. Only look at all
     * once the chain has been up long enough for the startup skew to have left
     * the window. */
    /* THE FILTER SITS BETWEEN (b) AND (c). (a) and (b) are both the whole
     * transponder -- the app reads it and puts it on udp/6300 unchanged; the
     * keep-list is applied inside the sender, downstream of that. So (b)->(c)
     * shrinking is the filter WORKING, and the ratio is the measurement we
     * wanted: it is the share of this multiplex that this service occupies.
     *
     * The dangerous reading is the opposite one. A ratio near 100% means the
     * keep-list did not take -- and then the whole 59 Mb/s is inside the RIST
     * pair, which this box does not have the memory for. That is worth a line
     * of its own, because the failure it precedes (packets vanishing inside
     * librist, a NACK storm, the OOM killer) surfaces nowhere near the cause. */
    if (s_rist.p8_filtering && elapsed > 3 * RIST_P8_CHAIN_REPORT_MS && d_b > 100000) {
        unsigned pct = (unsigned)((d_c * 100ULL) / d_b);

        RIST_LOG("p8chain   filter: b->c kept %u%% (%llu -> %llu kb/s) -- this "
                 "service's share of the transponder\n", pct,
                 (unsigned long long)((d_b * 8ULL) / iv),
                 (unsigned long long)((d_c * 8ULL) / iv));
        if (pct > 90)
            RIST_LOG("p8chain   *** FILTER NOT TAKING: %u%% of the transponder is "
                     "crossing the RIST pair. Check the sender's '[START] PID "
                     "filter' line -- unfiltered, this box runs out of memory "
                     "before it runs out of bandwidth ***\n", pct);
    }

    if (elapsed > 3 * RIST_P8_CHAIN_REPORT_MS && d_a > 1000000) {
        if (d_b < d_a - d_a / 50 && s_rist.senderr)
            RIST_LOG("p8chain   GAP a->b: the reader/sendto hop is shedding "
                     "(senderr=%llu). Check SO_SNDBUF above.\n",
                     (unsigned long long)s_rist.senderr);
        else if (!s_rist.p8_filtering && d_c && d_c < d_b - d_b / 50)
            RIST_LOG("p8chain   GAP b->c: loss across the RIST pair on loopback.\n");
        else if (d_d && d_d < d_c - d_c / 50)
            RIST_LOG("p8chain   GAP c->d: dmx%d is refusing bytes "
                     "(inj_busy=%llu) -- demux back-pressure.\n",
                     s_rist.p8_dmx, (unsigned long long)s_rist.p8_inj_busy);
    }
    /* Zero is not a gap, it is a dead stage, and it has one overwhelmingly
     * likely cause on this box. */
    if (elapsed > 2 * RIST_P8_CHAIN_REPORT_MS && d_b > 0 && c_by == 0)
        RIST_LOG("p8chain   *** (c) IS ZERO: nothing is coming out of the "
                 "receiver. Check whether the OOM killer took the sender "
                 "(dmesg: 'Killed process ... stb_part8_recei') ***\n");
    if (a_bad)
        RIST_LOG("p8chain   *** badsync %llu at the CAPTURE -- the whole-TP read is "
                 "tearing; everything downstream is downstream of that ***\n",
                 (unsigned long long)a_bad);

    s_rist.p8_cr_a = a_by; s_rist.p8_cr_b = b_by;
    s_rist.p8_cr_c = c_by; s_rist.p8_cr_d = d_by;
    s_rist.p8_chain_last = now;

    APP_TIMER_ADD(s_rist.p8_chain_timer, _rist_p8_chain_report_cb,
                  RIST_P8_CHAIN_REPORT_MS, TIMER_ONCE);
    return 0;
}

static void _rist_p8_tail_stop(void);
static int  _rist_screen_cb(void *arg);   /* defined further down; the tail's
                                           * watchdog hands the screen back to
                                           * player_av when dmx3 does not decode */

/* The reader: receiver output on loopback -> GxMediaApi_ModuleInjectData.
 *
 * InjectData returns the accepted length, 0 when the ES fifos are above 7/8
 * (gxmedia_demux.c backs off there deliberately), or <0 on a real failure.
 * ZERO IS NOT AN ERROR -- treating it as one would turn normal back-pressure
 * into a torn-down chain -- but a run of zeros with nothing decoding is the
 * signature of "the demux accepted nothing", so it is counted separately. */
static void _rist_p8_tail_reader(void *arg)
{
    static uint8_t buf[64 * 1024];

    (void)arg;
    RIST_T("p8tail: reader thread up on udp://@:%d\n", RIST_P8_OUT_PORT);

    while (s_rist.p8_tail_run) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
        fd_set rf;
        int n;

        if (s_rist.p8_tail_fd < 0)
            break;
        FD_ZERO(&rf);
        FD_SET(s_rist.p8_tail_fd, &rf);
        if (select(s_rist.p8_tail_fd + 1, &rf, NULL, NULL, &tv) <= 0)
            continue;

        n = (int)recv(s_rist.p8_tail_fd, buf, sizeof(buf), 0);
        if (n <= 0)
            continue;
        s_rist.p8_rx_bytes += (uint64_t)n;

        if (s_rist.p8_mod_started) {
            GxMediaModuleInject inj;
            int r;

            memset(&inj, 0, sizeof(inj));
            inj.source_type = GXMEDIA_SOURCE_TS;
            inj.buf         = buf;
            inj.len         = n;

            r = GxMediaApi_ModuleInjectData(s_rist.p8_mod, inj);
            s_rist.p8_inj_calls++;
            if (r > 0) {
                s_rist.p8_inj_bytes += (uint64_t)r;
                if (!s_rist.p8_saw_bytes) {
                    s_rist.p8_saw_bytes = 1;
                    RIST_LOG("p8tail: STAGE 4 -- first inject ACCEPTED %d of %d bytes\n", r, n);
                }
            } else if (r == 0) {
                s_rist.p8_inj_busy++;
            } else {
                s_rist.p8_inj_err++;
            }
        }
    }

    RIST_T("p8tail: reader thread exiting\n");
}

/* First-frame watchdog AND the stage report. Runs until it sees a frame or the
 * deadline passes; on the deadline it tears dmx3 down and hands the screen to
 * player_av, so the failure mode of this experiment is the Step 1 picture, not
 * a black screen. */
static int _rist_p8_tail_poll_cb(void *arg)
{
    GxMediaModuleInfo  info;
    GxMediaModuleState st;
    uint32_t elapsed;
    int have_info, have_state;

    (void)arg;
    s_rist.p8_tail_timer = NULL;

    if (!s_rist.p8_mod || !s_rist.p8_tail_run)
        return 0;

    elapsed = _rist_now_ms() - s_rist.p8_tail_t0;

    memset(&info, 0, sizeof(info));
    memset(&st,   0, sizeof(st));
    have_info  = (GxMediaApi_ModuleInfo(s_rist.p8_mod, &info)  == GXCORE_SUCCESS);
    have_state = (GxMediaApi_ModuleState(s_rist.p8_mod, &st)   == GXCORE_SUCCESS);

    /* A moving vpts is the only unambiguous "the decoder consumed a frame"
     * signal available here; the fifo levels say whether the bytes got that
     * far. Both are reported every poll so a stall names its own stage. */
    if (!s_rist.p8_saw_frame && have_info && info.vpts > 0) {
        s_rist.p8_saw_frame = 1;
        RIST_LOG("p8tail: STAGE 6 -- FIRST FRAME at T+%ums (vpts=%lld stc=%lld)\n",
                 elapsed, (long long)info.vpts, (long long)info.stc);
    }

    RIST_LOG("p8tail: T+%ums rx=%llu inj_ok=%llu busy=%llu err=%llu%s%s\n",
             elapsed,
             (unsigned long long)s_rist.p8_rx_bytes,
             (unsigned long long)s_rist.p8_inj_bytes,
             (unsigned long long)s_rist.p8_inj_busy,
             (unsigned long long)s_rist.p8_inj_err,
             have_info  ? "" : "  [ModuleInfo unavailable]",
             have_state ? "" : "  [ModuleState unavailable]");
    if (have_info)
        RIST_LOG("p8tail:   fifo ts_used=%d vid_used=%d aud_used=%d  apts=%lld vpts=%lld\n",
                 info.demux.ts_used_size, info.video.esv_used_size,
                 info.audio.esa_used_size,
                 (long long)info.apts, (long long)info.vpts);
    if (have_state)
        RIST_LOG("p8tail:   dec video state=%d err=%d  audio state=%d err=%d\n",
                 (int)st.video.state, (int)st.video.err_code,
                 (int)st.audio.state, (int)st.audio.err_code);

    _rist_p8_sec_poll();
    _rist_p8_sec_report(elapsed);

    if (s_rist.p8_saw_frame) {
        /* Video is decoding, so the tail's own job is done -- but the section
         * probe is a separate question and has to keep running to answer it.
         * Keep polling on the same timer rather than stopping here. */
        if (s_rist.p8_sec_on) {
            APP_TIMER_ADD(s_rist.p8_tail_timer, _rist_p8_tail_poll_cb,
                          RIST_P8_TAIL_POLL_MS, TIMER_ONCE);
        }
        return 0;
    }

    if (elapsed >= RIST_P8_TAIL_FIRSTFRAME_MS) {
        RIST_LOG("p8tail: NO FIRST FRAME after %ums -- REINJECTION DID NOT DECODE.\n",
                 elapsed);
        RIST_LOG("p8tail:   rx=%llu bytes, injected=%llu, busy=%llu, err=%llu\n",
                 (unsigned long long)s_rist.p8_rx_bytes,
                 (unsigned long long)s_rist.p8_inj_bytes,
                 (unsigned long long)s_rist.p8_inj_busy,
                 (unsigned long long)s_rist.p8_inj_err);
        RIST_LOG("p8tail:   %s\n",
                 (s_rist.p8_rx_bytes == 0)   ? "STAGE 3 FAILED: nothing arrived from the receiver" :
                 (s_rist.p8_inj_bytes == 0)  ? "STAGE 4 FAILED: the demux accepted no bytes" :
                                               "STAGE 5/6 FAILED: bytes went in, no frames came out");
        RIST_LOG("p8tail: falling back to player_av (Step 1 path) for this zap\n");
        _rist_p8_tail_stop();
        /* Hand the screen to the proven tail rather than leaving it black. */
        if (!s_rist.screen_started)
            APP_TIMER_ADD(s_rist.screen_timer, _rist_screen_cb, 100, TIMER_ONCE);
        return 0;
    }

    APP_TIMER_ADD(s_rist.p8_tail_timer, _rist_p8_tail_poll_cb,
                  RIST_P8_TAIL_POLL_MS, TIMER_ONCE);
    return 0;
}

/* Put the consumers' demux back on the tuner.
 *
 * ONLY for the dmx0 tail, and it is not optional there. GxMediaApi_ModuleClose()
 * releases our ES slots and drops the refcount, but it does NOT restore
 * dmx_config.source -- the instance stays DEMUX_SDRAM. On demux 3 that is
 * harmless, nothing else uses it. On demux 0 it is the factory fallback:
 *
 *   - GxFrontend_QueryStatus() (gxfrontend.c:2352) asks the NIM's demux -- 0 --
 *     for GxDemuxPropertyID_TSLockQuery. Left on SDRAM it never reports TS sync,
 *     so the next zap's lock poll times out on the delay1 ceiling instead of
 *     locking, on EVERY channel, Part 8 or not.
 *   - app_sdt/app_pat/TDT would filter a demux nothing feeds.
 *
 * The next factory play would eventually rewrite it (dvbsource_normal.c on the
 * play URL, app_epg_enable via GxDmxSetSource), but "eventually, if the right
 * consumer happens to restart" is not a fallback. Write it back here, at the
 * point we took it, so /tmp/ristp8 off is genuinely untouched behaviour.
 *
 * The handle comes from GxAvdev_OpenModule(), which is a refcounted singleton
 * per (type, id) (gxavdev.c:62-86): this returns the SAME handle every consumer
 * holds, so the config lands on the instance they are using, and the matching
 * close only decrements. */
static void _rist_p8_dmx_restore_tuner(void)
{
    GxDemuxProperty_ConfigDemux cfg;
    AppFrontend_Config          fe = {0};
    handle_t dev, mod;

    if (s_rist.p8_dmx != RIST_P8_DMX0_MODID)
        return;

    app_ioctl(s_rist.prog.tuner, FRONTEND_CONFIG_GET, &fe);

    dev = GxAvdev_CreateDevice(0);
    if (dev < 0) {
        RIST_LOG("p8tail: RESTORE FAILED -- no device; demux %d is still on "
                 "SDRAM and the tuner lock will not report\n", RIST_P8_DMX0_MODID);
        return;
    }
    mod = GxAvdev_OpenModule(dev, GXAV_MOD_DEMUX, RIST_P8_DMX0_MODID);
    if (mod < 0) {
        RIST_LOG("p8tail: RESTORE FAILED -- demux %d would not open; it is still "
                 "on SDRAM\n", RIST_P8_DMX0_MODID);
        GxAvdev_DestroyDevice(dev);
        return;
    }

    /* Same field-for-field shape the NIM writes at FRONTEND_OPEN
     * (app_nim_dvbs.c:405-413), so what we put back is what was there. */
    memset(&cfg, 0, sizeof(cfg));
    cfg.source           = fe.ts_src;
    cfg.ts_select        = FRONTEND;
    cfg.stream_mode      = DEMUX_PARALLEL;
    cfg.time_gate        = 0xf;
    cfg.byt_cnt_err_gate = 0x03;
    cfg.sync_loss_gate   = 0x03;
    cfg.sync_lock_gate   = 0x03;
    if (GxAVSetProperty(dev, mod, GxDemuxPropertyID_Config,
                        &cfg, sizeof(cfg)) < 0)
        RIST_LOG("p8tail: RESTORE FAILED -- demux %d config rejected; it is "
                 "still on SDRAM\n", RIST_P8_DMX0_MODID);
    else
        RIST_LOG("p8tail: demux %d restored to the tuner (source=%u)\n",
                 RIST_P8_DMX0_MODID, (unsigned)fe.ts_src);

    GxAvdev_CloseModule(dev, mod);
    GxAvdev_DestroyDevice(dev);
}

/* Re-point app_epg at whichever source demux 0 is now on.
 *
 * D1, AND IT IS THE ONLY CONSUMER THAT NEEDS THIS. Every other consumer just
 * allocates slots through the shared demux handle and is order-independent:
 * slots and filters sit downstream of the source selector, so they follow it
 * whenever it changes. app_epg does not. app_epg_enable() lands in
 * GxEpg_SubtChannelCreate(), which does an unconditional
 * GxDmxSetSource(epg_cfg.dmx_id, epg_cfg.ts_src) (gxepg_table.c:123) -- and
 * epg_cfg.dmx_id is TS_2_DMX, which is 0 on this build (app_nim.h:28-32;
 * CA_SUPPORT is 0 and SUBTTRANS_SUPPORT is undefined). So EPG WRITES demux 0's
 * source, and left alone it writes the tuner's -- silently undoing the
 * memory-fed tail the next time EPG restarts.
 *
 * Driven from the tail's start and stop rather than from app_normal_play,
 * because ordering is the whole point. app_prepare_for_play() issues its
 * app_epg_enable() BEFORE app_rist_play_change() runs, so anything decided
 * there would be a zap stale -- wrong on the first Part 8 zap and wrong again
 * on the zap that leaves it. Driving it from the tail re-points EPG at exactly
 * the moment demux 0 changes hands, in both directions.
 *
 * The disable is not optional on either side: app_epg_enable() early-returns
 * when s_Switch.flag is already EPG_ON (app_epg.c:157), so without tearing the
 * channel down first the call is a no-op and EPG keeps filtering the source it
 * was created on. */
static void _rist_p8_epg_repoint(uint32_t ts_src, const char *why)
{
    void app_epg_disable(void);
    void app_epg_enable(uint32_t ts_src, uint32_t dmx_id);

    app_epg_disable();
    app_epg_enable(ts_src, TS_2_DMX);
    RIST_LOG("p8tail: app_epg re-pointed at ts_src=%u on demux %d (%s)\n",
             (unsigned)ts_src, TS_2_DMX, why);
}

static void _rist_p8_tail_stop(void)
{
    const int was_dmx0 = (s_rist.p8_dmx == RIST_P8_DMX0_MODID);

    if (s_rist.p8_tail_run) {
        s_rist.p8_tail_run = 0;
        if (s_rist.p8_tail_thread > 0) {
            GxCore_ThreadJoin(s_rist.p8_tail_thread);
            s_rist.p8_tail_thread = -1;
        }
    }
    if (s_rist.p8_tail_fd >= 0) {
        close(s_rist.p8_tail_fd);
        s_rist.p8_tail_fd = -1;
    }
    _rist_p8_sec_stop();
    if (s_rist.p8_mod) {
        if (s_rist.p8_mod_started)
            GxMediaApi_ModuleStop(s_rist.p8_mod, 0);
        GxMediaApi_ModuleClose(s_rist.p8_mod);
        s_rist.p8_mod = 0;
        s_rist.p8_mod_started = 0;
        RIST_LOG("p8tail: dmx%d module closed\n", s_rist.p8_dmx);
        /* Ordered AFTER the module close: closing releases our ES slots, and
         * the source write should be the last thing that touches the instance
         * so nothing we own is still reading through it. */
        _rist_p8_dmx_restore_tuner();
        if (was_dmx0) {
            AppFrontend_Config fe = {0};
            app_ioctl(s_rist.prog.tuner, FRONTEND_CONFIG_GET, &fe);
            _rist_p8_epg_repoint(fe.ts_src, "tail down -- back on the tuner");
        }
    }
    s_rist.p8_tail_on = 0;
    /* Back to the safe default. A stale 0 here would aim a later teardown at
     * the consumers' demux. */
    s_rist.p8_dmx = RIST_P8_DMX_MODID;
}

/* Open, configure and start the memory-fed module, then start the reader.
 * Returns 0 on success. Every failure returns <0 and the caller falls back to
 * player_av -- there is no path here that leaves the screen with no owner. */
static int _rist_p8_tail_start(int demux_modid)
{
    GxMediaModuleMod     mod;
    GxMediaModuleModPara mpara;
    GxMediaModulePara    para;
    struct sockaddr_in   sa;
    int one = 1;

    /* Latched here and used by teardown, so setup and teardown can never name
     * different instances -- releasing the wrong one would take the consumers'
     * demux down with it. */
    s_rist.p8_dmx = demux_modid;

    if (!VALID_MARKER_PID(s_rist.prog.video_pid)) {
        RIST_LOG("p8tail: no video PID in the program record -- not starting dmx%d\n",
                 s_rist.p8_dmx);
        return -1;
    }

    memset(&mod, 0, sizeof(mod));
    /* Our own demux instance. Everything else stays 0: there is ONE hardware
     * video decoder and one audio decoder on this chip, and normal play has
     * already released them (app_player_close(PLAYER_FOR_NORMAL), driven by
     * app_rist_screen_enabled()). Asking for a second decoder id would fail or,
     * worse, half-succeed. */
    mod.demux.demux_modid = s_rist.p8_dmx;

    memset(&mpara, 0, sizeof(mpara));
    mpara.source_type   = GXMEDIA_SOURCE_TS;   /* -> DVR_INPUT_MEM -> DEMUX_SDRAM */
    mpara.protect_mode  = GXMEDIA_OUTPUT_NORMAL;   /* NCN is FTA; no CAS in Step 2 */
    mpara.a_stream_type = GXMEDIA_AVSTREAM_ES;

    s_rist.p8_mod = GxMediaApi_ModuleOpen3(mod, mpara);
    /* The API returns -1 on failure and a cast pointer on success, so test the
     * two failure values rather than <= 0: a user-space pointer with the top bit
     * set would read as negative and a valid module would be thrown away. */
    if (s_rist.p8_mod == 0 || s_rist.p8_mod == (handle_t)-1) {
        RIST_LOG("p8tail: STAGE 1 FAILED -- GxMediaApi_ModuleOpen3(dmx%d) returned %d\n",
                 s_rist.p8_dmx, (int)s_rist.p8_mod);
        s_rist.p8_mod = 0;
        return -1;
    }
    RIST_LOG("p8tail: STAGE 1 -- module open on dmx%d (source=TS -> DEMUX_SDRAM)\n",
             s_rist.p8_dmx);

    memset(&para, 0, sizeof(para));
    para.tsid   = -1;                  /* ModuleConfig forces this anyway */
    para.vidPid = (int)s_rist.prog.video_pid;
    para.audPid = (int)s_rist.prog.cur_audio_pid;
    para.pcrPid = (int)s_rist.prog.pcr_pid;
    para.freq_HZ   = 45000;
    para.sync_mode = 2;
    para.vparam.codectype   = _rist_p8_vcodec(s_rist.prog.video_type);
    para.vparam.decode_mode = 0;
    para.aparam.codectype   = _rist_p8_acodec(s_rist.prog.cur_audio_type);
    para.aparam.stream_data = GXMEDIA_AVSTREAM_ES;

    if (GxMediaApi_ModuleConfig(s_rist.p8_mod, para) != GXCORE_SUCCESS) {
        RIST_LOG("p8tail: STAGE 2 FAILED -- ModuleConfig(v=0x%04X a=0x%04X pcr=0x%04X "
                 "vcodec=0x%X acodec=0x%X)\n",
                 para.vidPid, para.audPid, para.pcrPid,
                 (unsigned)para.vparam.codectype, (unsigned)para.aparam.codectype);
        _rist_p8_tail_stop();
        return -1;
    }
    RIST_LOG("p8tail: STAGE 2 -- configured v=0x%04X a=0x%04X pcr=0x%04X "
             "vcodec=0x%X acodec=0x%X\n",
             para.vidPid, para.audPid, para.pcrPid,
             (unsigned)para.vparam.codectype, (unsigned)para.aparam.codectype);

    if (GxMediaApi_ModuleStart(s_rist.p8_mod) != GXCORE_SUCCESS) {
        RIST_LOG("p8tail: STAGE 3 FAILED -- ModuleStart\n");
        _rist_p8_tail_stop();
        return -1;
    }
    s_rist.p8_mod_started = 1;
    RIST_LOG("p8tail: STAGE 3 -- module started\n");

    s_rist.p8_tail_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_rist.p8_tail_fd < 0) {
        RIST_LOG("p8tail: socket() failed: %s\n", strerror(errno));
        _rist_p8_tail_stop();
        return -1;
    }
    setsockopt(s_rist.p8_tail_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port        = htons(RIST_P8_OUT_PORT);
    if (bind(s_rist.p8_tail_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        RIST_LOG("p8tail: bind 127.0.0.1:%d failed: %s\n",
                 RIST_P8_OUT_PORT, strerror(errno));
        _rist_p8_tail_stop();
        return -1;
    }

    s_rist.p8_rx_bytes  = 0;
    s_rist.p8_inj_bytes = 0;
    s_rist.p8_inj_calls = 0;
    s_rist.p8_inj_busy  = 0;
    s_rist.p8_inj_err   = 0;
    s_rist.p8_saw_bytes = 0;
    s_rist.p8_saw_frame = 0;
    s_rist.p8_tail_run  = 1;
    if (GxCore_ThreadCreate("app_rist_p8tail", &s_rist.p8_tail_thread,
                            _rist_p8_tail_reader, NULL, 64 * 1024,
                            GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS) {
        RIST_LOG("p8tail: reader thread create FAILED\n");
        s_rist.p8_tail_run    = 0;
        s_rist.p8_tail_thread = -1;
        _rist_p8_tail_stop();
        return -1;
    }

    s_rist.p8_tail_on = 1;
    s_rist.p8_tail_t0   = _rist_now_ms();
    _rist_p8_sec_start();          /* U1 probe; inert unless both knobs are set */
    RIST_LOG("p8tail: reading udp://@127.0.0.1:%d -> dmx%d  (first-frame deadline %dms)\n",
             RIST_P8_OUT_PORT, s_rist.p8_dmx, RIST_P8_TAIL_FIRSTFRAME_MS);

    /* Only on the dmx0 tail. On dmx3 the consumers' demux never changed, so
     * rebuilding the EPG channel would cost a rescan for nothing. */
    if (s_rist.p8_dmx == RIST_P8_DMX0_MODID)
        _rist_p8_epg_repoint(3, "demux 0 is now memory-fed");

    APP_TIMER_ADD(s_rist.p8_tail_timer, _rist_p8_tail_poll_cb,
                  RIST_P8_TAIL_POLL_MS, TIMER_ONCE);
    return 0;
}

/* --- what app_play_control asks -----------------------------------------
 *
 * ONE question, asked AFTER app_rist_play_change() has settled the path for
 * this zap: "are the SI consumers to be pointed at the repaired stream?"
 *
 * It cannot be asked before, because the answer depends on the chain having
 * been accepted (the failed_svc_id latch, the Part 8 gate). And it must not be
 * re-derived from the knob file at the call site -- see the latch comment in
 * play_change. */
int app_rist_p8_dmx0_confirmed(void)
{
    return s_rist.p8_dmx0_want;
}

/*
 * Part 8, Step 1: the video path.
 *
 *   dmx2 capture --UDP 6300--> stb_part8_receiver --RIST 6400--> ristreceiver
 *                --UDP 6500--> player_av
 *
 * Everything is on 127.0.0.1. That is not a stylistic preference: the headend's
 * recovery server spent a whole session reading zero bytes off a multicast group
 * it had bound but never joined (no IP_ADD_MEMBERSHIP), on a bridge that was
 * NO-CARRIER anyway. Every local hop on this box stays unicast loopback.
 *
 * ONE PEER on the receiver, unlike the Part 7 chain's two. There is no recovery
 * peer in Step 1, so there is nothing to weight, nothing to NACK and no reason
 * for timing-mode=1 -- that exists to reconcile two senders' clocks onto one
 * flow, and here there is one sender.
 *
 * NOT WATCHDOGGED. The Part 7 sender runs under rist_watchdog because it can
 * legitimately sit waiting for a first marker that may never arrive. The Part 8
 * sender has no such state -- it binds, cuts and emits -- so a restart loop
 * would hide a real failure rather than survive a transient. It is spawned
 * directly and a failure falls through to factory decode.
 */
static int _rist_p8_chain_start(void)
{
    /* in_url now carries the keep-list as well as the PCR PID, so it is sized
     * for RIST_API_PIDS_LEN plus the rest rather than the old 96. */
    static char in_url[320], out_url[96], recv_in[128], recv_out[96];
    static char pids[RIST_API_PIDS_LEN];
    const char *pids_src = NULL;
    char *tx_argv[8];
    char *rx_argv[6];
    int   bufms = _rist_read_int_file(RIST_BUFFER_FILE, RIST_P8_BUFFER_MS);

    if (bufms < 500) {
        RIST_LOG("p8: buffer %dms from %s is too small -- using %dms\n",
                 bufms, RIST_BUFFER_FILE, RIST_P8_BUFFER_MS);
        bufms = RIST_P8_BUFFER_MS;
    }

    /* The cutter's PID comes from THIS service's PMT, via the program record the
     * capture's own slot set is built from -- so the slot set and the cutter
     * cannot disagree about which PID carries the PCR. The headend derives its
     * -C the same way, from the service's PMT rather than by scanning, after
     * scanning handed an NCN channel TLC-GUYANA's PCR PID and it never cut.
     *
     * Without a usable PID there is nothing to cut on, and running uncut would
     * produce a stream that decodes here but can never align with the headend --
     * a false pass for the thing Step 1 exists to prove. Refuse instead. */
    if (!VALID_MARKER_PID(s_rist.prog.pcr_pid)) {
        RIST_LOG("p8: no usable PCR PID from the PMT (0x%04X) -- NOT starting "
                 "the Part 8 path, staying on factory decode\n", s_rist.prog.pcr_pid);
        return -1;
    }

    /* THE KEEP-LIST, and the refusal that goes with it.
     *
     * Part 8 captures the WHOLE transponder -- ~59 Mb/s, 201 PIDs -- because
     * that is the only way this demux will surrender the broadcast PSI the
     * repair needs. That stream must not reach the RIST hop unfiltered, and
     * "must not" here is measured, not cautious: librist does two mallocs per
     * packet and holds them for the buffer window, so at 59 Mb/s and 2000 ms it
     * wants ~17 MB in the sender's retransmit queue and ~17 MB again in the
     * receiver, against MemAvailable of ~25 MB on a box with 54 MB managed. It
     * does not fail cleanly at that point: packets vanish inside the library,
     * NACKs storm after them, and the run ends with the OOM killer taking the
     * sender -- which presents three stages downstream as "chain up, no video".
     *
     * The headend's list is authoritative. It is derived there from the tuned
     * service's PMT plus the fixed PSI set and shipped as part8_filter_pids, and
     * it must be used verbatim: the repair works by both ends cutting IDENTICAL
     * bytes, and a list this box derived for itself could differ by a PID while
     * still looking entirely reasonable.
     *
     * /tmp/ristp8pids overrides it, for a bench box with no headend to ask. Like
     * every other knob here /tmp is tmpfs, so it cannot outlive a reboot. */
    pids[0] = '\0';
    if (_rist_read_str_file(RIST_P8_PIDS_FILE, pids, sizeof(pids)))
        pids_src = RIST_P8_PIDS_FILE;
    else if (s_rist.rec.part8_filter_pids[0]) {
        snprintf(pids, sizeof(pids), "%s", s_rist.rec.part8_filter_pids);
        pids_src = "headend part8_filter_pids";
    }

    if (!pids[0]) {
        RIST_LOG("p8: NO PID KEEP-LIST -- the headend sent no part8_filter_pids "
                 "for service %d and %s is not set. REFUSING to start: the "
                 "capture is the whole transponder (~59 Mb/s) and putting that "
                 "on the RIST hop exhausts this box's memory rather than "
                 "degrading. Staying on factory decode.\n",
                 s_rist.rec.service_id, RIST_P8_PIDS_FILE);
        return -1;
    }
    RIST_LOG("p8: keep-list from %s: %s\n", pids_src, pids);
    s_rist.p8_filtering = 1;

    /* A list that does not name our own PCR PID cannot be cut on: the anchor
     * would be filtered away before the cutter ever saw it, every payload would
     * be plain greedy-7, and nothing would align. That is a headend/box
     * disagreement about which service this is, so refuse rather than run. */
    if (!_rist_pids_contains(pids, (int)s_rist.prog.pcr_pid)) {
        RIST_LOG("p8: keep-list does not contain our PCR PID 0x%04X (%u) -- "
                 "the headend is filtering a different service than the one we "
                 "are tuned to. REFUSING; staying on factory decode.\n",
                 s_rist.prog.pcr_pid, (unsigned)s_rist.prog.pcr_pid);
        return -1;
    }

    /* CROSS-CHECK, warn only -- the mirror of the part8_server_pcr_pid warning
     * in _rist_chain_start(). Their list stays authoritative because identical
     * bytes is the point, but a list missing the video or audio PID we are about
     * to decode is a real disagreement about the service, and it is invisible in
     * the stream: the chain comes up, the repair aligns, and the picture is
     * simply absent. Say it once here rather than leaving it to be found on the
     * screen. */
    if (VALID_MARKER_PID(s_rist.prog.video_pid)
        && !_rist_pids_contains(pids, (int)s_rist.prog.video_pid))
        RIST_LOG("p8: WARNING keep-list omits our VIDEO PID 0x%04X -- the "
                 "repaired stream will carry no picture for this service\n",
                 s_rist.prog.video_pid);
    if (VALID_MARKER_PID(s_rist.prog.cur_audio_pid)
        && !_rist_pids_contains(pids, (int)s_rist.prog.cur_audio_pid))
        RIST_LOG("p8: WARNING keep-list omits our AUDIO PID 0x%04X -- the "
                 "repaired stream will carry no sound for this service\n",
                 s_rist.prog.cur_audio_pid);

    /* THE CUTTER, and whether to run it at all.
     *
     * /tmp/ristp8cut = 0 drops ?pcr_cut= from the input URL, so the sender
     * packs plain greedy-7 with no anchor. It is a DIAGNOSTIC, not a mode: for
     * a box-only bring-up there is no headend to align with, so removing the
     * cutter removes a variable at zero cost. The PID filter is NOT part of
     * that trade and stays on either way -- it is what keeps the transponder
     * off the RIST hop, and running without it does not remove a variable, it
     * removes the box's memory.
     *
     * The cut is NOT the right thing to leave off, and the reasoning that it
     * might be does not survive the arithmetic. On whole-TP the cutter forces a
     * boundary at every PCR on this service's PID -- ~102/s at 59 Mb/s, one
     * every ~383 packets. Between two anchors both ends pack greedy-7 from the
     * SAME restart packet over the SAME interleaving, because both carry the
     * identical multiplex; the 32 other services' packets in between are the
     * same packets in the same order at each end. So the anchor rate is not
     * "only 1.83% of payloads are aligned" -- it is "both ends re-agree 102
     * times a second and everything between two agreements is aligned by
     * construction".
     *
     * Take the cutter away and that is exactly what is lost. Two ends starting
     * capture at different multiplex packets pack [N..N+6] and [M..M+6]; unless
     * N-M is a multiple of 7 the boundaries never coincide and no sequence
     * number indexes the same bytes. Whole-TP does not make alignment free --
     * it makes the anchor cheaper to find, because every end has every PID. */
    if (_rist_read_int_file(RIST_P8_CUT_FILE, 1) == 0) {
        snprintf(in_url, sizeof(in_url), "udp://@127.0.0.1:%d?pids=%s",
                 RIST_P8_CAP_PORT, pids);
        RIST_LOG("p8: CUTTER OFF (%s=0) -- plain greedy-7, no PCR anchor. "
                 "Diagnostic only: box and headend payload boundaries cannot "
                 "align without it. The PID filter stays ON regardless.\n",
                 RIST_P8_CUT_FILE);
    } else {
        snprintf(in_url, sizeof(in_url), "udp://@127.0.0.1:%d?pcr_cut=%u&pids=%s",
                 RIST_P8_CAP_PORT, (unsigned)s_rist.prog.pcr_pid, pids);
    }
    snprintf(out_url, sizeof(out_url), "rist://@127.0.0.1:%d?buffer=%d",
             RIST_P8_LOCAL_PORT, bufms);
    snprintf(recv_in, sizeof(recv_in), "rist://127.0.0.1:%d?buffer=%d",
             RIST_P8_LOCAL_PORT, bufms);
    snprintf(recv_out, sizeof(recv_out), "udp://127.0.0.1:%d", RIST_P8_OUT_PORT);

    {
        int mode = _rist_p8_tail_mode();
        static const char *const why[] = {
            "STEP 1 path, proven on hardware; echo dmx0 > " RIST_P8_TAIL_FILE,
            "STEP 2 experiment: reinject into a memory-fed demux",
            "demux 0 fed from the repaired stream; consumers stay where they are",
        };
        RIST_LOG("p8: tail=%s  (%s)\n", _rist_p8_tail_name(mode), why[mode]);
    }

    RIST_LOG("p8: ports cap=%d local=%d out=%d  buffer=%dms  pcr_cut=0x%04X (%u)"
             "  svc_id=%d prog=%d\n",
             RIST_P8_CAP_PORT, RIST_P8_LOCAL_PORT, RIST_P8_OUT_PORT, bufms,
             s_rist.prog.pcr_pid, (unsigned)s_rist.prog.pcr_pid,
             s_rist.prog.service_id, s_rist.prog.id);

    tx_argv[0] = (char *)RIST_BIN_P8_SENDER;
    tx_argv[1] = (char *)"-i";
    tx_argv[2] = in_url;
    tx_argv[3] = (char *)"-u";
    tx_argv[4] = out_url;
    tx_argv[5] = NULL;

    rx_argv[0] = (char *)RIST_BIN_RECEIVER;
    rx_argv[1] = (char *)"-i";
    rx_argv[2] = recv_in;
    rx_argv[3] = (char *)"-o";
    rx_argv[4] = recv_out;
    rx_argv[5] = NULL;

    /* MEMORY BUDGET, logged before the children are spawned.
     *
     * This box reports managed:54472kB. The two RIST processes are ~11MB RSS
     * EACH, the app is ~5MB, and the whole-TP DVR buffers are megabytes more.
     * An earlier build raised four buffers at once, added ~17MB, and the OOM
     * killer took stb_part8_receiver mid-run -- which presented as "chain up,
     * no video" with rx=0 at the tail, three stages away from the cause.
     * Print the headroom so that is visible before it happens rather than in a
     * kernel backtrace afterwards. */
    {
        FILE *mi = fopen("/proc/meminfo", "r");
        char line[128];
        long free_kb = -1, avail_kb = -1;

        if (mi) {
            while (fgets(line, sizeof(line), mi)) {
                if (strncmp(line, "MemFree:", 8) == 0)
                    free_kb = strtol(line + 8, NULL, 10);
                else if (strncmp(line, "MemAvailable:", 13) == 0)
                    avail_kb = strtol(line + 13, NULL, 10);
            }
            fclose(mi);
        }
        RIST_LOG("p8: memory before spawn: MemFree=%ldkB MemAvailable=%ldkB "
                 "(two RIST processes need ~22MB RSS between them)\n",
                 free_kb, avail_kb);
        if (free_kb >= 0 && free_kb < 8000)
            RIST_LOG("p8:   *** UNDER 8MB FREE -- the OOM killer takes the "
                     "SENDER first and the tail then sees rx=0 ***\n");
    }

    RIST_T("p8: spawning children\n");
    s_rist.pid_watchdog = _rist_spawn(tx_argv, RIST_PID_P8_SENDER, "stb_part8_receiver");
    s_rist.pid_receiver = _rist_spawn(rx_argv, RIST_PID_RECEIVER, "ristreceiver");
    RIST_T("p8: children spawned (sender=%d receiver=%d)\n",
           (int)s_rist.pid_watchdog, (int)s_rist.pid_receiver);

    if (s_rist.pid_watchdog <= 0 || s_rist.pid_receiver <= 0) {
        RIST_LOG("p8: start FAILED -> tearing down (factory decode this zap)\n");
        _rist_chain_stop();
        return -1;
    }

    s_rist.chain_running = 1;

    /* Armed here, not at tail start: if the tail never opens, (a) and (b) are
     * still the numbers that say why, and a chain that feeds nothing is exactly
     * when the report is most worth having. */
    s_rist.p8_chain_t0   = _rist_now_ms();
    s_rist.p8_chain_last = s_rist.p8_chain_t0;
    s_rist.p8_cr_a = s_rist.p8_cr_b = s_rist.p8_cr_c = s_rist.p8_cr_d = 0;
    APP_TIMER_ADD(s_rist.p8_chain_timer, _rist_p8_chain_report_cb,
                  RIST_P8_CHAIN_REPORT_MS, TIMER_ONCE);
    return 0;
}

static int _rist_chain_start(void)
{
    static char in_url[96], out_url[96], recv_in[512], recv_out[96];
    char  rec_url[RIST_API_URL_LEN];
    char *wd_argv[8];
    char *rx_argv[6];
    int   bufms = _rist_read_int_file(RIST_BUFFER_FILE, RIST_BUFFER_MS);

    /* The two chains are mutually exclusive -- one capture, one player_av -- so
     * Part 8 short-circuits here rather than being woven into the Part 7 arm
     * below. Everything after this line is the Part 7 path exactly as it was. */
    if (s_rist.p8_active)
        return _rist_p8_chain_start();

    if (bufms < 500) {                          /* below this nothing recovers */
        RIST_LOG("chain: buffer %dms from %s is too small -- using %dms\n",
                 bufms, RIST_BUFFER_FILE, RIST_BUFFER_MS);
        bufms = RIST_BUFFER_MS;
    }

    /* sender: UDP in from our capture, RIST out listening for the local receiver.
     * The sender's buffer is its retransmit history depth, so it tracks the
     * receiver's -- keeping packets longer than the receiver will ever ask for
     * them is just memory. */
    /* Part 8 PCR-boundary packetisation, OFF unless /tmp/ristpcrcut says 1.
     *
     * When on, the sender re-cuts its input so a payload starts at every packet
     * carrying a PCR on this service's PCR PID. The server's sender runs the
     * same librist with the same parameter, so both sides derive identical
     * payload boundaries from identical bytes and a repair lands where the box
     * expects it. Off, the sender is one datagram in, one payload out exactly as
     * today -- so this cannot disturb the Part 7 chain until it is switched on.
     *
     * The PID comes from the same program record the capture's slot set is
     * built from, so the cutter and the slot set cannot disagree about it. */
    /* Two ways to turn it on, and the API is the production one: the headend
     * sets part8_pcr_cut on a channel whose per-channel Part 8 sender is
     * running. /tmp/ristpcrcut stays as the bench override so a box can be
     * tested against a headend that has not been switched over yet.
     *
     * The PID is OURS either way -- never part8_server_pcr_pid. That value is
     * the headend's pre-uplink PID and is carried for diagnosis only; the PID
     * present in the bytes this cutter is about to split is the one the tuned
     * PMT reports. */
    if ((s_rist.rec.part8_pcr_cut || _rist_read_int_file(RIST_PCRCUT_FILE, 0) == 1)
        && VALID_MARKER_PID(s_rist.prog.pcr_pid))
        snprintf(in_url,  sizeof(in_url),  "udp://@127.0.0.1:%d?pcr_cut=%u",
                 RIST_CAP_PORT, (unsigned)s_rist.prog.pcr_pid);
    else
    snprintf(in_url,  sizeof(in_url),  "udp://@127.0.0.1:%d", RIST_CAP_PORT);

    /* A mismatch here is not fatal but it is always a misconfiguration, and it
     * is invisible in the stream, so say so once at chain start rather than
     * leaving it to be found by a repair that lands in the wrong place. */
    if (s_rist.rec.part8 && s_rist.rec.part8_server_pcr_pid
        && s_rist.rec.part8_server_pcr_pid != (int)s_rist.prog.pcr_pid) {
        RIST_LOG("chain: WARNING headend PCR PID 0x%04X != ours 0x%04X -- the "
                 "uplink is renumbering, so the two filtered streams differ and "
                 "Part 8 repairs will not align\n",
                 s_rist.rec.part8_server_pcr_pid, s_rist.prog.pcr_pid);
    }
    snprintf(out_url, sizeof(out_url), "rist://@127.0.0.1:%d?buffer=%d",
             RIST_LOCAL_PORT, bufms);

    /* The API's URL carries its own buffer=; replace it with ours (see
     * _rist_url_drop_param) so both peers agree and the flow buffer cannot
     * depend on connection order. If it will not fit, keep the API URL as-is:
     * a chain on the wrong buffer still works, and refusing to start would
     * cost the channel entirely. */
    /* Which recovery peer to attach to. part8_rist_url is only ever set when the
     * headend has a per-channel Part 8 sender actually running for this service
     * (getRecoveryChannels checks the unit is active before advertising it), so
     * an empty one means Part 7 -- and Part 7 is what rist_url has always
     * meant. It is not overwritten, so a channel serves both kinds of box at
     * once from the same ingest. */
    {
        const char *peer = (s_rist.rec.part8 && s_rist.rec.part8_rist_url[0])
                         ? s_rist.rec.part8_rist_url : s_rist.rec.rist_url;

        /* A Part 8 channel is a standalone record with no Part 7 peer, so
         * rist_url can legitimately be empty -- but then part8_rist_url must be
         * the one we picked. Neither present means the entry should never have
         * been cached; refuse rather than starting a chain aimed at "". */
        if (!peer || !peer[0]) {
            RIST_LOG("chain: no recovery peer in the API record for service %d "
                     "-- staying on the factory decode path\n",
                     s_rist.rec.service_id);
            return -1;
        }

        RIST_LOG("chain: recovery peer = %s (%s)\n", peer,
                 (peer == s_rist.rec.rist_url) ? "Part 7 marker path"
                                               : "Part 8 per-channel sender");

        if (_rist_url_drop_param(rec_url, sizeof(rec_url), peer, "buffer") < 0) {
            RIST_LOG("chain: could not rewrite buffer= in the recovery URL (too long)"
                     " -- using it unchanged, buffer may differ between peers\n");
            snprintf(rec_url, sizeof(rec_url), "%s", peer);
        }
    }

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
                 "rist://127.0.0.1:%d?weight=0&buffer=%d&timing-mode=1,%s%sweight=1000&timing-mode=1&buffer=%d",
                 RIST_LOCAL_PORT, bufms,
                 rec_url,
                 strchr(rec_url, '?') ? "&" : "?",
                 bufms);
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

    RIST_LOG("chain: ports cap=%d local=%d out=%d  buffer=%dms%s  (svc_id=%d \"%s\")\n",
             RIST_CAP_PORT, RIST_LOCAL_PORT, RIST_OUT_PORT, bufms,
             (bufms == RIST_BUFFER_MS) ? " (default)" : " (/tmp/ristbuffer)",
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
     * leaves chain_active 0 -> factory decode, untouched.
     *
     * screen_failed overrides both: once the chain has been given up on, the
     * decoder must go back to the tuner. In practice the replay clears
     * chain_active before this is next consulted, so this is belt and braces --
     * but the cost of getting the ordering wrong is a permanently dark screen,
     * which is precisely the failure the fallback exists to prevent. */
    if (s_rist.screen_failed)
        return 0;

    /* D2, DECODE CONTENTION. All three tails answer this the same way: YES,
     * suppress factory decode. There is ONE hardware video decoder, and on
     * every Part 8 tail something else owns it --
     *
     *   player_av : the loopback player (Step 1)
     *   dmx3      : our GxMediaApi module on demux 3 (Step 2)
     *   dmx0      : our GxMediaApi module on demux 0 -- the same module, same
     *               video/audio mod id 0, just bound to the instance the
     *               consumers already use.
     *
     * So there is no inversion here and no special case: chain_active drives
     * app_player_close(PLAYER_FOR_NORMAL) at app_play_control.c:1155 exactly as
     * it does for Part 7, and the memory-fed module takes the decoder that
     * releases.
     *
     * The fallback is the same single mechanism too: any chain or tail failure
     * clears chain_active (and p8_active) BEFORE app_normal_play reads this, so
     * suppression lifts in the same zap and factory decode runs. There is no
     * path that suppresses the tuner decode for a tail that did not come up. */
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
             s_rist.chain_active ? _rist_out_port() : s_rist.dst_port,
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
    /* The memory-fed tail holds the same video decoder player_av would, so it
     * has to be released at every point player_av would have been -- otherwise
     * the next zap's owner finds the decoder taken. It also holds udp/6500.
     *
     * On the dmx0 tail this is ALSO what hands demux 0 back to the tuner: the
     * module close releases our ES slots, and the next factory play writes
     * source = <tuner> through dvbsource_normal.c while app_epg_enable() writes
     * it again via GxDmxSetSource(). Two independent writers, both on the
     * normal path, so the factory fallback does not depend on us restoring
     * anything by hand. */
    if (s_rist.p8_tail_on || s_rist.p8_mod) {
        RIST_LOG("screen: stopping the dmx%d tail\n", s_rist.p8_dmx);
        _rist_p8_tail_stop();
        s_rist.screen_started = 0;
    }
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

        /* FALL BACK TO FACTORY DECODE.
         *
         * Until now giving up only stopped the "Waiting..." tip. The decode
         * suppression stayed on (app_rist_screen_enabled() keys purely on
         * chain_active), so a chain that never delivered left the tuner muted
         * and the screen dark indefinitely -- with no way back except a zap.
         *
         * The satellite feed is still arriving; the whole premise is that it is
         * present but needs repair. Showing it unrepaired is a real degraded
         * mode and strictly better than nothing. This is the net that catches
         * failures we have not predicted, so it must not depend on knowing why
         * the chain failed. */
        if (s_rist.chain_active && !s_rist.screen_failed) {
            s_rist.screen_failed = 1;
            s_rist.failed_svc_id = s_rist.rec.service_id;

            RIST_T("chain produced no picture -- falling back to tuner decode "
                   "for svc_id=%d\n", s_rist.failed_svc_id);

            _rist_screen_stop();     /* release the decoder before the tuner takes it */

            /* Replaying the channel re-runs app_normal_play, which now sees
             * app_rist_screen_enabled() == 0 and starts the tuner decode. The
             * failed_svc_id latch stops it from restarting the chain. */
            {
                extern int app_play_program_user_info(GxMsgProperty_NodeModify node_modify);
                GxMsgProperty_NodeModify nm;
                memset(&nm, 0, sizeof(nm));
                app_play_program_user_info(nm);
            }
        }
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

    /* A memory-fed tail owns the screen when one is selected AND it starts.
     * If it refuses to start we fall straight through to player_av in the same
     * zap -- the Step 1 path is the diagnostic fallback, and a tail failing
     * must cost a log line, not a picture.
     *
     * dmx0 is deliberately opened HERE rather than at chain start. Opening it
     * at chain start would put demux 0 on the memory source before the capture
     * has begun, and GxFrontend_QueryStatus() (gxfrontend.c:2352) asks the
     * NIM's demux -- demux 0 -- for GxDemuxPropertyID_TSLockQuery. A
     * memory-fed demux 0 has no TS sync from the tuner, so the lock poll that
     * gates _rist_capture_begin() would never see LOCKED and the capture would
     * only ever start on the delay1 ceiling. Opening after delay3 means the
     * lock check runs against a still-tuner-fed demux 0 and behaves exactly as
     * it does today. The consumers' SLOTS are unaffected by the later source
     * change: slots and filters sit downstream of the source selector. */
    if (s_rist.p8_active && !s_rist.p8_tail_on) {
        int mode = _rist_p8_tail_mode();
        if (mode == RIST_P8_TAIL_DMX3 || mode == RIST_P8_TAIL_DMX0) {
            int modid = (mode == RIST_P8_TAIL_DMX0)
                          ? RIST_P8_DMX0_MODID : RIST_P8_DMX_MODID;
            RIST_LOG("screen: tail=%s -- feeding demux %d from the receiver "
                     "instead of player_av\n", _rist_p8_tail_name(mode), modid);
            if (_rist_p8_tail_start(modid) == 0) {
                s_rist.screen_started = 1;   /* the tail owns the decoder now */
                return 0;
            }
            RIST_LOG("screen: dmx%d tail did not start -> player_av (Step 1 path)\n",
                     modid);
        }
    }

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
    APP_TIMER_REMOVE(s_rist.p8_chain_timer);
    _rist_screen_stop();                     /* release the video decoder for the next player */
    _rist_chain_stop();                      /* SIGTERM -> wait -> SIGKILL, both children */

    if (!s_rist.active && s_rist.reader_thread <= 0 && s_rist.udp_fd < 0 &&
        s_rist.rec_handle == 0 && !s_rist.ts_rec_held)
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

    /* Release the reference taken in _rist_capture_begin(). Ordered after the
     * reader thread is joined and the prog is stopped: this may be the last
     * reference, in which case it frees ctrl->prog, and nothing may be reading
     * through a handle into it. */
    if (s_rist.ts_rec_held) {
        app_ts_record_destroy();
        s_rist.ts_rec_held = 0;
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
     * thread that fills each prog's fifo from DEMUX/DVR instance 2.
     *
     * Each init() is one reference on the shared module, released by the
     * destroy() in app_rist_capture_stop(). Taken at most once, so a begin that
     * fails partway and is retried does not stack references that never come
     * back -- the module would then never tear down for anyone. */
    if (!s_rist.ts_rec_held) {
        if (app_ts_record_init(RIST_FIFO_SIZE, RIST_MAX_PROG) < 0) {
            RIST_LOG("start: app_ts_record_init FAILED\n");
            return;
        }
        s_rist.ts_rec_held = 1;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.prog_id  = (uint16_t)s_rist.prog.id;
    cfg.user_pmt = true;                 /* inject PAT/PMT -> self-contained TS */

    /* PART 8 CAPTURES THE WHOLE TRANSPONDER.
     *
     * Proven on hardware before this was wired in: DVR src = DVR_INPUT_TSPORT
     * with NO demux slot allocated delivers 201 distinct PIDs at 59083 kbit/s
     * with badsync 0 of 393390 packets and framing stride 188 phase 0 at 100%.
     * The PID list carried PAT 0x0000, CAT 0x0001, SDT 0x0011, EIT 0x0012,
     * TDT 0x0014, both PMTs (0x008D/0x008E) and both services' ES, in 59
     * consecutive video/audio/PCR groupings.
     *
     * THAT REPLACES THE SEVEN-PID BROADCAST-PSI CAPTURE, and it is strictly
     * better on the two things that capture existed for:
     *
     *   Byte alignment with the headend. The server filters a fixed PSI set
     *   plus EVERY stream the service declares. A slot capture could match the
     *   PSI half exactly and the ES half only for a one-video-one-audio
     *   service -- a second audio track, subtitles or teletext were in the
     *   server's cut and not in ours. With the whole multiplex there is nothing
     *   left to enumerate and nothing left to get wrong: we hold a superset and
     *   can filter in software to precisely the server's set.
     *
     *   SI for the consumers. app_epg/app_sdt/app_time needed 0x0011/0x0012/
     *   0x0014 in the stream. They are there because nothing was filtered out,
     *   so the demux slot budget stops being part of the question at all.
     *
     * user_pmt and ext_info are left at their zero values deliberately: under
     * full_tp app_ts_record ignores both (see TsRecConfig), because there is
     * nothing to choose. An injected PAT/PMT would be packets the multiplex
     * never carried -- the exact byte-level divergence the numbering cannot
     * tolerate. */
    if (s_rist.p8_active) {
        cfg.full_tp = true;
        RIST_LOG("p8: WHOLE-TP capture (DVR src=TSPORT, no slots) -- the entire "
                 "multiplex, broadcast PSI included\n");
        RIST_LOG("p8:   this service: PMT 0x%04X PCR 0x%04X video 0x%04X audio 0x%04X"
                 "  (cutter frames on the PCR PID)\n",
                 s_rist.prog.pmt_pid, s_rist.prog.pcr_pid,
                 s_rist.prog.video_pid, s_rist.prog.cur_audio_pid);
        RIST_LOG("p8:   tail=%s -- dmx0 demuxes the multiplex as if from the tuner; "
                 "decode + EPG/SDT/time run natively off it\n",
                 _rist_p8_tail_name(_rist_p8_tail_mode()));
    }

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
    } else if (s_rist.chain_active && !s_rist.p8_active) {
        RIST_LOG("capture: no usable marker_pid from the API (%d) -- sender will "
                 "wait for a first marker that never comes\n", s_rist.rec.marker_pid);
    }
    /* Part 8 has no marker by design, so it takes neither branch: there is
     * nothing for the headend to insert and nothing here to validate. */

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
        _rist_pid_sweep(RIST_PID_P8_SENDER, "stb_part8_receiver");
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
    s_rist.p8_active    = 0;
    s_rist.p8_filtering = 0;
    s_rist.p8_dmx0_want = 0;

    /* Clear the fallback latch as soon as a DIFFERENT service is selected, so
     * zapping away and back is a genuine retry. Staying on the same service
     * keeps the latch, which is what stops the replay from looping. */
    s_rist.screen_failed = 0;
    if (s_rist.failed_svc_id && s_rist.failed_svc_id != prog->service_id) {
        RIST_LOG("play_change: clearing the fallback latch (was svc_id=%d)\n",
                 s_rist.failed_svc_id);
        s_rist.failed_svc_id = 0;
    }

    {
        const char *why = NULL;
        int chain_on = _rist_chain_flag(&why);
        int p8_on    = (_rist_read_int_file(RIST_P8_FLAG_FILE, 0) == 1);

        RIST_LOG("play_change: chain=%s (%s)  part8=%s\n",
                 chain_on ? "ENABLED" : "DISABLED", why, p8_on ? "ON" : "off");

        /* PART 8 WINS, and takes no API record.
         *
         * Step 1 has no recovery peer, so there is nothing to look up: the box
         * cuts its own capture and decodes it on its own screen. Requiring an
         * API entry would make a bench test depend on the headend for a step
         * that does not involve the headend at all.
         *
         * It also has to short-circuit the Part 7 arm below rather than sit
         * beside it -- one capture and one player_av cannot serve two chains --
         * and the failed_svc_id latch still applies, so a Part 8 chain that gave
         * up this visit does not immediately restart into the same failure. */
        if (p8_on && s_rist.failed_svc_id
                  && s_rist.failed_svc_id == prog->service_id) {
            RIST_LOG("play_change: svc_id=%d already fell back this visit "
                     "-> factory path (zap away and back to retry)\n", prog->service_id);
        } else if (p8_on) {
            s_rist.p8_active    = 1;
            s_rist.chain_active = 1;     /* drives capture dest, screen, probe */
            /* Latched once here, not re-read from the knob later in the zap.
             * app_normal_play asks this straight after we return, to decide
             * what ts_src the SI consumers get; the tail itself opens later on
             * the delay3 timer. A knob changed in between would leave the
             * consumers and the tail disagreeing about demux 0. */
            s_rist.p8_dmx0_want = (_rist_p8_tail_mode() == RIST_P8_TAIL_DMX0);
            RIST_T("svc_id=%d -> PART 8 video path (tail=%s%s)\n",
                   prog->service_id, _rist_p8_tail_name(_rist_p8_tail_mode()),
                   s_rist.p8_dmx0_want
                     ? ", SI consumers to ts_src=3 on demux 0" : "");
        } else if (chain_on && s_rist.failed_svc_id
                     && s_rist.failed_svc_id == prog->service_id) {
            /* We already gave up on this service this visit. Re-arming here
             * would restart the chain that the fallback just replaced, and the
             * replay would fail identically -- an endless zap loop. */
            RIST_LOG("play_change: svc_id=%d already fell back to tuner decode "
                     "-> factory path (zap away and back to retry)\n", prog->service_id);
        } else if (chain_on) {
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
            /* Both, not just chain_active: p8_active still set would leave the
             * capture aimed at the Part 8 port and player_av pointed at an out
             * port nothing is writing -- a black screen instead of the factory
             * fallback this branch exists to give. */
            s_rist.chain_active  = 0;
            s_rist.p8_active     = 0;
            s_rist.p8_filtering  = 0;
            s_rist.p8_dmx0_want  = 0;
            RIST_T("chain start FAILED -> factory decode for this zap\n");
        } else {
            int d3 = _rist_read_int_file(RIST_DELAY3_FILE, RIST_DELAY3_MS);
            RIST_T("screen: player_av in %dms (delay3/chain) on udp://@:%d\n",
                   d3, _rist_out_port());
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
    /* NOT armed for the dmx0 tail, even though it might look like it should be.
     * PLAYER_FOR_NORMAL is closed on every Part 8 tail (D2 above) -- the picture
     * comes from our GxMediaApi module, not from a player this probe can query.
     * The dmx0 tail reports its own first frame from a moving vpts, in
     * _rist_p8_tail_poll_cb's STAGE 6 line. */
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
