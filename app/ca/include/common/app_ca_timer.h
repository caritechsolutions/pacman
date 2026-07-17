#ifndef __APP_CA_TIMER_H__
#define __APP_CA_TIMER_H__

#include "gui_core/gui_timer.h"

event_list* app_ca_create_timer(timer_event event, int time, void *data,  TIMER_FLAG flag);
int app_ca_reset_timer(event_list *pTimer, int time);
int app_ca_remove_timer(event_list *pTimer);
int app_ca_remove_timer_list(void);
int app_ca_timer_stop(event_list *pTimer);
int app_ca_timer_exec(void);
#endif
