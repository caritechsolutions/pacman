#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include "app_iptv.h"
#include <gx_apps.h>
#include "app_windows.h"
#include "app_common_select.h"
#include "app_keyboard_language.h"


#define MAX_IPTV_PROG_NAME 32
#define FIND_LISTVIEW     g_CommonSelect.widget.list
static int sFindNum = 0;
extern void app_iptv_play_ctrl(IPTVPlayChoice choice, int num);
extern IptvListClass *app_iptv_getIptvList(void);
extern IptvListClass *iptv_get_list_by_key(IptvListClass *list, char *key);
extern int iptv_get_list_play_no(IptvListClass *list, IptvItemClass *item);

/* Private variable------------------------------------------------------- */
/* widget macro -------------------------------------------------------- */

static IptvListClass *iptv_find_channellist = NULL;
static IptvListClass *iptv_channellist = NULL;

IptvListClass *app_iptv_get_list_by_key(char *FindKey)
{
    IptvListClass *iptvList = NULL;

    iptvList = app_iptv_getIptvList();

    if (iptvList && iptvList->iptv_item_list)
        return iptv_get_list_by_key(iptvList, FindKey);
    else
        return NULL;
}
status_t app_iptv_find_play(IptvItemClass *iptvChannel, int channelId)
{
    int i = 0;
    IptvListClass *iptvlist = NULL;

    if (NULL == iptvChannel)
    {
        return GXCORE_ERROR;
    }

    iptvlist = app_iptv_getIptvList();
    i = iptv_get_list_play_no(iptvlist, iptvChannel);
    if (i < 0)
        return GXCORE_ERROR;

    app_iptv_play_ctrl(IPTV_PLAY_LIST, i + 1);
    return GXCORE_SUCCESS;
}

static void iptv_find_keyboard_release_cb(PopKeyboard *data)
{
    if ((data->in_ret == POP_VAL_CANCEL) || (data->out_name == NULL) || (sFindNum == 0))
    {
        GUI_EndDialog(g_CommonSelect.widget.wnd);
    }
}

static void iptv_find_keyboard_change_cb(PopKeyboard *data)
{
    int cur_sel = 0;
    if (iptv_find_channellist)
    {
        iptv_list_free(iptv_find_channellist);
        iptv_find_channellist = NULL;
    }

    iptv_find_channellist = app_iptv_get_list_by_key(data->out_name);
    if(!strcmp(data->out_name,""))
    {
        iptv_channellist = app_iptv_getIptvList();
    }
    else
    {
        iptv_channellist = NULL;
    }
    if (iptv_find_channellist && iptv_find_channellist->iptv_item_list)
        sFindNum = iptv_find_channellist->iptv_item_num;
    else
        sFindNum = 0;

    if (0 < sFindNum)
    {
        GUI_SetProperty(FIND_LISTVIEW, "update_all", NULL);
        cur_sel = 0;
        GUI_SetProperty(FIND_LISTVIEW, "select", (void *)(&cur_sel));
    }
    else
    {
        GUI_SetProperty(FIND_LISTVIEW, "update_all", NULL);
        GUI_SetProperty(FIND_LISTVIEW, "select", (void *)(&cur_sel));
    }

    GUI_SetProperty(WND_KEYBOARD_LANGUAGE, "update", NULL);
}

int app_iptv_find_create(GuiWidget *widget, void *usrdata)
{
    static PopKeyboard keyboard;

    app_keyboard_save_cb(app_input_save_space_cb);
    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = NULL;
    keyboard.max_num = MAX_IPTV_PROG_NAME - 1;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = iptv_find_keyboard_change_cb;
    keyboard.release_cb = iptv_find_keyboard_release_cb;
    keyboard.usr_data   = NULL;
    keyboard.pos.x = APP_WND_FIND_KB_POS_X;
    keyboard.pos.y = APP_WND_FIND_KB_POS_Y;

    multi_language_keyboard_create(&keyboard);
    iptv_channellist = app_iptv_getIptvList();

    return EVENT_TRANSFER_STOP;
}

int app_iptv_find_destroy(GuiWidget *widget, void *usrdata)
{
    if (iptv_find_channellist)
    {
        iptv_list_free(iptv_find_channellist);
        iptv_find_channellist = NULL;
    }
	if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
	}
    return EVENT_TRANSFER_STOP;
}

int app_iptv_find_keypress(GuiWidget *widget, void *usrdata)
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
            switch (event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_DOWN:
                case STBK_UP:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                {
                    ret = EVENT_TRANSFER_STOP;
                }
                break;

                case STBK_EXIT:
                case STBK_MENU:
                {
                    GUI_EndDialog(g_CommonSelect.widget.wnd);
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

int app_iptv_find_got_focus(GuiWidget *widget, void *usrdata)
{

    return EVENT_TRANSFER_STOP;
}
int app_iptv_find_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

int app_iptv_find_list_get_total(GuiWidget *widget, void *usrdata)
{
    uint32_t total = 0;
    if(iptv_channellist)
    {
        total = iptv_channellist->iptv_item_num;
    }
    else
    {
        if (iptv_find_channellist && iptv_find_channellist->iptv_item_list)
            total = iptv_find_channellist->iptv_item_num;
        else
            total = 0;
    }
    return total;
}

int app_iptv_find_list_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)

    ListItemPara *item = NULL;
    static char buffer[CHANNEL_NUM_LEN];

    item = (ListItemPara *)usrdata;
    if (NULL == item)
        return GXCORE_ERROR;
    if (0 > item->sel)
        return GXCORE_ERROR;

    if((iptv_channellist == NULL) && ((NULL == iptv_find_channellist) || (iptv_find_channellist && (NULL == iptv_find_channellist->iptv_item_list))))
        return GXCORE_ERROR;

    //col-0: num
    item->x_offset = 2;
    item->image = NULL;
    memset(buffer, 0, CHANNEL_NUM_LEN);
    sprintf(buffer, " %d", (item->sel + 1));
    item->string = buffer;

    //col-1: channel name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    if(iptv_channellist)
    {
        item->string = (char *)iptv_channellist->iptv_item_list[item->sel].iptv_key;
    }
    else
    {
        item->string = (char *)iptv_find_channellist->iptv_item_list[item->sel].iptv_key;
    }
    return EVENT_TRANSFER_STOP;
}

int app_iptv_find_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t sel = 0;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
                case STBK_DOWN:
                case STBK_UP:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                {
                    ret = EVENT_TRANSFER_KEEPON;
                }
                break;

                case STBK_LEFT:
                case STBK_RIGHT:

                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_OK:
                {
                    GUI_GetProperty(FIND_LISTVIEW, "select", &sel);
                    if (iptv_find_channellist && iptv_find_channellist->iptv_item_list && (sel < iptv_find_channellist->iptv_item_num))
                    {
                        app_iptv_find_play(&iptv_find_channellist->iptv_item_list[sel], sel);
                    }

                    GUI_EndDialog(g_CommonSelect.widget.wnd);
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

void app_iptv_create_find_list(void)
{
    CommonSelectParas paras = {0};

    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = "Find";

    paras.signal.create = app_iptv_find_create;
    paras.signal.destroy = app_iptv_find_destroy;
    paras.signal.keypress = app_iptv_find_keypress;
    paras.signal.got_focus = app_iptv_find_got_focus;
    paras.signal.list_change = app_iptv_find_list_change;
    paras.signal.list_get_total = app_iptv_find_list_get_total;
    paras.signal.list_get_data = app_iptv_find_list_get_data;
    paras.signal.list_keypress = app_iptv_find_list_keypress;
    app_common_select_create_dialog(&paras);

}
#endif
