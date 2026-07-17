/*************************************************************************
	> File Name: app/include/app_alert_tip.h
	> Author:
	> Mail:
	> Created Time: 2021年03月12日 星期五 10时02分49秒
 ************************************************************************/
#include "gui_core.h"
#ifndef __APP_ALERT_TIP_H__
#define __APP_ALERT_TIP_H__

#if NO_SIGNAL_ALERT_TIP_SUPPORT
typedef void (*alert_key_callback)(void);
typedef void (*alert_timeout_callback)(void);

typedef struct {
	char *str_title;
	char *str_content;
	char *str_red;
	alert_key_callback key_red;
	char *str_yellow;
	alert_key_callback key_yellow;
    alert_key_callback key_exit;
	int timeout;
	alert_timeout_callback timeout_cb;
}AlertParas;

void app_alert_create_dialog(AlertParas *paras);
void app_alert_end_dialog(void);
#endif

#endif

