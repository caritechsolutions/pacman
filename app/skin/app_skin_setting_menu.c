/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"

#if SKIN_SUPPORT
#include "app_module.h"
#include "netapps/app_netapps_service.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#include "module/app_dir_module.h"
#include "service/gxhotplug.h"
#include "app_wnd_file_list.h"
#include <sys/stat.h>
#include <gxos/gxcore_os_core.h>
#include <gx_apps.h>
#include "app_wnd_system_setting_opt.h"

#include "app_skin_setting.h"
#include "app_skin_config.h"

#include "app_skin_xml.h"
#include "app_skin_flash.h"

enum SKIN_SETTING_ITEM_NAME
{
	SKIN_SETTING_ITEM_MODE = 0,  //online ,usb
	SKIN_SETTING_ITEM_URL,
	SKIN_SETTING_ITEM_EXEC,
	SKIN_SETTING_ITEM_TOTAL,
};

#define SKIN_ONLINE_URL_KEY                 "skin>online_url"
#define SKIN_USB_URL_KEY                    "skin>usb_url"

static char s_skin_current_mode = 0 ;
static char *s_file_path = NULL;
static char *s_file_path_bak = NULL;

static SystemSettingOpt  s_skin_setting_opt;
static SystemSettingItem s_skin_setting_item[SKIN_SETTING_ITEM_TOTAL];
static char *s_skin_setting_url  = NULL;
static char *s_file_setting_path = NULL;

int   gxapp_skin_online_set_url(char *url)
{
	if (url == NULL )
		return -1 ;

	APP_FREE(s_skin_setting_url);

	s_skin_setting_url = GxCore_Strdup(url);

	GxBus_ConfigSet(SKIN_ONLINE_URL_KEY,s_skin_setting_url);

	SKIN_DBG("[skin setm] set online url = %s \n",s_skin_setting_url);

	return 0 ;
}

int   gxapp_skin_usb_set_url(char *url)
{
	if (url == NULL )
		return -1 ;

	APP_FREE(s_file_setting_path);

	s_file_setting_path = GxCore_Strdup(url);

	GxBus_ConfigSet(SKIN_USB_URL_KEY,s_file_setting_path);

	SKIN_DBG("[skin setm] set usb url = %s \n",s_file_setting_path);

	return 0 ;
}

int   gxapp_skin_usb_set_memory_url(char *url)
{
	if (url == NULL )
		return -1 ;

	APP_FREE(s_file_setting_path);

	s_file_setting_path = GxCore_Strdup(url);

	SKIN_DBG("[skin setm] set usb memory url = %s \n",s_file_setting_path);

	return 0 ;
}

static void _app_skin_setting_url_input_release_cb(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if(data->out_name == NULL)
        return;

    strcpy(s_skin_setting_url, data->out_name);
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string = s_skin_setting_url;
    app_system_set_item_property(SKIN_SETTING_ITEM_URL, &s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty);
}

static int app_skin_setting_url_button_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    static PopKeyboard keyboard;
//printf("#### url callback \n");
    switch(key)
    {
        case STBK_OK:
        {
            memset(&keyboard, 0, sizeof(PopKeyboard));
            //keyboard.pos.x = WND_KEYBOARD_X;
            //keyboard.pos.y = WND_KEYBOARD_Y;

            keyboard.max_num = SKIN_ONLINE_URL_LEN-1;
            keyboard.out_name   = NULL;
            keyboard.change_cb  = NULL;
            keyboard.release_cb = _app_skin_setting_url_input_release_cb;
            keyboard.usr_data   = NULL;
            keyboard.in_name = s_skin_setting_url;
            multi_language_keyboard_create(&keyboard);
            break;
        }
        default:
            break;
    }
    return ret;
}

static int _app_skin_setting_path_filedlg_exit_cb(WndStatus ret)
{
    if(ret == WND_OK)
    {
#ifdef APP_SDK_VERSION_LOW
#else
        app_get_file_real_path(&s_file_path);
#endif

        if(s_file_path != NULL)
        {
            int str_len = 0;

            s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string = s_file_path;
            app_system_set_item_property(SKIN_SETTING_ITEM_URL, &s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty);

            ////bak////
            if(s_file_path_bak != NULL)
            {
                GxCore_Free(s_file_path_bak);
                s_file_path_bak = NULL;
            }

            str_len = strlen(s_file_path);
            s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if(s_file_path_bak != NULL)
            {
                memcpy(s_file_path_bak, s_file_path, str_len);
                s_file_path_bak[str_len] = '\0';
            }
        }
	}

    return 0;
}

static int app_skin_setting_path_button_press_callback(unsigned short key)
{

    int ret = EVENT_TRANSFER_KEEPON;
	//printf("#### usb callback \n");
    switch(key)
    {
        case STBK_OK:
        {
			SKIN_DBG("[skin setm] usb set path press ok \n");

			if(check_usb_status() == false)
			{
				PopDlg  pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_OK;
				pop.str = STR_ID_INSERT_USB;
				pop.mode = POP_MODE_UNBLOCK;
				pop.pos.x = APP_POP_DLG_POS_X;
				pop.pos.y = APP_POP_DLG_POS_Y;
				popdlg_create(&pop);
				return ret;
			}

			FileListParam file_para;
            memset(&file_para, 0, sizeof(file_para));

            file_para.cur_path = s_file_path_bak;
            file_para.dest_path = &s_file_path;
            file_para.suffix = "xml";
            file_para.pos_x = APP_WND_FILE_LIST_POS_X;
            file_para.pos_y = APP_WND_FILE_LIST_POS_Y;

#ifdef APP_SDK_VERSION_LOW
#else
            file_para.exit_cb = _app_skin_setting_path_filedlg_exit_cb;
#endif

            file_para.dest_mode = DEST_MODE_DIR;
#ifdef APP_SDK_VERSION_LOW
            WndStatus get_path_ret = WND_CANCLE;
            get_path_ret = app_get_file_path_dlg(&file_para);
           _app_skin_setting_path_filedlg_exit_cb(get_path_ret);
#else
            app_get_file_path_dlg(&file_para);
#endif

            break;
        }
        default:
            break;
    }
    return ret;
}

static int _app_skin_setting_exec_press_callback(unsigned short key)
{
    PopDlg  pop;

    if(key == STBK_OK)
    {
		if ( s_skin_current_mode == 0 )
		{
			if ( s_skin_setting_url )
			{
				SKIN_DBG("[skin setm] save online url = %s \n",s_skin_setting_url);
				GxBus_ConfigSet(SKIN_ONLINE_URL_KEY,s_skin_setting_url);
			}
		}
		else
		{
			if ( s_file_path_bak )
			{
				SKIN_DBG("[skin setm] usb src path = %s \n",s_file_path_bak);

				char *start = NULL ;
				char *end = NULL ;
				int str_len = 0;

				start = s_file_path_bak+1 ;
				end = strstr(start,"/");
				if ( end )
				{
					//SKIN_DBG("[skin setm] path1 = %s \n",end);
					start = end+1 ;
					//SKIN_DBG("[skin setm] path2 = %s \n",start);
					end = strstr(start,"/");

					if ( end )
					{
						str_len = end - s_file_path_bak ;
						SKIN_DBG("[skin setm] len = %d \n",str_len);
						s_file_path_bak[str_len] = '\0';
						SKIN_DBG("[skin setm] usb modify path2 = %s \n",s_file_path_bak);

						s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string = s_file_path_bak;
						app_system_set_item_property(SKIN_SETTING_ITEM_URL, &s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty);
					}
				}

				SKIN_DBG("[skin setm] usb save path = %s \n",s_file_path_bak);

				if ( gxapp_skin_check_usb_entry_path(s_file_path_bak) != 0 )
				{
					if ( s_file_setting_path != NULL )
						s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string= s_file_setting_path;
					else
						s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
					app_system_set_item_property(SKIN_SETTING_ITEM_URL, &s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty);

					GxCore_Free(s_file_path_bak);
					s_file_path_bak = NULL;
					s_file_path = NULL;

					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_OK;
					pop.str = "Save faile, no skin data in usb home directory!";
					pop.mode = POP_MODE_UNBLOCK;
					pop.pos.x = APP_POP_DLG_POS_X;
					pop.pos.y = APP_POP_DLG_POS_Y;
					popdlg_create(&pop);
					return EVENT_TRANSFER_KEEPON;
				}

				GxBus_ConfigSet(SKIN_USB_URL_KEY,s_file_path_bak);
				if(s_file_setting_path != NULL)
				{
					GxCore_Free(s_file_setting_path);
					s_file_setting_path = NULL;
				}
				s_file_setting_path = GxCore_Calloc(1, SKIN_USB_URL_LEN);
				GxBus_ConfigGet(SKIN_USB_URL_KEY, s_file_setting_path, SKIN_USB_URL_LEN, SKIN_USB_URL_DEFALUT);
				SKIN_DBG("[skin setm] get usb url = %s \n",s_file_setting_path);
			}
			else
			{
				SKIN_DBG("[skin setm] not set path \n");
				return EVENT_TRANSFER_STOP ;
			}
		}

		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.str = "Save Success!";
		pop.mode = POP_MODE_UNBLOCK;
		pop.pos.x = APP_POP_DLG_POS_X;
		pop.pos.y = APP_POP_DLG_POS_Y;
		popdlg_create(&pop);
    }
    return EVENT_TRANSFER_KEEPON;
}

static void app_skin_setting_url_item_init(void)
{
	s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemTitle = "URL";
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemType = ITEM_PUSH;
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string= s_skin_setting_url;
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemCallback.btnCallback.BtnPress= app_skin_setting_url_button_press_callback;
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemStatus = ITEM_NORMAL;
}

static void app_skin_setting_path_item_init(void)
{
	s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemTitle = "File Path";
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemType = ITEM_PUSH;
    if ( s_file_setting_path != NULL )
		s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string= s_file_setting_path;
	else
		s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemCallback.btnCallback.BtnPress= app_skin_setting_path_button_press_callback;
    s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemStatus = ITEM_NORMAL;
}

#if SKUSB_SUPPORT
static int _app_skin_setting_change_cb(int sel)
{
    if(sel == 0)
    {
        app_skin_setting_url_item_init();
    }
    else
    {
        app_skin_setting_path_item_init();
    }

	app_system_set_item_title(SKIN_SETTING_ITEM_URL,s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemTitle);
    app_system_set_item_property(SKIN_SETTING_ITEM_URL,&s_skin_setting_item[SKIN_SETTING_ITEM_URL].itemProperty);

	s_skin_current_mode = sel ;
    return 0;
}
#endif

static void app_skin_setting_mode_item_init(void)
{
	s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemTitle = "Mode";
	s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemType = ITEM_CHOICE;
#if SKUSB_SUPPORT
	if ( gxapp_skin_check_usb_device() == 0 )
	{
		s_skin_current_mode = 0 ;
		s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemProperty.itemPropertyCmb.content= "[Online]";
		s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemCallback.cmbCallback.CmbChange= NULL;
	}
	else
	{
		s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemProperty.itemPropertyCmb.content= "[Online,USB]";
		s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemCallback.cmbCallback.CmbChange= _app_skin_setting_change_cb;
	}
#else
	s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemProperty.itemPropertyCmb.content= "[Online]";
	s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemCallback.cmbCallback.CmbChange= NULL;
#endif
	s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemProperty.itemPropertyCmb.sel = s_skin_current_mode;
	s_skin_setting_item[SKIN_SETTING_ITEM_MODE].itemStatus = ITEM_NORMAL;
}

static void app_skin_setting_exec_item_init(void)
{
    s_skin_setting_item[SKIN_SETTING_ITEM_EXEC].itemTitle = "Save";
    s_skin_setting_item[SKIN_SETTING_ITEM_EXEC].itemType = ITEM_PUSH;
    s_skin_setting_item[SKIN_SETTING_ITEM_EXEC].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_skin_setting_item[SKIN_SETTING_ITEM_EXEC].itemCallback.btnCallback.BtnPress= _app_skin_setting_exec_press_callback;
    s_skin_setting_item[SKIN_SETTING_ITEM_EXEC].itemStatus = ITEM_NORMAL;
}

static int app_skin_setting_init(void)
{
    return 0;
}

static int app_skin_setting_exit_callback(ExitType exit_type)
{
	if(s_file_path_bak != NULL)
    {
        GxCore_Free(s_file_path_bak);
        s_file_path_bak = NULL;
    }
    s_file_path = NULL;
    return 0;
}

void app_create_skin_setting_menu(void)
{
	s_skin_setting_opt.menuTitle = "Setting";
	s_skin_setting_opt.itemNum = SKIN_SETTING_ITEM_TOTAL;
	s_skin_setting_opt.timeDisplay = TIP_HIDE;
	s_skin_setting_opt.item = s_skin_setting_item;

    app_skin_setting_init();

	app_skin_setting_mode_item_init();

	if(s_skin_current_mode == 0)
    {
        app_skin_setting_url_item_init();
    }
    else
    {
        app_skin_setting_path_item_init();
    }

	app_skin_setting_exec_item_init();

	s_skin_setting_opt.exit = app_skin_setting_exit_callback ;

	app_system_set_create(&s_skin_setting_opt);
}

char *gxapp_skin_online_get_url(void)
{
	return s_skin_setting_url ;
}

char *gxapp_skin_usb_get_url(void)
{
	return s_file_setting_path ;
}

int  gxapp_skin_setting_init(void)
{
	if ( s_skin_setting_url == NULL )
	{
		s_skin_setting_url = GxCore_Calloc(1, SKIN_ONLINE_URL_LEN);
		GxBus_ConfigGet(SKIN_ONLINE_URL_KEY, s_skin_setting_url, SKIN_ONLINE_URL_LEN, SKIN_ONLINE_URL_DEFALUT);
		SKIN_DBG("[skin setm] url = %s \n",s_skin_setting_url);
	}

	if ( s_file_setting_path == NULL )
	{
		s_file_setting_path = GxCore_Calloc(1, SKIN_USB_URL_LEN);
		GxBus_ConfigGet(SKIN_USB_URL_KEY, s_file_setting_path, SKIN_USB_URL_LEN, SKIN_USB_URL_DEFALUT);
		SKIN_DBG("[skin setm] path = %s \n",s_file_setting_path);
	}

	SKIN_DBG("[skin setm] update mode = %d \n",gxapp_skin_flash_get_update_mode());

	return 0 ;
}

int  gxapp_skin_setting_uninit(void)
{
	APP_FREE(s_skin_setting_url);

	if(s_file_setting_path != NULL)
    {
        GxCore_Free(s_file_setting_path);
        s_file_setting_path = NULL;
    }

	return 0 ;
}

#endif
