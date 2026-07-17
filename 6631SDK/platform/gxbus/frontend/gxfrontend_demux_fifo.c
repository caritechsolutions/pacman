#include "gxcore.h"
#include "av/avapi.h"
#include "module/berkeley/compat.h"
#define GXFRONTEND_DEMUX_MAX_FIFO_NUM  (32)
#define GXFRONTEND_DEMUX_TS_SIZE (2 * 188 * 1024)
typedef struct gxFrontendFifoMgr_s
{
    char isUsed;
    int dev;
    handle_t hFrontend;
    handle_t module;
    handle_t hFifo;
}gxFrontendFifoMgr_t;

static handle_t hFrontendFifoMutex = 0;
static gxFrontendFifoMgr_t gxFrontendFiFo[GXFRONTEND_DEMUX_MAX_FIFO_NUM] = {{0}};

static gxFrontendFifoMgr_t *allocateFrontendFifoMgr(void)
{
    int i = 0;
    for(i = 0; i < GXFRONTEND_DEMUX_MAX_FIFO_NUM; i++)
    {
        if(!gxFrontendFiFo[i].isUsed)
        {
            gxFrontendFiFo[i].isUsed = 1;
            return &gxFrontendFiFo[i];
        }
    }
    return NULL;
}

static void freeFrontendFifoMgr(gxFrontendFifoMgr_t *fifoMgr)
{
    int i = 0;
    for(i = 0; i < GXFRONTEND_DEMUX_MAX_FIFO_NUM; i++)
    {
        if((void*)(&gxFrontendFiFo[i]) == (void*)(fifoMgr))
        {
            memset(fifoMgr, 0x00, sizeof(gxFrontendFifoMgr_t));
        }
    }
}

static gxFrontendFifoMgr_t *checkFrontendFifoMgr(handle_t module)
{
    int i = 0;
    for(i = 0; i < GXFRONTEND_DEMUX_MAX_FIFO_NUM; i++)
    {
        if(module == gxFrontendFiFo[i].module)
        {
            return &gxFrontendFiFo[i];
        }
    }
    return NULL;
}

void gxfrontend_initFifo(void)
{
    GxCore_MutexCreate(&hFrontendFifoMutex);
}

int gxfrontend_createFifo(handle_t hFrontend, int dev, handle_t module)
{
    handle_t hFifo = 0;
    GxAvGateInfo ts_info = {0};
    gxFrontendFifoMgr_t *fifoMgr = NULL;
    if(hFrontend == 0)
    {
        return -1;
    }
    GxCore_MutexLock(hFrontendFifoMutex);
    fifoMgr = allocateFrontendFifoMgr();
    if(NULL == fifoMgr)
    {
        GxCore_MutexUnlock(hFrontendFifoMutex);
        return -1;
    }
    hFifo = GxAVFifoCreate(dev, NULL, GXFRONTEND_DEMUX_TS_SIZE, GXAV_NO_PTS_FIFO);
    if(0 == hFifo)
    {
        freeFrontendFifoMgr(fifoMgr);
        GxCore_MutexUnlock(hFrontendFifoMutex);
        return -1;
    }
    ts_info.almost_empty = GXFRONTEND_DEMUX_TS_SIZE * 7 / 8;
    ts_info.almost_full = 188 * 10;
    GxAVFifoConfig(dev, hFifo, &ts_info);
    GxAVFifoReset(dev, hFifo);
    GxAVLinkFifo(dev, module, hFifo, PIN_ID_SDRAM_INPUT, GXAV_PIN_INPUT);
    fifoMgr->dev = dev;
    fifoMgr->module = module;
    fifoMgr->hFifo = hFifo;
    fifoMgr->hFrontend = hFrontend;
    GxCore_MutexUnlock(hFrontendFifoMutex);
    return 0;
}

int gxfrontend_destroyFifo(handle_t module)
{
    gxFrontendFifoMgr_t *fifoMgr = NULL;
    GxCore_MutexLock(hFrontendFifoMutex);
    fifoMgr = checkFrontendFifoMgr(module);
    if(NULL == fifoMgr)
    {
        GxCore_MutexUnlock(hFrontendFifoMutex);
        return -1;
    }
    GxAVUnLinkFifo(fifoMgr->dev, fifoMgr->module, fifoMgr->hFifo);
    GxAVFifoDestroy(fifoMgr->dev, fifoMgr->hFifo);
    freeFrontendFifoMgr(fifoMgr);
    GxCore_MutexUnlock(hFrontendFifoMutex);
    return 0;
}

int gxfrontend_writeFifo(void *hFrontend, char *buffer, uint32_t length)
{
    int i = 0;
    int freeSize = 0;
    int writeSize = 0;
    int ret = 0;
    int dev = -1;
    handle_t hFifo = -1;
    GxCore_MutexLock(hFrontendFifoMutex);
    for(i = 0; i < GXFRONTEND_DEMUX_MAX_FIFO_NUM; i++)
    {
        if(gxFrontendFiFo[i].isUsed && gxFrontendFiFo[i].hFrontend == (handle_t)hFrontend)
        {
            dev = gxFrontendFiFo[i].dev;
            hFifo = gxFrontendFiFo[i].hFifo;
            freeSize = GxAVFifoGetFreeSize(dev, hFifo);
            if(freeSize > 0)
            {
                writeSize = MIN(freeSize, length);
                ret = GxAVFifoWriteData(dev, hFifo, buffer, writeSize, -1);
                if(ret != writeSize)
                {
                    GxCore_MutexUnlock(hFrontendFifoMutex);
                    return -1;
                }

            }
        }
    }
    GxCore_MutexUnlock(hFrontendFifoMutex);
    return 0;
}




