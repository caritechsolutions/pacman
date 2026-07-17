#ifndef __CASCAM_UI_PORTING_H__
#define __CASCAM_UI_PORTING_H__

#include "gxtype.h"
#define CASCAM_UI_MULTI_LAYER 1

#define FONT_NAME_LEN		7
#define SMALL_FONT_TYPE		"font_16"
#define BIG_FONT_TYPE		"font_26"
#define NORMAL_FONT_TYPE	"font_22"
#define FONT_TYPE_14	"font_14"
#define FONT_TYPE_16	"font_16"
#define FONT_TYPE_18	"font_18"
#define FONT_TYPE_20	"font_20"
#define FONT_TYPE_22	"font_22"
#define FONT_TYPE_24	"font_24"
#define FONT_TYPE_26	"font_26"
#define FONT_TYPE_28	"font_28"
#define FONT_TYPE_30	"font_30"
#define FONT_TYPE_36	"font_36"
#define FONT_TYPE_42	"font_42"
#define FONT_TYPE_46	"font_46"

void cascam_ui_get_timezone(double* p_time_zone);
void cascam_ui_exit_to_fullscreen(void);
void cascam_ui_program_play_by_pos(uint32_t pos);
void cascam_ui_program_play(void);
void cascam_ui_program_stop(void);
void cascam_ui_watch_limit_set_tp(void);

void cascam_ui_ippv_notify_before_create_dialog(void);
void cascam_ui_cur_ecm_pid_notify(void* para);
uint8_t cascam_ui_check_cur_ecmpid(uint16_t ecm_pid);
int32_t cascam_ui_check_notice_hide(void);
int32_t cascam_ui_check_notice_hide_by_video(void);
int32_t cascam_ui_check_notice_hide_by_video_use_gui(void);
int32_t cascam_ui_stop_descramble_check(void);
int32_t cascam_ui_check_popmsg_create_postion(uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h);
int32_t cascam_ui_check_osd_rolling_create_postion(uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h);
int32_t cascam_ui_check_super_finger_hide(void);
int32_t cascam_ui_check_super_osd_hide(void);
void cascam_ui_action_research(void);
void cascam_ui_action_show_system_info(void);
void cascam_ui_action_factory_reset(void);
void cascam_ui_action_reset(void);
void cascam_ui_action_request(uint16_t uri_value);
void cascam_ui_get_oem_hardware_version(uint32_t *h_version);
void cascam_ui_get_oem_software_version(uint32_t *s_version);
uint16_t cascam_ui_get_cur_serviceid(void);
uint16_t cascam_ui_get_play_state(void);
uint16_t cascam_ui_check_pvr_status(void);
void cascam_ui_stop_pvr(void);
void cascam_ui_get_app_password(char *buf);
//this function used for gui thread
int32_t cascam_ui_check_video_window(void);
int32_t cascam_ui_check_movie_view_window(void);
void cascam_ui_send_event_to_focus_win(GUI_Event *event);
time_t cascam_ui_get_tick_time(void);
void cascam_ui_set_pvr_type(uint8_t type);
uint8_t cascam_ui_get_playback_type(void);
int32_t cascam_ui_pause_resume_function(uint8_t pause);
int32_t cascam_ui_playback_seek_function(int64_t start_time_ms);
int32_t cascam_ui_playback_stop_function(void);
int32_t cascam_ui_show_video(void);
int32_t cascam_ui_hide_video(void);
#endif
