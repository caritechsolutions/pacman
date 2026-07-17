/*
 * TCP protocol
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
#include "avformat.h"
#include <unistd.h>
#include "network.h"
#include "gx_poll.h"
#include <sys/time.h>
#include <netinet/tcp.h>
#ifdef ECOS_OS
#include <sys/sockio.h>
#endif

#define TCP_TIMEOUT 5000000
#define TCP_OFFSET 1024
typedef struct TCPContext {
    int fd;
    int listen;
    int rw_timeout;
    int open_timeout;
    int listen_timeout;
    int recv_buffer_size;
    int send_buffer_size;
    int tcp_nodelay;
} TCPContext;

typedef struct {
    char buf[256]; // 256 byte
    char portstr[10]; // 10 byte
    char hostname[TCP_OFFSET]; // 1024 byte
    char proto[TCP_OFFSET]; // 1024 byte
    char path[TCP_OFFSET]; // 1024 byte
}tcp_url_param;

static int ff_tcp_getaddrinfo(TCPContext *s, tcp_url_param *url_param, struct addrinfo **ai, int is_proxy, tcp_url_param *proxy_param)
{
    int retry_dns_request = 4;
    int ret = 0;
    struct addrinfo hints = {0};
    do {
        if (url_check_interrupt_cb()) {
            return AVERROR_EXIT;
        }
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (s->listen)
            hints.ai_flags |= AI_PASSIVE;
        if (is_proxy)
            ret = getaddrinfo(proxy_param->hostname, proxy_param->portstr, &hints, ai);
        else
            ret = getaddrinfo(url_param->hostname, url_param->portstr, &hints, ai);
        if (ret) {
            GxCore_ThreadDelay(50); // delay 50ms
            if (1 == GxCore_IfSupportIPV6()) {
                hints.ai_family = AF_INET6;
                if (is_proxy)
                    ret = getaddrinfo(proxy_param->hostname, proxy_param->portstr, &hints, ai);
                else
                    ret = getaddrinfo(url_param->hostname, url_param->portstr, &hints, ai);
                GxCore_ThreadDelay(50);// delay 50ms
            }
        }
        retry_dns_request -= 1;
    } while ((retry_dns_request > 0) && ret);

    if ((retry_dns_request <= 0) && ret) {
        gxlogi_raw_l ("getaddrinfo retry:3,failed!,ret=%d.Error:[%s]\n", ret, gai_strerror(ret));
        return AVERROR(EIO);
    }
    return 0;
}

static void customize_fd(void *ctx, int fd, int enable_linger)
{
    URLContext *h = ctx;
    TCPContext *s = h->priv_data;
    /* Set the socket's send or receive buffer sizes, if specified.
       If unspecified or setting fails, system default is used. */
    int recv_buf_size = 0;
    AVDictionaryEntry *e = NULL;
    if ((e = av_dict_get(h->url_context_options, "ff_recvbuf_size", NULL, 0))) {
        recv_buf_size = atoi(e->value);
    } else {
        GxPlayer_SystemGet(PSYS_NETWORK_RECVBUF, &recv_buf_size);
    }
    s->recv_buffer_size = recv_buf_size;
    if (s->recv_buffer_size > 0) {
        if (setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &s->recv_buffer_size, sizeof (s->recv_buffer_size))) {
            gxlogi ("setsockopt(SO_RCVBUF)\n");
        }
    }
    if (s->send_buffer_size > 0) {
        if (setsockopt (fd, SOL_SOCKET, SO_SNDBUF, &s->send_buffer_size, sizeof (s->send_buffer_size))) {
            gxlogi ("setsockopt(SO_SNDBUF)\n");
        }
    }
    if (enable_linger) {
        struct linger so_linger;
        so_linger.l_onoff  = 1;
#ifdef LINUX_OS
        so_linger.l_linger = 1; // posix: The unit is 1 sec.
#else
        so_linger.l_linger = 0; // bsd: The unit is 1 tick(10ms).
#endif
        if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger))) {
            gxlogi ("setsockopt(SO_LINGER) fail.\n");
        }
    }

    if (s->tcp_nodelay > 0) {
        if (setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &s->tcp_nodelay, sizeof (s->tcp_nodelay))) {
            gxlogi ("setsockopt(TCP_NODELAY) fail.\n");
        }
    }
#if 0
    if (s->tcp_mss > 0) {
        if (setsockopt (fd, IPPROTO_TCP, TCP_MAXSEG, &s->tcp_mss, sizeof (s->tcp_mss))) {
            gxlogi ("setsockopt(TCP_MAXSEG) fail.\n");
        }
    }
#endif
}

static int tcp_open(URLContext *h, const char *uri, int flags)
{
    struct addrinfo *ai = NULL, *cur_ai = NULL;
    TCPContext *s = h->priv_data;
    socklen_t optlen;
    const char *p;
    AVDictionaryEntry *e = NULL;
    int ret = -1, sys_timeout = -1, port = 0, fd = -1, is_proxy = 0;
    tcp_url_param *url_param = NULL;
    tcp_url_param *proxy_param = NULL;

    if (!(url_param = av_mallocz(sizeof(tcp_url_param))))
        return AVERROR(ENOMEM);

    /* current just processing the socks5 non authentication */
    if ((e = av_dict_get(h->url_context_options, "socks5_proxy", NULL, 0))) {
        if (!(proxy_param = av_mallocz(sizeof(tcp_url_param)))) {
            if (url_param)
                av_freep(&url_param);
            return AVERROR(ENOMEM);
        }
        is_proxy = e->value && av_strstart(e->value, "socks5://", NULL);
        av_url_split(proxy_param->proto, sizeof(proxy_param->proto)-1, NULL, 0, proxy_param->hostname, sizeof(proxy_param->hostname)-1,
                &port, proxy_param->path, sizeof(proxy_param->path), e->value);
        port = (port > 0 && port < 65536) ? port : 1080;
        snprintf(proxy_param->portstr, sizeof(proxy_param->portstr)-1, "%d", port);
    }

    av_url_split(url_param->proto, sizeof(url_param->proto)-1, NULL, 0, url_param->hostname, sizeof(url_param->hostname)-1,
            &port, url_param->path, sizeof(url_param->path)-1, uri);
    if (strcmp(url_param->proto,"tcp") || port <= 0 || port >= 65536) {
        ret = AVERROR(EINVAL);
        goto fail;
    }
    snprintf(url_param->portstr, sizeof(url_param->portstr)-1, "%d", port);
    s->rw_timeout = TCP_TIMEOUT;
    s->tcp_nodelay = 0;
    if ((e = av_dict_get(h->url_context_options, "rw_timeout", NULL, 0))) {
        s->rw_timeout = strtol(e->value, NULL, 10)*100000;
    }
    if (s->rw_timeout >= 0) {
        s->open_timeout = s->rw_timeout;
    }
    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "listen", p)) {
            char *endptr = NULL;
            s->listen = strtol(url_param->buf, &endptr, 10);
            /* assume if no digits were found it is a request to enable it */
            if (url_param->buf == endptr)
                s->listen = 1;
        }
        if (av_find_info_tag(url_param->buf, sizeof(url_param->buf), "timeout", p)) {
            s->rw_timeout = strtol(url_param->buf, NULL, 10);
        }
    }
    /*dns parse.*/
    if (0 > ff_tcp_getaddrinfo(s, url_param, &ai, is_proxy, proxy_param)) {
        ret = AVERROR(EINVAL);
        ai = NULL;
        goto fail;
    }
    cur_ai = ai;
    if (s->listen > 0) {
        while (cur_ai && fd < 0) {
            fd = socket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol);
            if (fd < 0) {
                ret = ff_neterrno();
                cur_ai = cur_ai->ai_next;
            }
        }
        if (fd < 0) {
            ret = AVERROR(EINVAL);
            goto fail;
        }
        customize_fd(s, fd, 1);
    }

    if (s->listen == 2) {
        // multi-client
        if ((ret = ff_listen(fd, cur_ai->ai_addr, cur_ai->ai_addrlen, h)) < 0)
            goto fail;
    } else if (s->listen == 1) {
        // single client
        if ((ret = ff_listen_bind(fd, cur_ai->ai_addr, cur_ai->ai_addrlen, s->listen_timeout, h)) < 0)
            goto fail;
        // Socket descriptor already closed here. Safe to overwrite to client one.
        fd = ret;
    } else {
        fd = socket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol);
        if (fd < 0) {
            ret = AVERROR(EINVAL);
            goto fail;
        }
        if ((ret = ff_listen_connect(fd, cur_ai, s->open_timeout/1000, customize_fd, h)) < 0) {
            goto fail;
        }
    }
    h->is_streamed = 1;
    s->fd = fd;
    if(is_proxy) {
        // make socks5 proxy request
        in_addr_t addr;
        int next_pos;
        unsigned char req[1024];
        req[0] = 5;
        req[1] = 1;
        req[2] = 0;

        // send handshake
        if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
            //ret = ff_network_wait_fd_timeout(s->fd, 1, h->rw_timeout);
            ret = ff_network_wait_fd(s->fd, 1);
            if (ret)
                goto fail;
        }
        ret = send(s->fd, req, 3, 0);
        if (ret != 3)
            goto fail;

        // receive handshake response
        if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
            //ret = ff_network_wait_fd_timeout(s->fd, 0, h->rw_timeout);
            ret = ff_network_wait_fd(s->fd, 0);
            if (ret)
                goto fail;
        }
        ret = recv(s->fd, req, 2, 0);
        if (ret != 2) {
            ret = AVERROR_INVALIDDATA;
            goto fail;
        }

        // send connect request
        req[0] = 5;
        req[1] = 1;
        req[2] = 0;
        addr = inet_addr(url_param->hostname);
        req[3] = INADDR_NONE == addr ? 3 : 1;
        next_pos = 0;
        if(req[3] == 3) {
            req[4] = strlen(url_param->hostname);
            memcpy(req + 5, url_param->hostname, strlen(url_param->hostname));
            next_pos = 5 + strlen(url_param->hostname);
        } else {
            memcpy(req + 4, (unsigned char*)&addr, sizeof(addr));
            next_pos = 4 + sizeof(addr);
        }
        *(unsigned short *)(req + next_pos) = htons(port);
        // req[next_pos] = (htons(port) & 0x00FF);
        // req[next_pos + 1] = (htons(port) >> 8);

        if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
            //ret = ff_network_wait_fd_timeout(s->fd, 1, h->rw_timeout);
            ret = ff_network_wait_fd(s->fd, 1);
            if (ret)
                goto fail;
        }
        ret = send(s->fd, req, next_pos + 2, 0);
        if (ret != next_pos + 2)
            goto fail;

        // recv connect response
        if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
            //ret = ff_network_wait_fd_timeout(s->fd, 0, h->rw_timeout);
            ret = ff_network_wait_fd(s->fd, 0);
            if (ret)
                goto fail;
        }
        ret = recv(s->fd, req, 10, 0);
        if (ret != 10) {
            gxloge ("socks5 connect failed bytes %d\n", ret);
            ret = AVERROR_INVALIDDATA;
            goto fail;
        }
    }
    if (ai)
        freeaddrinfo(ai);
    if (url_param) {
        av_freep(&url_param);
    }
    if (proxy_param) {
        av_freep(&proxy_param);
    }
    return 0;
fail:
    if (fd >= 0)
        closesocket(fd);
    if (ai)
        freeaddrinfo(ai);
    if (url_param) {
        av_freep(&url_param);
    }
    if (proxy_param) {
        av_freep(&proxy_param);
    }
    return ret;
}

static int tcp_read(URLContext *h, uint8_t *buf, int size)
{
    TCPContext *s = h->priv_data;
    int ret;

    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        //ret = ff_network_wait_fd_timeout(s->fd, 0, h->rw_timeout);
        //if (ret) {
        ret = ff_network_wait_fd(s->fd, 0);
        if (ret < 0) {
            return ret;
        }
    }
    ret = recv(s->fd, buf, size, 0);
    return ret < 0 ? ff_neterrno() : ret;
}

static int tcp_write(URLContext *h, uint8_t *buf, int size)
{
    TCPContext *s = h->priv_data;
    int ret;

    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd(s->fd, 1);
        //ret = ff_network_wait_fd_timeout(s->fd, 1, h->rw_timeout);
        //if (ret) {
        if (ret < 0) {
            return ret;
        }
    }
    ret = send(s->fd, buf, size, 0);
    return ret < 0 ? ff_neterrno() : ret;
}

static int tcp_shutdown(URLContext *h, int flags)
{
    TCPContext *s = h->priv_data;
    int how = SHUT_RD;
    if (flags == URL_RDWR) {
        how = SHUT_RDWR;
    } else if (flags != URL_RDONLY) {
        how = SHUT_WR;
    }
    return shutdown(s->fd, how);
}

static int tcp_close(URLContext *h)
{
    TCPContext *s = h->priv_data;
    closesocket(s->fd);
    s->fd = -1;
    return 0;
}

static int tcp_get_file_handle(URLContext *h)
{
    TCPContext *s = h->priv_data;
    if (!s)
        return -1;
    return s->fd;
}

URLProtocol gx_tcp_protocol = {
    .name                = "tcp",
    .url_open            = tcp_open,
    .url_read            = tcp_read,
    .url_write           = tcp_write,
    .url_close           = tcp_close,
    .url_get_file_handle = tcp_get_file_handle,
    .url_shutdown        = tcp_shutdown,
    .priv_data_size      = sizeof(TCPContext),
};
