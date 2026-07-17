#ifndef __APP_VOICE_CONTROL_API__
#define __APP_VOICE_CONTROL_API__

//#include "../core/include/vc_api.h"
#include "app_config.h"
#include "app_pop.h"

// 定义错误类型的枚举
typedef enum {
    STATUS_TYPE_CAN_SUPPORT = 0,  // 0 - can support
    STATUS_TYPE_NOT_SUPPORT = 1,   // 1 - not support
    STATUS_TYPE_REPEAT = 2,        // 2 - repeat
    STATUS_TYPE_WINDOWN_UNSUPPORTED = 3, // 3 - the current scenario does not support
    STATUS_TYPE_SELECT_CHANNEL = 4 // 4 - select channel
} ErrorType;
#define MAX_FIND_NAME_COUNT 20
// device name
#ifdef V_UAC
#define app_vc_uac_device_in(ptr_name, id)           vc_uac_device_in(ptr_name, id)
#define app_vc_uac_device_out(ptr_name, id)          vc_uac_device_out(ptr_name, id)
// for debug
#define app_vc_uac_capture_start(ptr_path)           vc_uac_capture_start(ptr_path)
#define app_vc_uac_capture_stop()                    vc_uac_capture_stop()
#define app_vc_uac_capture_get_status()              vc_uac_capture_get_status()
#endif

#define app_vc_uart_device_in(ptr_name, id)          vc_uart_device_in(ptr_name, id)
#define app_vc_uart_device_out(ptr_name, id)         vc_uart_device_out(ptr_name, id)
// get dongle info
#define app_vc_get_device_info(ptr_info)             vc_get_device_info(ptr_info)
// get dongle work status
#define app_vc_get_device_status(ptr_status)         vc_get_device_status(ptr_status)
// check the dongle is exist or not
#define app_vc_check_device_exist()                  vc_device_status_check()
// update
#define app_vc_fw_upgrade(ptr_fw, fw_byte, cb_func)  vc_dongle_fw_upgrade(ptr_fw, fw_byte, cb_func)
#define app_vc_fw_upgrade_stop()                     vc_dongle_fw_upgrade_stop()
//
#define app_vc_send_rks_data(ptr_int_data, count)    vc_send_remote_key_data(ptr_int_data, count)


// module init
extern int app_vc_init(void);
// remote & cmd control
extern int app_get_forbidden_flag(void* params);
// message
extern int app_vc_cloud_msg_process(void *message);
#if VOICE_CONTROL_SUPPORT
// for cloud
extern int app_vc_cloud_init(void);
extern int app_vc_cloud_destroy(void);
extern int app_vc_cloud_open(int *handle);
extern int app_vc_cloud_close(int handle);
extern int app_vc_cloud_active(int handle);
extern int app_vc_cloud_data_read(int handle, unsigned char** data, unsigned int *read_real_size);
extern int app_vc_cloud_data_is_begin(void);
extern int app_vc_cloud_data_is_end(void);
extern int app_vc_cloud_data_term(void);


extern int app_vc_goto_fullscreen(void);
// play
extern int app_music_play_status_get(void);
extern int app_pic_play_status_get(void);
extern int app_movie_play_status_get(void);
extern int app_vc_common_view_table_area_select(void);
//cmd: 0-pause; 1-play; 2-record; 3-stop record
//ret: 0-success, 1-not support, 2-repeat operator, -1-err
extern int app_vc_fullscreen_reaction(int cmd);
// ret: 0-normal mode; 1-edit mode
extern int app_vc_channel_edit_mode_flag(void);
//ret: 0-can support; 1-not support; 2:repeat
extern int app_vc_channel_list_play_flag(void);
//ret: 0-can support; 1-not support;
extern int app_vc_channel_list_switch_channel(void);
//ret: 0-can support; 1-not support;
extern int app_vc_channel_edit_switch_channel(void);
// params: 0-stop, 1-pause; 2-play
// return: 0-support, 1-not support, 2-repeat, -1-failed
extern int app_vc_netvideo_play_flag(int flag);

extern int top_stbk_halt(uint32_t scan_code);
extern void book_popdlg_exec(PopDlgRet ret);
extern void popdlg_destroy(void);
extern void app_pvr_stop(void);
extern bool app_simple_lowpower_status_get(void);
extern void app_subt_resume(void);
extern void macro_ch_ed_stop_video(void);
extern int Gui_WriteKeyBuf(unsigned int key);
extern int app_vc_osdttx_flag_get(void);

extern int app_vc_number_switch(unsigned int num, char* name, unsigned int name_len);
extern int app_vc_net_number_switch(unsigned int num, char* name, unsigned int name_len);
extern int app_vc_switch_basic_keypress(int key);
extern int app_fuzzy_strings_matching(char *str1, char **str2_list, unsigned int str2_count, int *index, unsigned int *index_count, int passing_score);
extern uint32_t app_play_id_get(void);
extern unsigned int app_find_get_value_by_index(unsigned int sel,unsigned int num_elements);
extern void find_name_change_list(uint32_t *pids, int *match_index, unsigned int index_count);

extern void app_trigger_end_popdlg(void);
extern void app_multi_lang_keyborad_menu_destroy(void);
extern void app_wifi_tip_lost_focus_hide(void);
extern bool app_dvb2ip_resume_normal_play(void);
extern void app_system_trriger_func(void);

extern int app_pvr_stop_cb(PopDlgRet ret);
extern void app_create_gxiptv_menu(void);
extern void app_create_xtream_menu(void);
extern void app_create_stalker_menu(void);

// notify display
extern void app_voice_control_notify_ex(char *str);
extern void app_voice_control_notify_waiting(void);
extern int app_vc_volume_quick_set_ex(int step, char direct_flag,  char play_flag, unsigned int *current_volume, unsigned int *max_volume);
extern int app_vc_set_mute_quick_ex(char exec_flag, char is_mute);
extern int app_vc_set_play_quick(int is_play);
extern int app_vc_play_by_num_quick(unsigned int num, char *name, unsigned int name_len);
extern int app_vc_play_by_name_quick(char *name, char* real_channel_name, unsigned int real_name_len);
extern int app_vc_play_recall_quick(void);
extern int app_vc_switch_channel_quick(int cmd);
extern int app_vc_goto_fullscreen(void);
extern int app_vc_goto_epg(void);
extern int app_vc_goto_mainmenu(void);
extern int app_vc_record_control(int is_stop);
extern int app_vc_standby_control(int is_standby);
//extern int app_vc_stabdby_status_get(void);
extern char* app_vc_transform_strings(char* in_str);
extern int app_vc_goto_iptv(void);
extern int app_vc_goto_xtream(void);
extern int app_vc_goto_stalker(void);
#endif

#endif
