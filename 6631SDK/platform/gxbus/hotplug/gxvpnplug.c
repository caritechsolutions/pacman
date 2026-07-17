#ifdef ECOS_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxbus.h"
#include "gxcore.h"
#include "gxvpnmsg.h"
#include "module/config/gxconfig.h"
#include "gxos/gxcore_os_vpn.h"

status_t GxVPNStatusServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_VPN_STATUS, sizeof(GxMsgProperty_VPNStatus));
	sch = GxBus_SchedulerCreate("VPNConsoleScheduler", 1, 1024 * 10, GXOS_DEFAULT_PRIORITY -1 +priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxVPNStatusServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_VPN_STATUS);

	GxBus_ServiceUnlink(self);
}

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
void GxVPNStatusServiceConsole(handle_t self)
{
	GxMessage *msg;
	struct gx_vpn_msg vpn_msg;
	static short pptp_state = 0, l2tp_state = 0, openvpn_state = 0;
	int ret = GxCore_VPNMsgGetNextByName(&vpn_msg, 1);
	if(ret)
		return;
	if(pptp_state != vpn_msg.pptp_state) {
		msg = GxBus_MessageNew(GXMSG_VPN_STATUS);
		if(NULL == msg)
			return;
		GxMsgProperty_VPNStatus *status_msg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_VPNStatus);
		if(NULL == status_msg)
			return;
		strncpy(status_msg->dev_name, vpn_msg.dev_name, MIN(strlen(vpn_msg.dev_name), VPN_DEV_NAME_LEN));
		status_msg->type = VPN_TYPE_PPTP;
		status_msg->status = vpn_msg.pptp_state;
		pptp_state = vpn_msg.pptp_state;
		GxBus_MessageSend(msg);
	}

	if (l2tp_state != vpn_msg.l2tp_state) {
		msg = GxBus_MessageNew(GXMSG_VPN_STATUS);
		if(NULL == msg)
			return;
		GxMsgProperty_VPNStatus *status_msg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_VPNStatus);
		if(NULL == status_msg)
			return;
		strncpy(status_msg->dev_name, vpn_msg.dev_name, MIN(strlen(vpn_msg.dev_name), VPN_DEV_NAME_LEN));
		status_msg->type = VPN_TYPE_L2TP;
		status_msg->status = vpn_msg.l2tp_state;
		l2tp_state = vpn_msg.l2tp_state;
		GxBus_MessageSend(msg);
	}

	if (openvpn_state != vpn_msg.openvpn_state) {
		msg = GxBus_MessageNew(GXMSG_VPN_STATUS);
		if(NULL == msg)
			return;
		GxMsgProperty_VPNStatus *status_msg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_VPNStatus);
		if(NULL == status_msg)
			return;
		strncpy(status_msg->dev_name, vpn_msg.dev_name, MIN(strlen(vpn_msg.dev_name), VPN_DEV_NAME_LEN));
		status_msg->type = VPN_TYPE_OPENVPN;
		status_msg->status = vpn_msg.openvpn_state;
		openvpn_state = vpn_msg.openvpn_state;
		GxBus_MessageSend(msg);
	}
}

GxServiceClass vpn_status_service = {
        "vpn status service",
        GxVPNStatusServiceInit,
        GxVPNStatusServiceDestroy,
        NULL,
        GxVPNStatusServiceConsole,
        .priority_offset = 0,
};
#endif
