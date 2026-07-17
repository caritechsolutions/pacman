#include "app.h"
#if MIRACAST_SUPPORT
#include "app_windows.h"
#include "app_keyboard_language.h"
#include "app_module.h"
#include "app_miracast.h"
#include "full_screen.h"
#include "include/app_volume_value.h"
#include <gx_apps.h>
#include "netapps/app_netapps_common.h"
#include "app_wnd_system_setting_opt.h"
#include "bus/service/gxusbwifiplug.h"
#include "bus/module/wifi/gxwifi.h"

#define MIRACAST_SSID_NAME    "miracast>ssidname"
#define MIRACAST_WIFI_SELECT    "miracast>select"
#define MIRACAST_SSID_NAME_EDIT_LEN    (16)
#define MIRACAST_INFOBAR_TIMEOUT (5)
#define MIRACAST_DEV_NAME_LEN (16)
#define MIRACAST_WIFI_SELECT_DEFAULT    "ra0"
#define MIRACAST_PLAY_URL           "rtp://127.0.0.1:7236 -H no_cache_flag:1 stc_recover_mode:1 rtp_queue_size:0 screen_to_dmx_pts_lms:50 screen_to_dmx_pts_hms:100"
#define IMG_GIF                 "img_miracast_gif"
#define IMG_MUTE                "img_miracast_mute"
#define GIF_PATH                WORK_PATH"theme/image/netapps/loading.gif"
#define IMG_POPUP            "img_miracast_popup"
#define TXT_POPUP            "txt_miracast_popup"
#define TXT_DEVICE            "txt_miracast_device"
#define TXT_POPUP_AP     "txt_miracast_popup_ap"
#define TXT_POPUP_DETAIL     "txt_miracast_popup_detail"
#define TXT_POPUP_RENAME     "txt_miracast_popup_rename"
#define IMG_POPUP_RENAME     "img_miracast_popup_rename"
#define TXT_POPUP_SELECT     "txt_miracast_popup_select"
#define IMG_POPUP_SELECT     "img_miracast_popup_select"

#define TXT_MIRACAST_INFO_SSID "text_miracast_info_bar_title"
#define TXT_MIRACAST_INFO_TIME "text_miracast_info_time"
#define TXT_MIRACAST_INFO_DATE "text_miracast_info_date"
#define TXT_MIRACAST_INFO_ASPECT "text_miracast_info_aspect"
#define TXT_MIRACAST_INFO_RESOLUTION "text_miracast_info_resolution"
#define TXT_MIRACAST_INFO_NETSPEED "text_miracast_info_netspeed"

#define MAX_DEV_NUM 5
#define MAX_DEV_LEN 16

enum WIFI_SELECT_ITEM_NAME
{
    ITEM_SELECT_WIFI=0,
    ITEM_SELECT_TOTAL
};


typedef enum MiracastState
{
    MIRACAST_STATE_WAIT = 0,
    MIRACAST_STATE_CONNECTING,
    MIRACAST_STATE_CONNECTED,
    MIRACAST_STATE_ERROR
}MiracastState;

typedef enum MiracastPlayState
{
    MIRACAST_PLAY_NONE = 0,
    MIRACAST_PLAY_ERROR,
    MIRACAST_PLAY_PAUSE,
    MIRACAST_PLAY_RUNNING,
    MIRACAST_PLAY_END,
}MiracastPlayState;

struct MiracastUICtrol{
    char ssid[MIRACAST_SSID_NAME_MAX_LEN];
    char u_ssid[MIRACAST_SSID_NAME_MAX_LEN];
    char cur_dev[MIRACAST_DEV_NAME_LEN];
    int select_count;
    MiracastPlayState play_state;
    MiracastState miracast_state;
    ubyte64 net_dev;
    bool rename_enable;
    event_list* net_timer;
    event_list *gif_timer;
    int channel;
    uint32_t showinfo_time;
    event_list *info_timer;
};

extern int app_wifi_linkdown(char* dev_name);
//extern int app_wifi_getp2p_list(char (*destination)[5], int maxnum);
void app_wifi_select_menu_exec(void);

static char wifi_list[MAX_DEV_NUM][MAX_DEV_LEN];
static struct MiracastUICtrol MirCtrol = {{0}};

void app_miracast_send_ui_msg(WFD_EVENT type,const char* u_ssid)
{
    GxMessage   *new_msg = NULL;
    AppMsgMiracastState miracast_state;
    memset(&miracast_state, 0, sizeof(AppMsgMiracastState));
    miracast_state.type = type;
    if(u_ssid != NULL)
        memcpy(miracast_state.u_ssid,u_ssid,MIRACAST_SSID_NAME_MAX_LEN);
    AppMsgMessageClass* param=NULL;
    new_msg = GxBus_MessageNew(APPMSG_MESSAGE);
    param = (AppMsgMessageClass*)GxBus_GetMsgPropertyPtr(new_msg, AppMsgMessageClass);
    if(param != NULL)
    {
        param->type = APPMSG_MIRACAST_STATE_REPORT;
        memcpy(param->data,&miracast_state,sizeof(AppMsgMiracastState));
        GxBus_MessageSend(new_msg);
    }
    return;
}

static int _miracast_stb_ssid_get(char* ssid, int ssid_len)
{
    char ssid_name_tmp[MIRACAST_SSID_NAME_MAX_LEN] = {0};
    char default_ssid[MIRACAST_SSID_NAME_MAX_LEN] = {0};
    char chipidbuf[8] = {0};
    app_chip_sn_get(chipidbuf, 8);
    sprintf(default_ssid,"gx_%02x%02x%02x%02x",chipidbuf[4],chipidbuf[5],chipidbuf[6],chipidbuf[7]);
    GxBus_ConfigGet(MIRACAST_SSID_NAME, ssid_name_tmp, sizeof(ssid_name_tmp), default_ssid);
    snprintf(ssid, ssid_len, "%s", ssid_name_tmp);
    return 0;
}

int app_wifi_getp2p_list(char (*destination)[16], int maxnum)
{
    int i = 0;
    uint32_t dev_num = 0;
    struct p2p_device_list list;
    GxWiFi_GetP2PDevice(&list);
    dev_num = ( list.count > maxnum ) ? maxnum : list.count;
    for(i = 0; i < dev_num; i++)
    {
        memcpy(destination[i],list.dev_names[i],MAX_DEV_LEN);
    }
    GxWiFi_PutP2PDevice(&list);
    return dev_num;
}


static int _miracast_wifi_list_update(void)
{
    int i = 0;
    for (i = 0; i < MAX_DEV_NUM; i++) {
        memset(wifi_list[i], 0, MAX_DEV_LEN); // 使用 memset 将字符串清零
    }
    return app_wifi_getp2p_list(wifi_list,MAX_DEV_NUM);
}

static int _miracast_ui_ctrol_init(void)
{
    char wifi_name_tmp[MAX_DEV_LEN] = {0};
    int dev_num = 0;
    int i;
    bool sync_flag = false;
    GxBus_ConfigGet(MIRACAST_WIFI_SELECT, wifi_name_tmp, sizeof(wifi_name_tmp), MIRACAST_WIFI_SELECT_DEFAULT);
    dev_num = _miracast_wifi_list_update();
    if(dev_num <= 0)
        return -1;
    for(i = 0; i < dev_num; i++)
    {
        if(strcmp(wifi_name_tmp,wifi_list[i]) == 0)
        {
            memcpy(MirCtrol.cur_dev,wifi_list[i],MAX_DEV_LEN);
            sync_flag = true;
        }
       if((i == dev_num-1) && sync_flag == false)
        {
            memcpy(MirCtrol.cur_dev,wifi_list[0],MAX_DEV_LEN);
            GxBus_ConfigSet(MIRACAST_WIFI_SELECT,wifi_list[0]);
        }
    }

    memset(MirCtrol.ssid, 0 , sizeof(MirCtrol.ssid));
    _miracast_stb_ssid_get(MirCtrol.ssid, MIRACAST_SSID_NAME_MAX_LEN);
    memset(MirCtrol.u_ssid, 0, sizeof(MirCtrol.u_ssid));
    memset(MirCtrol.net_dev, 0, sizeof(MirCtrol.net_dev));
    app_network_get_cur_dev_and_state(&(MirCtrol.net_dev));
    MirCtrol.play_state = MIRACAST_PLAY_NONE;
    MirCtrol.miracast_state = MIRACAST_STATE_WAIT;
    MirCtrol.rename_enable = false;
    MirCtrol.gif_timer = NULL;
    MirCtrol.info_timer = NULL;
    MirCtrol.channel = 6;
    return 0;
}

static int _timer_miracast_info_timeout(void* data)
{
    MirCtrol.showinfo_time--;
    if(MirCtrol.showinfo_time <= 0)
    {
        if(MirCtrol.info_timer != NULL)
        {
            remove_timer(MirCtrol.info_timer);
            MirCtrol.info_timer = NULL;
        }
        GUI_EndDialog(WND_MIRACAST_INFO);
    }
    else
    {
        char Buffer[100];
        PlayerProgInfo  *info = app_player_prog_info_get();
        if(GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_IPTV, info) == GXCORE_SUCCESS)
        {
            sprintf((char*)Buffer,"%d KB/s", info->net_speed);
            GUI_SetProperty(TXT_MIRACAST_INFO_NETSPEED, "string", Buffer);
        }
    }
    return 0;
}

static int _wfd_event_callback(const WFD_EVENT event , const void* data)
{
    switch (event) {
        /* WFD事件流 */
        case WFD_EVENT_START:
            app_miracast_send_ui_msg(event,NULL);
            break;
        case WFD_EVENT_STOP:
            break;
        case WFD_EVENT_RESOLUTION_SET:
            break;
        case WFD_EVENT_HDCP_PORT:
            break;
        case WFD_EVENT_START_PLAYER:
            app_miracast_send_ui_msg(event,NULL);
            break;
        case WFD_EVENT_STOP_PLAYER:
        case WFD_EVENT_DISCONNECTED:
        case WFD_EVENT_P2P_DISCONNECT:
        case WFD_EVENT_P2P_TIMEOUT:
        case WFD_EVENT_P2P_DHCP_FAIL:
        case WFD_EVENT_P2P_GO_NEGO_FAIL:
        case WFD_EVENT_P2P_WSC_EXCHANGE_FAIL:
        case WFD_EVENT_P2P_CONNECT_FAIL:
            app_miracast_send_ui_msg(event,NULL);
            break;
            /* WiFi事件流 */
        case WFD_EVENT_WIFI_READY:
        case WFD_EVENT_WIFI_INVALID:
        case WFD_EVENT_WIFI_SET_SSID_DONE:
        case WFD_EVENT_WIFI_PLUGOUT:
            break;
            /* P2P连接方式事件流 */
        case WFD_EVENT_P2P_SCAN_START:
            break;
        case WFD_EVENT_P2P_GO_NEGO_SUCC:
        case WFD_EVENT_P2P_WSC_EXCHANGE_SUCC:
        case WFD_EVENT_P2P_CONNECT_SUCC:
            app_miracast_send_ui_msg(event,(const char *)data);
            break;
            // case WFD_EVENT_P2P_DISCONNECT:
        case WFD_EVENT_P2P_DHCP_BEGIN:
        case WFD_EVENT_P2P_DHCP_SUCC:
            break;
        default:
            break;
    }
    return 0;
}

static void _miracast_rename_cb(PopKeyboard *data)
{
    if(data == NULL)
        return;
    if(data->in_ret == POP_VAL_CANCEL)
        return;
    if (data->out_name == NULL)
        return;

    gxwfd_stop();
    GxBus_ConfigSet(MIRACAST_SSID_NAME, data->out_name);
    memset(MirCtrol.ssid, 0 , sizeof(MirCtrol.ssid));
    _miracast_stb_ssid_get(MirCtrol.ssid, MIRACAST_SSID_NAME_MAX_LEN);
    gxwfd_start(_wfd_event_callback,MirCtrol.ssid,MirCtrol.channel);
}

static void _miracast_show_poptips(char *msg1, char *msg2, char* msg3)
{
    GUI_SetProperty(TXT_POPUP, "string", msg1);
    GUI_SetProperty(IMG_POPUP, "state", "show");
    GUI_SetProperty(TXT_POPUP, "state", "show");
    GUI_SetProperty(TXT_DEVICE, "string", msg2);
    GUI_SetProperty(TXT_DEVICE, "state", "show");
    GUI_SetProperty(TXT_POPUP_AP, "string", " ");
    GUI_SetProperty(TXT_POPUP_AP, "state", "show");
    GUI_SetProperty(TXT_POPUP_DETAIL, "string", msg3);
    GUI_SetProperty(TXT_POPUP_DETAIL, "state", "show");
    if(MirCtrol.rename_enable == true)
    {
        GUI_SetProperty(IMG_POPUP_RENAME, "state", "show");
        GUI_SetProperty(TXT_POPUP_RENAME, "state", "show");
        if(MirCtrol.select_count > 1)
        {
            GUI_SetProperty(IMG_POPUP_SELECT, "state", "show");
            GUI_SetProperty(TXT_POPUP_SELECT, "state", "show");
        }
        else
        {
            GUI_SetProperty(IMG_POPUP_SELECT, "state", "hide");
            GUI_SetProperty(TXT_POPUP_SELECT, "state", "hide");
        }
    }
    else
    {
        GUI_SetProperty(IMG_POPUP_RENAME, "state", "hide");
        GUI_SetProperty(TXT_POPUP_RENAME, "state", "hide");
        GUI_SetProperty(IMG_POPUP_SELECT, "state", "hide");
        GUI_SetProperty(TXT_POPUP_SELECT, "state", "hide");
    }
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);
}


static void _miracast_hide_poptips(void)
{
    GUI_SetProperty(IMG_POPUP, "state", "hide");
    GUI_SetProperty(TXT_POPUP, "state", "hide");
    GUI_SetProperty(TXT_DEVICE, "state", "hide");
    GUI_SetProperty(TXT_POPUP_AP, "state", "hide");
    GUI_SetProperty(TXT_POPUP_DETAIL, "state", "hide");
    GUI_SetProperty(IMG_POPUP_RENAME, "state", "hide");
    GUI_SetProperty(TXT_POPUP_RENAME, "state", "hide");
    GUI_SetProperty(IMG_POPUP_SELECT, "state", "hide");
    GUI_SetProperty(TXT_POPUP_SELECT, "state", "hide");
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);
}

static int _miracast_draw_gif(void* usrdata)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
    return 0;
}

static void _miracast_load_gif(void)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
    GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _miracast_free_gif(void)
{
    APP_TIMER_REMOVE(MirCtrol.gif_timer);
    GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _miracast_show_gif(void)
{
    GUI_SetProperty(IMG_GIF, "state", "show");
    APP_TIMER_ADD(MirCtrol.gif_timer, _miracast_draw_gif, 100, TIMER_REPEAT);
}

static void _miracast_hide_gif(void)
{
    APP_TIMER_REMOVE(MirCtrol.gif_timer);
    GUI_SetProperty(IMG_GIF, "state", "hide");
}

static int _miracast_exit_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        GUI_EndDialog(WND_MIRACAST);
    }
    return 0;
}

static void _miracast_mute_exec(void)
{
    GxMsgProperty_PlayerAudioMute player_mute;

    if (g_AppFullArb.state.mute == STATE_ON)
    {
        player_mute = 0;
        g_AppFullArb.state.mute = STATE_OFF;
        app_set_hardware_unmute();
    }
    else
    {
        player_mute = 1;
        g_AppFullArb.state.mute = STATE_ON;
        app_set_hardware_mute();
    }
    app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
}


static void _miracast_mute_draw(void)
{
    if (g_AppFullArb.state.mute == STATE_ON)
    {
        GUI_SetProperty(IMG_MUTE, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_MUTE, "state", "osd_trans_hide");
    }
    GUI_SetProperty(IMG_MUTE, "draw_now", NULL);
}

void app_miracast_unmute_change(void)
{
    if(g_AppFullArb.state.mute == STATE_ON)
    {
        _miracast_mute_exec();
        _miracast_mute_draw();
    }
}

static void _miracast_dlg_init(void)
{
    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
    app_miracast_unmute_change();
    gxwfd_exit();
    MirCtrol.miracast_state = MIRACAST_STATE_WAIT;
    MirCtrol.play_state = MIRACAST_PLAY_NONE;
    gxwfd_init(MirCtrol.cur_dev);
    gxwfd_start(_wfd_event_callback,MirCtrol.ssid,MirCtrol.channel);
}

static int _miracast_play_msg_proc(GxMessage *msg)
{
    GxMsgProperty_PlayerStatusReport* player_status = NULL;
    player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

    switch(player_status->status)
    {
        case PLAYER_STATUS_ERROR:
            {
                app_log_min("\n[%s]%d PLAYER_STATUS_ERROR\n", __func__, __LINE__);
                _miracast_dlg_init();
            }
            break;

        case PLAYER_STATUS_PLAY_END:
            {
                app_log_min("\n[%s]%d PLAYER_STATUS_PLAY_END\n", __func__, __LINE__);
                _miracast_dlg_init();
            }
            break;

        case PLAYER_STATUS_PLAY_RUNNING:
            {
                _miracast_hide_poptips();
                MirCtrol.play_state = MIRACAST_PLAY_RUNNING;
            }
            break;

        case PLAYER_STATUS_PLAY_START:
            break;

        case PLAYER_STATUS_STOPPED:
            break;

        case PLAYER_STATUS_START_FILL_BUF:
            _miracast_show_gif();
            app_log_flow("\n[%s]%d PLAYER_STATUS_START_FILL_BUF\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_END_FILL_BUF:
            _miracast_hide_gif();
            app_log_flow("\n[%s]%d PLAYER_STATUS_END_FILL_BUF\n", __func__, __LINE__);
            break;
        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

static int _miracast_netreport_msg_proc(GxMessage *msg)
{
    switch(msg->msg_id)
    {
        case GXMSG_USBWIFI_HOTPLUG_IN:
            {
                MirCtrol.select_count = _miracast_wifi_list_update();
                if(MirCtrol.rename_enable == true)
                {
                    if(MirCtrol.select_count > 1)
                    {
                        GUI_SetProperty(IMG_POPUP_SELECT, "state", "show");
                        GUI_SetProperty(TXT_POPUP_SELECT, "state", "show");
                        app_update_keyboard_dlg();
                        app_update_pop_dlg(WND_POP_TIP);
                        app_update_pop_dlg(WND_POP_BOOK);
                    }
                }
           }
            break;
        case GXMSG_USBWIFI_HOTPLUG_OUT:
            {
                GxMsgProperty_UsbwifiHotPlug *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
                if(strcmp(plug->dev_name,MirCtrol.cur_dev) == 0)
                {
                    GUI_EndDialog("after wnd_miracast");
                    GUI_SetProperty(WND_MIRACAST, "draw_now", NULL);
                    GUI_EndDialog(WND_MIRACAST);
                }
                else
                {
                    if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS && GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) != GXCORE_SUCCESS)
                    {
                        GUI_EndDialog(WND_POP_OPTION_LIST);
                    }
                    MirCtrol.select_count = _miracast_wifi_list_update();
                    if(MirCtrol.rename_enable == true)
                    {
                        if(MirCtrol.select_count > 1)
                        {
                        }
                        else
                        {
                            GUI_SetProperty(IMG_POPUP_SELECT, "state", "hide");
                            GUI_SetProperty(TXT_POPUP_SELECT, "state", "hide");
                            app_update_keyboard_dlg();
                            app_update_pop_dlg(WND_POP_TIP);
                            app_update_pop_dlg(WND_POP_BOOK);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

static int _mircst_report_msg_proc(void *msg)
{
    AppMsgMiracastState *MirState = (AppMsgMiracastState*)msg;
    switch(MirState->type)
    {
        case WFD_EVENT_START:
            {
                app_log_info("\n[%s]%d WFD start  MirState->type = %d\n",__func__, __LINE__,MirState->type);
                MirCtrol.rename_enable = true;
                char ssid_show[64];
                char dev_show[32];
                memset(ssid_show,0,sizeof(ssid_show));
                memset(dev_show,0,sizeof(dev_show));
                MirCtrol.miracast_state = MIRACAST_STATE_WAIT;
                sprintf(ssid_show,"%s",MirCtrol.ssid);
                if(strcmp(MirCtrol.cur_dev,"ra0") == 0)
                    strcpy(dev_show,"Wi-Fi-0");
                else
                    strcpy(dev_show,"Wi-Fi-1");
                _miracast_show_poptips(ssid_show,dev_show,STR_ID_WAITING_CONNECTION);
            }
            break;
        case WFD_EVENT_START_PLAYER:
            {
                app_log_info("\n[%s]%d WFD start play  MirState->type = %d\n",__func__, __LINE__,MirState->type);
                if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
                    GUI_EndDialog(WND_POP_TIP);
                MirCtrol.miracast_state = MIRACAST_STATE_CONNECTED;
                GUI_SetProperty(TXT_POPUP_DETAIL, "string",STR_ID_SUCCESS);
                app_update_pop_dlg(WND_POP_BOOK);
                GxPlayer_MediaPlay(PLAYER_FOR_IPTV, MIRACAST_PLAY_URL, 0, 50, NULL);
            }
            break;
        case WFD_EVENT_P2P_TIMEOUT:
        case WFD_EVENT_P2P_DHCP_FAIL:
        case WFD_EVENT_P2P_GO_NEGO_FAIL:
        case WFD_EVENT_P2P_WSC_EXCHANGE_FAIL:
        case WFD_EVENT_P2P_CONNECT_FAIL:
        case WFD_EVENT_STOP_PLAYER:
        case WFD_EVENT_DISCONNECTED:
        case WFD_EVENT_P2P_DISCONNECT:
            {
                app_log_info("\n[%s]%d WFD disconnect MirState->type = %d\n", __func__, __LINE__,MirState->type);
                MirCtrol.miracast_state = MIRACAST_STATE_ERROR;
                _miracast_dlg_init();
            }
            break;
        case WFD_EVENT_P2P_CONNECT_SUCC:
            {
                app_log_info("\n[%s]%d WFD p2p connect succes  MirState->type = %d\n",__func__, __LINE__,MirState->type);
                if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
                    GUI_EndDialog(WND_POP_TIP);
                if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
                    GUI_EndDialog(WND_POP_OPTION_LIST);
                if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
                    GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
                MirCtrol.miracast_state = MIRACAST_STATE_CONNECTING;
                MirCtrol.rename_enable = false;
                GUI_SetProperty(IMG_POPUP_RENAME, "state", "hide");
                GUI_SetProperty(TXT_POPUP_RENAME, "state", "hide");
                GUI_SetProperty(IMG_POPUP_SELECT, "state", "hide");
                GUI_SetProperty(TXT_POPUP_SELECT, "state", "hide");
                memset(MirCtrol.u_ssid, 0 , sizeof(MirCtrol.u_ssid));
                memcpy(MirCtrol.u_ssid, MirState->u_ssid,sizeof(MirCtrol.u_ssid));
                GUI_SetProperty(TXT_POPUP_AP, "string",MirCtrol.u_ssid);
                GUI_SetProperty(TXT_POPUP_DETAIL, "string",STR_ID_CONNECTTING);
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);

            }
            break;
        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

int app_miracast_msg_proc(GxMessage *msg)
{
    switch(msg->msg_id)
    {
        case GXMSG_PLAYER_STATUS_REPORT:
            _miracast_play_msg_proc(msg);
            break;

        case GXMSG_USBWIFI_HOTPLUG_IN:
        case GXMSG_USBWIFI_HOTPLUG_OUT:
            _miracast_netreport_msg_proc(msg);
            break;
        case APPMSG_MESSAGE:
            {
                AppMsgMessageClass *new_message = NULL;
                new_message = (AppMsgMessageClass*)GxBus_GetMsgPropertyPtr(msg,AppMsgMessageClass);
                if(APPMSG_MIRACAST_STATE_REPORT == new_message->type && (NULL != new_message->data))
                    _mircst_report_msg_proc(new_message->data);
            }
            break;
        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

static int app_wifi_select_exit_callback(int32_t ret_sel, unsigned short key)
{
    if(strcmp(MirCtrol.cur_dev,wifi_list[ret_sel]) != 0)
    {
        gxwfd_exit();
        gxwfd_init(MirCtrol.cur_dev);
        memcpy(MirCtrol.cur_dev,wifi_list[ret_sel],MAX_DEV_LEN);
        GxBus_ConfigSet(MIRACAST_WIFI_SELECT, wifi_list[ret_sel]);
        gxwfd_start(_wfd_event_callback,MirCtrol.ssid,MirCtrol.channel);
    }
    return 0;
}

SIGNAL_HANDLER int app_miracast_create(GuiWidget *widget, void *usrdata)
{
    gxwfd_init(MirCtrol.cur_dev);
    _miracast_load_gif();
    _miracast_hide_gif();
    app_netapps_aspect_init();
    gxwfd_start(_wfd_event_callback,MirCtrol.ssid,MirCtrol.channel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_miracast_destroy(GuiWidget *widget, void *usrdata)
{
    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
    app_miracast_unmute_change();
    app_unset_window_back_ground(WND_MIRACAST,true,true);
    _miracast_free_gif();
    app_netapps_aspect_exit();
    gxwfd_exit();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_miracast_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_OK:
            case STBK_INFO:
                if(MirCtrol.play_state == MIRACAST_PLAY_RUNNING)
                {
                    GUI_CreateDialog(WND_MIRACAST_INFO);
                }
                break;
            case STBK_LEFT:
            case STBK_VOLDOWN:
            case STBK_RIGHT:
            case STBK_VOLUP:
                {
                    if(MirCtrol.play_state == MIRACAST_PLAY_RUNNING)
                    {
                        GUI_CreateDialog(WND_VOLUME);
                        GUI_SendEvent(WND_VOLUME, event);
                        _miracast_mute_draw();
                    }
                }
                break;
            case STBK_EXIT:
            case STBK_MENU:
                {
                    PopDlg pop = {0};
                    pop.type = POP_TYPE_YES_NO;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_MIRACAST_EXIT;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.exit_cb = _miracast_exit_pop_cb;
                    pop.pos.x = WND_DMR_POP_DLG_X;
                    pop.pos.y = WND_DMR_POP_DLG_Y;
                    popdlg_create(&pop);
                }
                break;
            case STBK_RED:
                if(MirCtrol.rename_enable == true)
                {
                    if(MirCtrol.select_count > 1)
                    {
                        int i = 0;
                        int j = 0;
                        int32_t init_value = 0;
                        char wifi_name_tmp[MAX_DEV_LEN] = {0};
                        GxBus_ConfigGet(MIRACAST_WIFI_SELECT, wifi_name_tmp, sizeof(wifi_name_tmp), MIRACAST_WIFI_SELECT_DEFAULT);
                        MirCtrol.select_count = _miracast_wifi_list_update();
                        char **p_info = NULL ;
                        p_info = (char**)GxCore_Calloc(MirCtrol.select_count, sizeof(char*));
                        if(NULL == p_info)
                        {
                            app_log_error("\033[31m!!!!!![%s, %d]:GxCore_Calloc failed!!!!!!!!\033[0m\n", __func__, __LINE__);
                            return EVENT_TRANSFER_STOP;
                        }
                        for(i = 0; i < MirCtrol.select_count; i++)
                        {
                            p_info[i] = (char*)GxCore_Malloc(MAX_DEV_LEN);
                            if(NULL == p_info[i])
                            {
                                app_log_error("\033[31m!!!![%s, %d]:GxCore_Calloc failed!!!!!\033[0m\n", __func__, __LINE__);
                                if(p_info != NULL)
                                {
                                    for(j = 0; j < i; j++)
                                    {
                                        if(p_info[j] != NULL)
                                            GxCore_Free(p_info[j]);
                                    }
                                    GxCore_Free(p_info);
                                }
                                return EVENT_TRANSFER_STOP;
                            }
                            if(strcmp(wifi_name_tmp,wifi_list[i]) == 0)
                            {
                                init_value = i;
                            }
                            memcpy(p_info[i],wifi_list[i],MAX_DEV_LEN);
                        }
                        PopList pop_list;
                        memset(&pop_list, 0, sizeof(PopList));
                        pop_list.sel = init_value;
                        pop_list.title = STR_ID_WIFI_SELECT;
                        pop_list.item_num = MirCtrol.select_count;
                        pop_list.item_content = p_info;
                        pop_list.show_num= true;
                        pop_list.mode = POP_MODE_UNBLOCK;
                        pop_list.exit_cb = app_wifi_select_exit_callback;
                        poplist_create(&pop_list);
                    }
                }
                else if(MirCtrol.play_state != MIRACAST_PLAY_NONE)
                {
                    app_netapps_aspect_hotkey();
                }
                break;
            case STBK_GREEN:
                {
                    if(MirCtrol.rename_enable == true)
                    {
                        static PopKeyboard keyboard;
                        char ssid_name_tmp[MIRACAST_SSID_NAME_MAX_LEN] = {0};
                        char default_ssid[MIRACAST_SSID_NAME_MAX_LEN] = {0};
                        char chipidbuf[8] = {0};
                        app_chip_sn_get(chipidbuf, 8);
                        sprintf(default_ssid,"gx_%02x%02x%02x%02x",chipidbuf[4],chipidbuf[5],chipidbuf[6],chipidbuf[7]);
                        GxBus_ConfigGet(MIRACAST_SSID_NAME, ssid_name_tmp, sizeof(ssid_name_tmp), default_ssid);
                        memset(&keyboard, 0, sizeof(PopKeyboard));
                        keyboard.in_name    = ssid_name_tmp;
                        keyboard.max_num    = MIRACAST_SSID_NAME_EDIT_LEN-1;
                        keyboard.out_name   = NULL;
                        keyboard.change_cb  = NULL;
                        keyboard.release_cb = _miracast_rename_cb;
                        keyboard.usr_data   = NULL;
                        keyboard.pos.x = 200;
                        keyboard.pos.y = 130;
                        multi_language_keyboard_create(&keyboard);
                    }
                    else if(MirCtrol.play_state != MIRACAST_PLAY_NONE)
                    {
                        _miracast_dlg_init();
                    }
                }
                break;
            case STBK_BLUE:
                break;
            case STBK_YELLOW:
                break;
            case STBK_MUTE:
                {
                    _miracast_mute_exec();
                    _miracast_mute_draw();
                }
                break;
            case STBK_ASPECT:
                {
                    app_netapps_aspect_hotkey();
                }
                break;
            default:
                break;
        }
    }
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_miracast_service(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    GxMessage * msg;

    APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

    event = (GUI_Event *)usrdata;
    msg = (GxMessage*)event->msg.service_msg;
    return app_miracast_msg_proc(msg);
}

SIGNAL_HANDLER int app_miracast_lost_focus(GuiWidget *widget, void *usrdata)
{
    app_log_debug("=====>%s\n", __func__);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_miracast_got_focus(GuiWidget *widget, void *usrdata)
{
    app_log_debug("=====>%s\n", __func__);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_miracast_info_create(GuiWidget *widget, void *usrdata)
{
    char Buffer[100];
    char time_str[24] = {0};
    char *tmp_str = NULL;
    char solution_str[15] = {0};
    GUI_SetProperty(TXT_MIRACAST_INFO_SSID, "string", MirCtrol.u_ssid);

    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));
    GUI_GetProperty(TXT_MIRACAST_INFO_TIME, "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, time_str+MIN_SEC_OFFSET) != 0))
    {
        GUI_SetProperty(TXT_MIRACAST_INFO_TIME, "string", time_str+MIN_SEC_OFFSET);
    }
    time_str[MIN_SEC_OFFSET] = '\0';
    GUI_GetProperty(TXT_MIRACAST_INFO_DATE, "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, time_str) != 0))
    {
        GUI_SetProperty(TXT_MIRACAST_INFO_DATE, "string", time_str);
    }

    PlayerProgInfo  *info = app_player_prog_info_get();
    if(GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_IPTV, info) == GXCORE_SUCCESS)
    {
        if((info->video.width >= 0 && info->video.width <= 1920)&&( info->video.height != 0 && info->video.height <= 1088))
        {
            sprintf(solution_str, "%d x %d", info->video.width, info->video.height);
            GUI_SetProperty(TXT_MIRACAST_INFO_RESOLUTION, "string", solution_str);
        }
        else
        {
            GUI_SetProperty(TXT_MIRACAST_INFO_RESOLUTION, "string", "NO RES");
        }
        memset(Buffer, 0, 100);
        sprintf((char*)Buffer,"%d KB/s", info->net_speed);
        GUI_SetProperty(TXT_MIRACAST_INFO_NETSPEED, "string", Buffer);
    }
    GUI_SetProperty(TXT_MIRACAST_INFO_ASPECT, "string", app_get_netapps_aspect_string());
    MirCtrol.showinfo_time = MIRACAST_INFOBAR_TIMEOUT;
    if (reset_timer(MirCtrol.info_timer) != 0)
    {
        MirCtrol.info_timer = create_timer(_timer_miracast_info_timeout, 1000, NULL, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_miracast_info_destroy(GuiWidget *widget, void *usrdata)
{
    if(MirCtrol.info_timer != NULL)
    {
        remove_timer(MirCtrol.info_timer);
        MirCtrol.info_timer = NULL;
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_miracast_info_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_MIRACAST_INFO);
                break;
            case VK_BOOK_TRIGGER:
            case STBK_PAUSE:
            case STBK_LEFT:
            case STBK_VOLDOWN:
            case STBK_RIGHT:
            case STBK_VOLUP:
            case STBK_RED:
            case STBK_GREEN:
            case STBK_BLUE:
            case STBK_YELLOW:
            case STBK_MUTE:
            case STBK_ASPECT:
                {
                    GUI_EndDialog(WND_MIRACAST_INFO);
                    GUI_SendEvent(WND_MIRACAST, event);
                }
                break;
            default:
                break;
        }
    }
	return EVENT_TRANSFER_STOP;
}

void app_create_miracast_menu(void)
{

    if( _miracast_ui_ctrol_init() != 0)
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.pos.x = POP_DLG_POS_X;
        pop.pos.y = POP_DLG_POS_Y;
        pop.str = STR_ID_CANT_MIRACAST;
        popdlg_create(&pop);
        return ;
    }
    GUI_CreateDialog(WND_MIRACAST);
}
#endif
