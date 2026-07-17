#include "gx_common.h"
#include "../vod_include/vod_common_def.h"

#ifdef LINUX_OS
#include <linux/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#endif

/*************************************************
  说明：
  连接服务器socket
  定义：
  int vod_porting_tcp_socket_create ( void )
  参数：
domain: AF_INET or AF_UNSPEC
type：SOCK_STREAM or SOCK_DGRAM
protocol：TCP or UDP 0 for default
返回：
<0：失败
>=0: 成功.
 **************************************************/
int vod_porting_tcp_socket_create(void)
{
#ifdef LINUX_OS
	int sock = -1;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	return sock;
#endif
	return -1;
}
/*************************************************
  说明：
  连接服务器socket
  定义：
  int vod_porting_udp_socket_create ( void )
  参数：
domain: AF_INET or AF_UNSPEC
type：SOCK_STREAM or SOCK_DGRAM
protocol：TCP or UDP 0 for default
返回：
<0：失败
>=0: 成功.
 **************************************************/
int vod_porting_udp_socket_create(void)
{
#ifdef LINUX_OS
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	return sock;
#endif
	return -1;
}

int vod_porting_socket_bind(int sock, unsigned long addr, unsigned short port)
{
#ifdef LINUX_OS
	struct sockaddr_in socka;
	memset(&socka, 0, sizeof(socka));
	socka.sin_family = AF_INET;
	socka.sin_addr.s_addr = addr;
	socka.sin_port = htons(port);
	return bind(sock, (struct sockaddr *)&socka, sizeof(socka));
#endif
	return -1;
}

int vod_porting_socket_join_multicast(int sock, unsigned long local_addr, unsigned long mcast_addr)
{
#ifdef LINUX_OS
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = mcast_addr;
	mreq.imr_interface.s_addr = local_addr;
	return setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
#endif
	return -1;
}
int vod_porting_socket_leave_multicast(int sock, unsigned long local_addr, unsigned long  mcast_addr)
{
#ifdef LINUX_OS
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = mcast_addr;
	mreq.imr_interface.s_addr = local_addr;
	return setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
#endif
	return -1;
}




/***************************************************
  说明:
  TCP用于连接SOCKET
parameter:
sock:socket id
name:server param:ip & port
namelen: sizeof(*name)
return:
>=0:ok
< 0:fail
 ***************************************************/
//int vod_porting_socket_connect(int sock, struct sockaddr * name, int namelen)
int vod_porting_socket_connect(int sock, unsigned short sport, unsigned long saddr)
{
#ifdef LINUX_OS
	struct sockaddr_in sockaddr;
	struct in_addr inadd;

	sockaddr.sin_family = AF_INET;

	sockaddr.sin_port = htons(sport);
	inadd.s_addr = saddr;
	sockaddr.sin_addr = inadd;

	{
		int error=-1, len;
		struct timeval tm;
		fd_set set;
		unsigned long ul = 1;

		len = sizeof(int);
		ioctl(sock, FIONBIO, &ul);
		int ret = 0;
		ret = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
		if(ret < 0)
		{
			if (errno != EINPROGRESS && errno != EAGAIN)//说明服务器链接不上
				return -2;
			tm.tv_sec = 5;
			tm.tv_usec = 0;
			FD_ZERO(&set);
			FD_SET(sock, &set);
			if( select(sock+1, NULL, &set, NULL, &tm) > 0)
			{
				getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
				if(error)
				{
					return -2;
				}
			}
			else
			{
				return -3;
			}
		}

		ul = 0;
		ioctl(sock, FIONBIO, &ul);
	}
	return 0;
#endif
	return -1;
}

/*************************************************
  说明：
  查询socket状态
  定义：
  int vod_porting_socket_select (int nfds, fd_set *readfds, fd_set *writefds, 
  fd_set *exceptfds, struct timeval_t *timeout)
  参数：
  参见LINUX下select
  返回：
  等同于标准函数select.
 **************************************************/
int vod_porting_socket_select (int sock, unsigned int timeout_ms)
{
#ifdef LINUX_OS
	fd_set read_set;
	struct timeval timeout;
	int ret = 0;

	if (timeout_ms != 0)
	{
		FD_ZERO(&read_set);
		FD_SET(sock, &read_set);
		timeout.tv_sec = (unsigned int)(((long)timeout_ms)/(long)1000);
		timeout.tv_usec = (unsigned int)((long)timeout_ms % 1000) * (long)1000;
	}
	else
	{
		return 1;
	}

	ret =  select(sock+1,&read_set, NULL, NULL, &timeout);
	if(ret > 0){
		if(FD_ISSET(sock, &read_set))
			return ret;
		else
			return 0;
	}
	return ret;
#endif
	return -1;
}

/*************************************************
  说明：
  Socket接收数据
  定义：
  int vod_porting_socket_recv (int socketid, void * recvbuf, int recvlen)
  参数：
Socketid : 句柄
Recvbuf：数据存放地址
Recvlen：数据接收最大长度
返回：
Recv数据长度.
 **************************************************/
int vod_porting_socket_recv (int sock, void * recvbuf, int recvlen)
{
#ifdef LINUX_OS
	return recv(sock,recvbuf,recvlen,0);
#endif
	return -1;
}
/*************************************************
  说明：
  Socket接收数据
  定义：
  int vod_porting_socket_recv (int socketid, void * recvbuf, int recvlen)
  参数：
Socketid : 句柄
Recvbuf：数据存放地址
Recvlen：数据接收最大长度
返回：
Recv数据长度.
 **************************************************/
int vod_porting_socket_recvfrom (int sock, unsigned short* port, unsigned long* addr,void * recvbuf, int recvlen)
{
#ifdef LINUX_OS
	int ret, len;
	struct sockaddr_in socka;
	memset(&socka, 0, sizeof(socka));
	len = sizeof(socka);
	ret = recvfrom(sock,recvbuf,recvlen,0,(struct sockaddr *)&socka, (socklen_t*)&len);
	if (ret > 0)
	{
		*port = ntohs(socka.sin_port);
		*addr = socka.sin_addr.s_addr;
	}
	return ret;
#endif
	return -1;
}

/*************************************************
  说明：
  Socket发送数据
  定义：
  int vod_porting_socket_send (int socketid, void * sendbuf, int sendlen)
  参数：
Socketid : 句柄
sendbuf：数据发送地址
sendlen：数据发送长度
返回：
Send成功数据长度.
 **************************************************/
int vod_porting_socket_send (int sock, void * sendbuf, int sendlen)
{
#ifdef LINUX_OS
	int ret = 0;
	{
		fd_set fd;
		struct timeval tv = {1,   0};

		FD_ZERO(&fd);
		FD_SET(sock,&fd);
		ret = select(sock+1,   NULL,   &fd,   NULL,   &tv);
		if(ret <= 0){
			gxlogd("%s %d errno %d....\n", __FUNCTION__, __LINE__, errno);
			return -1;
		}

		if (!FD_ISSET(sock, &fd)){
			gxlogd("%s %d errno %d....\n", __FUNCTION__, __LINE__, errno);
			return -1;
		}

		ret = send(sock, (char *)sendbuf, sendlen, 0);
		if ( ret != sendlen )
		{
			gxlogd("%s %d errno %d....\n", __FUNCTION__, __LINE__, errno);
			return -1;
		}
	}
	return ret;
#endif
	return -1;
}
/*************************************************
  说明：
  Socket发送数据
  定义：
  int vod_porting_socket_sendto(int socketid, void * sendbuf, int sendlen)
  参数：
Socketid : 句柄
sendbuf：数据发送地址
sendlen：数据发送长度
返回：
Send成功数据长度.
 **************************************************/
int vod_porting_socket_sendto(int sock, unsigned short port, unsigned long addr,void * sendbuf, int sendlen)
{
#ifdef LINUX_OS
	int ret = 0;
	int len = 0;
	struct sockaddr_in ser_addr = {0};
	ser_addr.sin_port = htons(port);
	ser_addr.sin_addr.s_addr = addr;
	len = sizeof(ser_addr);

	if(sock >= 0)
	{
		ret = sendto(sock, (char *)sendbuf, sendlen, 0,(struct sockaddr*)&ser_addr,len);
	}
	if ( ret != sendlen )
	{
		return -1 ;
	}
	return ret;
#endif
	return -1;
}

/*************************************************
  说明：
  关闭socket
  定义：
  int vod_porting_ socket_close (int socketid)
  参数：
Socketid : 句柄
返回：
等同于标准函数close
 **************************************************/
int vod_porting_socket_close (int socketid)
{
#ifdef LINUX_OS
	closesocket(socketid);
	return 0;
#endif
	return -1;
}

unsigned long vod_porting_server_name_to_addr(char * servername)
{
#ifdef LINUX_OS
	struct addrinfo hints, *ai;
	struct in_addr s_addr;
	struct sockaddr_in* snip;
	if ( servername == NULL )
	{
		return 0;
	}
	if (inet_aton(servername, &s_addr) != 0)
	{
		return s_addr.s_addr;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int ret = getaddrinfo(servername, NULL, &hints, &ai);
	if (ret) {
		gxlogd("hostname error %s\n", gai_strerror(ret));
		return 0;
	}
	snip = (struct sockaddr_in*)ai->ai_addr;
	s_addr = snip->sin_addr;
	freeaddrinfo(ai);
	return s_addr.s_addr;
#endif
	return 0;
}
/*************************************************************
  说明：

  定义：
  unsigned int vod_porting_htonl(unsigned int data)
  参数：

  返回：
 **************************************************************/

unsigned int vod_porting_htonl(unsigned int data)
{
#ifdef LINUX_OS
	return (htonl(data));
#endif
	return 0;
}
/*************************************************************
  说明：

  定义：
  unsigned short vod_porting_htons(unsigned short data)
  参数：

  返回：
 **************************************************************/

unsigned short vod_porting_htons(unsigned short data)
{
#ifdef LINUX_OS
	return (htons(data));
#endif
	return 0;
}
/*************************************************************
  说明：

  定义：
  unsigned int vod_porting_ntohs(unsigned int invalue);
  参数：

  返回：
 **************************************************************/
unsigned int vod_porting_ntohs(unsigned int invalue)
{
#ifdef LINUX_OS
	return ntohs(invalue);
#endif
	return 0;
}

/*************************************************************
  说明：

  定义：
  unsigned int vod_porting_ntohl(unsigned int invalue);
  参数：

  返回：
 **************************************************************/
unsigned int vod_porting_ntohl(unsigned int invalue)
{
#ifdef LINUX_OS
	return ntohl(invalue);
#endif
	return 0;
}
/*************************************************************
  说明：
  定义：
  const char * vod_porting_inet_addr (void);
  参数：

  返回：
 **************************************************************/
unsigned long vod_porting_inet_addr ( char *dotted )
{
#ifdef LINUX_OS
	return inet_addr(dotted);
#endif
	return 0;
}
