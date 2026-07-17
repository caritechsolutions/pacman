#ifndef __APP_DVB2IP_PLATFORM_H__
#define __APP_DVB2IP_PLATFORM_H__

#include "app_config.h"
#if DVB2IP_SERVER_SUPPORT
#include "gxbus.h"
#include "dvb2ip_export.h"
#include "module/pm/gxpm_manage.h"
typedef struct {
    uint8_t mode;  // 0 tv/radio, 1 fm
    uint8_t play_support;
} Dvb2ipStartMsg;

void app_dvb2ip_arg_init(void);
int app_dvb2ip_resource_alloc(void);
int app_dvb2ip_resource_free(void);
int app_dvb2ip_use_flag_get(void);
void app_dvb2ip_use_flag_set(int flag);
int app_dvb2ip_prio_flag_get(void);
void app_dvb2ip_prio_flag_set(int flag);
int app_dvb2ip_follow_flag_get(void);
void app_dvb2ip_follow_flag_set(int flag);
int app_dvb2ip_switch_flag_get(void);
void app_dvb2ip_switch_flag_set(int flag);
void app_dvb2ip_net_flag_set(int flag, const char *ip_addr);
void app_dvb2ip_gui_flag_set(int flag);
void app_dvb2ip_prog_num_update(void);

int app_dvb2ip_fm_stop_rec_tip_by_freq(uint32_t freq_khz);
int app_dvb2ip_switch_tip_get_by_prog(GxBusPmDataProg *prog);
int app_dvb2ip_switch_tip_get_by_pos(int prog_pos);
int app_dvb2ip_switch_tip_get_by_id(int prog_id);
int app_dvb2ip_switch_tip_get_force(void);
int app_dvb2ip_operate_tip_get_force(void);
bool app_dvb2ip_check_playing(void);
bool app_dvb2ip_fm_get_client_connect(void);
int app_msg_dvb2ip_play_start(Dvb2ipStartMsg *param);
int app_msg_dvb2ip_play_stop(void);

int app_dvb2ip_set_tp(int prog_id);
int app_dvb2ip_set_play(int prog_id);
bool app_dvb2ip_resume_normal_play(void);
#if DVB2IP_SERVER_USE_STATIC_SCREEN
int app_dvb2ip_play_entry(void);
int app_dvb2ip_play_hotplug_msg_proc(GxMessage *msg);
#endif
char *app_dvb2ip_get_prog_url(int prog_id);

#endif
#endif

