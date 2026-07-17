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

/* Functions --------------------------------------------------------------- */

extern GxMessage* GxBus_MessageTryGet(handle_t self);//为了接收stop消息
extern status_t GxBus_MessageTryFree(GxMessage *message);


static status_t gx_search_atsc_cable_set_tp(GxSearchClassObj * searchclass,uint32_t fre, uint32_t symb,
				       fe_spectral_inversion_t inversion, fe_modulation_t modulation)
{
	GxFrontend para = {0};
	int32_t ret = 0;

	para.demux = searchclass->search_demux_handle;
	para.dev = searchclass->search_device_handle;
	para.fre = fre;
	para.qam = modulation;
	para.symb = symb;
	para.tuner = searchclass->search_tuner_num_for_prog;
	para.type = FRONTEND_ATSC_C;

	ret = GxFrontend_SetTp(&para);
	if(ret == -1)//(((ret = GxFrontend_SetTp(&para)) != 0) && (ret != -2))
	{
		return GX_SEARCH_ERR;
	}
	else if(ret == -2)
	{
		return GX_SEARCH_CONTINUE;
	}

	return GX_SEARCH_OK;
}
status_t gx_atsc_search_c_tp_lock(void * p)
{
	uint32_t                tp_id = 0, status = FE_TIMEDOUT;
	GxBusPmDataTP           tp = { 0 };
	GxBusPmDataSat          sat = { 0 };
	status_t ret = 0;
	unsigned long           tick = 0;
	int32_t locked = 0;

    GxSearchClassObj * search_class = (GxSearchClassObj *)p;


 start_lock:
	if(search_class->search_tp_num != 0 && search_class->search_tp_finish_num < search_class->search_tp_num) {
        //当锁频时用户发送stop消息，需要用状态基来判断退出
        if(search_class->search_status != SEARCH_START)
        {
            return GX_SEARCH_FINISH;
        }
		tp_id = search_class->search_tp_id[search_class->search_tp_finish_num];
       
		GxBus_PmTpGetById(tp_id, &tp);
		GxBus_PmSatGetById(tp.sat_id, &sat);
		/*保存下该tp的tp id和sat id 用于节目信息 */
		search_class->search_tp_id_for_prog = tp_id;
		ret = gx_search_atsc_cable_set_tp(search_class,tp.frequency, tp.tp_c.symbol_rate,INVERSION_OFF,tp.tp_c.modulation);
		search_class->search_tp_finish_num++;
		if(ret == GX_SEARCH_ERR)
        {
            gx_search_sat_tp_reply(search_class->search_sat_num,
                    search_class->search_sat_finish_num,
                    search_class->search_tp_num,
                    search_class->search_tp_finish_num,
                    tp_id,
                    &tp,
                    &sat,
                    0,
                    GX_SEARCH_FRONT_ATSC_C);

            GxCore_ThreadYield();
            goto start_lock;
        }
		if(ret != GX_SEARCH_CONTINUE)
		{
			ret = GxFrontend_ReadStatus(sat.tuner,&status);
		}
		else
		{
			status = FE_HAS_LOCK;
		}

        tick = GxCore_TickStart(30);
        locked = 0;
		if(status == FE_HAS_LOCK)
		{
			do
			{
				GxCore_ThreadYield();
				if(GxFrontend_QueryStatus(search_class->search_tuner_num_for_prog) == 1)
				{
					locked = 1;
					break;
				}
			}while(!GxCore_TickEnd(tick));
		}
        if (locked == 0)
        {
            GxCore_ThreadYield();
            gx_search_sat_tp_reply(search_class->search_sat_num,
                    search_class->search_sat_finish_num,
                    search_class->search_tp_num,
                    search_class->search_tp_finish_num,
                    tp_id,
                    &tp,
                    &sat,
                    0,
                    GX_SEARCH_FRONT_ATSC_C);

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
                    &sat,
                    1,
                    GX_SEARCH_FRONT_ATSC_C);

        }

	} 
	else if (search_class->search_tp_finish_num == search_class->search_tp_num) 
	{
#if 0
		ret = gx_search_nit_exsit_check(); atsc 目前没有nit功能但是代码先保留方便以后增加
		if(ret == GX_SEARCH_OK)
		{
			goto start_lock;
		}
#endif
		return GX_SEARCH_FINISH;
	} 
	else 
	{
		return GX_SEARCH_ERR;
	}
	return GX_SEARCH_OK;
}

