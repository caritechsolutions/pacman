#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#ifdef XTREAM_SUPPORT
#include <gx_xtream.h>
#include <app_iptv_list_api.h>
#include "iptv/app_iptv_list_service.h"
#include "iptv/app_iptv.h"
#include "app_netapps_common.h"

#define XTREAM_FAVLIST_PATH "/home/gx/xtream_fav.xml"
#define XTREAM_URL_KEY    "netapps>xtream_url"
#define XTREAM_USER_KEY    "netapps>xtream_user"
#define XTREAM_PWD_KEY    "netapps>xtream_pwd"
#define XTREAM_INVALID_URL    "http://127.0.0.1"
#define XTREAM_DEFAULT_URL    XTREAM_INVALID_URL
#define XTREAM_DEFAULT_USER    "test"
#define XTREAM_DEFAULT_PWD    "test"
#define XTREAM_STREAMID_LEN (32)
#define XTREAM_IPTV_PIC "default"
#define XTREAM_VOD_PIC "default"
#define XTREAM_SERIES_PIC "default"
#define XTREAM_SETTING_PIC "default"
#define XTREAM_ITEM_NUM 4
#define XTREAM_PLAYGROUP        "xtream>play_group"
#define XTREAM_PLAYURL          "xtream>play_url"
#define XTREAM_PLAYNAME         "xtream>play_name"

#define XTREAM_MAX_CHANNEL_NUM 1000
#define XTREAM_DAY_TO_SEC (24*60*60)
#define XTREAM_LOGIN_RETYR_TIMES (10)


#define STR_ID_NET_XTREAM "Xtream"
#define STR_ID_NET_XTREAM_SETTING "Xtream Setting"
#define STR_ID_NET_XTREAM_IPTV "Xtream IPTV"
#define STR_ID_NET_XTREAM_VOD "Xtream VOD"
#define STR_ID_NET_XTREAM_SERIES "Xtream Series"




static int xtream_login_status = IPTV_LOGIN_PLEASE;
static int s_xtream_is_download = 0;
static int xtream_autologin_running = 0;
static int xtream_autologin_count = 0;
#ifdef IPTV_EPG_SUPPORT
static unsigned int xtream_epg_handle = 0;
static gx_xtream_epglist_t *xtream_full_epglist = NULL;
#endif

static void app_xtream_auto_login_func(void);

extern status_t app_create_xtreamvod_menu(void);
extern status_t app_create_xtreamseries_menu(void);

typedef void(* xtream_thread_func) (void);

static void xtream_thread_body(void* arg)
{
	xtream_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (xtream_thread_func)arg;
	if(func)
	{
		func();
	}
}

static void create_xtream_thread(xtream_thread_func func)
{
	static int thread_id = 0;
    uint32_t size = (func == app_xtream_auto_login_func) ? 12*1024 : 64*1024;
	GxCore_ThreadCreate("xtream", &thread_id, xtream_thread_body,
		(void *)func, size, GXOS_DEFAULT_PRIORITY);
}

static char *app_xtream_setting_url_get(void)
{
	char *url  = GxCore_Calloc(IPTV_PARAM_LEN, 1);
	GxBus_ConfigGet(XTREAM_URL_KEY, url, IPTV_PARAM_LEN,  XTREAM_DEFAULT_URL);
	return url;
}

static char *app_xtream_setting_user_get(void)
{
	char *user  = GxCore_Calloc(IPTV_PARAM_LEN, 1);
	GxBus_ConfigGet(XTREAM_USER_KEY, user, IPTV_PARAM_LEN,  XTREAM_DEFAULT_USER);
	return user;
}

static char *app_xtream_setting_pwd_get(void)
{
	char *pwd  = GxCore_Calloc(IPTV_PARAM_LEN, 1);
	GxBus_ConfigGet(XTREAM_PWD_KEY, pwd, IPTV_PARAM_LEN,  XTREAM_DEFAULT_PWD);
	return pwd;
}

static int app_is_xtreamiptv_busy(void)
{
	return s_xtream_is_download;
}

int app_xtream_set_logout(void)
{
	if (xtream_autologin_running > 0)
	{
		xtream_autologin_running = 2;
	}
	if (xtream_login_status == IPTV_LOGIN_SUCCESS)
	{
		if (IF_STATE_CONNECTED != app_network_get_cur_dev_and_state(NULL))
		{
			xtream_login_status = IPTV_LOGIN_FAILED;
			app_iptv_show_status(IPTV_SERVER_XTREAM);
		}
	}
	return 0;
}


static int app_xtream_check_account(void)
{
	int valid = 0;
	char *url = NULL;

	url = app_xtream_setting_url_get();
	if(strcmp(url, XTREAM_INVALID_URL) != 0)
	{
		valid = 1;
	}
	APP_FREE(url);

	return valid;
}

static void _xtream_login(void)
{
	gxapps_ret_t retcode = GXAPPS_SUCCESS;
	char *server_url = NULL;
	char *user = NULL;
	char *pwd = NULL;

	s_xtream_is_download = 1;
	xtream_login_status = IPTV_LOGIN_RUNNING;
	app_iptv_show_status(IPTV_SERVER_XTREAM);
	gx_xtream_logout();

	server_url = app_xtream_setting_url_get();
	user = app_xtream_setting_user_get();
	pwd = app_xtream_setting_pwd_get();
	retcode = gx_xtream_login(server_url, user, pwd);
	APP_FREE(server_url);
	APP_FREE(user);
	APP_FREE(pwd);
	if(retcode == GXAPPS_SUCCESS)
	{
		xtream_login_status = IPTV_LOGIN_SUCCESS;
	}
	else
	{
		xtream_login_status = IPTV_LOGIN_FAILED;
	}
	app_iptv_show_status(IPTV_SERVER_XTREAM);

	s_xtream_is_download = 0;
}

static void app_xtream_auto_login_func(void)
{
	int count = 0;
	while(xtream_autologin_running>0)
	{
		if(xtream_autologin_running == 1)
		{
			if(app_is_xtreamiptv_busy() || (xtream_login_status == IPTV_LOGIN_RUNNING))
			{
				GxCore_ThreadDelay(100);
			}
			else
			{
				_xtream_login();
				if((xtream_login_status == IPTV_LOGIN_SUCCESS))
				{
					xtream_autologin_running = 2;
					xtream_autologin_count = 0;
				}
				else
				{
					xtream_autologin_count++;
					if(xtream_autologin_count>=XTREAM_LOGIN_RETYR_TIMES)
					{
						xtream_autologin_running = 2;
					}
					else
					{
					count = 0;
					while((count++<30)&&(xtream_autologin_running == 1))
					{
						GxCore_ThreadDelay(100);
						}
					}
				}
			}
		}
		else
		{
			GxCore_ThreadDelay(100);
		}
	}
}

void app_xtream_auto_login_start(void)
{
	xtream_autologin_count = 0;
	if(app_xtream_check_account()>0)
	{
		if(xtream_autologin_running == 0)
		{
			xtream_autologin_running = 1;
			create_xtream_thread(app_xtream_auto_login_func);
		}
		else
		{
			xtream_autologin_running = 1;
		}
	}
}

void app_xtream_auto_login_stop(void)
{
	xtream_autologin_count = 0;
	if(xtream_autologin_running>0)
	{
		xtream_autologin_running = 2;
	}
}

static int _app_xtream_login_popdlg_exit_cb(PopDlgRet ret)
{
	if(ret == POP_VAL_OK)
	{
		app_iptv_tip_show(STR_ID_LOGIN_WAIT);
		create_xtream_thread(_xtream_login);
	}
	return 0 ;
}

void app_xtream_login(void)
{
	if(app_is_xtreamiptv_busy() || (xtream_login_status == IPTV_LOGIN_RUNNING))
	{
		app_iptv_tip_show("System Busy, Please Wait!");
	}
	else
	{
		app_xtream_auto_login_stop();
		if((xtream_login_status == IPTV_LOGIN_SUCCESS))
		{
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_YES_NO;
			pop.mode = POP_MODE_UNBLOCK;
			pop.str = STR_ID_RELOGIN;
			pop.exit_cb = _app_xtream_login_popdlg_exit_cb;
			popdlg_create(&pop);
		}
		else
		{
			_app_xtream_login_popdlg_exit_cb(POP_VAL_OK);
		}
	}
}

static char * app_xtream_get_categories_name(gx_xtream_categorylist_t *pCategorylist, int category_id)
{
	char *name = NULL;
	int i = 0;

	if(pCategorylist)
	{
		for(i=0;i<pCategorylist->category_num;i++)
		{
			if(pCategorylist->categories[i].category_id == category_id)
			{
				name = pCategorylist->categories[i].category_name;
				break;
			}
		}
	}

	return name;
}

static int app_xtream_need_limit_channel(void)
{
	int ret = 0;

	if(app_netapps_get_memory_size()<=64*1024*1024)
	{
		ret = 1;
	}

	return ret;
}

static void app_xtreamiptv_get_list(void)
{
	gxapps_ret_t ret = GXAPPS_SUCCESS;
	gx_xtream_livedata_t *livedata = NULL;
	IptvListClass *list = NULL;
	AppMsg_IptvList iptv_list;
	char *key=NULL;
	char *value=NULL;
	char *type=NULL;
	int i=0;
	char * categories_name =NULL;
	s_xtream_is_download = 1;
	if(xtream_login_status == IPTV_LOGIN_SUCCESS)
	{
		if(app_xtream_need_limit_channel())
		{
			gx_xtream_set_channel_max(XTREAM_MAX_CHANNEL_NUM);
		}
		ret = gx_xtream_get_channels(&livedata);
		if((ret == GXAPPS_SUCCESS)&&livedata&&livedata->channellist&&livedata->categorylist)
		{
			list = _iptv_list_new();
			for(i=0; i<livedata->channellist->channel_num; i++)
			{
                if(list->iptv_item_num == LIST_MAX_ITEM_NUM)
                    break;
				key = NULL;
				value = NULL;
				type = NULL;
				key = GxCore_Strdup(livedata->channellist->channels[i].channel_name);
				value = (char *)GxCore_Calloc(1, XTREAM_STREAMID_LEN);
				snprintf(value, XTREAM_STREAMID_LEN, "%d", livedata->channellist->channels[i].stream_id);

				categories_name = app_xtream_get_categories_name(livedata->categorylist, livedata->channellist->channels[i].category_id);
				if(categories_name)
					type = GxCore_Strdup(categories_name);

				if(key&&value)
				{
					list->iptv_item_list[list->iptv_item_num].iptv_key = key;
					list->iptv_item_list[list->iptv_item_num].iptv_value = value;
					list->iptv_item_list[list->iptv_item_num].iptv_type = type;
					list->iptv_item_num++;
				}
				else
				{
					APP_FREE(key);
					APP_FREE(value);
					APP_FREE(type);
				}
			}
		}

		if(livedata)
		{
			gx_xtream_free_channellist(livedata);
			livedata = NULL;
		}
	}
	s_xtream_is_download = 0;
	memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.retcode = ret;
	iptv_list.iptv_source = IPTV_LIST_REMOTE;
	iptv_list.iptv_server = IPTV_SERVER_XTREAM;
	iptv_list.iptv_list = list;
	app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);

}

status_t app_xtreamiptv_service_get_list(void)
{
	if(app_is_xtreamiptv_busy())
	{
		return GXCORE_ERROR;
	}
	create_xtream_thread(app_xtreamiptv_get_list);

	return GXCORE_SUCCESS;
}

int app_xtream_msg_proc(GxMessage *msg)
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
					app_xtream_auto_login_start();
				}
				else
				{
					app_xtream_set_logout();
				}
				break;
			}
		default:
			break;
	}
	return EVENT_TRANSFER_STOP;
}

static void save_xtream_playing_channel(IPTVChannel *channel)
{
	GxBus_ConfigSet(XTREAM_PLAYGROUP, channel->group);
	GxBus_ConfigSet(XTREAM_PLAYURL, channel->url);
	GxBus_ConfigSet(XTREAM_PLAYNAME, channel->name);
}

static IPTVChannel* get_xtream_save_channel(void)
{
	IPTVChannel *channel = (IPTVChannel*)GxCore_Malloc(sizeof(IPTVChannel));
	channel->group = (char*)GxCore_Malloc(MAX_IPTV_GROUP_LENGTH);
	channel->url = (char*)GxCore_Malloc(MAX_IPTV_URL_LENGTH);
	channel->name = (char*)GxCore_Malloc(MAX_IPTV_NAME_LENGTH);
	GxBus_ConfigGet(XTREAM_PLAYGROUP, channel->group, MAX_IPTV_GROUP_LENGTH, "");
	GxBus_ConfigGet(XTREAM_PLAYURL, channel->url, MAX_IPTV_URL_LENGTH, "");
	GxBus_ConfigGet(XTREAM_PLAYNAME, channel->name, MAX_IPTV_NAME_LENGTH, "");
	return channel;
}

#ifdef IPTV_FAV_SUPPORT
static gx_list_t* get_xtream_get_fav_list(void)
{
	gx_list_t *list = NULL;
	if(GXCORE_FILE_EXIST == GxCore_FileExists(XTREAM_FAVLIST_PATH))
	{
		list = gx_get_list_from_file(XTREAM_FAVLIST_PATH);
	}
	return list;
}
static void get_xtream_save_fav_list(gx_list_t* list)
{
	if(list)
	{
		gx_list_save_to_file(list, XTREAM_FAVLIST_PATH);
	}
}
#endif

static char* app_xtream_play_get_url(char *stream_id)
{
	char *url = NULL;
	char *ret = NULL;
	if(GXAPPS_SUCCESS == gx_xtream_get_live_playurl(atoi(stream_id), &url))
	{
		if(url)
		{
			ret = GxCore_Strdup(url);
			gx_xtream_free_live_playurl(url);
		}
	}
	return ret;
}

#ifdef IPTV_EPG_SUPPORT
static EpgEventList* app_xtream_get_short_epg(char *stream_id, int num)
{
	gx_xtream_epglist_t *epglist = NULL;
	EpgEventList *retlist = NULL;
	int i = 0;
	if(GXAPPS_SUCCESS == gx_xtream_get_short_epg(atoi(stream_id), num, &epglist))
	{
		if(epglist)
		{
			if(epglist->epg_num>0)
			{
				retlist = (EpgEventList *)GxCore_Malloc(sizeof(EpgEventList));
				memset(retlist, 0, sizeof(EpgEventList));
				retlist->epg_item = (EpgEventItem *)GxCore_Malloc(sizeof(EpgEventItem)*epglist->epg_num);
				memset(retlist->epg_item, 0, sizeof(EpgEventItem)*epglist->epg_num);
				for(i=0; (i<epglist->epg_num)&&(i<num); i++)
				{
					if(epglist->epgs[i].start_timestamp)
					{
						retlist->epg_item[i].start_time = app_iptv_get_time_str(epglist->epgs[i].start_timestamp);
					}
					if(epglist->epgs[i].end_timestamp)
					{
						retlist->epg_item[i].end_time = app_iptv_get_time_str(epglist->epgs[i].end_timestamp);
					}
					if(epglist->epgs[i].title)
					{
						retlist->epg_item[i].title = GxCore_Strdup(epglist->epgs[i].title);
					}
					if(epglist->epgs[i].description)
					{
						retlist->epg_item[i].description = GxCore_Strdup(epglist->epgs[i].description);
					}
				}
				retlist->epg_num = epglist->epg_num;
			}
			gx_xtream_free_epglist(epglist);
		}
	}
	return retlist;
}

static void app_xtream_free_full_epglist(void)
{
	if(xtream_full_epglist)
	{
		gx_xtream_free_epglist(xtream_full_epglist);
		xtream_full_epglist = NULL;
	}
}

static int app_xtream_is_day_exist(EpgDayList *daylist, char *timestamp)
{
	int exits = 0;
	int i = 0;
	int time1 = 0;
	int time2 = 0;
	if(timestamp&&daylist&&(daylist->day_num>0))
	{
		time1 = atoi(timestamp);
		for(i=0; i<daylist->day_num; i++)
		{
			time2 = atoi(daylist->day_item[i].day_id);
			if((time2>=time1)&&(time2<(time1+XTREAM_DAY_TO_SEC)))
			{
				exits = 1;
				break;
			}
		}
	}
	return exits;
}

static void app_xtream_add_day_item(char *start_timestamp, int epg_num, EpgDayList **daylist)
{
	EpgDayList *xtream_daylist = NULL;
	time_t time = 0;
	char time_str[64];
	xtream_daylist = *daylist;
	if(start_timestamp&&(epg_num>0))
	{
		if(!xtream_daylist)
		{
			xtream_daylist = (EpgDayList *)GxCore_Malloc(sizeof(EpgDayList));
			memset(xtream_daylist, 0, sizeof(EpgDayList));
			xtream_daylist->day_item = (EpgDayItem *)GxCore_Malloc(sizeof(EpgDayItem)*epg_num);
			memset(xtream_daylist->day_item, 0, sizeof(EpgDayItem)*epg_num);
			*daylist = xtream_daylist;
		}
		time = get_display_time_by_timezone((time_t)atoi(start_timestamp));
		time = (time/XTREAM_DAY_TO_SEC)*XTREAM_DAY_TO_SEC;
		memset(time_str, 0, sizeof(time_str));
		sprintf(time_str, "%d", (int)time);
		if(app_xtream_is_day_exist(xtream_daylist, time_str) == 0)
		{
			xtream_daylist->day_item[xtream_daylist->day_num].day_id = GxCore_Strdup(time_str);
			xtream_daylist->day_item[xtream_daylist->day_num].title = app_time_to_date_edit_str(time);
			xtream_daylist->day_num++;
		}
	}
}

static void app_xtream_get_current_day(EpgDayList *daylist)
{
	time_t time1 = 0;
	time_t time2 = 0;
	int i = 0;
	if(daylist&&(daylist->day_num>0))
	{
		daylist->cur_day = 0;
		time1 = get_display_time_by_timezone(time(NULL));
		for(i=0; i<daylist->day_num; i++)
		{
			time2 = atoi(daylist->day_item[i].day_id);
			if((time1>=time2)&&(time1<(time2+XTREAM_DAY_TO_SEC)))
			{
				daylist->cur_day = i;
				break;
			}
		}
	}
}

static EpgDayList* app_xtream_get_daylist_from_epglist(gx_xtream_epglist_t *epglist)
{
	EpgDayList *retlist = NULL;
	int i = 0;
	if(epglist&&(epglist->epg_num>0))
	{
		for(i=0; i<epglist->epg_num; i++)
		{
			app_xtream_add_day_item(epglist->epgs[i].start_timestamp, epglist->epg_num, &retlist);
		}
		app_xtream_get_current_day(retlist);
	}
	return retlist;
}

static EpgDayList* app_xtream_get_epg_days(char *stream_id)
{
	gx_xtream_epglist_t *epglist = NULL;
	EpgDayList *retlist = NULL;
	unsigned int handle = 0;
	xtream_epg_handle++;
	handle = xtream_epg_handle;
	if(GXAPPS_SUCCESS == gx_xtream_get_full_epg(atoi(stream_id), &epglist))
	{
		if(handle == xtream_epg_handle)
		{
			retlist = app_xtream_get_daylist_from_epglist(epglist);
			xtream_full_epglist = epglist;
		}
		else
		{
			gx_xtream_free_epglist(epglist);
		}
	}
	return retlist;
}

static EpgEventList* app_xtream_get_full_epg(char *stream_id, char *day_id)
{
	EpgEventList *retlist = NULL;
	int time_now = 0;
	int time1 = 0;
	int time2 = 0;
	int i = 0;
	if(xtream_full_epglist&&(xtream_full_epglist->epg_num>0))
	{
		time_now = get_display_time_by_timezone(time(NULL));
		time1 = atoi(day_id);
		for(i=0; i<xtream_full_epglist->epg_num; i++)
		{
			time2 = get_display_time_by_timezone((time_t)atoi(xtream_full_epglist->epgs[i].start_timestamp));
			if((time2>=time1)&&(time2<(time1+XTREAM_DAY_TO_SEC)))
			{
				if(!retlist)
				{
					retlist = (EpgEventList *)GxCore_Malloc(sizeof(EpgEventList));
					memset(retlist, 0, sizeof(EpgEventList));
					retlist->epg_item = (EpgEventItem *)GxCore_Malloc(sizeof(EpgEventItem)*xtream_full_epglist->epg_num);
					memset(retlist->epg_item, 0, sizeof(EpgEventItem)*xtream_full_epglist->epg_num);
				}
				if(xtream_full_epglist->epgs[i].start_timestamp)
				{
					retlist->epg_item[retlist->epg_num].start_time = app_iptv_get_time_str(xtream_full_epglist->epgs[i].start_timestamp);
				}
				if(xtream_full_epglist->epgs[i].end_timestamp)
				{
					retlist->epg_item[retlist->epg_num].end_time = app_iptv_get_time_str(xtream_full_epglist->epgs[i].end_timestamp);
				}
				if(xtream_full_epglist->epgs[i].title)
				{
					retlist->epg_item[retlist->epg_num].title = GxCore_Strdup(xtream_full_epglist->epgs[i].title);
				}
				if(xtream_full_epglist->epgs[i].description)
				{
					retlist->epg_item[retlist->epg_num].description = GxCore_Strdup(xtream_full_epglist->epgs[i].description);
				}
				if((time_now>=time2)&&(time_now<get_display_time_by_timezone((time_t)atoi(xtream_full_epglist->epgs[i].end_timestamp))))
				{
					retlist->cur_epg = retlist->epg_num;
				}
				retlist->epg_num++;
			}
		}
	}
	return retlist;
}

static void app_xtream_epg_exit(void)
{
	xtream_epg_handle++;
	app_xtream_free_full_epglist();
}
#endif

status_t app_create_xtreamiptv_menu(void)
{
	IPTVOps ops;

	memset(&ops, 0, sizeof(IPTVOps));
	ops.server_type = IPTV_SERVER_XTREAM;
	ops.server_title = STR_ID_NET_XTREAM_IPTV;
	ops.source_num = 1;
	ops.auto_play = 1;
	ops.set_save_channel = save_xtream_playing_channel;
	ops.get_save_channel = get_xtream_save_channel;
#ifdef IPTV_FAV_SUPPORT
	ops.get_fav_list = get_xtream_get_fav_list;
	ops.save_fav_list = get_xtream_save_fav_list;
#endif
	ops.get_url = app_xtream_play_get_url;
#ifdef IPTV_EPG_SUPPORT
	ops.epg_ctrl.get_short_epg = app_xtream_get_short_epg;
	ops.epg_ctrl.get_epg_days = app_xtream_get_epg_days;
	ops.epg_ctrl.get_full_epg = app_xtream_get_full_epg;
	ops.epg_ctrl.epg_exit = app_xtream_epg_exit;
#endif
	app_create_iptv_menu(&ops);

	return GXCORE_SUCCESS;
}

void xtream_iptv_create(void)
{
	if(xtream_login_status == IPTV_LOGIN_SUCCESS)
	{
		app_create_xtreamiptv_menu();
	}
	else
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.mode = POP_MODE_UNBLOCK;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_PLEASE_LOGIN;
		pop.timeout_sec= 3;
		popdlg_create(&pop);
	}
}

void xtream_vod_create(void)
{
	if(xtream_login_status == IPTV_LOGIN_SUCCESS)
	{
		app_create_xtreamvod_menu();
	}
	else
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.mode = POP_MODE_UNBLOCK;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_PLEASE_LOGIN;
		pop.timeout_sec= 3;
		popdlg_create(&pop);
	}
}

#ifdef XTREAM_SERIES_SUPPORT
void xtream_series_create(void)
{
	if(xtream_login_status == IPTV_LOGIN_SUCCESS)
	{
		app_create_xtreamseries_menu();
	}
	else
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.mode = POP_MODE_UNBLOCK;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_PLEASE_LOGIN;
		pop.timeout_sec= 3;
		popdlg_create(&pop);
	}
}
#endif

void xtream_setting_get(char *param1, char *param2, char *param3, char *param4)
{
	GxBus_ConfigGet(XTREAM_URL_KEY, param1, IPTV_PARAM_LEN,  XTREAM_DEFAULT_URL);
	GxBus_ConfigGet(XTREAM_USER_KEY, param2, IPTV_PARAM_LEN,  XTREAM_DEFAULT_USER);
	GxBus_ConfigGet(XTREAM_PWD_KEY, param3, IPTV_PARAM_LEN,  XTREAM_DEFAULT_PWD);
}

void xtream_setting_set(char *param1, char *param2, char *param3, char *param4)
{
	GxBus_ConfigSet(XTREAM_URL_KEY, param1);
	GxBus_ConfigSet(XTREAM_USER_KEY, param2);
	GxBus_ConfigSet(XTREAM_PWD_KEY, param3);
}

void xtream_setting_save(void)
{
	app_xtream_login();
}

int xtream_get_status(void)
{
	return xtream_login_status;
}

#ifdef IPTV_ACCOUNT_SUPPORT
gx_list_t* xtream_get_account_info(void)
{
	gx_list_t *list = NULL;
	gx_xtream_userinfo_t *userinfo = NULL;
	char *time_stamp = NULL;
	if(xtream_login_status == IPTV_LOGIN_SUCCESS)
	{
		userinfo = gx_xtream_get_userinfo();
		if(userinfo)
		{
			list = gx_list_new();
			if(userinfo->status)
			{
				gx_list_add_item(list, STR_ID_STATE, userinfo->status, NULL);
			}
			if(userinfo->create_timestamp)
			{
				time_stamp = app_time_to_format_str(get_display_time_by_timezone((time_t)atoi(userinfo->create_timestamp)));
				gx_list_add_item(list, STR_ID_NET_CREATE_TIME, time_stamp, NULL);
				APP_FREE(time_stamp);
			}
			if(userinfo->expire_timestamp)
			{
				time_stamp = app_time_to_format_str(get_display_time_by_timezone((time_t)atoi(userinfo->expire_timestamp)));
				gx_list_add_item(list, STR_ID_NET_EXPIRE_TIME, time_stamp, NULL);
				APP_FREE(time_stamp);
			}
			gx_xtream_free_userinfo(userinfo);
		}
	}
	return list;
}

static int xtream_redkey_pressfun(void)
{
    gx_list_t *list = NULL;
    if((list = xtream_get_account_info()) != NULL)
    {
        app_create_iptv_account_infor_menu(list);
    }
    else
    {
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_PLEASE_LOGIN;
		pop.timeout_sec= 3;
		popdlg_create(&pop);
    }
    return 0;
}

void xtream_colorkey_func(MenuFuncKey *key)
{
    if(key == NULL)
        return;

    key->redKey.keyName = STR_ID_INFORMATION;
    key->redKey.keyPressFun = xtream_redkey_pressfun;
    key->greenKey.keyName = NULL;
    key->greenKey.keyPressFun = NULL;
    key->blueKey.keyName = NULL;
    key->blueKey.keyPressFun = NULL;
    key->yellowKey.keyName = NULL;
    key->yellowKey.keyPressFun = NULL;
}
#endif

void xtream_create_setting(void)
{
	IPTVSetCtrl xtream_set;

	memset(&xtream_set, 0, sizeof(IPTVSetCtrl));
	xtream_set.server_type = IPTV_SERVER_XTREAM;
	xtream_set.name = STR_ID_NET_XTREAM_SETTING;
	xtream_set.param_num = 3;
	xtream_set.param1_name = STR_ID_SERVER_URL;
	xtream_set.param2_name = "Username";
	xtream_set.param3_name = "Password";
	xtream_set.setting_get = xtream_setting_get;
	xtream_set.setting_set = xtream_setting_set;
	xtream_set.save_func = xtream_setting_save;
	xtream_set.get_status = xtream_get_status;
#ifdef IPTV_ACCOUNT_SUPPORT
	xtream_set.funckey = xtream_colorkey_func;
#endif

	app_iptv_create_setting_menu(&xtream_set);
}

void app_create_xtream_menu(void)
{
	IptvItem xtream_item[XTREAM_ITEM_NUM];

	memset(xtream_item, 0, sizeof(IptvItem) * XTREAM_ITEM_NUM);

	xtream_item[0].item_name = STR_ID_NET_XTREAM_IPTV;
	xtream_item[0].item_pic = XTREAM_IPTV_PIC;
	xtream_item[0].item_func = xtream_iptv_create;

	xtream_item[1].item_name = STR_ID_NET_XTREAM_VOD;
	xtream_item[1].item_pic = XTREAM_IPTV_PIC;
	xtream_item[1].item_func = xtream_vod_create;

#ifdef XTREAM_SERIES_SUPPORT
	xtream_item[2].item_name = STR_ID_NET_XTREAM_SERIES;
	xtream_item[2].item_pic = XTREAM_SERIES_PIC;
	xtream_item[2].item_func = xtream_series_create;
	xtream_item[3].item_name = STR_ID_NET_XTREAM_SETTING;
	xtream_item[3].item_pic = XTREAM_IPTV_PIC;
	xtream_item[3].item_func = xtream_create_setting;
#else
	xtream_item[2].item_name = STR_ID_NET_XTREAM_SETTING;
	xtream_item[2].item_pic = XTREAM_IPTV_PIC;
	xtream_item[2].item_func = xtream_create_setting;
#endif
	app_create_iptv_browser_menu(STR_ID_NET_XTREAM, NULL, XTREAM_ITEM_NUM, xtream_item);
}

app_netapp_event_class app_netapp_event_xtream = {
	.name = "netapp xtream",
	.msg_wndName = NULL,
	.msg_proc_callback = app_xtream_msg_proc,
	.msg_dev_out_callback = app_xtream_set_logout,
	.key_net_open_check = NULL,
};

#endif
