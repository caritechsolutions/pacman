/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_ui_message.c
* Author    :	zhangyg
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  2.0	   2018.10.30		 zhangyg			  Creation
*****************************************************************************/
#include "app.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam_ui_common.h"
#include "cascam_ui_porting.h"

#define WND_CG_MESSAGE          "wnd_cryptoguard_message"
#define XML_CG_MESSAGE          "ca/cryptoguardcas/wnd_cryptoguard_message.xml"
#define NOTEPAD_CG_MESSAGE      "notepad_message_detail"
#define BUTTON_CG_MESSAGE       "btn_message_exit"
#define SCROLLBAR_CG_MESSAGE       "scrollbar_cg_message"

static CascamUiNoticeClass thiz_message_record;
static uint8_t thiz_message_force_display = 0;
static event_list  *message_timer = NULL;

void cascam_ui_exit_msg_ex_dialog(void);

static int message_dialog_timeout_cb(void *usrdata)
{
    cascam_timer_stop(message_timer);
    //cascam_remove_timer(message_timer);
    //message_timer = NULL;
    cascam_ui_exit_msg_ex_dialog();

    return 0;
}

void cascam_ui_create_msg_ex_dialog(CascamUiNoticeClass *para, uint8_t force)
{
    static char init_flag = 0;
    int ret = -1;

    if (0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_CG_MESSAGE, XML_CG_MESSAGE);
    }

    if ((NULL == para) || (NULL == para->str))
    {
        printf("\nCAS, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    if (GUI_CheckDialog(WND_CG_MESSAGE) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_CG_MESSAGE);
    }

    memcpy(&thiz_message_record, para, sizeof(CascamUiNoticeClass));
    thiz_message_force_display = force;

    ret = GUI_CreateDialog(WND_CG_MESSAGE);
    if (ret != 0)
    {
        printf("cascam_ui_create_msg_ex_dialog() error. \n");
        return;
    }

	if (para->timeout_sec > 0)
    {
		if (NULL != message_timer)
        {
			cascam_timer_stop(message_timer);
			//cascam_remove_timer(message_timer);
			//message_timer = NULL;
		}
        if(cascam_reset_timer(message_timer,  para->timeout_sec * 1000) != 0)
        {
            message_timer = cascam_create_timer(message_dialog_timeout_cb, para->timeout_sec * 1000, NULL, TIMER_ONCE);
        }

	}

    return;
}

void cascam_ui_exit_msg_ex_dialog(void)
{
    if (GUI_CheckDialog(WND_CG_MESSAGE) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_CG_MESSAGE);
    }

    memset(&thiz_message_record, 0, sizeof(CascamUiNoticeClass));
    thiz_message_force_display = 0;
}

SIGNAL_HANDLER int app_cg_message_create(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(NOTEPAD_CG_MESSAGE, "string", thiz_message_record.str);

    if (thiz_message_force_display != 0)
    {
        GUI_SetProperty(BUTTON_CG_MESSAGE, "string", "FORCED MESSAGE");
    }
	if(strlen(thiz_message_record.str) > 128)//means long message
	{
        GUI_SetProperty(SCROLLBAR_CG_MESSAGE, "format", "scroll_show");
	}

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cg_message_destroy(GuiWidget *widget, void *usrdata)
{
    memset(&thiz_message_record, 0, sizeof(CascamUiNoticeClass));
    thiz_message_force_display = 0;

    return EVENT_TRANSFER_STOP;
}

static void app_cg_message_details_keypress(uint16_t key)
{
	uint32_t value = 1;

    if (NULL != message_timer)
    {
        cascam_reset_timer(message_timer, 0);
    }

	switch(key)
	{
		case STBK_PAGE_UP:
			GUI_SetProperty(NOTEPAD_CG_MESSAGE, "line_up", &value);
			break;

		case STBK_PAGE_DOWN:
			GUI_SetProperty(NOTEPAD_CG_MESSAGE, "line_down", &value);
			break;

		default:
			break;
	}

    return;
}

SIGNAL_HANDLER int app_cg_message_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode, event->key.sym))
            {
                case STBK_EXIT:
                case STBK_MENU:
                case STBK_OK:
                {
                    if (thiz_message_force_display != 0)
                    {
                        return ret;
                    }

                    cascam_ui_exit_msg_ex_dialog();
                }
                break;

                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                    app_cg_message_details_keypress(find_virtualkey_ex(event->key.scancode, event->key.sym));
                    break;

                default:
                {
                    static GUI_Event global_event;

                    memset(&global_event, 0, sizeof(global_event));
                    global_event.type = event->type;
                    global_event.key.type = event->key.type;
                    global_event.key.scancode = event->key.scancode;
                    global_event.key.sym = event->key.sym;
                    GUI_SendEvent("wnd_full_screen", &global_event);
                }
                break;
            }

        default:
            break;
    }

    return ret;
}

#endif
