#ifndef __APP_NETAPPS_EVENT_H__
#define __APP_NETAPPS_EVENT_H__

typedef enum
{
	PLAYER_SOURCE_DVB = 0,
	PLAYER_SOURCE_FILE = 1,
	PLAYER_SOURCE_NET = 2,
}PlayerSource;

typedef enum
{
	PLAYER_EVENT_STOP = 0,
	PLAYER_EVENT_START = 1,
	PLAYER_EVENT_PAUSE = 2,
	PLAYER_EVENT_RUNNING = 3 ,
	PLAYER_EVENT_END = 4 ,
	PLAYER_EVENT_ERROR = 5,
	PLAYER_EVENT_BUFFERING = 6,
	PLAYER_EVENT_RESUME = 7,
	PLAYER_EVENT_SEEK = 8,
}PlayerEvent;

typedef enum
{
	GUI_EVENT_CREATE = 0,
	GUI_EVENT_EXIT = 1,
	GUI_EVENT_FOCUS = 2,
	GUI_EVENT_UNFOCUS = 3 ,
}GuiEvent;

typedef void (*PlayerEventCb)(PlayerSource, PlayerEvent, char *);

typedef void (*GuiEventCb)(GuiEvent, const char*, void*);

unsigned int app_netapps_player_event_malloc(PlayerEventCb callback);

void app_netapps_player_event_free(unsigned int handle);

unsigned int app_netapps_gui_event_malloc(GuiEventCb callback);

void app_netapps_gui_event_free(unsigned int handle);

#endif
