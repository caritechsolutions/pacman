/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	gxextra.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.12.10	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gxcore.h"
#include "gxmsg.h"
#include "gxbus.h"
#include "service/gxsi.h"
#include "service/gxextra.h"
#include "module/config/gxconfig.h"
#include "module/extra/gxextra_module.h"


/* Private types/constants ------------------------------------------------ */
#define GxExtra_gxlogd(...)      gxlogd( __VA_ARGS__ )
#define MAX_SYNC_TIME_UPDATE	(10)
#define MAX_TABLE_LEN   (64)

extern int32_t tdt_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t tot_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
//extern void GxSi_SyncTimeDo(void);

static int32_t s_TotDisable = 0;
static int32_t s_TdtSubtId = -1;
static int32_t s_TotSubtId = -1;
static handle_t s_ExtraHandle;
static uint8_t s_TableBuf[MAX_TABLE_LEN]={0};
static int32_t sync_time_count    = 0;
static int32_t sync_time_interval = 2000;

/* Private Functions ----------------------------------------------------- */
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
#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

	tm->tm_sec = BCD2DEC(bcd_time&0xff);
	tm->tm_min = BCD2DEC((bcd_time>>8)&0xff);
	tm->tm_hour = BCD2DEC((bcd_time>>16)&0xff);
}

static private_parse_status time_table_parser(uint8_t *p_section_data, uint32_t len)
{
	if(p_section_data[0] == TDT_TID)
	{
		tdt_head_parser(p_section_data, len, s_TableBuf);
	}
	else if(p_section_data[0] == TOT_TID)
	{
		tot_head_parser(p_section_data, len, s_TableBuf);
	}
//	memcpy(p_parsed_data, p_section_data, len);
	return PRIVATE_SUBTABLE_OK;
}

static void extra_sync_time(uint32_t ts_src)
{
	GxSubTableDetail subt_detail;
	GxMsgProperty_SiCreate	*params_create;
	GxMsgProperty_SiStart *params_start;
	GxMessage * msg;

	if (s_TdtSubtId == -1)
	{
		memset(&subt_detail, 0, sizeof(GxSubTableDetail));
		subt_detail.ts_src= (uint16_t)ts_src;
		subt_detail.si_filter.pid = TDT_PID;
		subt_detail.si_filter.match_depth = 1;
		subt_detail.si_filter.eq_or_neq = EQ_MATCH;
		subt_detail.si_filter.match[0] = TDT_TID;
		subt_detail.si_filter.mask[0] = 0xff;
		subt_detail.si_filter.crc = CRC_OFF;
		subt_detail.time_out = 300000;

		subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
		subt_detail.table_parse_cfg.table_parse_fun = time_table_parser;

		msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
		if(msg == NULL)
		{
			gxlogd("[extra] tdt create msg err!\n");
			return;
		}
		params_create = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiCreate);

		*params_create = &subt_detail;

		GxBus_MessageSendWait(msg);

		if (subt_detail.si_subtable_id == -1)
		{
			GxBus_MessageFree(msg);
			return;
		}

		GxExtra_gxlogd("tdt create subt ok\n");
		// get the return "si_subtable_id"
		s_TdtSubtId = subt_detail.si_subtable_id;
		GxBus_MessageFree(msg);//同步消息 需要发送方进行释放
	}

	// start si
	msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
	if(msg == NULL)
	{
		gxlogd("[extra] tdt start msg err!\n");
		return;
	}

	params_start = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiStart);
	*params_start = s_TdtSubtId;

	GxBus_MessageSend(msg);
	//GxExtra_gxlogd("tdt start subt id = %d\n",s_TdtSubtId);

	// Global config of TOT parser
	if (s_TotDisable)
		return;

	if (s_TotSubtId == -1)
	{
		memset(&subt_detail, 0, sizeof(GxSubTableDetail));
		subt_detail.ts_src= (uint16_t)ts_src;
		subt_detail.si_filter.pid = TOT_PID;
		subt_detail.si_filter.match_depth = 1;
		subt_detail.si_filter.eq_or_neq = EQ_MATCH;
		subt_detail.si_filter.match[0] = TOT_TID;
		subt_detail.si_filter.mask[0] = 0xff;
		subt_detail.time_out = 300000;
		subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
		subt_detail.table_parse_cfg.table_parse_fun = time_table_parser;

		msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
		if(msg == NULL)
		{
			gxlogd("[extra] tdt create msg err!\n");
			return;
		}
		params_create = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiCreate);

		*params_create = &subt_detail;

		GxBus_MessageSendWait(msg);

		if (subt_detail.si_subtable_id == -1)
		{
			GxBus_MessageFree(msg);
			return;
		}

		GxExtra_gxlogd("tdt create subt ok\n");
		// get the return "si_subtable_id"
		s_TotSubtId = subt_detail.si_subtable_id;
		GxBus_MessageFree(msg);//同步消息 需要发送方进行释放
	}

	// start si
	msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
	if(msg == NULL)
	{
		gxlogd("[extra] tdt start msg err!\n");
		return;
	}

	params_start = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiStart);
	*params_start = s_TotSubtId;

	GxBus_MessageSend(msg);
	//GxExtra_gxlogd("tdt start subt id = %d\n",s_TotSubtId);
}
static void extra_restart_fiter(void)
{
	GxMsgProperty_SiStart *params_start;
	GxMessage * msg;

	if (s_TdtSubtId != -1)
	{
		// start si
		msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
		if(msg == NULL)
		{
			gxlogd("[extra] tdt start msg err!\n");
			return;
		}

		params_start = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiStart);
		*params_start = s_TdtSubtId;

		GxBus_MessageSend(msg);
	}

	if (s_TotSubtId != -1)
	{
		// start si
		msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
		if(msg == NULL)
		{
			gxlogd("[extra] tdt start msg err!\n");
			return;
		}

		params_start = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiStart);
		*params_start = s_TotSubtId;

		GxBus_MessageSend(msg);
	}
}

static void extra_release_resource(void)
{
	GxMessage *msg;

	if (s_TdtSubtId != -1)
	{
		GxMsgProperty_SiRelease *params_release;

		msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
		if(msg == NULL)
		{
			gxlogd("[extra] tdt release msg err!\n");
		}

		params_release = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiRelease);
		*params_release = s_TdtSubtId;

		GxBus_MessageSend(msg);
	}
	if (s_TotSubtId != -1)
	{
		GxMsgProperty_SiRelease *params_release;

		msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
		if(msg == NULL)
		{
			gxlogd("[extra] tdt release msg err!\n");
			return;
		}

		params_release = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_SiRelease);
		*params_release = s_TotSubtId;

		GxBus_MessageSend(msg);
	}
	if (s_TdtSubtId != -1 || s_TotSubtId != -1)
	{
		GxBus_MessageEmpty(s_ExtraHandle);
	}

    s_TdtSubtId = -1;
    s_TotSubtId = -1;

}

static void extra_si_subtable_ok(GxParseResult *parse_result)
{
	GxMessage *msg;
	GxMsgProperty_ExtraTimeOk* p_time_ok = NULL;
	GxTime time_struct;
	time_t utc_time = 0;
	int32_t time_temp = 0;

	if (parse_result->table_id == TDT_TID)
	{
		TdtInfo *p_tdt_info;
		struct tm tm_time;
		//GxTime sys_time;

		p_tdt_info = (TdtInfo*)s_TableBuf;
		extra_mjd_to_tm((uint16_t)(p_tdt_info->utc_mjd), &tm_time);
		extra_bcd_to_tm(p_tdt_info->utc_time, &tm_time);

		// TODO: Add check date normal here ?
		tm_time.tm_mon -= 1;
		tm_time.tm_year -= 1900;

		//sys_time.seconds = mktime(&tm_time);
		//sys_time.microsecs = 0;

		//GxCore_SetLocalTime(&sys_time);

		utc_time = mktime(&tm_time);
		GxBus_ConfigGetInt(GXBUS_EXTRA_COMPARE_TIME, &time_temp ,0xffffffff);
		if(time_temp != 0xffffffff)
		{
			GxCore_GetLocalTime(&time_struct);
			if(abs(utc_time-(time_struct.seconds+time_struct.microsecs/1000))<time_temp)
			{
				return;
			}
		}

		// send message report sync time success
		msg = GxBus_MessageNew(GXMSG_EXTRA_SYNC_TIME_OK);
		p_time_ok = (GxMsgProperty_ExtraTimeOk*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_ExtraTimeOk);
		p_time_ok->utc_second = utc_time;
		p_time_ok->time_fountain = TDT_TIME;
		GxBus_MessageSend(msg);

		// si service sync time with the time just get
		//GxSi_SyncTimeDo();

		//GxBus_SiParserBufFree(parse_result->parsed_data);
	}
	else if(parse_result->table_id == TOT_TID)
	{
		TotInfo *p_tot_info;
		struct tm tm_time;

		p_tot_info = (TotInfo*)s_TableBuf;
		extra_mjd_to_tm((uint16_t)(p_tot_info->utc_mjd), &tm_time);
		extra_bcd_to_tm(p_tot_info->utc_time, &tm_time);

		// TODO: Add check date normal here ?
		tm_time.tm_mon -= 1;
		tm_time.tm_year -= 1900;

		utc_time = mktime(&tm_time);
		GxBus_ConfigGetInt(GXBUS_EXTRA_COMPARE_TIME, &time_temp, 0xffffffff);
		if(time_temp != 0xffffffff)
		{
			GxCore_GetLocalTime(&time_struct);
			if(abs(utc_time-(time_struct.seconds+time_struct.microsecs/1000))<time_temp)
			{
				return;
			}
		}
		// send message report sync time success
		msg = GxBus_MessageNew(GXMSG_EXTRA_SYNC_TIME_OK);
		p_time_ok = (GxMsgProperty_ExtraTimeOk*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_ExtraTimeOk);
		p_time_ok->utc_second = utc_time;
		p_time_ok->time_fountain = TOT_TIME;
		p_time_ok->offset_time_count = p_tot_info->offset_time_count;
		memcpy(p_time_ok->offset_time,p_tot_info->offset_time, p_tot_info->offset_time_count * sizeof(time_offset));
		GxBus_MessageSend(msg);

		// si service sync time with the time just get
		//GxSi_SyncTimeDo();

		//GxBus_SiParserBufFree(parse_result->parsed_data);
	}
}

/* Exported Functions ----------------------------------------------------- */
#if 0
/*later will be useful, need make it better*/
typedef void (*sync_time_do)(void *params);
static uint32_t s_TimerRecord = 0;
sync_time_do sync_time_update[MAX_SYNC_TIME_UPDATE] = {0};

void GxExtra_SyncTimeAdd(sync_time_do fun)
{
	uint32_t i;

	for (i=0; i<MAX_SYNC_TIME_UPDATE; i++)
	{
		if ((s_TimerRecord & (1<<i)) == 0)
		{
			sync_time_update[i] = fun;
		}

	}
}

void GxExtra_SyncTimeDel(sync_time_do fun)
{
	uint32_t i;

	for (i=0; i<MAX_SYNC_TIME_UPDATE; i++)
	{
		if (sync_time_update[i] == fun)
		{
			s_TimerRecord &=  (~(1<<i));
			sync_time_update[i] = NULL;
		}

	}
}

static void GxExtra_SyncTimeDo(void)
{
	uint32_t i;

	for (i=0; i<MAX_SYNC_TIME_UPDATE; i++)
	{
		if ((s_TimerRecord & (1<<i)) != 0)
		{
			(sync_time_update[i])(params);
 		}
	}
}
#endif


static status_t  GxExtraServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_ConfigGetInt(EXTRA_CONFIG_SYNC_TIME_INTERVAL, &sync_time_interval, DEFAULT_SYNC_TIME_INTERVAL);
	GxBus_ConfigGetInt(EXTRA_CONFIG_SYNC_TIME_COUNT,    &sync_time_count, DEFAULT_SYNC_TIME_COUNT);
	GxBus_ConfigGetInt(EXTRA_CONFIG_DISABLE_TOT_PARSER, &s_TotDisable, DEFAULT_DISABLE_TOT_PARSER);
	gxlogi("%s %d %x %x\n", __func__, __LINE__, sync_time_count, s_TotDisable);

	s_ExtraHandle = self;
	GxBus_MessageRegister(GXMSG_EXTRA_SYNC_TIME, sizeof(GxMsgProperty_ExtraSyncTime));
	GxBus_MessageRegister(GXMSG_EXTRA_SYNC_TIME_OK, sizeof(GxMsgProperty_ExtraTimeOk));
	GxBus_MessageRegister(GXMSG_EXTRA_RELEASE, 0);

	GxBus_MessageListen(self, GXMSG_EXTRA_SYNC_TIME);
	GxBus_MessageListen(self, GXMSG_EXTRA_RELEASE);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_OK);

	sch = GxBus_SchedulerCreate("ExtraMsgScheduler", GXBUS_SCHED_MSG, 1024*8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	if (sync_time_count > 0) {
		sch = GxBus_SchedulerCreate("ExtraConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024*8, GXOS_DEFAULT_PRIORITY+priority_offset);
		GxBus_ServiceLink(self, sch);
	}

	return GXCORE_SUCCESS;
}

static void GxExtraServiceDestroy(handle_t self)
{
	GxBus_MessageUnListen(self, GXMSG_EXTRA_SYNC_TIME);
	GxBus_MessageUnListen(self, GXMSG_EXTRA_RELEASE);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_OK);

	GxBus_MessageUnregister(GXMSG_EXTRA_SYNC_TIME);
	GxBus_MessageUnregister(GXMSG_EXTRA_SYNC_TIME_OK);
	GxBus_MessageUnregister(GXMSG_EXTRA_RELEASE);

	GxBus_ServiceUnlink(self);
	return;
}


static GxMsgStatus GxExtraServiceRecvMsg(handle_t self, GxMessage* Msg)
{
	GxParseResult *parse_result = NULL;
    GxMsgProperty_ExtraSyncTime *sync_time = NULL;
	int32_t continuly = 0;

	switch(Msg->msg_id)
	{
		case GXMSG_EXTRA_SYNC_TIME:
			sync_time = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_ExtraSyncTime);
			extra_sync_time(sync_time->ts_src);
			break;
		case GXMSG_SI_SUBTABLE_OK:
			parse_result = GxBus_GetMsgPropertyPtr(Msg,GxParseResult);

			extra_si_subtable_ok(parse_result);
			GxBus_ConfigGetInt(GXBUS_EXTRA_FILTER_CONTINULY, &continuly, GXBUS_EXTRA_NO);
			if(continuly == GXBUS_EXTRA_YES)
			{
				extra_restart_fiter();
			}
			break;
		case GXMSG_EXTRA_RELEASE:
			extra_release_resource();
			break;

		default:
			break;
	}

	return GXMSG_OK;
}

static void GxExtraServiceConsole(handle_t self)
{
	if (sync_time_count != 0) {
		GxMessage *msg;

		msg = GxBus_MessageNew(GXMSG_EXTRA_SYNC_TIME);
		GxBus_MessageSend(msg);

		if (sync_time_count > 0)
			sync_time_count--;
	}
	GxCore_ThreadDelay(sync_time_interval);
}

GxServiceClass extra_service = {
	"extra service",
	GxExtraServiceInit,
	GxExtraServiceDestroy,
	GxExtraServiceRecvMsg,
	GxExtraServiceConsole,
   .priority_offset = 0,
};

/* End of file -------------------------------------------------------------*/

