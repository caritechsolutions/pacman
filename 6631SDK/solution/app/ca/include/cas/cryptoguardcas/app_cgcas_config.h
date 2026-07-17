#ifndef __APP_CGCAS_CONFIG_H__
#define __APP_CGCAS_CONFIG_H__

#include "cascam.h"

void app_cgcas_init(void);
void app_cgcas_menu_exec(void);
void app_cgcas_switch_channel(CascamSwitchChannel switch_channel);
void app_cgcas_switch_freq(void);
void app_cgcas_stop_ca(void);
int32_t app_cgcas_do_event(CascamOsdEvent* para);
void app_cg_maturity_exec(void);
void app_cg_pin_exec(void);
void app_cg_strict_rating_exec(void);
int32_t app_cgcas_do_event_layer(CascamOsdEvent* para);
//bool app_cgcas_get_pvr_forbid(void);
//void app_cgcas_set_pvr_forbid(bool flag);
#endif
