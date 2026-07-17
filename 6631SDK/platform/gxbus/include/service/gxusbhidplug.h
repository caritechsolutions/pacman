#ifndef __GX_USBHIDPLUG_H__
#define __GX_USBHIDPLUG_H__

#include "gxbus.h"
#include "gxcore.h"

__BEGIN_DECLS

typedef struct {
	int port;
	int id;
	int error;
	char dev_name[32];
}GxMsgProperty_UsbHidHotPlug;

#define HID_DEV_NAME "input/js"
#define HID_DEV_NAME_LEN 9
#define HID_DEV_NAME_MAX_LEN 16

__END_DECLS

#endif /* __GX_USBHIDHOTPLUG_H__ */

