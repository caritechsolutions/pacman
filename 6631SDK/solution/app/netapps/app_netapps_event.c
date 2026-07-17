#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_windows.h"
#include "app_netapps_init.h"
#include "app_netapps_event.h"

typedef struct
{
    unsigned int handle;
    PlayerEventCb event_cb;
}PlayerEventData;

typedef struct GuiEventNode
{
	GuiEventCb event_cb;
	struct GuiEventNode *next;
}GuiEventNode;

static GuiEventNode *gui_event_list = NULL;

static void player_event_callback(void *data, void *usr_data)
{
	GxPlayerInfo *player_info =NULL;
	PlayerEventData *player_event = NULL;
	int source = -1;
	int event = -1;

	player_info = (GxPlayerInfo *)data;
	player_event = (PlayerEventData *)usr_data;
	if(player_info&&player_event&&player_event->event_cb)
	{
		if(player_info->file_format == 0)
		{
			source = PLAYER_SOURCE_FILE;
		}
		if(player_info->file_format == 1)
		{
			source = PLAYER_SOURCE_DVB;
		}
		if(player_info->file_format == 2)
		{
			source = PLAYER_SOURCE_NET;
		}
		if((source<0)&&player_info->url)
		{
			if(player_info->url[0] == '/')
			{
				source = PLAYER_SOURCE_FILE;
			}
			else if(strncmp(player_info->url, "dvb", 3) == 0)
			{
				source = PLAYER_SOURCE_DVB;
			}
			else if((strncmp(player_info->url, "http", 4) == 0)
				||(strncmp(player_info->url, "compose", 7) == 0)
				||(strncmp(player_info->url, "associationlibre", 16) == 0)
				)
			{
				source = PLAYER_SOURCE_NET;
			}
		}
		if(player_info->status == PLAYER_EHE_STATUS_STOPPED)
		{
			event = PLAYER_EVENT_STOP;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_PLAY_START)
		{
			event = PLAYER_EVENT_START;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_PLAY_PAUSE)
		{
			event = PLAYER_EVENT_PAUSE;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_PLAY_RUNNING)
		{
			event = PLAYER_EVENT_RUNNING;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_PLAY_END)
		{
			event = PLAYER_EVENT_END;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_ERROR)
		{
			event = PLAYER_EVENT_ERROR;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_START_FILL_BUF)
		{
			event = PLAYER_EVENT_BUFFERING;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_END_FILL_BUF)
		{
			event = PLAYER_EVENT_RUNNING;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_PLAY_RESUME)
		{
			event = PLAYER_EVENT_RESUME;
		}
		else if(player_info->status == PLAYER_EHE_STATUS_PLAY_SEEK)
		{
			event = PLAYER_EVENT_SEEK;
		}
		if((source>=0)&&(event>=0)&&(player_info->url))
		{
			player_event->event_cb(source, event, player_info->url);
		}
	}
}

unsigned int app_netapps_player_event_malloc(PlayerEventCb callback)
{
	PlayerEventData *player_event = NULL;

	player_event = (PlayerEventData *)GxCore_Calloc(1, sizeof(PlayerEventData));
	if(player_event)
	{
		player_event->event_cb = callback;
		player_event->handle = (unsigned int)GxPlayer_RegisterEventHook(player_event_callback, player_event);
		if(!player_event->handle)
		{
			GxCore_Free(player_event);
			player_event = NULL;
		}
	}
	return (unsigned int)player_event;
}

void app_netapps_player_event_free(unsigned int handle)
{
	PlayerEventData *player_event = (PlayerEventData *)handle;

	if(player_event)
	{
		if(player_event->handle)
		{
			GxPlayer_UnregisterEventHook((void *)player_event->handle);
			player_event->handle = 0;
		}
		GxCore_Free(player_event);
		player_event = NULL;
	}
}

SIGNAL_HANDLER int app_netapps_gui_create(const char* widgetname, void *usrdata)
{
	GuiEventNode *cur = NULL;

	cur = gui_event_list;
	while(cur)
	{
		if(cur->event_cb)
		{
			cur->event_cb(GUI_EVENT_CREATE, widgetname, usrdata);
		}
		cur = cur->next;
	}

	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_netapps_gui_destroy(const char* widgetname, void *usrdata)
{
	GuiEventNode *cur = NULL;

	cur = gui_event_list;
	while(cur)
	{
		if(cur->event_cb)
		{
			cur->event_cb(GUI_EVENT_EXIT, widgetname, usrdata);
		}
		cur = cur->next;
	}

	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_netapps_gui_got_focus(const char* widgetname, void *usrdata)
{
	GuiEventNode *cur = NULL;

	cur = gui_event_list;
	while(cur)
	{
		if(cur->event_cb)
		{
			cur->event_cb(GUI_EVENT_FOCUS, widgetname, usrdata);
		}
		cur = cur->next;
	}

	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_netapps_gui_lost_focus(const char* widgetname, void *usrdata)
{
	GuiEventNode *cur = NULL;

	cur = gui_event_list;
	while(cur)
	{
		if(cur->event_cb)
		{
			cur->event_cb(GUI_EVENT_UNFOCUS, widgetname, usrdata);
		}
		cur = cur->next;
	}

	return EVENT_TRANSFER_KEEPON;
}

void app_netapps_gui_init(void)
{
	GUI_RegisterGlobalSignal("create", "app_netapps_gui_create");
	GUI_RegisterGlobalSignal("destroy", "app_netapps_gui_destroy");
	GUI_RegisterGlobalSignal("got_focus", "app_netapps_gui_got_focus");
	GUI_RegisterGlobalSignal("lost_focus", "app_netapps_gui_lost_focus");
}

unsigned int app_netapps_gui_event_malloc(GuiEventCb callback)
{
	GuiEventNode *dst = NULL;

	if(callback)
	{
		dst = (GuiEventNode *)GxCore_Calloc(1, sizeof(GuiEventNode));
		if(dst)
		{
			dst->event_cb= callback;
			dst->next = NULL;
			if (!gui_event_list)
			{
				gui_event_list = dst;
			}
			else
			{
				GuiEventNode *cur = gui_event_list;
				while (cur->next)
				{
					cur = cur->next;
				}
				cur->next = dst;
			}
		}
	}

	return (unsigned int)dst;
}

void app_netapps_gui_event_free(unsigned int handle)
{
	GuiEventNode* cur = NULL;
	GuiEventNode* prev = NULL;
	GuiEventNode *dst = NULL;

	cur = gui_event_list;
	dst = (GuiEventNode *)handle;
	if(cur&&dst)
	{
		while(cur)
		{
			if(cur == dst)
			{
				if (!prev)
				{
					gui_event_list = cur->next;
				}
				else
				{
					prev->next = cur->next;
				}
				if(cur)
				{
					GxCore_Free(cur);
				}
				break;
			}
			prev = cur;
			cur = cur->next;
		}
	}
}

#endif
