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
#include "tcp.h"
#ifdef LINUX_OS
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>
#else
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/sockio.h>
#endif
#include "common.h"


#define TCP_BUF_SIZE 256
typedef struct TCPContext {
	int port;
	int retry_dns_request;
	int listen_socket;
	int timeout;
	int timeout1;
	int iGetSystemSetTimeout;
	char *hostname;
	char *proto;
	char *path;
	char *authorization;
	char portstr[10];
	char buf[TCP_BUF_SIZE];
} TCPContext;

static TCPContext* tcp_context_init(void)
{
	TCPContext *s = NULL;
	s = av_malloc(sizeof(TCPContext));
	if (!s) {
		return s;
	}
	s->port = -1;
	s->retry_dns_request = 3;
	s->listen_socket = 0;
	s->timeout = 50;
	s->timeout1 = 50;
	s->hostname = NULL;
	s->proto = NULL;
	s->path = NULL;
	s->authorization = NULL;
	memset (s->portstr, 0, sizeof(s->portstr));
	memset (s->buf, 0, sizeof(s->buf));
	return s;
}

static int tcp_context_free(TCPContext *s)
{
	if (s->hostname) {
		av_free (s->hostname);
		s->hostname = NULL;
	}
	if (s->proto) {
		av_free (s->proto);
		s->proto = NULL;
	}
	if (s->path) {
		av_free (s->path);
		s->path = NULL;
	}
	if (s) {
		av_free (s);
		s = NULL;
	}
	return 0;
}

static int url_find_info(char *arg, int arg_size, const char *tag1, const char *info)
{
	const char *p;
	char tag[128], *q;

	p = info;
	if (*p == '?')
		p++;
	for(;;) {
		q = tag;
		while (*p != '\0' && *p != '=' && *p != '&') {
			if ((q - tag) < sizeof(tag) - 1)
				*q++ = *p;
			p++;
		}
		*q = '\0';
		q = arg;
		if (*p == '=') {
			p++;
			while (*p != '&' && *p != '\0') {
				if ((q - arg) < arg_size - 1) {
					if (*p == '+')
						*q++ = ' ';
					else
						*q++ = *p;
				}
				p++;
			}
		}
		*q = '\0';
		if (!strcmp(tag, tag1))
			return 1;
		if (*p != '&')
			break;
		p++;
	}
	return 0;
}

static void url_split(TCPContext *s, const char *url)
{
	const char *p, *ls, *at, *col, *brk;

	/* parse protocol */
	if ((p = strchr(url, ':'))) {
		s->proto = av_strndup(url, p - url);
		p++; /* skip ':' */
		if (*p == '/') p++;
		if (*p == '/') p++;
	} else {
		/* no protocol means plain filename */
		s->path = av_strdup(url);
		return;
	}

	/* separate path from hostname */
	ls = strchr(p, '/');
	if(!ls)
		ls = strchr(p, '?');
	if(ls) {
		s->path = av_strdup(ls);
	} else {
		ls = &p[strlen(p)]; // XXX
	}

	/* the rest is hostname, use that to parse auth/port */
	if (ls != p) {
		/* authorization (user[:pass]@hostname) */
		if ((at = strchr(p, '@')) && at < ls) {
			s->authorization = av_strndup(p, at - p);
			p = at + 1; /* skip '@' */
		}

		if (*p == '[' && (brk = strchr(p, ']')) && brk < ls) {
			/* [host]:port */
			s->hostname = av_strndup(p+1, brk - p-1);
			if (brk[1] == ':')
				s->port = atoi(brk + 2);
		} else if ((col = strchr(p, ':')) && col < ls) {
			s->hostname = av_strndup(p, col - p);
			if (s)
				s->port = atoi(col + 1);
		} else {
			s->hostname = av_strndup(p, ls - p);
		}
	}
}

/* return non zero if error */
static int tcp_open(const char *uri, PLAYER_INTERRUPT_CBK interrupt_cbk, int ai_family)
{
	struct addrinfo hints, *ai = NULL, *cur_ai = NULL;
	int fd = -1, ret = -1;
	const char *p;
	socklen_t optlen;
	TCPContext *s = NULL;

	if (!(s = tcp_context_init()))
		return ret;

	GxPlayer_SystemGet(PSYS_NETWORK_TIMEOUT, &s->iGetSystemSetTimeout);
	/*Get system set timeout iGetSystemSetTimeout=10000, each call poll use 100ms, set timeout 10s, if request exceed 10s, please set PLAY_NETWORK_TIMEOUT*/
	if (s->iGetSystemSetTimeout > 0)
		s->timeout = s->timeout1 = s->iGetSystemSetTimeout/100;

	url_split(s, uri);

	if (strcmp(s->proto,"tcp") || s->port <= 0 || s->port >= 65536) {
		tcp_context_free(s);
		return -1;
	}

	p = strchr(uri, '?');
	if (p) {
		if (url_find_info(s->buf, sizeof(s->buf)-1, "listen", p))
			s->listen_socket = 1;
		if (url_find_info(s->buf, sizeof(s->buf)-1, "timeout", p)) {
			s->timeout = strtol(s->buf, NULL, 10);
		}
	}

	do {
		if (interrupt_cbk && interrupt_cbk()) {
			tcp_context_free(s);
			return -1;
		}
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = ai_family;
		hints.ai_socktype = SOCK_STREAM;
		snprintf(s->portstr, sizeof(s->portstr), "%d", s->port);
		ret = getaddrinfo(s->hostname, s->portstr, &hints, &ai);
		if (ret) {
			GxCore_ThreadDelay(50);
			s->retry_dns_request -= 1;
		}
	} while(s->retry_dns_request > 0 && ret);

	if ((s->retry_dns_request <= 0) && ret) {
		gxloge("retry:3,failed!![%s],uri:%s--Error:%s\n", (ai_family==AF_INET)?"ipv4":"ipv6", uri, gai_strerror(ret));
		tcp_context_free(s);
		return -1;
	}

	cur_ai = ai;

restart:
	s->timeout1 = s->timeout;
	ret = -1;
	fd = socket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol);
	if (fd < 0)
		goto fail;

	if (s->listen_socket) {
		int fd1;
		int reuse = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
		ret = bind(fd, cur_ai->ai_addr, cur_ai->ai_addrlen);
		if (ret) {
			ret = -1;
			goto fail1;
		}
		listen(fd, 1);
		fd1 = accept(fd, NULL, NULL);
		if (fd1 < 0) {
			ret = -1;
			goto fail1;
		}
		closesocket(fd);
		fd = fd1;
#ifdef LINUX_OS
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
#else
		int val = 1;
		ioctl(fd, FIONBIO, &val);
#endif
	} else {
#ifdef LINUX_OS
redo:
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
#else
		int val = 1;
		ioctl(fd, FIONBIO, &val);
#endif
		int recv_buf_size = 0;
		GxPlayer_SystemGet(PSYS_NETWORK_RECVBUF, &recv_buf_size);
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size));
		ret = connect(fd, cur_ai->ai_addr, cur_ai->ai_addrlen);
	}

	if (ret < 0) {
		struct pollfd p = {fd, POLLOUT, 0};
#ifdef LINUX_OS
		if (errno == EINTR) {
			if (interrupt_cbk && interrupt_cbk()) {
				ret = -1;
				goto fail1;
			}
			goto redo;
		}
#endif
		if (errno != EINPROGRESS && errno != EAGAIN)
			goto fail;

		/* wait until we are connected or until abort */
		while(s->timeout1--) {
			if (interrupt_cbk && interrupt_cbk()) {
				ret = -1;
				goto fail1;
			}
			ret = poll(&p, 1, 100);
			if (ret > 0)
				break;
		}
		optlen = sizeof(ret);
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &ret, &optlen)) {
			ret = AVERROR(errno);
		} else if (ret != 0) {
			ret = AVERROR(ret);
		}
		if (ret < 0) {
			gxloge("Connection to %s:%d failed:%s\n", s->hostname, s->port, strerror(ret));
			ret = -1;
			goto fail;
		} else if ((s->timeout1 <0) && (errno != EINPROGRESS && errno != EAGAIN)) {
			ret = -errno;
			goto fail1;
		}
	}

	if (fd > 0) {
		struct linger so_linger;
		so_linger.l_onoff  = 1;
#ifdef LINUX_OS
		so_linger.l_linger = 1; // posix: The unit is 1 sec.
#else
		so_linger.l_linger = 0; // bsd: The unit is 1 tick(10ms).
#endif
		setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
	}

	freeaddrinfo(ai);
	tcp_context_free(s);
	return fd;

fail:
	if (cur_ai->ai_next) {
		/* Retry with the next sockaddr */
		cur_ai = cur_ai->ai_next;
		if (fd >= 0)
			closesocket(fd);
		goto restart;
	}
fail1:
	if (fd >= 0)
		closesocket(fd);
	freeaddrinfo(ai);
	tcp_context_free(s);
	return ret;
}

int connect_2_server (char *host, int port, int verb, PLAYER_INTERRUPT_CBK interrupt_cbk)
{
	int fd = -1;
	char uri[512] = {0};
	int ai_family = AF_INET;

	snprintf(uri, sizeof(uri), "tcp://%s:%d", host, port);
	fd = tcp_open(uri,  interrupt_cbk, ai_family);
	if((fd < 0) && (1 == GxCore_IfSupportIPV6())){
		if (fd != -1)
			return fd;
		ai_family = AF_INET6;
		fd = tcp_open(uri, interrupt_cbk, ai_family);
	}
	return fd;
}
