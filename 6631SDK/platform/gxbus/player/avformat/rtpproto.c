/*
 * RTP network protocol
 * Copyright (c) 2002 Fabrice Bellard
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
 * RTP protocol
 */

#include "../avutil/avstring.h"
#include "../avutil/opt.h"
#include "avformat.h"
#include "rtp.h"
#include "rtpproto.h"

#include <stdarg.h>
#include "network.h"
#include <fcntl.h>
#include "gx_poll.h"
#include <sys/time.h>

typedef struct RTPContext {
    URLContext *rtp_hd, *rtcp_hd, *fec_hd;
    int rtp_fd, rtcp_fd;
    //IPSourceFilters filters;
    int write_to_source;
    struct sockaddr_storage last_rtp_source, last_rtcp_source;
    socklen_t last_rtp_source_len, last_rtcp_source_len;
    int ttl;
    int buffer_size;
    int rtcp_port, local_rtpport, local_rtcpport;
    int connect;
    int pkt_size;
    int dscp;
    char *sources;
    char *block;
    char *fec_options_str;
} RTPContext;

#define RTP_MAX_TIMEOUTS 200

/**
 * If no filename is given to av_open_input_file because you want to
 * get the local port first, then you must call this function to set
 * the remote server address.
 *
 * @param h media file context
 * @param uri of the remote server
 * @return zero if no error.
 */

int ff_rtp_set_remote_url(URLContext *h, const char *uri)
{
    RTPContext *s = h->priv_data;
    char hostname[256];
    int port, rtcp_port;
    const char *p;

    char buf[1024];
    char path[1024];

    av_url_split(NULL, 0, NULL, 0, hostname, sizeof(hostname), &port,
                 path, sizeof(path), uri);
    rtcp_port = port + 1;

    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(buf, sizeof(buf), "rtcpport", p)) {
            rtcp_port = strtol(buf, NULL, 10);
        }
    }

    ff_url_join(buf, sizeof(buf), "udp", NULL, hostname, port, "%s", path);
    ff_udp_set_remote_url(s->rtp_hd, buf);

    ff_url_join(buf, sizeof(buf), "udp", NULL, hostname, rtcp_port, "%s", path);
    ff_udp_set_remote_url(s->rtcp_hd, buf);
    return 0;
}

static int get_port(const struct sockaddr_storage *ss)
{
    if (ss->ss_family == AF_INET)
        return ntohs(((const struct sockaddr_in *)ss)->sin_port);
#if 0//HAVE_STRUCT_SOCKADDR_IN6
    if (ss->ss_family == AF_INET6)
        return ntohs(((const struct sockaddr_in6 *)ss)->sin6_port);
#endif
    return 0;
}

static void set_port(struct sockaddr_storage *ss, int port)
{
    if (ss->ss_family == AF_INET)
        ((struct sockaddr_in *)ss)->sin_port = htons(port);
#if 0 && HAVE_STRUCT_SOCKADDR_IN6
    else if (ss->ss_family == AF_INET6)
        ((struct sockaddr_in6 *)ss)->sin6_port = htons(port);
#endif
}

/**
 * add option to url of the form:
 * "http://host:port/path?option1=val1&option2=val2...
 */

//static av_printf_format(3, 4) 
void url_add_option(char *buf, int buf_size, const char *fmt, ...)
{
    char buf1[1024];
    va_list ap;

    va_start(ap, fmt);
    if (strchr(buf, '?'))
        av_strlcat(buf, "&", buf_size);
    else
        av_strlcat(buf, "?", buf_size);
    vsnprintf(buf1, sizeof(buf1), fmt, ap);
    av_strlcat(buf, buf1, buf_size);
    va_end(ap);
}

static void build_udp_url(RTPContext *s,
                          char *buf, int buf_size,
                          const char *hostname,
                          int port, int local_port,
                          const char *include_sources,
                          const char *exclude_sources)
{
    ff_url_join(buf, buf_size, "udp", NULL, hostname, port, NULL);
    if (local_port > 0)
        url_add_option(buf, buf_size, "localport=%d", local_port);
    if (s->ttl > 0)
        url_add_option(buf, buf_size, "ttl=%d", s->ttl);
    if (s->buffer_size > 0)
        url_add_option(buf, buf_size, "buffer_size=%d", s->buffer_size);
    if (s->pkt_size > 0)
        url_add_option(buf, buf_size, "pkt_size=%d", s->pkt_size);
    if (s->connect)
        url_add_option(buf, buf_size, "connect=1");
    if (s->dscp > 0)
        url_add_option(buf, buf_size, "dscp=%d", s->dscp);
    url_add_option(buf, buf_size, "fifo_size=0");
    if (include_sources && include_sources[0])
        url_add_option(buf, buf_size, "sources=%s", include_sources);
    if (exclude_sources && exclude_sources[0])
        url_add_option(buf, buf_size, "block=%s", exclude_sources);
}

/**
 * url syntax: rtp://host:port[?option=val...]
 * option: 'ttl=n'            : set the ttl value (for multicast only)
 *         'rtcpport=n'       : set the remote rtcp port to n
 *         'localrtpport=n'   : set the local rtp port to n
 *         'localrtcpport=n'  : set the local rtcp port to n
 *         'pkt_size=n'       : set max packet size
 *         'connect=0/1'      : do a connect() on the UDP socket
 *         'sources=ip[,ip]'  : list allowed source IP addresses
 *         'block=ip[,ip]'    : list disallowed source IP addresses
 *         'write_to_source=0/1' : send packets to the source address of the latest received packet
 *         'dscp=n'           : set DSCP value to n (QoS)
 * deprecated option:
 *         'localport=n'      : set the local port to n
 *
 * if rtcpport isn't set the rtcp port will be the rtp port + 1
 * if local rtp port isn't set any available port will be used for the local
 * rtp and rtcp ports
 * if the local rtcp port is not set it will be the local rtp port + 1
 */

#define RTP_OFFSET 1024
typedef struct {
    char hostname[RTP_OFFSET/4]; // 256 byte
    char include_sources[RTP_OFFSET]; // 1KB
    char exclude_sources[RTP_OFFSET]; // 1KB
    char buf[RTP_OFFSET]; // 1KB
    char path[RTP_OFFSET]; // 1KB
}rtsp_open_url_param;

static int rtp_open(URLContext *h, const char *uri, int flags)
{
    RTPContext *s = h->priv_data;
    AVDictionary *fec_opts = NULL;
    int rtp_port;
    char *fec_protocol = NULL;
    rtsp_open_url_param *url_param = av_mallocz(sizeof(rtsp_open_url_param));
    if (!url_param)
        goto fail;

    char *sources = url_param->include_sources, *block = url_param->exclude_sources;
    const char *p;
    int i, max_retry_count = 3;
    int rtcpflags;
    int ret = -1;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *e = NULL;

    av_url_split(NULL, 0, NULL, 0, url_param->hostname, sizeof(url_param->hostname), &rtp_port,
                 url_param->path, sizeof(url_param->path), uri);
    /* extract parameters */
    if (s->rtcp_port <= 0)
        s->rtcp_port = rtp_port + 1;

    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "ttl", p)) {
            s->ttl = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "rtcpport", p)) {
            s->rtcp_port = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "localport", p)) {
            s->local_rtpport = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "localrtpport", p)) {
            s->local_rtpport = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "localrtcpport", p)) {
            s->local_rtcpport = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "pkt_size", p)) {
            s->pkt_size = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "connect", p)) {
            s->connect = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "write_to_source", p)) {
            s->write_to_source = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "dscp", p)) {
            s->dscp = strtol(url_param->buf, NULL, 10);
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "sources", p)) {
            av_strlcpy(url_param->include_sources, url_param->buf, sizeof(url_param->include_sources));
            //ff_ip_parse_sources(h, buf, &s->filters);
        } else {
            //ff_ip_parse_sources(h, s->sources, &s->filters);
            sources = s->sources;
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "block", p)) {
            av_strlcpy(url_param->exclude_sources, url_param->buf, sizeof(url_param->exclude_sources));
            //ff_ip_parse_blocks(h, buf, &s->filters);
        } else {
            //ff_ip_parse_blocks(h, s->block, &s->filters);
            block = s->block;
        }
    }

    if (s->fec_options_str) {
        p = s->fec_options_str;

        if (!(fec_protocol = av_get_token(&p, "="))) {
            gxlogd("Failed to parse the FEC protocol value\n");
            goto fail;
        }
        if (strcmp(fec_protocol, "prompeg")) {
            gxlogd("Unsupported FEC protocol %s\n", fec_protocol);
            goto fail;
        }

        p = s->fec_options_str + strlen(fec_protocol);
        while (*p && *p == '=') p++;

        if (av_dict_parse_string(&fec_opts, p, "=", ":", 0) < 0) {
            gxlogd("Failed to parse the FEC options\n");
            goto fail;
        }
        if (s->ttl > 0) {
            snprintf(url_param->buf, sizeof (url_param->buf), "%d", s->ttl);
            av_dict_set(&fec_opts, "ttl", url_param->buf, 0);
        }
    }

    if ((e = av_dict_get(h->url_context_options, "pkt_size", NULL, 0))) {
        s->pkt_size = atoi(e->value);
    } else if ((e = av_dict_get(h->url_context_options, "buffer_size", NULL, 0))) {
        s->buffer_size = atoi(e->value);
    }

    for (i = 0; i < max_retry_count; i++) {
        build_udp_url(s, url_param->buf, sizeof(url_param->buf),
                      url_param->hostname, rtp_port, s->local_rtpport,
                      sources, block);
       if ((e = av_dict_get(h->url_context_options, "stream_type", NULL, 0))) {
            if (0 == strcasecmp(e->value, "audio_rtp"))
                av_dict_set(&opts, "set_so_recvbuf_stream_type", "audio_rtp", 0);
        }
        ret = url_open(&s->rtp_hd, url_param->buf, URL_RDWR, &opts);
        if (opts) {
            av_dict_free(&opts);
            opts = NULL;
        }

        if (ret < 0) {
            goto fail;
        }

        s->local_rtpport = ff_udp_get_local_port(s->rtp_hd);
        if(s->local_rtpport == 65535) {
            s->local_rtpport = -1;
            continue;
        }
        rtcpflags = flags | AVIO_FLAG_WRITE;
        if (s->local_rtcpport <= 0) {
            s->local_rtcpport = s->local_rtpport + 1;
            build_udp_url(s, url_param->buf, sizeof(url_param->buf),
                          url_param->hostname, s->rtcp_port, s->local_rtcpport, sources, block);
            av_dict_set(&opts, "set_so_recvbuf_stream_type", "av_rtcp", 0);
            ret = url_open(&s->rtcp_hd, url_param->buf, URL_RDWR, &opts);
            if (opts) {
                av_dict_free(&opts);
                opts = NULL;
            }
            if (ret < 0) {
                s->local_rtpport = s->local_rtcpport = -1;
                continue;
            }
            break;
        }
        build_udp_url(s, url_param->buf, sizeof(url_param->buf),
                      url_param->hostname, s->rtcp_port, s->local_rtcpport, sources, block);
        av_dict_set(&opts, "set_so_recvbuf_stream_type", "av_rtcp", 0);
        ret = url_open(&s->rtcp_hd, url_param->buf, URL_RDWR, &opts);
        if (opts) {
            av_dict_free(&opts);
            opts = NULL;
        }
        if (ret < 0) {
            goto fail;
        }
        break;
    }

    s->fec_hd = NULL;
    if (fec_protocol) {
        ff_url_join(url_param->buf, sizeof(url_param->buf), fec_protocol, NULL, url_param->hostname, rtp_port, NULL);
            av_dict_set(&opts, "set_so_recvbuf_stream_type", "av_fec", 0);
            ret = url_open(&s->fec_hd, url_param->buf, URL_RDWR, &h->url_context_options);
            if (ret < 0) {
                if (opts) {
                    av_dict_free(&opts);
                    opts = NULL;
                }
                goto fail;
            }
    }

    /* just to ease handle access. XXX: need to suppress direct handle
       access */
    s->rtp_fd = url_get_file_handle(s->rtp_hd);
    s->rtcp_fd = url_get_file_handle(s->rtcp_hd);

    h->max_packet_size = s->rtp_hd->max_packet_size;
    h->is_streamed = 1;

    av_free(fec_protocol);
    av_dict_free(&fec_opts);
    if (opts) {
        av_dict_free(&opts);
        opts = NULL;
    }
    if (url_param) {
        av_free(url_param);
        url_param = NULL;
    }
    return 0;

 fail:
    if (s->rtp_hd)
        ffurl_close(s->rtp_hd);
    if (s->rtcp_hd)
        ffurl_close(s->rtcp_hd);
    url_closep(&s->fec_hd);
    if (fec_protocol)
        av_free(fec_protocol);
    if (fec_opts)
        av_dict_free(&fec_opts);
    if (opts) {
        av_dict_free(&opts);
    }
    if (url_param) {
        av_free(url_param);
        url_param = NULL;
    }
    return AVERROR(EIO);
}

static int rtp_read(URLContext *h, uint8_t *buf, int size)
{
    RTPContext *s = h->priv_data;
    int len = 0, n = 0, i = 0;
    int timeout_cnt = 0;
    struct pollfd p[2] = {{s->rtp_fd, POLLIN, 0}, {s->rtcp_fd, POLLIN, 0}};
    int poll_delay = h->flags & AVIO_FLAG_NONBLOCK ? 0 : 100;
    struct sockaddr_storage *addrs[2] = { &s->last_rtp_source, &s->last_rtcp_source };
    socklen_t *addr_lens[2] = { &s->last_rtp_source_len, &s->last_rtcp_source_len };

    for(;;) {
       if (url_check_interrupt_cb())
           return AVERROR_EXIT;
        n = poll(p, 2, poll_delay);
        if (n > 0) {
            timeout_cnt = 0;
            /* first try RTCP, then RTP */
            for (i = 1; i >= 0; i--) {
                if (!(p[i].revents & POLLIN))
                    continue;
                *addr_lens[i] = sizeof(*addrs[i]);
                len = recvfrom(p[i].fd, buf, size, 0, (struct sockaddr *)addrs[i], addr_lens[i]);
                //len = recv(p[i].fd, buf, size, 0);
                if (len < 0) {
                    if (ff_neterrno() == AVERROR(EAGAIN) ||
                        ff_neterrno() == AVERROR(EINTR))
                        continue;
                    return AVERROR(EIO);
                }
                return len;
            }
        } else if (n < 0) {
            if (ff_neterrno() == AVERROR(EINTR))
                continue;
            return AVERROR(EIO);
        } else if ((0 == n) && (++timeout_cnt >= RTP_MAX_TIMEOUTS)) {
            return AVERROR(ETIMEDOUT);
        }
        if (h->flags & AVIO_FLAG_NONBLOCK) {
            return AVERROR(EAGAIN);
        }
    }
}

static int rtp_write(URLContext *h, uint8_t *buf, int size)
{
    RTPContext *s = h->priv_data;
    int ret, ret_fec;
    URLContext *hd;

    if (size < 2)
        return AVERROR(EINVAL);

    if ((buf[0] & 0xc0) != (RTP_VERSION << 6))
        gxlogd ("Data doesn't look like RTP packets,make sure the RTP muxer is used\n");

    if (s->write_to_source) {
        int fd;
        struct sockaddr_storage *source, temp_source;
        socklen_t *source_len, temp_len;
        if (!s->last_rtp_source.ss_family && !s->last_rtcp_source.ss_family) {
            // Intentionally not returning an error here
            return size;
        }

        if (RTP_PT_IS_RTCP(buf[1])) {
            fd = s->rtcp_fd;
            source     = &s->last_rtcp_source;
            source_len = &s->last_rtcp_source_len;
        } else {
            fd = s->rtp_fd;
            source     = &s->last_rtp_source;
            source_len = &s->last_rtp_source_len;
        }
        if (!source->ss_family) {
            source      = &temp_source;
            source_len  = &temp_len;
            if (RTP_PT_IS_RTCP(buf[1])) {
                temp_source = s->last_rtp_source;
                temp_len    = s->last_rtp_source_len;
                set_port(source, get_port(source) + 1);
            } else {
                temp_source = s->last_rtcp_source;
                temp_len    = s->last_rtcp_source_len;
                set_port(source, get_port(source) - 1);
            }
        }

        if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
            ret = ff_network_wait_fd(fd, 1);
            if (ret < 0)
                return ret;
        }
        ret = sendto(fd, buf, size, 0, (struct sockaddr *) source,
                     *source_len);

        return ret < 0 ? ff_neterrno() : ret;
    }

    if (RTP_PT_IS_RTCP(buf[1])) {
        /* RTCP payload type */
        hd = s->rtcp_hd;
    } else {
        /* RTP payload type */
        hd = s->rtp_hd;
    }

    if ((ret = url_write(hd, buf, size)) < 0) {
        return ret;
    }

    if (s->fec_hd && !RTP_PT_IS_RTCP(buf[1])) {
        if ((ret_fec = url_write(s->fec_hd, buf, size)) < 0) {
            gxlogd("Failed to send FEC\n");
            return ret_fec;
        }
    }

    return ret;
}

static int rtp_close(URLContext *h)
{
    RTPContext *s = h->priv_data;
#if 0
    ff_ip_reset_filters(&s->filters);
#endif
    ffurl_close(s->rtp_hd);
    ffurl_close(s->rtcp_hd);
    url_closep(&s->fec_hd);
    return 0;
}

/**
 * Return the local rtp port used by the RTP connection
 * @param h media file context
 * @return the local port number
 */

int ff_rtp_get_local_rtp_port(URLContext *h)
{
    RTPContext *s = h->priv_data;
    if (!s)
        return -1;
    return ff_udp_get_local_port(s->rtp_hd);
}

/**
 * Return the local rtcp port used by the RTP connection
 * @param h media file context
 * @return the local port number
 */

static int rtp_get_file_handle(URLContext *h)
{
    RTPContext *s = h->priv_data;
    return s->rtp_fd;
}

static int rtp_get_multi_file_handle(URLContext *h, int **handles,
                                     int *numhandles)
{
    RTPContext *s = h->priv_data;
    int *hs       = *handles = av_malloc(sizeof(**handles) * 2);
    if (!hs)
        return AVERROR(ENOMEM);
    hs[0] = s->rtp_fd;
    hs[1] = s->rtcp_fd;
    *numhandles = 2;
    return 0;
}

URLProtocol gx_rtp_protocol = {
    .name                      = "rtp",
    .url_open                  = rtp_open,
    .url_read                  = rtp_read,
    .url_write                 = rtp_write,
    .url_close                 = rtp_close,
    .url_get_file_handle       = rtp_get_file_handle,
    .url_get_multi_file_handle = rtp_get_multi_file_handle,
    .priv_data_size            = sizeof(RTPContext),
};
