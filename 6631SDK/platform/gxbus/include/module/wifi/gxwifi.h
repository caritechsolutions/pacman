#ifndef __GX_WIFI_H__
#define __GX_WIFI_H__

#include "gxcore.h"

struct p2p_device_list {
    char **dev_names;
    int count;
};

int GxWiFi_GetP2PDevice(struct p2p_device_list *list);
int GxWiFi_PutP2PDevice(struct p2p_device_list *list);

#endif