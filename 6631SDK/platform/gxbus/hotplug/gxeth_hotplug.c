#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxeth_hotplug.h"
#include "module/config/gxconfig.h"

status_t GxEthHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_ETH_HOTPLUG_IN, sizeof(GxMsgProperty_EthHotPlug));
	GxBus_MessageRegister(GXMSG_ETH_HOTPLUG_OUT, sizeof(GxMsgProperty_EthHotPlug));

	sch = GxBus_SchedulerCreate("Eth_HotPlugConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxEthHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_ETH_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_ETH_HOTPLUG_OUT);

	GxBus_ServiceUnlink(self);
}

void GxEthHotPlugServiceConsole(handle_t self)
{
	struct gx_eth_hot_device* dev;
	GxMessage *msg;
	GxMsgProperty_EthHotPlug* plug = NULL;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_EthHotplugWait();
	while(dev) {
		if (dev->action == PLUG_IN) {
			gxlogd("\n[ETH HOTPLUG] PLUG_IN: %s\n", dev->dev_name);

			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_ETH_HOTPLUG_IN);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_EthHotPlug);
			if(NULL == plug)
				break;
			snprintf(plug->dev_name, sizeof(plug->dev_name), ETH_DEV_NAME"0");
			GxBus_MessageSend(msg);

		}
		else if (dev->action == PLUG_OUT) {
			gxlogd("\n[ETH HOTPLUG] PLUG_OUT: %s\n", dev->dev_name);
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_ETH_HOTPLUG_OUT);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_EthHotPlug);
			if(NULL == plug)
				break;
			snprintf(plug->dev_name, sizeof(plug->dev_name), ETH_DEV_NAME"0");
			GxBus_MessageSend(msg);
		}

		dev = GxCore_EthHotplugGetNext(dev);
	}
	GxCore_EthHotplugClean();
}

GxServiceClass eth_hotplug_service = {
	"ethernet hotplug service",
	GxEthHotPlugServiceInit,
	GxEthHotPlugServiceDestroy,
	NULL,
	GxEthHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
