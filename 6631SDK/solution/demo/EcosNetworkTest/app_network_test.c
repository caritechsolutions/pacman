/**
 *  Example:
 *
 *  ping("www.baidu.com");
 *  ping("1.cn.pool.ntp.org");
 *  ping("ntp.sjtu.edu.cn");
 *  ping("s2c.time.edu.cn");
 *  ping("s2g.time.edu.cn");
 *  app_get_time_from_net();
 *  test_wget("http://yinyueshiting.baidu.com/data2/music/238976477/5830696230400128.mp3?xcode=fd32106e68e36b07880ea0ac285fa492fd7d78a8a732f6f4", NULL);
 *  iwpriv("ra0", "set", "RadioOn=0");
 *  iwpriv("ra0", "set", "RadioOn=1");
 *
 *
 */
#ifndef LINUS_OS

//#define __ECOS
#include <sys/types.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pkgconf/system.h>

#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>

#include <cyg/io/eth/netdev.h>
#include <string.h>

#include <shell.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_arp.h>
#include <cyg/io/eth/eth_drv.h>

#include "gxcore.h"
#include "app.h"
#include "app_module.h"
#include <string.h>

#define PING_TEST
#ifdef PING_TEST
#include <stdlib.h>

#include <network.h>
#include <arpa/inet.h>

#define UNIQUEID 0x1234
#define NUM_PINGS 4
#define MAX_PACKET 4096
#define MIN_PACKET   64
#define MAX_SEND   4000
#define PACKET_ADD  ((MAX_SEND - MIN_PACKET)/NUM_PINGS)
#define	PING_SERVER_NUM				30

/* Private Macros ---------------------------------------------------------- */
static unsigned int ok_recv = 0;
static unsigned int  bogus_recv = 0;
static unsigned char pkt1[MAX_PACKET];
static unsigned char pkt2[MAX_PACKET];

static struct bootp mbp;

externC cyg_tick_count_t cyg_current_time(void);
externC cyg_tick_count_t cyg_current_time(void);

static void ping_test(struct bootp *bp,const char *ping_server,int num);

static int inet_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register u_int sum = 0;
	u_short odd_byte = 0;

	/* Our algorithm is simple, using a 32 bit accumulator (sum),
	   we add sequential 16 bit words to it, and at the end, fold
	   back all the carry bits from the top 16 bits into the lower
	   16 bits. */
	while( nleft > 1 )
	{
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary                                         */
	if( nleft == 1 )
	{
		*(u_char *)(&odd_byte) = *(u_char *)w;
		sum += odd_byte;
	}

	/* add back carry outs from top 16 bits to low 16 bits                      */
	sum = (sum >> 16) + (sum & 0x0000ffff);          /* add hi 16 to low 16 */
	sum += (sum >> 16);                              /* add carry           */
	answer = ~sum;                                   /* truncate to 16 bits */
	return (answer);
}

/*---------------------------------------------------------------------------*/
/* check up icmp							*/
static int show_icmp(unsigned char *pkt, unsigned int len, struct sockaddr_in *from, struct sockaddr_in *to)
{
	volatile unsigned long long *tp, tv;
	struct ip *ip;
	struct icmp *icmp;
	tv = cyg_current_time();
	ip = (struct ip *)pkt;

	if ((len < sizeof(*ip)) || ip->ip_v != IPVERSION) {
		app_log_min("%s: Short packet or not IP! - Len: %d, Version: %d\n",
				inet_ntoa(from->sin_addr), len, ip->ip_v);
		return 0;
	}

	icmp = (struct icmp *)(pkt + sizeof(*ip));
	len -= (sizeof(*ip) + 8);
	tp = (unsigned long long *)&icmp->icmp_data;

	if (icmp->icmp_type != ICMP_ECHOREPLY) {
		app_log_min("%s: Invalid ICMP - type: %d\n",
				inet_ntoa(from->sin_addr), icmp->icmp_type);
		return 0;
	}

	if (icmp->icmp_id != UNIQUEID) {
		app_log_min("%s: ICMP received for wrong id - sent: %x, recvd: %x\n",
				inet_ntoa(from->sin_addr), UNIQUEID, icmp->icmp_id);
	}

	app_log_min("%d bytes from %s: ", len, inet_ntoa(from->sin_addr));
	app_log_min("icmp_seq=%d", icmp->icmp_seq);
	app_log_min(", time=%dms\n", (int)(tv - *tp)*10);
	return (from->sin_addr.s_addr == to->sin_addr.s_addr);
}

/*---------------------------------------------------------------------------*/
/* Ping host IP address							*/
static void ping_host(int s, struct sockaddr_in *host)
{
	struct icmp *icmp = (struct icmp *)pkt1;
	int icmp_len = MIN_PACKET;
	int seq;
	volatile unsigned long long *tp;
	long *dp;
	struct sockaddr_in from;
	//unsigned int len, fromlen,send_len;
	int len, fromlen,send_len;
	int i;

	for (seq = 0;  seq < 1;  seq++, icmp_len += PACKET_ADD )
	{
		// Build ICMP packet
		icmp->icmp_type = ICMP_ECHO;
		icmp->icmp_code = 0;
		icmp->icmp_cksum = 0;
		icmp->icmp_seq = seq;
		icmp->icmp_id = 0x1234;

		// Set up ping data
		tp = (unsigned long long *)&icmp->icmp_data;
		*tp++ = cyg_current_time();
		dp = (long *)tp;
		for (i = sizeof(*tp);  i < icmp_len;  i += sizeof(*dp)) {
			*dp++ = i;
		}

		// Add checksum
		icmp->icmp_cksum = inet_cksum( (unsigned short *)icmp, icmp_len+8);

		// Send it off
		send_len = sendto(s, icmp, icmp_len+8, 0, (struct sockaddr *)host, sizeof(*host));


		if ( send_len < 0)
		{
			// TNR_OFF();
			perror("sendto");
			continue;

		}
		fromlen = sizeof(from);

		//receive ICMP packets
		len = recvfrom(s, pkt2, sizeof(pkt2), 0, (struct sockaddr *)&from, (socklen_t *)(&fromlen));
		if (len < 0)
		{
			/*
			   icmp_len = MIN_PACKET - PACKET_ADD;
			   app_log_min("MIN_PACKET:%d  ",MIN_PACKET);
			   app_log_min("PACKET_ADD:%d  ",PACKET_ADD);
			   app_log_min("icmp_len:%d \r\n",icmp_len);
			   */
			app_log_min("ping request time out ! \r\n");
			bogus_recv++;
		}
		if(len >= 0)
		{
			if (show_icmp(pkt2, len, &from, host))
			{
				ok_recv++;
			}
			else
			{
				bogus_recv++;
			}
		}
		//if(len ==0){
		//	app_log_min("receive len is %d \r\n ",len);
		//}
	}
}

/*---------------------------------------------------------------------------*/
/* Init host IP address							*/
static void ping_test(struct bootp *bp,const char *ping_server,int num)
{
	struct protoent *p;
	struct timeval tv;
	struct sockaddr_in host;
	int s,i;

	/* Change domainname into IP Address */
	struct hostent *hent;

	hent = gethostbyname(ping_server);
	if (hent != NULL)
	{
		app_log_min("#########################################################################\n");
		app_log_min("          PASS:<%s is %s>\n", hent->h_name, inet_ntoa(*(struct in_addr *)hent->h_addr_list[0]));  //打印域名和IP地址
		app_log_min("#########################################################################\n");
	}

	if ((p = getprotobyname("icmp")) == (struct protoent *)0)
	{
		perror("getprotobyname");
		return;
	}


	/* Init the socket parameter                                                 */
	s = socket(AF_INET, SOCK_RAW, p->p_proto);
	if (s < 0) {
		perror("socket");
		return;
	}
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	// Set up host address
	memset(&host,0,sizeof(host));
	host.sin_family = AF_INET;
	host.sin_len = sizeof(host);
	if (hent)
		host.sin_addr = *(struct in_addr *)hent->h_addr_list[0];
	else
		host.sin_addr.s_addr = inet_addr(ping_server);
	host.sin_port = 0;

	/*---------------------------------------------------------------------------*/
	// Ping right IP address
	app_log_min("##############################################################\n");
	app_log_min("                 Ping server %s\n", inet_ntoa(host.sin_addr));
	app_log_min("##############################################################\n");
	ok_recv = 0;
	bogus_recv = 0;
	for(i=0;i<num;i++)
	{
		ping_host(s, &host);
	}
	app_log_min("\n**************************************************************\n");
	app_log_min("          Sent %d packets, received %d OK, %d bad\n",num , ok_recv, bogus_recv);
	app_log_min("**************************************************************\n");

	close(s);

}

void ping(char *ip)
{
    int num = 0;

    static int ping_num = 4;
    num = ping_num;

    ping_test(&mbp,ip,num);
    return;
}

#endif


#define NTP_TEST
#ifdef NTP_TEST

#define HOUR_TO_SEC      3600
#define debug            1
#define TIMEOUT          3
#define JAN_1970         0x83AA7E80
#define NTP_DEFAULT_PORT "123"

#define NTP_SERVER_0 "1.cn.pool.ntp.org"
#define NTP_SERVER_1 "ntp.sjtu.edu.cn"
#define NTP_SERVER_2 "s2c.time.edu.cn"
#define NTP_SERVER_3 "s2g.time.edu.cn"

static void construct_ntp_packet(char content[])
{
	long  timer;
	memset(content, 0, 48);
	content[0] = 0x1b;    // LI = 0 ; VN = 3 ; Mode = 3 (client);

	time((time_t *)&timer);
	timer = htonl(timer + JAN_1970 );

	memcpy(content + 40, &timer, sizeof(timer));  //trans_timastamp
}

static status_t get_ntp_time(int sockfd, struct sockaddr *server_addr)
{
	char           content[256];
	time_t         timet;
	long           temp;
    //socklen_t addrlen = sizeof(struct sockaddr_storage);
    socklen_t addrlen = sizeof(struct sockaddr_in);

	struct timeval block_time;
	fd_set         sockfd_set;
	FD_ZERO(&sockfd_set);
	FD_SET(sockfd, &sockfd_set);
	block_time.tv_sec = TIMEOUT;      //time out
	block_time.tv_usec = 0;

	construct_ntp_packet(content);
	if (sendto(sockfd, content, 48, 0, server_addr, addrlen) < 0)
	{
		perror("sendto error");
		return GXCORE_ERROR;
	}

	if(select(sockfd + 1, &sockfd_set, NULL, NULL, &block_time ) > 0)
	{
		if (recvfrom(sockfd, content, 256, 0, server_addr, &addrlen) < 0)
		{
			perror("recvfrom error");
			return GXCORE_ERROR;
		}
		else
		{
			memcpy(&temp, content + 40, 4);
			temp = (time_t)(ntohl(temp) - JAN_1970 );
			timet = (time_t)temp;
#if debug
            //app_log_min("[TZ: %ld]%s Time\t: %s", timezone, tzname[0], ctime(&timet));
            app_log_min("\tTime\t: %s", ctime(&timet));
#endif

			//g_AppTime.sync(&g_AppTime, timet+3);
			//g_AppBook.enable();
			//g_AppTime.start(&g_AppTime);
		}
	}
	else
	{
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

status_t app_get_time_from_net(void)
{
    int ret = GXCORE_ERROR;
	int sockfd = -1, i = 0;
	struct sockaddr_in *addr;
	char * ip[4] = {NTP_SERVER_0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3};

    struct addrinfo hints, *res, *ap;
    char *port = NTP_DEFAULT_PORT;
    int error;

    const char *ip_addr;
    char abuf[INET_ADDRSTRLEN];

    char content[256];
    construct_ntp_packet(content);

	for (i = 0 ; i < 4 ; i++ )
	{
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;

#if debug
        app_log_min("\nget info from\t: %s\n", ip[i]);
#endif
        error = getaddrinfo(ip[i], port, &hints, &res);
        if (error != 0) {
            //freeaddrinfo(res);
            app_log_min("getaddrinfo() error: %s\n", gai_strerror(error));
            continue;
        }

        for (ap = res; ap != NULL; ap = ap->ai_next) {
            if (ap->ai_family == AF_INET) {
                addr = (struct sockaddr_in *)ap->ai_addr;
                ip_addr = inet_ntop(AF_INET, (const void *)(&addr->sin_addr), abuf,INET_ADDRSTRLEN);
#if debug
                app_log_min("\taddress\t: %s\n", ip_addr?ip_addr:"unknown");
                app_log_min("\tport\t: %d\n", ntohs(addr->sin_port));
#endif
            }
            sockfd = socket(ap->ai_family, ap->ai_socktype, ap->ai_protocol);
            if (sockfd == -1)
                continue;
            break;
        }
        if (ap == NULL) {
            close(sockfd);
            freeaddrinfo(res);
            app_log_min("socket() error\n");
            exit(2);
        }

        ret = get_ntp_time(sockfd, ap->ai_addr);
        if (GXCORE_SUCCESS == ret)
        {
            app_log_min("success from ip\t: %s\n\n", ip_addr);
            close(sockfd);
            freeaddrinfo(res);
            break;
        }

        close(sockfd);
        freeaddrinfo(res);
	}

	return ret;
}

#endif


#define WGET_TEST
#ifdef WGET_TEST

/* $Id: miniwget.c,v 1.37 2010/04/12 20:39:42 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2005-2010 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#if defined(__amigaos__) && !defined(__amigaos4__)
#define socklen_t int
#else /* #if defined(__amigaos__) && !defined(__amigaos4__) */
#include <sys/select.h>
#endif /* #else defined(__amigaos__) && !defined(__amigaos4__) */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#define closesocket close
/* defining MINIUPNPC_IGNORE_EINTR enable the ignore of interruptions
 * during the connect() call */
#define MINIUPNPC_IGNORE_EINTR
//#if defined(__sun) || defined(sun)
//#define MIN(x,y) (((x)<(y))?(x):(y))
//#endif

//#include "/home/yanwzh/workspace/ecos/v1.9-wifi/gxcoreapi/ecos3.0/packages/fs/yaffs/v3_0/tests/rtc_1.h"

#define PRINT_SOCKET_ERROR(x) perror(x)
#define OS_STRING "Ubuntu/9.10"
#define MINIUPNPC_VERSION_STRING "1.4"
#define MAXHOSTNAMELEN  64

int ReceiveData(int socket, char * data, int length, int timeout)
{
    int n;
    fd_set socketSet;
    struct timeval timeval;
    FD_ZERO(&socketSet);
    FD_SET(socket, &socketSet);
    timeval.tv_sec = timeout / 1000;
    timeval.tv_usec = (timeout % 1000) * 1000;
    n = select(FD_SETSIZE, &socketSet, NULL, NULL, &timeval);
    if(n < 0)
    {
        PRINT_SOCKET_ERROR("select");
        return -1;
    }
    else if(n == 0)
    {
        return 0;
    }
    n = recv(socket, data, length, 0);
    if(n<0)
    {
        PRINT_SOCKET_ERROR("recv");
    }
    return n;
}

int connecthostport(const char * host, unsigned short port)
{
	int s, n;
#ifdef USE_GETHOSTBYNAME
	struct sockaddr_in dest;
	struct hostent *hp;
#else /* #ifdef USE_GETHOSTBYNAME */
	char port_str[8];
	struct addrinfo *ai, *p;
	struct addrinfo hints;
#endif /* #ifdef USE_GETHOSTBYNAME */
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
	struct timeval timeout;
#endif /* #ifdef MINIUPNPC_SET_SOCKET_TIMEOUT */

#ifdef USE_GETHOSTBYNAME
	hp = gethostbyname(host);
	if(hp == NULL)
	{
		herror(host);
		return -1;
	}
	memcpy(&dest.sin_addr, hp->h_addr, sizeof(dest.sin_addr));
	memset(dest.sin_zero, 0, sizeof(dest.sin_zero));
	s = socket(PF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		PRINT_SOCKET_ERROR("socket");
		return -1;
	}
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
	/* setting a 3 seconds timeout for the connect() call */
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
	{
		PRINT_SOCKET_ERROR("setsockopt");
	}
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
	{
		PRINT_SOCKET_ERROR("setsockopt");
	}
#endif /* #ifdef MINIUPNPC_SET_SOCKET_TIMEOUT */
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	n = connect(s, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
#ifdef MINIUPNPC_IGNORE_EINTR
	while(n < 0 && errno == EINTR)
	{
		socklen_t len;
		fd_set wset;
		int err;
		FD_ZERO(&wset);
		FD_SET(s, &wset);
		if((n = select(s + 1, NULL, &wset, NULL, NULL)) == -1 && errno == EINTR)
			continue;
		/*len = 0;*/
		/*n = getpeername(s, NULL, &len);*/
		len = sizeof(err);
		if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
			PRINT_SOCKET_ERROR("getsockopt");
			closesocket(s);
			return -1;
		}
		if(err != 0) {
			errno = err;
			n = -1;
		}
	}
#endif /* #ifdef MINIUPNPC_IGNORE_EINTR */
	if(n<0)
	{
		PRINT_SOCKET_ERROR("connect");
		closesocket(s);
		return -1;
	}
#else /* #ifdef USE_GETHOSTBYNAME */
	/* use getaddrinfo() instead of gethostbyname() */
	memset(&hints, 0, sizeof(hints));
	/* hints.ai_flags = AI_ADDRCONFIG; */
#ifdef AI_NUMERICSERV
	hints.ai_flags = AI_NUMERICSERV;
#endif
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC; /* AF_INET, AF_INET6 or AF_UNSPEC */
	/* hints.ai_protocol = IPPROTO_TCP; */
	snprintf(port_str, sizeof(port_str), "%hu", port);
	n = getaddrinfo(host, port_str, &hints, &ai);
	if(n != 0)
	{
#ifdef WIN32
		fprintf(stderr, "getaddrinfo() error : %d\n", n);
#else
		fprintf(stderr, "getaddrinfo() error : %s\n", gai_strerror(n));
#endif
		return -1;
	}
	s = -1;
	for(p = ai; p; p = p->ai_next)
	{
		s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(s < 0)
			continue;
#ifdef MINIUPNPC_SET_SOCKET_TIMEOUT
		/* setting a 3 seconds timeout for the connect() call */
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
		{
			PRINT_SOCKET_ERROR("setsockopt");
		}
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0)
		{
			PRINT_SOCKET_ERROR("setsockopt");
		}
#endif /* #ifdef MINIUPNPC_SET_SOCKET_TIMEOUT */
		n = connect(s, p->ai_addr, p->ai_addrlen);
#ifdef MINIUPNPC_IGNORE_EINTR
		while(n < 0 && errno == EINTR)
		{
			socklen_t len;
			fd_set wset;
			int err;
			FD_ZERO(&wset);
			FD_SET(s, &wset);
			if((n = select(s + 1, NULL, &wset, NULL, NULL)) == -1 && errno == EINTR)
				continue;
			/*len = 0;*/
			/*n = getpeername(s, NULL, &len);*/
			len = sizeof(err);
			if(getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
				PRINT_SOCKET_ERROR("getsockopt");
				closesocket(s);
				freeaddrinfo(ai);
				return -1;
			}
			if(err != 0) {
				errno = err;
				n = -1;
			}
		}
#endif /* #ifdef MINIUPNPC_IGNORE_EINTR */
		if(n < 0)
		{
			closesocket(s);
			continue;
		}
		else
		{
			break;
		}
	}
	freeaddrinfo(ai);
	if(s < 0)
	{
		PRINT_SOCKET_ERROR("socket");
		return -1;
	}
	if(n < 0)
	{
		PRINT_SOCKET_ERROR("connect");
		return -1;
	}
#endif /* #ifdef USE_GETHOSTBYNAME */
	return s;
}
/* miniwget3() :
 * do all the work.
 * Return NULL if something failed. */
static void * miniwget3(const char * url, const char * host,
		unsigned short port, const char * path,
		int * size, char * addr, int addr_len, const char * httpversion)
{
	int s;
	int n;
	int len;
	int sent;
	char buf[3096];

	*size = 0;
	s = connecthostport(host, port);
	if(s < 0) {
		return NULL;
	}

	len = snprintf(buf, sizeof(buf),
			"GET %s HTTP/%s\r\n"
			"Host: %s:%d\r\n"
			"Connection: Close\r\n"
			"User-Agent: " OS_STRING ", UPnP/1.0, MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"

			"\r\n",
			path, httpversion, host, port);
	sent = 0;
	/* sending the HTTP request */
	while(sent < len)
	{
		n = send(s, buf+sent, len-sent, 0);
		if(n < 0) {
			perror("send");
			closesocket(s);
			return NULL;
		}
		else {
			sent += n;
		}
	}
	{
		/* TODO : in order to support HTTP/1.1, chunked transfer encoding
		 *        must be supported. That means parsing of headers must be
		 *        added.                                                   */
		int headers=1;
		char * respbuffer = NULL;
		int allreadyread = 0;
		int i =0;
		app_log_min("\nwget receiving.");
		/*while((n = recv(s, buf, 2048, 0)) > 0)*/
		while((n = ReceiveData(s, buf, 3096, 50000)) > 0) {
			if(headers) {
				int i=0;
				while(i<n-3) {
					/* searching for the end of the HTTP headers */
					if(buf[i]=='\r' && buf[i+1]=='\n'
							&& buf[i+2]=='\r' && buf[i+3]=='\n') {
						headers = 0;	/* end */
						if(i<n-4) {
							if (!addr) {
								/* Copy the content into respbuffet */
								respbuffer = (char *)realloc((void *)respbuffer,
										allreadyread+(n-i-4));
							}
							else {
								respbuffer = addr;
							}
							memcpy(respbuffer+allreadyread, buf + i + 4, n-i-4);
							allreadyread += (n-i-4);
						}
						break;
					}
					i++;
				}
			}
			else {
				if (!addr) {
					respbuffer = (char *)realloc((void *)respbuffer,
							allreadyread+n);
				}
				else {
					respbuffer = addr;
				}
				memcpy(respbuffer+allreadyread, buf, n);
				allreadyread += n;
			}
			if (i++%100 == 0)
				app_log_min(".");
		}
		*size = allreadyread;
		app_log_min("end\n");
#ifdef DEBUG
		app_log_min("%d bytes read\n", *size);
#endif
		closesocket(s);
		return respbuffer;
	}
}

/* miniwget2() :
 * Call miniwget3(); retry with HTTP/1.1 if 1.0 fails. */
static void *miniwget2(const char * url, const char * host,
		unsigned short port, const char * path,
		int * size, char *addr, int addr_len)
{
	char * respbuffer;

	respbuffer = miniwget3(url, host, port, path, size, addr, addr_len, "1.0");
	if (*size == 0)
	{
#ifdef DEBUG
		app_log_min("Retrying with HTTP/1.1\n");
#endif
		if (addr)
			free(respbuffer);
		respbuffer = miniwget3(url, host, port, path, size, addr, addr_len, "1.1");
	}
	return respbuffer;
}




/* parseURL()
 * arguments :
 *   url :		source string not modified
 *   hostname :	hostname destination string (size of MAXHOSTNAMELEN+1)
 *   port :		port (destination)
 *   path :		pointer to the path part of the URL
 *
 * Return values :
 *    0 - Failure
 *    1 - Success         */
int parseURL(const char * url, char * hostname, unsigned short * port, char * * path)
{
	char * p1, *p2, *p3;
	p1 = strstr(url, "://");
	if(!p1)
		return 0;
	p1 += 3;
	if(  (url[0]!='h') || (url[1]!='t')
			||(url[2]!='t') || (url[3]!='p'))
		return 0;
	p2 = strchr(p1, ':');
	p3 = strchr(p1, '/');
	if(!p3)
		return 0;
	memset(hostname, 0, MAXHOSTNAMELEN + 1);
	if(!p2 || (p2>p3))
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p3-p1)));
		*port = 80;
	}
	else
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
		*port = 0;
		p2++;
		while( (*p2 >= '0') && (*p2 <= '9'))
		{
			*port *= 10;
			*port += (unsigned short)(*p2 - '0');
			p2++;
		}
	}
	*path = p3;
	return 1;
}

void * miniwget(const char * url, int * size)
{
	unsigned short port;
	char * path;
	/* protocol://host:port/chemin */
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(!parseURL(url, hostname, &port, &path))
		return NULL;
	return miniwget2(url, hostname, port, path, size, 0, 0);
}

void * miniwget_getaddr(const char * url, int * size, char * addr, int addrlen)
{
	unsigned short port;
	char * path;
	/* protocol://host:port/chemin */
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(addr)
		addr[0] = '\0';
	if(!parseURL(url, hostname, &port, &path))
		return NULL;
	return miniwget2(url, hostname, port, path, size, addr, addrlen);
}

int test_wget(char *url, char *addr)
{
	char *pfile;
	//char *url;
	//char *addr;
	int file_length;
	unsigned int addrlen;
	unsigned int time1,time2;
	unsigned int ms1,ms2;
    GxTime sys_time;


	if (url != NULL && addr == NULL) {
		//url = (const char*)argv[0];
		//time1=0;
		//StartTimer();
        GxCore_GetLocalTime(&sys_time);
        time1 = sys_time.seconds;
        ms1 = sys_time.microsecs;
		pfile = miniwget(url, &file_length);
        GxCore_GetLocalTime(&sys_time);
        time2 = sys_time.seconds;
        ms2 = sys_time.microsecs;
		//time2= GetTimerMSec();	// Get timer value in second
		if (pfile) {
			app_log_min("wget ok !\n");
			app_log_min("addr = 0x%p file_length = %d\n",pfile,file_length);
			//app_log_min("wget Speed=%dKB/S\n",(file_length/1024)*1000/(time2-time1));
            if(time2 != time1)
                app_log_min("wget Speed=%dKB/S\n",(file_length/1024)/(time2-time1));
            else
                app_log_min("wget Speed=%dKB/S\n",(file_length/1024)*1000/(ms2-ms1));
		}
		else
			app_log_min("Ivalid argument  \n");
	}
	else if (url != NULL && addr != NULL){
		//url = (const char*)argv[0];
		//addr = (char *)simple_strtoul((const char *)argv[1], NULL, 10);
		addrlen = 0;
		//time1=0;
		//StartTimer();
        GxCore_GetLocalTime(&sys_time);
        time1 = sys_time.seconds;
        ms1 = sys_time.microsecs;
		pfile = miniwget_getaddr(url, &file_length, addr, addrlen);
		//time2= GetTimerMSec();	// Get timer value in second
        GxCore_GetLocalTime(&sys_time);
        time2 = sys_time.seconds;
        ms2 = sys_time.microsecs;
		if (pfile) {
			app_log_min("wget ok !\n");
			app_log_min("addr = 0x%p file_length = %d\n",pfile,file_length);
            if(time2 != time1)
                app_log_min("wget Speed=%dKB/S\n",(file_length/1024)/(time2-time1));
            else
                app_log_min("wget Speed=%dKB/S\n",(file_length/1024)*1000/(ms2-ms1));
		}
		else
			app_log_min("Ivalid argument  \n");
	}
	else {
		app_log_min("wget <url> \n");
		app_log_min("wget <url> <addr> \n");
		return 0;
	}

	return 0;
}

#endif

#endif

