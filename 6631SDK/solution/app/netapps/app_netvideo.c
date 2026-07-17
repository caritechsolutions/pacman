#include "app.h"
#include "app_module.h"
#if NETAPPS_SUPPORT
#include "app_netvideo.h"
#include "app_windows.h"
#include <gx_apps.h>
#include "app_common_view.h"

void app_net_video_time_timer_stop(void)
{
	// do nothing
}

void app_net_video_time_timer_reset(void)
{
	// do nothing
}
int app_net_video_draw_gif(void* usrdata)
{
	return app_common_view_draw_gif(usrdata);
}

void app_net_video_show_gif(void)
{
	app_common_view_show_gif();
}

void app_net_video_hide_gif(void)
{
	app_common_view_hide_gif();
}

bool app_net_video_get_popup_msg_on(void)
{
	return app_common_view_popup_state_get();
}

void app_net_video_hide_popup_msg(void)
{
	app_common_view_hide_popup_msg();
}

void app_net_video_show_popup_msg(char* pMsg, int time_out)
{
	app_common_view_show_popup_msg(pMsg, time_out);
}

void app_net_video_page_info_update(int page_total, int page_num)
{
	app_common_view_update_page_info(page_num, page_total);
}

void app_net_video_enable_group_roll(void)
{
	GUI_SetProperty(g_CommonView.widget.list_group, "enable_roll", NULL);
}

void app_net_video_disable_group_roll(void)
{
	GUI_SetProperty(g_CommonView.widget.list_group, "stop_rolling", NULL);
}

void app_net_video_set_group_sel(unsigned int sel)
{
	GUI_SetProperty(g_CommonView.widget.list_group, "select", &sel);
}

void app_net_video_set_group_active(unsigned int sel)
{
    GUI_SetProperty(g_CommonView.widget.list_group, "active", &sel);
}

void app_net_video_set_menukey_string(char *str)
{
	GUI_SetProperty(g_CommonView.widget.tips.text_menu, "string", str);
}

void app_net_video_enable_menu_key(char *tip, void (*func)(void))
{
	app_common_view_change_tips_key(STBK_MENU, true, tip, func);
}

void app_net_video_disable_menu_key(void)
{
	app_common_view_change_tips_key(STBK_MENU, false, NULL, NULL);
}

void app_net_video_enable_red_key(char *tip, void (*func)(void))
{
	app_common_view_change_tips_key(STBK_RED, true, tip, func);
}

void app_net_video_disable_red_key(void)
{
	app_common_view_change_tips_key(STBK_RED, false, NULL, NULL);
}

void app_net_video_enable_green_key(char *tip, void (*func)(void))
{
	app_common_view_change_tips_key(STBK_GREEN, true, tip, func);
}

void app_net_video_disable_green_key(void)
{
	app_common_view_change_tips_key(STBK_GREEN, false, NULL, NULL);
}

void app_net_video_enable_yellow_key(char *tip, void (*func)(void))
{
	app_common_view_change_tips_key(STBK_YELLOW, true, tip, func);
}

void app_net_video_disable_yellow_key(void)
{
	app_common_view_change_tips_key(STBK_YELLOW, false, NULL, NULL);
}

void app_net_video_enable_blue_key(char *tip, void (*func)(void))
{
	app_common_view_change_tips_key(STBK_BLUE, true, tip, func);
}

void app_net_video_disable_blue_key(void)
{
	app_common_view_change_tips_key(STBK_BLUE, false, NULL, NULL);
}

void app_net_video_clear_all_item(void)
{
	app_common_view_clear_all_item(false);
}

void app_net_video_page_item_set_focus(int focus)
{
	app_common_view_focus_item_change(focus);
}

void app_net_video_group_update(int group_total, char **group_str)
{
	app_common_view_group_list_update(group_total, group_str);
}

status_t app_net_video_set_obj(NetVideoObj obj)
{
	if(obj >= OBJ_VIDEO_TOTAL)
		return GXCORE_ERROR;
	app_common_view_focus_area_change(obj);
	return GXCORE_SUCCESS;
}

void app_net_video_pic_download_stop(void)
{
	app_common_view_logo_download_stop();
}

void app_net_video_pic_download_start(void)
{
	app_common_view_logo_download_start();
}

status_t app_net_video_page_item_update(int video_num, NetVideoItem *video_item)
{
	int i = 0;

	if (video_num < 0 || video_num > NETVIDEO_MAX_PAGE_ITEM)
		return GXCORE_ERROR;
	for (i = 0; i < video_num; i++)
	{
		g_CommonView.ctrl_data.page_item_data[i].logo = video_item[i].video_pic;
		g_CommonView.ctrl_data.page_item_data[i].title = video_item[i].video_title;
		g_CommonView.ctrl_data.page_item_data[i].author = video_item[i].video_author;
		g_CommonView.ctrl_data.page_item_data[i].duration = video_item[i].video_duration;
		g_CommonView.ctrl_data.page_item_data[i].viewcnt = video_item[i].video_viewcnt;
	}
	for ( ; i < NETVIDEO_MAX_PAGE_ITEM; i++)
	{
		memset(g_CommonView.ctrl_data.page_item_data + i, 0, sizeof(CommonViewItemData));
	}
	app_common_view_update_page_item_all(video_num, g_CommonView.ctrl_data.page_item_data);

	return GXCORE_SUCCESS;
}

typedef void (*obj_change_cb)(NetVideoObj obj);
static obj_change_cb s_obj_change = NULL;
static void _obj_change_func(int obj)
{
	if (s_obj_change)
		s_obj_change(obj);
}

typedef void (*service_cb)(void *usrdata);
static service_cb net_video_service_func = NULL;
static int app_net_video_service(GuiWidget *widget, void *usrdata)
{
	if(net_video_service_func)
	{
		net_video_service_func(usrdata);
	}
	return EVENT_TRANSFER_STOP;
}

status_t app_net_video_create(char *menu_title, char *menu_logo, NetVideoMenuCb *video_cb, int group_sel)
{
	CommonViewParas paras = {0};

	paras.show_type = COMMON_VIEW_TABLE_ITEM_TYPE;
	paras.focus_area = COMMON_VIEW_AREA_LIST_GROUP;
	paras.orig_group_sel = group_sel;
	paras.net_video_flag = true;
	paras.image.img_title = menu_logo;
	paras.string.str_title = menu_title;
	paras.string.str_ok = STR_ID_SELECT_PLAY;

	if(video_cb)
	{
		paras.keyfunc.busy_check = video_cb->video_busy;
		paras.keyfunc.page_sel_change = video_cb->video_page_change;
		paras.keyfunc.exit_item_depth = video_cb->video_exit;
		paras.keyfunc.get_item_depth = video_cb->get_video_depth;
		paras.keyfunc.set_item_depth = video_cb->set_video_depth;
		paras.keyfunc.page_item_sel_change = video_cb->video_sel_change;
		paras.keyfunc.group_sel_change = video_cb->video_group_change;
		s_obj_change = video_cb->obj_change;
		if (s_obj_change)
			paras.keyfunc.focus_area_change = _obj_change_func;
		net_video_service_func = video_cb->service;
		if(net_video_service_func)
		{
			paras.signal.service = app_net_video_service;
			
		}
		paras.keyfunc.key_ok = video_cb->ok_press;
	}
	app_common_view_create_dialog(&paras);

	return GXCORE_SUCCESS;
}

int app_net_video_get_max_page_item(void)
{
	return COMMON_VIEW_MAX_PAGE_ITEM;
}

#endif
