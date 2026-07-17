#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#ifdef STALKER_SUPPORT
#include <gx_stalker.h>
#include <app_iptv_list_api.h>
#include "iptv/app_iptv_list_service.h"
#include "iptv/app_iptv.h"
#include "app_netapps_common.h"

#define STR_ID_NET_STALKER "Stalker"
#define STR_ID_NET_STALKER_SETTING "Stalker Setting"
#define STR_ID_NET_STALKER_IPTV "Stalker IPTV"
#define STR_ID_NET_STALKER_VOD "Stalker VOD"
#define STR_ID_MAC_ADDR   "Mac address"
#define STR_ID_NET_STALKER_SERIES "Stalker Series"

#define STALKER_FAVLIST_PATH "/home/gx/stalker_fav.xml"
#define STALKER_URL_KEY    "netapps>stalker_url"
#define STALKER_MAC_KEY    "netapps>stalker_mac"
#define STALKER_USER_KEY    "netapps>stalker_name"
#define STALKER_PASS_KEY    "netapps>stalker_pass"
#define STALKER_INVALID_URL    "http://127.0.0.1"
#define STALKER_DEFAULT_URL    STALKER_INVALID_URL
#define STALKER_DEFAULT_MAC    "00:11:22:33:44:55"
#define STALKER_DEFAULT_USER    "gx"
#define STALKER_DEFAULT_PASS    "gx"
#define STALKER_IPTV_PIC "default"
#define STALKER_VOD_PIC "default"
#define STALKER_SERIES_PIC "default"
#define STALKER_SETTING_PIC "default"
#define STALKER_ITEM_NUM 4
#define STALKER_PLAYGROUP       "stalker>play_group"
#define STALKER_PLAYURL         "stalker>play_url"
#define STALKER_PLAYNAME        "stalker>play_name"
#define STALKER_STREAM_ID_LEN (16)
#define STALKER_MAX_EPG_PAGE_NUM (10)
#define STALKER_LOGIN_RETYR_TIMES (10)
#define STALKER_MAX_CHANNEL_NUM 1000
#define STALKER_CHANNEL_BUFFER_SIZE 2*1024*1024

static int stalker_login_status = IPTV_LOGIN_PLEASE;
static int s_stalker_is_download = 0;
static int stalker_autologin_running = 0;
static int stalker_autologin_count = 0;

//#define SUPPORT_STALKER_CATEGORY

static void app_stalker_auto_login_func(void);

extern status_t app_create_stalkervod_menu(void);
extern status_t app_create_stalkerseries_menu(void);

typedef void(* stalker_thread_func) (void);

static void stalker_thread_body(void* arg)
{
	stalker_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (stalker_thread_func)arg;
	if(func)
	{
		func();
	}
}

static void create_stalker_thread(stalker_thread_func func)
{
	static int thread_id = 0;
    uint32_t size = (func == app_stalker_auto_login_func) ? 12*1024 : 64*1024;
	GxCore_ThreadCreate("stalker", &thread_id, stalker_thread_body,
		(void *)func, size, GXOS_DEFAULT_PRIORITY);
}

static char *app_stalker_setting_url_get(void)
{
	char *url  = GxCore_Calloc(IPTV_PARAM_LEN, 1);
	GxBus_ConfigGet(STALKER_URL_KEY, url, IPTV_PARAM_LEN,  STALKER_DEFAULT_URL);
	return url;
}

static char *app_stalker_setting_mac_get(void)
{
	char *mac  = GxCore_Calloc(IPTV_PARAM_LEN, 1);
	GxBus_ConfigGet(STALKER_MAC_KEY, mac, IPTV_PARAM_LEN,  STALKER_DEFAULT_MAC);
	return mac;
}

static char *app_stalker_setting_user_get(void)
{
	char *user  = GxCore_Calloc(IPTV_PARAM_LEN, 1);
	GxBus_ConfigGet(STALKER_USER_KEY, user, IPTV_PARAM_LEN,  STALKER_DEFAULT_USER);
	return user;
}

static char *app_stalker_setting_pass_get(void)
{
	char *pass  = GxCore_Calloc(IPTV_PARAM_LEN, 1);
	GxBus_ConfigGet(STALKER_PASS_KEY, pass, IPTV_PARAM_LEN,  STALKER_DEFAULT_PASS);
	return pass;
}

static int app_is_stalkeriptv_busy(void)
{
	return s_stalker_is_download;
}

int app_stalker_set_logout(void)
{
	stalker_autologin_count = 0;
	if (stalker_autologin_running > 0)
	{
		stalker_autologin_running = 2;
	}
	if (stalker_login_status == IPTV_LOGIN_SUCCESS)
	{
		if (IF_STATE_CONNECTED != app_network_get_cur_dev_and_state(NULL))
		{
			stalker_login_status = IPTV_LOGIN_FAILED;
			app_iptv_show_status(IPTV_SERVER_STALKER);
		}
	}
	return 0;
}

static int app_stalker_check_account(void)
{
	int valid = 0;
	char *url = NULL;

	url = app_stalker_setting_url_get();
	if(strcmp(url, STALKER_INVALID_URL) != 0)
	{
		valid = 1;
	}
	APP_FREE(url);

	return valid;
}

static void _stalker_login(void)
{
	gxapps_ret_t ret = GXAPPS_SUCCESS;
	char *url = NULL;
	char *mac = NULL;
	char *user = NULL;
	char *pass = NULL;

	s_stalker_is_download = 1;
	stalker_login_status = IPTV_LOGIN_RUNNING;
	app_iptv_show_status(IPTV_SERVER_STALKER);
	gx_stalker_logout();

	url = app_stalker_setting_url_get();
	mac = app_stalker_setting_mac_get();
	user = app_stalker_setting_user_get();
	pass = app_stalker_setting_pass_get();
	ret = gx_stalker_login(url, mac, user, pass);
	APP_FREE(url);
	APP_FREE(mac);
	APP_FREE(user);
	APP_FREE(pass);
	if(ret == GXAPPS_SUCCESS)
	{
		stalker_login_status = IPTV_LOGIN_SUCCESS;
	}
	else
	{
		stalker_login_status = IPTV_LOGIN_FAILED;
	}
	app_iptv_show_status(IPTV_SERVER_STALKER);

	s_stalker_is_download = 0;
}

static void app_stalker_auto_login_func(void)
{
	int count = 0;
	while(stalker_autologin_running>0)
	{
		if(stalker_autologin_running == 1)
		{
			if(app_is_stalkeriptv_busy() || (stalker_login_status == IPTV_LOGIN_RUNNING))
			{
				GxCore_ThreadDelay(100);
			}
			else
			{
				_stalker_login();
				if((stalker_login_status == IPTV_LOGIN_SUCCESS))
				{
					stalker_autologin_running = 2;
					stalker_autologin_count = 0;
				}
				else
				{
					stalker_autologin_count++;
					if(stalker_autologin_count>=STALKER_LOGIN_RETYR_TIMES)
					{
						stalker_autologin_running = 2;
					}
					else
					{
						count = 0;
						while((count++<30)&&(stalker_autologin_running == 1))
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

void app_stalker_auto_login_start(void)
{
	stalker_autologin_count = 0;
	if(app_stalker_check_account()>0)
	{
		if(stalker_autologin_running == 0)
		{
			stalker_autologin_running = 1;
			create_stalker_thread(app_stalker_auto_login_func);
		}
		else
		{
			stalker_autologin_running = 1;
		}
	}
}

void app_stalker_auto_login_stop(void)
{
	stalker_autologin_count = 0;
	if(stalker_autologin_running>0)
	{
		stalker_autologin_running = 2;
	}
}

static int _app_stalker_login_popdlg_exit_cb(PopDlgRet ret)
{
	if(ret == POP_VAL_OK)
	{
		app_iptv_tip_show(STR_ID_LOGIN_WAIT);
		create_stalker_thread(_stalker_login);
	}
	return 0 ;
}

void app_stalker_login(void)
{
	if(app_is_stalkeriptv_busy() || (stalker_login_status == IPTV_LOGIN_RUNNING))
	{
		app_iptv_tip_show("System Busy, Please Wait!");
	}
	else
	{
		app_stalker_auto_login_stop();
		if((stalker_login_status == IPTV_LOGIN_SUCCESS))
		{
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_YES_NO;
			pop.mode = POP_MODE_UNBLOCK;
			pop.str = STR_ID_RELOGIN;
			pop.exit_cb = _app_stalker_login_popdlg_exit_cb;
			popdlg_create(&pop) ;
		}
		else
		{
			_app_stalker_login_popdlg_exit_cb(POP_VAL_OK);
		}
	}
}

static char * app_stalker_get_categories_name(gx_stalker_categorylist_t *pCategorylist, int category_id)
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

static int app_stalker_need_limit_channel(void)
{
	int ret = 0;

	if(app_netapps_get_memory_size()<=64*1024*1024)
	{
		ret = 1;
	}

	return ret;
}

static void app_stalkeriptv_get_list(void)
{
	gx_stalker_channellist_t *stalker_channellist = NULL;
	IptvListClass *list = NULL;
	AppMsg_IptvList iptv_list;
	char *key=NULL, *value=NULL, *type=NULL;
	gxapps_ret_t retcode = GXAPPS_SUCCESS;
	int i=0;
	char * categories_name =NULL;
	int len = 0;
	s_stalker_is_download = 1;
	if(stalker_login_status == IPTV_LOGIN_SUCCESS)
	{
		if(app_stalker_need_limit_channel())
		{
			gx_stalker_set_channel_max(STALKER_MAX_CHANNEL_NUM);
			gx_stalker_set_channel_buffer_size(STALKER_CHANNEL_BUFFER_SIZE);
		}
#ifdef SUPPORT_STALKER_CATEGORY
		retcode = gx_stalker_get_category_channels	(-1,&stalker_channellist);
#else
		retcode = gx_stalker_get_channels(&stalker_channellist);
#endif
		if(retcode == GXAPPS_SUCCESS)
		{
			list = _iptv_list_new();
			for(i=0; i<stalker_channellist->channel_num; i++)
			{
                if(list->iptv_item_num == LIST_MAX_ITEM_NUM)
                    break;
				key = NULL;
				value = NULL;
				type = NULL;
				key = GxCore_Strdup(stalker_channellist->channellist[i].channel_name);
				len = strlen(stalker_channellist->channellist[i].play_cmd)+STALKER_STREAM_ID_LEN+2+1;
				value = GxCore_Calloc(1, len);
				snprintf(value, len-1, "%d:%d:%s",stalker_channellist->channellist[i].stream_id,stalker_channellist->channellist[i].lock,stalker_channellist->channellist[i].play_cmd);
				categories_name = app_stalker_get_categories_name(stalker_channellist->categorylist, stalker_channellist->channellist[i].category_id);
				if(categories_name)
					type = GxCore_Strdup(categories_name);

				list->iptv_item_list[list->iptv_item_num].iptv_key = key;
				list->iptv_item_list[list->iptv_item_num].iptv_value = value;
				list->iptv_item_list[list->iptv_item_num].iptv_type = type;
				list->iptv_item_num++;
			}
		}
		if(stalker_channellist)
		{
			gx_stalker_free_channellist(stalker_channellist);
			stalker_channellist = NULL;
		}
	}

	s_stalker_is_download = 0;
	memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.retcode = retcode;
	iptv_list.iptv_source = IPTV_LIST_REMOTE;
	iptv_list.iptv_server = IPTV_SERVER_STALKER;
	iptv_list.iptv_list = list;
	app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);

}

status_t app_stalkeriptv_service_get_list(void)
{
	if(app_is_stalkeriptv_busy())
	{
		return GXCORE_ERROR;
	}
	create_stalker_thread(app_stalkeriptv_get_list);

	return GXCORE_SUCCESS;
}

int app_stalker_msg_proc(GxMessage *msg)
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
					app_stalker_auto_login_start();
				}
				else
				{
					app_stalker_set_logout();
				}
				break;
			}
		default:
			break;
	}
	return EVENT_TRANSFER_STOP;
}


static void save_stalker_playing_channel(IPTVChannel *channel)
{
	GxBus_ConfigSet(STALKER_PLAYGROUP, channel->group);
	GxBus_ConfigSet(STALKER_PLAYURL, channel->url);
	GxBus_ConfigSet(STALKER_PLAYNAME, channel->name);
}

static IPTVChannel* get_stalker_save_channel(void)
{
	IPTVChannel *channel = (IPTVChannel*)GxCore_Malloc(sizeof(IPTVChannel));
	channel->group = (char*)GxCore_Malloc(MAX_IPTV_GROUP_LENGTH);
	channel->url = (char*)GxCore_Malloc(MAX_IPTV_URL_LENGTH);
	channel->name = (char*)GxCore_Malloc(MAX_IPTV_NAME_LENGTH);
	GxBus_ConfigGet(STALKER_PLAYGROUP, channel->group, MAX_IPTV_GROUP_LENGTH, "");
	GxBus_ConfigGet(STALKER_PLAYURL, channel->url, MAX_IPTV_URL_LENGTH, "");
	GxBus_ConfigGet(STALKER_PLAYNAME, channel->name, MAX_IPTV_NAME_LENGTH, "");
	return channel;
}

#ifdef IPTV_FAV_SUPPORT
static gx_list_t* get_stalker_get_fav_list(void)
{
	gx_list_t *list = NULL;
	if(GXCORE_FILE_EXIST == GxCore_FileExists(STALKER_FAVLIST_PATH))
	{
		list = gx_get_list_from_file(STALKER_FAVLIST_PATH);
	}
	return list;
}

static void get_stalker_save_fav_list(gx_list_t* list)
{
	if(list)
	{
		gx_list_save_to_file(list, STALKER_FAVLIST_PATH);
	}
}
#endif
static char* app_stalker_play_get_url(char *playcmd)
{
	char *url = NULL;
	char *ret = NULL;
	char *cmd = NULL;
	char *plock= NULL;
	cmd = strchr(playcmd, ':');
	if(cmd)
	{
		cmd++;
		plock = cmd;
		cmd = strchr(plock, ':');
		if(cmd)
		{
			cmd++;
		}
	}
	else
	{
		cmd = playcmd;
	}
	if(GXAPPS_SUCCESS == gx_stalker_get_live_playurl(cmd, &url))
	{
		if(url)
		{
			ret = GxCore_Strdup(url);
			gx_stalker_free_live_playurl(url);
		}
	}
	return ret;
}
static int app_stalker_check_lock(char *playcmd)
{
	char *cmd = NULL;
	char *plock= NULL;
	char *id = NULL;
	int lock = 0;
	cmd = strchr(playcmd, ':');
	if(cmd)
	{
		cmd++;
		plock = cmd;
		cmd = strchr(plock, ':');
		if(cmd)
		{
			id = gx_strndup(plock, cmd-plock);
			lock = atoi(id);
			free(id);
		}
	}
	return lock;
}

#ifdef IPTV_EPG_SUPPORT
static EpgEventList* app_stalker_get_short_epg(char *stream_id, int num)
{
	gx_stalker_epglist_t *epglist = NULL;
	EpgEventList *retlist = NULL;
	char *id = NULL;
	char *temp = NULL;
	int s_id = 0;
	int i = 0;
	temp = strchr(stream_id, ':');
	if(temp)
	{
		id = gx_strndup(stream_id, temp-stream_id);
		s_id = atoi(id);
		free(id);
	}
	else
	{
		s_id = atoi(stream_id);
	}
	if(GXAPPS_SUCCESS == gx_stalker_get_short_epg(s_id, num, &epglist))
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
			gx_stalker_free_epglist(epglist);
		}
	}
	return retlist;
}

static EpgDayList* app_stalker_get_epg_days(char *stream_id)
{
	gx_stalker_daylist_t *daylist = NULL;
	EpgDayList *retlist = NULL;
	int i = 0;
	if(GXAPPS_SUCCESS == gx_stalker_get_epg_days(&daylist))
	{
		if(daylist&&(daylist->day_num>0))
		{
			retlist = (EpgDayList *)GxCore_Malloc(sizeof(EpgDayList));
			memset(retlist, 0, sizeof(EpgDayList));
			retlist->day_item = (EpgDayItem *)GxCore_Malloc(sizeof(EpgDayItem)*daylist->day_num);
			memset(retlist->day_item, 0, sizeof(EpgDayItem)*daylist->day_num);
			for(i=0; i<daylist->day_num; i++)
			{
				retlist->day_item[retlist->day_num].day_id = GxCore_Strdup(daylist->days[i].id);
				retlist->day_item[retlist->day_num].title = GxCore_Strdup(daylist->days[i].title);
				if(daylist->days[i].is_today>0)
				{
					retlist->cur_day = retlist->day_num;
				}
				retlist->day_num++;
			}
		}
		gx_stalker_free_daylist(daylist);
	}
	return retlist;
}

static EpgEventList* app_stalker_get_full_epg(char *stream_id, char *day_id)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	EpgEventList *retlist = NULL;
	gx_stalker_epglist_t *tmplist = NULL;
	int time_now = 0;
	int time1 = 0;
	int time2 = 0;
	int i = 0;
	int j = 0;
	char *id = NULL;
	char *temp = NULL;
	int s_id = 0;
	int max_epg_num = 0;
	temp = strchr(stream_id, ':');
	if(temp)
	{
		id = gx_strndup(stream_id, temp-stream_id);
		s_id = atoi(id);
		free(id);
	}
	else
	{
		s_id = atoi(stream_id);
	}
	time_now = time(NULL);
	for(i=0; i<STALKER_MAX_EPG_PAGE_NUM; i++)
	{
		ret = gx_stalker_get_full_epg(s_id, day_id, i+1, &tmplist);
		if(ret == GXAPPS_SUCCESS)
		{
			if(tmplist&&(tmplist->epg_num>0)&&((tmplist->epg_total_num>0)))
			{
				if(!retlist)
				{
					max_epg_num = tmplist->epg_total_num;
					retlist = (EpgEventList *)GxCore_Malloc(sizeof(EpgEventList));
					memset(retlist, 0, sizeof(EpgEventList));
					retlist->epg_item = (EpgEventItem *)GxCore_Malloc(sizeof(EpgEventItem)*max_epg_num);
					memset(retlist->epg_item, 0, sizeof(EpgEventItem)*max_epg_num);
				}
				for(j=0; (j<tmplist->epg_num)&&(retlist->epg_num<max_epg_num); j++)
				{
					if (tmplist->epgs[j].id)
					{
						retlist->epg_item[retlist->epg_num].epg_id = GxCore_Strdup(tmplist->epgs[j].id);
					}
					if (tmplist->epgs[j].start_timestamp)
					{
						time1 = atoi(tmplist->epgs[j].start_timestamp);
						retlist->epg_item[retlist->epg_num].start_time = app_iptv_get_time_str(tmplist->epgs[j].start_timestamp);
					}
					if (tmplist->epgs[j].end_timestamp)
					{
						time2 = atoi(tmplist->epgs[j].end_timestamp);
						retlist->epg_item[retlist->epg_num].end_time = app_iptv_get_time_str(tmplist->epgs[j].end_timestamp);
					}
					if (tmplist->epgs[j].title)
					{
						retlist->epg_item[retlist->epg_num].title = GxCore_Strdup(tmplist->epgs[j].title);
					}
					if (tmplist->epgs[j].description)
					{
						retlist->epg_item[retlist->epg_num].description = GxCore_Strdup(tmplist->epgs[j].description);
					}
					retlist->epg_item[retlist->epg_num].playback = tmplist->epgs[j].mark_archive;
					if((time_now>=time1)&&(time_now<time2))
					{
						retlist->cur_epg = retlist->epg_num;
					}
					retlist->epg_num++;
				}
			}
			gx_stalker_free_epglist(tmplist);
			tmplist = NULL;
			if((max_epg_num>0)&&retlist&&(retlist->epg_num>=max_epg_num))
			{
				break;
			}
		}
		else
		{
			if(retlist)
			{
				app_iptv_free_epglist(retlist);
				retlist = NULL;
			}
			break;
		}
	}
	return retlist;
}

static char* app_stalker_get_epg_url(char *epg_id)
{
	char *url = NULL;
	char *ret_url = NULL;

	if(GXAPPS_SUCCESS == gx_stalker_get_epg_archive_playurl(epg_id, &url))
	{
		if(url)
		{
			ret_url = GxCore_Strdup(url);
		}
		gx_stalker_free_epg_archive_playurl(url);
	}

	return ret_url;
}
#endif

status_t app_create_stalkeriptv_menu(void)
{
	IPTVOps ops;
	memset(&ops, 0, sizeof(IPTVOps));
	ops.server_type = IPTV_SERVER_STALKER;
	ops.server_title = STR_ID_NET_STALKER_IPTV;
	ops.source_num = 1;
	ops.auto_play = 1;
	ops.set_save_channel = save_stalker_playing_channel;
	ops.get_save_channel = get_stalker_save_channel;
#ifdef IPTV_FAV_SUPPORT
	ops.get_fav_list = get_stalker_get_fav_list;
	ops.save_fav_list = get_stalker_save_fav_list;
#endif
	ops.get_url = app_stalker_play_get_url;
	ops.check_lock = app_stalker_check_lock;
#ifdef IPTV_EPG_SUPPORT
	ops.epg_ctrl.get_short_epg = app_stalker_get_short_epg;
	ops.epg_ctrl.get_epg_days = app_stalker_get_epg_days;
	ops.epg_ctrl.get_full_epg = app_stalker_get_full_epg;
	ops.epg_ctrl.get_epg_url = app_stalker_get_epg_url;
#endif
	app_create_iptv_menu(&ops);

	return GXCORE_SUCCESS;
}

void stalker_iptv_create(void)
{
	if(stalker_login_status == IPTV_LOGIN_SUCCESS)
	{
		app_create_stalkeriptv_menu();
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

void stalker_vod_create(void)
{
	if(stalker_login_status == IPTV_LOGIN_SUCCESS)
	{
		app_create_stalkervod_menu();
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

#ifdef STALKER_SERIES_SUPPORT
void stalker_series_create(void)
{
	if(stalker_login_status == IPTV_LOGIN_SUCCESS)
	{
		app_create_stalkerseries_menu();
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

void stalker_setting_get(char *param1, char *param2, char *param3, char *param4)
{
	GxBus_ConfigGet(STALKER_URL_KEY, param1, IPTV_PARAM_LEN,  STALKER_DEFAULT_URL);
	GxBus_ConfigGet(STALKER_MAC_KEY, param2, IPTV_PARAM_LEN,  STALKER_DEFAULT_MAC);
	GxBus_ConfigGet(STALKER_USER_KEY, param3, IPTV_PARAM_LEN,  STALKER_DEFAULT_USER);
	GxBus_ConfigGet(STALKER_PASS_KEY, param4, IPTV_PARAM_LEN,  STALKER_DEFAULT_PASS);

}

void stalker_setting_set(char *param1, char *param2, char *param3, char *param4)
{
	GxBus_ConfigSet(STALKER_URL_KEY, param1);
	GxBus_ConfigSet(STALKER_MAC_KEY, param2);
	GxBus_ConfigSet(STALKER_USER_KEY, param3);
	GxBus_ConfigSet(STALKER_PASS_KEY, param4);
}

void stalker_setting_save(void)
{
	app_stalker_login();
}

int stalker_get_status(void)
{
	return stalker_login_status;
}

#ifdef IPTV_ACCOUNT_SUPPORT
static int stalker_is_digit_str(char *str)
{
	int len = 0;
	int i = 0;
	if(str&&((len=strlen(str))>0))
	{
		for(i=0; i<len; i++)
		{
			if(!isdigit(str[i]))
			{
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

gx_list_t* stalker_get_account_info(void)
{
	gx_list_t *list = NULL;
	gx_stalker_userinfo_t *userinfo = NULL;
	char *time_stamp = NULL;
	if(stalker_login_status == IPTV_LOGIN_SUCCESS)
	{
		userinfo = gx_stalker_get_userinfo();
		if(userinfo)
		{
			list = gx_list_new();
			if(userinfo->mac)
			{
				gx_list_add_item(list, "MAC", userinfo->mac, NULL);
			}
			if(userinfo->expire_timestamp)
			{
				if(stalker_is_digit_str(userinfo->expire_timestamp)>0)
				{
					time_stamp = app_time_to_format_str(get_display_time_by_timezone((time_t)atoi(userinfo->expire_timestamp)));
				}
				else
				{
					time_stamp = GxCore_Strdup(userinfo->expire_timestamp);
				}
				gx_list_add_item(list, STR_ID_NET_EXPIRE_TIME, time_stamp, NULL);
				APP_FREE(time_stamp);
			}
			gx_stalker_free_userinfo(userinfo);
		}
	}
	return list;
}

static int stalker_redkey_pressfun(void)
{
    gx_list_t *list = NULL;
    if((list = stalker_get_account_info()) != NULL)
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


void stalker_colorkey_func(MenuFuncKey *key)
{
    if(key == NULL)
        return;

    key->redKey.keyName = STR_ID_INFORMATION;
    key->redKey.keyPressFun = stalker_redkey_pressfun;
    key->greenKey.keyName = NULL;
    key->greenKey.keyPressFun = NULL;
    key->blueKey.keyName = NULL;
    key->blueKey.keyPressFun = NULL;
    key->yellowKey.keyName = NULL;
    key->yellowKey.keyPressFun = NULL;
}
#endif

void stalker_create_setting(void)
{
	IPTVSetCtrl stalker_set;

	memset(&stalker_set, 0, sizeof(IPTVSetCtrl));
	stalker_set.server_type = IPTV_SERVER_STALKER;
	stalker_set.name = STR_ID_NET_STALKER_SETTING;
	stalker_set.param_num = 4;
	stalker_set.param1_name = STR_ID_SERVER_URL;
	stalker_set.param2_name = "Mac address";
	stalker_set.param3_name = "Username";
	stalker_set.param4_name = "Password";
	stalker_set.setting_get = stalker_setting_get;
	stalker_set.setting_set = stalker_setting_set;
	stalker_set.save_func = stalker_setting_save;
	stalker_set.get_status = stalker_get_status;
#ifdef IPTV_ACCOUNT_SUPPORT
	stalker_set.funckey = stalker_colorkey_func;
#endif

	app_iptv_create_setting_menu(&stalker_set);
}

void app_create_stalker_menu(void)
{
	IptvItem stalker_item[STALKER_ITEM_NUM];

	memset(stalker_item, 0, sizeof(IptvItem) * STALKER_ITEM_NUM);

	stalker_item[0].item_name = STR_ID_NET_STALKER_IPTV;
	stalker_item[0].item_pic = STALKER_IPTV_PIC;
	stalker_item[0].item_func = stalker_iptv_create;

	stalker_item[1].item_name = STR_ID_NET_STALKER_VOD;
	stalker_item[1].item_pic = STALKER_IPTV_PIC;
	stalker_item[1].item_func = stalker_vod_create;
#ifdef STALKER_SERIES_SUPPORT
	stalker_item[2].item_name = STR_ID_NET_STALKER_SERIES;
	stalker_item[2].item_pic = STALKER_SERIES_PIC;
	stalker_item[2].item_func = stalker_series_create;
	stalker_item[3].item_name = STR_ID_NET_STALKER_SETTING;
	stalker_item[3].item_pic = STALKER_IPTV_PIC;
	stalker_item[3].item_func = stalker_create_setting;
#else
	stalker_item[2].item_name = STR_ID_NET_STALKER_SETTING;
	stalker_item[2].item_pic = STALKER_IPTV_PIC;
	stalker_item[2].item_func = stalker_create_setting;
#endif
	app_create_iptv_browser_menu(STR_ID_NET_STALKER, NULL, STALKER_ITEM_NUM, stalker_item);
}

app_netapp_event_class app_netapp_event_stalker = {
	.name = "netapp stalker",
	.msg_wndName = NULL,
	.msg_proc_callback = app_stalker_msg_proc,
	.msg_dev_out_callback = app_stalker_set_logout,
	.key_net_open_check = NULL,
};
#endif
