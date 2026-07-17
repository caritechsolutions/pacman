#ifndef __APP_H__
#define __APP_H__

#include <gxtype.h>
#include "gui_core.h"
#include "gui_timer.h"
#include "app_key.h"
#include "module/player/gxplayer_module.h"
#include "gxservices.h"
#include "common/gxoem.h"

#include "app_config.h"
#include "app_send_msg.h"

#include "pmp_setting.h"
#include "pmp_tags.h"
#include "pmp_explorer.h"
#include "pmp_spectrum.h"
#include "pmp_subtitle.h"
#include "pmp_lyrics.h"
#include "pmp_id3.h"

#include "file_view.h"
#include "file_edit.h"
#include "play_manage.h"
#include "play_pic.h"
#include "play_movie.h"
#include "play_music.h"
#include "play_text.h"
#include "app_open_file.h"
#include "app_bsp.h"
#include "app_str_id.h"
#include "app_data_type.h"
#include "app_pop.h"
#include "app_utility.h"

#include "app_audio.h"
#include "app_misc.h"
#include "app_default_params.h"
#include "module/app_log.h"
#include "app_main_menu_theme_type.h"

#include "app_fuse_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PMP_ATTACH_DVB

#ifdef PMP_ATTACH_DVB

#define APP_Printf_Blue(...)	{app_log_info("\033[036m");\
					app_log_info(__VA_ARGS__);\
					app_log_info("\033[0m");}
#define APP_Printf(...)	app_log_info(__VA_ARGS__)

#define APP_FREE(x)	if(x){GxCore_Free(x);x=NULL;}

#define APP_CHECK_P(p, r) if(NULL == p){\
							app_log_info("\033[033m");\
							app_log_info("\n[%s: %d] %s\n", __FILE__, __LINE__, __FUNCTION__);\
							app_log_info("%s is NULL\n", #p);\
							app_log_info("\033[0m");\
							return r;}

#define APP_TIMER_ADD(timer, fun, timeout, flag) \
							if(timer){\
								reset_timer(timer);\
							 }else{\
								timer = create_timer(fun, timeout, 0, flag);}

#define APP_TIMER_REMOVE(x)	if(x){remove_timer(x);x=NULL;}

#endif

status_t app_block_msg_init(handle_t self);
status_t app_block_msg_destroy(handle_t self);
status_t app_msg_init(handle_t self);
status_t app_msg_destroy(handle_t self);
status_t app_init(void);
status_t app_root(void);

#define EPG_NAME_LEN 20

typedef struct
{
	uint32_t prog_id;
	time_t  event_time;
	uint8_t epgName[EPG_NAME_LEN];
}BookProgEvent;

typedef enum
{
	STATUS_SCRAMBLE,
	STATUS_LOCK,
	STATUS_NO_SIGNAL,
	STATUS_NO_PROGRAM,
	STATUS_NORMAL,
	STATUS_RECORD
}StbStatus;

#ifdef __cplusplus
}
#endif

#define GET_ARRAY_LEN(array,type,len) {len = (sizeof(array)/sizeof(type));}

//Don't support this in Ecos
#define FILE_EDIT_VALID
#define MEDIA_FILE_EDIT_UNVALID
#define DISK_FORMAT_UNVALID

#ifdef ECOS_OS
#define FILE_EDIT_VALID
#undef DISK_FORMAT_UNVALID
#endif


typedef enum
{
	WND_EXEC = 0,
	WND_CANCLE,
	WND_OK
}WndStatus;

typedef bool (*CheckCb)(void);

enum
{
	SERVICE_PLAYER = 0,
	SERVICE_FRONTEND,
	SERVICE_APP_DEAMON,
	SERVICE_SI,
	SERVICE_EXTRA,
	SERVICE_EPG,
	SERVICE_GUI_VIEW,
	SERVICE_SEARCH,
	SERVICE_BOOK,
#if NETWORK_SUPPORT
	SERVICE_NETWORK_DEAMON,
	SERVICE_NETTIME,
#endif
	SERVICE_HOTPLUG,
#ifdef ECOS_OS
#if WIFI_SUPPORT
	SERVICE_USBWIFI_HOTPLUG,
#endif
#if MOD_3G_SUPPORT
	SERVICE_USB3G_HOTPLUG,
#endif
#if ETH_SUPPORT
	SERVICE_WIRED_ETH_HOTPLUG,
#endif
#if USB_ETH_SUPPORT
	SERVICE_USBETH_HOTPLUG,
#endif
#if VPN_SUPPORT
    SERVICE_VPN_STATUS,
#endif
#if USB_TO_SERIAL_SUPPORT
    USBSERIAL_HOTPLUG_SERVICE,
#endif
#if BLUETOOTH_SUPPORT
    SERVICE_USBBT_HOTPLUG,
#endif
#if USB_MICROPHONE_SUPPORT
    SERVICE_VOICE_UAC,
#endif
#endif
	SERVICE_UPDATE,
#if (DEMOD_DVB_S)
	SERVICE_BLIND_SEARCH,
#endif
#if TKGS_SUPPORT
	SERVICE_TKGS,
#endif
#if AUTO_TEST_ENABLE
	SERVICE_AUTO_TEST,
#endif
	SERVICE_HDMI_HOTPLUG,
#if SAT2IP_SERVER_SUPPORT
	SERVICE_SATIP,
#endif
#if HDMI_CEC_SUPPORT
	SERVICE_HDMI_CEC,
#endif
	SERVICE_TOTAL
};
typedef char ubyte32[32];
typedef char ubyte64[64];
typedef unsigned char xmlChar;


extern handle_t g_app_msg_self;
extern GxServiceClass g_service_list[SERVICE_TOTAL];
extern bool app_system_set_callback_handler(CheckCb check_cb, ExitCb exit_cb);
extern PlayerProgInfo* app_player_prog_info_get(void);
extern void app_reboot(void);
#endif /* __APP_H__ */

