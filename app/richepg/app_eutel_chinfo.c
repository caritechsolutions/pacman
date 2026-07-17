#include "app.h"
#include "app_windows.h"
#include "full_screen.h"
#include "include/app_module.h"
#include "app_channel_info.h"
#if RICHEPG_SUPPORT
#include "app_eutel_control.h"
#include "app_eutel_common.h"
#include "richepg_epg.h"
#define IMG_STATE                       "img_full_state"
#define TXT_STATE                       "txt_full_state"

static int s_timer_cnt = 0;
static event_list* s_eutel_chinfo_timer = NULL;
static time_t s_chinfo_epg_start_time_bak = -1;

typedef struct {
	const char *wnd;

	const char *sys_date;
	const char *sys_time;
	const char *ch_logo;
	const char *ch_name;
	const char *ch_num;
	const char *cur_time;
	const char *cur_event;
	const char *next_time;
	const char *next_event;

	const char *lineup_logo;
	const char *lineup_title;
	const char *lineup_name;
	const char *genres_title;
	const char *genres_name;
	const char *img_ttx;
	const char *img_subt;
	const char *img_hd;
	const char *img_lock;
	const char *img_scrb;
} EutelChinfoWidget;

static EutelChinfoWidget s_eutel_chinfo_wgt;

void app_eutel_chinfo_widget_init(void)
{
	EutelChinfoWidget *widget = &s_eutel_chinfo_wgt;

	widget->wnd = WND_EUTEL_CHINFO;

	widget->sys_date = "text_eutel_chinfo_date";
	widget->sys_time = "text_eutel_chinfo_time";
	widget->ch_logo = "img_eutel_chinfo_logo";
	widget->ch_name = "text_eutel_chinfo_name";
	widget->ch_num = "text_eutel_chinfo_num";
	widget->cur_time = "text_eutel_chinfo_cur_time";
	widget->cur_event = "text_eutel_chinfo_cur_event";
	widget->next_time = "text_eutel_chinfo_next_time";
	widget->next_event = "text_eutel_chinfo_next_event";

	widget->lineup_logo = "img_eutel_chinfo_lineup";
	widget->lineup_title = "text_eutel_chinfo_lineup_title";
	widget->lineup_name = "text_eutel_chinfo_lineup";
	widget->genres_title = "text_eutel_chinfo_genres_title";
	widget->genres_name = "text_eutel_chinfo_genres";

	widget->img_ttx = "img_eutel_chinfo_ttx";
	widget->img_subt = "img_eutel_chinfo_subt";
	widget->img_hd = "img_eutel_chinfo_hd";
	widget->img_lock = "img_eutel_chinfo_lock";
	widget->img_scrb = "img_eutel_chinfo_scramble";
}

int app_eutel_chinfo_create_dialog(void)
{
	app_eutel_chinfo_widget_init();
	GUI_CreateDialog(s_eutel_chinfo_wgt.wnd);
	return 0;
}

static void _eutel_chinfo_show_flag_img(GxMsgProperty_NodeByPosGet *node_prog, bool force_update)
{
#define  IMG_TOTAL      (5)
	EutelChinfoWidget *wgt= &s_eutel_chinfo_wgt;
	int i = 0;
	const char* flag_img_widget[IMG_TOTAL] = {wgt->img_ttx, wgt->img_subt, wgt->img_hd, wgt->img_lock, wgt->img_scrb};
	char *image_flag_focus[IMG_TOTAL] = {"s_bar_ttx", "s_bar_subt", "s_bar_hd", "s_bar_lock", "s_bar_money"};
	char *image_flag_unfocus[IMG_TOTAL] = {"s_bar_ttx_unfocus", "s_bar_subt_unfocus", "s_bar_hd_unfocus",
										   "s_bar_lock_unfocus", "s_bar_money_unfocus"};
	static GxBusPmDataProgSwitch s_flag_type_bak[IMG_TOTAL] = {0};
	GxBusPmDataProgSwitch flag_type[IMG_TOTAL] = {0};

	flag_type[0] = node_prog->prog_data.ttx_flag;
	flag_type[1] = node_prog->prog_data.subt_flag | node_prog->prog_data.cc_flag;
	flag_type[2] = (GXBUS_PM_PROG_HD == node_prog->prog_data.definition) ? GXBUS_PM_PROG_BOOL_ENABLE : GXBUS_PM_PROG_BOOL_DISABLE;
	flag_type[3] = node_prog->prog_data.lock_flag;
	flag_type[4] = node_prog->prog_data.scramble_flag;

	if (force_update) {
		for (i=0; i < IMG_TOTAL; i++)
		{
			if (flag_type[i] == GXBUS_PM_PROG_BOOL_ENABLE)
				GUI_SetProperty(flag_img_widget[i], "img", image_flag_focus[i]);
			else
				GUI_SetProperty(flag_img_widget[i], "img", image_flag_unfocus[i]);
		}
	} else {
		for (i=0; i < IMG_TOTAL; i++)
		{
			if (flag_type[i] != s_flag_type_bak[i])
			{
				if (flag_type[i] == GXBUS_PM_PROG_BOOL_ENABLE)
					GUI_SetProperty(flag_img_widget[i], "img", image_flag_focus[i]);
				else
					GUI_SetProperty(flag_img_widget[i], "img", image_flag_unfocus[i]);
			}
		}
	}
	memcpy(s_flag_type_bak, flag_type, IMG_TOTAL * sizeof(GxBusPmDataProgSwitch));
}

void app_eutel_chinfo_show_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition)
{
	if (GUI_CheckDialog(WND_EUTEL_CHINFO) == GXCORE_SUCCESS)
			//|| GUI_CheckDialog(WND_EUTEL_RCHINFO) == GXCORE_SUCCESS)
	{
		if((video.height > 576) && (video.width > 720))
		{
			GUI_SetProperty(s_eutel_chinfo_wgt.img_hd, "img", "s_bar_hd");
		}
		else
		{
			if(definition != GXBUS_PM_PROG_HD)
			{
				GUI_SetProperty(s_eutel_chinfo_wgt.img_hd, "img", "s_bar_hd_unfocus");
			}
		}
	}

	if (GUI_CheckDialog(WND_EUTEL_CHMORE) == GXCORE_SUCCESS)
          //	|| GUI_CheckDialog(WND_EUTEL_RCHMORE) == GXCORE_SUCCESS)
	{
		void app_eutel_chmore_show_format_info(GxMsgProperty_NodeByPosGet *node_prog, const char *resolution_str);
		char solution_str[15] = {0};
		snprintf(solution_str, sizeof(solution_str), "%d x %d", video.width, video.height);
		app_eutel_chmore_show_format_info(NULL, solution_str);
	}
}

void app_eutel_chinfo_clear_epg_info(void)
{
	if (GUI_CheckDialog(WND_EUTEL_CHINFO) == GXCORE_SUCCESS)
			//|| GUI_CheckDialog(WND_EUTEL_RCHINFO) == GXCORE_SUCCESS)
	{
		EutelChinfoWidget *wgt= &s_eutel_chinfo_wgt;
#if 0
		GUI_SetProperty(wgt->cur_time, "string", STR_ID_ZERO_DURATION);
		GUI_SetProperty(wgt->cur_event, "string", STR_ID_NO_INFO);
		GUI_SetProperty(wgt->next_time, "string", STR_ID_ZERO_DURATION);
		GUI_SetProperty(wgt->next_event, "string", STR_ID_NO_INFO);
#else
		GUI_SetProperty(wgt->cur_time, "string", "-");
		GUI_SetProperty(wgt->cur_event, "string", "-");
		GUI_SetProperty(wgt->next_time, "string", "-");
		GUI_SetProperty(wgt->next_event, "string", "-");
#endif
	}
}

static void _eutel_chinfo_show_epg(GxMsgProperty_NodeByPosGet *node_prog)
{
	int ret = 0;
	char *cur_epg_time = NULL;
	char *cur_epg_str = NULL;
	char *next_epg_time = NULL;
	char *next_epg_str = NULL;
	EutelChinfoWidget *wgt= &s_eutel_chinfo_wgt;

	ret = app_eutel_epg_cur_next_epg_info_str_get(&cur_epg_time, &cur_epg_str, &next_epg_time, &next_epg_str);
	if (ret == GXCORE_SUCCESS)
	{
		app_set_widget_string(wgt->cur_time, cur_epg_time);
		app_set_widget_string(wgt->cur_event, cur_epg_str);
		app_set_widget_string(wgt->next_time, next_epg_time);
		app_set_widget_string(wgt->next_event, next_epg_str);
	}
}

static void _eutel_chinfo_show(bool force_update)
{
	EutelChinfoWidget *wgt= &s_eutel_chinfo_wgt;
	GxMsgProperty_NodeByPosGet node_prog = {0};

	node_prog.node_type = NODE_PROG;
	node_prog.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

	_eutel_chinfo_show_flag_img(&node_prog, force_update);  // flag image
	_eutel_chinfo_show_epg(&node_prog);       // epg
	app_show_sys_time_and_date(wgt->sys_time, wgt->sys_date);
}

static int eutel_chinfo_timer_exec(void *userdata)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EUTEL_COLLECT))
        return 0;

	if((g_AppFullArb.state.tv == STATE_OFF)
			|| (GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
			|| (GUI_CheckDialog(WND_EUTEL_CHMORE) == GXCORE_SUCCESS))
			//|| (GUI_CheckDialog(WND_EUTEL_RCHMORE) == GXCORE_SUCCESS))
	{
		if(GUI_CheckDialog(WND_PVR_BAR) != GXCORE_SUCCESS)
		{
			GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &s_timer_cnt, INFOBAR_TIMEOUT);
			_eutel_chinfo_show(false);
		}
	}
	else
	{
		s_timer_cnt--;
		if(s_timer_cnt == 0)
		{
			APP_TIMER_REMOVE(s_eutel_chinfo_timer);
			GUI_EndDialog(s_eutel_chinfo_wgt.wnd);
		}
		else
		{
			_eutel_chinfo_show(false);
		}
	}

	return 0;
}

static void _eutel_chinfo_show_once(void)
{
#define RICHEPG_STORE_LEN 256
	EutelChinfoWidget *wgt= &s_eutel_chinfo_wgt;
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	EutelChannelArg *arg = NULL;
	char buffer[RICHEPG_STORE_LEN];
	char *key = NULL;
	char *tmp_str = NULL;
	int sel = -1;
	GxMsgProperty_NodeByPosGet node_prog = {0};

	sel = eutel_channel_play_use_sel_get();
	node_prog.node_type = NODE_PROG;
	node_prog.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

	// prog_num
	if (sel >= 0)
		snprintf(buffer, RICHEPG_STORE_LEN, "%04d", ctrl->use_array[sel].lcn);
	else
		snprintf(buffer, RICHEPG_STORE_LEN, "%04d", g_AppPlayOps.normal_play.play_count+1);
	app_set_widget_string(wgt->ch_num, buffer); // prog_num

	// prog_logo
	key = eutel_channel_logo_key_get(sel);
	GUI_SetProperty(wgt->ch_logo, "img", key); // prog_logo

	// prog_name
	if (sel >= 0)
		app_set_widget_string(wgt->ch_name, ctrl->arg_array[ctrl->use_array[sel].pos].channel_name);
	else
		app_set_widget_string(wgt->ch_name, (char *)node_prog.prog_data.prog_name);

	// channel genres
	if (sel >= 0)
	{
		arg = &ctrl->arg_array[ctrl->use_array[sel].pos];
		if (arg->genre_num > 0)
		{
			tmp_str = eutel_array_name_get(arg->genre_num, arg->genre_ids, EUTEL_CHANNEL_GENRE);
			app_set_widget_string(wgt->genres_name, tmp_str);
		}
		else
		{
			app_set_widget_string(wgt->genres_name, STR_ID_BLANK);
		}
	}
	else
	{
		app_set_widget_string(wgt->genres_name, STR_ID_BLANK);
	}

	// lineup name
	memset(buffer, 0, RICHEPG_STORE_LEN);
	if (richepg_store_getstr(RICHEPG_LINEUP_NAME, buffer, RICHEPG_STORE_LEN) != NULL)
	{
		app_set_widget_string(wgt->lineup_name, buffer);
	}
	else
	{
		app_set_widget_string(wgt->lineup_name, STR_ID_BLANK);
	}

	// lineup logo
	GUI_SetProperty(wgt->lineup_logo, "img", eutel_lineup_logo_key_get());
}

static void _eutel_chinfo_timeout_reset(void)
{
	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &s_timer_cnt, INFOBAR_TIMEOUT);
	APP_TIMER_ADD(s_eutel_chinfo_timer, eutel_chinfo_timer_exec, 1000, TIMER_REPEAT);
}

void app_eutel_chinfo_update(void)
{
	_eutel_chinfo_show_once();
	_eutel_chinfo_show(true);
	_eutel_chinfo_timeout_reset();
}

int app_eutel_chinfo_show_info(void)
{
	s_chinfo_epg_start_time_bak = -1;
	eutel_chinfo_timer_exec(NULL);
	GUI_SetProperty(s_eutel_chinfo_wgt.wnd, "draw_now", NULL);
	APP_TIMER_ADD(s_eutel_chinfo_timer, eutel_chinfo_timer_exec, 1000, TIMER_REPEAT);

	return 0;
}

static int _eutel_chinfo_create(GuiWidget *widget, void *usrdata)
{
	app_eutel_chinfo_update();
	app_eutel_chinfo_show_info();

	if (GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
	{
		GUI_SetFocusWidget(WND_PASSWD);
	}
	app_eutel_ear_create();
	return EVENT_TRANSFER_STOP;
}

static int _eutel_chinfo_destroy(GuiWidget *widget, void *usrdata)
{
	app_eutel_ear_destroy();
	APP_TIMER_REMOVE(s_eutel_chinfo_timer);
	app_panel_show_prog_num(0);
	return EVENT_TRANSFER_STOP;
}
static int _eutel_chinfo_got_focus(GuiWidget *widget, void *usrdata)
{
	_eutel_chinfo_timeout_reset();
	g_AppFullArb.timer_start();
	return EVENT_TRANSFER_STOP;
}
static int _eutel_chinfo_lost_focus(GuiWidget *widget, void *usrdata)
{
	app_eutel_ear_hide();
	return EVENT_TRANSFER_STOP;
}

static int _eutel_chinfo_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_EXIT:
				case VK_BOOK_TRIGGER:
					{
						if(g_AppFullArb.state.tv == STATE_ON)
						{
							GUI_EndDialog(s_eutel_chinfo_wgt.wnd);
						}
					}
					break;

				case STBK_INFO:
					if(g_AppPvrOps.state == PVR_DUMMY)
					{
						g_AppFullArb.timer_stop();
						GUI_SetProperty(TXT_STATE, "string", " ");
						GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
						GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
						app_eutel_chmore_create_dialog();
					}
					else
					{
						if (s_eutel_chinfo_timer)
							timer_stop(s_eutel_chinfo_timer);
						GUI_CreateDialog(WND_PVR_BAR);
					}
					break;
#if WEBLET_SUPPORT
				case STBK_QR:
					GUI_SendEvent(WND_FULL_SCREEN, event);
					break;
#endif

				default:
					{
						if((g_AppFullArb.state.tv == STATE_ON)
								|| ((event->key.sym == STBK_OK)
									|| (event->key.sym == STBK_SAT)
									|| (event->key.sym == STBK_FAV)
								   ))
						{
							GUI_EndDialog(s_eutel_chinfo_wgt.wnd);
						}
						GUI_SendEvent(WND_FULL_SCREEN, event);
					}
					break;
			}
		default:
			break;
	}

	return ret;

}

SIGNAL_HANDLER int app_eutel_chinfo_create(GuiWidget *widget, void *usrdata)
{
	return _eutel_chinfo_create(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chinfo_destroy(GuiWidget *widget, void *usrdata)
{
	return _eutel_chinfo_destroy(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chinfo_got_focus(GuiWidget *widget, void *usrdata)
{
	return _eutel_chinfo_got_focus(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chinfo_lost_focus(GuiWidget *widget, void *usrdata)
{
	return _eutel_chinfo_lost_focus(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chinfo_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_chinfo_keypress(widget, usrdata);
}

#endif
