#include "app.h"
#ifdef IPTV_FAV_SUPPORT
#include "app_module.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include "app_iptv.h"
#include <gx_apps.h>
#include "app_windows.h"
#include "app_common_select.h"

#define MAX_IPTV_PROG_NAME 32
#define FAV_LISTVIEW     g_CommonSelect.widget.list
extern void app_iptv_play_ctrl(IPTVPlayChoice choice, int num);
extern  IptvIndexClass *app_iptv_getFavList(void);
extern IptvListClass *app_iptv_getAllList(void);
extern IptvIndexClass *app_iptv_getIndexList(int group);
extern int app_iptv_getCurGroupSelected(void);
extern int app_iptv_getCurIptvSelected(void);
extern int iptv_get_list_play_no(IptvListClass *list, IptvItemClass *item);
extern void app_net_video_change_focus_item(int index);
extern void app_iptv_changeGroupInList(int sel);
static int _press_green_key_return(void);

static int* s_fav_focus_item = NULL;
static int s_fav_list_enable = 0;
static int s_fav_cur_play = -1;

/* Private variable------------------------------------------------------- */
/* widget macro -------------------------------------------------------- */

status_t app_iptv_fav_play(IptvItemClass *iptvChannel, int channelId)
{
    int i = 0;
    IptvListClass *iptvlist = NULL;

    if(NULL == iptvChannel)
    {
        return GXCORE_ERROR;
    }

    iptvlist = app_iptv_getAllList();
    i = iptv_get_list_play_no(iptvlist, iptvChannel);
    if(i < 0)
        return GXCORE_ERROR;

    app_iptv_play_ctrl(IPTV_PLAY_LIST, i + 1);
    return GXCORE_SUCCESS;
}

int app_iptv_fav_create(GuiWidget *widget, void *usrdata)
{
    int i = 0;
    int active_sel = -1;
    int index = 0;

    if(s_fav_list_enable != 1)
    {
        IptvListClass *iptvList = app_iptv_getAllList();
        IptvIndexClass *favlist = app_iptv_getFavList();
        if(!favlist || !iptvList)
            return 0;
        s_fav_cur_play = -1;
        IptvIndexClass *curlist = app_iptv_getIndexList(app_iptv_getCurGroupSelected());
        if(!curlist)
            return 0;

        active_sel = app_iptv_getCurIptvSelected();
        for(i= 0 ; i < favlist->iptv_index_num; i++)
        {
            if(curlist->iptv_index[active_sel] == favlist->iptv_index[i])
            {
                s_fav_cur_play = i;
                break;
            }
        }
        iptv_indexlist_free(curlist);
        curlist = NULL;
        s_fav_list_enable = 1;
        if(s_fav_cur_play == -1)
        {
            s_fav_cur_play = 0;
            index = favlist->iptv_index[s_fav_cur_play];
            app_iptv_fav_play(&iptvList->iptv_item_list[index], index);
        }
    }
    GUI_SetProperty(FAV_LISTVIEW, "select", &s_fav_cur_play);
    GUI_SetProperty(FAV_LISTVIEW, "active", &s_fav_cur_play);
    *s_fav_focus_item = s_fav_cur_play;
    return EVENT_TRANSFER_STOP;
}

int app_iptv_fav_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

int app_iptv_fav_keypress(GuiWidget *widget, void *usrdata)
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

int app_iptv_fav_got_focus(GuiWidget *widget, void *usrdata)
{

    return EVENT_TRANSFER_STOP;
}
int app_iptv_fav_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

int app_iptv_fav_list_get_total(GuiWidget *widget, void *usrdata)
{
    int total = 0;
	IptvIndexClass *favlist = app_iptv_getFavList();

    if(favlist)
    {
        total = favlist->iptv_index_num;
    }
    return total;
}

int app_iptv_fav_list_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)

    ListItemPara *item = NULL;
    static char buffer[CHANNEL_NUM_LEN];
	IptvListClass *iptvList = app_iptv_getAllList();
	IptvIndexClass *favlist = app_iptv_getFavList();
	int index = 0;

    item = (ListItemPara *)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

	if(favlist)
	{
		if((favlist->iptv_index_num>0)&&(favlist->iptv_index_num>item->sel))
		{
			index = favlist->iptv_index[item->sel];
		}
		else
		{
			return GXCORE_ERROR;
		}
	}

    if(NULL == favlist)
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
    item->string = (char *)iptvList->iptv_item_list[index].iptv_key;

    return EVENT_TRANSFER_STOP;
}

static int _press_red_key_delate_fav(void)
{
	IptvIndexClass *favlist = NULL;
	int sel = -1;
	int index = -1;

	favlist = app_iptv_getFavList();
    GUI_GetProperty(FAV_LISTVIEW, "select", &sel);
    if(sel >= 0)
    {
        if((favlist->iptv_index_num>0)&&(favlist->iptv_index_num>sel))
        {
            index = favlist->iptv_index[sel];
        }
        if(index>=0)
        {
           if(sel == s_fav_cur_play)
            {
                _press_green_key_return();
                iptv_indexlist_del_item(favlist, index);
                return GXCORE_SUCCESS;
            }
            iptv_indexlist_del_item(favlist, index);
            if(sel < s_fav_cur_play)
            {
                s_fav_cur_play--;
                app_net_video_change_focus_item(s_fav_cur_play);
            }

            if(sel >= favlist->iptv_index_num)
            {
                sel = favlist->iptv_index_num-1;
                GUI_SetProperty(FAV_LISTVIEW, "select", &sel);
            }
            GUI_SetProperty(FAV_LISTVIEW, "update_all", NULL);
            GUI_SetProperty(FAV_LISTVIEW, "active", &s_fav_cur_play);
        }
    }
    return GXCORE_SUCCESS;
}

int app_fav_item_to_all_index(int item)
{
    int index = 0;
    int i = 0;
    int favIndex = 0;
	IptvListClass *iptvList = app_iptv_getAllList();
	IptvIndexClass *favlist = app_iptv_getFavList();
    int curGroup = app_iptv_getCurGroupSelected();
    IptvIndexClass *curlist = (curGroup == 0) ? NULL : app_iptv_getIndexList(curGroup);

    if(!favlist || !iptvList)
        return item;
    s_fav_cur_play = item;
    if(curGroup == 0)
    {
        index = favlist->iptv_index[item];
        if(iptvList && iptvList->iptv_item_list && (index < iptvList->iptv_item_num))
        {
            return iptv_get_list_play_no(iptvList, &iptvList->iptv_item_list[index]);
        }
    }
    else
    {
        favIndex = favlist->iptv_index[item];
        if (!curlist)
        {
            return item;
        }

        for (i = 0; i < curlist->iptv_index_num; i++) {
            if (curlist->iptv_index[i] == favIndex) {
                index = i;
                break;
            }
        }

        if (index != -1 && iptvList->iptv_item_list && index < iptvList->iptv_item_num) {
            return iptv_get_list_play_no(iptvList, &iptvList->iptv_item_list[index]);
        }
    }
    return item;
}

static int  _press_ok_fav_play(void)
{
	IptvListClass *iptvList = app_iptv_getAllList();
	IptvIndexClass *favlist = app_iptv_getFavList();
	int index = 0;
    int  sel = -1;
    int  active_sel = -1;

	GUI_GetProperty(FAV_LISTVIEW, "active", &active_sel);
    GUI_GetProperty(FAV_LISTVIEW, "select", &sel);
	if(favlist && sel >= 0)
	{
        if(active_sel != sel)
        {
            if((favlist->iptv_index_num>0)&&(favlist->iptv_index_num>sel))
            {
                index = favlist->iptv_index[sel];
            }
            else
            {
                return GXCORE_ERROR;
            }

            if(iptvList && iptvList->iptv_item_list && (index < iptvList->iptv_item_num))
            {
                s_fav_cur_play = sel;
                GUI_SetProperty(FAV_LISTVIEW, "active", &s_fav_cur_play);
                app_iptv_fav_play(&iptvList->iptv_item_list[index], index);
            }
        }
        else
        {
            GUI_EndDialog(g_CommonSelect.widget.wnd);
        }
	}

    return GXCORE_SUCCESS;
}

int app_iptv_fav_play_next(void)
{
    int index = 0;
	IptvIndexClass *favlist = app_iptv_getFavList();
    if(!favlist)
        return 0;

    if(s_fav_cur_play < (favlist->iptv_index_num - 1))
        s_fav_cur_play++;
    else
        s_fav_cur_play = 0;
    index = app_fav_item_to_all_index(s_fav_cur_play);
    return index;
}

int  app_iptv_fav_play_prev(void)
{
    int index = 0;
	IptvIndexClass *favlist = app_iptv_getFavList();
    if(!favlist)
        return 0;

    if(s_fav_cur_play == 0 || s_fav_cur_play == -1)
        s_fav_cur_play = (favlist->iptv_index_num - 1);
    else
        s_fav_cur_play--;
    index = app_fav_item_to_all_index(s_fav_cur_play);
    return index;
}


static int _press_green_key_return(void)
{
    extern int app_net_video_list_create_dialog(void);

    GUI_EndDialog(g_CommonSelect.widget.wnd);
    *s_fav_focus_item = app_fav_item_to_all_index(s_fav_cur_play);
    s_fav_list_enable = 0;
    s_fav_cur_play = -1;
    app_iptv_changeGroupInList(app_iptv_getCurGroupSelected());
    return app_net_video_list_create_dialog();
}

int app_iptv_fav_list_keypress(GuiWidget *widget, void *usrdata)
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
                case STBK_DOWN:
                case STBK_UP:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                {
                    ret = EVENT_TRANSFER_KEEPON;
                }
                break;

                case STBK_EXIT:
                case STBK_MENU:
                    ret = EVENT_TRANSFER_KEEPON;
                break;

                case STBK_OK:
                _press_ok_fav_play();
                break;

				case STBK_RED:
                _press_red_key_delate_fav();
                break;
				case STBK_GREEN:
                _press_green_key_return();
                break;
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

void app_iptv_create_fav_list(int* focus_item)
{
    CommonSelectParas paras = {0};
	IptvListClass *iptvList = app_iptv_getAllList();
	IptvIndexClass *favlist = app_iptv_getFavList();

    if(!favlist || !iptvList)
        return;

    if(favlist->iptv_index_num == 0)
        return;

    GUI_EndDialog(g_CommonSelect.widget.wnd);
    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = STR_ID_IPTV_FAV;
    paras.tip_strs.str_red = STR_ID_UNFAV;
    paras.tip_strs.str_green = STR_ID_RETURN;
    paras.signal.create = app_iptv_fav_create;
    paras.signal.destroy = app_iptv_fav_destroy;
    paras.signal.keypress = app_iptv_fav_keypress;
    paras.signal.got_focus = app_iptv_fav_got_focus;
    paras.signal.list_change = app_iptv_fav_list_change;
    paras.signal.list_get_total = app_iptv_fav_list_get_total;
    paras.signal.list_get_data = app_iptv_fav_list_get_data;
    paras.signal.list_keypress = app_iptv_fav_list_keypress;
    s_fav_focus_item = focus_item;
    app_common_select_create_dialog(&paras);
}

int app_iptv_fav_list_dlg_check(void)
{
    return s_fav_list_enable;
}

int app_iptv_fav_list_get_cur_play(void)
{
    return s_fav_cur_play;
}

void app_iptv_fav_list_dlg_clean(void)
{
    s_fav_list_enable = 0;
}
#endif
