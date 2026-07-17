#include <stdarg.h>
#include <stdio.h>
#include "cim_platform.h"
#include "gxcore.h"

#ifdef HAVE_LIB_CIM
static int Sem;

// 接口描述
// 创建全局信号量，初始为无信号状态
cim_void cim_semaphore_create(cim_void)
{
	//gxos_semaphore_init(&Sem, 0);
	GxCore_SemCreate(&Sem, 0);
}

cim_void cim_semaphore_post(cim_void)
{
	//gxos_semaphore_post(&Sem);
	GxCore_SemPost(Sem);
}

cim_bool cim_semaphore_wait(cim_uint nTime)
{
	//return gxos_semaphore_timed_wait(&Sem, (nTime + 9) / 10);
	int32_t ret;
	ret = GxCore_SemTimedWait(Sem, nTime/*(nTime + 9) / 10*/);
	if(ret == GXCORE_SUCCESS)return 1;
	else return 0;
}

cim_void cim_get_time(struct cim_time *pTime)
{
	GxTime tGxTime;
	GxCore_GetLocalTime(&tGxTime);
	pTime->m_nTimeL = tGxTime.seconds;
	pTime->m_nTimeH = 0;
	//com_time_get_time(&pTime->m_nTimeL);
	//pTime->m_nTimeH = 0;
}

struct cim_icam cim_icams[_CIM_MODULE_CNT_] =
{
	{
		NULL,//CAM0_InitDevice,
		NULL,//CAM0_DetectCard,
		NULL,//CAM0_ResetCard,
		NULL,//CAM0_EnableTSI,
		NULL,//CAM0_ReadMem,
		NULL,//CAM0_WriteMem,
		NULL,//CAM0_ReadIO,
		NULL,//CAM0_WriteIO
	},
};


#endif
///////////////////////////////////////////////////////////////////////////////
// end of cim_platform.c

