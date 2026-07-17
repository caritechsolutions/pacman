/*
 * =====================================================================================
 *
 *       Filename:  app_top.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年08月29日 15时10分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#include <fcntl.h>
#include "app.h"
#include "full_screen.h"
#include "gxextra.h"
#include "gxmsg.h"
#include "app_module.h"
#include "app_pop.h"
#include "app_bsp.h"
#include "app_book.h"
#include "app_key.h"
#include "app_default_params.h"
#include "app_epg.h"
#include "app_keyboard_language.h"
#include "app_channel_info.h"
#include "app_wnd_system_setting_opt.h"
#include "av/hal/gxav_hal_vout.h"
#if DEMOD_DVB_T || DEMOD_ISDBT
#include "app_t2_area_config.h"
#endif
#include "app_netaudio.h"

#include "app_msg.h"
#include "app_send_msg.h"
#include "app_fuse_api.h"
#if TKGS_SUPPORT
#include "tkgs/app_tkgs_background_upgrade.h"
#include "app_tkgs_upgrade.h"
#endif
#include "app_windows.h"
#if CASCAM_SUPPORT
#include "app_cascam.h"
#endif

#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if PROGRAM_NETUP_SUPPORT
#include "netapps/prognetup/app_prog_netup.h"
#endif

#if NETAPPS_SUPPORT
#include <gx_apps.h>
#include "app_netapps_init.h"
#endif

#if USB_TEST_SUPPORT
#include "app_usb_test.h"
#endif
#if VPN_SUPPORT
#include"bus/service/gxvpnmsg.h"
#endif
#if HDMI_CEC_SUPPORT
#include "hdmi_cec/app_cec_link.h"
#endif

#if VOICE_CONTROL_SUPPORT
#include "voice_control/include/app_voice_control_api.h"
#endif
#if PVR_PREVIEW_SUPPORT
#include "app_cover_pvr.h"
#endif

#if NETWORK_SUPPORT
#if WIFI_SUPPORT
#include "bus/service/gxusbwifiplug.h"
static char s_wifi_plug_dev_name[8] = {0} ;
#endif
#endif
#if USB_MICROPHONE_SUPPORT
#include "bus/service/gxusbaudioplug.h"
#endif

#if FACE_DETECT_SUPPORT
#include "app_face_detect.h"
#endif

typedef enum
{
	USB_PLUG_IN,
	USB_PLUG_OUT,
#if NETWORK_SUPPORT && WIFI_SUPPORT
	USB_WIFI_PLUG_IN,
	USB_WIFI_PLUG_OUT,
#endif
#if NETWORK_SUPPORT && BLUETOOTH_SUPPORT
	USB_BLUETOOTH_PLUG_IN,
	USB_BLUETOOTH_PLUG_OUT,
#endif
#if NETWORK_SUPPORT && MOD_3G_SUPPORT
    USB_3G_PLUG_IN,
    USB_3G_PLUG_OUT,
#endif
#if NETWORK_SUPPORT && USB_ETH_SUPPORT
    USB_NET_PLUG_IN,
    USB_NET_PLUG_OUT,
#endif
#if USB_MICROPHONE_SUPPORT
    USB_MICROPHONE_PLUG_IN,
    USB_MICROPHONE_PLUG_OUT,
#endif
#if NETWORK_SUPPORT && ETH_SUPPORT
    WIRED_PLUG_IN,
    WIRED_PLUG_OUT,
#endif
}UsbPlugStatus;

#define USB_INOUT_PROMPT_POP    "usb_inout_prompt"
#define USB_INOUT_PROMPT        "usb_inout_prompt"
#define USB_INOUT_IMAGE         "usb_inout_image"

static event_list *s_usb_inout_info_timer = NULL;
static uint32_t thiz_halt_scancode = 0;
#if TKGS_SUPPORT
extern bool app_tkgs_get_upgrade_finish_pop_status(void);
#endif
static bool s_avcodec_running = false;
extern int app_timer_setting_update_timer_list(void);
extern void tms_speed_change(float spd_speed);
extern void record_state_change(PlayerStatus state);
extern void tms_state_change(PlayerStatus state);
extern void app_dm_list_hotplug_proc(GxMessage* msg);
extern status_t app_pvr_media_hotplug(char *hotplug_dev);
extern status_t app_mcentre_hotplug_out(char *hotplug_dev);
extern status_t app_mcentre_hotplug_in(void);
extern bool app_multiscreen_on(void);
extern void app_pvrseek_flag_set(void);
extern void app_set_pvr_usbin_flag(bool bflag);
extern void file_list_destroy(void);
extern FullArb g_AppFullArb;
extern bool app_check_play_exec_flag(void);
extern int app_pop_list_end_dialog(void);
extern void app_channel_list_hotplug_out(uint32_t current_prog_id);
extern void app_update_channel_list_after_pvr_stop(void);
extern int app_channel_info_show_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition);
extern bool app_simple_lowpower_status_get(void);
extern void app_low_power(time_t sleep_time, uint32_t scan_code);
extern int movie_box_hide(void *usrdata);
extern int channel_set_group_fav_hd_type(GxMsgProperty_NodeByPosGet prog_node,int hd_type);
#if NETAPPS_SUPPORT
extern int app_play_bar_info_hide(void);
#endif
extern void app_stop_play_and_clean_full_screen(bool clean_bg);

#if USB_SAFELY_REMOVE_SUPPORT
extern void app_rusb_update(void);
#endif

#if NET_UPGRADE_SUPPORT
extern void app_net_upgrade_usb_update(void);
#endif

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
extern void app_network_msg_proc(GxMessage *msg);

#if WIFI_SUPPORT
extern void app_wifi_msg_proc(GxMessage *msg);
extern void app_wifi_msg_set_ap_false(void);
#endif
#if BLUETOOTH_SUPPORT
extern void app_bluetooth_msg_set_ap_false(void);
extern void app_bluetooth_sink_ui_status(GxMessage *msg);
#endif
#if MOD_3G_SUPPORT
extern void app_3g_msg_proc(GxMessage *msg);
#endif

#if SAMBA_SUPPORT
extern void app_samba_msg_proc(GxMessage *msg);
#endif

#if NETAPPS_SUPPORT

#if WEBAPI_SUPPORT
extern int app_webapi_msg_proc(GxMessage *msg);
extern void app_webapi_stop(void);
extern int app_webapi_netplay_avcodec_status(GxMessage* msg);
extern int app_webapi_netplay_msg_proc(GxMessage *msg);
#endif
#if IPTV_LITE_SUPPORT
void app_iptv_lite_create_menu(void);
#endif
#endif

#if VPN_SUPPORT
extern void app_vpn_msg_proc(GxMessage *msg);
#endif
#if APPLETS_SUPPORT
extern int app_applets_msg_proc(GxMessage *msg);
extern void app_applets_set_logout(void);
#endif
#endif // NETWORK_SUPPORT

#if NETAPPS_SUPPORT
extern int app_netvideo_play_msg_proc(GxMessage *msg);
extern int app_netapps_msg_proc(GxMessage *msg);
#endif

#if PVR_PREVIEW_SUPPORT
extern int app_cover_pvr_play_msg_proc(GxMessage *msg);
#endif

#if PLUGINSTORE_SUPPORT
extern int app_plugin_source_msg_proc(GxMessage *msg);
extern int app_plugin_listview_play_msg_proc(GxMessage *msg);
#if PLUGINONLINE_SUPPORT
extern int app_plugin_manage_msg_proc(GxMessage *msg);
#endif
#endif
#if DLNADMR_SUPPORT
extern int app_dlna_dmr_play_msg_proc(GxMessage *msg);
extern int app_dlna_dmr_ui_msg_proc(GxMessage *msg);
extern int app_dlna_dmr_hotplug_msg_proc(GxMessage *msg);
#endif
#if DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT
extern int app_dlna_dmp_ui_msg_proc(GxMessage *msg);
#endif
#if DLNADMS_SUPPORT
int app_dlna_dms_hotplug_msg_proc(GxMessage *msg);
int app_dlna_dms_ui_msg_proc(GxMessage *msg);
#endif
#ifdef SUBTTRANS_SUPPORT
int app_subttrans_ctl_msg_proc(GxMessage *msg);
#endif
#if HDMI_HDCP_SUPPORT
int app_hdcp_hdmi_plug_status(GxMessage* msg);
#endif
#if HDMI_CEC_SUPPORT
extern int app_cec_hdmiplug_process(GxMessage* msg);
#endif
#if OTA_SUPPORT
extern bool app_get_ota_upgrade_state(void);
#endif
#if SKIN_SUPPORT
extern int app_skin_msg_proc(GxMessage *msg);
#endif
#if BLUETOOTH_SUPPORT
extern  void app_bluetooth_msg_proc(GxMessage *msg);
#endif

static event_list* s_play_run_exec_timer = NULL;

static GxTime  time_start;//test 1203
static GxTime  time_end;
static float  switch_min = 50.0;
static float  switch_max = 0.0;
static float  switch_avg = 0.0;
static float  switch_sum = 0.0;
static int    switch_count = 0;

static void app_av_videoout_mode_for_plugmsg(GxMessage *msg)
{
    int init_value = 0;
    switch(msg->msg_id)
    {
        case GXMSG_HDMI_HOTPLUG_IN:
            {
                GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
                if(init_value != 0)
                {
                    GxVoutHAL_ChannelPowerOff(GXAV_VOUT_RCA);
                }
            }
            break;
        case GXMSG_HDMI_HOTPLUG_OUT:
            {
                GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
                if(init_value != 0)
                {
                    GxVoutHAL_ChannelPowerOn(GXAV_VOUT_RCA);
                }
            }
            break;
    }
}

#ifdef ECOS_OS
#if ETH_SUPPORT || USB_ETH_SUPPORT
static void _mac_addr_set(char *dev_name)
{
    char mac[] = "12:34:56:78:12:34";

    if(NULL == dev_name)
        return;

    if(GXCORE_ERROR == app_get_mac_addr(dev_name, mac))
        return;

    if(!strcmp(mac, "00:00:00:00:00:00") && 0 == generatorMac(mac))
        app_set_mac_addr(dev_name, mac);
}
#endif
#endif

void app_set_avcodec_flag(bool state)
{
    s_avcodec_running = state;
}

bool app_get_avcodec_flag(void)
{
    return s_avcodec_running;
}

bool app_check_vcodec_running_change(GxMsgProperty_PlayerAVCodecReport* avcodec)
{
    return (avcodec->vcodec.state == AVCODEC_RUNNING && ((avcodec->changed & VCODEC) != 0));
}

bool app_check_acodec_running_change(GxMsgProperty_PlayerAVCodecReport* avcodec)
{
    return (avcodec->acodec.state == AVCODEC_RUNNING && ((avcodec->changed & ACODEC) != 0));
}

static int top_avcodec_report(GxMessage* msg)
{
    GxMsgProperty_PlayerAVCodecReport *avcodec =  GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);
    int32_t ret = 0;
    GxMsgProperty_NodeByPosGet node = {0};
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    int32_t prog_id;
    PlayerStatusInfo player_status;
    memset(&player_status, 0, sizeof(player_status));

    app_getvalue(avcodec->url,"progid",&prog_id);//why progid? please see the function app_normal_play()

    if(strcmp(avcodec->player, PLAYER_FOR_NORMAL) == 0)
    {
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)) != GXCORE_SUCCESS)
        {
            app_log_error("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
            return -1;
        }

        if(app_check_vcodec_running_change(avcodec)) //tv
        {
            if(g_AppFullArb.state.tv == STATE_ON)
				app_pvrseek_flag_set();

            if(g_AppFullArb.state.tv == STATE_ON)
            {
				float  switch_interval = 0.0;
				GxCore_GetTickTime(&time_end);
		    		switch_interval = ((time_end.seconds - time_start.seconds)*1000000.0 - time_start.microsecs+ time_end.microsecs)/1000000.0;
				if(time_start.seconds > 0)
				{
				switch_sum += switch_interval;
				if( switch_interval > switch_max) switch_max = switch_interval;
				if( switch_interval < switch_min) switch_min = switch_interval;
				switch_count++;
				switch_avg = switch_sum / switch_count;
				}
				app_log_info("==========>Render Video time: %d s, %d us\n", time_end.seconds, time_end.microsecs);
				app_log_info("==========>Swtich Video interval: %.3f s\n", switch_interval);
				app_log_info("==========>Swtich Video (min, max, avg, count) = (%.3f s, %.3f s, %.3f s, %d)\n", switch_min, switch_max, switch_avg, switch_count);
                //if(strcmp(full_url, avcodec->url) == 0) //current prog
                if(prog_id == node.prog_data.id)
                {

					g_AppFullArb.scramble_set(2);
                    app_set_avcodec_flag(true);
                    GxTime time_test = {0};
                    GxCore_GetTickTime(&time_test);

                    PlayerProgInfo  *info = app_player_prog_info_get();

                    ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info); //solution check
#if ATSCCC_SUPPORT
                    if((app_channel_epg_check_dialog() != GXCORE_SUCCESS)
                            &&(GUI_CheckDialog(WND_CHANNEL_EDIT) != GXCORE_SUCCESS))
                    {
                        extern handle_t app_atsc_load(void);
                        app_atsc_load();
                    }
#endif
                    if(ret == 0)
                    {
                        GxMsgProperty_NodeByPosGet prog_node = {0};

						prog_node.node_type = NODE_PROG;
						prog_node.pos = g_AppPlayOps.normal_play.play_count;
						app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&prog_node);

                        if((info->video.height > 576) && (info->video.width > 720))
                            channel_set_group_fav_hd_type(prog_node, 1);
                        else
                        {
                            if(g_AppPlayOps.normal_play.view_info.group_mode != GROUP_MODE_HD)
                            {
                                channel_set_group_fav_hd_type(prog_node, 0);
                            }
                        }

                        if(app_channel_info_check_dialog() == GXCORE_SUCCESS)
                        {
                            app_channel_info_show_video_info(info->video, prog_node.prog_data.definition);

                            if(strcmp(GUI_GetFocusWindow(),WND_PVR_BAR)==0)
                            {
                                GUI_SetProperty(WND_PVR_BAR, "update", NULL);
                            }
                        }
                    }
                }
            }
        }

        if(app_check_acodec_running_change(avcodec)) //radio
        {
            if(g_AppFullArb.state.tv == STATE_OFF)
            {
                //if(strcmp(full_url, avcodec->url) == 0) //current prog
                if(prog_id == node.prog_data.id)
                {
                    app_set_avcodec_flag(true);
                }
            }
        }
        else
        {
            if(g_AppFullArb.state.tv == STATE_OFF)
            {
                if(prog_id == node.prog_data.id)
                {
                    if  (avcodec->acodec.state == AVCODEC_TIMEOUT && ((avcodec->changed & ACODEC) != 0))
                    {
                        app_set_avcodec_flag(false);
                    }
                }
            }
        }

        status_t get_player_status_ret = GXCORE_SUCCESS;
        get_player_status_ret=GxPlayer_MediaGetStatus(PLAYER_FOR_NORMAL, &player_status);
        if(g_AppFullArb.state.tv == STATE_ON)
        {
            if((get_player_status_ret != GXCORE_SUCCESS)
                    || (player_status.vcodec.state == AVCODEC_STOPPED)
                    || (player_status.vcodec.state == AVCODEC_READY))
            {
                app_set_avcodec_flag(false);
            }
        }
        else
        {
            if((get_player_status_ret != GXCORE_SUCCESS)
                    || (player_status.acodec.state == AVCODEC_STOPPED)
                    || (player_status.acodec.state == AVCODEC_READY))
            {
                app_set_avcodec_flag(false);
            }
        }
    }

    return EVENT_TRANSFER_KEEPON;
}


void app_play_run_exec_timer_rm(void)
{
	if(s_play_run_exec_timer != NULL)
	{
		remove_timer(s_play_run_exec_timer);
		s_play_run_exec_timer = NULL;
	}
}

static int _play_run_exec(void* usrdata)
{
	if(app_check_play_exec_flag() == true)
		return 0;

	app_play_run_exec_timer_rm();

	#ifdef MUITI_SCREEN_SUPPORE
	if(app_multiscreen_on() == FALSE)
	#endif
	{
		#if TKGS_SUPPORT
		if(g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_START)
		{
			g_AppTkgsBackUpgrade.ts_src = g_AppPlayOps.normal_play.ts_src;
			g_AppTkgsBackUpgrade.start();
		}
		else if(g_AppTkgsBackUpgrade.search_status & TKGS_BACKUPGRADE_STOP)
		{
            if(g_AppPmt.subt_id == -1)
            {
                g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
                g_AppPmt.start(&g_AppPmt);
                app_log_info("startPmt\n");
            }
		}
		#else
        if(g_AppPmt.subt_id == -1)
        {
		    g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
		    g_AppPmt.start(&g_AppPmt);
        }
		#endif
	}

//	g_AppTime.start(&g_AppTime);

	return 0;
}

static int prog_player_dumper_error(GxMsgProperty_PlayerStatusReport* status)
{
	APP_CHECK_P(status, 1);

	if((PLAYER_STATUS_ERROR == status->status || PLAYER_STATUS_PLAY_PAUSE == status->status)
            && PLAYER_ERROR_DUMPER_ERROR == status->error)
	{
		app_log_error("-----prog_player_dumper_error, stop tms/pvr!!!-----\n");
		if(0 == strcasecmp(WND_FILE_LIST, GUI_GetFocusWindow()))
		{
			file_list_destroy();
		}

		if((g_AppPvrOps.state != PVR_DUMMY) && (g_AppPvrOps.env.dev != NULL))
		{
			uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;

			app_pvr_stop();
			GUI_EndDialog(WND_PVR_BAR);
			app_channel_info_signal_end_dialog();
			GUI_EndDialog(WND_VOLUME);

			if((g_AppPlayOps.normal_play.view_info.tv_prog_cur == rec_prog_id
					&& g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
					|| (g_AppPlayOps.normal_play.view_info.radio_prog_cur == rec_prog_id
					&& g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO))
			{
				g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
				app_play_current_prog(PLAY_MODE_POINT);
				if(GUI_CheckDialog(WND_COMMON_SELECT) == GXCORE_SUCCESS)
					GUI_EndDialog(WND_COMMON_SELECT);
				g_AppPmt.start(&g_AppPmt);
			}

			GUI_SetInterface("flush", NULL);
			if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
			{
				PopDlg  pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_OK;
				pop.str = STR_ID_CANT_PVR;
				pop.mode = POP_MODE_UNBLOCK;
                popdlg_create(&pop);
			}
		}
	}

	return 0;
}

static int top_play_status_report(GxMessage* msg)
{
    int32_t prog_id;
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_PlayerStatusReport *status;
    status = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

    //receive pvr full
    if (strcmp((char*)(status->player), PLAYER_FOR_REC) == 0)
    {
        record_state_change(status->status);
    }
    else if (strcmp((char*)(status->player), PLAYER_FOR_NORMAL) == 0)
    {
        app_getvalue(status->url,"progid", &prog_id);//why progid? please see the function app_normal_play()

        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            app_log_error("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
            return -1;
        }

        if(prog_id != node.prog_data.id)
            return -1;

        // 目前时移和普通播放使用的都是"PLAYER_FOR_NORMAL"
        if(status->status == PLAYER_STATUS_PLAY_RUNNING)
        {
            if(0 != reset_timer(s_play_run_exec_timer))
            {
                s_play_run_exec_timer = create_timer(_play_run_exec, 60, NULL, TIMER_REPEAT);
            }
            /******************************************************************************/
            // TODO: for DEMUX 3 PID config, tdt/tot(0x14),epg(0x12), cat(0x01),pat(0x00),pmt()
#if CA_SUPPORT
            {
                if(app_tsd_check())
                {
                    int pidnum = 5;
                    int PidData[5] = {0x14,0x12,0x00,0x01};
                    GxMsgProperty_NodeByPosGet node_prog = {0};
                    node_prog.node_type = NODE_PROG;
                    node_prog.pos = g_AppPlayOps.normal_play.play_count;
                    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog))
                    {
#ifndef SUBTTRANS_SUPPORT
                        PidData[4] = node_prog.prog_data.pmt_pid;
#endif
                        app_exshift_pid_add(pidnum, PidData);
                    }
                    else
                    {
                        app_log_error("\nERROR, %s, %d\n",__FUNCTION__,__LINE__);
                    }
                }
            }
#endif

        }

        tms_state_change(status->status);
    }

    prog_player_dumper_error(status);

    return EVENT_TRANSFER_KEEPON;
}

static int top_play_speed_report(GxMessage* msg)
{
    GxMsgProperty_PlayerSpeedReport *spd;
    spd = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerSpeedReport);
    if(spd != NULL)
    {
        if (strcmp((char*)(spd->player), PLAYER_FOR_NORMAL) == 0)
        {
            tms_speed_change(spd->speed);
        }

		if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
		{
			if(GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS)
			{
				GxMsgProperty_PlayerSpeedReport *player_spd = NULL;
				player_spd = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerSpeedReport);
				extern void player_speed_change(GxMsgProperty_PlayerSpeedReport *spd);
				player_speed_change(player_spd);
				GUI_SetFocusWidget(WND_POP_BOOK);
			}
		}
    }

    return EVENT_TRANSFER_KEEPON;
}

static int top_hotplug_in(GxMessage* msg)
{
	 if(GUI_CheckDialog(WIN_FILE_BROWSER) == GXCORE_SUCCESS) //media centre
	 {
	 	app_mcentre_hotplug_in();
	 }
	 else if(GUI_CheckDialog(WND_DISK_MANAGE) == GXCORE_SUCCESS)
	 {
	 	app_dm_list_hotplug_proc(msg);
	 }

	 g_AppPvrOps.tms_delete(&g_AppPvrOps);

     app_update_pop_dlg(WND_POP_BOOK);
     return EVENT_TRANSFER_STOP;
}

static int top_hotplug_out(GxMessage* msg)
{
    GxMsgProperty_HotPlug *hot_plug = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_HotPlug);
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_FILE_LIST))
	{
        GUI_EndDialog("after wnd_file_list");
		file_list_destroy();
	}
    if(GUI_CheckDialog(WIN_FILE_BROWSER) == GXCORE_SUCCESS) //media centre
    {
        app_mcentre_hotplug_out(hot_plug->dev_name);
    }
    if(GUI_CheckDialog(WND_PVR_MEIDA) == GXCORE_SUCCESS) //pvr list
    {
        app_pvr_media_hotplug(hot_plug->dev_name);
    }
    else if(GUI_CheckDialog(WND_DISK_MANAGE) == GXCORE_SUCCESS)// dm setting
    {
        app_dm_list_hotplug_proc(msg);
    }

#if !OTT_SUPPORT
    uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;
    uint32_t current_tv_id = g_AppPlayOps.normal_play.view_info.tv_prog_cur;
    uint32_t current_radio_id = g_AppPlayOps.normal_play.view_info.radio_prog_cur;
    GxBusPmDataProgType stream_type = g_AppPlayOps.normal_play.view_info.stream_type;

    app_fuse_spec_proc_top_hotplug_out(hot_plug->dev_name);

    if(GUI_CheckDialog(WND_SYSTEM_SETTING) == GXCORE_SUCCESS)
    {
        extern char *app_upgrade_get_path(void);
        extern void app_upgrade_clear_path(void);
        if(app_system_get_title() != NULL && 0 == strcmp(app_system_get_title(), STR_ID_FIRMWARE_UPGRADE))
        {
            if(strstr(app_upgrade_get_path(), hot_plug->dev_name + 4) != NULL)
            {
                app_upgrade_clear_path();
            }
        }
    }

    if ((g_AppPvrOps.state != PVR_DUMMY)
            && (hot_plug->dev_name != NULL)
            && (g_AppPvrOps.env.dev != NULL)
            && (strncmp(hot_plug->dev_name, g_AppPvrOps.env.dev,strlen(hot_plug->dev_name)) == 0))
    {

        app_pvr_stop();
        GUI_EndDialog(WND_PVR_BAR);
        app_channel_info_signal_end_dialog();
        GUI_EndDialog(WND_VOLUME);
        GUI_EndDialog(WND_POP_QR);
        if (app_channel_info_check_dialog() != GXCORE_SUCCESS)
        {
            app_channel_info_create_dialog();
        }



        if((current_tv_id == rec_prog_id && stream_type == GXBUS_PM_PROG_TV)
            || (stream_type == GXBUS_PM_PROG_RADIO && current_radio_id == rec_prog_id))
        {
            g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
            app_play_current_prog(PLAY_MODE_POINT);
            if(GUI_CheckDialog(WND_COMMON_SELECT) == GXCORE_SUCCESS)
                GUI_EndDialog(WND_COMMON_SELECT);
            g_AppPmt.start(&g_AppPmt);
        }

        if(app_channel_list_check_dialog() == GXCORE_SUCCESS)
        {
            uint32_t current_prog_id = (stream_type == GXBUS_PM_PROG_TV) ? current_tv_id : current_radio_id;
            app_update_channel_list_after_pvr_stop();
            app_channel_list_hotplug_out(current_prog_id);
        }

        GUI_SetInterface("flush", NULL);
        if(GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_USB_OUT;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
            app_channel_info_update_under_pop_dlg();
        }
        else
        {
            GUI_SetProperty(WND_POP_BOOK, "update", NULL);
        }

    }
    else if(g_AppPvrOps.state == PVR_DUMMY)
    {
        if (GXCORE_SUCCESS == GUI_CheckDialog(WND_PVR_BAR))
        {
            GUI_EndDialog(WND_PVR_BAR);
            if (app_channel_info_check_dialog() != GXCORE_SUCCESS)
            {
                app_channel_info_create_dialog();
            }
        }
    }
#endif
    return EVENT_TRANSFER_STOP;
}

static int top_hotplug_clean(GxMessage* msg)
{
    GxMsgProperty_HotPlug *hot_plug = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_HotPlug);
    if(GUI_CheckDialog(WIN_FILE_BROWSER) == GXCORE_SUCCESS) //media centre
    {
        extern status_t app_mcentre_hotplug_clean(char *hotplug_dev);
        app_mcentre_hotplug_clean(hot_plug->dev_name);

    }
    if(GUI_CheckDialog(WND_DISK_MANAGE) == GXCORE_SUCCESS)// dm setting
    {
        app_dm_list_hotplug_proc(msg);
    }

    app_update_pop_dlg(WND_POP_BOOK);

    return EVENT_TRANSFER_STOP;
}
#if CVBS_SCALE_SUPPORT
static int s_message_cvbs_flag = 0;

int app_top_get_cvbs_mode_status(void)
{
	return s_message_cvbs_flag;
}

static int top_hdmiplug_status(GxMessage* msg)
{
    static int init_value = 1;
	//GxMsgProperty_PlayerVideoWindow play_wnd;
	PlayerWindow video_wnd = {0};

	GxAvRect viewport_rect = {0};
    int vpu_handle = 0;
	int av_device_handle = 0;
	//GxBus_ConfigGetInt(OUTPUT_KEY, &init_value, OUTPUT_DEFAULT);
	av_device_handle= GxAvdev_CreateDevice(0) ;
	vpu_handle = GxAvdev_OpenModule(av_device_handle, GXAV_MOD_VPU, 0);
	if((init_value)
		&&(GUI_CheckDialog(WND_EPG) != GXCORE_SUCCESS)
		&&(GUI_CheckDialog(WND_CHANNEL_EDIT) != GXCORE_SUCCESS))
	{
		if(msg->msg_id == GXMSG_HDMI_HOTPLUG_IN)
		{
			video_wnd.x = 0;
			video_wnd.y = 0;
			video_wnd.width = 1280*APP_THEME_XRES/1280;
			video_wnd.height = 720*APP_THEME_YRES/720;
			viewport_rect.x = video_wnd.x;
	        viewport_rect.y = 0;
	        viewport_rect.width = video_wnd.width;
	        viewport_rect.height = video_wnd.height;
			s_message_cvbs_flag = 0;
		}
		else//CVBS
		{
			video_wnd.x = 0;
			video_wnd.y = 16;
			video_wnd.width = 1248*APP_THEME_XRES/1280;
			video_wnd.height = 702*APP_THEME_YRES/720;
			viewport_rect.x = video_wnd.x;
	        viewport_rect.y = video_wnd.y;
	        viewport_rect.width = video_wnd.width;
	        viewport_rect.height = video_wnd.height-6;
			s_message_cvbs_flag = 1;
		}
		GxAvdev_SetLayerViewport(av_device_handle, vpu_handle, GX_LAYER_SPP,&viewport_rect);
		GxAvdev_SetLayerViewport(av_device_handle, vpu_handle, GX_LAYER_OSD, &viewport_rect);

		GxAvdev_CloseModule(av_device_handle, vpu_handle);
		GxAvdev_DestroyDevice(av_device_handle);

		//play_wnd.player = PLAYER_FOR_IPTV;
		//memcpy(&(play_wnd.rect), &video_wnd, sizeof(PlayerWindow));
		//app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW, (void *)(&play_wnd));
		//init_value = 0;
	}
	return EVENT_TRANSFER_STOP;
}
#endif

static int top_sync_time(GxMessage* msg)
{
    GxMsgProperty_ExtraTimeOk *timeData = NULL;

    timeData = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_ExtraTimeOk);
    const char *wnd = GUI_GetFocusWindow();

    // TODO TOT
    if(NULL != timeData
            && TDT_TIME == timeData->time_fountain)
    {
        g_AppTime.sync(&g_AppTime, timeData->utc_second);
        g_AppBook.invalid_remove();
        if((wnd != NULL) && (strcmp(wnd, WND_TIMER_SETTING) == 0))
        {
            app_timer_setting_update_timer_list();
        }
    }
    else if(NULL != timeData && TOT_TIME == timeData->time_fountain)
    {
#if (DEMOD_DVB_T || DEMOD_ISDBT)
        int i;
        char country_code[4] = {0};
        int 	country = 0;
        GxBus_ConfigGetInt(T2_COUNTRY, &country, T2_COUNTRY_VALUE);
        app_iso3166_country_code_get(g_CountryName[country],country_code,NULL);
        for(i = 0 ; i < timeData->offset_time_count; i++)
        {
            if(0 == memcmp((char *)timeData->offset_time[i].lang_code, country_code,strlen(country_code)))
            {
                g_AppTime.config.zone = timeData->offset_time[i].zone;
                GxBus_ConfigSetInt(TIME_ZONE, g_AppTime.config.zone);
                g_AppTime.config.summer = timeData->offset_time[i].summer;
                GxBus_ConfigSetInt(TIME_SUMMER, g_AppTime.config.summer);
                break;
            }
        }
#endif
        g_AppTime.sync(&g_AppTime, timeData->utc_second);
        g_AppBook.invalid_remove();
        if((wnd != NULL) && (strcmp(wnd, WND_TIMER_SETTING) == 0))
        {
            app_timer_setting_update_timer_list();
        }
    }

    return EVENT_TRANSFER_STOP;
}

static int top_frontend_check(GxMessage* msg)
{
    int led_lock_state = 0;
    AppFrontend_LockState lock;

    if(app_simple_lowpower_status_get() == true)
        return 0;

    if (msg->msg_id == GXMSG_FRONTEND_UNLOCKED)
    {
        GxMsgProperty_FrontendUnlocked *tuner = NULL;

        tuner = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_FrontendUnlocked);

        lock = FRONTEND_UNLOCK;
        app_ioctl(*tuner, FRONTEND_LOCK_STATE_SET, &lock);

        led_lock_state = 0;
        app_panel_ioctl_handle(PANEL_SHOW_LOCK, &led_lock_state);
    }
    else
    {
        GxMsgProperty_FrontendLocked *tuner = NULL;

        tuner = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_FrontendLocked);

        lock = FRONTEND_LOCKED;
        app_ioctl(*tuner, FRONTEND_LOCK_STATE_SET, &lock);

        led_lock_state = 1;
        app_panel_ioctl_handle(PANEL_SHOW_LOCK, &led_lock_state);
    }

    return EVENT_TRANSFER_STOP;
}

#if NETWORK_SUPPORT
static void _net_dev_out_for_play(const char* dev_name)
{
	ubyte64 cur_dev = {0};
	app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
	if((dev_name == NULL) || (strlen(cur_dev) == 0) || (strcasecmp(dev_name, cur_dev) == 0))
	{
		#if NETAPPS_SUPPORT
		extern int app_net_video_recovery_status(void);
		app_net_video_recovery_status();
		gxapps_service_stop();
		app_netapps_top_msg_dev_out_proc();
		#endif
		#if WEBAPI_SUPPORT
		app_webapi_stop();
		#endif
        #if APPLETS_SUPPORT
		app_applets_set_logout();
        #endif
    }
    //157276操作导致GUI栈空间存的窗口顺序被改变
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK) && strcasecmp(WND_POP_BOOK,GUI_GetFocusWindow()))

    {
        GUI_SetFocusWidget("btn_pop_book_ok");
    }
}
#endif

static int _usb_inout_info_timer_cb(void *usrdata)
{
    if(s_usb_inout_info_timer)
    {
        s_usb_inout_info_timer = NULL;
    }
    if (GUI_CheckPrompt(USB_INOUT_PROMPT_POP) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(USB_INOUT_PROMPT_POP);
    }
    music_view_spectrum_refresh();
    if(0 == strcmp(GUI_GetFocusWindow(), WND_SYSTEM_SETTING))
    {
        GUI_SetProperty(WND_SYSTEM_SETTING, "update", NULL);
    }

    return 0;
}

static void top_usb_inout_plug_info(UsbPlugStatus state)
{
    const char *pop_image = NULL;
#if NETWORK_SUPPORT
    const char *net_out_name = NULL;
#endif


    switch (state)
    {
        case USB_PLUG_IN:
            pop_image = "s_icon_usb_in";
            break;
        case USB_PLUG_OUT:
            pop_image = "s_icon_usb_out";
            break;
#if NETWORK_SUPPORT
#if WIFI_SUPPORT
        case USB_WIFI_PLUG_IN:
            pop_image = "s_icon_wifi_in";
            break;
        case USB_WIFI_PLUG_OUT:
            pop_image = "s_icon_wifi_out";
            net_out_name = s_wifi_plug_dev_name;
            break;
#endif
#if BLUETOOTH_SUPPORT
        case USB_BLUETOOTH_PLUG_IN:
            pop_image = "s_icon_bt_in";
            break;
        case USB_BLUETOOTH_PLUG_OUT:
            pop_image = "s_icon_bt_out";
            net_out_name = "bluetooth0";
            break;
#endif
#if MOD_3G_SUPPORT
        case USB_3G_PLUG_IN:
            pop_image = "s_icon_3g_in";
            break;
        case USB_3G_PLUG_OUT:
            pop_image = "s_icon_3g_out";
            net_out_name = "USB3G";
            break;
#endif
#if USB_ETH_SUPPORT
        case USB_NET_PLUG_IN:
            pop_image = "s_icon_usbnet_in";
            break;
        case USB_NET_PLUG_OUT:
            pop_image = "s_icon_usbnet_out";
            net_out_name = "eth1";
            break;
#endif
#if ETH_SUPPORT
        case WIRED_PLUG_IN:
            pop_image = "s_icon_wired_in";
            break;
        case WIRED_PLUG_OUT:
            pop_image = "s_icon_wired_out";
            net_out_name = "eth0";
            break;
#endif
#endif
#if USB_MICROPHONE_SUPPORT
        case USB_MICROPHONE_PLUG_IN:
            pop_image = "s_icon_micphone_in";
            break;
        case USB_MICROPHONE_PLUG_OUT:
            pop_image = "s_icon_micphone_out";
            break;
#endif
        default:
            break;
    }
    if (!pop_image)
        return;

    if(g_AppFullArb.state.ttx == STATE_ON)
    {
        g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
        GUI_SetProperty(WND_FULL_SCREEN, "update", NULL);
        GUI_SetProperty("img_full_mute", "img", "s_mute");
        GUI_SetProperty("img_full_pause", "img", "s_pause");
    }

    APP_TIMER_REMOVE(s_usb_inout_info_timer);
    if (GUI_CheckPrompt(USB_INOUT_PROMPT_POP) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(USB_INOUT_PROMPT_POP);
    }
    music_view_spectrum_refresh();
    GUI_CreatePrompt(APP_WND_TOP_USB_PRO_POS_X, APP_WND_TOP_USB_PRO_POS_Y, USB_INOUT_PROMPT_POP, USB_INOUT_PROMPT, "", "ontop");
    GUI_SetProperty(USB_INOUT_IMAGE, "img", (void*)pop_image);
    APP_TIMER_ADD(s_usb_inout_info_timer, _usb_inout_info_timer_cb, 2000, TIMER_ONCE);
#if NETWORK_SUPPORT
    if (net_out_name)
    {
        _net_dev_out_for_play(net_out_name);
    }
#endif
    GUI_SetInterface("flush", NULL);

#if USB_SAFELY_REMOVE_SUPPORT
    if (state == USB_PLUG_IN || state == USB_PLUG_OUT)
    {
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SYSTEM_SETTING))
        {
            char *menuTitle = app_system_get_title();
            if(NULL != menuTitle && 0 == strcmp(STR_ID_USB_REMOVE, menuTitle))
            {
                app_rusb_update();
            }
        }
    }
#endif
#if NET_UPGRADE_SUPPORT
    if (state == USB_PLUG_OUT)
    {
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SYSTEM_SETTING))
        {
            char *menuTitle = app_system_get_title();
            if(NULL != menuTitle && 0 == strcmp(STR_ID_NET_UPGRADE, menuTitle))
            {
                app_net_upgrade_usb_update();
            }
        }
    }
#endif
    if(0 == strcmp(GUI_GetFocusWindow(),WND_SYSTEM_SETTING))
    {
        GUI_SetProperty(WND_SYSTEM_SETTING, "update", NULL);
    }
}

void top_usb_plug_info(int state)
{
    if (state == 0)
        top_usb_inout_plug_info(USB_PLUG_IN);
    else
        top_usb_inout_plug_info(USB_PLUG_OUT);
}

bool app_trigger_check_popdlg(void)
{
	if(0 == strcasecmp(WND_FILE_LIST, GUI_GetFocusWindow()) )
		return true;
	if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
		return true;
	if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
		return true;
	if(0 == strcasecmp(WND_SAT_SEARCH_OPT, GUI_GetFocusWindow()))
		return true;
	if(0 == strcasecmp(WND_TP_SEARCH_OPT, GUI_GetFocusWindow()))
		return true;
	if(0 == strcasecmp(WND_TIMER_CHANNEL_LIST, GUI_GetFocusWindow()))
		return true;
#if SAT_FINDER_SUPPORT
	if(0 == strcasecmp(WND_SATFINDER, GUI_GetFocusWindow()))
		return true;
#endif
#if UNICABLE_SUPPORT
	if(GUI_CheckDialog(WND_UNICABLE_SET) == GXCORE_SUCCESS)
		return true;
#endif
	return false;
}

void app_trigger_end_popdlg(void)
{
	static GUI_Event event;
	event.type = GUI_KEYDOWN;
	event.key.sym = STBK_EXIT;
    movie_box_hide(NULL); //for avoiding bug 162816
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_FILE_LIST))
	{
        GUI_EndDialog("after wnd_file_list");
		file_list_destroy();
	}
	app_pop_list_end_dialog();
#if NETAPPS_SUPPORT
    app_play_bar_info_hide();
#endif
	popdlg_destroy();
	if(0 == strcasecmp(WND_SAT_SEARCH_OPT, GUI_GetFocusWindow()))
	{
		GUI_SendEvent("box_sat_search_opt", &event);
	}
	if(0 == strcasecmp(WND_TP_SEARCH_OPT, GUI_GetFocusWindow()))
	{
		GUI_SendEvent("box_tp_search_opt", &event);
	}
	if(0 == strcasecmp(WND_TIMER_CHANNEL_LIST, GUI_GetFocusWindow()))
	{
        	GUI_EndDialog(WND_TIMER_CHANNEL_LIST);
	}
#if SAT_FINDER_SUPPORT
	if(0 == strcasecmp(WND_SATFINDER, GUI_GetFocusWindow()))
	{
		GUI_EndDialog(WND_SATFINDER);
	}
#endif
#if UNICABLE_SUPPORT
	if(GUI_CheckDialog(WND_UNICABLE_SET) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_UNICABLE_SET);
	}
#endif
}

extern  void app_pvr_stop(void);
static int app_top_service(GUI_Event* event)
{
    int ret = EVENT_TRANSFER_KEEPON;
    APP_CHECK_P(event, EVENT_TRANSFER_STOP);
    GxMessage* msg;

    msg = (GxMessage*)event->msg.service_msg;
    if (msg == NULL)    return ret;

    if(((GUI_CheckDialog(WND_UPGRADE_PROCESS) == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WND_NETUPG_PROCESS) == GXCORE_SUCCESS)
#if OTA_SUPPORT
            || (app_get_ota_upgrade_state() == true)
#endif
       )
#if VOICE_CONTROL_SUPPORT
       && (msg->msg_id != APPMSG_MESSAGE)
#endif

      )
    {
        return ret;
    }
#if LNB_SHORT_DETECT_SUPPORT
    const char *focus_win = GUI_GetFocusWindow();
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH)
            && focus_win && strcmp(focus_win, WND_SEARCH))
    {
        extern int app_search_service_ext(GxMessage *pMsg);
        app_search_service_ext(msg);
    }
#endif
#if MIRACAST_SUPPORT
    const char *focus_win = GUI_GetFocusWindow();
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_MIRACAST)
            && focus_win && strcmp(focus_win, WND_MIRACAST))
    {
        extern int app_miracast_msg_proc(GxMessage *msg);
        app_miracast_msg_proc(msg);
    }
#endif
    switch(msg->msg_id)
    {

        #if TKGS_SUPPORT
        case GXMSG_TKGS_UPDATE_STATUS:
	 {
	   extern int app_tkgs_upgrade_process_service(GxMessage* pMsg);
            app_tkgs_upgrade_process_service(msg);
        }
            break;
        #endif
        case GXMSG_HOTPLUG_IN:
            top_usb_inout_plug_info(USB_PLUG_IN);
            ret = top_hotplug_in(msg);
            app_set_pvr_usbin_flag(true);
#if DLNADMS_SUPPORT
            if(GUI_CheckDialog(WND_DLNA_DMS) == GXCORE_SUCCESS)
                ret = app_dlna_dms_hotplug_msg_proc(msg);
#endif

#if APPLETS_SUPPORT
            if(GUI_CheckDialog(WND_APPLETS) == GXCORE_SUCCESS)
                ret = app_applets_msg_proc(msg);
#endif
#if SKIN_SUPPORT
            ret = app_skin_msg_proc(msg);
#endif
            app_fuse_spec_proc_top_hotplug_in(NULL);
            break;
        case GXMSG_HOTPLUG_OUT:
            top_usb_inout_plug_info(USB_PLUG_OUT);
            ret = top_hotplug_out(msg);
#if DLNADMS_SUPPORT
            if(GUI_CheckDialog(WND_DLNA_DMS) == GXCORE_SUCCESS)
                ret = app_dlna_dms_hotplug_msg_proc(msg);
#endif
#if APPLETS_SUPPORT
            if(GUI_CheckDialog(WND_APPLETS) == GXCORE_SUCCESS)
                ret = app_applets_msg_proc(msg);
#endif
#if SKIN_SUPPORT
            ret = app_skin_msg_proc(msg);
#endif
            break;
#ifdef ECOS_OS
#if USB_TO_SERIAL_SUPPORT
        case GXMSG_USBSERIAL_HOTPLUG_IN:
        {
            extern int uart_init(unsigned int Baudrate);
            uart_init(115200);
            break;
        }
        case GXMSG_USBSERIAL_HOTPLUG_OUT:
        {
            extern int uart_close(void);
            uart_close();
            break;
        }
#endif
#if NETWORK_SUPPORT && WIFI_SUPPORT
        case GXMSG_USBWIFI_HOTPLUG_IN:
            top_usb_inout_plug_info(USB_WIFI_PLUG_IN);
            app_log_info("[NET_DEV] USB WIFI Hotplug in!!\n");
            break;
        case GXMSG_USBWIFI_HOTPLUG_OUT:
        {
            GxMsgProperty_UsbwifiHotPlug *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
            int len ;
            if ( plug != NULL )
            {
                len = strlen(plug->dev_name);
                if (len >= 8 )
                   len = 7 ;
                memset(s_wifi_plug_dev_name,0,8);
                memcpy(s_wifi_plug_dev_name,plug->dev_name,len);
            }

            app_wifi_msg_set_ap_false();
            top_usb_inout_plug_info(USB_WIFI_PLUG_OUT);
            app_log_info("[NET_DEV] USB WIFI Hotplug out!!\n");
#if SKIN_SUPPORT
            app_skin_msg_proc(msg);
#endif
        }
            break;
#endif

#if NETWORK_SUPPORT && BLUETOOTH_SUPPORT
        case GXMSG_USBBT_HOTPLUG_IN:
            top_usb_inout_plug_info(USB_BLUETOOTH_PLUG_IN);
            app_log_info("[NET_DEV] USB bluetooth Hotplug in!!\n");
            break;
        case GXMSG_USBBT_HOTPLUG_OUT:
            top_usb_inout_plug_info(USB_BLUETOOTH_PLUG_OUT);
            app_log_info("[NET_DEV] USB bluetooth Hotplug out!!\n");
            break;
#endif

#if MOD_3G_SUPPORT
        case GXMSG_USB3G_HOTPLUG_IN:
            top_usb_inout_plug_info(USB_3G_PLUG_IN);
            app_log_info("[NET_DEV] USB 3G Hotplug in!!\n");
            break;
        case GXMSG_USB3G_HOTPLUG_OUT:
            top_usb_inout_plug_info(USB_3G_PLUG_OUT);
            app_log_info("[NET_DEV] USB 3G Hotplug out!!\n");
            break;
#endif

#if NETWORK_SUPPORT && USB_ETH_SUPPORT
        case GXMSG_USBNET_HOTPLUG_IN:
            _mac_addr_set("eth1");
            top_usb_inout_plug_info(USB_NET_PLUG_IN);
            app_log_info("[NET_DEV] USB2ETH Hotplug in!!\n");
            break;
        case GXMSG_USBNET_HOTPLUG_OUT:
            top_usb_inout_plug_info(USB_NET_PLUG_OUT);
            app_log_info("[NET_DEV] USB2ETH Hotplug out!!\n");
#if SKIN_SUPPORT
            app_skin_msg_proc(msg);
#endif
            break;
#endif
#if ETH_SUPPORT
        case GXMSG_ETH_HOTPLUG_IN:
            _mac_addr_set("eth0");
            top_usb_inout_plug_info(WIRED_PLUG_IN);
            app_log_info("[NET_DEV] Eth Hotplug in!!\n");
            break;
        case GXMSG_ETH_HOTPLUG_OUT:
            top_usb_inout_plug_info(WIRED_PLUG_OUT);
            app_log_info("[NET_DEV] Eth Hotplug out!!\n");
            break;
#endif
#endif
#if USB_MICROPHONE_SUPPORT
        case GXMSG_USBAUDIO_HOTPLUG_IN:
            {
                extern int app_mic_uac_device_in(char *name, int id);
                GxMsgProperty_UsbaudioHotPlug *m = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbaudioHotPlug);
                if(0 == app_mic_uac_device_in(m->dev_name, m->id))
                    top_usb_inout_plug_info(USB_PLUG_IN);
                top_usb_inout_plug_info(USB_MICROPHONE_PLUG_IN);

            }
            break;
        case GXMSG_USBAUDIO_HOTPLUG_OUT:
            {
                extern  int app_mic_uac_device_out(char *name, int id);
                GxMsgProperty_UsbaudioHotPlug *m = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbaudioHotPlug);
                if(0 == app_mic_uac_device_out(m->dev_name, m->id))
                    top_usb_inout_plug_info(USB_PLUG_OUT);
                top_usb_inout_plug_info(USB_MICROPHONE_PLUG_OUT);
            }
            break;
#endif
		case GXMSG_HDMI_HOTPLUG_IN:
		case GXMSG_HDMI_HOTPLUG_OUT:
            app_av_videoout_mode_for_plugmsg(msg);
#if CVBS_SCALE_SUPPORT
            top_hdmiplug_status(msg);
#endif
#if HDMI_CEC_SUPPORT
	        app_cec_hdmiplug_process(msg);
#endif
#if HDMI_HDCP_SUPPORT
            app_hdcp_hdmi_plug_status(msg);
#endif
            break;
#if HDMI_HDCP_SUPPORT
		case GXMSG_HDMI_HDCP_FAIL:
		case GXMSG_HDMI_HDCP_SUCCESS:
            app_hdcp_hdmi_plug_status(msg);
			break;
#endif
        case GXMSG_HOTPLUG_CLEAN:
            ret = top_hotplug_clean(msg);
            break;
        case GXMSG_PLAYER_AVCODEC_REPORT:
#if NETAPPS_SUPPORT
			app_netapps_top_msg_proc(msg);

            if(GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_SUCCESS)
                ret = app_netvideo_play_msg_proc(msg);
            else
#endif


#if WEBAPI_SUPPORT
            if(GUI_CheckDialog(WND_WEBAPI_NETPLAY) == GXCORE_SUCCESS)
                ret = app_webapi_netplay_avcodec_status(msg);
            else
#endif
                ret = top_avcodec_report(msg);
            break;
        case GXMSG_PLAYER_STATUS_REPORT:
#if PLUGINSTORE_SUPPORT
            if(GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY) == GXCORE_SUCCESS)
                ret = app_plugin_listview_play_msg_proc(msg);
#endif

#if NETAPPS_SUPPORT
            if(GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_SUCCESS)
                ret = app_netvideo_play_msg_proc(msg);
            else
#endif
#if PVR_PREVIEW_SUPPORT
        if(GUI_CheckDialog(WND_COVER_PVR) == GXCORE_SUCCESS)
            ret = app_cover_pvr_play_msg_proc(msg);
#endif

#if WEBAPI_SUPPORT
            if(GUI_CheckDialog(WND_WEBAPI_NETPLAY) == GXCORE_SUCCESS)
                ret = app_webapi_netplay_msg_proc(msg);
            else
#endif
#if DLNADMR_SUPPORT
            if(GUI_CheckDialog(WND_DLNA_DMR) == GXCORE_SUCCESS)
                ret = app_dlna_dmr_play_msg_proc(msg);
#endif
#if NETAPPS_SUPPORT
            app_netapps_top_msg_proc(msg);
#if NET_AUDIO_SUPPORT
            app_netaudio_auto_msg_proc(msg);
#endif
#endif
#if PLAYBACK_SPEED_SUPPORT
            if((GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS) && (GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS))
            {
                GxMsgProperty_PlayerStatusReport* player_status = NULL;
                player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
                if(PLAYER_STATUS_PLAY_END == player_status->status || PLAYER_STATUS_ERROR == player_status->status)
                    GUI_EndDialog(WND_POP_OPTION_LIST);
                GUI_SendEvent(WIN_MOVIE_VIEW, event);
                break;

            }
#endif
            ret = top_play_status_report(msg);
            break;
        case GXMSG_PLAYER_SPEED_REPORT:
            ret = top_play_speed_report(msg);
            break;
#if  HIGH_DYNAMIC_RANGE_SUPPORT
        case GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED:
            {
                extern int top_play_video_hdr_event_report(GxMessage* msg);
                ret = top_play_video_hdr_event_report(msg);
            }
            break;
#endif
        case GXMSG_EXTRA_SYNC_TIME_OK:
            ret = top_sync_time(msg);
            break;
        case GXMSG_FRONTEND_UNLOCKED:
        case GXMSG_FRONTEND_LOCKED:
            ret = top_frontend_check(msg);
            break;

        case GXMSG_BOOK_TRIGGER:
            {
#if TKGS_SUPPORT
                if(((GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)&&(app_tkgs_get_upgrade_finish_pop_status() == true))
                        || ((GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS) == GXCORE_SUCCESS)&&(app_simple_lowpower_status_get() == false)))
                {
                    break;
                }
#endif
				app_trigger_end_popdlg();
				GxMsgProperty_BookTrigger *book = NULL;
                book = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_BookTrigger);
                if ((NULL != book) && (app_fuse_spec_proc_top_book_trigger()>0))
                {
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
                if((NULL != book) && (app_simple_lowpower_status_get() == false))
                {
					g_AppBook.exec(book);
				}
                ret = EVENT_TRANSFER_STOP;
                break;
            }

#if NETWORK_SUPPORT
        case GXMSG_NETWORK_STATE_REPORT:
            {
                const char *wnd = GUI_GetFocusWindow();
#if 0//(CAM_SUPPORT > 0)
            GxMsgProperty_NetDevState *net_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            if(net_state->dev_state == IF_STATE_CONNECTED)
            {
                top_reset_cam();
            }
#endif
            if((wnd != NULL) && (strcmp(wnd, WND_NETWORK) == 0))
            {
                app_network_msg_proc(msg);
            }

#if WIFI_SUPPORT
                if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif
#if BLUETOOTH_SUPPORT
                if(GUI_CheckDialog(WND_BLUETOOTH) == GXCORE_SUCCESS)
                {
                    app_bluetooth_msg_proc(msg);
                }
#endif
#if MOD_3G_SUPPORT
                if(GUI_CheckDialog(WND_3G) == GXCORE_SUCCESS)
                {
                    app_3g_msg_proc(msg);
                }
#endif
#if DLNADMS_SUPPORT
            if(GUI_CheckDialog(WND_DLNA_DMS) == GXCORE_SUCCESS)
                ret = app_dlna_dms_hotplug_msg_proc(msg);
#endif
#if DLNADMR_SUPPORT
            if(GUI_CheckDialog(WND_DLNA_DMR) == GXCORE_SUCCESS)
                ret = app_dlna_dmr_hotplug_msg_proc(msg);
#endif
#if DVB2IP_SERVER_USE_STATIC_SCREEN
            if(GUI_CheckDialog(WND_DVB2IP_PLAY) == GXCORE_SUCCESS)
                ret = app_dvb2ip_play_hotplug_msg_proc(msg);
#endif
#if NETAPPS_SUPPORT
                app_netapps_msg_proc(msg);
#endif
#if NETAPPS_SUPPORT
#if WEBAPI_SUPPORT
				app_webapi_msg_proc(msg);
#endif
				app_netapps_top_msg_proc(msg);
#endif

#if SAMBA_SUPPORT
                app_samba_msg_proc(msg);
#endif
#if APPLETS_SUPPORT
                app_applets_msg_proc(msg);
#endif

#if SKIN_SUPPORT
                app_skin_msg_proc(msg);
#endif
               break;
        }

        case GXMSG_WIFI_SCAN_AP_OVER:
        case GXMSG_WIFI_SCAN_HIDDEN_AP_OVER:
            {
#if WIFI_SUPPORT
                app_wifi_msg_set_ap_false();
                if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif
                break;
            }
#if BLUETOOTH_SUPPORT
        case GXMSG_BLUETOOTH_SCAN_AP_OVER:
            {
                app_bluetooth_msg_set_ap_false();
                if(GUI_CheckDialog(WND_BLUETOOTH) == GXCORE_SUCCESS)
                {
                    app_bluetooth_msg_proc(msg);
                }
                break;
            }
        case GXMSG_USBBT_SINK_PLAYING_TITLE_INFO:
        case GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO:
        case GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO:
        case GXMSG_USBBT_SINK_TRACK_CHANGED:
        case APPMSG_BLUETOOTH_UI_CONTROL:
            {
                if(GUI_CheckDialog(WND_SINK_VIEW) == GXCORE_SUCCESS)
                {
                    app_bluetooth_sink_ui_status(msg);
                }
                break;
            }
#endif

        case GXMSG_NETWORK_PLUG_IN:
            {
                const char *wnd = GUI_GetFocusWindow();
                if((wnd != NULL) && (strcmp(wnd, WND_NETWORK) == 0))
                {
                    app_network_msg_proc(msg);
                }
#if WIFI_SUPPORT
                if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif
#if BLUETOOTH_SUPPORT
                if(GUI_CheckDialog(WND_BLUETOOTH) == GXCORE_SUCCESS)
                {
                    app_bluetooth_msg_proc(msg);
                }
#endif

#if MOD_3G_SUPPORT
                if(GUI_CheckDialog(WND_3G) == GXCORE_SUCCESS)
                {
                    app_3g_msg_proc(msg);
                }
#endif
#ifdef LINUX_OS
				{
					GxMsgProperty_NetDevType type;
					ubyte64 *dev_name = GxBus_GetMsgPropertyPtr(msg, ubyte64);
					if(dev_name != NULL)
					{
						memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
						strcpy(type.dev_name, *dev_name);
						app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
#if ETH_SUPPORT
						if(type.dev_type == IF_TYPE_WIRED)
						{
							top_usb_inout_plug_info(WIRED_PLUG_IN);
						}
#endif
#if USB_ETH_SUPPORT
						if(type.dev_type == IF_TYPE_USB_WIRED)
						{
							top_usb_inout_plug_info(USB_NET_PLUG_IN);
						}
#endif
#if WIFI_SUPPORT
						if(type.dev_type == IF_TYPE_WIFI)
						{
                            top_usb_inout_plug_info(USB_WIFI_PLUG_IN);
						}
#endif
#if BLUETOOTH_SUPPORT
						if(type.dev_type == IF_TYPE_BLUETOOTH)
						{
                            top_usb_inout_plug_info(USB_BLUETOOTH_PLUG_IN);
						}
#endif

#if (MOD_3G_SUPPORT > 0)
						 if(type.dev_type == IF_TYPE_3G || type.dev_type == IF_TYPE_ETH3G)
						{
							top_usb_inout_plug_info(USB_3G_PLUG_IN);
						}
#endif
					}
				}
#endif
                break;
            }

        case GXMSG_NETWORK_PLUG_OUT:
            {
                const char *wnd = GUI_GetFocusWindow();
                if((wnd != NULL) && (strcmp(wnd, WND_NETWORK) == 0))
                {
                    app_network_msg_proc(msg);
                }
#if DLNADMS_SUPPORT
            if(GUI_CheckDialog(WND_DLNA_DMS) == GXCORE_SUCCESS)
                ret = app_dlna_dms_hotplug_msg_proc(msg);
#endif
#if DLNADMR_SUPPORT
            if(GUI_CheckDialog(WND_DLNA_DMR) == GXCORE_SUCCESS)
                ret = app_dlna_dmr_hotplug_msg_proc(msg);
#endif
#if NET_AUDIO_SUPPORT
            app_netaudio_auto_msg_proc(msg);
#endif
#if WIFI_SUPPORT
                if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
                {
                    app_wifi_msg_proc(msg);
                }
#endif

#if BLUETOOTH_SUPPORT
                if(GUI_CheckDialog(WND_BLUETOOTH) == GXCORE_SUCCESS)
                {
                    app_bluetooth_msg_proc(msg);
                }
#endif

#if MOD_3G_SUPPORT
                if(GUI_CheckDialog(WND_3G) == GXCORE_SUCCESS)
                {
                    app_3g_msg_proc(msg);
                }
#endif

#if SAMBA_SUPPORT
                app_samba_msg_proc(msg);
#endif
#ifdef LINUX_OS
				{
					GxMsgProperty_NetDevType type;
					ubyte64 *dev_name = GxBus_GetMsgPropertyPtr(msg, ubyte64);
					if(dev_name != NULL)
					{
						memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
						strcpy(type.dev_name, *dev_name);
						app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
#if ETH_SUPPORT
						if(type.dev_type == IF_TYPE_WIRED)
						{
							top_usb_inout_plug_info(WIRED_PLUG_OUT);
						}
#endif
#if USB_ETH_SUPPORT
						if(type.dev_type == IF_TYPE_USB_WIRED)
						{
							top_usb_inout_plug_info(USB_NET_PLUG_OUT);
						}
#endif
#if WIFI_SUPPORT
						if(type.dev_type == IF_TYPE_WIFI)
						{
							app_wifi_msg_set_ap_false();
							top_usb_inout_plug_info(USB_WIFI_PLUG_OUT);
						}
#endif
#if BLUETOOTH_SUPPORT
						if(type.dev_type == IF_TYPE_BLUETOOTH)
						{
                            app_bluetooth_msg_set_ap_false();
							top_usb_inout_plug_info(USB_BLUETOOTH_PLUG_OUT);
						}
#endif
#if (MOD_3G_SUPPORT > 0)
						if(type.dev_type == IF_TYPE_3G || type.dev_type == IF_TYPE_ETH3G)
						{
							top_usb_inout_plug_info(USB_3G_PLUG_OUT);
						}
#endif
					}
				}
#endif

                break;
            }

#if NETAPPS_SUPPORT
        case APPMSG_IPTV_LIST_DONE:
        case APPMSG_IPTV_AD_PIC_DONE:
			ret = app_netapps_top_msg_proc(msg);
            break;
#endif
#endif

#if VPN_SUPPORT
        case APPMSG_VPN_STATE_REPORT:
            app_vpn_msg_proc(msg);
            break;
#endif
#if PLUGINSTORE_SUPPORT
        case APPMSG_PLUGIN_SOURCE_UI_CONTROL:
            if(GUI_CheckDialog(WND_PLUGIN_SOURCE_LIST) == GXCORE_SUCCESS)
                ret = app_plugin_source_msg_proc(msg);
            if(GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY) == GXCORE_SUCCESS)
                ret = app_plugin_listview_play_msg_proc(msg);
#if PLUGINONLINE_SUPPORT
            if(GUI_CheckDialog(WND_PLUGIN_MANAGE) == GXCORE_SUCCESS)
                ret = app_plugin_manage_msg_proc(msg);
#endif
            break;
#endif
#if WEBLET_SUPPORT
        case APPMSG_WEBLET_UI_CONTROL:
			{
				extern int app_weblet_msg_proc(GxMessage *msg);
				ret = app_weblet_msg_proc(msg);
			}
			break;
#endif
#if DLNADMR_SUPPORT || DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT || DLNADMS_SUPPORT
        case APPMSG_DLNA_UI_CONTROL:
			{
#if DLNADMR_SUPPORT
                if(GUI_CheckDialog(WND_DLNA_DMR) == GXCORE_SUCCESS)
                    ret = app_dlna_dmr_ui_msg_proc(msg);
#endif
#if DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT
                if(GUI_CheckDialog(WND_DLNA_DMP) == GXCORE_SUCCESS)
                    ret = app_dlna_dmp_ui_msg_proc(msg);
#endif
#if DLNADMS_SUPPORT
                if(GUI_CheckDialog(WND_DLNA_DMS) == GXCORE_SUCCESS)
                    ret = app_dlna_dms_ui_msg_proc(msg);
#endif
       		}
			break;
#endif
#ifdef SUBTTRANS_SUPPORT
        case APPMSG_SUBT_TRANS_CONTROL:
            {
                app_subttrans_ctl_msg_proc(msg);
            }
            break;
#endif
#if CASCAM_SUPPORT
        case GXMSG_CASCAM_EVENT:
            {
                app_log_info("\nCAS, %s, %d\n", __FUNCTION__,__LINE__);
                CascamOsdEvent *para = NULL;
                para = GxBus_GetMsgPropertyPtr(msg, CascamOsdEvent);
                app_ca_cascam_do_event(para);
            }
            break;
#endif
#if WEBAPI_SUPPORT
        case APPMSG_WEBAPI_UI_CONTROL:
			{
				ret = app_webapi_msg_proc(msg);
			}
			break;
#endif

#if SAT2IP_SERVER_SUPPORT
		case GXMSG_SATIP_PLAY_START:
			{
				int *param = NULL;
				param = GxBus_GetMsgPropertyPtr(msg, int);
				app_sat2ip_play_start(*param);
			}
			break;
		case GXMSG_SATIP_PLAY_STOP:
			{
				int *param = NULL;
				param = GxBus_GetMsgPropertyPtr(msg, int);
				app_sat2ip_play_stop(*param);
			}
			break;
#endif
#if DVB2IP_SERVER_SUPPORT
        case GXMSG_DVB2IP_PLAY_START:
            {
                Dvb2ipStartMsg *param= NULL;
                param = GxBus_GetMsgPropertyPtr(msg, Dvb2ipStartMsg);
                app_msg_dvb2ip_play_start(param);
            }
            break;
        case GXMSG_DVB2IP_PLAY_STOP:
            app_msg_dvb2ip_play_stop();
            break;
#endif

#if NETAPPS_SUPPORT
        case GXMSG_SUBTITLE_INIT:
        case GXMSG_SUBTITLE_DESTROY:
        case GXMSG_SUBTITLE_HIDE:
        case GXMSG_SUBTITLE_SHOW:
        case GXMSG_SUBTITLE_DRAW:
        case GXMSG_SUBTITLE_CLEAN:
        case APPMSG_NETVIDEO_PLAY_START:
        {
            if(GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_SUCCESS)
                app_netvideo_play_msg_proc(msg);
        }
        break;
        case APPMSG_NETVIDEO_DOWN_STATE_REPORT:
        {
#if WEBAPI_SUPPORT
            extern int app_supercast_update_msg_proc(GxMessage *msg);
            if((GUI_CheckDialog(WND_SUPERCAST) == GXCORE_SUCCESS) && (GUI_CheckDialog(WND_SYSTEM_SETTING) != GXCORE_SUCCESS))
                app_supercast_update_msg_proc(msg);
#endif
            break;
        }
#endif
#if USB_TEST_SUPPORT
        case APPMSG_USB_TEST:
            {
                GxMsgProperty_UsbTest *param = NULL;
                param = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbTest);
                if (param)
                    app_usb_test_msg_handle(*param);
            }
            break;
#endif
#if HDMI_CEC_SUPPORT
        case APPMSG_HDMI_CEC_UI_CONTROL:
			app_cec_link_ui_msg_proc(msg);
			break;
#endif
#if !OTT_SUPPORT
        case GXMSG_FRONTEND_DISH_MOVE:
        {
            MotorTip *tip = (MotorTip*)GxBus_GetMsgPropertyPtr(msg, MotorTip);
            if(tip->flag)
            {
                app_create_position_tip(tip->state, tip->motor_state);
            }
            else
            {
				popdlg_destroy();
            }
            break;
        }
#endif
#if PARENTAL_LOCK_SUPPORT
        case GXMSG_EPG_PARENTAL_SEND:
        case APPMSG_EPG_PARENTAL_SEND:
        {
            app_parental_rating_msg_proc(msg);
            break;
        }
#endif
#if IPTV_LITE_SUPPORT
        case APPMSG_IPTV_POWERON_PLAY:
        {
            extern int app_iptv_poweron_play_flag_get(void);
            if (app_iptv_poweron_play_flag_get() && (GXCORE_SUCCESS == GUI_CheckDialog(WND_FULL_SCREEN)) )
            {
                popdlg_destroy();//如果full screen 到 iptv_tips对话框还没创建的时候，网络连接正常并发过来消息
                if (app_iptv_poweron_play_flag_get())
                {
                    extern void app_iptv_poweron_play_flag_set(int flag);
                    app_iptv_poweron_play_flag_set(0);
                }
                g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_RADIO;
                g_AppFullArb.state.tv = STATE_OFF;
                app_stop_play_and_clean_full_screen(true);
                app_iptv_lite_create_menu();
            }
        }
        break;
#endif
#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT || BOUQUET_SUPPORT
  case APPMSG_ACU_STATE_REPORT:
        app_auto_update_msg_proc(msg);
        break;
#endif
#if PROGRAM_NETUP_SUPPORT
        case APPMSG_PROG_NETUP_STATE_REPORT:
            app_prog_netup_msg_proc(msg);
            break;
#endif

        case APPMSG_MESSAGE:
        {
#if VOICE_CONTROL_SUPPORT || FACE_DETECT_SUPPORT
            AppMsgMessageClass *new_message = NULL;
            new_message = (AppMsgMessageClass*)GxBus_GetMsgPropertyPtr(msg,AppMsgMessageClass);
#endif
#if VOICE_CONTROL_SUPPORT
            if(APPMSG_MESSAGE_VC == new_message->type)
                ret = app_vc_cloud_msg_process((void*)new_message);
#endif
#if FACE_DETECT_SUPPORT
            ret = app_face_detect_msg_process((void*)new_message);
#endif
        }
        break;

        default:
            break;
    }
    return ret;
}

int top_stbk_halt(uint32_t scan_code)
{
	time_t timerDur;
	time_t time_now;

#if POWER_ON_CONTROL_SUPPORT
    GxBus_ConfigSetInt(POWER_STANDBY_KEY, 0);
#endif
	time_now = g_AppTime.utc_get(&g_AppTime);
	timerDur = g_AppBook.get_sleep_time(time_now);
	app_low_power(timerDur, scan_code);
	return EVENT_TRANSFER_STOP;
}

static int _halt_stop_pvr_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        app_pvr_stop();
#if SHUTDOWN_MODE_SUPPORT
        extern int app_select_shutdow_mode_popup(void);
        int shutdown_mode_sel = 0;
        GxBus_ConfigGetInt(WIDGET_TEXT_SHUTDOWN_MODE_KEY, &shutdown_mode_sel, WIDGET_TEXT_SHUTDOWN_MODE_DEFAULT_KEY);
        if (1 == shutdown_mode_sel)
        {
            app_select_shutdow_mode_popup();
        }
        else
#endif
		{
            top_stbk_halt(thiz_halt_scancode);
        }
    }
    return 0;
}

static int app_top_keypress(GUI_Event* event)
{
    int ret = EVENT_TRANSFER_STOP;

    APP_CHECK_P(event, EVENT_TRANSFER_STOP);

#if AUTO_STANDBY_SUPPORT
    extern void app_set_auto_sleep_time(void);
    app_set_auto_sleep_time();
#endif

    if((app_simple_lowpower_status_get() == true) && (event->key.sym != STBK_HALT))
    {
        event->key.sym = SDLK_LAST;
    }
    if((event->key.sym != STBK_HALT) || (event->key.sym != SDLK_LAST))
    {
        app_panel_remote_led_show(true);
    }
    else
    {
        app_panel_remote_led_show(false);
    }

#if HDMI_CEC_SUPPORT
     extern int app_cec_stb_send_key(GUI_Event* event);
     app_cec_stb_send_key(event);
#endif

    switch(event->key.sym)
    {
#if CVBS_TEST_SUPPORT
        case STBK_F1:
            {
                if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) != GXCORE_SUCCESS)
                {
                    g_AppFullArb.timer_stop();
                    g_AppFullArb.state.pause = STATE_OFF;
                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                    extern void app_cvbs_test_menu_exec(void);
                    app_cvbs_test_menu_exec();
                    event->key.sym = 0;
                }
                break;
            }
#endif
        case STBK_HALT:
            {
#ifdef MEMORY_STABILITY_TEST_SUPPORT
#ifdef MEMORY_TEST_OUTPUT_OSD
                extern void app_mt_message_osd_exec(void);
                app_mt_message_osd_exec();
#endif
                {// memory stable test
                    extern void app_memory_test_start(void);
                    app_memory_test_start();
                }
                break;
#endif
            }
            {
                if((GUI_CheckDialog(WND_UPGRADE_PROCESS) == GXCORE_SUCCESS)
                    || (GUI_CheckDialog(WND_NETUPG_PROCESS) == GXCORE_SUCCESS)
                    || (GUI_CheckDialog(WND_SEARCH) == GXCORE_SUCCESS)
#if AUTO_UPDATE_SUPPORT
                    || (app_get_auto_update_flag() != 0)
#endif
#if PROGRAM_NETUP_SUPPORT
                    || (app_prog_netup_state_get() == true)
#endif
               #if TKGS_SUPPORT
                    || ((GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)&&(app_tkgs_get_upgrade_finish_pop_status() == true))
                    || ((GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS) == GXCORE_SUCCESS)&&(app_simple_lowpower_status_get() == false))
               #endif
                 )
                {
                    break;
                }
                if((g_AppPvrOps.state != PVR_RECORD)
                    && (g_AppPvrOps.state != PVR_TIMESHIFT)
                    && (g_AppPvrOps.state != PVR_TMS_AND_REC)
                    && (g_AppPvrOps.state != PVR_TP_RECORD))
                {
#if SHUTDOWN_MODE_SUPPORT
                    extern int app_select_shutdow_mode_popup(void);
                    int shutdown_mode_sel = 0;
                    GxBus_ConfigGetInt(WIDGET_TEXT_SHUTDOWN_MODE_KEY, &shutdown_mode_sel, WIDGET_TEXT_SHUTDOWN_MODE_DEFAULT_KEY);
                    if (1 == shutdown_mode_sel)
                    {
                        app_select_shutdow_mode_popup();
                    }
                    else
#endif
                    {
                        ret = top_stbk_halt(event->key.scancode);
                    }
                }
                else
                {
                    book_popdlg_exec(POP_VAL_CANCEL);
                    thiz_halt_scancode = event->key.scancode;
                    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP))
                    {
                        popdlg_destroy();
                    }
                    PopDlg  pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_YES_NO;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = app_pvr_get_tips(g_AppPvrOps.state);
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.exit_cb = _halt_stop_pvr_cb;
                    popdlg_create(&pop);
                }
            }
            break;
        default:
            return EVENT_TRANSFER_KEEPON;
    }

    return ret;
}

bool app_check_av_running(void)
{
    bool ret = false;
    AppFrontend_LockState lock;

    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
    if((lock == FRONTEND_LOCKED)
        && (s_avcodec_running == true))
    {
        ret = true;
    }
    else
    {
        if((s_avcodec_running == false)
            &&((g_AppPvrOps.state == PVR_TIMESHIFT)
                || (g_AppPvrOps.state == PVR_TMS_AND_REC)))
        {
            ret = true;
        }
        else
        {
            ret = false;
        }
    }
    return ret;
}

SIGNAL_HANDLER int top(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_KEEPON;

    if(NULL == usrdata) return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_SERVICE_MSG:
            ret = app_top_service(event);
            break;

#if LEARN_RCU_SUPPORT
        case GUI_KEYUP:
            {
                extern void app_remote_learn_rcu_key(GUI_Event* event);
                app_remote_learn_rcu_key(event);
                return EVENT_TRANSFER_STOP;
            }
#endif

        case GUI_KEYDOWN:
			GxCore_GetTickTime(&time_start);
			//app_log_debug("==========>Keypress time: %d s, %d us\n", time_start.seconds, time_start.microsecs);

            //如果是待机醒来维护阶段，接口收到HALT消息按键，直接开启视频输出。如果是其它按键，直接返回。add 20241113
            if ( app_fuse_spec_wake_from_reboot_flag_keypress(event->key.sym) )
            {
                return EVENT_TRANSFER_STOP ;
            }

            ret = app_top_keypress(event);
            break;
        default:
            return EVENT_TRANSFER_KEEPON;
    }

    return ret;
}
