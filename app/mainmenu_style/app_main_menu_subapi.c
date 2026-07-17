
#include "app_config.h"

#if APP_MAIN_MENU_OBJ_SUPPORT

#include <string.h>
#include <stdio.h>
#include "app.h"
#include "app_main_menu_exapi.h"
#include "app_main_menu.h"

//这个宏定义，只能主菜单换肤前首次基准菜单的类型选择CLASSIC_BLUE,才设置1
#if THEME_CLASSIC_BLUE
#define APP_OUT_ELF_THEME_CLASSIC_BLUE_SUPPORT 1
#else
#define APP_OUT_ELF_THEME_CLASSIC_BLUE_SUPPORT 0
#endif

extern void app_tv_channel_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_TV_CHANNEL    app_tv_channel_menu_exec

extern void app_radio_channel_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_RADIO_CHANNEL    app_radio_channel_menu_exec

extern void app_deleteall_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DELETE_ALL    app_deleteall_menu_exec

extern void app_time_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_TIME_SETTING    app_time_setting_menu_exec

extern void app_timer_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_TIMER_SETTING    app_timer_setting_menu_exec

extern void app_lang_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_LANGUAGE    app_lang_setting_menu_exec

extern void app_av_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_AV_SETTING    app_av_setting_menu_exec

extern void app_lock_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_LOCK_CONTROL   app_lock_setting_menu_exec

extern void app_osd_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_OSD_SETTING   app_osd_setting_menu_exec

extern void app_color_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_COLOR_SETTING   app_color_setting_menu_exec

#if NETWORK_SUPPORT
extern void app_network_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_NETWORK_SETTING   app_network_menu_exec
#define SUB_API_CREATE_APPUI_ENTRY_NETWORK           app_network_menu_exec
#endif

extern void app_advanced_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_ADVANCED_SETTING   app_advanced_setting_menu_exec

#if DEMOD_DVB_S
extern void app_antenna_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_ANTENNA   app_antenna_setting_menu_exec

extern void app_positioner_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_POSITIONER   app_positioner_setting_menu_exec

extern void app_sat_list_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_SAT_LIST   app_sat_list_menu_exec

extern void app_tp_list_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_TP_LIST   app_tp_list_menu_exec
#endif

#if SAT_FINDER_SUPPORT
extern void app_satfinder_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBFINDER   app_satfinder_menu_exec
#endif

extern void app_sysinfo_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_SYSTEM_INFO   app_sysinfo_menu_exec
#define SUB_API_CREATE_APPUI_ENTRY_SOFTINFO      app_sysinfo_menu_exec

extern void app_reset_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_FACTORY_RESET   app_reset_menu_exec

extern void app_upgrade_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_SOFTWARE_UPGRADE   app_upgrade_menu_exec
#define SUB_API_CREATE_APPUI_ENTRY_SOFTUPGRADE        app_upgrade_menu_exec

#if NET_UPGRADE_SUPPORT
extern void app_create_netupgrade_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_UPGRADE_BY_NET   app_create_netupgrade_menu
#endif

#if SKIN_SUPPORT
extern void app_create_skin_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_SKIN   app_create_skin_menu
#endif

#if APPLETS_SUPPORT
extern void app_create_applets_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_APPLETS   app_create_applets_menu
#endif

#ifdef XTREAM_SUPPORT
extern void app_create_xtream_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_XTREAM   app_create_xtream_menu
#endif

#ifdef IPTV_SUPPORT
extern void app_create_gxiptv_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_IPTV   app_create_gxiptv_menu
#endif

#ifdef STALKER_SUPPORT
extern void app_create_stalker_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_STALKER   app_create_stalker_menu
#endif

extern void app_media_file_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_MEDIA_FILE   app_media_file_menu_exec

extern void app_media_video_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_MEDIA_VIDEO   app_media_video_menu_exec

extern void app_media_music_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_MEDIA_MUSIC   app_media_music_menu_exec

extern void app_media_picture_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_MEDIA_PIC   app_media_picture_menu_exec

extern void app_create_pvr_media_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_PVR_MEDIA   app_create_pvr_media_menu

extern void app_pvr_set_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_PVR_MANAGEMENT   app_pvr_set_menu_exec

#if APP_OUT_ELF_THEME_CLASSIC_BLUE_SUPPORT
extern void app_media_centre_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_MEDIA_PLAYER   app_media_centre_menu_exec
#endif

extern void app_power_on_off_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_POWER_ON_OFF   app_power_on_off_menu_exec

extern void app_internet_menu_exec(void);
extern int  app_internet_menu_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_INTERNET   app_internet_menu_exec
#define SUB_API_CHECK_APPUI_ENTRY_INTERNET    app_internet_menu_check

extern void app_channel_edit_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_CHANNEL_EDIT   app_channel_edit_menu_exec

extern void app_create_epg_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_RPOGRAM_GUIDE   app_create_epg_menu

#if (DEMOD_DVB_T) && (0 == DEMOD_DVB_S)
extern void app_dvbt_time_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_T2_TIME_SETTING   app_dvbt_time_menu_exec
#endif

#if AUTO_STANDBY_SUPPORT
extern void app_sleep_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_SLEEP   app_sleep_setting_menu_exec
#endif

extern void app_timer_play_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_PLAY_TIMER_SETTING   app_timer_play_menu_exec

#if RF_MODULATOR_SUPPORT
extern void app_rf_modulator_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_RM_MODULATOR_SETTING   app_rf_modulator_setting_menu_exec
#endif

extern void app_dvbt_lock_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_LOCK_CONTROL   app_dvbt_lock_setting_menu_exec

#if PARENTAL_LOCK_SUPPORT
extern void app_parent_lock_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_PARENTAL_GUIDANCE   app_parent_lock_setting_menu_exec

extern void app_dvbt_parent_lock_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_PARENTAL_GUIDANCE   app_dvbt_parent_lock_setting_menu_exec
#endif

extern void app_t2_osd_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_T2_OSD_SETTING   app_t2_osd_setting_menu_exec

#if SAT2IP_SERVER_SUPPORT
extern void app_sat2ip_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_SAT2IP_SETTING   app_sat2ip_setting_menu_exec
#endif

#if DVB2IP_SERVER_SUPPORT
extern void app_dvb2ip_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVB2IP_SETTING   app_dvb2ip_setting_menu_exec
#endif

#if VPN_SUPPORT
extern void app_vpn_menu_create(void);
#define SUB_API_CREATE_APPUI_ENTRY_VPN   app_vpn_menu_create
#endif

#if PANEL_SETTING_SUPPORT
extern void app_panel_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_PANEL_SETTING   app_panel_setting_menu_exec

extern void app_dvbt_panel_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_T2_PANEL_SETTING   app_dvbt_panel_setting_menu_exec
#endif

#if LCN_SUPPORT
extern void app_lcn_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_LCN_SETTING   app_lcn_setting_menu_exec
#endif

#if HDMI_CEC_SUPPORT
extern void app_cec_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_CEC_SETTING   app_cec_setting_menu_exec
#endif

#if NET_SEARCH_SUPPORT
extern void app_net_search_setting_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_NET_SEARCH   app_net_search_setting_exec
#endif

#if TKGS_SUPPORT
extern void app_tkgs_upgrade_menu_create(void);
#define SUB_API_CREATE_APPUI_ENTRY_TKGS   app_tkgs_upgrade_menu_create
#endif

#if BLUETOOTH_SUPPORT
extern void app_bluetooth_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_BLUETOOTH_SETTING   app_bluetooth_menu_exec

extern void app_create_sink_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_SINK   app_create_sink_menu
#endif

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
extern void app_dvbt2_setting(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT    app_dvbt2_setting
#define SUB_API_CREATE_APPUI_ENTRY_ISDBT   app_dvbt2_setting
#endif

#if DEMOD_DVB_C
extern void app_dvbc_setting(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBC   app_dvbc_setting

extern void app_dvbc_auto_search_entry(void);
extern int  app_dvbc_auto_search_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBC_AUTOSCAN   app_dvbc_auto_search_entry
#define SUB_API_CHECK_APPUI_ENTRY_DVBC_AUTOSCAN    app_dvbc_auto_search_check

extern void app_dvbc_manual_search_menu_exec(void);
extern int  app_dvbc_manual_search_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBC_MANUALSCAN   app_dvbc_manual_search_menu_exec
#define SUB_API_CHECK_APPUI_ENTRY_DVBC_MANUALSCAN    app_dvbc_manual_search_check

extern void app_dvbc_all_search_menu_exec(void);
extern int  app_dvbc_all_search_menu_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBC_FULLSCAN   app_dvbc_all_search_menu_exec
#define SUB_API_CHECK_APPUI_ENTRY_DVBC_FULLSCAN    app_dvbc_all_search_menu_check

extern void app_dvbc_set_main_frequency_menu_exec(void);
extern int  app_dvbc_set_main_frequency_menu_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBC_SETFRE   app_dvbc_set_main_frequency_menu_exec
#define SUB_API_CHECK_APPUI_ENTRY_DVBC_SETFRE    app_dvbc_set_main_frequency_menu_check
#endif

#if DEMOD_DVB_T
extern void app_dvbt2_auto_search_entry(void);
extern int  app_dvbt_auto_search_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_AUTOSCAN   app_dvbt2_auto_search_entry
#define SUB_API_CHECK_APPUI_ENTRY_DVBT_AUTOSCAN    app_dvbt_auto_search_check

extern void app_dvbt_manual_search_menu_exec(void);
extern int  app_dvbt_manual_search_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_MANUALSCAN   app_dvbt_manual_search_menu_exec
#define SUB_API_CHECK_APPUI_ENTRY_DVBT_MANUALSCAN    app_dvbt_manual_search_check
#endif

extern void app_dvbt_media_centre_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_MEDIA_CENTRE   app_dvbt_media_centre_menu_exec
#define SUB_API_CREATE_APPUI_ENTRY_MEDIA_CENTRE        app_dvbt_media_centre_menu_exec

extern void app_dvbt_pvr_set_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_PVR_MANAGEMENT   app_dvbt_pvr_set_menu_exec

#if DEMOD_DTMB
extern void app_dtmb_setting(void);
#define SUB_API_CREATE_APPUI_ENTRY_DTMB   app_dtmb_setting

extern void app_dtmb_auto_search(void);
#define SUB_API_CREATE_APPUI_ENTRY_DTMB_AUTOSCAN   app_dtmb_auto_search

extern void app_dtmb_manual_search_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DTMB_MANUALSCAN   app_dtmb_manual_search_menu_exec

extern void app_dtmb_all_search_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DTMB_FULLSCAN   app_dtmb_all_search_menu_exec

extern void app_dtmb_main_freq_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DTMB_SETFRE   app_dtmb_main_freq_menu_exec
#endif

#if ADVANCED_PRINT_SUPPORT
extern void app_print_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_PRINT_SETTING   app_print_setting_menu_exec
#endif

#if CVBS_TEST_SUPPORT
extern void app_cvbs_test_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_CVBS_TEST_SETTING   app_cvbs_test_menu_exec
#endif

#if GAME_ENABLE
extern void app_game_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_GAME_CENTRE   app_game_menu_exec
#endif

extern void app_t2_upgrade_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_T2_SOFTWARE_UPGRADE   app_t2_upgrade_menu_exec

#if OTA_SUPPORT
extern void app_ota_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_OTA   app_ota_menu_exec
#endif

#if CI_SUPPORT
extern void app_dvb_ci_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVB_CI   app_dvb_ci_menu_exec
#endif

#if EX_SHIFT_SUPPORT
extern void app_special_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_CA_SETTING   app_special_menu_exec
#endif

#if CASCAM_SUPPORT
extern void app_ca_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_CASCAM_SETTING   app_ca_menu_exec
#endif

#if POWER_ON_CONTROL_SUPPORT
extern void app_poweroncontrol_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_POWERONCONTROL   app_poweroncontrol_setting_menu_exec
#endif

#if FM_SUPPORT
extern void app_dvbt_fm_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_FM   app_dvbt_fm_exec

extern void app_fm_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_FM_SETTING   app_fm_setting_menu_exec

extern void app_fm_auto_search_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_FM_AUTOSCAN   app_fm_auto_search_exec

extern void app_fm_play_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_FM_PLAY   app_fm_play_exec
#endif

#if USB_TEST_SUPPORT
extern void app_usb_test_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_USB_TEST   app_usb_test_menu_exec

extern void app_dvbt_usb_test_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_USB_TEST   app_dvbt_usb_test_menu_exec
#endif

#if USB_SAFELY_REMOVE_SUPPORT
extern void app_create_rusb_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_USB_REMOVE   app_create_rusb_menu
#endif

#if WEBLET_SUPPORT
extern void app_weblet_init_dialog(void);
#define SUB_API_CREATE_APPUI_ENTRY_WEBLET   app_weblet_init_dialog
#endif

#if PLUGINSTORE_SUPPORT
extern void app_create_pluginstore_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_PLUGIN_PORTAL   app_create_pluginstore_menu
#endif

#if SAMBA_SUPPORT
extern void app_create_samba_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_SAMBA   app_create_samba_menu
#endif

#if TRANSMISSION_SUPPORT
extern void app_create_transmission_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_TRANSMISSION   app_create_transmission_menu
#endif

#ifdef FREEIPTV_SUPPORT
extern void app_create_freeiptv_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_FREEIPTV   app_create_freeiptv_menu
#endif

#if DLNADMR_SUPPORT
extern void app_create_dlna_dmr_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_DLNA_DMR   app_create_dlna_dmr_menu
#endif

#if DLNADMP_SUPPORT
extern void app_create_dlna_dmp_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_DLNA_DMP   app_create_dlna_dmp_menu
#endif

#if DLNASAT2IP_SUPPORT
extern void app_create_dlna_sat2ip_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_DLNA_SAT2IP   app_create_dlna_sat2ip_menu
#endif

#if DLNADMS_SUPPORT
extern void app_create_dlna_dms_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_DLNA_DMS   app_create_dlna_dms_menu
#endif

extern void app_dvbt_dlna_menu_exec(void);
extern int  app_dvbt_dlna_menu_check(void);
#define SUB_API_CREATE_APPUI_ENTRY_DVBT_DLNA   app_dvbt_dlna_menu_exec
#define SUB_API_CHECK_APPUI_ENTRY_DVBT_DLNA    app_dvbt_dlna_menu_check

#if WEBAPI_SUPPORT
extern void app_create_supercast_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_SUPERCAST   app_create_supercast_menu_exec
#endif

#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
//extern void app_create_applets_youtube_menu(void);
//#define SUB_API_CREATE_APPUI_ENTRY_CER_YOUTUBE   app_create_applets_youtube_menu
#endif

#if LEARN_RCU_SUPPORT
extern void app_create_learn_new_rcu_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_LEARN_RCU   app_create_learn_new_rcu_menu
#endif

#if (DEMOD_DVB_S > 0)
extern void app_installatio_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_INSTALLATION   app_installatio_menu_exec
#endif

extern void app_setting_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_SETTING   app_setting_menu_exec

extern void app_utility_menu_exec(void);
#define SUB_API_CREATE_APPUI_ENTRY_UTILITY   app_utility_menu_exec

#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
extern void app_create_applets_weather_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_APPLET_WEATHER   app_create_applets_weather_menu
#endif

#if SOCKS5_SUPPORT
extern void app_create_socks5_menu(void);
#define SUB_API_CREATE_APPUI_ENTRY_SOCKS5   app_create_socks5_menu
#endif

#ifdef RSS_SUPPORT
extern void app_rss_create_dialog(void);
#define SUB_API_CREATE_APPUI_ENTRY_RSS   app_rss_create_dialog
#endif

// SETTING

extern int   app_program_sort_keypress(int key);
extern char* app_program_sort_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_SORT    app_program_sort_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_SORT    app_program_sort_content_get

#if LCN_SUPPORT
extern int   app_lcn_setting_keypress(int key);
extern char* app_lcn_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_LCN    app_lcn_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_LCN    app_lcn_setting_content_get
#endif

#if AUTO_UPDATE_SUPPORT
extern int   app_acu_setting_keypress(int key);
extern char* app_acu_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_ACU    app_acu_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_ACU    app_acu_setting_content_get
#endif

extern int   app_program_volum_scope_keypress(int key);
extern char* app_program_volum_scope_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_VOLUM_SCOPE    app_program_volum_scope_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_VOLUM_SCOPE    app_program_volum_scope_content_get

extern int   app_aspect_ratio_setting_keypress(int key);
extern char* app_aspect_ratio_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_ASPECT_RATIO    app_aspect_ratio_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_ASPECT_RATIO    app_aspect_ratio_setting_content_get

extern int   app_resolution_setting_keypress(int key);
extern char* app_resolution_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_RESOLUTION    app_resolution_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_RESOLUTION    app_resolution_setting_content_get

extern int   app_tv_format_setting_keypress(int key);
extern char* app_tv_format_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_TV_FORMAT    app_tv_format_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_TV_FORMAT    app_tv_format_setting_content_get

#ifdef SUPPORT_SCART
extern int   app_scart_setting_keypress(int key);
extern char* app_scart_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_SCART    app_scart_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_SCART    app_scart_setting_content_get
#endif

#if HIGH_DYNAMIC_RANGE_SUPPORT
extern int   app_hdr_mode_setting_keypress(int key);
extern char* app_hdr_mode_setting_content_get(void);
extern int   app_hdr_mode_setting_check(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_HDR_MODE    app_hdr_mode_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_HDR_MODE    app_hdr_mode_setting_content_get
#define SUB_API_CHECK_APPUI_SETTING_HDR_MODE    app_hdr_mode_setting_check
#endif

#if DEMOD_DVB_T || DEMOD_ISDBT
extern int   app_search_mode_setting_keypress(int key);
extern char* app_search_mode_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_SEARCH_MODE    app_search_mode_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_SEARCH_MODE    app_search_mode_setting_content_get

extern int   app_search_fta_only_setting_keypress(int key);
extern char* app_search_fta_only_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_FTA_ONLY    app_search_fta_only_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_FTA_ONLY    app_search_fta_only_setting_content_get

extern int   app_dvbt_country_setting_keypress(int key);
extern char* app_dvbt_country_setting_content_get(void);
extern int   app_dvbt_country_setting_check(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_COUNTRY    app_dvbt_country_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_COUNTRY    app_dvbt_country_setting_content_get
#define SUB_API_CHECK_APPUI_SETTING_COUNTRY    app_dvbt_country_setting_check
#endif

#if DEMOD_DVB_T
extern int   app_antenna_power_setting_keypress(int key);
extern char* app_antenna_power_setting_content_get(void);
extern int   app_antenna_power_setting_check(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_ANT_POWER    app_antenna_power_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_ANT_POWER    app_antenna_power_setting_content_get
#define SUB_API_CHECK_APPUI_SETTING_ANT_POWER    app_antenna_power_setting_check
#endif

#if AUTO_STANDBY_SUPPORT
extern int   app_sleep_setting_keypress(int key);
extern char* app_sleep_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_SLEEP    app_sleep_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_SLEEP    app_sleep_setting_content_get
#endif

#if POWER_ON_CONTROL_SUPPORT
extern int   app_poweroncontrol_setting_keypress(int key);
extern char* app_poweroncontrol_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_POWERONCONTROL    app_poweroncontrol_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_POWERONCONTROL    app_poweroncontrol_setting_content_get
#endif

extern int   app_osd_lang_setting_keypress(int key);
extern char* app_osd_lang_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_OSD_LAN    app_osd_lang_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_OSD_LAN    app_osd_lang_setting_content_get

extern int   app_epg_lang_setting_keypress(int key);
extern char* app_epg_lang_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_EPG_LAN    app_epg_lang_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_EPG_LAN    app_epg_lang_setting_content_get

extern int   app_subt_lang_setting_keypress(int key);
extern char* app_subt_lang_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_SUB_LAN    app_subt_lang_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_SUB_LAN    app_subt_lang_setting_content_get

extern int   app_subt_hoh_keypress(int key);
extern char* app_subt_hoh_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_HOH_SUB    app_subt_hoh_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_HOH_SUB    app_subt_hoh_content_get

extern int   app_ttx_lang_setting_keypress(int key);
extern char* app_ttx_lang_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_TTX_LAN    app_ttx_lang_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_TTX_LAN    app_ttx_lang_setting_content_get

extern int   app_first_audio_lang_setting_keypress(int key);
extern char* app_first_audio_lang_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_FIRST_AUDIO_LAN    app_first_audio_lang_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_FIRST_AUDIO_LAN    app_first_audio_lang_setting_content_get

extern int   app_second_audio_lang_setting_keypress(int key);
extern char* app_second_audio_lang_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_SECOND_AUDIO_LAN    app_second_audio_lang_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_SECOND_AUDIO_LAN    app_second_audio_lang_setting_content_get

extern int   app_digital_audio_setting_keypress(int key);
extern char* app_digital_audio_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_DIGITAL_AUDIO    app_digital_audio_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_DIGITAL_AUDIO    app_digital_audio_setting_content_get

extern int   app_audio_remember_setting_keypress(int key);
extern char* app_audio_remember_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_AUDIO_REME    app_audio_remember_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_AUDIO_REME    app_audio_remember_setting_content_get

#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
extern int   app_no_signal_auto_standby_setting_keypress(int key);
extern char* app_no_signal_auto_standby_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_NO_SIGNAL_AUTO_STANDBY    app_no_signal_auto_standby_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_NO_SIGNAL_AUTO_STANDBY    app_no_signal_auto_standby_content_get
#endif

#if AUDIO_DESCRIPTOR_SUPPORT
extern int   app_audio_desc_setting_keypress(int key);
extern char* app_audio_desc_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_AUDIO_DESC    app_audio_desc_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_AUDIO_DESC    app_audio_desc_setting_content_get
#endif

#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
extern int   app_ad_vol_offset_setting_keypress(int key);
extern char* app_ad_vol_offset_setting_content_get(void);
extern int   app_ad_vol_offset_setting_check(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_AD_VOL_OFFSET    app_ad_vol_offset_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_AD_VOL_OFFSET    app_ad_vol_offset_setting_content_get
#define SUB_API_CHECK_APPUI_SETTING_AD_VOL_OFFSET    app_ad_vol_offset_setting_check
#endif

#if IPTV_LITE_SUPPORT
extern int   app_poweron_play_keypress(int key);
extern char* app_poweron_play_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_POWERON_PLAY    app_poweron_play_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_POWERON_PLAY    app_poweron_play_content_get
#endif

#if PVR_STANDBY_TIP_SUPPORT
extern int   app_pvr_end_standby_setting_keypress(int key);
extern char* app_pvr_end_standby_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_PVR_END_STANDBY    app_pvr_end_standby_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_PVR_END_STANDBY    app_pvr_end_standby_content_get
#endif

#if NO_SIGNAL_ALERT_TIP_SUPPORT
extern int   app_no_signal_alert_tip_setting_keypress(int key);
extern char* app_no_signal_alert_tip_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_NO_SIGNAL_ALERT_TIP    app_no_signal_alert_tip_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_NO_SIGNAL_ALERT_TIP    app_no_signal_alert_tip_content_get
#endif

#if HDMI_CEC_SUPPORT
extern int   app_cec_setting_keypress(int key);
extern char* app_cec_setting_content_get(void);
#define SUB_API_SETTING_KEYPRESS_APPUI_SETTING_CEC    app_cec_setting_keypress
#define SUB_API_SETTING_GCONTENT_APPUI_SETTING_CEC    app_cec_setting_content_get
#endif

#define CALL_API_CREATE(SRC,DST)\
{\
    if ( SRC == DST ) \
	{\
		printf("sub create, id = %d \n",SRC); \
		SUB_API_CREATE_##SRC();  \
		return 0 ; \
	}\
}

#define CALL_API_CHECK(SRC,DST)\
{\
    if ( SRC == DST ) \
	{\
		return SUB_API_CHECK_##SRC();  \
	}\
}

#define CALL_API_SETTING_KEYPRESS(SRC,DST,KEY)\
{\
    if ( SRC == DST ) \
	{\
		return SUB_API_SETTING_KEYPRESS_##SRC(KEY);  \
	}\
}

#define CALL_API_SETTING_CONTENT(SRC,DST)\
{\
    if ( SRC == DST ) \
	{\
		return SUB_API_SETTING_GCONTENT_##SRC();  \
	}\
}

int gxapp_exapi_mainmenu_sub_create(int id)
{
	CALL_API_CREATE(APPUI_ENTRY_TV_CHANNEL,id);
	CALL_API_CREATE(APPUI_ENTRY_RADIO_CHANNEL,id);
	CALL_API_CREATE(APPUI_ENTRY_DELETE_ALL,id);

	CALL_API_CREATE(APPUI_ENTRY_TIME_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_TIMER_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_LANGUAGE,id);
	CALL_API_CREATE(APPUI_ENTRY_AV_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_LOCK_CONTROL,id);
	CALL_API_CREATE(APPUI_ENTRY_OSD_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_COLOR_SETTING,id);
#if NETWORK_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_NETWORK_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_NETWORK,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_ADVANCED_SETTING,id);

#if DEMOD_DVB_S
	CALL_API_CREATE(APPUI_ENTRY_ANTENNA,id);
	CALL_API_CREATE(APPUI_ENTRY_POSITIONER,id);
	CALL_API_CREATE(APPUI_ENTRY_SAT_LIST,id);
	CALL_API_CREATE(APPUI_ENTRY_TP_LIST,id);
#endif
#if SAT_FINDER_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DVBFINDER,id);
#endif

	CALL_API_CREATE(APPUI_ENTRY_SYSTEM_INFO,id);
	CALL_API_CREATE(APPUI_ENTRY_SOFTINFO,id);
	CALL_API_CREATE(APPUI_ENTRY_FACTORY_RESET,id);
	CALL_API_CREATE(APPUI_ENTRY_SOFTWARE_UPGRADE,id);
	CALL_API_CREATE(APPUI_ENTRY_SOFTUPGRADE,id);
#if NET_UPGRADE_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_UPGRADE_BY_NET,id);
#endif
#if SKIN_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_SKIN,id);
#endif
#if APPLETS_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_APPLETS,id);
#endif
#ifdef XTREAM_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_XTREAM,id);
#endif
#ifdef IPTV_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_IPTV,id);
#endif
#ifdef STALKER_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_STALKER,id);
#endif

	CALL_API_CREATE(APPUI_ENTRY_MEDIA_FILE,id);
	CALL_API_CREATE(APPUI_ENTRY_MEDIA_VIDEO,id);
	CALL_API_CREATE(APPUI_ENTRY_MEDIA_MUSIC,id);
	CALL_API_CREATE(APPUI_ENTRY_MEDIA_PIC,id);
	CALL_API_CREATE(APPUI_ENTRY_PVR_MEDIA,id);
	CALL_API_CREATE(APPUI_ENTRY_PVR_MANAGEMENT,id);
#if APP_OUT_ELF_THEME_CLASSIC_BLUE_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_MEDIA_PLAYER,id);
#endif

	CALL_API_CREATE(APPUI_ENTRY_POWER_ON_OFF,id);
	CALL_API_CREATE(APPUI_ENTRY_INTERNET,id);
	CALL_API_CREATE(APPUI_ENTRY_CHANNEL_EDIT,id);
	CALL_API_CREATE(APPUI_ENTRY_RPOGRAM_GUIDE,id);
#if (DEMOD_DVB_T) && (0 == DEMOD_DVB_S)
	CALL_API_CREATE(APPUI_ENTRY_T2_TIME_SETTING,id);
#endif
#if AUTO_STANDBY_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_SLEEP,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_PLAY_TIMER_SETTING,id);
#if RF_MODULATOR_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_RM_MODULATOR_SETTING,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_DVBT_LOCK_CONTROL,id);
#if PARENTAL_LOCK_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_PARENTAL_GUIDANCE,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBT_PARENTAL_GUIDANCE,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_T2_OSD_SETTING,id);
#if SAT2IP_SERVER_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_SAT2IP_SETTING,id);
#endif
#if DVB2IP_SERVER_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DVB2IP_SETTING,id);
#endif
#if VPN_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_VPN,id);
#endif
#if PANEL_SETTING_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_PANEL_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_T2_PANEL_SETTING,id);
#endif
#if LCN_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_LCN_SETTING,id);
#endif
#if HDMI_CEC_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_CEC_SETTING,id);
#endif
#if NET_SEARCH_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_NET_SEARCH,id);
#endif
#if TKGS_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_TKGS,id);
#endif

#if BLUETOOTH_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_BLUETOOTH_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_SINK,id);
#endif

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
	CALL_API_CREATE(APPUI_ENTRY_DVBT,id);
	CALL_API_CREATE(APPUI_ENTRY_ISDBT,id);
#endif
#if DEMOD_DVB_C
	CALL_API_CREATE(APPUI_ENTRY_DVBC,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBC_AUTOSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBC_MANUALSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBC_FULLSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBC_SETFRE,id);
#endif
#if DEMOD_DVB_T
	CALL_API_CREATE(APPUI_ENTRY_DVBT_AUTOSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBT_MANUALSCAN,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_DVBT_MEDIA_CENTRE,id);
	CALL_API_CREATE(APPUI_ENTRY_MEDIA_CENTRE,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBT_PVR_MANAGEMENT,id);
#if DEMOD_DTMB
	CALL_API_CREATE(APPUI_ENTRY_DTMB,id);
	CALL_API_CREATE(APPUI_ENTRY_DTMB_AUTOSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_DTMB_MANUALSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_DTMB_FULLSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_DTMB_SETFRE,id);
#endif

#if ADVANCED_PRINT_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_PRINT_SETTING,id);
#endif
#if CVBS_TEST_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_CVBS_TEST_SETTING,id);
#endif
#if GAME_ENABLE
	CALL_API_CREATE(APPUI_ENTRY_GAME_CENTRE,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_T2_SOFTWARE_UPGRADE,id);

#if OTA_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_OTA,id);
#endif
#if CI_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DVB_CI,id);
#endif
#if EX_SHIFT_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_CA_SETTING,id);
#endif
#if CASCAM_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_CASCAM_SETTING,id);
#endif
#if POWER_ON_CONTROL_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_POWERONCONTROL,id);
#endif

#if FM_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DVBT_FM,id);
	CALL_API_CREATE(APPUI_ENTRY_FM_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_FM_AUTOSCAN,id);
	CALL_API_CREATE(APPUI_ENTRY_FM_PLAY,id);
#endif

#if USB_TEST_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_USB_TEST,id);
	CALL_API_CREATE(APPUI_ENTRY_DVBT_USB_TEST,id);
#endif
#if USB_SAFELY_REMOVE_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_USB_REMOVE,id);
#endif

#if WEBLET_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_WEBLET,id);
#endif
#if PLUGINSTORE_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_PLUGIN_PORTAL,id);
#endif
#if SAMBA_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_SAMBA,id);
#endif
#if TRANSMISSION_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_TRANSMISSION,id);
#endif
#ifdef FREEIPTV_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_FREEIPTV,id);
#endif

#if DLNADMR_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DLNA_DMR,id);
#endif
#if DLNADMP_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DLNA_DMP,id);
#endif
#if DLNASAT2IP_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DLNA_SAT2IP,id);
#endif
#if DLNADMS_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_DLNA_DMS,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_DVBT_DLNA,id);
#if WEBAPI_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_SUPERCAST,id);
#endif
#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
	//CALL_API_CREATE(APPUI_ENTRY_CER_YOUTUBE,id);
#endif

#if LEARN_RCU_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_LEARN_RCU,id);
#endif
#if (DEMOD_DVB_S > 0)
	CALL_API_CREATE(APPUI_ENTRY_INSTALLATION,id);
#endif
	CALL_API_CREATE(APPUI_ENTRY_SETTING,id);
	CALL_API_CREATE(APPUI_ENTRY_UTILITY,id);

#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
	CALL_API_CREATE(APPUI_ENTRY_APPLET_WEATHER,id);
#endif

#if SOCKS5_SUPPORT
    CALL_API_CREATE(APPUI_ENTRY_SOCKS5,id);
#endif

#ifdef RSS_SUPPORT
    CALL_API_CREATE(APPUI_ENTRY_RSS,id);
#endif

	printf("create can not find, id = %d \n",id);
	return 0 ;
}

int gxapp_exapi_mainmenu_sub_check(int id)
{
	int ret = 0 ;
	CALL_API_CHECK(APPUI_ENTRY_INTERNET,id);
#if DEMOD_DVB_C
	CALL_API_CHECK(APPUI_ENTRY_DVBC_AUTOSCAN,id);
	CALL_API_CHECK(APPUI_ENTRY_DVBC_MANUALSCAN,id);
	CALL_API_CHECK(APPUI_ENTRY_DVBC_FULLSCAN,id);
	CALL_API_CHECK(APPUI_ENTRY_DVBC_SETFRE,id);
#endif
#if DEMOD_DVB_T
	CALL_API_CHECK(APPUI_ENTRY_DVBT_AUTOSCAN,id);
	CALL_API_CHECK(APPUI_ENTRY_DVBT_MANUALSCAN,id);
#endif
	CALL_API_CHECK(APPUI_ENTRY_DVBT_DLNA,id);

#if HIGH_DYNAMIC_RANGE_SUPPORT
	CALL_API_CHECK(APPUI_SETTING_HDR_MODE,id);
#endif
#if DEMOD_DVB_T || DEMOD_ISDBT
	CALL_API_CHECK(APPUI_SETTING_COUNTRY,id);
#endif
#if DEMOD_DVB_T
	CALL_API_CHECK(APPUI_SETTING_ANT_POWER,id);
#endif
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
	CALL_API_CHECK(APPUI_SETTING_AD_VOL_OFFSET,id);
#endif
	printf("check can not find, id = %d \n",id);
	return ret ;
}

int   gxapp_exapi_mainmenu_keypress(int id,int key)
{
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_SORT,id,key);
#if LCN_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_LCN,id,key);
#endif
#if AUTO_UPDATE_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_ACU,id,key);
#endif
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_VOLUM_SCOPE,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_ASPECT_RATIO,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_RESOLUTION,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_TV_FORMAT,id,key);
#ifdef SUPPORT_SCART
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_SCART,id,key);
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_HDR_MODE,id,key);
#endif
#if DEMOD_DVB_T || DEMOD_ISDBT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_SEARCH_MODE,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_FTA_ONLY,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_COUNTRY,id,key);
#endif
#if DEMOD_DVB_T
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_ANT_POWER,id,key);
#endif
#if AUTO_STANDBY_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_SLEEP,id,key);
#endif
#if POWER_ON_CONTROL_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_POWERONCONTROL,id,key);
#endif
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_OSD_LAN,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_EPG_LAN,id,key);

	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_SUB_LAN,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_HOH_SUB,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_TTX_LAN,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_FIRST_AUDIO_LAN,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_SECOND_AUDIO_LAN,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_DIGITAL_AUDIO,id,key);
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_AUDIO_REME,id,key);
#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_NO_SIGNAL_AUTO_STANDBY,id,key);
#endif
#if AUDIO_DESCRIPTOR_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_AUDIO_DESC,id,key);
#endif
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_AD_VOL_OFFSET,id,key);
#endif
#if IPTV_LITE_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_POWERON_PLAY,id,key);
#endif
#if PVR_STANDBY_TIP_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_PVR_END_STANDBY,id,key);
#endif
#if NO_SIGNAL_ALERT_TIP_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_NO_SIGNAL_ALERT_TIP,id,key);
#endif
#if HDMI_CEC_SUPPORT
	CALL_API_SETTING_KEYPRESS(APPUI_SETTING_CEC,id,key);
#endif

	printf("keypress can not find, id = %d \n",id);

	return 0 ;
}

char* gxapp_exapi_mainmenu_get_content(int id)
{
	CALL_API_SETTING_CONTENT(APPUI_SETTING_SORT,id);
#if LCN_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_LCN,id);
#endif
#if AUTO_UPDATE_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_ACU,id);
#endif
	CALL_API_SETTING_CONTENT(APPUI_SETTING_VOLUM_SCOPE,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_ASPECT_RATIO,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_RESOLUTION,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_TV_FORMAT,id);
#ifdef SUPPORT_SCART
	CALL_API_SETTING_CONTENT(APPUI_SETTING_SCART,id);
#endif
#if HIGH_DYNAMIC_RANGE_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_HDR_MODE,id);
#endif
#if DEMOD_DVB_T || DEMOD_ISDBT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_SEARCH_MODE,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_FTA_ONLY,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_COUNTRY,id);
#endif
#if DEMOD_DVB_T
	CALL_API_SETTING_CONTENT(APPUI_SETTING_ANT_POWER,id);
#endif
#if AUTO_STANDBY_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_SLEEP,id);
#endif
#if POWER_ON_CONTROL_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_POWERONCONTROL,id);
#endif
	CALL_API_SETTING_CONTENT(APPUI_SETTING_OSD_LAN,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_EPG_LAN,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_SUB_LAN,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_HOH_SUB,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_TTX_LAN,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_FIRST_AUDIO_LAN,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_SECOND_AUDIO_LAN,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_DIGITAL_AUDIO,id);
	CALL_API_SETTING_CONTENT(APPUI_SETTING_AUDIO_REME,id);
#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_NO_SIGNAL_AUTO_STANDBY,id);
#endif
#if AUDIO_DESCRIPTOR_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_AUDIO_DESC,id);
#endif
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_AD_VOL_OFFSET,id);
#endif
#if IPTV_LITE_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_POWERON_PLAY,id);
#endif
#if PVR_STANDBY_TIP_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_PVR_END_STANDBY,id);
#endif
#if NO_SIGNAL_ALERT_TIP_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_NO_SIGNAL_ALERT_TIP,id);
#endif
#if HDMI_CEC_SUPPORT
	CALL_API_SETTING_CONTENT(APPUI_SETTING_CEC,id);
#endif

	printf("content can not find, id = %d \n",id);
	return NULL ;
}

#endif

