#include "app.h"
#if MERGE_DB_SUPPORT
#include "app_pop.h"
#include <gxcore.h>
#include "app_wnd_file_list.h"
#include "app_windows.h"
#include "app_utility.h"
#include "app_wnd_system_setting_opt.h"
#include "app_keyboard_language.h"
#include <gx_apps.h>
#ifdef LINUX_OS
#define FILE_MODE "w+"
#define GX_DEFAULT_DB_PATH	"/home/root/default_data.db"
#define NETUPG_DB_FILE_PATH "/tmp/user_data.db"
#else
#define FILE_MODE "a+"//"rc"
#define GX_DEFAULT_DB_PATH	"/default_data.db"
#define NETUPG_DB_FILE_PATH "/mnt/user_data.db"
#endif
#define DB_SUFFIX "db;DB"
#define DEFAULT_DB "/default_data.db"
#define USER_DB "/user_data.db"
#define GX_PM_VERIFY_FILE_PATH "/home/gx/verify.db"

#define NETUPG_URL_EDIT "edit_system_setting_opt3"
#define NETUPG_USER_EDIT "edit_system_setting_opt4"
#define NETUPG_PWD_EDIT "edit_system_setting_opt5"
#define NETUPG_HTTP_PREFIX "http://"
#define NETUPG_HTTPS_PREFIX "https://"
#define NETUPG_FTP_PREFIX "ftp://"
#define NETUPG_HTTP_URL	"netapps>netupg_httpurl"
#define NETUPG_FTP_URL	"netapps>netupg_ftpurl"
#define NETUPG_FTP_USER "netapps>netupg_ftpuser"
#define NETUPG_FTP_PWD "netapps>netupg_ftppwd"
#define NETUPG_DEFAULT_HTTPURL "www.stbware.com/gx/upgrade"
#define NETUPG_DEFAULT_FTPURL "ftp.stbware.com/gx/upgrade"
#define NETUPG_DEFAULT_FTPUSER "gx"
#define NETUPG_DEFAULT_FTPPWD "gx"
#define NETUPG_MAX_URL_LEN (512)
#define NETUPG_MAX_USER_LEN (128)
#define NETUPG_MAX_PWD_LEN (128)

enum COLOR_ITEM_NAME
{
	ITEM_NETUPGRADE_PROTOCOL,
	ITEM_NETUPGRADE_URL,
	ITEM_NETUPGRADE_USER,
	ITEM_NETUPGRADE_PASSWORD,
	ITEM_NETUPGRADE_START,
	ITEM_NETUPGRADE_TOTAL,
};

typedef enum
{
    NET_UPG_PROTOCOL_HTTP = 0,
    NET_UPG_PROTOCOL_FTP,
    NET_UPG_PROTOCOL_TOTAL,
}NetUpgProtocol;


typedef struct
{
    NetUpgProtocol protocol;
    char *url;
    char *username;
    char *password;
}NetUpgPara;

static char *s_netupg_title_str[] = {
    "Protocol",
    "URL",
    "User",
    "Password",
    "Start",
};
NetUpgPara s_netupgrade_para;

static SystemSettingItem s_net_upgrade_item[ITEM_NETUPGRADE_TOTAL];
static SystemSettingOpt s_net_upgrade_setting_opt;
extern signed char init_utf8_position(const char* InStr,unsigned int *LastCharSel, unsigned int *MaxCharCount);

static void netupg_url_full_keyboard_proc(PopKeyboard *data)
{
	 if(data->in_ret == POP_VAL_CANCEL)
        	return;

    if(trim(data->out_name) == NULL)
       	     return;

	APP_FREE(s_netupgrade_para.url);
	s_netupgrade_para.url = GxCore_Strdup(data->out_name);
	if(s_netupgrade_para.protocol == NET_UPG_PROTOCOL_HTTP)
	{
		GxBus_ConfigSet(NETUPG_HTTP_URL, s_netupgrade_para.url);
	}
	else
	{
		GxBus_ConfigSet(NETUPG_FTP_URL, s_netupgrade_para.url);
	}
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.string = s_netupgrade_para.url;
    app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
}

static void netupg_user_full_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        	return;

    if(trim(data->out_name) == NULL)
       	     return;

    APP_FREE(s_netupgrade_para.username);
	s_netupgrade_para.username = GxCore_Strdup(data->out_name);
    GxBus_ConfigSet(NETUPG_FTP_USER, s_netupgrade_para.username);
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_netupgrade_para.username;
    app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
}

static void netupg_pwd_full_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        	return;

    if(trim(data->out_name) == NULL)
       	     return;

    APP_FREE(s_netupgrade_para.password);
    s_netupgrade_para.password = GxCore_Strdup(data->out_name);
    GxBus_ConfigSet(NETUPG_FTP_PWD, s_netupgrade_para.password);
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_netupgrade_para.password;
    app_system_set_item_property(ITEM_NETUPGRADE_PASSWORD,&s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty);
}

static int app_netupgrade_protocol_change_cb(int sel)
{
    char url[NETUPG_MAX_URL_LEN];
    char user[NETUPG_MAX_USER_LEN];
    char pwd[NETUPG_MAX_PWD_LEN];

    memset(url, 0, sizeof(url));
    memset(user, 0, sizeof(user));
    memset(pwd, 0, sizeof(pwd));

    APP_FREE(s_netupgrade_para.url);
    APP_FREE(s_netupgrade_para.username);
    APP_FREE(s_netupgrade_para.password);

    if(sel == NET_UPG_PROTOCOL_HTTP)
    {
        s_netupgrade_para.protocol = NET_UPG_PROTOCOL_HTTP;
        GxBus_ConfigGet(NETUPG_HTTP_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_HTTPURL);
        s_netupgrade_para.url = GxCore_Strdup(url);
        s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.string = s_netupgrade_para.url;
        s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_netupgrade_para.username;
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_netupgrade_para.password;
        app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_PASSWORD,&s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty);
        app_system_set_item_state_update(ITEM_NETUPGRADE_USER,ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_NETUPGRADE_PASSWORD,ITEM_DISABLE);
    }
    else
    {
        s_netupgrade_para.protocol = NET_UPG_PROTOCOL_FTP;
        GxBus_ConfigGet(NETUPG_FTP_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_FTPURL);
        GxBus_ConfigGet(NETUPG_FTP_USER, user, NETUPG_MAX_USER_LEN, NETUPG_DEFAULT_FTPUSER);
        GxBus_ConfigGet(NETUPG_FTP_PWD, pwd, NETUPG_MAX_PWD_LEN, NETUPG_DEFAULT_FTPPWD);
        s_netupgrade_para.url = GxCore_Strdup(url);
        s_netupgrade_para.username = GxCore_Strdup(user);
        s_netupgrade_para.password = GxCore_Strdup(pwd);
        s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.string = s_netupgrade_para.url;
        s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_netupgrade_para.username;
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_netupgrade_para.password;
        app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_PASSWORD,&s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty);
        app_system_set_item_state_update(ITEM_NETUPGRADE_USER,ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_NETUPGRADE_PASSWORD,ITEM_NORMAL);
    }
    return 0;
}

static int app_netupgrade_url_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    char *str = NULL;
    static PopKeyboard keyboard;
    switch (key)
    {
        case STBK_OK:
            {
                GUI_GetProperty(NETUPG_URL_EDIT, "string", &str);
                APP_FREE(s_netupgrade_para.url);
                s_netupgrade_para.url = GxCore_Strdup(str);
                if(s_netupgrade_para.protocol == NET_UPG_PROTOCOL_HTTP)
                {
                    GxBus_ConfigSet(NETUPG_HTTP_URL, s_netupgrade_para.url);
                }
                else
                {
                    GxBus_ConfigSet(NETUPG_FTP_URL, s_netupgrade_para.url);
                }
                memset(&keyboard, 0, sizeof(PopKeyboard));
                keyboard.in_name    = s_netupgrade_para.url;
                keyboard.max_num = NETUPG_MAX_URL_LEN-1;
                keyboard.out_name   = NULL;
                keyboard.change_cb  = NULL;
                keyboard.release_cb = netupg_url_full_keyboard_proc;
                keyboard.usr_data   = GXBUSCONFIG_INVALID;
                keyboard.pos.x = 500;
                multi_language_keyboard_create(&keyboard);
                ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_0:
        case STBK_1:
        case STBK_2:
        case STBK_3:
        case STBK_4:
        case STBK_5:
        case STBK_6:
        case STBK_7:
        case STBK_8:
        case STBK_9:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        case STBK_LEFT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_netupgrade_para.url,&lastsel,&maxcount);
                GUI_GetProperty(NETUPG_URL_EDIT, "select", &sel);
                if(sel == 0)
                {
                    sel=lastsel;
                    GUI_SetProperty(NETUPG_URL_EDIT, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_RIGHT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_netupgrade_para.url,&lastsel,&maxcount);
                GUI_GetProperty(NETUPG_URL_EDIT, "select", &sel);
                if(sel == lastsel)
                {
                    sel=0;
                    GUI_SetProperty(NETUPG_URL_EDIT, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
            {
                GUI_GetProperty(NETUPG_URL_EDIT, "string", &str);
                APP_FREE(s_netupgrade_para.url);
                s_netupgrade_para.url = GxCore_Strdup(str);
                if(s_netupgrade_para.protocol == NET_UPG_PROTOCOL_HTTP)
                {
                    GxBus_ConfigSet(NETUPG_HTTP_URL, s_netupgrade_para.url);
                }
                else
                {
                    GxBus_ConfigSet(NETUPG_FTP_URL, s_netupgrade_para.url);
                }
                ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        default:
            break;
    }
    return ret;
}

static int app_netupgrade_user_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    static PopKeyboard keyboard;
    char *str = NULL;
    switch (key)
    {
        case STBK_OK:
            {
                GUI_GetProperty(NETUPG_USER_EDIT, "string", &str);
                APP_FREE(s_netupgrade_para.username);
                s_netupgrade_para.username = GxCore_Strdup(str);
                GxBus_ConfigSet(NETUPG_FTP_USER, s_netupgrade_para.username);

                memset(&keyboard, 0, sizeof(PopKeyboard));
                keyboard.in_name    = s_netupgrade_para.username;
                keyboard.max_num = NETUPG_MAX_USER_LEN-1;
                keyboard.out_name   = NULL;
                keyboard.change_cb  = NULL;
                keyboard.release_cb = netupg_user_full_keyboard_proc;
                keyboard.usr_data   = GXBUSCONFIG_INVALID;
                keyboard.pos.x = 500;
                multi_language_keyboard_create(&keyboard);
                ret = EVENT_TRANSFER_KEEPON;
            }
        case STBK_0:
        case STBK_1:
        case STBK_2:
        case STBK_3:
        case STBK_4:
        case STBK_5:
        case STBK_6:
        case STBK_7:
        case STBK_8:
        case STBK_9:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        case STBK_LEFT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_netupgrade_para.username,&lastsel,&maxcount);
                GUI_GetProperty(NETUPG_USER_EDIT, "select", &sel);
                if(sel == 0)
                {
                    sel=lastsel;
                    GUI_SetProperty(NETUPG_USER_EDIT, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_RIGHT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_netupgrade_para.username,&lastsel,&maxcount);
                GUI_GetProperty(NETUPG_USER_EDIT, "select", &sel);
                if(sel == lastsel)
                {
                    sel=0;
                    GUI_SetProperty(NETUPG_USER_EDIT, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
            {
                GUI_GetProperty(NETUPG_USER_EDIT, "string", &str);
                APP_FREE(s_netupgrade_para.username);
                s_netupgrade_para.username = GxCore_Strdup(str);
                GxBus_ConfigSet(NETUPG_FTP_USER, s_netupgrade_para.username);

                ret = EVENT_TRANSFER_KEEPON;
            }
        default:
            break;
    }
    return ret;
}

static int app_netupgrade_pwd_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    static PopKeyboard keyboard;
    char *str = NULL;
    switch (key)
    {
        case STBK_OK:
            {
                GUI_GetProperty(NETUPG_PWD_EDIT, "string", &str);
                APP_FREE(s_netupgrade_para.password);
                s_netupgrade_para.password = GxCore_Strdup(str);
                GxBus_ConfigSet(NETUPG_FTP_PWD, s_netupgrade_para.password);

                memset(&keyboard, 0, sizeof(PopKeyboard));
                keyboard.in_name    = s_netupgrade_para.password;
                keyboard.max_num = NETUPG_MAX_PWD_LEN-1;
                keyboard.out_name   = NULL;
                keyboard.change_cb  = NULL;
                keyboard.release_cb = netupg_pwd_full_keyboard_proc;
                keyboard.usr_data   = GXBUSCONFIG_INVALID;
                keyboard.pos.x = 500;
                multi_language_keyboard_create(&keyboard);
                ret = EVENT_TRANSFER_KEEPON;
            }
        case STBK_0:
        case STBK_1:
        case STBK_2:
        case STBK_3:
        case STBK_4:
        case STBK_5:
        case STBK_6:
        case STBK_7:
        case STBK_8:
        case STBK_9:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        case STBK_LEFT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_netupgrade_para.password,&lastsel,&maxcount);
                GUI_GetProperty(NETUPG_PWD_EDIT, "select", &sel);
                if(sel == 0)
                {
                    sel=lastsel;
                    GUI_SetProperty(NETUPG_PWD_EDIT, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_RIGHT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_netupgrade_para.password,&lastsel,&maxcount);
                GUI_GetProperty(NETUPG_PWD_EDIT, "select", &sel);
                if(sel == lastsel)
                {
                    sel=0;
                    GUI_SetProperty(NETUPG_PWD_EDIT, "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
            {
                GUI_GetProperty(NETUPG_PWD_EDIT, "string", &str);
                APP_FREE(s_netupgrade_para.password);
                s_netupgrade_para.password = GxCore_Strdup(str);
                GxBus_ConfigSet(NETUPG_FTP_PWD, s_netupgrade_para.password);

                ret = EVENT_TRANSFER_KEEPON;
            }
        default:
            break;
    }
    return ret;
}



static void app_netupgrade_protocol_item_init(void)
{
	s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemTitle = s_netupg_title_str[ITEM_NETUPGRADE_PROTOCOL];
	s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemType = ITEM_CHOICE;
	s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemProperty.itemPropertyCmb.content= "[HTTP,FTP]";
	s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemProperty.itemPropertyCmb.sel = s_netupgrade_para.protocol;
	s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemCallback.cmbCallback.CmbChange = app_netupgrade_protocol_change_cb;
	s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemStatus = ITEM_NORMAL;
}

static void app_netupgrade_url_item_init(void)
{
	s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemTitle = s_netupg_title_str[ITEM_NETUPGRADE_URL];
	s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemType = ITEM_EDIT;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.format = "edit_string";
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.maxlen = "512";
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyEdit.string = s_netupgrade_para.url;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemCallback.editCallback.EditReachEnd= NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemCallback.editCallback.EditPress=app_netupgrade_url_edit_press_callback;
	s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemStatus = ITEM_NORMAL;
}

static void app_netupgrade_username_item_init(void)
{
	s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemTitle = s_netupg_title_str[ITEM_NETUPGRADE_USER];
	s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemType = ITEM_EDIT;
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.format = "edit_string";
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.maxlen = "128";
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_netupgrade_para.username;
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemCallback.editCallback.EditReachEnd= NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemCallback.editCallback.EditPress= app_netupgrade_user_edit_press_callback;
    if(s_netupgrade_para.protocol == NET_UPG_PROTOCOL_HTTP)
    {
        s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemStatus = ITEM_DISABLE;
    }
    else
    {
	    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemStatus = ITEM_NORMAL;
    }
}

static void app_netupgrade_password_item_init(void)
{
	s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemTitle = s_netupg_title_str[ITEM_NETUPGRADE_PASSWORD];
	s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemType = ITEM_EDIT;
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.format = "edit_string";
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.maxlen = "128";
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_netupgrade_para.password;
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemCallback.editCallback.EditReachEnd= NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemCallback.editCallback.EditPress= app_netupgrade_pwd_edit_press_callback;
    if(s_netupgrade_para.protocol == NET_UPG_PROTOCOL_HTTP)
    {
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemStatus = ITEM_NORMAL;
    }
}


static void app_netupgrade_exec_item_init(int (*start_btn_press)(unsigned short key))
{
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemTitle = STR_ID_START;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemType = ITEM_PUSH;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemCallback.btnCallback.BtnPress = start_btn_press;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemStatus = ITEM_NORMAL;
}

static void app_create_net_mergedb_menu(SystemSettingOpt *settingOpt, int (*start_btn_press)(unsigned short key))
{
    char url[NETUPG_MAX_URL_LEN];

    memset(&s_net_upgrade_setting_opt, 0, sizeof(SystemSettingOpt));
    memcpy(&s_net_upgrade_setting_opt,settingOpt,sizeof(SystemSettingOpt));
    s_net_upgrade_setting_opt.item = s_net_upgrade_item;
    s_netupgrade_para.protocol = NET_UPG_PROTOCOL_HTTP;
    GxBus_ConfigGet(NETUPG_HTTP_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_HTTPURL);
    s_netupgrade_para.url = GxCore_Strdup(url);
    s_netupgrade_para.username = NULL;
    s_netupgrade_para.password = NULL;


    app_netupgrade_protocol_item_init();
    app_netupgrade_url_item_init();
    app_netupgrade_username_item_init();
    app_netupgrade_password_item_init();
    app_netupgrade_exec_item_init(start_btn_press);

    app_system_set_create(&s_net_upgrade_setting_opt);
}
typedef struct
{
	unsigned int handle;
	unsigned int content_length;
	unsigned int download_size;
	unsigned int finish;
}NetUpgDownload;
enum
{
	EXPORT_USER_DB,
	EXPORT_DEF_DB,
};

typedef enum
{
	MERGE_DB_TYPE,
	MERGE_EXPORT_SWITCH,
	MERGE_DB_PATH,
	MERGE_DB_OK,
}GxMergeDB;

typedef enum
{
	MERGE_DB_EXPORT,
	MERGE_DB_INPUT_BY_USB,
	MERGE_DB_INPUT_BY_NET,
}MergeDbTypeSel;

enum gx_pm_verify_flag
{
	GX_PM_VERIFY_ERR = 0,
	GX_PM_VERIFY_OK
};
static char *s_file_path_bak = NULL;
static char *s_file_path = NULL;
static char *s_net_file_path = NULL;
static NetUpgDownload netupg_download_process;
extern void app_reboot(void);
extern status_t GxBus_PmCreateDefaultDb(int8_t* file_patch);
static event_list* netupg_download_process_timer = NULL;

static void netupg_download_process_timer_stop(void)
{
	if(netupg_download_process_timer)
	{
		remove_timer(netupg_download_process_timer);
		netupg_download_process_timer = NULL;
	}
}
static void netupg_software_download_stop(void)
{
	netupg_download_process_timer_stop();
	if(netupg_download_process.handle)
	{
		gxapps_curl_async_abort(netupg_download_process.handle);
	}
	memset(&netupg_download_process, 0, sizeof(NetUpgDownload));
	if(GXCORE_FILE_EXIST == GxCore_FileExists(NETUPG_DB_FILE_PATH))
	{
		GxCore_FileDelete(NETUPG_DB_FILE_PATH);
	}
}
static int netupg_download_process_timer_timeout(void *userdata)
{
    int Ret;
    int32_t init_value;
    if(netupg_download_process.finish)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec= 3;

        timer_stop(netupg_download_process_timer);
        if(netupg_download_process.finish == 1)
        {
            if (GxCore_FileExists(NETUPG_DB_FILE_PATH) != GXCORE_FILE_EXIST)
            {
                pop.str = STR_ID_ERR_DATE;
                popdlg_create(&pop);
                return -1;
            }
            GxBus_PmLock();
            Ret = GxBus_PmLoadDefault(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t*)NETUPG_DB_FILE_PATH);
            init_value = 0;
            GxBus_ConfigSetInt(FAV_INIT_KEY, init_value);
            GxBus_PmUnlock();
            if(Ret != 0)
            {
                popdlg_create(&pop);
            }
            else
            {
                pop.str = STR_ID_REBOOT_NOW;
                popdlg_create(&pop);
                GUI_SetInterface("flush",NULL);
                GxCore_ThreadDelay(2000);
                app_reboot();
            }
        }
        else
        {
            pop.str = STR_ID_ERR_DATE;
            popdlg_create(&pop);
        }
    }
    return 0;
}

static void netupg_download_process_timer_reset(void)
{
	if (reset_timer(netupg_download_process_timer) != 0)
	{
		netupg_download_process_timer = create_timer(netupg_download_process_timer_timeout, 500, NULL, TIMER_REPEAT);
	}
}

#ifdef ECOS_OS
int _export_default_db(char* file_path)
{
#define FILE_BUF_SIZE (1024)
	int ret = GXCORE_ERROR;
	handle_t default_db_file = 0;
	handle_t export_file = 0;
	uint64_t write_size = 0,read_size = 0;
	GxFileInfo info;
	char buf[FILE_BUF_SIZE];

	if(file_path == NULL)
	{
		return GXCORE_ERROR;
	}

	export_file = GxCore_Open((const char*)file_path, "w");
	if(export_file<0)
	{
		goto end;
	}

	default_db_file = GxCore_Open(GX_DEFAULT_DB_PATH, "r");
	if(default_db_file<0)
	{
		goto end;
	}

	GxCore_GetFileInfo(GX_DEFAULT_DB_PATH, &info);

	while(info.size_by_bytes - write_size >0)
	{
		uint32_t size = 0;

		read_size = ((info.size_by_bytes - write_size) > FILE_BUF_SIZE)?FILE_BUF_SIZE:(info.size_by_bytes - write_size);
		memset(buf, 0, FILE_BUF_SIZE);
		size = GxCore_Read(default_db_file,buf,1,read_size);

		size = GxCore_Write(export_file,buf,1,size);
		write_size += size;
	}

	if(write_size == info.size_by_bytes)
	{
		ret = GXCORE_SUCCESS;
	}
end:
	if(-1!=export_file)
		GxCore_Close(export_file);
	if(-1!=default_db_file)
		GxCore_Close(default_db_file);

	return ret;
}
#endif

static status_t export_db(void )
{
    uint32_t cmb_sel = 0;
    char buf[100] = {0};
    int ret = GXCORE_ERROR;
    PopDlg  pop;

    if(s_file_path == NULL)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.str = STR_ID_ERR_PATH;
		pop.type = POP_TYPE_OK;
		pop.format = POP_FORMAT_DLG;
		pop.mode = POP_MODE_UNBLOCK;
		pop.timeout_sec= 3;
		popdlg_create(&pop);
		return GXCORE_ERROR;
	}

    memset(buf, 0, sizeof(buf));
    GUI_GetProperty("cmb_export_switch", "select", &cmb_sel);
    if(cmb_sel == EXPORT_USER_DB)
    {
        sprintf(buf,"%s%s",s_file_path,USER_DB);
        ret = GxBus_PmCreateDefaultDb((int8_t*)buf);
    }
    else if(cmb_sel == EXPORT_DEF_DB)
    {
#ifdef LINUX_OS
        sprintf(buf,"cp %s %s%s",GX_DEFAULT_DB_PATH,s_file_path,DEFAULT_DB);
        ret = system(buf);
        if(ret != -1 && ret !=127)
        {
            ret = GXCORE_SUCCESS;
        }
#else
        sprintf(buf,"%s%s",s_file_path,DEFAULT_DB);
        ret = _export_default_db(buf);
#endif
    }
#ifdef LINUX_OS
    system("sync");
#endif
    memset(&pop, 0, sizeof(PopDlg));
    if(ret == GXCORE_SUCCESS)
    {
        pop.str = STR_ID_SUCCESS;
    }
    else
    {
        pop.str = STR_ID_ERR_DATE;
    }

    pop.type = POP_TYPE_OK;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec= 3;
    popdlg_create(&pop);

    return ret;
}

static void netupg_db_download_callback(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

    if(message == GXCURL_STATE_COMPLETE)
    {

        if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
        {
            netupg_download_process.finish = 1;
        }
        else
        {
            netupg_download_process.finish = 2;
        }
    }
}

static int _app_merge_db_filedlg_exit_cb(WndStatus ret)
{
    if(ret == WND_OK)
    {
        app_get_file_real_path(&s_file_path);
        if(s_file_path != NULL)
        {
            int str_len = 0;

            //s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty.itemPropertyBtn.string = s_file_path;
            //app_system_set_item_property(ITEM_UPGRADE_PATH, &s_upgrade_item[ITEM_UPGRADE_PATH].itemProperty);
            app_log_debug("\n[func:%s] s_file_path:(%s)\n",__func__,s_file_path);
            GUI_SetProperty("btn_merge_db_filepath", "string", (void*)s_file_path);

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
    return 0 ;
}

static int app_netupgrade_start_button_press_callback(unsigned short key)
{
    char *auth_str = NULL;
    static GUI_Event event;
    event.type = GUI_KEYDOWN;

    if(key == STBK_OK)
    {
        event.key.sym = STBK_EXIT;
        if(s_net_file_path != NULL)
        {
            GxCore_Free(s_net_file_path);
            s_net_file_path = NULL;
        }
        if((s_netupgrade_para.protocol == NET_UPG_PROTOCOL_FTP)&&(auth_str = netupg_ftp_get_authstr(s_netupgrade_para.username, s_netupgrade_para.password)))
        {
            if(0 == strncmp(s_netupgrade_para.url, NETUPG_FTP_PREFIX, strlen(NETUPG_FTP_PREFIX)))
            {
                s_net_file_path = (char *)GxCore_Calloc(1, strlen(s_netupgrade_para.url) + strlen(auth_str) + 1);
                sprintf(s_net_file_path, "%s%s%s", NETUPG_FTP_PREFIX, auth_str, (s_netupgrade_para.url + strlen(NETUPG_FTP_PREFIX)));
            }
            else
            {

                s_net_file_path = (char *)GxCore_Malloc(strlen(s_netupgrade_para.url)+strlen(auth_str) + strlen(NETUPG_FTP_PREFIX)+1);
                sprintf(s_net_file_path, "%s%s%s", NETUPG_FTP_PREFIX, auth_str, s_netupgrade_para.url);
            }
            GxCore_Free(auth_str);
        }
        else
        {
            if((0 == strncmp(s_netupgrade_para.url, NETUPG_HTTP_PREFIX, strlen(NETUPG_HTTP_PREFIX)))||(0 == strncmp(s_netupgrade_para.url, NETUPG_HTTPS_PREFIX, strlen(NETUPG_HTTPS_PREFIX))))
            {
                s_net_file_path = (char *)GxCore_Malloc(strlen(s_netupgrade_para.url)+1);
                sprintf(s_net_file_path, "%s",s_netupgrade_para.url);
            }
            else
            {
                s_net_file_path = (char *)GxCore_Malloc(strlen(NETUPG_HTTP_PREFIX)+strlen(s_netupgrade_para.url)+1);
                sprintf(s_net_file_path, "%s%s", NETUPG_HTTP_PREFIX, s_netupgrade_para.url);
            }
        }


        if(s_net_file_path != NULL)
        {
            GUI_SetProperty("btn_merge_db_filepath", "string", (void*)s_net_file_path);
        }
    }
    else
    {
        event.key.sym = key;
    }
    GUI_SendEvent(WND_SYSTEM_SETTING, &event);
    return 0 ;
}

static status_t app_merge_db_path_button_press(MergeDbTypeSel type)
{
    if(type != MERGE_DB_INPUT_BY_NET && check_usb_status() == false)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_INSERT_USB;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
    }
    else
    {
        FileListParam file_para;
        memset(&file_para, 0, sizeof(file_para));

        file_para.cur_path = s_file_path_bak;
        file_para.dest_path = &s_file_path;
        file_para.suffix = DB_SUFFIX;
        file_para.exit_cb = _app_merge_db_filedlg_exit_cb;
        if(type == MERGE_DB_INPUT_BY_USB)
        {
            file_para.dest_mode = DEST_MODE_FILE;
            app_get_file_path_dlg(&file_para);
        }
        else if(type == MERGE_DB_EXPORT)
        {
            file_para.dest_mode = DEST_MODE_DIR;
            app_get_file_path_dlg(&file_para);
        }
        else if(type == MERGE_DB_INPUT_BY_NET)
        {
            SystemSettingOpt upgrade_setting_opt={0};
            upgrade_setting_opt.menuTitle = STR_ID_MERGE_DB_BY_NET;
            upgrade_setting_opt.itemNum = ITEM_NETUPGRADE_TOTAL;
            upgrade_setting_opt.timeDisplay = TIP_HIDE;
           upgrade_setting_opt.bottmTipDisplay= TIP_HIDE;
            app_create_net_mergedb_menu(&upgrade_setting_opt,app_netupgrade_start_button_press_callback);
            GUI_SetProperty(WND_SYSTEM_SETTING, "back_ground", "null");
            if( APP_THEME_CLASSIC_DARK || APP_THEME_CLASSIC_BLUE ){
                GUI_SetProperty("img_system_setting_bottom", "state", "hide");
            }
            app_system_set_focus_item(ITEM_NETUPGRADE_PROTOCOL);
        }
    }
    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER  int app_merge_db_path_press(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	uint32_t cmb_sel = 0;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
				GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
				app_merge_db_path_button_press(cmb_sel);
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_merge_db_create(const char* widgetname, void *usrdata)
{
	s_file_path =NULL;
	if(s_net_file_path != NULL)
	{
		GxCore_Free(s_net_file_path);
		s_net_file_path = NULL;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_merge_db_destroy(const char* widgetname, void *usrdata)
{
    netupg_software_download_stop();
	if(s_file_path_bak != NULL)
	{
		GxCore_Free(s_file_path_bak);
		s_file_path_bak = NULL;
	}
	if(s_net_file_path != NULL)
	{
		GxCore_Free(s_net_file_path);
		s_net_file_path = NULL;
	}

	s_file_path =NULL;
	s_net_file_path =NULL;

    APP_FREE(s_netupgrade_para.url);
    APP_FREE(s_netupgrade_para.username);
    APP_FREE(s_netupgrade_para.password);
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER  int app_merge_db_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER  int app_merge_db_start_press(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	uint32_t cmb_sel = 0;
    int Ret;
    int32_t init_value;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
            {
                PopDlg  pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.mode = POP_MODE_UNBLOCK;

				GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
                if(cmb_sel != MERGE_DB_INPUT_BY_NET && check_usb_status() == false)
				{
					pop.str = STR_ID_INSERT_USB;
					popdlg_create(&pop);
					break;
				}

				if(MERGE_DB_EXPORT == cmb_sel)
				{
					export_db();
				}
				else if(MERGE_DB_INPUT_BY_USB == cmb_sel)
                {
                    if(s_file_path == NULL)
                    {
                        pop.str = STR_ID_ERR_PATH;
                        popdlg_create(&pop);
                        break; ;
                    }
                    GxBus_PmLock();
                    Ret = GxBus_PmLoadDefault(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, (uint8_t*)s_file_path);
                    init_value = 0;
                    GxBus_ConfigSetInt(FAV_INIT_KEY, init_value);
                    GxBus_PmUnlock();
                    if(Ret != 0)
                    {
                        pop.str = STR_ID_ERR_DATE;
                        popdlg_create(&pop);
                    }
                    else
                    {
                        pop.str = STR_ID_REBOOT_NOW;
                        popdlg_create(&pop);
                        GUI_SetInterface("flush",NULL);
                        GxCore_ThreadDelay(2000);
                        app_reboot();
                    }
                }
                else if(MERGE_DB_INPUT_BY_NET == cmb_sel)
                {
                    if(s_net_file_path == NULL)
                    {
                        pop.str = STR_ID_ERR_PATH;
                        popdlg_create(&pop);
                        break ;
                    }
                    netupg_software_download_stop();
                    netupg_download_process_timer_reset();
                    gx_curl_http_options_t http_options;
                    memset(&http_options, 0, sizeof(gx_curl_http_options_t));
                    netupg_download_process.handle = gxapps_curl_async_download_ext(s_net_file_path, &http_options, NETUPG_DB_FILE_PATH, NULL, 0, netupg_db_download_callback);

                    if(!netupg_download_process.handle)
                    {
                        netupg_download_process.finish = 2;
                    }
                }
                break;
            }
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_merge_db_box_keypress(const char* widgetname, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	uint32_t box_sel;

	if(event->type == GUI_KEYDOWN)
	{
		switch(event->key.sym)
		{
			case STBK_MENU:
			case STBK_EXIT:
				GUI_SetProperty("cmb_export_switch", "content", "[User DB,Default DB]");
				GUI_SetProperty("boxitem_export_switch", "state", "enable");
				GUI_EndDialog(WND_MERGE_DB);
				break;

			case STBK_OK:
				break;
			case STBK_UP:
				GUI_GetProperty("box_merge_db", "select", &box_sel);
				if(MERGE_DB_TYPE == box_sel)
				{
					box_sel = MERGE_DB_OK;
					GUI_SetProperty("box_merge_db", "select", &box_sel);
					app_log_debug("\n[func:%s] , box_sel:%d\n",__func__,box_sel);
				}
				else
				{
					box_sel--;
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				break;

			case STBK_DOWN:
				GUI_GetProperty("box_merge_db", "select", &box_sel);
				if(MERGE_DB_OK== box_sel)
				{
					box_sel = MERGE_DB_TYPE;
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				else if(MERGE_DB_TYPE== box_sel)
				{
					uint32_t cmb_sel = 0;

					GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
					if(MERGE_DB_EXPORT == cmb_sel)
					{
						box_sel = MERGE_EXPORT_SWITCH;
					}
					else
					{
						box_sel = MERGE_DB_PATH;
					}
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				else
				{
					box_sel++;
					GUI_SetProperty("box_merge_db", "select", &box_sel);
				}
				break;
			case STBK_RIGHT:
			case STBK_LEFT:
				GUI_GetProperty("box_merge_db", "select", &box_sel);
				if(MERGE_DB_TYPE== box_sel)
				{
					uint32_t cmb_sel = 0;

					if(s_file_path_bak != NULL)
					{
						GxCore_Free(s_file_path_bak);
						s_file_path_bak = NULL;
					}
					s_file_path =NULL;
					GUI_SetProperty("btn_merge_db_filepath", "string", STR_ID_PRESS_OK);

					GUI_GetProperty("cmb_merge_db_type", "select", &cmb_sel);
					if(MERGE_DB_EXPORT == cmb_sel)
					{
						GUI_SetProperty("cmb_export_switch", "content", "[User DB,Default DB]");
						GUI_SetProperty("boxitem_export_switch", "state", "enable");
					}
					else if(MERGE_DB_INPUT_BY_USB == cmb_sel || MERGE_DB_INPUT_BY_NET == cmb_sel)
					{
						GUI_SetProperty("cmb_export_switch",  "content", "[User DB]");
						GUI_SetProperty("boxitem_export_switch", "state", "disable");
					}
				}
				ret = EVENT_TRANSFER_KEEPON;
				break;

			default:
				break;
		}
	}
	return ret;
}

#endif

