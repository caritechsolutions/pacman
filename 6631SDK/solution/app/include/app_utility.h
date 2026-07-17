#ifndef __GXAPP_UTILITY_H__
#define __GXAPP_UTILITY_H__
#include "app.h"
#include "gxtype.h"
#include "gxcore.h"
#include <time.h>
#include "module/app_pvr.h"
#include "module/app_frontend.h"

#define ARRAY_LEN(array) ((NULL==array)   ? 0:sizeof(array) / sizeof(array[0]))
#define CMD_LEN_MAX (200)
#define IS_EMPTY_OR_NULL(str)	((NULL==str || str[0]=='\0') ? 1:0 )
#define GXCORE_FREE(str)	{ if(NULL!=str) GxCore_Free(str); str=NULL; }

typedef enum
{
	MD5_SUM_OK = 0,
	MD5_SUM_ERROR = 1
}Md5Sum;

typedef struct
{
	char *quality_bar;
	char *strength_bar;
}SignalBarWidget;

typedef struct
{
	int quality_val;
	int strength_val;
}SignalValue;

typedef enum
{
	WEEK_SUN = 0,
	WEEK_MON,
	WEEK_TUES,
	WEEK_WED,
	WEEK_THURS,
	WEEK_FRI,
	WEEK_SAT
}WeekDay;

typedef enum
{
    UPGRADE_START = 0,
    UPGRADE_SUCCESS,
    UPGRADE_STOPPED
}UpgradeState;

typedef enum
{
    WAKE_FROM_POWER_OFF = 0,
    WAKE_FROM_MANUA_STANDBY,
    WAKE_FROM_AUTO_STANDBY
}WakeState;

typedef enum {
    LONG_DATE_EDIT = 1 << 0,        // show "year month day"
    SHORT_DATE_EDIT = 1 << 1,       // show "month day"
    LONG_TIME_EDIT = 1 << 2,        // show "hour min sec"
    SHORT_TIME_EDIT = 1 << 3,       // show "hour min"
    TIME_AHEAD_EDIT = 1 << 4        // time ahead date
} DateTimeEditStr;

typedef enum
{
	YMD_WITH_SLASH = 0,
	YMD_WITH_HYPHEN,
	YMD_WITH_DOT,
	DMY_WITH_SLASH,
	DMY_WITH_HYPHEN,
	DMY_WITH_DOT,
	MDY_WITH_SLASH,
    MDY_WITH_HYPHEN,
	MDY_WITH_DOT,
}DateStrFormat;

typedef struct app_netapp_event_class_t
{
	const char*         name;
	const char*         msg_wndName;
	int  (*msg_proc_callback)       (GxMessage *msg);
	int  (*msg_dev_out_callback)       (void);
	int  (*key_net_open_check)       (int key);
}app_netapp_event_class;

typedef enum
{
    DVB_PLAY_MODE = 0,
    NET_PLAY_MODE,
    FILE_PLAY_MODE,
}SysDynPlayMode;


__BEGIN_DECLS

char* get_local_date(char *date_value,uint32_t data_len);

time_t get_local_time_by_timezone(time_t src_date);

time_t get_display_time_by_timezone(time_t src_date);

WeekDay app_weekday_local2gmt(WeekDay local_weekday, time_t local_day_sec);

WeekDay app_weekday_gmt2local(WeekDay gmt_weekday, time_t gmt_day_sec);

bool check_usb_status(void);
uint64_t get_usb_partition_space(char *partition_path);

char *get_usb_url_partition(char *file_url);
char *app_common_data_time_edit_str(time_t time, uint32_t mode, char *buf, size_t len);
void app_time_ms_to_edit_str(uint64_t time_ms, char* str, size_t len);
char *app_time_to_date_edit_str(time_t time);
char *app_time_to_hourmin_edit_str(time_t time);
char *app_time_to_hms_str(time_t time);
char *app_time_to_format_str(time_t time);
char *app_time_to_short_date_edit_str(time_t time);
status_t  app_edit_str_to_date_sec(char *date_str, time_t *res_sec);
time_t app_edit_str_to_hourmin_sec(char *time_str);
int app_date_string_format(void);
char *app_date_edit_format_get(void);
float app_float_edit_str_to_value(const char *str);

status_t app_signal_progbar_update(uint32_t tuner, SignalBarWidget *bar, SignalValue *val);

uint32_t app_hexstr2value(uint8_t* string);
void app_getvalue(const char* url, const char* item, int32_t *retVal);
uint32_t app_key_full_code(uint16_t key_code);

extern bool check_usb_write_permissions(const char *path);
extern int generatorMac(char *mac);
unsigned int app_get_tick_time(void);
int app_get_update_state(void);
void file_list_destroy(void);
int app_upgrade_crc_check_buf(char *p_data, int size);
uint32_t app_get_flash_size(void);
uint32_t app_get_ddr_size(void);
extern int app_enddialog_after_wnd(char* wnd);
extern int app_enddialog_wnd(char* wnd);
void app_set_widget_string(const char *wgt, const char *str);
void app_show_sys_time_and_date(const char *time_wgt, const char *date_wgt);
void app_set_window_back_ground(const char *wnd, const char *key, bool flush);
void app_update_window_back_ground(const char *wnd, const char *key, bool flush);
void app_unset_window_back_ground(const char *wnd, bool free_space, bool flush);
void app_set_widget_string(const char *wgt, const char *str);
void app_player_femem_init(void);
void app_player_femem_control(bool use_femem);
void app_progbar_update_value(const char *widget, int value);
uint32_t app_snr_progbar_update(uint32_t tuner, const char *wgt_bar, const char *wgt_val);
extern int app_language_code_cmp(char* language1, char* language2);
extern int app_iso3166_country_code_get(char* countroy_name,char* three_character_code,char* two_character_code);
extern void app_all_frontend_monitor_control(AppFrontend_Monitor monitor, bool clean_para);
extern void app_only_cur_frontend_monitor_start(uint32_t cur_tuner);
extern void app_set_fre_zero(char *url);
int app_get_sat_params(GxBusPmDataSat* sat, unsigned char demod_type);
extern int app_all_channels_del_by_sat_id(uint32_t sat_id);
extern uint8_t app_all_tp_del_by_sat_id(uint16_t sat_id);
extern uint8_t app_del_noprog_tp_by_sat_id(uint16_t sat_id);
extern uint32_t app_sat_id_get_by_type(GxBusPmDataSatType type);
extern int app_channel_num_check_by_sat_id(uint32_t sat_id, uint32_t *TvNum, uint32_t *RadioNum);
extern int app_all_channels_num_check(uint32_t *TvNum, uint32_t *RadioNum);
extern uint32_t app_channel_panel_led_get(uint32_t prog_id);
extern char *app_channel_num_str_get(GxMsgProperty_NodeByPosGet *node_prog, int32_t pos);
status_t app_pop_net_connect_check(void);
uint32_t app_tick_time_msec_get(void);
uint32_t app_tick_time_sec_get(void);
extern struct partition_info *app_get_partition(struct partition *table, const char* name);
bool app_ott_config_get(void);
#if UI_MIRROR_SUPPORT
extern int app_menu_mirror_get(void);
extern void app_menu_mirror_set(char* language);
#endif
#if FM_SUPPORT
int app_get_fm_tuner_id(void);
#endif
void app_player_mem_config(char *name, unsigned int size);
void app_system_performace_dynamic_change(SysDynPlayMode mode);
char *netupg_ftp_get_authstr(char *username, char *password);
extern void app_display_size_str_get(uint64_t size, char *str, size_t len);
extern int app_av_set_videoout_mode(uint8_t video_out_mode);
__END_DECLS
#endif
