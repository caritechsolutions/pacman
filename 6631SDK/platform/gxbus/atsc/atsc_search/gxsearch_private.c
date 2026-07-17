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
2014.05.14     xiahs		 create
***********************************************************/

/* Includes --------------------------------------------------------------- */
#include "include/gx_search_private.h"
#include "module/config/gxconfig.h"
#include "module/frontend/gxfrontend_module.h"
#include "gxavdev.h"
#include "gdi_core.h"


/* Functions --------------------------------------------------------------- */

/*
 * 因为在search服务时，对searchclass的操作实际上是单线程，没有必要封装成接口
 * */
GxSearchClassObj *  gx_search_get_service_paramsptr()
{
    GxSearchClassObj * search_class = &SearchServiceObj.searchclass;;
    return search_class;
}


static uint32_t  check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid)
{
    return 1;
}


status_t gx_search_nit_exsit_check(GxSearchClassObj * search_class)
{
	uint32_t i = 0;
	uint32_t tp_num = 0;
    
	if(search_class->nit.nit_switch == GX_SEARCH_NIT_ENABLE
		&&search_class->nit.search_nit_tp_count !=0)
	{
		if(search_class->search_tp_id != NULL)
		{
			GxCore_Free(search_class->search_tp_id);
			search_class->search_tp_id = NULL;
		}
        tp_num = search_class->search_tp_num = search_class->nit.search_nit_tp_count;
		search_class->search_tp_finish_num = 0;
		search_class->search_tp_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*tp_num);
        if(search_class->search_tp_id == NULL)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]--- search_tp_id malloc failed\n");
#endif
            return GX_SEARCH_ERR;
        }

		for(i=0; i<tp_num; i++)
		{
			search_class->search_tp_id[i] = search_class->nit.search_nit_tp_id[i];
		}
		search_class->nit.search_nit_tp_count =0;
		return GX_SEARCH_OK;
	}
    
	return GX_SEARCH_ERR;
}

status_t gx_search_init_frontend(GxSearchClassObj * search_class)
{
    status_t                ret;
    GxDemuxProperty_ConfigDemux config_demux;

    search_class->search_device_handle = GxAvdev_CreateDevice(0);
    if (search_class->search_device_handle < 0) {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---create device err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
    //frontend 
    search_class->search_demux_handle = 
        GxAvdev_OpenModule(search_class->search_device_handle, GXAV_MOD_DEMUX, search_class->search_demux_id);
    if(search_class->search_demux_handle<0)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---open demux  err!!\n");
#endif
        return GX_SEARCH_ERR;
    }

	search_class->blindclass->search_module_frontend = GxFrontend_IdToHandle(search_class->search_tuner_num_for_prog);
	if (search_class->blindclass->search_module_frontend < 0)
	{
#ifdef GX_SEARCH_DBUG                                                                         
		GX_BUS_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---open module  err!!\n");
#endif
		return GX_SEARCH_ERR;
	}


	//邦定demux和ts
	config_demux.source = search_class->search_ts_cur;
    config_demux.ts_select = FRONTEND;
    config_demux.stream_mode = DEMUX_PARALLEL;
    config_demux.time_gate = 0xf;
    config_demux.byt_cnt_err_gate = 0x03;
    config_demux.sync_loss_gate = 0x03;
    config_demux.sync_lock_gate = 0x03; 
    ret = GxAVSetProperty(search_class->search_device_handle,
            search_class->search_demux_handle, 
            GxDemuxPropertyID_Config, 
            &config_demux, 
            sizeof(GxDemuxProperty_ConfigDemux));
    if(ret < 0)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---config demux  err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
    return GX_SEARCH_OK;
}

status_t gx_search_realease_frontend(GxSearchClassObj * search_class)
{
    status_t                ret = 0;

    //frontend
    if(search_class->search_device_handle != GXCORE_INVALID_POINTER
            &&search_class->search_demux_handle != GXCORE_INVALID_POINTER)
    {
        ret = GxAvdev_CloseModule(search_class->search_device_handle, search_class->search_demux_handle);
        search_class->search_demux_handle = GXCORE_INVALID_POINTER;
        if (ret < 0) 
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF
                ("[SEARCH]---close module err!!search_demux_handle\n");
#endif

        }
        //松绑demux和ts
        //GxAvdev_FreeDemux(search_class.search_demux_id);

        ret = GxAvdev_DestroyDevice(search_class->search_device_handle);
        if (ret < 0) 
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF
                ("[SEARCH]---destroy device err!!\n");
#endif

        }
        search_class->search_device_handle = GXCORE_INVALID_POINTER;
    }

    return GX_SEARCH_OK;
}


void gx_search_sat_tp_reply(uint16_t sat_count,
									uint16_t sat_num,
									uint16_t tp_count,
									uint16_t tp_num,
                                    uint32_t tp_id,
									GxBusPmDataTP* tp,
									GxBusPmDataSat* sat,
                                    GxSearchFrontLockFlag lock_flag,
									GxSearchFrontType front_type)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SatTpReply *params = NULL;
	
	new_msg = GxBus_MessageNew(GXMSG_SEARCH_SAT_TP_REPLY);
	params =
	    (GxMsgProperty_SatTpReply *) GxBus_GetMsgPropertyPtr(new_msg,
								   GxMsgProperty_SatTpReply);
	
	params->sat_max_count = sat_count;
	params->sat_num = sat_num;
	params->tp_max_count = tp_count;
	params->tp_num = tp_num;
	params->tp_id = tp_id;
	params->frequency = tp->frequency;
	switch(front_type)
    {
        case GX_SEARCH_FRONT_ATSC_T:
            params->Atsc_tp_t.bandwidth = tp->tp_t.bandwidth;
            params->lock_flag = lock_flag;
            break;
        case GX_SEARCH_FRONT_ATSC_C:
            params->lock_flag = lock_flag;
            params->Atsc_tp_c.modulation = tp->tp_c.modulation;
            params->Atsc_tp_c.symbol_rate = tp->tp_c.symbol_rate;
            break;
        case GX_SEARCH_FRONT_DVB_S:
            params->lock_flag = lock_flag;
            params->tp_s.polar = tp->tp_s.polar;
            params->tp_s.symbol_rate = tp->tp_s.symbol_rate;
            memcpy(params->sat_name,sat->sat_s.sat_name,MAX_SAT_NAME);
            params->sat_name[MAX_SAT_NAME-1] = 0;
            break;

            /*  
                case GX_SEARCH_FRONT_T:
                params->tp_t.bandwidth = tp->tp_t.bandwidth;
                break;

                case GX_SEARCH_FRONT_C:
                params->tp_c.modulation = tp->tp_c.modulation;
                params->tp_c.symbol_rate = tp->tp_c.symbol_rate;
                break;

                case GX_SEARCH_FRONT_DTMB:
                params->tp_dtmb.modulation = tp->tp_dtmb.modulation;
                params->tp_dtmb.symbol_rate = tp->tp_dtmb.symbol_rate;
                break;

                case GX_SEARCH_FRONT_DVBT2:
                params->tp_dvbt2.bandwidth = tp->tp_dvbt2.bandwidth;
                break;
                */

        default:
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---sat tp reply err!!\n");
#endif
            break;
    }

	GxBus_MessageSend(new_msg);

	return;
}
/*
 *ATSC协议，在退出搜台时，初始化vct_obj
 * */
void gx_release_vct_obj(void)
{
    status_t ret;
    GxAtscProtocolObj * tmp_protocol = (GxAtscProtocolObj *)SearchServiceObj.protocol_obj;
    ProtocolVctObj * vct_obj = &tmp_protocol->vct_obj_info;

    if(vct_obj->vct_time_out != 0)
    {
        vct_obj->vct_time_out = 0;
    }

    if(vct_obj->si_vct != -1)
    {
       ret =  GxSubSystem_SiEngineFree(vct_obj->si_vct);
       if(ret != GX_SI_ENGINE_SUCCESS)
       {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_NORMAL_PRINTF("[search]---si engine free vct err\n");
#endif
       }
       vct_obj->si_vct = -1;
    }

    vct_obj->vct_create = NULL;
    vct_obj->vct_get = NULL;
    vct_obj->vct_realease = NULL;
    vct_obj->vct_time_out_deal = NULL;
}
/*  
 *DVB ATSC协议，在退出搜台时，初始化 pat_obj
 *  */
void gx_release_pat_obj(void)
{
    status_t ret;
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPatObj * pat_obj = NULL;

    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pat_obj = &(((GxAtscProtocolObj *)tmp_protocol)->pat_obj_info);
    }   
    else
    {
        pat_obj = &(((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info);
    }
    


    if(pat_obj->pat_time_out != 0)
    {
        pat_obj->pat_time_out = 0;
    }

    if(pat_obj->si_pat != -1)
    {
        ret = GxSubSystem_SiEngineFree(pat_obj->si_pat);
       if(ret != GX_SI_ENGINE_SUCCESS)
       {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_NORMAL_PRINTF("[search]---si engine free pat err\n");
#endif
       }
       pat_obj->si_pat = -1;
    }
    
    pat_obj->pat_create = NULL;
    pat_obj->pat_get = NULL;
    pat_obj->pat_realease = NULL;
    pat_obj->pat_time_out_deal = NULL;
}

/*  
 *DVB ATSC协议，搜台退出时初始化pmt_obj
 *  */
void gx_release_pmt_obj(void)
{
    uint32_t i;
    status_t ret;
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPmtObj * pmt_obj = NULL;
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmt_obj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
    }
    else
    {
        pmt_obj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
    }


    for(i = 0;i<pmt_obj->pmt_count;i++)
    {
        if(pmt_obj->pmt_info[i].si_pmt != -1)
        {
            ret = GxSubSystem_SiEngineFree(pmt_obj->pmt_info[i].si_pmt);
            if(ret != GX_SI_ENGINE_SUCCESS)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_NORMAL_PRINTF("[search]---si engine free pmt err\n");
#endif
            }
        }
        pmt_obj->pmt_info[i].si_pmt = -1;
        pmt_obj->pmt_info[i].pid = 0;
        pmt_obj->pmt_info[i].service_id = 0;
    }

    GxCore_Free(pmt_obj->pmt_info);
    pmt_obj->pmt_info = NULL;
    pmt_obj->pmt_start_count = 0;
    pmt_obj->pmt_finish_count = 0;
    pmt_obj->pmt_count= 0;
    pmt_obj->pmt_time_out = 0;

    pmt_obj->pmt_create = NULL;
    pmt_obj->pmt_get = NULL;
    pmt_obj->pmt_realease = NULL;
    pmt_obj->pmt_time_out_deal = NULL;
}

void gx_release_sdt_obj()
{
    void * tmp_searchclass = SearchServiceObj.protocol_obj;
    ProtocolSdtObj * sdtobj = NULL;
    status_t ret = 0;

    sdtobj = &((GxDvbProtocolObj *)tmp_searchclass)->sdt_obj_info;


    if(sdtobj->sdt_time_out != 0)
    {
        sdtobj->sdt_time_out  = 0;
    }

    if(sdtobj->si_sdt != -1)
    {
        ret =  GxSubSystem_SiEngineFree(sdtobj->si_sdt);
        if(ret != GX_SI_ENGINE_SUCCESS)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[search]---si engine free vct err\n");
#endif
        }
        sdtobj->si_sdt = -1;
    }

    sdtobj->sdt_create = NULL;
    sdtobj->sdt_get = NULL;
    sdtobj->sdt_release = NULL;
    sdtobj->sdt_time_out_deal = NULL;
}

void DVB_protocol_obj_realease()
{
    if(SearchServiceObj.protocol_obj == NULL)
    {
        return;
    }

    gx_release_sdt_obj();
    gx_release_pat_obj();
    gx_release_pmt_obj();
    
    GxCore_Free(SearchServiceObj.protocol_obj);
    SearchServiceObj.protocol_obj = NULL;
}

void ATSC_protocol_obj_realease()
{
   if(SearchServiceObj.protocol_obj == NULL) 
   {
       return;
   }

    
    gx_release_vct_obj();

    gx_release_pat_obj();

    gx_release_pmt_obj();

    //只有此处一个地方对protocol_obj释放操作，所以不用进行非NULL保护
    GxCore_Free(SearchServiceObj.protocol_obj);

    SearchServiceObj.protocol_obj = NULL;
}

void searchclass_obj_realease(GxSearchClassObj * tmp_searchclass)
{
    status_t ret = 0;

    tmp_searchclass->front_type = GX_SEARCH_FRONT_ATSC_T;
    tmp_searchclass->scan_type = GX_SEARCH_AUTO; 
    tmp_searchclass->prog_type = GX_SEARCH_TV;
    tmp_searchclass->fta_cas = GX_SEARCH_FTA;

    tmp_searchclass->search_sat_num = 0;
    tmp_searchclass->search_sat_finish_num = 0;
    tmp_searchclass->search_sat_id_for_prog = 0;
    tmp_searchclass->search_ts_cur = 0;
    tmp_searchclass->pat_version = 0;
    memset(&tmp_searchclass->search_new_prog_get,0,sizeof(GxMsgProperty_NewProgGet));
    if(tmp_searchclass->search_sat_id != NULL)
    {
        GxCore_Free(tmp_searchclass->search_sat_id);
        tmp_searchclass->search_sat_id = NULL;
    }

    if(tmp_searchclass->search_ts != NULL)
    {
        GxCore_Free(tmp_searchclass->search_ts);
        tmp_searchclass->search_ts = NULL;
    }

    tmp_searchclass->search_tp_num = 0;
    tmp_searchclass->search_tp_finish_num = 0;
    tmp_searchclass->search_tp_id_for_prog = 0;
    tmp_searchclass->search_tuner_num_for_prog = 0;
    if(tmp_searchclass->search_tp_id != NULL)
    {
        GxCore_Free(tmp_searchclass->search_tp_id);
        tmp_searchclass->search_tp_id = NULL;
    }

    tmp_searchclass->search_prog_id_num = 0;
    if(tmp_searchclass->search_prog_id_arry != NULL)
    {
        GxCore_Free(tmp_searchclass->search_prog_id_arry);
        tmp_searchclass->search_prog_id_arry = NULL;
    }
    
    tmp_searchclass->ext.ext_tp_num = 0;
    if(tmp_searchclass->ext.ext_tp_id != NULL)
    {
        GxCore_Free(tmp_searchclass->ext.ext_tp_id);
        tmp_searchclass->ext.ext_tp_id = NULL;
    }

    if(tmp_searchclass->nit.nit_switch == GX_SEARCH_NIT_ENABLE)
    {
        tmp_searchclass->nit.nit_switch = GX_SEARCH_NIT_DISABLE;
        tmp_searchclass->nit.search_nit_tp_count = 0;
        if(tmp_searchclass->nit.search_nit_tp_id != NULL)
        {
            GxCore_Free(tmp_searchclass->nit.search_nit_tp_id);
            tmp_searchclass->nit.search_nit_tp_id = NULL;
        }
        if(tmp_searchclass->nit.nit_obj != NULL)
        {
            if(tmp_searchclass->nit.nit_obj->si_nit != -1)
            {
                ret =  GxSubSystem_SiEngineFree(tmp_searchclass->nit.nit_obj->si_nit);
                if(ret != GX_SI_ENGINE_SUCCESS)
                {
#ifdef GX_SEARCH_DEBUG
                    GX_BUS_SEARCH_NORMAL_PRINTF("[search]---si engine free nit err\n");
#endif
                }
            }
            tmp_searchclass->nit.nit_obj->si_nit = -1;
            GxCore_Free(tmp_searchclass->nit.nit_obj);
            tmp_searchclass->nit.nit_obj = NULL;
        }
        tmp_searchclass->nit.nit_flg = 0;
        tmp_searchclass->nit.nit_tp_private_check = NULL;
    }

    tmp_searchclass->search_pid_params.video_pid = 0;
    tmp_searchclass->search_pid_params.audio_pid = 0;
    tmp_searchclass->search_pid_params.pcr_pid = 0;

    tmp_searchclass->tp_lock_func = NULL;
    tmp_searchclass->init_params_func = NULL;
    tmp_searchclass->modify_prog = NULL;
	tmp_searchclass->check_pmtpid_fun = NULL;
    tmp_searchclass->get_sdt_info_func = NULL;

    tmp_searchclass->search_demux_id = -1;
    tmp_searchclass->search_status = SEARCH_STOP;
}


/*退出search时清除变量 */
void gx_search_service_variable_exit_realease(GxSearchClassObj * searchclass)
{
    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    switch(SearchServiceObj.type)
    {
        case PROTOCOL_ATSC:
            ATSC_protocol_obj_realease();
            break;
        case PROTOCOL_DVB:
            DVB_protocol_obj_realease();
            break;
        default:
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("unknow protocol\n");
#endif
            break;
    }

    searchclass_obj_realease(searchclass);
    clear_search_list();

    switch(SearchServiceObj.type)
    {
        case PROTOCOL_ATSC:
            gx_search_realease_frontend(searchclass);
            break;
        case PROTOCOL_DVB:
            gx_search_realease_frontend(searchclass);
            break;
        default:
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("unknow protocol\n");
#endif
            break;
    }
    SearchServiceObj.initflag = 0;
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return ;
}

void gx_search_error_reply(GxMsgStatusReplyCode err) 
{
    GxMessage           * new_msg = NULL;
    GxMsgProperty_StatusReply   * params = NULL;

    new_msg = GxBus_MessageNew(GXMSG_SEARCH_STATUS_REPLY);
    params = 
        (GxMsgProperty_StatusReply *) GxBus_GetMsgPropertyPtr(new_msg,
                                       GxMsgProperty_StatusReply);

    memcpy(params, &(err), sizeof(GxMsgProperty_StatusReply));

    GxBus_MessageSend(new_msg);
    return ;
}


/* 
 *初始化用户在发送开始消息时携带的参数，避免在init_params中重复出现相同代码
 *目标是在init_params中只对各个前端有区别的参数进行初始化
 * */
void searchclass_public_init(GxSearchClassObj * search_class,void * params)
{
    GxMsgProperty_ScanStart * other_params = NULL;
    GxMsgProperty_ScanStart * s_params = NULL; 

    s_params = other_params  = (GxMsgProperty_ScanStart *)params;
    search_class->search_pid_params.audio_pid = other_params->ServiceParams.params_pid.audio_pid;
    search_class->search_pid_params.video_pid= other_params->ServiceParams.params_pid.video_pid;
    search_class->search_pid_params.pcr_pid= other_params->ServiceParams.params_pid.pcr_pid;
    search_class->transcodingflag = other_params->ServiceParams.transcodingflag;
    search_class->modify_prog  = other_params->ServiceParams.modify_prog;
	search_class->check_pmtpid_fun = other_params->ServiceParams.check_pmtpid_fun;
    search_class->get_sdt_info_func  = other_params->ServiceParams.get_sdt_info_func;
	if(other_params->ServiceParams.check_ca_fun)
	{
		search_class->check_ca_fun  = other_params->ServiceParams.check_ca_fun;
	}
	else
	{
		search_class->check_ca_fun = check_ca_system;
	}
    search_class->pmt_original_parse_fun  = other_params->ServiceParams.pmt_original_parse_fun;
	search_class->sdt_original_parse_fun = other_params->ServiceParams.sdt_original_parse_fun;
    search_class->scan_type = other_params->ServiceParams.scan_type; 
    search_class->prog_type = other_params->ServiceParams.tv_radio; 
    search_class->search_demux_id = other_params->ServiceParams.demux_id; 
    search_class->fta_cas = other_params->ServiceParams.fta_cas; 

    switch(search_class->front_type)
    {
        case GX_SEARCH_FRONT_ATSC_T:
        case GX_SEARCH_FRONT_ATSC_C:
        case GX_SEARCH_FRONT_DVB_T:
        case GX_SEARCH_FRONT_DVB_C:
        case GX_SEARCH_FRONT_DVB_DTMB:
        case GX_SEARCH_FRONT_DVB_DVBT2:
            search_class->search_diseqc = NULL;
            break;
        case GX_SEARCH_FRONT_DVB_S:
            search_class->search_diseqc = s_params->FrontendParams.s_info.search_diseqc; 
            break;
        default :
            break;
    }

    memset(&search_class->search_new_prog_get,0,sizeof(GxMsgProperty_NewProgGet));
    search_class->pat_version = 0;
    search_class->search_status = SEARCH_START;
    search_class->search_device_handle = GXCORE_INVALID_POINTER;
    search_class->search_demux_handle = GXCORE_INVALID_POINTER;
    SearchServiceObj.initflag = 1;
}

void gx_init_pat_obj(uint16_t pat_time_out)
{
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPatObj * patobj = NULL;

    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        patobj = &((GxAtscProtocolObj *)tmp_protocol)->pat_obj_info;
    }
    else if(SearchServiceObj.type == PROTOCOL_DVB)
    {
        patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;
    }

    if(pat_time_out != 0)
    {
        patobj->pat_time_out = pat_time_out;
    }
    else
    {
        patobj->pat_time_out = SEARCH_PAT_TIMEOUT_VALUE;
    }
    patobj->si_pat = -1;
    patobj->pat_create = Protocol_pat_create;
    patobj->pat_get = Protocol_pat_get;
    patobj->pat_realease = Protocol_pat_realease;
    patobj->pat_time_out_deal = Protocol_pat_time_out_deal;
}

void gx_init_vct_obj(uint16_t vct_time_out)
{
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolVctObj * vctobj = NULL;

    vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;

    if(vct_time_out != 0)
    {
        vctobj->vct_time_out = vct_time_out;
    }
    else
    {
        vctobj->vct_time_out = SEARCH_VCT_TIMEOUT_VALUE;
    }
    vctobj->si_vct = -1;
    vctobj->vct_create = Protocol_vct_create;
    vctobj->vct_get = Protocol_vct_get;
    vctobj->vct_realease = Protocol_vct_realease;
    vctobj->vct_time_out_deal = Protocol_vct_timeout_deal;
}


void gx_init_pmt_obj(uint16_t pmt_time_out)
{
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPmtObj * pmtobj = NULL;
    
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
    }
    else if(SearchServiceObj.type == PROTOCOL_DVB)
    {
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
    }

    if(pmt_time_out != 0)
    {
        pmtobj->pmt_time_out = pmt_time_out;
    }
    else
    {
        pmtobj->pmt_time_out = SEARCH_PMT_TIMEOUT_VALUE;
    }
    
    if(pmtobj->pmt_info != NULL)
    {
        GxCore_Free(pmtobj->pmt_info);
        pmtobj->pmt_info = NULL;
    }
    pmtobj->pmt_start_count = 0;
    pmtobj->pmt_finish_count = 0;
    pmtobj->pmt_count = 0;
    pmtobj->pmt_create = Protocol_pmt_create;
    pmtobj->pmt_get = Protocol_pmt_get;
    pmtobj->pmt_realease = Protocol_pmt_realease;
    pmtobj->pmt_time_out_deal = Protocol_pmt_timeout_deal;
}

void gx_init_sdt_obj(uint16_t sdt_time_out)
{
    void  * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolSdtObj * sdtobj = NULL;

    sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
    if(sdt_time_out != 0)
    {
        sdtobj->sdt_time_out = sdt_time_out;
    }
    else
    {
        sdtobj->sdt_time_out = SEARCH_SDT_TIMEOUT_VALUE;
    }

    sdtobj->si_sdt = -1;
    sdtobj->sdt_create = Protocol_sdt_create;
    sdtobj->sdt_get = Protocol_sdt_get;
    sdtobj->sdt_release = Protocol_sdt_realease;
    sdtobj->sdt_time_out_deal = Protocol_sdt_time_out_deal;
}


int32_t gx_atsc_protocol_init(void * search_params)
{
    GxSearchTimeOut * timeout = NULL;

    if(SearchServiceObj.protocol_obj == NULL)
    {
        SearchServiceObj.protocol_obj = GxCore_Mallocz(sizeof(GxAtscProtocolObj));
        if(SearchServiceObj.protocol_obj == NULL)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ----protocol obj malloc failed\n");
#endif
            goto err;
        }
    }
    timeout = &(((GxMsgProperty_ScanStart *)search_params)->ServiceParams.timeout);
    gx_init_pat_obj(timeout->pat);
    gx_init_pmt_obj(timeout->pmt);
    gx_init_vct_obj(timeout->vct);
    return 0;
err:
    return -1;
}

int32_t gx_dvb_protocol_init(void * search_params)
{
    GxSearchTimeOut * timeout = NULL;

    if(SearchServiceObj.protocol_obj == NULL)
    {
        SearchServiceObj.protocol_obj = GxCore_Mallocz(sizeof(GxDvbProtocolObj));
        if(SearchServiceObj.protocol_obj == NULL)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ----protocol obj malloc failed\n");
#endif
            goto err;
        }
    }
    timeout = &(((GxMsgProperty_ScanStart *)search_params)->ServiceParams.timeout);
    gx_init_pat_obj(timeout->pat);
    gx_init_pmt_obj(timeout->pmt);
    gx_init_sdt_obj(timeout->sdt);
    return 0;
err:
    return -1;
}

void gx_search_variable_next_tp_init()
{
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPmtObj * pmtobj = NULL;
    ProtocolPatObj * patobj = NULL;
    ProtocolSdtObj * sdtobj = NULL;
    ProtocolVctObj * vctobj = NULL;
    uint32_t pat_time_out = 0,pmt_time_out = 0,sdt_time_out = 0,vct_time_out = 0;
    

    clear_search_list();
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        patobj = &((GxAtscProtocolObj *)tmp_protocol)->pat_obj_info;
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
        pat_time_out = patobj->pat_time_out;
        pmt_time_out = pmtobj->pmt_time_out;
        vct_time_out = vctobj->vct_time_out;
        gx_release_pat_obj();
        gx_release_pmt_obj();
        gx_release_vct_obj();
        gx_init_pat_obj(pat_time_out);
        gx_init_vct_obj(vct_time_out);
        gx_init_pmt_obj(pmt_time_out);
    }
    else if(SearchServiceObj.type == PROTOCOL_DVB)
    {
        patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
        sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
        pat_time_out = patobj->pat_time_out;
        pmt_time_out = pmtobj->pmt_time_out;
        sdt_time_out = sdtobj->sdt_time_out; 
        gx_release_pat_obj();
        gx_release_pmt_obj();
        gx_release_sdt_obj();
       
        gx_init_pat_obj(pat_time_out);
        gx_init_pmt_obj(pmt_time_out);
        gx_init_sdt_obj(sdt_time_out);
    }
}


status_t gx_dvb_search_sat_init_params(void * p,void *search_params)
{
    uint32_t                exist_tp_count = 0;
    GxMsgProperty_ScanStart * params = NULL;
    uint32_t sat_id = 0;
    uint32_t tp_num = 0;
    uint32_t i = 0;
    GxBusPmDataTP * tp_arry = NULL;
    GxBusPmDataTP           tp = { 0 };
    GxBusPmDataSat          sat = { 0 };
    ProtocolNitObj * nitobj = NULL;
    GxSearchClassObj * search_class = (GxSearchClassObj *)p;
    params = (GxMsgProperty_ScanStart *)search_params;

    if(params->FrontendParams.s_info.info.max_num == 0
            ||params->FrontendParams.s_info.info.array == NULL)
    {
#ifdef GX_BUS_SEARCH_DBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---search params err!!\n");
#endif
        goto err;
    }


    search_class->search_sat_num = 1;
    search_class->search_sat_finish_num = 0;


#if 0
    /*有多少个pat body就意味着有多少个总的sdt body,所以空间应该开总的
      body,但是pat和sdt是同时解析的,所以无法通过pat body判断,直接开一个300 */

    search_class->search_sdt_body = (GxSearchSdtBody *)
        GxCore_Malloc(sizeof(GxSearchSdtBody) * MAX_PROG_PER_TP);	//每个tp最多300节目
#endif
    /*对于搜到的节目由于不定长 所以直接开最大节目数量 用完后释放 该地方会耗费几百k内存*/
    search_class->search_prog_id_arry = (uint32_t*)GxCore_Mallocz(sizeof(uint32_t) * MAX_PROG_COUNT);
    if(search_class->search_prog_id_arry == NULL)
    {
#ifdef GX_BUS_SEARCH_DBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc search_prog_arry space err!!\n");
#endif
        goto err;
    }
    search_class->search_prog_id_num = 0;

    /*开辟nit的tp空间,由于不确定具体数量,该空间为所能保存的tp最大数量减去
      已经存在的tp的数量 */
    search_class->nit.nit_switch = params->ServiceParams.nit_switch;
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

    switch (search_class->scan_type) {
        case GX_SEARCH_AUTO:
            search_class->search_sat_num = params->FrontendParams.s_info.info.max_num;
            search_class->search_sat_id =
                (uint32_t *) GxCore_Malloc(sizeof(uint32_t) * search_class->search_sat_num);
            if (search_class->search_sat_id == NULL) 
            {
#ifdef GX_BUS_SEARCH_DBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc sat id space err!!\n");
#endif
                goto err;
            }
            memcpy(search_class->search_sat_id, params->FrontendParams.s_info.info.array, 
                    sizeof(uint32_t) * search_class->search_sat_num);	//记录所要搜索的sat的id

            if(search_class->search_sat_num != 0 && search_class->search_sat_finish_num < search_class->search_sat_num)
            {
                sat_id = search_class->search_sat_id[search_class->search_sat_finish_num];
                tp_num = GxBus_PmTpNumGetBySat(sat_id);
                search_class->search_tp_num = tp_num;
                search_class->search_tp_finish_num = 0;
                search_class->search_tp_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*tp_num);
                tp_arry = (GxBusPmDataTP*)GxCore_Malloc(sizeof(GxBusPmDataTP)*tp_num);
                if(search_class->search_tp_id == NULL || tp_arry == NULL)
                {
#ifdef GX_BUS_SEARCH_DBUG
                    GX_BUS_SEARCH_ERRO_PRINTF
                        ("[SEARCH]---malloc sat id space err!!\n");
#endif
                    goto err;
                }
                GxBus_PmTpGetByPosInSat(0, tp_num, tp_arry);
                for(i = 0; i<tp_num; i++)
                {
                    search_class->search_tp_id[i] = tp_arry[i].id;
                }
                GxCore_Free(tp_arry);
            }
            search_class->search_sat_finish_num = 1;

            if(params->FrontendParams.s_info.info.ts == NULL)
            {
#ifdef GX_BUS_SEARCH_DBUG                                          
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---ts num err!!\n");
#endif                                                             
                goto err;
            }
            search_class->search_ts = (uint32_t *) GxCore_Malloc(sizeof(uint32_t)*search_class->search_sat_num);
            if(search_class->search_ts == NULL)
            {
#ifdef GX_BUS_SEARCH_DBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc ts num space err!!\n");
#endif
                goto err;
            }
            memcpy(search_class->search_ts, params->FrontendParams.s_info.info.ts, 
                    sizeof(uint32_t) * search_class->search_sat_num);//记录所要搜索的sat的ts
            search_class->search_ts_cur = search_class->search_ts[0];

            GxBus_PmSatGetById(sat_id, &sat);
            search_class->search_tuner_num_for_prog = sat.tuner;
            break;

        case GX_SEARCH_MANUAL:
#ifdef GX_BUS_SEARCH_DBUG
            GX_BUS_SEARCH_NORMAL_PRINTF
                ("[SEARCH]---start sat manual scan!!\n");
#endif
            search_class->search_tp_num = params->FrontendParams.s_info.info.max_num;	//记录搜索的tp的数量


            /*开辟传下来的tp的空间 */
            search_class->search_tp_id =
                (uint32_t *) GxCore_Malloc(sizeof(uint32_t) * search_class->search_tp_num);

            if (search_class->search_tp_id == NULL) {
#ifdef GX_BUS_SEARCH_DBUG
                GX_BUS_SEARCH_ERRO_PRINTF
                    ("[SEARCH]---malloc tp id space err!!\n");
#endif
                goto err;
            }

            memcpy(search_class->search_tp_id, params->FrontendParams.s_info.info.array, 
                    sizeof(uint32_t) * search_class->search_tp_num);	//记录所要搜索的tp的id

            if(params->FrontendParams.s_info.info.ts == NULL)
            {
#ifdef GX_BUS_SEARCH_DBUG                                          
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---ts num err!!\n");
#endif                                                             
                goto err;
            }
            search_class->search_ts = (uint32_t *) GxCore_Malloc(sizeof(uint32_t));
            if(search_class->search_ts == NULL)
            {
#ifdef GX_BUS_SEARCH_DBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc ts num space err!!\n");
#endif
                goto err;
            }
            *(search_class->search_ts) = *(params->FrontendParams.s_info.info.ts);
            search_class->search_ts_cur = *(search_class->search_ts);

            GxBus_PmTpGetById(search_class->search_tp_id[0], &tp);
            GxBus_PmSatGetById(tp.sat_id, &sat);
            search_class->search_tuner_num_for_prog = sat.tuner;

            search_class->search_sat_num = 1;
            search_class->search_sat_finish_num = 1;

            break;

        case GX_SEARCH_PID:
#ifdef GX_BUS_SEARCH_DBUG
            GX_BUS_SEARCH_NORMAL_PRINTF
                ("[SEARCH]---start sat pid scan!!\n");
#endif
            search_class->search_tp_num = params->FrontendParams.s_info.info.max_num;	//记录搜索的tp的数量
            search_class->nit.nit_switch = GX_SEARCH_NIT_DISABLE;

            /*开辟传下来的tp的空间 */
            search_class->search_tp_id =
                (uint32_t *) GxCore_Malloc(sizeof(uint32_t) * search_class->search_tp_num);

            if (search_class->search_tp_id == NULL) {
#ifdef GX_BUS_SEARCH_DBUG
                GX_BUS_SEARCH_ERRO_PRINTF
                    ("[SEARCH]---malloc tp id space err!!\n");
#endif
                goto err;
            }

            memcpy(search_class->search_tp_id, params->FrontendParams.s_info.info.array, 
                    sizeof(uint32_t) * search_class->search_tp_num);	//记录所要搜索的tp的id
            /*记录所要搜索的pid*/
            search_class->search_pid_params.audio_pid = params->ServiceParams.params_pid.audio_pid;
            search_class->search_pid_params.video_pid= params->ServiceParams.params_pid.video_pid;
            search_class->search_pid_params.pcr_pid= params->ServiceParams.params_pid.pcr_pid;

            if(params->FrontendParams.s_info.info.ts == NULL)
            {
#ifdef GX_BUS_SEARCH_DBUG                                          
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---ts num err!!\n");
#endif      
                goto err;
            }
            search_class->search_ts = (uint32_t *) GxCore_Malloc(sizeof(uint32_t));
            if(search_class->search_ts == NULL)
            {
#ifdef GX_BUS_SEARCH_DBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc ts num space err!!\n");
#endif
                goto err;
            }
            *(search_class->search_ts) = *(params->FrontendParams.s_info.info.ts);
            search_class->search_ts_cur = *(search_class->search_ts);

            GxBus_PmTpGetById(search_class->search_tp_id[0], &tp);
            GxBus_PmSatGetById(tp.sat_id, &sat);
            search_class->search_tuner_num_for_prog = sat.tuner;
            search_class->search_sat_num = 1;
            search_class->search_sat_finish_num = 1;

            break;

        default:
            return GX_SEARCH_ERR;

            break;
    }

    //gx_search_init_frontend();
    return GX_SEARCH_OK;
err:
    return GX_SEARCH_ERR;
}

status_t gx_search_other_init_params(void * p,void * search_params)
{
    GxMsgProperty_ScanStart * params = NULL;
    GxBusPmDataSat sat = {0};
    uint32_t                exist_tp_count = 0;
    ProtocolNitObj * nitobj = NULL;
    GxSearchClassObj * search_class = (GxSearchClassObj *)p;

    params = (GxMsgProperty_ScanStart * )search_params;
    SearchServiceObj.type = PROTOCOL_ATSC;

    if(params->FrontendParams.other_info.info.max_num == 0
            ||params->FrontendParams.other_info.info.array == NULL)
    {
#ifdef GX_BUS_SEARCH_DBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---search params err!!\n");
#endif
        goto err;

    }


    search_class->search_sat_num = 1;
    search_class->search_sat_finish_num = 1;

    search_class->nit.nit_switch = params->ServiceParams.nit_switch;
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

    /* 对于搜到的节目由于不定长 所以直接开最大节目数量 用完后释放 该地方会耗费几百k内存*/
    search_class->search_prog_id_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * MAX_PROG_COUNT);
    if(search_class->search_prog_id_arry == NULL)
    {
#ifdef ATSC_SEARCH_DBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc search_prog_arry space err!!\n");
#endif
        goto err;
    }
    search_class->search_prog_id_num = 0;


    switch(search_class->scan_type)
    {
        case GX_SEARCH_AUTO:
        case GX_SEARCH_MANUAL:
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---start t auto or manual scan!!\n");
#endif
            search_class->search_tp_num = params->FrontendParams.other_info.info.max_num;
            search_class->search_sat_id_for_prog = params->FrontendParams.other_info.sat_id;

            if(search_class->search_tp_id != NULL)
            {
                GxCore_Free(search_class->search_tp_id);
                search_class->search_tp_id = NULL;
            }
            search_class->search_tp_id = 
                (uint32_t *)GxCore_Mallocz(sizeof(uint32_t) * search_class->search_tp_num);
            if(search_class->search_tp_id == NULL)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc tp id space err!!\n");
#endif
                goto err;
            }
            memcpy(search_class->search_tp_id,params->FrontendParams.other_info.info.array,
                    sizeof(uint32_t)*search_class->search_tp_num);

            if(params->FrontendParams.other_info.info.ts == NULL)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---ts num err!!\n");
#endif
                goto err;
            }
            if(search_class->search_ts != NULL)
            {
                GxCore_Free(search_class->search_ts);
                search_class->search_ts = NULL;
            }
            search_class->search_ts = (uint32_t *)GxCore_Mallocz(sizeof(uint32_t));
            if(search_class->search_ts == NULL)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---malloc ts num space err!!\n");
#endif
                goto err;
            }
            *(search_class->search_ts) = *(params->FrontendParams.other_info.info.ts);
            search_class->search_ts_cur = *(search_class->search_ts);

            GxBus_PmSatGetById(search_class->search_sat_id_for_prog,&sat);
            search_class->search_tuner_num_for_prog = sat.tuner;
            break;
        default:
            return GX_SEARCH_ERR;
            break;
    }

    //gx_search_init_frontend();
    return GX_SEARCH_OK;
err:
    return GX_SEARCH_ERR;
}

static status_t gx_search_init(GxFrontendStatus status,
		void * params,GxSearchClassObj *  searchclass)
{
    int32_t ret = 0;
    //初始化searchclass公共部分
    searchclass_public_init(searchclass,params);
    
    switch(SearchServiceObj.type)
    {
        case PROTOCOL_ATSC:
            ret = gx_atsc_protocol_init(params);
            if(ret < 0)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---atsc protocol init err\n");
#endif
                goto err;
            }
            break;
        case PROTOCOL_DVB:
            ret = gx_dvb_protocol_init(params);
            if(ret < 0)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---dvb protocol init err\n");
#endif
            }
            break;
        default:
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("unknow protocol\n");
#endif
            break;
    }
	
    //初始化各个前端私有searchclass
    switch(searchclass->front_type)
    {
        case GX_SEARCH_FRONT_ATSC_T:
            searchclass->tp_lock_func = gx_atsc_search_t_tp_lock;
            searchclass->init_params_func = gx_search_other_init_params;
            break;
        case GX_SEARCH_FRONT_ATSC_C:
            searchclass->tp_lock_func = gx_atsc_search_c_tp_lock;
            searchclass->init_params_func = gx_search_other_init_params;
            break;
        case GX_SEARCH_FRONT_DVB_S:
			if(status == NORMAL)
			{
				searchclass->tp_lock_func = gx_dvb_search_sat_tp_lock;
				searchclass->init_params_func = gx_dvb_search_sat_init_params;
			}
			else
			{
				searchclass->tp_lock_func = gx_dvb_blind_search_tp_lock;
				searchclass->init_params_func = gx_dvb_blind_scan_init_params;
			}
            break;
        case GX_SEARCH_FRONT_DVB_T:
            searchclass->tp_lock_func = gx_dvb_search_t_tp_lock;
            searchclass->init_params_func = gx_search_other_init_params;
            break;
        case GX_SEARCH_FRONT_DVB_DTMB:
            searchclass->tp_lock_func = gx_dvb_search_dtmb_tp_lock;
            searchclass->init_params_func = gx_search_other_init_params;
            break;
        case GX_SEARCH_FRONT_DVB_DVBT2:
            searchclass->tp_lock_func = gx_dvb_search_dvbt2_tp_lock;
            searchclass->init_params_func = gx_search_other_init_params;
            break;
        default:
            break;
    }

    return GX_SEARCH_OK;
err:
    return GX_SEARCH_ERR;
}

void gx_search_stop_reply(GxSearchClassObj * tmp_searchclass)
{
    GxMessage * new_msg = NULL;
    GxBus_MessageEmpty(tmp_searchclass->SearchServiceMsgHandle);
    tmp_searchclass->search_status = SEARCH_STOP;
    new_msg = GxBus_MessageNew(GXMSG_SEARCH_STOP_OK);
    GxBus_MessageSend(new_msg);
    return;
}







status_t gx_search_scan_init_start(GxMessage * Msg,GxSearchClassObj * searchclass)
{
    GxMsgProperty_ScanStart* search_params = NULL;
    search_params = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_ScanStart);
    status_t ret = 0;

    switch(Msg->msg_id)
    {
        case GXMSG_SEARCH_ATSC_T_SCAN_START:
            SearchServiceObj.type = PROTOCOL_ATSC;
			searchclass->front_type = GX_SEARCH_FRONT_ATSC_T;
            ret = gx_search_init(NORMAL,search_params,searchclass);
            if(ret != GX_SEARCH_OK)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---init protocol err\n");
#endif
                goto err;
            }
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---start t scan!!\n");
#endif
            break;
        case GXMSG_SEARCH_ATSC_C_SCAN_START:
            SearchServiceObj.type = PROTOCOL_ATSC;
			searchclass->front_type = GX_SEARCH_FRONT_ATSC_C;
            ret = gx_search_init(NORMAL,search_params,searchclass);
            if(ret != GX_SEARCH_OK)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---start c scan!!\n");
#endif
                goto err; 
            }
            break;
        case GXMSG_SEARCH_DVB_SAT_SCAN_START:
            SearchServiceObj.type = PROTOCOL_DVB;
			searchclass->front_type = GX_SEARCH_FRONT_DVB_S;
            ret = gx_search_init(NORMAL,search_params,searchclass);
            if(ret != GX_SEARCH_OK)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---start s scan!!\n");
#endif
                goto err; 
            }
            break;
		case GXMSG_SEARCH_BLIND_SCAN_START:
			SearchServiceObj.type = PROTOCOL_DVB;
			searchclass->front_type = GX_SEARCH_FRONT_DVB_S;
			ret = gx_search_init(BLIND,search_params,searchclass);
            if(ret != GX_SEARCH_OK)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---start s scan!!\n");
#endif
                goto err; 
            }
            break;
        default:
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---start params err!!\n");
#endif
            return GX_SEARCH_ERR;
            break;
    }
    
    
    ret = searchclass->init_params_func((void *)searchclass,(void *)search_params);
    if(ret != GX_SEARCH_OK)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---init searchclass err\n");
#endif
        goto err;
    }
    gx_search_init_frontend(searchclass);
    return GX_SEARCH_OK;
err:
    return GX_SEARCH_ERR;
}



status_t gx_search_pmt_cas_get(GxSubsystemSiParseCaDes * cas1,
			       uint32_t cas1_count,
			       GxSubsystemSiParseCaDes * cas2,
			       uint32_t cas2_count, uint32_t stream_type,uint16_t video_pid,uint32_t flag,
                   AtscSearchNode_info_t * prog,GxBusPmDataStream *search_stream_body,
                   uint32_t search_stream_body_count,GxSearchClassObj * tmp_searchclass)
{
    uint32_t i = 0;
    GxSubsystemSiParseCaDes * cas_decriptor = NULL;

    switch (stream_type) {
        /*	case MPEG_1_VIDEO:
            case MPEG_2_VIDEO:
            case AVS:
            case H264:*/
        case 0://视频
            if(flag == 0)
            {
                prog->pmt_info.scramble_flag = GXBUS_PM_PROG_BOOL_DISABLE;
                prog->pmt_info.cas_id = CAS_ID_FREE;
                break;
            }
            if (cas2 != NULL)	//以分开加密优先
            {
                cas_decriptor = &cas2[0];
                prog->pmt_info.ecm_pid_v = cas_decriptor->ca_pid;
                prog->pmt_info.cas_id = CAS_ID_UNKNOW;
                for (i = 0; i < cas2_count; i++) {
                    cas_decriptor = &cas2[i];
                    if (cas_decriptor->ca_pid == video_pid){
                        if(tmp_searchclass->check_ca_fun(cas_decriptor->ca_system_id,video_pid,cas_decriptor->ca_pid))
                        {
                            //search_class.search_pmt_body.ecm_pid_video =
                               prog->pmt_info.ecm_pid_v = cas_decriptor->ca_pid;
                            //search_class.search_pmt_body.pmt_ca_desc.cas_id =
                            prog->pmt_info.cas_id=cas_decriptor->ca_system_id;
                            break;
                        }
                    }

                }
            }
            if (i == cas2_count && cas1 != NULL)	//分开加密没找到,找相同加密
            {
                cas_decriptor = &cas1[0];
                prog->pmt_info.ecm_pid_v = cas_decriptor->ca_pid;
                prog->pmt_info.cas_id = CAS_ID_UNKNOW;
                for (i = 0; i < cas1_count; i++) {
                    cas_decriptor = &cas1[i];
                    //if (cas_decriptor->elem_pid ==
                    //  search_class.search_pmt_body.video_pid) 
                    {
                        if(tmp_searchclass->check_ca_fun(cas_decriptor->ca_system_id,video_pid,cas_decriptor->ca_pid))
                        {
                            prog->pmt_info.ecm_pid_v =
                                cas_decriptor->ca_pid;
                            prog->pmt_info.cas_id =
                                cas_decriptor->ca_system_id;
                            break;
                        }
                    }

                }
            }
            break;

            /*	case MPEG_1_AUDIO:
                case MPEG_2_AUDIO:
                case AAC_ADTS:
                case AAC_LATM:*/
        case 1://音频
            if(flag == 0)
            {
                prog->pmt_info.scramble_flag = GXBUS_PM_PROG_BOOL_DISABLE;
                prog->pmt_info.cas_id = CAS_ID_FREE;
                break;
            }
            if (cas2 != NULL)	//以分开加密优先
            {
                cas_decriptor = &cas2[0];
                search_stream_body[search_stream_body_count].ecm_pid =
                    cas_decriptor->ca_pid;

                for (i = 0; i < cas2_count; i++) {
                    cas_decriptor = &cas2[i];
                    if (cas_decriptor->ca_pid ==
                            search_stream_body[search_stream_body_count].audio_pid) 
                    {
                        if(tmp_searchclass->check_ca_fun(cas_decriptor->ca_system_id,
                                    search_stream_body[search_stream_body_count].audio_pid,
                                    cas_decriptor->ca_pid))
                        {
                            search_stream_body[search_stream_body_count].ecm_pid =
                                cas_decriptor->ca_pid;

                            break;
                        }
                    }

                }
            }
            if (i == cas2_count && cas1 != NULL)	//分开加密没找到,找相同加密
            {
                cas_decriptor = &cas1[0];
                search_stream_body[search_stream_body_count].ecm_pid =
                    cas_decriptor->ca_pid;

                for (i = 0; i < cas1_count; i++) {
                    cas_decriptor = &cas1[i];
                    //if (cas_decriptor->elem_pid ==
                    //    search_class.search_stream_body
                    //   [search_class.search_stream_body_count].audio_pid) 
                    {
                        if(tmp_searchclass->check_ca_fun(cas_decriptor->ca_system_id,
                                    search_stream_body[search_stream_body_count].audio_pid,
                                    cas_decriptor->ca_pid))
                        {
                            search_stream_body[search_stream_body_count].ecm_pid =
                                cas_decriptor->ca_pid;

                            break;
                        }
                    }

                }
            }
            break;
    }
    return GX_SEARCH_OK;
#if 0
err:
    return GX_SEARCH_ERR;
#endif
}
static inline int tolower(int ch)
{
	return (ch >= 'A' && ch <= 'Z') ? ch - 'A' + 'a' : ch;
}
static void  gx_search_prog_check_audio_lang(uint16_t* audio_pid,
                                             GxBusPmDataProgAudioType* type,
                                             uint16_t * ecm_pid,GxBusPmDataStream * search_stream_body,
                                             uint32_t search_stream_body_count)
{
    uint32_t i = 0;
    uint32_t j = 0;
    int8_t lang_buff[128];
    int8_t lang[3][4];
    int8_t flag = -1;//-1代表还没有赋值，0代表没有找到优先语言，1代表找到了最优语言，2代表着找到了第二优语言
    int8_t* p = 0;
    uint8_t count = 0;

    memset(lang,0,3*4);
   if(NULL ==  GxBus_ConfigGet(GXBUS_SEARCH_LANG_PRIORITY,(char*)lang_buff,128,GXBUS_SEARCH_LANG_DEFAULT))
   {
       strcpy((char*)lang_buff,GXBUS_SEARCH_LANG_DEFAULT);
   }
    //lang_buff = "eng_chn"等等表示一个或者师多个国家的iso9660代码组合
    //现在支持两种优先语言
    for(p=lang_buff; ;)
    {
        if(*p!='\0'&&count!=2 )
        {
            memcpy(lang[count],p,3);
            count++;
            p+=3;
        }
        if(*p!='\0'&&count!=2 )
        {
            p++;
        }
        else
        {
            break;
        }
    }
	if(search_stream_body_count != 0)
	{
		*audio_pid = search_stream_body[0].audio_pid;
		*type = search_stream_body[0].audio_type;
		*ecm_pid = search_stream_body[0].ecm_pid;
	}
	for(i=0; i<search_stream_body_count; i++)
	{
		if(search_stream_body[i].audio_type != GXBUS_PM_AUDIO_AC3)
		{  
            for(j=0; j<sizeof(search_stream_body[i].name); j++)
            {
                search_stream_body[i].name[j] = tolower(search_stream_body[i].name[j]);
            }
            if(-1 == flag)
            {
                *audio_pid = search_stream_body[0].audio_pid;
                *type = search_stream_body[0].audio_type;
                *ecm_pid = search_stream_body[0].ecm_pid;
                flag = 0;
            }
            
            if(0 == strcmp((const char*)(search_stream_body[i].name),(const char*)(lang[0])))
            {
                *audio_pid = search_stream_body[i].audio_pid;
                *type = search_stream_body[i].audio_type;
                *ecm_pid = search_stream_body[i].ecm_pid;
                flag = 1;
                break;
            }
            else if(0 == strcmp((const char*)(search_stream_body[i].name),(const char*)(lang[1])))
            {
                *audio_pid = search_stream_body[i].audio_pid;
                *type = search_stream_body[i].audio_type; 
                *ecm_pid = search_stream_body[i].ecm_pid;
                flag = 2;
            }
		}
	}
    return;
}


static status_t get_pmt_stream_info(AtscSearchNode_info_t * prog,
		GxSubsystemSiParsePMT * pmt,GxSearchClassObj * tmp_searchclass)
{
	uint32_t ca_count = 0,ca_count2 = 0;
	GxSubsystemSiParseCaDes *ca_info = NULL;
	GxSubsystemSiParseCaDes *ca_info2 = NULL;
	GxSubsystemSiParseTtxDes * ttx = NULL;
	GxSubsystemSiParseTtxInfo * ttx_info = NULL;
	uint32_t video_audio = 0,i = 0,j = 0,k = 0,n = 0,stream_info_count = pmt->loop2_count;
	GxSubsystemSiParsePmtLoop2 * stream_info = NULL;
	uint32_t ca_flag = 0,audio_count = 0,search_stream_body_count = 0;
	GxBusPmDataStream       *search_stream_body = NULL;
	GxBusPmDataProgAudioType type = 0xff;
	uint32_t a_pid_temp = 0xff, v_pid_temp = 0, pcr_pid_temp = 0;
	GxSearchClassObj *searchclass;

	for(i = 0;i<pmt->loop2_count;i++)
	{
		//stream
		stream_info_count +=pmt->loop2[i].ac3_count;
		stream_info_count +=pmt->loop2[i].eac3_count;
		//ca2
		if(pmt->loop2[i].ca_count > 0)
		{
			ca_count2 +=pmt->loop2[i].ca_count;
			ca_info2 = GxCore_Realloc(ca_info2,ca_count2*sizeof(GxSubsystemSiParseCaDes));
			if(ca_info2 == NULL)
			{
#ifdef GX_SEARCH_DEBUG
				GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---get_pmt_stream_info ca_info2 realloc failed\n");
#endif
				return GX_SEARCH_ERR;
			}
			memset(&ca_info2[n],0,sizeof(GxSubsystemSiParseCaDes)*pmt->loop2[i].ca_count);
			memcpy(&ca_info2[n],pmt->loop2[i].ca,sizeof(GxSubsystemSiParseCaDes)*pmt->loop2[i].ca_count);
			n += pmt->loop2[i].ca_count;
		}
	}

	if(pmt->loop1.ca_count != 0)
	{
		ca_count = pmt->loop1.ca_count;
		ca_info = pmt->loop1.ca;
		ca_flag = 1;
	}

	search_stream_body =(GxBusPmDataStream *)GxCore_Mallocz(sizeof(GxBusPmDataStream) * stream_info_count);
	if(search_stream_body == NULL)
	{
#ifdef GX_SEARCH_DEBUG
		GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ------get_pmt_stream_info search_stream_body malloc failed\n");
#endif
		return GX_SEARCH_ERR;
	}

	//for(i=0 ; i<(stream_info_count - ac3_count);i++)//这里只获得pcm的信息
	for(i=0 ; i < pmt->loop2_count ; i++)//这里只获得pcm的信息
	{
		stream_info = &pmt->loop2[i];

		switch(stream_info->stream_type)
		{
		case MPEG_1_VIDEO:
		case MPEG_2_VIDEO:
			//search_class.search_pmt_body.service_type = GXBUS_PM_PROG_MPEG;
			prog->pmt_info.video_type = GXBUS_PM_PROG_MPEG;
			video_audio = 0;
			break;
		case MPEG_4_VIDEO:
			prog->pmt_info.video_type = GXBUS_PM_PROG_MPEG4;
			video_audio = 0;
			break;
		case H264:
			//search_class.search_pmt_body.service_type = GXBUS_PM_PROG_H264;
			prog->pmt_info.video_type = GXBUS_PM_PROG_H264;
			video_audio = 0;
			break;
		case H265:
			prog->pmt_info.video_type = GXBUS_PM_PROG_H265;
			video_audio = 0;
			break;
		case AVS:
			//search_class.search_pmt_body.service_type = GXBUS_PM_PROG_AVS;
			prog->pmt_info.video_type = GXBUS_PM_PROG_AVS;
			video_audio = 0;
			break;
		case MPEG_1_AUDIO:
            search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG1;	
			video_audio = 1;
			break;
		case MPEG_2_AUDIO:			
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG2;	
			video_audio = 1;
			break;
		case AAC_ADTS:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
			video_audio = 1;
			break;
		case AAC_LATM:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_LATM;
			video_audio = 1;
			break;
			/*case PRIVATE_PES_STREAM:  //出现在pmt中一般是EAC3
			//search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
			//video_audio = 1;
			break;*/
		case LPCM:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_LPCM;
			video_audio = 1;
			break;
		case AC3:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
			video_audio = 1;
			break;
		case DTS:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS;
			video_audio = 1;
			break;
		case DOLBY_TRUEHD:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DOLBY_TRUEHD;
			video_audio = 1;
			break;
		case AC3_PLUS:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS;
			video_audio = 1;
			break;
		case DTS_HD:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD;
			video_audio = 1;
			break;
		case DTS_MA:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_MA;
			video_audio = 1;
			break;
		case AC3_PLUS_SEC:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS_SEC;
			video_audio = 1;
			break;
		case DTS_HD_SEC:
			search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD_SEC;
			video_audio = 1;
			break;
		default:
			//search_class.search_stream_body_count = 0;
			//GxCore_Free(search_class.search_stream_body);
			//search_class.search_stream_body = NULL;
#if 0
#ifdef GX_SEARCH_DEBUG
			GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---can't support stream type!!\n");
#endif
#endif
			video_audio = 2;
			break;
		}
		if(video_audio == 0)
		{
			//tmp_searchclass->search_pmt_body.video_pid = stream_info[i].elem_pid;
			prog->pmt_info.video_pid = stream_info->elementary_pid;
			gx_search_pmt_cas_get(ca_info,
					ca_count,
					ca_info2,
					ca_count2,
					video_audio,
					stream_info->elementary_pid,
					ca_flag,prog,search_stream_body,
					search_stream_body_count,tmp_searchclass);
			j = 1;
		}

		if(video_audio == 1)
		{
			search_stream_body[search_stream_body_count].audio_pid =
				stream_info->elementary_pid;


			gx_search_pmt_cas_get(ca_info,
					ca_count,
					ca_info2,
					ca_count2,
					video_audio,stream_info->elementary_pid,ca_flag,prog,
					search_stream_body,search_stream_body_count,tmp_searchclass);
			memset(search_stream_body
					[search_stream_body_count].name, 0, 4);
			if (stream_info->iso_639_count != 0) {
				memcpy(search_stream_body
						[search_stream_body_count].name,
						stream_info->iso_639[0].language, 3);
			}
			audio_count++; //不包括ac3,在这里使用临时变量替换原来的tmp_searchclass->audio_count++
			search_stream_body_count++;    //包括ac3
			j = 1;
		}
		if(j == 0)
		{
			gxlogd("j ============================ 0\n");
			goto err;
		}
		//ac3
		if(pmt->loop2[i].ac3_count > 0)
		{
			prog->pmt_info.ac3_flag = GXBUS_PM_PROG_BOOL_ENABLE;
			for(j = 0;j<pmt->loop2[i].ac3_count;j++)
			{
				search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
				search_stream_body[search_stream_body_count].audio_pid = pmt->loop2[i].elementary_pid;
				memset(search_stream_body[search_stream_body_count].name,0,4);
				search_stream_body_count++;
			}
		}

		//eac3
		if(pmt->loop2[i].eac3_count > 0)
		{
			prog->pmt_info.ac3_flag = GXBUS_PM_PROG_BOOL_ENABLE;
			for(j = 0;j<pmt->loop2[i].eac3_count;j++)
			{
				search_stream_body[search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
				search_stream_body[search_stream_body_count].audio_pid = pmt->loop2[i].elementary_pid;
				memset(search_stream_body[search_stream_body_count].name,0,4);
				search_stream_body_count++;
			}
		}
		//subt
		if(pmt->loop2[i].subtitling_count > 0)
		{
			prog->pmt_info.subt_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		}

		//ttx
		if(pmt->loop2[i].ttx_count > 0)
		{
			prog->pmt_info.ttx_flag = GXBUS_PM_PROG_BOOL_ENABLE;
			for(j = 0;j<pmt->loop2[i].ttx_count;j++)
			{
				ttx = &pmt->loop2[i].ttx[j];
				for(k = 0;k<ttx->txt_info_count;k++)
				{
					ttx_info = &ttx->ttx_info[k];
					if((ttx_info->teletex_type == 0x02)||(ttx_info->teletex_type == 0x05))
					{
						prog->pmt_info.cc_flag = GXBUS_PM_PROG_BOOL_ENABLE;
						break;
					}
				}
				if(prog->pmt_info.cc_flag == GXBUS_PM_PROG_BOOL_ENABLE)
					break;
			}
		}
	}


	prog->pmt_info.audio_count = audio_count;    //不包括ac3
	if(prog->pmt_info.ac3_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		for(i = 0;i<search_stream_body_count;i++)
		{
			if(search_stream_body[i].audio_type == GXBUS_PM_AUDIO_AC3)//ac3
			{           
				prog->pmt_info.ac3_pid = search_stream_body[i].audio_pid;
				prog->pmt_info.cur_audio_type= GXBUS_PM_AUDIO_AC3;
				break;
			}
			else if(search_stream_body[i].audio_type == GXBUS_PM_AUDIO_EAC3)//eac3
			{           
				prog->pmt_info.ac3_pid = search_stream_body[i].audio_pid;
				prog->pmt_info.cur_audio_type= GXBUS_PM_AUDIO_EAC3;
				//  break;
			}
		}
	}

	//非NIT搜索，现在没有NIT搜索项取第一个非AC3的audio当cur_audio_pid
	searchclass = gx_search_get_service_paramsptr();
	if (searchclass->scan_type == GX_SEARCH_PID) {
		if (0xffffffff == searchclass->search_pid_params.video_pid)
			v_pid_temp = prog->pmt_info.video_pid ;
		else
			v_pid_temp = prog->pmt_info.video_pid;
		
		if (0xffffffff == searchclass->search_pid_params.pcr_pid)
			pcr_pid_temp = prog->pmt_info.pcr_pid;
		else
			pcr_pid_temp = searchclass->search_pid_params.pcr_pid;
		
		if (0xffffffff == searchclass->search_pid_params.audio_pid)
			a_pid_temp = prog->pmt_info.cur_audio_pid = 0xffff;
		else {
			for (i=0; i<search_stream_body_count; i++){
				if ((search_stream_body[i].audio_type != GXBUS_PM_AUDIO_AC3)
						&&(search_stream_body[i].audio_pid ==
							tmp_searchclass->search_pid_params.audio_pid)){
					prog->pmt_info.cur_audio_pid= search_stream_body[i].audio_pid;
					prog->pmt_info.cur_audio_type= search_stream_body[i].audio_type;
				}
			}
		}

		if ((prog->pmt_info.video_pid != v_pid_temp) ||
				(prog->pmt_info.cur_audio_pid != a_pid_temp) ||
				(prog->pmt_info.pcr_pid != pcr_pid_temp))
			goto err;
	}

	if (a_pid_temp == 0xffff) {
		gx_search_prog_check_audio_lang(&(prog->pmt_info.cur_audio_pid),
				&type,// &(prog.cur_audio_type),
				&(prog->pmt_info.cur_audio_ecm_pid),search_stream_body,search_stream_body_count);

		if (type != 0xff){
			prog->pmt_info.cur_audio_type = type;
		}
	}

	if((ca_info2 != NULL) &&(ca_count2 > 0))
	{
		GxCore_Free(ca_info2);
		ca_info2 = NULL;
	}
	if(search_stream_body != NULL)
	{
		GxCore_Free(search_stream_body);
		search_stream_body = NULL;
	}

	return GX_SEARCH_OK;
#if 1
err:

	if((ca_info2 != NULL) &&(ca_count2 > 0))
	{
		GxCore_Free(ca_info2);
		ca_info2 = NULL;
	}
	if(search_stream_body != NULL)
	{
		GxCore_Free(search_stream_body);
		search_stream_body = NULL;
	}
	return GX_SEARCH_ERR;
#endif
}

static status_t check_type_scramble(GxSearchClassObj * tmp_searchclass,GxBusPmDataProg * prog)
{
    switch(tmp_searchclass->prog_type)
    {
        case GX_SEARCH_TV:
            if(prog->service_type != GXBUS_PM_PROG_TV)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---tv radio err!!\n");
#endif
                return GX_SEARCH_PROG_CONDITION_ERR;
            }       
            break;  
        case GX_SEARCH_RADIO:
            if(prog->service_type != GXBUS_PM_PROG_RADIO)
            {           
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---tv radio err!!\n");
#endif
                return GX_SEARCH_PROG_CONDITION_ERR;
            }
            break;
        case GX_SEARCH_DATAPROGRAM:
            /*数据服务  
            if(prog->service_type != GX_SEARCH_DATAPROGRAM)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---tv dataprogram err!!\n");
#endif
            }
            */
        default:
            break;
    }
    switch(tmp_searchclass->fta_cas)
    {
        case GX_SEARCH_FTA:
            if(prog->scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---cas fta err!!\n");
#endif
                return GX_SEARCH_PROG_CONDITION_ERR;
            }
            break;

        case GX_SEARCH_CAS:
            if(prog->scramble_flag == GXBUS_PM_PROG_BOOL_DISABLE)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---cas fta err!!\n");
#endif
                return GX_SEARCH_PROG_CONDITION_ERR;
            }
            break;

        default:
            break;

    }
    return GX_SEARCH_OK;
}
static status_t find_pmt_pid(uint16_t service_id,uint16_t * pmt_pid)
{
    ProtocolPmtObj * pmtobj = NULL;
    uint32_t i =0;
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmtobj = &((GxAtscProtocolObj *)SearchServiceObj.protocol_obj)->pmt_obj_info;
    }
    else if(SearchServiceObj.type == PROTOCOL_DVB)
    {
        pmtobj = &((GxDvbProtocolObj *)SearchServiceObj.protocol_obj)->pmt_obj_info;
    }
    for(i = 0;i<pmtobj->pmt_count;i++)
    {
        if(service_id == pmtobj->pmt_info[i].service_id)
        {
            *pmt_pid=pmtobj->pmt_info[i].pid;
            return GX_SEARCH_OK;
        }
    }
    return GX_SEARCH_ERR;
}


status_t add_pmt_info(void * prog,GxSubsystemSiParsePMT * pmt,GxSearchClassObj * searchclass)
{
    status_t ret;
    Available_Pmt_Data_Obj * pmt_info = NULL; 

    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmt_info = &((AtscSearchNode_info_t * )prog)->pmt_info;
    }
    else
    {
        pmt_info = &((DvbSearchNode_info_t *)prog)->pmt_info;
    }

    pmt_info->service_id = pmt->program_number;//与VCT重复不过program_number一致
    pmt_info->pcr_pid = pmt->PCR_PID;

    ret = find_pmt_pid(pmt_info->service_id,&pmt_info->pmt_pid);
    if(ret != GX_SEARCH_OK)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---can not find pmt pid\n");
#endif
        return GX_SEARCH_ERR;
    }
    ret = get_pmt_stream_info(prog,pmt,searchclass);
    if(ret != GX_SEARCH_OK)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---get pmt stream info err\n");
#endif
        return ret;
    }
    return GX_SEARCH_OK;
}

static status_t get_public_prog_info(GxSearchClassObj * tmp_searchclass,GxBusPmDataProg * prog)
{
    uint32_t count = 0; 

    count = GxBus_PmProgExistChek(tmp_searchclass->search_tp_id_for_prog,
            prog->ts_id,prog->service_id,tmp_searchclass->original_network_id,tmp_searchclass->search_tuner_num_for_prog,
			0xff,
			0xff,
			0xff);
            
    count = 0;
    if (count != 0) {
        tmp_searchclass->search_new_prog_get.flag = GXBUS_PM_PROG_EXIST;
        return GX_SEARCH_PROG_EXIST;
    }
    prog->tp_id = tmp_searchclass->search_tp_id_for_prog;
    prog->sat_id = tmp_searchclass->search_sat_id_for_prog;
    prog->audio_volume = 50;
    prog->audio_mode = GXBUS_PM_PROG_AUDIO_MODE_LEFT;
    prog->logic_valid = 0;
    prog->original_id = tmp_searchclass->original_network_id;
    prog->pat_version = tmp_searchclass->pat_version;
    prog->ts_id = tmp_searchclass->ts_id;

    prog->skip_flag = GXBUS_PM_PROG_BOOL_DISABLE;
    prog->lock_flag = GXBUS_PM_PROG_BOOL_DISABLE;
    prog->favorite_flag = 0;

    prog->bouquet_id = 0;    //等待si解析bat表
    prog->pmt_version = 0;   //需要监控的话用初始0版本也是可以的
    prog->audio_level = GXBUS_PM_PROG_AUDIO_LECEL_MID;
    prog->tuner = tmp_searchclass->search_tuner_num_for_prog;
	prog->data_plp_id = 0xff;
	prog->common_plp_exist = 0xff;
	prog->common_plp_id = 0xff;
    return GX_SEARCH_OK;
}


status_t add_vct_info(AtscSearchNode_info_t * prog,GxSubsystemSiParseVCTloop * vct)
{
	int i = 0,j = 0;

	for(i=0 ; i<vct->sld_count ; i++)
	{
		for(j=0 ; j<vct->sldes[i].number_elements ; j++)
		{
			switch(vct->sldes[i].elemets_info[j].stream_type)
			{
			case 0x02: 
				//ATSC协议视频编码方式有mepg和h.262 2种考虑H.262已经过时先直接赋为MGEP
				prog->vct_info.video_type = GXBUS_PM_PROG_MPEG;
				break;
			case 0x81:
				prog->vct_info.audio_type = GXBUS_PM_AUDIO_AC3;
				break;
			default:
				//当stream_type不是音视频时，需要根据样机来判断取舍
				while(1);
				break;
			}
		}
	}
    prog->vct_info.major_num = vct->major_channel_number;
    prog->vct_info.minor_num = vct->minor_channel_number;
    prog->vct_info.source_id = vct->source_id;
    //prog->vct_info.ts_id = vct->channel_TSID;
    prog->vct_info.service_id = vct->program_number;
    memcpy(prog->vct_info.prog_name,vct->short_name,MIN_VCT_PM_NAME_LEN);


    if(vct->info.cvctinfo.path_select != 0)
    {
        prog->vct_info.path_select = vct->info.cvctinfo.path_select;
    }

    if(vct->info.cvctinfo.out_of_band != 0)
    {
        prog->vct_info.out_of_band = vct->info.cvctinfo.out_of_band;
    }

    switch(vct->service_type)
	{
	case 0x00:
		//reserved
		break;
	case 0x01:
		//模拟节目
		prog->vct_info.service_type = GX_SEARCH_ANALOG_TV;
		break;
	case 0x02:
		prog->vct_info.service_type = GXBUS_PM_PROG_TV;
		break;
	case 0x03:
		prog->vct_info.service_type = GXBUS_PM_PROG_RADIO;
		break;
	case 0x04:
		//数据服务
		break;
	default:
		prog->vct_info.service_type = GXBUS_PM_PROG_TV;
		break;
	}
    return GX_SEARCH_OK;
}

status_t add_sdt_info(DvbSearchNode_info_t * prog,GxSubsystemSiParseSdtInfo * sdt)
{
    uint32_t program_name_len = 0;
    uint8_t buff[MAX_PROG_NAME] = {0};
    prog->sdt_info.service_id = sdt->service_id;
    if(sdt->service_count > 0)
    {
        if (sdt->service->service_name_length > MAX_PROG_NAME)
            program_name_len = MAX_PROG_NAME - 1;
        else
            program_name_len = sdt->service->service_name_length;
        if(program_name_len > 0)
            memcpy(prog->sdt_info.service_name,sdt->service->service_name,program_name_len);
        else
            strcpy((char *)prog->sdt_info.service_name,"no name");

		if (sdt->service->service_provider_length > MAX_PROG_NAME)
			program_name_len = MAX_PROG_NAME - 1;
		else
			program_name_len = sdt->service->service_provider_length;
        if(program_name_len > 0)
            memcpy(prog->sdt_info.service_provider_name, sdt->service->provider_name, program_name_len);
        else
            strcpy((char *)prog->sdt_info.service_provider_name,"no name");




        if (sdt->service->service_type == SI_AUDIO_SERVICE) {
                prog->sdt_info.service_type = GXBUS_PM_PROG_RADIO;
        }else if(sdt->service->service_type == SI_VIDEO_SERVICE){
                prog->sdt_info.service_type = GXBUS_PM_PROG_TV;
        }
        else if((sdt->service->service_type == SI_NVOD_REFERENCE_SERVICE) ||
                        (sdt->service->service_type == SI_NVOD_TIMESHIFT_SERVICE)){
                prog->sdt_info.service_type = GXBUS_PM_PROG_NVOD;
        }
        else
        {
                prog->sdt_info.service_type = GXBUS_PM_PROG_TV;
        }
    }
    else
    {
        memset(prog->sdt_info.service_name,0,MAX_PROG_NAME);
        memset(buff,0,MAX_PROG_NAME);
    }
    return GX_SEARCH_OK;
}

void transcode(GxSearchTranscodingFlag flag,uint8_t * dest,uint8_t * from,uint32_t name_length) 
{
    uint32_t y = 0;
    uint32_t psize = 0;
    uint8_t *pstr = NULL;
    int32_t max_name_len = 0;
    //vct short_name 最大14字节，sdt 原来定义最大为20字节
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        max_name_len = MIN_VCT_PM_NAME_LEN;
    }
    else
    {
        max_name_len = MAX_PROG_NAME;
    }

    for(y = 0;y<name_length;y++)
    {
        if( from[y]<0x1f||from[y]>0x20)
        {
            break;
        }
    }

    if(flag == GXBUS_SEARCH_UTF8_TRANS_NO)
    {
        memcpy(dest,from,name_length-y);
    }
    else if(flag == GXBUS_SEARCH_UTF8_TRANS_YES)
    {
        if(gxgdi_iconv((const unsigned char *)&from[y],&pstr,name_length-y,&psize, NULL) == 0)
        {
            if(psize >= max_name_len)
            {
                name_length = MIN_VCT_PM_NAME_LEN - 1;
            }
            else
            {
                name_length = psize;
            }
            memcpy(dest,pstr,name_length);
            GxCore_Free(pstr);
        }
        else
        {
            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---iconv utf8 err!!------\n");
            memcpy(dest,&(from[y]),name_length-y);
        }
    }
    dest[max_name_len-1] = 0;

}

void get_vct_info(AtscSearchNode_info_t * info,GxBusPmDataProg * prog,GxSearchTranscodingFlag flag)
{
    prog->major_num = info->vct_info.major_num;
    prog->u3.minor_num = info->vct_info.minor_num;
    prog->u1.source_id = info->vct_info.source_id;
    prog->path_select = info->vct_info.path_select;
    prog->out_of_band = info->vct_info.out_of_band;
    prog->service_type = info->vct_info.service_type;
    if(prog->service_id == 0)
    {
        prog->service_id = info->vct_info.service_id;
    }
    
    transcode(flag,prog->prog_name,info->vct_info.prog_name,MIN_VCT_PM_NAME_LEN);
}

void get_sdt_info(DvbSearchNode_info_t * info,GxBusPmDataProg * prog,GxSearchClassObj * tmp_searchclass)
{
    if(prog->service_id == 0)
    {
        prog->service_id = info->sdt_info.service_id;
    }
    prog->service_type = info->sdt_info.service_type;
    prog->sdt_version = SearchServiceObj.searchclass.sdt_version;
    transcode(tmp_searchclass->transcodingflag,prog->prog_name,info->sdt_info.service_name,MAX_PROG_NAME);
    transcode(tmp_searchclass->transcodingflag,prog->u2.long_name,info->sdt_info.service_provider_name,MAX_PROG_NAME);
    
    if(tmp_searchclass->get_sdt_info_func != NULL)
    {
        GxSearchSdtInfo * sdtinfo = GxCore_Mallocz(sizeof(GxSearchSdtInfo));

        sdtinfo->service_id = info->sdt_info.service_id;
        sdtinfo->service_type = info->sdt_info.service_type;
        sdtinfo->sdt_version = SearchServiceObj.searchclass.sdt_version;
        memcpy(sdtinfo->service_name,info->sdt_info.service_name,MAX_PROG_NAME);
        tmp_searchclass->get_sdt_info_func(sdtinfo);
        GxCore_Free(sdtinfo);
    }
}


void get_pmt_info(void * info,GxBusPmDataProg * prog)
{
    Available_Pmt_Data_Obj * pmt_info = NULL;
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmt_info = &((AtscSearchNode_info_t *)info)->pmt_info;
    }
    else
    {
        pmt_info = &((DvbSearchNode_info_t *)info)->pmt_info;
    }

    prog->scramble_flag = pmt_info->scramble_flag;
    prog->ttx_flag = pmt_info->ttx_flag;
    prog->subt_flag = pmt_info->subt_flag;
    prog->cc_flag = pmt_info->cc_flag;
    prog->ac3_flag = pmt_info->ac3_flag;
    prog->pcr_pid = pmt_info->pcr_pid;
    prog->pmt_pid = pmt_info->pmt_pid;
    prog->video_pid = pmt_info->video_pid;
    prog->video_type = pmt_info->video_type;
    prog->ecm_pid_v = pmt_info->ecm_pid_v;
    prog->audio_count = pmt_info->audio_count;
    prog->ac3_pid = pmt_info->ac3_pid;
    prog->cur_audio_pid = pmt_info->cur_audio_pid;
    prog->cur_audio_type = pmt_info->cur_audio_type;
    prog->cur_audio_ecm_pid = pmt_info->cur_audio_ecm_pid;
    prog->cas_id = pmt_info->cas_id;
    if(prog->service_id == 0)
    {
        prog->service_id = pmt_info->service_id;
    }
}

void get_info_from_node(void * info,GxBusPmDataProg * prog,GxSearchClassObj * tmp_searchclass)
{
	
    if(SearchServiceObj.type == PROTOCOL_ATSC)
	{
		AtscSearchNode_info_t * atsc_info = (AtscSearchNode_info_t *)info;
		//当既过滤到VCT表又过滤到PMT表时，如果解析出来的音视频type不一样，需要根据样机来判断取舍
		if(atsc_info->pmt_info.service_id && atsc_info->vct_info.service_id)
		{
			if(atsc_info->pmt_info.video_type != atsc_info->vct_info.video_type ||
					atsc_info->pmt_info.cur_audio_type != atsc_info->vct_info.audio_type)
			{
				while(1);
			}
		}
		get_vct_info((AtscSearchNode_info_t *)info,prog,tmp_searchclass->transcodingflag);
	}
    else
    {
        get_sdt_info((DvbSearchNode_info_t *)info,prog,tmp_searchclass);
    }
    get_pmt_info(info,prog);
}

/* 
 *在search中根据pmt和vct/sdt添加节目
 * */
void search_merge(void * info,GxSearchClassObj * tmp_searchclass)
{
    GxMessage *new_msg = NULL;
    GxBusPmDataProg * prog = NULL;
    status_t ret;
    GxMsgProperty_NewProgGet * params = NULL;

    prog = GxCore_Mallocz(sizeof(GxBusPmDataProg));
    if(prog == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---search merge prog malloc failed\n");
#endif
        goto err;
    }
    
    get_info_from_node(info,prog,tmp_searchclass);
    
    ret = get_public_prog_info(tmp_searchclass,prog);
    if(ret != GX_SEARCH_OK)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[search] ---search prog exist\n");
#endif
        goto err;
    }
    
    ret = check_type_scramble(tmp_searchclass,prog);
    if(ret != GX_SEARCH_OK)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] --- search prog check failed\n");
#endif
        goto err;
    }

   if(tmp_searchclass->modify_prog != NULL)
    {
        ret = tmp_searchclass->modify_prog(prog);
        if(ret != GX_SEARCH_OK)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[search] ---usr modify_prog return err\n");
#endif
            goto err;
        }
    }

    ret = GxBus_PmProgAdd(prog);
    switch(ret)
    {
        case GX_PM_DBASE_FULL:
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[search] ---dbase full\n");
#endif
            /* 省去searchclassobj单独初始化，因为在dbase_full时停止搜台*/
            gx_search_error_reply(SEARCH_DBASE_OVERFLOW);
            //gx_search_service_variable_exit_realease();
            gx_search_stop_reply(tmp_searchclass);
            goto err;
            break;
        case GX_SEARCH_ERR:
            //不需要break
        case GX_SEARCH_PROG_CONDITION_ERR:
            goto err;
            break;
        default:
            //GX_SEARCH_PROG_EXIT
            break;
    }
    tmp_searchclass->search_new_prog_get.type = prog->service_type;
    tmp_searchclass->search_new_prog_get.flag = GXBUS_PM_PROG_NOT_EXIST;
    tmp_searchclass->search_new_prog_get.source_id = prog->u1.source_id;
    tmp_searchclass->search_new_prog_get.service_id = prog->service_id;
    memcpy(tmp_searchclass->search_new_prog_get.name,prog->prog_name,MIN_VCT_PM_NAME_LEN);
    new_msg = GxBus_MessageNew(GXMSG_SEARCH_NEW_PROG_GET);
    params =
        (GxMsgProperty_NewProgGet *) GxBus_GetMsgPropertyPtr(new_msg,
                GxMsgProperty_NewProgGet);
    memcpy(params, &(tmp_searchclass->search_new_prog_get), sizeof(GxMsgProperty_NewProgGet));
    GxBus_MessageSend(new_msg); 
    tmp_searchclass->search_prog_id_arry[tmp_searchclass->search_prog_id_num] = prog->id;
    tmp_searchclass->search_prog_id_num++;
    GxCore_Free(prog);
    return ;
err:
    //开始搜索下一个节目
    GxCore_Free(prog);
    return ;
}

/* 
 *开始atsc搜台的解表流程
 * */
status_t gx_atsc_search_start(GxSearchClassObj * searchclass)
{
    status_t ret = 0;
    GxAtscProtocolObj * tmp_protocol = (GxAtscProtocolObj *)SearchServiceObj.protocol_obj;
    /* 扩展和nit搜索 ATSC标准中先不考虑 */

    ret = tmp_protocol->pat_obj_info.pat_create(); 
    if(ret != GX_SEARCH_OK)
    {
        gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---creat pat filter err!!\n");
#endif
        return GX_SEARCH_ERR;
    }

    ret = tmp_protocol->vct_obj_info.vct_create();
    if(ret != GX_SEARCH_OK)
    {
        gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---creat vct filter err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
    return GX_SEARCH_OK;
}
/* 
 *完成私有解析后的dvb搜台流程
 * */
status_t gx_dvb_search_normal_start( GxSearchClassObj * tmp_searchclass)
{
    GxDvbProtocolObj * tmp_protocol = (GxDvbProtocolObj *)SearchServiceObj.protocol_obj;
    status_t ret = 0;

    if(tmp_searchclass->nit.nit_switch == GX_SEARCH_NIT_ENABLE)
    {
        ret = tmp_searchclass->nit.nit_obj->nit_create((void *)tmp_searchclass);
        if (ret != GX_SEARCH_OK)                                             
        {                                                                    
            gx_search_error_reply(SEARCH_ERROR);                             
#ifdef GX_BUS_SEARCH_DBUG                                        
            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---creat nit filter err!!\n");
#endif                                                           
            return GX_SEARCH_ERR;                                            
        }                                                                    
    }

    ret = tmp_protocol->pat_obj_info.pat_create();
    if(ret != GX_SEARCH_OK)
    {
        gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---creat pat filter err!!\n");
#endif
        return GX_SEARCH_ERR;

    }
    ret = tmp_protocol->sdt_obj_info.sdt_create();
    if(ret != GX_SEARCH_OK)
    {
        gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] --- create sdt filter err!\n");
#endif
        return GX_SEARCH_ERR;
    }
    return GX_SEARCH_OK;
}

/* 
 *开始DVB搜台的解表流程
 * */
status_t gx_dvb_search_start(GxSearchClassObj * searchclass)
{
    status_t ret = 0;

    if(searchclass->ext.need_search_ext != 0)
    {
        //并非搜索结束，只是将私有解析交给用户自己去做
        
        searchclass->search_status = SEARCH_PAUSE;
        return GX_SEARCH_OK;
    }

    ret = gx_dvb_search_normal_start(searchclass);
    if(ret != GX_SEARCH_OK)
    {
#ifdef ATSC_SEARCH_DBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---start normal filter err!!\n");
#endif
        return GX_SEARCH_ERR;
    }
   
    return GX_SEARCH_OK;
}

status_t gx_search_table_filter_start(GxSearchClassObj * searchclass)
{
    status_t ret = 0;
    ret = searchclass->tp_lock_func((void*)searchclass);
    if(ret == GX_SEARCH_OK)
    {
        //在这里开始根据协议走不同的解表流程
        if(SearchServiceObj.type == PROTOCOL_ATSC)
        {
            ret = gx_atsc_search_start(searchclass);
        }
        else if(SearchServiceObj.type == PROTOCOL_DVB)
        {
            ret = gx_dvb_search_start(searchclass);
        }

        if(ret != GX_SEARCH_OK)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---gx_atsc_search_start err!!\n ");            
#endif
            //gx_search_stop_reply();
        }

    }
    else if(ret == GX_SEARCH_ERR)
    {
        gx_search_error_reply(SEARCH_ERROR);
        gx_search_stop_reply(searchclass);
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---tp lock err!!\n\n");
#endif
    }
    else if(ret == GX_SEARCH_FINISH)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---scan finish !!\n");
#endif
        gx_search_stop_reply(searchclass);//这里不能省略，因为在auto_scan时需要这里发送stop_ok
    }
    //gx_search_set_service_paramsptr(searchclass,1);
    return ret;
}
