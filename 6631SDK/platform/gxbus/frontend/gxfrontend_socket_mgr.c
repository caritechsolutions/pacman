#include "gxfrontend_socket_udp.h"
#include "module/config/gxconfig.h"

#define MAX_DATA_SIZE    (65424)
#define MAX_SOCKET_NUM    (32)
#define MAX_IP_ADDR_LENGTH    (16)

typedef struct socketMgr_s
{
    int fd;
    int port;
    int timeoutCnt;
    bool isUsing;
    char ipAddr[MAX_IP_ADDR_LENGTH];
    GxFrontendNetProtocal protocal;
    socketMgrProtocalOps_t *protocalOps;
    int (*writeBuffer)(void *fd, char *buffer, unsigned int length);
}socketMgr_t;

static socketMgr_t socketMgr[MAX_SOCKET_NUM] = {{0}};
static handle_t hStreamThread = 0;
static handle_t hMgrMutex = 0;
static bool isRunning = TRUE;
static socketMgrProtocalOps_t *socketMgrProtocals[FRONTEND_NET_MAX] = {NULL};

int gxFrontendRegisterSocketProtocal(GxFrontendNetProtocal protocal, socketMgrProtocalOps_t *ops)
{
    if(protocal >= FRONTEND_NET_MAX || ops == NULL)
    {
        return -1;
    }
    socketMgrProtocals[protocal] = ops;
    return 0;
}

void deInitSocketMgr(void)
{
    int i = 0;
    for(i = 0;i < MAX_SOCKET_NUM;i++)
    {
        gxFrontendDestroySocketFd(&socketMgr[i]);
    }
}

socketMgr_t* getFreeSocketMgr(void)
{
    int i = 0;
    for(i = 0;i < MAX_SOCKET_NUM;i++)
    {
        if(!socketMgr[i].isUsing)
        {
            return &socketMgr[i];
        }
    }
    return NULL;
}

static int checkSocketParam(socketParam_t param)
{
    if(param.destIpAddr == NULL)
    {
        gxlogd("\n[%s] destIpAddr is NULL \n", __func__);
        return -1;
    }
    if(param.port >= 0 && param.port < 1024)
    {
        gxlogd("\n[%s] invalid port is %d,\n", __func__, param.port);
        return -1;
    }
    if(param.protocal >= FRONTEND_NET_MAX)
    {
        gxlogd("\n[%s] invalid protocal  is %d,\n", __func__, param.protocal);
        return -1;
    }
    if(param.func == NULL)
    {
        gxlogd("\n[%s] invalid func  is NULL,\n", __func__);
        return -1;
    }
    return 0;
}

void* gxFrontendCreateSocketFd(socketParam_t param)
{
    socketMgr_t* pSocketMgr = NULL;
    int ret = 0;
    if(-1 == checkSocketParam(param))
    {
        return NULL;
    }
    GxCore_MutexLock(hMgrMutex);
    pSocketMgr = getFreeSocketMgr();
    if(pSocketMgr == NULL)
    {
        gxlogd("\n no free socket Fd \n");
        GxCore_MutexUnlock(hMgrMutex);
        return NULL;
    }
    memset(pSocketMgr->ipAddr, 0x00, MAX_IP_ADDR_LENGTH);
    strncpy(pSocketMgr->ipAddr, param.destIpAddr, MAX_IP_ADDR_LENGTH);
    pSocketMgr->port = param.port;
    pSocketMgr->protocal = param.protocal;
    pSocketMgr->protocalOps = socketMgrProtocals[param.protocal];
    pSocketMgr->writeBuffer = param.func;
    if(NULL == pSocketMgr->protocalOps)
    {
        gxlogd("\n protocalOps is NULL \n");
        GxCore_MutexUnlock(hMgrMutex);
        return NULL;
    }
    ret = pSocketMgr->protocalOps->createSocketFd(param, &(pSocketMgr->fd));
    if(ret < 0)
    {
        gxlogd("\n createSocketFd return error protocal = %d \n", param.protocal);
        GxCore_MutexUnlock(hMgrMutex);
        return NULL;
    }
    pSocketMgr->isUsing = TRUE;
    GxCore_MutexUnlock(hMgrMutex);
    return pSocketMgr;
}

void gxFrontendClearTimeoutCnt(void *fd)
{
    if(fd == NULL)
    {
        return ;
    }
    socketMgr_t *pSocketMgr = (socketMgr_t*)fd;
    pSocketMgr->timeoutCnt = 0;
}

int gxFrontendGetTimeoutCnt(void *fd)
{
    if(fd == NULL)
    {
        return 0;
    }
    socketMgr_t *pSocketMgr = (socketMgr_t*)fd;
    return pSocketMgr->timeoutCnt;
}

void gxFrontendSetMgrUsingState(void* fd, bool state)
{
    socketMgr_t *pSocketMgr = (socketMgr_t*)fd;
    pSocketMgr->isUsing = state;
}

void gxFrontendDestroySocketFd(void* fd)
{
    socketMgr_t *pSocketMgr = NULL;
    socketParam_t param = {0};
    if(fd == NULL)
    {
        return;
    }
    GxCore_MutexLock(hMgrMutex);
    pSocketMgr = (socketMgr_t*)fd;
    if(pSocketMgr && pSocketMgr->isUsing == TRUE)
    {
        pSocketMgr->isUsing = FALSE;
        param.destIpAddr = pSocketMgr->ipAddr;
        param.port = pSocketMgr->port;
        param.protocal = pSocketMgr->protocal;
        if(NULL != pSocketMgr->protocalOps)
        {
            pSocketMgr->protocalOps->destroySocketFd(pSocketMgr->fd, &param);
        }
        param.protocal = 0;
        pSocketMgr->protocalOps = NULL;
        pSocketMgr->fd = 0;
        memset(pSocketMgr->ipAddr, 0x00, MAX_IP_ADDR_LENGTH);
        pSocketMgr->port = 0;
        pSocketMgr->writeBuffer = NULL;
    }
    GxCore_MutexUnlock(hMgrMutex);
}

static void netStreamProc(void *arg)
{
    char* buffer = NULL;
    int nBytes = 0;
    int i = 0;
    int ret = 0;
    int (*readData)(int fd, char *buffer, int length) = NULL;
    buffer = GxCore_Malloc(MAX_DATA_SIZE + 188);
    if(NULL == buffer)
    {
        gxlogd("\n [%s] malloc failed -1 \n", __func__);
        return;
    }
    while(isRunning)
    {
       GxCore_MutexLock(hMgrMutex);
        for(i = 0; i < MAX_SOCKET_NUM; i++)
        {
            if(socketMgr[i].isUsing && socketMgr[i].protocalOps)
            {
                readData = socketMgr[i].protocalOps->readData;
                if(NULL == readData)
                {
                    gxlogd("\nWarning: readData is NULL\n");
                    continue;
                }
                nBytes = readData(socketMgr[i].fd, buffer, MAX_DATA_SIZE);
                if (nBytes > 0)
                {
                    socketMgr[i].timeoutCnt = 0;
                    if(NULL != socketMgr[i].writeBuffer)
                    {
                        ret = socketMgr[i].writeBuffer((void*)&socketMgr[i], buffer, nBytes);
                    }
                }
                else
                {
                    socketMgr[i].timeoutCnt++;
                }
            }
        }
        GxCore_MutexUnlock(hMgrMutex);
        GxCore_ThreadDelay(1);
    }
    GxCore_Free(buffer);
}

void gxFrontendInitNet(void)
{
    gxFrontendRegisterSocketProtocal(FRONTEND_NET_UDP, &socketUdpOps);
    GxCore_MutexCreate(&hMgrMutex);
    GxCore_ThreadCreate("netStreanThread", &hStreamThread, netStreamProc, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY - 1);
}

void gxFrontendDeinitNet(void)
{
    isRunning = FALSE;
    deInitSocketMgr();
    GxCore_MutexDelete(hMgrMutex);
}

char *getConfigLocalNetworkName(void)
{
	int32_t netSource = 0;
	GxBus_ConfigGetInt(FRONTEND_CONFIG_NET_SOURCE, (int32_t *)&netSource , 0);
	return netSource ? "ra0" : "eth0";
}

void getLocalIpAddr(in_addr_t *addr, char* ifa_name)
{
    struct ifaddrs *ifAddrStruct = NULL;
    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct != NULL)
    {
       if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
       {
           if (strcmp(ifa_name,ifAddrStruct->ifa_name) == 0)
           {
               memcpy(addr, &(((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr.s_addr), sizeof(in_addr_t));
           }
       }
       ifAddrStruct = ifAddrStruct->ifa_next;
    }
    return;
}
