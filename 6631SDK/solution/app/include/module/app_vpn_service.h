#ifndef __APP_VPN_SERVICE_H__
#define __APP_VPN_SERVICE_H__

#include "app_config.h"
#if VPN_SUPPORT
#include "gxtype.h"
#include"bus/service/gxvpnmsg.h"


typedef enum _VpnState
{
    VPN_STATE_IDLE = 0,
    VPN_STATE_START = 1,
    VPN_STATE_OK = 2,
    VPN_STATE_CLOSE = 3,
}AppVpnState;

typedef struct _VpnInfoClass
{
    vpn_type vpn_type;
    ubyte32 vpn_psk;
    ubyte32 vpn_server;
    ubyte32 vpn_username;
    ubyte32 vpn_password;
}AppVpnInfoClass;

typedef AppVpnState AppMsgProperty_VpnState;
typedef AppVpnInfoClass AppMsgProperty_VpnInfo;
typedef int AppMsgProperty_VpnAuto;

status_t app_vpn_service_init(void);

char* app_vpn_openvpn_get_conf_file(void);

#endif
#endif
