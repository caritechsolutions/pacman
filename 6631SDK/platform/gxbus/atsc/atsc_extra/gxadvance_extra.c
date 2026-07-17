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

/*  Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxcore.h"
#include "gxmsg.h"
#include "gxbus.h"
//#include "service/gxsi.h"
#include "service/gxadvance_extra.h"
#include "include/gxadvance_extra_private.h"
/*  Private Function --------------------------------------------------------------- */
#define GXEXTRA_DEBUG (0)

static status_t  GxAdvanceExtraServiceInit(handle_t self,int priority_offset)
{
    handle_t sch;

    AdvanceExtraServiceObj.s_AdvanceExtraHandle = self;

    GxBus_MessageRegister(GXMSG_EXTRA_SYNC_TIME, sizeof(GxMsgProperty_ExtraSyncTime));
    GxBus_MessageRegister(GXMSG_EXTRA_SYNC_TIME_OK, sizeof(GxMsgProperty_ExtraTimeOk));
    GxBus_MessageRegister(GXMSG_EXTRA_RELEASE, 0);

    GxBus_MessageListen(self, GXMSG_EXTRA_SYNC_TIME);
    GxBus_MessageListen(self, GXMSG_EXTRA_RELEASE);

    sch = GxBus_SchedulerCreate("ExtraMsgScheduler", GXBUS_SCHED_MSG, 1024 * 10, GXOS_DEFAULT_PRIORITY+priority_offset);
    if (sch == GXCORE_INVALID_POINTER) {
        gxlogd("ExtraMsgScheuler create error.\n");
        return GXCORE_ERROR;
    }

    GxBus_ServiceLink(self, sch);
#if (GXEXTRA_DEBUG == 1)
    sch = GxBus_SchedulerCreate("ExtraConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 10, GXOS_DEFAULT_PRIORITY+priority_offset);
    if (sch == GXCORE_INVALID_POINTER) {
        gxlogd("ExtraConsoleScheuler create error.\n");
        return GXCORE_ERROR;
    }
    GxBus_ServiceLink(self, sch);
#endif
    return GXCORE_SUCCESS;
}

static void GxAdvanceExtraServiceDestroy(handle_t self)
{
    GxBus_MessageUnListen(self, GXMSG_EXTRA_SYNC_TIME);
    GxBus_MessageUnListen(self, GXMSG_EXTRA_RELEASE);

    GxBus_MessageUnregister(GXMSG_EXTRA_SYNC_TIME);
    GxBus_MessageUnregister(GXMSG_EXTRA_SYNC_TIME_OK);
    GxBus_MessageUnregister(GXMSG_EXTRA_RELEASE);

    GxBus_ServiceUnlink(self);
    return;
}

static GxMsgStatus GxAdvanceExtraServiceRecvMsg(handle_t self, GxMessage* Msg)
{
    GxMsgProperty_ExtraSyncTime *sync_time = NULL;

    switch(Msg->msg_id)
    {
        case GXMSG_EXTRA_SYNC_TIME:
            sync_time = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_ExtraSyncTime);
            extra_sync_time(sync_time);
            break;
        case GXMSG_EXTRA_RELEASE:
            extra_release_resource();
            break;
        default:
            break;
    }

    return GXMSG_OK;
}

GxServiceClass advance_extra_service = {
    "advance extra service",
    GxAdvanceExtraServiceInit,
    GxAdvanceExtraServiceDestroy,
    GxAdvanceExtraServiceRecvMsg,
#if (GXEXTRA_DEBUG == 1)
    GxExtraServiceConsole,
#else
    NULL,
#endif
    .priority_offset = 0,
};
