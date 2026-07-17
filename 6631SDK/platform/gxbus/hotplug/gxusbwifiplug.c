#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxusbwifiplug.h"
#include "module/config/gxconfig.h"

status_t GxUsbwifiHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USBWIFI_HOTPLUG_IN, sizeof(GxMsgProperty_UsbwifiHotPlug));
	GxBus_MessageRegister(GXMSG_USBWIFI_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbwifiHotPlug));
	GxBus_MessageRegister(GXMSG_USBWIFI_STATUS, sizeof(GxMsgProperty_WifiStatus));
	GxBus_MessageRegister(GXMSG_USBWIFI_SCAN_END, sizeof(GxMsgProperty_WifiStatus));

	sch = GxBus_SchedulerCreate("UsbwifiHotPlugConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxUsbwifiHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_OUT);
	GxBus_MessageUnregister(GXMSG_USBWIFI_STATUS);
	GxBus_MessageUnregister(GXMSG_USBWIFI_SCAN_END);

	GxBus_ServiceUnlink(self);
}

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
void GxUsbwifiHotPlugServiceConsole(handle_t self)
{
	struct gx_usbwifi_hot_device* dev;
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

	GxCore_UsbwifiHotplugWait();
	dev = GxCore_UsbwifiHotplugGetFirst();
	while(dev) {
		if (dev->hot) {
			dev->hot = 0;
			if (dev->action == PLUG_IN) {
				gxlogd("\n[USBWIFI HOTPLUG] PLUG_IN: %d\n", dev->id);

				/*MSG out*/
				msg = GxBus_MessageNew(GXMSG_USBWIFI_HOTPLUG_IN);
				if(NULL == msg)
					break;
				GxMsgProperty_UsbwifiHotPlug* plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
				if(NULL == plug)
					break;
				plug->id = dev->id;
				plug->error = dev->error;
				plug->port = dev->port;
				plug->band_cap = dev->band_cap;
				snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
				GxBus_MessageSend(msg);

			}
			else if (dev->action == PLUG_OUT) {
				gxlogd("\n[USBWIFI HOTPLUG] PLUG_OUT: %d\n", dev->id);
				/*MSG out*/
				msg = GxBus_MessageNew(GXMSG_USBWIFI_HOTPLUG_OUT);
				if(NULL == msg)
					break;
				GxMsgProperty_UsbwifiHotPlug* plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
				if(NULL == plug)
					break;
				plug->id = dev->id;
				plug->error = dev->error;
				plug->port = dev->port;
				snprintf(plug->dev_name, sizeof(plug->dev_name), "%s", dev->dev_name);
				GxBus_MessageSend(msg);

			}
		}
		dev = GxCore_UsbwifiHotplugGetNext(dev);
	}

	dev = GxCore_UsbwifiMsgGetFirst();
	while(dev) {
		switch (dev->action) {
			case SCAN_END:
				{
					msg = GxBus_MessageNew(GXMSG_USBWIFI_SCAN_END);
					if(NULL == msg)
						break;
					GxMsgProperty_WifiStatus* plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_WifiStatus);
					if(NULL == plug)
						break;
					strncpy(plug->dev_name, dev->dev_name, MIN(strlen(dev->dev_name), WIFI_DEV_NAME_MAX_LEN));
					plug->status = dev->status;
					GxBus_MessageSend(msg);
				}
				break;
			case STATUS_CHANGE:
				{
					msg = GxBus_MessageNew(GXMSG_USBWIFI_STATUS);
					if(NULL == msg)
						break;
					GxMsgProperty_WifiStatus* plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_WifiStatus);
					if(NULL == plug)
						break;
					strncpy(plug->dev_name, dev->dev_name, MIN(strlen(dev->dev_name), WIFI_DEV_NAME_MAX_LEN));
					plug->status = dev->status;
					GxBus_MessageSend(msg);
				}
				break;
			default:
				gxlogd("Unknow device action %d\r\n", dev->action);
				break;
		}
		dev = GxCore_UsbwifiMsgGetNext(dev);
	}

	GxCore_UsbwifiHotplugClean();
}

GxServiceClass usbwifi_hotplug_service = {
	"usbwifi hotplug service",
	GxUsbwifiHotPlugServiceInit,
	GxUsbwifiHotPlugServiceDestroy,
	NULL,
	GxUsbwifiHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
