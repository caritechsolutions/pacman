#ifndef __APP_CASCAM_H__
#define __APP_CASCAM_H__

#include "cascam.h"
/***the function cascam to stb***/
void cascam_app_cas_init(void);
void app_ca_menu_exec(void);
void app_ca_standby(void);
void app_ca_start_ca(void);
void app_ca_stop_ca(void);
void app_ca_stop_descramble(void);
void app_ca_switch_channel(CascamSwitchChannel switch_channel);
void app_ca_quick_switch_channel(CascamQuickSwitchChannel quick_switch_channel);
void app_ca_switch_freq(void);
void app_ca_add_service(CascamServiceInfo service_info);
void app_ca_delete_service(CascamServiceInfo service_info);
void app_ca_destroy_finger(void);
void app_ca_destroy_rolling_osd(void);
void app_ca_rebuild_msg_dialog(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void app_ca_exit_msg_dialog(void);
int32_t app_ca_check_pvr_forbid(void);
int32_t app_ca_pvr_control(uint32_t type, void* param);
int32_t app_ca_check_force_lock_status(void);
int32_t app_ca_cascam_do_event(CascamOsdEvent* para);
#endif
