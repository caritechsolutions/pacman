#ifndef __APP_RIST_API_H__
#define __APP_RIST_API_H__

#include "gxcore.h"

/*
 * Recovery API client (Step E).
 *
 * Fetches the headend's recovery channel list over plain HTTP and caches it in
 * memory, keyed by service_id. Step D looks the tuned service up here to decide
 * whether to start the RIST chain; Step F takes marker_pid from the same entry.
 *
 * Everything here is best-effort and failure-silent by design: if DNS fails, the
 * API is unreachable, or the list is empty, the cache stays empty, every lookup
 * misses, and the box behaves exactly as factory (normal tuner decode).
 */

#define RIST_API_MAX_CHANNELS   32
#define RIST_API_URL_LEN        160
#define RIST_API_NAME_LEN       64
/* The Part 8 keep-list as a CSV. Seven fixed PSI PIDs plus a service's PMT, PCR
 * and elementary streams is a dozen or so entries at up to 5 characters each;
 * 192 leaves room for an unusually fat service without approaching librist's
 * own 256-byte ?pids= field, which refuses a list too long to hold rather than
 * truncating it. A truncated list filters to a SHORT set, and a short set looks
 * like a working channel while dropping audio. */
#define RIST_API_PIDS_LEN       192

typedef struct _AppRistRecovery
{
    int  service_id;
    int  ts_id;
    int  marker_pid;
    char rist_url[RIST_API_URL_LEN];
    char name[RIST_API_NAME_LEN];

    /* ---- Part 8, all optional. A headend that serves only Part 7 sends none
     * of these, cJSON leaves them absent, and every field below stays 0/"" --
     * which is exactly the factory Part 7 behaviour. */

    /* 1 = this channel has a Part 8 per-channel recovery sender running. */
    int  part8;

    /* Where that sender listens. Used INSTEAD of rist_url when part8 is set;
     * rist_url keeps its Part 7 meaning and is left alone so a box that falls
     * back to Part 7 still has it. */
    char part8_rist_url[RIST_API_URL_LEN];

    /* 1 = cut our own sender's payloads at PCR boundaries, so our framing
     * matches the headend's. The PID is NOT taken from the API: see below. */
    int  part8_pcr_cut;

    /* The headend's PCR PID, PRE-UPLINK. DIAGNOSTIC ONLY -- never feed this to
     * our own ?pcr_cut. Ours must be the PID in the bytes WE are cutting, which
     * is what the tuned PMT says (GxBus_PmProgGetById -> prog.pcr_pid) after the
     * uplink mux has had its way. The headend refuses to enable Part 8 on a
     * channel that remaps, so the two agree today; keeping them separate means
     * a future remap breaks loudly here rather than silently misaligning. */
    int  part8_server_pcr_pid;

    /* THE PID SET THE HEADEND FILTERS THIS CHANNEL DOWN TO, as a comma-separated
     * list, straight from part8_filter_pids in the recovery record.
     *
     * The box captures the WHOLE transponder because that is the only way its
     * demux will hand over the broadcast PSI (a demux slot takes one scalar PID
     * and there are not enough slots). It must then filter to exactly this set
     * before the RIST hop, for two independent reasons:
     *
     *   - MEMORY. librist holds two mallocs per packet for the whole buffer
     *     window: at 59 Mb/s and 2000 ms that is ~17 MB in the sender's
     *     retransmit queue and ~17 MB again in the receiver, on a box with
     *     54 MB managed. It does not fit and the box wedges.
     *
     *   - ALIGNMENT. The repair works by both ends cutting IDENTICAL bytes
     *     identically. The headend cuts what tsp -P filter --pid left it, so
     *     the box must cut the same subset of the same multiplex.
     *
     * THE SERVER'S LIST IS AUTHORITATIVE, and note this is the OPPOSITE of the
     * rule for part8_server_pcr_pid above. The PCR PID must be ours because it
     * has to be the PID present in the bytes WE are cutting. The filter set must
     * be theirs because byte-identity with them is the point -- a list we derive
     * ourselves could differ by one PID and that misaligns every splice while
     * still looking like a working channel. We derive our own only to compare
     * and warn.
     *
     * Empty = the headend could not derive it (filterPids() refuses rather than
     * guessing). Part 8 is then declined for this channel; see
     * app_rist_capture.c. */
    char part8_filter_pids[RIST_API_PIDS_LEN];
} AppRistRecovery;

/**
 * @brief Start the API client worker (idempotent).
 *
 * Called when the network comes up with an IP. Spawns one detached worker that
 * fetches the list, retries on failure, then re-fetches periodically so channels
 * added/stopped at the headend are picked up without a reboot. Never blocks the
 * caller.
 */
extern void app_rist_api_start(void);

/**
 * @brief Look up a service in the cached recovery list.
 *
 * @param service_id  service id of the tuned program
 * @param out         filled in on hit; may be NULL to test presence only
 *
 * @return 0 on hit (recovery configured for this service), -1 on miss.
 *         A miss means "stay on the factory decode path".
 */
extern int app_rist_api_lookup(int service_id, AppRistRecovery *out);

/**
 * @brief Number of channels currently cached (0 = no recovery anywhere).
 */
extern int app_rist_api_count(void);

/**
 * @brief Force an immediate re-fetch (e.g. after a recovery connection failure).
 *        Non-blocking: it just wakes the worker.
 */
extern void app_rist_api_refresh(void);

/**
 * @brief Was the clock set from the network this boot?
 *
 * The box normally takes time from DVB, so with no satellite it runs on a stale
 * default (Jan 2015 was observed). Stats records carry this flag so the server
 * knows whether the box's own timestamps mean anything.
 *
 * @return 1 if SNTP set the clock, 0 otherwise.
 */
extern int app_rist_time_synced(void);

#endif /* __APP_RIST_API_H__ */
