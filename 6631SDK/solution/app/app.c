#include <fcntl.h>
#include "gxgui_view.h"
#include "gui_core.h"
#include "app.h"
#include "gxmsg.h"
#include "devapi/gxirr_api.h"
#include "common/gxoem.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "module/app_nim.h"
#include "app_windows.h"
#include "app_wnd_search.h"
#include "app_netaudio.h"
#include "app_channel_info.h"
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif
#if NETAPPS_SUPPORT
#include "netapps/app_netvideo_play.h"
#if WEBLET_SUPPORT
#include "app_weblet.h"
#endif
#if WEBAPI_SUPPORT
#include "app_webapi.h"
#endif
#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
#include "netapps/dlna/app_dlna.h"
#endif
#if PROGRAM_NETUP_SUPPORT
#include "netapps/prognetup/app_prog_netup.h"
#endif
#ifdef SUBTTRANS_SUPPORT
#include "gx_subttrans.h"
#endif
#endif
#if SAT_FINDER_SUPPORT
#include "module/satfinder/dvbfinder.h"
#endif
#if CASCAM_SUPPORT
#include "app_cascam.h"
#endif
#if LCN_SUPPORT
#include "app_lcn.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if USB_TEST_SUPPORT
#include "app_usb_test.h"
#endif
#include "app_book.h"
#include "full_screen.h"
#if HDMI_CEC_SUPPORT
#include "hdmi_cec/app_cec_link.h"

extern int app_cec_init(void);
#endif
#if PVR_RECORDING_POSTER_STYLE
#include "app_cover_pvr.h"
#endif

extern status_t app_system_gui_init(void);
extern void app_system_init(void);
extern status_t signal_connect_handler(void);
handle_t g_app_msg_self = 0;
static int s_unlisten_flag = 1;
static bool s_msg_register_flag = false;

#if BOOT_AD_SUPPORT
static status_t app_gx_logo_ad_show(void)
{
    char image_path[AD_IMAGE_PATH_LEN] = {0};
	char *suffix_str = NULL;
	uint32_t i = 0;
	int ad_show_time = 0;
	int logo_ad_sel = 0;

	GxBus_ConfigGetInt(WIDGET_TEXT_LOGO_AD_KEY, &logo_ad_sel, WIDGET_TEXT_LOGO_AD_DEFAULT_KEY);
	if (1 == logo_ad_sel)
	{
		for (i = 0; i < AD_MAX_COUNT; i++)
		{
		    suffix_str = ".jpg";
			if (i == 4)
				suffix_str = ".bmp";
			snprintf(image_path,sizeof(image_path),"%s%d%s",AD_GX_IMAGE_PATH_PREFIX,i, suffix_str);
			if(GXCORE_FILE_UNEXIST != GxCore_FileExists(image_path))
			{
				GDI_PlayImage(image_path,-1,-1);
				ad_show_time = AD_GX_LOGO_DEFAULT_TIMEOUT;
				GxCore_ThreadDelay(ad_show_time);
				GDI_StopImage(image_path);
			}
		}
	}
	return GXCORE_SUCCESS;
}
#endif
// PlayerProgInfo有5800多个字节
PlayerProgInfo* app_player_prog_info_get(void)
{
    static PlayerProgInfo s_player_prog_info;

    memset(&s_player_prog_info, 0, sizeof(PlayerProgInfo));

    return &s_player_prog_info;
}

status_t app_block_msg_init(handle_t self)
{
    if(s_unlisten_flag == 1)
    {
        g_app_msg_self = self;

        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);

        GxBus_MessageListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        //GxBus_MessageListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
        GxBus_MessageListen(self, GXMSG_PLAYER_SPEED_REPORT);
#if  HIGH_DYNAMIC_RANGE_SUPPORT
        GxBus_MessageListen(self, GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED);
#endif
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        //GxBus_MessageListen(self, GXMSG_HOTPLUG_IN);
        //GxBus_MessageListen(self, GXMSG_HOTPLUG_OUT);
        //GxBus_MessageListen(self, GXMSG_EPG_FULL);
		//GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
		//GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
        GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_OUT);
        GxBus_MessageListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageListen(self, GXMSG_UPDATE_STATUS);
        //GxBus_MessageListen(self, GXMSG_BOOK_TRIGGER);
		//GxBus_MessageListen(self, GXMSG_BOOK_FINISH);
        //GxBus_MessageListen(self, GXMSG_FRONTEND_LOCKED);
        //GxBus_MessageListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_CLEAN);

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageListen(self, GXMSG_NETWORK_STATE_REPORT);
        //GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_IN);
        //GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageListen(self, GXMSG_WIFI_SCAN_AP_OVER);
        GxBus_MessageListen(self, GXMSG_WIFI_SCAN_HIDDEN_AP_OVER);
#if BLUETOOTH_SUPPORT
        GxBus_MessageListen(self, GXMSG_BLUETOOTH_SCAN_AP_OVER);
        GxBus_MessageListen(self, APPMSG_BLUETOOTH_UI_CONTROL);
#endif

#endif

//#endif
#if TKGS_SUPPORT
        GxBus_MessageListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif
#if SAT_FINDER_SUPPORT
        GxBus_MessageListen(self, GXMSG_SAT_FIND_USER3);
#endif
    }
    if(s_unlisten_flag > 0)
    {
        s_unlisten_flag--;
    }
    return GXCORE_SUCCESS;
}

status_t app_block_msg_destroy(handle_t self)
{
    if(s_unlisten_flag == 0)
    {
        GxBus_MessageUnListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageUnListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        //GxBus_MessageUnListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_SPEED_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
#if  HIGH_DYNAMIC_RANGE_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED);
#endif
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        //GxBus_MessageUnListen(self, GXMSG_HOTPLUG_IN);
        //GxBus_MessageUnListen(self, GXMSG_HOTPLUG_OUT);
		//GxBus_MessageUnListen(self, GXMSG_EPG_FULL);
		//GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
        //GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_IN);
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_OUT);
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageUnListen(self, GXMSG_UPDATE_STATUS);
		//GxBus_MessageUnListen(self, GXMSG_BOOK_TRIGGER);
        //GxBus_MessageUnListen(self, GXMSG_BOOK_FINISH);
        //GxBus_MessageUnListen(self, GXMSG_FRONTEND_LOCKED);
        //GxBus_MessageUnListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_CLEAN);

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_NETWORK_STATE_REPORT);
        //GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_IN);
        //GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_AP_OVER);
        GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_HIDDEN_AP_OVER);
#if BLUETOOTH_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_BLUETOOTH_SCAN_AP_OVER);
        GxBus_MessageUnListen(self, APPMSG_BLUETOOTH_UI_CONTROL);
#endif

#endif
//#endif
#if TKGS_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif
#if SAT_FINDER_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_SAT_FIND_USER3);
#endif
        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);
    }
    s_unlisten_flag++;

    return GXCORE_SUCCESS;
}

status_t app_msg_init(handle_t self)
{
    if(s_msg_register_flag == false)
    {
        GxBus_MessageRegister(GXMSG_SUBTITLE_INIT, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_DESTROY, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_HIDE, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_SHOW, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_DRAW, sizeof(GxMsgProperty_AppSubtitle));
        GxBus_MessageRegister(GXMSG_SUBTITLE_CLEAN, sizeof(GxMsgProperty_AppSubtitle));

#if SAT_FINDER_SUPPORT
        GxBus_MessageRegister(GXMSG_SAT_FIND_USER3, sizeof(dream_ui_msg_t));
#endif
#if BLUETOOTH_SUPPORT
        GxBus_MessageRegister(APPMSG_BLUETOOTH_UI_CONTROL, sizeof(GxMsgProperty_ApList));
#endif
#if CASCAM_SUPPORT
        GxBus_MessageRegister(GXMSG_CASCAM_EVENT, sizeof(CascamOsdEvent));
#endif

#if PLUGINSTORE_SUPPORT
        extern void Plugin_msg_register(void);
        Plugin_msg_register();
#endif

#if WEBLET_SUPPORT
		extern void app_weblet_msg_register(void);
		app_weblet_msg_register();
#endif

#if WEBAPI_SUPPORT
    GxBus_MessageRegister(APPMSG_WEBAPI_UI_CONTROL, sizeof(AppMsg_WebapiUIControl));
#endif

#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
    GxBus_MessageRegister(APPMSG_DLNA_UI_CONTROL, sizeof(AppMsg_DlnaUIControl));
#endif
#ifdef SUBTTRANS_SUPPORT
    GxBus_MessageRegister(APPMSG_SUBT_TRANS_CONTROL, sizeof(AppMsg_SubtTransControl));
#endif
#if IPTV_LITE_SUPPORT
    GxBus_MessageRegister(APPMSG_IPTV_POWERON_PLAY,0 );
#endif
#if USB_TEST_SUPPORT
        GxBus_MessageRegister(APPMSG_USB_TEST, sizeof(GxMsgProperty_UsbTest));
#endif
#if HDMI_CEC_SUPPORT
        GxBus_MessageRegister(APPMSG_HDMI_CEC_UI_CONTROL, sizeof(AppMsg_CecUIControl));
#endif

        s_msg_register_flag = true;
    }

    if(s_unlisten_flag == 1)
    {
        g_app_msg_self = self;

        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);

        GxBus_MessageRegister(GXMSG_FRONTEND_DISH_MOVE, sizeof(MotorTip));
        GxBus_MessageListen(self, GXMSG_FRONTEND_DISH_MOVE);
        GxBus_MessageListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        GxBus_MessageListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
#if  HIGH_DYNAMIC_RANGE_SUPPORT
        GxBus_MessageListen(self, GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED);
#endif
        GxBus_MessageListen(self, GXMSG_PLAYER_SPEED_REPORT);
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        GxBus_MessageListen(self, GXMSG_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_HOTPLUG_OUT);
		//GxBus_MessageListen(self, GXMSG_EPG_FULL);
#if USB_TO_SERIAL_SUPPORT
        GxBus_MessageListen(self, GXMSG_USBSERIAL_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USBSERIAL_HOTPLUG_OUT);
#endif
#if WIFI_SUPPORT
        GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
#endif
#if BLUETOOTH_SUPPORT
        GxBus_MessageListen(self, GXMSG_USBBT_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USBBT_HOTPLUG_OUT);
        GxBus_MessageListen(self, GXMSG_BLUETOOTH_SCAN_AP_OVER);
        GxBus_MessageListen(self, GXMSG_USBBT_SINK_PLAYING_TITLE_INFO);
        GxBus_MessageListen(self, GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO);
        GxBus_MessageListen(self, GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO);
        GxBus_MessageListen(self, GXMSG_USBBT_SINK_TRACK_CHANGED);
        GxBus_MessageListen(self, APPMSG_BLUETOOTH_UI_CONTROL);
#endif
#if MOD_3G_SUPPORT
        GxBus_MessageListen(self, GXMSG_USB3G_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USB3G_HOTPLUG_OUT);
#endif
#if ETH_SUPPORT
		GxBus_MessageListen(self, GXMSG_ETH_HOTPLUG_IN);
		GxBus_MessageListen(self, GXMSG_ETH_HOTPLUG_OUT);
#endif
#if USB_ETH_SUPPORT
		GxBus_MessageListen(self, GXMSG_USBNET_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USBNET_HOTPLUG_OUT);
#endif
        GxBus_MessageRegister(APPMSG_MESSAGE, sizeof(AppMsgMessageClass));
        GxBus_MessageListen(self, APPMSG_MESSAGE);
#if USB_MICROPHONE_SUPPORT
        GxBus_MessageListen(self, GXMSG_USBAUDIO_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_USBAUDIO_HOTPLUG_OUT);
#endif
		GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_IN);
        GxBus_MessageListen(self, GXMSG_HDMI_HOTPLUG_OUT);
		#if HDMI_HDCP_SUPPORT
	    GxBus_MessageListen(self, GXMSG_HDMI_HDCP_FAIL);
	    GxBus_MessageListen(self, GXMSG_HDMI_HDCP_SUCCESS);
		#endif
        GxBus_MessageListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageListen(self, GXMSG_UPDATE_STATUS);
        GxBus_MessageListen(self, GXMSG_BOOK_TRIGGER);
        GxBus_MessageListen(self, GXMSG_BOOK_FINISH);
        GxBus_MessageListen(self, GXMSG_FRONTEND_LOCKED);
        GxBus_MessageListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageListen(self, GXMSG_SUBTITLE_CLEAN);
#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT || BOUQUET_SUPPORT
        GxBus_MessageRegister(APPMSG_ACU_STATE_REPORT, sizeof(AppMsgAcuStateUiCotrol));
        GxBus_MessageListen(self, APPMSG_ACU_STATE_REPORT);
#endif
//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageListen(self, GXMSG_NETWORK_STATE_REPORT);
        GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_IN);
        GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageListen(self, GXMSG_WIFI_SCAN_AP_OVER);
        GxBus_MessageListen(self, GXMSG_WIFI_SCAN_HIDDEN_AP_OVER);
#if PLUGINSTORE_SUPPORT
        GxBus_MessageListen(self, APPMSG_PLUGIN_SOURCE_UI_CONTROL);
#endif
#if NETAPPS_SUPPORT
        GxBus_MessageRegister(APPMSG_NETVIDEO_PLAY_START, sizeof(AppMsgVideoPlayer));
        GxBus_MessageListen(self, APPMSG_NETVIDEO_PLAY_START);
        GxBus_MessageRegister(APPMSG_NETVIDEO_DOWN_STATE_REPORT,sizeof(AppMsgNetappsDownReport));
        GxBus_MessageListen(self, APPMSG_NETVIDEO_DOWN_STATE_REPORT);
#endif
#if WEBLET_SUPPORT
        GxBus_MessageListen(self, APPMSG_WEBLET_UI_CONTROL);
#endif
#if WEBAPI_SUPPORT
        GxBus_MessageListen(self, APPMSG_WEBAPI_UI_CONTROL);
#endif
#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
        GxBus_MessageListen(self, APPMSG_DLNA_UI_CONTROL);
#endif
#if PROGRAM_NETUP_SUPPORT
        GxBus_MessageRegister(APPMSG_PROG_NETUP_STATE_REPORT, sizeof(AppMsgProgupStateUiCotrol));
        GxBus_MessageListen(self, APPMSG_PROG_NETUP_STATE_REPORT);
#endif
#ifdef SUBTTRANS_SUPPORT
        GxBus_MessageListen(self, APPMSG_SUBT_TRANS_CONTROL);
#endif
#endif

//#endif
#if TKGS_SUPPORT
        GxBus_MessageListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif
#if SAT_FINDER_SUPPORT
        GxBus_MessageListen(self, GXMSG_SAT_FIND_USER3);
#endif
#if CASCAM_SUPPORT
        GxBus_MessageListen(self, GXMSG_CASCAM_EVENT);
#endif
#if SAT2IP_SERVER_SUPPORT
        GxBus_MessageListen(self, GXMSG_SATIP_PLAY_START);
        GxBus_MessageListen(self, GXMSG_SATIP_PLAY_STOP);
#endif
#if DVB2IP_SERVER_SUPPORT
        GxBus_MessageRegister(GXMSG_DVB2IP_PLAY_START, sizeof(Dvb2ipStartMsg));
        GxBus_MessageRegister(GXMSG_DVB2IP_PLAY_STOP, 0);
        GxBus_MessageListen(self, GXMSG_DVB2IP_PLAY_START);
        GxBus_MessageListen(self, GXMSG_DVB2IP_PLAY_STOP);
#endif
#if PARENTAL_LOCK_SUPPORT
        GxBus_MessageRegister(APPMSG_EPG_PARENTAL_SEND, sizeof(GxMsgProperty_EpgParentalInfo));
        GxBus_MessageListen(self, APPMSG_EPG_PARENTAL_SEND);
        GxBus_MessageListen(self, GXMSG_EPG_PARENTAL_SEND);
#endif
#if IPTV_LITE_SUPPORT
        GxBus_MessageListen(self,APPMSG_IPTV_POWERON_PLAY);
#endif
#if USB_TEST_SUPPORT
        GxBus_MessageListen(self, APPMSG_USB_TEST);
#endif
#if HDMI_CEC_SUPPORT
		GxBus_MessageListen(self, APPMSG_HDMI_CEC_UI_CONTROL);
#endif
    }
    if(s_unlisten_flag > 0)
    {
        s_unlisten_flag--;
    }
    return GXCORE_SUCCESS;
}

status_t app_msg_destroy(handle_t self)
{
    if(s_unlisten_flag == 0)
    {
        GxBus_MessageUnListen(self, GXMSG_FRONTEND_DISH_MOVE);
        GxBus_MessageUnregister(GXMSG_FRONTEND_DISH_MOVE);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_NEW_PROG_GET);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STATUS_REPLY);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_STOP_OK);
        GxBus_MessageUnListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_STATUS_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_SPEED_REPORT);
        GxBus_MessageUnListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
#if  HIGH_DYNAMIC_RANGE_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED);
#endif
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
        GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_OUT);
		//GxBus_MessageUnListen(self, GXMSG_EPG_FULL);
#if USB_TO_SERIAL_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_USBSERIAL_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USBSERIAL_HOTPLUG_OUT);
#endif
#if WIFI_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
#endif
#if BLUETOOTH_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_USBBT_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USBBT_HOTPLUG_OUT);
        GxBus_MessageUnListen(self, GXMSG_BLUETOOTH_SCAN_AP_OVER);
        GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_PLAYING_TITLE_INFO);
        GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO);
        GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO);
        GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_TRACK_CHANGED);
        GxBus_MessageUnListen(self, APPMSG_BLUETOOTH_UI_CONTROL);
#endif
#if MOD_3G_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_OUT);
#endif
#if ETH_SUPPORT
		GxBus_MessageUnListen(self, GXMSG_ETH_HOTPLUG_IN);
		GxBus_MessageUnListen(self, GXMSG_ETH_HOTPLUG_OUT);
#endif
#if USB_ETH_SUPPORT
		GxBus_MessageUnListen(self, GXMSG_USBNET_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USBNET_HOTPLUG_OUT);
#endif
        GxBus_MessageUnListen(self, APPMSG_MESSAGE);
        GxBus_MessageUnregister(APPMSG_MESSAGE);
#if USB_MICROPHONE_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_USBAUDIO_HOTPLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_USBAUDIO_HOTPLUG_OUT);
#endif
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_IN);
		GxBus_MessageUnListen(self, GXMSG_HDMI_HOTPLUG_OUT);
		#if HDMI_HDCP_SUPPORT
	    GxBus_MessageUnListen(self, GXMSG_HDMI_HDCP_FAIL);
	    GxBus_MessageUnListen(self, GXMSG_HDMI_HDCP_SUCCESS);
		#endif
        GxBus_MessageUnListen(self, GXMSG_HOTPLUG_CLEAN);
        GxBus_MessageUnListen(self, GXMSG_UPDATE_STATUS);
        GxBus_MessageUnListen(self, GXMSG_BOOK_TRIGGER);
        GxBus_MessageUnListen(self, GXMSG_BOOK_FINISH);
        GxBus_MessageUnListen(self, GXMSG_FRONTEND_LOCKED);
        GxBus_MessageUnListen(self, GXMSG_FRONTEND_UNLOCKED);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_INIT);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DESTROY);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_HIDE);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_SHOW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_DRAW);
        GxBus_MessageUnListen(self, GXMSG_SUBTITLE_CLEAN);
#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT || BOUQUET_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_ACU_STATE_REPORT);
        GxBus_MessageUnregister(APPMSG_ACU_STATE_REPORT);
#endif
//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_NETWORK_STATE_REPORT);
        GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_IN);
        GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_OUT);
        GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_AP_OVER);
        GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_HIDDEN_AP_OVER);
#if PLUGINSTORE_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_PLUGIN_SOURCE_UI_CONTROL);
        extern void Plugin_msg_unregister(void);
        Plugin_msg_unregister();
#endif
#if WEBLET_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_WEBLET_UI_CONTROL);
		extern void app_weblet_msg_unregister(void);
		app_weblet_msg_unregister();
#endif
#if WEBAPI_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_WEBAPI_UI_CONTROL);
        GxBus_MessageUnregister(APPMSG_WEBAPI_UI_CONTROL);
#endif
#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_DLNA_UI_CONTROL);
        GxBus_MessageUnregister(APPMSG_DLNA_UI_CONTROL);
#endif
#if PROGRAM_NETUP_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_PROG_NETUP_STATE_REPORT);
        GxBus_MessageUnregister(APPMSG_PROG_NETUP_STATE_REPORT);
#endif
#ifdef SUBTTRANS_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_SUBT_TRANS_CONTROL);
        GxBus_MessageUnregister(APPMSG_SUBT_TRANS_CONTROL);
#endif
#if NETAPPS_SUPPORT
        GxBus_MessageUnListen(self,APPMSG_NETVIDEO_PLAY_START);
        GxBus_MessageUnregister(APPMSG_NETVIDEO_PLAY_START);
        GxBus_MessageUnListen(self, APPMSG_NETVIDEO_DOWN_STATE_REPORT);
        GxBus_MessageUnregister(APPMSG_NETVIDEO_DOWN_STATE_REPORT);
#endif
#endif
//#endif
#if TKGS_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_TKGS_UPDATE_STATUS);
#endif
#if SAT_FINDER_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_SAT_FIND_USER3);
#endif
#if CASCAM_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_CASCAM_EVENT);
#endif
#if SAT2IP_SERVER_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_SATIP_PLAY_START);
        GxBus_MessageUnListen(self, GXMSG_SATIP_PLAY_STOP);
#endif
#if DVB2IP_SERVER_SUPPORT
        GxBus_MessageUnListen(self, GXMSG_DVB2IP_PLAY_START);
        GxBus_MessageUnListen(self, GXMSG_DVB2IP_PLAY_STOP);
        GxBus_MessageUnregister(GXMSG_DVB2IP_PLAY_START);
        GxBus_MessageUnregister(GXMSG_DVB2IP_PLAY_STOP);
#endif
        GxBus_MessageEmptyByOps(&g_service_list[SERVICE_GUI_VIEW]);
#if PARENTAL_LOCK_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_EPG_PARENTAL_SEND);
        GxBus_MessageUnListen(self, GXMSG_EPG_PARENTAL_SEND);
        GxBus_MessageUnregister(APPMSG_EPG_PARENTAL_SEND);
#endif
#if IPTV_LITE_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_IPTV_POWERON_PLAY);
        GxBus_MessageUnregister(APPMSG_IPTV_POWERON_PLAY);
#endif
#if USB_TEST_SUPPORT
        GxBus_MessageUnListen(self, APPMSG_USB_TEST);
        GxBus_MessageUnregister(APPMSG_USB_TEST);
#endif
#if HDMI_CEC_SUPPORT
		GxBus_MessageUnListen(self, APPMSG_HDMI_CEC_UI_CONTROL);
        GxBus_MessageUnregister(APPMSG_HDMI_CEC_UI_CONTROL);
#endif
    }
    s_unlisten_flag++;

    return GXCORE_SUCCESS;
}

// modify the IRR simple code filter gate
static void config_irr_filter_gate(void)
{
	int sensitivity = 7;    //irr_simple_ignore_num is (MAX_SENS  - sensitivity) 3
    int ret;

    ret=GxIRR_Init();
    if(ret == 0)
    {
        if(GxIRR_SetSensitivity(sensitivity) != 0)
            app_log_error("\n>>>>>config irr err<<<<<<\n");
        GxIRR_Close();
    }
    else
        app_log_error("\n>>>>>open irr err<<<<<<\n");

}

extern void app_first_play(GxMsgProperty_NodeByPosGet *prog_node);
extern void app_first_play_after(void);
extern void app_play_frontend_config(GxMsgProperty_NodeByPosGet *prog_node);
extern void app_system_output_config(void);

static int _check_first_dvb_play(void)
{
    int iptv_play = 0;
#if IPTV_LITE_SUPPORT > 0
    GxBus_ConfigGetInt(POWERON_PLAY_KEY, &iptv_play, POWERON_PLAY_VALUE);
#endif
    if((0 == iptv_play) && (0 < g_AppPlayOps.normal_play.play_total))
    {
        return 1;
    }
    return 0;
}

static void app_boot_set_tp(void)
{
    GxMsgProperty_NodeByPosGet node = {0};
    if(1 == _check_first_dvb_play())
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            app_play_frontend_config(&node);
        }
    }
#if 0 == NDEMOD_WITH_1TUNER
    app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);
#endif
}

static void app_user_play(void)
{
    GxMsgProperty_NodeByPosGet node = {0};

    //open player for play
    app_player_open(PLAYER_FOR_NORMAL);

    // DVB play
    if(1 == _check_first_dvb_play())
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            app_first_play(&node);
        }
    }
}

// before play
void app_boot_init_before_ui(void)
{
//printf("\n%s, %d\n", __func__, __LINE__);
    // Print output init
#if ADVANCED_PRINT_SUPPORT
    extern int advanced_print_exec(void);
    advanced_print_exec();
#endif
    // BSP GPIO init
    app_bsp_gpio_init();
    // BSP Panel init
    app_bsp_panel_init();

#if OTT_SUPPORT
    return;
#endif
    // frontend init，after frontend service
{
    extern void app_frontend_init(void);
    app_frontend_init();
}
    // prog init
    /// loader default if dbase is empty
{
    extern void default_program_load(void);
	default_program_load();
}
    // Play list init
	g_AppPlayOps.program_play_init();
    // psi init
#if PSI_MODULE_SUPPORT
	app_psi_module_init();
#endif
#if OTA_SUPPORT
    extern void app_ota_dlp_init(void);
    app_ota_dlp_init();
#endif
    // TODO:
#if CASCAM_SUPPORT
    cascam_app_cas_init();
#endif
    //
#if CA_SUPPORT
    app_extend_register();
#endif
    // DVB CI
#if CI_SUPPORT
    extern void cim_init(void);
    cim_init();
    ci.init();
#endif
    // EMM init
#if EMM_SUPPORT
    app_emm_init();
#endif
#if NET_AUDIO_SUPPORT
    app_netaudio_tsbuff_time_init();
#endif
    // unmute
    //
#if APP_DEINTERLACE_SUPPORT
    VideoDeinterlaceParam param = {0};
    param.flag = VD_FLAG_DEINTERLACE_Y | VD_FLAG_DEINTERLACE_UV;
#ifdef FULL_HD_PP_MODE_ENABLE
    param.max_width = DEFAULT_PP_SUPPORT_WIDTH;
    param.max_height = DEFAULT_PP_SUPPORT_HEIGHT;
#else
    param.max_width = 1280;
    param.max_height = 720;
#endif
    GxPlayer_SetVideoDeinterlaceParam(&param);
#endif
    app_player_femem_init();

    app_system_performace_dynamic_change(DVB_PLAY_MODE);
    app_boot_set_tp();
    //printf("\n%s, %d\n", __func__, __LINE__);
}

// after play & ui
extern int app_resolution_init(void);
extern void app_boot_pvr_config(void);

// return 1: iptv play , else: DVB Play
static int app_boot_init_after_play(void)
{
    // book init
    //g_AppBook.enable(); //enable book event
    // OTA
#if HDMI_HDCP_SUPPORT
extern int app_hdcp_create_thread(void);
	app_hdcp_create_thread();
#endif
#if OTA_SUPPORT
    extern void app_ota_backend_polling_timer_init(void);
    app_ota_backend_polling_timer_init();
#endif
    //
	GxOem_Init();
    // irr config
	config_irr_filter_gate();
    // LCN
#if LCN_SUPPORT
	app_search_register_add_extend_table_callback(app_search_add_extend_table_call_back);
#if LCN_CONFLICT_HANDLE_SUPPORT
    app_lcn_conflict_handle_common_register();
#endif
#endif
    // after psi module init
#if BOUQUET_SUPPORT
    GxMsgProperty_NodeNumGet node_num_get = {0};
    app_bouquet_init();
    node_num_get.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));

    if(node_num_get.node_num > 0)
    {
        g_AppBat.start(&g_AppBat);
    }
#endif

#if NO_AVPROG_SAVE_SUPPORT
    GXSearch_NoAVProgSaveEnable();
#endif
    //
#if(DVBT2_ANT_POWER > 0)
    extern int app_dvbt2_antenna_power_init(void);
    app_dvbt2_antenna_power_init();
#endif
#if LNB_SHORT_DETECT_SUPPORT
    extern int app_dvbs_lnb_detect_init(void);
    app_dvbs_lnb_detect_init();
#endif
    //
#if POWER_ON_CONTROL_SUPPORT
    extern void app_system_setting_power_control(void);
    app_system_setting_power_control();
#endif

#if PANEL_SETTING_SUPPORT
    app_panel_show_prog_num(PANEL_DISPLAY_TIME);
#endif

#if SW_FILTER_SUPPORT
    gx_search_softfilter_register(GX_PMT_SW_FILTER);
#endif

#if AUTO_STANDBY_SUPPORT
	extern void app_set_auto_sleep_time(void);
    app_set_auto_sleep_time();
#endif
    app_resolution_init();
    app_boot_pvr_config();

    // time start, after extra service
    //g_AppTime.init(&g_AppTime);
    //g_AppTime.start(&g_AppTime);

/// IPTV play
#if IPTV_LITE_SUPPORT
    {
        extern int app_iptv_lite_is_active(void);
        extern int app_iptv_poweron_play(void) ;
        extern int app_iptv_poweron_play_flag_get(void);
        extern int app_iptv_poweron_play_tips(void);
        int poweron_play = 0;
        GxBus_ConfigGetInt(POWERON_PLAY_KEY, &poweron_play, POWERON_PLAY_VALUE);
        if(poweron_play == 1)
        {
            if ( app_iptv_poweron_play_flag_get() )
            {
                g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
                g_AppFullArb.state.tv = STATE_ON;
                app_iptv_poweron_play_tips();

                if ( app_iptv_lite_is_active() )
                {
                    app_iptv_poweron_play();
                }
            }

            return 1;
        }
    }
#endif
    // full arbitrate
#if SKIN_SUPPORT
    // skin will not create wnd_full_screen. g_AppFullArb init in full_screen create, it is a bug
    // suggest it may merger to public sdk version
    if ( GUI_CheckDialog(WND_FULL_SCREEN) == GXCORE_SUCCESS )
    {
        if ( g_AppFullArb.draw[EVENT_TV_RADIO] )
        {
            g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        }
        g_AppFullArb.timer_start();
    }
#else
    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    g_AppFullArb.timer_start();
#endif
#if HDMI_CEC_SUPPORT
    app_cec_init();
#endif
    return 0;
}

static void app_boot_process(void)
{
    int iptv_play = 0;

    //printf("\n%s, %d\n", __func__, __LINE__);
    iptv_play = app_boot_init_after_play();
    if((0 < g_AppPlayOps.normal_play.play_total) && (0 == iptv_play))
    {
        app_first_play_after();
        const char *wnd = GUI_GetFocusWindow();
        if (strcmp(wnd, WND_FULL_SCREEN) == 0 )
        {
            app_channel_info_create_dialog();
        }
        GUI_SetInterface("flush", NULL);
    }

    /*  modify 20230606
    WND_CHANNEL_INFO in gui_init create,do not protect. it will confilck book msg thread
    so delay book msg.
    */
    g_AppBook.enable(); //enable book event
    //printf("\n%s, %d\n", __func__, __LINE__);
}

static char thiz_play_flag = 0;
static void _boot_config_thread(void* arg)
{
    extern void app_system_output_config(void);
    GxCore_ThreadDetach();
    // output config, need after player service init
    app_system_output_config();
    if(thiz_play_flag++ > 0)
    {
        // play
        app_set_hardware_unmute();
        app_user_play();
    }

}

void app_boot_config(void)
{
    int handle = 0;
    GxCore_ThreadCreate("boot config", &handle, _boot_config_thread, NULL, 32*1024, GXOS_DEFAULT_PRIORITY);
}

status_t app_init(void)
{
	status_t ret = GXCORE_SUCCESS;

    // ui
    // GUI XML Signal function connect
	ret = signal_connect_handler();
	if(GXCORE_SUCCESS != ret)
	{
		app_log_error("[signal_connect_handler] ERROR\n");
	}
	gxapp_main_menu_theme_signal_init();
	ret = app_system_gui_init();
	if(GXCORE_SUCCESS != ret)
	{
        app_log_error("GUI_Init Error \n");
        return GXCORE_ERROR;
	}
#if LEARN_RCU_SUPPORT
    extern void app_remote_learn_rcu_result_init(void);
    app_remote_learn_rcu_result_init();
#endif

#if BOOT_AD_SUPPORT
	app_gx_logo_ad_show();
#endif

    if(app_ott_config_get()==false)
    {
        g_AppTime.init(&g_AppTime);
        g_AppTime.start(&g_AppTime);

#if SKIN_SUPPORT
        extern int  gxapp_skin_flash_get_recovry_status(void);
        if ( gxapp_skin_flash_get_recovry_status() <=0 )
        {
            ret = app_fuse_spec_system_init();
            GUI_CreateDialog(WND_FULL_SCREEN);
            app_fuse_spec_system_after_init(ret);
        }
        else
        {
            GxOem_Init();
            // irr config
            config_irr_filter_gate();
            app_log_info("GUI_Init recovery Success \n");
            return GXCORE_SUCCESS;
        }
#else
        ret = app_fuse_spec_system_init();
        GUI_CreateDialog(WND_FULL_SCREEN);
        app_fuse_spec_system_after_init(ret);
#endif
        #if PVR_RECORDING_POSTER_STYLE
        app_cover_pvr_init();
        #endif

        // play
        if(thiz_play_flag++ > 0)
        {
            app_set_hardware_unmute();
            app_user_play();
        }
        // after play
        app_boot_process();
    }
    else
    {
#if HDMI_HDCP_SUPPORT
        extern int app_hdcp_create_thread(void);
        app_hdcp_create_thread();
#endif
#if POWER_ON_CONTROL_SUPPORT
        extern void app_system_setting_power_control(void);
        app_system_setting_power_control();
#endif
#if PANEL_SETTING_SUPPORT
        app_panel_show_prog_num(PANEL_DISPLAY_TIME);
#endif
        app_resolution_init();
        g_AppTime.init(&g_AppTime);
        g_AppTime.start(&g_AppTime);

        GxOem_Init();
#if SKIN_SUPPORT
        extern int  gxapp_skin_flash_get_recovry_status(void);
        if ( gxapp_skin_flash_get_recovry_status() <=0 )
        {
            GUI_CreateDialog(WND_MAIN_MENU);
        }
        else
        {
            // irr config
            config_irr_filter_gate();
            app_log_info("GUI_Init recovery Success \n");
            return GXCORE_SUCCESS;
        }
#else
        GUI_CreateDialog(WND_MAIN_MENU);
#endif
#if HDMI_CEC_SUPPORT
        app_cec_init();
#endif

    }

    app_log_info("GUI_Init Success \n");
    return GXCORE_SUCCESS;
}

