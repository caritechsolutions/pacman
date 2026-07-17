#include "gxbus.h"
#include "gxmsg.h"
#include "gxavdev.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <network.h>
#include <arpa/inet.h>

#include "service/net_service.h"

#define NET_SERVICE_GET_MSG_PARAM(type) \
	type* param = GxBus_GetMsgPropertyPtr(msg,type); \
	if(param == NULL) return GXCORE_ERROR;

#define MAX_SOCK (2)
int   sock_handle[MAX_SOCK];
int net_is_init = 0;

static int recv_cnt[MAX_SOCK];

#define NET_RECV_BUF_SIZE (65*1024)
static char  *ibuf;

static char *server_ip   = "192.168.31.38";
static int   server_port = 8008;
static int net_detected = 0;

static struct network_info {
	const  char *ipaddress;
	const  char  *netmask;
	const  char  *broadcast;
	const  char  *gateway;
	const  char  *serverip;
} net_info = {
		.ipaddress = "192.168.31.172",  // Ip Address
		.netmask   = "255.255.252.0",   // netmask IP
		.broadcast = "192.168.31.255", // Broadcase IP
		.gateway   = "192.168.31.1",   // Gateway IP
		.serverip  = "192.168.31.38",   // Serverip Address
};
static struct bootp bootp_data;	

//static int network_setup_ip(const char *if_name,struct network_info *net_info)
//{
//	struct in_addr addr;
//	build_bootp_record(
//			&bootp_data,
//			if_name,
//			net_info->ipaddress,
//			net_info->netmask,
//			net_info->broadcast,
//			net_info->gateway,
//			net_info->serverip);
//
//	if (!init_net(if_name, &bootp_data)) {
//		printf("init_net failed !\n");
//		return -1;
//	}
//
//	printf("init_net succed.\n");
//	show_bootp(if_name, &bootp_data);
//
//	return 0;
//}

int socket_init(char *server_ip, int server_port)
{
	int i;

	for (i = 0; i < MAX_SOCK; i++) {
		sock_handle[i] = socket(AF_INET,SOCK_STREAM, 0);
		if (sock_handle[i] < 0) {
			extern void cyg_kmem_print_stats();
			cyg_kmem_print_stats();
			
    	    perror("socket");
    	    return 1;
    	}

		struct timeval timeout;
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		if(setsockopt(sock_handle[i], SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0 )
		{
			gxlogi("Set socket timeout failed!\n");
		}

    	struct sockaddr_in server;
    	server.sin_family = AF_INET;
    	server.sin_port = htons(server_port);
    	server.sin_addr.s_addr = inet_addr(server_ip);
    	socklen_t len = sizeof(struct sockaddr_in);
		if (connect(sock_handle[i], (struct sockaddr*)&server, len) < 0) {
    	    perror("connect Failed");
			close(sock_handle[i]);
    	    return 2;
    	}
		printf("\nNet [%d] connet success!\n", i);
	}

	return 0;
}

int network_uninit(void)
{
	int i;

	for (i = 0; i < MAX_SOCK; i++)
		close(sock_handle[i]);

	return 0;
}

status_t NetServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	//socket_init(server_ip, server_port);

	ibuf = malloc(NET_RECV_BUF_SIZE);
	if (ibuf == NULL)
		printf("[NET] ibuf malloc failed\n");

	GxBus_MessageRegister(GXMSG_NET_SEND, sizeof(GxDataNode));
	GxBus_MessageRegister(GXMSG_NET_RECEIVE, sizeof(GxDataNode));

	GxBus_MessageListen(self, GXMSG_NET_SEND);
	GxBus_MessageListen(self, GXMSG_NET_RECEIVE);
	GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
	GxBus_MessageListen(self, GXMSG_ETH_HOTPLUG_IN);
	GxBus_MessageListen(self, GXMSG_USBNET_HOTPLUG_IN);

	sch = GxBus_SchedulerCreate("NetConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 64, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	sch = GxBus_SchedulerCreate("NetMsgScheduler",  GXBUS_SCHED_MSG, 8*1024, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

GxMsgStatus NetServiceRecvMsg(handle_t self, GxMessage* msg)
{
	GxMsgStatus ret = GXCORE_SUCCESS;
	char connect_status = -1;
	extern unsigned int synopGMAC_get_link(void);

	if(msg == NULL)
		return -1;

	//gxlogi("[Net]: Recv Msg[%-2d]\n",msg->msg_id);

	switch(msg->msg_id) {
		case GXMSG_NET_SEND:
		{
			NET_SERVICE_GET_MSG_PARAM(GxDataNode);
			write(sock_handle[param->id], param->data, param->len);
			break;
		}
		case GXMSG_USBWIFI_HOTPLUG_IN:
		case GXMSG_ETH_HOTPLUG_IN:
		case GXMSG_USBNET_HOTPLUG_IN:
		{
			network_init("eth0");
			while(1) {
				connect_status = synopGMAC_get_link();
				if (connect_status == 1)
					break;
				GxCore_ThreadDelay(1000);
			}
			network_dhcp("eth0");


			gxlogi("Net Detected!\n",msg->msg_id);
			net_detected = 1;
			break;
		}
	default:
		ret = GXCORE_ERROR;
		break;
	}

	return ret;
}

void NetServiceDestroy(handle_t self)
{
	GxBus_MessageUnregister(GXMSG_NET_SEND);
	GxBus_MessageUnregister(GXMSG_NET_RECEIVE);
	GxBus_ServiceUnlink(self);
	network_uninit();
}

int dbg_net_data = 0;
void NetServiceConsole(handle_t self)
{
	GxMessage *msg = NULL;
	GxDataNode *param = NULL;
	int ret = 0, i;

	if ( !net_detected)
		return;
	if ( (!net_is_init) && net_detected) {
		ret = socket_init(server_ip, server_port);
		if ( ret == 0 ) {
			gxlogi("Net inited .......");
			net_is_init = 1;
		}
		else
			return;
	}

#if 0
	for (i = 0; i < MAX_SOCK; i++) {
	//for (i = 0; i < 1; i++) {
		ssize_t len = read(sock_handle[i], ibuf, NET_RECV_BUF_SIZE);
    	if (len > 0) {
			msg = GxBus_MessageNew(GXMSG_NET_RECEIVE);
			param = GxBus_GetMsgPropertyPtr(msg, GxDataNode);
			param->id   = i;
			param->data = ibuf;
			param->len  = len;
			GxBus_MessageSendWait(msg);
			GxBus_MessageFree(msg);
			recv_cnt[i] += len;
			if (dbg_net_data)
				printf("[NET%d] recv %d\n", i, recv_cnt[i]);
		}
	}
#endif
}

GxServiceClass net_service = {
	"net service",
	NetServiceInit,
	NetServiceDestroy,
	NetServiceRecvMsg,
	NetServiceConsole,
	.priority_offset = 0,
};

