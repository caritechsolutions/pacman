#ifndef APP_MIRACAST_H
#define APP_MIRACAST_H

#if MIRACAST_SUPPORT
#include "app_config.h"
#include "gxwfd.h"

#define MIRACAST_SSID_NAME_MAX_LEN    (32)

typedef struct _AppMsgMiracastState
{
    WFD_EVENT type;
    char u_ssid[MIRACAST_SSID_NAME_MAX_LEN];
}AppMsgMiracastState;

#endif
#endif

