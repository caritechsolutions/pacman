#include "app.h"
#if DLNADMP_SUPPORT || DLNASAT2IP_SUPPORT
#include "app_windows.h"
#include "app_keyboard_language.h"
#include "dlna.h"
#include "app_module.h"
#include "app_dlna.h"
#include "../app_netvideo_play.h"
#include <pthread.h>

#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#define MAX_PATH_LENGTH 512
#define IMG_RED                 "s_ts_red"
#define IMG_NET					"MP_ICON_NET"
#define IMG_DIR					"MP_ICON_FILE"
#define IMG_MOVIC				"MP_ICON_MOVICE"
#define IMG_PIC					"MP_ICON_PIC"
#define IMG_MUSIC				"MP_ICON_MUSIC"
#define IMG_UNKNOW				"MP_ICON_UNKNOW"

#ifdef ECOS_OS
#define IMG_MUSIC_PLAY_BACKGROUND    WORK_PATH"theme/image/bg.jpg"
#else
#define IMG_MUSIC_PLAY_BACKGROUND    WORK_PATH"theme/image/netapps/music_back.jpg"
#endif

#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#define IMG_GIF                 "img_dlna_dmp_gif"

#define WND_DLNA_DMP_TITLE "text_dlna_dmp_title"
#define WND_LISTVIEW_FILTER         "listview_dlna_dmp_filter"
#define WND_LISTVIEW         "listview_dlna_dmp"
#define LISTVIEW_BROWSE_PATH "text_dlna_dmp_path1"
#define LISTVIEW_INDEX_INFO "text_dlna_dmp_listview_index_info"

#define MAX_LIST_ITEM 10
#define MAX_LIST_FILTER_ITEM 4

static const char *FilterItemText[] = {
    "All",
    "Video",
    "Audio",
    "Picture"
};

static const char *FilterSat2IpItemText[] = {
    "All",
    "TV",
    "Radio"
};

typedef enum _FilterEnum {
	FILTER_ALL = 0,
	FILTER_VIDEO,
	FILTER_MUSIC,
	FILTER_PICTURE
}FilterEnum;

struct ThisUICtrol{
    event_list *gif_timer;
	int browse_level;
	int filter;
	bool filter_empty;
	bool is_sat2ip;
	DMPServerList *server_list;
	DMPServerNode *server_cur;
	DMPContentDirNode *dir_cur;
};

static struct ThisUICtrol this = {0};

static int _dmp_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;

	GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	return 0;
}

static void _dmp_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;

	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _dmp_free_gif(void)
{
    APP_TIMER_REMOVE(this.gif_timer);
	this.gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _dmp_show_gif(void)
{
	GUI_SetProperty(IMG_GIF, "state", "show");
	APP_TIMER_ADD(this.gif_timer, _dmp_draw_gif, 100, TIMER_REPEAT);
}

static void _dmp_hide_gif(void)
{
	APP_TIMER_REMOVE(this.gif_timer);
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

static int _get_ip(char *ip)
{
    GxMsgProperty_NetDevIpcfg s_ip_cfg;

    memset(&s_ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &s_ip_cfg.dev_name);
    if(s_ip_cfg.dev_name[0] == 0)
        return -1;
    app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_ip_cfg);
    strcpy(ip, s_ip_cfg.ip_addr);
    return 0;
}

static int _dmp_popdlg_cb_exit_dmp(PopDlgRet ret)
{
    GUI_EndDialog(WND_DLNA_DMP);
	return 0;
}

static void _dmp_show_popdlg(char *str, int32_t time_out, ExitCb exit_cb)
{
    if(NULL == str || time_out < 0)
        return;

	if(time_out == 0)
		time_out = 0xffffffff;

    PopDlg pop;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = str;
    pop.timeout_sec = time_out;
    pop.exit_cb = exit_cb;
    popdlg_create(&pop);

    app_update_pop_dlg(WND_POP_BOOK);

    return;
}

static int _server_event(DlnaEventTypeEnum event_type)
{
    AppMsg_DlnaUIControl ui_msg = {0};
    switch(event_type)
    {
        case DLNA_EVENT_SERVER_ADD:
       	{
    		ui_msg.type = UI_DMP_ADD_SERVER;
    		app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
            break;
        }
		default:
			break;
	}
    return 0;
}

static int _action_event(DlnaEventTypeEnum event_type)
{
    AppMsg_DlnaUIControl ui_msg = {0};
    switch(event_type)
    {
        case DLNA_EVENT_CONTENTSDIR_GET:
        {
		 	ui_msg.type = UI_DMP_UPDATE_DIR;
    		app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
            break;
        }
        case DLNA_EVENT_CONTENTSDIR_NULL:
        case DLNA_EVENT_ACTION_ACK_ERROR:
        {
		 	ui_msg.type = UI_DMP_ACTION_ACK_ERROR;
    		app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
            break;
        }
		default:
			break;
	}
    return 0;
}

static DMPContentDirList *_get_bottom_dir_list(void)
{
	DMPContentDirList *cur = NULL;
	int i = 0;

	if(NULL == this.server_list || NULL == this.server_list->dir_list)
		return NULL;

	cur = this.server_list->dir_list;
	for(i = 1; i < this.browse_level; i++)
	{
		cur = cur->child;
	}

	return cur;
}

static void _set_browse_path(void)
{
	char path[MAX_PATH_LENGTH] = {0};
	DMPContentDirList *cur = NULL;
	int i = 0;
	int len = 0;

    strcat(path,this.server_cur->friendly_name);
    strcat(path,"/");
	if(NULL == this.server_list || NULL == this.server_list->dir_list)
	{
		GUI_SetProperty(LISTVIEW_BROWSE_PATH, "string", path);
		return;
	}

	cur = this.server_list->dir_list;
	for(i = 1; i < this.browse_level; i++)
	{
		cur = cur->child;
		if(NULL != cur->parent_node)
		{
			len = strlen(path) + strlen(cur->parent_node->title) + 2;
			snprintf(path, len, "%s/%s", path, cur->parent_node->title);
		}
	}
	GUI_SetProperty(LISTVIEW_BROWSE_PATH, "string", path);
}

static void _set_browse_index_info(int total, int item)
{
	char info[20] = "\n";

    snprintf(info, sizeof(info), "%d/%d",item, total);
	GUI_SetProperty(LISTVIEW_INDEX_INFO, "string", info);
}

static void _filter_dir_list(void)
{
	int count = 0;
	DMPContentDirList *dir_list;
	DMPContentDirNode *node;

	this.filter_empty = false;
	dir_list = _get_bottom_dir_list();
	if(NULL == dir_list)
		return;

	ithread_mutex_lock(&dir_list->mutex);
	node = dir_list->next;
	while(node != dir_list)
	{
		if( (this.filter == FILTER_ALL) ||
			(node->media_type == DLNA_CONTAINER) ||
			(this.filter ==FILTER_VIDEO && node->media_type == DLNA_VIDEO) ||
			(this.filter ==FILTER_MUSIC && node->media_type == DLNA_AUDIO) ||
			(this.filter ==FILTER_PICTURE && node->media_type == DLNA_IMAGE))
		{
			count++;
			node->count = count; // 重新构建过滤后的item节点序号
		}
		node = node->next;
	}

	if(count == 0)
	{
		this.filter_empty = true;
		count = 1;
		dir_list->count = count; // 设置为1,该行为empty提示
		ithread_mutex_unlock(&dir_list->mutex);
		_set_browse_index_info(0, 0);
	}
	else
	{
		dir_list->count = count; // 设置过滤后的list长度
		ithread_mutex_unlock(&dir_list->mutex);
		_set_browse_index_info(dir_list->count, 1);
	}
}

void app_create_dlna_dmp_menu(void)
{
    ubyte32 ip_address = {0};

    if(_get_ip(ip_address) < 0)
    {
        _dmp_show_popdlg(STR_ID_LINK_NETWORK, 2, NULL);
        return;
    }
    if(GUI_CheckDialog(WND_DLNA_DMP) == GXCORE_ERROR)
    {
#if SAT2IP_SERVER_SUPPORT
		if (app_sat2ip_operate_tip_get_force() < 0)
			return;
		app_sat2ip_unset_gui_ok_fast();
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
        app_dvb2ip_gui_flag_set(0);
#endif
		memset(&this, 0, sizeof(struct ThisUICtrol));
        GUI_CreateDialog(WND_DLNA_DMP);
    }
}

void app_create_dlna_sat2ip_menu(void)
{
    ubyte32 ip_address = {0};

    if(_get_ip(ip_address) < 0)
    {
        _dmp_show_popdlg(STR_ID_LINK_NETWORK, 2, NULL);
        return;
    }
    if(GUI_CheckDialog(WND_DLNA_DMP) == GXCORE_ERROR)
    {
#if SAT2IP_SERVER_SUPPORT
		if (app_sat2ip_operate_tip_get_force() < 0)
			return;
		app_sat2ip_unset_gui_ok_fast();
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
        app_dvb2ip_gui_flag_set(0);
#endif
		memset(&this, 0, sizeof(struct ThisUICtrol));
		this.is_sat2ip = true;
        GUI_CreateDialog(WND_DLNA_DMP);
    }
}


SIGNAL_HANDLER int app_dlna_dmp_create(GuiWidget *widget, void *usrdata)
{
    DMPConfig config = {0};
    ubyte32 ip_address = {0};
	int port = 0;
	int item = 0;
	_dmp_load_gif();
    if(_get_ip(ip_address) < 0)
    {
        _dmp_show_popdlg(STR_ID_LINK_NETWORK, 0, _dmp_popdlg_cb_exit_dmp);
        return EVENT_TRANSFER_STOP;
    }

    config.ip_address = ip_address;
    config.port = port;
    config.enable_device = this.is_sat2ip ? SAT2IP_SERVER : MEDIA_SERVER|SAT2IP_SERVER;
    config.max_http_content = 0;
    this.server_list = DMP_start(&config, _server_event);
	if(NULL == this.server_list)
	{
        _dmp_show_popdlg(STR_ID_INVALID, 0, _dmp_popdlg_cb_exit_dmp);
        return EVENT_TRANSFER_STOP;
	}
	GUI_SetProperty(WND_LISTVIEW_FILTER, "active", &item);
    GUI_SetFocusWidget(WND_LISTVIEW);
	_dmp_show_gif();
	GUI_SetProperty(LISTVIEW_INDEX_INFO, "string", "");
	if(this.is_sat2ip)
	{
		GUI_SetProperty(WND_DLNA_DMP_TITLE, "string", "SAT2IP Player");
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dlna_dmp_destroy(GuiWidget *widget, void *usrdata)
{
	DMP_stop(&this.server_list);
    _dmp_hide_gif();
	_dmp_free_gif();
    gal_img_threshod_size(0);
#if SAT2IP_SERVER_SUPPORT
	app_sat2ip_set_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dlna_dmp_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_DLNA_DMP);
                break;
            default:
                break;
        }
    }
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dlna_dmp_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dlna_dmp_got_focus(GuiWidget *widget, void *usrdata)
{
	AppMsg_DlnaUIControl ui_msg;

	ui_msg.type = UI_DMP_GOT_FOCUS;
	app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int listview_dlna_dmp_filter_get_data(const char* widgetname, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;

	/*col-0: img*/
	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	/*col-1: file name*/
	item = item->next;
	if(NULL == item)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	item->string = this.is_sat2ip ? (char *)FilterSat2IpItemText[item->sel] : (char *)FilterItemText[item->sel];

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int listview_dlna_dmp_filter_change(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int listview_dlna_dmp_filter_get_count(const char* widgetname, void *usrdata)
{

	return this.is_sat2ip ? (MAX_LIST_FILTER_ITEM - 1) : MAX_LIST_FILTER_ITEM;
}

static void _listview_dlna_dmp_filter_ok_keypress(void)
{
	int sel = 0;
	DMPContentDirList *dir_list = NULL;

	GUI_GetProperty(WND_LISTVIEW_FILTER, "active", &sel);
	if(sel != this.filter)
	{
		dir_list = _get_bottom_dir_list();
		if(this.browse_level == 0 || NULL == dir_list)
		{
			GUI_SetProperty(WND_LISTVIEW_FILTER, "active", &this.filter);
			GUI_SetFocusWidget(WND_LISTVIEW);
			return;
		}

		_filter_dir_list();
		ithread_mutex_lock(&dir_list->mutex);
		if(!this.filter_empty && dir_list->count > 0)
		{
			this.dir_cur = dir_list;
			do{
				this.dir_cur = this.dir_cur->next;
				if( (this.filter == FILTER_ALL) ||
					(this.dir_cur->media_type == DLNA_CONTAINER) ||
					(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
					(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
					(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
				{
					break;
				}
			}while(1);
			int item = 0;
			ithread_mutex_unlock(&dir_list->mutex);
			GUI_SetProperty(WND_LISTVIEW, "select", &item);
		}
		else
		{
			this.dir_cur = NULL;
			ithread_mutex_unlock(&dir_list->mutex);
		}

		GUI_SetProperty(WND_LISTVIEW_FILTER, "active", &this.filter);
		GUI_SetProperty(WND_LISTVIEW, "update_all", NULL);
	}
	GUI_SetFocusWidget(WND_LISTVIEW);
}

SIGNAL_HANDLER int listview_dlna_dmp_filter_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_DOWN:
                this.filter++;
                if(this.is_sat2ip)
                {
                    if(this.filter >=(MAX_LIST_FILTER_ITEM - 1))
                        this.filter = 0;
                }
                else
                {
                    if(this.filter >=(MAX_LIST_FILTER_ITEM))
                        this.filter = 0;
                }
                GUI_SetProperty(WND_LISTVIEW_FILTER, "select", &this.filter);
                break;
            case STBK_UP:
                this.filter--;
                if(this.is_sat2ip)
                {
                    if(this.filter < 0)
                        this.filter = MAX_LIST_FILTER_ITEM - 2;
                }
                else
                {
                    if(this.filter < 0)
                        this.filter = MAX_LIST_FILTER_ITEM - 1;
                }
                GUI_SetProperty(WND_LISTVIEW_FILTER, "select", &this.filter);
                break;
            case STBK_OK:
            case STBK_RIGHT:
				_listview_dlna_dmp_filter_ok_keypress();
                break;
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_DLNA_DMP);
                break;
            default:
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int listview_dlna_dmp_get_data(const char* widgetname, void *usrdata)
{
    ListItemPara* item = NULL;
    item = (ListItemPara*)usrdata;
    if(NULL == item || NULL == this.server_list)
        return GXCORE_ERROR;

	if(this.browse_level == 0)
	{
		DMPServerNode *cur;
		/*col-1: img*/
		item = item->next;
		if(NULL == item)
			return GXCORE_ERROR;
		item->x_offset = 0;
		item->string = NULL;
		item->image = IMG_NET;

		/*col-2: file name*/
		item = item->next;
		if(NULL == item)
			return GXCORE_ERROR;
		item->x_offset = 0;
		item->image = NULL;

		ithread_mutex_lock(&this.server_list->mutex);
		cur = this.server_list->next;
		while(cur != this.server_list)
		{
			if(cur->count > item->sel)
				break;
			cur = cur->next;
		}
		item->string = cur->friendly_name;
    	ithread_mutex_unlock(&this.server_list->mutex);
	}
	else
	{
		DMPContentDirList *dir_list;
		DMPContentDirNode * cur;
		static char tmp_str[10];
		int count = 0;

		if(this.filter_empty == true)
		{
			item = item->next;
			if(NULL == item)
				return GXCORE_ERROR;
			item->x_offset = 0;
			item->image = IMG_UNKNOW;
			item->string = NULL;

			item = item->next;
			if(NULL == item)
				return GXCORE_ERROR;
			item->x_offset = 0;
			item->image = NULL;
			item->string = STR_ID_FILTER_EMPTY;

    		return EVENT_TRANSFER_STOP;
		}
		dir_list = _get_bottom_dir_list();
		if(NULL == dir_list && dir_list->count == 0)
			return GXCORE_ERROR;

		ithread_mutex_lock(&dir_list->mutex);
		cur = dir_list->next;
		while(cur != dir_list)
		{
			if( (this.filter == FILTER_ALL) ||
				(cur->media_type == DLNA_CONTAINER) ||
				(this.filter == FILTER_VIDEO && cur->media_type == DLNA_VIDEO) ||
			   	(this.filter == FILTER_MUSIC && cur->media_type == DLNA_AUDIO) ||
			   	(this.filter == FILTER_PICTURE && cur->media_type == DLNA_IMAGE))
			{
				count++;
				if(count > item->sel)
					break;
			}
			cur = cur->next;
		}
		if(cur == dir_list)
		{
    		ithread_mutex_unlock(&dir_list->mutex);
			return GXCORE_ERROR;
		}

		/*col-0: favorite*/
		item->x_offset = 0;
		item->string = NULL;
		item->image = NULL;// IMG_RED;

		/*col-1: img*/
		item = item->next;
		if(NULL == item)
		{
    		ithread_mutex_unlock(&dir_list->mutex);
			return GXCORE_ERROR;
		}
		item->x_offset = 0;
		item->string = NULL;
		if(cur->media_type == DLNA_CONTAINER)
			item->image = IMG_DIR;
		else if(cur->media_type == DLNA_VIDEO)
			item->image = IMG_MOVIC;
		else if(cur->media_type == DLNA_AUDIO)
			item->image = IMG_MUSIC;
		else if(cur->media_type == DLNA_IMAGE)
			item->image = IMG_PIC;
		else
			item->image = IMG_UNKNOW;

		/*col-2: file name*/
		item = item->next;
		if(NULL == item)
		{
    		ithread_mutex_unlock(&dir_list->mutex);
			return GXCORE_ERROR;
		}
		item->x_offset = 0;
		item->image = NULL;
		item->string = cur->title;

		/*col-3: time */
		item = item->next;
		if(NULL == item)
		{
			ithread_mutex_unlock(&dir_list->mutex);
			return GXCORE_ERROR;
		}
		item->x_offset = 0;
		item->image = NULL;
		if(cur->play_res)
		{
			snprintf(tmp_str, 10, "%s", cur->play_res->duration);
			char *p = strtok(tmp_str, ".");
			if(NULL == p)
			{
				if((cur->media_type == DLNA_VIDEO || cur->media_type == DLNA_AUDIO) && (cur->play_res->size == 0))
					snprintf(tmp_str, 10, "%s", "0:00:00");
				else
					snprintf(tmp_str, 10, "%s", "");
			}
			else
				snprintf(tmp_str, 10, "%s", p);
		}
		else
		{
            snprintf(tmp_str, 10, "%s", "");
		}
		item->string =tmp_str;
		ithread_mutex_unlock(&dir_list->mutex);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int listview_dlna_dmp_change(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int listview_dlna_dmp_get_count(const char* widgetname, void *usrdata)
{
	int count = 0;
	if(NULL == this.server_list)
		return 0;

	if(this.browse_level == 0)
	{
		ithread_mutex_lock(&this.server_list->mutex);
		count = this.server_list->count;
    	ithread_mutex_unlock(&this.server_list->mutex);
	}
	else
	{
		DMPContentDirList *dir_list;

		dir_list = _get_bottom_dir_list();
		if(NULL == dir_list)
			return 0;

		ithread_mutex_lock(&dir_list->mutex);
		count = dir_list->count;
		ithread_mutex_unlock(&dir_list->mutex);
	}
	return count;
}

static void _dmp_play_get_url(int source_index)
{
    char *url;

	if(NULL == this.dir_cur || NULL == this.dir_cur->play_res)
		return;
	url = this.dir_cur->play_res->url;

    if(url == NULL)
        return;

    app_log_info("play url:%s\n", url);
	if(this.dir_cur->media_type == DLNA_IMAGE)
	{
        gal_img_threshod_size(PIC_SHOW_MAX_SIZE);
		app_net_picture_play(url);
	}
    else
	{
		if(this.dir_cur->media_type == DLNA_AUDIO)
			app_net_set_music_background(IMG_MUSIC_PLAY_BACKGROUND);
		else if(this.dir_cur->media_type == DLNA_VIDEO)
			app_net_set_video_mode();

		app_net_video_start_play(url, PLAY_STATE_LOAD, 0, false);
	}
}

static void _dmp_play_restart(uint64_t cur_time)
{
    char *url;

	if(NULL == this.dir_cur || NULL == this.dir_cur->play_res)
		return;
	url = this.dir_cur->play_res->url;

    if(url == NULL)
        return;

    app_log_info("play url:%s\n", url);
    app_net_video_start_play(url, PLAY_STATE_LOAD, cur_time, false);
}


static int _dmp_play_list_total_get(void)
{
	DMPContentDirList *dir_list;
	DMPContentDirNode *cur;
	int count = 0;

	dir_list = _get_bottom_dir_list();
	if(NULL == dir_list)
		return 0;

	cur = dir_list->next;
	while(cur != dir_list)
	{
		if( (this.filter == FILTER_ALL && cur->media_type < DLNA_CONTAINER) ||
			(this.filter == FILTER_VIDEO && cur->media_type == DLNA_VIDEO) ||
			(this.filter == FILTER_MUSIC && cur->media_type == DLNA_AUDIO) ||
			(this.filter == FILTER_PICTURE && cur->media_type == DLNA_IMAGE))
		{
			count++;
		}
		cur = cur->next;
	}
	return count;
}

static char *_dmp_play_list_data_get(int sel)
{
	DMPContentDirList *dir_list;
	DMPContentDirNode *cur;
	int count = 0;

	dir_list = _get_bottom_dir_list();
	if(NULL == dir_list && dir_list->count == 0)
		return NULL;

	cur = dir_list->next;
	while(cur != dir_list)
	{
		if( (this.filter == FILTER_ALL && cur->media_type < DLNA_CONTAINER) ||
			(this.filter == FILTER_VIDEO && cur->media_type == DLNA_VIDEO) ||
			(this.filter == FILTER_MUSIC && cur->media_type == DLNA_AUDIO) ||
			(this.filter == FILTER_PICTURE && cur->media_type == DLNA_IMAGE))
		{
			count++;
			if(count > sel)
				break;
		}
		cur = cur->next;
	}
	if(cur == dir_list)
		return NULL;

	return cur->title;
}

static char *_dmp_play_list_image_get(int sel)
{
	DMPContentDirList *dir_list;
	DMPContentDirNode *cur;
	int count = 0;

	dir_list = _get_bottom_dir_list();
	if(NULL == dir_list && dir_list->count == 0)
		return NULL;

	cur = dir_list->next;
	while(cur != dir_list)
	{
		if( (this.filter == FILTER_ALL && cur->media_type < DLNA_CONTAINER) ||
			(this.filter == FILTER_VIDEO && cur->media_type == DLNA_VIDEO) ||
			(this.filter == FILTER_MUSIC && cur->media_type == DLNA_AUDIO) ||
			(this.filter == FILTER_PICTURE && cur->media_type == DLNA_IMAGE))
		{
			count++;
			if(count > sel)
				break;
		}
		cur = cur->next;
	}
	if(cur == dir_list)
		return NULL;

    if(cur->media_type == DLNA_CONTAINER)
        return IMG_DIR;
    else if(cur->media_type == DLNA_VIDEO)
        return IMG_MOVIC;
    else if(cur->media_type == DLNA_AUDIO)
        return IMG_MUSIC;
    else if(cur->media_type == DLNA_IMAGE)
        return IMG_PIC;
    else
        return IMG_UNKNOW;
}

static status_t _dmp_play_next(int force)
{
	DMPContentDirList *dir_list;

	dir_list = _get_bottom_dir_list();
	if(NULL == dir_list && dir_list->count == 0)
		return GXCORE_SUCCESS;

	do{
		this.dir_cur = this.dir_cur->next;
		if(this.dir_cur == dir_list)
			continue;
		if( (this.filter == FILTER_ALL && this.dir_cur->media_type < DLNA_CONTAINER) ||
			(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
			(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
			(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
		{
			break;
		}
	}while(1);
	_dmp_play_get_url(0);
	return GXCORE_SUCCESS;
}

static void _dmp_play_prev(void)
{
	DMPContentDirList *dir_list;

	dir_list = _get_bottom_dir_list();
	if(NULL == dir_list && dir_list->count == 0)
		return;

	do{
		this.dir_cur = this.dir_cur->prev;
		if(this.dir_cur == dir_list)
			continue;
		if( (this.filter == FILTER_ALL && this.dir_cur->media_type < DLNA_CONTAINER) ||
			(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
			(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
			(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
		{
			break;
		}
	}while(1);
	_dmp_play_get_url(0);
}

static int _dmp_play_list_ok(int item)
{
	int offset = item + 1;
	DMPContentDirList *dir_list;

	dir_list = _get_bottom_dir_list();
	if(NULL == dir_list && dir_list->count == 0)
		return -1;

	this.dir_cur = dir_list;
	do{
		this.dir_cur = this.dir_cur->next;
		if( (this.filter == FILTER_ALL && this.dir_cur->media_type < DLNA_CONTAINER) ||
			(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
			(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
			(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
		{
			offset --;
		}
	}while(offset > 0);

	_dmp_play_get_url(0);

	return 0;
}

static status_t app_dlna_dmp_play(DMPContentDirList *dir_list, char *title, char *image_url)
{
	int focus_item = 0;
		DMPContentDirNode *cur;
	NetVideoPlayOps play_ops;
	memset(&play_ops, 0, sizeof(NetVideoPlayOps));

	cur = dir_list->next;
	while(cur != this.dir_cur)
	{
		if( (this.filter == FILTER_ALL && cur->media_type < DLNA_CONTAINER) ||
			(this.filter == FILTER_VIDEO && cur->media_type == DLNA_VIDEO) ||
			(this.filter == FILTER_MUSIC && cur->media_type == DLNA_AUDIO) ||
			(this.filter == FILTER_PICTURE && cur->media_type == DLNA_IMAGE))
		{
			focus_item++;
		}
		cur = cur->next;
	}

	play_ops.get_url             = _dmp_play_get_url;
    play_ops.play_wnd_created    = false;
	play_ops.source_num = 1;
	play_ops.bar_title = "DLNA";
	play_ops.netvideo_title = title;
	play_ops.list_focus_item = focus_item;
	play_ops.play_ctrl.play_ok   = NULL;
	play_ops.play_ctrl.play_exit = NULL;
	play_ops.play_ctrl.play_prev = _dmp_play_prev;
	play_ops.play_ctrl.play_next = _dmp_play_next;
	play_ops.play_ctrl.play_restart = _dmp_play_restart;
	play_ops.play_ctrl.play_list_total_get = _dmp_play_list_total_get;
	play_ops.play_ctrl.play_list_data_get = _dmp_play_list_data_get;
	play_ops.play_ctrl.play_list_image_get = _dmp_play_list_image_get;
	play_ops.play_ctrl.play_list_ok = _dmp_play_list_ok;
    play_ops.netvideo_image = image_url;
    GUI_SetProperty(LISTVIEW_BROWSE_PATH, "rolling_stop", NULL);
	return app_net_video_play(&play_ops);
}

static void _listview_dlna_dmp_ok_keypress(void)
{
	DMPContentDirList *dir_list;
#define ID_LEN 64
	char id[ID_LEN] = "0";

	if(this.filter_empty == true)
		return;
	if(NULL == this.server_cur)
		return;
	if(NULL != this.gif_timer)
		return;

	if(this.browse_level == 0)
	{
		this.server_list->dir_list = DMP_directory_browse(this.server_cur, id, _action_event);
	}
	else
	{
		dir_list = _get_bottom_dir_list();
		if(NULL == dir_list || NULL == this.dir_cur)
			return;
		ithread_mutex_lock(&dir_list->mutex);
		if(this.dir_cur->media_type == DLNA_MEDIA_NONE)
		{
			ithread_mutex_unlock(&dir_list->mutex);
			return;
		}
		if(this.dir_cur->media_type != DLNA_CONTAINER)
		{
			if(this.dir_cur->media_type == DLNA_AUDIO)
			{
                app_dlna_dmp_play(dir_list, this.dir_cur->title, IMG_MUSIC_PLAY_BACKGROUND);
			}
			else if(this.dir_cur->media_type == DLNA_IMAGE)
			{
                app_dlna_dmp_play(dir_list, this.dir_cur->title, NULL);
			}
			else
			{
                app_dlna_dmp_play(dir_list, this.dir_cur->title, NULL);
			}
			ithread_mutex_unlock(&dir_list->mutex);
			return;
		}
		if(this.dir_cur->child_count == 0)
		{
        	_dmp_show_popdlg(STR_ID_NO_DATA_IN_SUBDIR, 1, NULL);
			ithread_mutex_unlock(&dir_list->mutex);
			return;
		}
		snprintf(id, ID_LEN, "%s", this.dir_cur->id);
		dir_list->child = DMP_directory_browse(this.server_cur, id, _action_event);
		dir_list->child->parent_node = this.dir_cur;
		ithread_mutex_unlock(&dir_list->mutex);
	}
	this.browse_level++;
    GUI_SetProperty(WND_LISTVIEW, "update_all", NULL);
	_dmp_show_gif();
	_set_browse_path();
	_set_browse_index_info(0, 0);
}

static void _listview_dlna_dmp_return_keypress(void)
{
	int total = 0;
	int item  = 0;
	DMPContentDirList *dir_list;

	this.filter_empty = false;
	if(this.browse_level == 0)
	{
        GUI_EndDialog(WND_DLNA_DMP);
	}
	else
	{
		dir_list = _get_bottom_dir_list();
		if(NULL == dir_list)
			return;

		ithread_mutex_lock(&dir_list->mutex);
		this.dir_cur = dir_list->parent_node;
		ithread_mutex_unlock(&dir_list->mutex);
        DMP_directory_free(&dir_list);
		dir_list = 	NULL;
		this.browse_level--;
		if(this.browse_level == 0)
		{
			item = this.server_cur->count - 1;
			total = this.server_list->count;
		}
		else
		{
			_filter_dir_list(); // filter类别可能被改变,重新过滤一下数据
			dir_list = _get_bottom_dir_list();
			ithread_mutex_lock(&dir_list->mutex);
			total = dir_list->count;
			item = this.dir_cur->count - 1;
			ithread_mutex_unlock(&dir_list->mutex);
		}
		_dmp_hide_gif();
		_set_browse_index_info(total, item + 1);
    	GUI_SetProperty(WND_LISTVIEW, "update_all", NULL);
		GUI_SetProperty(WND_LISTVIEW, "select", &item);
		_set_browse_path();
  	}
}

static void _listview_dlna_dmp_down_keypress(void)
{
	int total = 0;
	int item = 0;

	if(this.filter_empty == true)
		return;
	if(this.browse_level == 0)
	{
		if(NULL == this.server_list || NULL == this.server_cur)
			return;

		ithread_mutex_lock(&this.server_list->mutex);
		this.server_cur = this.server_cur->next;
		if(this.server_cur == this.server_list)
			this.server_cur = this.server_list->next;
		item = this.server_cur->count - 1;
		total = this.server_list->count;
		ithread_mutex_unlock(&this.server_list->mutex);
        GUI_SetProperty(LISTVIEW_BROWSE_PATH, "string", this.server_cur->friendly_name);
	}
	else
	{
		DMPContentDirList *dir_list = _get_bottom_dir_list();
		if(NULL == dir_list || NULL == this.dir_cur)
			return;

		ithread_mutex_lock(&dir_list->mutex);
		do{
			this.dir_cur = this.dir_cur->next;
			if(this.dir_cur == dir_list)
				continue;
			if( (this.filter == FILTER_ALL) ||
				(this.dir_cur->media_type == DLNA_CONTAINER) ||
				(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
				(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
				(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
			{
				break;
			}
		}while(dir_list->count > 0);
		item = this.dir_cur->count - 1;
		total = dir_list->count;
		ithread_mutex_unlock(&dir_list->mutex);
	}
	_set_browse_index_info(total, item + 1);
    GUI_SetProperty(WND_LISTVIEW, "select", &item);
}

static void _listview_dlna_dmp_up_keypress(void)
{
	int total = 0;
	int item = 0;

	if(this.filter_empty == true)
		return;
	if(this.browse_level == 0)
	{
		if(NULL == this.server_list || NULL == this.server_cur)
			return;

		ithread_mutex_lock(&this.server_list->mutex);
		this.server_cur = this.server_cur->prev;
		if(this.server_cur == this.server_list)
			this.server_cur = this.server_list->prev;
		item = this.server_cur->count - 1;
		total = this.server_list->count;
		ithread_mutex_unlock(&this.server_list->mutex);
        GUI_SetProperty(LISTVIEW_BROWSE_PATH, "string", this.server_cur->friendly_name);
	}
	else
	{
		DMPContentDirList *dir_list = _get_bottom_dir_list();
		if(NULL == dir_list || NULL == this.dir_cur)
			return;

		ithread_mutex_lock(&dir_list->mutex);
		do{
			this.dir_cur = this.dir_cur->prev;
			if(this.dir_cur == dir_list)
				continue;
			if( (this.filter == FILTER_ALL) ||
				(this.dir_cur->media_type == DLNA_CONTAINER) ||
				(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
				(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
				(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
			{
				break;
			}
		}while(dir_list->count > 0);
		item = this.dir_cur->count - 1;
		total = dir_list->count;
		ithread_mutex_unlock(&dir_list->mutex);
	}
	_set_browse_index_info(total, item + 1);
    GUI_SetProperty(WND_LISTVIEW, "select", &item);
}

static void _listview_dlna_dmp_page_up_keypress(void)
{
	int total = 0;
	int item = 0;
	int i = 0;
	int offset = 0;

	GUI_GetProperty(WND_LISTVIEW, "visible_row", &offset);
	if(this.filter_empty == true)
	{
    	GUI_SetFocusWidget(WND_LISTVIEW_FILTER);
	}
	if(this.browse_level == 0)
	{
		if(NULL == this.server_list || NULL == this.server_cur)
			return;

		ithread_mutex_lock(&this.server_list->mutex);
		for(i = 0; i < offset; i++)
		{
			this.server_cur = this.server_cur->prev;
			if(this.server_cur == this.server_list ||this.server_cur->count==1)
				break;
		}
		if(this.server_cur == this.server_list)
		{
			for(i = 0; i < offset; i++)
            {
                this.server_cur = this.server_cur->prev;
                if(this.server_list->count==this.server_cur->count)
                    break;
            }
		}
		item = this.server_cur->count - 1;
		total = this.server_list->count;
		ithread_mutex_unlock(&this.server_list->mutex);
        GUI_SetProperty(LISTVIEW_BROWSE_PATH, "string", this.server_cur->friendly_name);
	}
	else
	{
		DMPContentDirList *dir_list = _get_bottom_dir_list();
		if(NULL == dir_list || NULL == this.dir_cur)
			return;

		ithread_mutex_lock(&dir_list->mutex);

		do{
			this.dir_cur = this.dir_cur->prev;
			if( (this.filter == FILTER_ALL) ||
				(this.dir_cur->media_type == DLNA_CONTAINER) ||
				(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
				(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
				(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
			{
				offset --;
			}
			if(this.dir_cur == dir_list || this.dir_cur->count ==1)
				break;
		}while(offset > 0);

		if(this.dir_cur == dir_list)
		{
			do{
				this.dir_cur = this.dir_cur->prev;
				if( (this.filter == FILTER_ALL) ||
					(this.dir_cur->media_type == DLNA_CONTAINER) ||
					(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
					(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
					(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
				{
					break;
				}
			}while(1);
		}
		item = this.dir_cur->count - 1;
		total = dir_list->count;
		ithread_mutex_unlock(&dir_list->mutex);
	}
	_set_browse_index_info(total, item + 1);
    GUI_SetProperty(WND_LISTVIEW, "select", &item);
}

static void _listview_dlna_dmp_page_down_keypress(void)
{
	int total = 0;
	int item = 0;
	int i = 0;
	int offset = 0;

	if(this.filter_empty == true)
		return;
    GUI_GetProperty(WND_LISTVIEW, "visible_row", &offset);
	if(this.browse_level == 0)
	{
		if(NULL == this.server_list || NULL == this.server_cur)
			return;

		ithread_mutex_lock(&this.server_list->mutex);
		for(i = 0; i < offset; i++)
		{
			this.server_cur = this.server_cur->next;
			if(this.server_cur == this.server_list || this.server_cur->count==this.server_list->count)
				break;
		}
		if(this.server_cur == this.server_list)
			this.server_cur = this.server_list->next;
		item = this.server_cur->count - 1;
		total = this.server_list->count;
		ithread_mutex_unlock(&this.server_list->mutex);
        GUI_SetProperty(LISTVIEW_BROWSE_PATH, "string", this.server_cur->friendly_name);
	}
	else
	{
		DMPContentDirList *dir_list = _get_bottom_dir_list();
		if(NULL == dir_list || NULL == this.dir_cur)
			return;

		ithread_mutex_lock(&dir_list->mutex);
		do{
			this.dir_cur = this.dir_cur->next;
			if( (this.filter == FILTER_ALL) ||
				(this.dir_cur->media_type == DLNA_CONTAINER) ||
				(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
				(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
				(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
			{
				offset --;
			}
			if(this.dir_cur == dir_list || this.dir_cur->count == dir_list->count)
				break;
		}while(offset > 0);

		if(this.dir_cur == dir_list)
		{
			do{
				this.dir_cur = this.dir_cur->next;
				if( (this.filter == FILTER_ALL) ||
					(this.dir_cur->media_type == DLNA_CONTAINER) ||
					(this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
					(this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
					(this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
				{
					break;
				}
			}while(1);
		}
		item = this.dir_cur->count - 1;
		total = dir_list->count;
		ithread_mutex_unlock(&dir_list->mutex);
	}
	_set_browse_index_info(total, item + 1);
    GUI_SetProperty(WND_LISTVIEW, "select", &item);
}

SIGNAL_HANDLER int listview_dlna_dmp_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_OK:
				_listview_dlna_dmp_ok_keypress();
                break;
            case STBK_UP:
				_listview_dlna_dmp_up_keypress();
                break;
            case STBK_DOWN:
				_listview_dlna_dmp_down_keypress();
                break;
            case STBK_LEFT:
    			GUI_SetFocusWidget(WND_LISTVIEW_FILTER);
                break;
			case STBK_PAGE_DOWN:
				_listview_dlna_dmp_page_down_keypress();
                break;
			case STBK_PAGE_UP:
				_listview_dlna_dmp_page_up_keypress();
                break;
            case STBK_EXIT:
				_listview_dlna_dmp_return_keypress();
				break;
            case STBK_MENU:
                GUI_EndDialog(WND_DLNA_DMP);
                break;
            default:
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

static int _dmp_popdlg_cb_return_prev(PopDlgRet ret)
{
	_listview_dlna_dmp_return_keypress();
	return 0;
}

int app_dlna_dmp_ui_msg_proc(GxMessage *msg)
{
    AppMsg_DlnaUIControl *ui_msg;

    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    ui_msg = GxBus_GetMsgPropertyPtr(msg, AppMsg_DlnaUIControl);
    switch(ui_msg->type)
    {
        case UI_DMP_ADD_SERVER:
        {
            if(this.browse_level == 0 && NULL != this.server_list)
            {
                if(NULL == this.server_cur)
                {
                    this.server_cur = this.server_list->next;
                }
                int item = this.server_cur->count - 1;
                int total = this.server_list->count;

                _dmp_hide_gif();
                _set_browse_index_info(total, item + 1);
                GUI_SetProperty(WND_LISTVIEW, "select", &item);
                GUI_SetProperty(LISTVIEW_BROWSE_PATH, "string", this.server_cur->friendly_name);
                GUI_SetProperty(WND_LISTVIEW, "update_all", NULL);
            }
            break;
        }
        case UI_DMP_UPDATE_DIR:
		{
			DMPContentDirList *dir_list;

    		_dmp_hide_gif();
			dir_list = _get_bottom_dir_list();
			if(NULL == dir_list)
				return 0;
			_filter_dir_list();
			ithread_mutex_lock(&dir_list->mutex);
			if(!this.filter_empty && dir_list->count > 0)
			{
				this.dir_cur = dir_list;
                do{
                    this.dir_cur = this.dir_cur->next;
                    if( (this.filter == FILTER_ALL) ||
                        (this.dir_cur->media_type == DLNA_CONTAINER) ||
                        (this.filter == FILTER_VIDEO && this.dir_cur->media_type == DLNA_VIDEO) ||
                        (this.filter == FILTER_MUSIC && this.dir_cur->media_type == DLNA_AUDIO) ||
                        (this.filter == FILTER_PICTURE && this.dir_cur->media_type == DLNA_IMAGE))
                    {
                        break;
                    }
                }while(1);
				int item = this.dir_cur->count - 1;
				int total = dir_list->count;
				ithread_mutex_unlock(&dir_list->mutex);
				_set_browse_index_info(total, item + 1);
				GUI_SetProperty(WND_LISTVIEW, "select", &item);
			}
			else
			{
				this.dir_cur = NULL;
				ithread_mutex_unlock(&dir_list->mutex);
			}
    		GUI_SetProperty(WND_LISTVIEW, "update_all", NULL);
            break;
		}
        case UI_DMP_ACTION_ACK_ERROR:
        {
    		_dmp_hide_gif();
        	_dmp_show_popdlg(STR_ID_SERVER_ERROR, 0, _dmp_popdlg_cb_return_prev);
            break;
        }
		case UI_DMP_GOT_FOCUS:
		{
			DMPContentDirList *dir_list = _get_bottom_dir_list();
			if(NULL == dir_list || NULL == this.dir_cur)
            	break;

			if(NULL != this.dir_cur)
			{
				int item = this.dir_cur->count - 1;
				_set_browse_index_info(dir_list->count, item + 1);
				GUI_SetProperty(WND_LISTVIEW, "select", &item);
			}
            break;
		}
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}


#endif
