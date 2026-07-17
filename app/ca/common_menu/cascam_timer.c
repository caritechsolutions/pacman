#include "app.h"
#if CASCAM_SUPPORT
#include "cascam_ui_porting.h"
#include "app_ca_timer.h"

event_list* cascam_create_timer(timer_event event, int time, void *data,  TIMER_FLAG flag)
{
#if CASCAM_UI_MULTI_LAYER
	return app_ca_create_timer(event, time, data, flag);
#else
	return create_timer(event, time, data, flag);
#endif
}

int cascam_reset_timer(event_list *pTimer, int time)
{
#if CASCAM_UI_MULTI_LAYER
	return app_ca_reset_timer(pTimer, time);
#else
	return reset_timer(pTimer);
#endif
}

int cascam_remove_timer(event_list *pTimer)
{
#if CASCAM_UI_MULTI_LAYER
	return app_ca_remove_timer(pTimer);
#else
	return remove_timer(pTimer);
#endif
}

int cascam_timer_stop(event_list *pTimer)
{
#if CASCAM_UI_MULTI_LAYER
	return app_ca_timer_stop(pTimer);
#else
	return timer_stop(pTimer);
#endif
}

#endif
