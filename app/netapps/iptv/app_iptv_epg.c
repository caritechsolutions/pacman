#include "app.h"

#ifdef IPTV_EPG_SUPPORT
#include "app_module.h"
#include "app_iptv.h"
#include "../app_netvideo_play.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include <gx_apps.h>

#define IPTV_EPG_MENU	"wnd_iptv_epg"
#define IPTV_EPG_DAY_LIST	"list_iptv_epg_days"
#define IPTV_EPG_EVENT_LIST	"list_iptv_epg_events"
#define IPTV_EPG_TITLE_TEXT "text_iptv_epg_title"
#define IPTV_EPG_TXT_DATE   "text_iptv_epg_date"
#define IPTV_EPG_TXT_TIME   "text_iptv_epg_time"
#define IPTV_EPG_DETAIL_TEXT	"text_iptv_epg_detail"
#define IPTV_EPG_POP_IMG "img_iptv_epg_popup"
#define IPTV_EPG_POP_TXT "txt_iptv_epg_popup"
#define IPTV_EPG_TITLE_LEN (128)
#define IPTV_EPG_DETAIL_LEN (1024)

typedef enum
{
	IPTV_EPG_FOCUS_NONE = 0,
	IPTV_EPG_FOCUS_DAY,
	IPTV_EPG_FOCUS_EVENT,
}IptvEpgFocus;

typedef void(* iptv_epg_thread_func)(void);

static int is_iptv_epg_download = 0;
static unsigned int iptv_epg_data_handle = 0;
static unsigned int iptv_epg_geturl_handle = 0;
static int iptv_epg_daysdata_finish = 0;
static int iptv_epg_eventdata_finish = 0;
static int iptv_epg_msg_on = 0;
static NetappsEpgCtrl s_iptv_epg_ops;
static char *s_iptv_epg_prog_name = NULL;
static char *s_iptv_epg_prog_url = NULL;
static char *s_iptv_epg_detail = NULL;
static char *s_iptv_epg_id = NULL;
static char *iptv_epg_playurl = NULL;
static event_list* iptv_epg_data_timer = NULL;
static event_list* s_iptv_epg_time_timer = NULL;
static event_list* iptv_epg_popup_timer = NULL;
static EpgDayList *s_iptv_epg_day = NULL;
static EpgEventList *s_iptv_full_epg = NULL;
static int s_iptv_epg_day_index = -1;
static IptvEpgFocus s_iptv_epg_focus = IPTV_EPG_FOCUS_NONE;

static void app_iptv_epg_day_list_ok_press(void);
SIGNAL_HANDLER int app_iptv_epg_days_list_get_total(GuiWidget *widget, void *usrdata);
SIGNAL_HANDLER int app_iptv_epg_events_list_get_total(GuiWidget *widget, void *usrdata);
extern void app_iptv_play_halt(void);

static void iptv_epg_thread_body(void* arg)
{
	iptv_epg_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (iptv_epg_thread_func)arg;
	if(func)
	{
		func();
	}
}

static void create_iptv_epg_thread(iptv_epg_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("iptvepg", &thread_id, iptv_epg_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static int app_iptv_epg_busy(void)
{
	return is_iptv_epg_download;
}

static void app_iptv_epg_init(char *prog_name, char *url)
{
	if(prog_name&&(strlen(prog_name)>0))
	{
		s_iptv_epg_prog_name = GxCore_Calloc(1, IPTV_EPG_TITLE_LEN);
		snprintf(s_iptv_epg_prog_name, IPTV_EPG_TITLE_LEN-1, "TV Guide / %s", prog_name);
	}
	if(url&&(strlen(url)>0))
	{
		s_iptv_epg_prog_url = GxCore_Strdup(url);
	}
	s_iptv_epg_detail = GxCore_Calloc(1, IPTV_EPG_DETAIL_LEN);
}

static void app_iptv_epg_free(void)
{
	APP_FREE(s_iptv_epg_prog_name);
	APP_FREE(s_iptv_epg_prog_url);
	APP_FREE(s_iptv_epg_detail);
}

static void app_iptv_free_epg_days(void)
{
	if(s_iptv_epg_day)
	{
		app_iptv_free_daylist(s_iptv_epg_day);
		s_iptv_epg_day = NULL;
	}
}

static void app_iptv_free_full_epg(void)
{
	if(s_iptv_full_epg)
	{
		app_iptv_free_epglist(s_iptv_full_epg);
		s_iptv_full_epg = NULL;
	}
}

static void app_iptv_epg_show_title(void)
{
	if(s_iptv_epg_prog_name)
	{
		GUI_SetProperty(IPTV_EPG_TITLE_TEXT, "string", s_iptv_epg_prog_name);
		GUI_SetProperty(IPTV_EPG_TITLE_TEXT, "state", "show");
	}
	else
	{
		GUI_SetProperty(IPTV_EPG_TITLE_TEXT, "state", "hide");
	}
}

static int app_iptv_epg_time_update(void* userdata)
{
	char sys_time_str[24] = {0};
	char time_str_temp[24] = {0};

	g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str));
	// time
	if((strlen(sys_time_str) > 24) || (strlen(sys_time_str) < 5))
	{
		printf("lenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&sys_time_str[strlen(sys_time_str)-5],5);// 00:00
	GUI_SetProperty(IPTV_EPG_TXT_TIME, "string", time_str_temp);

	// ymd
	memcpy(time_str_temp,sys_time_str,strlen(sys_time_str));// 0000/00/00
	time_str_temp[strlen(sys_time_str)-5] = '\0';
	GUI_SetProperty(IPTV_EPG_TXT_DATE, "string", time_str_temp);
	return 0;
}

void app_iptv_epg_time_timer_stop(void)
{
	if(s_iptv_epg_time_timer)
	{
		remove_timer(s_iptv_epg_time_timer);
		s_iptv_epg_time_timer = NULL;
	}
}
void app_iptv_epg_time_timer_reset(void)
{
	app_iptv_epg_time_update(NULL);
	if(reset_timer(s_iptv_epg_time_timer) != 0 )
	{
		s_iptv_epg_time_timer = create_timer(app_iptv_epg_time_update,1000,NULL,TIMER_REPEAT);
	}
}

static void app_iptv_epg_events_hide(void)
{
	GUI_SetProperty(IPTV_EPG_EVENT_LIST, "state", "hide");
}

static void app_iptv_epg_detail_hide(void)
{
	GUI_SetProperty(IPTV_EPG_DETAIL_TEXT, "state", "hide");
}

static void app_iptv_epg_detail_show(EpgEventItem *item)
{
	if(item)
	{
		if(item->description&&(strlen(item->description)>1))
		{
			snprintf(s_iptv_epg_detail, IPTV_EPG_DETAIL_LEN-1, "%s-%s  %s",
				item->start_time,item->end_time,item->description);
		}
		else if(item->title&&(strlen(item->title)>1))
		{
			snprintf(s_iptv_epg_detail, IPTV_EPG_DETAIL_LEN-1, "%s-%s  %s",
				item->start_time,item->end_time,item->title);
		}
		else
		{
			snprintf(s_iptv_epg_detail, IPTV_EPG_DETAIL_LEN-1, "%s-%s  %s",
				item->start_time,item->end_time, STR_ID_NO_INFO);
		}
		GUI_SetProperty(IPTV_EPG_DETAIL_TEXT, "string", s_iptv_epg_detail);
		GUI_SetProperty(IPTV_EPG_DETAIL_TEXT, "state", "show");
	}
}

static void app_iptv_epg_tip_hide(void)
{
	iptv_epg_msg_on = 0;
	GUI_SetProperty(IPTV_EPG_POP_IMG, "state", "hide");
	GUI_SetProperty(IPTV_EPG_POP_TXT, "state", "hide");
	if(iptv_epg_popup_timer)
	{
		remove_timer(iptv_epg_popup_timer);
		iptv_epg_popup_timer = NULL;
	}
}

static int iptv_epg_popup_timer_timeout(void *userdata)
{
    iptv_epg_popup_timer = NULL;
	app_iptv_epg_tip_hide();
	return 0;
}

static void app_iptv_epg_tip_show(char *buf, int time_out)
{
	iptv_epg_msg_on = 1;
	GUI_SetProperty(IPTV_EPG_POP_IMG, "state", "show");
	GUI_SetProperty(IPTV_EPG_POP_TXT, "string", buf);
	GUI_SetProperty(IPTV_EPG_POP_TXT, "state", "show");
	if(time_out > 0)
	{
		if(reset_timer(iptv_epg_popup_timer) != 0)
		{
			iptv_epg_popup_timer = create_timer(iptv_epg_popup_timer_timeout, time_out, NULL, TIMER_ONCE);
		}
	}
}

static void app_iptv_epg_day_refresh_stop(void)
{
	iptv_epg_daysdata_finish = 0;
}
static void app_iptv_epg_day_refresh_start(void)
{
	iptv_epg_daysdata_finish = 1;
}

static void app_iptv_epg_event_refresh_stop(void)
{
	iptv_epg_eventdata_finish = 0;
}
static void app_iptv_epg_event_refresh_start(void)
{
	iptv_epg_eventdata_finish = 1;
}

static void app_iptv_epg_update_days(void)
{
	int sel = 0;
	if(s_iptv_epg_day&&(s_iptv_epg_day->day_num>0))
	{
		GUI_SetProperty(IPTV_EPG_DAY_LIST, "state", "show");
		GUI_SetProperty(IPTV_EPG_DAY_LIST, "update_all", NULL);
        GUI_SetFocusWidget(IPTV_EPG_DAY_LIST);
		if((s_iptv_epg_day->cur_day>=0)&&(s_iptv_epg_day->cur_day<s_iptv_epg_day->day_num))
		{
			sel = s_iptv_epg_day->cur_day;
		}
		GUI_SetProperty(IPTV_EPG_DAY_LIST, "select", &sel);
		s_iptv_epg_focus = IPTV_EPG_FOCUS_DAY;
	}
	else
	{
		app_iptv_epg_tip_show(STR_ID_NO_INFO, 1000);
	}
}

static void app_iptv_epg_update_events(void)
{
	int sel = 0;
	if(s_iptv_full_epg&&(s_iptv_full_epg->epg_num>0))
	{
		GUI_SetProperty(IPTV_EPG_EVENT_LIST, "state", "show");
		GUI_SetProperty(IPTV_EPG_EVENT_LIST, "update_all", NULL);
		if((s_iptv_full_epg->cur_epg>=0)&&(s_iptv_full_epg->cur_epg<s_iptv_full_epg->epg_num))
		{
			sel = s_iptv_full_epg->cur_epg;
		}
		GUI_SetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
	}
	else
	{
		app_iptv_epg_tip_show(STR_ID_NO_INFO, 1000);
	}
}

static int iptv_epg_days_timer_timeout(void)
{
	app_iptv_epg_tip_hide();
	app_iptv_epg_update_days();
	app_iptv_epg_day_refresh_stop();
	app_iptv_epg_day_list_ok_press();
	return 0;
}

static int iptv_epg_event_timer_timeout(void)
{
	app_iptv_epg_tip_hide();
	app_iptv_epg_update_events();
	app_iptv_epg_event_refresh_stop();
	return 0;
}

static void app_iptv_epg_data_timer_stop(void)
{
	if(iptv_epg_data_timer)
	{
		remove_timer(iptv_epg_data_timer);
		iptv_epg_data_timer = NULL;
	}
}

static int iptv_epg_data_timer_timeout(void *usrdata)
{
	if(iptv_epg_daysdata_finish)
	{
		iptv_epg_days_timer_timeout();
	}
	if(iptv_epg_eventdata_finish)
	{
		iptv_epg_event_timer_timeout();
	}
	return 0;
}

static void app_iptv_epg_data_timer_start(void)
{
	app_iptv_epg_data_timer_stop();
	iptv_epg_data_timer = create_timer(iptv_epg_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void iptv_epg_start_get_daysdata(void)
{
	unsigned int handle = 0;
	EpgDayList *data = NULL;

	is_iptv_epg_download = 1;
	iptv_epg_data_handle++;
	handle = iptv_epg_data_handle;
	if(s_iptv_epg_ops.get_epg_days&&s_iptv_epg_ops.get_full_epg&&s_iptv_epg_prog_url)
	{
		data = s_iptv_epg_ops.get_epg_days(s_iptv_epg_prog_url);
	}

	if(handle == iptv_epg_data_handle)
	{
		if(data)
		{
			s_iptv_epg_day = data;
		}
		app_iptv_epg_day_refresh_start();
	}
	else
	{
		if(data)
		{
			app_iptv_free_daylist(data);
		}
	}
	is_iptv_epg_download = 0;
}

static void iptv_epg_start_get_fullepg_data(void)
{
	unsigned int handle = 0;
	EpgEventList *data = NULL;

	is_iptv_epg_download = 1;
	iptv_epg_data_handle++;
	handle = iptv_epg_data_handle;
	if(s_iptv_epg_ops.get_full_epg&&s_iptv_epg_prog_url)
	{
		if((s_iptv_epg_day_index>=0)&&s_iptv_epg_day&&(s_iptv_epg_day->day_num>0)&&(s_iptv_epg_day_index<s_iptv_epg_day->day_num))
		{
			data = s_iptv_epg_ops.get_full_epg(s_iptv_epg_prog_url, s_iptv_epg_day->day_item[s_iptv_epg_day_index].day_id);
		}
	}

	if(handle == iptv_epg_data_handle)
	{
		if(data)
		{
			s_iptv_full_epg = data;
		}
		app_iptv_epg_event_refresh_start();
	}
	else
	{
		if(data)
		{
			app_iptv_free_epglist(data);
		}
	}
	is_iptv_epg_download = 0;
}

static void app_iptv_epg_days_update(void)
{
	GUI_SetInterface("flush",NULL);
	app_iptv_epg_tip_show("Updating...", 0);
	create_iptv_epg_thread(iptv_epg_start_get_daysdata);
}

static void app_iptv_epg_day_list_right_press(void)
{
	int sel = -1;

	if(app_iptv_epg_busy())
	{
		app_iptv_epg_tip_show("System Busy, Please Wait!", 1000);
		return;
	}

	if(s_iptv_full_epg&&(s_iptv_full_epg->epg_num>0))
	{
        GUI_SetFocusWidget(IPTV_EPG_EVENT_LIST);
		s_iptv_epg_focus = IPTV_EPG_FOCUS_EVENT;
		GUI_GetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
		if((sel>=0)&&(sel<s_iptv_full_epg->epg_num))
		{
			app_iptv_epg_detail_show(&s_iptv_full_epg->epg_item[sel]);
		}
	}
}

static void app_iptv_epg_day_list_ok_press(void)
{
	int sel = 0;

	if(app_iptv_epg_busy())
	{
		app_iptv_epg_tip_show("System Busy, Please Wait!", 1000);
		return;
	}

	GUI_GetProperty(IPTV_EPG_DAY_LIST, "select", &sel);
	if((sel>=0)&&s_iptv_epg_day&&(s_iptv_epg_day->day_num>0)&&(sel<s_iptv_epg_day->day_num))
	{
		app_iptv_epg_events_hide();
		app_iptv_epg_detail_hide();
		app_iptv_free_full_epg();
		GUI_SetProperty(IPTV_EPG_DAY_LIST, "active", &sel);
		s_iptv_epg_day_index = sel;
		app_iptv_epg_tip_show("Updating...", 0);
		create_iptv_epg_thread(iptv_epg_start_get_fullepg_data);
	}
}

static void app_iptv_epg_event_list_left_press(void)
{
	if(app_iptv_epg_busy())
	{
		app_iptv_epg_tip_show("System Busy, Please Wait!", 1000);
		return;
	}
	GUI_SetProperty(IPTV_EPG_EVENT_LIST, "stop_rolling", NULL);
	GUI_SetFocusWidget(IPTV_EPG_DAY_LIST);
	s_iptv_epg_focus = IPTV_EPG_FOCUS_DAY;
	app_iptv_epg_detail_hide();
}

static void app_iptv_epg_play_exit_ctrl(void)
{
	iptv_epg_geturl_handle++;
	app_iptv_epg_time_timer_reset();
	GUI_SetProperty(IPTV_EPG_EVENT_LIST, "enable_roll", NULL);
}

static void app_iptv_epg_play_restart_ctrl(uint64_t cur_time)
{
	app_net_video_start_play(iptv_epg_playurl, PLAY_STATE_LOAD, cur_time, false);
}

static void app_iptv_epg_play_get_url(int source_index)
{
	unsigned int handle = 0;
	char *url = NULL;

	iptv_epg_geturl_handle++;
	handle = iptv_epg_geturl_handle;
	APP_FREE(iptv_epg_playurl);
	if(s_iptv_epg_id)
	{
		url = s_iptv_epg_ops.get_epg_url(s_iptv_epg_id);
	}

	if(handle != iptv_epg_geturl_handle)
	{
		printf("player has exited!\n");
		APP_FREE(url);
		return;
	}

	iptv_epg_playurl = url;
	if(iptv_epg_playurl)
	{
		app_net_video_start_play(iptv_epg_playurl, PLAY_STATE_LOAD, 0, false);
	}
	else
	{
		app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}
}

static status_t app_iptv_epg_play(char *prog_name)
{
	NetVideoPlayOps play_ops;
	memset(&play_ops, 0, sizeof(NetVideoPlayOps));

	play_ops.get_url             = app_iptv_epg_play_get_url;
	play_ops.netvideo_title         = prog_name;
	play_ops.play_ctrl.play_ok   = NULL;
	play_ops.play_ctrl.play_exit = app_iptv_epg_play_exit_ctrl;
	play_ops.play_ctrl.play_prev = NULL;
	play_ops.play_ctrl.play_next = NULL;
	play_ops.play_ctrl.play_restart = app_iptv_epg_play_restart_ctrl;

	return app_epg_playback(&play_ops);
}

static void app_iptv_epg_event_list_ok_press(int sel)
{
	if(s_iptv_full_epg&&(s_iptv_full_epg->epg_num>0)&&(sel<s_iptv_full_epg->epg_num)
		&&(s_iptv_full_epg->epg_item[sel].epg_id)&&(s_iptv_full_epg->epg_item[sel].playback>0))
	{
		if(app_iptv_epg_busy())
		{
			app_iptv_epg_tip_show("System Busy, Please Wait!", 1000);
			return;
		}
		GUI_SetProperty(IPTV_EPG_EVENT_LIST, "stop_rolling", NULL);
		app_iptv_epg_time_timer_stop();
		APP_FREE(s_iptv_epg_id);
		s_iptv_epg_id = GxCore_Strdup(s_iptv_full_epg->epg_item[sel].epg_id);
		app_iptv_epg_play(s_iptv_full_epg->epg_item[sel].title);
	}
}

SIGNAL_HANDLER int app_iptv_epg_create(GuiWidget *widget, void *usrdata)
{
	iptv_epg_msg_on = 0;
	s_iptv_epg_day_index = -1;
	s_iptv_epg_focus = IPTV_EPG_FOCUS_NONE;
	app_iptv_epg_data_timer_start();
	app_iptv_epg_show_title();
	app_iptv_epg_time_timer_reset();
	app_iptv_epg_days_update();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_epg_destroy(GuiWidget *widget, void *usrdata)
{
	if(s_iptv_epg_ops.epg_exit)
	{
		s_iptv_epg_ops.epg_exit();
	}
	is_iptv_epg_download = 0;
	iptv_epg_data_handle++;
	app_iptv_epg_free();
	app_iptv_free_epg_days();
	app_iptv_free_full_epg();
	APP_FREE(s_iptv_epg_id);
	APP_FREE(iptv_epg_playurl);
	app_iptv_epg_day_refresh_stop();
	app_iptv_epg_event_refresh_stop();
    app_iptv_epg_time_timer_stop();
	app_iptv_epg_data_timer_stop();
	app_iptv_epg_tip_hide();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_epg_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;

			case STBK_EXIT:
			case STBK_MENU:
				GUI_EndDialog(IPTV_EPG_MENU);
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_epg_days_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	int sel = 0;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_RIGHT:
				app_iptv_epg_day_list_right_press();
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_OK:
				app_iptv_epg_day_list_ok_press();
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_PAGE_UP:
				sel = 0;
				GUI_GetProperty(IPTV_EPG_DAY_LIST, "select", &sel);
				if(0 == sel)
				{
					sel = app_iptv_epg_days_list_get_total(NULL, NULL) - 1;
					GUI_SetProperty(IPTV_EPG_DAY_LIST, "select", &sel);
					ret = EVENT_TRANSFER_STOP;
				}
				else
				{
					ret = EVENT_TRANSFER_KEEPON;
				}
				break;

			case STBK_PAGE_DOWN:
				sel = 0;
				GUI_GetProperty(IPTV_EPG_DAY_LIST, "select", &sel);
				if(sel == (app_iptv_epg_days_list_get_total(NULL, NULL) - 1))
				{
					sel = 0;
					GUI_SetProperty(IPTV_EPG_DAY_LIST, "select", &sel);
					ret = EVENT_TRANSFER_STOP;
				}
				else
				{
					ret = EVENT_TRANSFER_KEEPON;
				}
				break;

			default:
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_iptv_epg_days_list_get_total(GuiWidget *widget, void *usrdata)
{
	int total = 0;

	if(s_iptv_epg_day&&(s_iptv_epg_day->day_num>0))
	{
		total = s_iptv_epg_day->day_num;
	}
	return total;
}

SIGNAL_HANDLER int app_iptv_epg_days_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_STOP;
	if(item->sel<0)
		return EVENT_TRANSFER_STOP;

	if(s_iptv_epg_day&&(s_iptv_epg_day->day_num>0)&&(item->sel<s_iptv_epg_day->day_num))
	{
		item->image = NULL;
		item->string = s_iptv_epg_day->day_item[item->sel].title;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_epg_days_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_epg_events_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;
	int sel = 0;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_OK:
				if(s_iptv_epg_ops.get_epg_url)
				{
					sel = 0;
					GUI_GetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
					app_iptv_epg_event_list_ok_press(sel);
					ret = EVENT_TRANSFER_STOP;
				}
				break;

			case STBK_LEFT:
				app_iptv_epg_event_list_left_press();
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_PAGE_UP:
				sel = 0;
				GUI_GetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
				if(0 == sel)
				{
					sel = app_iptv_epg_events_list_get_total(NULL, NULL) - 1;
					GUI_SetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
					ret = EVENT_TRANSFER_STOP;
				}
				else
				{
					ret = EVENT_TRANSFER_KEEPON;
				}
				break;

			case STBK_PAGE_DOWN:
				sel = 0;
				GUI_GetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
				if(sel == (app_iptv_epg_events_list_get_total(NULL, NULL) - 1))
				{
					sel = 0;
					GUI_SetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
					ret = EVENT_TRANSFER_STOP;
				}
				else
				{
					ret = EVENT_TRANSFER_KEEPON;
				}
				break;

			default:
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_iptv_epg_events_list_get_total(GuiWidget *widget, void *usrdata)
{
	int total = 0;

	if(s_iptv_full_epg&&(s_iptv_full_epg->epg_num>0))
	{
		total = s_iptv_full_epg->epg_num;
	}
	return total;
}

SIGNAL_HANDLER int app_iptv_epg_events_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_STOP;
	if(item->sel<0)
		return EVENT_TRANSFER_STOP;

	if(s_iptv_full_epg&&(s_iptv_full_epg->epg_num>0)&&(item->sel<s_iptv_full_epg->epg_num))
	{
		//time
		item->image = NULL;
		item->string = s_iptv_full_epg->epg_item[item->sel].start_time;

		//playback  flag
		if(s_iptv_full_epg->epg_item[item->sel].playback>0)
		{
			item->next->image = "s_ts_blue";
		}
		else
		{
			item->next->image = NULL;
		}
		item->next->string = NULL;

		//title
		item->next->next->image = NULL;
		item->next->next->string = s_iptv_full_epg->epg_item[item->sel].title;

	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_epg_events_list_change(GuiWidget *widget, void *usrdata)
{
	int sel = -1;

	if(s_iptv_epg_focus == IPTV_EPG_FOCUS_EVENT)
	{
		GUI_GetProperty(IPTV_EPG_EVENT_LIST, "select", &sel);
		if(s_iptv_full_epg&&(s_iptv_full_epg->epg_num>0)&&(sel<s_iptv_full_epg->epg_num))
		{
			app_iptv_epg_detail_show(&s_iptv_full_epg->epg_item[sel]);
		}
	}
	return EVENT_TRANSFER_STOP;
}

void app_create_iptv_epg_menu(char *prog_name, char *prog_url, NetappsEpgCtrl *ops)
{
	app_iptv_epg_init(prog_name, prog_url);
	memset(&s_iptv_epg_ops, 0, sizeof(NetappsEpgCtrl));
	memcpy(&s_iptv_epg_ops, ops, sizeof(NetappsEpgCtrl));

	if(s_iptv_epg_ops.get_epg_days&&s_iptv_epg_ops.get_full_epg)
	{
		if(GUI_CheckDialog(IPTV_EPG_MENU) == GXCORE_ERROR)
		{
			GUI_CreateDialog(IPTV_EPG_MENU);
		}
	}
}

#else
SIGNAL_HANDLER int app_iptv_epg_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_days_list_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_days_list_get_total(GuiWidget *widget, void *usrdata)
{
	return 0;
}
SIGNAL_HANDLER int app_iptv_epg_days_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_days_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_events_list_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_events_list_get_total(GuiWidget *widget, void *usrdata)
{
	return 0;
}
SIGNAL_HANDLER int app_iptv_epg_events_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_iptv_epg_events_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#endif

