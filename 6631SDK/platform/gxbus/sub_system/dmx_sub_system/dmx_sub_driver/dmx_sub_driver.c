/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	dmx_sub_system.c
* Author    : 	zhangling
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.01	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "gxcore.h"
#include "gxavdev.h" 
#include "../include/dmx_sub_driver.h"

handle_t gx_dmx_sub_driver_open_device(uint32_t device_id)
{
    return GxAvdev_CreateDevice(0);
}

handle_t gx_dmx_sub_driver_open_module(uint32_t device,uint32_t  module_id)
{
    return GxAvdev_OpenModule(device, GXAV_MOD_DEMUX, module_id);
}

int32_t gx_dmx_sub_driver_config(handle_t device,handle_t demux,uint32_t ts)
{
    GxDemuxProperty_ConfigDemux config_demux;

    config_demux.source = ts;
    config_demux.ts_select = FRONTEND;
    config_demux.stream_mode = DEMUX_PARALLEL;
    config_demux.time_gate = 0xf;
    config_demux.byt_cnt_err_gate = 0x03;
    config_demux.sync_loss_gate = 0x03;
    config_demux.sync_lock_gate = 0x03;
    return GxAVSetProperty(device,demux,GxDemuxPropertyID_Config,&config_demux,sizeof(GxDemuxProperty_ConfigDemux));
}

int32_t gx_dmx_sub_driver_close_module(handle_t device,handle_t module)
{
    return GxAvdev_CloseModule(device,module);
}

int32_t gx_dmx_sub_driver_free_demux(int32_t dmx_id)
{
    return GxAvdev_FreeDemux(dmx_id);
}

int32_t gx_dmx_sub_driver_close_device(handle_t device)
{
    return GxAvdev_DestroyDevice(device);
}

int32_t gx_dmx_sub_driver_modify_filter(GxSubsystemFilterCtr* filter,GxSubsystemDmxAllocFilter* para)
{
    GxDemuxProperty_Filter dmx_filter_prop = {0};
    GxSubsystemDmxCtr* dmx = (GxSubsystemDmxCtr*)(filter->dmx);
    GxSubsystemSlotCtr* slot = (GxSubsystemSlotCtr*)(filter->slot);
    int32_t ret = 0;
    uint32_t i = 0;
    
	dmx_filter_prop.slot_id = slot->id;
	dmx_filter_prop.filter_id = filter->id; 
    dmx_filter_prop.depth = para->match_depth;

    for (i=0; i<para->match_depth; i++)
    {
        dmx_filter_prop.key[i].value = para->match[i];
        dmx_filter_prop.key[i].mask = para->mask[i];
    }

    (para->eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH) ? (dmx_filter_prop.flags = DMX_EQ):(dmx_filter_prop.flags = 0);

    if (para->crc != SUBSYSTEM_DMX_CRC_OFF)
    {
	    dmx_filter_prop.flags |= DMX_CRC_IRQ;
    }
    if (para->soft_filter == SUBSYSTEM_DMX_SOFT_ON)
    {
	    dmx_filter_prop.flags |= DMX_SW_FILTER;
	}
	ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterConfig,
			(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub config filter err !!\n");
#endif
        return -1;
    }
    return 0;
}
int32_t gx_dmx_sub_driver_alloc_filter(GxSubsystemDmxCtr* dmx,GxSubsystemSlotCtr* slot,GxSubsystemDmxAllocFilter* para)
{
    GxDemuxProperty_Filter dmx_filter_prop = {0};
    int32_t ret = 0;
    uint32_t i = 0;
    dmx_filter_prop.slot_id = slot->id;
	if(para->sw_buffer_size > 64*1024)
	{
		dmx_filter_prop.sw_buffer_size = para->sw_buffer_size;
	}

    ret = GxAVGetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterAlloc,
                                (void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub driver  alloc filter err !!\n");
#endif
        return -1;
    }
    dmx_filter_prop.depth = para->match_depth;

    for (i=0; i<para->match_depth; i++)
    {
        dmx_filter_prop.key[i].value = para->match[i];
        dmx_filter_prop.key[i].mask = para->mask[i];
    }

    (para->eq_or_neq == SUBSYSTEM_DMX_EQ_MATCH) ? (dmx_filter_prop.flags = DMX_EQ):(dmx_filter_prop.flags = 0);

    if (para->crc != SUBSYSTEM_DMX_CRC_OFF)
    {
	    dmx_filter_prop.flags |= DMX_CRC_IRQ;
    }
    if (para->soft_filter == SUBSYSTEM_DMX_SOFT_ON)
    {
	    dmx_filter_prop.flags |= DMX_SW_FILTER;
	}
	ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterConfig,
			(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub config filter err !!\n");
#endif
        return -1;
    }
    return dmx_filter_prop.filter_id;
}

int32_t gx_dmx_sub_driver_alloc_slot(GxSubsystemDmxCtr* dmx,GxSubsystemDmxAllocFilter* para)
{
    GxDemuxProperty_Slot dmx_slot_prop = {0};
    int32_t ret = 0;

	if(para->type == GX_SUB_DMX_FILTER_TYPE_PSI)
	{
		dmx_slot_prop.type = DEMUX_SLOT_PSI;
	}
	else
	{
		dmx_slot_prop.type = DEMUX_SLOT_PES;
	}

	dmx_slot_prop.pid = para->pid;
    ret = GxAVGetProperty(dmx->device,dmx->module, GxDemuxPropertyID_SlotAlloc,
                            (void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot)); 
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub driver alloc slot err !!\n");
#endif  
        return -1;
    } 
    dmx_slot_prop.pid = para->pid;
    dmx_slot_prop.flags = (DMX_REPEAT_MODE | DMX_AVOUT_EN);

    ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_SlotConfig,
                            (void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
    if(ret < 0)
    {                       
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub config slot err !!\n");
#endif  
        return -1;
    }
    return dmx_slot_prop.slot_id;
}

int32_t gx_dmx_sub_driver_start_filter(GxSubsystemFilterCtr* filter)
{
    GxSubsystemDmxCtr* dmx = NULL;
    GxSubsystemSlotCtr* slot = NULL;
    GxDemuxProperty_Slot dmx_slot_prop;
    GxDemuxProperty_Filter dmx_filter_prop;
    int32_t ret = 0;

    dmx = (GxSubsystemDmxCtr*)(filter->dmx);
    slot = (GxSubsystemSlotCtr*)(filter->slot);
    
    dmx_slot_prop.slot_id = slot->id;
    ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_SlotEnable,
            (void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub start slot err !!\n");
#endif  
        return -1;
    }
    dmx_filter_prop.filter_id = filter->id;
    ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterEnable,
            (void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub start filter err !!\n");
#endif  
        return -1;
    }
    return 0;
}

int32_t gx_dmx_sub_driver_stop_filter(GxSubsystemFilterCtr* filter)
{
    GxSubsystemDmxCtr* dmx = NULL;
    GxDemuxProperty_Filter dmx_filter_prop;
    int32_t ret = 0;
    GxSubsystemSlotCtr* slot = NULL;
    GxDemuxProperty_Slot dmx_slot_prop;
    GxDemuxProperty_FilterFifoReset dmx_fifo_reset;

    dmx = (GxSubsystemDmxCtr*)(filter->dmx);
    slot = (GxSubsystemSlotCtr*)(filter->slot);
    
    dmx_slot_prop.slot_id = slot->id;
    ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_SlotDisable,
            (void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub disable slot err !!\n");
#endif  
        return -1;
    }
    dmx_filter_prop.filter_id = filter->id;
    ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterDisable,
            (void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub disable filter err !!\n");
#endif  
        return -1;
    }
    dmx_fifo_reset.filter_id = filter->id; 
    ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterFIFOReset, 
            (void*)&dmx_fifo_reset, sizeof(GxDemuxProperty_FilterFifoReset));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub reset filter fifo err !!\n");
#endif  
        return -1;
    }
    return 0;
}

int32_t gx_dmx_sub_driver_release_filter(GxSubsystemFilterCtr* filter)
{
    GxSubsystemDmxCtr* dmx = NULL;
    GxDemuxProperty_Filter dmx_filter_prop;
    int32_t ret = 0;

    dmx = (GxSubsystemDmxCtr*)(filter->dmx);

    dmx_filter_prop.filter_id = filter->id;
    ret = GxAVSetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterFree,
            (void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub release filter driver err !!\n");
#endif  
        return -1;
    }
    memset(filter,0,sizeof(GxSubsystemFilterCtr));
    return 0;
}

void slot_ctr_init(GxSubsystemSlotCtr * slot)
{
    slot->id = 0;
    slot->dmx = 0;
    slot->pid = 0;
    memset(slot->filter,0,sizeof(GxSubsystemFilterCtr) * DMX_SUB_FILTER_NUM);
    slot->count = 0;
}

int32_t gx_dmx_sub_driver_release_slot(GxSubsystemSlotCtr* slot)
{
    GxSubsystemDmxCtr* dmx = NULL;
    GxDemuxProperty_Slot dmx_slot_prop;
    int32_t ret = 0;

    dmx = (GxSubsystemDmxCtr*)(slot->dmx);

    dmx_slot_prop.slot_id = slot->id;
    ret = GxAVSetProperty(dmx->device, dmx->module,GxDemuxPropertyID_SlotFree,
            (void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub release slot driver err !!\n");
#endif  
        return -1;
    }
    /* 
     *slot控制块中的filter为GxSubSystem_DmxInit时分配的内存，不能直接将filter指针清空
     *所以采取分别初始化
     * */
    slot_ctr_init(slot);
    //memset(slot,0,sizeof(GxSubsystemSlotCtr));
    return 0;
}

uint64_t gx_dmx_sub_driver_wait(GxSubsystemDmxCtr* dmx)
{
    GxDemuxProperty_FilterFifoQuery status = {0};
    int32_t ret = 0;
    uint32_t event_ret = 0;

    if(dmx->count == 0)//dmx没有被打开过
    {
        return 0;
    }
    GxAVWaitEvents(dmx->device, dmx->module,EVENT_DEMUX0_FILTRATE_PSI_END, 1000000, &event_ret);

    ret = GxAVGetProperty(dmx->device, dmx->module,GxDemuxPropertyID_FilterFIFOQuery,
            (void*)&status, sizeof(GxDemuxProperty_FilterFifoQuery));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        //DMX_SUB_ERR_PRINTF("dmx sub read filter status err !!\n");
#endif  
        return 0;
    }
    return status.state;
}

int32_t gx_dmx_sub_driver_read(GxSubsystemFilterCtr* filter,uint8_t* data, uint32_t buf_size)
{
    GxDemuxProperty_FilterRead dmx_filter_read;
    GxSubsystemDmxCtr* dmx = (GxSubsystemDmxCtr*)(filter->dmx);
    int32_t ret = 0;

    dmx_filter_read.filter_id = filter->id;
    dmx_filter_read.buffer = (void*)data;
    dmx_filter_read.max_size = buf_size;
    ret = GxAVGetProperty(dmx->device, dmx->module, GxDemuxPropertyID_FilterRead,
            (void*)&dmx_filter_read, sizeof(GxDemuxProperty_FilterRead));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub read filter driver err !!\n");
#endif  
        return 0;
    }
    return dmx_filter_read.read_size;
}
int32_t gx_dmx_sub_driver_query_status(GxSubsystemDmxCtr* dmx)
{
    GxDemuxProperty_TSLockQuery ts_lock_status = {TS_SYNC_UNLOCKED};
    int32_t ret = 0;


    ret = GxAVGetProperty(dmx->device, dmx->module,GxDemuxPropertyID_TSLockQuery,
            (void*)&ts_lock_status, sizeof(GxDemuxProperty_TSLockQuery));
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub query status err !!\n");
#endif  
        return -1;
    }
    return (ts_lock_status.ts_lock==TS_SYNC_LOCKED?1:0);
}
/* End of file -------------------------------------------------------------*/

