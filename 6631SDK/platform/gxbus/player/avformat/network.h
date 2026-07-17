/*
 * Copyright (c) 2007 The FFmpeg Project
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

#ifndef AVFORMAT_NETWORK_H
#define AVFORMAT_NETWORK_H

#include <errno.h>
#include "avio.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define ff_neterrno() AVERROR(errno)

#include <arpa/inet.h>
#include "gx_poll.h"

int ff_socket_nonblock(int socket, int enable);

extern int ff_network_inited_globally;
int ff_network_init(void);
void ff_network_close(void);

void ff_tls_init(void);
void ff_tls_deinit(void);

int ff_network_wait_fd(int fd, int write);
int ff_network_wait_fd_timeout(int fd, int write, int64_t timeout);

int ff_inet_aton (const char * str, struct in_addr * add);

int ff_http_match_no_proxy(const char *no_proxy, const char *hostname);
/* getaddrinfo constants */
#ifndef EAI_FAIL
#define EAI_FAIL 4
#endif

#ifndef EAI_FAMILY
#define EAI_FAMILY 5
#endif

#ifndef EAI_NONAME
#define EAI_NONAME 8
#endif

#ifndef AI_PASSIVE
#define AI_PASSIVE 1
#endif

#ifndef AI_CANONNAME
#define AI_CANONNAME 2
#endif

#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 4
#endif

#ifndef NI_NOFQDN
#define NI_NOFQDN 1
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 2
#endif

#ifndef NI_NAMERQD
#define NI_NAMERQD 4
#endif

#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 8
#endif

#ifndef NI_DGRAM
#define NI_DGRAM 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN INET_ADDRSTRLEN
#endif

#ifndef IN_MULTICAST
#define IN_MULTICAST(a) ((((uint32_t)(a)) & 0xf0000000) == 0xe0000000)
#endif
#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((uint8_t *) (a))[0] == 0xff)
#endif

#define POLLING_TIME 100 /// Time in milliseconds between interrupt check

int ff_is_multicast_address(struct sockaddr *addr);
int ff_socket_nonblock(int socket, int enable);
int ff_listen(int fd, const struct sockaddr *addr,
              socklen_t addrlen, void *logctx);
int ff_listen_bind(int fd, const struct sockaddr *addr, socklen_t addrlen, int timeout, URLContext *h);
int ff_listen_connect(int fd, struct addrinfo *cur_ai, int timeout, void (*customize_fd)(void *, int, int), void *customize_ctx);
int ff_network_sleep_interruptible(int64_t timeout);

#endif /* AVFORMAT_NETWORK_H */
