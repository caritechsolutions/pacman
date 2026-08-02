/*****************************************************************************
 * app_rist_api.c  --  recovery API client (Step E)
 *
 * Fetches the headend's recovery channel list and caches it by service_id:
 *
 *   GET http://recovery.caritech.net/api/recovery.php
 *   { "server_time": "...", "count": 1,
 *     "channels": [ { "service_id":1000, "ts_id":1, "name":"BBC",
 *                     "marker_pid":8176,
 *                     "rist_url":"rist://1.2.3.4:5700?buffer=8000" } ] }
 *
 * Step D asks app_rist_api_lookup() on zap: a HIT means this service has a live
 * recovery peer and the RIST chain should be started; a MISS means leave the
 * factory tuner path completely alone. Step F takes marker_pid from the entry.
 *
 * FAILURE POLICY -- this module can never black-screen a channel. If DNS fails,
 * the host is unreachable, the HTTP status is not 200, or the JSON is unusable,
 * the cache is simply left as it was (empty at boot), every lookup misses, and
 * the box behaves exactly as factory. Nothing here is on the decode path.
 *
 * The endpoint is overridable at runtime via /tmp/ristapi (one line, full URL)
 * so a test server can be used without a reflash. Plain HTTP is deliberate: no
 * TLS means the box's clock and CA store are irrelevant to the fetch.
 *
 * HTTP is hand-rolled over a socket rather than libcurl: it is ~100 lines, needs
 * no headers beyond the standard socket/netdb set, and keeps this off any
 * library whose headers we cannot verify from the build host. DNS comes from
 * getaddrinfo(), the same resolver librist itself uses on this target.
 *****************************************************************************/

#include "gxcore.h"
#include "app_config.h"
#include "app_module.h"
#include "app.h"
#include "module/app_rist_api.h"
#include "module/app_rist_stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

#include <cJSON.h>

/* ------------------------------------------------------------------ config */
#define RIST_API_DEFAULT_URL    "http://recovery.caritech.net/api/recovery.php"
#define RIST_API_URL_FILE       "/tmp/ristapi"          /* runtime endpoint override */
#define RIST_API_PERIOD_FILE    "/tmp/ristapisecs"      /* runtime refresh period    */
#define RIST_NTP_FILE           "/tmp/ristntp"          /* runtime NTP host override */
#define RIST_NTP_SERVER_1       "pool.ntp.org"
#define RIST_NTP_SERVER_2       "time.google.com"

#define RIST_API_PERIOD_SECS    60      /* steady-state re-fetch period */
#define RIST_API_RETRY_SECS     10      /* re-try period until the first success */
#define RIST_API_CONNECT_MS     5000
#define RIST_API_IO_SECS        8
#define RIST_API_RESP_MAX       (16 * 1024)
#define RIST_API_POST_MAX       (16 * 1024)

#define API_LOG(fmt, ...)       printf("[RISTAPI] " fmt, ##__VA_ARGS__)

/* ------------------------------------------------------------------- state */
static AppRistRecovery  s_chan[RIST_API_MAX_CHANNELS];
static int              s_chan_num  = 0;
static handle_t         s_mutex     = 0;
static handle_t         s_thread    = 0;
static int              s_started   = 0;
static volatile int     s_wake      = 0;   /* set by app_rist_api_refresh() */

/* --------------------------------------------------------------- helpers */
static void _api_url_get(char *out, int outsz)
{
    FILE *f = fopen(RIST_API_URL_FILE, "r");

    if (f) {
        char line[RIST_API_URL_LEN] = {0};
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
    strncpy(out, RIST_API_DEFAULT_URL, outsz - 1);
    out[outsz - 1] = '\0';
}

static int _api_period_get(void)
{
    FILE *f = fopen(RIST_API_PERIOD_FILE, "r");
    int v = RIST_API_PERIOD_SECS, t = 0;

    if (f) {
        if (fscanf(f, "%d", &t) == 1 && t >= 5 && t <= 3600)
            v = t;
        fclose(f);
    }
    return v;
}

/* Split "http://host[:port]/path" -> host, port, path. Returns 0 on success. */
static int _api_url_split(const char *url, char *host, int hostsz, int *port, char *path, int pathsz)
{
    const char *p = url, *slash = NULL, *colon = NULL;
    int n;

    if (strncmp(p, "http://", 7) == 0)
        p += 7;
    else if (strstr(url, "://"))
        return -1;                      /* https:// or anything else: not supported */

    slash = strchr(p, '/');
    n     = slash ? (int)(slash - p) : (int)strlen(p);
    if (n <= 0 || n >= hostsz)
        return -1;

    memcpy(host, p, n);
    host[n] = '\0';

    *port = 80;
    colon = strchr(host, ':');
    if (colon) {
        *port = atoi(colon + 1);
        if (*port <= 0 || *port > 65535)
            return -1;
        host[colon - host] = '\0';      /* trim ":port" off the host */
    }

    snprintf(path, pathsz, "%s", slash ? slash : "/");
    return 0;
}

/* connect() with a bounded wait, so a black-holed host cannot wedge the worker. */
static int _api_connect(const char *host, int port)
{
    char portstr[8];
    struct addrinfo hints, *res = NULL, *ai;
    int fd = -1, rc;

    snprintf(portstr, sizeof(portstr), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;        /* box is IPv4 */
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, portstr, &hints, &res);
    if (rc != 0 || res == NULL) {
        API_LOG("resolve FAILED for %s (%s) -- staying on factory path\n",
                host, gai_strerror(rc));
        return -1;
    }

    for (ai = res; ai; ai = ai->ai_next) {
        int flags;
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;

        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        rc = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (rc < 0 && errno == EINPROGRESS) {
            fd_set wset;
            struct timeval tv;
            int err = 0;
            socklen_t elen = sizeof(err);

            FD_ZERO(&wset);
            FD_SET(fd, &wset);
            tv.tv_sec  = RIST_API_CONNECT_MS / 1000;
            tv.tv_usec = (RIST_API_CONNECT_MS % 1000) * 1000;

            rc = select(fd + 1, NULL, &wset, NULL, &tv);
            if (rc > 0 && getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen) == 0 && err == 0)
                rc = 0;
            else
                rc = -1;
        }

        if (rc == 0) {
            struct timeval io;
            fcntl(fd, F_SETFL, flags);          /* back to blocking for read/write */
            io.tv_sec  = RIST_API_IO_SECS;
            io.tv_usec = 0;
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &io, sizeof(io));
            setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &io, sizeof(io));
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

/* GET the url into buf. Returns the HTTP status code, or -1 on transport error.
 * On 200 the response BODY (not the headers) is left in buf. */
static int _api_http_get(const char *url, const char *post_body, char *buf, int bufsz)
{
    char host[128] = {0}, path[256] = {0}, req[512];
    int  port = 80, fd, total = 0, n, status = -1;
    char *body;

    if (_api_url_split(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        API_LOG("bad endpoint URL: %s\n", url);
        return -1;
    }

    fd = _api_connect(host, port);
    if (fd < 0) {
        API_LOG("connect FAILED to %s:%d -- staying on factory path\n", host, port);
        return -1;
    }

    /* POST when there are stats to report, plain GET otherwise -- so a box with
     * nothing to say still works, and the endpoint stays curl-able by hand. */
    if (post_body && post_body[0]) {
        n = snprintf(req, sizeof(req),
                     "POST %s HTTP/1.0\r\n"
                     "Host: %s\r\n"
                     "User-Agent: gx6631-stb\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n\r\n",
                     path, host, (int)strlen(post_body));
    } else {
        n = snprintf(req, sizeof(req),
                     "GET %s HTTP/1.0\r\n"
                     "Host: %s\r\n"
                     "User-Agent: gx6631-stb\r\n"
                     "Connection: close\r\n\r\n",
                     path, host);
    }
    if (write(fd, req, n) != n) {
        API_LOG("request write FAILED\n");
        close(fd);
        return -1;
    }
    if (post_body && post_body[0]) {
        int blen = (int)strlen(post_body);
        if (write(fd, post_body, blen) != blen) {
            API_LOG("POST body write FAILED\n");
            close(fd);
            return -1;
        }
    }

    while (total < bufsz - 1) {
        n = read(fd, buf + total, bufsz - 1 - total);
        if (n <= 0)
            break;                       /* server closed (HTTP/1.0) or timed out */
        total += n;
    }
    close(fd);
    buf[total] = '\0';

    if (total <= 0) {
        API_LOG("empty response\n");
        return -1;
    }

    if (sscanf(buf, "HTTP/%*d.%*d %d", &status) != 1) {
        API_LOG("unparseable HTTP status line\n");
        return -1;
    }

    /* Move the body to the front of the buffer. */
    body = strstr(buf, "\r\n\r\n");
    if (body)
        body += 4;
    else if ((body = strstr(buf, "\n\n")) != NULL)
        body += 2;

    if (body)
        memmove(buf, body, strlen(body) + 1);
    else
        buf[0] = '\0';

    return status;
}

/* Parse the JSON body into the cache. Returns the channel count, or -1. */
static int _api_parse(const char *body)
{
    cJSON *root, *channels;
    int i, num, kept = 0;

    root = cJSON_Parse(body);
    if (!root) {
        API_LOG("JSON parse FAILED -- staying on factory path\n");
        return -1;
    }

    /* Stats acknowledgement: drop everything the server confirmed it stored, so a
     * lost response retries rather than double-counts. */
    {
        cJSON *ack = cJSON_GetObjectItem(root, "ack_id");
        if (ack && ack->valueint > 0)
            app_rist_stats_ack((unsigned)ack->valueint);
    }

    channels = cJSON_GetObjectItem(root, "channels");
    if (!channels) {
        API_LOG("no \"channels\" array in response\n");
        cJSON_Delete(root);
        return -1;
    }

    num = cJSON_GetArraySize(channels);

    GxCore_MutexLock(s_mutex);
    memset(s_chan, 0, sizeof(s_chan));

    for (i = 0; i < num && kept < RIST_API_MAX_CHANNELS; i++) {
        cJSON *c = cJSON_GetArrayItem(channels, i);
        cJSON *sid, *tsid, *mpid, *url, *name;

        if (!c)
            continue;

        sid  = cJSON_GetObjectItem(c, "service_id");
        tsid = cJSON_GetObjectItem(c, "ts_id");
        mpid = cJSON_GetObjectItem(c, "marker_pid");
        url  = cJSON_GetObjectItem(c, "rist_url");
        name = cJSON_GetObjectItem(c, "name");

        /* service_id + rist_url are the minimum needed to act on an entry. */
        if (!sid || !url || !url->valuestring)
            continue;

        s_chan[kept].service_id = sid->valueint;
        s_chan[kept].ts_id      = tsid ? tsid->valueint : 0;
        s_chan[kept].marker_pid = mpid ? mpid->valueint : 0;
        snprintf(s_chan[kept].rist_url, RIST_API_URL_LEN, "%s", url->valuestring);
        snprintf(s_chan[kept].name, RIST_API_NAME_LEN, "%s",
                 (name && name->valuestring) ? name->valuestring : "");
        kept++;
    }

    s_chan_num = kept;
    GxCore_MutexUnlock(s_mutex);

    cJSON_Delete(root);
    return kept;
}

static void _api_dump(void)
{
    int i;

    GxCore_MutexLock(s_mutex);
    API_LOG("cached %d recovery channel(s):\n", s_chan_num);
    for (i = 0; i < s_chan_num; i++) {
        API_LOG("  [%d] svc_id=%d ts_id=%d marker_pid=%d (0x%04X) name=\"%s\"\n",
                i, s_chan[i].service_id, s_chan[i].ts_id,
                s_chan[i].marker_pid, s_chan[i].marker_pid, s_chan[i].name);
        API_LOG("      rist_url=%s\n", s_chan[i].rist_url);
    }
    if (s_chan_num == 0)
        API_LOG("  (none -- every channel stays on the factory decode path)\n");
    GxCore_MutexUnlock(s_mutex);
}

/* -------------------------------------------------------------- SNTP time */
/* The box takes its clock from DVB, so with no satellite it runs on a stale
 * default (Jan 2015 observed). We do one SNTP step at network-up, before any
 * chain can start.
 *
 * Self-contained on purpose: the platform's own net-time path (gx_net_get_time,
 * app_net_time.c) is compiled out in this build (GET_NET_TIME_SUPPORT=0,
 * OTT_SUPPORT=0), and adding a busybox applet would be a rootfs dependency. This
 * is one 48-byte UDP exchange -- no new library, no new binary.
 *
 * Deliberately ONE step, at boot only: a clock that leaps while a chain is
 * running is the hazard, not a clock that is merely wrong. */
#define SNTP_PORT           123
#define SNTP_TIMEOUT_SECS   5
#define NTP_UNIX_DELTA      2208988800u    /* seconds between 1900 and 1970 epochs */

static int s_time_synced = 0;

int app_rist_time_synced(void)
{
    return s_time_synced;
}

static int _api_sntp_sync(const char *host)
{
    unsigned char pkt[48];
    struct addrinfo hints, *res = NULL;
    struct timeval tv;
    GxTime now;
    int fd = -1, ret = -1;
    unsigned int secs;
    time_t before, after;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(host, "123", &hints, &res) != 0 || res == NULL) {
        API_LOG("ntp: resolve FAILED for %s -- leaving the clock alone\n", host);
        return -1;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        goto out;

    tv.tv_sec  = SNTP_TIMEOUT_SECS;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    memset(pkt, 0, sizeof(pkt));
    pkt[0] = 0x1b;                     /* LI=0, VN=3, Mode=3 (client) */

    if (sendto(fd, pkt, sizeof(pkt), 0, res->ai_addr, res->ai_addrlen) != (int)sizeof(pkt)) {
        API_LOG("ntp: send FAILED to %s\n", host);
        goto out;
    }

    if (recv(fd, pkt, sizeof(pkt), 0) != (int)sizeof(pkt)) {
        API_LOG("ntp: no reply from %s within %ds\n", host, SNTP_TIMEOUT_SECS);
        goto out;
    }

    /* Transmit timestamp: bytes 40..43, seconds since 1900, big endian. */
    secs = ((unsigned int)pkt[40] << 24) | ((unsigned int)pkt[41] << 16) |
           ((unsigned int)pkt[42] << 8)  | (unsigned int)pkt[43];
    if (secs <= NTP_UNIX_DELTA) {
        API_LOG("ntp: implausible timestamp from %s -- ignoring\n", host);
        goto out;
    }

    before = time(NULL);
    after  = (time_t)(secs - NTP_UNIX_DELTA);

    /* Use the platform's setter, the same one app_time_sync() uses for DVB time. */
    now.seconds   = (uint32_t)after;
    now.microsecs = 0;
    GxCore_SetLocalTime(&now);

    s_time_synced = 1;
    ret = 0;
    API_LOG("ntp: clock stepped from %ld to %ld (%+ld s) via %s\n",
            (long)before, (long)after, (long)(after - before), host);

out:
    if (fd >= 0)
        close(fd);
    if (res)
        freeaddrinfo(res);
    return ret;
}

/* ---------------------------------------------------------------- worker */
static int _api_fetch_once(void)
{
    char url[RIST_API_URL_LEN] = {0};
    char *body = NULL, *post = NULL;
    int   status, kept = -1, nrec = 0;

    _api_url_get(url, sizeof(url));

    body = (char *)GxCore_Mallocz(RIST_API_RESP_MAX);
    if (!body) {
        API_LOG("response buffer alloc FAILED\n");
        return -1;
    }

    /* Build the stats POST body first: piggybacking keeps this to ONE round trip.
     * A failure to send leaves the records queued for the next cycle, and never
     * stops the channel list from being fetched. */
    post = (char *)GxCore_Mallocz(RIST_API_POST_MAX);
    if (post) {
        nrec = app_rist_stats_build_body(post, RIST_API_POST_MAX);
        if (nrec <= 0) {
            GxCore_Free(post);
            post = NULL;
        }
    }

    API_LOG("fetching %s (%s)\n", url, post ? "POST +stats" : "GET");
    status = _api_http_get(url, post, body, RIST_API_RESP_MAX);
    if (post) {
        if (status != 200)
            API_LOG("stats: %d record(s) NOT acknowledged -- retrying next cycle\n", nrec);
        GxCore_Free(post);
    }

    if (status == 200) {
        kept = _api_parse(body);
        if (kept >= 0)
            _api_dump();
    } else if (status > 0) {
        API_LOG("HTTP %d -- no recovery list, staying on factory path\n", status);
    }
    /* status < 0 already logged the transport reason. */

    GxCore_Free(body);
    return kept;
}

static void _api_worker(void *arg)
{
    int ok = 0;

    (void)arg;
    GxCore_ThreadDetach();

    /* Step the clock ONCE, before the first fetch and therefore before any chain
     * can start (a chain needs the list this fetch produces). Failure is silent:
     * the box carries on with whatever time it had. */
    {
        int i;
        const char *servers[] = { RIST_NTP_SERVER_1, RIST_NTP_SERVER_2 };
        char host[RIST_API_URL_LEN] = {0};
        FILE *f = fopen(RIST_NTP_FILE, "r");

        if (f) {                       /* runtime override, one hostname per line */
            if (fgets(host, sizeof(host) - 1, f)) {
                size_t n = strlen(host);
                while (n && (host[n-1] == '\n' || host[n-1] == '\r' || host[n-1] == ' '))
                    host[--n] = '\0';
            }
            fclose(f);
        }

        if (host[0]) {
            _api_sntp_sync(host);
        } else {
            for (i = 0; i < (int)(sizeof(servers) / sizeof(servers[0])); i++) {
                if (_api_sntp_sync(servers[i]) == 0)
                    break;
            }
        }
        if (!s_time_synced)
            API_LOG("ntp: NOT synced -- box time stays as-is (stats marked untrusted)\n");
    }

    while (1) {
        int period, waited = 0;

        if (_api_fetch_once() >= 0)
            ok = 1;

        /* Retry quickly until the first success (DNS/network may still be
         * settling right after the IP arrives), then settle into the steady
         * refresh period so channels added or stopped at the headend are
         * picked up without a reboot. */
        period = (ok ? _api_period_get() : RIST_API_RETRY_SECS) * 2;  /* in 500ms ticks */

        while (waited < period) {
            if (s_wake) {               /* app_rist_api_refresh() */
                s_wake = 0;
                break;
            }
            GxCore_ThreadDelay(500);
            waited++;
        }
    }
}

/* ------------------------------------------------------------------- API */
void app_rist_api_start(void)
{
    if (s_started)
        return;

    if (0 == s_mutex && GXCORE_SUCCESS != GxCore_MutexCreate(&s_mutex)) {
        API_LOG("mutex create FAILED\n");
        return;
    }

    if (GxCore_ThreadCreate("app_rist_api", &s_thread, _api_worker, NULL,
                            32 * 1024, GXOS_DEFAULT_PRIORITY) != GXCORE_SUCCESS) {
        API_LOG("worker thread create FAILED -- staying on factory path\n");
        return;
    }

    s_started = 1;
    API_LOG("client started (endpoint override: %s, refresh: %s)\n",
            RIST_API_URL_FILE, RIST_API_PERIOD_FILE);
}

int app_rist_api_lookup(int service_id, AppRistRecovery *out)
{
    int i, ret = -1;

    if (0 == s_mutex)
        return -1;

    GxCore_MutexLock(s_mutex);
    for (i = 0; i < s_chan_num; i++) {
        if (s_chan[i].service_id == service_id) {
            if (out)
                *out = s_chan[i];
            ret = 0;
            break;
        }
    }
    GxCore_MutexUnlock(s_mutex);
    return ret;
}

int app_rist_api_count(void)
{
    int n;

    if (0 == s_mutex)
        return 0;

    GxCore_MutexLock(s_mutex);
    n = s_chan_num;
    GxCore_MutexUnlock(s_mutex);
    return n;
}

void app_rist_api_refresh(void)
{
    s_wake = 1;
}
