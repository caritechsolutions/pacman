
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


static status_t	gx_search_sat_get_tuner_fre(GxBusPmDataTP* tp,
													GxBusPmDataSat* sat,
													uint32_t* fre)
{
	uint32_t lnb_fre = 0;
	GxBusPmSatLnbType type = 0;

	if(sat->sat_s.lnb2 == 0
		||sat->sat_s.lnb2 == sat->sat_s.lnb1)
	{
		type = GXBUS_PM_SAT_LNB_USER;
	}
	else if(sat->sat_s.lnb1<6000)
	{
		type = GXBUS_PM_SAT_LNB_OCS;
	}
	else if(sat->sat_s.lnb1>6000)
	{
		type = GXBUS_PM_SAT_LNB_UNIVERSAL;
	}
	
	switch(type)
	{
		case GXBUS_PM_SAT_LNB_USER:
			lnb_fre = sat->sat_s.lnb1;
			break;

		case GXBUS_PM_SAT_LNB_UNIVERSAL:
			if(tp->frequency >= 11700)
			{
                lnb_fre = sat->sat_s.lnb2;
			}
            else
            {
                lnb_fre = sat->sat_s.lnb1;
            }
			break;

		case GXBUS_PM_SAT_LNB_OCS:
			switch(tp->tp_s.polar)
			{
				case GXBUS_PM_TP_POLAR_H:
					lnb_fre = sat->sat_s.lnb1;
					break;

				case GXBUS_PM_TP_POLAR_V:
					lnb_fre = sat->sat_s.lnb2;
					break;

				case GXBUS_PM_TP_POLAR_AUTO:
					lnb_fre = sat->sat_s.lnb1;
					break;

				default:
					#ifdef GX_BUS_SEARCH_DBUG 
					GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat polar err!!\n");
					#endif
					return GX_SEARCH_ERR;
					break;
			}
			break;

		default:
			#ifdef GX_BUS_SEARCH_DBUG 
			GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb type err!!\n");
			#endif
			return GX_SEARCH_ERR;
			break;
	}
	if(lnb_fre >= tp->frequency)
	{
		*fre = lnb_fre - tp->frequency;
	}
	else
	{
		*fre = tp->frequency - lnb_fre;
	}
	return GX_SEARCH_OK;
}

static status_t	gx_search_sat_get_polar(GxBusPmDataTP* tp,
													GxBusPmDataSat* sat,
													fe_sec_voltage_t* polar)
{
	switch(sat->sat_s.lnb_power)
	{
		case GXBUS_PM_SAT_LNB_POWER_OFF:
			*polar = SEC_VOLTAGE_OFF;
			break;

		case GXBUS_PM_SAT_LNB_POWER_13V:
			*polar = SEC_VOLTAGE_13;
			break;

		case GXBUS_PM_SAT_LNB_POWER_18V:
			*polar = SEC_VOLTAGE_18;
			break;

		case GXBUS_PM_SAT_LNB_POWER_ON:
			switch(tp->tp_s.polar)
			{
				case GXBUS_PM_TP_POLAR_H:
					*polar = SEC_VOLTAGE_18;
					break;

				case GXBUS_PM_TP_POLAR_V:
					*polar = SEC_VOLTAGE_13;
					break;

				case GXBUS_PM_TP_POLAR_AUTO:
					*polar = SEC_VOLTAGE_18;
					break;

				default:
					#ifdef GX_BUS_SEARCH_DBUG 
					GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---tp polar type err!!\n");
					#endif
					return GX_SEARCH_ERR;
					break;
			}
			break;

			default:
				#ifdef GX_BUS_SEARCH_DBUG 
				GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb power err!!\n");
				#endif
				return GX_SEARCH_ERR;
				break;
	}

	return GX_SEARCH_OK;
}

static status_t gx_search_sat_22k_get(GxBusPmDataTP* tp,GxBusPmDataSat* sat,fe_sec_tone_mode_t* sat22k)
{
	if(sat->sat_s.lnb2 == 0
		||sat->sat_s.lnb2 == sat->sat_s.lnb1)
	{
         if(GXBUS_PM_SAT_22K_OFF == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_OFF;
		}
		else if(GXBUS_PM_SAT_22K_ON == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_ON;
		}
		else
		{
			*sat22k = SEC_TONE_ON;
		}
	}
	else if(sat->sat_s.lnb1<6000)
	{
		 if(GXBUS_PM_SAT_22K_OFF == sat->sat_s.switch_22K)
         {
             *sat22k = SEC_TONE_OFF;
         }
         else if(GXBUS_PM_SAT_22K_ON == sat->sat_s.switch_22K)
         {
             *sat22k = SEC_TONE_ON;
         }
         else
         {
             *sat22k = SEC_TONE_ON;
         }
	}
	else if(sat->sat_s.lnb1>6000)
	{
		 if(tp->frequency >= 11700)
         {
              *sat22k = SEC_TONE_ON;
         }
         else
         {
             *sat22k = SEC_TONE_OFF;
         }
	}
    return GX_SEARCH_OK;
}

static status_t gx_search_sat_set_tp(GxSearchClassObj * search_class,GxBusPmDataTP* tp,GxBusPmDataSat* sat)
{
	GxFrontend para = {0};
	fe_sec_tone_mode_t sat22k_mode = 0;
	uint32_t fre = 0;
	fe_sec_voltage_t polar = 0;
    int32_t ret = 0;
	//DVB-S
	gx_search_sat_get_tuner_fre(tp,sat,&fre);
	gx_search_sat_get_polar(tp,sat,&polar);
	gx_search_sat_22k_get(tp,sat,&sat22k_mode);
	para.sat22k = sat22k_mode;

	para.demux = search_class->search_demux_handle;
	para.dev = search_class->search_device_handle;
	para.fre = fre;
	para.polar = polar;
	para.symb = tp->tp_s.symbol_rate;
	para.tuner = search_class->search_tuner_num_for_prog;
	para.type = FRONTEND_DVB_S;

	if(search_class->search_diseqc != NULL)
	{
		search_class->search_diseqc(search_class->search_sat_id_for_prog,polar,sat22k_mode);
	}
	else
	{
		GxFrontend_SetVoltage(search_class->search_tuner_num_for_prog, polar);

		GxFrontend_Set22K(search_class->search_tuner_num_for_prog,sat22k_mode);
	}
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

status_t gx_dvb_search_sat_tp_lock(void * p)
{
	uint32_t                tp_id = 0;
	uint32_t                tp_num = 0;
	uint32_t                sat_id = 0;
	uint32_t				i = 0;
	uint32_t				locked = 0;
    unsigned long           tick = 0;
	GxBusPmDataTP * tp_arry = NULL;
	GxBusPmDataTP           tp = { 0 };
	GxBusPmDataSat          sat = { 0 };
	status_t ret = 0;
	uint32_t fre = 0, status = FE_TIMEDOUT;
    GxSearchClassObj * search_class = (GxSearchClassObj *)p;

 start_lock:

    if (search_class->search_tp_num != 0 && search_class->search_tp_finish_num < search_class->search_tp_num) {
        //当锁频时用户发送stop消息，需要用状态基来判断退出
        if(search_class->search_status != SEARCH_START)
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
        GxBus_PmTpGetById(tp_id, &tp);
        GxBus_PmSatGetById(tp.sat_id, &sat);
        search_class->search_tp_finish_num++;
        /*保存下该tp的tp id和sat id 用于节目信息和tuner*/
        search_class->search_sat_id_for_prog = tp.sat_id;
        search_class->search_tp_id_for_prog = tp.id;
        search_class->search_tuner_num_for_prog = sat.tuner;
        ret = gx_search_sat_set_tp(search_class,&tp,&sat);

        if(ret == GX_SEARCH_ERR)
        {
            GxCore_ThreadYield();
            gx_search_sat_tp_reply(search_class->search_sat_num,
                    search_class->search_sat_finish_num,
                    search_class->search_tp_num,
                    search_class->search_tp_finish_num,
                    tp_id,
                    &tp,
                    &sat,
                    GX_SEARCH_LOCK_ERR,
                    GX_SEARCH_FRONT_DVB_S);
            goto start_lock;
        }

        /*调用锁频函数,判断有没有锁定,没有锁定goto start_lock锁下一个tp,
          如果锁定再判断ts有没有锁定 都锁定了返回GX_OK */
        gx_search_sat_get_tuner_fre(&tp,&sat,&fre);
        if(ret != GX_SEARCH_CONTINUE)
        {
                ret = GxFrontend_ReadStatus(sat.tuner,&status);
        }
        else
        {
            status = FE_HAS_LOCK;
        }
#if 1
        if (tp.tp_s.symbol_rate <= 5000)
            i = 35;
        else if (tp.tp_s.symbol_rate > 10000)
            i = 25;
        else
            i = 30;

        tick = GxCore_TickStart(i);
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
#else
        locked = 0;
        //gxlogd("\n-----[present search ]----\n");
        if((GxFrontend_QueryFrontendStatus(search_class->search_tuner_num_for_prog) == 1)
                && (GxFrontend_QueryStatus(search_class->search_tuner_num_for_prog) == 1))
        {
            locked = 1;
        }
#endif
        if (locked == 0)
        {
            gx_search_sat_tp_reply(search_class->search_sat_num,
                    search_class->search_sat_finish_num,
                    search_class->search_tp_num,
                    search_class->search_tp_finish_num,
                    tp_id,
                    &tp,
                    &sat,
                    GX_SEARCH_LOCK_ERR,
                    GX_SEARCH_FRONT_DVB_S);
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
                    GX_SEARCH_LOCK_OK,
                    GX_SEARCH_FRONT_DVB_S);
        }
    } 
    else if (search_class->search_tp_finish_num == search_class->search_tp_num) 
    {
        //这里添加auto时通过卫星重新建立tp列表 goto start_lock 直到所有卫星也都finish了
        //才算整个搜索完成
		if(search_class->scan_type == GX_SEARCH_MANUAL
			||search_class->scan_type == GX_SEARCH_PID)
		{
			ret = gx_search_nit_exsit_check(search_class);
			if(ret == GX_SEARCH_OK)
			{	
				search_class->nit.nit_flg= NIT;
				search_class->nit.nit_switch = GX_SEARCH_NIT_DISABLE;
				goto start_lock;
			}
			return GX_SEARCH_FINISH;
		}
		else if(search_class->scan_type == GX_SEARCH_AUTO)
		{
			if(search_class->search_sat_num != 0 && search_class->search_sat_finish_num < search_class->search_sat_num)
			{
				sat_id = search_class->search_sat_id[search_class->search_sat_finish_num];
				if(search_class->search_tp_id != NULL)
				{
					GxCore_Free(search_class->search_tp_id);
					search_class->search_tp_id = NULL;
				}
				tp_num = GxBus_PmTpNumGetBySat(sat_id);
				search_class->search_tp_num = tp_num;
				search_class->search_tp_finish_num = 0;
				search_class->search_tp_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*tp_num);
				if(NULL == search_class->search_tp_id) {
					return GX_SEARCH_ERR;
				}

				tp_arry = (GxBusPmDataTP*)GxCore_Malloc(sizeof(GxBusPmDataTP)*tp_num);
				if(tp_arry == NULL)
				{
					GxCore_Free(search_class->search_tp_id);
					return GX_SEARCH_ERR;
				}
				GxBus_PmTpGetByPosInSat(0, tp_num, tp_arry);
				for(i = 0; i<tp_num; i++)
				{
					search_class->search_tp_id[i] = tp_arry[i].id;
				}
				GxCore_Free(tp_arry);

                gx_search_realease_frontend(search_class);//一定先释放前端，因为释放用到了search_class里面的东西
                search_class->search_ts_cur = search_class->search_ts[search_class->search_sat_finish_num];
				search_class->search_sat_finish_num++;
				GxBus_PmTpGetById(search_class->search_tp_id[0], &tp);
				GxBus_PmSatGetById(sat_id, &sat);
		        search_class->search_tuner_num_for_prog = sat.tuner;
                gx_search_init_frontend(search_class);
				
                gx_search_sat_tp_reply(search_class->search_sat_num,
								search_class->search_sat_finish_num,
								search_class->search_tp_num,
								search_class->search_tp_finish_num,
                                search_class->search_tp_id[0],
								&tp,
								&sat,
                                GX_SEARCH_LOCK_ERR,
								GX_SEARCH_FRONT_DVB_S);
				goto start_lock;
			}
			else
			{
				ret = gx_search_nit_exsit_check(search_class);
				if(ret == GX_SEARCH_OK)
				{
					search_class->nit.nit_flg= NIT;
					search_class->nit.nit_switch = GX_SEARCH_NIT_DISABLE;
					goto start_lock;
				}
				return GX_SEARCH_FINISH;
			}
		}
	} 
	else 
	{
		return GX_SEARCH_ERR;
	}
	return GX_SEARCH_OK;
}

