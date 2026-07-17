#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_iptv.h"
#include "app_windows.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include <gx_apps.h>

#define IPTV_BROWSER_TITLE "text_iptv_browser_title"
#define IPTV_BROWSER_LOGO "img_iptv_browser_logo"

#define MAX_ITEM_NUM (6)

static char *s_text_video_title[MAX_ITEM_NUM] =
{
	"text_iptv_browser_item_title1",
	"text_iptv_browser_item_title2",
	"text_iptv_browser_item_title3",
	"text_iptv_browser_item_title4",
	"text_iptv_browser_item_title5",
	"text_iptv_browser_item_title6",
};

static char *s_img_video_focus[MAX_ITEM_NUM] =
{
	"img_iptv_browser_item_focus1",
	"img_iptv_browser_item_focus2",
	"img_iptv_browser_item_focus3",
	"img_iptv_browser_item_focus4",
	"img_iptv_browser_item_focus5",
	"img_iptv_browser_item_focus6",
};

static char *s_img_video_unfocus[MAX_ITEM_NUM] =
{
	"img_iptv_browser_item_unfocus1",
	"img_iptv_browser_item_unfocus2",
	"img_iptv_browser_item_unfocus3",
	"img_iptv_browser_item_unfocus4",
	"img_iptv_browser_item_unfocus5",
	"img_iptv_browser_item_unfocus6",
};

static char *iptv_browser_title = NULL;
static char *iptv_browser_logo = NULL;
static int s_page_prog_num = 0;
static int s_page_prog_sel = 0;
static IptvItem s_page_iptv_item[MAX_ITEM_NUM];

static void app_iptv_browser_update_title(char *text, char *logo)
{
	if(text)
	{
		GUI_SetProperty(IPTV_BROWSER_TITLE, "string", text);
	}
	if(logo)
	{
		GUI_SetProperty(IPTV_BROWSER_LOGO, "img", logo);
	}
}

static void iptv_browser_show_item(int index, IptvItem *iptv_item)
{
	if((index >= MAX_ITEM_NUM) || (index < 0) || (iptv_item == NULL))
		return;

	//title
	if(iptv_item->item_name != NULL)
	{
		GUI_SetProperty(s_text_video_title[index], "string", iptv_item->item_name);
		GUI_SetProperty(s_text_video_title[index], "state", "show");
	}

	//pic
	if(iptv_item->item_pic != NULL)
	{
		GUI_SetProperty(s_img_video_unfocus[index], "img", iptv_item->item_pic);
		GUI_SetProperty(s_img_video_unfocus[index], "state", "show");
	}
}

static void iptv_browser_hide_item(int index)
{
	if((index >= MAX_ITEM_NUM) || (index < 0))
		return;

	GUI_SetProperty(s_text_video_title[index], "state", "hide");
	GUI_SetProperty(s_img_video_unfocus[index], "state", "hide");
}

static void iptv_browser_focus_item(int index)
{
    if((index >= 0) && (index < MAX_ITEM_NUM))
    {
        GUI_SetProperty(s_img_video_focus[index], "state", "show");
    }
}

static void iptv_browser_unfocus_item(int index)
{
    if((index >= 0) && (index < MAX_ITEM_NUM))
    {
        GUI_SetProperty(s_img_video_focus[index], "state", "hide");
    }
}

static status_t app_iptv_page_item_update(int iptv_num, IptvItem *iptv_item)
{
	int i = 0;
	int cur_sel = 0;

	cur_sel = s_page_prog_sel;
	iptv_browser_unfocus_item(cur_sel);
	for(i = 0; i < iptv_num; i++)
	{
		iptv_browser_show_item(i, iptv_item + i);
	}
	for(; i < MAX_ITEM_NUM; i++)
	{
		iptv_browser_hide_item(i);
	}

	if(cur_sel >= iptv_num)
	{
		cur_sel = iptv_num - 1;
	}

	iptv_browser_focus_item(cur_sel);

	if(cur_sel != s_page_prog_sel)
	{
		s_page_prog_sel = cur_sel;
	}

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_iptv_browser_create(GuiWidget *widget, void *usrdata)
{
	app_iptv_browser_update_title(iptv_browser_title, iptv_browser_logo);
	app_iptv_page_item_update(s_page_prog_num, s_page_iptv_item);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_browser_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

static void _iptv_up_keypress(void)
{
	int val = MAX_ITEM_NUM / 2;

	if((s_page_prog_sel >= val) && (s_page_prog_sel < MAX_ITEM_NUM))
	{
		iptv_browser_unfocus_item(s_page_prog_sel);
		s_page_prog_sel -= val;
		iptv_browser_focus_item(s_page_prog_sel);
	}
}

static void _iptv_down_keypress(void)
{
	int val = MAX_ITEM_NUM / 2;
	int sel = s_page_prog_sel + val;
	if(sel < s_page_prog_num)
	{
		iptv_browser_unfocus_item(s_page_prog_sel);
		s_page_prog_sel = sel;
		iptv_browser_focus_item(s_page_prog_sel);
	}
}

static void _iptv_left_keypress(void)
{
	if(s_page_prog_sel > 0)
	{
		iptv_browser_unfocus_item(s_page_prog_sel);
		s_page_prog_sel--;
		iptv_browser_focus_item(s_page_prog_sel);
	}
}

static void _iptv_right_keypress(void)
{
	if((s_page_prog_sel >= 0)&&(s_page_prog_sel < s_page_prog_num - 1))
	{
		iptv_browser_unfocus_item(s_page_prog_sel);
		s_page_prog_sel++;
		iptv_browser_focus_item(s_page_prog_sel);
	}
}

SIGNAL_HANDLER int app_iptv_browser_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_EXIT:
            case STBK_MENU:
    			GUI_EndDialog(WND_IPTV_BROWSER);
                break;

            case STBK_OK:
            	if(s_page_iptv_item[s_page_prog_sel].item_func)
            	{
            		s_page_iptv_item[s_page_prog_sel].item_func();
            	}
                break;

            case STBK_UP:
                _iptv_up_keypress();
                break;

            case STBK_DOWN:
                _iptv_down_keypress();
                break;

            case STBK_LEFT:
                _iptv_left_keypress();
                break;

            case STBK_RIGHT:
                _iptv_right_keypress();
                break;

            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

void app_create_iptv_browser_menu(char *title_txt, char *logo, int item_num, IptvItem *item_data)
{
	s_page_prog_sel = 0;
	iptv_browser_title = title_txt;
	iptv_browser_logo = logo;
	s_page_prog_num = item_num;
	if(s_page_prog_num>MAX_ITEM_NUM)
	{
		s_page_prog_num = MAX_ITEM_NUM;
	}
	memset(s_page_iptv_item, 0, sizeof(IptvItem) * MAX_ITEM_NUM);
	memcpy(s_page_iptv_item, item_data, sizeof(IptvItem) * s_page_prog_num);

	if(GUI_CheckDialog(WND_IPTV_BROWSER) == GXCORE_ERROR)
	{
		GUI_CreateDialog(WND_IPTV_BROWSER);
	}
}

#endif
