#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_iptv.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include "app_windows.h"
#include "full_screen.h"
#include <gx_apps.h>
#include "app_common_view.h"
#include "netapps/app_netvideo_play.h"
#include "app_iptv_list_api.h"
#include "app_common_select.h"

#ifdef LINUX_OS
#define IPTV_LOCAL_DATABASE    "/home/gx/local/iptv/iptv.xml"
#define IPTV_LAST_CH_FILE_PATH "/home/gx/local/iptv_lastch.xml"
#define IPTV_LOCAL_FAVLIST_PATH "/home/gx/local/iptv/iptv_loacl_fav.xml"
#endif
#ifdef ECOS_OS
#define IPTV_LOCAL_FAVLIST_PATH "/home/gx/iptv_loacl_fav.xml"
#define IPTV_LOCAL_DATABASE    "/home/gx/iptv.xml"
#define IPTV_LAST_CH_FILE_PATH "/home/gx/iptv_lastch.xml"
#endif

#define IPTV_LIST_SOURCE_KEY    "netapps>iptv_list_source"
#define IPTV_MENU_STYLE_KEY     "netapps>iptv_menu_style"
#define IPTV_MENU_STYLE_VALUE   COMMON_VIEW_TABLE_ITEM_TYPE
typedef struct {
    int busy_flag;
    int wnd_flag; // 0, not exist; 1, focus; 2, unfocus
    int direct_play_flag; // 0, play from iptv wnd; 1, direct_play
    int update_flag; // got_focus: 0, not update, 1, all update; 2, show update

    IptvListSource list_source;
    int group_total;
    int cur_group_sel;
    char **group_title;

    int page_total;
    int cur_page_num;
    int page_prog_num;
    int page_prog_sel;

    int play_group_sel;
    int play_prog_sel;

    IptvListClass *group_list;
    IptvListClass *ch_list;
    IptvListClass *sub_ch_list;
    IptvIndexClass *iptv_indexlist;
    IPTVChannel *play_channel;
    IPTVOps iptv_ops;
} IptvControl;

static IptvControl s_iptv_ctrl = {0};
static int sIptvMenuStyle = IPTV_MENU_STYLE_VALUE;

static event_list *s_iptv_play_lastchannel_timer = NULL;
static void app_iptv_focus_area_change(int area);
void app_iptv_play_ctrl(IPTVPlayChoice choice, int num);
extern char *m3u_convert_xml(char *File_path);
extern char *xml_convert_xml(char *File_path);
extern int app_iptv_service_save_remote_bak_list(IptvListClass *iptv_list);
extern int app_iptv_service_save_local_list(IptvListClass *iptv_list);
int app_iptv_list_check_change_source_support(void);
IptvListClass *app_iptv_getIptvList(void);
char **app_iptv_getAllGroup(void);
int app_iptv_getGroupNum(void);
void app_iptv_changeGroupInList(int sel);

#if IPTV_LITE_SUPPORT
static int s_iptv_lite_menuc_flag = 0;

int app_iptv_lite_clean_menu_flag(void)
{
    s_iptv_lite_menuc_flag = 0;
    return 0;
}
#endif

#ifdef IPTV_FAV_SUPPORT
static IptvIndexClass *iptv_favlist = NULL;

IptvListClass *app_iptv_getAllList(void)
{
	return s_iptv_ctrl.ch_list;
}

gx_list_t* get_local_iptv_fav_list(void)
{
	gx_list_t *list = NULL;
	if(GXCORE_FILE_EXIST == GxCore_FileExists(IPTV_LOCAL_FAVLIST_PATH))
	{
		list = gx_get_list_from_file(IPTV_LOCAL_FAVLIST_PATH);
	}
	return list;
}

void save_local_iptv_fav_list(gx_list_t* list)
{
	if(list)
	{
		gx_list_save_to_file(list, IPTV_LOCAL_FAVLIST_PATH);
	}
}

IptvIndexClass *app_iptv_getIndexList(int group)
{
	IptvIndexClass *indexlist = NULL;
	if(s_iptv_ctrl.ch_list&&s_iptv_ctrl.group_list&&(s_iptv_ctrl.group_list->iptv_item_num>0)&&(group<=s_iptv_ctrl.group_list->iptv_item_num))
	{
        if((s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_UNKOWN)||(s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_GX))
		{
			indexlist = iptv_get_indexlist_by_type(s_iptv_ctrl.ch_list, group==0 ? "ALL" : s_iptv_ctrl.group_list->iptv_item_list[group-1].iptv_key);
		}
		else
		{
			indexlist = iptv_get_indexlist_by_type_simple(s_iptv_ctrl.ch_list, group==0 ? "ALL" : s_iptv_ctrl.group_list->iptv_item_list[group-1].iptv_key);
		}
	}
	return indexlist;
}

static void app_iptv_free_favlist(void)
{
	if(iptv_favlist)
	{
		iptv_indexlist_free(iptv_favlist);
		iptv_favlist = NULL;
	}
}

static void app_iptv_FavList_init(void)
{
	gx_list_t *list = NULL;
	int i = 0;
	int j = 0;
	app_iptv_free_favlist();
	if(s_iptv_ctrl.ch_list&&(s_iptv_ctrl.ch_list->iptv_item_num>0)&&s_iptv_ctrl.iptv_ops.get_fav_list)
	{
		iptv_favlist = iptv_indexlist_new();
		list = s_iptv_ctrl.iptv_ops.get_fav_list();
		if(list)
		{
			if(list->item_num>0)
			{
				for(i=0; i<list->item_num; i++)
				{
					for(j=0; j<s_iptv_ctrl.ch_list->iptv_item_num; j++)
					{
						if((strcmp(list->items[i].key, s_iptv_ctrl.ch_list->iptv_item_list[j].iptv_key)==0)
							||(strcmp(list->items[i].value, s_iptv_ctrl.ch_list->iptv_item_list[j].iptv_value)==0))
						{
							iptv_indexlist_add_item(iptv_favlist, j);
							break;
						}
					}
				}
			}
			gx_list_free(list);
		}
	}
}

IptvIndexClass *app_iptv_getFavList(void)
{
	return iptv_favlist;
}

void app_iptv_saveFavList(void)
{
	gx_list_t *list = NULL;
	int i = 0;
	if(s_iptv_ctrl.ch_list&&(s_iptv_ctrl.ch_list->iptv_item_num>0)&&s_iptv_ctrl.iptv_ops.save_fav_list&&iptv_favlist)
	{
		list = gx_list_new();
		for(i=0; i<iptv_favlist->iptv_index_num; i++)
		{
			if(iptv_favlist->iptv_index[i]<s_iptv_ctrl.ch_list->iptv_item_num)
			{
				gx_list_add_item(list, s_iptv_ctrl.ch_list->iptv_item_list[iptv_favlist->iptv_index[i]].iptv_key, s_iptv_ctrl.ch_list->iptv_item_list[iptv_favlist->iptv_index[i]].iptv_value, NULL);
			}
		}
		s_iptv_ctrl.iptv_ops.save_fav_list(list);
		gx_list_free(list);
	}
}

int app_iptv_isFavSupport(void)
{
	int ret = 0;
	if(s_iptv_ctrl.ch_list&&(s_iptv_ctrl.ch_list->iptv_item_num>0)&&s_iptv_ctrl.iptv_ops.get_fav_list)
	{
		ret = 1;
	}
	return ret;
}

static int _iptv_fav_check(int item)
{
	IptvIndexClass *favlist = NULL;
	int fav = 0;
	int i = 0;

    favlist = app_iptv_getFavList();
	if(favlist&&(favlist->iptv_index_num>0)&&(item>=0))
	{
		for(i=0; i<favlist->iptv_index_num; i++)
		{
			if(favlist->iptv_index[i] == item)
			{
				fav = 1;
				break;
			}
		}
	}

	return fav;
}

static int app_iptv_fav_check(int sel)
{
    int index = -1;

    if(s_iptv_ctrl.iptv_indexlist&&(s_iptv_ctrl.iptv_indexlist->iptv_index_num>0)&&(s_iptv_ctrl.iptv_indexlist->iptv_index_num>sel))
    {
        index = s_iptv_ctrl.iptv_indexlist->iptv_index[sel];
    }
    if(index>=0)
    {
        return _iptv_fav_check(index);
    }

    return 0;
}

static int app_iptv_list_destroy(void)
{
	app_iptv_saveFavList();

    return GXAPPS_SUCCESS;
}


static int app_iptv_list_fav_ctl_callback(int *item)
{
	IptvIndexClass *favlist = NULL;
	int sel = -1;
	int index = -1;
	favlist = app_iptv_getFavList();
	if(favlist)
	{
		GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
		if(sel>=0)
		{
            if(s_iptv_ctrl.iptv_indexlist&&(s_iptv_ctrl.iptv_indexlist->iptv_index_num>0)&&(s_iptv_ctrl.iptv_indexlist->iptv_index_num>sel))
            {
                index = s_iptv_ctrl.iptv_indexlist->iptv_index[sel];
            }
            if(index>=0)
            {
                if(_iptv_fav_check(index)>0)
                {
                    iptv_indexlist_del_item(favlist, index);
                }
                else
                {
                    iptv_indexlist_add_item(favlist, index);
                }
                GUI_SetProperty(g_CommonSelect.widget.list, "update_row", &sel);
            }
		}
	}

    return 0;
}

static int app_iptv_list_favorites_callback(int *item)
{
    app_iptv_create_fav_list(item);
    return 0;
}
#endif

void app_iptv_factory_reset(void)
{
    if(GxCore_FileExists(IPTV_LOCAL_DATABASE) == GXCORE_FILE_EXIST)
        GxCore_FileDelete(IPTV_LOCAL_DATABASE);
    if(GxCore_FileExists(IPTV_LOCAL_FAVLIST_PATH) == GXCORE_FILE_EXIST)
        GxCore_FileDelete(IPTV_LOCAL_FAVLIST_PATH);
    if(GxCore_FileExists(IPTV_LAST_CH_FILE_PATH) == GXCORE_FILE_EXIST)
        GxCore_FileDelete(IPTV_LAST_CH_FILE_PATH);
    if(GxCore_FileExists("/home/gx/iptv_userserver.xml") == GXCORE_FILE_EXIST)
        GxCore_FileDelete("/home/gx/iptv_userserver.xml");
    if(GxCore_FileExists("/home/gx/rss_userchannel.xml") == GXCORE_FILE_EXIST)
        GxCore_FileDelete("/home/gx/rss_userchannel.xml");
    return;
}

#ifdef SUPPORT_IPTVLIST_PRESET
void app_iptv_local_preset(void)
{
    static char file_name[]="/dvb/theme/iptv_local.xml";
    static char* file_path = NULL;
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(IPTV_LOCAL_DATABASE))
    {
        file_path =file_name;
        app_send_msg_exec(APPMSG_IPTV_UPDATE_LOCAL_LIST, &file_path);
    }
}
#endif
void set_iptv_save_channel(IPTVChannel *channel)
{
    gx_list_t *iptv_lastch = NULL;
	if (GXCORE_FILE_EXIST != GxCore_FileExists(IPTV_LAST_CH_FILE_PATH))
	{
		iptv_lastch = gx_list_new();
	}
    else
    {
        iptv_lastch = gx_get_list_from_file(IPTV_LAST_CH_FILE_PATH);
    }

	if ((iptv_lastch != NULL)&&(channel != NULL))
	{
		if ((iptv_lastch->item_num >0) || (iptv_lastch->items != NULL))
		{
            if((strcmp(iptv_lastch->items[0].key,channel->name)== 0)
                &&(strcmp(iptv_lastch->items[0].value,channel->url)== 0)
                &&(strcmp(iptv_lastch->items[0].type,channel->group)== 0)
                )
            {
                if (iptv_lastch)
                {
                    gx_list_free(iptv_lastch);
                    iptv_lastch = NULL;
                }
                return ;
            }
			gx_list_edit_item(iptv_lastch, 0, channel->name, channel->url, channel->group);
			gx_list_save_to_file(iptv_lastch, IPTV_LAST_CH_FILE_PATH);
		}
	}
	if (iptv_lastch)
	{
		gx_list_free(iptv_lastch);
		iptv_lastch = NULL;
	}
	return ;
}

IPTVChannel* get_iptv_save_channel(void)
{
    IPTVChannel *channel = NULL;
    gx_list_t *iptv_lastch = NULL;

	if (GXCORE_FILE_EXIST != GxCore_FileExists(IPTV_LAST_CH_FILE_PATH))
	{
		iptv_lastch = gx_list_new();
		gx_list_add_item(iptv_lastch, "invalid", "invalid", "invalid");
		gx_list_save_to_file(iptv_lastch, IPTV_LAST_CH_FILE_PATH);
        if (iptv_lastch)
        {
            gx_list_free(iptv_lastch);
            iptv_lastch = NULL;
        }
		channel = (IPTVChannel*)GxCore_Malloc(sizeof(IPTVChannel));
		channel->group = (char*)GxCore_Malloc(MAX_IPTV_GROUP_LENGTH);
		channel->url = (char*)GxCore_Malloc(MAX_IPTV_URL_LENGTH);
		channel->name = (char*)GxCore_Malloc(MAX_IPTV_NAME_LENGTH);
		return channel;
	}

    channel = (IPTVChannel*)GxCore_Malloc(sizeof(IPTVChannel));
    channel->group = (char*)GxCore_Malloc(MAX_IPTV_GROUP_LENGTH);
    channel->url = (char*)GxCore_Malloc(MAX_IPTV_URL_LENGTH);
    channel->name = (char*)GxCore_Malloc(MAX_IPTV_NAME_LENGTH);

	iptv_lastch = gx_get_list_from_file(IPTV_LAST_CH_FILE_PATH);
	if (!iptv_lastch)
	{
		memset(channel->group,0x0,MAX_IPTV_GROUP_LENGTH);
		memset(channel->url,0x0,MAX_IPTV_URL_LENGTH);
		memset(channel->name,0x0,MAX_IPTV_NAME_LENGTH);
	}
	else
	{
		if ((iptv_lastch->item_num >0) || (iptv_lastch->items != NULL))
		{
			if (strlen(iptv_lastch->items[0].key) < MAX_IPTV_NAME_LENGTH)
			{
				strcpy((char *)(channel->name), (iptv_lastch->items[0].key));
			}
			else
			{
				strncpy((char *)(channel->name), (iptv_lastch->items[0].key), MAX_IPTV_NAME_LENGTH - 1);
			}
			if (strlen(iptv_lastch->items[0].value) < MAX_IPTV_URL_LENGTH)
			{
				strcpy((char *)(channel->url), (iptv_lastch->items[0].value));
			}
			else
			{
				strncpy((char *)(channel->url), (iptv_lastch->items[0].value), MAX_IPTV_URL_LENGTH - 1);
			}
			if (strlen(iptv_lastch->items[0].type) < MAX_IPTV_URL_LENGTH)
			{
				strcpy((char *)(channel->group), (iptv_lastch->items[0].type));
			}
			else
			{
				strncpy((char *)(channel->group), (iptv_lastch->items[0].type), MAX_IPTV_GROUP_LENGTH - 1);
			}
		}
	}
    if (iptv_lastch)
    {
        gx_list_free(iptv_lastch);
        iptv_lastch = NULL;
    }
    return channel;
}

int get_iptv_play_mode(void)
{
    return s_iptv_ctrl.iptv_ops.auto_play;
}

void set_iptv_play_mode(int auto_play)
{
    s_iptv_ctrl.iptv_ops.auto_play = auto_play;
}

void app_iptv_set_source(int source)
{
    GxBus_ConfigSetInt(IPTV_LIST_SOURCE_KEY, source);
}

int app_iptv_get_source(void)
{
    int source;
    GxBus_ConfigGetInt(IPTV_LIST_SOURCE_KEY, &source, IPTV_LIST_LOCAL);
    return source;
}

static void app_iptv_clean_list(void)
{
    if (s_iptv_ctrl.ch_list)
    {
        iptv_list_free(s_iptv_ctrl.ch_list);
        s_iptv_ctrl.ch_list = NULL;
    }
    if (s_iptv_ctrl.group_list)
    {
        iptv_list_free(s_iptv_ctrl.group_list);
        s_iptv_ctrl.group_list = NULL;
    }
    if (s_iptv_ctrl.sub_ch_list)
    {
        iptv_list_free(s_iptv_ctrl.sub_ch_list);
        s_iptv_ctrl.sub_ch_list = NULL;
    }
    if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
    {
        app_common_view_update_page_info(0, 0);
    }
    if(COMMON_VIEW_LIST_ITEM_TYPE == sIptvMenuStyle)
    {
        GUI_SetProperty( g_CommonView.widget.list_item, "update_all", NULL);
        if(GUI_CheckDialog(WND_FILE_LIST) == GXCORE_SUCCESS)
        {
            GUI_SetInterface("flush", NULL);
            GUI_SetProperty(WND_FILE_LIST, "update", NULL);
        }
    }
}

static void app_iptv_clean_data(void)
{
    APP_FREE(s_iptv_ctrl.group_title);
    s_iptv_ctrl.group_total = 0;
    s_iptv_ctrl.cur_group_sel = 0;
    s_iptv_ctrl.page_total = 0;
    s_iptv_ctrl.cur_page_num = 0;
    s_iptv_ctrl.page_prog_num = 0;
    s_iptv_ctrl.page_prog_sel = 0;
    s_iptv_ctrl.direct_play_flag = 0;
}

void app_iptv_clean_all_info(void)
{
    app_iptv_clean_list();
    app_iptv_clean_data();
}

static status_t app_iptv_list_get(IptvListSource list_source)
{
    static AppMsg_IptvList iptv_list;

    memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.iptv_source = list_source;
    iptv_list.iptv_server = s_iptv_ctrl.iptv_ops.server_type;
    iptv_list.remote_fastget = s_iptv_ctrl.iptv_ops.remote_fastget;
    s_iptv_ctrl.iptv_ops.remote_fastget = 0;

    s_iptv_ctrl.busy_flag = 1;
    return app_send_msg_exec(APPMSG_IPTV_GET_LIST, &iptv_list);
}

static void app_iptv_list_init(IptvListClass *list)
{
    app_iptv_clean_list();
    s_iptv_ctrl.ch_list = list;
    if (s_iptv_ctrl.ch_list && (s_iptv_ctrl.ch_list->iptv_item_num > 0))
    {
		if((s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_UNKOWN)||(s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_GX))
		{
            s_iptv_ctrl.group_list = iptv_get_group_list(s_iptv_ctrl.ch_list);
		}
		else
		{
            s_iptv_ctrl.group_list = iptv_get_group_list_simple(s_iptv_ctrl.ch_list);
		}
    }
#ifdef IPTV_FAV_SUPPORT
	app_iptv_FavList_init();
#endif
}

static status_t app_iptv_group_update(void)
{
    int i = 0;
    int group_num = 0;
    int add_num = 0;

    APP_FREE(s_iptv_ctrl.group_title);
    s_iptv_ctrl.group_total = 0;

    if (s_iptv_ctrl.group_list && s_iptv_ctrl.group_list->iptv_item_num > 0)
        add_num = s_iptv_ctrl.group_list->iptv_item_num;
    group_num = 1 + add_num;
    s_iptv_ctrl.group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
    if (s_iptv_ctrl.group_title == NULL)
    {
        return GXCORE_ERROR;
    }

    s_iptv_ctrl.group_title[0] = STR_ID_ALL;
    for(i = 0; i < add_num; i++)
    {
        s_iptv_ctrl.group_title[i+1] = s_iptv_ctrl.group_list->iptv_item_list[i].iptv_key;
    }
    s_iptv_ctrl.group_total = group_num;

    return GXCORE_SUCCESS;
}

static void app_iptv_group_rebuild(void)
{
    if (s_iptv_ctrl.group_list)
    {
        iptv_list_free(s_iptv_ctrl.group_list);
        s_iptv_ctrl.group_list = NULL;
    }
    if (s_iptv_ctrl.ch_list && (s_iptv_ctrl.ch_list->iptv_item_num > 0))
    {
        s_iptv_ctrl.group_list = iptv_get_group_list(s_iptv_ctrl.ch_list);
    }
    app_iptv_group_update();

}

static void app_iptv_list_update(void)
{
    s_iptv_ctrl.page_total = 0;
    if (s_iptv_ctrl.ch_list && s_iptv_ctrl.ch_list->iptv_item_num > 0)
    {
        if (s_iptv_ctrl.cur_group_sel == 0)
        {
            s_iptv_ctrl.page_total = s_iptv_ctrl.ch_list->iptv_item_num / COMMON_VIEW_MAX_PAGE_ITEM;
            if (s_iptv_ctrl.ch_list->iptv_item_num % COMMON_VIEW_MAX_PAGE_ITEM > 0)
                s_iptv_ctrl.page_total++;
        }
        else if((s_iptv_ctrl.cur_group_sel > 0)
                && (s_iptv_ctrl.group_list) && (s_iptv_ctrl.group_list->iptv_item_num > 0)
                && ((s_iptv_ctrl.cur_group_sel-1) <= s_iptv_ctrl.group_list->iptv_item_num))
        {
            if (s_iptv_ctrl.sub_ch_list)
            {
                iptv_list_free(s_iptv_ctrl.sub_ch_list);
                s_iptv_ctrl.sub_ch_list = NULL;
            }
			if((s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_UNKOWN)||(s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_GX))
			{
                s_iptv_ctrl.sub_ch_list = iptv_get_list_by_type(s_iptv_ctrl.ch_list,
                    s_iptv_ctrl.group_list->iptv_item_list[s_iptv_ctrl.cur_group_sel-1].iptv_key);
			}
			else
			{
                s_iptv_ctrl.sub_ch_list = iptv_get_list_by_type_simple(s_iptv_ctrl.ch_list,
                    s_iptv_ctrl.group_list->iptv_item_list[s_iptv_ctrl.cur_group_sel-1].iptv_key);
			}
            if (s_iptv_ctrl.sub_ch_list && s_iptv_ctrl.sub_ch_list->iptv_item_num > 0)
            {
                s_iptv_ctrl.page_total = s_iptv_ctrl.sub_ch_list->iptv_item_num / COMMON_VIEW_MAX_PAGE_ITEM;
                if (s_iptv_ctrl.sub_ch_list->iptv_item_num % COMMON_VIEW_MAX_PAGE_ITEM > 0)
                    s_iptv_ctrl.page_total++;
            }
            else
            {
                app_log_info("sublist null need to rebuild group list\n");
                /*sublist num =0,group list need update*/
                app_iptv_group_rebuild();
                s_iptv_ctrl.cur_page_num = 0;
                s_iptv_ctrl.cur_group_sel = 0;
                s_iptv_ctrl.page_total = s_iptv_ctrl.ch_list->iptv_item_num / COMMON_VIEW_MAX_PAGE_ITEM;
                if (s_iptv_ctrl.ch_list->iptv_item_num % COMMON_VIEW_MAX_PAGE_ITEM > 0)
                    s_iptv_ctrl.page_total++;
            }
        }
    }
    else
    {
        app_log_info("chlist null need to rebuild group list\n");
        /*sublist num =0,group list need update*/
        app_iptv_group_rebuild();
        s_iptv_ctrl.cur_group_sel = 0;
    }
    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_NETVIDEO_PLAY))
    {
        if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
        {
            app_common_view_update_page_info(0, s_iptv_ctrl.page_total);
        }
        else
        {
            if (strcmp(WND_COMMON_VIEW, GUI_GetFocusWindow()) == 0)
            {
                GUI_SetProperty(g_CommonView.widget.list_item, "update_all", NULL);
            }
        }
    }
#ifdef IPTV_FAV_SUPPORT
    if(s_iptv_ctrl.iptv_indexlist)
    {
        iptv_indexlist_free(s_iptv_ctrl.iptv_indexlist);
        s_iptv_ctrl.iptv_indexlist = NULL;
    }
    s_iptv_ctrl.iptv_indexlist = app_iptv_getIndexList(s_iptv_ctrl.cur_group_sel);
#endif
}

static void app_iptv_page_update(int page_num)
{
    int i = 0;
    int start_index, end_index;

    s_iptv_ctrl.page_prog_num = 0;
    memset(g_CommonView.ctrl_data.page_item_data, 0, sizeof(g_CommonView.ctrl_data.page_item_data));
    start_index = page_num * COMMON_VIEW_MAX_PAGE_ITEM;
    end_index = start_index + COMMON_VIEW_MAX_PAGE_ITEM;

    if (s_iptv_ctrl.ch_list && s_iptv_ctrl.ch_list->iptv_item_num > 0 && page_num >= 0 && page_num < s_iptv_ctrl.page_total)
    {
        if (s_iptv_ctrl.cur_group_sel == 0) //all
        {
            if (end_index > s_iptv_ctrl.ch_list->iptv_item_num)
                end_index = s_iptv_ctrl.ch_list->iptv_item_num;
            for (i = start_index; i < end_index; i++)
                g_CommonView.ctrl_data.page_item_data[i-start_index].title = s_iptv_ctrl.ch_list->iptv_item_list[i].iptv_key;
            s_iptv_ctrl.page_prog_num = end_index - start_index;
        }
        else if (s_iptv_ctrl.sub_ch_list && s_iptv_ctrl.sub_ch_list->iptv_item_num > 0)
        {
                if (end_index > s_iptv_ctrl.sub_ch_list->iptv_item_num)
                    end_index = s_iptv_ctrl.sub_ch_list->iptv_item_num;
                for (i = start_index; i < end_index; i++)
                    g_CommonView.ctrl_data.page_item_data[i-start_index].title = s_iptv_ctrl.sub_ch_list->iptv_item_list[i].iptv_key;
                s_iptv_ctrl.page_prog_num = end_index - start_index;
        }
    }
    if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
    {
        app_common_view_update_page_item_all(s_iptv_ctrl.page_prog_num, g_CommonView.ctrl_data.page_item_data);
    }
}


static void app_iptv_show_update(bool force_update)
{
    if (force_update || s_iptv_ctrl.wnd_flag == 1)
    {
        if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
        {
            app_iptv_page_update(s_iptv_ctrl.cur_page_num);
        }
        app_common_view_focus_area_change(g_CommonView.focus_area);
        GUI_SetProperty(g_CommonView.widget.list_group, "update_all", NULL);
        if(g_CommonView.focus_area == COMMON_VIEW_AREA_LIST_GROUP && (!strcmp(GUI_GetFocusWindow(), g_CommonView.widget.wnd)))
        {
            GUI_SetFocusWidget(g_CommonView.widget.list_group);
        }
        if (s_iptv_ctrl.group_total > 0)
        {
            GUI_SetProperty(g_CommonView.widget.list_group, "active", &s_iptv_ctrl.cur_group_sel);
        }

        s_iptv_ctrl.update_flag = 0;
    }
    else
    {
        s_iptv_ctrl.update_flag = 2;
    }
}

static void app_iptv_list_source_set(void)
{
    app_iptv_clean_all_info();
    if (s_iptv_ctrl.wnd_flag == 1)
    {
        GUI_SetProperty(g_CommonView.widget.list_group, "update_all", NULL);
        if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
        {
            app_iptv_page_update(s_iptv_ctrl.cur_page_num);
        }
        if (s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        {
            app_common_view_show_gif();
            app_common_view_show_popup_msg("Updating...", 0);
        }
    }
    app_iptv_list_get(s_iptv_ctrl.list_source);
}

static void free_play_channel(IPTVChannel* channel)
{
    if (channel)
    {
        if (channel->group)
            GxCore_Free(channel->group);
        if (channel->url)
            GxCore_Free(channel->url);
        if (channel->name)
            GxCore_Free(channel->name);
        GxCore_Free(channel);
    }
}

static void _iptv_play_set_save_channel(char *prog_name, char *prog_url)
{
    if(NULL == prog_name || NULL == prog_url)
        return;

    if(get_iptv_play_mode() && s_iptv_ctrl.play_channel != NULL)
    {
        if(s_iptv_ctrl.play_channel->group)
        {
            if(s_iptv_ctrl.group_title && s_iptv_ctrl.cur_group_sel < s_iptv_ctrl.group_total)
                strncpy(s_iptv_ctrl.play_channel->group, s_iptv_ctrl.group_title[s_iptv_ctrl.cur_group_sel], MAX_IPTV_GROUP_LENGTH - 1);
            else
                strncpy(s_iptv_ctrl.play_channel->group, STR_ID_ALL, MAX_IPTV_GROUP_LENGTH - 1);
        }
        if(s_iptv_ctrl.play_channel->url && s_iptv_ctrl.play_channel->name)
        {
            strncpy(s_iptv_ctrl.play_channel->url, prog_url, MAX_IPTV_URL_LENGTH - 1);
            strncpy(s_iptv_ctrl.play_channel->name, prog_name, MAX_IPTV_NAME_LENGTH - 1);
        }
        if(s_iptv_ctrl.iptv_ops.set_save_channel != NULL)
            s_iptv_ctrl.iptv_ops.set_save_channel(s_iptv_ctrl.play_channel);
    }
}

static void app_iptv_play_exit_ctrl(void)
{
    if (get_iptv_play_mode() == 0)
    {
        app_common_view_unfocus_item(g_CommonView.ctrl_data.page_item_sel);
        g_CommonView.ctrl_data.page_item_sel = s_iptv_ctrl.page_prog_sel;
        if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
        {
            app_common_view_update_page_info(s_iptv_ctrl.cur_page_num, s_iptv_ctrl.page_total);
        }
        else
        {
            GUI_SetProperty(g_CommonView.widget.list_item, "select", &(s_iptv_ctrl.play_prog_sel));
            GUI_SetProperty(g_CommonView.widget.list_item, "update_all",NULL );
        }
        app_iptv_show_update(true);
    }
    else
    {
        app_iptv_clean_all_info();
        APP_TIMER_REMOVE(s_iptv_play_lastchannel_timer);
    }

    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_MAIN_MENU))
    {
        g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
        extern void app_tv_radio_key_exec(void);
        app_tv_radio_key_exec();
    }
}
static int _iptv_play_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
    	PasswdDlg passwd_dlg;
	char *url = NULL;
	if(ret->result ==  PASSWD_OK)
	{
		if(s_iptv_ctrl.iptv_ops.get_url != NULL)
		{
			url = s_iptv_ctrl.iptv_ops.get_url((char* )usr_data);
			if(url)
			{
				app_net_video_start_play(url, PLAY_STATE_LOAD, 0, false);
				free(url);
				url = NULL;
			}
		}
		else
		{
			app_net_video_start_play(url, PLAY_STATE_LOAD, 0, false);
		}
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
	        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
	        passwd_dlg.passwd_exit_cb = _iptv_play_passwd_dlg_cb;
	        passwd_dlg.str = STR_ID_ERR_PASSWD;
	        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
	        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
	        passwd_dlg_create(&passwd_dlg);
    }
	return 0;
}
static void _url_redirect(char *url, uint64_t cur_time)
{
    int lock=0;
    PasswdDlg passwd_dlg;
    if(s_iptv_ctrl.iptv_ops.check_lock != NULL)
    {
        lock = s_iptv_ctrl.iptv_ops.check_lock(url);
	 if(lock)
	 {
	        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
	        passwd_dlg.passwd_exit_cb = _iptv_play_passwd_dlg_cb;
	        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
	        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
		 passwd_dlg.usr_data = url;
	        passwd_dlg_create(&passwd_dlg);
		 return;
	 }
    }
    if(s_iptv_ctrl.iptv_ops.get_url != NULL)
    {
        url = s_iptv_ctrl.iptv_ops.get_url(url);
	 if(url)
	 {
		 app_net_video_start_play(url, PLAY_STATE_LOAD, cur_time, false);
	        free(url);
	        url = NULL;
	 }
    }
    else
    {
	    app_net_video_start_play(url, PLAY_STATE_LOAD, cur_time, false);
    }
}

#ifdef IPTV_FAV_SUPPORT
static void _change_to_all_group(void)
{
    int cur_group_sel = -1 ;
    cur_group_sel = s_iptv_ctrl.cur_group_sel ;

    s_iptv_ctrl.cur_group_sel = 0;
    s_iptv_ctrl.page_total = s_iptv_ctrl.ch_list->iptv_item_num / COMMON_VIEW_MAX_PAGE_ITEM;
    if (s_iptv_ctrl.ch_list->iptv_item_num % COMMON_VIEW_MAX_PAGE_ITEM > 0)
        s_iptv_ctrl.page_total++;

    if(s_iptv_ctrl.iptv_indexlist && (cur_group_sel != 0) )
    {
        iptv_indexlist_free(s_iptv_ctrl.iptv_indexlist);
        s_iptv_ctrl.iptv_indexlist = NULL;
        s_iptv_ctrl.iptv_indexlist = app_iptv_getIndexList(s_iptv_ctrl.cur_group_sel);
    }
}
#endif

static void app_iptv_play_prev_ctrl(void)
{
    char *prog_name = NULL, *prog_url = NULL;
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        _change_to_all_group();
    }
#endif
    IptvListClass *tvList = (s_iptv_ctrl.cur_group_sel == 0) ? s_iptv_ctrl.ch_list : s_iptv_ctrl.sub_ch_list;

    if(NULL == tvList || 0 == tvList->iptv_item_num)
        return;

    s_iptv_ctrl.play_prog_sel = (s_iptv_ctrl.play_prog_sel > 0) ? (s_iptv_ctrl.play_prog_sel - 1) : (tvList->iptv_item_num - 1);
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        s_iptv_ctrl.play_prog_sel = app_iptv_fav_play_prev();
    }
#endif
    s_iptv_ctrl.cur_page_num = s_iptv_ctrl.play_prog_sel / COMMON_VIEW_MAX_PAGE_ITEM;
    s_iptv_ctrl.page_prog_sel = s_iptv_ctrl.play_prog_sel % COMMON_VIEW_MAX_PAGE_ITEM;

    prog_name = tvList->iptv_item_list[s_iptv_ctrl.play_prog_sel].iptv_key;
    prog_url = tvList->iptv_item_list[s_iptv_ctrl.play_prog_sel].iptv_value;

    _iptv_play_set_save_channel(prog_name, prog_url);
    _url_redirect(prog_url, 0);
}

static status_t app_iptv_play_next_ctrl(int force)
{
    char *prog_name = NULL, *prog_url = NULL;
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        _change_to_all_group();
    }
#endif
    IptvListClass *tvList = (s_iptv_ctrl.cur_group_sel == 0) ? s_iptv_ctrl.ch_list : s_iptv_ctrl.sub_ch_list;

    if(NULL == tvList || 0 == tvList->iptv_item_num)
        return GXCORE_SUCCESS;

    s_iptv_ctrl.play_prog_sel = (s_iptv_ctrl.play_prog_sel + 1 < tvList->iptv_item_num) ? (s_iptv_ctrl.play_prog_sel + 1) : (0);
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        s_iptv_ctrl.play_prog_sel = app_iptv_fav_play_next();
    }
#endif
    s_iptv_ctrl.cur_page_num = s_iptv_ctrl.play_prog_sel / COMMON_VIEW_MAX_PAGE_ITEM;
    s_iptv_ctrl.page_prog_sel = s_iptv_ctrl.play_prog_sel % COMMON_VIEW_MAX_PAGE_ITEM;

    prog_name = tvList->iptv_item_list[s_iptv_ctrl.play_prog_sel].iptv_key;
    prog_url = tvList->iptv_item_list[s_iptv_ctrl.play_prog_sel].iptv_value;

    _iptv_play_set_save_channel(prog_name, prog_url);
    _url_redirect(prog_url, 0);
    return GXCORE_SUCCESS;
}

static void app_iptv_play_restart_ctrl(uint64_t cur_time)
{
    IptvListClass *tvList = (s_iptv_ctrl.cur_group_sel == 0) ? s_iptv_ctrl.ch_list : s_iptv_ctrl.sub_ch_list;
    char *url = tvList->iptv_item_list[s_iptv_ctrl.play_prog_sel].iptv_value;
    _url_redirect(url, cur_time);
}

static void app_iptv_play_get_url_ctrl(int source_index)
{
    IptvListClass *tvList = (s_iptv_ctrl.cur_group_sel == 0) ? s_iptv_ctrl.ch_list : s_iptv_ctrl.sub_ch_list;
    char *url = tvList->iptv_item_list[s_iptv_ctrl.play_prog_sel].iptv_value;
    _url_redirect(url, 0);
}

static int app_iptv_play_list_ok(int item)
{
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        item = app_fav_item_to_all_index(item);
    }
#endif
	app_iptv_play_ctrl(IPTV_PLAY_LIST, item+1);
    return 0;
}

static int app_iptvlist_list_get_total(void)
{
    uint32_t total = 0;
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        IptvIndexClass *favlist = app_iptv_getFavList();
        return favlist->iptv_index_num;
    }
#endif
	IptvListClass *iptvList = app_iptv_getIptvList();
	if (iptvList!= NULL)
	{
		total = iptvList->iptv_item_num;
	}
    return total;
}

static char *app_iptv_play_list_data_get(int sel)
{
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        sel = app_fav_item_to_all_index(sel);
    }
#endif
	IptvListClass *iptv_list = app_iptv_getIptvList();

    if(NULL == iptv_list)
        return NULL;
    else
        return iptv_list->iptv_item_list[sel].iptv_key;
}

static void app_iptv_list_group_item_build(char *buffer, int size, char *group_name)
{
	int i = 0;
	char *temp = NULL;

	temp = group_name;
	while((*temp != '\0')&&(i<(size-1)))
	{
	#if 1 //please add patch 87578 first
		if(*temp == ',')
		{
			if(i<(size-2))
			{
				buffer[i++] = '\\';
				buffer[i++] = ',';
			}
			else
			{
				break;
			}
		}
		else
	#endif
		{
			buffer[i++] = *temp;
		}
		temp++;
	}
	buffer[i] = '\0';
}

static int app_iptv_list_cmb_group_build(char **cmb_title)
{
#define BUFFER_LENGTH_GROUP	(60)
	int i = 0;
	int total_id=0;
	char buffer[BUFFER_LENGTH_GROUP] = {0};
	char **iptvGroup = app_iptv_getAllGroup();
	//int len;

	total_id = app_iptv_getGroupNum();
	if((total_id <= 0) || (iptvGroup==NULL))
	{
		printf("\n_iptv_list_cmb_group_build, total id error...%s,%d\n",__FILE__,__LINE__);
		return -1;
	}
	*cmb_title = GxCore_Malloc(total_id*(BUFFER_LENGTH_GROUP));//need GxCore_Free
	if(*cmb_title == NULL)
	{
		printf("\n_iptv_list_cmb_group_build, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return -1;
	}
	memset(*cmb_title, 0, total_id*(BUFFER_LENGTH_GROUP));

	//first one
	strcat(*cmb_title, "[");
	for(i=0; i<total_id; i++)
	{
		memset(buffer, 0, BUFFER_LENGTH_GROUP);
		if (i==0)
		{
			app_iptv_list_group_item_build(buffer, BUFFER_LENGTH_GROUP, iptvGroup[i]);
		}
		else
		{
			buffer[0] = ',';
			app_iptv_list_group_item_build(buffer+1, BUFFER_LENGTH_GROUP-1, iptvGroup[i]);
		}
		strcat(*cmb_title, buffer);
	}
	strcat(*cmb_title, "]");
    return 1;
}

static void app_iptv_play_cmb_title_free(char **cmb_title)
{
    if(NULL != *cmb_title)
    {
        GxCore_Free(*cmb_title);
        *cmb_title = NULL;
    }
}

static int app_iptv_play_change_group(int sel)
{

	int list_sel = 0;

    if(sel != s_iptv_ctrl.cur_group_sel)
    {
	    app_iptv_changeGroupInList(sel);
	    IptvListClass *iptvChannel = NULL;
	    iptvChannel = app_iptv_getIptvList();
	    if (iptvChannel)
	    {
	    	if (list_sel<iptvChannel->iptv_item_num)
	    	{
	    		app_iptv_play_ctrl(IPTV_PLAY_LIST, list_sel + 1);
	    	}
            return 1;
	    }
        return 0;
    }
    return 0;
}

static int app_iptv_list_check_fullscreen(void)
{
    return s_iptv_ctrl.iptv_ops.fullscreen_flag;
}
int app_iptv_list_set_fullscreen(int flag)
{
    s_iptv_ctrl.iptv_ops.fullscreen_flag = flag;
    return s_iptv_ctrl.iptv_ops.fullscreen_flag;
}

static int app_iptv_list_check_sort_support(void)
{
    return s_iptv_ctrl.iptv_ops.iptv_lite_flag;
}

int app_iptv_list_check_change_source_support(void)
{
    if (s_iptv_ctrl.iptv_ops.iptv_lite_flag && s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        return 1;
    return 0;
}

static int sort_iptv_list(IptvListClass *list_src, int type, int cur_pos)
{
    int i = 0;
    int j = 0;
    int list_num = 0;
    char key_first_word_bf[2] = {0};
    char key_first_word_af[2] = {0};
    IptvItemClass cur_play_item = {0};

    if (list_src == NULL || cur_pos > list_src->iptv_item_num - 1)
    {
        return cur_pos;
    }

    cur_play_item = list_src->iptv_item_list[cur_pos];

    IptvListClass *list_tmp = NULL;
    list_tmp = (IptvListClass *)malloc(sizeof(IptvListClass));
    memset(list_tmp, 0, sizeof(IptvListClass));
    list_tmp->iptv_item_list = (IptvItemClass *)malloc(1 * sizeof(IptvItemClass));
    memset(list_tmp->iptv_item_list, 0, 1 * sizeof(IptvItemClass));

    list_num = list_src->iptv_item_num;

    if (list_num > 0)
    {
        for (i = 0; i < list_num; i++)
        {
            for (j = 0; j < list_num - 1 - i; j++)
            {
                memcpy(key_first_word_bf, list_src->iptv_item_list[j].iptv_key, 2);
                memcpy(key_first_word_af, list_src->iptv_item_list[j + 1].iptv_key, 2);
                // app_log_info("========bf letter:%x %x=========\n",key_first_word_bf[0],key_first_word_bf[1]);
                // app_log_info("========af letter:%x %x=========\n",key_first_word_af[0],key_first_word_af[1]);
                //#############################RUSSIA##########################################
                if (key_first_word_bf[0] == 0xd0) //208
                {
                    if ((key_first_word_bf[1] >= 0xb0) && (key_first_word_bf[1] <= 0xbf))
                    {
                        key_first_word_bf[0] = key_first_word_bf[1] - 32; //turn to big letter to compare   144 -  159
                    }
                    else
                    {
                        key_first_word_bf[0] = key_first_word_bf[1];
                    }
                }

                if (key_first_word_bf[0] == 0xd1) //209
                {
                    if (key_first_word_bf[1] == 0x91)
                    {
                        key_first_word_bf[0] = 176;    //   176
                    }
                    else if (key_first_word_bf[1] == 0x94)
                    {
                        key_first_word_bf[0] = 177;   //    177
                    }
                    else
                    {
                        if ((key_first_word_bf[1] >= 0x80) && (key_first_word_bf[1] <= 0x8f))
                        {
                            key_first_word_bf[0] = key_first_word_bf[1] + 32; //turn to big letter to compare   160 - 175
                        }
                        else
                        {
                            key_first_word_bf[0] = key_first_word_bf[1];
                        }
                    }
                }

                if (key_first_word_af[0] == 0xd0) //208
                {
                    if ((key_first_word_af[1] >= 0xb0) && (key_first_word_af[1] <= 0xbf))
                    {
                        key_first_word_af[0] = key_first_word_af[1] - 32; //turn to big letter to compare   144 -  159
                    }
                    else
                    {
                        key_first_word_af[0] = key_first_word_af[1];
                    }
                }

                if (key_first_word_af[0] == 0xd1) //209
                {
                    if (key_first_word_af[1] == 0x91)
                    {
                        key_first_word_af[0] = 176;    //   176
                    }
                    else if (key_first_word_af[1] == 0x94)
                    {
                        key_first_word_af[0] = 177;   //    177
                    }
                    else
                    {
                        if ((key_first_word_bf[1] >= 0x80) && (key_first_word_bf[1] <= 0x8f))
                        {
                            key_first_word_af[0] = key_first_word_af[1] + 32; //turn to big letter to compare   160 - 175
                        }
                        else
                        {
                            key_first_word_af[0] = key_first_word_af[1];
                        }
                    }
                }

                //#############################ENGLISH##############################################

                if ((key_first_word_bf[0] >= 65) && (key_first_word_bf[0] <= 90))
                {
                    key_first_word_bf[0] = key_first_word_bf[0] + 32;//all turn to a-z compare    97 - 122
                }

                if ((key_first_word_af[0] >= 65) && (key_first_word_af[0] <= 90))
                {
                    key_first_word_af[0] = key_first_word_af[0] + 32;//all turn to a-z compare  97 - 122
                }
                //#############################OTHER WORD############################################

                if ((key_first_word_bf[0] < 97) || ((key_first_word_bf[0] > 122) && (key_first_word_bf[0] < 128)))
                {
                    if (type == 1)
                        key_first_word_bf[0] = 0;
                    if (type == 0)
                        key_first_word_bf[0] = 255;
                }

                if ((key_first_word_af[0] < 97) || ((key_first_word_af[0] > 122) && (key_first_word_af[0] < 128)))
                {
                    if (type == 1)
                        key_first_word_af[0] = 0;
                    if (type == 0)
                        key_first_word_af[0] = 255;
                }

                //###############################################################################
                //printf("==========bf:%x=====af:%x=======\n",key_first_word_bf[0],key_first_word_af[0]);
                if (type == 1) //Z-A
                {
                    if (((key_first_word_af[0]) > (key_first_word_bf[0])) || ((key_first_word_bf[0] == 0) && (key_first_word_af[0] == 0)))
                    {
                        list_tmp->iptv_item_list[0].iptv_key = list_src->iptv_item_list[j].iptv_key;
                        list_tmp->iptv_item_list[0].iptv_value = list_src->iptv_item_list[j].iptv_value;
                        list_tmp->iptv_item_list[0].iptv_type = list_src->iptv_item_list[j].iptv_type;
                        list_tmp->iptv_item_list[0].iptv_ad = list_src->iptv_item_list[j].iptv_ad;

                        list_src->iptv_item_list[j].iptv_key = list_src->iptv_item_list[j + 1].iptv_key;
                        list_src->iptv_item_list[j].iptv_value = list_src->iptv_item_list[j + 1].iptv_value;
                        list_src->iptv_item_list[j].iptv_type = list_src->iptv_item_list[j + 1].iptv_type;
                        list_src->iptv_item_list[j].iptv_ad = list_src->iptv_item_list[j + 1].iptv_ad;

                        list_src->iptv_item_list[j + 1].iptv_key = list_tmp->iptv_item_list[0].iptv_key;
                        list_src->iptv_item_list[j + 1].iptv_value = list_tmp->iptv_item_list[0].iptv_value;
                        list_src->iptv_item_list[j + 1].iptv_type = list_tmp->iptv_item_list[0].iptv_type;
                        list_src->iptv_item_list[j + 1].iptv_ad = list_tmp->iptv_item_list[0].iptv_ad;
                    }
                }
                else if (type == 0)	 // A - Z
                {
                    if (((key_first_word_af[0]) < (key_first_word_bf[0])) || ((key_first_word_bf[0] == 255) && (key_first_word_af[0] == 255)))
                    {
                        list_tmp->iptv_item_list[0].iptv_key = list_src->iptv_item_list[j].iptv_key;
                        list_tmp->iptv_item_list[0].iptv_value = list_src->iptv_item_list[j].iptv_value;
                        list_tmp->iptv_item_list[0].iptv_type = list_src->iptv_item_list[j].iptv_type;
                        list_tmp->iptv_item_list[0].iptv_ad = list_src->iptv_item_list[j].iptv_ad;

                        list_src->iptv_item_list[j].iptv_key = list_src->iptv_item_list[j + 1].iptv_key;
                        list_src->iptv_item_list[j].iptv_value = list_src->iptv_item_list[j + 1].iptv_value;
                        list_src->iptv_item_list[j].iptv_type = list_src->iptv_item_list[j + 1].iptv_type;
                        list_src->iptv_item_list[j].iptv_ad = list_src->iptv_item_list[j + 1].iptv_ad;

                        list_src->iptv_item_list[j + 1].iptv_key = list_tmp->iptv_item_list[0].iptv_key;
                        list_src->iptv_item_list[j + 1].iptv_value = list_tmp->iptv_item_list[0].iptv_value;
                        list_src->iptv_item_list[j + 1].iptv_type = list_tmp->iptv_item_list[0].iptv_type;
                        list_src->iptv_item_list[j + 1].iptv_ad = list_tmp->iptv_item_list[0].iptv_ad;
                    }
                }
            }
        }
    }

    for(i = 0; i < list_num; i++)
    {
        if(0 == memcmp(&cur_play_item, &list_src->iptv_item_list[i], sizeof(IptvItemClass)))
        {
            cur_pos = i;
            break;
        }
    }

    GxCore_Free(list_tmp->iptv_item_list);
    GxCore_Free(list_tmp);

    return cur_pos;
}

int app_iptv_list_sort_progress(void)
{
    int pos_all = 0;
    int pos_sub = 0;
    static int s_sort_type = 0;

    pos_all = sort_iptv_list(s_iptv_ctrl.ch_list, s_sort_type, s_iptv_ctrl.play_prog_sel);
    pos_sub = sort_iptv_list(s_iptv_ctrl.sub_ch_list, s_sort_type, s_iptv_ctrl.play_prog_sel);
    s_sort_type = (s_sort_type == 0) ? 1 : 0;
    if (s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        app_iptv_service_save_remote_bak_list(s_iptv_ctrl.ch_list);
    else
        app_iptv_service_save_local_list(s_iptv_ctrl.ch_list);
    s_iptv_ctrl.play_prog_sel = (s_iptv_ctrl.cur_group_sel == 0) ? pos_all : pos_sub;

    return s_iptv_ctrl.play_prog_sel;
}

static int app_iptv_list_red_callback(int *item)
{
    if(app_iptv_list_check_sort_support())
    {
    	int sel = app_iptv_list_sort_progress();
    	GUI_SetProperty(g_CommonSelect.widget.list, "update_all", NULL);
    	GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
    	GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
        *item = sel;
    }
    return 0;
}

static void app_iptv_play_tv_radio_ctrl(void)
{
    if(app_iptv_list_check_fullscreen())
    {
        APP_TIMER_REMOVE(s_iptv_play_lastchannel_timer);
        extern void app_tv_radio_key_exec(void);
        app_tv_radio_key_exec();
        app_iptv_list_set_fullscreen(0);
    }
}


static void app_iptv_play_menu_ctrl(void)
{
    if(app_iptv_list_check_fullscreen())
    {
        extern int app_create_menu_wnd(void);
        app_create_menu_wnd();
        app_iptv_list_set_fullscreen(0);
#if IPTV_LITE_SUPPORT
        app_iptv_lite_clean_menu_flag();//press menu to exit iptv.
#endif
    }
}

static int app_iptv_list_green_callback(int *item)
{
#ifdef IPTV_CLOUD_SUPPORT
    if (app_iptv_list_check_change_source_support())
	{
			extern int app_iptv_cloud_create_dialog(void);
			GUI_EndDialog(g_CommonSelect.widget.wnd);
	        app_iptv_cloud_create_dialog();
	}
#endif
    return 0;
}

static int app_iptv_list_find_callback(int *item)
{
    GUI_EndDialog(g_CommonSelect.widget.wnd);
    GUI_SetInterface("flush", NULL);
    extern void app_iptv_create_find_list(void);
    app_iptv_create_find_list();

    return 0;
}

static status_t app_iptv_try_play(IptvItemClass *item)
{
    NetVideoPlayOps play_ops;
    char *prog_name = item->iptv_key;
    char *prog_url = item->iptv_value;

    memset(&play_ops, 0, sizeof(NetVideoPlayOps));
#ifdef IPTV_EPG_SUPPORT
    memcpy(&play_ops.epg_ctrl, &s_iptv_ctrl.iptv_ops.epg_ctrl, sizeof(NetappsEpgCtrl));
#endif
    _iptv_play_set_save_channel(prog_name, prog_url);

    play_ops.get_url = app_iptv_play_get_url_ctrl;
    play_ops.play_wnd_created = false;
    play_ops.source_num = 1;
    play_ops.bar_title = STR_ID_NET_IPTV;
    play_ops.netvideo_title = prog_name;
    play_ops.netvideo_url = prog_url;
    play_ops.list_focus_item = s_iptv_ctrl.play_prog_sel;
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
        play_ops.list_focus_item = app_iptv_fav_list_get_cur_play();
#endif
    play_ops.play_mode = s_iptv_ctrl.iptv_ops.fullscreen_flag;
    play_ops.cmb_sel = s_iptv_ctrl.cur_group_sel;
    play_ops.netvideo_image = NULL;

    play_ops.play_ctrl.play_ok   = NULL;
    play_ops.play_ctrl.play_exit = app_iptv_play_exit_ctrl;
    play_ops.play_ctrl.play_prev = app_iptv_play_prev_ctrl;
    play_ops.play_ctrl.play_next = app_iptv_play_next_ctrl;
    play_ops.play_ctrl.play_restart = app_iptv_play_restart_ctrl;

    play_ops.play_ctrl.play_list_total_get = app_iptvlist_list_get_total;
    play_ops.play_ctrl.play_list_data_get = app_iptv_play_list_data_get;
    play_ops.play_ctrl.play_list_ok = app_iptv_play_list_ok;
    play_ops.play_ctrl.play_list_cmb_title_create = app_iptv_list_cmb_group_build;
    play_ops.play_ctrl.play_list_cmb_title_free = app_iptv_play_cmb_title_free;
    play_ops.play_ctrl.play_list_change_group = app_iptv_play_change_group;
#ifdef IPTV_FAV_SUPPORT
    play_ops.play_ctrl.play_list_fav_check = app_iptv_fav_check;
    play_ops.play_ctrl.play_list_destroy = app_iptv_list_destroy;
#endif
    play_ops.play_ctrl.play_menu = app_iptv_play_menu_ctrl;
    play_ops.play_ctrl.play_tv_radio = app_iptv_play_tv_radio_ctrl;
    if(app_iptv_list_check_fullscreen())
    {
        if(app_iptv_list_check_sort_support())
        {
            play_ops.play_ctrl.play_list_red = app_iptv_list_red_callback;
            play_ops.play_ctrl.str_red = STR_ID_SORT;
        }
        if(app_iptv_list_check_change_source_support())
        {
            play_ops.play_ctrl.play_list_green = app_iptv_list_green_callback;
            play_ops.play_ctrl.str_green = STR_ID_SOURCE;
        }
    }
#ifdef IPTV_FAV_SUPPORT
    else if(app_iptv_isFavSupport())
    {
        play_ops.play_ctrl.play_list_red = app_iptv_list_fav_ctl_callback;
        play_ops.play_ctrl.str_red = STR_ID_FAV_UNFAV;
        play_ops.play_ctrl.play_list_green = app_iptv_list_favorites_callback;
        play_ops.play_ctrl.str_green = STR_ID_FAVORITES;
    }
#endif

    play_ops.play_ctrl.play_list_yellow = app_iptv_list_find_callback;
    play_ops.play_ctrl.str_yellow = STR_ID_FIND;

    return app_net_video_play(&play_ops);
}

static void app_iptv_last_play_get(void)
{
    int i = 0;
    int group_sel = 0;
    IptvListClass *temp_ch = (s_iptv_ctrl.cur_group_sel == 0) ? s_iptv_ctrl.ch_list : s_iptv_ctrl.sub_ch_list;
    bool malloc_flag = false;

    free_play_channel(s_iptv_ctrl.play_channel);
    s_iptv_ctrl.play_channel = NULL;
    if (s_iptv_ctrl.iptv_ops.get_save_channel != NULL)
        s_iptv_ctrl.play_channel = s_iptv_ctrl.iptv_ops.get_save_channel();

    if (s_iptv_ctrl.play_channel && s_iptv_ctrl.play_channel->group && s_iptv_ctrl.play_channel->url && s_iptv_ctrl.play_channel->name && s_iptv_ctrl.group_title)
    {
        for (group_sel = 0; group_sel < s_iptv_ctrl.group_total; group_sel++)
        {
            if (strncmp(s_iptv_ctrl.group_title[group_sel], s_iptv_ctrl.play_channel->group, MAX_IPTV_GROUP_LENGTH) != 0)
                continue;
            malloc_flag = false;
            if (group_sel != s_iptv_ctrl.cur_group_sel)
            {
                if((s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_UNKOWN)||(s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_GX))
                {
                    temp_ch = iptv_get_list_by_type(s_iptv_ctrl.ch_list, s_iptv_ctrl.play_channel->group); //需要iptv_list_free释放
                }
                else
                {
                    temp_ch = iptv_get_list_by_type_simple(s_iptv_ctrl.ch_list, s_iptv_ctrl.play_channel->group); //需要iptv_list_free释放
                }
                if (temp_ch == NULL)
                    break;
                malloc_flag = true;
            }
            for(i = 0; i < temp_ch->iptv_item_num; i++)
            {
                if (strncmp(s_iptv_ctrl.play_channel->name, temp_ch->iptv_item_list[i].iptv_key, MAX_IPTV_NAME_LENGTH) == 0
                        && strncmp(s_iptv_ctrl.play_channel->url, temp_ch->iptv_item_list[i].iptv_value, MAX_IPTV_URL_LENGTH) == 0)
                {
                    if (temp_ch && malloc_flag)
                    {
                        iptv_list_free(temp_ch);
                        temp_ch = NULL;
                    }
                    s_iptv_ctrl.cur_group_sel = group_sel;
                    s_iptv_ctrl.play_prog_sel = i;
                    app_iptv_list_update();
                    return;
                }
            }
            if (temp_ch && malloc_flag)
            {
                iptv_list_free(temp_ch);
                temp_ch = NULL;
            }
            break;
        }
    }

    s_iptv_ctrl.cur_group_sel = 0;
    s_iptv_ctrl.play_prog_sel = 0;
}

void app_iptv_play_ctrl(IPTVPlayChoice choice, int num)
{
    bool stop_play = true;
#ifdef IPTV_FAV_SUPPORT
    if(app_iptv_fav_list_dlg_check())
    {
        _change_to_all_group();
    }
#endif
    IptvListClass *tvList = (s_iptv_ctrl.cur_group_sel == 0) ? s_iptv_ctrl.ch_list : s_iptv_ctrl.sub_ch_list;

    switch (choice)
    {
        case IPTV_PLAY_POINT:
            s_iptv_ctrl.play_prog_sel = (num > 0 && num <= tvList->iptv_item_num) ? (num - 1) : (0);
            app_net_video_stop_play();
            break;
        case IPTV_PLAY_LIST:
            if (s_iptv_ctrl.play_group_sel == s_iptv_ctrl.cur_group_sel && ((num-1) == s_iptv_ctrl.play_prog_sel))
                return;
            s_iptv_ctrl.play_group_sel = s_iptv_ctrl.cur_group_sel;
            s_iptv_ctrl.play_prog_sel = (num > 0 && num <= tvList->iptv_item_num) ? (num - 1) : (0);
            break;

        case IPTV_PLAY_LAST:
            app_iptv_last_play_get();
            tvList = (s_iptv_ctrl.cur_group_sel == 0) ? s_iptv_ctrl.ch_list : s_iptv_ctrl.sub_ch_list;
            stop_play = false;
            break;

        case IPTV_PLAY_PAGE:
            s_iptv_ctrl.play_group_sel = s_iptv_ctrl.cur_group_sel;
            s_iptv_ctrl.play_prog_sel = s_iptv_ctrl.cur_page_num * COMMON_VIEW_MAX_PAGE_ITEM + s_iptv_ctrl.page_prog_sel;
            stop_play = false;
            goto play;
        default:
            return;
    }

    s_iptv_ctrl.cur_page_num = s_iptv_ctrl.play_prog_sel / COMMON_VIEW_MAX_PAGE_ITEM;
    s_iptv_ctrl.page_prog_sel = s_iptv_ctrl.play_prog_sel % COMMON_VIEW_MAX_PAGE_ITEM;
play:
    if (stop_play)
        app_net_video_stop_play();
    app_log_info("[%s:%d] play: cur_group:%d, prog_sel:%d\n", __func__, __LINE__, s_iptv_ctrl.cur_group_sel, s_iptv_ctrl.play_prog_sel);
    app_iptv_try_play(&tvList->iptv_item_list[s_iptv_ctrl.play_prog_sel]);
}

static char *file_path = NULL;
static char *file_path_bak = NULL;

static int _app_iptv_local_filedlg_exit_cb(WndStatus ret)
{
    int str_len = 0;

    if (ret == WND_OK)
    {
        app_get_file_real_path(&file_path);
        if (file_path != NULL)
        {
            ////backup the path...////
            APP_FREE(file_path_bak);
            str_len = strlen(file_path);
            file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if (file_path_bak != NULL)
            {
                memcpy(file_path_bak, file_path, str_len);
                file_path_bak[str_len] = '\0';
            }

            app_common_view_show_gif();
            char *tmp_path = NULL;
            if (strcasecmp(file_path+str_len-4,".m3u") == 0 || strcasecmp(file_path+str_len-5,".m3u8") == 0)
                tmp_path = m3u_convert_xml(file_path);
            else
                tmp_path = xml_convert_xml(file_path);
            app_send_msg_exec(APPMSG_IPTV_UPDATE_LOCAL_LIST, &tmp_path);
            app_iptv_list_source_set();
        }
    }
    return 0 ;
}

static void app_iptv_update_local_list(void)
{
    int ret = 0;
    ret = app_update_xml_m3u_path(file_path_bak, &file_path, _app_iptv_local_filedlg_exit_cb);
    if (ret == -1)
    {
        app_common_view_show_popup_msg(STR_ID_INSERT_USB, 1000);
        return;
    }
}

static void app_iptv_set_title(void)
{
    char buffer[64];

    if (s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        snprintf(buffer, sizeof(buffer), "%s IPTV", s_iptv_ctrl.iptv_ops.server_title);
    else
        snprintf(buffer, sizeof(buffer), "%s IPTV", STR_ID_LOCAL);
    app_set_widget_string(g_CommonView.widget.text_title, buffer);
}

static void app_iptv_key_menu_cb(void)
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if (s_iptv_ctrl.iptv_ops.source_num > 1)
        {
            switch(s_iptv_ctrl.list_source)
            {
                case IPTV_LIST_LOCAL:
                    s_iptv_ctrl.list_source = IPTV_LIST_REMOTE;
                    break;
                case IPTV_LIST_REMOTE:
                    s_iptv_ctrl.list_source = IPTV_LIST_LOCAL;
                    break;
                default:
                    return;
            }
            app_iptv_focus_area_change(area);
            app_iptv_list_source_set();
            return;
        }
    }
}

static void app_iptv_key_ok_cb(void)
{
    int sel = 0;
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        GUI_GetProperty(g_CommonView.widget.list_group, "select", &sel);
        if (s_iptv_ctrl.cur_group_sel != sel && sel >= 0)
        {
            s_iptv_ctrl.cur_group_sel = sel;
            app_iptv_list_update();
            s_iptv_ctrl.cur_page_num = 0;
            app_iptv_page_update(s_iptv_ctrl.cur_page_num);
            /*#389253 切换分组后重设list模式下的 select项*/
            if(g_CommonView.show_type == COMMON_VIEW_LIST_ITEM_TYPE)
            {
                g_CommonView.ctrl_data.item_sel = 0;
                sel = 0;
                GUI_SetProperty(g_CommonView.widget.list_item, "select", &sel);
            }
        }
    }
    else if (area == COMMON_VIEW_AREA_TABLE_ITEM)
    {
        s_iptv_ctrl.page_prog_sel = g_CommonView.ctrl_data.page_item_sel;
        app_iptv_play_ctrl(IPTV_PLAY_PAGE, 0);
    }
    else if (area == COMMON_VIEW_AREA_LIST_ITEM)
    {
        s_iptv_ctrl.play_prog_sel = -1;
        GUI_GetProperty(g_CommonView.widget.list_item, "select", &sel);
        app_iptv_play_ctrl(IPTV_PLAY_LIST, sel+1);
    }
}

static void app_iptv_key_red_server_cb(void)
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if (s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        {
            app_iptv_list_source_set();
        }
    }
}

#ifdef IPTV_CLOUD_SUPPORT
void app_iptv_cloud_source_change_notice(void)
{
    if(s_iptv_ctrl.wnd_flag == 0)
    {
        app_iptv_clean_all_info();
        app_iptv_list_get(s_iptv_ctrl.list_source);
    }
    else
    {
        s_iptv_ctrl.update_flag = 1;
    }
}

static void app_iptv_key_green_server_cb(void)
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if (s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        {
            extern int app_iptv_cloud_create_dialog(void);
            app_iptv_cloud_create_dialog();
        }
    }
}

static void app_iptv_key_blue_server_cb(void)
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if (s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        {
            extern int app_iptv_server_usb_add(void);
            if (app_iptv_server_usb_add() < 0)
                return;
            app_iptv_list_source_set();
        }
    }
}
#endif

int delete_iptv_list_by_item(IptvListClass *list_src, IptvItemClass * item)
{
    int i = 0;
    if ((list_src == NULL)||(list_src->iptv_item_num <= 0)||(item == NULL))
    {
        return -1;
    }
    for(i = 0;i < list_src->iptv_item_num;i++)
    {
        if ((strcmp(list_src->iptv_item_list[i].iptv_key,item->iptv_key) == 0)
            &&(strcmp(list_src->iptv_item_list[i].iptv_value,item->iptv_value) == 0)
            &&(strcmp(list_src->iptv_item_list[i].iptv_type,item->iptv_type) == 0))
        {
            break;
        }
    }
    /*free cur pos listitem for delete*/
    if(i < list_src->iptv_item_num)
    {
        APP_FREE(list_src->iptv_item_list[i].iptv_key);
        APP_FREE(list_src->iptv_item_list[i].iptv_value);
        APP_FREE(list_src->iptv_item_list[i].iptv_type);
        APP_FREE(list_src->iptv_item_list[i].iptv_ad.ad_name);
        APP_FREE(list_src->iptv_item_list[i].iptv_ad.ad_type);
        APP_FREE(list_src->iptv_item_list[i].iptv_ad.ad_value);
    }
    if (i < (list_src->iptv_item_num-1))
    {
        memcpy(&(list_src->iptv_item_list[i]),&(list_src->iptv_item_list[i+1]),sizeof(IptvItemClass)*(list_src->iptv_item_num-1-i));
    }

    list_src->iptv_item_num-=1;

    return 0;
}

int delete_iptv_list(IptvListClass *list_src, int cur_pos)
{
    if (list_src == NULL || cur_pos > list_src->iptv_item_num - 1)
    {
        return -1;
    }

    /*free cur pos listitem for delete*/
    APP_FREE(list_src->iptv_item_list[cur_pos].iptv_key);
    APP_FREE(list_src->iptv_item_list[cur_pos].iptv_value);
    APP_FREE(list_src->iptv_item_list[cur_pos].iptv_type);
    APP_FREE(list_src->iptv_item_list[cur_pos].iptv_ad.ad_name);
    APP_FREE(list_src->iptv_item_list[cur_pos].iptv_ad.ad_type);
    APP_FREE(list_src->iptv_item_list[cur_pos].iptv_ad.ad_value);

    if (cur_pos < (list_src->iptv_item_num-1))
    {
        memcpy(&(list_src->iptv_item_list[cur_pos]),&(list_src->iptv_item_list[cur_pos+1]),sizeof(IptvItemClass)*(list_src->iptv_item_num-1-cur_pos));
    }

    list_src->iptv_item_num-=1;
    return 0;
}
static int app_iptv_del_item_cb(PopDlgRet ret)
{
    int cur_foucs = 0;

    if (ret == POP_VAL_OK)
    {

        if(s_iptv_ctrl.ch_list->iptv_item_num == 1)
        {
            if(GxCore_FileExists(IPTV_LOCAL_DATABASE) == GXCORE_FILE_EXIST)
            {
                GxCore_FileDelete(IPTV_LOCAL_DATABASE);
                if(GxCore_FileExists(IPTV_LOCAL_FAVLIST_PATH) == GXCORE_FILE_EXIST)
                    GxCore_FileDelete(IPTV_LOCAL_FAVLIST_PATH);
                if(GxCore_FileExists(IPTV_LAST_CH_FILE_PATH) == GXCORE_FILE_EXIST)
                    GxCore_FileDelete(IPTV_LAST_CH_FILE_PATH);
            }
            app_iptv_list_source_set();
            app_common_view_show_popup_msg(STR_ID_DEL_CH_OK, 2000);
            /*delete all item ,focus area need to change to list group*/
            app_common_view_focus_area_change(COMMON_VIEW_AREA_LIST_GROUP);
        }
        else
        {
            if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
            {
                cur_foucs = g_CommonView.ctrl_data.page_sel* COMMON_VIEW_MAX_PAGE_ITEM + g_CommonView.ctrl_data.page_item_sel;
            }
            else
            {
                GUI_GetProperty(g_CommonView.widget.list_item, "select", &cur_foucs);
            }
            if(cur_foucs < 0)
            {
                app_log_error("cur_foucs error === %d \n",cur_foucs);
                return -1;
            }
            if (s_iptv_ctrl.cur_group_sel == 0)
            {
                delete_iptv_list(s_iptv_ctrl.ch_list,cur_foucs);
            }
            else
            {
                delete_iptv_list_by_item(s_iptv_ctrl.ch_list,&(s_iptv_ctrl.sub_ch_list->iptv_item_list[cur_foucs]));
            }

            app_iptv_service_save_local_list(s_iptv_ctrl.ch_list);

            app_iptv_group_rebuild();
            app_iptv_list_update();

            if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
            {
                if(s_iptv_ctrl.cur_page_num +1 > s_iptv_ctrl.page_total )
                {
                    s_iptv_ctrl.cur_page_num = s_iptv_ctrl.page_total-1;
                }
                app_common_view_update_page_info(s_iptv_ctrl.cur_page_num, s_iptv_ctrl.page_total);
            }
            else
            {
                if(cur_foucs >= s_iptv_ctrl.ch_list->iptv_item_num)
                {
                    cur_foucs = s_iptv_ctrl.ch_list->iptv_item_num - 1;
                    GUI_SetProperty(g_CommonView.widget.list_item, "select", &cur_foucs);
                }
                GUI_SetProperty( g_CommonView.widget.list_item, "update_all", NULL);
            }
            app_iptv_show_update(true);
            app_common_view_show_popup_msg(STR_ID_DEL_CH_OK, 2000);
        }
    }
    return 0;
}
static void app_iptv_key_red_cb(void)
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area != COMMON_VIEW_AREA_LIST_GROUP)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.format = POP_FORMAT_DLG;
        pop.str = "Sure to delete this Channel?";
        pop.title = "Delete Channel";
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = app_iptv_del_item_cb;
        popdlg_create(&pop);
    }
}
static int app_iptv_del_all_item_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        if(GxCore_FileExists(IPTV_LOCAL_DATABASE) == GXCORE_FILE_EXIST)
        {
            GxCore_FileDelete(IPTV_LOCAL_DATABASE);
            if(GxCore_FileExists(IPTV_LOCAL_FAVLIST_PATH) == GXCORE_FILE_EXIST)
                GxCore_FileDelete(IPTV_LOCAL_FAVLIST_PATH);
            if(GxCore_FileExists(IPTV_LAST_CH_FILE_PATH) == GXCORE_FILE_EXIST)
                GxCore_FileDelete(IPTV_LAST_CH_FILE_PATH);
        }
        app_iptv_list_source_set();
        app_common_view_show_popup_msg(STR_ID_DEL_CH_OK, 2000);
       /*delete all item ,focus area need to change to list group*/
       app_common_view_focus_area_change(COMMON_VIEW_AREA_LIST_GROUP);
    }
    return 0;
}

static void app_iptv_key_green_cb(void)
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area != COMMON_VIEW_AREA_LIST_GROUP)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.format = POP_FORMAT_DLG;
        pop.str = "Sure to delete All Channel?";
        pop.title = "Delete Channel";
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = app_iptv_del_all_item_cb;
        popdlg_create(&pop);
    }
}


static void app_iptv_key_blue_cb(void)
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if (s_iptv_ctrl.list_source == IPTV_LIST_LOCAL)
        {
            app_iptv_update_local_list();
        }
    }
}
static void app_iptv_key_yellow_cb(void)
{
    int sel = 0;
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_TABLE_ITEM)
    {
       sIptvMenuStyle = COMMON_VIEW_LIST_ITEM_TYPE;
    }
    else if (area == COMMON_VIEW_AREA_LIST_ITEM)
    {
        sIptvMenuStyle = COMMON_VIEW_TABLE_ITEM_TYPE;
    }
    g_CommonView.show_type = sIptvMenuStyle;
    GxBus_ConfigSetInt(IPTV_MENU_STYLE_KEY, sIptvMenuStyle);

    app_common_view_change_show_type();
    app_iptv_group_update();
    app_iptv_list_update();
    if(sIptvMenuStyle == COMMON_VIEW_LIST_ITEM_TYPE)
    {
        sel = s_iptv_ctrl.cur_page_num*COMMON_VIEW_MAX_PAGE_ITEM+g_CommonView.ctrl_data.page_item_sel;
        s_iptv_ctrl.page_prog_sel = g_CommonView.ctrl_data.page_item_sel;
        GUI_SetProperty(g_CommonView.widget.list_item, "update_all", NULL);
        GUI_SetProperty(g_CommonView.widget.list_item, "select", &sel);
        app_common_view_focus_area_change(COMMON_VIEW_AREA_LIST_ITEM);
    }
    else
    {
        GUI_GetProperty(g_CommonView.widget.list_item, "select", &sel);
        s_iptv_ctrl.cur_page_num = sel/COMMON_VIEW_MAX_PAGE_ITEM;
        s_iptv_ctrl.page_prog_sel = sel % COMMON_VIEW_MAX_PAGE_ITEM;
         g_CommonView.ctrl_data.page_item_sel = s_iptv_ctrl.page_prog_sel;
        app_iptv_page_update(s_iptv_ctrl.cur_page_num);
        app_common_view_focus_area_change(COMMON_VIEW_AREA_TABLE_ITEM);
        app_common_view_update_page_info(s_iptv_ctrl.cur_page_num, s_iptv_ctrl.page_total);
        /*modify kk 20230420  for bug #346829
          Page View的焦点是大于COMMON_VIEW_MAX_PAGE_ITEM的项，然后切换到List View，再切换
          到Page View，这时候app_iptv_list_update()会把page_sel设置为0，需要最后计算好焦点后再更新page info*/
    }
    return ;
}

static int app_iptv_busy_check(void)
{
    return s_iptv_ctrl.busy_flag;
}

static void app_iptv_page_sel_change(int page)
{
    s_iptv_ctrl.cur_page_num = page;
    app_iptv_page_update(s_iptv_ctrl.cur_page_num);
}

static void app_iptv_focus_area_change(int area)
{
    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        app_iptv_set_title();
        app_common_view_change_tips_key(STBK_OK, true, "Select", app_iptv_key_ok_cb);
        if (s_iptv_ctrl.list_source == IPTV_LIST_REMOTE)
        {
            if (s_iptv_ctrl.iptv_ops.source_num > 1)
                app_common_view_change_tips_key(STBK_MENU, true, STR_ID_LOCAL, app_iptv_key_menu_cb);
            app_common_view_change_tips_key(STBK_RED, true, STR_ID_REFRESH, app_iptv_key_red_server_cb);
            app_common_view_change_tips_key(STBK_YELLOW, false, NULL, NULL);
#ifdef IPTV_CLOUD_SUPPORT
            app_common_view_change_tips_key(STBK_GREEN, true, STR_ID_IPTV_ENTER_LINK, app_iptv_key_green_server_cb);
            app_common_view_change_tips_key(STBK_BLUE, true, STR_ID_IPTV_IMPORT_LINK, app_iptv_key_blue_server_cb);
#else
            app_common_view_change_tips_key(STBK_GREEN, false, NULL, NULL);
            app_common_view_change_tips_key(STBK_BLUE, false, NULL, NULL);
#endif
        }
        else
        {
            if (s_iptv_ctrl.iptv_ops.source_num > 1)
                app_common_view_change_tips_key(STBK_MENU, true, s_iptv_ctrl.iptv_ops.server_title, app_iptv_key_menu_cb);
            app_common_view_change_tips_key(STBK_RED, false, NULL, NULL);
            app_common_view_change_tips_key(STBK_GREEN, false, NULL, NULL);
            app_common_view_change_tips_key(STBK_YELLOW, false, NULL, NULL);
            app_common_view_change_tips_key(STBK_BLUE, true, "Import Channel", app_iptv_key_blue_cb);
        }
    }
    else
    {
        app_common_view_change_tips_key(STBK_OK, true, "Play", app_iptv_key_ok_cb);
        app_common_view_change_tips_key(STBK_MENU, false, NULL, NULL);
        app_common_view_change_tips_key(STBK_RED, true, "Delete Channel", app_iptv_key_red_cb);
        app_common_view_change_tips_key(STBK_GREEN, true, "Delete All", app_iptv_key_green_cb);

        if (area == COMMON_VIEW_AREA_TABLE_ITEM)
        {
            app_common_view_change_tips_key(STBK_YELLOW, true,STR_ID_IPTV_LISTVIEW, app_iptv_key_yellow_cb);
        }
        else
        {
            app_common_view_change_tips_key(STBK_YELLOW, true, "Page View", app_iptv_key_yellow_cb);
        }
        app_common_view_change_tips_key(STBK_BLUE, false, NULL, NULL);
    }
}

int app_iptv_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    return s_iptv_ctrl.group_total;
}

int app_iptv_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if (NULL == item || 0 > item->sel)
        return EVENT_TRANSFER_STOP;
    if (s_iptv_ctrl.group_title == NULL)
        return EVENT_TRANSFER_STOP;

    //col-0: channel name
    item->image = NULL;
    item->string = s_iptv_ctrl.group_title[item->sel];

    return EVENT_TRANSFER_STOP;
}
int app_iptv_item_list_get_total(GuiWidget *widget, void *usrdata)
{
    if (s_iptv_ctrl.ch_list && s_iptv_ctrl.ch_list->iptv_item_num > 0)
    {
        if (s_iptv_ctrl.cur_group_sel == 0)
        {
            /*all group*/
            return s_iptv_ctrl.ch_list->iptv_item_num;
        }
        else
        {
            /*sub group*/
            if (s_iptv_ctrl.sub_ch_list)
            {
                return s_iptv_ctrl.sub_ch_list->iptv_item_num;
            }
        }
    }
    return 0;
}

int app_iptv_item_list_get_data(GuiWidget *widget, void *usrdata)
{
    static char buffer1[8];
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if (NULL == item || 0 > item->sel)
        return EVENT_TRANSFER_STOP;
    if (s_iptv_ctrl.ch_list == NULL)
        return EVENT_TRANSFER_STOP;

    //col-0: channel No.
     item->image = NULL;
     sprintf(buffer1, "%03d", (item->sel + 1));
     item->string = buffer1;

     //col-1: channel name
     item = item->next;
     item->image = NULL;

    if (s_iptv_ctrl.cur_group_sel == 0)
    {
        item->string = s_iptv_ctrl.ch_list->iptv_item_list[item->sel].iptv_key;
    }
    else
    {
        if (s_iptv_ctrl.sub_ch_list)
        {
            item->string = s_iptv_ctrl.sub_ch_list->iptv_item_list[item->sel].iptv_key;
        }
    }


    return EVENT_TRANSFER_STOP;
}

int app_iptv_create(GuiWidget *widget, void *usrdata)
{
#ifdef SUPPORT_IPTVLIST_PRESET
    app_iptv_local_preset();
#endif
    return EVENT_TRANSFER_STOP;
}

int app_iptv_destroy(GuiWidget *widget, void *usrdata)
{
    s_iptv_ctrl.busy_flag = 0;
    s_iptv_ctrl.wnd_flag = 0;
    s_iptv_ctrl.direct_play_flag = 0;
    s_iptv_ctrl.update_flag = 0;
    app_iptv_clean_all_info();
#ifdef IPTV_FAV_SUPPORT
	app_iptv_free_favlist();
    if(s_iptv_ctrl.iptv_indexlist)
    {
        iptv_indexlist_free(s_iptv_ctrl.iptv_indexlist);
        s_iptv_ctrl.iptv_indexlist = NULL;
    }
#endif
    if (s_iptv_ctrl.iptv_ops.source_num > 1)
    {
        app_iptv_set_source(s_iptv_ctrl.list_source);
    }
    memset(&s_iptv_ctrl.iptv_ops, 0, sizeof(IPTVOps));

    return EVENT_TRANSFER_STOP;
}

int app_iptv_got_focus(GuiWidget *widget, void *usrdata)
{
    s_iptv_ctrl.wnd_flag = 1;
    if (s_iptv_ctrl.update_flag == 1)
    {
        app_iptv_list_source_set();
        s_iptv_ctrl.update_flag = 0;
    }

    if(COMMON_VIEW_TABLE_ITEM_TYPE == sIptvMenuStyle)
    {
        app_iptv_show_update(false);
		if(s_iptv_ctrl.page_total > 1)
        {
            GUI_SetProperty(g_CommonView.widget.img_down, "state", "show");
        }
    }
    return EVENT_TRANSFER_STOP;
}

static void _set_text_widget_str_rolling_stop(void)
{
    char temp_widget[64];
    memset(temp_widget,0,sizeof(temp_widget));
    sprintf(temp_widget, "%s%d", g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel+1);
    GUI_SetProperty(temp_widget,"format","static");
}

int app_iptv_lost_focus(GuiWidget *widget, void *usrdata)
{
    s_iptv_ctrl.wnd_flag = 2;
    app_common_view_hide_gif();
    app_common_view_hide_popup_msg();
    _set_text_widget_str_rolling_stop();
    GUI_SetProperty(g_CommonView.widget.img_down, "state", "hide");
    return EVENT_TRANSFER_STOP;
}

static int _iptv_power_on_play_noprogram_exitcb(PopDlgRet ret)
{
    g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
    g_AppFullArb.state.tv = STATE_ON;
    app_iptv_play_tv_radio_ctrl();
    return 0;
}

int app_iptv_browser_msg_proc(GxMessage *msg)
{
    AppMsg_IptvList *iptv_list = NULL;
    char **pic_path = NULL;
    GxMsgProperty_NetDevState *dev_state = NULL;
    if (msg == NULL)
        return EVENT_TRANSFER_STOP;

    switch(msg->msg_id)
    {
        case APPMSG_IPTV_LIST_DONE:
            s_iptv_ctrl.busy_flag = 0;
            iptv_list = GxBus_GetMsgPropertyPtr(msg, AppMsg_IptvList);
            if (s_iptv_ctrl.wnd_flag == 1)
            {
                app_common_view_hide_gif();
                app_common_view_hide_popup_msg();
            }
            if (iptv_list && iptv_list->iptv_list)
            {
                if ((s_iptv_ctrl.wnd_flag != 0 || s_iptv_ctrl.direct_play_flag != 0)
                        && (s_iptv_ctrl.list_source == iptv_list->iptv_source)
                        && (s_iptv_ctrl.list_source != IPTV_LIST_REMOTE || s_iptv_ctrl.iptv_ops.server_type == iptv_list->iptv_server))
                {
                    app_iptv_list_init(iptv_list->iptv_list);
                    app_iptv_group_update();
                    app_iptv_list_update();
                    app_iptv_show_update(false);
                }
                else
                {
                    iptv_list_free(iptv_list->iptv_list);
                }
            }
            else
            {
                if (strcmp(GUI_GetFocusWindow(), WND_NETVIDEO_PLAY) == 0)
                {
                    PopDlg pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.format = POP_FORMAT_DLG;
                    pop.mode = POP_MODE_UNBLOCK;

                    if (s_iptv_ctrl.iptv_ops.fullscreen_flag)
                    {
                        if (iptv_list && !iptv_list->iptv_list)
                        {
                            GUI_EndDialog(WND_NETVIDEO_PLAY);
                            pop.str = STR_ID_NO_PROGRAM;
                            pop.timeout_sec = -1;
                            pop.exit_cb = _iptv_power_on_play_noprogram_exitcb;
                            popdlg_create(&pop);
                        }
                        break;
                    }
                    GUI_EndDialog(WND_NETVIDEO_PLAY);
                    if(iptv_list->retcode == GXAPPS_SERVER_ERROR)
                    {
                        pop.str = STR_ID_NO_RESULTS;
                    }
                    else if(iptv_list->retcode == GXAPPS_NETWORK_ERROR)
                    {
                        pop.str = STR_ID_SERVER_FAIL;
                    }
                    else if(iptv_list->retcode == GXAPPS_INTERNAL_ERROR)
                    {
                        pop.str = STR_ID_INTER_ERROR;
                    }
                    else if(iptv_list->retcode == GXAPPS_UNKNOWN_ERROR)
                    {
                        pop.str = STR_ID_UNKNOWN_ERROR;
                    }
                    else if(iptv_list->retcode == GXAPPS_AUTH_ERROR)
                    {
                        pop.str = STR_ID_AUTH_ERROR;
                    }
                    else
                    {
                        pop.str = STR_ID_UNKNOWN_ERROR;
                    }
                    pop.timeout_sec = 3;
                    popdlg_create(&pop);
                }
#ifdef IPTV_CLOUD_SUPPORT
                if (strcmp(GUI_GetFocusWindow(), WND_COMMON_VIEW) == 0)
                {
                    if(s_iptv_ctrl.list_source == IPTV_LIST_REMOTE && _get_remote_channels_total())
                        app_common_view_show_popup_msg(STR_ID_SERVER_FAIL, 3000);
                }
#endif
            }
            break;

        case APPMSG_IPTV_AD_PIC_DONE:
            pic_path = GxBus_GetMsgPropertyPtr(msg, char*);
            if (*pic_path != NULL)
                GxCore_FileDelete(*pic_path);
            break;

        case GXMSG_NETWORK_STATE_REPORT:
            if(GUI_CheckDialog(WND_COMMON_VIEW) != GXCORE_SUCCESS)
            {
                break;
            }
            if (s_iptv_ctrl.wnd_flag == 0 && s_iptv_ctrl.direct_play_flag == 0)
                break;
            dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            if (s_iptv_ctrl.ch_list == NULL && s_iptv_ctrl.list_source == IPTV_LIST_REMOTE
                    && dev_state->dev_state == IF_STATE_CONNECTED)
            {
                app_iptv_list_source_set();
            }
            break;

        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

static int app_iptv_play_last_channel(void *userdata)
{
	IptvListClass *iptvChannel = NULL;

	iptvChannel = app_iptv_getIptvList();
	if(iptvChannel&&(iptvChannel->iptv_item_num>0))
	{
		APP_TIMER_REMOVE(s_iptv_play_lastchannel_timer);
		if(get_iptv_play_mode())
		{
			app_iptv_play_ctrl(IPTV_PLAY_LAST, 0);
		}
	}

	return 0;
}

static void app_iptv_play_create_ctrl(void)
{
    APP_TIMER_ADD(s_iptv_play_lastchannel_timer, app_iptv_play_last_channel, 300, TIMER_REPEAT);
}

status_t app_create_iptv_menu(IPTVOps *ops)
{
    CommonViewParas paras = {0};
    NetVideoPlayOps play_ops;

    GxBus_ConfigGetInt(IPTV_MENU_STYLE_KEY, &sIptvMenuStyle, IPTV_MENU_STYLE_VALUE);
    memset(&s_iptv_ctrl.iptv_ops, 0, sizeof(IPTVOps));
    memcpy(&s_iptv_ctrl.iptv_ops, ops, sizeof(IPTVOps));

    if (s_iptv_ctrl.iptv_ops.source_num > 1 || s_iptv_ctrl.iptv_ops.server_type == IPTV_SERVER_GX)
        s_iptv_ctrl.list_source = app_iptv_get_source();
    else if(s_iptv_ctrl.iptv_ops.server_type != IPTV_SERVER_UNKOWN)
        s_iptv_ctrl.list_source = IPTV_LIST_REMOTE;
    else
        s_iptv_ctrl.list_source = IPTV_LIST_LOCAL;

    app_iptv_service_start();
    app_iptv_clean_data();

    s_iptv_ctrl.busy_flag = 0;
    s_iptv_ctrl.update_flag = 0;
    if(s_iptv_ctrl.iptv_ops.auto_play == 0)
    {
        s_iptv_ctrl.wnd_flag = 1;
        s_iptv_ctrl.direct_play_flag = 0;

        paras.show_type = sIptvMenuStyle;
        paras.focus_area = COMMON_VIEW_AREA_LIST_GROUP;
        paras.orig_group_sel = 0;
        paras.net_video_flag = true;
        paras.string.str_title = STR_ID_NET_IPTV;

        paras.keyfunc.busy_check = app_iptv_busy_check;
        paras.keyfunc.page_sel_change = app_iptv_page_sel_change;
        paras.keyfunc.focus_area_change = app_iptv_focus_area_change;

        paras.signal.create= app_iptv_create;
        paras.signal.destroy = app_iptv_destroy;
        paras.signal.got_focus = app_iptv_got_focus;
        paras.signal.lost_focus = app_iptv_lost_focus;
        paras.signal.group_list_get_total = app_iptv_group_list_get_total;
        paras.signal.group_list_get_data = app_iptv_group_list_get_data;
        paras.signal.item_list_get_total = app_iptv_item_list_get_total;
        paras.signal.item_list_get_data = app_iptv_item_list_get_data;

        app_common_view_create_dialog(&paras);
        app_iptv_list_source_set();
    }
    else
    {
        s_iptv_ctrl.wnd_flag = 0;
        s_iptv_ctrl.direct_play_flag = 1;
        s_iptv_ctrl.cur_group_sel = 0;
        app_iptv_list_get(s_iptv_ctrl.list_source);
        memset(&play_ops, 0, sizeof(NetVideoPlayOps));
        play_ops.bar_title = STR_ID_NET_IPTV;
        play_ops.play_mode = s_iptv_ctrl.iptv_ops.fullscreen_flag;
        play_ops.play_ctrl.play_menu = app_iptv_play_menu_ctrl;
        play_ops.play_ctrl.play_exit = app_iptv_play_exit_ctrl;
        play_ops.play_ctrl.play_create = app_iptv_play_create_ctrl;
        play_ops.play_ctrl.play_tv_radio = app_iptv_play_tv_radio_ctrl;
        app_net_video_play(&play_ops);
    }

    return GXCORE_SUCCESS;
}

int app_iptv_getGroupNum(void)
{
    return s_iptv_ctrl.group_total;
}

int app_iptv_getCurGroupSelected(void)
{
    return s_iptv_ctrl.cur_group_sel;
}

char **app_iptv_getAllGroup(void)
{
    return s_iptv_ctrl.group_title;
}

IptvListClass *app_iptv_getIptvList(void)
{
    return (s_iptv_ctrl.cur_group_sel == 0) ? (s_iptv_ctrl.ch_list) : (s_iptv_ctrl.sub_ch_list);
}

int app_iptv_getCurIptvSelected(void)
{
    return s_iptv_ctrl.play_prog_sel;
}

void app_iptv_changeGroupInList(int sel)
{
    if (s_iptv_ctrl.cur_group_sel != sel)
    {
        s_iptv_ctrl.cur_group_sel = sel;
        app_iptv_list_update();
    }
}

#ifdef IPTV_EPG_SUPPORT
void app_iptv_free_daylist(EpgDayList *daylist)
{
	int i = 0;
	if(daylist)
	{
		if(daylist->day_item)
		{
			if(daylist->day_num>0)
			{
				for(i=0; i<daylist->day_num; i++)
				{
					APP_FREE(daylist->day_item[i].day_id);
					APP_FREE(daylist->day_item[i].title);
				}
			}
			APP_FREE(daylist->day_item);
		}
		APP_FREE(daylist);
	}
}

void app_iptv_free_epglist(EpgEventList *epglist)
{
	int i = 0;
	if(epglist)
	{
		if(epglist->epg_item)
		{
			if(epglist->epg_num>0)
			{
				for(i=0; i<epglist->epg_num; i++)
				{
					APP_FREE(epglist->epg_item[i].start_time);
					APP_FREE(epglist->epg_item[i].end_time);
					APP_FREE(epglist->epg_item[i].title);
					APP_FREE(epglist->epg_item[i].description);
				}
			}
			APP_FREE(epglist->epg_item);
		}
		APP_FREE(epglist);
	}
}

char *app_iptv_get_time_str(char *timestamp)
{
	time_t time = 0;
	char *time_str = NULL;
	time = get_display_time_by_timezone((time_t)atoi(timestamp));
	time_str = app_time_to_hourmin_edit_str(time);
	return time_str;
}
#endif

#if IPTV_LITE_SUPPORT
void app_iptv_lite_create_menu(void)
{
    IPTVOps ops;

    s_iptv_lite_menuc_flag = 1;
    memset(&ops, 0, sizeof(IPTVOps));
    ops.server_type = IPTV_SERVER_GX;
    ops.server_title = STR_ID_BLANK;
    ops.source_num = 1;
    ops.auto_play = 1;
    ops.remote_fastget = 1;
    ops.fullscreen_flag = 1;
    ops.iptv_lite_flag = 1;
    ops.set_save_channel = set_iptv_save_channel;
    ops.get_save_channel = get_iptv_save_channel;
#ifdef IPTV_FAV_SUPPORT
    ops.get_fav_list = get_local_iptv_fav_list;
    ops.save_fav_list = save_local_iptv_fav_list;
#endif
    app_iptv_list_set_fullscreen(ops.fullscreen_flag);
    app_create_iptv_menu(&ops);
}
static int sIptv_PowerON_play_flag = 1;

int app_iptv_poweron_play_flag_get(void)
{
    return sIptv_PowerON_play_flag ;
}

void app_iptv_poweron_play_flag_set(int flag)
{
    sIptv_PowerON_play_flag = flag;
}

int app_iptv_poweron_play(void)
{
	status_t retval = GXCORE_ERROR;
	GxMessage *new_msg = NULL;
	if(sIptv_PowerON_play_flag == 0)
	{
		APP_Printf("===%s, %d=====>>>  no need to play iptv \n",__FUNCTION__, __LINE__);
		return 0;
	}
    new_msg = GxBus_MessageNew(APPMSG_IPTV_POWERON_PLAY);
    if(NULL == new_msg )
    {
        APP_Printf("===%s, %d=====>>>  GxBus_MessageNew failed \n",__FUNCTION__, __LINE__);
        return 0;
    }

    retval = GxBus_MessageSendWait(new_msg);
    if(GXCORE_ERROR == retval )
    {
        APP_Printf("===%s, %d=====>>>  GxBus_MessageSend failed \n",__FUNCTION__, __LINE__);
    }

    GxBus_MessageFree(new_msg);

    return 0;
}
static int _poweron_play_cb_exit(PopDlgRet ret)
{
	ubyte64 cur_dev;
	GxMsgProperty_NetDevState state;

	memset(cur_dev, 0, sizeof(ubyte64));
	app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
	memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
	strcpy(state.dev_name, cur_dev);
	app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
	if (state.dev_state != IF_STATE_CONNECTED)
	{
		extern void app_prog_play_resume_exec(void);
		app_prog_play_resume_exec();
	}
	sIptv_PowerON_play_flag = 0;
	return 0;
}

int app_iptv_poweron_play_tips(void)
{
	PopDlg pop = {0};
	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_NO_BTN;
	pop.format = POP_FORMAT_DLG;
	pop.str = "Connecting...";
	pop.mode = POP_MODE_UNBLOCK;
	pop.timeout_sec = 40;
	pop.exit_cb = _poweron_play_cb_exit;
	popdlg_create(&pop);
	return 0;
}

int app_iptv_lite_is_active(void)
{
	ubyte64 cur_dev;
	GxMsgProperty_NetDevState state;

	memset(cur_dev, 0, sizeof(ubyte64));
	app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
	memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
	strcpy(state.dev_name, cur_dev);
	app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
	if((state.dev_state == IF_STATE_CONNECTED)&&(GxCore_FileExists(IPTV_LOCAL_DATABASE) == GXCORE_FILE_EXIST))
	{
		return 1;
	}

	return 0;
}

int app_iptv_lite_check(void)
{
    if((app_iptv_lite_is_active())&&(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
		&&(s_iptv_lite_menuc_flag) )
    {
        return 1;
    }

    return 0;
}

#endif

int app_iptv_database_check(void)
{
    if(GxCore_FileExists(IPTV_LOCAL_DATABASE) == GXCORE_FILE_EXIST)
    {
        return GXCORE_FILE_EXIST;
    }
    else
    {
        return GXCORE_FILE_UNEXIST;
    }
}

app_netapp_event_class app_netapp_event_iptv = {
	.name = "netapp iptv",
	.msg_wndName = NULL,
	.msg_proc_callback = app_iptv_browser_msg_proc,
	.msg_dev_out_callback = NULL,
	.key_net_open_check = NULL,
};

#endif

