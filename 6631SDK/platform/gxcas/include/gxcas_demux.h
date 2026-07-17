#ifndef __GXCAS_DEMUX_H__
#define __GXCAS_DEMUX_H__

#include "gxtype.h"
#include "autoconf.h"

#define MAX_FILTER_COUNT (64)
#define MAX_DEMUX_NUM		(4)

typedef struct ca_filter_s {
	uint16_t      pid;
	uint8_t       match[18] ;
	uint8_t       mask[18] ;
	uint8_t       depth;
#define  REPEAT_FLAG       (1<<0)
#define  EQ_FLAG           (1<<1)
#define  CRC_FLAG          (1<<2)
	uint32_t      flags;
	int32_t       nWaitSeconds;  /*设置的超时最大时间,单位s*/
	void*         user;
}sifilter_params_t;

void GxCas_SiFilter_Init(void);
handle_t GxCas_SiFilter_Start(sifilter_params_t params);
int32_t GxCas_SiFilter_Stop(handle_t handle);
void GxCas_SiFilter_Reset(handle_t handle);
int32_t GxCas_SiFilter_GetInfo(handle_t handle, sifilter_params_t *params);
int32_t GxCas_SiFilter_EmmEcm_CheckCrc(uint8_t* pData);
int32_t GxCas_SiFilter_EmmEcm_CheckSame(uint8_t* pData);
int32_t GxCas_SiFilter_CheckCrc(uint8_t *asection);
int32_t GxCas_SiFilter_Read(handle_t handle, uint8_t *buffer, uint32_t size, uint32_t *len);

handle_t GxCas_Pes_Start(uint16_t      pid);
int32_t GxCas_Pes_Stop(handle_t handle);


int32_t GxCas_Demux_Init(uint32_t tsid, uint32_t dmxid);
int32_t GxCas_ChannelFree(handle_t handle);
int32_t GxCas_ChannelSetPID(handle_t handle, uint16_t pid, bool repeat);
int32_t GxCas_ChannelEnable(handle_t handle);
int32_t GxCas_ChannelDisable(handle_t  handle);
#ifdef CONFIG_ABV_PVR
#ifdef CONFIG_DMX_OLD
int32_t GxCas_Demux_Destroy(uint32_t dmxid);
#else
int32_t GxCas_Demux_Destroy(void);
#endif
handle_t GxCas_ChannelAllocate(uint16_t pid, uint32_t dmxid);
handle_t GxCas_ChannelGetByPid(uint16_t pid, uint32_t dmxid);
#else
int32_t GxCas_Demux_Destroy(void);
handle_t GxCas_ChannelAllocate(uint16_t pid);
handle_t GxCas_PesChannelAllocate(uint16_t pid);
handle_t GxCas_ChannelGetByPid(uint16_t pid);
#endif
handle_t GxCas_FilterAllocate(handle_t handle);
int32_t GxCas_FilterGetPID(handle_t handle,int16_t *pid);
int32_t GxCas_FilterFree(handle_t  handle);
int32_t GxCas_FilterSetup(handle_t handle, const uint8_t *match, const uint8_t *mask,
		bool Equal, bool CRCFlag, size_t Depth);
int32_t GxCas_FilterEnable(handle_t handle);
int32_t GxCas_FilterDisable(handle_t handle);
int32_t GxCas_QueryAndGetData(handle_t handle, uint8_t *buffer, uint32_t size, uint32_t *len);

#endif

