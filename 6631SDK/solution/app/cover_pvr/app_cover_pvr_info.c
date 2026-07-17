#include "app_config.h"
#if PVR_RECORDING_POSTER_STYLE
#include "app_windows.h"
#include "app_cover_pvr.h"
#include "app_utility.h"

#define NO_EVNET_NAME_STR               "-"   // STR_ID_UNKNOW

typedef struct {
	const char *wnd;

	const char *event_logo;
	const char *event_title;
	const char *event_parent;
	const char *event_genres;
	const char *event_date;
	const char *event_duration;
	const char *event_details;
} CoverPvrChMoreWidget;

typedef struct {
    CoverPvrChMoreWidget wgt;
    play_record_info info;
    bool reverse_flag;
}CoverPvrInfoCtrl;

static CoverPvrInfoCtrl s_cover_pvr_info_ctrl;

static char *_cover_pvr_info_date_str_get(time_t sec, bool full_flag)
{
	static char buffer[16];
	char *date_str = NULL;

	memset(buffer, 0, sizeof(buffer));
	if (full_flag)
		date_str = app_time_to_date_edit_str(sec);
	else
		date_str = app_time_to_short_date_edit_str(sec);
	if (date_str)
		strncpy(buffer, date_str, sizeof(buffer) - 1);
	APP_FREE(date_str);

	return buffer;
}

static char *_cover_pvr_info_duration_str_get(time_t start_time, time_t finish_time, uint32_t flag)
{
	static char buffer[32];
	char *start_str = NULL;
	char *finish_str = NULL;
	char *date_str = NULL;
	char *join_str = NULL;
    CoverPvrInfoCtrl *ctrl = &s_cover_pvr_info_ctrl;

	memset(buffer, 0, sizeof(buffer));
	join_str = " - ";
	start_str = app_time_to_hourmin_edit_str(start_time);
	finish_str = app_time_to_hourmin_edit_str(finish_time);
	if (!start_str && !finish_str)
		goto end;

    if (ctrl->reverse_flag)
        snprintf(buffer, sizeof(buffer), "%s%s%s", finish_str, join_str, start_str);
    else
        snprintf(buffer, sizeof(buffer), "%s%s%s", start_str, join_str, finish_str);

end:
	APP_FREE(start_str);
	APP_FREE(finish_str);
	APP_FREE(date_str);
	return buffer;
}

static void _cover_pvr_info_widget_init(void)
{
	CoverPvrInfoCtrl *ctrl = &s_cover_pvr_info_ctrl;
	CoverPvrChMoreWidget *widget = &ctrl->wgt;

	widget->wnd = WND_COVER_PVR_INFO;
	widget->event_logo = "img_cover_pvr_info_event_logo";
	widget->event_title = "text_cover_pvr_info_event_title";
	widget->event_parent = "text_cover_pvr_info_event_parent";
	widget->event_genres = "text_cover_pvr_info_event_genres";
	widget->event_date = "text_cover_pvr_info_event_date";
	widget->event_duration = "text_cover_pvr_info_event_duration";
	widget->event_details = "notepad_cover_pvr_info_event_details";
}

static int _cover_pvr_show_event_info(play_record_info *event)
{
#define HOURMIN_STR_LEN     6
	CoverPvrInfoCtrl *ctrl = &s_cover_pvr_info_ctrl;
	char buffer[8] = {0};
	char *tmp_str = NULL;

	if (event->event_title && strlen(event->event_title) > 0)
		tmp_str = event->event_title;
	else
		tmp_str = NO_EVNET_NAME_STR;
	GUI_SetProperty(ctrl->wgt.event_title, "string", tmp_str);

	if (event->genres_str && strlen(event->genres_str) > 0)
		tmp_str = event->genres_str;
	else
		tmp_str = STR_ID_BLANK;
	GUI_SetProperty(ctrl->wgt.event_genres, "string", tmp_str);

	if (event->parent_rate > 0)
	{
		snprintf(buffer, sizeof(buffer), "%d", event->parent_rate);
		tmp_str = buffer;
	}
	else
	{
		tmp_str = STR_ID_BLANK;
	}
	GUI_SetProperty(ctrl->wgt.event_parent, "string", tmp_str);

	tmp_str = _cover_pvr_info_date_str_get(event->start_time, true);;
	GUI_SetProperty(ctrl->wgt.event_date, "string", tmp_str);

	tmp_str = _cover_pvr_info_duration_str_get(event->start_time, event->finish_time, 0);
	GUI_SetProperty(ctrl->wgt.event_duration, "string", tmp_str);

	if (event->event_logo && strlen(event->event_logo) > 0)
	{
		gal_add_key_path(COVER_PVR_EVENT_KEY, event->event_logo);
		GUI_SetProperty(ctrl->wgt.event_logo, "img", COVER_PVR_EVENT_KEY);
	}
	else
	{
		GUI_SetProperty(ctrl->wgt.event_logo, "img", COVER_PVR_EVENT_KEY_DF);
	}

	if (event->event_desc && strlen(event->event_desc) > 0)
		tmp_str = event->event_desc;

	GUI_SetProperty(ctrl->wgt.event_details, "string", tmp_str);

	return 0;
}

int app_cover_pvr_info_create_dialog(bool reverse_flag)
{
	CoverPvrInfoCtrl *ctrl = &s_cover_pvr_info_ctrl;
	if (GUI_CheckDialog(WND_COVER_PVR) == GXCORE_SUCCESS)
	{
		play_movie_info cur_info = {0};

		play_movie_get_info(&cur_info);
		if (app_cover_pvr_event_info_get(cur_info.no, cur_info.name, true, &ctrl->info) < 0)
			return -1;

		_cover_pvr_info_widget_init();
		GUI_CreateDialog(ctrl->wgt.wnd);
		_cover_pvr_show_event_info(&ctrl->info);
		app_cover_pvr_event_info_free(&ctrl->info);
        ctrl->reverse_flag = reverse_flag;
		return 0;
	}

	return -1;
}

static int _cover_pvr_info_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

static int _cover_pvr_info_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

static int _cover_pvr_info_keypress(GuiWidget *widget, void *usrdata)
{
	CoverPvrInfoCtrl *ctrl = &s_cover_pvr_info_ctrl;
	GUI_Event *event = NULL;
	uint32_t value = 1;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;
			case STBK_EXIT:
			case STBK_MENU:
				GUI_EndDialog(ctrl->wgt.wnd);
				break;
			case STBK_UP:
				GUI_SetProperty(ctrl->wgt.event_details, "line_up", &value);
				break;
			case STBK_DOWN:
				GUI_SetProperty(ctrl->wgt.event_details, "line_down", &value);
				break;
			case STBK_PAGE_UP:
				GUI_SetProperty(ctrl->wgt.event_details, "page_up", &value);
				break;
			case STBK_PAGE_DOWN:
				GUI_SetProperty(ctrl->wgt.event_details, "page_down", &value);
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

static int _cover_pvr_info_service(GuiWidget *widget, void *usrdata)
{
    CoverPvrInfoCtrl *ctrl = &s_cover_pvr_info_ctrl;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	GUI_SendEvent(WIN_MOVIE_VIEW, event);
	GUI_SetProperty(ctrl->wgt.wnd, "update", NULL);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cover_pvr_info_create(GuiWidget *widget, void *usrdata)
{
	return _cover_pvr_info_create(widget, usrdata);
}

SIGNAL_HANDLER int app_cover_pvr_info_destroy(GuiWidget *widget, void *usrdata)
{
	return _cover_pvr_info_destroy(widget, usrdata);
}

SIGNAL_HANDLER int app_cover_pvr_info_keypress(GuiWidget *widget, void *usrdata)
{
	return _cover_pvr_info_keypress(widget, usrdata);
}

SIGNAL_HANDLER int app_cover_pvr_info_service(GuiWidget *widget, void *usrdata)
{
	return _cover_pvr_info_service(widget, usrdata);
}
#endif
