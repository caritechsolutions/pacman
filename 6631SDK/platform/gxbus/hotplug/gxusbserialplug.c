#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxusbserialplug.h"
#include "module/config/gxconfig.h"


status_t GxUsbserialHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USBSERIAL_HOTPLUG_IN, sizeof(GxMsgProperty_UsbserialHotPlug));
	GxBus_MessageRegister(GXMSG_USBSERIAL_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbserialHotPlug));

	sch = GxBus_SchedulerCreate("UsbserialHotPlugConsoleSch", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxUsbserialHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USBSERIAL_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBSERIAL_HOTPLUG_OUT);

	GxBus_ServiceUnlink(self);
}

void GxUsbserialHotPlugServiceConsole(handle_t self)
{
	struct gx_usbserial_hot_device* dev;
	GxMessage *msg;
	GxMsgProperty_UsbserialHotPlug* plug = NULL;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_UsbserialHotplugWait();
	while(dev) {
		if (dev->action == PLUG_IN) {
			gxlogd("\n[USBSERIAL HOTPLUG] PLUG_IN: %d\n", dev->id);

			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBSERIAL_HOTPLUG_IN);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbserialHotPlug);
			if(NULL == plug)
				break;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			GxBus_MessageSend(msg);

		}
		else if (dev->action == PLUG_OUT) {
			gxlogd("\n[USBSERIAL HOTPLUG] PLUG_OUT: %d\n", dev->id);
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBSERIAL_HOTPLUG_OUT);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbserialHotPlug);
			if(NULL == plug)
				break;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			GxBus_MessageSend(msg);

		}

		dev = GxCore_UsbserialHotplugGetNext(dev);
	}
	GxCore_UsbserialHotplugClean();
}

GxServiceClass usbserial_hotplug_service = {
	"usbserial hotplug service",
	GxUsbserialHotPlugServiceInit,
	GxUsbserialHotPlugServiceDestroy,
	NULL,
	GxUsbserialHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
