/*
 * =====================================================================================
 *
 *       Filename:  gxdvb_blind_search.c
 *
 *    Description: 盲扫相关
 *
 *        Version:  1.0
 *        Created:  2015年01月28日 11时12分58秒
 *       Revision:  none
 *       Compiler:  csky-ecos
 *
 *         Author:  xiahuangshuai,
 *   Organization:
 *
 * =====================================================================================
 */

#include <sys/ioctl.h>
#include "include/gx_search_private.h"
#include "module/frontend/gxfrontend_module.h"
#include "gxcore.h"

status_t gx_dvb_blind_search_tp_lock(void * p)
{
	return GX_SEARCH_OK;
}

uint32_t gx_blind_get_status(void)
{
	return SearchServiceObj.searchclass.blindclass->blind_status;
}


void gx_blind_set_status(int32_t status,GxBlindSearchClass* blindclass)
{
	blindclass->blind_status = status;
	GxCore_SemPost(SearchServiceObj.searchclass.blindclass->search_blind_sem);
	return;
}

status_t gx_dvb_blind_scan_init_params(void *p,void* search_params)
{
    uint32_t                exist_tp_count = 0;
    ProtocolNitObj * nitobj = NULL;

	GxSearchClassObj* search_class = (GxSearchClassObj*)p;
    GxMsgProperty_ScanStart* params = (GxMsgProperty_ScanStart*)search_params;

	if(params->FrontendParams.s_info.info.max_num == 0
            ||params->FrontendParams.s_info.info.array == NULL)
    {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
        GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---search params err!!\n");
#endif
        return GX_SEARCH_ERR;
    }

    search_class->nit.nit_switch = params->ServiceParams.nit_switch;

    /*开辟nit的tp空间,由于不确定具体数量,该空间为所能保存的tp最大数量减去
      已经存在的tp的数量 */
	if (search_class->nit.nit_switch == GX_SEARCH_NIT_ENABLE)
    {
        exist_tp_count = GxBus_PmTpNumGet();

        if (MAX_TP_COUNT > exist_tp_count)
        {
            search_class->nit.search_nit_tp_id = (uint32_t *)
                GxCore_Malloc(sizeof(uint32_t) *
                        (MAX_TP_COUNT - exist_tp_count));
        }
        if(params->ServiceParams.nit_tp_private_check != NULL)
        {
            search_class->nit.nit_tp_private_check = params->ServiceParams.nit_tp_private_check;
        }
        else
        {
            search_class->nit.nit_tp_private_check = NULL;
        }

        search_class->nit.nit_obj = GxCore_Mallocz(sizeof(ProtocolNitObj));
        if(search_class->nit.nit_obj == NULL)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("nit search nit_obj malloc failed\n");
#endif
            goto err;
        }

        nitobj = search_class->nit.nit_obj;
        nitobj->si_nit = -1;
        if(params->ServiceParams.timeout.nit != 0)
        {
            nitobj->nit_time_out = params->ServiceParams.timeout.nit;
        }
        else
        {
            nitobj->nit_time_out = SEARCH_NIT_TIMEOUT_VALUE;
        }

        nitobj->nit_create = Protocol_nit_create;
        nitobj->nit_get = Protocol_nit_get;
        nitobj->nit_time_out_deal = Protocol_nit_time_out_deal;
        nitobj->nit_realsease = Protocol_nit_release;
    }

#if 0
    /*有多少个pat body就意味着有多少个总的sdt body,所以空间应该开总的
      body,但是pat和sdt是同时解析的,所以无法通过pat body判断,直接开一个300 */

    search_class->search_sdt_body = (GxSearchSdtBody *)
        GxCore_Malloc(sizeof(GxSearchSdtBody) * MAX_PROG_PER_TP);	//每个tp最多300节目
#endif

    /*对于搜到的节目由于不定长 所以直接开最大节目数量 用完后释放 该地方会耗费几百k内存*/
    search_class->search_prog_id_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * MAX_PROG_COUNT);
    if(search_class->search_prog_id_arry == NULL)
    {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
        GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---malloc search_prog_arry space err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
    search_class->search_prog_id_num = 0;

    /*记录搜索扩展的信息*/
	if(params->ServiceParams.ext_tp_id != NULL && params->ServiceParams.ext_tp_num != 0)
    {
        search_class->ext.ext_tp_num = params->ServiceParams.ext_tp_num;
        search_class->ext.ext_tp_id = GxCore_Mallocz(sizeof(uint32_t)*params->ServiceParams.ext_tp_num);
        if(search_class->ext.ext_tp_id == NULL)
        {
#ifdef GX_BUS_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---malloc ext_tp space err!\n");
#endif
            goto err;
        }
        memcpy(search_class->ext.ext_tp_id,params->ServiceParams.ext_tp_id,sizeof(uint32_t)*params->ServiceParams.ext_tp_num);
    }


    search_class->search_sat_num = params->FrontendParams.s_info.info.max_num;
    search_class->search_sat_id =
        (uint32_t *) GxCore_Malloc(sizeof(uint32_t) * search_class->search_sat_num);
    if (search_class->search_sat_id == NULL)
    {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
        GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---malloc sat id space err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
    memcpy(search_class->search_sat_id, params->FrontendParams.s_info.info.array,
			sizeof(uint32_t) * search_class->search_sat_num);	//记录所要搜索的sat的id

    if(params->FrontendParams.s_info.info.ts == NULL)
    {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
        GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---ts num err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
    search_class->search_ts = (uint32_t *) GxCore_Malloc(sizeof(uint32_t)*search_class->search_sat_num);
    if(search_class->search_ts == NULL)
    {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
        GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---malloc ts num space err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
    memcpy(search_class->search_ts,params->FrontendParams.s_info.info.ts, sizeof(uint32_t) * search_class->search_sat_num);//记录所要搜索的sat的ts

    return GX_SEARCH_OK;
err:
    return GX_SEARCH_ERR;
}


status_t gx_search_blind_get_params(GxSearchClassObj* searchclass)
{
	GxBlindSearchClass* params = searchclass->blindclass;
	uint32_t sat_id = 0;
	GxBusPmDataSat sat = {0};
	status_t ret = 0;
	GxMessage* new_msg = NULL;
	GxMsgProperty_BlindScanReply *msg_params = NULL;
	if(searchclass->search_sat_num != 0
			&&searchclass->search_sat_finish_num<searchclass->search_sat_num)
	{
		sat_id = searchclass->search_sat_id[searchclass->search_sat_finish_num];
		if(GXCORE_ERROR == GxBus_PmSatGetById(sat_id,&sat))
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---get sat by id err!!\n");
#endif
			return GX_SEARCH_FINISH;
		}

		/*判断是双本振还是单本振*/
		params->local_fre1 = sat.sat_s.lnb1;
		params->local_fre2 = sat.sat_s.lnb2;
		if((sat.sat_s.lnb1 != sat.sat_s.lnb2)
				&& sat.sat_s.lnb2 != 0)
		{
			params->lnb_ocs =BLIND_SCAN_LNB_OCS;
		}
		else
		{
			params->lnb_ocs =BLIND_SCAN_LNB_SINGLE;
		}
		params->switch_22k = sat.sat_s.switch_22K;
		/*判断盲扫的极化方向*/
		if(sat.sat_s.lnb_power== GXBUS_PM_SAT_LNB_POWER_ON)
		{
			params->polar = BLIND_SCAN_POLAR_ALL;
		}
		else if(sat.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_18V)
		{
			params->polar = BLIND_SCAN_POLAR_H;
		}
		else if(sat.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_13V)
		{
			params->polar = BLIND_SCAN_POLAR_V;
		}
		/*if(BLIND_POLAR_ALL == search_class.search_blind_polar)
		  {
		  params->polar = BLIND_SCAN_POLAR_ALL;
		  }
		  else if(BLIND_POLAR_H == search_class.search_blind_polar)
		  {
		  params->polar = BLIND_SCAN_POLAR_H;
		  }
		  else if(BLIND_POLAR_V == search_class.search_blind_polar)
		  {
		  params->polar = BLIND_SCAN_POLAR_H;
		  }*/
		/*设置起始频率和结束频率*/
		params->start_fre = params->BLIND_SCAN_LOW_FREQ_MHZ;
		params->end_fre = params->BLIND_SCAN_HIGH_FREQ_MHZ;
		/*判断是ku波段还是c波段*/
		if(sat.sat_s.lnb1<7000 && sat.sat_s.lnb2<7000)
		{
			params->lnb_type = BLIND_SCAN_LNB_C;
		}
		else
		{
			params->lnb_type = BLIND_SCAN_LNB_KU;
		}
		gx_search_realease_frontend(searchclass);//一定先释放前端，因为释放用到了search_class里面的东西
		searchclass->search_ts_cur = searchclass->search_ts[searchclass->search_sat_finish_num];
		searchclass->search_tuner_num_for_prog = sat.tuner;
		searchclass->search_sat_id_for_prog = sat_id;
		ret = gx_search_init_frontend(searchclass);
		if(GX_SEARCH_OK != ret)
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---init frontend err!!\n");
#endif
			return GX_SEARCH_FINISH;
		}
		searchclass->search_sat_finish_num++;
		params->window_num  = 0;
		params->window_total = 0;

		params->try_tp_stage = 0;
		params->window_num_filt = 0;

		new_msg = GxBus_MessageNew(GXMSG_SEARCH_BLIND_SCAN_REPLY);
		msg_params =
			(GxMsgProperty_BlindScanReply *) GxBus_GetMsgPropertyPtr(new_msg,
					GxMsgProperty_BlindScanReply);

		msg_params->frequency = 0;
		msg_params->sat_max_count = searchclass->search_sat_num;
		memcpy(msg_params->sat_name,sat.sat_s.sat_name,MAX_SAT_NAME);
		msg_params->sat_name[MAX_SAT_NAME-1] = 0;
		msg_params->sat_num = searchclass->search_sat_finish_num;
		msg_params->tp_s.polar = 0;
		msg_params->tp_s.symbol_rate = 0;
		msg_params->type = BLIND_TP;
		msg_params->tp_id = 0;
		msg_params->sat_id = searchclass->search_sat_id_for_prog;
		GxBus_MessageSend(new_msg);

		return GX_SEARCH_OK;

	}
	else
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---blind finish!!\n");
#endif
		return GX_SEARCH_FINISH;
	}
}


static bool gx_blind_progress_check_stop(void)
{
    return ((SearchServiceObj.searchclass.blindclass->blind_status  == SEARCH_STOP)?true:false);
}


static uint32_t  gx_blind_single_double_poar_progress(uint32_t start,GxBlindSearchClass* blind_scan)
{
    uint32_t progress = 0;
    if(blind_scan->start_flag == 1)//已经开始调用驱动了，进度计算使用window
    {
        if(blind_scan->window_total == 0)
        {
            return 0;
        }
        progress = start +(blind_scan->window_num *10)/blind_scan->window_total;
    }
    else//还没有调用驱动或者调用驱动已经结束
    {
        if(blind_scan->window_num_filt == 0)
        {
            return 0;
        }
        progress = start + 10 + (blind_scan->try_tp_stage *40)/blind_scan->window_num_filt;
    }
    return progress;
}


static uint32_t  gx_blind_single_one_poar_progress(uint32_t start,GxBlindSearchClass* blind_scan)
{
    uint32_t progress = 0;;
    if(blind_scan->start_flag == 1)//已经开始调用驱动了，进度计算使用window
    {
        if(blind_scan->window_total == 0)
        {
            return 0;
        }
        progress = start +(blind_scan->window_num *10)/blind_scan->window_total;
    }
    else//还没有调用驱动或者调用驱动已经结束
    {
        if(blind_scan->window_num_filt == 0)
        {
            return 0;
        }
        progress = start + 10 + (blind_scan->try_tp_stage *90)/blind_scan->window_num_filt;
    }
    return progress;
}
static uint32_t  gx_blind_ocs_double_poar_ku_progress(uint32_t start,GxBlindSearchClass* blind_scan)
{
    uint32_t progress = 0;;
    if(blind_scan->start_flag == 1)//已经开始调用驱动了，进度计算使用window
    {
        if(blind_scan->window_total == 0)
        {
            return 0;
        }
        progress = start +(blind_scan->window_num *10)/blind_scan->window_total;
    }
    else//还没有调用驱动或者调用驱动已经结束
    {
        if(blind_scan->window_num_filt == 0)
        {
            return 0;
        }
        progress = start + 10 + (blind_scan->try_tp_stage *15)/blind_scan->window_num_filt;
    }
    return progress;
}


static status_t gx_blind_send_progress(uint8_t progress, GxBlindSearchStage stage)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_BlindScanReply *params = NULL;
	GxSearchClassObj *searchclass = gx_search_get_service_paramsptr();

	new_msg = GxBus_MessageNew(GXMSG_SEARCH_BLIND_SCAN_REPLY);
	params =(GxMsgProperty_BlindScanReply *) GxBus_GetMsgPropertyPtr(new_msg,
								  									 GxMsgProperty_BlindScanReply);
	params->progress = progress;
	params->type = BLIND_PROGRESS;
    params->stage = stage;
	params->tp_id = 0;
	params->sat_id = searchclass->search_sat_id_for_prog;
	GxBus_MessageSend(new_msg);
	return GX_SEARCH_OK;
}


static status_t gx_blind_progress(GxBlindSearchClass* blind_scan)
{
    uint32_t progress = 0;
    GxBlindSearchStage blind_stage = BLIND_STAGE_NONE;

    if(gx_blind_progress_check_stop())
    {
        return GX_SEARCH_FINISH;
    }

    switch(blind_scan->polar_stage)
    {
        case BLIND_SINGLE_ONE_POLAR:           //单本振单极化
        case BLIND_OCS_C_ONE_POLAR:            //双本振c波段单极化
            progress = gx_blind_single_one_poar_progress(0,blind_scan);
            break;
        case BLIND_SINGLE_DOUBLE_POLAR_1:      //单本振双极化第一个极化
        case BLIND_OCS_C_DOUBLE_POLAR_1:       //双本振c波段双极化第一个极化
        case BLIND_OCS_KU_ONE_POLAR_LOW:       //双本振ku波段单极化low
            progress =  gx_blind_single_double_poar_progress(0,blind_scan);
            break;
        case BLIND_SINGLE_DOUBLE_POLAR_2:      //单本振双极化第二个极化
        case BLIND_OCS_C_DOUBLE_POLAR_2:       //双本振c波段双极化第二个极化
        case BLIND_OCS_KU_ONE_POLAR_HIGH:      //双本振ku波段单极化high
            progress =  gx_blind_single_double_poar_progress(50,blind_scan);
            break;
        case BLIND_OCS_KU_DOUBLE_POLAR_LOW1:   //双本振ku波段双极化第一个极化low
            progress = gx_blind_ocs_double_poar_ku_progress(0,blind_scan);
            break;
        case BLIND_OCS_KU_DOUBLE_POLAR_LOW2:   //双本振ku波段双极化第二个极化low
            progress = gx_blind_ocs_double_poar_ku_progress(25,blind_scan);
            break;
        case BLIND_OCS_KU_DOUBLE_POLAR_HIGH1:  //双本振ku波段双极化第一个极化high
            progress = gx_blind_ocs_double_poar_ku_progress(50,blind_scan);
            break;
        case BLIND_OCS_KU_DOUBLE_POLAR_HIGH2:  //双本振ku波段双极化第二个极化high
            progress = gx_blind_ocs_double_poar_ku_progress(75,blind_scan);
            break;

        default:
            return GX_SEARCH_OK;
    }
    (blind_scan->start_flag == 1) ? (blind_stage = BLIND_STAGE_TP) : (blind_stage = BLIND_STAGE_PROG);
    gx_blind_send_progress((uint8_t)progress, blind_stage);
    return GX_SEARCH_OK;
}



static status_t gx_blind_set_22k(uint32_t flag,GxSearchClassObj* searchclass)
{
    int32_t ret = 0;
    if(flag == GXBUS_PM_SAT_22K_ON)
    {
        ret = ioctl(searchclass->blindclass->search_module_frontend, FE_SET_TONE, SEC_TONE_ON);
    }
    else
    {
        ret = ioctl(searchclass->blindclass->search_module_frontend, FE_SET_TONE, SEC_TONE_OFF);
    }
    if(ret == -1)
    {
        GX_BUS_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---set frontend 22K err!!\n");
        return GX_SEARCH_ERR;
    }
    return GX_SEARCH_OK;
}

static status_t gx_blind_set_poarl(uint8_t polar,GxSearchClassObj* searchclass)
{
    int32_t ret = -1;
    switch(polar)
    {
        case BLIND_SCAN_POLAR_H:
            ret = ioctl(searchclass->blindclass->search_module_frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
            break;

        case BLIND_SCAN_POLAR_V:
            ret = ioctl(searchclass->blindclass->search_module_frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
            break;

        default:
            break;
    }
    if(ret == -1)
    {
        GX_BUS_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---set frontend voltage err!!\n");
        return GX_SEARCH_ERR;
    }
    return GX_SEARCH_OK;
}


static int32_t gx_blind_sort_tp_by_fre(const void *p1, const void *p2)
{
	int32_t fre1 = 0;
	int32_t fre2 = 0;
	fre1 = ((GxBlindSearchTp*)p1)->fre;
	fre2 = ((GxBlindSearchTp*)p2)->fre;
	return fre1 - fre2;
}


static int32_t gx_blind_scan_window(GxBlindSearchClass* blind_scan)
{
    uint32_t fcenter = 0;
	struct fe_set_window_parameters params;
	struct dvb_frontend_parameters* get_params = NULL;
	uint32_t i = 0;
	int32_t tp_num = 0;

	if(blind_scan->start_fre < blind_scan->BLIND_SCAN_LOW_FREQ_MHZ)
    {
        blind_scan->start_fre = blind_scan->BLIND_SCAN_LOW_FREQ_MHZ;
    }
    if(blind_scan->end_fre > blind_scan->BLIND_SCAN_HIGH_FREQ_MHZ)
    {
        blind_scan->end_fre = blind_scan->BLIND_SCAN_HIGH_FREQ_MHZ;
    }

	blind_scan->window_total = (blind_scan->end_fre + blind_scan->BLIND_SCAN_STEP_MHZ - blind_scan->start_fre)/
		blind_scan->BLIND_SCAN_STEP_MHZ;
	blind_scan->window_num= 0;
	blind_scan->start_flag = 1;
	fcenter = blind_scan->start_fre;
	/*先进行正常符号率的盲扫*/
    do
    {
		i = 0;
        if(gx_blind_get_status() == SEARCH_STOP)
        {
			tp_num = -1;
            goto err;
        }
        params.fcenter = fcenter;
        params.lpf_bw_window = blind_scan->BLIND_SCAN_WINDOW_SIZE_K;
        if(ioctl(blind_scan->search_module_frontend, FE_SET_BLINDSCAN, &params) < 0)
        {
            goto err;
        }
		//gxlogd("~~~~~~~~~%s %d num = %d size = %d~~~~~~\n",__FILE__,__LINE__,params.tp_num,params.tp_num*sizeof(struct dvb_frontend_parameters));
		if(params.tp_num != 0)
		{
			get_params = GxCore_Malloc(params.tp_num*sizeof(struct dvb_frontend_parameters));
			if(get_params == NULL)
			{
				tp_num = -1;
				goto err;
			}
			get_params->u.dvbs_param.tp_num_bs = params.tp_num;
			if(ioctl(blind_scan->search_module_frontend, FE_GET_BLINDSCAN, get_params) < 0)
			{
				tp_num = -1;
				goto err;
			}
			for(i=0; i<params.tp_num; i++)
			{
				if(tp_num+i >= BLIND_SCAN_TP_COUNT)
				{
					goto finish;
				}
				blind_scan->tp[i+tp_num].fre = get_params[i].frequency;
				blind_scan->tp[i+tp_num].symbol_rate = get_params[i].u.dvbs_param.symbol_rate;
			}
		}
        gx_blind_progress(blind_scan);
        tp_num += i;
        fcenter += blind_scan->BLIND_SCAN_STEP_MHZ;
        blind_scan->window_num += 1;

		if(get_params != NULL)
		{
			GxCore_Free(get_params);
			get_params = NULL;
		}
    }while(fcenter <= blind_scan->end_fre + blind_scan->BLIND_SCAN_STEP_MHZ);
    /*进行低符号率的盲扫*/
//    fcenter = blind_scan->start_fre;
   /*do
    {
        params.fcenter = fcenter;
        params.lpf_bw_window = BLIND_SCAN_WINDOW_SIZE_K;
        params.lsymbol_rate = 0;
        if(ioctl(search_class.search_module_frontend, FE_SET_BLINDSCAN, &params) < 0)
        {
            goto err;
        }
        get_params = GxCore_Malloc(params.tp_num*sizeof(struct dvb_frontend_parameters));
        if(get_params == NULL)
        {
            goto err;
        }
        if(ioctl(search_class.search_module_frontend, FE_GET_BLINDSCAN, get_params) < 0)
        {
            goto err;
        }
        for(i=0; i<params.tp_num; i++)
        {
            if(tp_num+i > BLIND_SCAN_TP_COUNT)
            {
                goto finish;
            }

            blind_scan->tp[i+tp_num].fre = get_params[i].frequency;
            blind_scan->tp[i+tp_num].symbol_rate = get_params[i].symbol_rate;
        }
        tp_num += i;
        fcenter += BLIND_SCAN_STEP_MHZ;
        blind_scan->window_num += 1;
        gx_blind_progress(blind_scan);
        GxCore_Free(get_params);
        get_params = NULL;
    }while(fcenter <= blind_scan->end_fre + BLIND_SCAN_STEP_MHZ);*/
finish:
    if(get_params != NULL)
    {
        GxCore_Free(get_params);
    }
    return tp_num;
err:
    if(get_params != NULL)
    {
        GxCore_Free(get_params);
    }
    return tp_num;
}


static int32_t gx_blind_sort_tp(GxBlindSearchClass* blind_scan)
{
    int32_t tp_num = 0;
    int32_t fre_temp = 0;
    int32_t symbol_temp = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t sym_index = 4;
    GxBlindSearchTp* tp_temp = NULL;

    tp_num = gx_blind_scan_window(blind_scan);
    if(tp_num <=0)
    {
        return tp_num;
    }
    tp_temp = GxCore_Malloc(sizeof(GxBlindSearchTp)*BLIND_SCAN_TP_COUNT);
    if(tp_temp == NULL)
    {
		GX_BUS_SEARCH_NORMAL_PRINTF("[blind search]---no memery!!\n");
        return 0;
    }
    //sort by fre
    qsort(blind_scan->tp,tp_num,sizeof(GxBlindSearchTp),gx_blind_sort_tp_by_fre);

    //delete the semblable tp
    for(i=0; i<tp_num; i++)
    {
    	if(gx_blind_get_status() == SEARCH_STOP)
    	{
			GxCore_Free(tp_temp);
			 return 0;
		}
        if(blind_scan->tp[i].fre < 945000||
                blind_scan->tp[i].fre > 2155000||
                blind_scan->tp[i].symbol_rate < 800||
                blind_scan->tp[i].symbol_rate > 50000)
        {
            continue;
        }
		if(i != (tp_num-1))
		{
	        fre_temp = blind_scan->tp[i+1].fre - blind_scan->tp[i].fre;
	        symbol_temp = blind_scan->tp[i+1].symbol_rate - blind_scan->tp[i].symbol_rate;
	        if(symbol_temp < 0)
	        {
	            symbol_temp = blind_scan->tp[i].symbol_rate - blind_scan->tp[i+1].symbol_rate;
	        }
			// above 40M symbole, need to record, check it is ok or not after "get_tpinfo"
	        if((blind_scan->tp[i].symbol_rate <= 40000) && (((fre_temp <= blind_scan->tp[i].symbol_rate/sym_index)&&
	                    (fre_temp <= blind_scan->tp[i+1].symbol_rate/sym_index)&&
	                    (symbol_temp < blind_scan->tp[i].symbol_rate/32)&&
	                    (symbol_temp < blind_scan->tp[i+1].symbol_rate/32))||
	                fre_temp <= 500))
	        {
	            continue;
	        }
		}
		//gxlogd("\n---[2][sort tp][fre = %d][sry = %d]-----\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate);
        memcpy(&tp_temp[j],&(blind_scan->tp[i]),sizeof(GxBlindSearchTp));
        j++;
    }
    memcpy(blind_scan->tp,tp_temp,sizeof(GxBlindSearchTp)*j);
    GxCore_Free(tp_temp);
    return j;
}


static void gx_blind_scan_get_tp_info(GxBlindSearchClass* blind_scan,uint32_t i, struct dvb_frontend_parameters params)
{
	uint32_t if_frequency = 0;
	uint32_t local_frequency = 0;
	switch(params.u.dvbs_param.type)
	{
		case FE_DVB_S:
			blind_scan->tp[i].type = DVBS;
			blind_scan->tp[i].qpsk = params.u.dvbs_param.modulation;
			break;

		case FE_DIRECTV:
			blind_scan->tp[i].type = DIRECTV;
			blind_scan->tp[i].qpsk = params.u.dvbs_param.modulation;
			break;

		case FE_DVB_S2:
			blind_scan->tp[i].type = DVBS2;
			blind_scan->tp[i].qpsk = params.u.dvbs_param.modulation;
			break;

        default:
            blind_scan->tp[i].type = DVBS2;
            blind_scan->tp[i].qpsk = DVBS2_8PSK_23;
            break;
    }
    if_frequency = (params.frequency*1000+500)/1000;

    if(blind_scan->lnb_type == BLIND_SCAN_LNB_C)
    {
        if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS)//双本振
        {
            if(blind_scan->tp_polar == GXBUS_PM_TP_POLAR_H)
            {
                local_frequency = blind_scan->local_fre1;
            }
            else
            {
                local_frequency = blind_scan->local_fre2;
            }
        }
        else
        {
            local_frequency = blind_scan->local_fre1;
        }
        if(local_frequency > if_frequency)
        {
            blind_scan->tp[i].fre = local_frequency - if_frequency;
        }
        else
        {
            blind_scan->tp[i].fre = local_frequency + if_frequency;
        }
    }
    else
    {
        if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS)
        {
            if(blind_scan->switch_22k == GXBUS_PM_SAT_22K_ON)//22k on
            {
                if(blind_scan->local_fre1 > blind_scan->local_fre2)
                {
                    local_frequency = blind_scan->local_fre1;
                }
                else
                {
                    local_frequency = blind_scan->local_fre2;
                }
            }
            else
            {
                if(blind_scan->local_fre1 > blind_scan->local_fre2)
                {
                    local_frequency = blind_scan->local_fre2;
                }
                else
                {
                    local_frequency = blind_scan->local_fre1;
                }
            }
        }
        else
        {
            local_frequency = blind_scan->local_fre1;
        }
        blind_scan->tp[i].fre = local_frequency + if_frequency;
    }
	blind_scan->tp[i].symbol_rate = params.u.dvbs_param.symbol_rate;
    return;
}



static void gx_search_blind_new_tp_get(uint32_t i)
{
	int32_t count = 0;
	GxBusPmDataTP tp = {0};
	GxBusPmDataSat sat = {0};
	GxBusPmTpPolar temp_polar = 0;
	GxMessage              *new_msg = NULL;
    GxMsgProperty_BlindScanReply *params = NULL;
	GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("gx_search_table_filter_start malloc failed\n");
#endif
		while(1);
        return ;
    }

	GxBlindSearchClass* blind_scan = searchclass->blindclass;
    temp_polar = blind_scan->tp_polar;
    count = GxBus_PmTpExistChek(searchclass->search_sat_id_for_prog,
            blind_scan->tp[i].fre,
            blind_scan->tp[i].symbol_rate,
            temp_polar,
            0,
            (uint16_t*)(&(tp.id)));
    if(0 == count)
    {
        tp.frequency = blind_scan->tp[i].fre;
        tp.sat_id = searchclass->search_sat_id_for_prog;
        tp.tp_s.polar = temp_polar;
        tp.tp_s.symbol_rate = blind_scan->tp[i].symbol_rate;
        GxBus_PmTpAdd(&tp);
    }
    searchclass->search_tp_id_for_prog = tp.id;
    GxBus_PmSatGetById(searchclass->search_sat_id_for_prog,&sat);
	new_msg = GxBus_MessageNew(GXMSG_SEARCH_BLIND_SCAN_REPLY);
	params =
	    (GxMsgProperty_BlindScanReply *) GxBus_GetMsgPropertyPtr(new_msg,
								   GxMsgProperty_BlindScanReply);

	params->tp_id = tp.id;
	params->sat_id = searchclass->search_sat_id_for_prog;
	params->frequency = blind_scan->tp[i].fre;
	params->sat_max_count = searchclass->search_sat_num;
	memcpy(params->sat_name,sat.sat_s.sat_name,MAX_SAT_NAME);
	params->sat_name[MAX_SAT_NAME-1] = 0;
	params->sat_num = searchclass->search_sat_finish_num;
	params->tp_s.polar = temp_polar;
	params->tp_s.symbol_rate = blind_scan->tp[i].symbol_rate;
	params->type = BLIND_TP;
    params->mode = blind_scan->tp[i].type;
    params->qpsk = blind_scan->tp[i].qpsk;
	GxBus_MessageSend(new_msg);
	return ;
}

static status_t gx_blind_scan_tp(GxSearchClassObj* searchclass)
{
	int32_t tp_num;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t tp_count = 0;
	struct dvb_frontend_parameters params;
	uint32_t retry = 0;
	uint32_t locked = 0;
	//fe_status_t festatus;
	unsigned long           tick = 0;
	GxDemuxProperty_TSLockQuery ts_lock_status = {TS_SYNC_UNLOCKED};
	fe_status_t status;
	GxBusPmDataTP tp_record = {0};
	int frequency = 0;
	GxBlindSearchClass* blind_scan = searchclass->blindclass;

	tp_num = gx_blind_sort_tp(blind_scan);
	if(tp_num == -1)
	{
		ret = GX_SEARCH_FINISH;
		goto blind_no_ok;
	}
	blind_scan->window_num_filt = tp_num;
	blind_scan->try_tp_stage = 0;
	blind_scan->start_flag = 0;
	ret = gx_blind_progress(blind_scan);
	if(ret != GX_SEARCH_OK)
	{
		goto blind_no_ok;
	}

	memset(&tp_record,0,sizeof(GxBusPmDataTP));

	ioctl(blind_scan->search_module_frontend, FE_SET_FRONTEND_TUNE_MODE, 1);
	for(i=0; i<tp_num; i++)
	{
		ret = gx_blind_get_status();
		if(ret == SEARCH_STOP)
		{
			goto blind_no_ok;
		}

		params.frequency          = blind_scan->tp[i].fre;
		params.u.dvbs_param.symbol_rate = blind_scan->tp[i].symbol_rate*1000;
		params.u.dvbs_param.fec_inner   = FEC_AUTO;

		//gxlogd("set :  fre = %d  symbolrate = %d\n",params.frequency,params.u.dvbs_param.symbol_rate);
		blind_scan->try_tp_stage = i+1;
		if (ioctl(blind_scan->search_module_frontend, FE_SET_FRONTEND, &params) != 0)
		{
			goto send_progress;
		}

		locked = 0;
		status = FE_HAS_LOCK;

		if (blind_scan->tp[i].symbol_rate <= 5000)
			retry = 35;
		else if (blind_scan->tp[i].symbol_rate > 10000)
			retry = 25;
		else
			retry = 30;

		tick = GxCore_TickStart(retry);
		if(status == FE_HAS_LOCK)
		{
			do{
				ret = GxAVGetProperty(searchclass->search_device_handle,
						searchclass->search_demux_handle,
						GxDemuxPropertyID_TSLockQuery,
						&ts_lock_status,
						sizeof(GxDemuxProperty_TSLockQuery));
				if(ret < 0 )
				{
					//gxlogd("~~~~~~~~~~~~%s %d ~~~ret = %d~~~~~~~\n",__FILE__,__LINE__,status);
					goto blind_no_ok;
				}
				if(ts_lock_status.ts_lock==TS_SYNC_LOCKED)
				{
					break;
				}
			}while(!GxCore_TickEnd(tick));
		}
		else
		{
			goto send_progress;

		}

		if(ts_lock_status.ts_lock==TS_SYNC_LOCKED)
		{
			switch(status)
			{
				case FE_HAS_LOCK:
					locked = 1;
					//gxlogd("blind ok tp fre = %d\n",params.frequency);
					break;

				case FE_TIMEDOUT:
					//gxlogd("blind time out !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					locked = 2;
					break;
				default:
					GxCore_ThreadYield();
					break;
			}
		}
		else
		{
			GxCore_ThreadYield();
			continue;
		}
		if(locked == 2)
		{
			goto send_progress;
		}
		else if(locked == 1)
		{
			if(ioctl(blind_scan->search_module_frontend,FE_GET_FRONTEND,&params)>=0)
			{
				gx_blind_scan_get_tp_info(blind_scan,i,params);
				// filter the same tp, before scan program. Have already sort by frequency,so now only compare with the last one.
				{
					if(blind_scan->tp[i].symbol_rate > 20000)
						frequency = blind_scan->tp[i].symbol_rate/4000;
					else
						frequency = 3;
					if(tp_record.frequency != 0)
					{
						if(((blind_scan->tp[i].fre > (tp_record.frequency - frequency))
								&& (blind_scan->tp[i].fre < (tp_record.frequency + frequency)))
						   && ((blind_scan->tp[i].symbol_rate > (tp_record.tp_s.symbol_rate - 100))
						     	&& (blind_scan->tp[i].symbol_rate < (tp_record.tp_s.symbol_rate + 100))))
						{
							// find the same tp, skip it
							//gxlogd("\nBLIND SCAN, find the same TP[%d:%d], skip it\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate);
							goto send_progress;
						}
					}
					tp_record.frequency = blind_scan->tp[i].fre;
					tp_record.tp_s.symbol_rate = blind_scan->tp[i].symbol_rate;
				}

				tp_count++;
				//gxlogd("get :  fre = %d  symbolrate = %d,tp_count:%d\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate,tp_count);
				gx_blind_progress(blind_scan);
				gx_search_blind_new_tp_get(i);
				ret = gx_search_table_filter_start(searchclass);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
				GxCore_SemWait(blind_scan->search_blind_sem);
				ret = gx_blind_get_status();
				if(ret == SEARCH_STOP)
				{
					goto blind_no_ok;
				}
				gx_search_variable_next_tp_init();
			}
		}
send_progress:
		gx_blind_progress(blind_scan);
		continue;

	}

	ioctl(blind_scan->search_module_frontend, FE_SET_FRONTEND_TUNE_MODE, 0);
	return GX_SEARCH_OK;
blind_no_ok:

	ioctl(blind_scan->search_module_frontend, FE_SET_FRONTEND_TUNE_MODE, 0);
	return ret;
}


static status_t gx_blind_start(uint8_t polar,int32_t sat22k,GxSearchClassObj* searchclass)
{
    int32_t ret = 0;
	int32_t sat22k_temp = 0;
	GxBlindSearchClass* blind_scan = searchclass->blindclass;
    switch(polar)
    {
        case BLIND_SCAN_POLAR_V:
            blind_scan->tp_polar = GXBUS_PM_TP_POLAR_V;
            break;

        case BLIND_SCAN_POLAR_H:
            blind_scan->tp_polar = GXBUS_PM_TP_POLAR_H;
            break;

        default:
            break;
    }
    if(sat22k == GXBUS_PM_SAT_22K_ON)
    {
		sat22k_temp = SEC_TONE_ON;
    }
    else
    {
		sat22k_temp = SEC_TONE_OFF;
    }
    if(ret == -1)
    {
        GX_BUS_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---set frontend 22K err!!\n");
        return GX_SEARCH_ERR;
    }
	if(searchclass->search_diseqc != NULL)
	{
		switch(polar)
		{
			case BLIND_SCAN_POLAR_H:
				searchclass->search_diseqc(searchclass->search_sat_id_for_prog,SEC_VOLTAGE_18,sat22k_temp);
				break;

			case BLIND_SCAN_POLAR_V:
				searchclass->search_diseqc(searchclass->search_sat_id_for_prog,SEC_VOLTAGE_13,sat22k_temp);
				break;

			default:
				break;
		}
	}
	else
	{
        ret = gx_blind_set_22k(sat22k,searchclass);
        if(ret != GX_SEARCH_OK)
        {
            goto blind_no_ok;//
        }
		ret = gx_blind_set_poarl(polar,searchclass);
		if(ret != GX_SEARCH_OK)
		{
			goto blind_no_ok;
		}
	}
    ret = gx_blind_scan_tp(searchclass);
    if(ret != GX_SEARCH_OK)
    {
        goto blind_no_ok;
    }

    return GX_SEARCH_OK;

blind_no_ok:
    return ret;
}


status_t gx_blind_search(GxSearchClassObj* searchclass)
{
    status_t ret = 0;
    uint8_t i = 0;
	GxBlindSearchClass* blind_scan = searchclass->blindclass;
    blind_scan->start_flag = 0;

    if(blind_scan->polar == BLIND_SCAN_POLAR_ALL)
    {
        i = 1;
    }
    if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_SINGLE)//单本振
    {
        if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫h
        {
            blind_scan->window_num_filt = 0;
            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_SINGLE_ONE_POLAR;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_SINGLE_DOUBLE_POLAR_1;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            ret = gx_blind_start(BLIND_SCAN_POLAR_H,blind_scan->switch_22k, searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
            i = 1;
        }
        blind_scan->window_num_filt = 0;
        if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫v
        {
            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_SINGLE_ONE_POLAR;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_SINGLE_DOUBLE_POLAR_2;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            ret = gx_blind_start(BLIND_SCAN_POLAR_V,blind_scan->switch_22k, searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
        }
    }
    else if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS
            &&blind_scan->lnb_type == BLIND_SCAN_LNB_C)//c 波段双本振
    {
        if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫h
        {
            blind_scan->window_num_filt = 0;
            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_OCS_C_ONE_POLAR;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_OCS_C_DOUBLE_POLAR_1;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            blind_scan->start_fre = blind_scan->BLIND_SCAN_LOW_FREQ_MHZ;
            blind_scan->end_fre = ((blind_scan->BLIND_SCAN_LOW_FREQ_MHZ + blind_scan->BLIND_SCAN_HIGH_FREQ_MHZ)/2-50);
            ret = gx_blind_start(BLIND_SCAN_POLAR_H,blind_scan->switch_22k, searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
        }
        if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫v
        {
            blind_scan->window_num_filt = 0;
            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_OCS_C_ONE_POLAR;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_OCS_C_DOUBLE_POLAR_2;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            blind_scan->start_fre = (blind_scan->BLIND_SCAN_LOW_FREQ_MHZ + blind_scan->BLIND_SCAN_HIGH_FREQ_MHZ)/2;
            blind_scan->end_fre = blind_scan->BLIND_SCAN_HIGH_FREQ_MHZ;
            ret = gx_blind_start(BLIND_SCAN_POLAR_V,blind_scan->switch_22k, searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
        }

    }
    else if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS
            &&blind_scan->lnb_type == BLIND_SCAN_LNB_KU)//KU 波段双本振
    {
        if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫low h
        {
            blind_scan->window_num_filt = 0;
			blind_scan->start_fre = 1100;
            blind_scan->end_fre = blind_scan->BLIND_SCAN_HIGH_FREQ_MHZ;
			blind_scan->switch_22k = GXBUS_PM_SAT_22K_ON;
            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_LOW;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_LOW1;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            ret = gx_blind_start(BLIND_SCAN_POLAR_H,GXBUS_PM_SAT_22K_ON,searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
        }
        if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫low v
        {
            blind_scan->window_num_filt = 0;
			blind_scan->start_fre = 1100;
            blind_scan->end_fre = blind_scan->BLIND_SCAN_HIGH_FREQ_MHZ;
			blind_scan->switch_22k = GXBUS_PM_SAT_22K_ON;
            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_LOW;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_LOW2;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            ret = gx_blind_start(BLIND_SCAN_POLAR_V,GXBUS_PM_SAT_22K_ON,searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
        }
        if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫high h
        {
            blind_scan->window_num_filt = 0;
			blind_scan->start_fre = blind_scan->BLIND_SCAN_LOW_FREQ_MHZ;
            blind_scan->end_fre = 1950;
			blind_scan->switch_22k = GXBUS_PM_SAT_22K_OFF;
            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_HIGH;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_HIGH1;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            ret = gx_blind_start(BLIND_SCAN_POLAR_H,GXBUS_PM_SAT_22K_OFF,searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
        }
        if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫high v
        {
            blind_scan->window_num_filt = 0;
			blind_scan->start_fre = blind_scan->BLIND_SCAN_LOW_FREQ_MHZ;
            blind_scan->end_fre = 1950;
			blind_scan->switch_22k = GXBUS_PM_SAT_22K_OFF;

            if(i == 0)
            {
                blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_HIGH;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            else
            {
                blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_HIGH2;
                ret = gx_blind_progress(blind_scan);
                if(ret != GX_SEARCH_OK)
                {
                    goto blind_no_ok;
                }
            }
            ret = gx_blind_start(BLIND_SCAN_POLAR_V,GXBUS_PM_SAT_22K_OFF,searchclass);
            if(ret != GX_SEARCH_OK)
            {
                goto blind_no_ok;
            }
        }
    }
    else
    {
		GX_BUS_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---err blind type!!\n");
		return GX_SEARCH_ERR;
    }
    return GX_SEARCH_OK;

blind_no_ok:
    return ret;
}



status_t gx_search_blind_scan_init_start(GxMessage* msg,GxSearchClassObj* searchclass)
{
	GxMsgProperty_ScanStart* search_params = NULL;
	GxBlindSearchClass* blindclass = NULL;
    handle_t sch;
	status_t ret = 0;
	searchclass->blindclass = &blindclassobj;

	blindclass = searchclass->blindclass;

    sch = GxBus_SchedulerCreate("AtscSearchConsoleScheduler",GXBUS_SCHED_CONSOLE,1024*32,GXOS_DEFAULT_PRIORITY);
    GxBus_ServiceLink(searchclass->SearchServiceMsgHandle,sch);


	blindclass->blind_status = SEARCH_RUNNING;
	blindclass->tp = GxCore_Malloc(sizeof(GxBlindSearchTp)*BLIND_SCAN_TP_COUNT);
    if(blindclass->tp == NULL)
    {
		GX_BUS_SEARCH_NORMAL_PRINTF("[blind search]---no memery!!\n");
		return GX_SEARCH_ERR;
    }

	search_params = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_ScanStart);

	if(search_params->FrontendParams.s_info.blind_info.enable == BLIND_SCAN_MODE_ENABLE)
	{
		blindclass->BLIND_SCAN_STEP_MHZ = search_params->FrontendParams.s_info.blind_info.step_mhz;
		blindclass->BLIND_SCAN_WINDOW_SIZE_K = search_params->FrontendParams.s_info.blind_info.window_size_k;
		if(search_params->FrontendParams.s_info.blind_info.low_freq_mhz < GX_BLIND_SEARCH_DEFAULT_LOW_LIMIT_FREQ_MHZ)
		{
			blindclass->BLIND_SCAN_LOW_FREQ_MHZ = GX_BLIND_SEARCH_DEFAULT_LOW_LIMIT_FREQ_MHZ;
		}
		else
		{
			blindclass->BLIND_SCAN_LOW_FREQ_MHZ = search_params->FrontendParams.s_info.blind_info.low_freq_mhz;
		}

		if(search_params->FrontendParams.s_info.blind_info.high_freq_mhz > GX_BLIND_SEARCH_DEFAULT_HIGH_LIMIT_FREQ_MHZ)
		{
			blindclass->BLIND_SCAN_HIGH_FREQ_MHZ = GX_BLIND_SEARCH_DEFAULT_HIGH_LIMIT_FREQ_MHZ;
		}
		else
		{
			blindclass->BLIND_SCAN_HIGH_FREQ_MHZ = search_params->FrontendParams.s_info.blind_info.high_freq_mhz;
		}
	}
	else
	{
		blindclass->BLIND_SCAN_STEP_MHZ = GX_BLIND_SEARCH_DEFAULT_STEP_MHZ;
		blindclass->BLIND_SCAN_WINDOW_SIZE_K = GX_BLIND_SEARCH_DEFAULT_WINDOW_SIZE_K;
		blindclass->BLIND_SCAN_LOW_FREQ_MHZ = GX_BLIND_SEARCH_DEFAULT_LOW_LIMIT_FREQ_MHZ;
		blindclass->BLIND_SCAN_HIGH_FREQ_MHZ = GX_BLIND_SEARCH_DEFAULT_HIGH_LIMIT_FREQ_MHZ;
	}

	ret = gx_search_blind_get_params(searchclass);
	if(GX_SEARCH_OK != ret)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---blind finish!!\n");
#endif
		return GX_SEARCH_FINISH;
	}
	GxFrontendClearErrFreFlag();
	GxCore_SemPost(blindclass->search_blind_sem);
	return GX_SEARCH_OK;
}
