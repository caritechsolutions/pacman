#ifdef NO_OS
#include "net_ipv4.h"
#include "net.h"
#include "bootconfig.h"
#endif
#include "common.h"
#include "gui.h"
#include "gxdlp_api.h"
#include "upgrade.h"

#ifdef NO_OS
static int network_up(int force)
{
	int state;

	if ((state = net_found()) == NETDEV_NODEV) {
		gxloge("Network device is not exist\n");
		return 1;
	} else if (state == NETDEV_UP && !force) {
		return 0;
	}

	if (g_bootconfig.protocol == BOOTNET_NONE) {
		gxloge("Configure network setting first\n");
		return 2;
	} else if (net_dev_up() == 0) {
		gxloge("Networking is enabled\n");
		return 0;
	} else {
		gxloge("Networking failure\n");
		return 3;
	}

	return 0;
}
#endif

int tftp_download_data(struct download_data *ddata)
{
#define MAX_FILE_LEN (8 * 1024 * 1024)  //由于TFTP传输文件无法文件大小,先假定文件最大大小为8M,后续考虑怎么获取文件大小,
#ifdef NO_OS
	struct GxOTA_Msg msg;
	in_addr_t ipaddr = 0;;

	char* tftp_server_ip  = NULL;
	char* host_ip = NULL; //device ip addr
	char* update_file = NULL;
	unsigned int transfer_len = 0;
	unsigned char* tftp_buf = NULL;
	unsigned char try_time = 5;

	msg.type = GXOTA_GUI_PROCESS;
	msg.percent = 10;
	gui_show(0, &msg, NULL);

	tftp_server_ip = GxDLP_Get_SERVER_IP_ADDR();
	host_ip = GxDLP_Get_HOST_IP_ADDR();
	update_file = GxDLP_Get_UpgradeFileName();

	if (tftp_server_ip == NULL || host_ip == NULL || update_file == NULL)
		return -1;

	g_bootconfig.tftp_port = GxDLP_Get_TFTP_PORT();
	if ((g_bootconfig.tftp_port != IPPORT_TFTP && g_bootconfig.tftp_port < IPPORT_RESERVED)
		|| (g_bootconfig.tftp_port < 0))
		return -1;

	tftp_buf = (unsigned char*)malloc(MAX_FILE_LEN);
	if (tftp_buf == NULL) {
		gxloge("malloc failed\n");
		return -1;
	}

	if (parse_ipaddr((char *)host_ip, &g_bootconfig.ipaddr) != 0 || !ipv4_ipaddr_valid(g_bootconfig.ipaddr)) {
		gxloge("Invalid HOST IP address\n");
		free(tftp_buf);
		return -1;
	}

	if (parse_ipaddr((char *)tftp_server_ip, &ipaddr) != 0 || !ipv4_ipaddr_valid(ipaddr)) {
		gxloge("Invalid TFTP SERVER IP address\n");
		free(tftp_buf);
		return -1;
	}

	net_init();
	if (network_up(0) != 0) {
		free(tftp_buf);
		return -1;
	}

	while(try_time--) {
		if(icmp_ping(ipaddr, 1) > 0)
			break;
		gxloge("ping failed, please check the network hardware connection");
		gxloge(" and restart the board to try again.rest try_times %d\n", try_time);
		net_dev_down();
		net_dev_up();
	}

	if (try_time == 0) {
		free(tftp_buf);
		return -1;
	}

	msg.type = GXOTA_GUI_PROCESS;
	msg.percent = 25;
	gui_show(0, &msg, NULL);

	if ((ipv4_tftp(ipaddr, g_bootconfig.tftp_port, TFTPOP_RRQ, (char *)update_file, (unsigned int)tftp_buf, &transfer_len)) != 0) {
		free(tftp_buf);
		return -1;
	}

	msg.type = GXOTA_GUI_PROCESS;
	msg.percent = 48;
	gui_show(0, &msg, NULL);
	net_dev_down();
	ddata->data = tftp_buf;
	ddata->len = transfer_len;
	return 0;
#else
	return -1;
#endif
}
