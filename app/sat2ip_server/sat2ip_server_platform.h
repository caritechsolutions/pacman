#ifndef __SAT2IP_SERVER_PLATFORM_H__
#define __SAT2IP_SERVER_PLATFORM_H__

#include "app_config.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server_porting.h"
#include "module/pm/gxpm_manage.h"


#define SATIP_USE_FLAG_KEY              "satip>use_flag"        // 是否启用功能
#define SATIP_PRIO_FLAG_KEY             "satip>prio_flag"       // 是否开启高优先级，ecos开高优先级才能播凤凰高清
#define SATIP_SWITCH_FLAG_KEY           "satip>switch_prog"     // 是否允许跨频点切台
#define SATIP_RTSP_FLAG_KEY             "satip>rtsp_flag"       // 是否支持dms的rtsp播放链接
#define SATIP_CLIENT_FLAG_KEY           "satip>client_flag"     // 是否支持同一个客户主机IP启用多个客户端
#define SATIP_EXIT_REMOVE_FILES_KEY     "satip>remove_flag"     // 是否退出功能时删除satip描述文件

#define SATIP_USE_FLAG_DEFAULT          1
#ifdef LINUX_OS
#define SATIP_PRIO_FLAG_DEFAULT         0
#else
#define SATIP_PRIO_FLAG_DEFAULT         1
#endif
#define SATIP_SWITCH_FLAG_DEFAULT       0
#define SATIP_RTSP_FLAG_DEFAULT         0
#define SATIP_CLIENT_FLAG_DEFAULT       0
#define SATIP_EXIT_REMOVE_FILES_DEFAULT 1

void app_sat2ip_arg_init(void);
int app_sat2ip_use_flag_get(void);
void app_sat2ip_use_flag_set(int flag);
int app_sat2ip_prio_flag_get(void);
void app_sat2ip_prio_flag_set(int flag);
int app_sat2ip_switch_flag_get(void);
void app_sat2ip_switch_flag_set(int flag);
int app_sat2ip_rtsp_flag_get(void);
void app_sat2ip_rtsp_flag_set(int flag);
int app_sat2ip_client_flag_get(void);
void app_sat2ip_client_flag_set(int flag);
int app_sat2ip_remove_flag_get(void);
void app_sat2ip_remove_flag_set(int flag);

void app_sat2ip_update_prog_total(void);
void app_sat2ip_set_net_ok(const char* ip_addr);
void app_sat2ip_unset_net_ok(void);
void app_sat2ip_set_gui_ok(void);
void app_sat2ip_unset_gui_ok(void);
void app_sat2ip_unset_gui_ok_fast(void); // for dlna close sat2ip
void app_sat2ip_set_dlg_ok(void);
void app_sat2ip_unset_dlg_ok(void);
void app_sat2ip_set_ignore_sec(int sec);
void app_sat2ip_set_dialog_pause(void);
void app_sat2ip_set_dialog_resume(void);

void app_sat2ip_play_start(int prog_id);
void app_sat2ip_play_stop(int prog_id);
bool app_sat2ip_check_playing(void);
int app_sat2ip_play_change(GxBusPmDataProg *prog);
int app_sat2ip_switch_tip_get_by_prog(GxBusPmDataProg *prog);
int app_sat2ip_switch_tip_get_by_pos(int prog_pos);
int app_sat2ip_switch_tip_get_by_id(int prog_id);
int app_sat2ip_switch_tip_get_force(void);
int app_sat2ip_operate_tip_get_force(void);

#endif
#endif
