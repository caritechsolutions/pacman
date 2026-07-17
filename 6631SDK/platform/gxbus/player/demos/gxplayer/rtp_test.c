#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "log.h"
#include "gxplayer_module.h"

typedef struct {
	const char *ip;
	int         port;
} rtp_params_t;

static char buf[2048];
static rtp_params_t test_params;
static int test_thread_id = 0;
static int test_running = 0;

static struct addrinfo* udp_host(const char *hostname, int port,
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
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "%s %d\n", __func__, __LINE__);
	}

	return res;
}

extern unsigned int av_get_tick(const char* prompt, unsigned int line);
static void rtp_test(void *pargs)
{
	int sockfd = 0;
	struct sockaddr_in src_addr;
	socklen_t src_len = sizeof(src_addr);
	int ret = 0;
	struct addrinfo *res = NULL;
	int last_seq = -1;
	int rec_size = 2*1024*1024;
	int rec_pack_num = 0;
	unsigned int time_ms = 0;

	res = udp_host(NULL, test_params.port, SOCK_DGRAM, AF_INET, AI_PASSIVE);
	if (res == NULL) {
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "%s %d\n", __func__, __LINE__);
		return;
	}

	sockfd = socket(res->ai_family, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "%s %d\n", __func__, __LINE__);
		return;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rec_size, sizeof(rec_size));

	//绑定socket对象与通信链接
	ret = bind(sockfd, (struct sockaddr*)res->ai_addr, res->ai_addrlen);
	if (ret < 0) {
		Log_Printf(LOG_GXPLAYER, LOG_INFO,
				"ip %s, port %d bind failed\n", test_params.ip, test_params.port);
		return;
	}

	while (test_running) {
		ret = recvfrom(sockfd, &buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &src_len);
		if (ret > 0) {
			int seq = ((buf[2]&0x3)<<8)|buf[3];
			int packet_total = 1<<10;

			if (last_seq == -1) {
				//Log_Printf(LOG_GXPLAYER, LOG_INFO, "%d\n", seq);
				if (seq != 0)
					continue;
			} else if (last_seq > seq) {
				char src_host[INET6_ADDRSTRLEN] = {0};
				unsigned int _time_ms = av_get_tick(NULL, 0);
				int dis_time_ms = 0;

				inet_ntop(src_addr.sin_family,
						&src_addr.sin_addr, src_host, INET6_ADDRSTRLEN);
				if (time_ms != 0)
					dis_time_ms = _time_ms - time_ms;
				time_ms = _time_ms;

				if (dis_time_ms > 0) {
					Log_Printf(LOG_GXPLAYER, LOG_INFO,
							"host %s %d, seq: %d %d, packet %d %d, %.2f%%, bitrate %d mbps\n",
							src_host, src_addr.sin_port, last_seq, seq,
							rec_pack_num, packet_total, rec_pack_num*100.0/packet_total,
							ret*1000/dis_time_ms*rec_pack_num*8/1024/1024);
				} else
					Log_Printf(LOG_GXPLAYER, LOG_INFO,
							"host %s %d, seq: %d %d, packet %d %d, %.2f%%, bitrate - mbps\n",
							src_host, src_addr.sin_port, last_seq, seq,
							rec_pack_num, packet_total, rec_pack_num*100.0/packet_total);
				rec_pack_num = 0;
			}

			rec_pack_num++;
			last_seq = seq;
		} else {
			Log_Printf(LOG_GXPLAYER, LOG_INFO, "xxxxxxxxx\n", __func__, __LINE__);
			GxCore_ThreadDelay(1);
		}
	}

	close(sockfd);
	return;
}

int GxRTP_Test_Start(const char *ip, int port)
{
	if (!test_running) {
		test_params.ip = strdup(ip);
		test_params.port = port;
		test_running = 1;
		GxCore_ThreadCreate("rtp_test",
				&test_thread_id,
				rtp_test,
				NULL,
				64 * 1024,
				GXOS_DEFAULT_PRIORITY - 1);
	} else {
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "Please stop rtp test..\n", __func__, __LINE__);
	}

	return 0;
}

int GxRTP_Test_Stop(void)
{
	if (test_running) {
		test_running = 0;
		if (test_params.ip) {
			free((void*)test_params.ip);
			test_params.ip = NULL;
			test_params.port = 0;
		}
		GxCore_ThreadJoin(test_thread_id);
	} else{
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "not rtp test..\n", __func__, __LINE__);
	}

	return 0;
}
