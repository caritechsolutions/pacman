#include "app.h"
#include "app_windows.h"
#include "full_screen.h"
#include "app_keyboard_language.h"
#include "app_module.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if RICHEPG_SUPPORT
#include "app_eutel_control.h"
#include "app_eutel_common.h"
#include "channel_common.h"

typedef struct {
	bool lock_flag;
	const char *wnd;
	const char *text_title;
	const char *text_mark;
	const char *lv_list;
	const char *lv_logo_list;
} EutelChListWidget;

static EutelChListWidget s_eutel_chlist_wgt;

void app_eutel_chlist_widget_init(void)
{
	EutelChListWidget *widget = &s_eutel_chlist_wgt;

	widget->wnd = WND_EUTEL_CHLIST;
	widget->text_title = "text_eutel_chlist_title";
	widget->text_mark = "text_eutel_chlist_green_tip";
	widget->lv_list = "listview_eutel_chlist";
	widget->lv_logo_list = "listview_eutel_chlist_logo";
}

int app_eutel_chlist_create_dialog(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->use_total == 0)
	{
		EUTEL_INFO("No prog!\n");
		return -1;
	}
	if (GUI_CheckDialog(WND_EUTEL_CHINFO) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_EUTEL_CHINFO);
	}
/*
	if (GUI_CheckDialog(WND_EUTEL_RCHINFO) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_EUTEL_RCHINFO);
	}
*/
	app_eutel_chlist_widget_init();
	GUI_CreateDialog(s_eutel_chlist_wgt.wnd);

	return 0;
}

static void _eutel_chlist_change_title_tip(bool update_title)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	const char *tmp_name = NULL;

	if (update_title)
	{
		eutel_channel_group_title(s_eutel_chlist_wgt.text_title);
	}
	tmp_name = (ctrl->filter_sel == 1) ? STR_ID_ALL : STR_ID_MARK_LIST;
	GUI_SetProperty(s_eutel_chlist_wgt.text_mark, "string", (char*)tmp_name);
}

static void _eutel_chlist_list_update_all(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = 0;

	GUI_SetProperty(s_eutel_chlist_wgt.lv_logo_list, "update_all", NULL);
	GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "update_all", NULL);
	sel = eutel_channel_play_use_sel_get();
	GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "active", &sel);
	if (ctrl->use_total > 0 && sel < 0)
		sel = 0;
	GUI_SetProperty(s_eutel_chlist_wgt.lv_logo_list, "select", &sel);
	GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "select", &sel);
}

static void eutel_chlist_find_keyboard_release_cb(PopKeyboard *data)
{
	return;
}

static void eutel_chlist_find_keyboard_change_cb(PopKeyboard *data)
{
	eutel_channel_group_find_build(data->out_name);
	_eutel_chlist_list_update_all();
	GUI_SetProperty(WND_KEYBOARD_LANGUAGE, "update", NULL);
}

int app_eutel_chlist_find_keyboard(void)
{
	static PopKeyboard keyboard;

	memset(&keyboard, 0, sizeof(PopKeyboard));
	keyboard.in_name    = NULL;
	keyboard.max_num    = MAX_PROG_NAME - 1;
	keyboard.out_name   = NULL;
	keyboard.change_cb  = eutel_chlist_find_keyboard_change_cb;
	keyboard.release_cb = eutel_chlist_find_keyboard_release_cb;
	keyboard.usr_data   = NULL;
	keyboard.pos.x      = APP_WND_FIND_KB_POS_X;
	keyboard.pos.y      = APP_WND_FIND_KB_POS_Y;
	keyboard.lang_sel   = app_richepg_keyboard_sel();
	multi_language_keyboard_create(&keyboard);

	return EVENT_TRANSFER_STOP;
}

static int _eutel_chlist_ok_keypress(pvr_state cur_pvr)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = 0;
	int lcn = 0;

	if (ctrl->use_total == 0)
		return -1;
	GUI_GetProperty(s_eutel_chlist_wgt.lv_list, "select", &sel);
	if (sel >= ctrl->use_total || sel < 0)
		return -1;

	ctrl->filter_sel_bak = ctrl->filter_sel;
	lcn = ctrl->use_array[sel].lcn;

	if ((lcn != ctrl->cur_tv_lcn && 0 == ctrl->cur_stream_type)
			|| (lcn != ctrl->cur_ra_lcn && 1 == ctrl->cur_stream_type)
			|| g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK
			|| g_AppFullArb.state.pause == STATE_ON)
	{
		GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "active", &sel);
		g_AppFullArb.state.pause = STATE_OFF;
		g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
		if ( s_eutel_chlist_wgt.lock_flag &&
		    (ctrl->arg_array[ctrl->use_array[sel].pos].lock_flag != ctrl->arg_array[ctrl->use_array[sel].pos].lock_flag_bak) )
		{
			eutel_channel_lock_save_all();
		}
		eutel_channel_play_by_lcn(PLAY_MODE_POINT, lcn);
	}
	else
	{
		GUI_EndDialog(s_eutel_chlist_wgt.wnd);
		GUI_SetInterface("flush", NULL);
		app_eutel_chinfo_create_dialog();
	}
	return 0;
}

static int _eutel_chlist_stop_pvr_cb(PopDlgRet ret)
{
	pvr_state cur_pvr = app_pvr_get_state();

	if (POP_VAL_OK == ret)
	{
		GUI_SetInterface("flush", NULL);
		if(PVR_TIMESHIFT == cur_pvr)
		{
			app_pvr_tms_stop();
		}
		else
		{
			app_pvr_stop();
		}

		_eutel_chlist_ok_keypress(cur_pvr);
	}
	return 0;
}

static int _eutel_chlist_num_reponse(int num_value)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int filter_sel = ctrl->filter_sel;
	int orig_sel = 0;
	int sel = ctrl->use_total - 1;
	int i = 0;

	GUI_GetProperty(s_eutel_chlist_wgt.lv_list, "select", &orig_sel);
	for (i = 0; i < ctrl->use_total; i++)
	{
		if (ctrl->use_array[i].lcn == num_value)
		{
			sel = i;
			if (orig_sel != sel)
			{
				GUI_SetProperty(s_eutel_chlist_wgt.lv_logo_list, "select", &sel);
				GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "select", &sel);
			}
			return 0;
		}
	}

	if (filter_sel != 0)
	{
		eutel_channel_mark_save_all();
		eutel_channel_lock_save_all();
		eutel_channel_group_all_build();
		_eutel_chlist_change_title_tip(true);

		GUI_SetProperty(s_eutel_chlist_wgt.lv_logo_list, "update_all", NULL);
		GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "update_all", NULL);
		sel = eutel_channel_play_use_sel_get();
		GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "active", &sel);
	}

	sel = ctrl->use_total - 1;
	for (i = 0; i < ctrl->use_total; i++)
	{
		if (ctrl->use_array[i].lcn == num_value)
		{
			sel = i;
			break;
		}
		else if (ctrl->use_array[i].lcn > num_value)
		{
			if (i == 0)
			{
				sel = i;
			}
			else
			{
				if (ctrl->use_array[i].lcn - num_value < num_value - ctrl->use_array[i-1].lcn)
					sel = i;
				else
					sel = i - 1;
			}
			break;
		}
	}

	if (orig_sel != sel || filter_sel != 0)
	{
		GUI_SetProperty(s_eutel_chlist_wgt.lv_logo_list, "select", &sel);
		GUI_SetProperty(s_eutel_chlist_wgt.lv_list, "select", &sel);
	}

	return 0;
}

static int _eutel_chlist_create(GuiWidget *widget, void *usrdata)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	ctrl->filter_sel_bak = ctrl->filter_sel;
	s_eutel_chlist_wgt.lock_flag = false;
	_eutel_chlist_change_title_tip(true);
	_eutel_chlist_list_update_all();
	GUI_SetFocusWidget(s_eutel_chlist_wgt.lv_list);
	app_number_response_cb_set(_eutel_chlist_num_reponse);

	return EVENT_TRANSFER_STOP;
}

static int _eutel_chlist_destroy(GuiWidget *widget, void *usrdata)
{
	app_number_response_cb_set(NULL);
	eutel_channel_mark_save_all();
	eutel_channel_lock_save_all();
	s_eutel_chlist_wgt.lock_flag = false;
	app_richepg_chfilter_keep_sec_check_set();
	return EVENT_TRANSFER_STOP;
}

static int _eutel_chlist_change_sel_keypress(GuiWidget *widget, void *usrdata, int widget_sel)
{
	int ret = EVENT_TRANSFER_STOP;
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	GUI_Event *event = NULL;
	const char *list_name = NULL;
	int sel = 0;

	if (ctrl->use_total == 0)
		return ret;

	event = (GUI_Event *)usrdata;
	if (widget_sel == 0)
	{
		list_name = s_eutel_chlist_wgt.lv_list;
		GUI_SendEvent(s_eutel_chlist_wgt.lv_logo_list, event);
	}
	else
	{
		list_name = s_eutel_chlist_wgt.lv_logo_list;
	}

	switch(event->type)
	{
		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_UP:
				case STBK_DOWN:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				//case STBK_LEFT:
				case STBK_PAGE_UP:
					GUI_GetProperty(list_name, "select", &sel);
					if (sel == 0)
					{
						sel = ctrl->use_total - 1;
						GUI_SetProperty(list_name, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						event->key.sym = STBK_PAGE_UP;
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;

				//case STBK_RIGHT:
				case STBK_PAGE_DOWN:
					GUI_GetProperty(list_name, "select", &sel);
					if (ctrl->use_total == sel+1)
					{
						sel = 0;
						GUI_SetProperty(list_name, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						event->key.sym = STBK_PAGE_DOWN;
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

static int _channel_lock_passwd_dlg_cb(void *usr_data)
{
	int sel = 0;
	s_eutel_chlist_wgt.lock_flag = true;
	GUI_GetProperty(s_eutel_chlist_wgt.lv_list, "select", &sel);
	if (eutel_channel_lock_change(sel, false) == 0)
	{
		eutel_channel_update_current_row(s_eutel_chlist_wgt.lv_list);
	}
	return 0;
}

static int _eutel_chlist_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	EutelChannelArg *arg = NULL;
	GUI_Event *event = NULL;
	GxMsgProperty_NodeByIdGet node_prog = {0};
	int sel = 0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(s_eutel_chlist_wgt.wnd);
					GUI_SetInterface("flush", NULL);
					app_eutel_chinfo_create_dialog();
					break;

				case VK_BOOK_TRIGGER:
					GUI_EndDialog(s_eutel_chlist_wgt.wnd);
					break;

				case STBK_OK:
					if (ctrl->use_total == 0)
						break;
					GUI_GetProperty(s_eutel_chlist_wgt.lv_list, "select", &sel);
					if (sel >= ctrl->use_total || sel < 0)
						break;
					arg = &ctrl->arg_array[ctrl->use_array[sel].pos];

					node_prog.node_type = NODE_PROG;
					node_prog.id = arg->prog_id;
					app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_prog);

					if((PVR_DUMMY != g_AppPvrOps.state)
							&& (node_prog.prog_data.id == g_AppPvrOps.env.prog_id))
					{
						ctrl->filter_sel_bak = ctrl->filter_sel;
						GUI_EndDialog(s_eutel_chlist_wgt.wnd);
						GUI_SetInterface("flush",NULL);
						app_eutel_chinfo_create_dialog();
						break;
					}
#if SAT2IP_SERVER_SUPPORT
					if (app_sat2ip_switch_tip_get_by_prog(&node_prog.prog_data) < 0)
						break;
#endif
					if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_eutel_chlist_stop_pvr_cb))
					{
						_eutel_chlist_ok_keypress(app_pvr_get_state());
					}
					break;

				case STBK_RED:
					GUI_GetProperty(s_eutel_chlist_wgt.lv_list, "select", &sel);
					if (eutel_channel_mark_change(sel, false) == 0)
					{
						eutel_channel_update_current_row(s_eutel_chlist_wgt.lv_list);
					}
					break;

				case STBK_GREEN:
				case STBK_FAV:
					eutel_channel_mark_save_all();
					eutel_channel_lock_save_all();
					if (ctrl->filter_sel == 1)
						eutel_channel_group_all_build();
					else
						eutel_channel_group_mark_build();
					_eutel_chlist_change_title_tip(true);
					_eutel_chlist_list_update_all();
					break;

				case STBK_YELLOW:
					eutel_channel_mark_save_all();
					eutel_channel_lock_save_all();
					app_eutel_chlist_filter_pop(s_eutel_chlist_wgt.text_title);
					_eutel_chlist_change_title_tip(false);
					_eutel_chlist_list_update_all();
					break;

				case STBK_SAT:
					if ( app_eutel_sat_list_create_dialog() )
					{
						GUI_EndDialog(s_eutel_chlist_wgt.wnd);
						GUI_SetInterface("flush", NULL);
					}
					break;

				case STBK_BLUE:
				case STBK_F1:
					eutel_channel_mark_save_all();
					eutel_channel_lock_save_all();
					eutel_channel_group_all_build();
					GUI_SetProperty(s_eutel_chlist_wgt.text_title, "string", STR_ID_FIND);
					_eutel_chlist_change_title_tip(false);
					_eutel_chlist_list_update_all();
					app_eutel_chlist_find_keyboard();
					break;

				case STBK_0:
					if (s_eutel_chlist_wgt.lock_flag)
						_channel_lock_passwd_dlg_cb(NULL);
					else
						app_passwd_dlg_operate(_channel_lock_passwd_dlg_cb);
					break;

				case STBK_1:
				case STBK_2:
				case STBK_3:
				case STBK_4:
				case STBK_5:
				case STBK_6:
				case STBK_7:
				case STBK_8:
				case STBK_9:
					GUI_EndDialog(WND_VOLUME);
					GUI_CreateDialog(WND_NUMBER);
					GUI_SendEvent(WND_NUMBER, event);
					break;
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

int  _eutel_chlist_list_switch_keypress(int left_right)
{
	int i = 0 ;
	int cur_sel = -1;
	int sat_id = 0 ;
	uint32_t sel = 0;
	uint32_t id_total = 0;
	uint32_t *id_buffer = NULL;
	AppIdOps *cl_id = NULL ;

	cl_id = new_id_ops();
	channel_get_valid_group_id(cl_id, NULL, NULL, NULL, VIEW_INFO_SKIP_CONTAIN);
	if(channel_id_total_and_buffer_dump(cl_id, &id_total, &id_buffer) == GXCORE_ERROR)
	{
		EUTEL_ERR("----group channel_id_total_and_buffer_dump error, %d----\n",__LINE__);
		del_id_ops(cl_id);
		return -1;
	}
	if ( 1 >= id_total )
	{
		GxCore_Free(id_buffer);
		del_id_ops(cl_id);
		return -1;
	}

	for(i=0; i<id_total; i++)
	{
		if ( (i==0) && (id_buffer[0] == ALL_SAT))
			continue ;

		if((id_buffer[i] & FAV_MASK) == 0)//sat
		{
			if(g_AppPlayOps.normal_play.view_info.sat_id == id_buffer[i])
			{
				cur_sel = i;
				break;
			}
		}
	}
	if ( cur_sel < 0 || cur_sel >= id_total )
	{
		EUTEL_ERR("can not find cur group sel = %d \n",cur_sel);
		GxCore_Free(id_buffer);
		del_id_ops(cl_id);
		return -1;
	}

	if ( left_right )//right
	{
		if ((cur_sel+1) >= id_total )
			sel = 0 ;
		else
			sel = cur_sel + 1;
	}
	else
	{
		if (cur_sel <= 0 )
			sel = id_total - 1 ;
		else
			sel = cur_sel - 1;
	}
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if((0 == sel) && (id_buffer[0] == ALL_SAT))//all satellite
	{
		g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
	}
	else//sat or fav
	{
		if(id_buffer[sel] == HD_MASK)
		{
			g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_HD;
		}
		else if((id_buffer[sel] & FAV_MASK) == 0)//sat
		{
			g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
			g_AppPlayOps.normal_play.view_info.sat_id = id_buffer[sel];
		}
		else//fav
		{
			g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_FAV;
			g_AppPlayOps.normal_play.view_info.fav_id = (id_buffer[sel] & (~FAV_MASK));
		}
	}

	if ( app_fuse_spec_is_sattv_work_mode() )
	{
		if ( app_fuse_spec_switch_work_group_change() < 0 )
		{
			g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
			g_AppPlayOps.normal_play.view_info.sat_id = sat_id;
		}
	}
	else
	{
		GUI_EndDialog(WND_EUTEL_CHLIST);
		app_fuse_spec_switch_work_group_change();
		g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
		GUI_CreateDialog(WND_CHANNEL_LIST);
		app_play_current_prog(PLAY_MODE_POINT);
	}

	GxCore_Free(id_buffer);
	del_id_ops(cl_id);
	return 0;
}

static int _eutel_chlist_list_keypress(GuiWidget *widget, void *usrdata)
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
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_EXIT:
				case STBK_MENU:
				case VK_BOOK_TRIGGER:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				case STBK_UP:
				case STBK_DOWN:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					ret = _eutel_chlist_change_sel_keypress(widget, usrdata, 0);
					break;

				case STBK_LEFT:
					_eutel_chlist_list_switch_keypress(0);
					break;
				case STBK_RIGHT:
					_eutel_chlist_list_switch_keypress(1);
					break;

				case STBK_OK:
				case STBK_RED:
				case STBK_GREEN:
				case STBK_FAV:
				case STBK_YELLOW:
				case STBK_SAT:
				case STBK_BLUE:
				case STBK_F1:
				case STBK_1:
				case STBK_2:
				case STBK_3:
				case STBK_4:
				case STBK_5:
				case STBK_6:
				case STBK_7:
				case STBK_8:
				case STBK_9:
				case STBK_0:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

static int _eutel_chlist_logo_list_keypress(GuiWidget *widget, void *usrdata)
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
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_EXIT:
				case STBK_MENU:
				case VK_BOOK_TRIGGER:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				case STBK_UP:
				case STBK_DOWN:
				case STBK_LEFT:
				case STBK_PAGE_UP:
				case STBK_RIGHT:
				case STBK_PAGE_DOWN:
					ret = _eutel_chlist_change_sel_keypress(widget, usrdata, 1);
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

static int _eutel_chlist_list_get_total(GuiWidget *widget, void *usrdata)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	return ctrl->use_total;
}

static int _eutel_chlist_logo_list_get_data(GuiWidget *widget, void *usrdata)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;

	ListItemPara* item = NULL;
	item = (ListItemPara*)usrdata;
	if(item == NULL || item->sel >= ctrl->use_total || item->sel < 0)
		return GXCORE_ERROR;

	//col-0: channel logo
	item->x_offset = 0;
	item->zoom = true;
	item->string = NULL;
	item->image = eutel_channel_logo_key_get(item->sel);

	return EVENT_TRANSFER_STOP;
}

static int _eutel_chlist_list_get_data(GuiWidget *widget, void *usrdata)
{
#define EUTEL_CH_KEY_STR_LEN    32
#define EUTEL_CH_LCN_STR_LEN    6

	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	EutelChannelArg *arg = NULL;
	static char lcn_str[EUTEL_CH_LCN_STR_LEN];

	ListItemPara* item = NULL;
	item = (ListItemPara*)usrdata;
	if(item == NULL || item->sel >= ctrl->use_total || item->sel < 0)
		return GXCORE_ERROR;

	arg = &ctrl->arg_array[ctrl->use_array[item->sel].pos];
	snprintf(lcn_str, EUTEL_CH_LCN_STR_LEN, "%04d", ctrl->use_array[item->sel].lcn);

	//col-0: channel lcn
	item->x_offset = 0;
	item->string = lcn_str;
	item->image = NULL;

	//col-1: channel name
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = arg->channel_name;
	item->image = NULL;

	//channel scramble is not necessary

	//col-2: channel lock
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	if ((g_AppPvrOps.state == PVR_RECORD || g_AppPvrOps.state == PVR_TMS_AND_REC)
			&& g_AppPvrOps.env.prog_id == arg->prog_id)
		item->image = "s_ts_red";
	else if (arg->lock_flag)
		item->image = "s_icon_lock";
	else
		item->image = NULL;

	//col-3: channel mark
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	if (arg->mark_flag)
		item->image = "richepg_mark";
	else
		item->image = NULL;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_chlist_create(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_create(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_destroy(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_destroy(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_got_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_chlist_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_chlist_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_keypress(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_list_get_total(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_list_get_total(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_list_get_data(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_list_get_data(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_list_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_list_keypress(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_chlist_logo_list_get_total(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_list_get_total(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_logo_list_get_data(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_logo_list_get_data(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_logo_list_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_chlist_logo_list_keypress(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_chlist_logo_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

#endif
