#include "app.h"
#ifdef STALKER_SERIES_SUPPORT
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

#define STR_ID_NET_STALKER_SERIES "Stalker Series"
#define STS_LOGO    "stalkerseries"

typedef void(* sts_thread_func)(void);

typedef enum
{
	ST_DATA_SERIES = 0,
	ST_DATA_SEASON,
	ST_DATA_MAX
}STDataType;

typedef struct sts_pagepos_s
{
	int cur_big_page;
	int page_video_num;
	int cur_page;
	int sel;
	int total_page;
}sts_pagepos_t;

typedef struct sts_grouppos_s
{
	int cur_group;
	int sel_group;
	int total_group;
}sts_grouppos_t;

typedef struct sts_nav_bak_s
{
	STDataType type;
	int cur_big_page;
	int page_video_num;
	int cur_page;
	int sel;
	int total_page;
}sts_nav_bak_t;

static NetVideoObj s_video_obj = OBJ_VIDEO_GROUP;
static sts_pagepos_t sts_page_pos = {0};
static sts_grouppos_t sts_group_pos = {0};
static sts_nav_bak_t sts_nav_show_bak = {0};
static NetVideoItem s_page_video_item[NETVIDEO_MAX_PAGE_ITEM];

static char **s_sts_group_title = NULL;;
static char *sts_playurl = NULL;
static char *sts_search_keyword = NULL;
static bool is_page_get_info = false;
static int is_sts_download = 0;
static int sts_video_depth = 0;
static STDataType sts_data_type = ST_DATA_SERIES;
static int sts_categorydata_finish = 0;
static int sts_videodata_finish = 0;
static unsigned int sts_data_handle = 0;
static unsigned int sts_geturl_handle = 0;
static int sts_episode_play_index = -1;

static gx_stalker_categorylist_t *sts_categorylist = NULL;
static gx_stalker_serieslist_t *sts_serieslist = NULL;
static gx_stalker_seasonlist_t *sts_seasonlist = NULL;
static event_list* sts_data_timer = NULL;
static gxapps_ret_t sts_ret_status = GXAPPS_SUCCESS;

static status_t app_sts_update_videodata(void);
static void app_sts_ok_press_cb(void);

static void app_sts_show_error_msg(void)
{
	if(sts_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_net_video_show_popup_msg("No results!", 1000);
	}
	else if(sts_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_net_video_show_popup_msg("Network error!", 1000);
	}
	else if(sts_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_net_video_show_popup_msg("Internal error!", 1000);
	}
	else if(sts_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_net_video_show_popup_msg("Unknown error!", 1000);
	}
}

static void sts_thread_body(void* arg)
{
	sts_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (sts_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_sts_thread(sts_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("sts", &thread_id, sts_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static int app_sts_video_busy(void)
{
	return is_sts_download;
}

static void app_sts_set_video_depth(int depth)
{
	sts_video_depth = depth;
}

static int app_sts_get_video_depth(void)
{
	return sts_video_depth;
}

static void app_sts_nav_show_bak(void)
{
	sts_nav_show_bak.type = sts_data_type;
	sts_nav_show_bak.cur_big_page = sts_page_pos.cur_big_page;
	sts_nav_show_bak.page_video_num = sts_page_pos.page_video_num;
	sts_nav_show_bak.cur_page = sts_page_pos.cur_page;
	sts_nav_show_bak.sel = sts_page_pos.sel;
	sts_nav_show_bak.total_page = sts_page_pos.total_page;
}

static void app_sts_nav_show_restore(void)
{
	sts_data_type = sts_nav_show_bak.type;
	sts_page_pos.cur_big_page = sts_nav_show_bak.cur_big_page;
	sts_page_pos.page_video_num = sts_nav_show_bak.page_video_num;
	sts_page_pos.cur_page = sts_nav_show_bak.cur_page;
	sts_page_pos.sel = sts_nav_show_bak.sel;
	sts_page_pos.total_page = sts_nav_show_bak.total_page;
}

static void app_sts_clean_menu_info(void)
{
	sts_page_pos.cur_big_page = 0;
	sts_page_pos.page_video_num = 0;
	sts_page_pos.cur_page = 0;
	sts_page_pos.sel = 0;
	sts_page_pos.total_page = 0;

	sts_group_pos.cur_group = 1;
	sts_group_pos.sel_group = 1;
	sts_group_pos.total_group = 0;
	is_page_get_info = false;
	sts_video_depth = 0;
	sts_data_type = ST_DATA_SERIES;
	sts_episode_play_index = -1;
	app_sts_set_video_depth(0);
	memset(&sts_nav_show_bak, 0, sizeof(sts_nav_bak_t));
}

static int app_sts_seriesdata_exits(void)
{
	int res = 0;
	if((sts_serieslist&&(sts_serieslist->series_num>0))
		&&(sts_page_pos.page_video_num>0)
		&&(sts_page_pos.cur_page>=(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM*(sts_page_pos.cur_big_page-1)))
		&&(sts_page_pos.cur_page<(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM*sts_page_pos.cur_big_page)))
	{
		res = 1;
	}
	return res;
}

static int app_sts_seasondata_exits(void)
{
	int res = 0;
	if((sts_seasonlist&&(sts_seasonlist->season_num>0))
		&&(sts_page_pos.page_video_num>0)
		&&(sts_page_pos.cur_page>=(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM*(sts_page_pos.cur_big_page-1)))
		&&(sts_page_pos.cur_page<(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM*sts_page_pos.cur_big_page)))
	{
		res = 1;
	}
	return res;
}

static status_t app_sts_update_group(void)
{
	int i= 0;
	int group_num = 0;

	APP_FREE(s_sts_group_title);
	if(sts_categorylist&&sts_categorylist->categories&&sts_categorylist->category_num>0)
	{
		group_num = 1 + sts_categorylist->category_num;
		s_sts_group_title = (char**)GxCore_Calloc(group_num, sizeof(char*));
		if(s_sts_group_title == NULL)
		{
			return GXCORE_ERROR;
		}

		s_sts_group_title[0] = STR_ID_SEARCH;
		for(i=0; i<sts_categorylist->category_num; i++)
		{
			s_sts_group_title[i+1] = sts_categorylist->categories[i].category_name;
		}
	}

	sts_group_pos.total_group = group_num;
	app_net_video_group_update(sts_group_pos.total_group, s_sts_group_title);

	return GXCORE_SUCCESS;
}

static void app_sts_categoryfresh_stop(void)
{
	sts_categorydata_finish = 0;
}

static void app_sts_categoryfresh_start(void)
{
	sts_categorydata_finish = 1;
}

static void app_sts_videorefresh_stop(void)
{
	sts_videodata_finish = 0;
}

static void app_sts_videorefresh_start(STDataType type)
{
	if((type>=ST_DATA_SERIES)&&(type<ST_DATA_MAX))
	{
		sts_data_type = type;
	}
	else
	{
		sts_data_type = ST_DATA_SERIES;
	}
	sts_videodata_finish = 1;
}

static int sts_categorydata_timeout(void)
{
	app_net_video_hide_popup_msg();
	app_sts_show_error_msg();
	app_sts_update_group();
	app_sts_categoryfresh_stop();
	app_sts_ok_press_cb();
	return 0;
}

static int sts_videodata_timeout(void)
{
	app_net_video_hide_popup_msg();
	app_sts_update_videodata();
	app_net_video_pic_download_start();
	app_sts_videorefresh_stop();
	return 0;
}

static void app_sts_data_timer_stop(void)
{
	if(sts_data_timer)
	{
		remove_timer(sts_data_timer);
		sts_data_timer = NULL;
	}
}

static int sts_data_timer_timeout(void *usrdata)
{
	if(sts_videodata_finish&&(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS))
	{
		sts_videodata_timeout();
	}
	else if(sts_categorydata_finish&&(GUI_CheckDialog("wnd_pop_book") != GXCORE_SUCCESS))
	{
		sts_categorydata_timeout();
	}
	return 0;
}

static void app_sts_data_timer_start(void)
{
	app_sts_data_timer_stop();
	sts_data_timer = create_timer(sts_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void sts_free_categorydata(void)
{
	if(sts_categorylist)
	{
		gx_stalker_free_categorylist(sts_categorylist);
		sts_categorylist = NULL;
	}
}

static void sts_free_seriesdata(void)
{
	if(sts_serieslist)
	{
		gx_stalker_free_serieslist(sts_serieslist);
		sts_serieslist = NULL;
	}
}

static void sts_free_seasondata(void)
{
	if(sts_seasonlist)
	{
		gx_stalker_free_seasonlist(sts_seasonlist);
		sts_seasonlist = NULL;
	}
}

static void sts_start_category_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_stalker_categorylist_t *data = NULL;

	is_sts_download = 1;
	sts_data_handle++;
	handle = sts_data_handle;
	ret = gx_stalker_get_series_categories(&data);
	if(handle == sts_data_handle)
	{
		sts_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			sts_categorylist = data;
		}
		app_sts_categoryfresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_categorylist(data);
		}
	}
	is_sts_download = 0;
}

static void sts_start_category_series_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_stalker_serieslist_t *data = NULL;

	is_sts_download = 1;
	sts_data_handle++;
	handle = sts_data_handle;
	if(sts_page_pos.page_video_num==0)
	{
		sts_page_pos.cur_big_page = 1;
	}
	else
	{
		sts_page_pos.cur_big_page = sts_page_pos.cur_page/(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM)+1;
	}
	ret = gx_stalker_get_category_series(sts_categorylist->categories[sts_group_pos.sel_group-1].category_id, sts_page_pos.cur_big_page, &data);
	if(handle == sts_data_handle)
	{
		sts_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			sts_serieslist = data;
			if(sts_page_pos.page_video_num==0)
			{
				sts_page_pos.page_video_num = (sts_serieslist->series_num/NETVIDEO_MAX_PAGE_ITEM)*NETVIDEO_MAX_PAGE_ITEM;
				if(sts_serieslist->series_num% NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					sts_page_pos.page_video_num += NETVIDEO_MAX_PAGE_ITEM;
				}
				sts_page_pos.total_page = (sts_serieslist->series_total_num/NETVIDEO_MAX_PAGE_ITEM);
				if(sts_serieslist->series_total_num% NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					sts_page_pos.total_page++;
				}
			}
			is_page_get_info = true;
		}
		app_sts_videorefresh_start(ST_DATA_SERIES);
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_serieslist(data);
		}
	}
	is_sts_download = 0;
}

static void sts_start_search_series_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_stalker_serieslist_t *data = NULL;

	is_sts_download = 1;
	sts_data_handle++;
	handle = sts_data_handle;
	if(sts_page_pos.page_video_num==0)
	{
		sts_page_pos.cur_big_page = 1;
	}
	else
	{
		sts_page_pos.cur_big_page = sts_page_pos.cur_page/(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM)+1;
	}
	ret = gx_stalker_search_series(sts_search_keyword, sts_page_pos.cur_big_page, &data);
	if(handle == sts_data_handle)
	{
		sts_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			sts_serieslist = data;
			if(sts_page_pos.page_video_num==0)
			{
				sts_page_pos.page_video_num = (sts_serieslist->series_num/NETVIDEO_MAX_PAGE_ITEM)*NETVIDEO_MAX_PAGE_ITEM;
				if(sts_serieslist->series_num% NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					sts_page_pos.page_video_num += NETVIDEO_MAX_PAGE_ITEM;
				}
				sts_page_pos.total_page = (sts_serieslist->series_total_num/NETVIDEO_MAX_PAGE_ITEM);
				if(sts_serieslist->series_total_num% NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					sts_page_pos.total_page++;
				}
			}
			is_page_get_info = true;
		}
		app_sts_videorefresh_start(ST_DATA_SERIES);
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_serieslist(data);
		}
	}
	is_sts_download = 0;
}

static void sts_start_series_seasons_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	int small_page = 0;
	int index = 0;
	gx_stalker_seasonlist_t *data = NULL;

	is_sts_download = 1;
	sts_data_handle++;
	handle = sts_data_handle;
	if(sts_page_pos.page_video_num==0)
	{
		sts_page_pos.cur_big_page = 1;
	}
	else
	{
		sts_page_pos.cur_big_page = sts_page_pos.cur_page/(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM)+1;
	}
	if(sts_serieslist&&(sts_serieslist->series_num>0)&&(sts_nav_show_bak.sel<sts_serieslist->series_num))
	{
		small_page = sts_nav_show_bak.cur_page%(sts_nav_show_bak.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
		index = small_page*NETVIDEO_MAX_PAGE_ITEM+sts_nav_show_bak.sel;
		if(index<sts_serieslist->series_num)
		{
			ret = gx_stalker_get_series_seasons(sts_serieslist->series[index].category_id, sts_serieslist->series[index].series_id, sts_page_pos.cur_big_page, &data);
		}
	}
	if(handle == sts_data_handle)
	{
		sts_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			sts_seasonlist = data;
			if(sts_page_pos.page_video_num==0)
			{
				sts_page_pos.page_video_num = (sts_seasonlist->season_num/NETVIDEO_MAX_PAGE_ITEM)*NETVIDEO_MAX_PAGE_ITEM;
				if(sts_seasonlist->season_num% NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					sts_page_pos.page_video_num += NETVIDEO_MAX_PAGE_ITEM;
				}
				sts_page_pos.total_page = (sts_seasonlist->season_total_num/NETVIDEO_MAX_PAGE_ITEM);
				if(sts_seasonlist->season_total_num% NETVIDEO_MAX_PAGE_ITEM > 0)
				{
					sts_page_pos.total_page++;
				}
			}
			is_page_get_info = true;
		}
		app_sts_videorefresh_start(ST_DATA_SEASON);
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_seasonlist(data);
		}
	}
	is_sts_download = 0;
}

void _sts_search_full_keyboard_proc(PopKeyboard *data)
{
	 if(data->in_ret == POP_VAL_CANCEL)
	 {
		return;
	 }

	if((data->out_name == NULL)||strlen(data->out_name)==0)
	{
		return;
	}
	APP_FREE(sts_search_keyword);
	sts_search_keyword = GxCore_Strdup(data->out_name);
	if(app_sts_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
	}
	else
	{
		app_net_video_show_popup_msg("Updating...", 0);
		create_sts_thread(sts_start_search_series_feed);
	}
}

static void _stalker_series_search(void)
{
    static PopKeyboard keyboard;

    app_keyboard_save_cb(app_input_save_space_cb);
    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = NULL;
    keyboard.max_num = 127;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = NULL;
    keyboard.release_cb = _sts_search_full_keyboard_proc;
    keyboard.usr_data   = NULL;
    keyboard.pos.x = 500;
    multi_language_keyboard_create(&keyboard);
}

static void app_sts_category_update(void)
{
	app_sts_categoryfresh_stop();
	sts_free_categorydata();
	app_net_video_show_popup_msg("Updating...", 0);
	create_sts_thread(sts_start_category_feed);
}

static status_t app_sts_update_videodata(void)
{
	int i = 0;
	int curPageProgNum = 0;
	int small_page = 0;

	if(is_page_get_info&&(sts_data_type == ST_DATA_SERIES)&&app_sts_seriesdata_exits())
	{
		small_page = sts_page_pos.cur_page%(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
		if(sts_serieslist->series_num>=((small_page+1)*NETVIDEO_MAX_PAGE_ITEM))
		{
			curPageProgNum = NETVIDEO_MAX_PAGE_ITEM;
		}
		else
		{
			curPageProgNum = sts_serieslist->series_num%NETVIDEO_MAX_PAGE_ITEM;
		}

		for(i = 0; i < curPageProgNum; i++)
		{
			s_page_video_item[i].video_duration = NULL;
			s_page_video_item[i].video_title = sts_serieslist->series[small_page*NETVIDEO_MAX_PAGE_ITEM+i].name;
			s_page_video_item[i].video_pic = sts_serieslist->series[small_page*NETVIDEO_MAX_PAGE_ITEM+i].pic_url;
			s_page_video_item[i].video_author  = sts_serieslist->series[small_page*NETVIDEO_MAX_PAGE_ITEM+i].genre;
			s_page_video_item[i].video_viewcnt = NULL;
		}

		app_net_video_page_item_update(curPageProgNum, s_page_video_item);
		app_net_video_page_info_update(sts_page_pos.total_page, sts_page_pos.cur_page);
        if(OBJ_VIDEO_PAGE == s_video_obj)
            app_net_video_page_item_set_focus(sts_page_pos.sel);
	}
	else if(is_page_get_info&&(sts_data_type == ST_DATA_SEASON)&&app_sts_seasondata_exits())
	{
		small_page = sts_page_pos.cur_page%(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
		if(sts_seasonlist->season_num>=((small_page+1)*NETVIDEO_MAX_PAGE_ITEM))
		{
			curPageProgNum = NETVIDEO_MAX_PAGE_ITEM;
		}
		else
		{
			curPageProgNum = sts_seasonlist->season_num%NETVIDEO_MAX_PAGE_ITEM;
		}

		for(i = 0; i < curPageProgNum; i++)
		{
			s_page_video_item[i].video_duration = NULL;
			s_page_video_item[i].video_title = sts_seasonlist->seasons[small_page*NETVIDEO_MAX_PAGE_ITEM+i].name;
			s_page_video_item[i].video_pic = sts_seasonlist->seasons[small_page*NETVIDEO_MAX_PAGE_ITEM+i].pic_url;
			s_page_video_item[i].video_author  = sts_seasonlist->seasons[small_page*NETVIDEO_MAX_PAGE_ITEM+i].genre;
			s_page_video_item[i].video_viewcnt = NULL;
		}

		app_net_video_page_item_update(curPageProgNum, s_page_video_item);
		app_net_video_page_info_update(sts_page_pos.total_page, sts_page_pos.cur_page);
        app_net_video_page_item_set_focus(sts_page_pos.sel);
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
		app_sts_show_error_msg();
	}
	return GXCORE_SUCCESS;
}

static void app_sts_play_exit_ctrl(void)
{
	sts_geturl_handle++;
}

static void app_sts_play_restart_ctrl(uint64_t cur_time)
{
	app_net_video_start_play(sts_playurl, PLAY_STATE_LOAD, cur_time, false);
}

static void app_sts_play_get_videourl(int source_index)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	int small_page = 0;
	int index = 0;
	char *url = NULL;

	sts_geturl_handle++;
	handle = sts_geturl_handle;
	APP_FREE(sts_playurl);
	if(sts_seasonlist&&(sts_seasonlist->season_num>0)&&(sts_page_pos.sel<sts_seasonlist->season_num))
	{
		small_page = sts_page_pos.cur_page%(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
		index = small_page*NETVIDEO_MAX_PAGE_ITEM+sts_page_pos.sel;
		if(index<sts_seasonlist->season_num)
		{
			if((sts_episode_play_index>=0)&&(sts_seasonlist->seasons[index].episode_num>0)&&(sts_episode_play_index<sts_seasonlist->seasons[index].episode_num))
			{
				ret = gx_stalker_get_episode_playurl(sts_seasonlist->seasons[index].play_cmd, sts_seasonlist->seasons[index].episodes[sts_episode_play_index], &url);
			}
		}
	}

	if(handle != sts_geturl_handle)
	{
		printf("player has exited!\n");
		if(ret == GXAPPS_SUCCESS)
		{
			gx_stalker_free_episode_playurl(url);
		}
		return;
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			printf("video url:%s\n", url);
			sts_playurl = GxCore_Strdup(url);
			gx_stalker_free_episode_playurl(url);
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

	if((ret == GXAPPS_SUCCESS)&&sts_playurl)
	{
		app_net_video_start_play(sts_playurl, PLAY_STATE_LOAD, 0, false);
	}
	else
	{
		app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}

}

static status_t app_sts_play_video(char *prog_name)
{
	NetVideoPlayOps play_ops;
	memset(&play_ops, 0, sizeof(NetVideoPlayOps));

	play_ops.get_url             = app_sts_play_get_videourl;
	play_ops.netvideo_title         = prog_name;
	play_ops.source_num = 1;
	play_ops.play_ctrl.play_ok   = NULL;
	play_ops.play_ctrl.play_exit = app_sts_play_exit_ctrl;
	play_ops.play_ctrl.play_prev = NULL;
	play_ops.play_ctrl.play_next = NULL;
	play_ops.play_ctrl.play_restart = app_sts_play_restart_ctrl;

	return app_net_video_play(&play_ops);
}

static void app_sts_obj_change_cb(NetVideoObj obj)
{
	s_video_obj = obj;
}

static void app_sts_group_change_cb(int group_sel)
{
	sts_group_pos.cur_group = group_sel;
}

static void app_sts_page_change_cb(int page_num)
{
	if((page_num >= 0) && (page_num < sts_page_pos.total_page))
	{
		is_page_get_info = false;
		app_net_video_pic_download_stop();
		app_sts_videorefresh_stop();
		app_net_video_clear_all_item();
		sts_page_pos.cur_page = page_num;
		if(app_sts_video_busy())
		{
			app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
		}
		else
		{
			if(sts_data_type == ST_DATA_SEASON)
			{
				if(app_sts_seasondata_exits())
				{
					is_page_get_info = true;
					app_sts_videorefresh_start(ST_DATA_SEASON);
				}
				else
				{
					sts_free_seasondata();
					app_net_video_show_popup_msg("Updating...", 0);
					create_sts_thread(sts_start_series_seasons_feed);
				}
			}
			else
			{
				if(app_sts_seriesdata_exits())
				{
					is_page_get_info = true;
					app_sts_videorefresh_start(ST_DATA_SERIES);
				}
				else
				{
					sts_free_seriesdata();
					app_net_video_show_popup_msg("Updating...", 0);
					if(sts_group_pos.sel_group == 0) //search
					{
						create_sts_thread(sts_start_search_series_feed);
					}
					else
					{
						create_sts_thread(sts_start_category_series_feed);
					}
				}
			}
		}
	}
}

static void app_sts_video_change_cb(int video_sel)
{
	if(sts_page_pos.sel != video_sel)
	{
		sts_page_pos.sel = video_sel;
	}
}

static void app_sts_menu_exit_cb(void)
{
	if(app_sts_get_video_depth()>0)
	{
		app_sts_set_video_depth(0);
		app_sts_nav_show_restore();
		app_net_video_pic_download_stop();
		app_sts_videorefresh_stop();
		app_net_video_clear_all_item();
		app_net_video_page_info_update(0, 0);
		app_sts_videorefresh_start(sts_data_type);
	}
	else
	{
		sts_data_handle++;
		app_sts_categoryfresh_stop();
		app_sts_videorefresh_stop();
		app_sts_data_timer_stop();
		APP_FREE(s_sts_group_title);
		APP_FREE(sts_playurl);
		APP_FREE(sts_search_keyword);
		app_sts_clean_menu_info();
		sts_free_categorydata();
		sts_free_seriesdata();
		sts_free_seasondata();
		app_netapps_app_destroy();
	}
}

static void app_sts_series_season_update(void)
{
	app_sts_nav_show_bak();
	app_sts_set_video_depth(1);
	app_net_video_clear_all_item();
    app_net_video_page_item_update(0, NULL);
	app_net_video_page_info_update(0, 0);
	sts_page_pos.cur_big_page = 0;
	sts_page_pos.page_video_num = 0;
	sts_page_pos.cur_page = 0;
	sts_page_pos.sel = 0;
	sts_page_pos.total_page = 0;
	sts_free_seasondata();
	app_net_video_show_popup_msg("Updating...", 0);
	create_sts_thread(sts_start_series_seasons_feed);
}

static int32_t app_sts_episode_pop_list_cb(int32_t ret_sel, unsigned short key)
{
	int small_page = 0;
	int index = 0;

    if(STBK_OK == key)
    {
        small_page = sts_page_pos.cur_page%(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
        index = small_page*NETVIDEO_MAX_PAGE_ITEM+sts_page_pos.sel;
        if((index<sts_seasonlist->season_num)&&(sts_seasonlist->seasons[index].episode_num>0)
                &&(ret_sel>=0)&&(ret_sel<sts_seasonlist->seasons[index].episode_num))
        {
            sts_episode_play_index = ret_sel;
            app_sts_play_video(sts_seasonlist->seasons[index].episodes[ret_sel]);
        }
    }

	return 0;
}

static void app_sts_episode_list_select(void)
{
	PopList pop_list;
	int small_page = 0;
	int index = 0;

	small_page = sts_page_pos.cur_page%(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
	index = small_page*NETVIDEO_MAX_PAGE_ITEM+sts_page_pos.sel;
	if((index<sts_seasonlist->season_num)&&(sts_seasonlist->seasons[index].episode_num>0))
	{
		if(sts_seasonlist->seasons[index].episode_num == 1)
		{
			app_sts_episode_pop_list_cb(0, STBK_OK);
		}
		else
		{
			memset(&pop_list, 0, sizeof(PopList));
			pop_list.title = "Episodes";
			pop_list.item_num = sts_seasonlist->seasons[index].episode_num;
			pop_list.item_content = sts_seasonlist->seasons[index].episodes;
			pop_list.sel = 0;
			pop_list.show_num = false;
			pop_list.mode = POP_MODE_UNBLOCK;
			pop_list.exit_cb = app_sts_episode_pop_list_cb;
			poplist_create(&pop_list);
		}
	}
}

static void app_sts_ok_press_cb(void)
{
	int small_page = 0;
	int index = 0;

	if(app_sts_video_busy())
	{
		app_net_video_show_popup_msg("System Busy, Please Wait!", 1000);
		return;
	}
	if((s_video_obj == OBJ_VIDEO_GROUP)
		&&sts_categorylist
		&&sts_categorylist->categories
		&&(sts_categorylist->category_num>0))
	{
		is_page_get_info = false;
		app_net_video_pic_download_stop();
		app_sts_videorefresh_stop();
		app_net_video_clear_all_item();
		sts_free_seriesdata();
		sts_free_seasondata();
		app_net_video_page_item_update(0, NULL);
		app_net_video_page_info_update(0, 0);
		sts_group_pos.sel_group = sts_group_pos.cur_group;
		sts_page_pos.cur_big_page = 0;
		sts_page_pos.page_video_num = 0;
		sts_page_pos.cur_page = 0;
		sts_page_pos.sel = 0;
		sts_page_pos.total_page = 0;
		app_sts_set_video_depth(0);
		if(sts_group_pos.cur_group == 0) //search
		{
			app_net_video_hide_popup_msg();
			_stalker_series_search();
		}
		else
		{
			app_net_video_show_popup_msg("Updating...", 0);
			create_sts_thread(sts_start_category_series_feed);
		}
	}
	else if((s_video_obj == OBJ_VIDEO_PAGE)&&is_page_get_info)
	{
		app_net_video_hide_gif();
		app_net_video_pic_download_stop();
		app_sts_videorefresh_stop();
		if((sts_data_type == ST_DATA_SERIES)&&sts_serieslist&&(sts_serieslist->series_num>0)
			&&(sts_page_pos.sel<sts_serieslist->series_num))
		{
			app_sts_series_season_update();
		}
		else if((sts_data_type == ST_DATA_SEASON)&&sts_seasonlist&&(sts_seasonlist->season_num>0)
			&&(sts_page_pos.sel<sts_seasonlist->season_num))
		{
			small_page = sts_page_pos.cur_page%(sts_page_pos.page_video_num/NETVIDEO_MAX_PAGE_ITEM);
			index = small_page*NETVIDEO_MAX_PAGE_ITEM+sts_page_pos.sel;
			if(index<sts_seasonlist->season_num)
			{
				app_net_video_disable_group_roll();
				app_sts_episode_list_select();
			}
		}
	}
}

status_t app_create_stalkerseries_menu(void)
{
	NetVideoMenuCb video_cb;
	app_netapps_app_init();
	app_sts_data_timer_start();
	app_sts_clean_menu_info();
	app_sts_obj_change_cb(OBJ_VIDEO_GROUP);
	memset(&video_cb, 0, sizeof(NetVideoMenuCb));
	video_cb.obj_change         = app_sts_obj_change_cb;
	video_cb.video_group_change = app_sts_group_change_cb;
	video_cb.video_page_change  = app_sts_page_change_cb;
	video_cb.video_sel_change   = app_sts_video_change_cb;
	video_cb.video_exit         = app_sts_menu_exit_cb;
	video_cb.ok_press           = app_sts_ok_press_cb;
	video_cb.video_busy        = app_sts_video_busy;
	video_cb.get_video_depth       = app_sts_get_video_depth;
	video_cb.set_video_depth       = app_sts_set_video_depth;

	app_net_video_create(STR_ID_NET_STALKER_SERIES, STS_LOGO, &video_cb, sts_group_pos.cur_group);

	GUI_SetInterface("flush", NULL);
	app_net_video_page_info_update(0, 0);
	app_sts_category_update();

	return GXCORE_SUCCESS;
}

#endif

