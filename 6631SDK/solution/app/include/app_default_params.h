/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_default_params.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	            shenbin         creation
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __GXAPP_DEFAULT_PARAMS__
#define __GXAPP_DEFAULT_PARAMS__

/* Includes --------------------------------------------------------------- */
#include "app_config.h"

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif
#include "gui_core.h"
#include "app_key.h"
#include "module/config/gxconfig.h"
/* Exported Constants ----------------------------------------------------- */

//1 KEY
// first power on
#define POWER_ON_KEY		"power>first"

#if TUNER_AUTO_ADAPT_SUPPORT
// Tuner Adapt
#define FE_TUNER1_ADAPT  "fe>tuner1_adapt"
#define FE_TUNER2_ADAPT  "fe>tuner2_adapt"
#define FE_TUNER3_ADAPT  "fe>tuner3_adapt"
#define FE_TUNER4_ADAPT  "fe>tuner4_adapt"
#define FE_TUNER_ADAPT_INVALID_INDEX  0xFF

// Tuner Address Adapt
#define FE_TUNER1_ADDR_ADAPT  "fe>tuner1_addr_adapt"
#define FE_TUNER2_ADDR_ADAPT  "fe>tuner2_addr_adapt"
#define FE_TUNER3_ADDR_ADAPT  "fe>tuner3_addr_adapt"
#define FE_TUNER4_ADDR_ADAPT  "fe>tuner4_addr_adapt"
#define FE_TUNER_ADDR_ADAPT_INVALID_INDEX  0xFF
#endif

#if DEMOD_AUTO_ADAPT_SUPPORT
// Demod Adapt
#define FE_DEMOD1_ADAPT  "fe>demod1_adapt"
#define FE_DEMOD2_ADAPT  "fe>demod2_adapt"
#define FE_DEMOD3_ADAPT  "fe>demod3_adapt"
#define FE_DEMOD4_ADAPT  "fe>demod4_adapt"
#define FE_DEMOD_ADAPT_INVALID_INDEX  0xFF

// Demod Address Adapt
#define FE_DEMOD1_ADDR_ADAPT  "fe>demod1_addr_adapt"
#define FE_DEMOD2_ADDR_ADAPT  "fe>demod2_addr_adapt"
#define FE_DEMOD3_ADDR_ADAPT  "fe>demod3_addr_adapt"
#define FE_DEMOD4_ADDR_ADAPT  "fe>demod4_addr_adapt"
#define FE_DEMOD_ADDR_ADAPT_INVALID_INDEX  0xFF

#endif

// Color Setting
#define BRIGHTNESS_KEY	"av>video_brightness"
#define CONTRAST_KEY	"av>video_contrast"
#define SATURATION_KEY	"av>video_saturation"
#define SHARPNESS_KEY   "av>video_sharpness"
#define HUE_KEY         "av>video_hue"

#define VIDEO_DELAY_TIME "netaudio>delaytime"
#define NET_AUDIO_SER_KEY "netaudio>service"
#define NET_AUDIO_SER_VALUE "http://192.168.31.138:8000/iptv_1.xml"
#define VIDEO_DELAY_TIME_VALUE 0

//Subtitle
#define SUBT_WIDTH_MAX  APP_XRES
#define SUBT_HEIGHT_MAX APP_YRES
// Av Setting
#define AUDIO_VOLUME_SCOPE	"av>audio_scope"
#define APP_VIDEO_EXTRA_FB_NUM PLAYER_CONFIG_VIDEO_EXTRA_FB
#define AUDIO_VOLUME_KEY	PLAYER_CONFIG_AUDIO_VOLUME
#define USB_AUDIO_VOLUME_KEY	"av>usb_volume"
// audio show different with volume to player, exchange will cause missmatch, so sotre
#define AUDIO_MAP_VAL_KEY	"av>audio_map"
#define SOUND_MODE_KEY		PLAYER_CONFIG_AUDIO_TRACK
#define VOLUME_MODE_KEY	"av>volume_scope"
#define AUDIO_LEVEL_KEY		"av>audio_level"
#define MUTE_KEY			"av>mute"
/* VideoOutputInterface */
#define	ASPECT_MODE_KEY		 	PLAYER_CONFIG_VIDEO_ASPECT
#define	VIDEO_INTERFACE_KEY	    PLAYER_CONFIG_VIDEO_INTERFACE
#define	TV_STANDARD_KEY		    "av>video_standard"
//#define	TV_STANDARD1_KEY		    PLAYER_CONFIG_VIDEO_RESOLUTION1
#define NTSC_TYPE_KEY          PLAYER_CONFIG_VIDEO_AUTO_NTSC
#define	SPDIF_OUTPUT_KEY		PLAYER_CONFIG_AUDIO_SPDIF_SOURCE
#define TV_SPDIF_KEY    PLAYER_CONFIG_AUDIO_AC3_MODE
#define SPDIF_CONF_KEY       "av>spdif_config"

// delay parameters
#define PP_OUTPUT_MPEG2_TIME 	PLAYER_CONFIG_VIDEO_MPEG2_COVER_FRAME
#define PP_OUTPUT_H264_TIME     PLAYER_CONFIG_VIDEO_H264_COVER_FRAME

#define SCART_CONF_KEY       "av>scart_config"
#define TV_SCART_KEY         "av>scart"
#define TV_SCART_SCREEN			PLAYER_CONFIG_VIDEO_SCART_SCREEN1

#if AUDIO_DESCRIPTOR_SUPPORT
#define AD_CONF_KEY       	"av>ad_config"
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
#define AD_VOL_OFFSET_KEY       	"av>ad_vol"
#endif
#endif
#define AV_VIDEO_OUT_MODE "av>videoout_mode"
#define VIDEO_OUT_MODE_DEFAULT_VALUE  1 //0：HDMI and CVBS 1：HDMI or CVBS
#define TV_RATIO_KEY		    PLAYER_CONFIG_VIDEO_DISPLAY_SCREEN
#define TV_ADAPT_KEY	    PLAYER_CONFIG_VIDEO_AUTO_ADAPT

#define ADVANCED_AUDIO_REMEMBER_KEY "advanced>adreme_mode"
#define ADVANCED_AUDIO_REMEMBER_VALUE 0 // 0: off 1: on

#if PLAYBACK_SPEED_SUPPORT
#define AUDIO_SPEED_MODE_KEY "advanced>audio_speedmode"
#define AUDIO_SPEED_MODE_VALUE 0 //0: normal 1: advanced
#endif

// OSD Setting
#define	OSD_LANG_KEY	"osd>osd_lang"
#define	TRANS_LEVEL_KEY	"osd>trans_level"
#define	INFOBAR_TIMEOUT_KEY		"osd>infobar_timeout"
#define	VOLUMEBAR_TIMEOUT_KEY	"osd>volumebar_timeout"

// Language Setting
//#define	MENU_LANG_KEY	"lang>osd_lang" equal OSD_LANG_KEY
#define	AUDIO_1ST_KEY	"lang>audio_1st"
#define	AUDIO_2ND_KEY	"lang>audio_2nd"
#define SUBTILE_1ST_KEY "lang>sub_1st"
#define SUBTILE_2ND_KEY "lang>sub_2nd"
#define	EPG_LANG_KEY	"lang>epg_lang"
#define	TTX_LANG_KEY	"lang>ttx_lang"
//priority sound track
#define PRIORITY_LANG_KEY GXBUS_SEARCH_LANG_PRIORITY

// Lock Setting
#define MENU_LOCK_KEY	"lock>menu_lock"
#define	CHANNEL_LOCK_KEY	"lock>channel_lock"
#define PASSWORD_VERIFY "passwd>verify"
#define	PASSWORD_KEY0	"lock>password0"
#define	PASSWORD_KEY1	"lock>password1"
#define	PASSWORD_KEY2	"lock>password2"
#define	PASSWORD_KEY3	"lock>password3"
#if LONG_PASSWORD_SUPPORT
#define	PASSWORD_KEY4   "lock>password4"
#define	PASSWORD_KEY5   "lock>password5"
#endif
#define MENU_BOOT_KEY	"lock>boot_lock"
#define MENU_STAND3HOURS_KEY	"lock>stand3hours_lock"
#define CHED_SORTMODE_KEY "ched>sortmode"
#define CHED_SORTMODE_KEY_VALUE 0xff

#if BOOT_AD_SUPPORT
#define WIDGET_TEXT_VIDEO_AD_KEY	"ad>ad_video"
#define WIDGET_TEXT_VIDEO_AD_DEFAULT_KEY  1  // 0:Off, 1:On
#define WIDGET_TEXT_LOGO_AD_KEY	"ad>ad_logo"
#define WIDGET_TEXT_LOGO_AD_DEFAULT_KEY  1  // 0:Off, 1:On
#define AD_MAX_COUNT  (4)
#define AD_IMAGE_PATH_LEN (128)
#define AD_GX_VIDEO_DEFAULT_TIMEOUT   (9000)
#define AD_GX_LOGO_DEFAULT_TIMEOUT   (2000)
#define AD_GX_IMAGE_PATH_PREFIX  "/dvb/theme/image/ad/gxlogo_"
#define AD_GX_VIDEO_PATH  "/dvb/theme/image/ad/gx_video_ad.mp4"
#define AD_PLAYER  "player4"
#endif
// Local Coordinate
#define LONGITUDE_KEY		"coordinate>longitude"
#define	LONGITUDE_DIRECT_KEY	"coordinate>longitude_direct"
#define	LATITUDE_KEY	"coordinate>latitude"
#define	LATITUDE_DIRECT_KEY	"coordinate>latitude_direct"

//time setting
#define TIME_DISP       "time>disp"
#define TIME_MODE	    "time>mode"
#define TIME_HALF_ZONE	"time>halfzone"
#define TIME_ZONE	    "time>zone"
#define TIME_SUMMER     "time>summer"
#define TIME_HALF_ZONE_VALUE	    0
//panel setting
#if (PANEL_SETTING_SUPPORT)
#define PANEL_LED_BRIGHTNESS_KEY    "panel>led_brightness"
#define PANEL_LED_DISPLAY_KEY       "panel>led_display"
#define PANEL_LED_BRIGHTNESS        1   /*0 low 1 mid 2 hig*/
#define PANEL_LED_DISPLAY           1   /*0 show channel No. 1 show time*/
#endif
#if STANDBY_SHOW_TIME_SHPPORT
#define PANEL_LED_STANDBY_TIME_KEY       "panel>led_standby"
#define PANEL_STANDBY_TIME           1   /*0 standby show off 1 standby show time*/
#endif

//LOG SET
#define LOG_OUT_PUT_TYPE   "log>out_put_type"
#define LOG_DISK_SELECT   "log>disk_select"
// PVR Control
#define PVR_PLAYER_TMS_KEY  PLAYER_CONFIG_PVR_TIME_SHIFT_FLAG
#define PVR_TIMESHIFT_KEY   "pvr>tms_flag"
#define PVR_FILE_SIZE_KEY   PLAYER_CONFIG_PVR_RECORD_VOLUME_SIZE
#define PVR_DURATION_KEY    "pvr>duration"
#define PVR_PARTITION_KEY   "pvr>partition"
#define PVR_TRUE_DIR_NAME_KEY   "pvr>true_dir_name"
#define PVR_SECTIONRECORD_KEY   "pvr>section_record"
#define PVR_FORCE_HWTS_KEY   PLAYER_CONFIG_PVR_PLAYBACK_FORCE_HWTS
#define PLAYER_PVR_RECORD_EXIST_OVERLAY  PLAYER_CONFIG_PVR_RECORD_EXIST_OVERLAY

//EPG info clean control
#define EPG_INFO_CLEAN_KEY "epg_info>clean"
#define EPG_INFO_CLEAN_SAT_CHANGE 0
#define EPG_INFO_CLEAN_TP_CHANGE  1
#define EPG_INFO_NO_CLEAN         2


/*subtitle*/
#define SUBTITLE_HOH_KEY    "subtitle>hoh"

//nit
#define SEARCH_NIT_LOOP_MODE		GXBUS_SEARCH_NIT_LOOP
#define APP_SEARCH_NIT_LOOP_YES 	GXBUS_SEARCH_NIT_LOOP_YES ///<nit搜索时进行tp的循环搜索
#define APP_SEARCH_NIT_LOOP_NO 	GXBUS_SEARCH_NIT_LOOP_NO  ///<nit搜索时不进行tp的循环搜索

//dts
#define APP_SEARCH_DTS_SUPPORT  GXBUS_SEARCH_DTS_SUPPORT
#define APP_GXBUS_SEARCH_DTS_NO GXBUS_SEARCH_DTS_NO
//SI engine
#define APP_SI_SMALL_BUF_SIZE  GXBUS_SI_SMALL_BUF_SIZE //"gxsi_small_buf_size"
#define APP_SI_SMALL_BUF_COUNT GXBUS_SI_SMALL_BUF_COUNT //"gxsi_small_buf_count"
#define APP_SI_PER_READ_SIZE_DEF   8092 //bus默认是64k

// 方案确认不使用SI ENGINE的EIT标准过滤模式，使用如下的参数可以优化内存使用，一般小内存模式下推荐使用
// SI ENGINE1.9.8-8版本默认参数是SIZE=20kbytes，NUM=80
#define APP_SI_SMALL_BUF_SIZE_DEF  4096 // 考虑到私有数据section模式可能会到4096bytes
#define APP_SI_SMALL_BUF_NUM 32

#if EX_SHIFT_SUPPORT
#define EXSHIFT_KEY		"ca>exshift_enable"
// second
#define EXSHIFT_TIME		"ca>exshift time"
#define EXSHIFT_DEFAULT			0
#define EXSHIFT_TIME_DEFAULT			0
#endif

#define	UART_BAUD_KEY		"uart>baud_rate_limit"
// 1: UPPER LIMIT; 0:LOWER LIMIT
#define	UART_BAUD_RATE_LIMIT	0

#define TUNER_CONFIG_SET    "tuner>config_set"

//freeze
#define FREEZE_SWITCH	PLAYER_CONFIG_VIDEO_FREEZE_SWITCH

//bus lock circle
#define FRONTEND_WATCH_TIME_KEY FRONTEND_CONFIG_WATCH_TIME

// search mode
#define SEARCH_MODE_KEY "search>mode"

// player for play file, the MAX memery pole
#define PLAY_RESERVED_MEMSIZE	PLAYER_CONFIG_PLAY_RESERVED_MEMSIZE

// pp mode control, if the video size is larger than the default size, the PP mode is closed.
#define PLAY_PP_MODE_MAX_WIDTH	PLAYER_CONFIG_VIDEO_PP_MAX_WIDTH
#define PLAY_PP_MODE_MAX_HEIGHT	PLAYER_CONFIG_VIDEO_PP_MAX_HEIGHT
#define DEFAULT_PP_SUPPORT_WIDTH    1920
#define DEFAULT_PP_SUPPORT_HEIGHT   1088

#define ALL_TV_KEY      "prog_id>tv"
#define ALL_TV_ID      0
#define ALL_RADIO_KEY   "prog_id>radio"
#define ALL_RADIO_ID   0
#define HD_TV_KEY       "hd_id>tv"
#define HD_TV_ID   0

#define FAV_INIT_KEY		"fav>rename"

#if (DEMOD_DVB_T || DEMOD_ISDBT)
#define T2_COUNTRY          "osd>t2_country"
#define T2_COUNTRY_VALUE        0
#define T2_ANTENNA_POWER_KEY        "search>power"
#define T2_ANTENNA_POWER_VALUE      0//1: ON 0: OFF
#define DVBT2_ANT_POWER               1
#else
#define DVBT2_ANT_POWER               0
#endif

/*POWER_ON_CONTROL_SUPPORT*/
#if POWER_ON_CONTROL_SUPPORT
#define POWER_STANDBY_KEY       "power>standbyflag"
#define POWER_CONTROL_KEY       "power>ctrlflag"
#define POWER_STANDBY       1    /*0.STANDBY 1 POWERON*/
#define POWER_CONTROL       2    /*0.STANDBY 1 last state 2 POWRE ON*/
#endif

#if DEMOD_DVB_C || DEMOD_DVB_T || DEMOD_ISDBT
#define SEARCH_MODE_TYPE_KEY        "search>modetype"
#define SEARCH_MODE_TYPE_VALUE      1         /*//1 T2  0 C*/
#endif

#if (DVBC_TUNE_AUTO_SUPPORT > 0)
#define DVBC_TUNE_MODE FE_DVBC_TUNE_QAM_SYM_AUTO
#else
#define DVBC_TUNE_MODE FE_DVBC_TUNE_MANUAL
#endif


#define MAIN_FREQ_KEY  "search>main_frequency"
#define MAIN_FREQ_VALUE  299000
#define MAIN_FREQ_SYM_KEY  "search>symbol_rate"
#define MAIN_FREQ_SYM_VALUE  6875
#define MAIN_FREQ_QAM_KEY  "search>qam"
#define MAIN_FREQ_QAM_VALUE  2
#define MAIN_FREQ_BANDWIDTH_KEY  "search>bandwidth"
#define MAIN_FREQ_BANDWIDTH_VALUE  BANDWIDTH_8_MHZ
#define MAIN_FREQ_NITVERSION	"MainFreNitVersion"
#define MAIN_FREQ_NITVERSION_DV 32
#define SEARCH_FTA_CAS_KEY          "search>fta_cas"
#define SEARCH_FTA_CAS_VALUE        3/** 1 :FTA 3 ALL**/

/*media outside subtile*/
#define MEDIA_SUBTILE_LANGUAGE "media->sublang"
#define NETAPPS_SUBTILE_LANGUAGE "netapps->sublang"
/*media music id3 info */
#define MUSIC_ID3_LANG "music->id3lang"
#define MUSIC_ID3_CP "music->id3codepage"
#define MUSIC_ID3_CHARSET "music->id3charset"
#define MUSIC_ID3_CHARSET_VALUE   2  /*0: ISO8859          1 :windows 125x 2:----utf8*/
#define MUSIC_ID3_CP_VALUE         0

//1 VALUE
// first power on
#define POWER_ON		0    /*0.not power on/1.already power on*/

// Color Setting
#define BRIGHTNESS	50
#define CONTRAST	50
#define SATURATION	50
#define SHARPNESS   5
#define HUE         50

// Av Setting
#define AUDIO_SCOPE		1	/*0.channel/1.global*/
#define AUDIO_VOLUME	51	/*0~100*/
#define AUDIO_STEP      4
#define SOUND_MODE      3	/*0.mono/1.L/2.R/3.Stereo*/
#define VOLUME_SCOMPE_      0	/*0.global/1.channelo*/
#define VOLUME_CHANNEL      1	/*1.channelo*/
#define VOLUME_SMART        2	/*2.smart*/
#define AUDIO_LEVEL		1	/*0.Low/1.Mid/2.High*/
#define MUTE			0	/*0.unMute/1.mute*/
#define	ASPECT_MODE		0 	/*0.atuo/1.Letter Box/2.Pan & scan*/
#define VIDEO_MOSAIC_GATE		100
// VIDEO OUTPUT DELAY PARA
#define MPEG_AV_DELAY_FRAME		6
#define H264_AV_DELAY_FRAME		1
#define SMART_VOLUME_LEVEL	"sm>volume_level"
#define SMART_VOLUME_TIMES	"sm>times"
#define SMART_GAIN_LEVEL    (5)
#define SMART_GAIN_TIME     15

#define	VIDEO_INTERFACE_SCART GX_VIDEO_OUTPUT_HDMI|GX_VIDEO_OUTPUT_RCA|GX_VIDEO_OUTPUT_SCART
#define	VIDEO_INTERFACE_MODE GX_VIDEO_OUTPUT_HDMI|GX_VIDEO_OUTPUT_RCA
// not support the "VIDEO_OUTPUT_HDMI_1080P_50HZ" and "VIDEO_OUTPUT_HDMI_1080P_60HZ"
#define	VIDEO_INTERFACE GX_VIDEO_OUTPUT_HDMI | GX_VIDEO_OUTPUT_RCA
#define	TV_STANDARD	    GX_VIDEO_OUTPUT_1080I_50HZ
#define NTSC_TYPE      GX_VIDEO_OUTPUT_NTSC_M //VIDEO_OUTPUT_NTSC_M, VIDEO_OUTPUT_NTSC_443

#define	SPDIF_OUTPUT	3	/*0.PCM/1.ac3 / 2.EAC3 /3.Auto*/
#define	AC3_BYPASS		3	/*1.BYPASS; 0.not bypass*/

#define	RF_MODE			0	/*0.PAL BG/1.PAL I/2.PAL DK/3.NTSC M*/
#define TV_RATIO		1   /*0.4:3/1.16:9*/
#define TV_ADAPT_AUTO 	1  /*0 don't auto 1 auto*/
#define TV_ADAPT_NOAUTO 	0  /*0 don't auto*/
#define TV_SCART        1  //  0 CVBS, 1 RGB
#define SCART_CONF      0 //0 off; 1 on
#define SPDIF_CONF      0 //0 off; 1 on

#if AUDIO_DESCRIPTOR_SUPPORT
#define	AD_CONFIG		0	/*1.enable; 0.disable*/
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
#define AD_VOL_OFFSET_VALUE       	0  /* -3  -2  -1  0 1 2 3*/
#endif
#endif
#define FAV_INIT_VALUE 0

//3: 0-25 Ó³Éä³É0-75 4: 0-25 Ó³Éä³É0-100
#define VOLUME_NUM (4)

#if HDMI_CEC_SUPPORT
#define CEC_CONF_KEY        "av>cec_config"
#define CEC_CONFIG_VALUE       1  /*1.enable; 0.disable*/
#define CEC_USE_TV_REMOTE_CONTROL_KEY        "av>cec_use_tv_rcu"
#define CEC_USE_TV_REMOTE_CONTROL_VALUE       1  /*1.enable; 0.disable*/
#define CEC_TV_ON_STB_ACTIVITY_KEY        "av>cec_tv_on_stb_activity"
#define CEC_TV_ON_STB_ACTIVITY_VALUE       1  /*1.enable; 0.disable*/
#define CEC_STB_STANDBY_TV_ACTIVITY_KEY        "av>cec_stb_standby_tv_activityg"
#define CEC_STB_STANDBY_TV_ACTIVITY_VALUE       1  /*1.enable; 0.disable*/
#define CEC_SYSTEM_AUDIO_KEY        "av>cec_system_audio"
#define CEC_SYSTEM_AUDIO_VALUE       1  /*1.enable; 0.disable*/
#endif

// OSD Setting
#define OSD_TRANS_100PERCENT 255
#define OSD_TRANS_30PERCENT 78*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_40PERCENT 102*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_50PERCENT 128*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_60PERCENT 156*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_70PERCENT 178*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_80PERCENT 204*OSD_TRANS_100PERCENT/255
#define OSD_TRANS_90PERCENT 230*OSD_TRANS_100PERCENT/255

#define OSD_LANG 0
#define AUDIO1_LANG   0
#define AUDIO2_LANG   0
#define SUB_LANG   0
#define EPG_LANG   0
#define TTX_LANG   0
#define	TRANS_LEVEL	OSD_TRANS_90PERCENT /*0~255*/
#define	INFOBAR_TIMEOUT	3	/*1~8*/
#define	VOLUMEBAR_TIMEOUT	3	/*1~8*/
#define	PVRBAR_TIMEOUT	8

//priority sound track
#if AUDIO_LANG_QAA2OST_SUPPORT
#define PRIORITY_LANG_VALUE "ost_eng" //AUDIO_1ST & AUDIO_2ND
#else
#define PRIORITY_LANG_VALUE "eng_chs" //AUDIO_1ST & AUDIO_2ND
#endif

// Lock Setting
#define MENU_LOCK	    0	/*0.unlock/1.lock*/
#define	CHANNEL_LOCK	1	/*0.unlock/1.lock*/
#if PARENTAL_LOCK_SUPPORT
#define	PARENTAL_GUIDANCE_KEY    "system>parental"
#define PARENTAL_GUIDANCE_VALUE	0   /*0~17, 0:lock all, 17:view all, 1~16: 3years~18years*/
#endif
#define BOOT_LOCK	    0	/*0.unlock/1.lock*/
#define	STAND3HOUR_LOCK	0	/*0.unlock/1.lock*/
#define PASSWD_VERIFY_ERR 0
#define PASSWD_VERIFY_OK  1
#define	PASSWORD_1	('0')	/*KEY_0*/
#define	PASSWORD_2	('0')	/*KEY_0*/
#define	PASSWORD_3	('0')	/*KEY_0*/
#define	PASSWORD_4	('0')	/*KEY_0*/
#if LONG_PASSWORD_SUPPORT
#define	PASSWORD_5      ('0')	/*KEY_0*/
#define	PASSWORD_6      ('0')	/*KEY_0*/
#define GRNERAL_PASSWD  "876543"
#else
#define GRNERAL_PASSWD  "8765"
#endif

// Local Coordinate
#define LONGITUDE	1202	/*-180~180,the real value is LONGITUDE/10.0*/
#define	LONGITUDE_DIRECT	0	/*0.east/1.west*/
#define	LATITUDE	303	/*-90~90,the real value is LATITUDE/10.0*/
#define	LATITUDE_DIRECT	1	/*0.south/1.north*/

//time setting
#define TIME_DISP_VALUE 0   /* 0.ymd/1.dmy */
#define	TIME_MODE_VALUE	0	/*0.GTM/1.LOCAL*/
#define TIME_ZONE_VALUE	0	/*-12 ~ 12*/
#define TIME_SUMMER_VALUE	0	/*0.off/1.on*/
//sleep setting
#if AUTO_STANDBY_SUPPORT
#define TIME_AUTO_STANDBY    "time>auto_standby"
#define TIME_AUTO_STANDBY_VALUE	0	/*0.off/1.on*/
#define TIME_HALF_HOUR_VALUE 0
#define SLEEP_VALUE              0// 0: off other:on
#define SLEEP_KEY                "time>sleep"
#define TIME_HALF_HOUR         "time>half_hour"
#endif

#define T2_CAPTIONAL_KEY            "time>captional"
#define T2_REGION_KEY               "time>region"
#define T2_SORT_FLAG                "program>sort"
#define T2_ACU_KEY                  "program>acu"
#define T2_LCN_VIRTAUL_CNT          "virtaul>lcn"

#define T2_REGION_VALUE             0//default for 0
#define	T2_CAPTIONAL_VALUE          0//g_CaptionalName[]

#define T2_SORT_FLAG_VALUE          2//LCN sort
#define T2_ACU_VALUE                1//0:OFF 1:ON

#define T2_VIRTAUL_CNT              0

#if ((RF_MODULATOR_SUPPORT > 0) && (RF_RT500 > 0))
#define RF_CH_NUMER_KEY			"rf>chNO"
#define RF_SOUND_MODE_KEY		"rf>soundnode"

#define RF_CH_NUMER				21
#define RF_SOUND_MODE			1
#endif
#if ((RF_MODULATOR_SUPPORT > 0) && (RF_RT534 >0))
#define RF_ON_OFF_KEY			"rf>rt534_on_off"
#define RF_MODE_KEY				"rf>rt534_ch3_ch4"

#define RF_POWER_DEFAULT		1//    off
#define RF_MODE_DEFAULT			0//    CH3
#endif

#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
#define NO_SIGNAL_AUTO_STANDBY_KEY "no_sig>standby"
#define NO_SIGNAL_AUTO_STANDBY_VALUE 0 //OFF
#endif

#if LCN_SUPPORT || TKGS_SUPPORT
#define LCN_KEY                  "program>lcn"
#define LCN_VALUE                0//0:OFF 1:ON
#define PROGRAM_LCN_SORT_KEY     "sort>lcn"
#define PROGRAM_LCN_SORT_VALUE   1//0:OFF 1:ON
#endif

#if PVR_STANDBY_TIP_SUPPORT
#define PVR_END_STANDBY_KEY "pvr_end>standby"
#define PVR_END_STANDBY_VALUE 0 //OFF
#endif

#if NO_SIGNAL_ALERT_TIP_SUPPORT
#define NO_SIGNAL_ALERT_TIP_KEY "no_sig>alert"
#define NO_SIGNAL_ALERT_TIP_VALUE 0 //OFF
#endif
// pvr control
#define PVR_TIMESHIFT_FLAG	    0 /*0.time shift off / 1.time shift on*/
#define PVR_SECTIONRECORD_FLAG 0 /*0.section record off / 1.section record on*/
#define PVR_FILE_SIZE_VALUE     2048/*1024 == 1G ,2048 == 2G, 4096 == 4G*/
#define PVR_DURATION_VALUE      0   /* 0-2Hour ...*/
#if QUICK_SWITCH_SUPPORT
#define PVR_FORCE_HWTS_FLAG	    0 /*0.MPEG TS / 1.HW_TS*/
#else
#define PVR_FORCE_HWTS_FLAG	    1 /*0.MPEG TS / 1.HW_TS*/
#endif
#ifdef ECOS_OS
#define PVR_PARTITION          "/dev/usb01"
#endif
#ifdef LINUX_OS
#define PVR_PARTITION          "/dev/sda1"
#define PVR_TRUE_DIR_NAME          "/GxPvr"
#endif
/*subtitle*/
#define SUBTITLE_MODE_VALUE	0	/*0.subtitle mode off 1.dvb subtitle 2.ttx subtitle 3.pause*/
#define SUBTITLE_LANG_VALUE	0	/*0-off 1-N: lang sel*/
#define SUBTITLE_HOH_VALUE 0   /*0-off 1: on*/

/*freeze*/
#define FREEZE_SWITCH_VALUE   1    /*0. unfreeze / 1. freeze*/

#define FRONTEND_WATCH_TIME_VALUE 100
#define FRONTEND_WATCH_TIME_COUNT 40
#define SEARCH_MODE_VALUE   0

#if (DEMOD_DVB_S == 0)
#define SYS_MAX_SAT		(3)
#define SYS_MAX_TP		(400)
#define SYS_MAX_PROG	(800)
#else
#define SYS_MAX_SAT    (100)
#define SYS_MAX_TP     (3000)
#define SYS_MAX_PROG    (5000)
#endif

#ifdef ECOS_OS
#define DEFAULT_DB_PATH	NULL
#else
#define DEFAULT_DB_PATH	"/home/root/default_data.db"
#endif

//for PROGRAM CODE CHARGE
#define SEARCH_PROGRAM_CODE GXBUS_SEARCH_UTF8
#define	SEARCH_PROGRAM_CODE_UTF8 GXBUS_SEARCH_UTF8_YES

//1 CONTENT
#define BRIGHTNESS_0	0
#define BRIGHTNESS_1	10
#define BRIGHTNESS_2	20
#define BRIGHTNESS_3	30
#define BRIGHTNESS_4	40
#define BRIGHTNESS_5	50
#define BRIGHTNESS_6	60
#define BRIGHTNESS_7	70
#define BRIGHTNESS_8	80
#define BRIGHTNESS_9	90
#define BRIGHTNESS_10	100


#define CONTRAST_0	0
#define CONTRAST_1	10
#define CONTRAST_2	20
#define CONTRAST_3	30
#define CONTRAST_4	40
#define CONTRAST_5	50
#define CONTRAST_6	60
#define CONTRAST_7	70
#define CONTRAST_8	80
#define CONTRAST_9	90
#define CONTRAST_10	100


#define SATURATION_0	0
#define SATURATION_1	10
#define SATURATION_2	20
#define SATURATION_3	30
#define SATURATION_4	40
#define SATURATION_5	50
#define SATURATION_6	60
#define SATURATION_7	70
#define SATURATION_8	80
#define SATURATION_9	90
#define SATURATION_10	100



#define AUDIO_TRACK_L	0
#define AUDIO_TRACK_R	1
#define AUDIO_TRACK_STERO	2

#define AUDIO_LEVEL_LOW	0
#define AUDIO_LEVEL_MID	1
#define AUDIO_LEVEL_HIGH	2

#define MUTE_DISABLE  0
#define MUTE_ENABLE  1


#define SCREEN_RATIO_4X3	0
#define SCREEN_RATIO_16X9	1

#define SCREEN_TYPE_AUTO 0
#define SCREEN_TYPE_SCAN  1
#define SCREEN_TYPE_LETTER_BOX  2

#define AV_OUTPUT_SCART_CVBS	0
#define AV_OUTPUT_SCART_RGB	1
#define AV_OUTPUT_S_VIDEO		2

#define SPDIF_OUTPUT_OFF	0
#define SPDIF_OUTPUT_PCM	1
#define SPDIF_OUTPUT_DOLBY_DIGITAL 2


#define	RF_MODE_PAL_BG	0
#define	RF_MODE_PAL_I	1
#define	RF_MODE_PAL_DK	2
#define	RF_MODE_NTSC_M	3
#define MENU_LOCK_DISABLE	0
#define MENU_LOCK_ENABLE	1

#define CHANNEL_LOCK_DISABLE	0
#define CHANNEL_LOCK_ENABLE	1

#define REDIO_RECODE_SUPPORT	1

//NetWork Setting
#define _3G_TYPE_KEY "3g>type"
#define _3G_TYPE_DEFAULT 0  //0-auto 1-manual
#define _3G_AREA_KEY "3g>area"
#define _3G_AREA_DEFAULT 0
#define _3G_OPEA_SEL_KEY "3g>operator_sel"
#define _3G_OPEA_SEL_DEFAULT 0

#ifdef ECOS_OS
#define DEV_WIFI_MODE_KEY "network>wifimode"
#define IP_TYPE_WIFI_KEY  "network>wifiiptype"
#define WIFI_IP_ADDR_KEY  "network>wifiipaddr"
#define WIFI_NET_MASK_KEY "network>wifinetmask"
#define WIFI_GATE_WAY_KEY "network>wifigateway"
#define WIFI_DNS1_KEY     "network>wifidns1"
#define WIFI_DNS2_KEY     "network>wifidns2"

#define DEV_BT_MODE_KEY "network>btmode"
#define BT_STATUS_KEY   "network>btstatus"
#define BT_STATUS_DEFAULT "none"
#define BT_LINK_KEY_MODE "bt>mode"
#define BT_LINK_KEY_TYPE "bt>type"
#define BT_LINK_KEY      "bt>linkkey"
#define BT_LINK_KEY_MAC  "bt>mac"

#define DEV_3G_MODE_KEY "network>3gmode"
#define IP_TYPE_3G_KEY "network>3giptype"
#define USB3G_IP_ADDR_KEY  "network>3gipaddr"
#define USB3G_NET_MASK_KEY "network>3gnetmask"
#define USB3G_GATE_WAY_KEY "network>3ggateway"
#define USB3G_DNS1_KEY     "network>3gdns1"
#define USB3G_DNS2_KEY     "network>3gdns2"

#define DEV_GPRS_MODE_KEY "network>gprsmode"
#define IP_TYPE_GPRS_KEY "network>gprsiptype"
#define GPRS_IP_ADDR_KEY  "network>gprsipaddr"
#define GPRS_NET_MASK_KEY "network>gprsnetmask"
#define GPRS_GATE_WAY_KEY "network>gprsgateway"
#define GPRS_DNS1_KEY     "network>gprsdns1"
#define GPRS_DNS2_KEY     "network>gprsdns2"

#define DEV_WIRED_MODE_KEY "network>wiredmode"
#define IP_TYPE_WIRED_KEY "network>wirediptype"
#define WIRED_IP_ADDR_KEY  "network>wiredipaddr"
#define WIRED_NET_MASK_KEY "network>wirednetmask"
#define WIRED_GATE_WAY_KEY "network>wiredgateway"
#define WIRED_DNS1_KEY     "network>wireddns1"
#define WIRED_DNS2_KEY     "network>wireddns2"

#define RNDIS_MODE_KEY    "network>rndismode"
#define RNDIS_TYPE_KEY    "network>rndisiptype"
#define RNDIS_IP_ADDR_KEY  "network>rndisipaddrkey"
#define RNDIS_NET_MASK_KEY "network>rndisnetmaskkey"
#define RNDIS_GATE_WAY_KEY "network>rndisgateway"
#define RNDIS_DNS1_KEY     "network>rndisdns1key"
#define RNDIS_DNS2_KEY     "network>rndisdns2key"
#define DEV_USBETH_MODE_KEY "network>devusbwiredmodekey"
#define IP_TYPE_USBETH_KEY  "network>iptypeusbnetkey"
#define USBETH_IP_ADDR_KEY  "network>usbnetipaddrkey"
#define USBETH_NET_MASK_KEY "network>usbnetnetmaskkey"
#define USBETH_GATE_WAY_KEY "network>usbnetgatewaykey"
#define USBETH_DNS1_KEY     "network>usbnetdns1key"
#define USBETH_DNS2_KEY     "network>usbnetdns2key"

#define DEV_MODE_DEFALT 1   //1:open 0:close
#define IP_TYPE_DEFALT  1   //1:dhcp 0:static

#define DEV_MODE_DEFALT 1   //1:open 0:close
#define IP_TYPE_DEFALT  1   //1:dhcp 0:static
#define WIFI_IP_ADDR_DEFALT  "0.0.0.0"
#define WIFI_NET_MASK_DEFALT "0.0.0.0"
#define WIFI_GATE_WAY_DEFALT "0.0.0.0"
#define WIFI_DNS1_DEFALT     "0.0.0.0"
#define WIFI_DNS2_DEFALT     "0.0.0.0"
#define USB3G_IP_ADDR_DEFALT  "0.0.0.0"
#define USB3G_NET_MASK_DEFALT "0.0.0.0"
#define USB3G_GATE_WAY_DEFALT "0.0.0.0"
#define USB3G_DNS1_DEFALT     "0.0.0.0"
#define USB3G_DNS2_DEFALT     "0.0.0.0"
#define GPRS_IP_ADDR_DEFALT  "0.0.0.0"
#define GPRS_NET_MASK_DEFALT "0.0.0.0"
#define GPRS_GATE_WAY_DEFALT "0.0.0.0"
#define GPRS_DNS1_DEFALT     "0.0.0.0"
#define GPRS_DNS2_DEFALT     "0.0.0.0"
#define WIRED_IP_ADDR_DEFALT  "0.0.0.0"
#define WIRED_NET_MASK_DEFALT "0.0.0.0"
#define WIRED_GATE_WAY_DEFALT "0.0.0.0"
#define WIRED_DNS1_DEFALT     "0.0.0.0"
#define WIRED_DNS2_DEFALT     "0.0.0.0"
#endif

#define WIFI_IP_ADDR_DEFALT  "0.0.0.0"
#define WIFI_NET_MASK_DEFALT "0.0.0.0"
#define WIFI_GATE_WAY_DEFALT "0.0.0.0"
#define WIFI_DNS1_DEFALT     "0.0.0.0"
#define WIFI_DNS2_DEFALT     "0.0.0.0"
#define USB3G_IP_ADDR_DEFALT  "0.0.0.0"
#define USB3G_NET_MASK_DEFALT "0.0.0.0"
#define USB3G_GATE_WAY_DEFALT "0.0.0.0"
#define USB3G_DNS1_DEFALT     "0.0.0.0"
#define USB3G_DNS2_DEFALT     "0.0.0.0"
#define GPRS_IP_ADDR_DEFALT  "0.0.0.0"
#define GPRS_NET_MASK_DEFALT "0.0.0.0"
#define GPRS_GATE_WAY_DEFALT "0.0.0.0"
#define GPRS_DNS1_DEFALT     "0.0.0.0"
#define GPRS_DNS2_DEFALT     "0.0.0.0"
#define WIRED_IP_ADDR_DEFALT  "0.0.0.0"
#define WIRED_NET_MASK_DEFALT "0.0.0.0"
#define WIRED_GATE_WAY_DEFALT "0.0.0.0"
#define WIRED_DNS1_DEFALT     "0.0.0.0"
#define WIRED_DNS2_DEFALT     "0.0.0.0"
#define USBETH_IP_ADDR_DEFALT  "0.0.0.0"
#define USBETH_NET_MASK_DEFALT "0.0.0.0"
#define USBETH_GATE_WAY_DEFALT "0.0.0.0"
#define USBETH_DNS1_DEFALT     "8.8.8.8"
#define USBETH_DNS2_DEFALT     "208.67.222.222"
#define NETWORK_IP_ADDR_DEFALT  "0.0.0.0"
#define NETWORK_NET_MASK_DEFALT "0.0.0.0"
#define NETWORK_GATE_WAY_DEFALT "0.0.0.0"
#define NETWORK_DNS1_DEFALT     "0.0.0.0"
#define NETWORK_DNS2_DEFALT     "0.0.0.0"

#define AP0_DEV_KEY "network>ap0dev"
#define AP0_SSID_KEY "network>ap0ssid"
#define AP0_MAC_KEY "network>ap0mac"
#define AP0_PSK_KEY "network>ap0psk"
#define AP0_HIDE_KEY "network>ap0hide"

#define AP_DEV_DEFAULT NET_IF_DEV_WIFI0
#define AP_SSID_DEFAULT "NO NAME-"
#define AP_MAC_DEFAULT "00:00:00:00:00:00"
#define AP_PSK_DEFAULT ""
#define AP_HIDE_DEFAULT (0)
#define BT0_DEV_KEY "network>bt0dev"
#define BT0_MAC_KEY "network>bt0mac"
#define BT0_SSID_KEY "network>bt0name"

#define BT_DEV_DEFAULT NET_IF_DEV_USBBT
#define BT_MAC_DEFAULT "00:00:00:00:00:00"

#if NETAPPS_SUPPORT
#define IPTV_PARAM_LEN (128)

#define POWERON_PLAY_KEY "Poweron->play"
#define POWERON_PLAY_VALUE 0 /*0 -----TV 1 ----IPTV*/

#endif

#if WEBAPI_SUPPORT
#define WEBAPI_SWITCH "network>webapimode"
#define WEBAPI_SWITCH_ON 0
#define SUPERCAST_SERVER_NAME "network>supercastservername"
#define SUPERCAST_SERVER_NAME_DEFAULT "gx-webapi-box"
#endif

#if PVR_STANDBY_SUPPORT
#define PVR_STANDBY                 "pvr>standby"
#define PVR_STANDYB_FLAG            0
#endif

#if MINI_MEM_SUPPORT
#define PIC_SHOW_MAX_SIZE (1280*1024*3)
#else
#define PIC_SHOW_MAX_SIZE (3840*2160*3)
#endif

#if FM_SUPPORT
#define FM_STEP "fm>step"
#define FM_STEP_VAL 1
#define FM_PLAY_INDEX "fm>play_index"
#define FM_PLAY_INDEX_VAL 0
#define BAND_KEY           "fm>band"
#define BAND_VALUE         0
#define ANT_GAIN_KEY	   "fm>ant_gain"
#define ANT_GAIN_VALUE     0
#endif

#define MODIFY_FATMOUNT

//The SIZE unit
#define SIZE_UNIT_K    (uint64_t)(1024) //powers of 1024
#define SIZE_UNIT_M    (uint64_t)(SIZE_UNIT_K * SIZE_UNIT_K)
#define SIZE_UNIT_G    (uint64_t)(SIZE_UNIT_M * SIZE_UNIT_K)
#define SIZE_UNIT_T    (uint64_t)(SIZE_UNIT_G * SIZE_UNIT_K)

#define DDR_512M_SIZE (64*SIZE_UNIT_M)
#define DDR_1G_SIZE (128*SIZE_UNIT_M)
#define DDR_2G_SIZE (256*SIZE_UNIT_M)
#ifdef __cplusplus
}
#endif

#if START_CHANNEL_SUPPORT
#define START_CHANNEL_ID	"channel>id"
#define START_CHANNEL_DEFAULT_KEY  0
#endif
#if SHUTDOWN_MODE_SUPPORT
#define WIDGET_TEXT_SHUTDOWN_MODE_KEY	"channel>shutdown_mode"
#define WIDGET_TEXT_SHUTDOWN_MODE_DEFAULT_KEY  0  // 0:Off, 1:On
#endif
#endif /* __GXAPP_DEFAULT_PARAMS__ */
/* End of file -------------------------------------------------------------*/


