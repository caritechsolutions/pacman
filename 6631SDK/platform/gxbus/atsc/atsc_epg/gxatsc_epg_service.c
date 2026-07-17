/*****************************************************************************
 *		        CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2009, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	gxatsc_epg_service.c
 * Author    :	shenjch
 * Project   :	GoXceed
 * Type      :
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 0.0	        2014.03.24	          shenjch        creation
 *****************************************************************************/
#include "gxmsg.h"
#include "gxcore.h"
#include "gxservices.h"
#include "gxatsc_epg.h"
#include "gxatsc_epg_private.h"

status_t GxAtscEpgServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	memset(&g_aes,0,sizeof(AtscEpgService_t));
	g_aes.s_EpgServiceHandle = self;
	GxCore_MutexCreate(&(g_aes.mutex));

	GxBus_MessageRegister(GXMSG_ATSC_EPG_GET, sizeof(GxMsgProperty_AtscEpgInfo));
	GxBus_MessageRegister(GXMSG_ATSC_EPG_FREE, sizeof(GxMsgProperty_AtscEpgInfo));
	GxBus_MessageRegister(GXMSG_ATSC_EPG_CREATE, sizeof(GxMsgProperty_AtscEpgCreate));
	GxBus_MessageRegister(GXMSG_ATSC_EPG_CLEAN, 0);
	GxBus_MessageRegister(GXMSG_ATSC_EPG_RELEASE,0);
	GxBus_MessageRegister(GXMSG_ATSC_EPG_END, 0);//table_ok的时候发消息
	GxBus_MessageRegister(GXMSG_ATSC_EPG_BEGIN,0);//监控mgt版本

	GxBus_MessageListen(self, GXMSG_ATSC_EPG_GET);
	GxBus_MessageListen(self, GXMSG_ATSC_EPG_FREE);
	GxBus_MessageListen(self, GXMSG_ATSC_EPG_CREATE);
	GxBus_MessageListen(self, GXMSG_ATSC_EPG_CLEAN);
	GxBus_MessageListen(self, GXMSG_ATSC_EPG_RELEASE);

	sch = GxBus_SchedulerCreate("EpgMsgScheduler", GXBUS_SCHED_MSG, 1024 * 50, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxAtscEpgServiceDestroy(handle_t self)
{
	GxBus_MessageUnListen(self, GXMSG_ATSC_EPG_GET);
	GxBus_MessageUnListen(self, GXMSG_ATSC_EPG_FREE);
	GxBus_MessageUnListen(self, GXMSG_ATSC_EPG_CREATE);
	GxBus_MessageUnListen(self, GXMSG_ATSC_EPG_CLEAN);
	GxBus_MessageUnListen(self, GXMSG_ATSC_EPG_RELEASE);

	GxBus_MessageUnregister(GXMSG_ATSC_EPG_GET);
	GxBus_MessageUnregister(GXMSG_ATSC_EPG_FREE);
	GxBus_MessageUnregister(GXMSG_ATSC_EPG_CREATE);
	GxBus_MessageUnregister(GXMSG_ATSC_EPG_CLEAN);
	GxBus_MessageUnregister(GXMSG_ATSC_EPG_RELEASE);
	GxBus_MessageUnregister(GXMSG_ATSC_EPG_END);
	GxBus_MessageUnregister(GXMSG_ATSC_EPG_BEGIN);

	GxCore_MutexDelete(g_aes.mutex);

	GxBus_ServiceUnlink(self);
	return;
}

GxMsgStatus GxAtscEpgServiceRecvMsg(handle_t self, GxMessage* Msg)
{
	switch(Msg->msg_id)
	{
		case GXMSG_ATSC_EPG_GET:
			{
				GxMsgProperty_AtscEpgInfo* epg_get = NULL;
				epg_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_AtscEpgInfo);
				service_epg_get(epg_get);
				break;
			}
		case GXMSG_ATSC_EPG_FREE:
			{
				GxMsgProperty_AtscEpgInfo* epg_free = NULL;
				epg_free = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_AtscEpgInfo);
				service_epg_free(epg_free);
				break;
			}
		case GXMSG_ATSC_EPG_CREATE:
			{
				GxMsgProperty_AtscEpgCreate* epg_create = NULL;
				epg_create = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_AtscEpgCreate);
				service_epg_create(epg_create);
				break;
			}
		case GXMSG_ATSC_EPG_CLEAN:
			service_epg_clean();
			break;
		case GXMSG_ATSC_EPG_RELEASE:
			service_epg_release();
			break;
		default:
			break;
	}

	return GXMSG_OK;
}

GxServiceClass atsc_epg_service = {
	"atsc epg service",
	GxAtscEpgServiceInit,
	GxAtscEpgServiceDestroy,
	GxAtscEpgServiceRecvMsg,
	NULL,
	.priority_offset = 0,
};

/* End of file -------------------------------------------------------------*/

