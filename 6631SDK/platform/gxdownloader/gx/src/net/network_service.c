#ifdef ECOS_OS
#include <sys/socket.h>
#include <net/if.h>
#include <network.h>
#include <dhcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cyg/ns/dns/dns.h>
#include "gxdlp_api.h"
#include "gxota_msg.h"
#include "network_service.h"
#include "config.h"
#include "progress.h"
#include "gui.h"

extern int net_ifconfig(char *ifrname,char *cmd,char *argument);
extern int network_down(char *if_name);
extern int network_dhcp(char *if_name);
extern void network_dhcp_release(char *if_name);

static int network_wifi_connect(char *mac, char *psk)
{
	int trycount = GxDLP_Get_WIFIConnectTimes();

	if (trycount > 5 || trycount < 0)
		trycount = 3;
	while (wifi_start(NET_IF_DEV_WIFI0, NULL, mac, psk) != GXCORE_SUCCESS) {
		gxloge("wifi connect failed\n");
		if (trycount == 0 || trycount == 1)
			return -1;
		trycount--;
		gxloge("try connect again %d\n", trycount);
	}

	return 0;
}

int network_config(char *net_if_dev)
{
	static char net_if_dev_name[20] = {NET_IF_DEV_LOOP};

	update_progress_percent(0);
	update_progress_status(GXUPDATE_CONFIG);

	if (strcmp(net_if_dev_name, NET_IF_DEV_LOOP) != 0) {
		network_dhcp_release(net_if_dev_name);
		network_down(net_if_dev_name);
	}

	memset(net_if_dev_name, 0x00, sizeof(net_if_dev_name));
	memcpy(net_if_dev_name, net_if_dev, strlen(net_if_dev));
	if (!strcmp(net_if_dev, NET_IF_DEV_WIFI0)) {
		const char *bssid = GxDLP_Get_WIFI_BSSID();
		const char *psk = GxDLP_Get_WIFI_PSK();

		//if (network_wifi_connect((char *)"08:9b:4b:97:0a:3c", (char *)"hellohello") != 0) {
		if (network_wifi_connect((char *)bssid, (char *)psk) != 0) {
			gxloge("wifi connect error");
			update_progress_error(GXUPDATE_WIFI_CONNECT_ERROR);
			return -1;
		}
	}
	network_init(net_if_dev);
	if (network_dhcp(net_if_dev) != 0) {
		gxloge("network %s dhcp error\n", net_if_dev);
		update_progress_error(GXUPDATE_NETWORK_INIT_ERROR);
		return -1;
	}
	update_progress_status(GXUPDATE_CONFIG_OK);
	return 0;
}
#endif
