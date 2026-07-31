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

typedef struct _AppRistRecovery
{
    int  service_id;
    int  ts_id;
    int  marker_pid;
    char rist_url[RIST_API_URL_LEN];
    char name[RIST_API_NAME_LEN];
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

#endif /* __APP_RIST_API_H__ */
