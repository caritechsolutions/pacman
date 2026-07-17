#ifndef __APP_SKIN_SETTING_H_202309261514__
#define __APP_SKIN_SETTING_H_202309261514__

#include <stdio.h>

#define SKIN_NO_USB_RETURN    (-11)

#define FULL_XML_PATH_LENGTH   100

#define WND_SKIN                  "wnd_skin"
#define WND_SKPREVIEW             "wnd_skpreview"
#define WND_SKRECOVERY            "wnd_skrecovery"
#define SKIN_UPDATE_BIN_NAME      "download_skin.bin"

// printf define
//#define SKIN_INFO_SUPPROT
#define SKIN_DEBUG_SUPPROT
#define SKIN_ERR_SUPPROT

#ifdef SKIN_INFO_SUPPROT
#define SKIN_INFO(fmt, args...)  do {                         \
    printf(fmt, ##args);                                      \
} while(0)
#else
#define SKIN_INFO(...) do{}while(0)
#endif

#ifdef SKIN_DEBUG_SUPPROT
#define SKIN_DBG(fmt, args...)  do {                         \
    printf(fmt, ##args);                                     \
} while(0)
#else
#define SKIN_DBG(...) do{}while(0)
#endif

#ifdef SKIN_ERR_SUPPROT
#define SKIN_ERR(fmt, args...)  do {                         \
    printf(fmt, ##args);                                     \
} while(0)
#else
#define SKIN_ERR(...) do{}while(0)
#endif

//1 match
int   gxapp_skin_version_is_matched(char *version);

int   gxapp_skin_online_is_disconnect(void);
int   gxapp_skin_online_get_connect_state(void);

int   gxapp_skin_get_usb_entry_path(void);
int   gxapp_skin_check_usb_entry_path(char *path);
int   gxapp_skin_check_usb_device(void);

// for setting menu code
int   gxapp_skin_setting_init(void);
int   gxapp_skin_setting_uninit(void);

char *gxapp_skin_online_get_url(void);
int   gxapp_skin_online_set_url(char *url);

char *gxapp_skin_usb_get_url(void);
int   gxapp_skin_usb_set_url(char *url);
int   gxapp_skin_usb_set_memory_url(char *url);

int   gxapp_skin_check_key_to_debug(unsigned int key);

void  gxapp_skin_app_close_all_before_reboot(void);
void  gxapp_skin_app_api_reboot(void);

void  gxapp_skin_app_reboot(void);

#endif
