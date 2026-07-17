/*************************************************************************
	> File Name: net_upgrade.c
	> Author:
	> Mail:
	> Created Time: 2024年07月02日 星期二 15时45分01秒
 ************************************************************************/

#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_net_upgrade.h"
#include "app_wnd_file_list.h"
#include "app_keyboard_language.h"
#include "app_wnd_system_setting_opt.h"
#if NET_UPGRADE_SUPPORT

#define NETUPG_INTERNAL_URL	"netapps>netupg_internalurl"
#define NETUPG_HTTP_URL	"netapps>netupg_httpurl"
#define NETUPG_HTTPS_URL	"netapps>netupg_httpsurl"
#define NETUPG_FTP_URL	"netapps>netupg_ftpurl"
#define NETUPG_FILE_URL	"netapps>netupg_fileurl"
#define NETUPG_FTP_USER "netapps>netupg_ftpuser"
#define NETUPG_FTP_PWD "netapps>netupg_ftppwd"
#define NETUPG_CFG_FILE_NAME "upgrade_setting.xml"
#define NETUPG_DEFAULT_INTERNAL_URL "http://192.168.3.236:8080/"NETUPG_CFG_FILE_NAME
#define NETUPG_DEFAULT_HTTP_URL "http://192.168.3.236:8080/"NETUPG_CFG_FILE_NAME
#define NETUPG_DEFAULT_HTTPS_URL "https://192.168.3.236:8081/"NETUPG_CFG_FILE_NAME
#define NETUPG_DEFAULT_FILE_URL "Press OK"
#define NETUPG_DEFAULT_FTPURL "ftp://192.168.3.236:8082/"NETUPG_CFG_FILE_NAME
#define NETUPG_DEFAULT_FTPUSER "gx"
#define NETUPG_DEFAULT_FTPPWD "gx"
#define NETUPG_PROGBAR_PROCESS "progbar_netupg_process"
#define NETUPG_TEXT_PROCESS_TIP "text_netupg_process_tip"
#define NETUPG_TEXT_PROCESS_INFO1 "txt_netupg_process_info1"
#define NETUPG_TEXT_PROCESS_INFO2 "txt_netupg_process_info2"
#define NETUPG_TXT_MSG_BACK "img_system_setting_timezone_back"
#define NETUPG_TXT_MSG      "text_system_setting_msg"
#define NETUPG_PROCESS_BAR "progbar_system_setting_progress"
#define NETUPG_PROCESS_TEXT "text_system_setting_progress"
#define NETUPG_PROCESS_TEXT_DEFAULT_VALUE "0 KB"
#define DEF_PROTOCOL "[INTERNAL,HTTP,HTTPS,FTP,FILE]"
#define RED_KEY_NAME "URL to INTERNAL"
#define BMP_OK              "s_bar_choice_blue4"
#define BMP_CANCEL          "s_bar_choice_blue4_l"
#define IMG_OPT             "img_net_upgrade_tip_opt"
#define BTN_OK              "btn_net_upgrade_tip_ok"
#define BTN_CANCEL          "btn_net_upgrade_tip_cancel"
#define TEXT_TIP_TIME       "text_net_upgrade_tip_time"
#define NOTEPAD_UPGRADE_TIP "notepad_net_upgrade_tip"

static bool s_upgrade_tip_select_ok = false;
static int32_t s_tip_show_time = 0;
static char *s_upg_setting_info_url = NULL;
static char *s_pop_msg_buf = NULL;
static int s_upg_setting_info_sw_ver_match = 1;
static event_list* netupg_tip_show_timer = NULL;
static event_list* netupg_exec_timer = NULL;
static char *s_file_path = NULL;
static char *s_file_path_bak = NULL;

enum COL_ITEM_NAME
{
    ITEM_NETUPGRADE_PROTOCOL=0,
    ITEM_NETUPGRADE_URL,
    ITEM_NETUPGRADE_USER,
    ITEM_NETUPGRADE_PASSWORD,
    ITEM_NETUPGRADE_START,
    ITEM_NETUPGRADE_TOTAL,
};

static SystemSettingItem s_net_upgrade_item[ITEM_NETUPGRADE_TOTAL];
SystemSettingOpt s_net_upgrade_setting_opt;
extern signed char init_utf8_position(const char* InStr,unsigned int *LastCharSel, unsigned int *MaxCharCount);

typedef enum
{
    NET_UPG_PROTOCOL_INTERNAL = 0,
    NET_UPG_PROTOCOL_HTTP,
    NET_UPG_PROTOCOL_HTTPS,
    NET_UPG_PROTOCOL_FTP,
    NET_UPG_PROTOCOL_FILE,
    NET_UPG_PROTOCOL_TOTAL,
}NetUpgProtocol;

typedef struct
{
    NetUpgProtocol protocol;
    char *url;
    char *username;
    char *password;
}NetUpgUiPara;

typedef struct
{
    char *sw_ver;
    char *url;
    char *show_time;
    char *log;
}NetUpgSettingInfo;

NetUpgUiPara s_upg_ui_para = {0};
static char *s_netupg_title_str[] = {
    "Protocol",
    "URL",
    "User",
    "Password",
    "Start",
};

static char *s_upgrade_edit_item[] =
{
    "edit_system_setting_opt1",
    "edit_system_setting_opt2",
    "edit_system_setting_opt3",
    "edit_system_setting_opt4",
    "edit_system_setting_opt5",
    "edit_system_setting_opt6",
    "edit_system_setting_opt7",
};


void app_netupg_show_msg(char *buf)
{
    if(buf != NULL)
    {
        GUI_SetProperty(NETUPG_TXT_MSG, "string", buf);
        app_update_pop_dlg(WND_KEYBOARD_LANGUAGE);
    }
}

static void app_netupg_progress_reset(void)
{
    int rate = 0;
    GUI_SetProperty(NETUPG_PROCESS_BAR, "value", (void*)&rate);
    GUI_SetProperty(NETUPG_PROCESS_TEXT, "string", NETUPG_PROCESS_TEXT_DEFAULT_VALUE);
}

static char* app_netupg_get_url(void)
{
    char *netupg_wholeurl = NULL;
    char def_url[NETUPG_MAX_URL_LEN] = {0};
    char *url = NULL,*temp = NULL;
    unsigned int len = 0;

    if(s_upg_ui_para.url && strlen(s_upg_ui_para.url)>0)
    {
        url = GxCore_Strdup(s_upg_ui_para.url);
    }
    if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_INTERNAL)
    {
        GxBus_ConfigGet(NETUPG_INTERNAL_URL, def_url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_INTERNAL_URL);
        netupg_wholeurl = GxCore_Strdup(def_url);
    }
    else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTP)
    {
        if(s_upg_ui_para.url&&(strlen(s_upg_ui_para.url)>0))
        {
            if(0 == strncmp(url, NETUPG_HTTP_PREFIX, strlen(NETUPG_HTTP_PREFIX)))
            {
                len = strlen(url)+1;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if (netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s", url);
            }
            else
            {
                len = strlen(NETUPG_HTTP_PREFIX)+strlen(url)+2;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if (netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s%s", NETUPG_HTTP_PREFIX, url);
            }
        }
        else
        {
            app_netupg_show_msg(STR_ID_INVALID_URL);
        }
    }
    else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTPS)
    {
        if(s_upg_ui_para.url&&(strlen(s_upg_ui_para.url)>0))
        {
            if(0 == strncmp(url, NETUPG_HTTPS_PREFIX, strlen(NETUPG_HTTPS_PREFIX)))
            {
                len = strlen(url)+1;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if (netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s", url);
            }
            else
            {
                len = strlen(NETUPG_HTTPS_PREFIX)+strlen(url)+2;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if (netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s%s", NETUPG_HTTP_PREFIX, url);
            }
        }
        else
        {
            app_netupg_show_msg(STR_ID_INVALID_URL);
        }
    }
    else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_FTP)
    {
        char *auth_str = NULL;
        len = strlen(NETUPG_FTP_PREFIX);
        auth_str = netupg_ftp_get_authstr(s_upg_ui_para.username, s_upg_ui_para.password);
        if(auth_str)
        {
            len += strlen(auth_str);
        }
        len += strlen(url)+2;
        if(0 == strncmp(url, NETUPG_FTP_PREFIX, strlen(NETUPG_FTP_PREFIX)))
        {
            temp = GxCore_Strdup(url+strlen(NETUPG_FTP_PREFIX));
            APP_FREE(url);
            url = temp;
        }
        netupg_wholeurl = (char *)GxCore_Malloc(len);
        if (netupg_wholeurl != NULL)
        {
            if(auth_str)
            {
                sprintf(netupg_wholeurl, "%s%s%s", NETUPG_FTP_PREFIX, auth_str, url);
            }
            else
            {
                sprintf(netupg_wholeurl, "%s%s", NETUPG_FTP_PREFIX, url);
            }
        }
        APP_FREE(auth_str);
    }
    else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_FILE)
    {
        if(s_upg_ui_para.url&&(strlen(s_upg_ui_para.url)>0) && (0 != strcmp(s_upg_ui_para.url,NETUPG_DEFAULT_FILE_URL)))
        {
            netupg_wholeurl = GxCore_Strdup(s_upg_ui_para.url);
        }
        else
        {
            app_netupg_show_msg(STR_ID_INVALID_URL);
        }
    }
    APP_FREE(url);
    return netupg_wholeurl;
}

static void app_netupg_exec_timer_stop(void)
{
    if(netupg_exec_timer)
    {
        remove_timer(netupg_exec_timer);
        netupg_exec_timer = NULL;
    }
}

static void _netupg_get_ui_info(char *buf)
{
    int pop_msg_len = 0;
    char *sw_ver = NULL, *sw_ver_match = NULL, *timeout = NULL, *log = NULL, *url = NULL;
    char sw_ver_info[32]={0};

    pop_msg_len = strlen(buf)+100;
    s_pop_msg_buf = (char *)GxCore_Malloc(pop_msg_len);
    if (s_pop_msg_buf == NULL)
        return;
    memset(s_pop_msg_buf, 0, pop_msg_len);

    s_upg_setting_info_url = (char *)GxCore_Malloc(NETUPG_MAX_URL_LEN);
    if (s_upg_setting_info_url == NULL)
        return;
    memset(s_upg_setting_info_url, 0, NETUPG_MAX_URL_LEN);

    sw_ver = strtok(buf, NETUPG_CONFIG_INFO_DELEMITER);
    sprintf(s_pop_msg_buf,"%s %s? \n",STR_ID_UPDATE_NEW_VER, sw_ver);
    sw_ver_match = strtok(NULL, NETUPG_CONFIG_INFO_DELEMITER);
    if(strcmp(sw_ver_match,NETUPG_CONFIG_INFO_INVALID_FILL) != 0)
        s_upg_setting_info_sw_ver_match = atoi(sw_ver_match);
    url = strtok(NULL,NETUPG_CONFIG_INFO_DELEMITER);
    sprintf(s_upg_setting_info_url,"%s",url);
    timeout = strtok(NULL,NETUPG_CONFIG_INFO_DELEMITER);
    if(strcmp(timeout,NETUPG_CONFIG_INFO_INVALID_FILL) != 0)
        s_tip_show_time = atoi(timeout);
    else
        s_tip_show_time = 0;
    log = strtok(NULL, "");
    if(strcmp(log,NETUPG_CONFIG_INFO_INVALID_FILL) != 0)
    {
        strcat(s_pop_msg_buf,log);
    }
    sprintf(sw_ver_info, "Has new version %s",sw_ver);
    app_netupg_show_msg(sw_ver_info);
}

static int netupg_exec_timer_timeout(void *usrdata)
{
    char process_str[128] = {0};
    uint32_t upg_download_rate = 0;
    NetUpgProcess  upg_process = {0};
    char *tip = NULL;
    int ret = 0;
    char *info = NULL;
    int rate = 0;

    memset(process_str, 0, sizeof(process_str));
    upg_process = app_upgrade_process_get();
    if(upg_process.mode == NET_UPG_DOWNLOAD_CONFIG)
    {
        if(upg_process.finish)
        {
            if(upg_process.finish == 1)
            {
                ret = app_cfg_file_parse(&info);
                if(ret == 0)
                {
                    _netupg_get_ui_info(info);
                    GUI_CreateDialog(WND_NET_UPGRADE_TIP);
                }
                else
                {
                    app_netupg_show_msg(info);
                }
                APP_FREE(info);
                app_netupg_exec_timer_stop();
            }
            else
            {
                app_netupg_show_msg(STR_ID_FAIL_GET_UPGRADE_SETTING);
                app_netupg_exec_timer_stop();
            }
        }
    }
    else if(upg_process.mode == NET_UPG_DOWNLOAD_BIN)
    {
        if(upg_process.finish)
        {
            if(upg_process.finish == 1)
            {
                upg_download_rate = 100;
                GUI_SetProperty(NETUPG_PROCESS_BAR, "value", (void*)&upg_download_rate);
                snprintf(process_str, sizeof(process_str), "%u/%u KB", upg_process.cur_size/1024, upg_process.total_size/1024);
                GUI_SetProperty(NETUPG_PROCESS_TEXT, "string", process_str);

                ret = app_upgrade_file_verify(&tip);
                if(ret != 0)
                {
                    app_netupg_exec_timer_stop();
                    app_netupg_show_msg(tip);
                    APP_FREE(tip);
                }
                else
                {
                    ret = app_upgrade_exec();
                    if(ret != 0)
                    {
                        app_netupg_show_msg(STR_ID_RECOVERY_OS_FAILED);
                        app_netupg_progress_reset();
                        app_netupg_exec_timer_stop();
                    }
                }
            }
            else
            {
                app_netupg_show_msg(STR_ID_FIRMWARE_FAILED);
                app_netupg_progress_reset();
                app_netupg_exec_timer_stop();
            }
        }
        else
        {
            if(upg_process.total_size>0)
            {
                upg_download_rate = upg_process.cur_size*100/upg_process.total_size;
                if(upg_download_rate>100)
                {
                    upg_download_rate = 100;
                }
                GUI_SetProperty(NETUPG_PROCESS_BAR, "value", (void*)&upg_download_rate);
                snprintf(process_str, sizeof(process_str), "%u/%u KB", upg_process.cur_size/1024, upg_process.total_size/1024);
                GUI_SetProperty(NETUPG_PROCESS_TEXT, "string", process_str);
            }
            else
            {
                snprintf(process_str, sizeof(process_str), "%u KB", upg_process.cur_size/1024);
                GUI_SetProperty(NETUPG_PROCESS_TEXT, "string", process_str);
            }
        }
    }
    else if(upg_process.mode == NET_UPG_DOWNLOAD_RECOVERY_OS)
    {
        if(upg_process.finish)
        {
            if(upg_process.finish == 1)
            {
                upg_download_rate = 100;
                GUI_SetProperty(NETUPG_PROCESS_BAR, "value", (void*)&upg_download_rate);
                snprintf(process_str, sizeof(process_str), "%u/%u KB", upg_process.cur_size/1024, upg_process.total_size/1024);
                GUI_SetProperty(NETUPG_PROCESS_TEXT, "string", process_str);

                ret = app_upgrade_file_verify(&tip);
                if(ret != 0)
                {
                    app_netupg_exec_timer_stop();
                    app_netupg_show_msg(tip);
                    APP_FREE(tip);
                }
                else
                {
                    app_upgrade();
                }
            }
            else
            {
                app_netupg_show_msg(STR_ID_RECOVERY_OS_FAILED);
                app_netupg_progress_reset();
                app_netupg_exec_timer_stop();
            }
        }
        else
        {
            app_netupg_show_msg(STR_ID_DL_RECOVERY_OS);
            if(upg_process.total_size > 0)
            {
                upg_download_rate = upg_process.cur_size*100/upg_process.total_size;
                if(upg_download_rate > 100)
                {
                    upg_download_rate = 100;
                }
                GUI_SetProperty(NETUPG_PROCESS_BAR, "value", (void*)&upg_download_rate);
                snprintf(process_str, sizeof(process_str), "%u/%u KB", upg_process.cur_size/1024, upg_process.total_size/1024);
                GUI_SetProperty(NETUPG_PROCESS_TEXT, "string", process_str);
            }
            else
            {
                snprintf(process_str, sizeof(process_str), "%u KB", upg_process.cur_size/1024);
                GUI_SetProperty(NETUPG_PROCESS_TEXT, "string", process_str);
            }
        }
    }
    else if(upg_process.mode == NET_UPG_UPDATE_SKIP_RECOVERY_PART)
    {
        upg_process = app_upgrade_process_get();
        if(upg_process.finish == 1)
        {
           app_upg_main_upgrade_finish_process();
        }
        else if(upg_process.finish == 2)
        {
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_SOFTWARE_ERROR);
            app_netupg_exec_timer_stop();
        }
        else if(upg_process.finish == 3)
        {
            rate = 100;
            GUI_SetProperty(NETUPG_PROGBAR_PROCESS, "value", (void*)&rate);
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_RECOVERY_OS_SUCC);
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO2, "string", STR_ID_REBOOT_NOW);
            GUI_SetInterface("flush",NULL);
            GxCore_ThreadDelay(2000);
            app_reboot();
        }
        else
        {
            if(upg_process.total_size > 0)
            {
                rate = upg_process.cur_size*100/upg_process.total_size;
                if(rate >= 95)
                    rate = 95;
                GUI_SetProperty(NETUPG_PROGBAR_PROCESS, "value", (void*)&rate);
            }
        }
    }
    else if(upg_process.mode == NET_UPG_UPDATE_RECOVERY_PART)
    {
        upg_process = app_upgrade_process_get();
        if(upg_process.finish == 1)
        {
            rate = 100;
            GUI_SetProperty(NETUPG_PROGBAR_PROCESS, "value", (void*)&rate);
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_UPGRADE_SUCCESS);
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO2, "string", STR_ID_REBOOT_NOW);
            GUI_SetInterface("flush",NULL);
            GxCore_ThreadDelay(2000);
            app_reboot();
        }
        else if(upg_process.finish == 2)
        {
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_SOFTWARE_ERROR);
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO2, "string", STR_ID_REBOOT_NOW);
            GUI_SetInterface("flush",NULL);
            GxCore_ThreadDelay(2000);
            app_reboot();
        }
        else
        {
            if(upg_process.total_size>0)
            {
                rate = upg_process.cur_size*100/upg_process.total_size;
                GUI_SetProperty(NETUPG_PROGBAR_PROCESS, "value", (void*)&rate);
            }
        }
    }
    else if(upg_process.mode == NET_UPG_UPDATE_NO_SAFE)
    {
        upg_process = app_upgrade_process_get();
        if(upg_process.finish == 1)
        {
            rate = 100;
            _upg_update_write_sw_version();
            GUI_SetProperty(NETUPG_PROGBAR_PROCESS, "value", (void*)&rate);
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_UPGRADE_SUCCESS);
            GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO2, "string", STR_ID_REBOOT_NOW);
            GUI_SetInterface("flush",NULL);
            GxCore_ThreadDelay(2000);
            app_reboot();
        }
        else if(upg_process.finish == 2)
        {
            if(upg_process.total_size>0)
            {
                rate = upg_process.cur_size*100/upg_process.total_size;
            }
            if(rate == 0)
            {
                GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_SOFTWARE_ERROR);
                GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO2, "string", "");
                app_netupg_exec_timer_stop();
                GxCore_ThreadDelay(2000);
                GUI_EndDialog(WND_NETUPG_PROCESS);
                app_netupg_show_msg(STR_ID_SOFTWARE_ERROR);
                app_netupg_progress_reset();
            }
            else
            {
                GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_SOFTWARE_ERROR);
                GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO2, "string", STR_ID_REBOOT_NOW);
                GUI_SetInterface("flush",NULL);
                GxCore_ThreadDelay(2000);
                app_reboot();
            }
        }
        else
        {
            if(upg_process.total_size>0)
            {
                rate = upg_process.cur_size*100/upg_process.total_size;
                GUI_SetProperty(NETUPG_PROGBAR_PROCESS, "value", (void*)&rate);
            }
        }
    }
    return 0;
}

static void app_netupg_exec_timer_start(void)
{
    app_netupg_exec_timer_stop();
    netupg_exec_timer = create_timer(netupg_exec_timer_timeout, 500, NULL, TIMER_REPEAT);
}

static bool _netupg_url_validity_check(const char *url)
{
    if (url == NULL || strlen(url) == 0) {
        return false;
    }

    const char *lastSlash = strrchr(url, '/');
    if (lastSlash == NULL) {
        return false;
    }
    return strcmp(lastSlash + 1, "upgrade_setting.xml") == 0;
}


static int app_netupgrade_start_button_press_callback(unsigned short key)
{
    NetUpgProcess  upg_process = {0};

    upg_process = app_upgrade_process_get();
    if((upg_process.mode != NET_UPG_NONE) && (upg_process.finish == 0))
    {
        return EVENT_TRANSFER_STOP;
    }

    GUI_SetProperty(NETUPG_TXT_MSG, "string", " ");
    app_netupg_progress_reset();

    if(key == STBK_OK)
    {
        char *netupg_wholeurl = NULL;
        netupg_wholeurl = app_netupg_get_url();
        if(_netupg_url_validity_check(netupg_wholeurl) == false)
        {
            app_netupg_show_msg(STR_ID_INVALID_URL);
            return EVENT_TRANSFER_STOP;
        }
        if(netupg_wholeurl != NULL)
        {
            if(s_upg_ui_para.protocol != NET_UPG_PROTOCOL_FILE)
            {
                if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_INTERNAL)
                {
                    if(0 != strncmp(netupg_wholeurl, NETUPG_FILE_PREFIX, strlen(NETUPG_FILE_PREFIX)))
                    {
                        if(IF_STATE_CONNECTED != app_network_get_cur_dev_and_state(NULL))
                        {
                            app_netupg_show_msg(STR_ID_CONNECT_TIP);
                            return EVENT_TRANSFER_STOP;
                        }
                    }
                }
                else
                {
                    if(IF_STATE_CONNECTED != app_network_get_cur_dev_and_state(NULL))
                    {
                        app_netupg_show_msg(STR_ID_CONNECT_TIP);
                        return EVENT_TRANSFER_STOP;
                    }
                }
            }
            if(GXCORE_FILE_EXIST == GxCore_FileExists(UPG_SERVER_CONFIG_FILE))
            {
                GxCore_FileDelete(UPG_SERVER_CONFIG_FILE);
            }
            app_netupg_exec_timer_start();
            app_netupg_show_msg(STR_ID_GET_UP_INFO);
            memset(&s_upg_process, 0, sizeof(NetUpgProcess));
            s_upg_process.mode = NET_UPG_DOWNLOAD_CONFIG;
            app_cfg_file_get(netupg_wholeurl);
            APP_FREE(netupg_wholeurl);
        }
    }
    return EVENT_TRANSFER_KEEPON;
}

static void netupg_url_full_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if(trim(data->out_name) == NULL)
        return;

    APP_FREE(s_upg_ui_para.url);
    s_upg_ui_para.url = GxCore_Strdup(data->out_name);
    if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTP)
    {
        GxBus_ConfigSet(NETUPG_HTTP_URL, s_upg_ui_para.url);
    }
    else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTPS)
    {
        GxBus_ConfigSet(NETUPG_HTTPS_URL, s_upg_ui_para.url);
    }
    else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_FTP)
    {
        GxBus_ConfigSet(NETUPG_FTP_URL, s_upg_ui_para.url);
    }
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = s_upg_ui_para.url;
    app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
}

static void netupg_user_full_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;
    if(trim(data->out_name) == NULL)
        return;

    APP_FREE(s_upg_ui_para.username);
    s_upg_ui_para.username = GxCore_Strdup(data->out_name);
    GxBus_ConfigSet(NETUPG_FTP_USER, s_upg_ui_para.username);
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_upg_ui_para.username;
    app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
}

static void netupg_pwd_full_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;
    if(trim(data->out_name) == NULL)
        return;

    APP_FREE(s_upg_ui_para.password);
    s_upg_ui_para.password = GxCore_Strdup(data->out_name);
    GxBus_ConfigSet(NETUPG_FTP_PWD, s_upg_ui_para.password);
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_upg_ui_para.password;
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

    GUI_SetProperty(NETUPG_TXT_MSG, "string", " ");
    app_netupg_progress_reset();

    APP_FREE(s_upg_ui_para.url);
    APP_FREE(s_upg_ui_para.username);
    APP_FREE(s_upg_ui_para.password);
    APP_FREE(s_file_path_bak);
    s_file_path = NULL;
    if(sel == NET_UPG_PROTOCOL_INTERNAL)
    {
        s_upg_ui_para.protocol = sel;
        GxBus_ConfigGet(NETUPG_INTERNAL_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_INTERNAL_URL);
        s_upg_ui_para.url = GxCore_Strdup(url);
        s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = s_upg_ui_para.url;
        s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = "";
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = "";
        app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_PASSWORD,&s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty);
        app_system_set_item_state_update(ITEM_NETUPGRADE_URL,ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_NETUPGRADE_USER,ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_NETUPGRADE_PASSWORD,ITEM_DISABLE);
    }
    else if(sel == NET_UPG_PROTOCOL_HTTP || sel == NET_UPG_PROTOCOL_HTTPS)
    {
        s_upg_ui_para.protocol = sel;
        if(sel == NET_UPG_PROTOCOL_HTTP)
        {
            GxBus_ConfigGet(NETUPG_HTTP_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_HTTP_URL);
        }
        else
        {
            GxBus_ConfigGet(NETUPG_HTTPS_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_HTTPS_URL);
        }
        s_upg_ui_para.url = GxCore_Strdup(url);
        s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = s_upg_ui_para.url;
        s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_upg_ui_para.username;
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_upg_ui_para.password;
        app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_PASSWORD,&s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty);
        app_system_set_item_state_update(ITEM_NETUPGRADE_URL,ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_NETUPGRADE_USER,ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_NETUPGRADE_PASSWORD,ITEM_DISABLE);
    }
    else if(sel == NET_UPG_PROTOCOL_FTP)
    {
        s_upg_ui_para.protocol = NET_UPG_PROTOCOL_FTP;
        GxBus_ConfigGet(NETUPG_FTP_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_FTPURL);
        GxBus_ConfigGet(NETUPG_FTP_USER, user, NETUPG_MAX_USER_LEN, NETUPG_DEFAULT_FTPUSER);
        GxBus_ConfigGet(NETUPG_FTP_PWD, pwd, NETUPG_MAX_PWD_LEN, NETUPG_DEFAULT_FTPPWD);
        s_upg_ui_para.url = GxCore_Strdup(url);
        s_upg_ui_para.username = GxCore_Strdup(user);
        s_upg_ui_para.password = GxCore_Strdup(pwd);
        s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = s_upg_ui_para.url;
        s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_upg_ui_para.username;
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_upg_ui_para.password;
        app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_PASSWORD,&s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty);
        app_system_set_item_state_update(ITEM_NETUPGRADE_USER,ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_NETUPGRADE_PASSWORD,ITEM_NORMAL);
    }
    else if(sel == NET_UPG_PROTOCOL_FILE)
    {
        s_upg_ui_para.protocol = sel;
        GxBus_ConfigGet(NETUPG_FILE_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_FILE_URL);
        s_upg_ui_para.url = GxCore_Strdup(url);
        s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = url;
        s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = "";
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = "";
        app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_USER,&s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty);
        app_system_set_item_property(ITEM_NETUPGRADE_PASSWORD,&s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty);
        app_system_set_item_state_update(ITEM_NETUPGRADE_URL,ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_NETUPGRADE_USER,ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_NETUPGRADE_PASSWORD,ITEM_DISABLE);
    }
    return 0;
}

static int _app_upgrade_get_file_url_exit_cb(WndStatus ret)
{
    if(ret == WND_OK)
    {
        app_get_file_real_path(&s_file_path);

        if(s_file_path != NULL)
        {
            int str_len = 0;
            APP_FREE(s_upg_ui_para.url);
            s_upg_ui_para.url =(char*)GxCore_Malloc(strlen(s_file_path)+10);
            if(s_upg_ui_para.url != NULL)
            {
                strcpy(s_upg_ui_para.url,"file:/");
                strcpy(s_upg_ui_para.url+6,s_file_path);
            }
            GxBus_ConfigSet(NETUPG_FILE_URL, s_upg_ui_para.url);
            s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = s_upg_ui_para.url;
            app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);

            ////bak////
            APP_FREE(s_file_path_bak);

            str_len = strlen(s_file_path);
            s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if(s_file_path_bak != NULL)
            {
                memcpy(s_file_path_bak, s_file_path, str_len);
                s_file_path_bak[str_len] = '\0';
            }
        }
    }
    else
    {
        if(s_file_path_bak == NULL)
        {
            s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
            app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        }
    }
    return 0;
}

static int app_netupgrade_url_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    static PopKeyboard keyboard;
    switch (key)
    {
        case STBK_OK:
            {
                if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_FILE)
                {
                    if(check_usb_status() == false)
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
                        file_para.suffix = "xml;XML";
                        file_para.pos_x = WND_FILE_LIST_POS_X;
                        file_para.pos_y = WND_FILE_LIST_POS_Y;
                        file_para.exit_cb = _app_upgrade_get_file_url_exit_cb;
                        file_para.dest_mode = DEST_MODE_FILE;
                        app_get_file_path_dlg(&file_para);
                    }
                    s_file_path =NULL;
                }
                else
                {
                    memset(&keyboard, 0, sizeof(PopKeyboard));
                    keyboard.in_name    = s_upg_ui_para.url;
                    keyboard.max_num = NETUPG_MAX_URL_LEN-1;
                    keyboard.out_name   = NULL;
                    keyboard.change_cb  = NULL;
                    keyboard.release_cb = netupg_url_full_keyboard_proc;
                    keyboard.usr_data   = GXBUSCONFIG_INVALID;
                    keyboard.pos.x = 500;
                    multi_language_keyboard_create(&keyboard);
                    ret = EVENT_TRANSFER_KEEPON;
                }
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
                GUI_GetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_USER], "string", &str);

                memset(&keyboard, 0, sizeof(PopKeyboard));
                keyboard.in_name    = str;
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
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        case STBK_LEFT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_upg_ui_para.username,&lastsel,&maxcount);
                GUI_GetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_USER], "select", &sel);
                if(sel == 0)
                {
                    sel=lastsel;
                    GUI_SetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_USER], "select", &sel);
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

                init_utf8_position(s_upg_ui_para.username,&lastsel,&maxcount);
                GUI_GetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_USER], "select", &sel);
                if(sel == lastsel)
                {
                    sel=0;
                    GUI_SetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_USER], "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
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
                GUI_GetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_PASSWORD], "string", &str);

                memset(&keyboard, 0, sizeof(PopKeyboard));
                keyboard.in_name    = str;
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
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
            ret = EVENT_TRANSFER_KEEPON;
            break;
        case STBK_LEFT:
            {
                int sel;
                unsigned int lastsel;
                unsigned int maxcount;

                init_utf8_position(s_upg_ui_para.password,&lastsel,&maxcount);
                GUI_GetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_PASSWORD], "select", &sel);
                if(sel == 0)
                {
                    sel=lastsel;
                    GUI_SetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_PASSWORD], "select", &sel);
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

                init_utf8_position(s_upg_ui_para.password,&lastsel,&maxcount);
                GUI_GetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_PASSWORD], "select", &sel);
                if(sel == lastsel)
                {
                    sel=0;
                    GUI_SetProperty(s_upgrade_edit_item[ITEM_NETUPGRADE_PASSWORD], "select", &sel);
                }
                else
                    ret = EVENT_TRANSFER_KEEPON;
            }
            break;
        default:
            break;
    }
    return ret;
}

static void app_netupgrade_exec_item_init(void)
{
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemTitle = STR_ID_START;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemType = ITEM_PUSH;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemCallback.btnCallback.BtnPress = app_netupgrade_start_button_press_callback;
    s_net_upgrade_item[ITEM_NETUPGRADE_START].itemStatus = ITEM_NORMAL;
}

static void app_netupgrade_protocol_item_init(void)
{
    s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemTitle = s_netupg_title_str[ITEM_NETUPGRADE_PROTOCOL];
    s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemType = ITEM_CHOICE;
    s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemProperty.itemPropertyCmb.content= DEF_PROTOCOL;
    s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemProperty.itemPropertyCmb.sel = s_upg_ui_para.protocol;
    s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemCallback.cmbCallback.CmbChange = app_netupgrade_protocol_change_cb;
    s_net_upgrade_item[ITEM_NETUPGRADE_PROTOCOL].itemStatus = ITEM_NORMAL;
}

static void app_netupgrade_url_item_init(void)
{
    s_file_path =NULL;
    s_file_path_bak = NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemTitle = s_netupg_title_str[ITEM_NETUPGRADE_URL];
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemType = ITEM_PUSH;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = s_upg_ui_para.url;
    s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemCallback.btnCallback.BtnPress = app_netupgrade_url_edit_press_callback;
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
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemProperty.itemPropertyEdit.string = s_upg_ui_para.username;
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemCallback.editCallback.EditReachEnd= NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_USER].itemCallback.editCallback.EditPress= app_netupgrade_user_edit_press_callback;
    if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTP)
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
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemProperty.itemPropertyEdit.string = s_upg_ui_para.password;
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemCallback.editCallback.EditReachEnd= NULL;
    s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemCallback.editCallback.EditPress= app_netupgrade_pwd_edit_press_callback;
    if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTP)
    {
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_net_upgrade_item[ITEM_NETUPGRADE_PASSWORD].itemStatus = ITEM_NORMAL;
    }
}

SIGNAL_HANDLER int app_netupg_progcess_create(GuiWidget * widget, void *usrdata)
{
    int rate = 0;

    GUI_SetProperty(NETUPG_PROGBAR_PROCESS, "value", (void*)&rate);
    GUI_SetProperty(NETUPG_TEXT_PROCESS_TIP, "string", STR_ID_UPGRADE);
    GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO1, "string", STR_ID_WAITING);
    GUI_SetProperty(NETUPG_TEXT_PROCESS_INFO2, "string", STR_ID_DONT_CUT_PWER);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netupg_progcess_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netupg_progcess_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static int app_net_upgrade_red_key(void)
{
    char *netupg_wholeurl = NULL;
    char *url = NULL,*temp = NULL;
    unsigned int len = 0;

    if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_INTERNAL)
        return 0;
    if(s_upg_ui_para.url&&(strlen(s_upg_ui_para.url)>0))
    {
        url = GxCore_Strdup(s_upg_ui_para.url);
        if(url[strlen(url)-1] == '/')
        {
            url[strlen(url)-1] = '\0';
        }
        if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTP)
        {
            if(0 == strncmp(url, NETUPG_HTTP_PREFIX, strlen(NETUPG_HTTP_PREFIX)))
            {
                len = strlen(url)+1;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if(netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s", url);
            }
            else
            {
                len = strlen(NETUPG_HTTP_PREFIX)+strlen(url)+2;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if(netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s%s", NETUPG_HTTP_PREFIX, url);
            }
        }
        else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_HTTPS)
        {
            if(0 == strncmp(url, NETUPG_HTTPS_PREFIX, strlen(NETUPG_HTTPS_PREFIX)))
            {
                len = strlen(url)+1;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if(netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s", url);
            }
            else
            {
                len = strlen(NETUPG_HTTPS_PREFIX)+strlen(url)+2;
                netupg_wholeurl = (char *)GxCore_Malloc(len);
                if(netupg_wholeurl != NULL)
                    sprintf(netupg_wholeurl, "%s%s", NETUPG_HTTPS_PREFIX, url);
            }
        }
        else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_FTP)
        {
            char *auth_str = NULL;
            len = strlen(NETUPG_FTP_PREFIX);
            auth_str = netupg_ftp_get_authstr(s_upg_ui_para.username, s_upg_ui_para.password);
            if(auth_str)
            {
                len += strlen(auth_str);
            }
            if(0 == strncmp(url, NETUPG_FTP_PREFIX, strlen(NETUPG_FTP_PREFIX)))
            {
                temp = GxCore_Strdup(url+strlen(NETUPG_FTP_PREFIX));
                APP_FREE(url);
                url = temp;
            }
            len += strlen(url)+2;
            netupg_wholeurl = (char *)GxCore_Malloc(len);
            if(netupg_wholeurl != NULL)
            {
                if(auth_str)
                {
                    sprintf(netupg_wholeurl, "%s%s%s", NETUPG_FTP_PREFIX, auth_str, url);
                }
                else
                {
                    sprintf(netupg_wholeurl, "%s%s", NETUPG_FTP_PREFIX, url);
                }
            }
            APP_FREE(auth_str);
        }
        else if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_FILE)
        {
            netupg_wholeurl = GxCore_Strdup(s_upg_ui_para.url);
        }
        APP_FREE(url);
        GxBus_ConfigSet(NETUPG_INTERNAL_URL, netupg_wholeurl);
        APP_FREE(netupg_wholeurl);
    }

    return 0;
}

static int app_net_upgrade_exit(ExitType exit_type)
{
    extern int app_net_upgrade_exit_callback(void);
    app_net_upgrade_exit_callback();
    GxBus_ConfigSet(NETUPG_FILE_URL, NETUPG_DEFAULT_FILE_URL);
    APP_FREE(s_upg_ui_para.url);
    APP_FREE(s_upg_ui_para.username);
    APP_FREE(s_upg_ui_para.password);

    return 0;
}

void app_create_netupgrade_menu(void)
{
    char url[NETUPG_MAX_URL_LEN];

    s_net_upgrade_setting_opt.menuTitle = STR_ID_NET_UPGRADE;
    s_net_upgrade_setting_opt.itemNum = ITEM_NETUPGRADE_TOTAL;
    s_net_upgrade_setting_opt.timeDisplay = TIP_HIDE;
    if(APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE)
    {
        s_net_upgrade_setting_opt.topTipImgDisplay = TIP_HIDE;
        s_net_upgrade_setting_opt.bottmTipDisplay= TIP_HIDE;
    }
    s_net_upgrade_setting_opt.menuFuncKey.redKey.keyName = RED_KEY_NAME;
    s_net_upgrade_setting_opt.menuFuncKey.redKey.keyPressFun = app_net_upgrade_red_key;
    s_net_upgrade_setting_opt.exit = app_net_upgrade_exit;

    s_net_upgrade_setting_opt.item = s_net_upgrade_item;
    s_upg_ui_para.protocol = NET_UPG_PROTOCOL_INTERNAL;
    GxBus_ConfigGet(NETUPG_INTERNAL_URL, url, NETUPG_MAX_URL_LEN, NETUPG_DEFAULT_INTERNAL_URL);
    s_upg_ui_para.url = GxCore_Strdup(url);
    s_upg_ui_para.username = NULL;
    s_upg_ui_para.password = NULL;

    app_netupgrade_protocol_item_init();
    app_netupgrade_url_item_init();
    app_netupgrade_username_item_init();
    app_netupgrade_password_item_init();
    app_netupgrade_exec_item_init();

    app_system_set_create(&s_net_upgrade_setting_opt);
    if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_INTERNAL)
    {
        app_netupgrade_protocol_change_cb(s_upg_ui_para.protocol);
    }
    /*img_system_setting_opt_back1 only for theme classic_purple*/
    if(APP_THEME_CLASSIC_PURPLE)
        GUI_SetProperty("img_system_setting_opt_back1","state","show");
    GUI_SetProperty(NETUPG_TXT_MSG_BACK,"state","show");
    GUI_SetProperty(NETUPG_TXT_MSG,"state","show");
    GUI_SetProperty(NETUPG_PROCESS_BAR,"state","show");
    GUI_SetProperty(NETUPG_PROCESS_TEXT,"state","show");
    app_netupg_progress_reset();
    app_upgrade_protocol_register();
    APP_FREE(s_file_path_bak);
}

static void app_netupg_tip_show_timer_stop(void)
{
    if(netupg_tip_show_timer)
    {
        remove_timer(netupg_tip_show_timer);
        netupg_tip_show_timer = NULL;
    }
}

static int netupg_tip_show_timer_timeout(void *usrdata)
{
    char buf[10]={0};

    s_tip_show_time--;
    sprintf(buf, "%d S", s_tip_show_time);
    GUI_SetProperty(TEXT_TIP_TIME, "string", buf);
    if(s_tip_show_time <=0)
    {
        app_netupg_show_msg("Default ok!");
        GUI_Event event = {0};
        event.type = GUI_KEYDOWN;
        event.key.sym = STBK_OK;
        GUI_SendEvent(WND_NET_UPGRADE_TIP, &event);
        app_netupg_tip_show_timer_stop();
    }
    return 0;
}

static void app_netupg_tip_show_timer_start(void)
{
    app_netupg_tip_show_timer_stop();
    netupg_tip_show_timer = create_timer(netupg_tip_show_timer_timeout, 1000, NULL, TIMER_REPEAT);
}

static void app_upgrade_tip_details_keypress(uint16_t key)
{
    uint32_t value = 1;

    switch(key)
    {
        case STBK_UP:
            GUI_SetProperty(NOTEPAD_UPGRADE_TIP, "line_up", &value);
            break;
        case STBK_DOWN:
            GUI_SetProperty(NOTEPAD_UPGRADE_TIP, "line_down", &value);
            break;
        case STBK_PAGE_UP:
            GUI_SetProperty(NOTEPAD_UPGRADE_TIP, "page_up", &value);
            break;
        case STBK_PAGE_DOWN:
            GUI_SetProperty(NOTEPAD_UPGRADE_TIP, "page_down", &value);
            break;
        default:
            break;
    }
}

void app_net_upgrade_usb_update(void)
{
    if(s_upg_ui_para.protocol == NET_UPG_PROTOCOL_FILE)
    {
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NET_UPGRADE_TIP))
        {
            GUI_EndDialog(WND_NET_UPGRADE_TIP);
            GUI_SetInterface("flush", NULL);
        }
        GUI_SetProperty(NETUPG_TXT_MSG, "string", " ");
        app_netupg_progress_reset();
        GxBus_ConfigSet(NETUPG_FILE_URL, NETUPG_DEFAULT_FILE_URL);
        s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty.itemPropertyBtn.string = NETUPG_DEFAULT_FILE_URL;
        app_system_set_item_property(ITEM_NETUPGRADE_URL,&s_net_upgrade_item[ITEM_NETUPGRADE_URL].itemProperty);
        APP_FREE(s_upg_ui_para.url);
        s_upg_ui_para.url = GxCore_Strdup(NETUPG_DEFAULT_FILE_URL);
    }
}

SIGNAL_HANDLER int app_net_upgrade_tip_create(GuiWidget *widget, void *usrdata)
{
    char time_buf[10]={0};

    GUI_SetProperty(NOTEPAD_UPGRADE_TIP, "string", s_pop_msg_buf);

    if(s_upg_setting_info_sw_ver_match == 0)
    {
        GUI_SetProperty(IMG_OPT, "img", BMP_OK);
        GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
        GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
        GUI_SetFocusWidget(BTN_OK);
        s_upgrade_tip_select_ok = true;
    }
    else
    {
        GUI_SetProperty(IMG_OPT, "img", BMP_CANCEL);
        GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
        GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
        GUI_SetFocusWidget(BTN_CANCEL);
        s_upgrade_tip_select_ok = false;

    }
    if(s_tip_show_time > 0)
    {
        sprintf(time_buf, "%d S",s_tip_show_time);
        GUI_SetProperty(TEXT_TIP_TIME, "string", time_buf);
        app_netupg_tip_show_timer_start();
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_net_upgrade_tip_destroy(GuiWidget *widget, void *usrdata)
{
    s_tip_show_time = 0;
    app_netupg_tip_show_timer_stop();
    APP_FREE(s_pop_msg_buf);
    APP_FREE(s_upg_setting_info_url);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_net_upgrade_tip_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = 0;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_OK:
                if(s_upgrade_tip_select_ok)
                {
                    app_netupg_show_msg(STR_ID_DL_FW);
                    app_netupg_exec_timer_start();
                    ret = app_upgrade_file_get_init(s_upg_setting_info_url);
                    if(ret == -1)
                    {
                        app_netupg_exec_timer_stop();
                        app_netupg_show_msg(STR_ID_FIRMWARE_FAILED);
                    }
                    GUI_EndDialog(WND_NET_UPGRADE_TIP);
                }
                else
                {
                    app_netupg_show_msg(STR_ID_USER_CANCEL);
                    GUI_EndDialog(WND_NET_UPGRADE_TIP);
                }
                break;
            case STBK_LEFT:
            case STBK_RIGHT:
                if(s_upgrade_tip_select_ok)
                {
                    GUI_SetProperty(IMG_OPT, "img", BMP_CANCEL);
                    GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
                    GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                    GUI_SetFocusWidget(BTN_CANCEL);
                    s_upgrade_tip_select_ok = false;
                }
                else
                {
                    GUI_SetProperty(IMG_OPT, "img", BMP_OK);
                    GUI_SetProperty(BTN_OK, "string", STR_ID_OK);
                    GUI_SetProperty(BTN_CANCEL, "string", STR_ID_CANCEL);
                    GUI_SetFocusWidget(BTN_OK);
                    s_upgrade_tip_select_ok = true;
                }
                break;
            case STBK_UP:
            case STBK_DOWN:
            case STBK_PAGE_UP:
            case STBK_PAGE_DOWN:
                app_upgrade_tip_details_keypress(find_virtualkey_ex(event->key.scancode,event->key.sym));
                break;
            case STBK_MENU:
            case STBK_EXIT:
                GUI_EndDialog(WND_NET_UPGRADE_TIP);
                app_netupg_show_msg(STR_ID_USER_CANCEL);
                return EVENT_TRANSFER_KEEPON;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}
#endif
