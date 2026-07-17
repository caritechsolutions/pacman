#ifndef __GX_USBBTPLUG_H__
#define __GX_USBBTPLUG_H__

#include "gxbus.h"
#include "gxcore.h"
#include <gxos/gxcore_os_bt.h>

__BEGIN_DECLS

typedef struct {
	int port;
	int id;
	int error;
}GxMsgProperty_UsbBtHotPlug;

typedef struct {
	GxBTDev info;
}GxMsgProperty_UsbBtScanResult;

typedef struct {
	GxBTLinkKey key;
}GxMsgProperty_UsbBtLinkKey;

typedef struct {
	char string[256];
}GxMsgProperty_UsbBtPlayingInfo;

__END_DECLS

#endif /* __GX_USBBTHOTPLUG_H__ */

