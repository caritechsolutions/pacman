/*
 * =====================================================================================
 *       Filename:  app_pop_dialog.c
 *    Description:
 *        Version:  1.0
 *        Created:  20160821
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  B.Z.
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
// support 2 automatic text, can change ok, yes/no,
#include "app_pop.h"
#include "app_utility.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_channel_info.h"

#define TXT_TITLE               "txt_pop_dialog_title"
#define BTN_YES                 "btn_pop_dialog_yes"
#define BTN_OK                  "btn_pop_dialog_ok"
#define BTN_CANCEL              "btn_pop_dialog_cancel"
#define TXT_TIP                 "txt_pop_dialog_content"
#define TXT_TITILE              "txt_pop_dialog_title"
#define TXT_TIME                "txt_pop_dialog_time"


#define SEC_PER_DAY             (60 * 60 * 24)

static PopDlgRet       thiz_dialog_return           = POP_VAL_CANCEL;
static PopDlgRet       thiz_dialog_return_temp      = POP_VAL_OK;
static PopDlgEx        thiz_dialog_param            = {0};
static const char      *thiz_focus_wnd              = NULL;
static event_list      *thiz_dialog_timer           = NULL;
static char            *thiz_str                    = NULL;
//static bool            thiz_dialog_destroy_flag     = false;

SIGNAL_HANDLER  int app_pop_dialog_set_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;

	event = (GUI_Event *)usrdata;
	msg = (GxMessage*)event->msg.service_msg;

	if(GXMSG_PLAYER_SPEED_REPORT == msg->msg_id || GXMSG_PLAYER_STATUS_REPORT == msg->msg_id)
	{
        GUI_SendEvent(thiz_focus_wnd, event);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_dialog_create(GuiWidget *widget, void *usrdata)
{
#define WND_POPTIP_X       408
#define WND_POPTIP_Y       152

    int32_t pos_x = 0;
    int32_t pos_y = 0;

    if((POP_TYPE_NO_BTN == thiz_dialog_param.type)
            || (POP_TYPE_WAIT == thiz_dialog_param.type)
            || (POP_TYPE_NO_EXIT == thiz_dialog_param.type))
    {
        GUI_SetProperty(BTN_YES,    "state", "hide");
        GUI_SetProperty(BTN_OK,     "state", "hide");
        GUI_SetProperty(BTN_CANCEL, "state", "hide");
    }
    else if(POP_TYPE_OK == thiz_dialog_param.type)
    {
        GUI_SetProperty(BTN_YES,    "state", "hide");
        GUI_SetProperty(BTN_CANCEL, "state", "hide");
        GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
        GUI_SetFocusWidget(BTN_OK);
    }
    else
    {// yes/no
#if 0
        if (thiz_dialog_return_temp == POP_VAL_OK)
        {
            GUI_SetProperty(BTN_CANCEL, "state", "hide");
            GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
            GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
            GUI_SetFocusWidget(BTN_OK);
        }
        else
#endif
        {
            GUI_SetProperty(BTN_OK,     "state", "hide");
            GUI_SetProperty(BTN_YES, "string", STR_ID_YES);
            GUI_SetProperty(BTN_CANCEL, "string", STR_ID_NO);
            GUI_SetFocusWidget(BTN_CANCEL);
        }
    }

    if((thiz_dialog_param.pos.x == 0) && (thiz_dialog_param.pos.y == 0))
    {
        pos_x = WND_POPTIP_X;
        pos_y = WND_POPTIP_Y;
    }
    else
    {
        pos_x = thiz_dialog_param.pos.x;
        pos_y = thiz_dialog_param.pos.y;
    }
    GUI_SetProperty(WND_POP_DIALOG, "move_window_x", &pos_x);
    GUI_SetProperty(WND_POP_DIALOG, "move_window_y", &pos_y);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_dialog_destroy(GuiWidget *widget, void *usrdata)
{
    if(thiz_dialog_timer != NULL)
    {
        remove_timer(thiz_dialog_timer);
        thiz_dialog_timer = NULL;
    }
#if 0
    thiz_dialog_return_temp = POP_VAL_OK;
#endif
    if(thiz_str != NULL)
    {
        GxCore_Free(thiz_str);
        thiz_str = NULL;
    }
#if 0
    GUI_SetProperty(TXT_TIME, "string", " ");
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_dialog_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                timer_stop(thiz_dialog_timer);
                thiz_dialog_return = POP_VAL_CANCEL;

                GUI_EndDialog(WND_POP_DIALOG);
                if(NULL != thiz_dialog_param.exit_cb)
                {
                    GUI_SetInterface("flush",NULL);
                    thiz_dialog_param.exit_cb(thiz_dialog_return);
                }
                //ret = EVENT_TRANSFER_KEEPON;
                break;

            case STBK_OK:
                if ((thiz_dialog_param.type == POP_TYPE_YES_NO)
                        || (thiz_dialog_param.type == POP_TYPE_OK))
                {
                    timer_stop(thiz_dialog_timer);
                    thiz_dialog_return = thiz_dialog_return_temp;

                    GUI_EndDialog(WND_POP_DIALOG);
                    if(NULL != thiz_dialog_param.exit_cb)
                    {
                        GUI_SetInterface("flush",NULL);
                        thiz_dialog_param.exit_cb(thiz_dialog_return);
                    }
                }
                break;
            case STBK_LEFT:
            case STBK_RIGHT:
                if(thiz_dialog_param.type == POP_TYPE_YES_NO)
                {
                    if (thiz_dialog_return_temp == POP_VAL_CANCEL)
                    {
                        thiz_dialog_return_temp = POP_VAL_OK;
                        //GUI_SetProperty(BTN_YES, "string", STR_ID_YES);
                        //GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                        GUI_SetFocusWidget(BTN_YES);
                    }
                    else
                    {
                        thiz_dialog_return_temp = POP_VAL_CANCEL;
                        //GUI_SetProperty(BTN_YES, "string", STR_ID_OK);
                        //GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                        GUI_SetFocusWidget(BTN_CANCEL);
                    }
                }
                break;
            case STBK_MENU:
            case STBK_EXIT:
                if((POP_TYPE_NO_EXIT != thiz_dialog_param.type))
                {
                    timer_stop(thiz_dialog_timer);
                    thiz_dialog_return = POP_VAL_CANCEL;

                    GUI_EndDialog(WND_POP_DIALOG);
                    if(NULL != thiz_dialog_param.exit_cb)
                    {
                        GUI_SetInterface("flush",NULL);
                        thiz_dialog_param.exit_cb(thiz_dialog_return);
                    }
                }
                break;
            default:
                break;
        }
    }
    return ret;
}

static int pop_dialog_timeout_cb(void *usrdata)
{
    thiz_dialog_param.timeout_sec--;

    if(thiz_dialog_param.show_time == true)
    {
        if(thiz_str != NULL)
        {
            GxCore_Free(thiz_str);
            thiz_str = NULL;
        }
        thiz_str = app_time_to_hms_str(thiz_dialog_param.timeout_sec);
        GUI_SetProperty(TXT_TIME, "string", (void*)(thiz_str));
        GUI_SetProperty(TXT_TIME, "draw_now", NULL);
    }
    if(thiz_dialog_param.timeout_sec == 0)
    {
        timer_stop(thiz_dialog_timer);

        GUI_EndDialog(WND_POP_DIALOG);
        if(NULL != thiz_dialog_param.exit_cb)
        {
            GUI_SetInterface("flush",NULL);
            thiz_dialog_param.exit_cb(thiz_dialog_return);
        }
    }
    return 0;
}

int pop_dialog_create(PopDlgEx* dlg)
{
    time_t cur_time = 0;
   // const char *wnd = GUI_GetFocusWindow();
	//int min_count=0;
	thiz_focus_wnd = GUI_GetFocusWindow();

    if(NULL == dlg)
    {
        app_log_error("\nERROR, %s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(GUI_CheckDialog(WND_POP_DIALOG) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_POP_DIALOG);
        GUI_SetProperty(WND_POP_DIALOG, "draw_now", NULL);
        if(NULL != thiz_dialog_param.exit_cb)
        {
            thiz_dialog_param.exit_cb(thiz_dialog_return);
        }
    }
    memcpy(&thiz_dialog_param, dlg, sizeof(PopDlgEx));

    thiz_dialog_return = thiz_dialog_param.default_ret;
    thiz_dialog_return_temp = thiz_dialog_param.default_ret;

    GUI_CreateDialog(WND_POP_DIALOG);

// config the dialog format
    if(thiz_dialog_param.format == POP_FORMAT_DLG)
    {
        GUI_SetProperty(WND_POP_DIALOG, "format", "dialog");
    }
    else
    {
        GUI_SetProperty(WND_POP_DIALOG, "format", "popup");
    }

// content font size control
    if(thiz_dialog_param.font_default)
    {
        GUI_SetProperty(TXT_TIP, "font", thiz_dialog_param.font_default);
    }
    if(thiz_dialog_param.font_style)
    {
        GUI_SetProperty(TXT_TIP, "font", thiz_dialog_param.font_style);
    }

// set string display
    if((thiz_dialog_param.title != NULL)
            && (strlen(thiz_dialog_param.title) != 0))
    {
        GUI_SetProperty(TXT_TITILE, "string", (void*)(thiz_dialog_param.title));
    }
    if((thiz_dialog_param.str != NULL)
            && (strlen(thiz_dialog_param.str) != 0))
    {
        GUI_SetProperty(TXT_TIP, "string", (void*)(thiz_dialog_param.str));
    }

// set time display
    if(thiz_dialog_param.show_time == true)
    {
        if(thiz_dialog_param.timeout_sec == 0)
        {
            cur_time = g_AppTime.utc_get(&g_AppTime);
            cur_time = get_display_time_by_timezone(cur_time);
            cur_time %= SEC_PER_DAY;
            thiz_str = app_time_to_hourmin_edit_str(cur_time);
        }
        else
        {
            thiz_str = app_time_to_hms_str(thiz_dialog_param.timeout_sec);
        }

        GUI_SetProperty(TXT_TIME, "state", "show");
        GUI_SetProperty(TXT_TIME, "string", (void*)(thiz_str));
    }

// deal with the create callback
    if(NULL != thiz_dialog_param.create_cb)
    {
        thiz_dialog_param.create_cb();
    }

// deal with the timeout function
    if(thiz_dialog_param.timeout_sec == 0)
    {
        if(POP_TYPE_NO_BTN == thiz_dialog_param.type)
        {
            GUI_EndDialog(WND_POP_DIALOG);
            if(NULL != thiz_dialog_param.exit_cb)
            {
                GUI_SetInterface("flush",NULL);
                thiz_dialog_param.exit_cb(thiz_dialog_return);
            }
        }
    }
    else
    {
        if(thiz_dialog_timer != NULL)
        {
            remove_timer(thiz_dialog_timer);
            thiz_dialog_timer = NULL;
        }
        thiz_dialog_timer = create_timer(pop_dialog_timeout_cb, SEC_TO_MILLISEC,  NULL, TIMER_REPEAT);
    }

    return 0;
}

void pop_dialog_destroy(void)
{
    if(GUI_CheckDialog(WND_POP_DIALOG) == GXCORE_SUCCESS)
    {
        timer_stop(thiz_dialog_timer);
        GUI_EndDialog(WND_POP_DIALOG);
        if(NULL != thiz_dialog_param.exit_cb)
        {
            GUI_SetInterface("flush",NULL);
            thiz_dialog_param.exit_cb(thiz_dialog_return);
        }
    }
}


