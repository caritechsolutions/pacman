/*****************************************************************************
 * app_rist_stats.c  --  viewing statistics (RAM ring, flushed on the API call)
 *
 * One record per view. Opened on zap, closed on the next zap, serialised into
 * the POST body of the existing 60s channel-list call. See app_rist_stats.h for
 * the storage/timing rationale.
 *
 * Nothing here may block a zap: recording is a couple of memcpys under a mutex,
 * and the network flush happens on the API worker thread.
 *****************************************************************************/

#include "gxcore.h"
#include "app_config.h"
#include "app_module.h"
#include "app.h"
#include "module/app_rist_stats.h"
#include "module/app_rist_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#if DVB2IP_SERVER_SUPPORT

#define STATS_LOG(fmt, ...)  printf("[RISTSTAT] " fmt, ##__VA_ARGS__)

/* Satellite-source coverage over a view. We can observe whether the capture was
 * feeding the chain (i.e. the satellite peer had a source); "none" means the
 * receiver was necessarily recovery-only, which is the case that matters
 * operationally. True per-second FSR state lives inside the receiver and would
 * need it to report back -- see the note in the schema. */
typedef enum { SAT_NONE = 0, SAT_PARTIAL = 1, SAT_FULL = 2 } SatCoverage;

typedef struct
{
    unsigned    id;                         /* monotonic per box, for dedup */
    int         service_id;
    int         ts_id;
    char        name[RIST_STATS_NAME_LEN];
    AppRistPath path;
    unsigned    start_uptime_ms;
    unsigned    duration_ms;
    int         first_frame_ms;             /* -1 = never reached */
    SatCoverage sat;
    int         open;                       /* still being viewed */
} StatRec;

static StatRec   s_ring[RIST_STATS_MAX_RECORDS];
static int       s_head = 0;                /* next write slot */
static int       s_count = 0;               /* live records in the ring */
static unsigned  s_next_id = 1;
static unsigned  s_dropped = 0;             /* overwritten before delivery */
static int       s_open_idx = -1;
static handle_t  s_mutex = 0;
static char      s_box_id[24] = {0};

/* Monotonic milliseconds since boot: immune to the clock steps that make wall
 * time unusable on this box. */
static unsigned _stats_uptime_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (unsigned)ts.tv_sec * 1000u + (unsigned)(ts.tv_nsec / 1000000);
}

static void _stats_init(void)
{
    if (0 == s_mutex)
        GxCore_MutexCreate(&s_mutex);
}

/* Box identity: the MAC address. Chosen over any model/cname string (which is
 * "gx6602" for every unit) because it is unique, stable across reboots and
 * reflashes, needs no provisioning, and is readable without touching flash. */
const char *app_rist_stats_box_id(void)
{
    static const char *ifaces[] = { "/sys/class/net/eth0/address",
                                    "/sys/class/net/wlan0/address",
                                    "/sys/class/net/ra0/address" };
    unsigned i;

    if (s_box_id[0])
        return s_box_id;

    for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++) {
        FILE *f = fopen(ifaces[i], "r");
        if (f) {
            if (fgets(s_box_id, sizeof(s_box_id) - 1, f)) {
                size_t n = strlen(s_box_id);
                while (n && (s_box_id[n-1] == '\n' || s_box_id[n-1] == '\r'))
                    s_box_id[--n] = '\0';
            }
            fclose(f);
            if (s_box_id[0] && strcmp(s_box_id, "00:00:00:00:00:00") != 0)
                return s_box_id;
            s_box_id[0] = '\0';
        }
    }

    snprintf(s_box_id, sizeof(s_box_id), "unknown");
    return s_box_id;
}

void app_rist_stats_view_end(void)
{
    _stats_init();
    GxCore_MutexLock(s_mutex);

    if (s_open_idx >= 0) {
        StatRec *r = &s_ring[s_open_idx];
        if (r->open) {
            r->duration_ms = _stats_uptime_ms() - r->start_uptime_ms;
            r->open = 0;
            STATS_LOG("view end id=%u svc=%d \"%s\" path=%s dur=%ums first_frame=%dms sat=%d\n",
                      r->id, r->service_id, r->name,
                      (r->path == RIST_PATH_RIST) ? "rist" : "tuner",
                      r->duration_ms, r->first_frame_ms, (int)r->sat);
        }
        s_open_idx = -1;
    }

    GxCore_MutexUnlock(s_mutex);
}

void app_rist_stats_view_start(int service_id, int ts_id, const char *name, AppRistPath path)
{
    StatRec *r;

    app_rist_stats_view_end();          /* close any still-open view first */

    _stats_init();
    GxCore_MutexLock(s_mutex);

    r = &s_ring[s_head];
    if (s_count == RIST_STATS_MAX_RECORDS) {
        /* Ring full: overwrite the oldest. Bounded memory beats completeness for
         * a box that cannot reach the server for hours. */
        s_dropped++;
    } else {
        s_count++;
    }

    memset(r, 0, sizeof(*r));
    r->id              = s_next_id++;
    r->service_id      = service_id;
    r->ts_id           = ts_id;
    r->path            = path;
    r->start_uptime_ms = _stats_uptime_ms();
    r->first_frame_ms  = -1;
    r->sat             = SAT_NONE;
    r->open            = 1;
    snprintf(r->name, RIST_STATS_NAME_LEN, "%s", name ? name : "");

    s_open_idx = s_head;
    s_head = (s_head + 1) % RIST_STATS_MAX_RECORDS;

    GxCore_MutexUnlock(s_mutex);
}

void app_rist_stats_first_frame(unsigned ms)
{
    _stats_init();
    GxCore_MutexLock(s_mutex);
    if (s_open_idx >= 0 && s_ring[s_open_idx].open && s_ring[s_open_idx].first_frame_ms < 0)
        s_ring[s_open_idx].first_frame_ms = (int)ms;
    GxCore_MutexUnlock(s_mutex);
}

void app_rist_stats_sat_source(int feeding)
{
    _stats_init();
    GxCore_MutexLock(s_mutex);
    if (s_open_idx >= 0 && s_ring[s_open_idx].open) {
        /* Capture started after the view opened -> the satellite peer only had a
         * source for part of the view; recovery carried the rest. */
        if (feeding)
            s_ring[s_open_idx].sat = SAT_PARTIAL;
    }
    GxCore_MutexUnlock(s_mutex);
}

static const char *_sat_str(SatCoverage s)
{
    return (s == SAT_FULL) ? "full" : (s == SAT_PARTIAL) ? "partial" : "none";
}

int app_rist_stats_build_body(char *buf, int bufsz)
{
    int i, idx, n = 0, sent = 0, first = 1;

    _stats_init();
    GxCore_MutexLock(s_mutex);

    if (s_count == 0) {
        GxCore_MutexUnlock(s_mutex);
        return 0;                        /* nothing to report -> plain GET */
    }

    n += snprintf(buf + n, bufsz - n,
                  "{\"schema\":1,\"box_id\":\"%s\",\"uptime_ms\":%u,"
                  "\"clock_synced\":%s,\"box_time\":%ld,\"dropped\":%u,\"records\":[",
                  app_rist_stats_box_id(), _stats_uptime_ms(),
                  app_rist_time_synced() ? "true" : "false",
                  (long)time(NULL), s_dropped);

    /* Oldest first, so a truncated batch still makes forward progress. */
    idx = (s_head - s_count + RIST_STATS_MAX_RECORDS) % RIST_STATS_MAX_RECORDS;
    for (i = 0; i < s_count; i++, idx = (idx + 1) % RIST_STATS_MAX_RECORDS) {
        StatRec *r = &s_ring[idx];
        unsigned dur;
        int need;

        if (r->open)
            continue;                    /* still being watched: send when it closes */

        dur = r->duration_ms;
        need = snprintf(NULL, 0,
                        "%s{\"id\":%u,\"service_id\":%d,\"ts_id\":%d,\"name\":\"%s\","
                        "\"path\":\"%s\",\"start_uptime_ms\":%u,\"duration_ms\":%u,"
                        "\"first_frame_ms\":%d,\"sat_source\":\"%s\"}",
                        first ? "" : ",", r->id, r->service_id, r->ts_id, r->name,
                        (r->path == RIST_PATH_RIST) ? "rist" : "tuner",
                        r->start_uptime_ms, dur, r->first_frame_ms, _sat_str(r->sat));

        if (n + need + 4 >= bufsz)
            break;                       /* leave room for "]}" -- rest goes next cycle */

        n += snprintf(buf + n, bufsz - n,
                      "%s{\"id\":%u,\"service_id\":%d,\"ts_id\":%d,\"name\":\"%s\","
                      "\"path\":\"%s\",\"start_uptime_ms\":%u,\"duration_ms\":%u,"
                      "\"first_frame_ms\":%d,\"sat_source\":\"%s\"}",
                      first ? "" : ",", r->id, r->service_id, r->ts_id, r->name,
                      (r->path == RIST_PATH_RIST) ? "rist" : "tuner",
                      r->start_uptime_ms, dur, r->first_frame_ms, _sat_str(r->sat));
        first = 0;
        sent++;
    }

    n += snprintf(buf + n, bufsz - n, "]}");
    GxCore_MutexUnlock(s_mutex);

    return sent;
}

void app_rist_stats_ack(unsigned ack_id)
{
    int i, idx, kept = 0;
    static StatRec tmp[RIST_STATS_MAX_RECORDS];

    if (ack_id == 0)
        return;

    _stats_init();
    GxCore_MutexLock(s_mutex);

    idx = (s_head - s_count + RIST_STATS_MAX_RECORDS) % RIST_STATS_MAX_RECORDS;
    for (i = 0; i < s_count; i++, idx = (idx + 1) % RIST_STATS_MAX_RECORDS) {
        StatRec *r = &s_ring[idx];
        /* Keep anything the server has not acknowledged, and anything still open
         * (an open record was never sent). */
        if (r->open || r->id > ack_id)
            tmp[kept++] = *r;
    }

    memcpy(s_ring, tmp, sizeof(StatRec) * kept);
    s_count = kept;
    s_head  = kept % RIST_STATS_MAX_RECORDS;

    /* The open record moved: find it again. */
    s_open_idx = -1;
    for (i = 0; i < kept; i++) {
        if (s_ring[i].open) {
            s_open_idx = i;
            break;
        }
    }

    STATS_LOG("ack up to id=%u -> %d record(s) still pending\n", ack_id, kept);
    GxCore_MutexUnlock(s_mutex);
}

#endif /* DVB2IP_SERVER_SUPPORT */
