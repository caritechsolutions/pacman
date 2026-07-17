#include "app.h"
#include "app_module.h"
#if NETAPPS_SUPPORT
#include "app_netapps_init.h"
#include "app_netapps_common.h"
#include "gx_apps.h"
#include "gui_core/gdi_core.h"
#include <devapi/gxotp_api.h>
#include "app_common_select.h"
#include "app_netvideo_play.h"
#include "qrencode.h"

typedef enum
{
	NETSET_ASPECT_RATIO_AUTO = 0,
	NETSET_ASPECT_RATIO_FULL_SCREEN,
	NETSET_ASPECT_RATIO_ORIGINAL_SIZE,
	NETSET_ASPECT_RATIO_4_3,
	NETSET_ASPECT_RATIO_16_9,
	NETSET_ASPECT_RATIO_MAX
}netapps_set_aspect_ratio;

typedef struct _NetappsSubtCtrl
{
	GuiSurface     *surface;
	char           *layer_name;
	GXGDI_Rect      rect;
	uint32_t	fore_color;
	uint32_t	back_color;
	bool            init;
	event_list*     view_mode_timer;
	netapps_set_aspect_ratio record_aspect;
}NetappsSubtCtrl;

typedef struct netapps_disk_s
{
	char *name;
	char *path;
}netapps_disk_t;

typedef struct netapps_disklist_s
{
	int disk_num;
	netapps_disk_t *disks;
}netapps_disklist_t;

typedef struct netapps_gdidata_s
{
	netapps_gdi_type type;
	GuiSurface *surface;
	char *layer;
	GXGDI_Rect rect;
	unsigned int fore_color;
	unsigned int back_color;
	char *content;
	int created;
}netapps_gdidata_t;

static NetappsSubtCtrl s_NetappsSubtCtrl = {
	.surface = NULL,
	.layer_name = NULL,
	.back_color = 0x0c0c0c,
	.fore_color = 0xf4f4f4,
	.init = false,
	.rect = {WND_NET_ASPECT_POS_X,WND_NET_ASPECT_POS_Y,150,50},
	.view_mode_timer = NULL,
	.record_aspect = NETSET_ASPECT_RATIO_AUTO,
};

static char* netapps_aspect_radio[NETSET_ASPECT_RATIO_MAX] =
{
	"Auto", //NETSET_ASPECT_RATIO_AUTO
	"Full screen", //NETSET_ASPECT_RATIO_FULL_SCREEN
	"Original size", //NETSET_ASPECT_RATIO_ORIGINAL_SIZE
	"4:3", //NETSET_ASPECT_RATIO_4_3
	"16:9", //NETSET_ASPECT_RATIO_16_9
};

static int netapps_compose_url_flag = 0;
static int s_netapps_bandwidth_num = 0;
static char **s_netapps_bandwidth_title = NULL;;
#define PLAYER_BANDWIDTH_LEN (32)

#define NETAPPS_APP_ID	"netapps>app_id"
#define NETAPPS_DEFAULT_APP_ID "none"

#define  GX_NETAPP_EVENT_MAX (16)
static int netapp_event_class_num = 0;
static app_netapp_event_class* g_netapp_event_classes[GX_NETAPP_EVENT_MAX];

extern void app_av_setting_init(void);
extern void app_netapps_event_init_ex(void);
extern int hd_zoom_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);

void app_netapps_app_init(void)
{
	g_AppPlayOps.program_stop();
	g_AppPvrOps.tms_delete(&g_AppPvrOps);
}

void app_netapps_app_destroy(void)
{

}

int app_netapps_msg_proc(GxMessage *msg)
{
	if(msg == NULL)
		return EVENT_TRANSFER_STOP;

	switch(msg->msg_id)
	{
		case GXMSG_NETWORK_STATE_REPORT:
			{
				GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
				if(dev_state->dev_state == IF_STATE_CONNECTED)
				{
					gxapps_service_start();
				}
				break;
			}
		default:
			break;
	}
	return EVENT_TRANSFER_STOP;
}

void app_netapps_player_init(void)
{
#ifdef COMPOSE_URL_SUPPORT
	netapps_compose_url_flag = 1;
	GxPlayer_ModuleRegisterStreamCompose_V2();
#endif

#ifdef MPEG_DASH_SUPPORT
	GxPlayer_ModuleRegisterStreamMPEGDASH_V2();
#endif
#ifdef ASSOCIATIONLIBRE_URL_SUPPORT
	netapps_compose_url_flag = 2;
    GxPlayer_ModuleRegisterStreamAssociationLibre_V2();
#endif
    s_NetappsSubtCtrl.rect.x = APP_WND_NET_ASPECT_POS_X ;
    s_NetappsSubtCtrl.rect.y = APP_WND_NET_ASPECT_POS_Y ;
}

int app_netapps_check_compose_url(void)
{
	return netapps_compose_url_flag;
}

char *app_netapps_get_system_language(void)
{
	int osd_lang = 0;

	GxBus_ConfigGetInt(OSD_LANG_KEY, &osd_lang, OSD_LANG);
	if(osd_lang>=LANG_MAX)
	{
		osd_lang = 0;
	}
	return g_LangName[osd_lang];
}

static int netapps_aspect_osd_layer_init(void)
{
	NetappsSubtCtrl *ctrl = &s_NetappsSubtCtrl;

	if(true == ctrl->init)
	{
		app_log_error("please deinit first\n");
		return -1;
	}

	ctrl->surface = hd_get_surface((GAL_Rect *)&ctrl->rect, APP_UI_THEME_BPP, NULL, false);
	if(NULL == ctrl->surface)
	{
		app_log_error("Create surface failed.\n");
		goto err;
	}

	ctrl->layer_name = GxGDI_LayerRegister(ctrl->surface, ctrl->rect.x, ctrl->rect.y);
	if(NULL == ctrl->layer_name)
	{
		app_log_error("Register surface failed.\n");
		goto err;
	}

	ctrl->init = true;
	return 0;

err:
	if (ctrl->layer_name)
	{
		GxGDI_LayerUnregister(ctrl->layer_name);
		ctrl->layer_name = NULL;
	}
	if(ctrl->surface)
	{
		hd_free_surface(ctrl->surface);
		ctrl->surface = NULL;
	}

	return -1;
}

static int netapps_aspect_osd_layer_draw_string(void* string)
{
	NetappsSubtCtrl *ctrl = &s_NetappsSubtCtrl;
	GXGDI_Rect draw_rect = {0};

	if(NULL == ctrl->surface)
	{
		app_log_error("---error-----!Fill NULL surface.\n");
		return -1;
	}

	draw_rect.x = 0;
	draw_rect.y = 0;
	draw_rect.w = ctrl->rect.w;
	draw_rect.h = ctrl->rect.h;
	GXGDI_Lock();
	GXGDI_Begin();
	GXGDI_FillRect(ctrl->surface, &draw_rect, ctrl->back_color);
	GXGDI_DrawString(ctrl->surface, string, &draw_rect, ALIGNMENT_HCENTRE | ALIGNMENT_VCENTRE, ctrl->fore_color, GDI_STRING_PARAGRAPH);
	GXGDI_End();
	GxGDI_LayerUpdate(ctrl->layer_name);
	GXGDI_Unlock();

	return 0;
}

static int netapps_aspect_osd_layer_quit(void)
{
	NetappsSubtCtrl *ctrl = &s_NetappsSubtCtrl;

	if(false == ctrl->init)
	{
		app_log_debug("Have been quited\n");
		return -1;
	}
	GxGDI_LayerUnregister(ctrl->layer_name);
	ctrl->layer_name = NULL;
	hd_free_surface(ctrl->surface);
	ctrl->surface = NULL;
	ctrl->init = false;

	return 0;
}

static int app_netapps_aspect_hotkey_timeout(void *usrdata)
{
	NetappsSubtCtrl *ctrl = &s_NetappsSubtCtrl;

	APP_TIMER_REMOVE(ctrl->view_mode_timer);
	netapps_aspect_osd_layer_quit();

	return 0;
}

static int app_netapps_aspect_set(int value)
{
	GxMessage *msg_aspect;
	GxMsgProperty_PlayerVideoAspect *config_aspect;

	msg_aspect = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_ASPECT);
	APP_CHECK_P(msg_aspect, GXCORE_ERROR);
	config_aspect = GxBus_GetMsgPropertyPtr(msg_aspect, GxMsgProperty_PlayerVideoAspect);
	APP_CHECK_P(config_aspect, GXCORE_ERROR);

	switch (value)
	{
		case NETSET_ASPECT_RATIO_AUTO:
			*config_aspect=ASPECT_RAW_RATIO;
			break;
		case NETSET_ASPECT_RATIO_FULL_SCREEN:
			*config_aspect=ASPECT_NORMAL;
			break;
		case NETSET_ASPECT_RATIO_ORIGINAL_SIZE:
			*config_aspect=ASPECT_RAW_SIZE;
			break;
		case NETSET_ASPECT_RATIO_4_3:
			*config_aspect=ASPECT_4X3_CUT;
			break;
		case NETSET_ASPECT_RATIO_16_9:
			*config_aspect=ASPECT_16X9_CUT;
			break;
		default:
			break;
	}
	GxBus_MessageSend(msg_aspect);

	return 0;
}

void app_netapps_aspect_init(void)
{
	NetappsSubtCtrl *ctrl = &s_NetappsSubtCtrl;

	ctrl->record_aspect = NETSET_ASPECT_RATIO_AUTO;
	app_netapps_aspect_set(ctrl->record_aspect);
}

char* app_get_netapps_aspect_string(void)
{
    NetappsSubtCtrl *ctrl = &s_NetappsSubtCtrl;
    return netapps_aspect_radio[ctrl->record_aspect];
}

void app_netapps_aspect_hotkey(void)
{
	NetappsSubtCtrl *ctrl = &s_NetappsSubtCtrl;

	ctrl->record_aspect++;
	if(ctrl->record_aspect >= NETSET_ASPECT_RATIO_MAX)
	{
		ctrl->record_aspect = NETSET_ASPECT_RATIO_AUTO;
	}

	app_netapps_aspect_set(ctrl->record_aspect);
	netapps_aspect_osd_layer_quit();
	netapps_aspect_osd_layer_init();
	netapps_aspect_osd_layer_draw_string(netapps_aspect_radio[ctrl->record_aspect]);
	if(ctrl->view_mode_timer)
	{
		reset_timer(ctrl->view_mode_timer);
	}
	else
	{
		ctrl->view_mode_timer = create_timer(app_netapps_aspect_hotkey_timeout, 3000, 0, TIMER_REPEAT);
	}
}

void app_netapps_aspect_exit(void)
{
	app_netapps_aspect_hotkey_timeout(NULL);
	app_av_setting_init();
}

static void _netapps_bandwidth_destroy(void)
{
    int i = 0;

    if(s_netapps_bandwidth_title)
    {
        for(i=0; i<s_netapps_bandwidth_num; i++)
        {
            APP_FREE(s_netapps_bandwidth_title[i]);
        }
        GxCore_Free(s_netapps_bandwidth_title);
        s_netapps_bandwidth_title = NULL;
    }
    s_netapps_bandwidth_num = 0;
}

int app_netapps_bandwidth_get_num(void)
{
    PlayerProgInfo *info = (PlayerProgInfo *)app_get_netvideo_playinfo();
    if(info == NULL)
        return -1;

    return info->bandwidth_num;
}

static void _netapps_bandwidth_init(void)
{
    int i = 0;

    PlayerProgInfo *info = (PlayerProgInfo *)app_get_netvideo_playinfo();
    if(info == NULL)
        return;
    if(info->bandwidth_num>0)
    {
        _netapps_bandwidth_destroy();
        s_netapps_bandwidth_title = (char**)GxCore_Calloc(info->bandwidth_num, sizeof(char*));
        for(i=0; i<info->bandwidth_num; i++)
        {
            s_netapps_bandwidth_title[i] = GxCore_Mallocz(PLAYER_BANDWIDTH_LEN);
            snprintf(s_netapps_bandwidth_title[i], PLAYER_BANDWIDTH_LEN-1, "%d kbps", info->bandwidth[i].bandwidth/1000);
        }
        s_netapps_bandwidth_num = info->bandwidth_num;
    }
    GUI_SetProperty(g_CommonSelect.widget.list, "select", &info->cur_bandwidth_id);
    GUI_SetProperty(g_CommonSelect.widget.list, "active", &info->cur_bandwidth_id);
    GUI_SetFocusWidget(g_CommonSelect.widget.list);
}

static void _netapps_bandwidth_switch(void)
{
	GxMsgProperty_PlayerBandwidthSwitch netapps_bandswitch = {0};
	int sel = -1;
	int active = -1;
	if(s_netapps_bandwidth_title&&(s_netapps_bandwidth_num>0))
	{
		GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
		GUI_GetProperty(g_CommonSelect.widget.list, "active", &active);
		if((sel>=0)&&(sel<s_netapps_bandwidth_num)&&(active!=sel))
		{
			GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
			netapps_bandswitch.player = PLAYER_FOR_IPTV;
			netapps_bandswitch.index = sel;
			app_send_msg_exec(GXMSG_PLAYER_BANDWIDTH_SWITCH, (void *)(&netapps_bandswitch));
			GUI_EndDialog(g_CommonSelect.widget.wnd);
		}
	}
}

int app_netapps_bandwidth_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;

    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                    break;
                default:
                    break;
            }
    }

    return EVENT_TRANSFER_STOP;
}

int app_netapps_bandwidth_create(GuiWidget *widget, void *usrdata)
{
    _netapps_bandwidth_init();
    return EVENT_TRANSFER_STOP;
}

int app_netapps_bandwidth_destroy(GuiWidget *widget, void *usrdata)
{
    _netapps_bandwidth_destroy();
    return EVENT_TRANSFER_STOP;
}

int app_netapps_bandwidth_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t sel=0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
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
					ret = EVENT_TRANSFER_KEEPON;
					break;

				case STBK_LEFT:
				case STBK_RIGHT:
					break;

				case STBK_PAGE_UP:
					sel = 0;
					GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
					if(0 == sel)
					{
						if(s_netapps_bandwidth_num > 1)
						{
						    sel = s_netapps_bandwidth_num - 1;
						    GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
						}
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;

				case STBK_PAGE_DOWN:
					sel = 0;
					GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
					if(s_netapps_bandwidth_num > 1)
					{
					    if(sel == (s_netapps_bandwidth_num - 1))
					    {
					    	sel = 0;
					    	GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
					    	ret = EVENT_TRANSFER_STOP;
					    }
					    else
					    {
					    	ret = EVENT_TRANSFER_KEEPON;
					    }
					}
					break;

				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(g_CommonSelect.widget.wnd);
					GUI_SetInterface("flush",NULL);
					break;
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;
				case STBK_OK:
					_netapps_bandwidth_switch();
					break;
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

int app_netapps_bandwidth_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)
	ListItemPara* item = NULL;
	static char buffer[CHANNEL_NUM_LEN];

	item = (ListItemPara*)usrdata;
	if((NULL == item) || (0 > item->sel))
	{
		return EVENT_TRANSFER_STOP;
	}

	//col-0: num
	item->x_offset = 2;
	item->image = NULL;
	memset(buffer, 0, CHANNEL_NUM_LEN);
	if(s_netapps_bandwidth_num < 10000)
		sprintf(buffer, "%04d", (item->sel+1));
	else
		sprintf(buffer, "%05d", (item->sel+1));
	item->string = buffer;

	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = s_netapps_bandwidth_title[item->sel];
	return EVENT_TRANSFER_STOP;
}

int app_netapps_bandwidth_get_total(GuiWidget *widget, void *usrdata)
{
    return s_netapps_bandwidth_num;
}

int app_netapps_bandwidth_create_dialog(void)
{
	CommonSelectParas paras = {0};

	paras.choice = COMMON_SELECT_TITLE_ONLY;
	paras.title = STR_ID_IPTV_BANDWIDTH;
	paras.tip_strs.str_red = NULL;
	paras.tip_strs.str_green = NULL;
	paras.tip_strs.str_yellow = NULL;
	paras.tip_strs.str_blue = NULL;
	paras.signal.create = app_netapps_bandwidth_create;
	paras.signal.destroy = app_netapps_bandwidth_destroy;
	paras.signal.keypress = app_netapps_bandwidth_keypress;
	paras.signal.group_change = NULL;
	paras.signal.list_get_total = app_netapps_bandwidth_get_total;
	paras.signal.list_get_data = app_netapps_bandwidth_get_data;
	paras.signal.list_keypress = app_netapps_bandwidth_list_keypress;
	return app_common_select_create_dialog(&paras);
}

unsigned int app_netapps_get_storage_size(void)
{
	return app_get_flash_size();
}

unsigned int app_netapps_get_memory_size(void)
{
	return app_get_ddr_size();
}

int app_netapps_get_memory_info(netapps_mem_info *info)
{
	GxCoreMemInfo mem_info;

	if(info)
	{
		memset(info, 0, sizeof(netapps_mem_info));
		memset(&mem_info, 0, sizeof(GxCoreMemInfo));
		if(GxCore_GetMemInfo(&mem_info) == 0)
		{
			info->total = mem_info.total;
			info->free = mem_info.free;
			return 0;
		}
	}

	return -1;
}

void app_netapps_get_ui_size(int *width, int *height)
{
     *width  = APP_THEME_XRES;
     *height = APP_THEME_YRES;
}

int app_netapps_get_time_offsets(void)
{
	int half = 0;
	int zone = 0;
	int summer = 0;

	app_time_zone_config_get(&zone,&half,&summer);
	return app_time_zone_sec_get(zone, half, summer);
}

unsigned int app_netapps_disk_malloc(void)
{
	netapps_disklist_t *disklist = NULL;
	HotplugPartitionList *partition_list = NULL;
	int i = 0;

	partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	if(partition_list&&(partition_list->partition_num>0))
	{
		disklist = (netapps_disklist_t *)GxCore_Calloc(1, sizeof(netapps_disklist_t));
		if(disklist)
		{
			disklist->disks = (netapps_disk_t *)GxCore_Calloc(partition_list->partition_num, sizeof(netapps_disk_t));
			if(disklist->disks)
			{
				for(i=0; i<partition_list->partition_num; i++)
				{
					disklist->disks[disklist->disk_num].name = GxCore_Strdup(partition_list->partition[i].partition_name);
					disklist->disks[disklist->disk_num].path = GxCore_Strdup(partition_list->partition[i].partition_entry);
					disklist->disk_num++;
				}
				return (unsigned int)disklist;
			}
			else
			{
				GxCore_Free(disklist);
			}
		}
	}

	return 0;
}

void app_netapps_disk_free(unsigned int handle)
{
	netapps_disklist_t *disklist = NULL;
	int i = 0;

	disklist = (netapps_disklist_t *)handle;
	if(disklist)
	{
		if(disklist->disks)
		{
			for(i=0; i<disklist->disk_num; i++)
			{
				APP_FREE(disklist->disks[i].name);
				APP_FREE(disklist->disks[i].path);
			}
			GxCore_Free(disklist->disks);
			disklist->disks = NULL;
		}
		GxCore_Free(disklist);
		disklist = NULL;
	}
}

int app_netapps_disk_get_num(unsigned int handle)
{
	netapps_disklist_t *disklist = NULL;

	disklist = (netapps_disklist_t *)handle;
	if(disklist)
	{
		return disklist->disk_num;
	}

	return 0;
}

char *app_netapps_disk_get_name(unsigned int handle, int index)
{
	netapps_disklist_t *disklist = NULL;

	disklist = (netapps_disklist_t *)handle;
	if(disklist&&(disklist->disks)&&(index>=0)&&(index<disklist->disk_num))
	{
		return disklist->disks[index].name;
	}

	return NULL;
}

char *app_netapps_disk_get_path(unsigned int handle, int index)
{
	netapps_disklist_t *disklist = NULL;

	disklist = (netapps_disklist_t *)handle;
	if(disklist&&(disklist->disks)&&(index>=0)&&(index<disklist->disk_num))
	{
		return disklist->disks[index].path;
	}

	return NULL;
}

char *app_netapps_config_get_string(const char *key, char *buf, size_t buf_size, const char *dvalue)
{
	return GxBus_ConfigGet(key, buf, buf_size, dvalue);
}

int *app_netapps_config_get_int(const char *key, int *buf, int dvalue)
{
	return GxBus_ConfigGetInt(key, buf, dvalue);
}

double *app_netapps_config_get_double(const char *key, double *buf, double dvalue)
{
	return GxBus_ConfigGetDouble(key, buf, dvalue);
}

int app_netapps_config_set_string(const char *key, const char *value)
{
	return GxBus_ConfigSet(key, value);
}

int app_netapps_config_set_int(const char *key, int value)
{
	return GxBus_ConfigSetInt(key, value);
}

int app_netapps_config_set_double(const char *key, double value)
{
	return GxBus_ConfigSetDouble(key, value);
}

unsigned int app_netapps_hw_malloc(unsigned int size)
{
	GxHwMallocObj *hw_data = NULL;

	if(size>0)
	{
		hw_data = (GxHwMallocObj *)malloc(sizeof(GxHwMallocObj));
		if(hw_data)
		{
			memset(hw_data, 0, sizeof(GxHwMallocObj));
			hw_data->size = size;
			GxCore_HwMalloc(hw_data, MALLOC_NO_CACHE);
			if(hw_data->usr_p)
			{
				return (unsigned int)hw_data;
			}
			else
			{
				free(hw_data);
				hw_data = NULL;
			}
		}
	}

	return 0;
}

void *app_netapps_hw_get(unsigned int handle)
{
	GxHwMallocObj *hw_data = NULL;

	hw_data = (GxHwMallocObj *)handle;
	if(hw_data)
	{
		return hw_data->usr_p;
	}

	return NULL;
}

void app_netapps_hw_free(unsigned int handle)
{
	GxHwMallocObj *hw_data = NULL;

	hw_data = (GxHwMallocObj *)handle;
	if(hw_data)
	{
		GxCore_HwFree(hw_data);
		free(hw_data);
	}
}

unsigned int app_netapps_gdi_init(netapps_gdi_type type, int pos_x, int pos_y, int width, int height)
{
	netapps_gdidata_t *gdi_data = NULL;

	if((type>=NETAPPS_GDI_TEXT)&&(type<NETAPPS_GDI_MAX)
		&&(pos_x>=0)&&(pos_y>=0)&&(width>0)&&(height>0))
	{
		gdi_data = (netapps_gdidata_t *)GxCore_Calloc(1, sizeof(netapps_gdidata_t));
		if(gdi_data)
		{
			gdi_data->type = type;
			gdi_data->rect.x = pos_x;
			gdi_data->rect.y = pos_y;
			gdi_data->rect.w = width;
			gdi_data->rect.h = height;
			gdi_data->surface = hd_get_surface((GAL_Rect *)&gdi_data->rect, APP_UI_THEME_BPP, NULL, false);
			gdi_data->layer = GxGDI_LayerRegister(gdi_data->surface, gdi_data->rect.x, gdi_data->rect.y);
			if(gdi_data->surface&&gdi_data->layer)
			{
				GxGDI_LayerEnable(gdi_data->layer, false);
				return (unsigned int)gdi_data;
			}
			else
			{
				if(gdi_data->layer)
				{
					GxGDI_LayerUnregister(gdi_data->layer);
				}
				if(gdi_data->surface)
				{
					hd_free_surface(gdi_data->surface);
				}
				GxCore_Free(gdi_data);
				gdi_data = NULL;
			}
		}
	}

	return 0;
}

void app_netapps_gdi_set_background(unsigned int handle, unsigned int color)
{
	netapps_gdidata_t *gdi_data = NULL;

	gdi_data = (netapps_gdidata_t *)handle;
	if(gdi_data)
	{
		gdi_data->back_color = color;
	}
}

void app_netapps_gdi_set_foreground(unsigned int handle, unsigned int color)
{
	netapps_gdidata_t *gdi_data = NULL;

	gdi_data = (netapps_gdidata_t *)handle;
	if(gdi_data)
	{
		gdi_data->fore_color = color;
	}
}

void app_netapps_gdi_set_content(unsigned int handle, char *content)
{
	netapps_gdidata_t *gdi_data = NULL;

	gdi_data = (netapps_gdidata_t *)handle;
	if(gdi_data)
	{
		if(gdi_data->content)
		{
			GxCore_Free(gdi_data->content);
			gdi_data->content = NULL;
		}
		if(content)
		{
			gdi_data->content = GxCore_Strdup(content);
		}
	}
}

#if QR_POP_SUPPORT
static void fillRow(unsigned char *row, int num, const unsigned char color[],int bpp)
{
	int i;

	for(i = 0; i < num; i++)
	{
		memcpy(row, color, bpp);
		row += bpp;
	}
}

static int app_netapps_get_qr_data(char *content, GuiSurface *dst, GAL_Rect *dst_rect)
{
	unsigned char fg_color[4] = {0, 0, 0, 0};
	unsigned char bg_color[4] = {255, 255, 255, 255};
	GAL_Rect rect = {0, 0, 0, 0};
	int margin = 4;
	int size = 2;
	QRcode *qrcode;
	unsigned char* png_ptr =NULL;
	unsigned char *row, *p;
	int x, y, xx, yy;
	int realwidth;
	int s_bpp = APP_UI_THEME_BPP/8;
	GuiSurface *surface = NULL;
	int ret = 0;

	/* must be version 0, QR_ECLEVEL_H and case sensitive */
	qrcode = QRcode_encodeString(content, 0, QR_ECLEVEL_H, QR_MODE_8, 1);
	realwidth = (qrcode->width + margin * s_bpp) * size;

	row = (unsigned char *)malloc(realwidth * s_bpp);
	rect.w = realwidth;
	rect.h = realwidth;
	surface = hd_get_surface(&rect, APP_UI_THEME_BPP, NULL, 0);
	if(!surface)
	{
		goto end;
	}
	png_ptr = surface->data;

	GXGDI_Begin();
	/* top margin */
	fillRow(row, realwidth, bg_color,s_bpp);
	for(y = 0; y < margin * size; y++)
	{
		memcpy(png_ptr, row,realwidth*s_bpp);
		png_ptr =png_ptr+(realwidth*s_bpp);
	}

	/* data */
	p = qrcode->data;
	for(y = 0; y < qrcode->width; y++)
	{
		fillRow(row, realwidth, bg_color,s_bpp);
		for(x = 0; x < qrcode->width; x++)
		{
			for(xx = 0; xx < size; xx++)
			{
				if(*p & 1)
				{
					memcpy(&row[((margin + x) * size + xx) * s_bpp], fg_color, s_bpp);
				}
			}
			p++;
		}

		for(yy = 0; yy < size; yy++)
		{
			memcpy(png_ptr, row,realwidth*s_bpp);
			png_ptr = png_ptr+(realwidth*s_bpp);
		}
	}

	/* bottom margin */
	fillRow(row, realwidth, bg_color,s_bpp);
	for(y = 0; y < margin * size; y++)
	{
		memcpy(png_ptr, row,realwidth*s_bpp);
		png_ptr =png_ptr+(realwidth*s_bpp);
	}

	hd_zoom_surface(surface, &rect, dst, dst_rect);
	GXGDI_End();
	ret = 1;

end:
	free(row);
	QRcode_free(qrcode);
	QRcode_clearCache();
	if(surface)
	{
		hd_free_surface(surface);
	}

	return ret;
}
#else
static int app_netapps_get_qr_data(char *content, GuiSurface *dst, GAL_Rect *dst_rect)
{
	return 0;
}
#endif

int app_netapps_gdi_show(unsigned int handle)
{
	netapps_gdidata_t *gdi_data = NULL;
	GXGDI_Rect draw_rect = {0};
	int res = 0;
	int ret = 0;

	gdi_data = (netapps_gdidata_t *)handle;
	if(gdi_data&&gdi_data->layer&&gdi_data->surface)
	{
		if(gdi_data->created>0)
		{
			GxGDI_LayerEnable(gdi_data->layer, true);
			ret = 1;
		}
		else
		{
			draw_rect.x = 0;
			draw_rect.y = 0;
			draw_rect.w = gdi_data->rect.w;
			draw_rect.h = gdi_data->rect.h;
			if(gdi_data->type == NETAPPS_GDI_TEXT)
			{
				GXGDI_Lock();
				GXGDI_Begin();
				GXGDI_FillRect(gdi_data->surface, &draw_rect, gdi_data->back_color);
				GXGDI_DrawString(gdi_data->surface, gdi_data->content, &draw_rect, ALIGNMENT_HCENTRE | ALIGNMENT_VCENTRE, gdi_data->fore_color, GDI_STRING_PARAGRAPH);
				GXGDI_End();
				GxGDI_LayerEnable(gdi_data->layer, true);
				GxGDI_LayerUpdate(gdi_data->layer);
				gdi_data->created = 1;
				ret = 1;
				GXGDI_Unlock();
			}
			else if(gdi_data->type == NETAPPS_GDI_PIC)
			{
				if((gdi_data->content)&&(GxCore_FileExists(gdi_data->content) == GXCORE_FILE_EXIST))
				{
					image_desc *pdesc = NULL;;
					GAL_Rect rect = {0, 0, 0, 0};
					GuiSurface *surface = NULL;
					GXGDI_Lock();
					pdesc = gal_img_load(NULL, gdi_data->content);
					if(pdesc&&(pdesc->width>0)&&(pdesc->height>0))
					{
						rect.w = pdesc->width;
						rect.h = pdesc->height;
						surface = hd_get_surface(&rect, APP_UI_THEME_BPP, NULL, 0);
						if(surface)
						{
							GXGDI_Begin();
							GXGDI_DrawImage(surface, pdesc, 0, 0);
							hd_zoom_surface(surface, &rect, gdi_data->surface, (GAL_Rect *)&draw_rect);
							GXGDI_End();
							GxGDI_LayerEnable(gdi_data->layer, true);
							GxGDI_LayerUpdate(gdi_data->layer);
							gdi_data->created = 1;
							ret = 1;
							hd_free_surface(surface);
						}
					}
					if(pdesc)
					{
						gal_img_release(pdesc);
					}
					GXGDI_Unlock();
				}
			}
			else if(gdi_data->type == NETAPPS_GDI_QR)
			{
				if(gdi_data->content)
				{
					GXGDI_Lock();
					res = app_netapps_get_qr_data(gdi_data->content, gdi_data->surface, (GAL_Rect *)&draw_rect);
					if(res > 0)
					{
						GxGDI_LayerEnable(gdi_data->layer, true);
						GxGDI_LayerUpdate(gdi_data->layer);
						gdi_data->created = 1;
						ret = 1;
					}
					GXGDI_Unlock();
				}
			}
		}
	}

	return ret;
}

void app_netapps_gdi_hide(unsigned int handle)
{
	netapps_gdidata_t *gdi_data = NULL;

	gdi_data = (netapps_gdidata_t *)handle;
	if(gdi_data&&gdi_data->layer&&gdi_data->surface&&(gdi_data->created>0))
	{
		GxGDI_LayerEnable(gdi_data->layer, false);
	}
}

void app_netapps_gdi_destroy(unsigned int handle)
{
	netapps_gdidata_t *gdi_data = NULL;

	gdi_data = (netapps_gdidata_t *)handle;
	if(gdi_data)
	{
		if(gdi_data->layer)
		{
			GxGDI_LayerUnregister(gdi_data->layer);
			gdi_data->layer = NULL;
		}
		if(gdi_data->surface)
		{
			hd_free_surface(gdi_data->surface);
			gdi_data->surface = NULL;
		}
		if(gdi_data->content)
		{
			GxCore_Free(gdi_data->content);
			gdi_data->content = NULL;
		}
		GxCore_Free(gdi_data);
		gdi_data = NULL;
	}
}

int app_netapps_set_app_id(const char *app_id)
{
	return app_netapps_config_set_string(NETAPPS_APP_ID, app_id);
}

int app_netapps_get_app_id(char *buf, size_t buf_size)
{
	app_netapps_config_get_string(NETAPPS_APP_ID, buf, buf_size, NETAPPS_DEFAULT_APP_ID);
	return 0;
}

int app_netapps_chip_otp_write(unsigned int addr, unsigned char *buf, unsigned int size)
{
	return GxOtp_Write(addr, buf, size);
}

int app_netapps_chip_otp_read(unsigned int addr, unsigned char *buf, unsigned int size)
{
	return GxOtp_Read(addr, buf, size);
}

#define STRINGIFY(x) #x
#define ARCH_STR STRINGIFY(ARCH)
int app_netapps_chip_register_write(unsigned int addr, unsigned int value)
{
#ifdef ECOS_OS
	unsigned int reg_ptr = 0;
	unsigned int base_addr = 0;

	if(addr % sizeof(unsigned int) != 0)
	{
		return -1;
	}

	if(strcmp(ARCH_STR, "arm") == 0)
	{
		base_addr = 0x80000000;
	}
	else
	{
		base_addr = 0xa0000000;
	}
	reg_ptr = addr;
	if(reg_ptr<base_addr)
	{
		reg_ptr = reg_ptr + base_addr;
	}
	*(volatile unsigned int *)reg_ptr = value;
#else
	int dev = -1;
	unsigned int *reg_ptr = NULL;

	if(addr % sizeof(unsigned int) != 0)
	{
		return -1;
	}
	dev = GxAvdev_CreateDevice(0);
	if(dev <= 0)
	{
		return -1;
	}

	reg_ptr = (unsigned int *)GxCore_Map(dev, addr, sizeof(unsigned int));
	if(reg_ptr)
	{
		*reg_ptr = value;
		GxCore_UnMap(dev, reg_ptr, sizeof(unsigned int));
	}
	GxAvdev_DestroyDevice(dev);
#endif
	return 0;
}

int app_netapps_chip_register_read(unsigned int addr, unsigned int *value)
{
#ifdef ECOS_OS
	unsigned int reg_ptr = 0;
	unsigned int base_addr = 0;

	if(addr % sizeof(unsigned int) != 0)
	{
		return -1;
	}

	if(strcmp(ARCH_STR, "arm") == 0)
	{
		base_addr = 0x80000000;
	}
	else
	{
		base_addr = 0xa0000000;
	}
	reg_ptr = addr;
	if(reg_ptr<base_addr)
	{
		reg_ptr = reg_ptr + base_addr;
	}
	*value = *(volatile unsigned int *)reg_ptr;
#else
	int dev = -1;
	unsigned int *reg_ptr = NULL;

	if(addr % sizeof(unsigned int) != 0)
	{
		return -1;
	}
	dev = GxAvdev_CreateDevice(0);
	if(dev <= 0)
	{
		return -1;
	}

	reg_ptr = (unsigned int *)GxCore_Map(dev, addr, sizeof(unsigned int));
	if(reg_ptr)
	{
		*value = *reg_ptr;
		GxCore_UnMap(dev, reg_ptr, sizeof(unsigned int));
	}
	GxAvdev_DestroyDevice(dev);
#endif
	return 0;
}

void app_netapp_event_register(app_netapp_event_class* cls)
{
	int i = 0;

	if (netapp_event_class_num == 0)
		memset(&g_netapp_event_classes, 0, sizeof(g_netapp_event_classes));

	if (netapp_event_class_num >= GX_NETAPP_EVENT_MAX)
		return;

	for (i=0; i<netapp_event_class_num; i++) {
		if (g_netapp_event_classes[i] == cls)
			return;
	}

	g_netapp_event_classes[netapp_event_class_num++] = cls;

}

int app_netapps_top_msg_proc(GxMessage *msg)
{
	int i=0;
	int iRet=EVENT_TRANSFER_STOP;

	for(i=0;i<netapp_event_class_num;i++)
	{
		if(NULL != g_netapp_event_classes[i]->msg_proc_callback)
		{
			if(NULL != g_netapp_event_classes[i]->msg_wndName)
			{
				if(GXCORE_SUCCESS == GUI_CheckDialog(g_netapp_event_classes[i]->msg_wndName))
				{
					iRet = g_netapp_event_classes[i]->msg_proc_callback(msg);
				}
			}
			else
			{
				iRet = g_netapp_event_classes[i]->msg_proc_callback(msg);
			}
		}
	}
	return iRet;
}

void app_netapps_top_msg_dev_out_proc(void)
{
	int i = 0;

	for(i=0;i<netapp_event_class_num;i++)
	{
		if(NULL != g_netapp_event_classes[i]->msg_dev_out_callback)
		{
			g_netapp_event_classes[i]->msg_dev_out_callback();
		}
	}
}

int app_netapps_event_key_check(int key)
{
	int i = 0;
	int iRet = 0;

	for(i=0;i < netapp_event_class_num;i++)
	{
		if(NULL != g_netapp_event_classes[i]->key_net_open_check)
		{
			iRet = g_netapp_event_classes[i]->key_net_open_check(key);
		}
	}
	return iRet;
}

void app_netapps_event_init(void)
{
#if NETAPPS_SUPPORT
	extern app_netapp_event_class app_netapp_event_iptv;
	app_netapp_event_register(&app_netapp_event_iptv);
#endif
#ifdef XTREAM_SUPPORT
	extern app_netapp_event_class app_netapp_event_xtream;
	app_netapp_event_register(&app_netapp_event_xtream);
#endif
#ifdef STALKER_SUPPORT
	extern app_netapp_event_class app_netapp_event_stalker;
	app_netapp_event_register(&app_netapp_event_stalker);
#endif
#ifdef IPTV_EPG_SUPPORT
	extern app_netapp_event_class app_netapp_event_iptv_epg;
	app_netapp_event_register(&app_netapp_event_iptv_epg);
#endif
	app_netapps_event_init_ex();
}

#endif
