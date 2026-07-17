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

#include "app_pop.h"
#include "app_utility.h"
#include "app_module.h"
#include "app_windows.h"
#include "full_screen.h"
#include "app_channel_info.h"

#define TXT_TITLE           "txt_pop_tip_title"
#define IMG_OPT             "img_pop_tip_opt"
#define BMP_OK              "s_bar_choice_blue4"
#define BMP_CANCEL          "s_bar_choice_blue4_l"
#define BTN_OK              "btn_pop_tip_ok"
#define BTN_CANCEL          "btn_pop_tip_cancel"
#define TXT_TIP             "txt_pop_tip"
#define TXT_TITILE          "txt_pop_tip_title"
#define TXT_TIME             "txt_pop_time"


#define SEC_PER_DAY (60 * 60 * 24)

static PopDlgRet       s_PopDlgRet        = POP_VAL_CANCEL;
static PopDlgRet       s_RetTemp        = POP_VAL_OK;
static PopDlg s_PopDlgParam = {0};
static const char *s_focus_wnd = NULL;
static PopDlgDisplyType s_pop_displaytype = POP_DISPLAY_ON_BOT;

static enum
{
	POP_DLG_END,
	POP_DLG_START,
}s_PopDlgStatus = POP_DLG_END;

extern uint32_t app_diseqc12_get_tuner_id(void);

static event_list  *s_PopDlgTimer = NULL;
static char *s_time_str = NULL;
static bool s_pop_block_destroy_quick = false;
bool app_check_pop_block_state(void)
{
    return s_PopDlgStatus;
}

SIGNAL_HANDLER  int app_popdlg_set_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;

	event = (GUI_Event *)usrdata;
	msg = (GxMessage*)event->msg.service_msg;

	if(GXMSG_PLAYER_SPEED_REPORT == msg->msg_id || GXMSG_PLAYER_STATUS_REPORT == msg->msg_id)
	{
        //app_log_debug("s_focus_wnd = %s  %d %d\n",s_focus_wnd,msg->msg_id,GXMSG_PLAYER_SPEED_REPORT);
        if(0 == strcasecmp(WIN_MEDIA_BAR, s_focus_wnd))
        {
            //if send to win_media_bar, maybe this window has been destroyed#bug71293
            GUI_SendEvent(WIN_MOVIE_VIEW, event);
        }
        else if(0 != strcasecmp(WND_POP_TIP, s_focus_wnd))
            GUI_SendEvent(s_focus_wnd, event);
	}
	return EVENT_TRANSFER_STOP;
}
//#endif

SIGNAL_HANDLER int app_popdlg_create(GuiWidget *widget, void *usrdata)
{
    int32_t pos_x = 0;
    int32_t pos_y = 0;

    if(POP_TYPE_NO_BTN == s_PopDlgParam.type || POP_TYPE_WAIT == s_PopDlgParam.type || POP_TYPE_NO_EXIT == s_PopDlgParam.type)
    {
        GUI_SetProperty(IMG_OPT,    "state", "hide");
        GUI_SetProperty(BTN_OK,     "state", "hide");
        GUI_SetProperty(BTN_CANCEL, "state", "hide");
    }
    else if(POP_TYPE_OK == s_PopDlgParam.type)
    {
        GUI_SetProperty(BTN_CANCEL, "state", "hide");
        GUI_SetProperty(IMG_OPT, "img", BMP_OK);
        GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
        GUI_SetFocusWidget(BTN_OK);
    }
    else
    {
        if (s_RetTemp == POP_VAL_OK)
        {
            GUI_SetProperty(IMG_OPT, "img", BMP_OK);
            GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
            GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
            GUI_SetFocusWidget(BTN_OK);
        }
        else
        {
            GUI_SetProperty(IMG_OPT, "img", BMP_CANCEL);
            GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
            GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
            GUI_SetFocusWidget(BTN_CANCEL);
        }
    }

    if((s_PopDlgParam.pos.x == 0) && (s_PopDlgParam.pos.y == 0))
    {
        pos_x = APP_WND_POP_TIP_POS_X;
        pos_y = APP_WND_POP_TIP_POS_Y;
    }
    else
    {
        pos_x = s_PopDlgParam.pos.x;
        pos_y = s_PopDlgParam.pos.y;
    }
    GUI_SetProperty(WND_POP_TIP, "move_window_x", &pos_x);
    GUI_SetProperty(WND_POP_TIP, "move_window_y", &pos_y);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_popdlg_destroy(GuiWidget *widget, void *usrdata)
{
    if(s_PopDlgTimer != NULL)
    {
        remove_timer(s_PopDlgTimer);
        s_PopDlgTimer = NULL;
    }
    s_RetTemp = POP_VAL_OK;

    if(s_time_str != NULL)
    {
        GxCore_Free(s_time_str);
        s_time_str = NULL;
    }
    GUI_SetProperty(TXT_TIME, "string", " ");
    app_set_popdlg_display_type(POP_DISPLAY_ON_BOT);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_popdlg_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                popdlg_destroy();
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_OK:
                if ((s_PopDlgParam.type == POP_TYPE_YES_NO)
                        || (s_PopDlgParam.type == POP_TYPE_OK))
                {
                    timer_stop(s_PopDlgTimer);
                    s_PopDlgRet = s_RetTemp;
                    s_PopDlgStatus = POP_DLG_END;
                    if(s_PopDlgParam.mode == POP_MODE_UNBLOCK)
                    {
                        GUI_EndDialog(WND_POP_TIP);
                        if(NULL != s_PopDlgParam.exit_cb)
                        {
                            s_PopDlgParam.exit_cb(s_PopDlgRet);
                        }
                    }
                }

                break;

            case STBK_LEFT:
            case STBK_RIGHT:
                if(s_PopDlgParam.type == POP_TYPE_YES_NO)
                {
                    if (s_RetTemp == POP_VAL_CANCEL)
                    {
                        s_RetTemp = POP_VAL_OK;
                        GUI_SetProperty(IMG_OPT, "img", BMP_OK);
                        GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
                        GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                        GUI_SetFocusWidget(BTN_OK);
                    }
                    else
                    {
                        s_RetTemp = POP_VAL_CANCEL;
                        GUI_SetProperty(IMG_OPT, "img", BMP_CANCEL);
                        GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
                        GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                        GUI_SetFocusWidget(BTN_CANCEL);
                    }
                }
                break;

            case STBK_MENU:
            case STBK_EXIT:
                if(POP_TYPE_NO_EXIT != s_PopDlgParam.type)
                {
                    timer_stop(s_PopDlgTimer);
                    s_PopDlgRet = POP_VAL_CANCEL;
                    s_PopDlgStatus = POP_DLG_END;
                    if(s_PopDlgParam.mode == POP_MODE_UNBLOCK)
                    {
                        GUI_EndDialog(WND_POP_TIP);
                        if(NULL != s_PopDlgParam.exit_cb)
                        {
                            s_PopDlgParam.exit_cb(s_PopDlgRet);
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

extern event_list* text_auto_roll_timer;
static int pop_timeout_cb(void *usrdata)
{
    s_PopDlgParam.timeout_sec--;

    if(s_PopDlgParam.show_time == true)
    {
        if(s_time_str != NULL)
        {
            GxCore_Free(s_time_str);
            s_time_str = NULL;
        }
        s_time_str = app_time_to_hms_str(s_PopDlgParam.timeout_sec);
        GUI_SetProperty(TXT_TIME, "string", (void*)(s_time_str));
        GUI_SetProperty(TXT_TIME, "draw_now", NULL);
    }

    if(s_PopDlgParam.timeout_sec == 0)
    {
        timer_stop(s_PopDlgTimer);
        s_PopDlgStatus = POP_DLG_END;
        if(s_PopDlgParam.mode == POP_MODE_UNBLOCK)
        {
            GUI_EndDialog(WND_POP_TIP);
			GUI_SetInterface("flush",NULL);
             if(NULL != s_PopDlgParam.exit_cb)
			 {
				 GUI_SetInterface("flush",NULL);
                s_PopDlgParam.exit_cb(s_PopDlgRet);
			 }
        }
    }

#if 0
	if(GUI_CheckDialog(WIN_TEXT_VIEW) == GXCORE_SUCCESS)
	{
		reset_timer(text_auto_roll_timer);
	}
#endif

    return 0;
}
/*
just for factory test
*/
void FactoryTest_EndDialog(void)
{
	GUI_EndDialog(WND_POP_TIP);
}


PopDlgRet popdlg_create(PopDlg* dlg)
{
    time_t cur_time = 0;
    const char *wnd = GUI_GetFocusWindow();
	s_focus_wnd = NULL;
	s_focus_wnd = GUI_GetFocusWindow();

    if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
    {
        s_PopDlgStatus = POP_DLG_END;
        if(s_PopDlgParam.mode == POP_MODE_BLOCK)
        {
            s_pop_block_destroy_quick = true;
            app_block_msg_init(g_app_msg_self);
        }
        GUI_EndDialog(WND_POP_TIP);
        GUI_SetProperty(WND_POP_TIP, "draw_now", NULL);
        if(NULL != s_PopDlgParam.exit_cb)
            s_PopDlgParam.exit_cb(s_PopDlgRet);
    }

    if((wnd != NULL) && ((strcmp(wnd, WND_FULL_SCREEN) == 0) || (app_channel_info_compare_dialog(wnd) == 0)))
    {
        app_clean_fullscreen_tip(false, false, false);
    }

#if 0
	if(GUI_CheckDialog(WIN_TEXT_VIEW) == GXCORE_SUCCESS)
	{
		timer_stop(text_auto_roll_timer);
	}
#endif

    memcpy(&s_PopDlgParam, dlg, sizeof(PopDlg));

    s_PopDlgStatus = POP_DLG_START;
    s_PopDlgRet = s_PopDlgParam.default_ret;
    s_RetTemp = s_PopDlgParam.default_ret;

    GUI_CreateDialog(WND_POP_TIP);

    if(s_PopDlgParam.format == POP_FORMAT_DLG)
    {
        GUI_SetProperty(WND_POP_TIP, "format", "dialog");
    }
    else
    {
        GUI_SetProperty(WND_POP_TIP, "format", "popup");
    }

    if(s_PopDlgParam.title != NULL)
        GUI_SetProperty(TXT_TITILE, "string", (void*)(s_PopDlgParam.title));
    if(s_PopDlgParam.str != NULL)
        GUI_SetProperty(TXT_TIP, "string", (void*)(s_PopDlgParam.str));

    if(s_time_str != NULL)
    {
        GxCore_Free(s_time_str);
        s_time_str = NULL;
    }
    if(s_PopDlgParam.show_time == true)
    {
        if(s_PopDlgParam.timeout_sec == 0)
        {
            cur_time = g_AppTime.utc_get(&g_AppTime);
            cur_time = get_display_time_by_timezone(cur_time);
            cur_time %= SEC_PER_DAY;
            s_time_str = app_time_to_hourmin_edit_str(cur_time);
        }
        else
        {
            s_time_str = app_time_to_hms_str(s_PopDlgParam.timeout_sec);
        }

        GUI_SetProperty(TXT_TIME, "string", (void*)(s_time_str));
    }

	//GUI_SetInterface("flush",NULL);

    if(NULL != s_PopDlgParam.create_cb)
        s_PopDlgParam.create_cb();

    if(s_PopDlgParam.timeout_sec == 0)
    {
        if(POP_TYPE_NO_BTN == s_PopDlgParam.type)
        {
            s_PopDlgStatus = POP_DLG_END;
            if(s_PopDlgParam.mode == POP_MODE_UNBLOCK)
            {
                GUI_EndDialog(WND_POP_TIP);
                if(NULL != s_PopDlgParam.exit_cb)
				{
					GUI_SetInterface("flush",NULL);
                    s_PopDlgParam.exit_cb(s_PopDlgRet);
				}

#if 0
				if(GUI_CheckDialog(WIN_TEXT_VIEW) == GXCORE_SUCCESS)
				{
					reset_timer(text_auto_roll_timer);
				}
#endif
            }
        }
    }
    else
    {
        if(s_PopDlgTimer != NULL)
        {
            remove_timer(s_PopDlgTimer);
            s_PopDlgTimer = NULL;
        }

        if (reset_timer(s_PopDlgTimer) != 0)
        {
            s_PopDlgTimer = create_timer(pop_timeout_cb, SEC_TO_MILLISEC,  NULL, TIMER_REPEAT);
        }
    }

	if(s_PopDlgParam.mode == POP_MODE_BLOCK)
	{
		app_log_error("######## popdlg mode is POP_MODE_BLOCK, please modify POP_MODE_UNBLOCK \n");
		app_block_msg_destroy(g_app_msg_self);
		while(s_PopDlgStatus == POP_DLG_START)
		{
			GUI_LoopEvent();
			GxCore_ThreadDelay(50);
		}

		GUI_StartSchedule();
		if(s_pop_block_destroy_quick == true)
		{
			s_pop_block_destroy_quick = false;
		}
		else
		{
			app_block_msg_init(g_app_msg_self);
			GUI_EndDialog(WND_POP_TIP);
			if(NULL != s_PopDlgParam.exit_cb)
			{
				GUI_SetInterface("flush",NULL);
				s_PopDlgParam.exit_cb(s_PopDlgRet);
			}
		}
#if 0
		if(GUI_CheckDialog(WIN_TEXT_VIEW) == GXCORE_SUCCESS)
		{
			reset_timer(text_auto_roll_timer);
		}
#endif
	}
    return s_PopDlgRet;

}

void popdlg_destroy(void)
{
	if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
	{
		timer_stop(s_PopDlgTimer);
		s_PopDlgRet = POP_VAL_CANCEL;
		s_PopDlgStatus = POP_DLG_END;
		if(s_PopDlgParam.mode == POP_MODE_BLOCK)
		{
			s_pop_block_destroy_quick = true;
			app_block_msg_init(g_app_msg_self);
			GUI_EndDialog(WND_POP_TIP);
			GUI_SetInterface("flush",NULL);
			if(NULL != s_PopDlgParam.exit_cb)
			{
			    s_PopDlgParam.exit_cb(s_PopDlgRet);
			}
		}
		else
		{
			GUI_EndDialog(WND_POP_TIP);
			GUI_SetInterface("flush",NULL);
			if(NULL != s_PopDlgParam.exit_cb)
			{
			    s_PopDlgParam.exit_cb(s_PopDlgRet);
			}
		}
	}
}

void app_pop_reset_timeout(int timeout)
{
    if(s_PopDlgTimer != NULL)
    {
        s_PopDlgParam.timeout_sec = timeout;
        reset_timer(s_PopDlgTimer);
    }
    return;
}

void app_pop_set_string(const char* string)
{
    if(string == NULL)
        return;

    if( GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
    {
        GUI_SetProperty(TXT_TIP, "string", (void*)string);
        GUI_SetInterface("flush",NULL);
    }
    return;
}

void app_update_pop_dlg(const char* window_name)
{
    if (GUI_CheckDialog(window_name) == GXCORE_SUCCESS)
        GUI_SetProperty(window_name,"update",NULL);
}

void app_pop_set_exitcb(int (*Exit_Cb)(PopDlgRet))
{

    if( GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
        s_PopDlgParam.exit_cb = Exit_Cb;
    return;

}

PopDlgDisplyType app_get_popdlg_display_type(void)
{
    return s_pop_displaytype;
}

void app_set_popdlg_display_type(PopDlgDisplyType type)
{
    s_pop_displaytype = type;
}

void popdlg_update_status(PopDlg* dlg)
{
	#if 0
	int ret,state = -1;

	if( GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
	{
		GUI_CreateDialog(WND_POP_TIP);
	}

	if(dlg->str != NULL)
	{
		ret = GUI_SetProperty(TXT_TIP, "state","enable");
		ret = GUI_SetProperty(TXT_TIP, "state","show");
		ret = GUI_SetProperty(TXT_TIP, "string", (void*)dlg->str );
		ret = GUI_SetProperty(TXT_TIP, "draw_now", NULL);

		//GUI_GetProperty(TXT_TIP,"state",&state);
		//app_log_debug("in %s:%d state = %d\n",__func__,__LINE__,state);
	}
	#endif
	//....
}
