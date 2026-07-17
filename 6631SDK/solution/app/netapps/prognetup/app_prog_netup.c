#include "app.h"
#if PROGRAM_NETUP_SUPPORT
#include "app_module.h"
#include "app_windows.h"
#include "channel_common.h"
#include "app_wnd_system_setting_opt.h"
#include <gx_apps.h>
#include <gxcore.h>
#include "app_channel_info.h"
#include "app_prog_netup.h"

#define PER_MALLOC_PROG_NUM 200
#define MAX_INPUT_VERSION_LEN 16
#define MAX_INPUT_URL_LEN 256
#define SEC_PER_DAY (24*60*60)
#define NET_PROG_NET_UPDATE_KEY "netprognetup>updatekey"
#define NET_PROG_NET_UPDATE_SER_KEY "netprognetup>server"
#define NET_PROG_NET_UPDATE_DATAVERSION_KEY "netprognetup>dataversion"
#define NET_PROG_NET_UPDATE_TIME_KEY "netprognetup>updatetime"
#define NET_PROG_NET_UPDATE_CYCLE_KEY "netprognetup>cycletime"
#define NET_PROG_NET_UPDATE_DEFAULT_TIME 1730355381
#define NET_PROG_NET_UPDATE_DEFAULT_CYCLE 0              // 0: one day 1: two day 2：three day
#define NET_PROG_NET_UPDATE_DEFAULT_VERSION "2.1.0.0"
#define NET_PROG_NETUP_CON_PATH "/mnt/upgrade_setting.xml"
#define NET_PROG_NETUP_SAT_PATH "/home/gx/sat.db"
#define NET_PROG_NETUP_TP_PATH "/home/gx/tp.db"
#define NET_PROG_NETUP_PROG_PATH "/home/gx/prog.db"
#define NET_PROG_NET_UPDATE_SER "http://192.168.31.137:8000/upgrade_setting.xml"

typedef struct
{
    uint16_t ts_id;
    uint16_t original_id;
    uint16_t service_id;        // 节目的searvice id
}NetupProg;

typedef struct
{
    NetupProg prog;
    uint8_t skip_flag ; // 跳过标记
    uint8_t lock_flag ;
    uint32_t favorite_flag;
    uint32_t definition;
    GxBusPmDataProgAduioTrack audio_mode;
    uint8_t audio_volume;
} LocalProgInfo;

typedef enum
{
    WORK_OFF,
    WORK_ON
}ProgNetUpMode;

enum
{
    ITEM_PROG_NETUP_TYPE = 0,
    ITEM_PROG_NETUP_CYCLE,
    ITEM_PROG_NETUP_SERVER,
    ITEM_PROG_NETUP_MAX,
};

typedef struct
{
    uint8_t database_version;
    uint32_t data_version;
    int32_t work_mode;
    char    server_url[MAX_INPUT_URL_LEN];
    time_t  time;
    int32_t cycle_time;
    uint8_t    work_state;
    event_list* work_start_timer;
    bool in_sys_menu;
}ProgNetUpCtrol;

static NetupProg s_curprog = {0};
static ProgNetUpCtrol s_prognetup_ctrol = {0};
static SystemSettingOpt  s_ProgNetUpOpt;
static SystemSettingItem s_ProgNetUpItem[ITEM_PROG_NETUP_MAX];
static LocalProgInfo* s_localprog_info = NULL;
static int s_favprog_num = 0;
extern uint32_t app_play_id_set(uint32_t id);
extern void app_epg_preprocess_for_acu(void);
extern void app_epg_reset_for_acu(void);
#define DEFAULT_PROG_DB_PATH "/mnt/usb01/default_data.db"

static void _prog_netup_ctrol_init(void)
{
    int a,b,c,d;
    APP_TIMER_REMOVE(s_prognetup_ctrol.work_start_timer);
    char ret_buf[MAX_INPUT_VERSION_LEN] = {0};
    GxBus_ConfigGetInt(NET_PROG_NET_UPDATE_KEY, &(s_prognetup_ctrol.work_mode), WORK_OFF);
    GxBus_ConfigGet(NET_PROG_NET_UPDATE_SER_KEY, s_prognetup_ctrol.server_url, MAX_INPUT_URL_LEN, NET_PROG_NET_UPDATE_SER);
    GxBus_ConfigGetInt(NET_PROG_NET_UPDATE_TIME_KEY,(int32_t *)&(s_prognetup_ctrol.time),NET_PROG_NET_UPDATE_DEFAULT_TIME);
    GxBus_ConfigGetInt(NET_PROG_NET_UPDATE_CYCLE_KEY,&(s_prognetup_ctrol.cycle_time),NET_PROG_NET_UPDATE_DEFAULT_CYCLE);
    GxBus_ConfigGet(NET_PROG_NET_UPDATE_DATAVERSION_KEY, ret_buf, MAX_INPUT_VERSION_LEN, NET_PROG_NET_UPDATE_DEFAULT_VERSION);
    if (sscanf(ret_buf, "%x.%x.%x.%x", &a, &b, &c, &d) == 4)
    {
        s_prognetup_ctrol.database_version = a;
        s_prognetup_ctrol.data_version = (b << 16) | (c << 8) | d;
    }
    s_prognetup_ctrol.work_state = 0;
    s_prognetup_ctrol.in_sys_menu = false;
}

static time_t _prog_netup_get_time(void)
{
    gx_net_time_t *net_time = NULL;
    net_time = gx_net_get_time();
    if(net_time != NULL)
    {
        time_t time = app_mktime(net_time->tm_year+1900, net_time->tm_mon+1, net_time->tm_mday, net_time->tm_hour, net_time->tm_min, net_time->tm_sec);
        GxCore_Free(net_time);
        net_time = NULL;
        return time;
    }
    return 0;
}

static int _realloc_local_prog_info(void)
{
    if(s_favprog_num%PER_MALLOC_PROG_NUM == 0)
    {
        LocalProgInfo* local_info = NULL;
        if(s_favprog_num == 0)
        {
            if(s_localprog_info != NULL)
            {
                GxCore_Free(s_localprog_info);
                s_localprog_info=NULL;
            }
            local_info = GxCore_Mallocz(sizeof(LocalProgInfo)*PER_MALLOC_PROG_NUM);
        }
        else
            local_info = GxCore_Realloc(s_localprog_info,sizeof(LocalProgInfo)*(s_favprog_num+PER_MALLOC_PROG_NUM));
        if(local_info == NULL)
        {
            app_log_error("malloc s_localprog_info is failed!!!!!!!!!\n");
            return -1;
        }
        s_localprog_info = local_info;
        memset(&(s_localprog_info[s_favprog_num]),0,sizeof(LocalProgInfo)*PER_MALLOC_PROG_NUM);
    }
    return 0;
}

static int _free_local_prog_info(void)
{
    if(s_localprog_info != NULL)
    {
        GxCore_Free(s_localprog_info);
        s_localprog_info=NULL;
    }
    s_favprog_num = 0;
    return 0;
}



static int  _prog_netup_parse_xml(const char *filename, char* dataversion, char* dataurl)
{
    GXxmlDocPtr doc;
    GXxmlNodePtr root = NULL;
    char *key = NULL;
    char *value = NULL;
    doc = GXxmlParseFile(filename);
    if (NULL == doc)
    {
        gxlogd("\nERROR, GXxml parse error.\n");
        return GXCORE_ERROR;
    }
    root = doc->root;
    if((!root )||(0 !=  GXxmlStrcmp(root->name, (const unsigned char *)"upgrade")))
    {
        GXxmlFreeDoc(doc);
        app_log_error("\033[1;31m[%s] no valid root node\n\033[0m", __func__);
        return GXCORE_ERROR;
    }

    GXxmlNodePtr current_node = root->childs;
    while (current_node) {
        if((!key)&&(0 == GXxmlStrcmp(current_node->name, (const unsigned char *)"version")))
        {
            key = (char *)GXxmlNodeGetContent(current_node);
            memcpy(dataversion,key,MAX_INPUT_VERSION_LEN);
        }
        else if((!value)&&(0 == GXxmlStrcmp(current_node->name, (const unsigned char *)"url")))
        {
            value = (char *)GXxmlNodeGetContent(current_node);
            memcpy(dataurl,value,MAX_INPUT_URL_LEN);
        }
        current_node = current_node->next;
    }
    APP_FREE(key);
    APP_FREE(value);
    GXxmlFreeDoc(doc);
    return 0;
}

void _prog_netup_send_ui_msg(ProgupStateType type,int sec, const char* string)
{
    GxMessage   *new_msg = NULL;
    AppMsgProgupStateUiCotrol* param=NULL;
    new_msg = GxBus_MessageNew(APPMSG_PROG_NETUP_STATE_REPORT);
    param = (AppMsgProgupStateUiCotrol*)GxBus_GetMsgPropertyPtr(new_msg, AppMsgProgupStateUiCotrol);
    if(param != NULL)
    {
        param->type = type;
        param->time = sec;
        param->str = (char*)string;
        GxBus_MessageSend(new_msg);
    }
    return;
}

static void net_get_data_thread_body(void* data)
{
	GxCore_ThreadDetach();
    int ret = 0;
    gx_curl_http_options_t opt = {0};
    char dversion[MAX_INPUT_VERSION_LEN] = {0};
    char durl[MAX_INPUT_URL_LEN] = {0};
    gx_curl_data_t curldata = {0};
    gx_curl_data_t durldata = {0};
    handle_t fp = 0;
    uint32_t datav = 0;
    int a,b,c,d;
    ret = gxapps_curl_httpfunc(s_prognetup_ctrol.server_url,GX_CURL_HTTP_GET,&opt,&curldata);
    if (ret == GXAPPS_SUCCESS)
    {
        fp = GxCore_Open(NET_PROG_NETUP_CON_PATH, "w+");
        if (fp)
        {
            GxCore_Write(fp,curldata.buffer, 1, curldata.used_size);
            GxCore_Close(fp);
            if(_prog_netup_parse_xml(NET_PROG_NETUP_CON_PATH,dversion,durl) == GXCORE_ERROR)
            {
                app_log_error("parse upgrade_setting.xml error\n");
                _prog_netup_send_ui_msg(PROG_UP_FAIL,0,NULL);
                goto finish1;
            }
            sscanf(dversion, "%x.%x.%x.%x", &a, &b, &c, &d);
            if(s_prognetup_ctrol.database_version != a)
            {
                app_log_info("the database_version is no same\n");
                _prog_netup_send_ui_msg(PROG_UP_STOP,0,NULL);
                goto finish1;
            }
            datav = (b << 16) | (c << 8) | d;
            if(datav <= s_prognetup_ctrol.data_version)
            {
                app_log_info("There is no new database version, no need to update\n");
                _prog_netup_send_ui_msg(PROG_UP_STOP,0,NULL);
                goto finish1;
            }
            if(gxapps_curl_httpfunc(durl,GX_CURL_HTTP_GET,&opt,&durldata) == GXAPPS_SUCCESS)
            {
                if(curldata.used_size < 16)
                {
                    app_log_error("Downloaded data is too small.\n");
                    _prog_netup_send_ui_msg(PROG_UP_FAIL,0,NULL);
                    goto finish2;
                }

                unsigned long file_crc;
                memcpy(&file_crc, durldata.buffer, 4);
                file_crc = ntohl(file_crc); // 转换为主机字节序

                size_t sat_length, tp_length, prog_length;
                memcpy(&sat_length, durldata.buffer + 4, 4);
                memcpy(&tp_length, durldata.buffer + 8 , 4);
                memcpy(&prog_length, durldata.buffer + 12 , 4);

                sat_length = ntohl(sat_length);
                tp_length = ntohl(tp_length);
                prog_length = ntohl(prog_length);

                unsigned long calculated_crc = GxCore_Crc32(0L,(const unsigned char*)durldata.buffer + 16, sat_length + tp_length + prog_length);
                if (calculated_crc != file_crc) {
                    app_log_error("CRC check failed!\n");
                    _prog_netup_send_ui_msg(PROG_UP_FAIL,0,NULL);
                    goto finish2;
                }

                _prog_netup_send_ui_msg(PROG_UP_START,0,NULL);
                while(s_prognetup_ctrol.work_state == 0)
                {
                    GxCore_ThreadDelay(100);
                }
                if(s_prognetup_ctrol.work_state == 2)
                {
                    _prog_netup_send_ui_msg(PROG_UP_STOP,0,NULL);
                    goto finish2;
                }

                GxBus_PmLock();
                GxBus_PmDbaseClose();
                handle_t sat_file = GxCore_Open(NET_PROG_NETUP_SAT_PATH, "w+");
                GxCore_Write(sat_file,durldata.buffer + 16, 1, sat_length);
                GxCore_Sync(sat_file);
                GxCore_Close(sat_file);

                handle_t tp_file = GxCore_Open(NET_PROG_NETUP_TP_PATH, "w+");
                GxCore_Write(tp_file, durldata.buffer + 16 + sat_length, 1, tp_length);
                GxCore_Sync(tp_file);
                GxCore_Close(tp_file);

                handle_t prog_file = GxCore_Open(NET_PROG_NETUP_PROG_PATH, "w+");
                GxCore_Write(prog_file, durldata.buffer + 16 + sat_length + tp_length, 1, prog_length);
                GxCore_Sync(prog_file);
                GxCore_Close(prog_file);

                GxBus_PmDbaseInit(SYS_MAX_SAT, SYS_MAX_TP, SYS_MAX_PROG, NULL);
                GxBus_PmUnlock();
                GxBus_ConfigSet(NET_PROG_NET_UPDATE_DATAVERSION_KEY,dversion);
                s_prognetup_ctrol.data_version = datav;
                _prog_netup_send_ui_msg(PROG_UP_SUCC,0,NULL);
            }
            else
            {
                app_log_error("get data failed\n");
                _prog_netup_send_ui_msg(PROG_UP_FAIL,0,NULL);
                goto finish2;
            }
        }
        else
        {
            app_log_error("open upgrade_setting.xml failed\n");
            _prog_netup_send_ui_msg(PROG_UP_FAIL,0,NULL);
        }
    }
    else
    {
        app_log_error("get upgrade_setting.xml failed\n");
        _prog_netup_send_ui_msg(PROG_UP_FAIL,0,NULL);
    }
finish2:
    gxapps_curl_free(&durldata);
finish1:
    GxCore_FileDelete(NET_PROG_NETUP_CON_PATH);
    gxapps_curl_free(&curldata);
}

static void _create_prog_list_upgrade_thread(void)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("program_net_update", &thread_id, net_get_data_thread_body,
		NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}


static int _get_local_prog_list(void)
{
    int i = 0;
    uint32_t prog_num = 0;
    GxBusPmDataProg prog_data;
    GxBusPmViewInfo view_info;
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
    view_info.group_mode = GROUP_MODE_ALL;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    view_info.stream_type = GXBUS_PM_PROG_ALL;
    GxBus_PmViewInfoMemSet(&view_info);
    prog_num = GxBus_PmProgNumGet();
    for(i = 0; i < prog_num; i++)
    {
        _realloc_local_prog_info();
        GxBus_PmProgGetByPos(i,1,(void*)&prog_data);
        s_localprog_info[s_favprog_num].prog.ts_id = prog_data.ts_id;
        s_localprog_info[s_favprog_num].prog.service_id = prog_data.service_id;
        s_localprog_info[s_favprog_num].prog.original_id = prog_data.original_id;
        s_localprog_info[s_favprog_num].skip_flag = prog_data.skip_flag;
        s_localprog_info[s_favprog_num].lock_flag = prog_data.lock_flag;
        s_localprog_info[s_favprog_num].favorite_flag = prog_data.favorite_flag;
        s_localprog_info[s_favprog_num].audio_mode = prog_data.audio_mode;
        s_localprog_info[s_favprog_num].audio_volume = prog_data.audio_volume;
        s_favprog_num++;
    }
    GxBus_PmViewInfoMemClear();
    return 0;
}

static status_t _local_prog_list_init(void)
{
    _get_local_prog_list();
	return GXCORE_SUCCESS;
}

static int _prog_net_update_exec_timer(void *usrdata)
{
    APP_TIMER_REMOVE(s_prognetup_ctrol.work_start_timer);
    if(IF_STATE_CONNECTED != app_network_get_cur_dev_and_state(NULL))
    {
        return -1;
    }
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH) ||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_UPGRADE_PROCESS)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_OTA_UPGRADE)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_POSITIONER_SETTING)||
            GXCORE_SUCCESS == GUI_CheckDialog(WND_TIMER_SETTING))

    {
        APP_TIMER_ADD(s_prognetup_ctrol.work_start_timer, _prog_net_update_exec_timer, 2000, TIMER_REPEAT);
        return -1;
    }
    time_t time = _prog_netup_get_time();
    if(time == 0)
    {
        APP_TIMER_ADD(s_prognetup_ctrol.work_start_timer, _prog_net_update_exec_timer, 2000, TIMER_REPEAT);
        return -1;
    }
    if((time - s_prognetup_ctrol.time) > ((s_prognetup_ctrol.cycle_time +1)*SEC_PER_DAY))
    {
        _create_prog_list_upgrade_thread();
        s_prognetup_ctrol.time = time;
        GxBus_ConfigSetInt(NET_PROG_NET_UPDATE_TIME_KEY,time);
        APP_TIMER_ADD(s_prognetup_ctrol.work_start_timer, _prog_net_update_exec_timer, (s_prognetup_ctrol.cycle_time+1)*SEC_PER_DAY*1000, TIMER_REPEAT);
    }
    else
    {
        APP_TIMER_ADD(s_prognetup_ctrol.work_start_timer, _prog_net_update_exec_timer, (((s_prognetup_ctrol.cycle_time +1)*SEC_PER_DAY) + s_prognetup_ctrol.time -time)*1000, TIMER_REPEAT);
    }
    return 0;
}

static int _prog_netup_timer_start(void)
{
    _prog_netup_ctrol_init();
    if(s_prognetup_ctrol.work_state == 1)
        return -1;
    if(s_prognetup_ctrol.work_mode == WORK_ON)
    {
        _prog_net_update_exec_timer(NULL);
    }
    return 0;
}

static int _prog_netup_timer_stop(void)
{
    APP_TIMER_REMOVE(s_prognetup_ctrol.work_start_timer);
    return 0;
}

static int _prog_netup_type_change_callback(int sel)
{
    if(sel == WORK_ON)
    {
        app_system_set_item_state_update(ITEM_PROG_NETUP_SERVER, ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_PROG_NETUP_CYCLE, ITEM_NORMAL);
    }
    else
    {
        app_system_set_item_state_update(ITEM_PROG_NETUP_CYCLE, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_PROG_NETUP_SERVER, ITEM_DISABLE);
    }
    return 0;
}

static void _prog_netup_setting_type_item_init(void)
{
    s_ProgNetUpItem[ITEM_PROG_NETUP_TYPE].itemTitle = STR_ID_PROGRAM_NETUP_TITLE;
    s_ProgNetUpItem[ITEM_PROG_NETUP_TYPE].itemType = ITEM_CHOICE;
    s_ProgNetUpItem[ITEM_PROG_NETUP_TYPE].itemProperty.itemPropertyCmb.content = "[Off,On]";
    s_ProgNetUpItem[ITEM_PROG_NETUP_TYPE].itemProperty.itemPropertyCmb.sel = s_prognetup_ctrol.work_mode;
    s_ProgNetUpItem[ITEM_PROG_NETUP_TYPE].itemCallback.cmbCallback.CmbChange = _prog_netup_type_change_callback;
    s_ProgNetUpItem[ITEM_PROG_NETUP_TYPE].itemStatus = ITEM_NORMAL;
}

static void _prog_netup_setting_cycle_item_init(void)
{
    s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemTitle = STR_ID_CYCLE_TIME;
    s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemType = ITEM_CHOICE;
    s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemProperty.itemPropertyCmb.content = "[24h,48h,72h]";
    s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemProperty.itemPropertyCmb.sel = s_prognetup_ctrol.cycle_time;
    s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemCallback.cmbCallback.CmbChange = NULL;
    s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemStatus = ITEM_NORMAL;
    if(s_prognetup_ctrol.work_mode == WORK_OFF)
    {
        s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_ProgNetUpItem[ITEM_PROG_NETUP_CYCLE].itemStatus = ITEM_NORMAL;
    }

}

static void _prog_netup_server_release_cb(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
    {
        return;
    }

    if (data->out_name == NULL)
        return;
    if(memcmp(s_prognetup_ctrol.server_url,data->out_name,MAX_INPUT_URL_LEN) != 0)
    {
        memset(s_prognetup_ctrol.server_url,0,MAX_INPUT_URL_LEN);
        memcpy(s_prognetup_ctrol.server_url,data->out_name, MAX_INPUT_URL_LEN);
        s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemProperty.itemPropertyBtn.string = s_prognetup_ctrol.server_url;
        app_system_set_item_property(ITEM_PROG_NETUP_SERVER, &s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemProperty);

    }
    return;
}


static int _prog_netup_server_edit_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        static PopKeyboard keyboard;
        memset(&keyboard, 0, sizeof(PopKeyboard));
        keyboard.in_name    = s_prognetup_ctrol.server_url;
        keyboard.max_num = MAX_INPUT_URL_LEN - 1;
        keyboard.out_name   = NULL;
        keyboard.change_cb  = NULL;
        keyboard.release_cb = _prog_netup_server_release_cb;
        multi_language_keyboard_create(&keyboard);
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}


static void _prog_netup_setting_server_item_init(void)
{
    s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemTitle = STR_ID_SERVER_URL;
    s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemType = ITEM_PUSH;
    s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemProperty.itemPropertyBtn.string = s_prognetup_ctrol.server_url;
    s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemCallback.btnCallback.BtnPress = _prog_netup_server_edit_press_callback;
    if(s_prognetup_ctrol.work_mode == WORK_OFF)
    {
        s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_ProgNetUpItem[ITEM_PROG_NETUP_SERVER].itemStatus = ITEM_NORMAL;
    }
}

static bool _prog_netup_check_cb(void)
{
    uint32_t cycle_sel = 0;
    uint32_t cmb_sel = 0;
    GUI_GetProperty(app_system_get_combobox(ITEM_PROG_NETUP_TYPE), "select", &cmb_sel);
    GUI_GetProperty(app_system_get_combobox(ITEM_PROG_NETUP_CYCLE),"select",&cycle_sel);
    char ret_buf[MAX_INPUT_URL_LEN] = {0};
    GxBus_ConfigGet(NET_PROG_NET_UPDATE_SER_KEY, ret_buf, MAX_INPUT_URL_LEN, NET_PROG_NET_UPDATE_SER);
    if(s_prognetup_ctrol.work_mode != cmb_sel || (memcmp(s_prognetup_ctrol.server_url,ret_buf,MAX_INPUT_URL_LEN) != 0) || s_prognetup_ctrol.cycle_time != cycle_sel)
    {
        s_prognetup_ctrol.work_mode = cmb_sel;
        s_prognetup_ctrol.cycle_time = cycle_sel;
        return true;
    }
    s_prognetup_ctrol.in_sys_menu = false;
    _prog_netup_timer_start();
    return false;
}

static int _prog_netup_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        GxBus_ConfigSetInt(NET_PROG_NET_UPDATE_KEY, s_prognetup_ctrol.work_mode);
        GxBus_ConfigSetInt(NET_PROG_NET_UPDATE_CYCLE_KEY, s_prognetup_ctrol.cycle_time);
        GxBus_ConfigSet(NET_PROG_NET_UPDATE_SER_KEY,s_prognetup_ctrol.server_url);
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    s_prognetup_ctrol.in_sys_menu = false;
    _prog_netup_timer_start();
    return 0;
}



static int _prog_netup_exit_callback(ExitType exit_type)
{
    app_system_set_callback_handler(_prog_netup_check_cb,_prog_netup_save_pop_cb);

    return EXIT_RET_POP;
}

void app_prog_netup_menu_create(void)
{
#if AUTO_UPDATE_SUPPORT
    int32_t acu_config;
    GxBus_ConfigGetInt(T2_ACU_KEY, &acu_config, T2_ACU_VALUE);
    if(acu_config)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.default_ret = POP_VAL_CANCEL;
        pop.str = STR_ID_ACU_CLOSE;
        pop.timeout_sec = 3000;
        popdlg_create(&pop);
        return;
    }
#endif
    s_prognetup_ctrol.in_sys_menu = true;
    _prog_netup_ctrol_init();
    _prog_netup_timer_stop();
    memset(&s_ProgNetUpOpt, 0, sizeof(SystemSettingOpt));
    s_ProgNetUpOpt.menuTitle = STR_ID_PROGRAM_NETUP_TITLE;
    s_ProgNetUpOpt.itemNum = ITEM_PROG_NETUP_MAX;
    s_ProgNetUpOpt.item = s_ProgNetUpItem;
    s_ProgNetUpOpt.timeDisplay = TIP_HIDE;

    _prog_netup_setting_type_item_init();
    _prog_netup_setting_cycle_item_init();
    _prog_netup_setting_server_item_init();

    s_ProgNetUpOpt.exit = _prog_netup_exit_callback;
    app_system_set_create(&s_ProgNetUpOpt);
}

bool app_prog_netup_state_get(void)
{
    if(s_prognetup_ctrol.work_state == 1)
        return true;
    else
        return false;
}

bool app_prog_netup_workmode_get(void)
{
    if(s_prognetup_ctrol.work_mode == WORK_ON)
        return true;
    else
        return false;
}



int app_prog_netup_msg_proc(GxMessage *msg)
{
    static GxTime tick_start,tick_end;
    uint32_t sec = 0;

    int ret = GXCORE_SUCCESS;
    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_STATE_REPORT:
            {
                GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                if(dev_state->dev_state == IF_STATE_CONNECTED)
                {
                    if(s_prognetup_ctrol.in_sys_menu == false)
                        _prog_netup_timer_start();
                }
                else if(dev_state->dev_state == IF_STATE_DISCONNECTED)
                {
                    _prog_netup_timer_stop();
                }
            }
            break;
        case APPMSG_PROG_NETUP_STATE_REPORT:
            {
                AppMsgProgupStateUiCotrol* Progupstate = GxBus_GetMsgPropertyPtr(msg, AppMsgProgupStateUiCotrol);
                switch(Progupstate->type)
                {
                    case PROG_UP_START:
                        {
                            book_popdlg_exec(POP_VAL_CANCEL);
                            popdlg_destroy();
                            if((GXCORE_SUCCESS == app_channel_list_check_dialog()) || (GUI_CheckDialog(WND_COMMON_SELECT) == GXCORE_SUCCESS))
                            {
                                if(GUI_CheckDialog(WND_NETVIDEO_PLAY) != GXCORE_SUCCESS)
                                    GUI_EndDialog("after wnd_full_screen");
                            }
                            if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
                            {
                                app_epg_preprocess_for_acu();
                            }
                            else
                            {
                                g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt,STBK_EXIT);
                            }
                            if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
                            {
                                GUI_EndDialog(WND_PASSWD);
                            }
                            app_channel_info_signal_end_dialog();
                            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH) ||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_UPGRADE_PROCESS)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_OTA_UPGRADE)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_POSITIONER_SETTING)||
                                    GXCORE_SUCCESS == GUI_CheckDialog(WND_TIMER_SETTING))
                            {
                                s_prognetup_ctrol.work_state = 2;
                                APP_TIMER_ADD(s_prognetup_ctrol.work_start_timer, _prog_net_update_exec_timer, 2000, TIMER_REPEAT);
                                return -1;
                            }
                            s_prognetup_ctrol.work_state = 1;
                            PopDlg  pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_EXIT;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.format = POP_FORMAT_DLG;
                            pop.default_ret = POP_VAL_CANCEL;
                            pop.str = AUTO_UPDATE_SHOW_INFO;
                            pop.timeout_sec = 1000;
                            popdlg_create(&pop);
                            app_log_info("net upgrade db start!!!!!!!!!!!!!!!!\n");
                            g_AppFullArb.timer_stop();
                            _local_prog_list_init();
                            GxBusPmDataProg prog_data = {0};
                            GxBus_PmProgGetByPos(g_AppPlayOps.normal_play.play_count,1,(void*)&prog_data);
                            s_curprog.ts_id = prog_data.ts_id;
                            s_curprog.service_id = prog_data.service_id;
                            s_curprog.original_id = prog_data.original_id;
                            GxCore_GetTickTime(&tick_start);
                        }
                        break;
                    case PROG_UP_SUCC:
                        {
                            uint32_t i = 0;
                            uint16_t progid = 0;
                            uint16_t satid = 0;
                            GxBusPmDataProg Prog = {0};
                            GxBus_PmLock();
                            for(i = 0; i < s_favprog_num; i++)
                            {
                                ret = GxBus_PmProgGetBySpecialId(s_localprog_info[i].prog.ts_id,s_localprog_info[i].prog.service_id,s_localprog_info[i].prog.original_id,&Prog);
                                if(ret == GXCORE_SUCCESS)
                                {
                                    if(s_curprog.ts_id == Prog.ts_id && s_curprog.service_id == Prog.service_id && s_curprog.original_id == Prog.original_id)
                                    {
                                        progid = Prog.id;
                                        satid = Prog.sat_id;
                                    }
                                    if((Prog.skip_flag != s_localprog_info[i].skip_flag)
                                            || (Prog.lock_flag != s_localprog_info[i].lock_flag)
                                            || (Prog.favorite_flag != s_localprog_info[i].favorite_flag)
                                            || (Prog.audio_mode != s_localprog_info[i].audio_mode)
                                            || (Prog.audio_volume != s_localprog_info[i].audio_volume))
                                    {
                                        Prog.skip_flag = s_localprog_info[i].skip_flag;
                                        Prog.lock_flag = s_localprog_info[i].lock_flag;
                                        Prog.favorite_flag = s_localprog_info[i].favorite_flag;
                                        Prog.audio_mode = s_localprog_info[i].audio_mode;
                                        Prog.audio_volume = s_localprog_info[i].audio_volume;
                                        GxBus_PmProgInfoModify(&Prog);
                                    }
                                }
                            }
                            GxBus_PmUnlock();
                            GxBus_PmSync(GXBUS_PM_SYNC_PROG);
                           _free_local_prog_info();
                            s_prognetup_ctrol.work_state = 0;
                            GxCore_GetTickTime(&tick_end);
                            if((tick_end.microsecs- tick_start.microsecs) >=0 )
                                sec = tick_end.seconds-tick_start.seconds;
                            else
                                sec = tick_end.seconds- tick_start.seconds - 1;
                            if(sec >= 3)
                                popdlg_destroy();
                            else
                                app_pop_reset_timeout(3-sec);
                            if(progid == 0)
                            {
                                GxBusPmViewInfo view_info;
                                app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
                                view_info.group_mode = GROUP_MODE_ALL;
                                g_AppPlayOps.play_list_create(&view_info);
                                if((GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
                                        && (GUI_CheckDialog(WIN_FILE_BROWSER) != GXCORE_SUCCESS)
                                        && (GUI_CheckDialog(WND_SYSTEM_SETTING) != GXCORE_SUCCESS))
                                {
                                    if((GUI_CheckDialog(WND_COMMON_SELECT) == GXCORE_SUCCESS))
                                    {
                                        GUI_EndDialog("after wnd_full_screen");
                                    }
                                    pvr_state cur_pvr = app_pvr_get_state();
                                    if(PVR_DUMMY != cur_pvr)
                                    {
                                        app_pvr_stop();
                                    }
                                    if(g_AppFullArb.state.pause == STATE_ON)
                                    {
                                        g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                                        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                    }
                                    if(g_AppPlayOps.normal_play.play_total == 0)
                                    {
                                        GUI_EndDialog("after wnd_full_screen");
                                        g_AppPlayOps.program_stop();
                                        break;
                                    }
                                    g_AppPlayOps.program_play(PLAY_TYPE_FORCE, 0);
                                }
                            }
                            else
                            {
                                if(g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_SAT)
                                {
                                    g_AppPlayOps.normal_play.view_info.sat_id = satid;
                                }
                                app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *) & (g_AppPlayOps.normal_play.view_info));
                                app_play_id_set(progid);
                                g_AppPlayOps.normal_play.play_total = GxBus_PmProgNumGet();
                                g_AppChOps.create(g_AppPlayOps.normal_play.play_total);
                                app_get_prog_pos_by_id(&g_AppPlayOps.normal_play.view_info, progid, &g_AppPlayOps.normal_play.play_count);
                            }
                            if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
                            {
                                book_popdlg_exec(POP_VAL_CANCEL);
                                GUI_SetProperty("listview_epg_prog", "update_all", NULL);
#if LVLIST_EPG_SUPPORT
                                GUI_SetProperty("listview_epg_prog", "select", &g_AppPlayOps.normal_play.play_count);
#endif
                                GUI_SetProperty("listview_epg_prog", "active", &g_AppPlayOps.normal_play.play_count);
                                app_epg_reset_for_acu();
                            }
                            app_log_info("net upgrade db successful!!!!!!!!!!!!!!!!\n");
                        }
                        break;
                    case PROG_UP_STOP:
                        {
                            if(s_prognetup_ctrol.work_state == 2)
                                s_prognetup_ctrol.work_state = 0;
                            app_log_info("net upgrade is stop.\n");
                        }
                        break;
                    case PROG_UP_FAIL:
                        {
                            app_log_error("net upgrade failed!\n");
                        }
                        break;
                    default:
                        break;
                }

            }
            break;
        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}
#endif
