#ifndef __GX_USBWIFIPLUG_H__
#define __GX_USBWIFIPLUG_H__

#include "gxbus.h"
#include "gxcore.h"
#include "gxeth_hotplug.h"

__BEGIN_DECLS

#define WIFI_DEV_NAME "wlan"
#define WIFI_DEV_NAME_LEN 4
#define WIFI_DEV_NAME_MAX_LEN ETH_DEV_NAME_MAX_LEN
#define GxMsgProperty_UsbwifiHotPlug GxMsgProperty_EthHotPlug
#define GxMsgProperty_WifiStatus GxMsgProperty_EthHotPlug

__END_DECLS

#endif /* __GX_HOTPLUG_H__ */

