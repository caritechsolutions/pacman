#include "app.h"
#include "app_windows.h"
#include "channel_common.h"
#include "module/app_frontend.h"
#include "full_screen.h"
#include "include/app_module.h"
#if RICHEPG_SUPPORT
#include "app_eutel_control.h"
#include "app_eutel_common.h"
#include "richepg_epg.h"
#include "richepg.h"

#define NO_EVNET_NAME_STR               "-"   // STR_ID_UNKNOW

static event_list* s_eutel_chmore_timer = NULL;
static time_t s_chmore_epg_start_time_bak = -1;
static int s_chmore_cur_next_event_flag = 0;  // 0,cur; 1,next
static char *s_event_detail_notepad_bak = NULL;

typedef struct {
	const char *wnd;

	const char *strength_bar;
	const char *strength_val;
	const char *quality_bar;
	const char *quality_val;

	const char *cur_event_btn;
	const char *next_event_btn;

	const char *event_logo;
	const char *event_title;
	const char *event_parent;
	const char *event_genres;
	const char *event_date;
	const char *event_duration;
	const char *event_details;

	const char *mod;
	const char *tp;
	const char *av;
	const char *pid;
	const char *cas;
} EutelChMoreWidget;

static EutelChMoreWidget s_eutel_chmore_wgt;

void app_eutel_chmore_widget_init(void)
{
	EutelChMoreWidget *widget = &s_eutel_chmore_wgt;

	widget->wnd = WND_EUTEL_CHMORE;

	widget->strength_bar = "progbar_eutel_chmore_strength";
	widget->strength_val = "text_eutel_chmore_strength_value";
	widget->quality_bar = "progbar_eutel_chmore_quality";
	widget->quality_val = "text_eutel_chmore_quality_value";

	widget->cur_event_btn = "btn_eutel_chmore_cur_event";
	widget->next_event_btn = "btn_eutel_chmore_next_event";

	widget->event_logo = "img_eutel_chmore_event_logo";
	widget->event_title = "text_eutel_chmore_event_title";
	widget->event_parent = "text_eutel_chmore_event_parent";
	widget->event_genres = "text_eutel_chmore_event_genres";
	widget->event_date = "text_eutel_chmore_event_date";
	widget->event_duration = "text_eutel_chmore_event_duration";
	widget->event_details = "notepad_eutel_chmore_event_details";

	widget->mod = "text_eutel_chmore_module_value";
	widget->tp = "text_eutel_chmore_tp_value";
	widget->av = "text_eutel_chmore_format_value";
	widget->pid = "text_eutel_chmore_pid_value";
	widget->cas = "text_eutel_chmore_smartcard";
}

int app_eutel_chmore_create_dialog(void)
{
	app_eutel_chmore_widget_init();

	GUI_CreateDialog(s_eutel_chmore_wgt.wnd);

	return 0;
}

static int _eutel_chmore_show_epg_info(void)
{
#define HOURMIN_STR_LEN     6
	extern void app_eutel_epg_hourmin_str_get(time_t sec, char *str);
	RichepgEpgEvent *event = NULL;
	char buffer[8] = {0};
	char *tmp_str = NULL;

	event = app_eutel_cur_next_epg_info_get(s_chmore_cur_next_event_flag);
	if (event)
	{
		if (s_chmore_epg_start_time_bak == event->start_time)
			return 0;

		s_chmore_epg_start_time_bak = event->start_time;
		if (event->event_title && strlen(event->event_title) > 0)
			tmp_str = event->event_title;
		else
			tmp_str = NO_EVNET_NAME_STR;
		GUI_SetProperty(s_eutel_chmore_wgt.event_title, "string", tmp_str);

		if (event->genres_num > 0)
			tmp_str = eutel_array_name_get(event->genres_num, event->genres_ids, EUTEL_CONTENT_GENRE);
		else
			tmp_str = STR_ID_BLANK;
		GUI_SetProperty(s_eutel_chmore_wgt.event_genres, "string", tmp_str);

		if (event->parent_rate > 0)
		{
			snprintf(buffer, sizeof(buffer), "%d", event->parent_rate);
			GUI_SetProperty(s_eutel_chmore_wgt.event_parent, "string", buffer);
		}
		else
		{
			GUI_SetProperty(s_eutel_chmore_wgt.event_parent, "string", STR_ID_BLANK);
		}

		tmp_str = app_richepg_date_str_get(event->start_time, true);
		GUI_SetProperty(s_eutel_chmore_wgt.event_date, "string", tmp_str);

		tmp_str = app_richepg_duration_str_get(event->start_time, event->finish_time, 0);
		GUI_SetProperty(s_eutel_chmore_wgt.event_duration, "string", tmp_str);

		char *logo_path = richepg_get_logo_path(event->event_logo, RICHEPG_LOGO_EVENT);
		if (logo_path)
		{
			gal_add_key_path(EUTEL_EPG_EVENT_KEY, logo_path);
			GUI_SetProperty(s_eutel_chmore_wgt.event_logo, "img", EUTEL_EPG_EVENT_KEY);
		}
		else
		{
			GUI_SetProperty(s_eutel_chmore_wgt.event_logo, "img", EUTEL_EPG_EVENT_KEY_DF);
		}

		if (event->event_desc && strlen(event->event_desc) > 0)
		{
			// GUI不会保存notepad的字符串，应用需要一直维持此字符串存在
			APP_FREE(s_event_detail_notepad_bak);
			s_event_detail_notepad_bak = GxCore_Strdup(event->event_desc);
			tmp_str = s_event_detail_notepad_bak;
		}
		else
		{
			tmp_str = app_richepg_epg_no_info_str_get();
		}
		GUI_SetProperty(s_eutel_chmore_wgt.event_details, "string", tmp_str);
	}
	else
	{
		if (s_chmore_epg_start_time_bak == 0)
			return 0;

		s_chmore_epg_start_time_bak = 0;
		GUI_SetProperty(s_eutel_chmore_wgt.event_title, "string", STR_ID_NO_EVENT);
		GUI_SetProperty(s_eutel_chmore_wgt.event_genres, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_chmore_wgt.event_parent, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_chmore_wgt.event_date, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_chmore_wgt.event_duration, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_chmore_wgt.event_logo, "img", EUTEL_EPG_EVENT_KEY_DF);
		tmp_str = app_richepg_epg_no_info_str_get();
		GUI_SetProperty(s_eutel_chmore_wgt.event_details, "string", tmp_str);
	}
	return 0;
}

static void _eutel_chmore_show_signal_info(void)
{
	unsigned int value = 0;
	char buffer[20] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};

	// progbar strength
	app_ioctl( g_AppPlayOps.normal_play.tuner, FRONTEND_STRENGTH_GET, &value);
	siganl_bar.strength_bar = (char*)s_eutel_chmore_wgt.strength_bar;
	siganl_val.strength_val = value;

	// strength value
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d%%", value);
	app_set_widget_string(s_eutel_chmore_wgt.strength_val, buffer);

	// progbar quality
	app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_QUALITY_GET, &value);
	siganl_bar.quality_bar= (char*)s_eutel_chmore_wgt.quality_bar;
	siganl_val.quality_val = value;

	// quality value
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d%%", value);
	app_set_widget_string(s_eutel_chmore_wgt.quality_val, buffer);

	app_signal_progbar_update(g_AppPlayOps.normal_play.tuner, &siganl_bar, &siganl_val);
}

static void _eutel_chmore_show_modulation_info(GxMsgProperty_NodeByPosGet *node_prog)
{
	extern char *app_prog_module_str_get(GxMsgProperty_NodeByPosGet *node_prog);
	char *module_str = NULL;
	module_str = app_prog_module_str_get(node_prog);
	app_set_widget_string(s_eutel_chmore_wgt.mod, module_str);
}

static void _eutel_chmore_show_tp_info(GxMsgProperty_NodeByPosGet *node_prog)
{
#define _TP_LEN  64
	char buffer[_TP_LEN+MAX_SAT_NAME+2] = {0};
	channel_prog_tp_info_str_get(&node_prog->prog_data, buffer, _TP_LEN);
#if DEMOD_DVB_S
	GxMsgProperty_NodeByIdGet node_sat = {0};
	node_sat.node_type = NODE_SAT;
	node_sat.id = node_prog->prog_data.sat_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);
	if (node_sat.sat_data.type == GXBUS_PM_SAT_S && strlen((char*)node_sat.sat_data.sat_s.sat_name) > 0)
	{
		strcat(buffer, ", ");
		strcat(buffer, (char*)node_sat.sat_data.sat_s.sat_name);

	}
#endif
	app_set_widget_string(s_eutel_chmore_wgt.tp, buffer);
}

static void _eutel_chmore_show_pid_info(GxMsgProperty_NodeByPosGet *node_prog)
{
#define PID_INFO_LEN  64
	char buffer[PID_INFO_LEN] = {0};
	if (node_prog->prog_data.service_type == GXBUS_PM_PROG_TV)
	{
		snprintf(buffer, PID_INFO_LEN, "V:%d A:%d PCR:%d", node_prog->prog_data.video_pid,
				node_prog->prog_data.cur_audio_pid, node_prog->prog_data.pcr_pid);
	}
	else
	{
		snprintf(buffer, PID_INFO_LEN, "A:%d PCR:%d",
				node_prog->prog_data.cur_audio_pid, node_prog->prog_data.pcr_pid);
	}
	app_set_widget_string(s_eutel_chmore_wgt.pid, buffer);
}

void app_eutel_chmore_show_format_info(GxMsgProperty_NodeByPosGet *node_prog, const char *resolution_str)
{
#define AV_TYPE_STR_LEN 16
#define AV_INFO_LEN  128
	extern char *app_channel_play_resolution_str_get(GxMsgProperty_NodeByPosGet *node_prog);
	AppProgAudioType a_type;
	AppProgVideoType v_type;
	char audio_type[AV_TYPE_STR_LEN] = {0};
	char video_type[AV_TYPE_STR_LEN] = {0};
	const char *res_str = NULL;
	char buffer[AV_INFO_LEN] = {0};

	GxMsgProperty_NodeByPosGet node;
	if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}

	a_type = channel_get_app_audio_type(node_prog->prog_data.cur_audio_type);
	channel_get_audio_type_str(audio_type, a_type, AV_TYPE_STR_LEN);
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
	{
		v_type = channel_get_app_video_type(node_prog->prog_data.video_type);
		channel_get_video_type_str(video_type, v_type, AV_TYPE_STR_LEN);
		if (resolution_str)
			res_str = resolution_str;
		else
			res_str = app_channel_play_resolution_str_get(node_prog);
		if (res_str && strlen(res_str) > 0)
			snprintf(buffer, AV_INFO_LEN, "V:%s %s A:%s", video_type, res_str, audio_type);
		else
			snprintf(buffer, AV_INFO_LEN, "V:%s A:%s", video_type, audio_type);
	}
	else
	{
		snprintf(buffer, AV_INFO_LEN, "A:%s", audio_type);
	}
	app_set_widget_string(s_eutel_chmore_wgt.av, buffer);
}

static void _eutel_chmore_show_cas_info(GxMsgProperty_NodeByPosGet *node_prog)
{
#define _CAS_LEN  200
	extern void app_channel_info_show_ca_system_name(uint16_t cas_id, char *buffer, int len);
	char buffer[_CAS_LEN] = {0};
	app_channel_info_show_ca_system_name(node_prog->prog_data.cas_id, buffer, _CAS_LEN); // cas info
	app_set_widget_string(s_eutel_chmore_wgt.cas, buffer);
}

static int eutel_chmore_timer_exec(void *userdata)
{
	static int cnt = 0;
	GxMsgProperty_NodeByPosGet node_prog = {0};

	_eutel_chmore_show_signal_info();
	if (++cnt >= 10) {
		cnt = 0;
		node_prog.node_type = NODE_PROG;
		node_prog.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

		_eutel_chmore_show_epg_info();
		app_eutel_chmore_show_format_info(&node_prog, NULL);
	}

	return 0;
}

int app_eutel_chmore_show_info(void)
{
	GxMsgProperty_NodeByPosGet node_prog = {0};

	node_prog.node_type = NODE_PROG;
	node_prog.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

	s_chmore_epg_start_time_bak = -1;
	_eutel_chmore_show_epg_info();
	_eutel_chmore_show_signal_info();
	_eutel_chmore_show_modulation_info(&node_prog);
	_eutel_chmore_show_tp_info(&node_prog);
	_eutel_chmore_show_pid_info(&node_prog);
	app_eutel_chmore_show_format_info(&node_prog, NULL);
	_eutel_chmore_show_cas_info(&node_prog);

	GUI_SetProperty(s_eutel_chmore_wgt.wnd, "draw_now", NULL);
	APP_TIMER_ADD(s_eutel_chmore_timer, eutel_chmore_timer_exec, 100, TIMER_REPEAT);

	return 0;
}

static int _eutel_chmore_create(GuiWidget *widget, void *usrdata)
{
	s_chmore_cur_next_event_flag = 0;
	GUI_SetFocusWidget(s_eutel_chmore_wgt.cur_event_btn);
	app_eutel_chmore_show_info();
	return EVENT_TRANSFER_STOP;
}
static int _eutel_chmore_destroy(GuiWidget *widget, void *usrdata)
{
	APP_FREE(s_event_detail_notepad_bak);
	APP_TIMER_REMOVE(s_eutel_chmore_timer);
	return EVENT_TRANSFER_STOP;
}
static int _eutel_chmore_got_focus(GuiWidget *widget, void *usrdata)
{
	if (s_eutel_chmore_timer)
		reset_timer(s_eutel_chmore_timer);
	return EVENT_TRANSFER_STOP;
}
static int _eutel_chmore_lost_focus(GuiWidget *widget, void *usrdata)
{
	if (s_eutel_chmore_timer)
		timer_stop(s_eutel_chmore_timer);
	return EVENT_TRANSFER_STOP;
}

static int _eutel_chmore_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	uint32_t value = 1;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_INFO:
			case STBK_EXIT:
			case STBK_MENU:
			case VK_BOOK_TRIGGER:
				GUI_EndDialog(s_eutel_chmore_wgt.wnd);
				if(g_AppFullArb.state.tv == STATE_ON)
				{
					GUI_EndDialog(WND_EUTEL_CHINFO);
					//GUI_EndDialog(WND_EUTEL_RCHINFO);
				}
				break;
			case STBK_LEFT:
			case STBK_RIGHT:
				if (s_chmore_cur_next_event_flag == 0)
				{
					s_chmore_cur_next_event_flag = 1;
					GUI_SetFocusWidget(s_eutel_chmore_wgt.next_event_btn);
				}
				else
				{
					s_chmore_cur_next_event_flag = 0;
					GUI_SetFocusWidget(s_eutel_chmore_wgt.cur_event_btn);
				}
				app_eutel_chmore_show_info();
				break;
			case STBK_UP:
				GUI_SetProperty(s_eutel_chmore_wgt.event_details, "line_up", &value);
				break;
			case STBK_DOWN:
				GUI_SetProperty(s_eutel_chmore_wgt.event_details, "line_down", &value);
				break;
			case STBK_PAGE_UP:
				GUI_SetProperty(s_eutel_chmore_wgt.event_details, "page_up", &value);
				break;
			case STBK_PAGE_DOWN:
				GUI_SetProperty(s_eutel_chmore_wgt.event_details, "page_down", &value);
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_chmore_create(GuiWidget *widget, void *usrdata)
{
	return _eutel_chmore_create(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chmore_destroy(GuiWidget *widget, void *usrdata)
{
	return _eutel_chmore_destroy(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chmore_got_focus(GuiWidget *widget, void *usrdata)
{
	return _eutel_chmore_got_focus(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chmore_lost_focus(GuiWidget *widget, void *usrdata)
{
	return _eutel_chmore_lost_focus(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chmore_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_chmore_keypress(widget, usrdata);
}

#endif

