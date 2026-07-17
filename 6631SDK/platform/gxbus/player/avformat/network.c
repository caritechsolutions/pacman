/*
 * Copyright (c) 2007 The Libav Project
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "../avutil/avutil.h"
#include "avcodec.h"
#include "network.h"
#include <pthread.h>
#include "avio.h"
#include "avformat.h"
#ifndef LINUX_OS
#include "gx_poll.h"
#endif

#ifdef CONFIG_DEMUX_TLS
#if defined(CONFIG_HAVE_WOLFSSL)
#include <cyassl/openssl/ssl.h>
#include <cyassl/openssl/crypto.h>
#elif defined(CONFIG_HAVE_OPENSSL)
#include <openssl/ssl.h>
#include <openssl/crypto.h>
#endif
static int openssl_init;
pthread_mutex_t *openssl_mutexes;
static void openssl_lock(int mode, int type, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(&openssl_mutexes[type]);
    else
        pthread_mutex_unlock(&openssl_mutexes[type]);
}

static unsigned long openssl_thread_id(void)
{
    return (intptr_t) pthread_self();
}
#endif

void ff_tls_init(void)
{
#ifdef CONFIG_DEMUX_TLS
    if (!openssl_init) {
        int i;
        SSL_library_init();
        SSL_load_error_strings();

        openssl_mutexes = av_mallocz(sizeof(pthread_mutex_t)*CRYPTO_num_locks());
        if (!openssl_mutexes)
            return;
        for (i = 0; i < CRYPTO_num_locks(); i++)
            pthread_mutex_init(&openssl_mutexes[i], NULL);
        CRYPTO_set_locking_callback(openssl_lock);
        CRYPTO_set_id_callback(openssl_thread_id);
    }
    openssl_init++;
#endif
    return;
}

void ff_tls_deinit(void)
{
#ifdef CONFIG_DEMUX_TLS
	openssl_init--;
	if (!openssl_init) {
		int i;
		CRYPTO_set_locking_callback(NULL);
		for (i = 0; i < CRYPTO_num_locks(); i++)
			pthread_mutex_destroy(&openssl_mutexes[i]);
		av_free(openssl_mutexes);
	}
#endif
	return;
}

int ff_socket_nonblock(int socket, int enable)
{
#ifdef LINUX_OS
    if (enable)
        return fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
    else
        return fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) & ~O_NONBLOCK);
#else
    int val;
    val = enable?1:0;
    ioctl(socket, FIONBIO, &val);
#endif
    return 0;
}

int ff_network_init(void)
{
    return 1;
}

int ff_network_wait_fd(int fd, int write)
{
    int ev = write ? POLLOUT : POLLIN;
    struct pollfd p = { .fd = fd, .events = ev, .revents = 0 };
    int ret;
    ret = poll(&p, 1, POLLING_TIME);
    return ((ret < 0) ? ff_neterrno() : p.revents & (ev | POLLERR | POLLHUP)) ? 0 : AVERROR(EAGAIN);
}

int ff_network_wait_fd_timeout(int fd, int write, int64_t timeout)
{
    int ret = 0;
    int64_t wait_start = 0;

    while (1) {
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
        ret = ff_network_wait_fd(fd, write);
        if (ret != AVERROR(EAGAIN)) {
            return ret;
        }
        if (timeout > 0) {
            if (!wait_start)
                wait_start = gx_av_gettime_relative();
            else if (gx_av_gettime_relative() - wait_start > timeout) {
                return AVERROR(ETIMEDOUT);
            }
        }
    }
	return ret;
}

static int ff_poll_interrupt(struct pollfd *p, int nfds, int timeout)
{
    int runs = timeout / POLLING_TIME;
    int ret = 0;

    do {
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
        ret = poll(p, nfds, POLLING_TIME);
        if (ret != 0) {
            if (ret < 0)
                ret = ff_neterrno();
            if (ret == AVERROR(EINTR))
                continue;
            break;
        }
    } while (timeout <= 0 || runs-- > 0);

    if (!ret)
        return AVERROR(ETIMEDOUT);
    return ret;
}

int ff_listen_connect(int fd, struct addrinfo *cur_ai, int timeout, void (*customize_fd)(void *, int, int), void *customize_ctx)
{
    struct pollfd p = {fd, POLLOUT, 0};
    int ret = 0;
    socklen_t optlen;
    URLContext *h = customize_ctx;

    if (ff_socket_nonblock(fd, 1) < 0)
        gxlogi ("ff_socket_nonblock failed\n");

    if (customize_fd)
        customize_fd(customize_ctx, fd, 1);

    while ((ret = connect(fd, cur_ai->ai_addr, cur_ai->ai_addrlen))) {
        ret = ff_neterrno();
        switch (ret) {
        case AVERROR(EINTR):
            if (url_check_interrupt_cb()) {
                return AVERROR_EXIT;
            }
            continue;
        case AVERROR(EINPROGRESS):
        case AVERROR(EAGAIN):
            ret = ff_poll_interrupt(&p, 1, timeout);
            if (ret < 0) {
                gxloge("Connection to %s failed.\n", h->filename);
                return ret;
            }
            optlen = sizeof(ret);
            if (getsockopt (fd, SOL_SOCKET, SO_ERROR, &ret, &optlen))
                ret = AVUNERROR(ff_neterrno());
            ret = (ret != 0)?AVERROR(ret):ret;
        default:
            return ret;
        }
    }
    return ret;
}


void ff_network_close(void)
{
    return;
}

int ff_is_multicast_address(struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET) {
        return IN_MULTICAST(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr));
    }
    if (addr->sa_family == AF_INET6) {
        return IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6 *)addr)->sin6_addr);
    }

    return 0;
}

static int match_host_pattern(const char *pattern, const char *hostname)
{
    int len_p, len_h;
    if (!strcmp(pattern, "*"))
        return 1;
    // Skip a possible *. at the start of the pattern
    if (pattern[0] == '*')
        pattern++;
    if (pattern[0] == '.')
        pattern++;
    len_p = strlen(pattern);
    len_h = strlen(hostname);
    if (len_p > len_h)
        return 0;
    // Simply check if the end of hostname is equal to 'pattern'
    if (!strcmp(pattern, &hostname[len_h - len_p])) {
        if (len_h == len_p)
            return 1; // Exact match
        if (hostname[len_h - len_p - 1] == '.')
            return 1; // The matched substring is a domain and not just a substring of a domain
    }
    return 0;
}

int ff_http_match_no_proxy(const char *no_proxy, const char *hostname)
{
    char *buf, *start;
    int ret = 0;
    if (!no_proxy)
        return 0;
    if (!hostname)
        return 0;
    buf = av_strdup(no_proxy);
    if (!buf)
        return 0;
    start = buf;
    while (start) {
        char *sep, *next = NULL;
        start += strspn(start, " ,");
        sep = start + strcspn(start, " ,");
        if (*sep) {
            next = sep + 1;
            *sep = '\0';
        }
        if (match_host_pattern(start, hostname)) {
            ret = 1;
            break;
        }
        start = next;
    }
    av_free(buf);
    return ret;
}

int ff_accept(int fd, int timeout, URLContext *h)
{
    int ret;
    struct pollfd lp = { fd, POLLIN, 0 };

    ret = ff_poll_interrupt(&lp, 1, timeout);
    if (ret < 0)
        return ret;

    ret = accept(fd, NULL, NULL);
    if (ret < 0)
        return ff_neterrno();
    if (ff_socket_nonblock(ret, 1) < 0)
        gxlogi ("ff_socket_nonblock failed\n");

    return ret;
}

int ff_listen(int fd, const struct sockaddr *addr,
              socklen_t addrlen, void *logctx)
{
    int ret;
    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
        gxlogi_raw_l ("setsockopt(SO_REUSEADDR) failed\n");
    }
    ret = bind(fd, addr, addrlen);
    if (ret)
        return ff_neterrno();

    ret = listen(fd, 1);
    if (ret)
        return ff_neterrno();
    return ret;
}

int ff_listen_bind(int fd, const struct sockaddr *addr, socklen_t addrlen, int timeout, URLContext *h)
{
    int ret;
    if ((ret = ff_listen(fd, addr, addrlen, h)) < 0)
        return ret;
    if ((ret = ff_accept(fd, timeout, h)) < 0)
        return ret;
    closesocket(fd);
    return ret;
}

int ff_network_sleep_interruptible(int64_t timeout)
{
    int64_t wait_start = av_gettime();

    while (1) {
        int64_t time_left;
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
        time_left = timeout - (av_gettime() - wait_start);
        if (time_left <= 0)
            return AVERROR(ETIMEDOUT);
        GxCore_ThreadDelay(FFMIN(time_left/1000, 100));
    }
    return 0;
}

