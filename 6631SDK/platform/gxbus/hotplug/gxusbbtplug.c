#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxusbbtplug.h"
#include "module/config/gxconfig.h"

static void bt_event_cbk(GxBTCallbackEvent event_type, void *packet, int size)
{
	GxMessage *msg;
	GxMsgProperty_UsbBtScanResult *result = NULL;
	GxMsgProperty_UsbBtLinkKey    *key;
	GxMsgProperty_UsbBtPlayingInfo *pi;

	switch(event_type)
	{
		// general
		case GXBT_INIT_SUCCESSFUL:
			gxlogd (" <Status Changed> init success\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_INIT_SUCCESS);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_SCAN_RESULT:
			msg = GxBus_MessageNew(GXMSG_USBBT_SCAN_RESULT);
			if(NULL == msg)
				break;

			result = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtScanResult);
			if(NULL == result)
				break;

			memcpy(&result->info, packet, sizeof(GxBTDev));
			gxlogd ("device name: %s addr: \"%s\"\n", result->info.name, result->info.bd_addr_str);
			GxBus_MessageSend(msg);
			break;
		case GXBT_SCAN_FINISH:
			gxlogd (" <Status Changed> scan finish\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SCAN_FINISH);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_LINK_KEY_NOTIFY:
			msg = GxBus_MessageNew(GXMSG_USBBT_LINK_KEY_NOTIFY);
			if(NULL == msg)
				break;

			key = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtLinkKey);
			if(NULL == key)
				break;

			memcpy(&key->key, packet, sizeof(GxBTLinkKey));
			GxBus_MessageSend(msg);
			break;

		// sink
		case GXBT_A2DP_SINK_CONNECT:
			gxlogd (" <Status Changed> SINK CONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_CONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_CONNECT_FAILED:
			gxlogd (" <Status Changed> SINK CONNECT FAILED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_CONNECT_FAILED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_DISCONNECT:
			gxlogd (" <Status Changed> SINK DISCONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_DISCONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_PLAY:
			gxlogd (" <Status Changed> SINK PLAY\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAY);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_PAUSE:
			gxlogd (" <Status Changed> SINK PAUSE\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PAUSE);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SINK_PLAYING_TITLE_INFO:
		case GXBT_A2DP_SINK_PLAYING_ARTIST_INFO:
		case GXBT_A2DP_SINK_PLAYING_ALBUM_INFO:
			if (event_type == GXBT_A2DP_SINK_PLAYING_TITLE_INFO)
				msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAYING_TITLE_INFO);
			else if (event_type == GXBT_A2DP_SINK_PLAYING_ARTIST_INFO)
				msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO);
			else if (event_type == GXBT_A2DP_SINK_PLAYING_ALBUM_INFO)
				msg = GxBus_MessageNew(GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO);
			else
				msg = NULL;

			if(NULL == msg)
				break;

			pi = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtPlayingInfo);
			if(NULL == pi)
				break;

			memcpy(pi->string, packet, size);
			if (event_type == GXBT_A2DP_SINK_PLAYING_TITLE_INFO)
				gxlogd ("Sink Audio Playing Title Info %s\n", pi->string);
			else if (event_type == GXBT_A2DP_SINK_PLAYING_ARTIST_INFO)
				gxlogd ("Sink Audio Playing Artist Info %s\n", pi->string);
			else if (event_type == GXBT_A2DP_SINK_PLAYING_ALBUM_INFO)
				gxlogd ("Sink Audio Playing Album Info %s\n", pi->string);
			GxBus_MessageSend(msg);
			break;

		case GXBT_A2DP_SINK_TRACK_CHANGED:
			gxlogd (" <Status Changed> SINK Track Changed\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SINK_TRACK_CHANGED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;

		// source
		case GXBT_A2DP_SOURCE_CONNECT:
			gxlogd (" <Status Changed> SOURCE CONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_CONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_CONNECT_FAILED:
			gxlogd (" <Status Changed> SOURCE CONNECT FAILED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_CONNECT_FAILED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_DISCONNECT:
			gxlogd (" <Status Changed> SOURCE DISCONNECT\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_DISCONNECT);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_PLAY:
			gxlogd (" <Status Changed> SOURCE PLAY\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_PLAY);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_PAUSE:
			gxlogd (" <Status Changed> SOURCE PAUSE\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_PAUSE);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_STREAM_ESTABLISHED:
			gxlogd (" <Status Changed> SOURCE STREAM ESTABLISHED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		case GXBT_A2DP_SOURCE_STREAM_RELEASED:
			gxlogd (" <Status Changed> SOURCE STREAM RELEASED\n");
			msg = GxBus_MessageNew(GXMSG_USBBT_SOURCE_STREAM_RELEASED);
			if(NULL == msg)
				break;
			GxBus_MessageSend(msg);
			break;
		default:
			gxlogd ("%s Unknow msg : %d\n", __func__, event_type);
			break;
	}
}

status_t GxUsbbtHotPlugServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_USBBT_HOTPLUG_IN,  sizeof(GxMsgProperty_UsbBtHotPlug));
	GxBus_MessageRegister(GXMSG_USBBT_HOTPLUG_OUT, sizeof(GxMsgProperty_UsbBtHotPlug));

	GxBus_MessageRegister(GXMSG_USBBT_INIT_SUCCESS,          0);
	GxBus_MessageRegister(GXMSG_USBBT_SCAN_RESULT,           sizeof(GxMsgProperty_UsbBtScanResult));
	GxBus_MessageRegister(GXMSG_USBBT_SCAN_FINISH,           0);
	GxBus_MessageRegister(GXMSG_USBBT_LINK_KEY_NOTIFY,       sizeof(GxMsgProperty_UsbBtLinkKey));

	GxBus_MessageRegister(GXMSG_USBBT_SINK_CONNECT,          0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_CONNECT_FAILED,   0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_DISCONNECT,       0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAY,             0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PAUSE,            0);
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAYING_TITLE_INFO,  sizeof(GxMsgProperty_UsbBtPlayingInfo));
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO, sizeof(GxMsgProperty_UsbBtPlayingInfo));
	GxBus_MessageRegister(GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO,  sizeof(GxMsgProperty_UsbBtPlayingInfo));
	GxBus_MessageRegister(GXMSG_USBBT_SINK_TRACK_CHANGED,  0);

	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_CONNECT,        0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_CONNECT_FAILED, 0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_DISCONNECT,     0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_PLAY,           0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_PAUSE,          0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED, 0);
	GxBus_MessageRegister(GXMSG_USBBT_SOURCE_STREAM_RELEASED,    0);

	sch = GxBus_SchedulerCreate("UsbbtHotPlugConsoleSch", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxUsbbtHotPlugServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_USBBT_HOTPLUG_IN);
	GxBus_MessageUnregister(GXMSG_USBBT_HOTPLUG_OUT);

	GxBus_MessageUnregister(GXMSG_USBBT_INIT_SUCCESS);
	GxBus_MessageUnregister(GXMSG_USBBT_SCAN_FINISH);
	GxBus_MessageUnregister(GXMSG_USBBT_LINK_KEY_NOTIFY);
	GxBus_MessageUnregister(GXMSG_USBBT_SCAN_RESULT);

	GxBus_MessageUnregister(GXMSG_USBBT_SINK_CONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_CONNECT_FAILED);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_DISCONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAY);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PAUSE);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAYING_TITLE_INFO);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO);
	GxBus_MessageUnregister(GXMSG_USBBT_SINK_TRACK_CHANGED);

	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_CONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_CONNECT_FAILED);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_DISCONNECT);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_PLAY);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_PAUSE);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED);
	GxBus_MessageUnregister(GXMSG_USBBT_SOURCE_STREAM_RELEASED);

	GxBus_ServiceUnlink(self);
}

void GxUsbbtHotPlugServiceConsole(handle_t self)
{
	struct gx_usbbt_hot_device* dev;
	GxMessage *msg;
	GxMsgProperty_UsbBtHotPlug* plug = NULL;
	static int bt_cbk_ref = 0;
	char key_value[10] = {0};

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("false", key_value)) {
		static uint8_t first = 0;

		if(0 == first) {
			GxCore_ThreadDelay(5000);
			first = 1;
		}
	}

	dev = GxCore_UsbbtHotplugWait();
	while(dev) {
		if (dev->action == PLUG_IN) {
			gxlogd("\n[USBBT HOTPLUG] PLUG_IN: %d\n", dev->id);

			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBBT_HOTPLUG_IN);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			GxBus_MessageSend(msg);

			if (!bt_cbk_ref)
				GxCore_BTRegisterCallback(bt_event_cbk);

			bt_cbk_ref++;
		}
		else if (dev->action == PLUG_OUT) {
			bt_cbk_ref--;

			if (!bt_cbk_ref)
				GxCore_BTRegisterCallback(NULL);

			gxlogd("\n[USBBT HOTPLUG] PLUG_OUT: %d\n", dev->id);
			/*MSG out*/
			msg = GxBus_MessageNew(GXMSG_USBBT_HOTPLUG_OUT);
			if(NULL == msg)
				break;
			plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtHotPlug);
			if(NULL == plug)
				break;
			plug->id = dev->id;
			plug->error = dev->error;
			plug->port = dev->port;
			GxBus_MessageSend(msg);
		}

		dev = GxCore_UsbbtHotplugGetNext(dev);
	}
	GxCore_UsbbtHotplugClean();
}

GxServiceClass usbbt_hotplug_service = {
	"usbbt hotplug service",
	GxUsbbtHotPlugServiceInit,
	GxUsbbtHotPlugServiceDestroy,
	NULL,
	GxUsbbtHotPlugServiceConsole,
	.priority_offset = 0,
};
#endif
