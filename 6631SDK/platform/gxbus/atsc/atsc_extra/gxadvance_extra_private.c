/* ****************************************************************************
 * *                          CONFIDENTIAL                             
 * *        Hangzhou GuoXin Science and Technology Co., Ltd.             
 * *                      (C)2009, All right reserved
 * ******************************************************************************
 *
 * ******************************************************************************
 * * File Name :   gxadvance_extra.c
 * * Author    :   xiahuangshuai
 * * Project   :   GoXceed 
 * * Type      :   
 * ******************************************************************************
 * * Purpose   :   
 * ******************************************************************************
 * * Release History:
 *   VERSION   Date              AUTHOR         Description
 *  0.0      2014.9.9        xiahuangshuai            creation
 ******************************************************************************/
#include "include/gxadvance_extra_private.h"


void gxadvance_extra_protocol_dvb_release()
{
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;
    
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    
    if(tmp_protocol->tot_obj_info.si_tot != -1)
    {
        GxSubSystem_SiEngineFree(tmp_protocol->tot_obj_info.si_tot);
        tmp_protocol->tot_obj_info.si_tot = -1;
    }

    if(tmp_protocol->tdt_obj_info.si_tdt != -1)
    {
        GxSubSystem_SiEngineFree(tmp_protocol->tdt_obj_info.si_tdt);
        tmp_protocol->tdt_obj_info.si_tdt = -1;
    }
    
    GxCore_Free(AdvanceExtraServiceObj.protocol_obj);
    AdvanceExtraServiceObj.protocol_obj = NULL;
}

void gxadvance_extra_protocol_atsc_release()
{
    GxAdvanceExtraAtscProtocol * tmp_protocol = NULL;

    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;

    if(tmp_protocol->stt_obj_info.si_stt != -1)
    {
        GxSubSystem_SiEngineFree(tmp_protocol->stt_obj_info.si_stt);
        tmp_protocol->stt_obj_info.si_stt = -1;
    }
    GxCore_Free(AdvanceExtraServiceObj.protocol_obj);
    AdvanceExtraServiceObj.protocol_obj = NULL;
}

void extra_release_resource()
{
    GxCore_MutexLock(AdvanceExtraServiceObj.mutex);
    if(AdvanceExtraServiceObj.protocol == PROTOCOL_DVB)
    {
        gxadvance_extra_protocol_dvb_release();
    }else if(AdvanceExtraServiceObj.protocol == PROTOCOL_ATSC) 
    {
        gxadvance_extra_protocol_atsc_release();
    }
    GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
}


uint32_t gxadvance_extra_protocol_dvb_init()
{
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;

    AdvanceExtraServiceObj.protocol_obj = GxCore_Mallocz(sizeof(GxAdvanceExtraDvbProtocol));
    if(AdvanceExtraServiceObj.protocol_obj == NULL)
    {
#ifdef GX_ADVANCE_EXTRA_ERR_DEBUG
        ADVANCE_EXTRA_ERR_PRINTF("advance extra dvb protocol malloc failed\n");
#endif
        return false;
    }
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    
    tmp_protocol->tot_obj_info.tot_create = Protocol_tot_create;
    tmp_protocol->tot_obj_info.tot_release = Protocol_tot_release;
    tmp_protocol->tot_obj_info.tot_get = Protocol_tot_get;
    tmp_protocol->tot_obj_info.tot_time_out_deal = Protocol_tot_time_out_deal;

    tmp_protocol->tdt_obj_info.tdt_create = Protocol_tdt_create;
    tmp_protocol->tdt_obj_info.tdt_release = Protocol_tdt_release;
    tmp_protocol->tdt_obj_info.tdt_get = Protocol_tdt_get;
    tmp_protocol->tdt_obj_info.tdt_time_out_deal = Protocol_tdt_time_out_deal;

    return true;
}

uint32_t gxadvance_extra_protocol_atsc_init()
{
    GxAdvanceExtraAtscProtocol * tmp_protocol = NULL;

    AdvanceExtraServiceObj.protocol_obj = GxCore_Mallocz(sizeof(GxAdvanceExtraAtscProtocol));
    if(AdvanceExtraServiceObj.protocol_obj == NULL)
    {
#ifdef GX_ADVANCE_EXTRA_ERR_DEBUG
        ADVANCE_EXTRA_ERR_PRINTF("advance extra atsc protocol malloc failed\n");
#endif
        return false;
    }
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    
    tmp_protocol->stt_obj_info.stt_create = Protocol_stt_create;
    tmp_protocol->stt_obj_info.stt_release = Protocol_stt_release;
    tmp_protocol->stt_obj_info.stt_get = Protocol_stt_get;
    tmp_protocol->stt_obj_info.stt_time_out_deal = Protocol_stt_time_out_deal;

    return true;
}

void gxadvance_extra_start_filter(uint32_t ts_id,uint32_t dmx_id)
{
    if(AdvanceExtraServiceObj.protocol == PROTOCOL_DVB)
    {
        ((GxAdvanceExtraDvbProtocol *)AdvanceExtraServiceObj.protocol_obj)->tot_obj_info.tot_create(ts_id,dmx_id);
        ((GxAdvanceExtraDvbProtocol *)AdvanceExtraServiceObj.protocol_obj)->tdt_obj_info.tdt_create(ts_id,dmx_id);
    }
    else
    {
        ((GxAdvanceExtraAtscProtocol *)AdvanceExtraServiceObj.protocol_obj)->stt_obj_info.stt_create(ts_id,dmx_id);
    }

}

void extra_sync_time(GxMsgProperty_ExtraSyncTime * sync_time)
{
    uint32_t ret = 0;
    AdvanceExtraServiceObj.protocol = sync_time->dmx_protocol.protocol;

    if(sync_time->dmx_protocol.protocol == PROTOCOL_DVB)
    {
        AdvanceExtraServiceObj.protocol = PROTOCOL_DVB;
        ret = gxadvance_extra_protocol_dvb_init();
    }
    else if(sync_time->dmx_protocol.protocol == PROTOCOL_ATSC)
    {
        AdvanceExtraServiceObj.protocol = PROTOCOL_ATSC;
        ret = gxadvance_extra_protocol_atsc_init();
    }

    if(ret == 0)
    {
#ifdef GX_ADVANCE_EXTRA_ERR_DEBUG
        ADVANCE_EXTRA_ERR_PRINTF("advance extra protocol init err\n");
#endif
        while(1);
    }
    else
    {
        gxadvance_extra_start_filter(sync_time->ts_src,sync_time->dmx_protocol.dmx_id);
    }
}
