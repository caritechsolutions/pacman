#ifndef __GX_USBUVCPLUG_H__
#define __GX_USBUVCPLUG_H__

#include "gxbus.h"
#include "gxcore.h"

__BEGIN_DECLS

typedef struct {
	int port;
	int id;
	int error;
	char dev_name[32];
}GxMsgProperty_UsbUvcHotPlug;

#define UVC_DEV_NAME "video"
#define UVC_DEV_NAME_LEN 5
#define UVC_DEV_NAME_MAX_LEN 16

__END_DECLS

#endif /* __GX_USBUVCHOTPLUG_H__ */

