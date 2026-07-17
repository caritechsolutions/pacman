#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxusb3gplug.h"
#include "module/config/gxconfig.h"

status_t GxUsb3gHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USB3G_HOTPLUG_IN, sizeof(GxMsgProperty_Usb3gHotPlug));
	GxBus_MessageRegister(GXMSG_USB3G_HOTPLUG_OUT, sizeof(GxMsgProperty_Usb3gHotPlug));

	sch = GxBus_SchedulerCreate("Usb3gHotPlugConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxUsb3gHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_OUT);

	GxBus_ServiceUnlink(self);
}

void GxUsb3gHotPlugServiceConsole(handle_t self)
{
	struct gx_usb3g_hot_device* dev;
	GxMessage *msg;
	GxMsgProperty_Usb3gHotPlug* plug = NULL;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_Usb3gHotplugWait();
	while(dev) {
		if (dev->action == PLUG_IN) {
			gxlogd("\n[USB3G HOTPLUG] PLUG_IN: %d\n", dev->id);

			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USB3G_HOTPLUG_IN);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}
		else if (dev->action == PLUG_OUT) {
			gxlogd("\n[USB3G HOTPLUG] PLUG_OUT: %d\n", dev->id);
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USB3G_HOTPLUG_OUT);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}

		dev = GxCore_Usb3gHotplugGetNext(dev);
	}
	GxCore_Usb3gHotplugClean();
}

GxServiceClass usb3g_hotplug_service = {
	"usb3g hotplug service",
	GxUsb3gHotPlugServiceInit,
	GxUsb3gHotPlugServiceDestroy,
	NULL,
	GxUsb3gHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
