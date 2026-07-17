#ifndef __GX_MEDIA_COMMON_H__
#define __GX_MEDIA_COMMON_H__

#define GX_MEDIAAPI_SUCCESS (0)
#define GX_MEDIAAPI_ERROR   (-1)

#define GxMediaAPI_MemAlloc(size, type)          av_memhole_malloc(size, type)
#define GxMediaAPI_MemFree(p, type)              av_memhole_free(p, type)
#define GxMediaAPI_FreezeFrameAlloc(id, addr, size)  GxPlayer_SystemFreezeFrameBufferAlloc(id, &(addr),&(size))

#if(defined(ECOS_OS) || defined(LINUX_OS))

#include "stheader.h"
#include "gx_common.h"


#define GxMediaAPI_SetVideoDisply(rect)    do {    \
	DisplayScreen Screen;                          \
	GxPlayer_SystemGet(PSYS_VOUT_SCREEN, &Screen); \
	rect.x = 0;                                    \
	rect.y = 0;                                    \
	rect.width  = Screen.xres;                     \
	rect.height = Screen.yres;                     \
} while(0)

#define GxMediaAPI_CreateThread(name, id, func, data, size, pri)  GxCore_ThreadCreate(name, id, func, data, size, pri)
#define GxMediaAPI_JoinThread(id)                                 GxCore_ThreadJoin(id)
#else

#include "gx_mem.h"
#include "gx_system.h"
#include "gx_videomem.h"
#include "gx_mediafilter.h"
#include "gxplayer_decoder.h"

#define DELAY10MS            (10)
#define VIDEO_FREEZE_FRAME   (1920 * 1088 * 1.75 + 16 + 1024 * 2)
#define VIDEO_DISPLAY_WIDTH  (1920)
#define VIDEO_DISPLAY_HEIGHT (1080)

#define GxAvdev_CreateDevice(pid)               GxAVCreateDevice(pid)
#define GxAvdev_DestroyDevice(dev)              GxAVDestroyDevice(dev)
#define GxAvdev_OpenModule(dev,type,module)     GxAVOpenModule(dev,type,module)
#define GxAvdev_CloseModule(dev,module)         GxAVCloseModule(dev,module)

#define GxMediaAPI_SetVideoDisply(rect)    do { \
	rect.x = 0;                                 \
	rect.y = 0;                                 \
	rect.width  = VIDEO_DISPLAY_WIDTH;          \
	rect.height = VIDEO_DISPLAY_HEIGHT;         \
} while(0)

#define GxMediaAPI_CreateThread(name, id, func, data, size, pri)  do {  \
	func(data);                                                         \
} while (0)

#define GxMediaAPI_JoinThread(id)                                 do {  \
} while(0)                                                              \

extern int acodec_std2hw(int type);
extern int acodec_hw2std(int type);
extern int vcodec_std2hw(int type);
extern int vcodec_hw2std(int type);

#endif

#ifdef GXDBG
#define GxMediaAPI_debug(...)      gxlogi(__VA_ARGS__ )
#else
#define GxMediaAPI_debug(...)      ((void)0)
#endif

#define CHECK_MEDIA_MOLUDE_VALID(m)                                \
	do {                                                           \
		if (m == NULL) {                                           \
			gxlogd("[%s %d]: is NULL", __FUNCTION__, __LINE__);    \
			return GX_MEDIAAPI_ERROR;                              \
		}                                                          \
		if (m->valid == 0) {                                       \
			gxlogd("[%s %d]: is unvalid", __FUNCTION__, __LINE__); \
			return GX_MEDIAAPI_ERROR;                              \
		}                                                          \
	}while(0)

#endif
