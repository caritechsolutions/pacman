#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxusbnetplug.h"
#include "module/config/gxconfig.h"


status_t GxUsbnetHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USBNET_HOTPLUG_IN, sizeof(GxMsgProperty_UsbnetHotPlug));
	GxBus_MessageRegister(GXMSG_USBNET_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbnetHotPlug));
	GxBus_MessageRegister(GXMSG_USBNET_STATUS, sizeof(GxMsgProperty_UsbnetStatus));

	sch = GxBus_SchedulerCreate("UsbnetHotPlugConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxUsbnetHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USBNET_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBNET_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBNET_STATUS);

	GxBus_ServiceUnlink(self);
}

void GxUsbnetHotPlugServiceConsole(handle_t self)
{
	struct gx_usbnet_hot_device* dev;
	GxMessage *msg;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_UsbnetHotplugWait();
	while(dev) {
		if (dev->action == PLUG_IN) {
			gxlogd("\n[USBNET HOTPLUG] PLUG_IN: %d\n", dev->id);

			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBNET_HOTPLUG_IN);
			if(NULL == msg)
				break;
			GxMsgProperty_UsbnetHotPlug *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbnetHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}
		else if (dev->action == PLUG_OUT) {
			gxlogd("\n[USBNET HOTPLUG] PLUG_OUT: %d\n", dev->id);
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBNET_HOTPLUG_OUT);
			if(NULL == msg)
				break;
			GxMsgProperty_UsbnetHotPlug *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbnetHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}
		else if (dev->action == STATUS_CHANGE) {
			gxlogd("\n[USBNET STATUS_CHANGE] STATUS_CHANGE: %d\n", dev->status);
			msg = GxBus_MessageNew(GXMSG_USBNET_STATUS);
			if(NULL == msg)
				break;
			GxMsgProperty_UsbnetStatus *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbnetStatus);
			if(NULL == plug)
				break;
			plug->error = dev->error;
			plug->port = dev->port;
			plug->status = dev->status;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}
		dev = GxCore_UsbnetHotplugGetNext(dev);
	}
	GxCore_UsbnetHotplugClean();
}

GxServiceClass usbnet_hotplug_service = {
	"usbnet hotplug service",
	GxUsbnetHotPlugServiceInit,
	GxUsbnetHotPlugServiceDestroy,
	NULL,
	GxUsbnetHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
