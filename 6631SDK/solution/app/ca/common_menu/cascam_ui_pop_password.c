#include "app.h"
#if CASCAM_SUPPORT
#include "cascam_ui_pop.h"
#include "cascam_ui_porting.h"
#include "cascam_ui_common.h"

#define WND_PASSWD                      "wnd_flexible_passwd"
#define XML_PASSWD					    "ca/common_menu/wnd_pop_passwd.xml"
#define EDIT_PASSWD_INPUT               "edit_flexible_passwd_input"
#define TEXT_PASSWD_TIP                 "text_flexible_passwd_tip"
#define MAX_PASSWD_LENGTH               32
#define DEFAULT_PASSWD_LENGTH           4

static CascamPasswdDlg s_flexible_passwd_dlg;
static event_list  *thiz_pop_dialog_timer = NULL;
static int _pop_dialog_display_timer(void *usrdata)
{
	GUI_EndDialog(WND_PASSWD);

	return 0;
}


void app_flexible_passwd_end_dialog(void* usrdata)
{
	GUI_EndDialog(WND_PASSWD);

	if(s_flexible_passwd_dlg.passwd_exit_cb != NULL)
	{
		CascamPopPwdRet passwd_ret = {CASCAM_PASSWD_CANCEL, NULL};
		passwd_ret.event = (GUI_Event *)usrdata;
		passwd_ret.result = CASCAM_PASSWD_CANCEL;
		s_flexible_passwd_dlg.passwd_exit_cb(&passwd_ret, s_flexible_passwd_dlg.usr_data);
	}
}

static bool _passwd_check(char* passwd_buf)
{
	char sys_passwd[DEFAULT_PASSWD_LENGTH] = {0};
	int i = 0;
	bool ret = true;

	cascam_ui_get_app_password(sys_passwd);

	for(i = 0; i < DEFAULT_PASSWD_LENGTH; i++)
	{
		if(sys_passwd[i] != passwd_buf[i])
		{
			ret = false;
			break;
		}
	}

	if(ret == false)
	{
		char *general = GRNERAL_PASSWD;

		ret = true;
		for(i = 0; i < DEFAULT_PASSWD_LENGTH; i++)
		{
			if(general[i] != passwd_buf[i])
			{
				ret = false;
				break;
			}
		}
	}

	return ret;
}

static int _passwd_confirm(void *usrdata)
{
	const char *passwd_str = NULL;
	GUI_GetProperty(EDIT_PASSWD_INPUT, "string", &passwd_str);
	if(passwd_str == NULL)
		return -1;

	int passwd_len = strlen(passwd_str);
	if(passwd_len == 0)
		return -1;

	char passwd_buf[MAX_PASSWD_LENGTH] = {0};
	memcpy(passwd_buf, passwd_str, passwd_len);

	CascamPopPwdRet passwd_ret = {CASCAM_PASSWD_CANCEL, NULL};
	passwd_ret.event = (GUI_Event *)usrdata;

	if(s_flexible_passwd_dlg.passwd_check_cb != NULL)
	{
		if(s_flexible_passwd_dlg.passwd_check_cb(passwd_buf, passwd_len) == true)
			passwd_ret.result = CASCAM_PASSWD_OK;
		else
			passwd_ret.result = CASCAM_PASSWD_ERROR;
	}
	else
	{
		if(_passwd_check(passwd_buf) == true)
			passwd_ret.result = CASCAM_PASSWD_OK;
		else
			passwd_ret.result = CASCAM_PASSWD_ERROR;
	}
	GUI_EndDialog(WND_PASSWD);
	if(s_flexible_passwd_dlg.passwd_exit_cb != NULL)
	{
		s_flexible_passwd_dlg.passwd_exit_cb(&passwd_ret, s_flexible_passwd_dlg.usr_data);
	}

	return 0;
}

static int _passwd_len_show(void)
{
#if 0
	int passwd_len = 0;
	const char *passwd_str = NULL;
	GUI_GetProperty(EDIT_PASSWD_INPUT, "string", &passwd_str);
	if(passwd_str != NULL)
	{
		passwd_len = strlen(passwd_str);
	}
	if(passwd_len != 0)
	{
		char buffer[100] = {0};
		snprintf(buffer, sizeof(buffer), "Password length is %d!", passwd_len);
		GUI_SetProperty(TEXT_PASSWD_TIP, "string", buffer);
	}
	else
	{
		GUI_SetProperty(TEXT_PASSWD_TIP, "string", " ");
	}
#endif

	return 0;
}

static int _passwd_clear(void)
{
	GUI_SetProperty(EDIT_PASSWD_INPUT, "clear", NULL);

	_passwd_len_show();

	return 0;
}

static int _passwd_back(void)
{
	const char *passwd_str = NULL;
	GUI_GetProperty(EDIT_PASSWD_INPUT, "string", &passwd_str);
	if(passwd_str == NULL)
		return -1;

	int passwd_len = strlen(passwd_str);
	if(passwd_len == 0)
		return -1;

	int select = 0;
	GUI_GetProperty(EDIT_PASSWD_INPUT, "select", &select);

	char passwd_buf[MAX_PASSWD_LENGTH] = {0};
	memcpy(passwd_buf, passwd_str, passwd_len-1);
	GUI_SetProperty(EDIT_PASSWD_INPUT, "string", passwd_buf);

	select--;
	GUI_SetProperty(EDIT_PASSWD_INPUT, "select", &select);

	_passwd_len_show();

	return 0;
}

SIGNAL_HANDLER int app_flexible_passwd_input_reach_end(GuiWidget *widget, void *usrdata)
{
	_passwd_confirm(usrdata);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_flexible_passwd_input_change(GuiWidget *widget, void *usrdata)
{
	const char *passwd_str = NULL;
	GUI_GetProperty(EDIT_PASSWD_INPUT, "string", &passwd_str);
	if(passwd_str != NULL && strlen(passwd_str) != 0)
		_passwd_len_show();

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_flexible_passwd_input_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN == event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_LEFT:
				_passwd_back();
				break;
			case STBK_RIGHT:
				break;
			case STBK_BLUE:
				_passwd_clear();
				break;
//			case STBK_OK:
//				_passwd_confirm(usrdata);
//				break;
			default:
				ret = EVENT_TRANSFER_KEEPON;
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_flexible_passwd_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_flexible_passwd_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_flexible_passwd_keypress(GuiWidget *widget, void *usrdata)
{
	if(s_flexible_passwd_dlg.passwd_key_cb != NULL)
	{
		if(EVENT_TRANSFER_STOP == s_flexible_passwd_dlg.passwd_key_cb(widget, usrdata))
			return EVENT_TRANSFER_STOP;
	}

	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN == event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_EXIT:
			case STBK_MENU:
				if(s_flexible_passwd_dlg.block_exit)
					break;
				app_flexible_passwd_end_dialog(usrdata);
				break;

			case STBK_UP:
			case STBK_DOWN:
			case STBK_PAGE_UP:
			case STBK_PAGE_DOWN:
				if(s_flexible_passwd_dlg.passwd_key_msg == CASCAM_KEY_MSG_KEEPON)
				{
					GUI_EndDialog(WND_PASSWD);

					const char *parent = NULL;
					parent = GUI_GetFocusWidget();
					if(parent != NULL)
					{
						GUI_SendEvent(parent, event);
					}
				}
				break;
			default:
				break;
		}
	}

	return ret;
}

status_t flexible_passwd_dlg_create(CascamPasswdDlg *passwd_dlg)
{
#define WND_PASSWD_X 320
#define WND_PASSWD_Y 180

	static char init_flag = 0;
	if(passwd_dlg == NULL)
		return GXCORE_ERROR;

	memcpy(&s_flexible_passwd_dlg, passwd_dlg, sizeof(CascamPasswdDlg));

	if(0 == init_flag) {
		init_flag = 1;
		GUI_LinkDialog(WND_PASSWD, XML_PASSWD);
	}
	if((GXCORE_SUCCESS != GUI_CheckDialog(WND_PASSWD)) || (passwd_dlg->passwd_create_mode == CASCAM_PASSWD_REBUILD))
	{
		GUI_CreateDialog(WND_PASSWD);

		if(passwd_dlg->str != NULL)
			GUI_SetProperty(TEXT_PASSWD_TIP, "string", passwd_dlg->str);
		else
			GUI_SetProperty(TEXT_PASSWD_TIP, "string", " ");

		uint32_t len = passwd_dlg->input_len;
		if(len == 0)
			len = DEFAULT_PASSWD_LENGTH;

		static char edit_len[3] = {0};
		snprintf(edit_len , sizeof(edit_len), "%u", len);
		GUI_SetProperty(EDIT_PASSWD_INPUT, "maxlen", &edit_len);


		int32_t pos_x = 0, pos_y = 0;
		if ((passwd_dlg->pos.x==0)&&(passwd_dlg->pos.y==0))
		{
			pos_x = WND_PASSWD_X;
			pos_y = WND_PASSWD_Y;
		}
		else
		{
			pos_x = passwd_dlg->pos.x;
			pos_y = passwd_dlg->pos.y;
		}
		GUI_SetProperty(WND_PASSWD, "move_window_x", &pos_x);
		GUI_SetProperty(WND_PASSWD, "move_window_y", &pos_y);
	}

	if(passwd_dlg->duration > 0){
		if (reset_timer(thiz_pop_dialog_timer) != 0)
			thiz_pop_dialog_timer = create_timer(_pop_dialog_display_timer, passwd_dlg->duration, NULL, TIMER_ONCE);
	}
	return GXCORE_SUCCESS;
}
#endif
