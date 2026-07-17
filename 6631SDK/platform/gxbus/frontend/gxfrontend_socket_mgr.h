#ifndef _GXFRONTEND_SOCKET_MGR_H_
#define _GXFRONTEND_SOCKET_MGR_H_
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ifaddrs.h>
#include "gxcore.h"
#include "module/frontend/gxfrontend_module.h"

#ifdef MIN
#undef MIN
#define MIN(x,y) ((x)<(y)? (x):(y))
#else
#define MIN(x,y) ((x)<(y)? (x):(y))
#endif

typedef enum{
 	FRONTEND_NET_NONE,
	FRONTEND_NET_UDP,
	FRONTEND_NET_TCP,
	FRONTEND_NET_RAW,
	FRONTEND_NET_MAX
}GxFrontendNetProtocal;

typedef int (*writeBuffer)(void *fd, char *buffer, unsigned int length);

typedef struct socketParam_s
{
    char* destIpAddr;
    int port;
    bool isReuseAddr;
    GxFrontendNetProtocal protocal;
    writeBuffer func;
}socketParam_t;

typedef struct socketMgrProtocalOps_s
{
    int (*createSocketFd)(socketParam_t param, int *fd);
    void (*destroySocketFd)(int fd, socketParam_t *param);
    int (*readData)(int fd,char *buffer,int length);
}socketMgrProtocalOps_t;

void* gxFrontendCreateSocketFd(socketParam_t socketParam);

void gxFrontendDestroySocketFd(void *fd);

void gxFrontendInitNet(void);

void gxFrontendDeinitNet(void);

int gxFrontendGetTimeoutCnt(void *fd);

void gxFrontendClearTimeoutCnt(void *fd);

int gxFrontendRegisterSocketProtocal(GxFrontendNetProtocal protocal, socketMgrProtocalOps_t *ops);

char *getConfigLocalNetworkName(void);

void getLocalIpAddr(in_addr_t *addr, char *ifa_name);

#endif
