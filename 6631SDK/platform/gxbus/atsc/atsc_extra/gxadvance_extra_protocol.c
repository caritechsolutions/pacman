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

/******************include******************************/
#include "include/gxadvance_extra_private.h"
#include "gxmsg.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_tot.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_stt.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_tdt.h"
#define TOT_TIME_OUT_VALUE      (300000)
#define TDT_TIME_OUT_VALUE      (300000)
#define STT_TIME_OUT_VALUE      (300000)
#define TOT_PID     (0x14)
#define TDT_PID     (0x14)
#define STT_PID     (0x1FFB)
#define TDT_TID     (0x70)
#define TOT_TID     (0x73)
#define STT_TID     (0xcd)
#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))
#define TODATA0_15BIT(data1,data2)              ((data1<<8 )|data2)
#define TODATA0_23BIT(data1,data2,data3)        ((data1<<16)|(data2<<8)|data3)
#define TODATA0_7BIT(data)                      ((data>>8)&0xff)


/*  *****************function******************************/
static void  extra_mjd_to_tm(uint16_t mjd, struct tm *tm)
{
	uint8_t IK = 0;
	
	tm->tm_year = (uint32_t)( (mjd -15078.2) /365.25 );
	tm->tm_mon = (uint32_t)( (mjd - 14956.1 - (int32_t)(tm->tm_year * 365.25) ) 
																/30.6001);
	tm->tm_mday = (uint32_t)(mjd -14956 - (int32_t)(tm->tm_year * 365.25) 
										- (int32_t)(tm->tm_mon * 30.6001));
	
	if(tm->tm_mon == 14 || tm->tm_mon == 15)
	{
		IK = 1;
	}
	else 
	{
		IK = 0;
	}

	tm->tm_year += (IK+1900);
	tm->tm_mon = tm->tm_mon - 1 - IK*12;
}

static void extra_bcd_to_tm(uint32_t bcd_time, struct tm *tm)
{

	tm->tm_sec = BCD2DEC(bcd_time&0xff);
	tm->tm_min = BCD2DEC((bcd_time>>8)&0xff);
	tm->tm_hour = BCD2DEC((bcd_time>>16)&0xff);
}

static void gx_advance_extra_time_ok_reply(GxMsgProperty_ExtraTimeOk * time_ok)
{
    GxMessage *msg;
    GxMsgProperty_ExtraTimeOk * p_time_ok = NULL;

    msg = GxBus_MessageNew(GXMSG_EXTRA_SYNC_TIME_OK);
    p_time_ok = (GxMsgProperty_ExtraTimeOk*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_ExtraTimeOk);
    memcpy(p_time_ok,time_ok,sizeof(GxMsgProperty_ExtraTimeOk));
    if(time_ok->time_zone == 0)
    {
        p_time_ok->time_zone = 0xfffffff;//int型，7个F代表没有时区信息，用户需要手动设置时区
    }
    
    gxlogd("gx_advance_extra_time_ok_reply\n");
    GxBus_MessageSend(msg);
}

/*  *****************tot相关******************************/
void Protocol_tot_release(SiEngineHandle si)
{
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;

    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    if(tmp_protocol->tot_obj_info.si_tot != -1)
    {
        GxSubSystem_SiEngineFree(si);
        tmp_protocol->tot_obj_info.si_tot = -1;
    }
}

int32_t Protocol_tot_time_out_deal(SiEngineHandle si)
{
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;


    GxCore_MutexLock(AdvanceExtraServiceObj.mutex); 
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    if(tmp_protocol->tot_obj_info.si_tot != -1)
    {
        tmp_protocol->tot_obj_info.tot_release(si);
    }
    GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
    return 0;
}

int32_t Protocol_tot_get(SiEngineHandle si, void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParseTOT *tot = (GxSubsystemSiParseTOT *)table; 
    int32_t i = 0,j = 0;
    uint32_t now_seconds = 0,change_seconds = 0,utc_time = 0,utc_mjd = 0;
    struct tm tm_time;
    int16_t timeoffset = 0;
    int8_t timeoffsethour = 0;
    GxMsgProperty_ExtraTimeOk tot_time_ok = {0};
    GxSubsystemSiParseLocalTimeOfffset * time_offset = NULL;
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;
    

    GxCore_MutexLock(AdvanceExtraServiceObj.mutex);
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    utc_mjd = TODATA0_15BIT(tot->UTC_time[0],tot->UTC_time[1]);
    utc_time = TODATA0_23BIT(tot->UTC_time[2],tot->UTC_time[3],tot->UTC_time[4]);
    extra_mjd_to_tm((uint16_t)utc_mjd,&tm_time);
    extra_bcd_to_tm(utc_time,&tm_time);
    tm_time.tm_mon -= 1;
    tm_time.tm_year -= 1900;
    now_seconds = mktime(&tm_time);
    
    /* 多个循环的意义是什么 */
    for(i = 0;i<tot->local_time_des_count;i++)
    {
        for(j = 0;j<tot->local_time_des[i].time_offset_count;j++)
        {
            time_offset = &tot->local_time_des[i].time_offset[j];
            /* 
             *在标准的tot表中local_time_offset表示16位的时间偏移，高8位表示小时偏移，低8位表示分偏移，目前使用中只需要计算小时便宜
             *所以先取高8位，然后将BCD码进行转换
             * */
            timeoffsethour =TODATA0_7BIT(time_offset->local_time_offset);
            timeoffsethour =BCD2DEC(timeoffsethour);

            if(time_offset->local_time_offset_polarity == 1)
            {
                timeoffset -= timeoffsethour;
            }
            else
            {
                timeoffset = timeoffsethour;
            }
            utc_mjd = TODATA0_15BIT(time_offset->time_of_change[0],time_offset->time_of_change[1]);
            utc_time = TODATA0_23BIT(time_offset->time_of_change[2],time_offset->time_of_change[3],time_offset->time_of_change[4]);
            extra_mjd_to_tm((uint16_t)utc_mjd,&tm_time);
            extra_bcd_to_tm(utc_time,&tm_time);
            tm_time.tm_mon -= 1;
            tm_time.tm_year -= 1900;
            change_seconds = mktime(&tm_time);
            /* 
             *原来根据now_seconds > change_seconds比较，但是根据协议UTC时间变化时下一个时间偏移，感觉应该用!=做判断，需要验证
             * */
            if(now_seconds != change_seconds)
            {
                timeoffset = 0;
                timeoffsethour = TODATA0_7BIT(time_offset->local_time_offset);
                timeoffsethour =  BCD2DEC(timeoffsethour);

                if(time_offset->local_time_offset_polarity == 1)
                {
                    timeoffset -= timeoffsethour;
                }
                else
                {
                    timeoffset = timeoffsethour;
                }
            }
        }
    }
    tot_time_ok.utc_second = now_seconds;
    tot_time_ok.time_zone = timeoffset;
    tot_time_ok.time_fountain = TOT_TIME;
    gx_advance_extra_time_ok_reply(&tot_time_ok);

    if(status == TABLE_OK)
    {
        tmp_protocol->tot_obj_info.tot_release(si);
        GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
        return 0;
    }
    else
    {
        GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
        return 1;
    }
}

int32_t Protocol_tot_create(uint32_t ts_id,uint32_t dmx_id)
{
    GxSubsystemSiEngineAlloc para = {0};
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;

    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;

    para.dmx_id = dmx_id;
    para.ts = ts_id;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = TOT_PID;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] = TOT_TID; 
    para.filter.mask[0] = 0xff;

    para.table = tmp_protocol->tot_obj_info.tot_get;
    para.time_out = tmp_protocol->tot_obj_info.tot_time_out_deal;
    para.sec_num_check = NULL;
    para.time_ms = TOT_TIME_OUT_VALUE;

    tmp_protocol->tot_obj_info.si_tot = GxSubSystem_SiEngineAlloc(&para);
    if(tmp_protocol->tot_obj_info.si_tot == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search pmt alloc failed\n");
#endif
        return false;
    }

    GxSubSystem_SiEngineStart(tmp_protocol->tot_obj_info.si_tot);

    return true;
}

/*  *****************tdt相关******************************/
void Protocol_tdt_release(SiEngineHandle si)
{
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;

    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    if(tmp_protocol->tdt_obj_info.si_tdt != -1)
    {
        GxSubSystem_SiEngineFree(si);
        tmp_protocol->tdt_obj_info.si_tdt = -1;
    }
}

int32_t Protocol_tdt_time_out_deal(SiEngineHandle si)
{
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;


    GxCore_MutexLock(AdvanceExtraServiceObj.mutex); 
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    if(tmp_protocol->tdt_obj_info.si_tdt != -1)
    {
        tmp_protocol->tdt_obj_info.tdt_release(si);
    }
    GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
    return 0;
}

int32_t Protocol_tdt_get(SiEngineHandle si, void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParseTDT *tdt = (GxSubsystemSiParseTDT*)table;
    uint32_t utc_time = 0,utc_mjd = 0;
    struct tm tm_time;
    GxMsgProperty_ExtraTimeOk tdt_time_ok = {0};
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;

    GxCore_MutexLock(AdvanceExtraServiceObj.mutex);
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    utc_mjd = TODATA0_15BIT(tdt->UTC_time[0],tdt->UTC_time[1]);
    utc_time = TODATA0_23BIT(tdt->UTC_time[2],tdt->UTC_time[3],tdt->UTC_time[4]);
    extra_mjd_to_tm((uint16_t)utc_mjd,&tm_time);
    extra_bcd_to_tm(utc_time,&tm_time);
    tm_time.tm_mon -= 1;
    tm_time.tm_year -= 1900;
    
    tdt_time_ok.utc_second = mktime(&tm_time);
    tdt_time_ok.time_fountain = TDT_TIME;
    gx_advance_extra_time_ok_reply(&tdt_time_ok);
    
    if(status == TABLE_OK)
    {
        tmp_protocol->tdt_obj_info.tdt_release(si);
        GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
        return 0;
    }
    else
    {
        GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
        return 1;
    }
}


int32_t Protocol_tdt_create(uint32_t ts_id,uint32_t dmx_id)
{
    GxSubsystemSiEngineAlloc para = {0};
    GxAdvanceExtraDvbProtocol * tmp_protocol = NULL;

    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;

    para.dmx_id = dmx_id;
    para.ts = ts_id;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = TDT_PID;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] = TDT_TID; 
    para.filter.mask[0] = 0xff;

    para.table = tmp_protocol->tdt_obj_info.tdt_get;
    para.time_out = tmp_protocol->tdt_obj_info.tdt_time_out_deal;
    para.sec_num_check = NULL;
    para.time_ms = TDT_TIME_OUT_VALUE;

    tmp_protocol->tdt_obj_info.si_tdt = GxSubSystem_SiEngineAlloc(&para);
    if(tmp_protocol->tdt_obj_info.si_tdt == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search pmt alloc failed\n");
#endif
        return false;
    }

    GxSubSystem_SiEngineStart(tmp_protocol->tdt_obj_info.si_tdt);

    return true;
}
/*  *****************stt相关******************************/
void Protocol_stt_release(SiEngineHandle si)
{
    GxAdvanceExtraAtscProtocol * tmp_protocol = NULL;

    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    if(tmp_protocol->stt_obj_info.si_stt != -1)
    {
        GxSubSystem_SiEngineFree(si);
        tmp_protocol->stt_obj_info.si_stt = -1;
    }
}

int32_t Protocol_stt_time_out_deal(SiEngineHandle si)
{
    GxAdvanceExtraAtscProtocol * tmp_protocol = NULL;


    GxCore_MutexLock(AdvanceExtraServiceObj.mutex); 
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;
    if(tmp_protocol->stt_obj_info.si_stt != -1)
    {
        tmp_protocol->stt_obj_info.stt_release(si);
    }
    GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
    return 0;
}

int32_t Protocol_stt_get(SiEngineHandle si, void * table,SiEngineParseStatus status)
{
    GxSubsystemSiParseSTT * stt = (GxSubsystemSiParseSTT* )table;
    GxMsgProperty_ExtraTimeOk tdt_time_ok = {0};
    GxAdvanceExtraAtscProtocol * tmp_protocol = NULL;

    GxCore_MutexLock(AdvanceExtraServiceObj.mutex);
    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;

    tdt_time_ok.utc_second = stt->system_time - stt->GPS_UTC_offset;
    tdt_time_ok.time_fountain = STT_TIME;
    gx_advance_extra_time_ok_reply(&tdt_time_ok);

    if(status == TABLE_OK)
    {
        tmp_protocol->stt_obj_info.stt_release(si);
        GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
        return 0;
    }
    else
    {
        GxCore_MutexUnlock(AdvanceExtraServiceObj.mutex);
        return 1;
    }
}


int32_t Protocol_stt_create(uint32_t ts_id,uint32_t dmx_id)
{
    GxSubsystemSiEngineAlloc para = {0};
    GxAdvanceExtraAtscProtocol * tmp_protocol = NULL;

    tmp_protocol = AdvanceExtraServiceObj.protocol_obj;

    para.dmx_id = dmx_id;
    para.ts = ts_id;
    para.filter.type = GX_SUB_DMX_FILTER_TYPE_PSI;
    para.filter.pid = STT_PID;
    para.filter.match_depth = 1;
    para.filter.eq_or_neq = SUBSYSTEM_DMX_EQ_MATCH;
    para.filter.crc = SUBSYSTEM_DMX_CRC_ON;
    para.filter.soft_filter = SUBSYSTEM_DMX_SOFT_OFF;
    memset(para.filter.mask,0x00,18);
    para.filter.match[0] = STT_TID; 
    para.filter.mask[0] = 0xff;

    para.table = tmp_protocol->stt_obj_info.stt_get;
    para.time_out = tmp_protocol->stt_obj_info.stt_time_out_deal;
    para.sec_num_check = NULL;
    para.time_ms = STT_TIME_OUT_VALUE;

    tmp_protocol->stt_obj_info.si_stt = GxSubSystem_SiEngineAlloc(&para);
    if(tmp_protocol->stt_obj_info.si_stt == -1)
    {
#ifdef GX_SEARCH_DEBUG
        GX_BUS_SEARCH_ERRO_PRINTF("search pmt alloc failed\n");
#endif
        return false;
    }

    GxSubSystem_SiEngineStart(tmp_protocol->stt_obj_info.si_stt);

    return true;
}
