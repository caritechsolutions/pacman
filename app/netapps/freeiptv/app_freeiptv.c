#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#ifdef FREEIPTV_SUPPORT
#include <gx_freeiptv.h>
#include <app_iptv_list_api.h>
#include "iptv/app_iptv_list_service.h"
#include "iptv/app_iptv.h"

#define STR_ID_NET_FREEIPTV "Free IPTV"

#define FREEIPTV_PLAYGROUP       "freeiptv>play_group"
#define FREEIPTV_PLAYURL         "freeiptv>play_url"
#define FREEIPTV_PLAYNAME        "freeiptv>play_name"

static int s_freeiptv_is_download = 0;

typedef void(* freeiptv_thread_func) (void);

static void freeiptv_thread_body(void* arg)
{
	freeiptv_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (freeiptv_thread_func)arg;
	if(func)
	{
		func();
	}
}

static void create_freeiptv_thread(freeiptv_thread_func func)
{
	static int thread_id = 0;

	GxCore_ThreadCreate("freeiptv", &thread_id, freeiptv_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static int app_is_freeiptv_busy(void)
{
	return s_freeiptv_is_download;
}

static char * app_freeiptv_get_categories_name(gx_freeiptv_categorylist_t *pCategorylist, int category_id)
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

static void app_freeiptv_get_list(void)
{
	gx_freeiptv_channellist_t *freeiptv_channellist = NULL;
	IptvListClass *list = NULL;
	AppMsg_IptvList iptv_list;
	char *key=NULL, *value=NULL, *type=NULL;
	gxapps_ret_t ret = GXAPPS_SUCCESS;
	char * categories_name =NULL;
	int i=0;

	s_freeiptv_is_download = 1;
	ret = gx_freeiptv_get_channels(&freeiptv_channellist);
	if(ret == GXAPPS_SUCCESS)
	{
		list = _iptv_list_new();
		for(i=0; i<freeiptv_channellist->channel_num; i++)
		{
            if(list->iptv_item_num == LIST_MAX_ITEM_NUM)
                break;
			key = NULL;
			value = NULL;
			type = NULL;
			key = GxCore_Strdup(freeiptv_channellist->channels[i].channel_name);
			value = GxCore_Strdup(freeiptv_channellist->channels[i].play_url);
			categories_name = app_freeiptv_get_categories_name(freeiptv_channellist->categorylist, freeiptv_channellist->channels[i].category_id);
			if(categories_name)
				type = GxCore_Strdup(categories_name);

			list->iptv_item_list[list->iptv_item_num].iptv_key = key;
			list->iptv_item_list[list->iptv_item_num].iptv_value = value;
			list->iptv_item_list[list->iptv_item_num].iptv_type = type;
			list->iptv_item_num++;
		}
	}
	if(freeiptv_channellist)
	{
		gx_freeiptv_free_channellist(freeiptv_channellist);
		freeiptv_channellist = NULL;
	}

	s_freeiptv_is_download = 0;
	memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.retcode = ret;
	iptv_list.iptv_source = IPTV_LIST_REMOTE;
	iptv_list.iptv_server = IPTV_SERVER_FREEIPTV;
	iptv_list.iptv_list = list;
	app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);

}

status_t app_freeiptv_service_get_list(void)
{
	if(app_is_freeiptv_busy())
	{
		return GXCORE_ERROR;
	}
	create_freeiptv_thread(app_freeiptv_get_list);

	return GXCORE_SUCCESS;
}

static void save_freeiptv_playing_channel(IPTVChannel *channel)
{
	GxBus_ConfigSet(FREEIPTV_PLAYGROUP, channel->group);
	GxBus_ConfigSet(FREEIPTV_PLAYURL, channel->url);
	GxBus_ConfigSet(FREEIPTV_PLAYNAME, channel->name);
}

static IPTVChannel* get_freeiptv_save_channel(void)
{
	IPTVChannel *channel = (IPTVChannel*)GxCore_Malloc(sizeof(IPTVChannel));
	channel->group = (char*)GxCore_Malloc(MAX_IPTV_GROUP_LENGTH);
	channel->url = (char*)GxCore_Malloc(MAX_IPTV_URL_LENGTH);
	channel->name = (char*)GxCore_Malloc(MAX_IPTV_NAME_LENGTH);
	GxBus_ConfigGet(FREEIPTV_PLAYGROUP, channel->group, MAX_IPTV_GROUP_LENGTH, "");
	GxBus_ConfigGet(FREEIPTV_PLAYURL, channel->url, MAX_IPTV_URL_LENGTH, "");
	GxBus_ConfigGet(FREEIPTV_PLAYNAME, channel->name, MAX_IPTV_NAME_LENGTH, "");
	return channel;
}

void app_create_freeiptv_menu(void)
{
	IPTVOps ops;

	memset(&ops, 0, sizeof(IPTVOps));
	ops.server_type = IPTV_SERVER_FREEIPTV;
	ops.server_title = STR_ID_NET_FREEIPTV;
	ops.source_num = 1;
	ops.auto_play = 1;
	ops.set_save_channel = save_freeiptv_playing_channel;
	ops.get_save_channel = get_freeiptv_save_channel;
	app_create_iptv_menu(&ops);
}

#endif
