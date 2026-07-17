#include "app.h"
#if NET_SEARCH_SUPPORT
#include "app_common_search_setting.h"
#include "app_net_search_list.h"
#include "module/app_frontend.h"
#include "app_net_search_api.h"
#include "app_default_params.h"
#include "app_net_search_api.h"
#include "app_wnd_file_list.h"
#include "app_volume_value.h"
#include "app_wnd_search.h"
#include "module/app_log.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_module.h"
#include "app_book.h"

#define SERVER_LIST_TITLE	"text_server_list_title"
CSLOps ThisListCtrl = {0};

SIGNAL_HANDLER int32_t app_server_list_create(GuiWidget *widget, void *usrdata)
{
	if(ThisListCtrl.ListTitle != NULL)
	{
		GUI_SetProperty(SERVER_LIST_TITLE, "string", ThisListCtrl.ListTitle);
		GUI_SetProperty(SERVER_LIST_TITLE, "state", "show");
	}
	else
	{
		GUI_SetProperty(SERVER_LIST_TITLE, "state", "hide");
	}
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_server_list_destroy(GuiWidget *widget, void *usrdata)
{
	if(ThisListCtrl.ExitCb != NULL)
	{
		ThisListCtrl.ExitCb();
	}
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_server_list_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int32_t app_server_list_list_create(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty("list_server_list", "select", &ThisListCtrl.ListSel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_server_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                break;
            case STBK_EXIT:
            case STBK_MENU:
                    GUI_EndDialog(WND_SERVER_LIST);
                    GUI_SetInterface("flush", NULL);
                break;
            case STBK_OK:
            case STBK_RED:
            case STBK_GREEN:
            case STBK_YELLOW:
            case STBK_BLUE:
				if(ThisListCtrl.KeypressCb != NULL)
				{
					int list_sel = 0;
					GUI_GetProperty("list_server_list", "select", &list_sel);
					ThisListCtrl.KeypressCb(find_virtualkey_ex(event->key.scancode,event->key.sym), list_sel);
				}
                break;
        }
    }
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int32_t app_server_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	if(ThisListCtrl.GetTotalCb != NULL)
	{
		return ThisListCtrl.GetTotalCb();
	}
	else
		return 0;
}

SIGNAL_HANDLER int32_t app_server_list_list_get_data(GuiWidget *widget, void *usrdata)
{
	if(ThisListCtrl.GetDataCb != NULL)
	{
		ThisListCtrl.GetDataCb(usrdata);
	}
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int32_t app_server_list_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

int32_t app_csl_dialog_create(CSLOps *opt)
{
    int32_t ret = -1;
	if(opt == NULL)
		return ret;

	memcpy(&ThisListCtrl, opt, sizeof(CSLOps));
	if(GUI_CheckDialog(WND_SERVER_LIST) != GXCORE_SUCCESS)
	{
		ret = GUI_CreateDialog(WND_SERVER_LIST);
	}
	if(ThisListCtrl.CreateCb != NULL)
	{
		ret = ThisListCtrl.CreateCb();
	}
	return ret;
}

void app_csl_list_row_updata(int updata_row)
{
	GUI_SetProperty("list_server_list", "update_row", &updata_row);
}

void app_csl_list_updata(void)
{
	GUI_SetProperty("list_server_list", "update_all", NULL);
}

#endif
