#include "app.h"
#ifdef STALKER_SUPPORT
#include "app_pop.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_netvideo.h"
#include "app_netvideo_play.h"
#include "app_wnd_file_list.h"
#include "app_netapps_service.h"
#include "app_netapps_common.h"
#include <gx_stalker.h>
#include "app_keyboard_language.h"

#define STR_ID_NET_STALKER_VOD "Stalker VOD"

#define STALKERVOD_LOGO    "stalkervod"

typedef void(* stalkervod_thread_func)(void);

typedef struct stalkervod_pagepos_s
{
	int cur_big_page;
	int page_video_num;
	int cur_page;
	int sel;
	int total_page;
}stalkervod_pagepos_t;

typedef struct stalkervod_grouppos_s
{
	int cur_group;
	int sel_group;
	int total_group;
}stalkervod_grouppos_t;

static NetVideoObj s_video_obj = OBJ_VIDEO_GROUP;
static stalkervod_pagepos_t stalkervod_page_pos = {0};
static stalkervod_grouppos_t stalkervod_group_pos = {0};
static NetVideoItem s_page_video_item[NETVIDEO_MAX_PAGE_ITEM];

static char **s_sv_group_title = NULL;;
static char *stalkervod_playurl = NULL;
static char *stalkervod_search_keyword = NULL;
static bool is_page_get_info = false;
static int is_stalkervod_download = 0;
static int stalkervod_videodata_finish = 0;
static int stalkervod_categorydata_finish = 0;
static unsigned int stalkervod_data_handle = 0;
static unsigned int stalkervod_geturl_handle = 0;
static int stalkervod_series_play_index = -1;

static gx_stalker_categorylist_t *stalkervod_categorylist = NULL;
static gx_stalker_movielist_t *stalkervod_videolist = NULL;
static event_list* stalkervod_data_timer = NULL;
static gxapps_ret_t stalkervod_ret_status = GXAPPS_SUCCESS;

static status_t app_stalkervod_update_videodata(void);
static status_t app_stalkervod_update_group(void);
static void app_stalkervod_ok_press_cb(void);

static void app_stalkervod_show_error_msg(void)
{
	if(stalkervod_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_net_video_show_popup_msg("No results!", 1000);
	}
	else if(stalkervod_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_net_video_show_popup_msg("Network error!", 1000);
	}
	else if(stalkervod_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_net_video_show_popup_msg("Internal error!", 1000);
	}
	else if(stalkervod_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_net_video_show_popup_msg("Unknown error!", 1000);
	}
}

static int app_stalkervod_video_busy(void)
{
	return is_stalkervod_download;
}

static void stalkervod_thread_body(void* arg)
{
	stalkervod_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (stalkervod_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_stalkervod_thread(stalkervod_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("stalkervod", &thread_id, stalkervod_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static void app_stalkervod_clean_menu_info(void)
{
	stalkervod_page_pos.cur_big_page = 0;
	stalkervod_page_pos.page_video_num = 0;
	stalkervod_page_pos.cur_page = 0;
	stalkervod_page_pos.sel = 0;
	stalkervod_page_pos.total_page = 0;

	stalkervod_group_pos.cur_group = 1;
	stalkervod_group_pos.sel_group = 1;
	stalkervod_group_pos.total_group = 0;
	is_page_get_info = false;
	stalkervod_series_play_index = -1;
}

static int app_stalkervod_videodata_exits(void)
{
	int res = 0;
	if((stalkervod_videolist&&(stalkervod_videolist->movie_num>0))
		&&(stalkervod_page_pos.page_video_num>0)
		&&(stalkervod_page_pos.cur_page>=(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM*(stalkervod_page_pos.cur_big_page-1)))
		&&(stalkervod_page_pos.cur_page<(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM*stalkervod_page_pos.cur_big_page)))
	{
		res = 1;
	}
	return res;
}

static void app_stalkervod_videorefresh_stop(void)
{
	stalkervod_videodata_finish = 0;
}

static void app_stalkerlvod_videorefresh_start(void)
{
	stalkervod_videodata_finish = 1;
}

static void app_stalkervod_categoryrefresh_stop(void)
{
	stalkervod_categorydata_finish = 0;
}

static void app_stalkervod_categoryrefresh_start(void)
{
	stalkervod_categorydata_finish = 1;
}

static int stalkervod_videodata_timer_timeout(void *usrdata)
{
	app_net_video_hide_popup_msg();
	app_stalkervod_update_videodata();
	app_net_video_pic_download_start();
	app_stalkervod_videorefresh_stop();
	return 0;
}

static int stalkervod_categorydata_timer_timeout(void *usrdata)
{
	app_net_video_hide_popup_msg();
	app_stalkervod_show_error_msg();
	app_stalkervod_update_group();
	app_stalkervod_categoryrefresh_stop();
	app_stalkervod_ok_press_cb();
	return 0;
}

static void app_stalkervod_data_timer_stop(void)
{
	if(stalkervod_data_timer)
	{
		remove_timer(stalkervod_data_timer);
		stalkervod_data_timer = NULL;
	}
}

static int stalkervod_data_timer_timeout(void *usrdata)
{
	if(stalkervod_videodata_finish && (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
	{
		stalkervod_videodata_timer_timeout(NULL);
	}
	if(stalkervod_categorydata_finish && (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
	{
		stalkervod_categorydata_timer_timeout(NULL);
	}
	return 0;
}

static void app_stalkervod_data_timer_start(void)
{
	app_stalkervod_data_timer_stop();
	stalkervod_data_timer = create_timer(stalkervod_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void stalkervod_free_videodata(void)
{
	if(stalkervod_videolist)
	{
		gx_stalker_free_movielist(stalkervod_videolist);
		stalkervod_videolist = NULL;
	}
}

static void stalkervod_free_categorydata(void)
{
	if(stalkervod_categorylist)
	{
		gx_stalker_free_categorylist(stalkervod_categorylist);
		stalkervod_categorylist = NULL;
	}
}

static void stalkervod_start_category_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_stalker_categorylist_t *category = NULL;

	is_stalkervod_download = 1;
	stalkervod_data_handle++;
	handle = stalkervod_data_handle;
	ret = gx_stalker_get_movie_categories(&category);
	if(handle == stalkervod_data_handle)
	{
		stalkervod_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			stalkervod_categorylist = category;
		}
		app_stalkervod_categoryrefresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_categorylist(category);
		}
	}
	is_stalkervod_download = 0;
}

static void stalkervod_start_category_video_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_stalker_movielist_t	*data = NULL;

	is_stalkervod_download = 1;
	stalkervod_data_handle++;
	handle = stalkervod_data_handle;
	stalkervod_free_videodata();
	if(stalkervod_page_pos.page_video_num==0)
	{
		stalkervod_page_pos.cur_big_page = 1;
	}
	else
	{
		stalkervod_page_pos.cur_big_page = stalkervod_page_pos.cur_page/(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM)+1;
	}
	ret = gx_stalker_get_category_movies(stalkervod_categorylist->categories[stalkervod_group_pos.sel_group-1].category_id, stalkervod_page_pos.cur_big_page, &data);
	if(handle == stalkervod_data_handle)
	{
		stalkervod_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			stalkervod_videolist = data;
			if(stalkervod_page_pos.page_video_num==0)
			{
				stalkervod_page_pos.page_video_num = (stalkervod_videolist->movie_num/NETVIDEO_MAX_PAGE_ITEM)*NETVIDEO_MAX_PAGE_ITEM;
				if(stalkervod_videolist->movie_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					stalkervod_page_pos.page_video_num += NETVIDEO_MAX_PAGE_ITEM;
				}
				stalkervod_page_pos.total_page = (stalkervod_videolist->movie_total_num/NETVIDEO_MAX_PAGE_ITEM);
				if(stalkervod_videolist->movie_total_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					stalkervod_page_pos.total_page++;
				}
			}
			is_page_get_info = true;
		}
		app_stalkerlvod_videorefresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_movielist(data);
		}
	}
	is_stalkervod_download = 0;
}

static void stalkervod_start_search_video_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_stalker_movielist_t	*data = NULL;

	is_stalkervod_download = 1;
	stalkervod_data_handle++;
	handle = stalkervod_data_handle;
	stalkervod_free_videodata();
	if(stalkervod_page_pos.page_video_num==0)
	{
		stalkervod_page_pos.cur_big_page = 1;
	}
	else
	{
		stalkervod_page_pos.cur_big_page = stalkervod_page_pos.cur_page/(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM)+1;
	}
	ret = gx_stalker_search_movies(stalkervod_search_keyword, stalkervod_page_pos.cur_big_page, &data);
	if(handle == stalkervod_data_handle)
	{
		stalkervod_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			stalkervod_videolist = data;
			if(stalkervod_page_pos.page_video_num==0)
			{
				stalkervod_page_pos.page_video_num = (stalkervod_videolist->movie_num/NETVIDEO_MAX_PAGE_ITEM)*NETVIDEO_MAX_PAGE_ITEM;
				if(stalkervod_videolist->movie_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					stalkervod_page_pos.page_video_num += NETVIDEO_MAX_PAGE_ITEM;
				}
				stalkervod_page_pos.total_page = (stalkervod_videolist->movie_total_num/NETVIDEO_MAX_PAGE_ITEM);
				if(stalkervod_videolist->movie_total_num % NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					stalkervod_page_pos.total_page++;
				}
			}
			is_page_get_info = true;
		}
		app_stalkerlvod_videorefresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_movielist(data);
		}
	}
	is_stalkervod_download = 0;
}

void _stalker_vod_search_full_keyboard_proc(PopKeyboard *data)
{
	 if(data->in_ret == POP_VAL_CANCEL)
	 {
		return;
	 }

	if((data->out_name == NULL)||strlen(data->out_name)==0)
	{
		return;
	}
	APP_FREE(stalkervod_search_keyword);
	stalkervod_search_keyword = GxCore_Strdup(data->out_name);
	if(app_stalkervod_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
	}
	else
	{
		app_net_video_show_popup_msg("Updating...", 0);
		create_stalkervod_thread(stalkervod_start_search_video_feed);
	}
}

static void _stalker_vod_search(void)
{
    static PopKeyboard keyboard;

    app_keyboard_save_cb(app_input_save_space_cb);
    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = NULL;
    keyboard.max_num = 127;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = NULL;
    keyboard.release_cb = _stalker_vod_search_full_keyboard_proc;
    keyboard.usr_data   = NULL;
    keyboard.pos.x = 500;
    multi_language_keyboard_create(&keyboard);
}

static status_t app_stalkervod_update_group(void)
{
	int i= 0;
	int group_num = 0;

	APP_FREE(s_sv_group_title);

	if(stalkervod_categorylist&&stalkervod_categorylist->categories&&stalkervod_categorylist->category_num>0)
	{
		group_num = 1 + stalkervod_categorylist->category_num;
		s_sv_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
		if(s_sv_group_title == NULL)
		{
			return GXCORE_ERROR;
		}

		s_sv_group_title[0] = STR_ID_SEARCH;
		for(i=0; i<stalkervod_categorylist->category_num; i++)
		{
			s_sv_group_title[i+1] = stalkervod_categorylist->categories[i].category_name;
		}
		stalkervod_group_pos.cur_group = 1;
		stalkervod_group_pos.sel_group = 1;
	}

	stalkervod_group_pos.total_group = group_num;
	app_net_video_group_update(stalkervod_group_pos.total_group, s_sv_group_title);

	return GXCORE_SUCCESS;
}

static status_t app_stalkervod_update_videodata(void)
{
	int i = 0;
	int curPageProgNum = 0;
	int small_page = 0;

	if(is_page_get_info&&app_stalkervod_videodata_exits())
	{
		small_page = stalkervod_page_pos.cur_page%(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
		if(stalkervod_videolist->movie_num>=((small_page+1)*NETVIDEO_MAX_PAGE_ITEM))
		{
			curPageProgNum = NETVIDEO_MAX_PAGE_ITEM;
		}
		else
		{
			curPageProgNum = stalkervod_videolist->movie_num%NETVIDEO_MAX_PAGE_ITEM;
		}

		for(i = 0; i < curPageProgNum; i++)
		{
			s_page_video_item[i].video_title = stalkervod_videolist->movielist[small_page*NETVIDEO_MAX_PAGE_ITEM+i].movie_name;
			s_page_video_item[i].video_pic = stalkervod_videolist->movielist[small_page*NETVIDEO_MAX_PAGE_ITEM+i].pic_url;
		}
		app_net_video_page_item_update(curPageProgNum, s_page_video_item);
		app_net_video_page_info_update(stalkervod_page_pos.total_page, stalkervod_page_pos.cur_page);
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
		app_stalkervod_show_error_msg();
	}

	return GXCORE_SUCCESS;
}

static void app_stalkervod_play_exit_ctrl(void)
{
	stalkervod_geturl_handle++;
    app_net_video_enable_group_roll();
}

static void app_stalkervod_play_restart_ctrl(uint64_t cur_time)
{
	app_net_video_start_play(stalkervod_playurl, PLAY_STATE_LOAD, cur_time, false);
}

static void app_stalkervod_play_get_url(int source_index)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	int small_page = 0;
	unsigned int handle = 0;
	char *url = NULL;

	stalkervod_geturl_handle++;
	handle = stalkervod_geturl_handle;
	APP_FREE(stalkervod_playurl);
	if(stalkervod_videolist&&(stalkervod_videolist->movielist)
		&&(stalkervod_videolist->movie_num>stalkervod_page_pos.sel))
	{
	 	small_page = stalkervod_page_pos.cur_page%(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
		if((small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel)<stalkervod_videolist->movie_num)
		{
			if(stalkervod_series_play_index>=0)
			{
				ret = gx_stalker_get_movie_playurl(stalkervod_videolist->movielist[small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel].play_cmd, stalkervod_series_play_index, &url);
			}
			else
			{
				ret = gx_stalker_get_movie_playurl(stalkervod_videolist->movielist[small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel].play_cmd, -1, &url);
			}
		}
	}

	if(handle != stalkervod_geturl_handle)
	{
		app_log_info("player has exited!\n");
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_movie_playurl(url);
		}
		return;
	}

	if((ret == GXAPPS_SUCCESS)&&url)
	{
		stalkervod_playurl = GxCore_Strdup(url);
		gx_stalker_free_movie_playurl(url);
		app_log_info("play url:%s\n", stalkervod_playurl);
		app_net_video_start_play(stalkervod_playurl, PLAY_STATE_LOAD, 0, false);
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

static status_t app_stalkervod_play(char *prog_name)
{
	NetVideoPlayOps play_ops;
	memset(&play_ops, 0, sizeof(NetVideoPlayOps));

	play_ops.get_url             = app_stalkervod_play_get_url;
	play_ops.netvideo_title         = prog_name;
	play_ops.play_ctrl.play_ok   = NULL;
	play_ops.play_ctrl.play_exit = app_stalkervod_play_exit_ctrl;
	play_ops.play_ctrl.play_prev = NULL;
	play_ops.play_ctrl.play_next = NULL;
	play_ops.play_ctrl.play_restart = app_stalkervod_play_restart_ctrl;

	return app_net_video_play(&play_ops);
}

static void app_stalkervod_obj_change_cb(NetVideoObj obj)
{
	s_video_obj = obj;
}

static void app_stalkervod_group_change_cb(int group_sel)
{
	stalkervod_group_pos.cur_group = group_sel;
}

static void app_stalkervod_page_change_cb(int page_num)
{
	if((page_num >= 0) && (page_num < stalkervod_page_pos.total_page))
	{
		app_net_video_pic_download_stop();
		app_stalkervod_videorefresh_stop();
		app_net_video_clear_all_item();
		stalkervod_page_pos.cur_page = page_num;
		if(app_stalkervod_videodata_exits())
		{
			is_page_get_info = true;
			app_stalkerlvod_videorefresh_start();
		}
		else
		{
			if(app_stalkervod_video_busy())
			{
				app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
			}
			else
			{
				stalkervod_free_videodata();
				app_net_video_show_popup_msg("Updating...", 0);
				if(stalkervod_group_pos.sel_group == 0) //search
				{
					create_stalkervod_thread(stalkervod_start_search_video_feed);
				}
				else
				{
					create_stalkervod_thread(stalkervod_start_category_video_feed);
				}
			}
		}
	}
}

static void app_stalkervod_video_change_cb(int video_sel)
{
	if(stalkervod_page_pos.sel != video_sel)
	{
		stalkervod_page_pos.sel = video_sel;
	}
}

static void app_stalkervod_menu_exit_cb(void)
{
	stalkervod_data_handle++;
	app_stalkervod_categoryrefresh_stop();
	app_stalkervod_videorefresh_stop();
	app_stalkervod_data_timer_stop();
	APP_FREE(s_sv_group_title);
	APP_FREE(stalkervod_playurl);
	APP_FREE(stalkervod_search_keyword);
	app_stalkervod_clean_menu_info();
	stalkervod_free_videodata();
	stalkervod_free_categorydata();
	app_netapps_app_destroy();
}

static void app_stalkervod_category_update(void)
{
	app_stalkervod_categoryrefresh_stop();
	stalkervod_free_categorydata();
	if(app_stalkervod_video_busy())
	{
		app_stalkervod_update_group();
		GUI_SetInterface("flush", NULL);
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
	}
	else
	{
		app_net_video_show_popup_msg("Updating...", 0);
		create_stalkervod_thread(stalkervod_start_category_feed);
	}
}

static int32_t app_stalkervod_episode_pop_list_cb(int32_t ret_sel, unsigned short key)
{
	int small_page = 0;
	int index = 0;

    if(STBK_OK == key)
    {
        small_page = stalkervod_page_pos.cur_page%(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
        index = small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel;
        if((index<stalkervod_videolist->movie_num)&&(stalkervod_videolist->movielist[index].episode_num>0)
                &&(ret_sel>=0)&&(ret_sel<stalkervod_videolist->movielist[index].episode_num))
        {
            stalkervod_series_play_index = ret_sel;
            app_stalkervod_play(stalkervod_videolist->movielist[index].movie_name);
        }
    }

	return 0;
}

static void app_stalkervod_episode_list_select(void)
{
	PopList pop_list;
	int small_page = 0;
	int index = 0;

	small_page = stalkervod_page_pos.cur_page%(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
	index = small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel;
	if((index<stalkervod_videolist->movie_num)&&(stalkervod_videolist->movielist[index].episode_num>0))
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = "Episodes";
		pop_list.item_num = stalkervod_videolist->movielist[index].episode_num;
		pop_list.item_content = stalkervod_videolist->movielist[index].episodes;
		pop_list.sel = 0;
		pop_list.show_num = false;
		pop_list.mode = POP_MODE_UNBLOCK;
		pop_list.exit_cb = app_stalkervod_episode_pop_list_cb;
		poplist_create(&pop_list);

	}
}

static void app_stalkervod_ok_press_cb(void)
{
	int small_page = 0;

	if((s_video_obj == OBJ_VIDEO_GROUP)
		&&stalkervod_categorylist
		&&stalkervod_categorylist->categories
		&&(stalkervod_categorylist->category_num>0))
	{
		is_page_get_info = false;
		app_stalkervod_videorefresh_stop();
		app_net_video_clear_all_item();
		stalkervod_free_videodata();
		app_net_video_page_item_update(0, NULL);
		app_net_video_page_info_update(0, 0);
		stalkervod_group_pos.sel_group = stalkervod_group_pos.cur_group;
		stalkervod_page_pos.cur_big_page = 0;
		stalkervod_page_pos.page_video_num = 0;
		stalkervod_page_pos.cur_page = 0;
		stalkervod_page_pos.sel = 0;
		stalkervod_page_pos.total_page = 0;
		if(stalkervod_group_pos.cur_group == 0) //search
		{
			app_net_video_hide_popup_msg();
			_stalker_vod_search();
		}
		else
		{
			app_net_video_show_popup_msg("Updating...", 0);
			create_stalkervod_thread(stalkervod_start_category_video_feed);
		}
	}
	else if((s_video_obj == OBJ_VIDEO_PAGE)&&is_page_get_info
		&&stalkervod_videolist&&(stalkervod_videolist->movielist)
		&&(stalkervod_videolist->movie_num>stalkervod_page_pos.sel))
	{
		small_page = stalkervod_page_pos.cur_page%(stalkervod_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
		if((small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel)<stalkervod_videolist->movie_num)
		{
			app_net_video_hide_gif();
            app_net_video_disable_group_roll();
			app_net_video_pic_download_stop();
			app_stalkervod_videorefresh_stop();
			stalkervod_series_play_index = -1;
			if(stalkervod_videolist->movielist[small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel].episode_num>0)
			{
				app_stalkervod_episode_list_select();
			}
			else
			{
				app_stalkervod_play(stalkervod_videolist->movielist[small_page*NETVIDEO_MAX_PAGE_ITEM+stalkervod_page_pos.sel].movie_name);
			}
		}
	}
}

status_t app_create_stalkervod_menu(void)
{
	NetVideoMenuCb video_cb;
	app_netapps_app_init();
	app_stalkervod_data_timer_start();
	app_stalkervod_clean_menu_info();
	app_stalkervod_obj_change_cb(OBJ_VIDEO_GROUP);
	memset(&video_cb, 0, sizeof(NetVideoMenuCb));
	video_cb.obj_change         = app_stalkervod_obj_change_cb;
	video_cb.video_group_change = app_stalkervod_group_change_cb;
	video_cb.video_page_change  = app_stalkervod_page_change_cb;
	video_cb.video_sel_change   = app_stalkervod_video_change_cb;
	video_cb.video_exit         = app_stalkervod_menu_exit_cb;
	video_cb.ok_press           = app_stalkervod_ok_press_cb;
	video_cb.video_busy        = app_stalkervod_video_busy;
	app_net_video_create(STR_ID_NET_STALKER_VOD, STALKERVOD_LOGO, &video_cb, stalkervod_group_pos.cur_group);

	GUI_SetInterface("flush", NULL);
	app_net_video_page_info_update(0, 0);
	app_stalkervod_category_update();

	return GXCORE_SUCCESS;
}
#endif
