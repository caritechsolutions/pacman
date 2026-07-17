#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxusbuvcplug.h"
#include "module/config/gxconfig.h"

status_t GxUsbuvcHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USBUVC_HOTPLUG_IN, sizeof(GxMsgProperty_UsbUvcHotPlug));
	GxBus_MessageRegister(GXMSG_USBUVC_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbUvcHotPlug));

	sch = GxBus_SchedulerCreate("UsbuvcHotPlugConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxUsbuvcHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USBUVC_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBUVC_HOTPLUG_OUT);

	GxBus_ServiceUnlink(self);
}

void GxUsbuvcHotPlugServiceConsole(handle_t self)
{
	struct gx_usbuvc_hot_device* dev;
	GxMessage *msg;
	GxMsgProperty_UsbUvcHotPlug* plug = NULL;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_UsbuvcHotplugWait();
	while(dev) {
		if (dev->action == PLUG_IN) {
			gxlogd("\n[USBUVC HOTPLUG] PLUG_IN: %d\n", dev->id);

			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBUVC_HOTPLUG_IN);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbUvcHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}
		else if (dev->action == PLUG_OUT) {
			gxlogd("\n[USBUVC HOTPLUG] PLUG_OUT: %d\n", dev->id);
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBUVC_HOTPLUG_OUT);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbUvcHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
			GxBus_MessageSend(msg);
		}

		dev = GxCore_UsbuvcHotplugGetNext(dev);
	}
	GxCore_UsbuvcHotplugClean();
}

GxServiceClass usbuvc_hotplug_service = {
	"usbuvc hotplug service",
	GxUsbuvcHotPlugServiceInit,
	GxUsbuvcHotPlugServiceDestroy,
	NULL,
	GxUsbuvcHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
