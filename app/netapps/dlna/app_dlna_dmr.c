#include "app.h"
#if DLNADMR_SUPPORT
#include "app_windows.h"
#include "app_keyboard_language.h"
#include "dlna.h"
#include "app_module.h"
#include "full_screen.h"
#include "include/app_volume_value.h"
#include "../app_play_bar_info.h"
#include "app_dlna.h"
#include <gx_apps.h>
#include "netapps/app_netapps_common.h"

#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#define DLNA_DMR_UUID    "dlna>dmruuid"
#define DLNA_DMR_NAME    "dlna>dmrname"
#define DLNA_DMR_NAME_MAX_LEN    (20)
#define DLNA_DMR_NAME_DEFAULT    "Lucky STB Render"
#define IMG_MUTE                "img_dlna_dmr_mute"
#define IMG_PAUSE                "img_dlna_dmr_pause"
#define IMG_GIF                 "img_dlna_dmr_gif"
#define GIF_PATH                WORK_PATH"theme/image/netapps/loading.gif"
#define IMG_POPUP            "img_dlna_dmr_popup"
#define TXT_POPUP            "txt_dlna_dmr_popup"
#define TXT_POPUP_DETAIL     "txt_dlna_dmr_popup_detail"
#define TXT_POPUP_RENAME     "txt_dlna_dmr_popup_rename"
#define IMG_POPUP_RENAME     "img_dlna_dmr_popup_rename"
#define IMG_FAIL_PATH WORK_PATH"theme/image/netapps/fail.jpg"
#ifdef ECOS_OS
#define IMG_MUSIC_PLAY_BACKGROUND    WORK_PATH"theme/image/bg.jpg"
#define TMP_PIC_DOWNLOAD_FILE "/mnt/play-tmp.png"
#else
#define IMG_MUSIC_PLAY_BACKGROUND    WORK_PATH"theme/image/netapps/music_back.jpg"
#define TMP_PIC_DOWNLOAD_FILE "/tmp/play-tmp.png"
#endif

struct ThisUICtrol{
    char dmr_name[64];
    bool dmr_running;
    unsigned int pic_download_handle;
    unsigned int pic_download_abort;
    int seek_enable;
    int rename_enable;
    int restart;
    event_list *gif_timer;
    event_list *msg_timer;
    AppMsg_DlnaUIControl ui_msg;
    DMRCtrlCallback dmr_ctrl;
    TransportStatusEnum trans_status;
    TransportStateEnum trans_state;
    DMRPlayInfo *info;
};

static struct ThisUICtrol this = {{0}};
static char *key_img = "dlna_background_key";

static int _get_ip(char *ip)
{
    GxMsgProperty_NetDevIpcfg s_ip_cfg;

    memset(&s_ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &s_ip_cfg.dev_name);
    if(s_ip_cfg.dev_name[0] == 0)
        return -1;
    app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_ip_cfg);
    strcpy(ip, s_ip_cfg.ip_addr);
    return 0;
}

static void _dmr_hide_play_infobar(void)
{
    app_play_bar_info_hide();
}

static int _dmr_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;

	GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	return 0;
}

static void _dmr_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;

	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _dmr_free_gif(void)
{
    APP_TIMER_REMOVE(this.gif_timer);
	this.gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _dmr_show_gif(void)
{
    if(this.dmr_running)
    {
        GUI_SetProperty(IMG_GIF, "state", "show");
        APP_TIMER_ADD(this.gif_timer, _dmr_draw_gif, 100, TIMER_REPEAT);
    }
}

static void _dmr_hide_gif(void)
{
	APP_TIMER_REMOVE(this.gif_timer);
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

static void _dmr_show_poptips(char *msg1, char *msg2)
{
    if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
        app_multi_lang_keyborad_menu_destroy();
    GUI_SetInterface("flush", NULL);
    GUI_SetProperty(TXT_POPUP, "string", msg1);
    GUI_SetProperty(TXT_POPUP_DETAIL, "string", msg2);
    GUI_SetProperty(IMG_POPUP, "state", "show");
    GUI_SetProperty(TXT_POPUP, "state", "show");
    GUI_SetProperty(TXT_POPUP_DETAIL, "state", "show");
    if(msg1 != NULL)
    {
        GUI_SetProperty(IMG_POPUP_RENAME, "state", "show");
        GUI_SetProperty(TXT_POPUP_RENAME, "state", "show");
		this.rename_enable = 1;
    }
    else
    {
        GUI_SetProperty(IMG_POPUP_RENAME, "state", "hide");
        GUI_SetProperty(TXT_POPUP_RENAME, "state", "hide");
		this.rename_enable = 0;
    }
    app_update_pop_dlg(WND_POP_BOOK);
}

static void _dmr_hide_poptips(void)
{
    GUI_SetProperty(IMG_POPUP, "state", "hide");
    GUI_SetProperty(TXT_POPUP, "state", "hide");
    GUI_SetProperty(TXT_POPUP_DETAIL, "state", "hide");
    GUI_SetProperty(IMG_POPUP_RENAME, "state", "hide");
    GUI_SetProperty(TXT_POPUP_RENAME, "state", "hide");
    this.rename_enable = 0;
}
static void _dmr_show_popdlg(char *str, int32_t time_out)
{
    if(NULL == str || time_out < 0)
        return;

    PopDlg pop;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = str;
    pop.timeout_sec = time_out;

    popdlg_create(&pop);

    app_update_pop_dlg(WND_POP_BOOK);

    return;
}

static void _dmr_mute_exec(void)
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

static void _dmr_mute_draw(void)
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

void app_dlna_dmr_unmute_change(void)
{
    if(g_AppFullArb.state.mute == STATE_ON)
    {
        _dmr_mute_exec();
        _dmr_mute_draw();
    }
}

static void _dmr_pause_draw(bool show)
{
    if (show)
    {
        GUI_SetProperty(IMG_PAUSE, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_PAUSE, "state", "osd_trans_hide");
    }
    GUI_SetProperty(IMG_PAUSE, "draw_now", NULL);
}

void app_create_dlna_dmr_menu(void)
{
    ubyte32 ip_address = {0};

    if(_get_ip(ip_address) < 0)
    {
        _dmr_show_popdlg(STR_ID_LINK_NETWORK, 2);
        return;
    }

    if(GUI_CheckDialog(WND_DLNA_DMR) == GXCORE_ERROR)
    {
#if SAT2IP_SERVER_SUPPORT
		if (app_sat2ip_operate_tip_get_force() < 0)
			return;
		app_sat2ip_unset_gui_ok_fast();
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
        app_dvb2ip_gui_flag_set(0);
#endif
		memset(&this, 0, sizeof(struct ThisUICtrol));
        GUI_CreateDialog(WND_DLNA_DMR);
    }
}

static void _pic_download_stop(void)
{
    this.pic_download_abort = 1;
    if(this.pic_download_handle != 0)
    {
        gxapps_curl_async_abort(this.pic_download_handle);
        this.pic_download_handle = 0;
    }
    if(GXCORE_FILE_EXIST == GxCore_FileExists(TMP_PIC_DOWNLOAD_FILE))
    {
        GxCore_FileDelete(TMP_PIC_DOWNLOAD_FILE);
    }
    this.pic_download_abort = 0;
}

static void _pic_download_cb(int message, int body)
{
    AppMsg_DlnaUIControl ui_msg = {0};
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

	if(message == GXCURL_STATE_COMPLETE)
	{
        if(this.pic_download_abort)
            return;
		if(callback_data->handle > 0)
		{
            if(callback_data->complete_data.size > 0)
            {
                if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
                {
                    ui_msg.type = UI_PIC_DOWNLOAD_SUCCESS;
                    app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
                    return;
                }
            }
		}
        ui_msg.type = UI_PIC_DOWNLOAD_ERROR;
        app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
	}
}

static int app_dlna_dmr_play(DMRPlayInfo *info, int is_new_url)
{
	GxMsgProperty_PlayerResume player_resume = {0};
    AppMsg_DlnaUIControl ui_msg = {0};

    if(!is_new_url && this.trans_state == TRANS_STATE_PLAYING)
        return 0;


    if(!is_new_url && this.trans_state == TRANS_STATE_PAUSE)
    {
        player_resume.player = PLAYER_FOR_IPTV;
        app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
    }
    else if(info->media_type != DLNA_IMAGE || is_new_url)
    {
        this.info = info;
        ui_msg.type = UI_DMR_PLAY;
        app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
    }
    return 0;
}

static int app_dlna_dmr_stop(void)
{
    AppMsg_DlnaUIControl ui_msg = {0};
	GxMsgProperty_PlayerStop player_stop = {0};
    if((this.info)&&(this.info->media_type == DLNA_IMAGE)&&(this.trans_state == TRANS_STATE_STOPPED))
    {
        _pic_download_stop();
        ui_msg.type = UI_WAITING_CONNECTION;
        app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
    }
    else if(this.trans_state == TRANS_STATE_PLAYING || this.trans_state == TRANS_STATE_PAUSE)
    {
        ui_msg.type = UI_WAITING_CONNECTION;
        app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
        player_stop.player = PLAYER_FOR_IPTV;
        app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
    }
    this.trans_state = TRANS_STATE_STOPPED;
    return 0;
}

static int app_dlna_dmr_pause_resume(void)
{
	GxMsgProperty_PlayerResume player_resume = {0};
    GxMsgProperty_PlayerPause player_pause = {0};

    if(this.trans_state == TRANS_STATE_PAUSE)
    {
        player_resume.player = PLAYER_FOR_IPTV;
        app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
    }
    else if(this.trans_state != TRANS_STATE_STOPPED)
    {
        player_pause.player = PLAYER_FOR_IPTV;
        app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
        this.trans_state = TRANS_STATE_PAUSE;
    }

    return 0;

}

static int app_dlna_dmr_seek(uint64 time)
{
	GxMsgProperty_PlayerSeek seek = {0};
    if(this.seek_enable)
    {
        memset(&seek, 0, sizeof(GxMsgProperty_PlayerSeek));
        seek.player = PLAYER_FOR_IPTV;
        seek.time = time * 1000;
        seek.flag = SEEK_ORIGIN_SET;

        app_send_msg_exec(GXMSG_PLAYER_SEEK, &seek);
    }
    return 0;
}

static int app_dlna_dmr_get_position_info(uint64 *ptotal_time, uint64 *pcur_time, uint64 *ptotal_count)
{
    PlayTimeInfo  tinfo = {0};

    if(GxPlayer_MediaGetTime(PLAYER_FOR_IPTV, &tinfo) == GXCORE_SUCCESS)
	{
        *pcur_time = (uint64)(tinfo.current / 1000);
        *ptotal_time = (uint64)(tinfo.totle / 1000);
	    *ptotal_count = 2147483647;
        if(*ptotal_time > 0)
            this.seek_enable = 1;
        else
            this.seek_enable = 0;

        return 0;
    }

	*ptotal_count = 2147483647;
    return 0;
}

static int app_dlna_dmr_get_transport_info(TransportStatusEnum *status, TransportStateEnum *state)
{
    *status = this.trans_status;
    *state = this.trans_state;
    return 0;
}

static int app_dlna_dmr_get_volume(int *value)
{
    *value = app_system_vol_get();
    return 0;
}

static int app_dlna_dmr_set_volume(int value)
{
    AppMsg_DlnaUIControl ui_msg = {0};

    ui_msg.type = UI_DMR_SET_VOLUME;
    ui_msg.valume = value > 99 ? 99 : value;
    app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);

    return 0;
}

static int app_dlna_dmr_error(DLNAErrorNoEnum err_no, const char *err_str)
{
    AppMsg_DlnaUIControl ui_msg = {0};

    ui_msg.type = UI_DLNA_ERROR;
    ui_msg.err_str = (char *)err_str;
    app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
    app_log_error("app_dlna_dmr_error:%s\n", err_str);
    return 0;
}

char *_dmr_get_uuid(void)
{
    static char uuid[42];
    int i = 0;

	GxBus_ConfigGet(DLNA_DMR_UUID, uuid, sizeof(uuid), "NULL");
    if(strncmp(uuid, "NULL", 4) == 0)
    {
        srand((unsigned int)time(NULL));
        sprintf(uuid,"%s", "uuid:");
        for(i = 5; i < 41; i++)
        {
            sprintf(uuid + i, "%x", rand()%0xf);
            if(i == 12 || i == 17 || i == 22 || i == 27)
            {
                i++;
                sprintf(uuid + i, "%s", "-");
            }
        }
        GxBus_ConfigSet(DLNA_DMR_UUID, uuid);
    }
    app_log_info("dmr %s\n", uuid);
	return uuid;
}

static void _dlna_dmr_rename_cb(PopKeyboard *data)
{
    DMRConfig config = {0};
    ubyte32 ip_address = {0};
    int ret = 0;

    if(data == NULL)
        return;
    if(data->in_ret == POP_VAL_CANCEL)
        return;
    if (data->out_name == NULL)
        return;

    if(_get_ip(ip_address) < 0)
    {
        app_log_error("ip address get error\n");
        _dmr_show_popdlg(STR_ID_LINK_NETWORK, 2);
        return;
    }

    DMR_stop();
    memset(this.dmr_name, 0 , sizeof(this.dmr_name));
    snprintf(this.dmr_name, 64, "%s  [%s]", data->out_name, ip_address);
    _dmr_show_poptips(this.dmr_name, STR_ID_WAITING_CONNECTION);
    GxBus_ConfigSet(DLNA_DMR_NAME, data->out_name);

    config.ip_address = ip_address;
    config.port = 54321;
    config.friendly_name = this.dmr_name;
    config.udn = _dmr_get_uuid();

    ret = DMR_start(&config, &this.dmr_ctrl);
    if(ret != 0)
    {
        _dmr_show_poptips(NULL, STR_ID_DMR_START_ERR);
    }
}

static int app_dlna_dmr_start_config(DMRCtrlCallback* dmr_ctrl)
{
    DMRConfig config = {0};
    ubyte32 ip_address = {0};
    char dmr_name_tmp[64] = {0};
    int ret = 0;

    popdlg_destroy();
    if(_get_ip(ip_address) < 0)
    {
        app_log_error("ip address get error\n");
        _dmr_show_poptips(NULL,STR_ID_LINK_NETWORK);
        return -1;
    }
    GxBus_ConfigGet(DLNA_DMR_NAME, dmr_name_tmp, sizeof(dmr_name_tmp), DLNA_DMR_NAME_DEFAULT);
    memset(this.dmr_name, 0 , sizeof(this.dmr_name));
    snprintf(this.dmr_name, 64, "%s  [%s]", dmr_name_tmp, ip_address);
    config.ip_address = ip_address;
    config.port = 54321;
    config.friendly_name = this.dmr_name;
    config.udn = _dmr_get_uuid();

    ret = DMR_start(&config, dmr_ctrl);
    if(ret != 0)
    {
        _dmr_show_poptips(NULL, STR_ID_DMR_START_ERR);
    }
    this.dmr_running = true;
    _dmr_show_poptips(this.dmr_name, STR_ID_WAITING_CONNECTION);

    return 0;
}

SIGNAL_HANDLER int app_dlna_dmr_create(GuiWidget *widget, void *usrdata)
{
    int32_t audio_vol = 0;
    app_system_performace_dynamic_change(NET_PLAY_MODE);
    this.dmr_ctrl.play = app_dlna_dmr_play;
    this.dmr_ctrl.stop= app_dlna_dmr_stop;
    this.dmr_ctrl.pause = app_dlna_dmr_pause_resume;
    this.dmr_ctrl.seek= app_dlna_dmr_seek;
    this.dmr_ctrl.get_position_info = app_dlna_dmr_get_position_info;
    this.dmr_ctrl.get_transport_info = app_dlna_dmr_get_transport_info;
    this.dmr_ctrl.get_volume = app_dlna_dmr_get_volume;
    this.dmr_ctrl.set_volume = app_dlna_dmr_set_volume;
    this.dmr_ctrl.error = app_dlna_dmr_error;

    GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
    app_system_vol_set(audio_vol);
    _dmr_load_gif();
    _dmr_mute_draw();
    GDI_RegisterJPEG();
    app_netapps_aspect_init();
    app_dlna_dmr_start_config(&this.dmr_ctrl);
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_dlna_dmr_destroy(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_PlayerStop player_stop = {0};

    player_stop.player = PLAYER_FOR_IPTV;
    app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
    app_netapps_aspect_exit();
    DMR_stop();
    this.dmr_running = false;
    app_unset_window_back_ground(WND_DLNA_DMR, false, true);
    _dmr_hide_gif();
    _dmr_hide_poptips();
    _dmr_free_gif();
    _pic_download_stop();
    gal_img_threshod_size(0);
    GDI_StopImageNoClear("*.gif");
    GDI_UnRegisterJPEG();
#if SAT2IP_SERVER_SUPPORT
	app_sat2ip_set_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    app_system_performace_dynamic_change(DVB_PLAY_MODE);
	return EVENT_TRANSFER_STOP;
}

static void _dmr_ok_keypress(void)
{
    PlayBarInfoOps play_info_ops;

    if((this.dmr_running == false)||(this.info == NULL)||(this.info->media_type == DLNA_IMAGE) || (this.trans_state != TRANS_STATE_PLAYING && this.trans_state != TRANS_STATE_PAUSE))
        return;

    play_info_ops.channel_num = 0;
    play_info_ops.name = this.info->title;
    play_info_ops.url = this.info->url;
    play_info_ops.bar_title = "DLNA";
    app_play_bar_info_show(&play_info_ops);
}

static int _dmr_exit_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        GUI_EndDialog(WND_DLNA_DMR);
    }
    return 0;
}

SIGNAL_HANDLER int app_dlna_dmr_keypress(GuiWidget *widget, void *usrdata)
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
                    _dmr_ok_keypress();
                break;
            case STBK_UP:
                break;
            case STBK_DOWN:
                break;
            case STBK_LEFT:
            case STBK_VOLDOWN:
                    event->key.sym = APPK_VOLDOWN;
                    GUI_CreateDialog(WND_VOLUME);
                    GUI_SendEvent(WND_VOLUME, event);
                    _dmr_mute_draw();
                break;
            case STBK_RIGHT:
            case STBK_VOLUP:
                    event->key.sym = APPK_VOLUP;
                    GUI_CreateDialog(WND_VOLUME);
                    GUI_SendEvent(WND_VOLUME, event);
                    _dmr_mute_draw();
                break;
            case STBK_EXIT:
            case STBK_MENU:
                {
                    PopDlg pop = {0};
                    pop.type = POP_TYPE_YES_NO;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_STOP_DMR_INFO;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.exit_cb = _dmr_exit_pop_cb;
                    pop.pos.x = APP_WND_DMR_POP_DLG_X;
                    pop.pos.y = APP_WND_DMR_POP_DLG_Y;
                    popdlg_create(&pop);
                }
                break;
            case STBK_RED:
                break;
            case STBK_GREEN:
                {
                    if(this.rename_enable)
                    {
                        static PopKeyboard keyboard;
                        char dmr_name_tmp[64] = {0};
                        GxBus_ConfigGet(DLNA_DMR_NAME, dmr_name_tmp, sizeof(dmr_name_tmp), DLNA_DMR_NAME_DEFAULT);
                        memset(&keyboard, 0, sizeof(PopKeyboard));
                        keyboard.in_name    = dmr_name_tmp;
                        keyboard.max_num    = DLNA_DMR_NAME_MAX_LEN-1;
                        keyboard.out_name   = NULL;
                        keyboard.change_cb  = NULL;
                        keyboard.release_cb = _dlna_dmr_rename_cb;
                        keyboard.usr_data   = NULL;
                        keyboard.pos.x = 200;
                        keyboard.pos.y = 130;
                        multi_language_keyboard_create(&keyboard);
                    }
                }
                break;
            case STBK_MUTE:
                _dmr_mute_exec();
                _dmr_mute_draw();
                break;
            case STBK_PAUSE:
                app_dlna_dmr_pause_resume();
                break;
            case STBK_PLAY:
                if(this.trans_state == TRANS_STATE_PAUSE)
                {
                    GxMsgProperty_PlayerResume player_resume = {0};
                    player_resume.player = PLAYER_FOR_IPTV;
                    app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
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

SIGNAL_HANDLER int app_dlna_dmr_lost_focus(GuiWidget *widget, void *usrdata)
{
    app_log_debug("=====>%s\n", __func__);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dlna_dmr_got_focus(GuiWidget *widget, void *usrdata)
{
    app_log_debug("=====>%s\n", __func__);
    return EVENT_TRANSFER_STOP;
}

static int app_dlna_dmr_ui_msg_proc_exec(void *usrdata)
{
    AppMsg_DlnaUIControl *ui_msg = &this.ui_msg;
    int ret = GXCORE_SUCCESS;

    APP_TIMER_REMOVE(this.msg_timer);
    _dmr_hide_play_infobar();
    switch(ui_msg->type)
    {
        case UI_WAITING_CONNECTION:
        {
            GxMsgProperty_PlayerStop player_stop = {0};
            player_stop.player = PLAYER_FOR_IPTV;
            app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
            _dmr_hide_gif();
            GDI_StopImageNoClear("*.gif");
            _dmr_pause_draw(false);
            popdlg_destroy();
            GUI_SetInterface("clear_image", NULL);
            GUI_SetInterface("image_enable", NULL);
            app_update_window_back_ground(WND_DLNA_DMR, NULL, true);
            _dmr_show_poptips(this.dmr_name, STR_ID_WAITING_CONNECTION);
            break;
        }
        case UI_DMR_SET_VOLUME:
        {
            GxMsgProperty_PlayerAudioMute player_mute = 0;
            g_AppFullArb.state.mute = STATE_OFF;
            app_set_hardware_unmute();
            app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
            _dmr_mute_draw();
            app_volume_value_set(ui_msg->valume);
            break;
        }
        case UI_DMR_PLAY:
        {
	        GxMsgProperty_PlayerStop player_stop = {0};

            _pic_download_stop();
            _dmr_show_gif();
            _dmr_hide_poptips();
            popdlg_destroy();
            GDI_StopImageNoClear("*.gif");
            player_stop.player = PLAYER_FOR_IPTV;
            app_log_info("xxxxxxxxxxxxxxxxxxxx app_dlna_dmr_play:%s, media_type:%d\n", this.info->url, this.info->media_type);
            if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
            {
                popdlg_destroy();
                app_multi_lang_keyborad_menu_destroy();
            }
            if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
            {
                GUI_EndDialog(WND_VOLUME);
            }
           if(this.info->media_type == DLNA_IMAGE)
            {
                GUI_SetInterface("clear_image", NULL);
                GUI_SetInterface("image_enable", NULL);
                app_update_window_back_ground(WND_DLNA_DMR, NULL, true);
                app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
                if(this.pic_download_abort == 0)
                    this.pic_download_handle =  gxapps_curl_async_download(this.info->url, TMP_PIC_DOWNLOAD_FILE, _pic_download_cb);
            }
            else
            {
                GxMsgProperty_PlayerPlay* video_play = NULL;
                video_play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
                if(video_play == NULL)
                {
                    app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
                    return 0;
                }

                video_play->player = PLAYER_FOR_IPTV;
                snprintf(video_play->url, PLAYER_URL_LONG, "%s", this.info->url);

                app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
                if(this.info->media_type == DLNA_AUDIO)
                {
                    gal_add_key_path(key_img, IMG_MUSIC_PLAY_BACKGROUND);
                    app_update_window_back_ground(WND_DLNA_DMR, key_img, true);
                }
                else
                {
                    app_unset_window_back_ground(WND_DLNA_DMR, true, true);
                }
                app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)video_play);
                GXCORE_FREE(video_play);
            }
            break;
        }
        case UI_PIC_DOWNLOAD_SUCCESS:
        {
            _dmr_hide_gif();
            if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
            {
                GUI_EndDialog(WND_VOLUME);
            }
            gal_img_threshod_size(PIC_SHOW_MAX_SIZE);
            GDI_StopImageNoClear("*.gif");
            GUI_SetProperty(WND_DLNA_DMR, "back_ground", NULL);
            GUI_SetInterface("flush", NULL);
            GUI_SetInterface("clear_image", NULL);
            GUI_SetInterface("image_enable", NULL);
            ret = GDI_PlayImage(TMP_PIC_DOWNLOAD_FILE, -1, -1);
            GxCore_FileDelete(TMP_PIC_DOWNLOAD_FILE);
            if(ret != GXCORE_SUCCESS && GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS)
            {
                _dmr_show_poptips(NULL, STR_ID_PLAY_ERROR);
            }
            break;
        }
        case UI_PIC_DOWNLOAD_ERROR:
        {
            GxCore_FileDelete(TMP_PIC_DOWNLOAD_FILE);
            _dmr_hide_gif();
            GDI_StopImageNoClear("*.gif");
            GUI_SetProperty(WND_DLNA_DMR, "back_ground", NULL);
            GUI_SetInterface("flush", NULL);
            GUI_SetInterface("clear_image", NULL);
            GUI_SetInterface("image_enable", NULL);
            ret = GDI_PlayImage(IMG_FAIL_PATH, -1, -1);
            if(GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS && ret != GXCORE_SUCCESS)
            {
                _dmr_show_poptips(NULL, STR_ID_PLAY_ERROR);
            }
            break;
        }
        case UI_DLNA_ERROR:
        {
            _dmr_hide_gif();
            _dmr_show_popdlg(ui_msg->err_str, 1);
            break;
        }
        default:
            break;
    }

    return 0;
}

int app_dlna_dmr_ui_msg_proc(GxMessage *msg)
{
    AppMsg_DlnaUIControl *ui_msg;

    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    ui_msg = GxBus_GetMsgPropertyPtr(msg, AppMsg_DlnaUIControl);
    app_log_debug("ui_msg type:%d\n", ui_msg->type);
    switch(ui_msg->type)
    {
        case UI_WAITING_CONNECTION:
        case UI_DMR_SET_VOLUME:
        case UI_DMR_PLAY:
        case UI_PIC_DOWNLOAD_SUCCESS:
        case UI_PIC_DOWNLOAD_ERROR:
        case UI_DLNA_ERROR:
            memcpy(&this.ui_msg, ui_msg, sizeof(this.ui_msg));
            APP_TIMER_ADD(this.msg_timer, app_dlna_dmr_ui_msg_proc_exec, 10, TIMER_REPEAT);
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}


int app_dlna_dmr_play_msg_proc(GxMessage *msg)
{
    GxMsgProperty_PlayerStop player_stop = {0};
    PlayerNetInfo info = {0};
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

    if((player_status == NULL) || (strcmp((char*)(player_status->player), PLAYER_FOR_IPTV) != 0) ||((this.info->url != NULL)&&(strcmp((char*)(player_status->url), this.info->url) != 0)))
	{
        return EVENT_TRANSFER_STOP;
	}

	switch(player_status->status)
	{
		case PLAYER_STATUS_ERROR:
            if(player_status->error == PLAYER_ERROR_SOURCE_RESTART)
            {
                GxMsgProperty_PlayerPlay* video_play = NULL;
                video_play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
                if(video_play == NULL)
                {
                    app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
                    return EVENT_TRANSFER_STOP;
                }

                video_play->player = PLAYER_FOR_IPTV;
                snprintf(video_play->url, PLAYER_URL_LONG, "%s", player_status->url);
                if (GxPlayer_MediaGetNetInfo(PLAYER_FOR_IPTV, &info) == GXCORE_SUCCESS)
                    video_play->start = info.restart_time;
                else
                    video_play->start = 0;
                app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)video_play);
                GXCORE_FREE(video_play);
                this.restart = 1;
                _dmr_show_gif();
                app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_RESTART,restart time:%lld\n", __func__, __LINE__, video_play->start);
            }
            else
            {
                player_stop.player = PLAYER_FOR_IPTV;
                app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
                _dmr_hide_play_infobar();
                _dmr_hide_gif();
                popdlg_destroy();
                _dmr_show_poptips(NULL, STR_ID_PLAY_ERROR);
                this.trans_state = TRANS_STATE_STOPPED;
                this.restart = 0;
                app_log_error("\n[%s]%d PLAYER_STATUS_ERROR: %d\n", __func__, __LINE__, player_status->error);
            }
            break;

		case PLAYER_STATUS_PLAY_END:
            player_stop.player = PLAYER_FOR_IPTV;
            app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
            _dmr_hide_play_infobar();
            _dmr_hide_gif();
            popdlg_destroy();
            _dmr_show_poptips(NULL, STR_ID_PLAY_END);
            this.trans_state = TRANS_STATE_STOPPED;
            this.restart = 0;
            _dmr_pause_draw(false);
			app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_END\n", __func__, __LINE__);
			break;

		case PLAYER_STATUS_PLAY_RUNNING:
            _dmr_hide_gif();
            _dmr_pause_draw(false);
            PlayBarInfoOps play_info_ops;
            play_info_ops.channel_num = 0;
            play_info_ops.name = this.info->title;
            play_info_ops.url = this.info->url;
            play_info_ops.bar_title = "DLNA";
            if(!this.restart)
            {
                app_play_bar_info_hide();
                app_play_bar_info_show(&play_info_ops);
            }
            this.restart = 0;
            this.trans_state = TRANS_STATE_PLAYING;
            app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_RUNNING\n", __func__, __LINE__);
			break;

		case PLAYER_STATUS_PLAY_START:
			app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_START\n", __func__, __LINE__);
			break;

		case PLAYER_STATUS_PLAY_PAUSE:
            _dmr_hide_gif();
            _dmr_pause_draw(true);
			app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_PAUSE\n", __func__, __LINE__);
			break;
        case PLAYER_STATUS_STOPPED:
            _dmr_hide_play_infobar();
            _dmr_hide_gif();
			app_log_flow("\n[%s]%d PLAYER_STATUS_STOPPED\n", __func__, __LINE__);
			break;

        case PLAYER_STATUS_START_FILL_BUF:
            _dmr_show_gif();
			app_log_flow("\n[%s]%d PLAYER_STATUS_START_FILL_BUF\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_END_FILL_BUF:
            _dmr_hide_gif();
			app_log_flow("\n[%s]%d PLAYER_STATUS_END_FILL_BUF\n", __func__, __LINE__);
            break;
		default:
			break;
	}
    return EVENT_TRANSFER_STOP;
}

int app_dlna_dmr_hotplug_msg_proc(GxMessage *msg)
{
    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_PLUG_OUT:
            if(this.dmr_running)
            {
                ubyte32 ip_address = {0};
                if(_get_ip(ip_address) < 0)
                {
                    GxMsgProperty_PlayerStop player_stop = {0};

                    player_stop.player = PLAYER_FOR_IPTV;
                    app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
                    this.trans_state = TRANS_STATE_STOPPED;
                    DMR_stop();
                    this.dmr_running = false;
                    popdlg_destroy();
                    _dmr_hide_play_infobar();
                    _dmr_hide_gif();
                    _dmr_pause_draw(false);
                    GUI_SetProperty(WND_DLNA_DMR, "back_ground", NULL);
                    GUI_SetInterface("flush", NULL);
                    GUI_SetInterface("clear_image", NULL);
                    GUI_SetInterface("image_enable", NULL);
                    _dmr_show_poptips(NULL,STR_ID_LINK_NETWORK);
                }
            }
            break;
        case GXMSG_NETWORK_STATE_REPORT:
            {
                GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                if(this.dmr_running)
                {
                    if(!(dev_state->dev_state & IF_STATE_CONNECTED))
                    {
                        GxMsgProperty_PlayerStop player_stop = {0};

                        player_stop.player = PLAYER_FOR_IPTV;
                        app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
                        this.trans_state = TRANS_STATE_STOPPED;
                        DMR_stop();
                        this.dmr_running = false;
                        popdlg_destroy();
                        _dmr_hide_play_infobar();
                        _dmr_hide_gif();
                        GUI_SetProperty(WND_DLNA_DMR, "back_ground", NULL);
                        GUI_SetInterface("flush", NULL);
                        GUI_SetInterface("clear_image", NULL);
                        GUI_SetInterface("image_enable", NULL);
                        _dmr_show_poptips(NULL,STR_ID_SERVER_FAIL);
                    }
                }
                else
                {
                    if(dev_state->dev_state == IF_STATE_CONNECTED)
                    {
                        app_dlna_dmr_start_config(&this.dmr_ctrl);
                    }
                }
            }
            break;
    }
    return EVENT_TRANSFER_STOP;
}

#endif
