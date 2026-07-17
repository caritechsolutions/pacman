/************************************************************
Copyright (C), 2014, GX S&T Co., Ltd.
FileName   :	gxtkgs_blocking_manager.c
Author     :
Version    : 	1.0
Date       :
Description:
Version    :
History    :
Date                Author      Modification
2014.06.11     liqg         create
***********************************************************/
/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "tkgs_protocol.h"
#include "gxtkgs.h"
#if TKGS_SUPPORT

#define FILE_MODE  "w+"

#define GX_TKGS_BLOCKING_FILE_NAME "/home/gx/tkgs_blocking_db"

static bool TkgsDeliveryBlockingCheck(GxTkgsBlockingDeliveryFilter *filter, GxBusPmDataTP *tp);
static bool TkgsNameBlockingCheck(GxTkgsBlockingNameFilter *filter, char *name);

/*初始化标记*/
//static uint32_t gx_tkgs_blocking_init_flag = 0;

/*互斥量句柄*/
static handle_t gx_tkgs_blocking_mutex_handle = -1;


/**
 * @brief  保存blocking数据
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t GxBus_TkgsBlockingSave(GxTkgsBlocking *pBlocking)
{
	handle_t ret = GXCORE_FILE_UNEXIST;
	handle_t file;

	if (pBlocking == NULL)
	{
		return GXCORE_ERROR;
	}

    if(gx_tkgs_blocking_mutex_handle == -1)
    {
        GxCore_MutexCreate(&gx_tkgs_blocking_mutex_handle);
    }

	GxCore_MutexLock(gx_tkgs_blocking_mutex_handle);
	ret =  GxCore_FileExists  (GX_TKGS_BLOCKING_FILE_NAME);
	if(ret == GXCORE_FILE_EXIST)
	{
		GxCore_FileDelete(GX_TKGS_BLOCKING_FILE_NAME);
	}

    file = GxCore_Open(GX_TKGS_BLOCKING_FILE_NAME, FILE_MODE);
    if(file<0)
    {
    	GxCore_MutexUnlock(gx_tkgs_blocking_mutex_handle);
		return GXCORE_ERROR;
	}

	GxCore_Write(file, pBlocking, 1,sizeof(GxTkgsBlocking));
	GxCore_Close(file);
	GxCore_MutexUnlock(gx_tkgs_blocking_mutex_handle);
	return GXCORE_SUCCESS;

}


/**
 * @brief  获取blocking数据
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t GxBus_TkgsBlockingGet(GxTkgsBlocking *pBlocking)
{
	size_t real_size = 0;
	handle_t ret = GXCORE_FILE_UNEXIST;
	handle_t file;

	if (pBlocking == NULL)
	{
		return GXCORE_ERROR;
	}

    if(gx_tkgs_blocking_mutex_handle == -1)
    {
        GxCore_MutexCreate(&gx_tkgs_blocking_mutex_handle);
    }

	GxCore_MutexLock(gx_tkgs_blocking_mutex_handle);
	ret =  GxCore_FileExists  (GX_TKGS_BLOCKING_FILE_NAME);
	if(ret == GXCORE_FILE_UNEXIST)
	{
		GxCore_MutexUnlock(gx_tkgs_blocking_mutex_handle);
		return GXCORE_ERROR;
	}

    file = GxCore_Open(GX_TKGS_BLOCKING_FILE_NAME, "r"/*FILE_MODE*/);
    if(file<0)
    {
    	GxCore_Close(file);
    	GxCore_MutexUnlock(gx_tkgs_blocking_mutex_handle);
		return GXCORE_ERROR;
	}
	real_size = GxCore_Read(file, pBlocking, 1,sizeof(GxTkgsBlocking));
	if(real_size != sizeof(GxTkgsBlocking))
	{
		GxCore_Close(file);
    	GxCore_MutexUnlock(gx_tkgs_blocking_mutex_handle);
		return GXCORE_ERROR;
	}
	GxCore_Close(file);
	GxCore_MutexUnlock(gx_tkgs_blocking_mutex_handle);
	return GXCORE_SUCCESS;

}

/**
 * @brief blocking过滤
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
status_t GxBus_TkgsBlockingCheck(GxTkgsBlockCheckParm *param)
{
    GxMsgProperty_NodeByPosGet node_prog = {0};
    GxMsgProperty_NodeByIdGet node_tp = {0};
	GxTkgsBlocking blocking = {0};
	int i;

	if (param == NULL)
	{
		return GXCORE_ERROR;
	}

	param->resault = 0;

	if(GXCORE_SUCCESS != GxBus_TkgsBlockingGet(&blocking))
	{
		return GXCORE_ERROR;
	}

	if (blocking.block_num == 0)
	{
		//no blocking
		return GXCORE_SUCCESS;
	}

	node_prog.node_type = NODE_PROG;
	node_prog.id = param->prog_id;
	if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node_prog)))
	{
		return GXCORE_ERROR;
	}

	node_tp.node_type = NODE_TP;
	node_tp.id = node_prog.prog_data.tp_id;
	if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node_tp)))
	{
		return GXCORE_ERROR;
	}

	for (i=0; i<blocking.block_num; i++)
	{
		if ((blocking.filter[i].filter_flag &TKGS_DELIVERY_FILTER) == TKGS_DELIVERY_FILTER)
		{
			//delivery check
			if (TkgsDeliveryBlockingCheck(&blocking.filter[i].delivery_filter, &node_tp.tp_data) == true)
			{
				param->resault = 1;
				return GXCORE_SUCCESS;
			}
		}

		if ((blocking.filter[i].filter_flag & TKGS_SERVICE_ID_FILTER) == TKGS_SERVICE_ID_FILTER)
		{
			//service id check
			if (blocking.filter[i].service_id == node_prog.prog_data.service_id)
			{
				param->resault = 1;
				return GXCORE_SUCCESS;
			}

		}

		if ((blocking.filter[i].filter_flag & TKGS_NETWORK_ID_FILTER) == TKGS_NETWORK_ID_FILTER)
		{
			//network id check
			if (blocking.filter[i].network_id == node_prog.prog_data.original_id)
			{
				param->resault = 1;
				return GXCORE_SUCCESS;
			}

		}

		if ((blocking.filter[i].filter_flag & TKGS_TRANSPORT_STREAM_ID_FILTER) == TKGS_TRANSPORT_STREAM_ID_FILTER)
		{
			//transport stream id check
			//network id check
			if (blocking.filter[i].transport_stream_id == node_prog.prog_data.ts_id)
			{
				param->resault = 1;
				return GXCORE_SUCCESS;
			}

		}

		if ((blocking.filter[i].filter_flag &TKGS_VIDEO_PID_FILTER) == TKGS_VIDEO_PID_FILTER)
		{
			//video pid check
			if (blocking.filter[i].video_pid == node_prog.prog_data.video_pid)
			{
				param->resault = 1;
				return GXCORE_SUCCESS;
			}

		}

		if ((blocking.filter[i].filter_flag & TKGS_AUDIO_PID_FILTER) == TKGS_AUDIO_PID_FILTER)
		{
			//audio pid check
			if (blocking.filter[i].audio_pid == node_prog.prog_data.cur_audio_pid)
			{
				param->resault = 1;
				return GXCORE_SUCCESS;
			}

		}

		if ((blocking.filter[i].filter_flag & TKGS_NAME_FILTER) == TKGS_NAME_FILTER)
		{
			//name filter check
			if (TkgsNameBlockingCheck(&blocking.filter[i].name_filter, (char*)node_prog.prog_data.prog_name) == true)
			{
				param->resault = 1;
				return GXCORE_SUCCESS;
			}
		}
	}
	return GXCORE_SUCCESS;
}

/**
 * @brief Delivery blocking过滤
 * @return   	GXCORE_SUCCESS:执行正常
 				GXCORE_ERROR:执行失败
 */
static bool TkgsDeliveryBlockingCheck(GxTkgsBlockingDeliveryFilter *filter, GxBusPmDataTP *tp)
{
	uint32_t tempData;


	if ((filter == NULL) || (tp == NULL))
	{
		return false;
	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_FREQ_FILTER) == TKGS_DELIVERY_FREQ_FILTER)
	{
		tempData = tp->frequency > filter->delivery.frequency? (tp->frequency -  filter->delivery.frequency):
			(filter->delivery.frequency - tp->frequency);
		if 	(tempData <= filter->frequency_tolerance)
		{
			return true;
		}
	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_SYMB_FILTER) == TKGS_DELIVERY_SYMB_FILTER)
	{
		tempData = tp->tp_s.symbol_rate > filter->delivery.symbol_rate? (tp->tp_s.symbol_rate -  filter->delivery.symbol_rate):
			(filter->delivery.symbol_rate - tp->tp_s.symbol_rate);
		if 	(tempData <= filter->symbolrate_tolerance)
		{
			return true;
		}

	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_POL_FILTER) == TKGS_DELIVERY_POL_FILTER)
	{
		if (filter->delivery.polarization == tp->tp_s.polar)//二者的极化方式定义相同吗
		{
			return true;
		}
	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_ORBITAL_FILTER) == TKGS_DELIVERY_ORBITAL_FILTER)
	{
		GxMsgProperty_NodeByIdGet node_sat = {0};

		node_sat.node_type = NODE_SAT;
		node_sat.id = tp->sat_id;
		if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node_sat)))
		{
			return false;
		}

		if ((node_sat.sat_data.sat_s.longitude == filter->delivery.orbital_position)
			&& (node_sat.sat_data.sat_s.longitude_direct != filter->delivery.west_east_flag))//二者方向定义相反
		{
			return true;
		}
	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_MOD_TYPE_FILTER) == TKGS_DELIVERY_MOD_TYPE_FILTER)
	{
		return false;
	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_MOD_SYS_FILTER) == TKGS_DELIVERY_MOD_SYS_FILTER)
	{
		//0 DVB-S, 1 DVB-S2
		return false;
	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_MOD_FILTER)== TKGS_DELIVERY_MOD_FILTER)
	{
		//00 Auto, 01 QPSK, 10 8PSK, 11 16-QAM (n/a for DVB-S2)
		return false;
	}

	if ((filter->delivery_filter_flag & TKGS_DELIVERY_FEC_FILTER) == TKGS_DELIVERY_FEC_FILTER)
	{
		return false;
	}
	return false;

}

/**
 * @brief Delivery blocking过滤
 * @return   true:比较结果相同
 				false:比较结果不同
 */
static bool TkgsNameBlockingCheck(GxTkgsBlockingNameFilter *filter, char *name)
{
	if ((filter == NULL) || (name == NULL))
	{
		return false;
	}

	if (filter->match_type == GX_TKGS_EXACT_MATCH)
	{
		if ((strlen(filter->name) == strlen(name))
			&& (strncmp(filter->name, name, strlen(name)) == 0))
		{
			return true;
		}
	}
	else if (filter->match_type == GX_TKGS_MATCH_AT_START)
	{
		if (strncmp(filter->name, name, strlen(filter->name)) == 0)
		{
			return true;
		}

	}
	else if (filter->match_type == GX_TKGS_MATCH_OCCUR_ANYWHERE)
	{
		if (strstr(name, filter->name) != NULL)
		{
			return true;
		}
	}

	return false;
}
#endif
