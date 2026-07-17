
/*
==============================================================================
 *
 *       Filename:  app_fuse_api.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2024年7月22日 14时27分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * ===========================================================================

特有的接口如下命名
app_fuse_spec_xxxx
接口内部实现，RICHEPG_SUPPORT是主体，其它只是返回值

通用的接口如下命名
app_fuse_common_xxx
接口内部实现，RICHEPG_SUPPORT下，会处理2个模式下的。其它只处理公版模式下的。
*/

#ifndef APP_FUSE_API_H
#define APP_FUSE_API_H


/* 以下2个配置可以根据方案特点进行配置 */

/*  sattv卫星由于特殊性，其相关的节目，epg信息等，需要保存到存储设备(flash或者usb)
 *　而方案比如6605s，flash比较小，所以安装的数据是存放在usb设备中，flash只存放一些必须的数据。
 *  1: 如果不存在usb设备，sattv只能安装一颗卫星，并且很多sattv特有的功能都没法使用。
 *  2: 如果安装的时候，存在usb设备，并且安装了多颗sattv卫星,但是使用中，拔掉usb,那也只能使用一颗satt卫星。
 *  另外存到usb的sattv数据，方案内存空间是无法加载所有数据的，所以存在切换sattv卫星的时候，需要从usb中加载一些数据，
 *  这样就造成切换sattv卫星的时候，由于加载数据，而造成“慢”的现象，特别是菜单切换中。
 *  由于这个特点，所以可以根据方案的特点，采用了下面的一些宏控制。
 */

/* sattv卫星切换中，用增加内存的方式来保存简易的pos和sel的关系，
 * 在菜单channel edit中使用，加快切换速度
 * 如果只装一个sattv卫星，不使用，如果装多个sattv卫星，打开使用
 */
#define APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT

#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
/*
 * 简易数据保存只在菜单channel edit进入后使用，平时不使用。
 * 优点是进入channel edit菜单才使用内存，平时不会占用内存。缺点是进入edit菜单后，sattv卫星需要切换后才能保存简易数据，开始有切换提示。
 * 关闭宏，如果sattv卫星在其它应用场景切换过，那就会保存简易数据，进入channel edit后，不需要进行sattv卫星切换。
 */
#define APP_EUTEL_CHANNEL_EASY_SAVE_ONLY_EDIT
#endif


/* 以下配置为系统使用，不要随便更改 */

//切换到freescan，进行sattv卫星维护的检查。不要改动，要不待机维护会不成功。
#define APP_EUTEL_MAINT_CHECK_ANY_MODE

#define EUTUL_SAT_MAX_NUM (4)

#define APP_FUSE_FREE_SCAN_WORK_MODE (1)
#define APP_FUSE_SATTV_WORK_MODE     (2)


int  app_fuse_spec_set_prog_check_callback(void);
int  app_fuse_spec_del_prog_check_callback(void);

int  app_fuse_spec_play_parental_lock_start(void);
int  app_fuse_spec_play_parental_lock_stop(void);
int  app_fuse_spec_play_set_lcn(void);

int  app_fuse_spec_is_sattv_work_mode(void);
int  app_fuse_spec_is_sattv_view_mode(int cur_type,int check_type,int group,int sat_id);

int  app_fuse_spec_switch_work_mode(int mode);
int  app_fuse_spec_prepare_switch_work_env(int set_mode,int group,int sat_id);
int  app_fuse_spec_switch_work_env(int group,int sat_id);
int  app_fuse_spec_switch_work_edit_group_change(void);
int  app_fuse_spec_switch_work_edit_exit(void);

int  app_fuse_spec_switch_work_group_change(void);

int  app_fuse_spec_add_group_id(int* p_satid,int size);
int  app_fuse_spec_tv_radio_num(int type);

int  app_fuse_spec_sat_key_is_redefine(void);

int  app_fuse_spec_get_maint_wake_up_time(unsigned int wake_time);
int  app_fuse_spec_wake_from_reboot_flag_keypress(int key_event);
int  app_fuse_spec_get_wake_from_reboot_memory_flag_value(void);

/*- 开机 init -
 *  ！！！！！！！！！！！
 *  接口开始，有一个关闭视频输出。在系统设置的开始设置好HDMI后到关闭视频输出稍微有点时间，如果发现有视频输出，需要提前。
 *  把这个功能放在这里，是为了代码融合的时候，在其它地方就不需要增加了。
 */
int  app_fuse_spec_system_init(void);

/*- 开机 init 滞后操作 - */
int  app_fuse_spec_system_after_init(int mode);

/*- 重置 恢复出厂设置操作 - */
int  app_fuse_spec_system_reset(void);

int  app_fuse_spec_gui_reverse(void);

/*- 设置语言后，重建相关的数据 - */
int  app_fuse_spec_build_by_language_change(unsigned int lang);

int  app_fuse_spec_epg_start_work(void);
int  app_fuse_spec_epg_stop_work(void);
int  app_fuse_spec_epg_set_timer_event(int type,bool bflag, int book_id);
int  app_fuse_spec_epg_refresh_tip(void);

int  app_fuse_spec_get_pvr_statue(void);
int  app_fuse_spec_pvr_file_info_update(const char *path, const char *name);
int  app_fuse_spec_pvr_file_info_save(void);
int  app_fuse_spec_pvr_record_create_fullsreeen(void);

int  app_fuse_spec_full_screen_create_dialog(GUI_Event *event,int change_flag);
int  app_fuse_spec_full_screen_check_dialog(const char *wnd);

int  app_fuse_spec_proc_top_hotplug_in(char *hotplug_dev);
int  app_fuse_spec_proc_top_hotplug_out(char *hotplug_dev);

int  app_fuse_spec_proc_top_book_trigger(void);

int  app_fuse_spec_sat_type_check(int sat_id);

int  app_fuse_spec_satellite_switch_check_dialog(void);

/*
 *  通用接口，如果RICHEPG_SUPPORT不支持的话，会只使用公版功能
 */

char* app_fuse_common_get_pragram_num_str(unsigned int sel);

int  app_fuse_common_channel_info_update_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition);
int  app_fuse_common_channel_info_check_dialog(void);
int  app_fuse_common_channel_info_compare_dialog(const char *wnd);
int  app_fuse_common_channel_info_create_dialog(void);
int  app_fuse_common_channel_info_end_dialog(void);

int  app_fuse_common_channel_info_signal_check_dialog(void);
int  app_fuse_common_channel_info_signal_compare_dialog(const char *wnd);
int  app_fuse_common_channel_info_signal_end_dialog(void);
int  app_fuse_common_channel_info_update_dialog(void);
int  app_fuse_common_channel_info_update_under_pop_dlg(void);

int  app_fuse_common_channel_info_clear_epg_str(void);

int  app_fuse_common_channel_list_check_dialog(void);
int  app_fuse_common_channel_list_compare_dialog(const char *wnd);

int  app_fuse_common_epg_check_dialog(void);
int  app_fuse_common_epg_compare_dialog(const char *wnd);
int  app_fuse_common_epg_create_dialog(void);

int  app_fuse_common_pvr_record_pre_create_dialog(int on_fullscreen);
int  app_fuse_common_pvr_record_create_dialog(void);

#endif

