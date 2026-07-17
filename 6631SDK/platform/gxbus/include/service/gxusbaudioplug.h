#ifndef __GX_USBAUDIOPLUG_H__
#define __GX_USBAUDIOPLUG_H__

#include "gxbus.h"
#include "gxcore.h"
#include "gxusbserialplug.h"

__BEGIN_DECLS

#define USBAUDIO_DEV_NAME_MAX_LEN 64

typedef struct {
	char dev_name[USBAUDIO_DEV_NAME_MAX_LEN];
	int port;
	union {
		int id;
		int status;
	};
	int error;
}GxMsgProperty_UsbaudioHotPlug;

__END_DECLS

#endif /* __GX_USBNETHOTPLUG_H__ */

