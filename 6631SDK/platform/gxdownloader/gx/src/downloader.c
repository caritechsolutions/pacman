#include "config.h"
#include "gxdlp_api.h"
#include "gxota_msg.h"
#include "downloader.h"
#include "fe.h"
#include "usb_download.h"
#include "dsmcc_download.h"
#include "http_download.h"
#include "gxota_api.h"
#include "network_service.h"
#include "gxupdate_api.h"
#include "gxupdate_msg.h"
#include "progress.h"

extern __attribute__((weak)) int network_config(char *net_if_dev)
{
	return -1;
}

int download_data(struct download_data *ddata, GxDLP_Download_Type download_type)
{
	if(GXDLP_USB_DOWNLOAD == download_type) {
#ifdef CONFIG_ENABLE_USB_UPGRADE
		if (0 != usb_download_data(ddata)) {
			gxloge("USB Get File Failed!\r\n");
			return -1;
		}
#else
		gxloge("don't support usb download way!\r\n");
		return -1;
#endif
	} else if(GXDLP_OTA_DDTEX_DOWNLOAD == download_type) {
#ifdef CONFIG_ENABLE_OTA
		int trycount = GxDLP_Get_OTADDTEXDownTimes();

		if (trycount > 3 || trycount < 0)
			trycount = 1;
		if (-1 == frontend_set()) {
			gxloge("OTA Frontend Set Failed!\r\n");
			return -1;
		}

		while (GXOTA_DATA_ERROR == dsmcc_download_data(ddata)) {
			gxloge("OTA DDTEX File Failed!\r\n");
			if (trycount == 0 || trycount == 1)
				return -1;
			trycount--;
			gxloge("try get again %d\r\n", trycount);
		}
#else
		gxloge("don't support ota ddtex download way!\r\n");
		return -1;
#endif
	} else if(GXDLP_NET_TFTP_DOWNLOAD == download_type) {
#if defined(CONFIG_NET) && defined(CONFIG_ENABLE_NET_TFTP)
		if (0 != tftp_download_data(ddata)) {
			gxloge("NET Get File Failed!\r\n");
			return -1;
		}
#else
		gxloge("don't support tftp download way!\r\n");
		return -1;
#endif
	} else if(GXDLP_NET_HTTP_DOWNLOAD == download_type) {
#if defined(CONFIG_NET) && defined(CONFIG_ENABLE_NET_HTTP)
		int i = 0;
		int ret = 0;
		static int wait_hotplug = 1;
		GxDLP_Net_Device_Type highest_net_dev = GXDLP_NET_DEVICE_ETH;

		GxDLP_Get_HighestPriNetDevice(&highest_net_dev);
		if (wait_hotplug) {
			wait_hotplug = 0;
			GxCore_ThreadDelay(5000); //wait hotplug thread detect usbwifi
		}
		for (i = 0; i < GXDLP_NET_DEVICE_MAX; i++) {
			int trycount = GxDLP_Get_HTTPGetTimes();
			int net_device = GXDLP_NET_DEVICE_ETH;
			if (trycount > 3 || trycount <= 0)
				trycount = 1;

			if (i == 0)
				net_device = highest_net_dev;
			else {
				if (i == highest_net_dev)
					continue;
				net_device = i;
			}

			if (net_device == GXDLP_NET_DEVICE_ETH) {
#ifdef CONFIG_ETH_SUPPORT
				gxlogd("network_config eth0\n");
				if (network_config(NET_IF_DEV_ETH0) != 0)
					continue;
#else
				continue;
#endif
			} else if (net_device == GXDLP_NET_DEVICE_USBETH) {
#ifdef CONFIG_USB_ETH_SUPPORT
				gxlogd("network_config eth1\n");
				if (network_config(NET_IF_DEV_ETH1) != 0)
					continue;
#else
				continue;
#endif
			} else if (net_device == GXDLP_NET_DEVICE_WIFI) {
#ifdef CONFIG_WIFI_SUPPORT
				gxlogd("network_config wifi\n");
				if (network_config(NET_IF_DEV_WIFI0) != 0)
					continue;
#else
				continue;
#endif
			} else
				continue;

			while ((ret = http_download_data(ddata)) != 0) {
				gxloge("HTTP Get File Failed!\r\n");
				if (trycount == 0 || trycount == 1)
					break;
				trycount--;
				gxloge("try get again %d\r\n", trycount);
			}

			if (ret == 0)
				break;
		}

		if (i == GXDLP_NET_DEVICE_MAX)
			return -1;
#else
		gxloge("don't support http download way!\r\n");
		return -1;
#endif
	} else {
		gxloge("don't support this way!\r\n");
		return -1;
	}

	return 0;
}

int GxUpdate_SetConfig(GxUpdateCfg *cfg)
{
	if (cfg == NULL) {
		return -2;
	}
	if (cfg->type == GXDLP_NET_HTTP_DOWNLOAD) {
		GxDLP_Set_URL(cfg->http.url);
		GxDLP_Set_HTTPGetTimes(cfg->http.http_get_times);
	}

	GxDLP_Set_UpdateType(cfg->type);

	GxDLP_Sync();

	return 0;
}

int GxUpdate_SetWifiParam(GxUpdateWifiParam *wifi)
{
	if (wifi == NULL) {
		gxloge("param wifi is NULL\n");
		return -2;
	}

	if (wifi->is_highestpri)
		GxDLP_Set_HighestPriNetDevice(GXDLP_NET_DEVICE_WIFI);
	GxDLP_Set_WIFI_BSSID(wifi->bssid);
	GxDLP_Set_WIFI_PSK(wifi->psk);
	GxDLP_Set_WIFIConnectTimes(wifi->connect_times);

	GxDLP_Sync();

	return 0;
}

int GxUpdate_SetHighesetPriNetDevice(GxDLP_Net_Device_Type net_dev)
{
	GxDLP_Set_HighestPriNetDevice(net_dev);

	GxDLP_Sync();

	return 0;
}

int GxUpdate_SetUpgradeTypeList(GxUpdateTypeListCfg *list_cfg)
{
	int i = 0;

	for (i = 0; i < GXDLP_DOWNLOAD_SEQ_MAX; i++) {
		if (list_cfg->type_list[i] == GXDLP_OTA_FAILED
		    || list_cfg->type_list[i] >= GXDLP_DOWNLOADER_MAX)
			GxDLP_Set_UpdateTypeByIndex(GXDLP_NONE_DOWNLOAD, i);
		else
			GxDLP_Set_UpdateTypeByIndex(list_cfg->type_list[i], i);
	}

	GxDLP_Sync();

	return 0;
}

#ifndef NO_OS
int GxUpdate_StartInstall(void)
{
	GxUpdateMsg msg;
	update_get_cur_progress(&msg);
	if (msg.download_type == GXDLP_NET_HTTP_DOWNLOAD)
		http_upgrade_set_install_flag(1);
	else if (msg.download_type == GXDLP_OTA_DDTEX_DOWNLOAD)
		GxOTA_Start_Write_Flash();
	return 0;
}
#endif
