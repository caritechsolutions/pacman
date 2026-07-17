/************************************************************ 
Copyright (C), 2007-2009, GX S&T Co., Ltd. 
FileName   :	debug_Service.c
Author     : 	zhangling
Version    : 	1.0
Date	   :
Description:	
Version    :	
History    :	
Date				Author  	Modification 
2007.03.14     zhangling		 create
***********************************************************/

/* Includes --------------------------------------------------------------- */


#include <stdlib.h>
#include <stdio.h>
#include "gxcore.h"
#include "gxbus.h"
#include "module/config/gxconfig.h"
#include "service/gxdebug.h"
#ifdef ECOS_OS
#include <cyg/kernel/kapi.h>
#endif

//#include "kernelcalls.h"
/* Exported Macros -------------------------------------------------------- */
/* Exported Types --------------------------------------------------------- */
/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Private Macros --------------------------------_------------------------ */

/* Private Types ---------------------------------------------------------- */
/* Private Constants ------------------------------------------------------ */

/* Private Variables ------------------------------------------------------ */
/* Debug Defined ---------------------------------------------------------- */

/* Private Functions ------------------------------------------------------ */
/* Exported Functions ----------------------------------------------------- */

/**
 * @brief 初始化搜索服务
 * @param
 * @Return
 */
status_t GxDebugInit(handle_t self,int priority_offset)
{
	handle_t                sch;
	sch = GxBus_SchedulerCreate("DebugConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);       
	GxBus_ServiceLink(self, sch);
	return 0;
}

/**
 * @brief 销毁搜索服务
 * @param
 * @Return
 */
void GxDebugDestroy(handle_t self)
{
	GxBus_ServiceUnlink(self);
	return;
}

#ifdef ECOS_OS
uint8_t *md5_1 = NULL;
uint8_t *md5_2 = NULL;
#define CODE_CHECK_TIME (30)//每30秒检测一次
void GxDebugServiceConsole(handle_t self)
{

	struct gx_malloc_info *heap;
	extern struct gx_malloc_info *gx_malloc_info_get(void);
	uint32_t j = 0;
	int32_t ctr = 0;

	GxBus_ConfigGetInt(GXBUS_DEBUG_CTR, &ctr, GXBUS_DEBUG_WATCH_CODE);

	if ((ctr & GXBUS_DEBUG_WATCH_HEAP) !=0)
	{
		/*watch heap*/
		heap = (struct gx_malloc_info *)gx_malloc_info_get();
		gxlogd("\n------------------------------\n");
		gxlogd("heap.heap_total = 0x%x\n",heap->heap_total);
		gxlogd("heap.heap_blocks = 0x%x\n",heap->heap_blocks);
		gxlogd("heap.heap_allocated = 0x%x\n",heap->heap_allocated);
		gxlogd("heap.heap_free = 0x%x\n",heap->heap_free);
		gxlogd("heap.heap_maxfree = 0x%x\n",heap->heap_maxfree);
		gxlogd("--------------------------------\n");
	}

	if ((ctr & GXBUS_DEBUG_WATCH_CODE) !=0)
	{
		/*watch code*/
		if (j == CODE_CHECK_TIME)
		{
			if (md5_2 == NULL)
			{
				md5_2 = GxCore_Malloc(MD5_DIGEST_LENGTH);
			}
			GxCore_CodeMd5(md5_2);
			if (md5_1 != NULL)
			{
				if(memcmp(md5_1, md5_2, MD5_DIGEST_LENGTH) != 0)
				{
					while(j-- != 0)
					{
						gxlogd("!!!!!!!!!!!!!!!!!!!!!memory err! The code in memory is changed!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					}
				}
			}
			else
			{
				md5_1 = GxCore_Malloc(MD5_DIGEST_LENGTH);
				memcpy(md5_1, md5_2, MD5_DIGEST_LENGTH);
			}
			j = 0; 
		}
		j++;
	}
	GxCore_ThreadDelay(1000);
}
#endif

#ifdef LINUX_OS
void GxDebugServiceConsole(handle_t self)
{
	while(1)
	{
		GxCore_ThreadDelay(10000);
	}
}
#endif
GxServiceClass debug_service = {
	"debug service",
	GxDebugInit,
	GxDebugDestroy,
	NULL,
	GxDebugServiceConsole,
};
