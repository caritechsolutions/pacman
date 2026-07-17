#ifndef __GX_USBNETPLUG_H__
#define __GX_USBNETPLUG_H__

#include "gxbus.h"
#include "gxcore.h"
#include "gxeth_hotplug.h"

__BEGIN_DECLS

#define USBETH_DEV_NAME_MAX_LEN ETH_DEV_NAME_MAX_LEN
#define GxMsgProperty_UsbnetHotPlug GxMsgProperty_EthHotPlug
#define GxMsgProperty_UsbnetStatus GxMsgProperty_EthHotPlug

__END_DECLS

#endif /* __GX_USBNETHOTPLUG_H__ */

