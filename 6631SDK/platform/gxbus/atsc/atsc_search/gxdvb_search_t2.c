
/************************************************************ 
Copyright (C), 2007-2009, GX S&T Co., Ltd. 
FileName   :	search_Service.c
Author     : 	xiahs
Version    : 	1.0
Date	   :
Description:	
Version    :	
History    :	
Date				Author  	Modification 
2007.03.14     xiahs		 create
***********************************************************/

/* Includes --------------------------------------------------------------- */

#include "module/frontend/gxfrontend_module.h"
#include "include/gx_search_private.h"
#include "gxcore.h"
#include <sys/ioctl.h>


/* Functions --------------------------------------------------------------- */

extern GxMessage* GxBus_MessageTryGet(handle_t self);//为了接收stop消息
extern status_t GxBus_MessageTryFree(GxMessage *message);


static status_t gx_search_set_dvbt2_id(GxSearchClassObj * search_class,uint8_t data_plp_id,uint8_t common_plp_exist,uint8_t common_plp_id)
{
	GxFrontend para = {0};
	uint32_t status = FE_TIMEDOUT;
	status_t ret = 0;
	
	para.demux = search_class->search_demux_handle;
	para.dev = search_class->search_device_handle;
	para.fre = 0xffffffff;
	para.tuner = search_class->search_tuner_num_for_prog;
	para.type = FRONTEND_DVB_T2;
	para.data_plp_id = data_plp_id;
	para.common_plp_exist = common_plp_exist;
	para.common_plp_id = common_plp_id;
	para.type_1501 = 0;
	
	ret = GxFrontend_SetTp(&para);
	
	if(ret == -1)//(((ret = GxFrontend_SetTp(&para)) != 0) && (ret != -2))
	{
		return GX_SEARCH_ERR;
	}
	else if(ret == -2)
	{
		status = FE_HAS_LOCK;
	}
	else
	{
		GxFrontend_ReadStatus(para.tuner,&status);
	}

	if(status == FE_HAS_LOCK)
	{
        if (GxFrontend_QueryStatus(search_class->search_tuner_num_for_prog) != 1) {
			return GX_SEARCH_ERR;
		}
		return GX_SEARCH_OK;
	}

	return GX_SEARCH_ERR;
}

static status_t gx_search_dvbt2_get_id(void)
{
    struct fe_set_window_parameters params;
    struct dvb_frontend_parameters* get_params = NULL;
	uint32_t i = 0, mode = 3;
	handle_t frontend = 0;
    GxSearchClassObj * search_class = &SearchServiceObj.searchclass;

	if(search_class->dvbt2_info.dvbt2_id != NULL)
	{
		GxCore_Free(search_class->dvbt2_info.dvbt2_id);
		search_class->dvbt2_info.dvbt2_id = NULL;
	}
	frontend = GxFrontend_IdToHandle(search_class->search_tuner_num_for_prog);	
	if(ioctl(frontend, FE_READ_BER, &mode) < 0)
	{
		return GX_SEARCH_ERR;
	}
	gxlogd("FE_READ_BER mode=%d\n", mode);
	if (mode == 1)
	{
		search_class->dvbt2_info.dvbt2_id_num = 0;
		return GX_SEARCH_OK;
	}
	
	if (mode == 3)
	{
		search_class->dvbt2_info.dvbt2_id_num = 0;
		return GX_SEARCH_ERR;
	}
	
	if(ioctl(frontend, FE_SET_BLINDSCAN, &params) < 0)
	{
		return GX_SEARCH_ERR;
	}
	search_class->dvbt2_info.dvbt2_id_num = params.tp_num;
	if (params.tp_num == 0)
	{
		return GX_SEARCH_ERR;
	}
	
	get_params = GxCore_Malloc(params.tp_num*sizeof(struct dvb_frontend_parameters));
	if(get_params == NULL) {
		return GX_SEARCH_ERR;
	}
	search_class->dvbt2_info.dvbt2_id = GxCore_Malloc(params.tp_num*sizeof(GxSearchDvbt2Id));
	if(search_class->dvbt2_info.dvbt2_id == NULL)
	{
		GxCore_Free(get_params);
		return GX_SEARCH_ERR;
	}
	if(ioctl(frontend, FE_GET_BLINDSCAN, get_params) < 0)
	{
		GxCore_Free(search_class->dvbt2_info.dvbt2_id);
		GxCore_Free(get_params);
		return GX_SEARCH_ERR;
	}
	for(i=0; i<params.tp_num; i++)
	{
		search_class->dvbt2_info.dvbt2_id[i].data_plp_id = get_params[i].frequency & 0xff;
		search_class->dvbt2_info.dvbt2_id[i].common_plp_exist = (get_params[i].frequency >> 8) & 0xff;
		search_class->dvbt2_info.dvbt2_id[i].common_plp_id = (get_params[i].frequency >> 16) & 0xff;
	}
	GxCore_Free(get_params);
	return GX_SEARCH_OK;
}
static status_t gx_search_dvbt2_set_tp(uint32_t fre, fe_bandwidth_t bandwidth)
{
	GxFrontend para = {0};
	uint32_t status = FE_TIMEDOUT;
	status_t ret = 0;
    GxSearchClassObj * search_class = &SearchServiceObj.searchclass;

	para.demux = search_class->search_demux_handle;
	para.dev = search_class->search_device_handle;
	para.fre = fre;
	para.bandwidth = bandwidth;
	para.tuner = search_class->search_tuner_num_for_prog;
	para.type = FRONTEND_DVB_T2;
	para.data_plp_id = 0xff;
	para.common_plp_exist = 0xff;
	para.common_plp_id = 0xff;
	para.type_1501 = 0;
	
	ret = GxFrontend_SetTp(&para);
	if(ret == -1)//(((ret = GxFrontend_SetTp(&para)) != 0) && (ret != -2))
	{	
		return GX_SEARCH_ERR;
	}
	else if(ret == -2)
	{
		status = FE_HAS_LOCK;
	}
	else
	{
		GxFrontend_ReadStatus(para.tuner,&status);
	}
		
	if(status == FE_HAS_LOCK)
	{
		if(gx_search_dvbt2_get_id() == GX_SEARCH_OK)
		{
			return GX_SEARCH_OK;
		}
	}

	return GX_SEARCH_ERR;
}



status_t gx_dvb_search_dvbt2_tp_lock(void * p)
{
	uint32_t                tp_id = 0;
	GxBusPmDataTP           tp = { 0 };
	status_t ret = 0;
    uint32_t i = 0 ;
    GxSearchClassObj * search_class = (GxSearchClassObj *)p;

 start_lock:
	if(search_class->dvbt2_info.dvbt2_id_finish < search_class->dvbt2_info.dvbt2_id_num && search_class->dvbt2_info.dvbt2_id_num != 0)
	{
		if(gx_search_set_dvbt2_id(search_class,search_class->dvbt2_info.dvbt2_id[search_class->dvbt2_info.dvbt2_id_finish].data_plp_id,
		search_class->dvbt2_info.dvbt2_id[search_class->dvbt2_info.dvbt2_id_finish].common_plp_exist,
		search_class->dvbt2_info.dvbt2_id[search_class->dvbt2_info.dvbt2_id_finish].common_plp_id) == GX_SEARCH_OK)
		{
			search_class->dvbt2_info.dvbt2_id_finish++;
			return GX_SEARCH_OK;
		}
		else
		{
			search_class->dvbt2_info.dvbt2_id_finish++;
            GxCore_ThreadYield();
            goto start_lock;
		}
	}

	if (search_class->search_tp_num != 0 && search_class->search_tp_finish_num < search_class->search_tp_num) 
	{
     //当锁频时用户发送stop消息，需要用状态基来判断退出
        if(SearchServiceObj.searchclass.search_status != SEARCH_START)
        {
            return GX_SEARCH_FINISH;
        }
		tp_id = search_class->search_tp_id[search_class->search_tp_finish_num];
        search_class->ext.need_search_ext = 0;
        if(search_class->ext.ext_tp_id == NULL)
        {
            search_class->ext.need_search_ext = 0;
        }
        else
        {
            for(i=0; i<search_class->ext.ext_tp_num; i++)
            {
                if(search_class->ext.ext_tp_id[i] == tp_id)
                {
                    search_class->ext.need_search_ext = 1;
                    break;
                }
            }
        }
		search_class->dvbt2_info.dvbt2_id_num = 0;
		search_class->dvbt2_info.dvbt2_id_finish = 0;
		if(search_class->dvbt2_info.dvbt2_id != NULL)
		{
			GxCore_Free(search_class->dvbt2_info.dvbt2_id);
		    search_class->dvbt2_info.dvbt2_id = NULL;
		}
		GxBus_PmTpGetById(tp_id, &tp);
		/*保存下该tp的tp id和sat id 用于节目信息 */
		search_class->search_tp_id_for_prog = tp_id;
		
		ret = gx_search_dvbt2_set_tp(tp.frequency, tp.tp_t.bandwidth);
		search_class->search_tp_finish_num++;
		if(ret == GX_SEARCH_ERR)
        {
            GxCore_ThreadYield();
            gx_search_sat_tp_reply(search_class->search_sat_num,
                    search_class->search_sat_finish_num,
                    search_class->search_tp_num,
                    search_class->search_tp_finish_num,
                    tp_id,
                    &tp,
                    NULL,
                    GX_SEARCH_LOCK_ERR,
                    GX_SEARCH_FRONT_DVB_DVBT2);

            goto start_lock;
        }

		if(gx_search_set_dvbt2_id(search_class,search_class->dvbt2_info.dvbt2_id[search_class->dvbt2_info.dvbt2_id_finish].data_plp_id,
		search_class->dvbt2_info.dvbt2_id[search_class->dvbt2_info.dvbt2_id_finish].common_plp_exist,
		search_class->dvbt2_info.dvbt2_id[search_class->dvbt2_info.dvbt2_id_finish].common_plp_id) != GX_SEARCH_OK)
		{
			search_class->dvbt2_info.dvbt2_id_finish++;
			GxCore_ThreadYield();
			goto start_lock;
		}
		if(search_class->dvbt2_info.dvbt2_id_num != 0)
		{
			search_class->dvbt2_info.dvbt2_id_finish++;
		}
		/*调用锁频函数,判断有没有锁定,没有锁定goto start_lock锁下一个tp,
		   如果锁定再判断ts有没有锁定 都锁定了返回GX_OK */
        if (GxFrontend_QueryStatus(search_class->search_tuner_num_for_prog) != 1) {
            GxCore_ThreadYield();//因为不停锁频的时候会占用大量时间片，放弃下避免其他服务出问题
            gx_search_sat_tp_reply(search_class->search_sat_num,
                    search_class->search_sat_finish_num,
                    search_class->search_tp_num,
                    search_class->search_tp_finish_num,
                    tp_id,
                    &tp,
                    NULL,
                    GX_SEARCH_LOCK_ERR,
                    GX_SEARCH_FRONT_DVB_DVBT2);

            goto start_lock;
        }
        else
        {
            gx_search_sat_tp_reply(search_class->search_sat_num,
                    search_class->search_sat_finish_num,
                    search_class->search_tp_num,
                    search_class->search_tp_finish_num,
                    tp_id,
                    &tp,
                    NULL,
                    GX_SEARCH_LOCK_OK,
                    GX_SEARCH_FRONT_DVB_DVBT2);

        }
	} 
	else if (search_class->search_tp_finish_num == search_class->search_tp_num) 
	{
		ret = gx_search_nit_exsit_check(search_class);
		if(ret == GX_SEARCH_OK)
		{
			GxCore_ThreadYield();
			goto start_lock;
		}
		return GX_SEARCH_FINISH;
	} 
	else 
	{
		return GX_SEARCH_ERR;
	}
	return GX_SEARCH_OK;
}
