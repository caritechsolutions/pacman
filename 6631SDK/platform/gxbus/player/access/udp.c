#include "gx_url.h"
#include "gx_common.h"
#include "gx_system.h"
#include "udp.h"
#include "../avformat/gx_poll.h"
#include <netinet/in.h>
#include <netdb.h>

#define IS_MULTICAST(a) ((((uint32_t)(a)) & 0xf0000000) == 0xe0000000)
#define IS_BROADCAST(a) ((((uint32_t)(a)) & 0x000000FF) == 0x000000FF)

#ifdef LINUX_OS
static handle_t mutex = -1;
#endif
static int udp_join_multicast_group(int sockfd, struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		struct ip_mreq mreq;

		mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
		mreq.imr_interface.s_addr= INADDR_ANY;
		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *)&mreq, sizeof(mreq)) < 0) {
			av_log(NULL, AV_LOG_ERROR, "setsockopt(IP_ADD_MEMBERSHIP): %s\n", strerror(errno));
			return -1;
		}
	}
	return 0;
}

static int udp_leave_multicast_group(int sockfd, struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		struct ip_mreq mreq;

		mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
		mreq.imr_interface.s_addr= INADDR_ANY;
		if (setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const void *)&mreq, sizeof(mreq)) < 0) {
			av_log(NULL, AV_LOG_ERROR, "setsockopt(IP_DROP_MEMBERSHIP): %s\n", strerror(errno));
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
		av_log(NULL, AV_LOG_ERROR, "udp_resolve_host: %s\n", gai_strerror(error));
	}

	return res;
}

static int udp_set_url(struct sockaddr_storage *addr,
		const char *hostname, int port)
{
	struct addrinfo *res0;
	int addr_len;

	res0 = udp_resolve_host(hostname, port, SOCK_DGRAM, AF_UNSPEC, 0);
	if (res0 == 0) return -1;
	memcpy(addr, res0->ai_addr, res0->ai_addrlen);
	addr_len = res0->ai_addrlen;
	freeaddrinfo(res0);

	return addr_len;
}

static int udp_socket_create(UDPContext *s, struct sockaddr_storage *addr, int *addr_len)
{
	int udp_fd = -1;
	struct addrinfo *res0 = NULL, *res = NULL;

	res0 = udp_resolve_host(NULL, s->local_port, SOCK_DGRAM, AF_INET, AI_PASSIVE);
	if (res0 == 0)
		goto fail;

	for (res = res0; res; res=res->ai_next) {
		udp_fd = socket(res->ai_family, SOCK_DGRAM, 0);
		if (udp_fd > 0) break;
		gxlogf("socket: %s\n", strerror(errno));
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

	if (getnameinfo((struct sockaddr *)addr, addr_len, NULL, 0,  sbuf, sizeof(sbuf), NI_NUMERICSERV) != 0) {
		av_log(NULL, AV_LOG_ERROR, "getnameinfo: %s\n", strerror(errno));
		return -1;
	}

	return strtol(sbuf, NULL, 10);
}

static int _is_multicast_address(struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		return IS_MULTICAST(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr));
	}

	return 0;
}

static int _is_broadcast_address(struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		return IS_BROADCAST(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr));
	}
	return 0;
}

static int _set_remote_url(UDPContext* s, GxURL* uri)
{
	s->dest_addr_len = udp_set_url(&s->dest_addr, (const char*)uri->hostname, uri->port);
	if (s->dest_addr_len < 0) {
		return -1;
	}

	s->is_multicast = _is_multicast_address((struct sockaddr*) &s->dest_addr);
	return 0;
}

UDPContext* Gx_UDP_Open4Server(GxURL *uri)
{
	int  udp_fd = -1;
	struct sockaddr_in addr;
	UDPContext *s = av_mallocz(sizeof(UDPContext));

	if(s == NULL)
		return NULL;

	s->ttl = 16;
	s->buffer_size = 240*1024;
	s->local_port = uri->port;

	s->dest_addr_len = udp_set_url(&s->dest_addr, (const char*)uri->hostname, uri->port);
	if (s->dest_addr_len < 0) {
		goto fail;
	}

	s->is_broadcast = _is_broadcast_address((struct sockaddr*) &s->dest_addr);

	udp_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (udp_fd < 0)
		goto fail;

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(s->local_port);
	inet_pton(AF_INET, uri->hostname, &addr.sin_addr);

	if (s->is_broadcast) {
		int on = 1;
		if (setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) != 0)
			goto fail;
	}

	/*
	if (connect(udp_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		goto fail;
	}  */
#ifdef LINUX_OS
	if(mutex == -1) {
		int ret = GxCore_MutexCreate(&mutex);
		if (ret < 0) {
			gxloge ("create mutex for udp failed.\n");
			goto fail;
		}
	}
#endif
	s->udp_fd = udp_fd;
	return s;

fail:
	if (udp_fd >= 0)
		closesocket(udp_fd);
	return NULL;
}

UDPContext* Gx_UDP_Open(GxURL *uri)
{
	int  udp_fd = -1, tmp, bind_ret = -1;
	struct sockaddr_storage my_addr;
	int len;
	UDPContext *s = av_mallocz(sizeof(UDPContext));

	if(s == NULL)
		return NULL;

	s->ttl = 16;
	s->buffer_size = 240*1024;
	s->local_port = uri->port;

	if (_set_remote_url(s, uri) < 0)
		goto fail;

	udp_fd = udp_socket_create(s, &my_addr, &len);
	if (udp_fd < 0)
		goto fail;

	if (s->reuse_socket || s->is_multicast) {
		s->reuse_socket = 1;
		if (setsockopt (udp_fd, SOL_SOCKET, SO_REUSEADDR, &(s->reuse_socket), sizeof(s->reuse_socket)) != 0)
			goto fail;
	}

	if (s->is_multicast) {
		struct sockaddr_in* localaddr = (struct sockaddr_in*)&s->dest_addr;
		localaddr->sin_port=htons(uri->port);
		bind_ret = bind(udp_fd,(struct sockaddr *)&localaddr, sizeof(localaddr));
	}

	if (bind_ret < 0 && bind(udp_fd,(struct sockaddr *)&my_addr, len) < 0) {
		gxlogf("bind failed: %s\n", strerror(errno));
		goto fail;
	}

	len = sizeof(my_addr);
	getsockname(udp_fd, (struct sockaddr *)&my_addr, (socklen_t *)(&len));
	s->local_port = udp_port(&my_addr, len);

	if (s->is_multicast) {
		if (udp_join_multicast_group(udp_fd, (struct sockaddr *)&s->dest_addr) < 0)
			goto fail;
	}

	tmp = s->buffer_size;
	if (setsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &tmp, sizeof(tmp)) < 0) {
		gxlogf("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
	}
	/* make the socket non-blocking */
//	fcntl(udp_fd, F_SETFL, fcntl(udp_fd, F_GETFL) | O_NONBLOCK);
//	需要知道服务器的端口号才能connect,否则接收不到数据
//	connect(udp_fd, (struct sockaddr*)&s->dest_addr, s->dest_addr_len);

	s->udp_fd = udp_fd;
	return s;

fail:
	if (udp_fd >= 0)
		closesocket(udp_fd);
	return NULL;
}

int Gx_UDP_Read(UDPContext* s, uint8_t *buf, int size)
{
#ifdef LINUX_OS
UDP_READ_AGAIN:
	if(s->udp_data_len > 0){
		int recv_size = s->udp_data_len>size?size:s->udp_data_len;
		memcpy(buf, s->udp_data+s->udp_data_pos, recv_size);
		s->udp_data_len -=  recv_size;
		s->udp_data_pos += recv_size;
		return recv_size;
	}
#endif

	int ret = 0, udp_fd = s?s->udp_fd:-1;
	int ev = POLLIN;
	struct pollfd p = { .fd = udp_fd, .events = ev, .revents = 0 };
	ret = poll(&p, 1, 200);
	if(ret > 0){
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
			s->error_count = 0;
			num = FFMIN(num, STREAM_UDP_MAX_COUNT);
			memset(msgs, 0, sizeof(msgs));

			if(num == 0){
				iovecs[0].iov_base = s->udp_data;
				iovecs[0].iov_len  = STREAM_UDP_MAX_PKT_SIZE;
				msgs[0].msg_hdr.msg_iov = &iovecs[0];
				msgs[0].msg_hdr.msg_iovlen = 1;
				if( recvmmsg(udp_fd, msgs, 1, 0x10000, &timeout) <= 0)
					return 0 ;
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
					iovecs[i].iov_base = udp_buf[ i];
					iovecs[i].iov_len  = STREAM_UDP_MAX_PKT_SIZE;
					msgs[i].msg_hdr.msg_iov = &iovecs[i];
					msgs[i].msg_hdr.msg_iovlen = 1;
				}
				if( (ret = recvmmsg(udp_fd, msgs, num, 0x10000, &timeout)) <= 0) {
					goto fail;
				}
				for(i = 0; i < ret; i++){
					memcpy(buf+recv_size, udp_buf[i],  msgs[i].msg_len);
					recv_size += msgs[i].msg_len;
				}
				if(ret  != num) {
					GxCore_ThreadDelay(10);
				}
fail:
				for(i = 0; i < num; i++){
					if (udp_buf[i]) {
						av_free(udp_buf[i]);
						udp_buf[i] = NULL;
					}
				}
				if (udp_buf) {
					av_free(udp_buf);
					udp_buf = NULL;
				}
				return recv_size;
			}
		} else {
			s->error_count = 0;
			return recv(udp_fd, buf, size, 0);
		}
#else
		s->error_count = 0;
		return recv(udp_fd, buf, size, 0);
#endif
	}else if(ret == 0){
		s->error_count++;
		ret = s->error_count>50?-1:0;
	}
	return ret;
}

int Gx_UDP_Write(UDPContext* s, uint8_t *buf, int size)
{
	int ret = 0, udp_fd = s?s->udp_fd:-1;
#ifdef LINUX_OS
	s->error_count = 0;
	GxCore_MutexLock(mutex);
	int i = 0;
	for(i = 0; i < size; i += STREAM_UDP_MAX_PKT_SIZE) {
		if (size - i < STREAM_UDP_MAX_PKT_SIZE)
			ret = sendto(udp_fd, buf+i, size - i, 0, (struct sockaddr *)&s->dest_addr, sizeof(s->dest_addr));
		else
			ret = sendto(udp_fd, buf+i, STREAM_UDP_MAX_PKT_SIZE, 0, (struct sockaddr *)&s->dest_addr, sizeof(s->dest_addr));
		if (ret == -1) {
		    GxCore_MutexUnlock(mutex);
		    return 0;
		}
		usleep(100);
	}
	GxCore_MutexUnlock(mutex);
	return size;
#else
	s->error_count = 0;
	return send(udp_fd, buf, size, 0);
#endif
}

void Gx_UDP_Close(UDPContext* s)
{
	if(s == NULL) return;

	if (s->is_multicast)
		udp_leave_multicast_group(s->udp_fd, (struct sockaddr *)&s->dest_addr);

	closesocket(s->udp_fd);
	av_free(s);
#ifdef LINUX_OS
	GxCore_MutexDelete(mutex);
#endif
	return;
}

