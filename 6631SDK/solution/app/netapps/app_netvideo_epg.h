#ifndef APP_NETVIDEO_EPG_H
#define APP_NETVIDEO_EPG_H
#include "app_config.h"

typedef struct _EpgDayItem
{
	char *day_id;
	char *title;
}EpgDayItem;

typedef struct _EpgDayList
{
	int day_num;
	int cur_day;
	EpgDayItem *day_item;
}EpgDayList;

typedef struct _EpgEventItem
{
	char *epg_id;
	char *start_time;
	char *end_time;
	char *title;
	char *description;
	int playback;
}EpgEventItem;

typedef struct _EpgEventList
{
	int epg_num;
	int cur_epg;
	EpgEventItem *epg_item;
}EpgEventList;

typedef struct _NetappsEpgCtrl
{
	EpgEventList* (*get_short_epg)(char *, int);
	EpgDayList* (*get_epg_days)(char *);
	EpgEventList* (*get_full_epg)(char *, char *);
	char* (*get_epg_url)(char *);
	void (*epg_exit)(void);
}NetappsEpgCtrl;

#endif
