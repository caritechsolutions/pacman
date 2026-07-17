/*
 * UDP prototype streaming system
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * UDP protocol
 */

#define _BSD_SOURCE     /* Needed for using struct ip_mreq with recent glibc */

#include "avformat.h"
#include "../avutil/fifo.h"
#include "../avutil/intreadwrite.h"
#include "../avutil/avstring.h"
#include <unistd.h>
#include <fcntl.h>
#include "network.h"
#include <sys/time.h>

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

#define UDP_TX_BUF_SIZE 32768
#define UDP_RX_BUF_SIZE 524288//393216
#define UDP_MAX_PKT_SIZE 65536
#define UDP_HEADER_SIZE 8

#ifndef IPPROTO_UDPLITE
#define IPPROTO_UDPLITE                                  136
#endif
#define UDPLITE_SEND_CSCOV                               10
#define UDPLITE_RECV_CSCOV                               11
#define STREAM_UDP_MAX_COUNT                             64
#define STREAM_UDP_MAX_PKT_SIZE                          1472
#define HAVE_PTHREAD_CANCEL                              0
#define UDP_BIG_MEM_FIFO_SIZE                            (28*1024)
#define UDP_SMALL_MEM_FIFO_SIZE                          (5*1024)
#define DEBUG_READ_PACKET_TIMES_MS                        1500

/*Only used for streams that support streaming connections (for example:. ts,. flv)*/
#define STREAM_UDP_PVR_DEBUG 0
#if STREAM_UDP_PVR_DEBUG
    static char* stream_udp_pvr_file = "/mnt/usb01/udp_downlod.ts"; /*ecos*/
    //static char* stream_udp_pvr_file = "/media/sda1/udp_downlod.ts"; /*linux*/
#endif

typedef struct {
    int udp_fd;
    int ttl;
    int udplite_coverage;
    int buffer_size;
    int is_multicast;
    int local_port;
    int pkt_size;
    int reuse_socket;
    int overrun_nonfatal;
    struct sockaddr_storage dest_addr;
    int dest_addr_len;
    int is_connected;
    /* Circular Buffer variables for use in UDP receive code */
    int circular_buffer_size;
    AVFifo *fifo;
    int circular_buffer_error;
    int udp_data_len;
    int udp_data_pos;
    int64_t bitrate; /* number of bits to send per second */
    int64_t burst_bits;
    int close_req;
    handle_t circular_buffer_thread;
    handle_t mutex;
    handle_t cond;
    int thread_started;
    uint8_t tmp[UDP_MAX_PKT_SIZE+4];
#ifdef LINUX_OS
    char udp_data[STREAM_UDP_MAX_PKT_SIZE];
#endif
    char *localaddr;
    int remaining_in_dg;
    int is_open_udp_cache_pthread;
#if STREAM_UDP_PVR_DEBUG
    FILE *download_stream_fp;
#endif
    struct sockaddr_storage local_addr_storage;
} UDPContext;

static int udp_set_multicast_ttl(int sockfd, int mcastTTL,
                                 struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET) {
        if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &mcastTTL, sizeof(mcastTTL)) < 0) {
            return ff_neterrno();
        }
    }
    if (addr->sa_family == AF_INET6) {
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &mcastTTL, sizeof(mcastTTL)) < 0) {
            return ff_neterrno();
        }
    }
    return 0;
}

static int udp_join_multicast_group(int sockfd, struct sockaddr *addr, struct sockaddr *local_addr)
{
    if (addr->sa_family == AF_INET) {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
        if (local_addr)
            mreq.imr_interface= ((struct sockaddr_in *)local_addr)->sin_addr;
        else
            mreq.imr_interface.s_addr= INADDR_ANY;
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *)&mreq, sizeof(mreq)) < 0) {
            gxloge ("setsockopt(IP_ADD_MEMBERSHIP)\n");
            return ff_neterrno();
        }
    }
    if (addr->sa_family == AF_INET6) {
        struct ipv6_mreq mreq6;
        memcpy(&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6 *)addr)->sin6_addr), sizeof(struct in6_addr));
        mreq6.ipv6mr_interface= 0;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq6, sizeof(mreq6)) < 0) {
            gxloge ("setsockopt(IPV6_ADD_MEMBERSHIP)\n");
            return ff_neterrno();
        }
    }
    return 0;
}

static int udp_leave_multicast_group(int sockfd, struct sockaddr *addr, struct sockaddr *local_addr)
{
    if (addr->sa_family == AF_INET) {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
        if (local_addr)
            mreq.imr_interface = ((struct sockaddr_in *)local_addr)->sin_addr;
        else
            mreq.imr_interface.s_addr= INADDR_ANY;
        if (setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const void *)&mreq, sizeof(mreq)) < 0) {
            gxloge ("setsockopt(IP_DROP_MEMBERSHIP) fail.\n");
            return -1;
        }
    }
    if (addr->sa_family == AF_INET6) {
        struct ipv6_mreq mreq6;
        memcpy(&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6 *)addr)->sin6_addr), sizeof(struct in6_addr));
        mreq6.ipv6mr_interface= 0;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq6, sizeof(mreq6)) < 0) {
            gxloge ("setsockopt(IPV6_DROP_MEMBERSHIP) fail.\n");
            return -1;
        }
    }
    return 0;
}

static struct addrinfo* udp_resolve_host(const char *hostname, int port,
                                         int type, int family, int flags)
{
    struct addrinfo hints = { 0 }, *res = 0;
    int error;
    char sport[16];
    const char *node = 0, *service = "0";

    if (port > 0) {
        snprintf(sport, sizeof(sport), "%d", port);
        service = sport;
    }
    if ((hostname) && (hostname[0] != '\0') && (hostname[0] != '?')) {
        node = hostname;
    }
    hints.ai_socktype = type;
    hints.ai_family   = family;
    hints.ai_flags = flags;
    if ((error = getaddrinfo(node, service, &hints, &res))) {
        res = NULL;
    }
    return res;
}

static int udp_set_url(struct sockaddr_storage *addr,
                       const char *hostname, int port)
{
    struct addrinfo *res0;
    int addr_len;

    res0 = udp_resolve_host(hostname, port, SOCK_DGRAM, AF_UNSPEC, 0);
    if (res0 == 0) return AVERROR(EIO);
    memcpy(addr, res0->ai_addr, res0->ai_addrlen);
    addr_len = res0->ai_addrlen;
    freeaddrinfo(res0);
    return addr_len;
}

static int udp_socket_create(UDPContext *s, struct sockaddr_storage *addr,
                             int *addr_len, const char *localaddr)
{
    int udp_fd = -1;
    struct addrinfo *res0 = NULL, *res = NULL;
    int family = AF_UNSPEC;

    if (((struct sockaddr *) &s->dest_addr)->sa_family)
        family = ((struct sockaddr *) &s->dest_addr)->sa_family;
    res0 = udp_resolve_host((localaddr && localaddr[0]) ? localaddr : NULL, s->local_port,
                            SOCK_DGRAM, family, AI_PASSIVE);
    if (res0 == 0)
        goto fail;
    for (res = res0; res; res=res->ai_next) {
        if (s->udplite_coverage)
            udp_fd = socket(res->ai_family, SOCK_DGRAM, IPPROTO_UDPLITE);
        else
            udp_fd = socket(res->ai_family, SOCK_DGRAM, 0);
        if (udp_fd > 0) break;
    }

    if (udp_fd < 0)
        goto fail;

    memcpy(addr, res->ai_addr, res->ai_addrlen);
    *addr_len = res->ai_addrlen;
    freeaddrinfo(res0);
    return udp_fd;
 fail:
    if (udp_fd >= 0)
        closesocket(udp_fd);
    if(res0)
        freeaddrinfo(res0);
    return -1;
}

static int udp_port(struct sockaddr_storage *addr, int addr_len)
{
    char sbuf[sizeof(int)*3+1];
    int error;
    if ((error = getnameinfo((struct sockaddr *)addr, addr_len, NULL, 0,  sbuf, sizeof(sbuf), NI_NUMERICSERV)) != 0) {
        gxloge ("getnameinfo: %s\n", gai_strerror(error));
        return -1;
    }
    return strtol(sbuf, NULL, 10);
}

/**
 * If no filename is given to avformat_open_input because you want to
 * get the local port first, then you must call this function to set
 * the remote server address.
 *
 * url syntax: udp://host:port[?option=val...]
 * option: 'ttl=n'       : set the ttl value (for multicast only)
 *         'localport=n' : set the local port
 *         'pkt_size=n'  : set max packet size
 *         'reuse=1'     : enable reusing the socket
 *         'overrun_nonfatal=1': survive in case of circular buffer overrun
 *
 * @param h media file context
 * @param uri of the remote server
 * @return zero if no error.
 */
int ff_udp_set_remote_url(URLContext *h, const char *uri)
{
    UDPContext *s = h->priv_data;
    char hostname[256], buf[10];
    int port;
    const char *p;

    av_url_split(NULL, 0, NULL, 0, hostname, sizeof(hostname), &port, NULL, 0, uri);

    /* set the destination address */
    s->dest_addr_len = udp_set_url(&s->dest_addr, hostname, port);
    if (s->dest_addr_len < 0) {
        return AVERROR(EIO);
    }
    s->is_multicast = ff_is_multicast_address((struct sockaddr*) &s->dest_addr);
    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(buf, sizeof(buf), "connect", p)) {
            int was_connected = s->is_connected;
            s->is_connected = strtol(buf, NULL, 10);
            if (s->is_connected && !was_connected) {
                if (connect(s->udp_fd, (struct sockaddr *) &s->dest_addr, s->dest_addr_len)) {
                    s->is_connected = 0;
                    gxloge ("connect fail.\n");
                    return AVERROR(EIO);
                }
            }
        }
    }
    return 0;
}

/**
 * Return the local port used by the UDP connection
 * @param h media file context
 * @return the local port number
 */
int ff_udp_get_local_port(URLContext *h)
{
    UDPContext *s = h->priv_data;
    return s->local_port;
}

/**
 * Return the udp file handle for select() usage to wait for several RTP
 * streams at the same time.
 * @param h media file context
 */
int udp_get_file_handle(URLContext *h)
{
    UDPContext *s = h->priv_data;
    return s->udp_fd;
}

static void circular_buffer_task_rx( void *_URLContext)
{
    URLContext *h = _URLContext;
    UDPContext *s = h->priv_data;
    unsigned int cost_time;
    int debug_network_stream = 0;
#if !HAVE_PTHREAD_CANCEL
    int ret;
    fd_set rfds;
    struct timeval tv;
#endif

    cost_time = av_get_tick_ms(NULL, 0);
    GxPlayer_SystemGet(PSYS_DEBUG_NETWORK_STREAM, &debug_network_stream);

#if HAVE_PTHREAD_CANCEL
    GxCore_MutexLock(s->mutex);
    if (ff_socket_nonblock(s->udp_fd, 0) < 0) {
        gxloge ("Failed to set nonblock mode\n");
        s->circular_buffer_error = AVERROR(EIO);
        goto end;
    }
#endif
    while(!s->close_req) {
        int len = 0;
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);
        if (url_check_interrupt_cb()) {
            s->circular_buffer_error = AVERROR_EXIT;
            goto end;
        }
#if HAVE_PTHREAD_CANCEL
        GxCore_MutexUnlock(s->mutex);
#else
        FD_ZERO(&rfds);
        FD_SET(s->udp_fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        ret = select(s->udp_fd + 1, &rfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (ff_neterrno() == AVERROR(EINTR))
                continue;
            s->circular_buffer_error = AVERROR(EIO);
            goto end;
        }
        if (!(ret > 0 && FD_ISSET(s->udp_fd, &rfds)))
            continue;
#endif
        len = recv(s->udp_fd, s->tmp+4, sizeof(s->tmp)-4, 0);
#if HAVE_PTHREAD_CANCEL
        GxCore_MutexLock(s->mutex);
#endif
        if (len < 0) {
            if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR)) {
                s->circular_buffer_error = ff_neterrno();
                gxloge ("udp.c rx recv fail. size=%d.(%d)..\n", len, ff_neterrno());
                goto end;
            }
            continue;
        }
        AV_WL32(s->tmp, len);

        if (debug_network_stream) {
            if ((av_get_tick_ms(NULL, 0) - cost_time) > DEBUG_READ_PACKET_TIMES_MS) {
                gxlogi_raw_l ("udp.c 1.5s printf current read len=%d fifo_idle=%d.\n", len, av_fifo_can_write(s->fifo));
                cost_time = av_get_tick_ms(NULL, 0);
            }
        }
        if (av_fifo_can_write(s->fifo) < (len + 4)) {
            /* No Space left */
            if (s->overrun_nonfatal) {
                gxlogi_raw_l ("Circular buffer overrun. cur_read_size:%d, fifo idle:%d[fifo_totol:%d].\n", len, av_fifo_can_write(s->fifo), s->circular_buffer_size);
                //GxCore_CondSignal(s->cond);
#if HAVE_PTHREAD_CANCEL
                GxCore_MutexUnlock(s->mutex);
#endif
                GxCore_ThreadDelay(1000);
#if HAVE_PTHREAD_CANCEL
                GxCore_MutexLock(s->mutex);
#endif
                continue;
            } else {
                s->circular_buffer_error = AVERROR(EIO);
                goto end;
            }
        }
#if !HAVE_PTHREAD_CANCEL
        //GxCore_MutexLock(s->mutex);
        while(GxCore_MutexTrylock(s->mutex) != GXCORE_SUCCESS) {
            GxCore_ThreadDelay(10);
        }
#endif
        av_fifo_write(s->fifo, s->tmp, len + 4);
        GxCore_CondSignal(s->cond);
#if !HAVE_PTHREAD_CANCEL
        GxCore_MutexUnlock(s->mutex);
#endif
    }
end:
#if !HAVE_PTHREAD_CANCEL
    GxCore_MutexLock(s->mutex);
#endif
    GxCore_CondSignal(s->cond);
    GxCore_MutexUnlock(s->mutex);
    return;
}

static void circular_buffer_task_tx( void *_URLContext)
{
    URLContext *h = _URLContext;
    UDPContext *s = h->priv_data;
    int64_t target_timestamp = gx_av_gettime_relative();
    int64_t start_timestamp = gx_av_gettime_relative();
    int64_t sent_bits = 0;
    int64_t burst_interval = s->bitrate ? (s->burst_bits * 1000000 / s->bitrate) : 0;
    int64_t max_delay = s->bitrate ?  ((int64_t)h->max_packet_size * 8 * 1000000 / s->bitrate + 1) : 0;

    GxCore_MutexLock(s->mutex);
    if (ff_socket_nonblock(s->udp_fd, 1) < 0) {
        gxloge ("Failed to set nonblocking mode\n");
        s->circular_buffer_error = AVERROR(EIO);
        goto end;
    }

    for(;;) {
        int len;
        const uint8_t *p;
        uint8_t tmp[4];
        int64_t timestamp;

        len = av_fifo_can_read(s->fifo);
        while (len<4) {
            if (s->close_req)
                goto end;
            GxCore_CondWait(s->cond, s->mutex, -1);
            len = av_fifo_can_read(s->fifo);
        }

        av_fifo_read(s->fifo, tmp, 4);
        len = AV_RL32(tmp);

        if (len < 0) {
            goto end;
        }
        if (len > sizeof(s->tmp)) {
            gxloge ("read len fail. len=%d(%d).\n", len, sizeof(s->tmp));
            goto end;
        }
        av_fifo_read(s->fifo, s->tmp, len);
        GxCore_MutexUnlock(s->mutex);
        if (s->bitrate) {
            timestamp = gx_av_gettime_relative();
            if (timestamp < target_timestamp) {
                int64_t delay = target_timestamp - timestamp;
                    if (delay > max_delay) {
                        delay = max_delay;
                        start_timestamp = timestamp + delay;
                        sent_bits = 0;
                    }
                    GxCore_ThreadDelay(delay/1000);
            } else {
                if (timestamp - burst_interval > target_timestamp) {
                    start_timestamp = timestamp - burst_interval;
                    sent_bits = 0;
                }
            }
            sent_bits += len * 8;
            target_timestamp = start_timestamp + sent_bits * 1000000 / s->bitrate;
        }

        p = s->tmp;
        while (len) {
            int ret;
            if (len <= 0)
                goto end;
            if (!s->is_connected) {
                ret = sendto (s->udp_fd, p, len, 0, (struct sockaddr *) &s->dest_addr, s->dest_addr_len);
            } else {
                ret = send(s->udp_fd, p, len, 0);
            }
            if (ret >= 0) {
                len -= ret;
                p += ret;
            } else {
                ret = ff_neterrno();
                if (ret != AVERROR(EAGAIN) && ret != AVERROR(EINTR)) {
                    GxCore_MutexLock(s->mutex);
                    s->circular_buffer_error = ret;
                    GxCore_MutexUnlock(s->mutex);
                    return;
                }
            }
        }
        GxCore_MutexLock(s->mutex);
    }
end:
    GxCore_MutexUnlock(s->mutex);
    return;
}

static int is_large_system_memory(void)
{
    #define PLAYER_DEFAULT_SYSMEM_SIZE	(8*1024*1024)
    int sysMemSize = 0;
	GxPlayer_SystemGet(PSYS_PACKET_CACHE, &sysMemSize);
    if (sysMemSize >= PLAYER_DEFAULT_SYSMEM_SIZE) {
        return 1;
    }
    return 0;
}

static int udp_open(URLContext *h, const char *uri, int flags)
{
    int port, udp_fd = -1, tmp, bind_ret = -1, ret, is_output;
    UDPContext *s = h->priv_data;
    const char *p;
    char buf[256], hostname[1024];
    struct sockaddr_storage my_addr;
    int len, reuse_specified = 0, usr_recvbuffer_size = 0, min_recvbuffer_size = 0;
    AVDictionaryEntry *e = NULL;

    h->is_streamed = 1;
    s->pkt_size = 1472;
    is_output = !(flags & AVIO_FLAG_READ);
    s->ttl = 16;
    s->close_req = 0;
    s->overrun_nonfatal = 1;
    s->circular_buffer_size = is_large_system_memory()? UDP_BIG_MEM_FIFO_SIZE : UDP_SMALL_MEM_FIFO_SIZE; //memory >8M :set fifo_size:5M,other:1M.

    min_recvbuffer_size = is_output ? UDP_TX_BUF_SIZE : UDP_RX_BUF_SIZE;
    GxPlayer_SystemGet(PSYS_NETWORK_RECVBUF, &usr_recvbuffer_size);

    s->is_open_udp_cache_pthread = 1;
    if ((e = av_dict_get(h->url_context_options, "nobuffer", NULL, 0))) {
        s->is_open_udp_cache_pthread = atoi(e->value)?0:1;
    }

    if (usr_recvbuffer_size <= min_recvbuffer_size) {
        s->buffer_size = min_recvbuffer_size;
        gxlogi("Info! set network_recvbuf_size default size (%d)\n", s->buffer_size);
    } else {
        s->buffer_size = usr_recvbuffer_size;
        gxlogi("Info! set network_recvbuf_size user size (%d)\n", s->buffer_size);
    }

    if ((e = av_dict_get(h->url_context_options, "set_so_recvbuf_stream_type", NULL, 0))) {
        if (0 == strcasecmp(e->value, "av_rtcp"))
            s->buffer_size = s->buffer_size/24; //16KB
        else if (0 == strcasecmp(e->value, "audio_rtp"))
            s->buffer_size = s->buffer_size/12; //32KB
    }

    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(buf, sizeof(buf), "reuse", p)) {
            char *endptr = NULL;
            s->reuse_socket = strtol(buf, &endptr, 10);
            /* assume if no digits were found it is a request to enable it */
            if (buf == endptr)
                s->reuse_socket = 1;
            reuse_specified = 1;
        }
        if (av_find_info_tag(buf, sizeof(buf), "overrun_nonfatal", p)) {
            char *endptr = NULL;
            s->overrun_nonfatal = strtol(buf, &endptr, 10);
            /* assume if no digits were found it is a request to enable it */
            if (buf == endptr)
                s->overrun_nonfatal = 1;
        }
        if (av_find_info_tag(buf, sizeof(buf), "ttl", p)) {
            s->ttl = strtol(buf, NULL, 10);
            if (s->ttl < 0 || s->ttl > 255) {
                gxloge ("ttl(%d) should be in range [0,255]\n", s->ttl);
                ret = AVERROR(EINVAL);
                goto fail;
            }
        }
        if (av_find_info_tag(buf, sizeof(buf), "udplite_coverage", p)) {
            s->udplite_coverage = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "localport", p)) {
            s->local_port = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "pkt_size", p)) {
            h->max_packet_size = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "buffer_size", p)) {
            s->buffer_size = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "connect", p)) {
            s->is_connected = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "fifo_size", p)) {
            s->circular_buffer_size = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "localaddr", p)) {
            if (s->localaddr) {
                av_free(s->localaddr);
                s->localaddr = NULL;
            }
            s->localaddr = av_strdup(buf);
            if (!s->localaddr) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
        }
    } else {
        if ((e = av_dict_get(h->url_context_options, "fifo_size", NULL, 0))) {
            s->circular_buffer_size = strtol(e->value, NULL, 10);
        }
    }
    s->circular_buffer_size *= 188;
    if (s->is_open_udp_cache_pthread) {
        h->max_packet_size = (flags & AVIO_FLAG_WRITE)?s->pkt_size:UDP_MAX_PKT_SIZE;
    } else {
        h->max_packet_size = s->pkt_size;
    }
    /* fill the dest addr */
    av_url_split(NULL, 0, NULL, 0, hostname, sizeof(hostname), &port, NULL, 0, uri);
    /* XXX: fix av_url_split */
    if (hostname[0] == '\0' || hostname[0] == '?') {
        /* only accepts null hostname if input */
        if (!(flags & AVIO_FLAG_READ)) {
            ret = AVERROR(EINVAL);
            goto fail;
        }
    } else {
        if ((ret = ff_udp_set_remote_url(h, uri)) < 0)
            goto fail;
    }

    if ((s->is_multicast || !s->local_port) && (h->flags & AVIO_FLAG_READ))
        s->local_port = port;
    udp_fd = udp_socket_create(s, &my_addr, &len, s->localaddr);
    if (udp_fd < 0) {
        ret = AVERROR(EIO);
        goto fail;
    }

    s->local_addr_storage=my_addr; //store for future multicast join

    /* Follow the requested reuse option, unless it's multicast in which
     * case enable reuse unless explicitly disabled.
     */
    if (s->reuse_socket || (s->is_multicast && !reuse_specified)) {
        s->reuse_socket = 1;
        if (setsockopt (udp_fd, SOL_SOCKET, SO_REUSEADDR, &(s->reuse_socket), sizeof(s->reuse_socket)) != 0) {
            ret = ff_neterrno();
            goto fail;
        }
    }

    /* Set the checksum coverage for UDP-Lite (RFC 3828) for sending and receiving.
     * The receiver coverage has to be less than or equal to the sender coverage.
     * Otherwise, the receiver will drop all packets.
     */
   if (s->udplite_coverage) {
        setsockopt (udp_fd, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV, &(s->udplite_coverage), sizeof(s->udplite_coverage));
        setsockopt (udp_fd, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV, &(s->udplite_coverage), sizeof(s->udplite_coverage));
   }

    /* If multicast, try binding the multicast address first, to avoid
     * receiving UDP packets from other sources aimed at the same UDP
     * port. This fails on windows. This makes sending to the same address
     * using sendto() fail, so only do it if we're opened in read-only mode. */
    if (s->is_multicast && (h->flags & AVIO_FLAG_READ)) {
        bind_ret = bind(udp_fd,(struct sockaddr *)&s->dest_addr, len);
    }
    /* bind to the local address if not multicast or if the multicast
     * bind failed */
    /* the bind is needed to give a port to the socket now */
    if (bind_ret < 0 && bind(udp_fd,(struct sockaddr *)&my_addr, len) < 0) {
        ret = ff_neterrno();
        gxloge ("bind failed ret=%d\n", ret);
        goto fail;
    }

    len = sizeof(my_addr);
    getsockname(udp_fd, (struct sockaddr *)&my_addr, (socklen_t *)(&len));
    s->local_port = udp_port(&my_addr, len);

    if (s->is_multicast) {
        if (h->flags & AVIO_FLAG_WRITE) {
            /* output */
            if ((ret = udp_set_multicast_ttl(udp_fd, s->ttl, (struct sockaddr *)&s->dest_addr)) < 0)
                goto fail;
        }
        if (h->flags & AVIO_FLAG_READ) {
            /* input */
            if ((ret= udp_join_multicast_group(udp_fd, (struct sockaddr *)&s->dest_addr, (struct sockaddr *)&s->local_addr_storage)) < 0)
                goto fail;
        }
    }

    tmp = s->buffer_size;
    if (setsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &tmp, sizeof(tmp)) < 0) {
        gxloge ("setsockopt(SO_RECVBUF) fail size=%d.\n", tmp);
    }

    if (!is_output) {
        /* make the socket non-blocking */
        ff_socket_nonblock(udp_fd, 1);
    }
    if (s->is_connected) {
        if (connect(udp_fd, (struct sockaddr *) &s->dest_addr, s->dest_addr_len)) {
            ret = ff_neterrno();
            gxloge ("connect fail..ret=%d.\n", ret);
            goto fail;
        }
    }

#if STREAM_UDP_PVR_DEBUG
    if(s->download_stream_fp)
        fclose(s->download_stream_fp);
    s->download_stream_fp = fopen(stream_udp_pvr_file,"wb+");
#endif

    s->udp_fd = udp_fd;

    if (s->is_open_udp_cache_pthread && ((!is_output && s->circular_buffer_size) || (is_output && s->bitrate && s->circular_buffer_size))) {
        /* start the task going */
        s->fifo = av_fifo_alloc2(s->circular_buffer_size, 1, 0);
        if (!s->fifo) {
            ret = AVERROR(ENOMEM);
            gxloge ("fifo malloc2 failed\n");
            goto fail;
        }
        ret = GxCore_MutexCreate(&(s->mutex));
        if (ret != 0) {
            gxloge ("pthread_mutex_init failed: %s\n", strerror(ret));
            ret = AVERROR(ret);
            goto fail;
        }
        ret = GxCore_CondInit(&s->cond);
        if (ret != 0) {
            gxloge ("pthread_cond_init failed: %s\n", strerror(ret));
            ret = AVERROR(ret);
            goto cond_fail;
        }
        ret = GxCore_ThreadCreate("app_udp_rtx", &s->circular_buffer_thread, (is_output?circular_buffer_task_tx:circular_buffer_task_rx), h, 32*1024, GXOS_DEFAULT_PRIORITY);
        if (ret != 0) {
            gxloge ("GxCore_ThreadCreate failed : %s\n", strerror(ret));
            ret = AVERROR(ret);
            goto thread_fail;
        }
        s->thread_started = 1;
    }
    if (s->localaddr) {
        av_free(s->localaddr);
        s->localaddr = NULL;
    }
    return 0;
    if (s->is_open_udp_cache_pthread) {
thread_fail:
        GxCore_CondDelete(s->cond);
cond_fail:
        GxCore_MutexDelete(s->mutex);
    }
fail:
    if (udp_fd >= 0)
        closesocket(udp_fd);
    if (s->is_open_udp_cache_pthread)
        av_fifo_freep2(&s->fifo);
    if (s->localaddr) {
        av_free(s->localaddr);
        s->localaddr = NULL;
    }
    return ret;
}

static int udplite_open(URLContext *h, const char *uri, int flags)
{
    UDPContext *s = h->priv_data;
    // set default checksum coverage
    s->udplite_coverage = UDP_HEADER_SIZE;
    return udp_open(h, uri, flags);
}

static int udp_read(URLContext *h, uint8_t *buf, int size)
{
    UDPContext *s = h->priv_data;
    int ret = 0;
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (s->is_open_udp_cache_pthread && s->fifo) {
        int avail = 0, nonblock = h->flags & AVIO_FLAG_NONBLOCK;
        //GxCore_MutexLock(s->mutex);
        while(GxCore_MutexTrylock(s->mutex) != GXCORE_SUCCESS) {
            GxCore_ThreadDelay(10);
        }
        do {
            avail = av_fifo_can_read(s->fifo);
            if (avail) {
                uint8_t tmp[4];
                av_fifo_read(s->fifo, tmp, 4);
                avail = AV_RL32(tmp);
                if(avail > size){
                    gxlogi_raw_l ("Part of datagram lost due to insufficient buffer size.[%d.%d]\n", avail, size);
                    avail = size;
                }
                av_fifo_read(s->fifo, buf, avail);
                av_fifo_drain2(s->fifo, AV_RL32(tmp) - avail);
                GxCore_MutexUnlock(s->mutex);
#if STREAM_UDP_PVR_DEBUG
                fwrite(buf, avail, 1, s->download_stream_fp);
#endif
                return avail;
            } else if(s->circular_buffer_error){
                int err = s->circular_buffer_error;
                GxCore_MutexUnlock(s->mutex);
                return err;
            } else if(nonblock) {
                GxCore_MutexUnlock(s->mutex);
                return AVERROR(EAGAIN);
            } else {
                /* FIXME: using the monotonic clock would be better, but it does not exist on all supported platforms. */
                int err = GxCore_CondWait(s->cond, s->mutex, 200);
                if (err) {
                    GxCore_MutexUnlock(s->mutex);
                    return AVERROR(EAGAIN);
                }
                nonblock = 1;
            }
        } while(1);
    }
#ifdef LINUX_OS
UDP_READ_AGAIN:
    if(s->udp_data_len > 0) {
        int recv_size = s->udp_data_len>size?size:s->udp_data_len;
        memcpy(buf, s->udp_data+s->udp_data_pos, recv_size);
        s->udp_data_len -= recv_size;
        s->udp_data_pos += recv_size;
        return recv_size;
    }
#endif
    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd(s->udp_fd, 0);
        if (ret < 0)
            return ret;
    }
#ifdef LINUX_OS
    unsigned int linux_os_version;
    GxPlayer_SystemGet(PSYS_SYSTEM_VERSION, &linux_os_version);
    if (linux_os_version >= KERNEL_VERSION(2,6,33)) {
        int num = size/(STREAM_UDP_MAX_PKT_SIZE);
        struct mmsghdr msgs[STREAM_UDP_MAX_COUNT];
        struct iovec iovecs[STREAM_UDP_MAX_COUNT];
        struct timespec timeout;

        timeout.tv_sec = 1;
        timeout.tv_nsec = 0;
        num = FFMIN(num, STREAM_UDP_MAX_COUNT);
        memset(msgs, 0, sizeof(msgs));
        if(num == 0) {
            iovecs[0].iov_base = s->udp_data;
            iovecs[0].iov_len  = STREAM_UDP_MAX_PKT_SIZE;
            msgs[0].msg_hdr.msg_iov = &iovecs[0];
            msgs[0].msg_hdr.msg_iovlen = 1;
            if(recvmmsg(s->udp_fd, msgs, 1, 0x10000, &timeout) <= 0) {
                gxloge ("udp.c recvmmsg fail.ret=%d.\n", ret);
                return 0 ;
            }
            s->udp_data_len = msgs[0].msg_len;
            s->udp_data_pos = 0;
            goto UDP_READ_AGAIN;
        } else {
            char **udp_buf = NULL;
            int i = 0, recv_size = 0;
            udp_buf=(char ** )av_mallocz(sizeof(char *)*num);
            if (!udp_buf) {
                goto fail;
            }
            for(i = 0; i < num; i++) {
                udp_buf[i]=av_mallocz(sizeof(char)*STREAM_UDP_MAX_PKT_SIZE);
                if (!(udp_buf[i])) {
                    goto fail;
                }
            }
            for (i = 0; i < num; i++) {
                iovecs[i].iov_base = udp_buf[i];
                iovecs[i].iov_len  = STREAM_UDP_MAX_PKT_SIZE;
                msgs[i].msg_hdr.msg_iov = &iovecs[i];
                msgs[i].msg_hdr.msg_iovlen = 1;
            }
            if((ret = recvmmsg(s->udp_fd, msgs, num, 0x10000, &timeout)) <= 0) {
                goto fail;
            }
            for(i = 0; i < ret; i++) {
                memcpy(buf+recv_size, udp_buf[i],  msgs[i].msg_len);
                recv_size += msgs[i].msg_len;
            }
            if (ret != num) {
                GxCore_ThreadDelay(10);
            }
fail:
            for(i = 0; i < num; i++) {
                if (udp_buf[i]) {
                    av_free(udp_buf[i]);
                    udp_buf[i] = NULL;
                }
            }
            if (udp_buf) {
                av_free(udp_buf);
                udp_buf = NULL;
            }
#if STREAM_UDP_PVR_DEBUG
            if (ret > 0 && s->download_stream_fp)
                fwrite(buf, recv_size, 1, s->download_stream_fp);
#endif
            return recv_size;
        }
    }
#endif
    ret = recv(s->udp_fd, buf, size, 0);
#if STREAM_UDP_PVR_DEBUG
    if (ret > 0 && s->download_stream_fp)
        fwrite(buf, ret, 1, s->download_stream_fp);
#endif
    return ret < 0 ? ff_neterrno() : ret;
}

static int udp_write(URLContext *h, uint8_t *buf, int size)
{
    UDPContext *s = h->priv_data;
    int ret;
    if (s->is_open_udp_cache_pthread && s->fifo) {
        uint8_t tmp[4];
        GxCore_MutexLock(s->mutex);
        /* Return error if last tx failed.
           Here we can't know on which packet error was, but it needs to know that error exists.
        */
        if (s->circular_buffer_error<0) {
            int err = s->circular_buffer_error;
            GxCore_MutexUnlock(s->mutex);
            return err;
        }
        if (av_fifo_can_write(s->fifo) < size + 4) {
            /* What about a partial packet tx ? */
            GxCore_MutexUnlock(s->mutex);
            return AVERROR(ENOMEM);
        }
        AV_WL32(tmp, size);
        av_fifo_write(s->fifo, tmp, 4); /* size of packet */
        av_fifo_write(s->fifo, buf, size); /* the data */
        GxCore_CondSignal(s->cond);
        GxCore_MutexUnlock(s->mutex);
        return size;
    }

    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd(s->udp_fd, 1);
        if (ret < 0)
            return ret;
    }

    if (!s->is_connected) {
        ret = sendto (s->udp_fd, buf, size, 0, (struct sockaddr *) &s->dest_addr, s->dest_addr_len);
    } else {
        ret = send(s->udp_fd, buf, size, 0);
    }
    return ret < 0 ? ff_neterrno() : ret;
}

static int udp_close(URLContext *h)
{
    UDPContext *s = h->priv_data;
    // Request close once writing is finished
    if (s->is_open_udp_cache_pthread && (s->thread_started && !(h->flags & AVIO_FLAG_READ))) {
        GxCore_MutexLock(s->mutex);
        s->close_req = 1;
        GxCore_CondSignal(s->cond);
        GxCore_MutexUnlock(s->mutex);
    }

#if STREAM_UDP_PVR_DEBUG
    fclose(s->download_stream_fp);
#endif
    if (s->is_multicast && (h->flags != URL_WRONLY))
        udp_leave_multicast_group(s->udp_fd, (struct sockaddr *)&s->dest_addr, (struct sockaddr *)&s->local_addr_storage);

    if (s->is_open_udp_cache_pthread && s->thread_started) {
        s->close_req = 1;
        // Cancel only read, as write has been signaled as success to the user
        GxCore_ThreadJoin(s->circular_buffer_thread);
        GxCore_MutexDelete(s->mutex);
        GxCore_CondDelete(s->cond);
    }
    closesocket(s->udp_fd);
    if (s->is_open_udp_cache_pthread)
        av_fifo_freep2(&s->fifo);
    return 0;
}

URLProtocol gx_udp_protocol = {
    .name                = "udp",
    .url_open            = udp_open,
    .url_read            = udp_read,
    .url_write           = udp_write,
    .url_close           = udp_close,
    .url_get_file_handle = udp_get_file_handle,
    .priv_data_size      = sizeof(UDPContext),
};

URLProtocol gx_udplite_protocol = {
    .name                = "udplite",
    .url_open            = udplite_open,
    .url_read            = udp_read,
    .url_write           = udp_write,
    .url_close           = udp_close,
    .url_get_file_handle = udp_get_file_handle,
    .priv_data_size      = sizeof(UDPContext),
};

