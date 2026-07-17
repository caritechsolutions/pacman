#include "app.h"

#if SAMBA_SUPPORT
#include "app_samba_login.h"
#include "app_samba.h"
#include "app_windows.h"

#define BOX_SAMBA_LOGIN    "box_samba_login_opt"

#define SAMBA_PASSWD_INTAGLIO    "******"
#define SAMBA_PATH_HEADER    "//"
enum
{
    SAMBA_LOGIN_ITEM_PATH = 0,
    SAMBA_LOGIN_ITEM_USER,
    SAMBA_LOGIN_ITEM_PASSWD,
    SAMBA_LOGIN_ITEM_SAVE,
    SAMBA_LOGIN_ITEM_TOTAL,
};

static char *thiz_samba_box_button[] =
{
    "btn_samba_login_path",
    "btn_samba_login_username",
    "btn_samba_login_passwd",
};

static AppSambaInfoClass thiz_samba_login_data;
static AppSambaInfoClass *thiz_samba_login_ret;
static SambaLoginExitCb thiz_samba_login_exit_cb = NULL;
static PopKeyboard thiz_samba_login_keyboard;

static char unc_invalid_characters[] = "\\:*?\"<>|";
static int is_unc_path_valid(char *path)
{
	int ret = 0;
	int i = 0;
	char *temp = NULL;

	if(path&&(strlen(path)>0))
	{
		if(strncmp(path, "//", 2)==0)
		{
			for(i=0; i<strlen(unc_invalid_characters); i++)
			{
				temp = strchr(path, unc_invalid_characters[i]);
				if(temp)
				{
					break;
				}
			}
			if(!temp)
			{
				ret = 1;
			}
		}
	}
	return ret;
}

static void _samba_login_pop_msg(char *msg)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.str = msg;
    pop.title = NULL;
    pop.mode = POP_MODE_UNBLOCK;
    pop.create_cb = NULL;
    pop.exit_cb = NULL;
    pop.timeout_sec = 1;
    popdlg_create(&pop);
}

void _samba_login_get_params(AppSambaInfoClass *samba_ret)
{
    if(samba_ret != NULL)
    {
        strlcpy(samba_ret->samba_svr_path, thiz_samba_login_data.samba_svr_path, sizeof(samba_ret->samba_svr_path));
        strlcpy(samba_ret->samba_username, thiz_samba_login_data.samba_username, sizeof(samba_ret->samba_username));
        strlcpy(samba_ret->samba_passwd, thiz_samba_login_data.samba_passwd, sizeof(samba_ret->samba_passwd));
    }
}

static void _samba_login_path_keyboard_cb(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    if(is_unc_path_valid(data->out_name))
    {
        GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_PATH], "string", data->out_name);
        memset(thiz_samba_login_data.samba_svr_path, 0, sizeof(thiz_samba_login_data.samba_svr_path));
        strlcpy(thiz_samba_login_data.samba_svr_path, data->out_name, sizeof(thiz_samba_login_data.samba_svr_path));
    }
    else
    {
        _samba_login_pop_msg(STR_ID_ERR_PATH);
    }
}

void _samba_login_path_edit(void)
{
    memset(&thiz_samba_login_keyboard, 0, sizeof(PopKeyboard));
    if(strlen(thiz_samba_login_data.samba_svr_path) > 0)
        thiz_samba_login_keyboard.in_name = thiz_samba_login_data.samba_svr_path;
    else
        thiz_samba_login_keyboard.in_name = SAMBA_PATH_HEADER;
    thiz_samba_login_keyboard.max_num = MAX_SAMBA_SVR_PATH_LEN;
    thiz_samba_login_keyboard.release_cb = _samba_login_path_keyboard_cb;
    multi_language_keyboard_create(&thiz_samba_login_keyboard);
}

static void _samba_login_user_keyboard_cb(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_USER], "string", data->out_name);
    memset(thiz_samba_login_data.samba_username, 0, sizeof(thiz_samba_login_data.samba_username));
    strlcpy(thiz_samba_login_data.samba_username, data->out_name, sizeof(thiz_samba_login_data.samba_username));
}

void _samba_login_user_edit(void)
{
    memset(&thiz_samba_login_keyboard, 0, sizeof(PopKeyboard));
    thiz_samba_login_keyboard.in_name = thiz_samba_login_data.samba_username;
    thiz_samba_login_keyboard.max_num = MAX_SAMBA_USERNAME_LEN;
    thiz_samba_login_keyboard.release_cb = _samba_login_user_keyboard_cb;
    multi_language_keyboard_create(&thiz_samba_login_keyboard);
}

static void _samba_login_passwd_keyboard_cb(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    if(strlen(data->out_name) > 0)
        GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_PASSWD], "string", SAMBA_PASSWD_INTAGLIO);
    else
        GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_PASSWD], "string", STR_ID_BLANK);

    memset(thiz_samba_login_data.samba_passwd, 0, sizeof(thiz_samba_login_data.samba_passwd));
    strlcpy(thiz_samba_login_data.samba_passwd, data->out_name, sizeof(thiz_samba_login_data.samba_passwd));
}


void _samba_login_passwd_edit(void)
{
    memset(&thiz_samba_login_keyboard, 0, sizeof(PopKeyboard));
    thiz_samba_login_keyboard.in_name = thiz_samba_login_data.samba_passwd;
    thiz_samba_login_keyboard.max_num = MAX_SAMBA_PASSWD_LEN;
    thiz_samba_login_keyboard.release_cb = _samba_login_passwd_keyboard_cb;
    multi_language_keyboard_create(&thiz_samba_login_keyboard);
}

void _samba_login_save_proc(void)
{
    GUI_EndDialog(WND_SAMBA_LOGIN);
    _samba_login_get_params(thiz_samba_login_ret);
    if(thiz_samba_login_exit_cb != NULL)
    {
        thiz_samba_login_exit_cb(true, thiz_samba_login_ret);
    }
}

void _samba_login_ok_key_proc(void)
{
    uint32_t box_sel;

    GUI_GetProperty(BOX_SAMBA_LOGIN, "select", &box_sel);

    switch(box_sel)
    {
        case SAMBA_LOGIN_ITEM_PATH:
            _samba_login_path_edit();
            break;

        case SAMBA_LOGIN_ITEM_USER:
            _samba_login_user_edit();
            break;

        case SAMBA_LOGIN_ITEM_PASSWD:
            _samba_login_passwd_edit();
            break;

        case SAMBA_LOGIN_ITEM_SAVE:
            _samba_login_save_proc();
            break;

        default:
            break;
    }
}

SIGNAL_HANDLER int On_samba_login_create(const char* widgetname, void *userdata)
{
    if(strlen(thiz_samba_login_data.samba_svr_path) > 0)
        GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_PATH], "string", &thiz_samba_login_data.samba_svr_path);
    else
        GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_PATH], "string", SAMBA_PATH_HEADER);

    GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_USER], "string", &thiz_samba_login_data.samba_username);

    if(strlen(thiz_samba_login_data.samba_passwd) > 0)
        GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_PASSWD], "string", SAMBA_PASSWD_INTAGLIO);
    else
        GUI_SetProperty(thiz_samba_box_button[SAMBA_LOGIN_ITEM_PASSWD], "string", STR_ID_BLANK);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_samba_login_keypress(const char* widgetname, void *userdata)
{
    GUI_Event *event = (GUI_Event*)userdata;
    switch(event->type)
    {
        case GUI_KEYDOWN:
			switch(event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    app_log_info("[%s]VK_BOOK_TRIGGER VK_BOOK_TRIGGER VK_BOOK_TRIGGER\n", __func__);
                    GUI_EndDialog("after wnd_full_screen");
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_samba_login_box_keypress(GuiWidget *widget, void *userdata)
{
    GUI_Event *event = NULL;
    int box_sel;
    int ret = EVENT_TRANSFER_KEEPON;

    event = (GUI_Event *)userdata;
    switch(event->type)
    {
        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog(WND_SAMBA_LOGIN);
                        _samba_login_get_params(thiz_samba_login_ret);
                        if(thiz_samba_login_exit_cb != NULL)
                        {
                            thiz_samba_login_exit_cb(false, thiz_samba_login_ret);
                        }
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                case STBK_OK:
                    _samba_login_ok_key_proc();
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_UP:
                    GUI_GetProperty(BOX_SAMBA_LOGIN, "select", &box_sel);
                    if(box_sel == SAMBA_LOGIN_ITEM_PATH)
                    {
                        box_sel = SAMBA_LOGIN_ITEM_SAVE;
                        GUI_SetProperty(BOX_SAMBA_LOGIN, "select", &box_sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                case STBK_DOWN:
                    GUI_GetProperty(BOX_SAMBA_LOGIN, "select", &box_sel);
                    if(box_sel == SAMBA_LOGIN_ITEM_SAVE)
                    {
                        box_sel = SAMBA_LOGIN_ITEM_PATH;
                        GUI_SetProperty(BOX_SAMBA_LOGIN, "select", &box_sel);
                        ret = EVENT_TRANSFER_STOP;
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

status_t app_create_samba_login(AppSambaInfoClass *samba_in, AppSambaInfoClass *samba_ret, SambaLoginExitCb exit_cb)
{
    memset(&thiz_samba_login_data, 0, sizeof(AppSambaInfoClass));
    if(samba_in != NULL)
        memcpy(&thiz_samba_login_data, samba_in, sizeof(AppSambaInfoClass));
    if(samba_ret != NULL)
        memset(samba_ret, 0, sizeof(AppSambaInfoClass));
    thiz_samba_login_ret = samba_ret;
    thiz_samba_login_exit_cb = exit_cb;
    GUI_CreateDialog(WND_SAMBA_LOGIN);

    return GXCORE_SUCCESS;
}

#endif
