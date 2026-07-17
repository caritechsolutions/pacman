/*************************************************************************
	> File Name: app/app_alert_tip.c
	> Author:
	> Mail:
	> Created Time: 2021年03月12日 星期五 09时23分30秒
 ************************************************************************/
#include "app.h"
#include "app_windows.h"
#include "app_str_id.h"
#include "app_main_menu_tip.h"
#include "app_book.h"
#include "app_module.h"
#include "app_alert_tip.h"

#if NO_SIGNAL_ALERT_TIP_SUPPORT

#define TEXT_ALERT_TIP_TITLE "text_alert_tip_title"
#define TEXT_ALERT_TIP_CONTENT "text_alert_tip_content"
#define IMG_ALERT_RED "img_alert_red"
#define TEXT_ALERT_RED "text_alert_red"
#define IMG_ALERT_YELLOW "img_alert_yellow"
#define TEXT_ALERT_YELLOW "text_alert_yellow"

static AlertParas s_alert_params={0};
static event_list* s_alert_timer = NULL;

static int app_alert_timer_timeout(void *userdata)
{
	s_alert_timer = NULL;
	if(s_alert_params.timeout_cb)
	{
		s_alert_params.timeout_cb();
	}
	return 0;
}

static void app_alert_update(void)
{
	if(s_alert_params.str_title)
	{
		GUI_SetProperty(TEXT_ALERT_TIP_TITLE, "string", s_alert_params.str_title);
		GUI_SetProperty(TEXT_ALERT_TIP_TITLE, "state", "show");
	}
	else
	{
		GUI_SetProperty(TEXT_ALERT_TIP_TITLE, "state", "hide");
	}
	if(s_alert_params.str_content)
	{
		GUI_SetProperty(TEXT_ALERT_TIP_CONTENT, "string", s_alert_params.str_content);
		GUI_SetProperty(TEXT_ALERT_TIP_CONTENT, "state", "show");
	}
	else
	{
		GUI_SetProperty(TEXT_ALERT_TIP_CONTENT, "state", "hide");
	}
	if(s_alert_params.str_red)
	{
		GUI_SetProperty(IMG_ALERT_RED, "state", "show");
		GUI_SetProperty(TEXT_ALERT_RED, "string", s_alert_params.str_red);
		GUI_SetProperty(TEXT_ALERT_RED, "state", "show");
	}
	else
	{
		GUI_SetProperty(IMG_ALERT_RED, "state", "hide");
		GUI_SetProperty(TEXT_ALERT_RED, "state", "hide");
	}
	if(s_alert_params.str_yellow)
	{
		GUI_SetProperty(IMG_ALERT_YELLOW, "state", "show");
		GUI_SetProperty(TEXT_ALERT_YELLOW, "string", s_alert_params.str_yellow);
		GUI_SetProperty(TEXT_ALERT_YELLOW, "state", "show");
	}
	else
	{
		GUI_SetProperty(IMG_ALERT_YELLOW, "state", "hide");
		GUI_SetProperty(TEXT_ALERT_YELLOW, "state", "hide");
	}
	if((s_alert_params.timeout>0)&&s_alert_params.timeout_cb)
	{
		APP_TIMER_REMOVE(s_alert_timer);
		s_alert_timer = create_timer(app_alert_timer_timeout, s_alert_params.timeout*1000, NULL, TIMER_ONCE);
	}
}

SIGNAL_HANDLER int app_alert_create(GuiWidget *widget, void *usrdata)
{
	app_alert_update();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_alert_destroy(GuiWidget *widget, void *usrdata)
{
	APP_TIMER_REMOVE(s_alert_timer);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_alert_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch (event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_MENU:
				case STBK_EXIT:
                    GUI_EndDialog(WND_ALERT_TIP);
                    if(s_alert_params.key_exit)
                        s_alert_params.key_exit();
					break;

				case STBK_RED:
					if(s_alert_params.key_red)
					{
						s_alert_params.key_red();
					}
					break;

				case STBK_YELLOW:
					if(s_alert_params.key_yellow)
					{
						s_alert_params.key_yellow();
					}
					break;

				default:
					break;
			}

		default:
			break;
	}

	return ret;
}

void app_alert_create_dialog(AlertParas *paras)
{
	if(GUI_CheckDialog(WND_ALERT_TIP) == GXCORE_ERROR)
	{
		memcpy(&s_alert_params, paras, sizeof(AlertParas));
		GUI_CreateDialog(WND_ALERT_TIP);
	}
}

void app_alert_end_dialog(void)
{
	if(GUI_CheckDialog(WND_ALERT_TIP) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_ALERT_TIP);
	}
}
#endif
