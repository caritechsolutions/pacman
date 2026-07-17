#include "app.h"
#ifdef IPTV_ACCOUNT_SUPPORT
#include "app_module.h"
#include "app_iptv.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include "app_windows.h"
#include <gx_apps.h>

#define MAX_ITEM_NUM (9)

static char *s_iptv_account_option[MAX_ITEM_NUM] =
{
	"text_iptv_account_option1",
	"text_iptv_account_option2",
	"text_iptv_account_option3",
	"text_iptv_account_option4",
	"text_iptv_account_option5",
	"text_iptv_account_option6",
	"text_iptv_account_option7",
	"text_iptv_account_option8",
	"text_iptv_account_option9",
};

static char *s_iptv_account_value[MAX_ITEM_NUM] =
{
	"text_iptv_account_value1",
	"text_iptv_account_value2",
	"text_iptv_account_value3",
	"text_iptv_account_value4",
	"text_iptv_account_value5",
	"text_iptv_account_value6",
	"text_iptv_account_value7",
	"text_iptv_account_value8",
	"text_iptv_account_value9",
};


static gx_list_t *iptv_account_info_list = NULL;

static void iptv_account_info_free(void)
{
	if(iptv_account_info_list)
	{
		gx_list_free(iptv_account_info_list);
		iptv_account_info_list = NULL;
	}
}

static void app_iptv_account_info_update(void)
{
	int i = 0;

	if(iptv_account_info_list&&(iptv_account_info_list->item_num>0))
	{
		for(i=0; (i<iptv_account_info_list->item_num)&&(i<MAX_ITEM_NUM); i++)
		{
			if(iptv_account_info_list->items[i].key)
			{
				GUI_SetProperty(s_iptv_account_option[i], "state", "show");
				GUI_SetProperty(s_iptv_account_option[i], "string", iptv_account_info_list->items[i].key);
			}
			if(iptv_account_info_list->items[i].value)
			{
				GUI_SetProperty(s_iptv_account_value[i], "state", "show");
				GUI_SetProperty(s_iptv_account_value[i], "string", iptv_account_info_list->items[i].value);
			}

		}
	}
}

SIGNAL_HANDLER int app_iptv_account_create(GuiWidget *widget, void *usrdata)
{
	app_iptv_account_info_update();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_account_destroy(GuiWidget *widget, void *usrdata)
{
	iptv_account_info_free();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_account_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                app_system_reset_unfocus_image();
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_EXIT:
            case STBK_MENU:
    			GUI_EndDialog(WND_IPTV_ACCOUNT);
                break;

            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

void app_create_iptv_account_infor_menu(gx_list_t *list)
{
	iptv_account_info_free();
	iptv_account_info_list = list;
	if(GUI_CheckDialog(WND_IPTV_ACCOUNT) == GXCORE_ERROR)
	{
		GUI_CreateDialog(WND_IPTV_ACCOUNT);
	}
}

#endif
