#include "app.h"
#if (SOCKS5_SUPPORT > 0)
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "app_keyboard_language.h"
#include "app_pop.h"
#include <gx_apps.h>


#define SOCKS5_EDIT_LEN 32
#define MAX_INPUT_PROXY_URL_LEN 256
#define SOCKS5_MODE_KEY "socks5>modekey"
#define SOCKS5_PROXY_SER "socks5>server"
#define SOCKS5_PROXY_USR "socks5>usrname"
#define SOCKS5_PROXY_PWD "socks5>pwd"
#define SOCKS5_DEFAULT_MODE 0
#define SOCKS5_DEFAULT_SERVER "192.168.1.71:1080"
#define SOCKS5_DEFAULT_USR "gx_test"
#define SOCKS5_DEFAULT_PWD "123456"

enum SOCKS5_ITEM_NAME
{
    ITEM_SOCKS5_MODE,
    ITEM_SOCKS5_URL,
    ITEM_SOCKS5_USERNAME,
    ITEM_SOCKS5_PASSWORD,
    ITEM_SOCKS5_SAVE,
    ITEM_SOCKS5_TOTAL
};

typedef struct str_socks5
{
    int32_t mode;
    char proxy_url[MAX_INPUT_PROXY_URL_LEN];
    char username[SOCKS5_EDIT_LEN];
    char password[SOCKS5_EDIT_LEN];
}Socks5Data;

static Socks5Data s_socks5_data;
static Socks5Data s_socks5_data_trace;
static SystemSettingOpt s_socks5_opt;
static SystemSettingItem s_socks5_item[ITEM_SOCKS5_TOTAL];

static bool app_socks5_cfg_change(void)
{
    if(s_socks5_data_trace.mode != s_socks5_data.mode)
        return true;

    if(s_socks5_data_trace.mode == 1)
    {
        if(memcmp(s_socks5_data_trace.username, s_socks5_data.username,SOCKS5_EDIT_LEN) != 0)
            return true;

        if(memcmp(s_socks5_data_trace.password, s_socks5_data.password,SOCKS5_EDIT_LEN) != 0)
            return true;

        if(memcmp(s_socks5_data_trace.proxy_url, s_socks5_data.proxy_url,MAX_INPUT_PROXY_URL_LEN) != 0)
            return true;
    }

    return false;
}

static void app_socks5_save(void)
{
    GxBus_ConfigSetInt(SOCKS5_MODE_KEY, s_socks5_data_trace.mode);
    if(s_socks5_data_trace.mode == 1)
    {
        GxBus_ConfigSet(SOCKS5_PROXY_SER, s_socks5_data_trace.proxy_url);
        GxBus_ConfigSet(SOCKS5_PROXY_USR, s_socks5_data_trace.username);
        GxBus_ConfigSet(SOCKS5_PROXY_PWD, s_socks5_data_trace.password);
        gxapps_curl_set_proxy(s_socks5_data_trace.proxy_url,CURLPROXY_SOCKS5,NULL,NULL);
    }
    else
    {
        gxapps_curl_set_proxy(NULL,0,NULL,NULL);
    }
}

static void app_socks5_url_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL || trim(data->out_name) == NULL)
        return;

    memset(s_socks5_data_trace.proxy_url, 0, MAX_INPUT_PROXY_URL_LEN);
    strcpy(s_socks5_data_trace.proxy_url, data->out_name);
    s_socks5_item[ITEM_SOCKS5_URL].itemProperty.itemPropertyBtn.string = s_socks5_data_trace.proxy_url;
    app_system_set_item_property(ITEM_SOCKS5_URL, &s_socks5_item[ITEM_SOCKS5_URL].itemProperty);
}


static void app_socks5_username_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL || trim(data->out_name) == NULL)
        return;

    memset(s_socks5_data_trace.username, 0, SOCKS5_EDIT_LEN);
    strcpy(s_socks5_data_trace.username, data->out_name);
    s_socks5_item[ITEM_SOCKS5_USERNAME].itemProperty.itemPropertyBtn.string = s_socks5_data_trace.username;
    app_system_set_item_property(ITEM_SOCKS5_USERNAME, &s_socks5_item[ITEM_SOCKS5_USERNAME].itemProperty);
}

static void app_socks5_password_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    memset(s_socks5_data_trace.password, 0, SOCKS5_EDIT_LEN+1);
    strcpy(s_socks5_data_trace.password, data->out_name);
}

static int app_socks5_url_btn_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    static PopKeyboard keyboard;

    if(key == STBK_OK)
    {
        memset(&keyboard, 0, sizeof(PopKeyboard));
        keyboard.in_name    = s_socks5_data_trace.proxy_url;
        keyboard.max_num = MAX_INPUT_PROXY_URL_LEN;
        keyboard.out_name   = NULL;
        keyboard.change_cb  = NULL;
        keyboard.release_cb = app_socks5_url_keyboard_proc;
        keyboard.usr_data   = FILE_CHAR_NAME_INVALID;
        keyboard.pos.x = 500;
        multi_language_keyboard_create(&keyboard);
        ret = EVENT_TRANSFER_STOP;
    }
    return ret;
}

static int app_socks5_username_btn_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    static PopKeyboard keyboard;

    if(key == STBK_OK)
    {
        memset(&keyboard, 0, sizeof(PopKeyboard));
        keyboard.in_name    = s_socks5_data_trace.username;
        keyboard.max_num = SOCKS5_EDIT_LEN;
        keyboard.out_name   = NULL;
        keyboard.change_cb  = NULL;
        keyboard.release_cb = app_socks5_username_keyboard_proc;
        keyboard.usr_data   = FILE_CHAR_NAME_INVALID;
        keyboard.pos.x = 500;
        multi_language_keyboard_create(&keyboard);
        ret = EVENT_TRANSFER_STOP;
    }
    return ret;
}

static int app_socks5_password_btn_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    static PopKeyboard keyboard;

    if(key == STBK_OK)
    {
        memset(&keyboard, 0, sizeof(PopKeyboard));
        keyboard.in_name    = s_socks5_data_trace.password;
        keyboard.max_num = SOCKS5_EDIT_LEN;
        keyboard.out_name   = NULL;
        keyboard.change_cb  = NULL;
        keyboard.release_cb = app_socks5_password_keyboard_proc;
        keyboard.usr_data   = FILE_CHAR_NAME_INVALID;
        keyboard.pos.x = 500;
        multi_language_keyboard_create(&keyboard);
        ret = EVENT_TRANSFER_STOP;
    }
    return ret;
}

static int _socks5_save_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        s_socks5_data_trace.mode = s_socks5_item[ITEM_SOCKS5_MODE].itemProperty.itemPropertyCmb.sel;
        if(app_socks5_cfg_change() == true)
        {
            app_socks5_save();
        }
    }
    return 0;
}

static int app_socks5_save_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        PopDlg  pop = {0};
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_SAVE_INFO;
        pop.exit_cb = _socks5_save_cb;
        popdlg_create(&pop);
    }
    return EVENT_TRANSFER_KEEPON;
}

static int app_socks5_mode_change_callback(int sel)
{
    if(sel == 0)
    {
        app_system_set_item_state_update(ITEM_SOCKS5_URL, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_SOCKS5_USERNAME, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_SOCKS5_PASSWORD, ITEM_DISABLE);
    }
    else
    {
        app_system_set_item_state_update(ITEM_SOCKS5_URL, ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_SOCKS5_USERNAME, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_SOCKS5_PASSWORD, ITEM_DISABLE);
    }
    return EVENT_TRANSFER_KEEPON;
}

static int app_socks5_setting_exit_callback(ExitType exit_type)
{
    return EXIT_RET_DEFAULT;
}

static void app_socks5_setting_mode_item_init(void)
{
    s_socks5_item[ITEM_SOCKS5_MODE].itemTitle = STR_ID_MODE;
    s_socks5_item[ITEM_SOCKS5_MODE].itemType = ITEM_CHOICE;
    s_socks5_item[ITEM_SOCKS5_MODE].itemProperty.itemPropertyCmb.content= "[Off,On]";
    s_socks5_item[ITEM_SOCKS5_MODE].itemProperty.itemPropertyCmb.sel = s_socks5_data_trace.mode;
    s_socks5_item[ITEM_SOCKS5_MODE].itemCallback.cmbCallback.CmbChange = app_socks5_mode_change_callback;
    s_socks5_item[ITEM_SOCKS5_MODE].itemStatus = ITEM_NORMAL;
}

static void app_socks5_setting_url_item_init(void)
{
    s_socks5_item[ITEM_SOCKS5_URL].itemTitle = STR_ID_SERVER_URL;
    s_socks5_item[ITEM_SOCKS5_URL].itemType = ITEM_PUSH;
    s_socks5_item[ITEM_SOCKS5_URL].itemProperty.itemPropertyBtn.string = s_socks5_data_trace.proxy_url;
    s_socks5_item[ITEM_SOCKS5_URL].itemCallback.btnCallback.BtnPress= app_socks5_url_btn_callback;
    if(s_socks5_data_trace.mode == 0)
        s_socks5_item[ITEM_SOCKS5_URL].itemStatus = ITEM_DISABLE;
    else
        s_socks5_item[ITEM_SOCKS5_URL].itemStatus = ITEM_NORMAL;
}

static void app_socks5_setting_user_item_init(void)
{
    s_socks5_item[ITEM_SOCKS5_USERNAME].itemTitle = STR_ID_USER;
    s_socks5_item[ITEM_SOCKS5_USERNAME].itemType = ITEM_PUSH;
    s_socks5_item[ITEM_SOCKS5_USERNAME].itemProperty.itemPropertyBtn.string = s_socks5_data.username;
    s_socks5_item[ITEM_SOCKS5_USERNAME].itemCallback.btnCallback.BtnPress= app_socks5_username_btn_callback;
    if(s_socks5_data_trace.mode == 0)
        s_socks5_item[ITEM_SOCKS5_USERNAME].itemStatus = ITEM_DISABLE;
    else
        s_socks5_item[ITEM_SOCKS5_USERNAME].itemStatus = ITEM_NORMAL;
}

static void app_socks5_setting_password_item_init(void)
{
    s_socks5_item[ITEM_SOCKS5_PASSWORD].itemTitle = STR_ID_PASSWD;
    s_socks5_item[ITEM_SOCKS5_PASSWORD].itemType = ITEM_PUSH;
    s_socks5_item[ITEM_SOCKS5_PASSWORD].itemProperty.itemPropertyBtn.string = "********";
    s_socks5_item[ITEM_SOCKS5_PASSWORD].itemCallback.btnCallback.BtnPress= app_socks5_password_btn_callback;
    if(s_socks5_data_trace.mode == 0)
        s_socks5_item[ITEM_SOCKS5_PASSWORD].itemStatus = ITEM_DISABLE;
    else
        s_socks5_item[ITEM_SOCKS5_PASSWORD].itemStatus = ITEM_NORMAL;
}

static void app_socks5_setting_save_item_init(void)
{
    s_socks5_item[ITEM_SOCKS5_SAVE].itemTitle = STR_ID_SAVE;
    s_socks5_item[ITEM_SOCKS5_SAVE].itemType = ITEM_PUSH;
    s_socks5_item[ITEM_SOCKS5_SAVE].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_socks5_item[ITEM_SOCKS5_SAVE].itemCallback.btnCallback.BtnPress = app_socks5_save_press_callback;
}

void app_create_socks5_menu(void)
{
    memset(&s_socks5_data, 0, sizeof(Socks5Data));
    memset(&s_socks5_data_trace, 0, sizeof(Socks5Data));
    GxBus_ConfigGetInt(SOCKS5_MODE_KEY, &(s_socks5_data.mode), SOCKS5_DEFAULT_MODE);
    GxBus_ConfigGet(SOCKS5_PROXY_SER, s_socks5_data.proxy_url, MAX_INPUT_PROXY_URL_LEN,SOCKS5_DEFAULT_SERVER);
    GxBus_ConfigGet(SOCKS5_PROXY_USR, s_socks5_data.username, SOCKS5_EDIT_LEN,SOCKS5_DEFAULT_USR);
    GxBus_ConfigGet(SOCKS5_PROXY_PWD, s_socks5_data.password, SOCKS5_EDIT_LEN,SOCKS5_DEFAULT_PWD);
    memcpy(&s_socks5_data_trace, &s_socks5_data, sizeof(Socks5Data));
    memset(&s_socks5_opt, 0 ,sizeof(s_socks5_opt));
    s_socks5_opt.menuTitle = STR_ID_SOCKS5;
    s_socks5_opt.itemNum = ITEM_SOCKS5_TOTAL;
    s_socks5_opt.timeDisplay = TIP_HIDE;
    s_socks5_opt.topTipImgDisplay = TIP_HIDE;
    s_socks5_opt.bottmTipDisplay = TIP_HIDE;
    s_socks5_opt.item = s_socks5_item;

    app_socks5_setting_mode_item_init();
    app_socks5_setting_user_item_init();
    app_socks5_setting_password_item_init();
    app_socks5_setting_url_item_init();
    app_socks5_setting_save_item_init();
    s_socks5_opt.exit = app_socks5_setting_exit_callback;

    app_system_set_create(&s_socks5_opt);
}

void app_socks5_prxoy_url_get(char* resurl,char *url, int length)
{
    int mode = 0;
    char server[MAX_INPUT_PROXY_URL_LEN] = {0};
    GxBus_ConfigGetInt(SOCKS5_MODE_KEY, &mode, SOCKS5_DEFAULT_MODE);
    if(mode == 1)
    {
        GxBus_ConfigGet(SOCKS5_PROXY_SER, server, MAX_INPUT_PROXY_URL_LEN,SOCKS5_DEFAULT_SERVER);
        snprintf(resurl, length,"%s -H socks5_proxy:\"socks5://%s\"", url,server);
    }
    else
    {
        snprintf(resurl, length,"%s", url);
    }
    return;
}
#endif
