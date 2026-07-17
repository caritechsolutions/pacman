#ifndef __CASCAM_UI_COMMON_H__
#define __CASCAM_UI_COMMON_H__

#include "gxtype.h"
#include "cascam.h"

//CASCAM COMMON MSGID
#define CASCAM_MSG_CHECK_WIN     0xffff0001
#define CASCAM_MSG_CHECK_PROMPT  0xffff0002
#define CASCAM_MSG_USE_DEFINE_1  0xffff0010

typedef struct{
    void (* ca_init)(void);
    void (* ca_menu_exec)(void);
    void (* ca_standby)(void);
    void (* ca_start_ca)(void);
    void (* ca_stop_ca)(void);
    void (* ca_stop_descramble)(void);
    void (* ca_switch_freq)(void);
    void (* ca_switch_channel)(CascamSwitchChannel param);
    void (* ca_quick_switch_channel)(CascamQuickSwitchChannel param);
    void (* ca_add_service)(CascamServiceInfo param);
    void (* ca_delete_service)(CascamServiceInfo param);
    void (* ca_destroy_finger)(void);
    void (* ca_destroy_rolling_osd)(void);
    int32_t (* ca_do_event)(CascamOsdEvent* para);
    int32_t (* ca_do_event_layer)(CascamOsdEvent* para);
    bool (* ca_check_pvr_forbid)(void);
    int32_t (* ca_pvr_control)(uint32_t type, void* param);// type: CascamUiPVRCtrlType
    int32_t (* ca_check_force_lock_status)(void);
}cascam_ui_control;

typedef enum {
    CASCAM_UI_UNFORCE = 0,// can switch channel, and ...
    CASCAM_UI_FORCE,// can not support remote and panel.
}CascamUiForceType;

typedef enum {
    CASCAM_UI_PVR_RECORD_START = 0,
    CASCAM_UI_PVR_RECORD_STOP,
    CASCAM_UI_PVR_PLAY_START,
    CASCAM_UI_PVR_PLAY_STOP,
    CASCAM_UI_PVR_SET_STATE,
    CASCAM_UI_PVR_GET_STATE,
    CASCAM_UI_PVR_DO_NOTHING,
}CascamUiPVRCtrlType;

typedef enum {
	CASCAM_WINDOW_WIN,
	CASCAM_WINDOW_PROMPT
}CascamWindowType;

typedef enum{
	CASCAM_UI_LOCK_SERVICE = 0,
	CASCAM_UI_FORCE_OSD,
	CASCAM_UI_FORCE_FINGER,
	CASCAM_UI_WATCH_LIMIT,
	CASCAM_UI_LOCK_STATUS_NUM,
}CascamLockStatus;

typedef enum{
	CASCAM_UI_EMAIL_ICONHIDE = 0,
	CASCAM_UI_EMAIL_NEW,
	CASCAM_UI_EMAIL_SPACE_FULL,
}CascamUiMailNotify;

typedef struct{
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
    char *str;
	unsigned int timeout_sec;
}CascamUiNoticeClass;

typedef struct{
    uint8_t *data;
    uint32_t length;
}CascamBuffer;

//mail notify
void cascam_ui_mail_notify(CascamUiMailNotify* p );

//message
void cascam_ui_init_msg(CascamUiNoticeClass *para);
void cascam_ui_clean_msg(void);
char cascam_ui_is_notice(void);
unsigned char *cascam_ui_get_notice_str(void);
void cascam_ui_create_msg_dialog(CascamUiNoticeClass *para);
void cascam_ui_exit_msg_dialog(void);

event_list* cascam_create_timer(timer_event event, int time, void *data,  TIMER_FLAG flag);
int cascam_reset_timer(event_list *pTimer, int time); //time: <=0 is due to original param, >0 is new time
int cascam_remove_timer(event_list *pTimer);
int cascam_timer_stop(event_list *pTimer);
void cascam_ui_create_msg_dialog_for_app(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void cascam_ui_exit_msg_dialog_for_app(void);

//check gui window/prompt
uint8_t cascam_ui_get_win_flag(void);
void cascam_ui_set_win_flag(uint8_t flag);
int32_t cascam_ui_check_gui_dialog(void* p);
int32_t cascam_ui_check_gui_prompt(void* p);
int32_t cascam_ui_check_window_exist(char* name, CascamWindowType type);
int32_t cascam_ui_send_msg_to_gui(uint32_t sub_id, CascamBuffer* data);
#endif
