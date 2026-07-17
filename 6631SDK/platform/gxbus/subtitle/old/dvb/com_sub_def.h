#ifndef __COM_SUB_DEF_H__
#define __COM_SUB_DEF_H__

#include <gxcore.h>
#include "sub_debug.h"
#include "subtitle/sub_def.h"
#include "gui_core/gui_timer.h"

#define APP_NO_ERROR                        (0)
#define APP_ERR_NO_DATA                     (5)
#define COMMON_PRINTF                       DBG_SUB
#define gxos_thread_delay                   GxCore_ThreadDelay
#define CHIP_GX3002                         0
struct com_subtitle {
	GxSubtitle_Render*  render;
    GxSubtitle_Timer*   timer_ops;
    subt_event_list*    timer;
	int32_t             sync_milliseconds;
	uint8_t*            PixelBufferPtr;
	uint8_t*            ObjectBufferPtr;
	uint8_t*            CompositionBufferPtr;
	uint8_t*            PageBufferPtr; 
	handle_t            lock;
};

typedef uint32_t        AppErr_t ;
typedef uint8_t         u8;
typedef uint16_t        u16;
typedef uint16_t        U16;
typedef uint32_t        u32;
typedef bool            BOOL;

#endif
