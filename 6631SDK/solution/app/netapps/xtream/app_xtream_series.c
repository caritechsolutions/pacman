#include "app.h"
#ifdef XTREAM_SERIES_SUPPORT
#include "app_pop.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_netvideo.h"
#include "app_netvideo_play.h"
#include "app_wnd_file_list.h"
#include "app_netapps_service.h"
#include "app_netapps_common.h"
#include <gx_xtream.h>

#define XTS_LOGO    "xtreamseries"
#define LIST_GROUP    "list_netvideo_group"
#define TEXT_MENU_KEY "text_netvideo_menu"

#define STR_ID_NET_XTREAM_SERIES "Xtream Series"

typedef void(* xts_thread_func)(void);

typedef enum
{
	XT_DATA_SERIES = 0,
	XT_DATA_EPISODE,
	XT_DATA_MAX
}STDataType;

typedef struct xts_pagepos_s
{
	int cur_big_page;
	int page_video_num;
	int cur_page;
	int sel;
	int total_page;
}xts_pagepos_t;

typedef struct xts_grouppos_s
{
	int cur_group;
	int sel_group;
	int total_group;
}xts_grouppos_t;

typedef struct xts_nav_bak_s
{
	STDataType type;
	int cur_big_page;
	int page_video_num;
	int cur_page;
	int sel;
	int total_page;
}xts_nav_bak_t;

static NetVideoObj s_video_obj = OBJ_VIDEO_GROUP;
static xts_pagepos_t xts_page_pos = {0};
static xts_grouppos_t xts_group_pos = {0};
static xts_nav_bak_t xts_nav_show_bak = {0};
static NetVideoItem s_page_video_item[NETVIDEO_MAX_PAGE_ITEM];

static char **s_xts_group_title = NULL;;
static char *xts_playurl = NULL;
static bool is_page_get_info = false;
static int is_xts_download = 0;
static int xts_video_depth = 0;
static STDataType xts_data_type = XT_DATA_SERIES;
static int xts_categorydata_finish = 0;
static int xts_videodata_finish = 0;
static unsigned int xts_data_handle = 0;
static unsigned int xts_geturl_handle = 0;
static int xts_episode_play_index = -1;

static gx_xtream_categorylist_t *xts_categorylist = NULL;
static gx_xtream_serieslist_t *xts_serieslist = NULL;
static gx_xtream_episodelist_t *xts_episodelist = NULL;
static event_list* xts_data_timer = NULL;
static gxapps_ret_t xts_ret_status = GXAPPS_SUCCESS;

static status_t app_xts_update_videodata(void);
static void app_xts_ok_press_cb(void);

static void app_xts_show_error_msg(void)
{
	if(xts_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_net_video_show_popup_msg("No results!", 1000);
	}
	else if(xts_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_net_video_show_popup_msg("Network error!", 1000);
	}
	else if(xts_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_net_video_show_popup_msg("Internal error!", 1000);
	}
	else if(xts_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_net_video_show_popup_msg("Unknown error!", 1000);
	}
}

static void xts_thread_body(void* arg)
{
	xts_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (xts_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_xts_thread(xts_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("xts", &thread_id, xts_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static int app_xts_video_busy(void)
{
	return is_xts_download;
}

static void app_xts_set_video_depth(int depth)
{
	xts_video_depth = depth;
}

static int app_xts_get_video_depth(void)
{
	return xts_video_depth;
}

static void app_xts_nav_show_bak(void)
{
	xts_nav_show_bak.type = xts_data_type;
	xts_nav_show_bak.cur_page = xts_page_pos.cur_page;
	xts_nav_show_bak.sel = xts_page_pos.sel;
	xts_nav_show_bak.total_page = xts_page_pos.total_page;
}

static void app_xts_nav_show_restore(void)
{
	xts_data_type = xts_nav_show_bak.type;
	xts_page_pos.cur_page = xts_nav_show_bak.cur_page;
	xts_page_pos.sel = xts_nav_show_bak.sel;
	xts_page_pos.total_page = xts_nav_show_bak.total_page;
}

static void app_xts_clean_menu_info(void)
{
	xts_page_pos.cur_page = 0;
	xts_page_pos.sel = 0;
	xts_page_pos.total_page = 0;

	xts_group_pos.cur_group = 0;
	xts_group_pos.sel_group = 0;
	xts_group_pos.total_group = 0;
	is_page_get_info = false;
	xts_video_depth = 0;
	xts_data_type = XT_DATA_SERIES;
	xts_episode_play_index = -1;
	app_xts_set_video_depth(0);
	memset(&xts_nav_show_bak, 0, sizeof(xts_nav_bak_t));
}

static status_t app_xts_update_group(void)
{
	int i= 0;
	int group_num = 0;

	APP_FREE(s_xts_group_title);
	if(xts_categorylist&&xts_categorylist->categories&&xts_categorylist->category_num>0)
	{
		group_num = xts_categorylist->category_num;
		s_xts_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
		if(s_xts_group_title == NULL)
		{
			return GXCORE_ERROR;
		}

		for(i=0; i<group_num; i++)
		{
			s_xts_group_title[i] = xts_categorylist->categories[i].category_name;
		}
	}

	xts_group_pos.total_group = group_num;
	app_net_video_group_update(xts_group_pos.total_group, s_xts_group_title);

	return GXCORE_SUCCESS;
}

static void app_xts_categoryfresh_stop(void)
{
	xts_categorydata_finish = 0;
}

static void app_xts_categoryfresh_start(void)
{
	xts_categorydata_finish = 1;
}

static void app_xts_videorefresh_stop(void)
{
	xts_videodata_finish = 0;
}

static void app_xts_videorefresh_start(STDataType type)
{
	if((type>=XT_DATA_SERIES)&&(type<XT_DATA_MAX))
	{
		xts_data_type = type;
	}
	else
	{
		xts_data_type = XT_DATA_SERIES;
	}
	xts_videodata_finish = 1;
}

static int xts_categorydata_timeout(void)
{
	app_net_video_hide_popup_msg();
	app_xts_show_error_msg();
	app_xts_update_group();
	app_xts_categoryfresh_stop();
	app_xts_ok_press_cb();
	return 0;
}

static int xts_videodata_timeout(void)
{
	app_net_video_hide_popup_msg();
	app_net_video_page_item_set_focus(xts_page_pos.sel);
	app_xts_update_videodata();
	app_net_video_pic_download_start();
	app_xts_videorefresh_stop();
	return 0;
}

static void app_xts_data_timer_stop(void)
{
	if(xts_data_timer)
	{
		remove_timer(xts_data_timer);
		xts_data_timer = NULL;
	}
}

static int xts_data_timer_timeout(void *usrdata)
{
	if(xts_videodata_finish&&(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS))
	{
		xts_videodata_timeout();
	}
	else if(xts_categorydata_finish&&(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS))
	{
		xts_categorydata_timeout();
	}
	return 0;
}

static void app_xts_data_timer_start(void)
{
	app_xts_data_timer_stop();
	xts_data_timer = create_timer(xts_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void xts_free_categorydata(void)
{
	if(xts_categorylist)
	{
		gx_xtream_free_categorylist(xts_categorylist);
		xts_categorylist = NULL;
	}
}

static void xts_free_seriesdata(void)
{
	if(xts_serieslist)
	{
		gx_xtream_free_serieslist(xts_serieslist);
		xts_serieslist = NULL;
	}
}

static void xts_free_episodedata(void)
{
	if(xts_episodelist)
	{
		gx_xtream_free_episodelist(xts_episodelist);
		xts_episodelist = NULL;
	}
}

static void xts_start_category_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_xtream_categorylist_t *data = NULL;

	is_xts_download = 1;
	xts_data_handle++;
	handle = xts_data_handle;
	ret = gx_xtream_get_series_categories(&data);
	if(handle == xts_data_handle)
	{
		xts_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			xts_categorylist = data;
		}
		app_xts_categoryfresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_xtream_free_categorylist(data);
		}
	}
	is_xts_download = 0;
}

static void xts_start_category_series_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_xtream_serieslist_t *data = NULL;

	is_xts_download = 1;
	xts_data_handle++;
	handle = xts_data_handle;
	ret = gx_xtream_get_category_series(xts_categorylist->categories[xts_group_pos.sel_group].category_id, &data);
	if(handle == xts_data_handle)
	{
		xts_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			xts_serieslist = data;
			xts_page_pos.cur_page = 0;
			xts_page_pos.sel = 0;
			xts_page_pos.total_page = (xts_serieslist->series_num/NETVIDEO_MAX_PAGE_ITEM);
			if(xts_serieslist->series_num% NETVIDEO_MAX_PAGE_ITEM > 0)
			{
				xts_page_pos.total_page++;
			}
			is_page_get_info = true;
		}
		app_xts_videorefresh_start(XT_DATA_SERIES);
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_xtream_free_serieslist(data);
		}
	}
	is_xts_download = 0;
}

static void xts_start_series_episodes_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_xtream_episodelist_t *data = NULL;

	is_xts_download = 1;
	xts_data_handle++;
	handle = xts_data_handle;
	if(xts_serieslist&&(xts_serieslist->series_num>0)
		&&((xts_nav_show_bak.cur_page*NETVIDEO_MAX_PAGE_ITEM+xts_nav_show_bak.sel)<xts_serieslist->series_num))
	{
		ret = gx_xtream_get_series_episodes(xts_serieslist->series[xts_nav_show_bak.cur_page*NETVIDEO_MAX_PAGE_ITEM+xts_nav_show_bak.sel].series_id, &data);
	}
	if(handle == xts_data_handle)
	{
		xts_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			xts_episodelist = data;
			xts_page_pos.cur_page = 0;
			xts_page_pos.sel = 0;
			xts_page_pos.total_page = (xts_episodelist->episode_num/NETVIDEO_MAX_PAGE_ITEM);
			if(xts_episodelist->episode_num% NETVIDEO_MAX_PAGE_ITEM > 0)
			{
				xts_page_pos.total_page++;
			}
			is_page_get_info = true;
		}
		app_xts_videorefresh_start(XT_DATA_EPISODE);
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_xtream_free_episodelist(data);
		}
	}
	is_xts_download = 0;
}

static void app_xts_category_update(void)
{
	app_xts_categoryfresh_stop();
	xts_free_categorydata();
	app_net_video_show_popup_msg("Updating...", 0);
	create_xts_thread(xts_start_category_feed);
}

static status_t app_xts_update_videodata(void)
{
	int i = 0;
	int curPageProgNum = 0;

	if(is_page_get_info&&(xts_data_type == XT_DATA_SERIES)&&xts_serieslist)
	{
		if(xts_serieslist->series_num>=((xts_page_pos.cur_page+1)*NETVIDEO_MAX_PAGE_ITEM))
		{
			curPageProgNum = NETVIDEO_MAX_PAGE_ITEM;
		}
		else
		{
			curPageProgNum = xts_serieslist->series_num%NETVIDEO_MAX_PAGE_ITEM;
		}

		for(i = 0; i < curPageProgNum; i++)
		{
			s_page_video_item[i].video_duration = NULL;
			s_page_video_item[i].video_title = xts_serieslist->series[xts_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+i].series_name;
			s_page_video_item[i].video_pic = xts_serieslist->series[xts_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+i].pic_url;
			s_page_video_item[i].video_author  = NULL;
			s_page_video_item[i].video_viewcnt = NULL;
		}

		app_net_video_page_item_update(curPageProgNum, s_page_video_item);
		app_net_video_page_info_update(xts_page_pos.total_page, xts_page_pos.cur_page);
	}
	else if(is_page_get_info&&(xts_data_type == XT_DATA_EPISODE)&&xts_episodelist)
	{
		if(xts_episodelist->episode_num>=((xts_page_pos.cur_page+1)*NETVIDEO_MAX_PAGE_ITEM))
		{
			curPageProgNum = NETVIDEO_MAX_PAGE_ITEM;
		}
		else
		{
			curPageProgNum = xts_episodelist->episode_num%NETVIDEO_MAX_PAGE_ITEM;
		}

		for(i = 0; i < curPageProgNum; i++)
		{
			s_page_video_item[i].video_duration = NULL;
			s_page_video_item[i].video_title = xts_episodelist->episodes[xts_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+i].episode_name;
			s_page_video_item[i].video_pic = xts_episodelist->episodes[xts_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+i].pic_url;
			s_page_video_item[i].video_author  = xts_episodelist->episodes[xts_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+i].season_name;
			s_page_video_item[i].video_viewcnt = NULL;
		}

		app_net_video_page_item_update(curPageProgNum, s_page_video_item);
		app_net_video_page_info_update(xts_page_pos.total_page, xts_page_pos.cur_page);
	}
	else
	{
		app_net_video_hide_gif();
		for(i = 0; i < NETVIDEO_MAX_PAGE_ITEM; i++)
		{
			s_page_video_item[i].video_duration = NULL;
			s_page_video_item[i].video_title   = STR_ID_UPDATE_FAILED;
			s_page_video_item[i].video_pic     = NULL;
			s_page_video_item[i].video_author  = NULL;
			s_page_video_item[i].video_viewcnt = NULL;
		}
		app_net_video_page_item_update(NETVIDEO_MAX_PAGE_ITEM, s_page_video_item);
		app_xts_show_error_msg();
	}
	return GXCORE_SUCCESS;
}

static void app_xts_play_exit_ctrl(void)
{
	xts_geturl_handle++;
}

static void app_xts_play_restart_ctrl(uint64_t cur_time)
{
	app_net_video_start_play(xts_playurl, PLAY_STATE_LOAD, cur_time, false);
}

static void app_xts_play_get_videourl(int source_index)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	int index = 0;
	char *url = NULL;

	xts_geturl_handle++;
	handle = xts_geturl_handle;
	APP_FREE(xts_playurl);
	index = xts_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+xts_page_pos.sel;
	if(xts_episodelist&&(xts_episodelist->episode_num>0)&&(index<xts_episodelist->episode_num))
	{
		ret = gx_xtream_get_episode_playurl(xts_episodelist->episodes[index].stream_ext, &url);
	}

	if(handle != xts_geturl_handle)
	{
		printf("player has exited!\n");
		if(ret == GXAPPS_SUCCESS)
		{
			gx_xtream_free_episode_playurl(url);
		}
		return;
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			printf("video url:%s\n", url);
			xts_playurl = GxCore_Strdup(url);
			gx_xtream_free_episode_playurl(url);
		}
		else
		{
			if(ret == GXAPPS_INTERNAL_ERROR)
			{
				printf("internal error\n");
			}
			else if(ret == GXAPPS_NETWORK_ERROR)
			{
				printf("network error\n");
			}
			else if(ret == GXAPPS_SERVER_ERROR)
			{
				printf("data from server is invalid\n");
			}
			else
			{
				printf("unknown error\n");
			}
		}
	}

	if((ret == GXAPPS_SUCCESS)&&xts_playurl)
	{
		app_net_video_start_play(xts_playurl, PLAY_STATE_LOAD, 0, false);
	}
	else
	{
		app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}

}

static status_t app_xts_play_video(char *prog_name)
{
	NetVideoPlayOps play_ops;
	memset(&play_ops, 0, sizeof(NetVideoPlayOps));

	play_ops.get_url             = app_xts_play_get_videourl;
	play_ops.netvideo_title         = prog_name;
	play_ops.source_num = 1;
	play_ops.play_ctrl.play_ok   = NULL;
	play_ops.play_ctrl.play_exit = app_xts_play_exit_ctrl;
	play_ops.play_ctrl.play_prev = NULL;
	play_ops.play_ctrl.play_next = NULL;
	play_ops.play_ctrl.play_restart = app_xts_play_restart_ctrl;

	return app_net_video_play(&play_ops);
}

static void app_xts_obj_change_cb(NetVideoObj obj)
{
	s_video_obj = obj;
}

static void app_xts_group_change_cb(int group_sel)
{
	xts_group_pos.cur_group = group_sel;
}

static void app_xts_page_change_cb(int page_num)
{
	if((page_num >= 0) && (page_num < xts_page_pos.total_page))
	{
		is_page_get_info = false;
		app_net_video_pic_download_stop();
		app_xts_videorefresh_stop();
		app_net_video_clear_all_item();
		xts_page_pos.cur_page = page_num;
		if(app_xts_video_busy())
		{
			app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
		}
		else
		{
			if(xts_data_type == XT_DATA_EPISODE)
			{
				is_page_get_info = true;
				app_xts_videorefresh_start(XT_DATA_EPISODE);
			}
			else
			{
				is_page_get_info = true;
				app_xts_videorefresh_start(XT_DATA_SERIES);
			}
		}
	}
}

static void app_xts_video_change_cb(int video_sel)
{
	if(xts_page_pos.sel != video_sel)
	{
		xts_page_pos.sel = video_sel;
	}
}

static void app_xts_menu_exit_cb(void)
{
	if(app_xts_get_video_depth()>0)
	{
		app_xts_set_video_depth(0);
		app_xts_nav_show_restore();
		app_net_video_pic_download_stop();
		app_xts_videorefresh_stop();
		app_net_video_clear_all_item();
		app_net_video_page_info_update(0, 0);
		app_xts_videorefresh_start(xts_data_type);
	}
	else
	{
		xts_data_handle++;
		app_xts_categoryfresh_stop();
		app_xts_videorefresh_stop();
		app_xts_data_timer_stop();
		APP_FREE(s_xts_group_title);
		APP_FREE(xts_playurl);
		app_xts_clean_menu_info();
		xts_free_categorydata();
		xts_free_seriesdata();
		xts_free_episodedata();
		app_netapps_app_destroy();
	}
}

static void app_xts_series_episode_update(void)
{
	app_net_video_clear_all_item();
	app_net_video_page_info_update(0, 0);
	app_xts_nav_show_bak();
	app_xts_set_video_depth(1);
	xts_page_pos.cur_page = 0;
	xts_page_pos.sel = 0;
	xts_page_pos.total_page = 0;
	xts_free_episodedata();
	app_net_video_show_popup_msg("Updating...", 0);
	create_xts_thread(xts_start_series_episodes_feed);
}

static void app_xts_ok_press_cb(void)
{
	int index = 0;

	if(app_xts_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
		return;
	}
	if((s_video_obj == OBJ_VIDEO_GROUP)
		&&xts_categorylist
		&&xts_categorylist->categories
		&&(xts_categorylist->category_num>0))
	{
		is_page_get_info = false;
		app_net_video_pic_download_stop();
		app_xts_videorefresh_stop();
		app_net_video_clear_all_item();
		xts_free_seriesdata();
		xts_free_episodedata();
		app_net_video_page_item_update(0, NULL);
		app_net_video_page_info_update(0, 0);
		xts_group_pos.sel_group = xts_group_pos.cur_group;
		xts_page_pos.cur_page = 0;
		xts_page_pos.sel = 0;
		xts_page_pos.total_page = 0;
		app_xts_set_video_depth(0);
		app_net_video_show_popup_msg("Updating...", 0);
		create_xts_thread(xts_start_category_series_feed);
	}
	else if((s_video_obj == OBJ_VIDEO_PAGE)&&is_page_get_info)
	{
		app_net_video_hide_gif();
		app_net_video_pic_download_stop();
		app_xts_videorefresh_stop();
		index = xts_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+xts_page_pos.sel;
		if((xts_data_type == XT_DATA_SERIES)&&xts_serieslist&&(xts_serieslist->series_num>0)
			&&(index<xts_serieslist->series_num))
		{
            app_common_view_focus_item_change(0);
			app_xts_series_episode_update();
		}
		else if((xts_data_type == XT_DATA_EPISODE)&&xts_episodelist&&(xts_episodelist->episode_num>0)
			&&(index<xts_episodelist->episode_num))
		{
			GUI_SetProperty(LIST_GROUP, "stop_rolling", NULL);
			app_xts_play_video(xts_episodelist->episodes[index].episode_name);
		}
	}
}

status_t app_create_xtreamseries_menu(void)
{
	NetVideoMenuCb video_cb;
	app_netapps_app_init();
	app_xts_data_timer_start();
	app_xts_clean_menu_info();
	app_xts_obj_change_cb(OBJ_VIDEO_GROUP);
	memset(&video_cb, 0, sizeof(NetVideoMenuCb));
	video_cb.obj_change         = app_xts_obj_change_cb;
	video_cb.video_group_change = app_xts_group_change_cb;
	video_cb.video_page_change  = app_xts_page_change_cb;
	video_cb.video_sel_change   = app_xts_video_change_cb;
	video_cb.video_exit         = app_xts_menu_exit_cb;
	video_cb.ok_press           = app_xts_ok_press_cb;
	video_cb.video_busy        = app_xts_video_busy;
	video_cb.get_video_depth       = app_xts_get_video_depth;
	video_cb.set_video_depth       = app_xts_set_video_depth;

	app_net_video_create(STR_ID_NET_XTREAM_SERIES, XTS_LOGO, &video_cb, xts_group_pos.cur_group);

	GUI_SetInterface("flush", NULL);
	app_net_video_page_info_update(0, 0);
	app_xts_category_update();

	return GXCORE_SUCCESS;
}

#endif

