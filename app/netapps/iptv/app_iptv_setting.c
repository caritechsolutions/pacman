#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_wnd_file_list.h"
#include "app_iptv_list_service.h"
#include "app_iptv.h"
#include <gx_apps.h>
#include "app_default_params.h"


#define WND_KEYBOARD_X 360
#define WND_KEYBOARD_Y 130
#define TEXT_SYS_SET_TIMEZONE_UTC	"text_system_setting_timezone"
#define IMG_SYS_SET_TIMEZONE_UTC_BAK	"img_system_setting_timezone_back"

enum iptv_setting_itm
{
	IPTV_SETTING_PARAM1 = 0,
	IPTV_SETTING_PARAM2,
	IPTV_SETTING_PARAM3,
	IPTV_SETTING_PARAM4,
	IPTV_SETTING_SAVE,
	IPTV_SETTING_MAX
};

static IPTVSetCtrl iptv_setting_ctrl;
static SystemSettingOpt *iptv_setting_opt = NULL;
static SystemSettingItem *iptv_setting_item = NULL;
static char *iptv_param1 = NULL;
static char *iptv_param2 = NULL;
static char *iptv_param3 = NULL;
static char *iptv_param4 = NULL;
static int iptv_menu_flag = 0;

static void app_iptv_setting_itemdata_init(void)
{
	iptv_setting_opt=(SystemSettingOpt *)GxCore_Calloc(1, sizeof(SystemSettingOpt));
	iptv_setting_item=(SystemSettingItem *)GxCore_Calloc(1, sizeof(SystemSettingItem)*IPTV_SETTING_MAX);
	iptv_param1 = GxCore_Calloc(1, iptv_setting_ctrl.param_len[0]);
	iptv_param2 = GxCore_Calloc(1, iptv_setting_ctrl.param_len[1]);
	iptv_param3 = GxCore_Calloc(1, iptv_setting_ctrl.param_len[2]);
	iptv_param4 = GxCore_Calloc(1, iptv_setting_ctrl.param_len[3]);
}

static void app_iptv_setting_itemdata_destroy(void)
{
	APP_FREE(iptv_setting_opt);
	APP_FREE(iptv_setting_item);
	APP_FREE(iptv_param1);
	APP_FREE(iptv_param2);
	APP_FREE(iptv_param3);
	APP_FREE(iptv_param4);
}

static void app_iptv_setting_get(void)
{
	if(iptv_setting_ctrl.setting_get)
	{
		iptv_setting_ctrl.setting_get(iptv_param1, iptv_param2, iptv_param3, iptv_param4);
	}
}

static void app_iptv_setting_save_func(void)
{
	if(iptv_setting_ctrl.setting_set)
	{
		iptv_setting_ctrl.setting_set(iptv_param1, iptv_param2, iptv_param3, iptv_param4);
	}
	if(iptv_setting_ctrl.save_func)
	{
		iptv_setting_ctrl.save_func();
	}
}

void app_iptv_tip_show(char *buf)
{
	char* focus_win = NULL;
	char tmp_str[128];

	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win)
		return;

	if(strcasecmp(focus_win, WND_SYSTEM_SETTING))
		return;

	if(buf)
	{
		snprintf(tmp_str,sizeof(tmp_str)-1,"%s",buf);
	}
	else
	{
		memset(tmp_str, 0, sizeof(tmp_str));
	}
	GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC, "state", "show");
	GUI_SetProperty(IMG_SYS_SET_TIMEZONE_UTC_BAK, "state", "show");
	GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC, "string", tmp_str);
}

void app_iptv_show_status(int server_type)
{
	int status = -1;
	char *message = NULL;
	if((iptv_menu_flag>0)&&(server_type == iptv_setting_ctrl.server_type))
	{
		if(iptv_setting_ctrl.get_message)
		{
			message = iptv_setting_ctrl.get_message();
			if(message)
			{
				app_iptv_tip_show(message);
				GxCore_Free(message);
				return;
			}
		}
		if(iptv_setting_ctrl.get_status)
		{
			status = iptv_setting_ctrl.get_status();
			if(status == IPTV_LOGIN_RUNNING)
			{
				app_iptv_tip_show(STR_ID_LOGIN_WAIT);
			}
			else if(status == IPTV_LOGIN_SUCCESS)
			{
				app_iptv_tip_show(STR_ID_LOGIN_OK);
			}
			else if(status == IPTV_LOGIN_FAILED)
			{
				app_iptv_tip_show(STR_ID_LOGIN_FAIL);
			}
			else if(status == IPTV_LOGIN_PLEASE)
			{
				app_iptv_tip_show(STR_ID_PLEASE_LOGIN);
			}
			else
			{
				app_iptv_tip_show(NULL);
			}
		}
	}
}

static void _iptv_param1_input_release_cb(PopKeyboard *data)
{
	app_iptv_show_status(iptv_setting_ctrl.server_type);
	if(data->in_ret == POP_VAL_CANCEL)
		return;

	if(data->out_name == NULL)
		return;

	strcpy(iptv_param1, data->out_name);
	iptv_setting_item[IPTV_SETTING_PARAM1].itemProperty.itemPropertyBtn.string = iptv_param1;
	app_system_set_item_property(IPTV_SETTING_PARAM1, &iptv_setting_item[IPTV_SETTING_PARAM1].itemProperty);
}

static int app_iptv_setting_param1_button_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;
	static PopKeyboard keyboard;

	switch(key)
	{
		case STBK_OK:
		{
			memset(&keyboard, 0, sizeof(PopKeyboard));
			keyboard.pos.x = WND_KEYBOARD_X;
			keyboard.pos.y = WND_KEYBOARD_Y;

			keyboard.max_num = iptv_setting_ctrl.param_len[0]-1;
			keyboard.out_name	= NULL;
			keyboard.change_cb	= NULL;
			keyboard.release_cb = _iptv_param1_input_release_cb;
			keyboard.usr_data	= NULL;
			keyboard.in_name = iptv_param1;
			multi_language_keyboard_create(&keyboard);
			break;
		}
		default:
			break;
	}
	return ret;
}

static void _iptv_param2_input_release_cb(PopKeyboard *data)
{
	app_iptv_show_status(iptv_setting_ctrl.server_type);
	if(data->in_ret == POP_VAL_CANCEL)
		return;

	if(data->out_name == NULL)
		return;

	strcpy(iptv_param2, data->out_name);
	iptv_setting_item[IPTV_SETTING_PARAM2].itemProperty.itemPropertyBtn.string = iptv_param2;
	app_system_set_item_property(IPTV_SETTING_PARAM2, &iptv_setting_item[IPTV_SETTING_PARAM2].itemProperty);
}

static int app_iptv_setting_param2_button_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;
	static PopKeyboard keyboard;

	switch(key)
	{
		case STBK_OK:
		{
			memset(&keyboard, 0, sizeof(PopKeyboard));
			keyboard.pos.x = WND_KEYBOARD_X;
			keyboard.pos.y = WND_KEYBOARD_Y;

			keyboard.max_num = iptv_setting_ctrl.param_len[1]-1;
			keyboard.out_name	= NULL;
			keyboard.change_cb	= NULL;
			keyboard.release_cb = _iptv_param2_input_release_cb;
			keyboard.usr_data	= NULL;
			keyboard.in_name = iptv_param2;
			multi_language_keyboard_create(&keyboard);
			break;
		}
		default:
			break;
	}
	return ret;
}

static void _iptv_param3_input_release_cb(PopKeyboard *data)
{
	app_iptv_show_status(iptv_setting_ctrl.server_type);
	if(data->in_ret == POP_VAL_CANCEL)
		return;

	if(data->out_name == NULL)
		return;

	strcpy(iptv_param3, data->out_name);
	iptv_setting_item[IPTV_SETTING_PARAM3].itemProperty.itemPropertyBtn.string = iptv_param3;
	app_system_set_item_property(IPTV_SETTING_PARAM3, &iptv_setting_item[IPTV_SETTING_PARAM3].itemProperty);
}

static int app_iptv_setting_param3_button_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;
	static PopKeyboard keyboard;

	switch(key)
	{
		case STBK_OK:
		{
			memset(&keyboard, 0, sizeof(PopKeyboard));
			keyboard.pos.x = WND_KEYBOARD_X;
			keyboard.pos.y = WND_KEYBOARD_Y;

			keyboard.max_num = iptv_setting_ctrl.param_len[2]-1;
			keyboard.out_name	= NULL;
			keyboard.change_cb	= NULL;
			keyboard.release_cb = _iptv_param3_input_release_cb;
			keyboard.usr_data	= NULL;
			keyboard.in_name = iptv_param3;
			multi_language_keyboard_create(&keyboard);
			break;
		}
		default:
			break;
	}
	return ret;
}

static void _iptv_param4_input_release_cb(PopKeyboard *data)
{
	app_iptv_show_status(iptv_setting_ctrl.server_type);
	if(data->in_ret == POP_VAL_CANCEL)
		return;

	if(data->out_name == NULL)
		return;

	strcpy(iptv_param4, data->out_name);
	iptv_setting_item[IPTV_SETTING_PARAM4].itemProperty.itemPropertyBtn.string = iptv_param4;
	app_system_set_item_property(IPTV_SETTING_PARAM4, &iptv_setting_item[IPTV_SETTING_PARAM4].itemProperty);
}

static int app_iptv_setting_param4_button_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;
	static PopKeyboard keyboard;

	switch(key)
	{
		case STBK_OK:
		{
			memset(&keyboard, 0, sizeof(PopKeyboard));
			keyboard.pos.x = WND_KEYBOARD_X;
			keyboard.pos.y = WND_KEYBOARD_Y;

			keyboard.max_num = iptv_setting_ctrl.param_len[3]-1;
			keyboard.out_name	= NULL;
			keyboard.change_cb	= NULL;
			keyboard.release_cb = _iptv_param4_input_release_cb;
			keyboard.usr_data	= NULL;
			keyboard.in_name = iptv_param4;
			multi_language_keyboard_create(&keyboard);
			break;
		}
		default:
			break;
	}
	return ret;
}

static int app_iptv_save_button_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
		{
			app_iptv_setting_save_func();
			break;
		}

		default:
			break;
	}
	return ret;
}

static int app_iptv_login_exit_callback(ExitType exit_type)
{
	app_iptv_setting_itemdata_destroy();
	iptv_menu_flag = 0;
	return 0;
}

static void app_iptv_setting_param1_init(char *name)
{
	iptv_setting_item[IPTV_SETTING_PARAM1].itemTitle = name;
	iptv_setting_item[IPTV_SETTING_PARAM1].itemType = ITEM_PUSH;
	iptv_setting_item[IPTV_SETTING_PARAM1].itemProperty.itemPropertyBtn.string= iptv_param1;
	iptv_setting_item[IPTV_SETTING_PARAM1].itemCallback.btnCallback.BtnPress= app_iptv_setting_param1_button_press_callback;
	iptv_setting_item[IPTV_SETTING_PARAM1].itemStatus = ITEM_NORMAL;
}

static void app_iptv_setting_param2_init(char *name)
{
	iptv_setting_item[IPTV_SETTING_PARAM2].itemTitle = name;
	iptv_setting_item[IPTV_SETTING_PARAM2].itemType = ITEM_PUSH;
	iptv_setting_item[IPTV_SETTING_PARAM2].itemProperty.itemPropertyBtn.string= iptv_param2;
	iptv_setting_item[IPTV_SETTING_PARAM2].itemCallback.btnCallback.BtnPress = app_iptv_setting_param2_button_press_callback;
	iptv_setting_item[IPTV_SETTING_PARAM2].itemStatus = ITEM_NORMAL;
}

static void app_iptv_setting_param3_init(char *name)
{
	iptv_setting_item[IPTV_SETTING_PARAM3].itemTitle = name;
	iptv_setting_item[IPTV_SETTING_PARAM3].itemType = ITEM_PUSH;
	iptv_setting_item[IPTV_SETTING_PARAM3].itemProperty.itemPropertyBtn.string= iptv_param3;
	iptv_setting_item[IPTV_SETTING_PARAM3].itemCallback.btnCallback.BtnPress = app_iptv_setting_param3_button_press_callback;
	iptv_setting_item[IPTV_SETTING_PARAM3].itemStatus = ITEM_NORMAL;
}

static void app_iptv_setting_param4_init(char *name)
{
	iptv_setting_item[IPTV_SETTING_PARAM4].itemTitle = name;
	iptv_setting_item[IPTV_SETTING_PARAM4].itemType = ITEM_PUSH;
	iptv_setting_item[IPTV_SETTING_PARAM4].itemProperty.itemPropertyBtn.string= iptv_param4;
	iptv_setting_item[IPTV_SETTING_PARAM4].itemCallback.btnCallback.BtnPress = app_iptv_setting_param4_button_press_callback;
	iptv_setting_item[IPTV_SETTING_PARAM4].itemStatus = ITEM_NORMAL;
}

static void app_iptv_setting_save_init(void)
{
	int item_save = 0;

	item_save = iptv_setting_ctrl.param_num;
	iptv_setting_item[item_save].itemTitle = "Save";
	iptv_setting_item[item_save].itemType = ITEM_PUSH;
	iptv_setting_item[item_save].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
	iptv_setting_item[item_save].itemCallback.btnCallback.BtnPress= app_iptv_save_button_press_callback;
	iptv_setting_item[item_save].itemStatus = ITEM_NORMAL;
}

static void app_iptv_setting_param_init(void)
{
	if(iptv_setting_ctrl.param_num>0)
	{
		app_iptv_setting_param1_init(iptv_setting_ctrl.param1_name);
	}
	if(iptv_setting_ctrl.param_num>1)
	{
		app_iptv_setting_param2_init(iptv_setting_ctrl.param2_name);
	}
	if(iptv_setting_ctrl.param_num>2)
	{
		app_iptv_setting_param3_init(iptv_setting_ctrl.param3_name);
	}
	if(iptv_setting_ctrl.param_num>3)
	{
		app_iptv_setting_param4_init(iptv_setting_ctrl.param4_name);
	}
}

static void app_iptv_setting_colorkey_init(void)
{
    if(iptv_setting_ctrl.funckey)
    {
        MenuFuncKey colorkey;
        iptv_setting_ctrl.funckey(&colorkey);
        if(colorkey.redKey.keyPressFun) {
            iptv_setting_opt->menuFuncKey.redKey.keyName =     colorkey.redKey.keyName;
            iptv_setting_opt->menuFuncKey.redKey.keyPressFun = colorkey.redKey.keyPressFun;
        }
        if(colorkey.greenKey.keyPressFun) {
            iptv_setting_opt->menuFuncKey.greenKey.keyName =     colorkey.greenKey.keyName;
            iptv_setting_opt->menuFuncKey.greenKey.keyPressFun = colorkey.greenKey.keyPressFun;
        }
        if(colorkey.blueKey.keyPressFun) {
            iptv_setting_opt->menuFuncKey.blueKey.keyName =     colorkey.blueKey.keyName;
            iptv_setting_opt->menuFuncKey.blueKey.keyPressFun = colorkey.blueKey.keyPressFun;
        }
        if(colorkey.yellowKey.keyPressFun) {
            iptv_setting_opt->menuFuncKey.yellowKey.keyName =     colorkey.yellowKey.keyName;
            iptv_setting_opt->menuFuncKey.yellowKey.keyPressFun = colorkey.yellowKey.keyPressFun;
        }
    }
}

static void app_iptv_setting_menu_init(void)
{
	app_iptv_setting_get();
	memset(iptv_setting_opt, 0 ,sizeof(SystemSettingOpt));
	memset(iptv_setting_item, 0 ,sizeof(SystemSettingItem)*IPTV_SETTING_MAX);
	iptv_setting_opt->menuTitle = (char *)(iptv_setting_ctrl.name);
	iptv_setting_opt->timeDisplay = TIP_HIDE;
	iptv_setting_opt->item = iptv_setting_item;
	iptv_setting_opt->itemNum = 1 + iptv_setting_ctrl.param_num;
	app_iptv_setting_param_init();
	iptv_setting_opt->exit = app_iptv_login_exit_callback;
	app_iptv_setting_save_init();
    app_iptv_setting_colorkey_init();
	app_system_set_create(iptv_setting_opt);
	app_iptv_show_status(iptv_setting_ctrl.server_type);
}

void app_iptv_create_setting_menu(IPTVSetCtrl *set)
{
	int i = 0;
	iptv_menu_flag = 1;
	memcpy(&iptv_setting_ctrl, set, sizeof(IPTVSetCtrl));
	for (i = 0; i < MAX_IPTV_ITEM_NUM; i++)
	{
		if (iptv_setting_ctrl.param_len[i] == 0)
			iptv_setting_ctrl.param_len[i] = IPTV_PARAM_LEN;
	}
	GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"string",NULL);
	app_iptv_setting_itemdata_init();
	app_iptv_setting_menu_init();

	GUI_SetProperty(TEXT_SYS_SET_TIMEZONE_UTC,"state","show");
	GUI_SetProperty(IMG_SYS_SET_TIMEZONE_UTC_BAK,"state","show");
}

#endif
