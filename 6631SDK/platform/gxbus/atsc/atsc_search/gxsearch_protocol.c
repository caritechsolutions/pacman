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

/* *****************include******************************/
#include <stdio.h>
#include "service/gxadvance_search.h"
#include "include/gx_search_private.h"
#include "module/config/gxconfig.h"
#include "gxcore.h"


#define ATSC_SEARCH_INITED      (1)
#define PMT_TID 0x02
#define T_VCT_TID 0xc8
#define C_VCT_TID 0xc9
#define PAT_TID 0x00
static GX_LIST_HEAD(AtscSearch_add_info_list);

/* *****************function******************************/

void Protocol_realease_node(SearchLlist_info_t * info_list)
{
    if(info_list != NULL)
    {
        if(info_list->prog != NULL)
        {
            GxCore_Free(info_list->prog);
            info_list->prog = NULL;
        }
        GxCore_Free(info_list);
    }
}

/*
 *目前仅用于搜台中途退出gx_search_service_variable_exit_realease时使用所以没有加锁，
 *如果以后有其他地方用到必须考虑加锁问题
 * */
void clear_search_list()
{
    SearchLlist_info_t  * info_list = NULL; 
    SearchLlist_info_t  * tmp_node = NULL;


    gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
    {
        gxlist_del(&(info_list->list_head));
        Protocol_realease_node(info_list);  
        info_list = NULL;
    }
}

/* ================================PMT 相关======================================= */

//将是否PMT全部开启过滤封装成接口，返回值与Protocol_obj中的SearchTableGet一致
//注意如果返回1时表示继续使用这个资源过滤其他表，一定要注意释放控制块中保存的si句柄
static int32_t Protocol_pmt_si_reuse(SiEngineHandle si)
{
    uint32_t i;
    status_t ret;
    ProtocolPmtObj * pmtobj = NULL;
    if(SearchServiceObj.type == PROTOCOL_ATSC)
        pmtobj = &((GxAtscProtocolObj *)SearchServiceObj.protocol_obj)->pmt_obj_info;
    else
        pmtobj = &((GxDvbProtocolObj *)SearchServiceObj.protocol_obj)->pmt_obj_info;

    if(pmtobj->pmt_start_count < pmtobj->pmt_count)//如果pmt没有过滤完全
    {
        for(i = pmtobj->pmt_start_count;i<pmtobj->pmt_count;i++)
        {
            pmtobj->pmt_info[i].si_pmt = si;
            ret = pmtobj->pmt_create(&pmtobj->pmt_info[i],1);
            if(ret != GX_SEARCH_OK)
            { 
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---pmt reuse create failed\n");
#endif
                return 0;//理论情况下必定成功,所以不会进入这个判断
            }
            else
            {
                pmtobj->pmt_start_count++;
                return 1;//pmt没有过滤完全，复用此SI句柄
            }
        }
    }
    else
    {
        return 0;
    }
    return 1;
}



int32_t Protocol_pmt_timeout_deal(SiEngineHandle si)
{
    uint32_t value;
    void * tmp_protocol = NULL;
    ProtocolPmtObj * pmtobj = NULL;
    ProtocolVctObj * vctobj = NULL;
    ProtocolSdtObj * sdtobj = NULL;
    SearchLlist_info_t  * info_list = NULL; 
    SearchLlist_info_t  * tmp_node = NULL;
    PmtInfo_t * pmtinfo = NULL;
	SiEngineHandle tmp_si_handle = -1;
#ifdef GX_SEARCH_DEBUG
    GX_BUS_SEARCH_NORMAL_PRINTF("pmt time out\n");
#endif
    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    pmtinfo = (PmtInfo_t *)GxSubSystem_SiEngineGetUser(si);
    tmp_protocol = SearchServiceObj.protocol_obj;


    if((SearchServiceObj.initflag == 0)||(pmtobj->pmt_finish_count == pmtobj->pmt_count)||(tmp_protocol == NULL))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_NORMAL_PRINTF("pmt time out but search service already free\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }

    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
        tmp_si_handle = vctobj->si_vct;
    }
    else
    {
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
        sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
        tmp_si_handle = sdtobj->si_sdt;
    }


    pmtobj->pmt_finish_count++;
    value = Protocol_pmt_si_reuse(si);
    //所有pmt表过滤完全(2个方面：全部的PMT表申请过滤成功和全部PMT表都已经过滤到)
    if((value == 0))
    {
        pmtobj->pmt_realease(pmtinfo);
        //vct,pmt过滤完全
        if((tmp_si_handle == -1)&&(pmtobj->pmt_finish_count == pmtobj->pmt_count))
        {
            gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
            {
                search_merge(info_list->prog,searchclass);
                gxlist_del(&(info_list->list_head));
                Protocol_realease_node(info_list);
                info_list = NULL;
            }
            //当开启nit搜索时，需要判断nit是否已经收到再去锁下个频点
            if(searchclass->nit.nit_obj != NULL)
			{
				if(searchclass->nit.nit_obj->si_nit == -1)
				{    
					gx_search_variable_next_tp_init();
					if(searchclass->blindclass != NULL)
					{
						gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
					}
					else
					{
						gx_search_table_filter_start(searchclass);
					}
				}
			}
            else
			{
				gx_search_variable_next_tp_init();
				if(searchclass->blindclass != NULL)
				{
					gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
				}
				else
				{
					gx_search_table_filter_start(searchclass);
				}
			}
        }
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;//真正的table_ok
    }
    pmtinfo->si_pmt = -1;
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 1;//本来应该return 0，为了提高效率复用次si句柄，所以return 1
#if 0
err:
    pmtobj->pmt_realease(pmtinfo);
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 0;
#endif
}

void Protocol_pmt_realease(PmtInfo_t * pmt)
{
    if(pmt->si_pmt != -1)
    {
        GxSubSystem_SiEngineFree(pmt->si_pmt);
    }
    pmt->si_pmt = -1;
}

int32_t Protocol_pmt_get(SiEngineHandle si, void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParsePMT * pmt = (GxSubsystemSiParsePMT *)table;
    SearchLlist_info_t  * info_list = NULL; 
    SearchLlist_info_t  * tmp_node = NULL;
    int32_t exist_flag = 0,value = 0;
    status_t ret = 0;
    void * tmp_protocol = NULL;
    ProtocolPmtObj * pmtobj = NULL;
    ProtocolVctObj * vctobj = NULL;
    ProtocolSdtObj * sdtobj = NULL;
    void * prog = NULL;
	SiEngineHandle tmp_si_handle = -1;
	
    PmtInfo_t * pmtinfo =NULL;

    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    tmp_protocol = SearchServiceObj.protocol_obj;
    pmtinfo =(PmtInfo_t *)GxSubSystem_SiEngineGetUser(si);
    if((tmp_protocol == NULL) || SearchServiceObj.initflag == 0)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("pmt get but service free\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
		tmp_si_handle = vctobj->si_vct;
    }
    else
    {
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
        sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
        tmp_si_handle = sdtobj->si_sdt;
    }
    if((pmtobj->pmt_finish_count == pmtobj->pmt_count)||(pmt->table_id != PMT_TID))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("pmt get but service free\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }

    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }
    gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
    {
        if((pmt->program_number == info_list->program_number)&&(info_list->vct_sdt_flag == 1))

        {
            ret = add_pmt_info(info_list->prog,pmt,searchclass);
            if(ret != GX_SEARCH_OK)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[search] ----ATSC search add_pmt_info err\n");
#endif
                gxlist_del(&(info_list->list_head));
                Protocol_realease_node(info_list);  
                info_list = NULL;
                exist_flag = 1;
                goto table_ok;
                //goto err;
            }
            search_merge(info_list->prog,searchclass);
            gxlist_del(&(info_list->list_head));
            Protocol_realease_node(info_list);  
            info_list = NULL;
            exist_flag = 1;
            break;
        }
    }

    if((exist_flag == 0)&&(tmp_si_handle != -1))//在链表中没有找到相同program_number的节点并且vct还在过滤
    {
        info_list = GxCore_Mallocz(sizeof(SearchLlist_info_t));
        if(info_list == NULL)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("pmt atsc_search_info node malloc failed\n");
#endif
            goto table_ok;
        }
        if(SearchServiceObj.type == PROTOCOL_ATSC)
        {
            info_list->prog = GxCore_Mallocz(sizeof(AtscSearchNode_info_t));
        }
        else
        {
            info_list->prog = GxCore_Mallocz(sizeof(DvbSearchNode_info_t));
        }

        if(info_list->prog == NULL)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("pmt atsc_search info_list prog malloc failed\n");
#endif
            GxCore_Free(info_list);
            goto table_ok;
        }

        info_list->program_number = pmt->program_number;
        info_list->pmt_flag = 1;
        ret = add_pmt_info(info_list->prog,pmt,searchclass);
        if(ret != GX_SEARCH_OK)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[search] ----ATSC search add_pmt_info err\n");
#endif
            GxCore_Free(info_list->prog);
            GxCore_Free(info_list);
            goto table_ok;
        }

        gxlist_add_tail(&(info_list->list_head), &AtscSearch_add_info_list);
    }
    else if((exist_flag == 0)&&(tmp_si_handle == -1))
    {
        //vct已经过滤完，没有对应的节点，当前没有创建节点，所以免去链表操作
        if(SearchServiceObj.type == PROTOCOL_ATSC)
        {
            prog = GxCore_Mallocz(sizeof(AtscSearchNode_info_t));
        }
        else
        {
            prog = GxCore_Mallocz(sizeof(DvbSearchNode_info_t));
        }
        if(prog == NULL)
        {
#ifdef GX_SEARCH_DEBUG    
            GX_BUS_SEARCH_ERRO_PRINTF("[search] ----ATSC search prog malloc err\n");
#endif
            goto table_ok;
        }

        ret = add_pmt_info(prog,pmt,searchclass);
        if(ret != GX_SEARCH_OK)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("[search] ----ATSC search add_pmt_info err\n");
#endif
            GxCore_Free(prog);
            goto table_ok;
        }

        search_merge(prog,searchclass); 
        GxCore_Free(prog);
        prog = NULL;
    }

table_ok:
    if(status == TABLE_OK)
    {
        pmtobj->pmt_finish_count++;
        value = Protocol_pmt_si_reuse(si);
        //所有pmt表过滤完全(2个方面：全部的PMT表申请过滤成功和全部PMT表都已经过滤到)
        if((value == 0))
        {
            pmtobj->pmt_realease(pmtinfo);
            //vct,pmt过滤完全
            if((tmp_si_handle == -1)&&(pmtobj->pmt_finish_count == pmtobj->pmt_count))
            {
                gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
                {
                    search_merge(info_list->prog,searchclass);
                    gxlist_del(&(info_list->list_head));
                    Protocol_realease_node(info_list);
                    info_list = NULL;
                }
                //当开启nit搜索时，需要判断nit是否已经收到再去锁下个频点
                if(searchclass->nit.nit_obj != NULL)
                {
                    if(searchclass->nit.nit_obj->si_nit == -1)
					{    
						gx_search_variable_next_tp_init();
						if(searchclass->blindclass != NULL)
						{
							gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
						}
						else
						{
							gx_search_table_filter_start(searchclass);
						}
					}
                }
                else
				{
					gx_search_variable_next_tp_init();
					if(searchclass->blindclass != NULL)
					{
						gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
					}
					else
					{
						gx_search_table_filter_start(searchclass);
					}
				}

            }
            GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
            return 0;//真正的table_ok
        }
        pmtinfo->si_pmt = -1;
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;//本来应该return 0，为了提高效率复用次si句柄，所以return 1
    }
    else
    {
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
#if 0
err:
    //非正常情况退出,停止过滤
    pmtobj->pmt_realease(pmtinfo);
    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    return 0;
#endif
}


int32_t GetOriginalSection(SiEngineHandle si,uint8_t *section, uint32_t len,SiEngineParseStatus status)
{
	uint8_t tid = section[0];
	switch(tid)
	{
	case ADVANCE_SEARCH_PMT_TID:
		SearchServiceObj.searchclass.pmt_original_parse_fun(section,len);
		break;
	case ADVANCE_SEARCH_SDT_ACTUAL_TS_TID:
		SearchServiceObj.searchclass.sdt_original_parse_fun(section,len);
		break;
	default:
		break;
	}
	return 1;
}


status_t Protocol_pmt_create(PmtInfo_t * pmt , uint32_t flag)
{
    GxSubsystemSiEngineAlloc para = {0};
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPmtObj * pmtobj = NULL;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
    }
    else
    {
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
    }

    para.dmx_id = searchclass->search_demux_id;
    para.ts = searchclass->search_ts_cur;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = pmt->pid;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] = PMT_TID; 
	para.filter.mask[0] = 0xff;
    para.front_type = searchclass->front_type;
    para.p = (void *)pmt;

	if(searchclass->pmt_original_parse_fun)
	{
		para.original = GetOriginalSection;
	}
	else
	{
		para.original = NULL;
	}
    para.table = pmtobj->pmt_get;
    para.time_out =pmtobj->pmt_time_out_deal;
    para.sec_num_check = NULL;
    para.time_ms =pmtobj->pmt_time_out;

    if(flag == 0)
    {
        pmt->si_pmt = GxSubSystem_SiEngineAlloc(&para);
        if(pmt->si_pmt == -1)
        {
#ifdef GX_SEARCH_DEBUG
            GX_BUS_SEARCH_ERRO_PRINTF("search pmt alloc failed\n");
#endif
            return GX_SEARCH_ERR;
        }

        GxSubSystem_SiEngineStart(pmt->si_pmt);
    }
    else
    {
        GxSubSystem_SiEngineModify(pmt->si_pmt,&para);
    }

    return GX_SEARCH_OK;
}

/* ================================VCT 相关======================================= */
/* 调用此release函数有3个地方：1.exit_release;2.vct_get;3.vct_time_out_deal都加了全局锁，所以此函数不用加锁
 * */
void Protocol_vct_realease(SiEngineHandle si)
{
    ProtocolVctObj * vctobj = NULL;
    void * tmp_protocol = NULL;

    tmp_protocol = SearchServiceObj.protocol_obj;
    if((tmp_protocol == NULL)||(SearchServiceObj.initflag == 0))
    {
        return;
    }

    if(SearchServiceObj.type == PROTOCOL_ATSC)
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
    else
    {
        vctobj = NULL;
        return;
    }

    if(si != -1)
    {
        GxSubSystem_SiEngineFree(si);
    }
    vctobj->si_vct = -1;
}

int32_t Protocol_vct_timeout_deal(SiEngineHandle si)
{
    void * tmp_protocol = NULL;
    ProtocolVctObj * vctobj = NULL;
    ProtocolPmtObj * pmtobj = NULL;
    SearchLlist_info_t  * info_list = NULL; 
    SearchLlist_info_t  * tmp_node = NULL;
    uint32_t ret;

    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    tmp_protocol = SearchServiceObj.protocol_obj;
    if((tmp_protocol == NULL)||(SearchServiceObj.initflag == 0))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("vct time_out  but vct already freeed\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;

    }
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }


    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
    }

    if(vctobj->si_vct == -1)
    {
        goto err;
    }
    ret = Protocol_pmt_si_reuse(si);
    if(ret == 0)
    {
        vctobj->vct_realease(si);
        if(pmtobj->pmt_finish_count == pmtobj->pmt_count)
        {
            gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
            {
                search_merge(info_list->prog,searchclass);
                gxlist_del(&(info_list->list_head));
                Protocol_realease_node(info_list);
                info_list = NULL;
            }
            gx_search_variable_next_tp_init();
            gx_search_table_filter_start(searchclass);
        }
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 1;
err:
    vctobj->vct_realease(si);
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 0;
}

int32_t Protocol_vct_get(SiEngineHandle si,void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParseVCT * vct = (GxSubsystemSiParseVCT * )table;
    GxSubsystemSiParseVCTloop * loop = NULL;
    SearchLlist_info_t  * info_list = NULL; 
    SearchLlist_info_t * tmp_node = NULL;
    AtscSearchNode_info_t * prog = NULL;
    uint32_t i = 0,exist_flag = 0,ret = 0;
    void * tmp_protocol = NULL;
    ProtocolVctObj * vctobj = NULL;
    ProtocolPmtObj * pmtobj = NULL;


    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    
    tmp_protocol = SearchServiceObj.protocol_obj;
    if((tmp_protocol == NULL)||(SearchServiceObj.initflag == 0))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("vct get but search service already freed\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
    }

    if((vctobj->si_vct == -1)||(vct->table_id != T_VCT_TID))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("vct get but search service already freed\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    if(vct->num_channels_in_section>0)
    {
        for(i = 0; i<vct->num_channels_in_section ; i++)
        {
            exist_flag = 0;
            loop = &vct->loop[i];
            gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
            {
                if((loop->program_number == info_list->program_number)&&(info_list->pmt_flag == 1))
                {
                    ret = add_vct_info((AtscSearchNode_info_t *)info_list->prog,loop);
                    if(ret != GX_SEARCH_OK)
                    {
#ifdef GX_SEARCH_DEBUG
                        GX_BUS_SEARCH_ERRO_PRINTF("[search] ----ATSC search add_vct_info err\n");
#endif
                        goto err;
                    }
                    search_merge(info_list->prog,searchclass);
                    gxlist_del(&(info_list->list_head));
                    Protocol_realease_node(info_list);
                    info_list = NULL;
                    exist_flag = 1;
                    break;
                }
            }
            //在链表中没找到相同program_num的节点分2种情况，1.pmt没解析到;2.pat还没解析完或者还没过滤到
            //if((exist_flag == 0)&&((pmtobj->pmt_finish_count < pmtobj->pmt_count)&&(pmtobj->pmt_count > 0)))
            if((exist_flag == 0)&&((pmtobj->pmt_finish_count < pmtobj->pmt_count)))
            {
                //没有找到相同program_number的节点
                info_list = GxCore_Mallocz(sizeof(SearchLlist_info_t));
                if(info_list == NULL)
                {
#ifdef GX_SEARCH_DEBUG
                    GX_BUS_SEARCH_ERRO_PRINTF("vct atsc_search_info node malloc failed\n");
#endif
                    goto err;
                }
                info_list->prog = GxCore_Mallocz(sizeof(AtscSearchNode_info_t));
                if(info_list->prog == NULL)
                {
#ifdef GX_SEARCH_DEBUG
                    GX_BUS_SEARCH_ERRO_PRINTF("pmt atsc_search info_list prog malloc failed\n");
#endif
                }

                info_list->program_number = loop->program_number;
                info_list->vct_sdt_flag = 1;
                ret = add_vct_info((AtscSearchNode_info_t *)info_list->prog,loop);
                if(ret != GX_SEARCH_OK)
                {
#ifdef GX_SEARCH_DEBUG
                    GX_BUS_SEARCH_ERRO_PRINTF("[search] ----ATSC search add_vct_info err\n");
#endif
                    goto err;
                }
                gxlist_add_tail(&(info_list->list_head), &AtscSearch_add_info_list);
            }
            //else if((exist_flag == 0)&&((pmtobj->pmt_finish_count == pmtobj->pmt_count) &&(pmtobj->pmt_count > 0)))
            else if((exist_flag == 0)&&((pmtobj->pmt_finish_count == pmtobj->pmt_count)))
            {
                prog = GxCore_Mallocz(sizeof(AtscSearchNode_info_t));
                if(prog == NULL)
                {
#ifdef GX_SEARCH_DEBUG
                    GX_BUS_SEARCH_ERRO_PRINTF("[search] ---ATSC search vct prog malloc err\n");
#endif
                    goto err;
                }
                ret = add_vct_info((AtscSearchNode_info_t *)prog,loop);
                if(ret != GX_SEARCH_OK)
                {
#ifdef GX_SEARCH_DEBUG
                    GX_BUS_SEARCH_ERRO_PRINTF("[search] ----ATSC search add_vct_info err\n");
#endif
                    goto err;
                }
                
                search_merge(prog,searchclass);
                GxCore_Free(prog);
                prog = NULL;
            }
        }
    }

    if(status == TABLE_OK)
    {
        ret = Protocol_pmt_si_reuse(si);
        if(ret == 0)
        {
            vctobj->vct_realease(si);
            if(pmtobj->pmt_finish_count == pmtobj->pmt_count)
            {
                gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
                {
                    search_merge(info_list->prog,searchclass);
                    gxlist_del(&(info_list->list_head));
                    Protocol_realease_node(info_list);
                    info_list = NULL;
                }
                gx_search_variable_next_tp_init();
			    gx_search_table_filter_start(searchclass);
            }
            GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
            return 0;
        }
        vctobj->si_vct = -1;
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
    else
    {
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
err:
    vctobj->vct_realease(si);
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 0;
}

/*  
 *此函数的触发条件为search服务单一线程，所以不能在create时同时进行exit_release，无需加锁保护
 *  */
status_t Protocol_vct_create()
{
#define VCT_PID 0x1FFB
    GxSubsystemSiEngineAlloc para = {0};
    uint32_t tid = 0;
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    ProtocolVctObj * vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        if(searchclass->front_type == GX_SEARCH_FRONT_ATSC_T)
            tid = T_VCT_TID;
        else if(searchclass->front_type == GX_SEARCH_FRONT_ATSC_C)
            tid = C_VCT_TID;
    }
    else
    {
        return GX_SEARCH_ERR;
    }

    para.dmx_id = searchclass->search_demux_id;
    para.ts = searchclass->search_ts_cur;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = VCT_PID;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] = tid;
    para.filter.mask[0] = 0xff;
    para.front_type = searchclass->front_type;

    para.original = NULL;
    para.table = vctobj->vct_get;
    para.time_out = vctobj->vct_time_out_deal;
    para.sec_num_check = NULL;
    para.time_ms = vctobj->vct_time_out;

    vctobj->si_vct = GxSubSystem_SiEngineAlloc(&para);
    if(vctobj->si_vct == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search vct alloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    GxSubSystem_SiEngineStart(vctobj->si_vct);
    return GX_SEARCH_OK;
}

/* ================================PAT 相关======================================= */
int32_t Protocol_pat_time_out_deal(SiEngineHandle si)
{
	void * tmp_protocol = NULL;
	ProtocolPatObj * patobj = NULL;
	ProtocolPmtObj * pmtobj = NULL;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();

#ifdef GX_SEARCH_DEBUG
	GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---PAT search err\n");
#endif
	GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
	tmp_protocol = SearchServiceObj.protocol_obj;
	if((SearchServiceObj.initflag == 0)||(tmp_protocol == NULL))
	{
#ifdef GX_SEARCH_DEBUG
		GX_BUS_SEARCH_ERRO_PRINTF("pat time out but search service already stopped\n");
#endif
		goto out;
	}
	if(SearchServiceObj.type == PROTOCOL_ATSC)
	{
		patobj = &((GxAtscProtocolObj *)tmp_protocol)->pat_obj_info;
		pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
	}
	else
	{
		pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
		patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;
	}

	patobj->pat_realease(si);
	//当开启nit搜索时，需要判断nit是否已经收到再去锁下个频点
	if(searchclass->nit.nit_obj != NULL)
	{
		if(searchclass->nit.nit_obj->si_nit == -1)
		{
			gx_search_variable_next_tp_init();
			if(searchclass->blindclass != NULL)
			{
				gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
			}
			else
			{
				gx_search_table_filter_start(searchclass);
			}
		}
	}
	else
	{
		gx_search_variable_next_tp_init();
		if(searchclass->blindclass != NULL)
		{
			gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
		}
		else
		{
			gx_search_table_filter_start(searchclass);
		}
	}
out:
	GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
	return 0;
}

void Protocol_pat_realease(SiEngineHandle si)
{
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPatObj * patobj = NULL;
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        patobj = &((GxAtscProtocolObj *)tmp_protocol)->pat_obj_info;
    }
    else
    {
        patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;
    }

    if(si != -1)
    {
        GxSubSystem_SiEngineFree(si);
    }
    patobj->si_pat = -1;
}

int32_t Protocol_pat_get(SiEngineHandle si, void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParsePAT * pat = (GxSubsystemSiParsePAT *)table;
    GxSubsystemSiParseProgInfo * pmt_info = NULL;
    int32_t i = 0,value = 0;
    status_t ret = 0;
    void * tmp_protocol = NULL;
    ProtocolPatObj * patobj = NULL;
    ProtocolPmtObj * pmtobj = NULL;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();

    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    GxSearchClassObj * tmp_searchclass = gx_search_get_service_paramsptr();
    if(tmp_searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }    
    tmp_protocol = SearchServiceObj.protocol_obj;
    if((tmp_protocol == NULL)||(SearchServiceObj.initflag == 0))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search protocol already free\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }

    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        patobj = &((GxAtscProtocolObj *)tmp_protocol)->pat_obj_info;
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
    }
    else
    {
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
        patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;
    }

    if(pat->table_id != PAT_TID)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("pat filter all\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    
    tmp_searchclass->pat_version = pat->version_number;
    for(i = 0;i<pat->prog_count;i++)
    {
        pmt_info = &pat->prog_info[i];
		if(tmp_searchclass->check_pmtpid_fun != NULL)
		{
			if(tmp_searchclass->check_pmtpid_fun(pmt_info->pmt_pid,pmt_info->prog_num) == false)
				continue;
		}
        if(pmt_info->prog_num > 0)
        {
            pmtobj->pmt_count++;
            pmtobj->pmt_info = 
                GxCore_Realloc(pmtobj->pmt_info,pmtobj->pmt_count*sizeof(PmtInfo_t));
            if(pmtobj->pmt_info == NULL)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("pat get pmt_info realloc failed\n");
#endif
                goto err;
            }
            memset(&pmtobj->pmt_info[pmtobj->pmt_count - 1],0,sizeof(PmtInfo_t));

            pmtobj->pmt_info[pmtobj->pmt_count - 1].service_id = pmt_info->prog_num;
            pmtobj->pmt_info[pmtobj->pmt_count - 1].pid = pmt_info->pmt_pid;
            pmtobj->pmt_info[pmtobj->pmt_count - 1].si_pmt = -1;
            //gxlogd("pid :%d,network id:%d,prog_num:%d\n",pmt_info->pmt_pid,pmt_info->network_pid,pmt_info->prog_num);
        }
    }

    for(i = 0;i<pmtobj->pmt_count;i++)
    {
        ret =pmtobj->pmt_create(&pmtobj->pmt_info[i],0); 
        if(ret != GX_SEARCH_OK)
            break;
        else
            pmtobj->pmt_start_count++;
    }

    if(status == TABLE_OK)
    {
        //最高效率的使用filter
        value = Protocol_pmt_si_reuse(si);
        if(value == 0)
		{
			patobj->pat_realease(si);
			if (pat->prog_count == 0) {
				//当开启nit搜索时，需要判断nit是否已经收到再去锁下个频点
				if(searchclass->nit.nit_obj != NULL)
				{
					if(searchclass->nit.nit_obj->si_nit == -1)
					{    
						gx_search_variable_next_tp_init();
						if(searchclass->blindclass != NULL)
						{
							gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
						}
						else
						{
							gx_search_table_filter_start(searchclass);
						}
					}
				}
				else
				{
					gx_search_variable_next_tp_init();
					if(searchclass->blindclass != NULL)
					{
						gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
					}
					else
					{
						gx_search_table_filter_start(searchclass);
					}
				}
			}
			GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
			return 0;
		}
        patobj->si_pat = -1;
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
    else
    {
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
err:
    patobj->pat_realease(si);
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 0;
}


status_t Protocol_pat_create()
{
#define PAT_PID 0x00 
    GxSubsystemSiEngineAlloc para = {0};

    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolPatObj * patobj = NULL;
    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        patobj = &((GxAtscProtocolObj *)tmp_protocol)->pat_obj_info;
    }
    else
    {
        patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;
    }
     GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    para.dmx_id = searchclass->search_demux_id;
    para.ts = searchclass->search_ts_cur;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = PAT_PID;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] = PAT_TID;
    para.filter.mask[0] = 0xff;
    para.front_type = searchclass->front_type;

    para.original = NULL;
    para.table = patobj->pat_get;
    para.time_out = patobj->pat_time_out_deal;//PAT如果无法过滤成功，表示搜台无法继续,所以超时采用默认的停止处理
    para.sec_num_check = NULL;
    para.time_ms = patobj->pat_time_out;

    patobj->si_pat = GxSubSystem_SiEngineAlloc(&para);
    if(patobj->si_pat == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search pat alloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    GxSubSystem_SiEngineStart(patobj->si_pat);
    return GX_SEARCH_OK;
}


/* ================================SDT 相关======================================= */
void Protocol_sdt_realease(SiEngineHandle si)
{
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolSdtObj * sdtobj = NULL;
    sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;

    if(sdtobj->si_sdt != -1)
    {
        GxSubSystem_SiEngineFree(si);
    }
    sdtobj->si_sdt = -1;
}

int32_t Protocol_sdt_time_out_deal(SiEngineHandle si)
{
    void * tmp_protocol = NULL;
    uint32_t ret = 0;
    ProtocolSdtObj *sdtobj = NULL;
    ProtocolPmtObj *pmtobj = NULL;
	ProtocolPatObj *patobj = NULL;
    SearchLlist_info_t  * info_list = NULL; 
    SearchLlist_info_t * tmp_node = NULL;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }
    gxlogd("sdt_time_out_deal \n");
    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    tmp_protocol = SearchServiceObj.protocol_obj;
    if((SearchServiceObj.initflag == 0)||(tmp_protocol == NULL))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("sdt time out but service already free\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
    pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
    patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;

    ret = Protocol_pmt_si_reuse(si);
    if(ret == 0)
	{
		sdtobj->sdt_release(si);
		if((pmtobj->pmt_finish_count == pmtobj->pmt_count) && (patobj->si_pat == -1))
		{
			gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
			{
				search_merge(info_list->prog,searchclass);
				gxlist_del(&(info_list->list_head));
				Protocol_realease_node(info_list);
				info_list = NULL;
			}

			//当开启nit搜索时，需要判断nit是否已经收到再去锁下个频点
			if(searchclass->nit.nit_obj != NULL)
			{
				if(searchclass->nit.nit_obj->si_nit == -1)
				{    
					gx_search_variable_next_tp_init();
					//为什么这里set_status和gx_search_table_filter_start不能同时执行，需要好好思考
					if(searchclass->blindclass != NULL)
					{
						gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
					}
					else
					{
						gx_search_table_filter_start(searchclass);
					}
				}
			}
			else
			{
				gx_search_variable_next_tp_init();
				if(searchclass->blindclass != NULL)
				{
					gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
				}
				else
				{
					gx_search_table_filter_start(searchclass);
				}
			}

		}
		GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
		return 0;
	}
    sdtobj->si_sdt = -1;
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 1;
}

int32_t Protocol_sdt_get(SiEngineHandle si,void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParseSDT * sdt = (GxSubsystemSiParseSDT *)table;
    void * tmp_protocol = NULL;
    uint32_t i = 0,exist_flag = 0,ret = 0;
    SearchLlist_info_t * info_list = NULL;
    SearchLlist_info_t * tmp_node = NULL;
    DvbSearchNode_info_t * prog = NULL;
    GxSubsystemSiParseSdtInfo * sdt_info = NULL;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    tmp_protocol = SearchServiceObj.protocol_obj;
    if((SearchServiceObj.initflag == 0)||(tmp_protocol == NULL))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("sdt get but service already free\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    ProtocolSdtObj *sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
    ProtocolPmtObj *pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
	ProtocolPatObj *patobj = &((GxDvbProtocolObj *)tmp_protocol)->pat_obj_info;
    if(sdtobj->si_sdt == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("sdt get but service already free\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }

    SearchServiceObj.searchclass.original_network_id = sdt->original_network_id;
    SearchServiceObj.searchclass.sdt_version = sdt->version_number;
    for(i = 0;i < sdt->sdt_info_count ; i++)
    {
        sdt_info = &sdt->sdt_info[i];
        exist_flag = 0;
        gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
        {
            if((info_list->program_number == sdt_info->service_id)&&(info_list->pmt_flag == 1))
            {
                ret = add_sdt_info((DvbSearchNode_info_t *)info_list->prog,sdt_info);
                if(ret != GX_SEARCH_OK)
                {
#ifdef GX_SEARCH_DEBUG
                    GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] --- DVB sdt info add err\n");
#endif
                    goto err;
                }
                search_merge(info_list->prog,searchclass);
#if 0
                gxlogd("info_list:%d,%x,%x,%x\n",info_list->program_number,&info_list->list_head,
                        info_list->list_head.next,info_list->list_head.prev);
#endif
                gxlist_del(&(info_list->list_head));
                Protocol_realease_node(info_list);
                info_list = NULL;
                exist_flag = 1;
                break;
            }
        }
        if((exist_flag == 0) &&(pmtobj->pmt_finish_count < pmtobj->pmt_count))
        {
            info_list = GxCore_Mallocz(sizeof(SearchLlist_info_t));
            if(info_list == NULL)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---add sdt node malloc failed\n");
#endif
                goto err;
            }

            info_list->prog = GxCore_Mallocz(sizeof(DvbSearchNode_info_t));
            if(info_list->prog == NULL)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---add sdt node malloc failed\n");
#endif
                GxCore_Free(info_list);
                goto err;
            }
            info_list->vct_sdt_flag = 1;
            info_list->program_number = sdt_info->service_id;
            ret = add_sdt_info((DvbSearchNode_info_t *)info_list->prog,sdt_info);
            if(ret != GX_SEARCH_OK)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] ---add sdt info failed\n");
#endif
                GxCore_Free(info_list->prog);
                GxCore_Free(info_list);
                goto err;
            }
            gxlist_add_tail(&(info_list->list_head), &AtscSearch_add_info_list);
        }
        if((exist_flag == 0)&&(pmtobj->pmt_finish_count == pmtobj->pmt_count))
        {
            prog = GxCore_Mallocz(sizeof(DvbSearchNode_info_t));
            if(prog == NULL)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] --- sdt merge failed\n");
#endif
                goto err;
            }
            ret = add_sdt_info((DvbSearchNode_info_t *)prog,sdt_info);
            if(ret != GX_SEARCH_OK)
            {
#ifdef GX_SEARCH_DEBUG
                GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH] --- add sdt info failed\n");
#endif
                GxCore_Free(prog);
                goto err;
            }
            search_merge((void *)prog,searchclass); 
            GxCore_Free(prog); prog = NULL; 
        } 
    } 
    if(status == TABLE_OK) {
        ret = Protocol_pmt_si_reuse(si);
        if(ret == 0)
        {
            sdtobj->sdt_release(si);
            if((pmtobj->pmt_finish_count == pmtobj->pmt_count) && patobj->si_pat == -1)
			{
				gxlist_for_each_entry_safe(info_list,tmp_node,&AtscSearch_add_info_list,list_head)
				{
					//search_merge(info_list->prog);
					gxlist_del(&(info_list->list_head));
					Protocol_realease_node(info_list);
					info_list = NULL;
				}


				//当开启nit搜索时，需要判断nit是否已经收到再去锁下个频点
				if(searchclass->nit.nit_obj != NULL)
				{
					if(searchclass->nit.nit_obj->si_nit == -1)
					{    
						gx_search_variable_next_tp_init();
						if(searchclass->blindclass != NULL)
						{
							gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
						}
						else
						{
							gx_search_table_filter_start(searchclass);
						}
					}
				}
				else
				{
					gx_search_variable_next_tp_init();
					if(searchclass->blindclass != NULL)
					{
						gx_blind_set_status(SEARCH_RUNNING,searchclass->blindclass);
					}
					else
					{
						gx_search_table_filter_start(searchclass);
					}
				}
			}
            GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
            return 0;
        }
        sdtobj->si_sdt = -1;
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
    else
    {
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
err:
    sdtobj->sdt_release(si);
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 0;
}

status_t Protocol_sdt_create()
{
#define SDT_PID                 (0x11)
#define SDT_ACTUAL_TS_TID       (0x42)
    GxSubsystemSiEngineAlloc para = {0};
    void * tmp_protocol = SearchServiceObj.protocol_obj;
    ProtocolSdtObj * sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
    GxSearchClassObj * searchclass = gx_search_get_service_paramsptr();
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }


    para.dmx_id = searchclass->search_demux_id;
    para.ts = searchclass->search_ts_cur;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = SDT_PID;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] = SDT_ACTUAL_TS_TID;
    para.filter.mask[0] = 0xff;
    para.front_type = searchclass->front_type;

	if(searchclass->sdt_original_parse_fun)
	{
		para.original = GetOriginalSection;
	}
	else
	{
		para.original = NULL;
	}
    para.table = sdtobj->sdt_get;
    para.time_out = sdtobj->sdt_time_out_deal;
    para.sec_num_check = NULL;
    para.time_ms = sdtobj->sdt_time_out;

    sdtobj->si_sdt = GxSubSystem_SiEngineAlloc(&para);
    if(sdtobj->si_sdt == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search sdt alloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    GxSubSystem_SiEngineStart(sdtobj->si_sdt);
    return GX_SEARCH_OK;
}

/* ================================NIT 相关======================================= */
/* 
 *当开启nit搜索时，每一个频点进行一次nit搜索，所以一次nit finish不能释放nitobj，需要在exit release中去释放
 * */
void Protocol_nit_release(SiEngineHandle si)
{
    if(si != -1)
    {
        GxSubSystem_SiEngineFree(si);
    }
    SearchServiceObj.searchclass.nit.nit_obj->si_nit = -1;
}

int32_t Protocol_nit_time_out_deal(SiEngineHandle si)
{
    void * tmp_protocol = NULL;
    SiEngineHandle tmp_si_handle = -1;
    ProtocolPmtObj * pmtobj = NULL;
    ProtocolVctObj * vctobj = NULL;
    ProtocolSdtObj * sdtobj = NULL;
    uint32_t value;


    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    GxSearchClassObj * search_class = gx_search_get_service_paramsptr();
    if(search_class == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }
    tmp_protocol = SearchServiceObj.protocol_obj;
    if(SearchServiceObj.initflag == 0)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("nit time_out  but nit already freeed\n");
#endif
        goto err;
    }
    if(search_class->nit.nit_obj == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("nit time_out  but nit already freeed\n");
#endif
        goto err;
    }

    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
        tmp_si_handle = vctobj->si_vct;
    }
    else
    {
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
        sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
        tmp_si_handle = sdtobj->si_sdt;
    }

    value = Protocol_pmt_si_reuse(si);
    //所有pmt表过滤完全(2个方面：全部的PMT表申请过滤成功和全部PMT表都已经过滤到)
    if((value == 0))
    {
        //vct,pmt过滤完全
        search_class->nit.nit_obj->nit_realsease(si);
        if((tmp_si_handle == -1)&&(pmtobj->pmt_finish_count == pmtobj->pmt_count))
        {
            gx_search_variable_next_tp_init();
            gx_search_table_filter_start(search_class);
        }
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;//真正的table_ok
    }
    else
    {
        search_class->nit.nit_obj->si_nit = -1;;
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
err:
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 0;
}

int32_t Protocol_nit_get(SiEngineHandle si,void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParseNIT * nit = (GxSubsystemSiParseNIT *)table;    
    GxSubsystemSiParseNitLoop2 * loop = NULL;
    GxBusPmDataTP tp={0};
    int32_t i = 0,j = 0,ret = 0;
    int32_t nit_loop = 0;
    int32_t tp_exist_count = 0;
    uint16_t tp_id = 0;
    void * tmp_protocol = NULL;
    SiEngineHandle tmp_si_handle = -1;
    ProtocolPmtObj * pmtobj = NULL;
    ProtocolVctObj * vctobj = NULL;
    ProtocolSdtObj * sdtobj = NULL;
    uint32_t value;
    GxSearchClassObj * search_class = gx_search_get_service_paramsptr();
    if(search_class == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }


    GxCore_MutexLock(SearchServiceObj.searchclass.list_mutex);
    if((SearchServiceObj.initflag == 0) || (SearchServiceObj.searchclass.nit.nit_obj == NULL))
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("nit get but nit already freed\n");
#endif
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 0;
    }
    tmp_protocol = SearchServiceObj.protocol_obj;

    if(SearchServiceObj.type == PROTOCOL_ATSC)
    {
        pmtobj = &((GxAtscProtocolObj *)tmp_protocol)->pmt_obj_info;
        vctobj = &((GxAtscProtocolObj *)tmp_protocol)->vct_obj_info;
        tmp_si_handle = vctobj->si_vct;
    }
    else
    {
        pmtobj = &((GxDvbProtocolObj *)tmp_protocol)->pmt_obj_info;
        sdtobj = &((GxDvbProtocolObj *)tmp_protocol)->sdt_obj_info;
        tmp_si_handle = sdtobj->si_sdt;
    }

    for(i = 0 ; i<nit->loop2_count ; i++)
    {
        loop = &nit->loop2[i];
        switch(SearchServiceObj.searchclass.front_type)
        {
            case GX_SEARCH_FRONT_DVB_S: 
                for(j = 0;j < loop->satellite_delivery_system_count;j++)
                {
                    tp.frequency = loop->satellite_delivery_system[j].frequency;
                    //BCD码再使用时需要转码
                    tp.frequency = GET_DEC32(tp.frequency)/100;
                    if(tp.frequency>13450 || tp.frequency<3000)
                    {
                        continue;
                    }
                    tp.sat_id = search_class->search_sat_id_for_prog;
                    tp.tp_s.symbol_rate = loop->satellite_delivery_system[j].symbol_rate;
                    //BCD码再使用时需要转码
                    tp.tp_s.symbol_rate = GET_DEC32(tp.tp_s.symbol_rate)/10;
                    if((loop->satellite_delivery_system[j].polarization == 0) || 
                            (loop->satellite_delivery_system[j].polarization == 2))
                    {
                        tp.tp_s.polar = GXBUS_PM_TP_POLAR_H;
                    }
                    else if((loop->satellite_delivery_system[j].polarization == 1) || 
                            (loop->satellite_delivery_system[j].polarization == 3))
                    {
                        tp.tp_s.polar = GXBUS_PM_TP_POLAR_V;
                    }
                    else
                    {
                        tp.tp_s.polar = GXBUS_PM_TP_POLAR_H;
                    }
                    if(SearchServiceObj.searchclass.nit.nit_tp_private_check != NULL)
                    {
                        tp_exist_count = SearchServiceObj.searchclass.nit.nit_tp_private_check(tp.sat_id,
                                tp.frequency,tp.tp_s.symbol_rate,tp.tp_s.polar); 
                    }
                    else
                    {
                        tp_exist_count = GxBus_PmTpExistChek(tp.sat_id, 
                                tp.frequency, 
                                tp.tp_s.symbol_rate,
                                tp.tp_s.polar,
                                0,
                                &tp_id);
                    }


                    GxBus_ConfigGetInt(GXBUS_SEARCH_NIT_LOOP, &nit_loop,GXBUS_SEARCH_NIT_LOOP_YES);
                    if(nit_loop == GXBUS_SEARCH_NIT_LOOP_NO && tp_exist_count != 0)
                    {
                        break;
                    }
                    else
                    {
                        if(tp_exist_count == 0)
                        {
                            ret = GxBus_PmTpAdd(&tp);
                        }
                        else
                        {
                            tp.id = tp_id;
                            ret = GXCORE_SUCCESS;
                        }
                        if(ret == GXCORE_SUCCESS)
                        {
                            search_class->nit.search_nit_tp_id[search_class->nit.search_nit_tp_count] = tp.id;
                            search_class->nit.search_nit_tp_count++;
                            break;
                        }
                        else if(ret == GX_PM_DBASE_FULL)
                        {
#ifdef GX_BUS_SEARCH_DBUG
                            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---nit tp save full!!\n");
#endif
                            goto err;
                        }
                        else if(ret == GXCORE_ERROR)
                        {
#ifdef GX_BUS_SEARCH_DBUG
                            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---nit tp save err!!\n");
#endif
                            goto err;
                        }
                    }
                }
                break;
            case GX_SEARCH_FRONT_DVB_T:
                for(j = 0;j < loop->terrestrial_delivery_system_count;j++)
                {
                    tp.frequency =loop->terrestrial_delivery_system[j].centre_frequency;
                    tp.sat_id = search_class->search_sat_id_for_prog;
                    if(loop->terrestrial_delivery_system[j].banwidth == 0)
                    {
                        tp.tp_t.bandwidth = BANDWIDTH_8_MHZ;
                    }
                    else if(loop->terrestrial_delivery_system[j].banwidth == 1)
                    {
                        tp.tp_t.bandwidth = BANDWIDTH_7_MHZ;
                    }
                    else
                    {
                        tp.tp_t.bandwidth = BANDWIDTH_8_MHZ;
                    }
                    tp_exist_count = GxBus_PmTpExistChek(tp.sat_id, 
                            tp.frequency, 
                            0,
                            0,
                            0,
                            &tp_id);
                    if(tp_exist_count == 0)
                    {
                        ret = GxBus_PmTpAdd(&tp);
                        if(ret == GXCORE_SUCCESS)
                        {
                            search_class->nit.search_nit_tp_id[search_class->nit.search_nit_tp_count] = tp.id;
                            search_class->nit.search_nit_tp_count++;
                            break;
                        }
                        else if(ret == GX_PM_DBASE_FULL)
                        {
#ifdef GX_BUS_SEARCH_DBUG
                            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---nit tp save full!!\n");
#endif
                            goto err;
                        }
                        else if(ret == GXCORE_ERROR)
                        {
#ifdef GX_BUS_SEARCH_DBUG
                            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---nit tp save err!!\n");
#endif
                            //GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
                            goto err;
                        }
                    }
                }
                break;
            case GX_SEARCH_FRONT_DVB_C:
                for(j = 0;j < loop->cable_delivery_system_count;j++)
                {
                    tp.frequency = loop->cable_delivery_system[j].frequency;
                    tp.sat_id = search_class->search_sat_id_for_prog;
                    tp.tp_c.symbol_rate = loop->cable_delivery_system[j].symbol_rate;

                    if(loop->cable_delivery_system[j].modulation == 0x1)
                    {
                        tp.tp_c.modulation= DVBC_16QAM;
                    }
                    else if(loop->cable_delivery_system[j].modulation == 0x2)
                    {
                        tp.tp_c.modulation= DVBC_32QAM;
                    }
                    else if(loop->cable_delivery_system[j].modulation == 0x3)
                    {
                        tp.tp_c.modulation= DVBC_64QAM;
                    }
                    else if(loop->cable_delivery_system[j].modulation == 0x4)
                    {
                        tp.tp_c.modulation= DVBC_128QAM;
                    }
                    else if(loop->cable_delivery_system[j].modulation == 0x5)
                    {
                        tp.tp_c.modulation= DVBC_256QAM;
                    }
                    else
                    {
                        tp.tp_c.modulation= DVBC_32QAM;
                    }
                    tp_exist_count = GxBus_PmTpExistChek(tp.sat_id, 
                            tp.frequency, 
                            tp.tp_c.symbol_rate,
                            0,
                            tp.tp_c.modulation,
                            &tp_id);
                    if(tp_exist_count == 0)
                    {
                        ret = GxBus_PmTpAdd(&tp);
                        if(ret == GXCORE_SUCCESS)
                        {
                            search_class->nit.search_nit_tp_id[search_class->nit.search_nit_tp_count] = tp.id;
                            search_class->nit.search_nit_tp_count++;
                            break;
                        }
                        else if(ret == GX_PM_DBASE_FULL)
                        {
#ifdef GX_BUS_SEARCH_DBUG
                            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---nit tp save full!!\n");
#endif
                            //GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
                            goto err;
                        }
                        else if(ret == GXCORE_ERROR)
                        {
#ifdef GX_BUS_SEARCH_DBUG
                            GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---nit tp save err!!\n");
#endif
                            //GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
                            goto err;
                        }
                    }
                    else
                    {
                        search_class->nit.search_nit_tp_id[search_class->nit.search_nit_tp_count] = tp_id;
                        search_class->nit.search_nit_tp_count++;
                    }
                }
                break;
            case GX_SEARCH_FRONT_DVB_DTMB:
                break;
            default:
                break;
        }
    }

    if(status == TABLE_OK)
    {
        value = Protocol_pmt_si_reuse(si);
        //所有pmt表过滤完全(2个方面：全部的PMT表申请过滤成功和全部PMT表都已经过滤到)
        if((value == 0))
        {
            search_class->nit.nit_obj->nit_realsease(si);
            //vct,pmt过滤完全
            if((tmp_si_handle == -1)&&(pmtobj->pmt_finish_count == pmtobj->pmt_count))
            {
                gx_search_variable_next_tp_init();
                gx_search_table_filter_start(search_class);
            }
            GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
            return 0;//真正的table_ok
        }
        search_class->nit.nit_obj->si_nit = -1;
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
    else
    {
        GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
        return 1;
    }
err:
    search_class->nit.nit_obj->nit_realsease(si);
    GxCore_MutexUnlock(SearchServiceObj.searchclass.list_mutex);
    return 0;
}


status_t Protocol_nit_create(void * p)
{
#define NIT_PID                     (0x0010)
#define NIT_ACTUAL_NETWORK_TID      (0x40)
    GxSubsystemSiEngineAlloc para = {0};

    GxSearchClassObj * searchclass = (GxSearchClassObj *)p;
    if(searchclass == NULL)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("gx_search_table_filter_start malloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }


    para.dmx_id = searchclass->search_demux_id;
    para.ts = searchclass->search_ts_cur;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = NIT_PID;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] =  NIT_ACTUAL_NETWORK_TID;
    para.filter.mask[0] = 0xff;
    para.front_type = searchclass->front_type;
    para.p = (void *)searchclass->nit.nit_obj;

    para.original = NULL;
    para.table = searchclass->nit.nit_obj->nit_get;
    para.time_out = searchclass->nit.nit_obj->nit_time_out_deal;
    para.sec_num_check = NULL;
    para.time_ms = searchclass->nit.nit_obj->nit_time_out;

    searchclass->nit.nit_obj->si_nit = GxSubSystem_SiEngineAlloc(&para);
    if(searchclass->nit.nit_obj->si_nit == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search sdt alloc failed\n");
#endif
        return GX_SEARCH_ERR;
    }

    GxSubSystem_SiEngineStart(searchclass->nit.nit_obj->si_nit);
    return GX_SEARCH_OK;
}
