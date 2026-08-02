#ifndef __APP_RIST_STATS_H__
#define __APP_RIST_STATS_H__

#include "gxcore.h"

/*
 * Viewing statistics (per-view records, flushed on the existing API call).
 *
 * One record per view: opened on zap, closed on the next zap. Records live in a
 * RAM ring buffer (never flash -- a per-zap write would wear it), are POSTed
 * with the 60s channel-list fetch, and are dropped oldest-first if the server is
 * unreachable for a long time.
 *
 * Timing is monotonic uptime, NOT wall clock: the box takes time from DVB, so
 * with no satellite its clock can be years out. The server applies wall clock on
 * receipt; the box's own idea of the time is sent separately and flagged.
 */

#define RIST_STATS_MAX_RECORDS  256     /* ring capacity, ~30KB */
#define RIST_STATS_NAME_LEN     64

/* Which delivery path carried the view. */
typedef enum
{
    RIST_PATH_TUNER = 0,    /* factory path: tuner -> demux -> decode */
    RIST_PATH_RIST  = 1,    /* RIST chain: capture/recovery -> receiver -> screen */
} AppRistPath;

/**
 * @brief Open a view record (called on zap, after the chain decision).
 *        Implicitly closes any record still open.
 */
extern void app_rist_stats_view_start(int service_id, int ts_id, const char *name,
                                      AppRistPath path);

/**
 * @brief Close the open view record, stamping its duration.
 */
extern void app_rist_stats_view_end(void);

/**
 * @brief Note that the picture appeared, and how long it took (ms from view start).
 *        Recorded once per view; a view that never reaches this reports -1.
 */
extern void app_rist_stats_first_frame(unsigned ms);

/**
 * @brief Note whether the satellite capture was feeding the chain for this view.
 *        Called when the capture starts; absence means the view ran recovery-only.
 */
extern void app_rist_stats_sat_source(int feeding);

/**
 * @brief Serialise pending records as the JSON POST body.
 *
 * @param buf/bufsz  caller's buffer
 * @return number of records serialised (0 = nothing to send, send a plain GET),
 *         or -1 on error.
 */
extern int app_rist_stats_build_body(char *buf, int bufsz);

/**
 * @brief Drop every record the server acknowledged (id <= ack_id).
 *        Unacknowledged records stay for the next cycle, so a lost response
 *        retries rather than loses; the server dedups on (box_id, id).
 */
extern void app_rist_stats_ack(unsigned ack_id);

/**
 * @brief Stable per-box identifier (MAC address, "aa:bb:cc:dd:ee:ff").
 *        Placeholder identity -- see chip_sn/ca_id below.
 */
extern const char *app_rist_stats_box_id(void);

/**
 * @brief Which interface box_id came from ("eth0"/"wlan0"/"ra0"/"none").
 */
extern const char *app_rist_stats_box_iface(void);

/**
 * @brief Chipset serial from the OTP fuses, as lowercase hex ("" if unavailable).
 *        Chipset-tied, so it survives a swapped WiFi dongle, and needs no CA stack.
 */
extern const char *app_rist_stats_chip_sn(void);

/**
 * @brief CryptoGuard CA id, or "" until the CA stack is integrated.
 */
extern const char *app_rist_stats_ca_id(void);

/**
 * @brief Resolve and log every identity once, at startup, so an identity change
 *        shows up on serial instead of as a phantom duplicate box in the stats.
 */
extern void app_rist_stats_identity_log(void);

#endif /* __APP_RIST_STATS_H__ */
