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
#include "include/gx_search_private.h"
#define GX_SEARCH_DEBUG


/* *****************function******************************/
/**
 * @brief 初始化搜索服务
 * @param
 * @Return
 */

status_t GxAdvanceSearchInit(handle_t self,int priority_offset)
{
    handle_t sch;
  
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("gx_search_table_filter_start malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }
	GxCore_MutexCreate(&searchclass->list_mutex);
    GxCore_MutexCreate(&searchclass->search_mutex);

    searchclass->SearchServiceMsgHandle = self;
    /*由于表解析由SI_parser线程触发，同一个表的操作是单线程，多个表间同步直接用全局锁 
    GxCore_MutexCreate(search_obj.vct_mutex);
    GxCore_MutexCreate(search_obj.pat_mutex);
    GxCore_MutexCreate(search_obj.pmt_mutex);
    */
    GxBus_MessageRegister(GXMSG_SEARCH_ATSC_T_SCAN_START,sizeof(GxMsgProperty_ScanStart));
    GxBus_MessageRegister(GXMSG_SEARCH_ATSC_C_SCAN_START,sizeof(GxMsgProperty_ScanStart));
	GxBus_MessageRegister(GXMSG_SEARCH_BLIND_SCAN_START,sizeof(GxMsgProperty_ScanStart));
    GxBus_MessageRegister(GXMSG_SEARCH_SCAN_STOP,0);
    GxBus_MessageRegister(GXMSG_SEARCH_SAVE,0);
    GxBus_MessageRegister(GXMSG_SEARCH_NOT_SAVE,0);
    GxBus_MessageRegister(GXMSG_SEARCH_DVB_SAT_SCAN_START,sizeof(GxMsgProperty_ScanStart));
	GxBus_MessageRegister(GXMSG_SEARCH_BLIND_SCAN_REPLY,sizeof(GxMsgProperty_BlindScanReply));

    GxBus_MessageRegister(GXMSG_SEARCH_STOP_OK,0);
    GxBus_MessageRegister(GXMSG_SEARCH_NEW_PROG_GET,sizeof(GxMsgProperty_NewProgGet));
    GxBus_MessageRegister(GXMSG_SEARCH_STATUS_REPLY,sizeof(GxMsgProperty_StatusReply));
    GxBus_MessageRegister(GXMSG_SEARCH_SAT_TP_REPLY,sizeof(GxMsgProperty_SatTpReply));
    GxBus_MessageRegister(GXMSG_SEARCH_CONTINUE,0);

    GxBus_MessageListen(self,GXMSG_SEARCH_ATSC_T_SCAN_START);
    GxBus_MessageListen(self,GXMSG_SEARCH_ATSC_C_SCAN_START);
	GxBus_MessageListen(self,GXMSG_SEARCH_BLIND_SCAN_START);
    GxBus_MessageListen(self,GXMSG_SEARCH_SCAN_STOP);
    GxBus_MessageListen(self,GXMSG_SEARCH_SAVE);
    GxBus_MessageListen(self,GXMSG_SEARCH_NOT_SAVE);
    GxBus_MessageListen(self,GXMSG_SEARCH_CONTINUE);
    GxBus_MessageListen(self,GXMSG_SEARCH_DVB_SAT_SCAN_START);

    sch = GxBus_SchedulerCreate("AtscSearchMsgScheduler",GXBUS_SCHED_MSG,1024*32,GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

	if(GXCORE_SUCCESS != GxCore_SemCreate(&blindclassobj.search_blind_sem,0))
	{
#ifdef GX_SEARCH_DEBUG                                                         
		GX_BUS_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---create blind sem err !!\n");
#endif
		return GXCORE_ERROR;
	}


	searchclass->search_status = SEARCH_STOP;
	return GX_SEARCH_OK;
}

void GxAdvanceSearchServiceConsole(handle_t self)
{
    status_t ret = 0;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("gx_search_table_filter_start malloc failed\n");
#endif
		while(1);
        return ;
    }

    while(1)
    {
        GxCore_SemWait(searchclass->blindclass->search_blind_sem);
start_blind:
        ret = gx_blind_get_status();
        if(ret != SEARCH_RUNNING)
        {
            gxlogd("gx_blind_get_status error\n");
            goto blind_no_ok;
        }
        ret = gx_blind_search(searchclass);
        if(ret != GX_SEARCH_OK)
        {
            gxlogd("gx_blind_search error\n");
            goto blind_no_ok;
        }
        ret = gx_search_blind_get_params(searchclass);
        if(GX_SEARCH_OK != ret)
        {
            gxlogd("gx_search_blind_get_params error\n");
            goto blind_no_ok;
        }
		GxCore_ThreadYield();
        goto start_blind;

blind_no_ok:
        if(ret == GX_SEARCH_FINISH)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---blind finish!!\n");
#endif
			GxCore_GetTickTime(&searchclass->stop_time);
			gxlogd("start time:%d,end time:%d,use time:%d\n",
					searchclass->start_time.seconds,searchclass->stop_time.seconds,
				searchclass->stop_time.seconds-searchclass->start_time.seconds);
			//gx_search_service_variable_exit_realease(searchclass);
			gx_search_stop_reply(searchclass);
            GxBus_PmSync(GXBUS_PM_SYNC_TP);
        }
        else
        {
            GX_BUS_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---blind scan err!!\n");		
			//gx_search_service_variable_exit_realease(searchclass);
			gx_search_stop_reply(searchclass);
            GxBus_PmSync(GXBUS_PM_SYNC_TP);
        }
    }
}

void GxAdvanceSearchDestroy(handle_t self)
{

    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    //GxSearchClassObj * searchclass = &SearchServiceObj.searchclass;
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("gx_search_table_filter_start malloc failed\n");
#endif
    }

	GxBus_MessageUnListen(self,GXMSG_SEARCH_ATSC_T_SCAN_START);
	GxBus_MessageUnListen(self,GXMSG_SEARCH_ATSC_C_SCAN_START);
	GxBus_MessageUnListen(self,GXMSG_SEARCH_BLIND_SCAN_START);
	GxBus_MessageUnListen(self,GXMSG_SEARCH_SCAN_STOP);
	GxBus_MessageUnListen(self,GXMSG_SEARCH_SAVE);
	GxBus_MessageUnListen(self,GXMSG_SEARCH_NOT_SAVE);
	GxBus_MessageUnListen(self,GXMSG_SEARCH_CONTINUE);
	GxBus_MessageUnListen(self,GXMSG_SEARCH_DVB_SAT_SCAN_START);

	GxBus_MessageUnregister(GXMSG_SEARCH_SAT_TP_REPLY);
	GxBus_MessageUnregister(GXMSG_SEARCH_STATUS_REPLY);
	GxBus_MessageUnregister(GXMSG_SEARCH_STOP_OK);
	GxBus_MessageUnregister(GXMSG_SEARCH_NEW_PROG_GET);
	GxBus_MessageUnregister(GXMSG_SEARCH_ATSC_T_SCAN_START);
	GxBus_MessageUnregister(GXMSG_SEARCH_ATSC_C_SCAN_START);
	GxBus_MessageUnregister(GXMSG_SEARCH_BLIND_SCAN_START);
	GxBus_MessageUnregister(GXMSG_SEARCH_SCAN_STOP);
	GxBus_MessageUnregister(GXMSG_SEARCH_SAVE);
	GxBus_MessageUnregister(GXMSG_SEARCH_NOT_SAVE);
	GxBus_MessageUnregister(GXMSG_SEARCH_CONTINUE);
	GxBus_MessageUnregister(GXMSG_SEARCH_DVB_SAT_SCAN_START);
	GxBus_MessageUnregister(GXMSG_SEARCH_BLIND_SCAN_REPLY);

#if 0
	if(SearchServiceObj.type == PROTOCOL_ATSC)
	{
		GxCore_MutexDelete(((GxAtscProtocolObj *)SearchServiceObj.protocol_obj)->vct_obj_info.vct_mutex);
		GxCore_MutexDelete(((GxAtscProtocolObj *)SearchServiceObj.protocol_obj)->pat_obj_info.pat_mutex);
		GxCore_MutexDelete(((GxAtscProtocolObj *)SearchServiceObj.protocol_obj)->pmt_obj_info.pmt_mutex);
	}
#endif
	GxCore_MutexDelete(searchclass->list_mutex);
	
	GxBus_ServiceUnlink(self);
	return;
}

GxMsgStatus GxAdvanceSearchServiceRecvMsg(handle_t self, GxMessage * Msg)
{
    GxMsgStatus status = GXMSG_OK;
	GxBusPmViewInfo sys = {0};
    status_t ret = 0;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }


    switch(Msg->msg_id)
	{
	case GXMSG_SEARCH_BLIND_SCAN_START:
		if(searchclass->search_status != SEARCH_STOP)
		{
#ifdef GX_SEARCH_DEBUG
			GX_BUS_SEARCH_NORMAL_PRINTF("[ATSC SEARCH]---search have runnig!!\n");
#endif
			return GX_SEARCH_ERR;
		}
		memset(&searchclass->start_time,0,sizeof(GxTime));
		memset(&searchclass->stop_time,0,sizeof(GxTime));
		GxCore_GetTickTime(&searchclass->start_time);
		ret = gx_search_scan_init_start(Msg,searchclass);
		if(ret != GX_SEARCH_OK)
		{
			gx_search_stop_reply(searchclass);
			break;
		}

		ret = gx_search_blind_scan_init_start(Msg,searchclass);
		if(ret != GX_SEARCH_OK)
		{
			gx_search_stop_reply(searchclass);
		}
		break;
	case GXMSG_SEARCH_ATSC_T_SCAN_START:
	case GXMSG_SEARCH_ATSC_C_SCAN_START:
	case GXMSG_SEARCH_DVB_SAT_SCAN_START:
		if(searchclass->search_status != SEARCH_STOP)
		{
#ifdef GX_SEARCH_DEBUG
			GX_BUS_SEARCH_NORMAL_PRINTF("[ATSC SEARCH]---search have runnig!!\n");
#endif
			return GX_SEARCH_ERR;
		}
		ret = gx_search_scan_init_start(Msg,searchclass);
		if(ret != GX_SEARCH_OK)
		{
			gx_search_stop_reply(searchclass);               
			break;
		}

		ret = gx_search_table_filter_start(searchclass);
		if(ret != GX_SEARCH_OK)
		{
			gx_search_stop_reply(searchclass);
		}
		status = GXMSG_OK;
		break;
	case GXMSG_SEARCH_SCAN_STOP:
		if(searchclass->search_status == SEARCH_STOP)
		{
			break;
		}
#ifdef GX_SEARCH_DEBUG
		GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---stop msg !!\n");
#endif
		gx_search_stop_reply(searchclass);
		status = GXMSG_OK;
		break;
	case GXMSG_SEARCH_SAVE:
		if(searchclass->search_status != SEARCH_STOP)
		{
			break;
		}
#ifdef GX_SEARCH_DEBUG
		GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---save msg!!\n");
#endif
		/*保存搜索到的节目 */
		//GxBus_PmTransactionCommit();

		if (searchclass->search_prog_id_arry != NULL) 
		{
			GxCore_Free(searchclass->search_prog_id_arry);
			searchclass->search_prog_id_arry = NULL;
			GxBus_PmViewInfoGet(&sys);
			if(VIEW_INFO_DISABLE == sys.status)
			{
				sys.status = VIEW_INFO_ENABLE;//当搜索到节目的时候需要建立节目列表
				GxBus_PmViewInfoModify(&sys);//通过整个函数创建了节目列表
			}
			GxBus_PmSync(GXBUS_PM_SYNC_PROG);//同步里面会重建prog list了
		}
		searchclass->search_prog_id_num = 0;
		gx_search_service_variable_exit_realease(searchclass);

		status = GXMSG_OK;
		//GxBus_PmDbaseBackupDel();
		break;
	case GXMSG_SEARCH_NOT_SAVE:
		if(searchclass->search_status != SEARCH_STOP)
		{
			break;
		}
#ifdef GX_SEARCH_DEBUG
		GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---not save msg!!\n");
#endif
		/*不保存搜到的节目 回滚事务 */
		//GxBus_PmTransactionRollback();
		if (searchclass->search_prog_id_arry != NULL
				&&searchclass->search_prog_id_num != 0) 
		{
			GxBus_PmProgDelete(searchclass->search_prog_id_arry, searchclass->search_prog_id_num);
			GxCore_Free(searchclass->search_prog_id_arry);
			searchclass->search_prog_id_arry = NULL;
			GxBus_PmSync(GXBUS_PM_SYNC_PROG);//同步里面会重建prog list了
		}
		searchclass->search_prog_id_num = 0;
		gx_search_service_variable_exit_realease(searchclass);
		status = GXMSG_OK;
		//gx_search_stop_reply();//此时硬件资源早就被gx_search_variable_next_tp_init释放光了,可以直接发stop ok
		//GxBus_PmDbaseComebackFromMem();
		break;
	case GXMSG_SEARCH_CONTINUE:
		if(searchclass->search_status != SEARCH_PAUSE)
		{
			break;
		}
		gxlogd("------------------------------------------------------------------\nGXMSG_SEARCH_CONTINUE\n");
		searchclass->ext.need_search_ext = 0;
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
			//gx_search_service_variable_exit_realease();
			gx_search_stop_reply(searchclass);
		}
		searchclass->search_status = SEARCH_START;
		status = GXMSG_OK;
		break;
	default:
		break;
	}
    return status;
}

GxServiceClass advance_search_service = {
	"Advance search service",
	GxAdvanceSearchInit,
	GxAdvanceSearchDestroy,
	GxAdvanceSearchServiceRecvMsg,
	GxAdvanceSearchServiceConsole,
	.priority_offset = 0,
};
