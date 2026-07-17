/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_filter.c
* Author    :	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0	2009.09.01	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxcore.h"
#include "gxavdev.h"
#include "module/si/si_filter.h"
#include "module/si/si_public.h"
#include "av/gxav_demux_propertytypes.h"
#include "av/gxav_module_property.h"
#include "av/gxav_event_type.h"
#include "av/avapi.h"

#define SOFT_DEMUX  (0)



#define SI_BASE_PRINTF(...) gxlogd( __VA_ARGS__ )
#define GX_BUS_SI_DRIVER_ERRO_PRINTF(msg)\
do{ SI_BASE_PRINTF("\n\n*****search error*****\n");\
    SI_BASE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
    SI_BASE_PRINTF("%s\n",msg);\
    SI_BASE_PRINTF("~~~~~search error~~~~~\n");\
}while(0)   
/* Private types/constants ------------------------------------------------ */
/**
 *si filter control block
 */
struct si_filter_ctr
{
	uint32_t      demux_handle;    ///< Demux handle
	int16_t       si_filter_id;         ///< si filter id
	int8_t        dmx_slot_id;     ///< dmx slot id
	int8_t        dmx_filter_id;   ///< dmx filter id
	GxSiFilter    si_filter;       ///< si filter info
};


/* Private Variables ------------------------------------------------------ */
struct demux
{
    uint32_t handle;
    uint32_t count;
};
struct demux * s_Demux;
//static uint32_t s_DemuxHandle[MAX_DEMUX_NUM] = {0};

uint32_t MAX_DEMUX_NUM = 0;
uint32_t MAX_SI_FILTER_NUM = 0;

// per bit record the SiFilter used status, 1 means in use
static uint64_t s_SiFilterStatus=0;
static handle_t s_SiFilterSem= 0;
// SiFilter control block, record the content of every SiFilter
static struct si_filter_ctr * s_SiFilterBlk;

/* Exported Variables ----------------------------------------------------- */
static int32_t dev;

/* Private Macros --------------------------------------------------------- */
// 1 - open    0 - close
#define CHECK_DEMUX_OPEN(demux_id)	(s_Demux[demux_id].handle == 0)

#define CHECK_API_RET(api_ret, err_ret)		do{\
							if (api_ret < 0){\
								gxlogd("api ret err %s %d\n",__FUNCTION__, __LINE__);\
								return err_ret;}\
								}while(0)

#define CHECK_POINT(point, err_ret)		do{\
							if (point == NULL){\
								gxlogd("null point in %s %d\n",__FUNCTION__, __LINE__);\
								return err_ret;}\
								}while(0)

// from si_parser
extern void GxBus_SiParserMemFree(int16_t si_subtable_id);

/* Private Functions ------------------------------------------------------ */
/**
 * @brief check the dmx slot in use?
 * @param pid : dmx_slot_id
 * @Return  exist dmx_slot_id
		-1   the dmx_slot_id not in use
 */
static int16_t  check_dmx_slot_id(uint16_t pid,uint16_t DemuxId)
{
	uint16_t i;

	for (i=0; i<MAX_SI_FILTER_NUM; i++)
	{
		if ((((s_SiFilterStatus>>i)&1ULL) == 1ULL) 
			&& (pid == s_SiFilterBlk[i].si_filter.pid)
			&& (s_Demux[DemuxId].handle == s_SiFilterBlk[i].demux_handle))
		{
			return s_SiFilterBlk[i].dmx_slot_id;
		}
	}

	return -1;
}

/**
 * @brief check the dmx slot can free?
 * @param pid : dmx_slot_id
 * @Return  0 - no other use this slot, can free
		-1  the slot can't free
 */
static int check_slot_can_GxCore_Free(int8_t dmx_slot_id)
{
    uint32_t i=0;

	for (i=0; i<MAX_SI_FILTER_NUM; i++)
	{
		if (s_SiFilterBlk[i].dmx_slot_id == dmx_slot_id)
		{
			i = 0xffff;
			break;
		}
	}

	if (i != 0xffff)
        return 0;

    return -1;
}

/**
 * @brief get free si_filter_record
 * @param void
 * @Return si_filter_id
		-1   error
 */
static void si_filter_record(uint32_t demux_handle, GxDemuxProperty_Filter *dmx_filter_prop, GxSiFilter *si_filter)
{
	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[dmx_filter_prop->filter_id];
	// save info to filter_ctr

	p_filter_ctr->demux_handle = demux_handle;
	p_filter_ctr->si_filter_id = dmx_filter_prop->filter_id;
	p_filter_ctr->dmx_slot_id = dmx_filter_prop->slot_id;
	p_filter_ctr->dmx_filter_id = dmx_filter_prop->filter_id;
	memcpy(&(p_filter_ctr->si_filter), si_filter, sizeof(GxSiFilter));

	// mark already in use
	s_SiFilterStatus |= (1ULL<<p_filter_ctr->si_filter_id);
    /*当第一次建立filter时需要发送一次信号量*/
    if((s_SiFilterStatus &(~(1ULL<<p_filter_ctr->si_filter_id))) == 0)
    {
        GxBus_SiFilterSemPost();
    }
}

/**
 * @brief free si_filter_id
 * @param si_filter_id
 * @Return void
 */
static void si_filter_clear(int16_t si_filter_id)
{
	memset(&s_SiFilterBlk[si_filter_id], 0xff, sizeof(struct si_filter_ctr));

	s_SiFilterStatus &= (~(1ULL<<si_filter_id));
}



#if( SOFT_DEMUX==0)

status_t ts_demux_disconnect(uint16_t demux_id)
{
#if 0
	GxStatus api_ret;
#endif
	if (demux_id > (MAX_DEMUX_NUM-1)
			|| s_Demux[demux_id].handle == 0)
	{
		return GXCORE_ERROR;
	}

	//    GxAvdev_FreeDemux(demux_id);

	s_Demux[demux_id].count--;
#if 0
	if (s_Demux[demux_id].count == 0)
	{
		s_Demux[demux_id].handle = 0;

		api_ret = GxAvdev_CloseModule(dev, s_Demux[demux_id].handle);
		CHECK_API_RET(api_ret, GXCORE_ERROR);
	}
#endif
	return GXCORE_SUCCESS;
}

status_t ts_demux_connect(uint16_t ts_src, uint16_t demux_id)
{
    GxDemuxProperty_ConfigDemux config_demux;
	int32_t api_ret;
	static int init_sifilterblk = 0;

//    *demux_id = GxAvdev_AllocDemux(ts_src);

    if (demux_id > (MAX_DEMUX_NUM-1))
	{
		return GXCORE_ERROR;
	}

    // demux already open,means work ok
	if (!CHECK_DEMUX_OPEN(demux_id))
    {
        s_Demux[demux_id].count++;
		return GXCORE_SUCCESS;
	}

	if (init_sifilterblk == 0)
	{
		// before use s_SiFilterBlk, clear first.
		memset(s_SiFilterBlk, 0xff, sizeof(struct si_filter_ctr)*MAX_SI_FILTER_NUM);
		init_sifilterblk = 1;
	}

	s_Demux[demux_id].handle = (uint32_t)(GxAvdev_OpenModule(dev, GXAV_MOD_DEMUX, demux_id));

	if (s_Demux[demux_id].handle <= 0)
	{
		return GXCORE_ERROR;
	}
	else
	{
		config_demux.source = ts_src;
		config_demux.ts_select = FRONTEND;
		config_demux.stream_mode = DEMUX_PARALLEL;
		config_demux.time_gate = 0xf;
		config_demux.byt_cnt_err_gate = 0x03;
		config_demux.sync_loss_gate = 0x03;
		config_demux.sync_lock_gate = 0x03;
		api_ret = GxAVSetProperty(dev,s_Demux[demux_id].handle, GxDemuxPropertyID_Config, &config_demux, \
					sizeof(GxDemuxProperty_ConfigDemux));

		if (api_ret < 0){
			ts_demux_disconnect(demux_id);
			return GXCORE_ERROR;
		}
	}

    s_Demux[demux_id].count++;
	return GXCORE_SUCCESS;
}

/**
 * @brief 根据demux_id打开demux, must open all demux_id first
 * @param demux_id: demux id
 * @Return demux打开成功: GXCORE_SUCCESS
 *		  demux打开失败: GXCORE_ERROR
 */
status_t GxBus_SiFilterOpenAv(void)
{
	dev = GxAvdev_CreateDevice(0);

	if (dev < 0)
	{
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

/**
 * @brief 根据demux_id关闭demux
 * @param demux_id: demux id
 * @Return GXCORE_SUCCESS：demux关闭成功
 *		  GXCORE_ERROR：demux关闭失败
 */
status_t GxBus_SiFilterCloseAv(void)
{
	GxAvdev_DestroyDevice(dev);

	dev = -1;

	return GXCORE_SUCCESS;
}


int32_t get_hard_num(void)
{
    MAX_DEMUX_NUM = 2;
    MAX_SI_FILTER_NUM = 64;
    return 0;
}

int32_t GxBus_SiDmxInit(void)
{
    int32_t ret = 0;
    ret = get_hard_num();
    if(ret < 0)
    {
#ifdef SI_BASE_DEBUG 
       GX_BUS_SI_DRIVER_ERRO_PRINTF("get hard num err\n"); 
#endif
       return -1;
    }
    if(s_Demux != NULL)
    {
        GxCore_Free(s_Demux);
        s_Demux = NULL;
    }
    s_Demux = GxCore_Mallocz(sizeof(struct demux) * MAX_DEMUX_NUM);
    if(s_Demux == NULL)
    {
#ifdef SI_BASE_DEBUG 
       GX_BUS_SI_DRIVER_ERRO_PRINTF("s_Demux malloc failed\n"); 
#endif
       return -2;
    }

    if(s_SiFilterBlk != NULL)
    {
        GxCore_Free(s_SiFilterBlk);
        s_SiFilterBlk = NULL;
    }
    s_SiFilterBlk = GxCore_Mallocz(sizeof(struct si_filter_ctr) * MAX_SI_FILTER_NUM);
    if(s_SiFilterBlk == NULL)
    {
#ifdef SI_BASE_DEBUG 
       GX_BUS_SI_DRIVER_ERRO_PRINTF("s_Demux malloc failed\n"); 
#endif
       return -3;
    }
    return 0;
}

/* Exported Functions ----------------------------------------------------- */
/**
 * @brief 根据channel控制块信息，创建一路si filter，并得到si_filter_id
 * @param demux_id:  创建的SiFilter挂载在哪个demux下
 * @param si_filter[IN]:  si_filter的配置信息
 * @Return SiFilter的ID值
 * @				-1  create failed
 */
int16_t GxBus_SiFilterCreate (uint16_t ts_src, uint16_t demux_id, GxSiFilter *si_filter)
{
	GxDemuxProperty_Slot dmx_slot_prop;
	GxDemuxProperty_Filter dmx_filter_prop;
	int32_t api_ret;
	uint32_t i;
	int16_t dmx_slot_id;
	int8_t need_free_slot = 0;

	CHECK_POINT(si_filter, (-1));

    if (GXCORE_SUCCESS != ts_demux_connect(ts_src, demux_id))
    {
        return -1;
    }

	memset(&dmx_slot_prop, 0, sizeof(GxDemuxProperty_Slot));
	memset(&dmx_filter_prop, 0, sizeof(GxDemuxProperty_Filter));

	dmx_slot_id = check_dmx_slot_id(si_filter->pid,demux_id);
	if (dmx_slot_id == -1)
	{
		// alloc and config dmx_slot-----------------------------------------------------
		dmx_slot_prop.type = DEMUX_SLOT_PSI;
		dmx_slot_prop.pid = si_filter->pid;
		api_ret = GxAVGetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotAlloc,
						(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));

		if (api_ret < 0){
			return -1;
		}

		//gxlogd("alloc slot hw !dmx_slot_id = %d\n",dmx_slot_prop.slot_id);

		//dmx_slot_prop.slot_id already get from SlotAlloc
		dmx_slot_prop.type = DEMUX_SLOT_PSI;
		dmx_slot_prop.flags = (DMX_REPEAT_MODE | DMX_AVOUT_EN);

		api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotConfig,
						(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));

		if (api_ret < 0)
		{
			api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotFree,
								(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
			CHECK_API_RET(api_ret, GXCORE_ERROR);
			return -1;
		}

		dmx_filter_prop.slot_id = dmx_slot_prop.slot_id;
		need_free_slot = 1;
	}
	else
	{
		// get dmx_slot by dmx_slot_id
		dmx_filter_prop.slot_id = dmx_slot_id;
	}
	
	if (si_filter->sw_buffer_size > 0)
		dmx_filter_prop.sw_buffer_size = si_filter->sw_buffer_size;

	if (si_filter->hw_buffer_size > 0)
		dmx_filter_prop.hw_buffer_size = si_filter->hw_buffer_size;

	// alloc and config dmx_filter-----------------------------------------------------
	api_ret = GxAVGetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterAlloc,
						(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

	if (api_ret < 0)
	{
        if ( need_free_slot == 1)
        {
            api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotFree,
                    (void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
            CHECK_API_RET(api_ret, GXCORE_ERROR);
        }
		return -1;
	}

	//dmx_filter_prop.filter_id already get from FilterAlloc
	dmx_filter_prop.depth = si_filter->match_depth;

	for (i=0; i<si_filter->match_depth; i++)
	{
		dmx_filter_prop.key[i].value = si_filter->match[i];
		dmx_filter_prop.key[i].mask = si_filter->mask[i];
	}

	(si_filter->eq_or_neq == 1) ? (dmx_filter_prop.flags = DMX_EQ):(dmx_filter_prop.flags = 0);

	if (si_filter->crc != CRC_OFF)
	{
		dmx_filter_prop.flags |= DMX_CRC_IRQ;
	}
	if (si_filter->soft_filter == SOFT_ON)
	{
		dmx_filter_prop.flags |= DMX_SW_FILTER;
	}

	dmx_filter_prop.flags |= DMX_AUTO_RESET_EN;

	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterConfig,
						(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

	if (api_ret < 0){
		api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterFree,
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

		CHECK_API_RET(api_ret, GXCORE_ERROR);

        if (need_free_slot == 1)
        {
            api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotFree,
                    (void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
            CHECK_API_RET(api_ret, GXCORE_ERROR);
        }
		return -1;
	}

	// save info to filter_ctr
	si_filter_record(s_Demux[demux_id].handle, &dmx_filter_prop, si_filter);

	return dmx_filter_prop.filter_id;
}

/**
 * @brief 根据si_filter_id销毁一路si_filter
 * @param si_filter_id: 创建时得到的si_filter id
 * @Return GXCORE_SUCCESS    GXCORE_ERROR
 */
status_t GxBus_SiFilterDestroy (uint16_t demux_id, int16_t si_filter_id)
{
	GxDemuxProperty_Slot dmx_slot_prop;
	GxDemuxProperty_Filter dmx_filter_prop;
	int32_t api_ret;

	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[si_filter_id];
	int8_t dmx_slot_id;


	// record the dmx_slot_id, check the dmx_slot need free?
	dmx_slot_id = p_filter_ctr->dmx_slot_id;
	memset(&dmx_slot_prop,0,sizeof(GxDemuxProperty_Slot));

	// free dmx_filter
	dmx_filter_prop.filter_id = p_filter_ctr->dmx_filter_id;
	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterFree,
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));

	CHECK_API_RET(api_ret, GXCORE_ERROR);

	//si_filter_GxCore_Free(si_filter_id);
	si_filter_clear(si_filter_id);
	// clr status
	//s_SiFilterStatus &= (~(1<<si_filter_id));


	// check the dmx_slot need free?
	if (check_slot_can_GxCore_Free(dmx_slot_id) == 0)
	{
		dmx_slot_prop.slot_id = dmx_slot_id;
		dmx_slot_prop.type = DEMUX_SLOT_PSI;
		api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotFree,
								(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
		CHECK_API_RET(api_ret, GXCORE_ERROR);
	}

	// release all mem allocd for the si_filter_id
	GxBus_SiParserMemFree(si_filter_id);

    ts_demux_disconnect(demux_id);

	return GXCORE_SUCCESS;
}

/**
 * @brief 根据si_filter _id启动一路si filter
 * @param si_filter_id: 创建时得到的si filter id
 * @Return GXCORE_SUCCESS    GXCORE_ERROR
 */
status_t GxBus_SiFilterStart (uint16_t demux_id, int16_t si_filter_id)
{
	GxDemuxProperty_Slot dmx_slot_prop;
	GxDemuxProperty_Filter dmx_filter_prop;
	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[si_filter_id];
	int32_t api_ret = 0;

	memset(&dmx_slot_prop,0,sizeof(GxDemuxProperty_Slot));
	dmx_slot_prop.slot_id = p_filter_ctr->dmx_slot_id;
	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotEnable,
								(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	dmx_filter_prop.filter_id = p_filter_ctr->dmx_filter_id;
	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterEnable, 
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	return GXCORE_SUCCESS;
}

/**
 * @brief 根据si_filter_id停止一路si filter，带复位dmx_filter功能
 * @param si_filter_id: 创建时得到的si filter id
 * @Return GXCORE_SUCCESS    GXCORE_ERROR
 */
status_t GxBus_SiFilterStop (uint16_t demux_id, int16_t si_filter_id)
{
	GxDemuxProperty_Filter dmx_filter_prop;
	GxDemuxProperty_FilterFifoReset dmx_fifo_reset;
	int32_t  api_ret = 0;

	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[si_filter_id];

	dmx_filter_prop.filter_id = p_filter_ctr->dmx_filter_id;
	dmx_fifo_reset.filter_id = p_filter_ctr->dmx_filter_id;

	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterDisable,
								(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterFIFOReset,
								(void*)&dmx_fifo_reset, sizeof(GxDemuxProperty_FilterFifoReset));

	CHECK_API_RET(api_ret, GXCORE_ERROR);

	return GXCORE_SUCCESS;
}

/**
 * @brief 根据si_filter_id,修改si filter,不支持修改pid,如需修改申请新的SiFilter
 * @param si_filter_id: 创建时得到的si filter id
 * @param si_filter[IN]:  si_filter的配置信息
 * @Return GXCORE_SUCCESS    GXCORE_ERROR
 */
status_t GxBus_SiFilterModify (uint16_t demux_id, int16_t si_filter_id, GxSiFilter *si_filter)
{
	GxDemuxProperty_Slot dmx_slot_prop;
	GxDemuxProperty_Filter dmx_filter_prop;
	GxDemuxProperty_FilterFifoReset dmx_fifo_reset;
	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[si_filter_id];
	int32_t  api_ret;
	uint16_t i;

	CHECK_POINT(si_filter, GXCORE_ERROR);

    if (si_filter->eq_or_neq > EQ_MATCH)
    {
        return GXCORE_ERROR;
    }

	memset(&dmx_slot_prop,0,sizeof(GxDemuxProperty_Slot));
	if (p_filter_ctr->si_filter.pid != si_filter->pid)
	{
		// slot disable
		dmx_slot_prop.slot_id = p_filter_ctr->dmx_slot_id;
		api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotDisable,
										(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
		CHECK_API_RET(api_ret, GXCORE_ERROR);
	}

	// filter disable
	dmx_filter_prop.filter_id = p_filter_ctr->dmx_filter_id;
	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterDisable,
									(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	// filter fifo reset
	dmx_fifo_reset.filter_id = p_filter_ctr->dmx_filter_id;
	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterFIFOReset,
								(void*)&dmx_fifo_reset, sizeof(GxDemuxProperty_FilterFifoReset));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	// release all mem allocd for the si_filter_id
//	GxBus_SiParserMemFree(si_filter_id);

	if (p_filter_ctr->si_filter.pid != si_filter->pid)
	{

		// slot modify
		dmx_slot_prop.type = DEMUX_SLOT_PSI;
		dmx_slot_prop.pid = si_filter->pid;
		dmx_slot_prop.type = DEMUX_SLOT_PSI;
		dmx_slot_prop.flags = (DMX_REPEAT_MODE | DMX_AVOUT_EN);

		api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotConfig,
						(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
		CHECK_API_RET(api_ret, GXCORE_ERROR);
	}

	dmx_filter_prop.slot_id = p_filter_ctr->dmx_slot_id;
	dmx_filter_prop.filter_id = p_filter_ctr->dmx_filter_id;
	dmx_filter_prop.depth = si_filter->match_depth;

	for (i=0; i<si_filter->match_depth; i++)
	{
		dmx_filter_prop.key[i].value = si_filter->match[i];
		dmx_filter_prop.key[i].mask = si_filter->mask[i];
	}

	(si_filter->eq_or_neq == 1) ? (dmx_filter_prop.flags = DMX_EQ):(dmx_filter_prop.flags = 0);

	if (si_filter->crc != CRC_OFF)
	{
		dmx_filter_prop.flags |= DMX_CRC_IRQ;
	}
	if (si_filter->soft_filter == SOFT_ON)
	{
		dmx_filter_prop.flags |= DMX_SW_FILTER;
	}

	dmx_filter_prop.flags |= DMX_AUTO_RESET_EN;

	// filter modify
	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterConfig,
									(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	if (p_filter_ctr->si_filter.pid != si_filter->pid)
	{
		// slot enable
		api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_SlotEnable,
									(void*)&dmx_slot_prop, sizeof(GxDemuxProperty_Slot));
		CHECK_API_RET(api_ret, GXCORE_ERROR);

	}

	// filter start
	api_ret = GxAVSetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterEnable,
									(void*)&dmx_filter_prop, sizeof(GxDemuxProperty_Filter));
	CHECK_API_RET(api_ret, GXCORE_ERROR);

	// save info to filter_ctr
	memcpy(&(p_filter_ctr->si_filter), si_filter, sizeof(GxSiFilter));

	return GXCORE_SUCCESS;
}

/**
 * @brief 查询哪一路si filter有过滤到数据
 * @param si_filter_status[OUT]: 一共32位，任意一位置位，就代表该位所代表的si_filter有过滤到数据。
 * @Return void
 */
status_t GxBus_SiFilterQuery (uint16_t demux_id, uint64_t *si_filter_status)
{
	GxDemuxProperty_FilterFifoQuery status = {0};
	int32_t api_ret = 0;
	uint32_t event_ret, event;

	CHECK_POINT(si_filter_status, GXCORE_ERROR);

    if (s_Demux[demux_id].handle != 0)
	{
		if ((demux_id % 2) == 0)
			event = EVENT_DEMUX0_FILTRATE_PSI_END;
		else
			event = EVENT_DEMUX1_FILTRATE_PSI_END;

		GxAVWaitEvents(dev, s_Demux[demux_id].handle, event, 500000, &event_ret);

        if (s_SiFilterStatus == 0)
        {
            return GXCORE_ERROR;
        }

        api_ret = GxAVGetProperty(dev, s_Demux[demux_id].handle,
                GxDemuxPropertyID_FilterFIFOQuery,
                (void*)&status, sizeof(GxDemuxProperty_FilterFifoQuery));
        CHECK_API_RET(api_ret, GXCORE_ERROR);

        *si_filter_status = status.state;

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

/**
 * @brief 根据si_filter_id获得某路si_filter的数据
 * @param si_filter_id: 创建时得到的si filter id
 * @Return size_t: 实际读到的大小
 */
size_t GxBus_SiFilterRead(uint16_t demux_id, int16_t si_filter_id, uint8_t  *data_buf, size_t  data_len)
{
	GxDemuxProperty_FilterRead dmx_filter_read;
	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[si_filter_id];
	int32_t api_ret;

	CHECK_POINT(data_buf, 0);

	dmx_filter_read.filter_id = p_filter_ctr->dmx_filter_id;
	dmx_filter_read.buffer = (void*)data_buf;
	dmx_filter_read.max_size = data_len;

	api_ret = GxAVGetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_FilterRead,
								(void*)&dmx_filter_read, sizeof(GxDemuxProperty_FilterRead));
	CHECK_API_RET(api_ret, 0);

	return dmx_filter_read.read_size;
}

const GxSiFilter* GxBus_SiFilterGet(int16_t si_filter_id)
{
	return &(s_SiFilterBlk[si_filter_id].si_filter);
}

GxSiQueryStatus GxBus_SiTsLockQuery (uint32_t demux_id)
{
	int32_t api_ret;

	GxDemuxProperty_TSLockQuery ts_lock_status;

	api_ret = GxAVGetProperty(dev, s_Demux[demux_id].handle, GxDemuxPropertyID_TSLockQuery,
							(void*)&ts_lock_status, sizeof(GxDemuxProperty_TSLockQuery));
	CHECK_API_RET(api_ret, SI_TS_UNLOCK);

	if (ts_lock_status.ts_lock == TS_SYNC_UNLOCKED)
	{
		return SI_TS_UNLOCK;
	}

	return SI_TS_LOCK;
}
void GxBus_SiFilterSemInit(void)
{
    if(GXCORE_SUCCESS != GxCore_SemCreate(&s_SiFilterSem,0))
    {
        gxlogd("si create sem err !!\n");
    }
    return;
}

void GxBus_SiFilterSemPost(void)
{
    GxCore_SemPost(s_SiFilterSem);
    return;
}
void GxBus_SiFilterSemWait(void)
{
    while(s_SiFilterStatus == 0)
    {
        GxCore_SemWait(s_SiFilterSem);
    }
    return;
}
void GxBus_SiFilterSemDel(void)
{
    GxCore_SemDelete(s_SiFilterSem);
    s_SiFilterSem = 0;
    return;
}
#else
#include "soft_demux.h"
uint32_t s_FilterStart = 0;  // 0 stop   1 start
/**
 * @brief 根据channel控制块信息，创建一路si filter，并得到si_filter_id
 * @param demux_id:  创建的SiFilter挂载在哪个demux下
 * @param si_filter[IN]:  si_filter的配置信息
 * @Return SiFilter的ID值
 */
int16_t GxBus_SiFilterCreate (uint32_t demux_id, GxSiFilter *si_filter)
{
	uint32_t demux_handle = 0x12345678;
	struct si_filter_ctr *p_filter_ctr;
	int16_t si_filter_id;
	int16_t dmx_slot_id;

	if (s_DemuxHandle[demux_id] == 0)
	{
		return -1;
	}

	if (CHECK_DEMUX_OPEN(demux_id) == 0)
	{
		gxlogd("open demux!\n");
	}

	si_filter_id = si_filter_alloc();

	if (si_filter_id == -1)
	{
		return -1;
	}

	p_filter_ctr = &s_SiFilterBlk[si_filter_id];


	// TODO: judge weather the pid in use ???
	dmx_slot_id = check_dmx_slot_id(si_filter->pid,demux_id);
	if (dmx_slot_id == -1)
	{
		// alloc and config dmx_slot-----------------------------------------------------
		SetPidValue(si_filter->pid);
	}
	// alloc and config dmx_filter-----------------------------------------------------
	ConfigFilter(si_filter->match, si_filter->mask, si_filter->match_depth);

	// save info to filter_ctr
	p_filter_ctr->demux_handle = demux_handle;
	p_filter_ctr->si_filter_id = 0;
	p_filter_ctr->dmx_slot_id = 0;
	p_filter_ctr->dmx_filter_id = 0;
	memcpy(&(p_filter_ctr->si_filter), si_filter, sizeof(GxSiFilter));

	return p_filter_ctr->si_filter_id;
}


/**
 * @brief 根据si_filter_id销毁一路si_filter
 * @param si_filter_id: 创建时得到的si_filter id
 * @Return void
 */
status_t GxBus_SiFilterDestroy (int16_t si_filter_id)
{
	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[si_filter_id];
	uint16_t i;
	uint8_t dmx_slot_id;

	// record the dmx_slot_id, check the dmx_slot need free?
	dmx_slot_id = p_filter_ctr->dmx_slot_id;

	// free dmx_filter
	gxlogd("free dmx filter\n");

	si_filter_GxCore_Free(si_filter_id);

	// check the dmx_slot need free?
	for (i=0; i<MAX_SI_FILTER_NUM; i++)
	{
		if (s_SiFilterBlk[i].dmx_slot_id == dmx_slot_id)
		{
			i = 0xffff;
			break;
		}
	}

	if (i != 0xffff)
	{
		gxlogd("free dmx slot too\n");
	}
	return GXCORE_SUCCESS;

}

/**
 * @brief 根据si_filter _id启动一路si filter
 * @param si_filter_id: 创建时得到的si filter id
 * @Return void
 */
status_t GxBus_SiFilterStart (int16_t si_filter_id)
{
	s_FilterStart = 1;
	return GXCORE_SUCCESS;
}

/**
 * @brief 根据si_filter_id停止一路si filter，带复位dmx_filter功能
 * @param si_filter_id: 创建时得到的si filter id
 * @Return void
 */
status_t GxBus_SiFilterStop (int16_t si_filter_id)
{
	ClrAnalysePos();
	s_FilterStart = 0;
	return GXCORE_SUCCESS;
}

/**
 * @brief 根据si_filter_id,修改si filter
 * @param si_filter_id: 创建时得到的si filter id
 * @param si_filter[IN]: si filter 的配置信息
 * @Return void
 */
status_t GxBus_SiFilterModify (int16_t si_filter_id, GxSiFilter *si_filter)
{
	struct si_filter_ctr *p_filter_ctr = &s_SiFilterBlk[si_filter_id];

	ClrAnalysePos();
	s_FilterStart = 0;

	SetPidValue(si_filter->pid);
	ConfigFilter(si_filter->match, si_filter->mask, si_filter->match_depth);

	// save info to filter_ctr
	memcpy(&(p_filter_ctr->si_filter), si_filter, sizeof(GxSiFilter));

	return GXCORE_SUCCESS;
	//s_FilterStart = 1;
}

/**
 * @brief 查询哪一路si filter有过滤到数据
 * @param si_filter_status[OUT]: 一共32位，任意一位置位，就代表该位所代表的si_filter有过滤到数据。
 * @Return void
 */
status_t GxBus_SiFilterQuery (uint64_t *si_filter_status)
{
	while(!s_FilterStart){;}

	*si_filter_status = 1;

	return GXCORE_SUCCESS;
}

/**
 * @brief 根据si_filter_id获得某路si_filter的数据
 * @param si_filter_id: 创建时得到的si filter id
 * @Return size_t: 实际读到的大小
 */
size_t GxBus_SiFilterRead(int16_t si_filter_id,	uint8_t  *data_buf, size_t  data_len)
{
	uint8_t *pData;

	pData = GetSectionData();
	memcpy(data_buf, pData, data_len);

	return (((data_buf[1] & 0x0f)<< 8 ) |data_buf[2]);
}

const GxSiFilter* GxBus_SiFilterGet(int16_t si_filter_id)
{
	return &(s_SiFilterBlk[si_filter_id].si_filter);
}

GxSiQueryStatus GxBus_SiTsLockQuery (uint32_t demux_id)
{
	return SI_TS_LOCK;
}

#endif
/* End of file -------------------------------------------------------------*/

