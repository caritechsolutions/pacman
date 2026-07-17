#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxusbhidplug.h"
#include "module/config/gxconfig.h"

status_t GxUsbhidHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USBHID_HOTPLUG_IN, sizeof(GxMsgProperty_UsbHidHotPlug));
	GxBus_MessageRegister(GXMSG_USBHID_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbHidHotPlug));

	sch = GxBus_SchedulerCreate("UsbhidHotPlugConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxUsbhidHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USBHID_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBHID_HOTPLUG_OUT);

	GxBus_ServiceUnlink(self);
}

void GxUsbhidHotPlugServiceConsole(handle_t self)
{
	struct gx_usbhid_hot_device* dev;
	GxMessage *msg;
	GxMsgProperty_UsbHidHotPlug* plug = NULL;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_UsbhidHotplugWait();
	while(dev) {
		if (dev->action == PLUG_IN) {
			gxlogd("\n[USBHID HOTPLUG] PLUG_IN: %d\n", dev->id);

			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBHID_HOTPLUG_IN);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbHidHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}
		else if (dev->action == PLUG_OUT) {
			gxlogd("\n[USBHID HOTPLUG] PLUG_OUT: %d\n", dev->id);
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBHID_HOTPLUG_OUT);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbHidHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}

		dev = GxCore_UsbhidHotplugGetNext(dev);
	}
	GxCore_UsbhidHotplugClean();
}

GxServiceClass usbhid_hotplug_service = {
	"usbhid hotplug service",
	GxUsbhidHotPlugServiceInit,
	GxUsbhidHotPlugServiceDestroy,
	NULL,
	GxUsbhidHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
