/**
 * @file gxupdate.c
 * @author lixb
 * @brief Goxceed Update在线升级服务实现
 */
#include "gxmsg.h"
#include "service/gxupdate.h"
#include "update/gxupdate_stream.h"
#include "update/gxupdate_protocol_usb.h"
#include "update/gxupdate_protocol_ts.h"
#include "update/gxupdate_partition_flash.h"
#include "update/gxupdate_partition_file.h"
#include "gxupdate_debug.h"

#define UPDATE_STREAM_0             ("stream0")
static GxUpdate_ProtocolOps* protocol_list[] =
{
    &gxupdate_protocol_ts,
    &gxupdate_protocol_usb,
    NULL
};
static GxUpdate_PartitionOps* partition_list[] =
{
    &gxupdate_partition_flash,
    &gxupdate_partition_file,
    NULL
};

handle_t update_stream_handle = E_INVALID_HANDLE;

static void update_error(int32_t err)
{
    GxMessage*                  msg;
    GxMsgProperty_UpdateStatus* status;

    msg = GxBus_MessageNew(GXMSG_UPDATE_STATUS);
    if (msg == NULL) {
        return;
    }

    status          = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UpdateStatus);
    status->type    = GXUPDATE_STATUS_ERROR;
    status->error   = err;

    GxBus_MessageSend(msg);
}

static void update_percent(int32_t percent)
{
    GxMessage*                  msg;
    GxMsgProperty_UpdateStatus* status;

    msg = GxBus_MessageNew(GXMSG_UPDATE_STATUS);
    if (msg == NULL) {
        return;
    }

    status          = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UpdateStatus);
    status->type    = GXUPDATE_STATUS_PRERCENT;
    status->percent = percent;

    GxBus_MessageSend(msg);
}

static status_t GxUpdate_Init(handle_t self,int priority_offset)
{
    handle_t sch;
    GxUpdate_ProtocolOps**  update_protocol_list = protocol_list;
    GxUpdate_PartitionOps** update_partition_list = partition_list;

    GxBus_MessageRegister(GXMSG_UPDATE_OPEN, sizeof(GxMsgProperty_UpdateOpen));
    GxBus_MessageRegister(GXMSG_UPDATE_START, 0);
    GxBus_MessageRegister(GXMSG_UPDATE_STOP, 0);
    GxBus_MessageRegister(GXMSG_UPDATE_STATUS, sizeof(GxMsgProperty_UpdateStatus));

    GxBus_MessageRegister(GXMSG_UPDATE_PROTOCOL_SELECT,
                                sizeof(GxMsgProperty_UpdateProtocolSelect));
    GxBus_MessageRegister(GXMSG_UPDATE_PROTOCOL_CTRL,
                                sizeof(GxUpdate_IoCtrl));
    GxBus_MessageRegister(GXMSG_UPDATE_PARTITION_SELECT,
                                sizeof(GxMsgProperty_UpdatePartitionSelect));
    GxBus_MessageRegister(GXMSG_UPDATE_PARTITION_CTRL,
                                sizeof(GxUpdate_IoCtrl));

    GxBus_MessageListen(self, GXMSG_UPDATE_OPEN);
    GxBus_MessageListen(self, GXMSG_UPDATE_START);
    GxBus_MessageListen(self, GXMSG_UPDATE_STOP);
    GxBus_MessageListen(self, GXMSG_UPDATE_PROTOCOL_SELECT);
    GxBus_MessageListen(self, GXMSG_UPDATE_PROTOCOL_CTRL);
    GxBus_MessageListen(self, GXMSG_UPDATE_PARTITION_SELECT);
    GxBus_MessageListen(self, GXMSG_UPDATE_PARTITION_CTRL);

    GxUpdate_StreamInit(update_protocol_list, update_partition_list);

    sch = GxBus_SchedulerCreate("update msg", GXBUS_SCHED_MSG, 8*1024,
                         GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    sch = GxBus_SchedulerCreate("update console", GXBUS_SCHED_CONSOLE, 8*1024,
                         GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    return GXCORE_SUCCESS;
}

static void GxUpdate_Destroy(handle_t self)
{

    GxBus_MessageUnregister(GXMSG_UPDATE_OPEN);
    GxBus_MessageUnregister(GXMSG_UPDATE_START);
    GxBus_MessageUnregister(GXMSG_UPDATE_STOP);
    GxBus_MessageUnregister(GXMSG_UPDATE_STATUS);
    GxBus_MessageUnregister(GXMSG_UPDATE_PROTOCOL_SELECT);
    GxBus_MessageUnregister(GXMSG_UPDATE_PROTOCOL_CTRL);
    GxBus_MessageUnregister(GXMSG_UPDATE_PARTITION_SELECT);
    GxBus_MessageUnregister(GXMSG_UPDATE_PARTITION_CTRL);

    GxBus_ServiceUnlink(self);
}

static GxMsgStatus GxUpdate_MsgProcess(handle_t self, GxMessage* msg)
{
    switch (msg->msg_id) {
    case GXMSG_UPDATE_OPEN: {
    		DBG_UPDATE("%s, GXMSG_UPDATE_OPEN\n", __FUNCTION__);
            if (update_stream_handle != E_INVALID_HANDLE) {
              GxUpdate_StreamClearStatus(update_stream_handle);
                //GxUpdate_StreamClose(update_stream_handle);
                break;
            }

            update_stream_handle = GxUpdate_StreamOpen(UPDATE_STREAM_0);
        }
        break;
    case GXMSG_UPDATE_PROTOCOL_SELECT: {
    		DBG_UPDATE("%s, GXMSG_UPDATE_PROTOCOL_SELECT\n", __FUNCTION__);
            GxMsgProperty_UpdateProtocolSelect* p;
            p=GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_UpdateProtocolSelect);
            if (p == NULL) {
                break;
            }
            GxUpdate_StreamIoctl(update_stream_handle,
                                       GXUPDATE_STREAM_PROTOCOL_SELECT,
                                       p,
                                       sizeof(GxUpdate_ProtocolName));
        }
        break;

    case GXMSG_UPDATE_PROTOCOL_CTRL: {
            GxMsgProperty_UpdateProtocolCtrl* p;
            DBG_UPDATE("%s, GXMSG_UPDATE_PROTOCOL_CTRL\n", __FUNCTION__);
    	    p = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UpdateProtocolCtrl);
    	    if (p == NULL) {
    	    	break;
    	    }
            GxUpdate_StreamIoctl(update_stream_handle,
                                       GXUPDATE_STREAM_PROTOCOL_CTRL,
                                       p,
                                       sizeof(GxMsgProperty_UpdateProtocolCtrl));
        }
        break;
    case GXMSG_UPDATE_PARTITION_SELECT: {
            GxMsgProperty_UpdatePartitionSelect* p;
            DBG_UPDATE("%s, GXMSG_UPDATE_PARTITION_SELECT\n", __FUNCTION__);
            p=GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_UpdatePartitionSelect);
            if (p == NULL) {
                break;
            }
            GxUpdate_StreamIoctl(update_stream_handle,
                                       GXUPDATE_STREAM_PARTITION_SELECT,
                                       p,
                                       sizeof(GxUpdate_PartitionName));
        }
        break;

    case GXMSG_UPDATE_PARTITION_CTRL: {
            GxMsgProperty_UpdatePartitionCtrl* p;
            DBG_UPDATE("%s, GXMSG_UPDATE_PARTITION_SELECT\n", __FUNCTION__);
    	    p = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UpdatePartitionCtrl);
    	    if (p == NULL) {
    	    	break;
    	    }
            GxUpdate_StreamIoctl(update_stream_handle,
                                       GXUPDATE_STREAM_PARTITION_CTRL,
                                       p,
                                       sizeof(GxMsgProperty_UpdatePartitionCtrl));
         }
        break;


    case GXMSG_UPDATE_START: {
    		DBG_UPDATE("%s, GXMSG_UPDATE_PARTITION_SELECT\n", __FUNCTION__);
            GxUpdate_StreamIoctl(update_stream_handle,
                                       GXUPDATE_STREAM_UPDATE_START,
                                       NULL,
                                       0);

        }
        break;
    case GXMSG_UPDATE_STOP: {
    		DBG_UPDATE("%s, GXMSG_UPDATE_PARTITION_SELECT\n", __FUNCTION__);
            GxUpdate_StreamIoctl(update_stream_handle,
                                       GXUPDATE_STREAM_UPDATE_STOP,
                                       NULL,
                                       0);

            GxUpdate_StreamClose(update_stream_handle);
            update_stream_handle = E_INVALID_HANDLE;
        }
        break;
    default:
        break;
    }
    return GXMSG_OK;
}

static void GxUpdate_ConsoleProcess(handle_t self)
{
    int32_t                     ret;
    int32_t						status;

    if (update_stream_handle == E_INVALID_HANDLE) {
        GxCore_ThreadDelay(500);
        return;
    }


    ret = GxUpdate_StreamIoctl(update_stream_handle,
                               GXUPDATE_STREAM_UPDATE_GET_STATUS,
                               &status,
                               sizeof(int32_t));

    if (ret == E_OK) {
    	if (status <= 0) {
        	update_error(status);
        } else {
			update_percent(status);
        }
    }
}


GxServiceClass update_service = {
    "Update Service",
    GxUpdate_Init,
    GxUpdate_Destroy,
    GxUpdate_MsgProcess,
    GxUpdate_ConsoleProcess
};

