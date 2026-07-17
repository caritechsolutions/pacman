/*
 * =====================================================================================
 *
 *       Filename:  app_pop_tip.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年08月18日 09时47分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_utility.h"
#include "app_module.h"
#include "app_book.h"
#include "app_windows.h"
#include "app_channel_info.h"

#define TXT_BOOK_TITLE           "txt_pop_book_title"
#define IMG_BOOK_OPT             "img_pop_book_opt"
#define BMP_OK              "s_bar_choice_blue4"
#define BMP_CANCLE          "s_bar_choice_blue4_l"
#define TXT_BOOK_TIP             "txt_pop_book"
#define TXT_BOOK_SEC         "txt_pop_second"
#define STR_ID_30_SEC         "30"
#define DEFAULT_TIMEOUT_SEC   30

static event_list* sp_CountTimer = NULL;
static BookDlg thiz_book_param = {0};
static BookDlgRet thiz_book_ret;
static bool thiz_refresh_time = true;

static int timer_power_off_refresh_tip(void *userdata)
{
    char buffer[10] = {0};
    const char *focus_wnd = NULL;


    focus_wnd = GUI_GetFocusWindow();
    if((GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS) && (strcmp(focus_wnd, WND_POP_BOOK) != 0))
    {
        GUI_SetFocusWidget("btn_pop_book_ok");
        GUI_SetProperty(WND_POP_BOOK,"update",NULL);
    }

    if(0 == thiz_book_param.timeout_sec)
        return 0;

    thiz_book_param.timeout_sec--;
    sprintf(buffer,"%d", thiz_book_param.timeout_sec);

    if(thiz_refresh_time)
        GUI_SetProperty(TXT_BOOK_SEC, "string", buffer);

    if(0 == thiz_book_param.timeout_sec)
        book_popdlg_exec(thiz_book_ret.ret);

    return 0;
}

SIGNAL_HANDLER  int app_book_popdlg_set_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;

	event = (GUI_Event *)usrdata;
	msg = (GxMessage*)event->msg.service_msg;

    if(GXMSG_PLAYER_SPEED_REPORT == msg->msg_id
            || GXMSG_PLAYER_STATUS_REPORT == msg->msg_id
            || GXMSG_PLAYER_AVCODEC_REPORT == msg->msg_id)
    {
        if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
        {
            GUI_SendEvent(WIN_MOVIE_VIEW, event);
        }
        if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
        {
            GUI_SendEvent(WIN_MUSIC_VIEW, event);
        }
    }
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_create(GuiWidget *widget, void *usrdata)
{
    int32_t pos_x = 295;
    int32_t pos_y = 190;

	if((thiz_book_param.pos.x==0)&&(thiz_book_param.pos.y==0))
    {
    }
    else
    {
        pos_x = thiz_book_param.pos.x;
        pos_y = thiz_book_param.pos.y;
		GUI_SetProperty(WND_POP_BOOK, "move_window_x", &pos_x);
		GUI_SetProperty(WND_POP_BOOK, "move_window_y", &pos_y);
    }
    GUI_SetProperty(IMG_BOOK_OPT, "img", BMP_OK);
    GUI_SetFocusWidget("btn_pop_book_ok");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_got_focus(GuiWidget *widget, void *usrdata)
{
    thiz_refresh_time = true;
    timer_power_off_refresh_tip(NULL);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_lost_focus(GuiWidget *widget, void *usrdata)
{
    thiz_refresh_time = false;
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_destroy(GuiWidget *widget, void *usrdata)
{
    thiz_refresh_time = true;
    APP_TIMER_REMOVE(sp_CountTimer);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_book_popdlg_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
				book_popdlg_exec(thiz_book_ret.ret);
				break;
			case STBK_LEFT:
			case STBK_RIGHT:
				if(thiz_book_ret.ret == POP_VAL_OK)
				{
                    			thiz_book_ret.ret = POP_VAL_CANCEL;
					GUI_SetProperty(IMG_BOOK_OPT, "img", BMP_CANCLE);
					GUI_SetFocusWidget("btn_pop_book_cancel");
				}
				else
				{
					thiz_book_ret.ret = POP_VAL_OK;
					GUI_SetProperty(IMG_BOOK_OPT, "img", BMP_OK);
					GUI_SetFocusWidget("btn_pop_book_ok");
				}
				break;
			case STBK_MENU:
			case STBK_EXIT:
                book_popdlg_exec(POP_VAL_CANCEL);
				break;

			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

status_t book_popdlg_create(BookDlg *dlg)
{
    char buf[10] = {0};
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
    {
        return GXCORE_ERROR;
	}

	if ((strcasecmp(WND_WIFI, GUI_GetFocusWindow()) == 0) && (strcasecmp("button_wifi_psk", GUI_GetFocusWidget()) == 0))
	{
		GUI_SetProperty("img_wifi_tip_psk", "state", "hide");
	}

	if ((strcasecmp(WND_WIFI, GUI_GetFocusWindow()) == 0) &&
			((strcasecmp("button_wifi_connect", GUI_GetFocusWidget()) == 0) ||
			 (strcasecmp("button_wifi_cancel", GUI_GetFocusWidget()) == 0)))
	{
		GUI_SetProperty("img_wifi_tip_opt", "state", "hide");
    }

    memcpy(&thiz_book_param, dlg, sizeof(BookDlg));
    memcpy(&thiz_book_ret.cache, &dlg->cache, sizeof(book_cache));
    thiz_book_ret.ret = POP_VAL_OK;
    GUI_CreateDialog(WND_POP_BOOK);

    if(POP_FORMAT_DLG == thiz_book_param.format)
    {
        GUI_SetProperty(WND_POP_BOOK, "format", "dialog");
    }
    else
    {
        GUI_SetProperty(WND_POP_BOOK, "format", "popup");
    }

    if(thiz_book_param.timeout_sec <= DEFAULT_TIMEOUT_SEC)
    {
        thiz_book_param.timeout_sec = DEFAULT_TIMEOUT_SEC;
    }

    sprintf(buf,"%d", thiz_book_param.timeout_sec);
    GUI_SetProperty(TXT_BOOK_SEC, "string", buf);
	if(BOOK_POWER_OFF == thiz_book_param.type)
	{
		GUI_SetProperty(TXT_BOOK_TIP, "string", STR_ID_POWER_OFF_TIP);
		GUI_SetProperty(TXT_BOOK_TITLE, "string", STR_ID_POWER_OFF);

	}
	if(BOOK_PROGRAM_PVR == thiz_book_param.type
            || BOOK_PROGRAM_PLAY == thiz_book_param.type)
	{
		GUI_SetProperty(TXT_BOOK_TIP, "string", STR_ID_BOOK_ARRIVE_INFO);
		GUI_SetProperty(TXT_BOOK_TITLE, "string", STR_ID_BOOK);
	}
    APP_TIMER_ADD(sp_CountTimer, timer_power_off_refresh_tip, 1000, TIMER_REPEAT);

	return GXCORE_SUCCESS;
}

status_t book_popdlg_recreate(void)
{
    char buf[10] = {0};
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
    {
        return GXCORE_ERROR;
    }

    GUI_CreateDialog(WND_POP_BOOK);

    if(thiz_book_ret.ret == POP_VAL_CANCEL)
    {
        GUI_SetProperty(IMG_BOOK_OPT, "img", BMP_CANCLE);
        GUI_SetFocusWidget("btn_pop_book_cancel");
    }

    if(POP_FORMAT_DLG == thiz_book_param.format)
    {
        GUI_SetProperty(WND_POP_BOOK, "format", "dialog");
    }
    else
    {
        GUI_SetProperty(WND_POP_BOOK, "format", "popup");
    }

    sprintf(buf,"%d", thiz_book_param.timeout_sec);
    GUI_SetProperty(TXT_BOOK_SEC, "string", buf);
	if(BOOK_POWER_OFF == thiz_book_param.type)
	{
		GUI_SetProperty(TXT_BOOK_TIP, "string", STR_ID_POWER_OFF_TIP);
		GUI_SetProperty(TXT_BOOK_TITLE, "string", STR_ID_POWER_OFF);

	}
	if(BOOK_PROGRAM_PVR == thiz_book_param.type
            || BOOK_PROGRAM_PLAY == thiz_book_param.type)
	{
		GUI_SetProperty(TXT_BOOK_TIP, "string", STR_ID_BOOK_ARRIVE_INFO);
		GUI_SetProperty(TXT_BOOK_TITLE, "string", STR_ID_BOOK);
	}
    APP_TIMER_ADD(sp_CountTimer, timer_power_off_refresh_tip, 1000, TIMER_REPEAT);

	return GXCORE_SUCCESS;
}

void book_popdlg_exec(PopDlgRet ret)
{
    if(POP_VAL_NONE == ret)
    {
        return;
    }
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
    {
        GUI_EndDialog(WND_POP_BOOK);

        if ((strcasecmp(WND_WIFI, GUI_GetFocusWindow()) == 0) && (strcasecmp("button_wifi_psk", GUI_GetFocusWidget()) == 0))
        {
            GUI_SetProperty("img_wifi_tip_psk", "state", "show");
        }

        if ((strcasecmp(WND_WIFI, GUI_GetFocusWindow()) == 0) &&
                ((strcasecmp("button_wifi_connect", GUI_GetFocusWidget()) == 0) ||
                 (strcasecmp("button_wifi_cancel", GUI_GetFocusWidget()) == 0)))
        {
            GUI_SetProperty("img_wifi_tip_opt", "state", "show");
        }
        if(GXCORE_SUCCESS != app_channel_epg_check_dialog())
        {
            GUI_SetInterface("flush",NULL);
        }

        if(thiz_book_param.exit_cb)
        {
            thiz_book_ret.ret = ret;
            thiz_book_param.exit_cb(&thiz_book_ret);
        }
    }

    return;
}

