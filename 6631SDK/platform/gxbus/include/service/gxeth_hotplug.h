#ifndef __GX_ETH_HOTPLUG_H__
#define __GX_ETH_HOTPLUG_H__

#include "gxbus.h"
#include "gxcore.h"

__BEGIN_DECLS

#define ETH_DEV_NAME "eth"
#define ETH_DEV_NAME_LEN 3
#define ETH_DEV_NAME_MAX_LEN 16

#define WIFI_BAND_24G    (1 << 0)
#define WIFI_BAND_5G    (1 << 1)

typedef struct {
	char dev_name[ETH_DEV_NAME_MAX_LEN];
	int port;
	union {
		int id;
		int status;
	};
	int band_cap; // WIFI_BAND_24G | WIFI_BAND_5G
	int error;
}GxMsgProperty_EthHotPlug;

__END_DECLS

#endif /* __GX_HOTPLUG_H__ */

