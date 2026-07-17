#include "app.h"
#ifdef XTREAM_SUPPORT
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

#define XTREAMVOD_LOGO    "xtreamvod"

#define STR_ID_NET_XTREAM_VOD "Xtream VOD"

typedef void(* xtreamvod_thread_func)(void);

typedef struct xtreamvod_pagepos_s
{
	int cur_page;
	int sel;
	int total_page;
}xtreamvod_pagepos_t;

typedef struct xtreamvod_grouppos_s
{
	int cur_group;
	int sel_group;
	int total_group;
}xtreamvod_grouppos_t;

static NetVideoObj s_video_obj = OBJ_VIDEO_GROUP;
static xtreamvod_pagepos_t xtreamvod_page_pos = {0};
static xtreamvod_grouppos_t xtreamvod_group_pos = {0};
static NetVideoItem s_page_video_item[NETVIDEO_MAX_PAGE_ITEM];

static char **s_xt_group_title = NULL;;
static char *xtreamvod_playurl = NULL;
static bool is_page_get_info = false;
static int is_xtreamvod_download = 0;
static unsigned int xtreamvod_data_handle = 0;
static unsigned int xtreamvod_geturl_handle = 0;

static gx_xtream_categorylist_t *xtreamvod_categorylist = NULL;
static gx_xtream_movielist_t *xtreamvod_videolist = NULL;
static event_list* xtreamvod_data_timer = NULL;
static int xtreamvod_categorydata_finish = 0;
static int xtreamvod_videodata_finish = 0;
static gxapps_ret_t xtreamvod_ret_status = GXAPPS_SUCCESS;

static status_t app_xtreamvod_update_videodata(void);
static status_t app_xtreamvod_update_group(void);
static void app_xtreamvod_ok_press_cb(void);

static int app_xtreamvod_video_busy(void)
{
	return is_xtreamvod_download;
}

static void xtreamvod_thread_body(void* arg)
{
	xtreamvod_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (xtreamvod_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_xtreamvod_thread(xtreamvod_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("xtreamvod", &thread_id, xtreamvod_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static void app_xtreamvod_show_error_msg(void)
{
	if(xtreamvod_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_net_video_show_popup_msg("No results!", 1000);
	}
	else if(xtreamvod_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_net_video_show_popup_msg("Network error!", 1000);
	}
	else if(xtreamvod_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_net_video_show_popup_msg("Internal error!", 1000);
	}
	else if(xtreamvod_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_net_video_show_popup_msg("Unknown error!", 1000);
	}
}

static void app_xtreamvod_clean_menu_info(void)
{
	xtreamvod_page_pos.cur_page = 0;
	xtreamvod_page_pos.sel = 0;
	xtreamvod_page_pos.total_page = 0;

	xtreamvod_group_pos.cur_group = 0;
	xtreamvod_group_pos.sel_group = 0;
	xtreamvod_group_pos.total_group = 0;
	is_page_get_info = false;
}

static void app_xtreamvod_categoryrefresh_stop(void)
{
	xtreamvod_categorydata_finish = 0;
}

static void app_xtreamvod_categoryrefresh_start(void)
{
	xtreamvod_categorydata_finish = 1;
}

static void app_xtreamvod_videorefresh_stop(void)
{
	xtreamvod_videodata_finish = 0;
}

static void app_xtreamvod_videorefresh_start(void)
{
	xtreamvod_videodata_finish = 1;
}

static int xtreamvod_categorydata_timer_timeout(void *usrdata)
{
	app_net_video_hide_popup_msg();
	app_xtreamvod_show_error_msg();
	app_xtreamvod_update_group();
	app_xtreamvod_categoryrefresh_stop();
	app_xtreamvod_ok_press_cb();
	return 0;
}

static int xtreamvod_videodata_timer_timeout(void *usrdata)
{
	app_net_video_hide_popup_msg();
	app_xtreamvod_update_videodata();
	app_net_video_pic_download_start();
	app_xtreamvod_videorefresh_stop();
	return 0;
}

static void app_xtreamvod_data_timer_stop(void)
{
	if(xtreamvod_data_timer)
	{
		remove_timer(xtreamvod_data_timer);
		xtreamvod_data_timer = NULL;
	}
}

static int xtreamvod_data_timer_timeout(void *usrdata)
{
	if(xtreamvod_categorydata_finish && (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
	{
		xtreamvod_categorydata_timer_timeout(NULL);
	}
	if(xtreamvod_videodata_finish && (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
	{
		xtreamvod_videodata_timer_timeout(NULL);
	}
	return 0;
}

static void app_xtreamvod_data_timer_start(void)
{
	app_xtreamvod_data_timer_stop();
	xtreamvod_data_timer = create_timer(xtreamvod_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void xtreamvod_free_categorydata(void)
{
	if(xtreamvod_categorylist)
	{
		gx_xtream_free_categorylist(xtreamvod_categorylist);
		xtreamvod_categorylist = NULL;
	}
}

static void xtreamvod_free_videodata(void)
{
	if(xtreamvod_videolist)
	{
		gx_xtream_free_movielist(xtreamvod_videolist);
		xtreamvod_videolist = NULL;
	}
}

static void xtreamvod_start_category_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_xtream_categorylist_t *data = NULL;

	is_xtreamvod_download = 1;
	xtreamvod_data_handle++;
	handle = xtreamvod_data_handle;
	ret = gx_xtream_get_movie_categories(&data);
	if(handle == xtreamvod_data_handle)
	{
		xtreamvod_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			xtreamvod_categorylist = data;
		}
		app_xtreamvod_categoryrefresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_xtream_free_categorylist(data);
		}
	}
	is_xtreamvod_download = 0;
}

static void xtreamvod_start_category_video_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_xtream_movielist_t	*data = NULL;

	is_xtreamvod_download = 1;
	xtreamvod_data_handle++;
	handle = xtreamvod_data_handle;
	xtreamvod_free_videodata();
	ret = gx_xtream_get_category_movies(xtreamvod_categorylist->categories[xtreamvod_group_pos.sel_group].category_id, &data);
	if(handle == xtreamvod_data_handle)
	{
		xtreamvod_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			xtreamvod_videolist = data;
			xtreamvod_page_pos.total_page = xtreamvod_videolist->movie_num / NETVIDEO_MAX_PAGE_ITEM;
			if( xtreamvod_videolist->movie_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				xtreamvod_page_pos.total_page++;
			is_page_get_info = true;
		}
		app_xtreamvod_videorefresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_xtream_free_movielist(data);
		}
	}
	is_xtreamvod_download = 0;
}

static status_t app_xtreamvod_update_group(void)
{
	int i= 0;
	int group_num = 0;

	APP_FREE(s_xt_group_title);
	if(xtreamvod_categorylist&&xtreamvod_categorylist->categories&&xtreamvod_categorylist->category_num>0)
	{
		group_num = xtreamvod_categorylist->category_num;
		s_xt_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
		if(s_xt_group_title == NULL)
		{
			return GXCORE_ERROR;
		}
		for(i=0; i<xtreamvod_categorylist->category_num; i++)
		{
			s_xt_group_title[i] = xtreamvod_categorylist->categories[i].category_name;
		}
	}
	xtreamvod_group_pos.cur_group = 0;
	xtreamvod_group_pos.sel_group = 0;
	xtreamvod_group_pos.total_group = group_num;
	app_net_video_group_update(xtreamvod_group_pos.total_group, s_xt_group_title);

	return GXCORE_SUCCESS;
}

static status_t app_xtreamvod_update_videodata(void)
{
	int i = 0;
	int curPageProgNum = 0;

	if(is_page_get_info&&xtreamvod_videolist)
	{
		if(xtreamvod_videolist->movie_num>=((xtreamvod_page_pos.cur_page+1)*NETVIDEO_MAX_PAGE_ITEM))
		{
			curPageProgNum = NETVIDEO_MAX_PAGE_ITEM;
		}
		else
		{
			curPageProgNum = xtreamvod_videolist->movie_num%NETVIDEO_MAX_PAGE_ITEM;
		}

		for(i = 0; i < curPageProgNum; i++)
		{
			s_page_video_item[i].video_title = xtreamvod_videolist->movies[xtreamvod_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+i].movie_name;
			s_page_video_item[i].video_pic = xtreamvod_videolist->movies[xtreamvod_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+i].pic_url;
		}
		app_net_video_page_item_update(curPageProgNum, s_page_video_item);
		app_net_video_page_info_update(xtreamvod_page_pos.total_page, xtreamvod_page_pos.cur_page);
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
		app_xtreamvod_show_error_msg();
	}

	return GXCORE_SUCCESS;
}

static void app_xtreamvod_play_exit_ctrl(void)
{
	xtreamvod_geturl_handle++;
    app_net_video_enable_group_roll();
}

static void app_xtreamvod_play_restart_ctrl(uint64_t cur_time)
{
	app_net_video_start_play(xtreamvod_playurl, PLAY_STATE_LOAD, cur_time, false);
}

static void app_xtreamvod_play_get_url(int source_index)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	char *url = NULL;

	xtreamvod_geturl_handle++;
	handle = xtreamvod_geturl_handle;
	APP_FREE(xtreamvod_playurl);

	if(xtreamvod_videolist&&(xtreamvod_videolist->movies)
		&&(xtreamvod_videolist->movie_num>(xtreamvod_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+xtreamvod_page_pos.sel)))
	{
		ret = gx_xtream_get_movie_playurl(xtreamvod_videolist->movies[xtreamvod_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+xtreamvod_page_pos.sel].stream_ext, &url);
	}


	if(handle != xtreamvod_geturl_handle)
	{
		app_log_info("player has exited!\n");
		if(ret == GXAPPS_SUCCESS)
		{
			gx_xtream_free_movie_playurl(url);
		}
		return;
	}

	if((ret == GXAPPS_SUCCESS)&&url)
	{
		xtreamvod_playurl = GxCore_Strdup(url);
		gx_xtream_free_movie_playurl(url);
		app_log_info("play url:%s\n", xtreamvod_playurl);
		app_net_video_start_play(xtreamvod_playurl, PLAY_STATE_LOAD, 0, false);
	}
	else
	{
		if(ret == GXAPPS_NETWORK_ERROR)
		{
			app_net_video_set_error_str("Network error!");
		}
		else if(ret == GXAPPS_SERVER_ERROR)
		{
			app_net_video_set_error_str("Movie not founded on server");
		}
		else
		{
			app_net_video_set_error_str("Internal error!");
		}
		app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}
}

static status_t app_xtreamvod_play(char *prog_name)
{
	NetVideoPlayOps play_ops;
	memset(&play_ops, 0, sizeof(NetVideoPlayOps));

	play_ops.get_url             = app_xtreamvod_play_get_url;
	play_ops.netvideo_title         = prog_name;
	play_ops.play_ctrl.play_ok   = NULL;
	play_ops.play_ctrl.play_exit = app_xtreamvod_play_exit_ctrl;
	play_ops.play_ctrl.play_prev = NULL;
	play_ops.play_ctrl.play_next = NULL;
	play_ops.play_ctrl.play_restart = app_xtreamvod_play_restart_ctrl;

	return app_net_video_play(&play_ops);
}

static void app_xtreamvod_obj_change_cb(NetVideoObj obj)
{
	s_video_obj = obj;
}

static void app_xtreamvod_group_change_cb(int group_sel)
{
	xtreamvod_group_pos.cur_group = group_sel;
}

static void app_xtreamvod_page_change_cb(int page_num)
{
	if((page_num >= 0) && (page_num < xtreamvod_page_pos.total_page))
	{
		is_page_get_info = true;
		app_net_video_pic_download_stop();
		app_net_video_clear_all_item();
		xtreamvod_page_pos.cur_page = page_num;
		app_xtreamvod_update_videodata();
		app_net_video_pic_download_start();
	}
}

static void app_xtreamvod_video_change_cb(int video_sel)
{
	if(xtreamvod_page_pos.sel != video_sel)
	{
		xtreamvod_page_pos.sel = video_sel;
	}
}

static void app_xtreamvod_menu_exit_cb(void)
{
	xtreamvod_data_handle++;
	app_xtreamvod_categoryrefresh_stop();
	app_xtreamvod_videorefresh_stop();
	app_xtreamvod_data_timer_stop();
	APP_FREE(s_xt_group_title);
	APP_FREE(xtreamvod_playurl);
	app_xtreamvod_clean_menu_info();
	xtreamvod_free_categorydata();
	xtreamvod_free_videodata();
	app_netapps_app_destroy();
}

static void app_xtreamvod_ok_press_cb(void)
{
	if((s_video_obj == OBJ_VIDEO_GROUP)
		&&xtreamvod_categorylist
		&&xtreamvod_categorylist->categories
		&&(xtreamvod_categorylist->category_num>0))
	{
		is_page_get_info = false;
		app_xtreamvod_videorefresh_stop();
		app_net_video_clear_all_item();
		xtreamvod_free_videodata();
		app_net_video_page_item_update(0, NULL);
		app_net_video_page_info_update(0, 0);
		xtreamvod_group_pos.sel_group = xtreamvod_group_pos.cur_group;
		xtreamvod_page_pos.cur_page = 0;
		xtreamvod_page_pos.sel = 0;
		xtreamvod_page_pos.total_page = 0;
		app_net_video_show_popup_msg("Updating...", 0);
		create_xtreamvod_thread(xtreamvod_start_category_video_feed);
	}
	else if((s_video_obj == OBJ_VIDEO_PAGE)&&is_page_get_info
		&&xtreamvod_videolist&&(xtreamvod_videolist->movies)
		&&(xtreamvod_videolist->movie_num>(xtreamvod_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+xtreamvod_page_pos.sel)))
	{
		app_net_video_hide_gif();
		app_net_video_disable_group_roll();
		app_net_video_pic_download_stop();
		app_xtreamvod_play(xtreamvod_videolist->movies[xtreamvod_page_pos.cur_page*NETVIDEO_MAX_PAGE_ITEM+xtreamvod_page_pos.sel].movie_name);
	}
}

static void app_xtreamvod_category_update(void)
{
	app_xtreamvod_categoryrefresh_stop();
	xtreamvod_free_categorydata();
	if(app_xtreamvod_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
	}
	else
	{
		app_net_video_show_popup_msg("Updating...", 0);
		create_xtreamvod_thread(xtreamvod_start_category_feed);
	}
}

status_t app_create_xtreamvod_menu(void)
{
	NetVideoMenuCb video_cb;
	app_netapps_app_init();
	app_xtreamvod_data_timer_start();
	app_xtreamvod_clean_menu_info();
	app_xtreamvod_obj_change_cb(OBJ_VIDEO_GROUP);
	memset(&video_cb, 0, sizeof(NetVideoMenuCb));
	video_cb.obj_change         = app_xtreamvod_obj_change_cb;
	video_cb.video_group_change = app_xtreamvod_group_change_cb;
	video_cb.video_page_change  = app_xtreamvod_page_change_cb;
	video_cb.video_sel_change   = app_xtreamvod_video_change_cb;
	video_cb.video_exit         = app_xtreamvod_menu_exit_cb;
	video_cb.ok_press           = app_xtreamvod_ok_press_cb;
	video_cb.video_busy        = app_xtreamvod_video_busy;
	app_net_video_create(STR_ID_NET_XTREAM_VOD, XTREAMVOD_LOGO, &video_cb, xtreamvod_group_pos.cur_group);

	GUI_SetInterface("flush", NULL);
	app_net_video_page_info_update(0, 0);
	app_xtreamvod_category_update();
	return GXCORE_SUCCESS;
}
#endif
