#ifndef __GX_VPNPLUG_H__
#define __GX_VPNPLUG_H__
#define VPN_DEV_NAME_LEN 8
typedef enum vpn_type {
	VPN_TYPE_PPTP = 0,
	VPN_TYPE_L2TP = 1,
	VPN_TYPE_OPENVPN = 2,
} vpn_type;

typedef struct {
        char dev_name[VPN_DEV_NAME_LEN];
	vpn_type type;
        int status;
}GxMsgProperty_VPNStatus;
#endif
