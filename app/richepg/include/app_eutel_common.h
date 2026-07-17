#ifndef __APP_EUTEL_COMMON_H__
#define __APP_EUTEL_COMMON_H__

#include "app_config.h"
#include "app_default_params.h"
#include "app_eutel_windows.h"
#if RICHEPG_SUPPORT
#include "app_eutel_control.h"
#include "richepg_epg.h"
#include "app_eutel_porting.h"
#include "app_eutel_sat_fuse.h"

#define AUTO_MAINTENANCE_HOUR           4

#define EUTEL_EPG_EVENT_SIZE            5000
#define EPG_VIEW_DAYS_SET_SUPPORT       0

#define RICHEPG_MODE_KEY                "richepg_mode"
#define RICHEPG_MODE_DEF                1

#define RICHEPG_LINEUP_MODE_KEY         "richepg_lineup_mode"
#define RICHEPG_LINEUP_MODE_DEF         0 // switch of sat when changing lineup: 0, not process ECL/EPG; 1, process them

#define RICHEPG_UPDATE_MODE_KEY         "richepg_update_mode"
#define RICHEPG_UPDATE_MODE_DEF         1 // 0, only maintenance current selected sat; 1, all

#define RICHEPG_UPDATE_ATTACH_KEY       "richepg_update_attach"
#define RICHEPG_UPDATE_ATTACH_DEF       -1 // -1, Off; 12~21. means 12:00~21:00

#define RICHEPG_START_MAINT_KEY         "richepg_start_maint"
#define RICHEPG_START_MAINT_DEF         0

#define RICHEPG_EPG_VIEW_STYLE_KEY      "richepg_epg_view_style"
#define RICHEPG_EPG_VIEW_STYLE_DEF      0 // 0, program view; 1, channel view

#if EPG_VIEW_DAYS_SET_SUPPORT
#define RICHEPG_EPG_VIEW_DAYS_KEY       "richepg_epg_view_days"
#define RICHEPG_EPG_VIEW_DAYS_DEF       3
#define RICHEPG_EPG_VIEW_DAYS_MIN       2
#define RICHEPG_EPG_VIEW_DAYS_MAX       8
#define RICHEPG_EPG_VIEW_DAYS_STR_ARRAY {"2","3","4","5","6","7","8"}
#define RICHEPG_EPG_VIEW_DAYS_CONTENT   "[2,3,4,5,6,7,8]"
#endif

#define RICHEPG_EPG_EVENING_START_KEY   "richepg_epg_evening_start"
#define RICHEPG_EPG_EVENING_START_DEF   19

#define RICHEPG_EPG_TOMORROW_START      5

#define RICHEPG_EPG_KEEP_SEC_KEY        "richepg_epg_keep_sec"
#define RICHEPG_EPG_KEEP_SEC_DEF        120  // unit is sec, "[Off,1 min,2 min,5 min,10 min,30 min,1 hour]"

#define RICHEPG_CHFILTER_KEEP_SEC_KEY   "richepg_chfilter_keep_sec"
#define RICHEPG_CHFILTER_KEEP_SEC_DEF   30  // unit is sec, "[Off,0.5 min,2 min,10 min,30 min,1 hour,Always]"

#define RICHEPG_NUM_PRESS_TIMEOUT_KEY   "richepg_num_press_timeout"
#define RICHEPG_NUM_PRESS_TIMEOUT_DEF   1500 // unit is msec, "[0.5s,1.0s,1.5s,2.0s,2.5s,3.0s,3.5s,4.0s,4.5s,5.0s]"

#define RICHEPG_CERTIFICATION_KEY       "richepg_certification"
#define RICHEPG_CERTIFICATION_DEF       0 // 0, can't access to sat with name of "Certification"; 1, can access

#define RICHEPG_POWER_ON_TIME_KEY       "richepg_power_on_time"
#define RICHEPG_POWER_ON_TIME_DEF       0

#define RICHEPG_FREE_MEMORY_KEY         "richepg_free_memory_flag"
#define RICHEPG_FREE_MEMORY_DEF         0

typedef enum {
	DURATION_SHORT_JOINER = 1 << 0,
	DURATION_WITH_DATE = 1 << 1,
	DURATION_FULL_DATE = 1 << 2
} TimeDurationFormat;

char *app_richepg_date_str_get(time_t sec, bool full_flag);
char *app_richepg_duration_str_get(time_t start_time, time_t finish_time, uint32_t flag);
bool app_richepg_mode_get(void);
void app_richepg_mode_set(bool mode);
bool app_richepg_lineup_mode_get(void);
void app_richepg_lineup_mode_set(bool mode);
bool app_richepg_update_mode_get(void);
void app_richepg_update_mode_set(bool mode);
int app_richepg_update_attach_get(void);
void app_richepg_update_attach_set(int init_value);
time_t app_richepg_start_maint_time_get(void);
void app_richepg_start_maint_time_set(time_t value);
bool app_richepg_epg_view_style_get(void);
void app_richepg_epg_view_style_set(bool mode);
#if EPG_VIEW_DAYS_SET_SUPPORT
int app_richepg_epg_view_days_get(void);
void app_richepg_epg_view_days_set(int init_value);
#endif
int app_richepg_epg_evening_start_get(void);
void app_richepg_epg_evening_start_set(int init_value);
int app_richepg_epg_keep_sec_get(void);
void app_richepg_epg_keep_sec_set(int init_value);
int app_richepg_chfilter_keep_sec_get(void);
void app_richepg_chfilter_keep_sec_set(int init_value);
int app_eutel_epg2_bak_timer_stop(void);
int app_richepg_num_press_timeout_get(void);
void app_richepg_num_press_timeout_set(int init_value);
bool app_richepg_certification_access_get(void);
void app_richepg_certification_access_set(bool mode);
void app_richepg_certification_access_set_flag(bool flag);
void app_richepg_config_reset(void);
int app_richepg_chfilter_keep_sec_clear(bool build_flag);
int app_richepg_chfilter_keep_sec_check_set(void);

int app_eutel_chinfo_create_dialog(void);
int app_eutel_chmore_create_dialog(void);
int app_eutel_chlist_create_dialog(void);
int app_eutel_epg_real_create_dialog(void);
int app_eutel_epg_create_dialog(void);
int app_eutel_epg2_create_dialog(void);
int app_eutel_info_create_dialog(void);
int app_eutel_record_create_dialog(void);
int app_eutel_recordinfo_create_dialog(void);
int app_eutel_sat_list_create_dialog(void);// 1 success ; other failer

RichepgEpgEvent *app_eutel_epg_info_get(void);
void app_eutel_epg_info_release(void);
RichepgEpgEvent *app_eutel_cur_next_epg_info_get(int flag); // 0,cur; 1,next
extern int app_eutel_epg_cur_next_epg_info_str_get(char **cur_time, char **cur_str, char **next_time, char **next_str);
extern bool app_eutel_parental_lock_info_get(bool force_flag, bool *change_flag);
bool app_eutel_parental_lock_passwd_flag_get(void);
void app_eutel_parental_lock_passwd_flag_set(bool flag);
bool app_eutel_parental_lock_replay(void);

bool app_richepg_gui_reverse(void);
int app_richepg_keyboard_sel(void);
char *app_richepg_translate_str(char *str);
char *app_richepg_epg_no_info_str_get(void);

status_t app_eutel_record_hotplug(char *hotplug_dev);
int app_eutel_record_manage_init(bool on_fullscreen);
int app_eutel_record_ctrl_build(void);

// ear
void app_eutel_ear_hide(void);
void app_eutel_ear_show(void);
void app_eutel_ear_create(void);
void app_eutel_ear_destroy(void);

// installation / maintenance /database
bool app_eutel_install_collect_check(void);
bool app_eutel_maint_collect_check(void);
bool app_eutel_antenna_check_end_dialog(void);
bool app_eutel_maintenance_check_end_dialog(void);
bool app_eutel_process_check_all_no_stop(void);
int app_eutel_lineup_create_dialog(bool install_flag);
int app_eutel_change_lineup(int lineup_id);
int app_eutel_maint_check_start(void);
int app_eutel_maint_check_stop(void);
void app_eutel_epg_maint_popup(char *tip, bool cancel_to_epg);
void app_eutel_time_update(void);

// add fuse 20241106
int app_eutel_core_switch_work_data(int change_mode,int sat_id,int x,int y,int epg_build);

// switch mode and lineup
int app_eutel_key_switch_mode(void);
int app_eutel_key_switch_lineup(void);
int app_eutel_key_switch_all(void);

// collect information
void app_eutel_collect_release(void);
int app_eutel_collect_add_item_genre_add(int count, char *genres_name);
int app_eutel_collect_add_item_ch_mod(int lcn, char *prog_name, char *genres_name);
int app_eutel_collect_add_item_ch_add(int lcn, char *prog_name, char *genres_name);
int app_eutel_collect_add_item_rec_add(int ch_id, time_t rec_duration, char *rec_name, char *genres_name);
int app_eutel_collect_add_item_ecl_maint(char *sat_name, time_t seconds);
int app_eutel_collect_add_item_epg_maint(char *sat_name, time_t seconds);
int app_eutel_collect_add_item_start_maint(char *sat_name, time_t seconds);
int app_eutel_collect_add_item_finish_maint(char *sat_name, time_t seconds);
int app_eutel_collect_add_item_fail_maint(char *sat_name, time_t seconds);
void app_eutel_collect_start_index_record(void);
void app_eutel_collect_finish_index_record(void);
int app_eutel_collect_save(void);
int app_eutel_collect_parse(void);
int app_eutel_collect_reset(void);// reset api 20221027

int app_eutel_collect_create_dialog(void);
void app_eutel_collect_info_create_dialog(void);
int app_eutel_collect_update_dialog(void);

int app_eutel_epg_plug_in_out(void);

int app_eutel_get_memory_free_flag(void);
void app_eutel_set_memory_free_flag(int flag);

#endif
#endif

