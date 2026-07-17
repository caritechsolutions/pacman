/**
 *
 * @file        gxsubtitle_def.h
 * @brief
 * @version:    1.1.0
 * @date        04/08/2010 08:51:42 AM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#ifndef __GX_SUBTITTLE_DEF_H__
#define __GX_SUBTITTLE_DEF_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "gxcore.h"
#include "subtitle/sub_def.h"
#include "sub_debug.h"

//#define __SEGMENT_DEBUG__
//#define __PAGE_DEBUG__
#define __REGION_DEBUG__
//#define __CLUT_DEBUG__
//#define __OBJECT_DEBUG__
//#define __DISPLAY_DEBUG__
//#define __DECODE_DEBUG__

#ifdef __SEGMENT_DEBUG__
#define DBGSegment(...)                 DBG(__VA_ARGS__)
#else
#define DBGSegment(...)
#endif

#ifdef __PAGE_DEBUG__
#define DBGPage(...)                    DBG(__VA_ARGS__)
#else
#define DBGPage(...)
#endif

#ifdef __CLUT_DEBUG__
#define __CLUT_DEBUG__
#define DBGClut(...)                    DBG(__VA_ARGS__)
#else
#define DBGClut(...)
#endif

#ifdef __REGION_DEBUG__
#define DBGRegion(...)                  DBG(__VA_ARGS__)
#else
#define DBGRegion(...)
#endif

#ifdef __OBJECT_DEBUG__
#define DBGObject(...)                  DBG(__VA_ARGS__)
#else
#define DBGObject(...)
#endif

#ifdef __DISPLAY_DEBUG__
#define DBGDisplay(...)                 DBG(__VA_ARGS__)
#else
#define DBGDisplay(...)
#endif

#ifdef __DECODE_DEBUG__
#define DBGDecode(...)                  DBG(__VA_ARGS__)
#else
#define DBGDecode(...)
#endif

extern GxSubtitle_DecoderOps       gxsub_dvb_decoder;

#ifdef __cplusplus
}
#endif
#endif

