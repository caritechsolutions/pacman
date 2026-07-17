#include "gxbus.h"
#include "gxmsg.h"
#include "gxavdev.h"
#include "gxhdmi_hotplug.h"
#include "gxhdmi_device.h"
#include "module/config/gxconfig.h"
#include "module/player/gxplayer_module.h"

#define HDMI_SERVICE_GET_MSG_PARAM(type)                  \
	type* param = GxBus_GetMsgPropertyPtr(msg,type); \
if(param == NULL) return GXCORE_ERROR;


status_t GxHdmiHotplugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxHdmi_DeviceInit();

	GxBus_MessageRegister(GXMSG_HDMI_HOTPLUG_IN,   0);
	GxBus_MessageRegister(GXMSG_HDMI_HOTPLUG_OUT,  0);
	GxBus_MessageRegister(GXMSG_HDMI_HDCP_FAIL,    0);
	GxBus_MessageRegister(GXMSG_HDMI_HDCP_SUCCESS, 0);
	GxBus_MessageRegister(GXMSG_HDMI_HDCP_ENABLE,  sizeof(GxMsgProperty_HdmiHdcpEnable));
	GxBus_MessageListen(self, GXMSG_HDMI_HDCP_ENABLE);
	GxBus_MessageRegister(GXMSG_HDMI_VERSION_INFO,  sizeof(GxMsgProperty_HdmiVersionInfo));
	GxBus_MessageListen(self, GXMSG_HDMI_VERSION_INFO);

	sch = GxBus_SchedulerCreate("HdmiHotPlugConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	sch = GxBus_SchedulerCreate("HdmiHotPlugMsgScheduler",  GXBUS_SCHED_MSG, 8*1024, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

GxMsgStatus GxHdmiHotPlugServiceRecvMsg(handle_t self, GxMessage*  msg)
{
	GxMsgStatus ret = GXCORE_SUCCESS;

	return ret;
}

void GxHdmiHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_HDMI_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_HDMI_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_HDMI_HDCP_FAIL);
	GxBus_MessageUnregister(GXMSG_HDMI_HDCP_SUCCESS);
	GxBus_ServiceUnlink(self);
	GxHdmi_DeviceUnInit();
}

void GxHdmiHotplugServiceConsole(handle_t self)
{
	GxMessage *msg = NULL;
	int ret = 0;
	uint32_t hdmi_event = 0;
	uint32_t wait_event = 0;
	GxVideoOutProperty_HdmiHdcpEnable auth = {0};

	wait_event |= (EVENT_HDMI_HOTPLUG_IN|EVENT_HDMI_HOTPLUG_OUT|EVENT_HDMI_READ_EDID|EVENT_HDMI_READ_EDID_ERR);
	wait_event |= (EVENT_HDMI_HDCP_FAIL|EVENT_HDMI_HDCP_SUCCESS|EVENT_HDMI_START_AUTH|EVENT_HDMI_RXSENSE_OK|EVENT_HDMI_RXSENSE_ERROR);

	ret = GxAVWaitEvents(hdmi_device.dev, hdmi_device.hdmi, wait_event, 100000, &hdmi_event);
	if (ret >= 0) {
		gxlogi("GxHdmiHotplugServiceConsole event = %x\n", hdmi_event);
		if (((hdmi_event & EVENT_HDMI_HOTPLUG_IN) == EVENT_HDMI_HOTPLUG_IN) ||
			  ((hdmi_event & EVENT_HDMI_HOTPLUG_OUT) == EVENT_HDMI_HOTPLUG_OUT)) {
			GxVideoOutProperty_OutHdmiStatus stat;

			GxAVGetProperty(hdmi_device.dev, hdmi_device.vo, \
					GxVideoOutPropertyID_HdmiStatus, &stat, sizeof(GxVideoOutProperty_OutHdmiStatus));

			if (stat.status == HDMI_PLUG_OUT) {
				msg = GxBus_MessageNew(GXMSG_HDMI_HOTPLUG_OUT);
				if (msg)
					GxBus_MessageSend(msg);
				gxlogi("Hdmi HotPlug Out\n");
			} else if(stat.status == HDMI_PLUG_IN) {
				msg = GxBus_MessageNew(GXMSG_HDMI_HOTPLUG_IN);
				if (msg)
					GxBus_MessageSend(msg);
				gxlogi("Hdmi HotPlug In\n");
			}

		}

		if ((hdmi_event & EVENT_HDMI_READ_EDID) == EVENT_HDMI_READ_EDID || (hdmi_event & EVENT_HDMI_READ_EDID_ERR) == EVENT_HDMI_READ_EDID_ERR) {
			gxlogi("Hdmi HotPlug Read Edid finish\n");
#if 0 // 驱动会按需做 HDCP
			if (hdmi_device.hdcp_enable) {
				gxlogi("Hdmi start hdcp\n");
				auth.enable = 1;
				GxAVSetProperty(hdmi_device.dev, hdmi_device.vo, \
						GxVideoOutPropertyID_HDCPEnable, &auth, sizeof(GxVideoOutProperty_HdmiHdcpEnable));
			}
#endif
		}

		if ((hdmi_event & EVENT_HDMI_HDCP_FAIL) == EVENT_HDMI_HDCP_FAIL) {
			msg = GxBus_MessageNew(GXMSG_HDMI_HDCP_FAIL);
			if (msg)
				GxBus_MessageSend(msg);

			gxlogi("Hdmi HotPlug Hdcp fail\n");
		}

		if ((hdmi_event & EVENT_HDMI_HDCP_SUCCESS) == EVENT_HDMI_HDCP_SUCCESS) {
			msg = GxBus_MessageNew(GXMSG_HDMI_HDCP_SUCCESS);
			if (msg)
				GxBus_MessageSend(msg);

			gxlogi("Hdmi HotPlug Hdcp success\n");
		}
		if ((hdmi_event & EVENT_HDMI_RXSENSE_OK) == EVENT_HDMI_RXSENSE_OK) {
			msg = GxBus_MessageNew(GXMSG_HDMI_RXSENSE_OK);
			if (msg)
				GxBus_MessageSend(msg);

			gxlogi("Hdmi Rxsense ok\n");
		}
		if ((hdmi_event & EVENT_HDMI_RXSENSE_ERROR) == EVENT_HDMI_RXSENSE_ERROR) {
			msg = GxBus_MessageNew(GXMSG_HDMI_RXSENSE_ERROR);
			if (msg)
				GxBus_MessageSend(msg);

			gxlogi("Hdmi Rxsense error\n");
		}
	}
}

GxServiceClass hdmi_hotplug_service = {
	"hdmi hotplug service",
	GxHdmiHotplugServiceInit,
	GxHdmiHotPlugServiceDestroy,
	GxHdmiHotPlugServiceRecvMsg,
	GxHdmiHotplugServiceConsole,
	.priority_offset = 0,
};
