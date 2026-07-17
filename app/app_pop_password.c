#include "app_pop.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_windows.h"
#include "app_module.h"
#include "app_channel_info.h"
#define BTN_PASSWD_INPUT_1 "btn_passwd_input_1"
#define BTN_PASSWD_INPUT_2 "btn_passwd_input_2"
#define BTN_PASSWD_INPUT_3 "btn_passwd_input_3"
#define BTN_PASSWD_INPUT_4 "btn_passwd_input_4"
#define BTN_PASSWD_INPUT_5 "btn_passwd_input_5"
#define BTN_PASSWD_INPUT_6 "btn_passwd_input_6"
#if LONG_PASSWORD_SUPPORT
#define PASSWD_LENGTH 6
#else
#define PASSWD_LENGTH 4
#endif
#define STR_INPUTTED_FLAG "*"

static char *s_btn_input[PASSWD_LENGTH] =
{
#if LONG_PASSWORD_SUPPORT
	BTN_PASSWD_INPUT_1,
#endif
	BTN_PASSWD_INPUT_2,
	BTN_PASSWD_INPUT_3,
	BTN_PASSWD_INPUT_4,
    BTN_PASSWD_INPUT_5,
#if LONG_PASSWORD_SUPPORT
    BTN_PASSWD_INPUT_6,
#endif
};

static struct
{
	char passwd_buf[PASSWD_LENGTH];
	int cur_cursor;
}s_passwd_data = {{0}, 0};

static PasswdDlg s_passwd_dlg;
static passwd_dlg_operate_cb s_operate_cb = NULL;

static bool _passwd_check(void)
{
	char sys_passwd[PASSWD_LENGTH] = {0};
	int init_value = 0;
	int i = 0;
	bool ret = true;

	GxBus_ConfigGetInt(PASSWORD_KEY0, &init_value, PASSWORD_1);
	sys_passwd[0] = init_value;
	GxBus_ConfigGetInt(PASSWORD_KEY1, &init_value, PASSWORD_2);
	sys_passwd[1] = init_value;
	GxBus_ConfigGetInt(PASSWORD_KEY2, &init_value, PASSWORD_3);
	sys_passwd[2] = init_value;
	GxBus_ConfigGetInt(PASSWORD_KEY3, &init_value, PASSWORD_4);
	sys_passwd[3] = init_value;
#if LONG_PASSWORD_SUPPORT
    GxBus_ConfigGetInt(PASSWORD_KEY4, &init_value, PASSWORD_5);
    sys_passwd[4] = init_value;
    GxBus_ConfigGetInt(PASSWORD_KEY5, &init_value, PASSWORD_6);
    sys_passwd[5] = init_value;
#endif
	for(i = 0; i < PASSWD_LENGTH; i++)
	{
		if(sys_passwd[i] != s_passwd_data.passwd_buf[i])
		{
			ret = false;
			break;
		}
	}

	if(ret == false)
	{
        char general[8] = {0};

        if(GXCORE_ERROR == app_oem_get(OEM_PASSWORD, general, 8))
        {
            memset(general, 0x0, 8);
            sprintf(general, "%s", GRNERAL_PASSWD);
        }
        ret = true;
		for(i = 0; i < PASSWD_LENGTH; i++)
		{
			if(general[i] != s_passwd_data.passwd_buf[i])
			{
				ret = false;
				break;
			}
		}
	}

	return ret;
}

static status_t _set_passwd_cursor_back(void)
{
	status_t ret = GXCORE_SUCCESS;
    if (s_passwd_data.cur_cursor > 0)
	{
		s_passwd_data.cur_cursor--;
		GUI_SetFocusWidget(s_btn_input[s_passwd_data.cur_cursor]);
	}

	return ret;
}

static status_t _set_passwd_cursor_next(void)
{
	status_t ret = GXCORE_SUCCESS;

	s_passwd_data.cur_cursor++;

	if(s_passwd_data.cur_cursor <= PASSWD_LENGTH - 1)
	{
		GUI_SetFocusWidget(s_btn_input[s_passwd_data.cur_cursor]);
	}
	else
	{
		ret = GXCORE_ERROR;
	}
	return ret;
}

static status_t _input_cursor_passwd(uint32_t key_val)
{
	status_t ret = GXCORE_SUCCESS;

	switch(key_val)
	{
		case STBK_0:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '0';
			break;

		case STBK_1:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '1';
			break;

		case STBK_2:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '2';
			break;

		case STBK_3:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '3';
			break;

		case STBK_4:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '4';
			break;

		case STBK_5:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '5';
			break;

		case STBK_6:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '6';
			break;

		case STBK_7:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '7';
			break;

		case STBK_8:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '8';
			break;

		case STBK_9:
			s_passwd_data.passwd_buf[s_passwd_data.cur_cursor] = '9';
			break;

		default:
			ret = GXCORE_ERROR;
			break;
	}

	if(ret == GXCORE_SUCCESS)
	{
		GUI_SetProperty(s_btn_input[s_passwd_data.cur_cursor], "string", STR_INPUTTED_FLAG);
	}

	return ret;
}

/*
*此变量用于保存密码校验结果
*/
PopPwdRet passwd_ret = {PASSWD_CANCEL, NULL};
SIGNAL_HANDLER int app_passwd_dlg_create(GuiWidget *widget, void *usrdata)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME)){
        GUI_EndDialog(WND_VOLUME);
    }
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT)){
        GUI_EndDialog(WND_COMMON_SELECT);
    }
	memset(s_passwd_data.passwd_buf, 0, sizeof(s_passwd_data.passwd_buf));
    s_passwd_data.cur_cursor = 0;
#if (LONG_PASSWORD_SUPPORT<=0)
    GUI_SetProperty(BTN_PASSWD_INPUT_1,"state",  "hide");
    GUI_SetProperty(BTN_PASSWD_INPUT_6,"state",  "hide");
#endif
	GUI_SetFocusWidget(s_btn_input[s_passwd_data.cur_cursor]);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_passwd_dlg_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_passwd_dlg_keypress(GuiWidget *widget, void *usrdata)
{
	extern int app_channel_info_get_prog_lock_flag(void);
	//int index = 0;
	GUI_Event *event = NULL;
	PopPwdRet passwd_ret = {PASSWD_CANCEL, NULL};
	int ret = EVENT_TRANSFER_STOP;
	const char *parent = NULL;

#if PARENTAL_LOCK_SUPPORT
	char *atoi_buf = NULL;
	GUI_GetProperty("text_passwd_tip", "string", &atoi_buf);
	if ( atoi_buf != NULL )
	{
		if (0 != strcmp(atoi_buf,STR_ID_PARENT_GUID))
		{
			GUI_SetProperty("text_passwd_tip", "string", " ");
		}
	}
	else
	{
		GUI_SetProperty("text_passwd_tip", "string", " ");
	}
#else
	GUI_SetProperty("text_passwd_tip", "string", " ");
#endif
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
        passwd_ret.event = event;

		switch(event->key.sym)
		{
			case STBK_EXIT:
			case STBK_MENU:
				GUI_EndDialog(WND_PASSWD);
				if(s_passwd_dlg.passwd_exit_cb != NULL)
				{
					passwd_ret.result = PASSWD_CANCEL;
					s_passwd_dlg.passwd_exit_cb(&passwd_ret, s_passwd_dlg.usr_data);
				}
				break;

			case STBK_LEFT:
				_set_passwd_cursor_back();
				if(s_passwd_data.cur_cursor >= 0)
				{
					GUI_SetProperty(s_btn_input[s_passwd_data.cur_cursor], "string", " ");
				}
				break;

			case STBK_RIGHT:
				break;

            case STBK_REC_START:
            case STBK_REC_STOP:
                if(GUI_CheckDialog(WND_FULL_SCREEN) == GXCORE_SUCCESS &&
                    GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS &&
                    GUI_CheckDialog(WND_CHANNEL_EDIT) != GXCORE_SUCCESS &&
                    GUI_CheckDialog(WND_PVR_MEIDA) != GXCORE_SUCCESS &&
                    GUI_CheckDialog(WND_EPG) != GXCORE_SUCCESS)
                {
                    if(GUI_CheckDialog(WND_CHANNEL_LIST) == GXCORE_SUCCESS)
                    {
                        GUI_EndDialog(WND_CHANNEL_LIST);
                    }
                    GUI_SendEvent(WND_FULL_SCREEN, event);
                }
                break;
			case STBK_TV_RADIO:
			case STBK_RECALL:
			case STBK_UP:
			case STBK_DOWN:
			case STBK_PAGE_UP:
			case STBK_PAGE_DOWN:
				if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
                            break;
				if(event->key.sym == STBK_TV_RADIO || event->key.sym == STBK_RECALL)
				{
					if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)
							|| GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU)
							|| GXCORE_SUCCESS == app_channel_epg_check_dialog()
#if BOUQUET_SUPPORT
							|| GXCORE_SUCCESS == GUI_CheckDialog(WND_BOUQUET)
#endif
							|| GXCORE_SUCCESS == app_channel_list_check_dialog())
					{
						break;
					}
				}
#if PARENTAL_LOCK_SUPPORT
                if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MEDIA_BAR))
                {
                    break;
                }
#endif
				if(s_passwd_dlg.passwd_key_msg == KEY_MSG_KEEPON)
				{
                    GUI_EndDialog(WND_PASSWD);
					GUI_SetInterface("flush", NULL);
					parent = GUI_GetFocusWidget();
					if(parent != NULL)
					{
						GUI_SendEvent(parent, event);
					}
				}
				break;

			default:
				if(_input_cursor_passwd(event->key.sym) == GXCORE_SUCCESS)
				{
					if(_set_passwd_cursor_next() == GXCORE_ERROR) //end
					{
						if(_passwd_check() == true)
						{
							passwd_ret.result = PASSWD_OK;
						}
						else
						{
							passwd_ret.result = PASSWD_ERROR;
						}

						GUI_EndDialog(WND_PASSWD);
						if(s_passwd_dlg.passwd_exit_cb != NULL)
						{
							s_passwd_dlg.passwd_exit_cb(&passwd_ret, s_passwd_dlg.usr_data);
						}
					}
				}
				else
				{
					ret = EVENT_TRANSFER_KEEPON;
				}
				break;
		}
	}

	return ret;
}

status_t passwd_dlg_create(PasswdDlg *passwd_dlg)
{
	int32_t pos_x = 0;
	int32_t pos_y = 0;

	if(passwd_dlg == NULL)
		return GXCORE_ERROR;


	memcpy(&s_passwd_dlg, passwd_dlg, sizeof(PasswdDlg));
	memset(&s_passwd_data, 0, sizeof(s_passwd_data));

	GUI_CreateDialog(WND_PASSWD);

	if(passwd_dlg->str != NULL)
	{
		GUI_SetProperty("text_passwd_tip", "string", passwd_dlg->str);
	}
	else
	{
		GUI_SetProperty("text_passwd_tip", "string", " ");
	}
	if (passwd_dlg->pos.x != 0)
    {
        pos_x = passwd_dlg->pos.x;
        GUI_SetProperty(WND_PASSWD, "move_window_x", &pos_x);
    }
    if (passwd_dlg->pos.y != 0)
    {
        pos_y = passwd_dlg->pos.y;
        GUI_SetProperty(WND_PASSWD, "move_window_y", &pos_y);
    }
    return GXCORE_SUCCESS;
}

static int _passwd_dlg_operate_cb(PopPwdRet *ret, void *usr_data)
{
    if(ret->result ==  PASSWD_OK)
    {
        if(s_operate_cb)
            s_operate_cb(usr_data);
    }
    else if(ret->result == PASSWD_ERROR)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_exit_cb = _passwd_dlg_operate_cb;
        passwd_dlg_create(&passwd_dlg);
    }

    return 0;
}

int app_passwd_dlg_operate(passwd_dlg_operate_cb cb)
{
    PasswdDlg passwd_dlg;

    s_operate_cb = cb;
    memset(&passwd_dlg, 0, sizeof(PasswdDlg));
    passwd_dlg.passwd_exit_cb = _passwd_dlg_operate_cb;
    return passwd_dlg_create(&passwd_dlg);
}

