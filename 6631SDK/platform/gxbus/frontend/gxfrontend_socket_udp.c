#include "gxfrontend_socket_udp.h"
#ifdef ECOS_OS
#include <sys/time.h>
#include <sys/select.h>
#endif
#define MAX_RECV_BUFFER_SIZE    (0x10000L)
#define MAX_FD_SIZE    (64)
#define IS_MULTICAST(a)    ((((unsigned long)(a)) & 0x000000F0) == 0x000000E0)
#define LOCAL_NETWORK_NAME    getConfigLocalNetworkName()

static int addLocalIpAddrToMultigroup(int fd, char *destMultiIpAddr, char *LocalIfName)
{
    struct ip_mreq mreq = {{0}};
    int tmpSize = MAX_RECV_BUFFER_SIZE;
    /* use setsockopt() to request that the kernel join a multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(destMultiIpAddr);
    getLocalIpAddr(&(mreq.imr_interface.s_addr), LocalIfName);
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        gxlogd("\n add multigroup failed -1\n");
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &tmpSize, sizeof(tmpSize)) < 0)
    {
        gxlogd("\n setsockopt(SO_SNDBUF) error\n");
    }
    return 0;
}

static int removeLocalIpAddrFromMultigroup(int fd, char* destMultiIpAddr, char * LocalIfName)
{
    struct ip_mreq mreq = {{0}};
    /* use setsockopt() to request that the kernel join a multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(destMultiIpAddr);
    getLocalIpAddr(&(mreq.imr_interface.s_addr), LocalIfName);
    if(setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        gxlogd("\n remove multigroup failed -1\n");
        return -1;
    }
    return 0;
}

static int createSocketFdUdp(socketParam_t param, int *allocFd)
{
    int fd = 0;
    int tmpIpAddr = 0;
    int isMultiCast = 0;
    struct sockaddr_in addr = {0};
    unsigned int yes = 1;
    if(allocFd == NULL)
    {
        return -1;
    }
    gxlogd("\n destIpAddr = %s,port = %d \n",param.destIpAddr,param.port);
    /* create what looks like an ordinary UDP socket */
    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        gxlogd("\n socket return -1 \n");
        return -1;
    }
    /* allow multiple sockets to use the same PORT number */
    if(param.isReuseAddr == TRUE
            &&setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0)
    {
        gxlogd("\n setsockopt return -1\n");
        close(fd);
        return -1;
    }
    /* set up destination address */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(param.port);
    tmpIpAddr = inet_addr(param.destIpAddr);
    isMultiCast = IS_MULTICAST(tmpIpAddr);
    gxlogd("\nisMultiCast=%d\n",isMultiCast);
    addr.sin_addr.s_addr = INADDR_ANY;
    /* bind to receive address */
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
       gxlogd("\nbind error return -1\n");
       close(fd);
       return -1;
    }
    if(isMultiCast)
    {
        if(addLocalIpAddrToMultigroup(fd, param.destIpAddr, LOCAL_NETWORK_NAME) < 0)
        {
            gxlogd("\naddLocalIpAddrToMultigroup return -1\n");
            close(fd);
            return -1;
        }
    }
    *allocFd = fd;
    return 0;
}

static void destroySocketFdUdp(int fd, socketParam_t *param)
{
     int tmpIpAddr = 0;
     tmpIpAddr = inet_addr(param->destIpAddr);
     if(IS_MULTICAST(tmpIpAddr))
     {
         removeLocalIpAddrFromMultigroup(fd, param->destIpAddr, LOCAL_NETWORK_NAME);
     }
     close(fd);
}

static int readDataFromSocketUdp(int fd, char *buffer, int length)
{
    int ret = 0;
    int nBytes = 0;
    fd_set read_set;
    struct timeval timeout = {0};
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);
    ret = select(fd+1, &read_set, NULL, NULL, &timeout);
    if (ret > 0)
    {
        nBytes = recv(fd, buffer, length, 0);
    }
    else if(ret == 0)
    {
        gxlogd("\n socket time out\n");
    }
    return nBytes;
}

socketMgrProtocalOps_t socketUdpOps =
{
    createSocketFdUdp,
    destroySocketFdUdp,
    readDataFromSocketUdp
};

