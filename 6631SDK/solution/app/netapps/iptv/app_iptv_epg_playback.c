#include "app.h"
#if IPTV_EPG_PLAYBACK_SUPPORT
#include "app_module.h"
#include "../app_netvideo.h"
#include "../app_netvideo_play.h"
#include "full_screen.h"
#include "app_volume_value.h"
#include "app_windows.h"
#include <gx_apps.h>
#include "../app_play_bar_info.h"

#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"

#define IMG_MUTE    "img_epg_playback_mute"
#define IMG_MUTE1    "img_epg_playback_mute1"
#define IMG_EPG_PLAYBACK_PAUSE "img_epg_playback_pause"
#define IMG_GIF    "fimg_epg_playback_gif"
#define TXT_POPUP    "txt_epg_playback_popup"
#define IMG_POPUP    "img_epg_playback_popup"
#define TXT_PLAY_WAIT "text_epg_playback_wait"
#define TEXT_VIDEO_PLAY_NETSPEED "text_epg_playback_netspeed"
#define CACHE_SLIDER_PROCESS    "sliderbar_epg_playback_cache"

extern void app_av_setting_init(void);

status_t app_epg_playback_start_play(char *url, NetPlayState play_state, int64_t start, bool show_info);

struct ThisUICtrol{
    event_list *gif_timer;
    event_list *refresh_timer;
    int32_t is_get_url_busy;
    int32_t play_restart;
    NetVideoPlayOps play_ops;
    NetPlayState play_state;
    NetPlayState play_state_bak;
    NetPlayErrorType play_error;
    char *play_error_str;
};


static struct ThisUICtrol this = {0};

static void app_epg_playback_free_error_str(void)
{
	APP_FREE(this.play_error_str);
}

static int _epg_playback_draw_gif(void *usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_BOOK))
	{
		GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	}
	return 0;
}

static void _epg_playback_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _epg_playback_free_gif(void)
{
	APP_TIMER_REMOVE(this.gif_timer);
	this.gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _epg_playback_show_gif(void)
{
    GUI_SetProperty(IMG_GIF, "state", "show");
    if(NULL == this.gif_timer)//使用reset_timer会导致回调函数延时调用
        this.gif_timer = create_timer(_epg_playback_draw_gif, 200, NULL, TIMER_REPEAT);
}

static void _epg_playback_hide_gif(void)
{
	APP_TIMER_REMOVE(this.gif_timer);
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

static void _show_popdlg_msg(char *str)
{
    PopDlg pop;

    if(NULL == str)
        return;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = str;
    pop.timeout_sec = 1;
    popdlg_create(&pop);

    app_update_pop_dlg(WND_POP_BOOK);

    return;
}

static void _show_popup_msg(char* msg)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT) || GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK) || GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP))
    {
        return;
    }

	GUI_SetProperty(TXT_POPUP, "string", msg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");
	GUI_SetProperty(TXT_PLAY_WAIT, "state", "hide");
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);
}

static void _hide_popup_msg(void)
{
	GUI_SetProperty(IMG_POPUP, "state", "hide");
	GUI_SetProperty(TXT_POPUP, "state", "hide");
	GUI_SetProperty(TXT_PLAY_WAIT, "state", "hide");
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);
}

static int _epg_playback_netspeed_show(void)
{
	char buf_tmp[16] = {0};
	PlayerNetInfo netInfo = {0};

    if(GXCORE_SUCCESS == GxPlayer_MediaGetNetInfo(PLAYER_FOR_IPTV, &netInfo))
    {
        if(this.play_state == PLAY_STATE_LOAD || (this.play_state == PLAY_STATE_PAUSE && netInfo.eof != 1))
        {
            GUI_SetProperty(CACHE_SLIDER_PROCESS, "state", "show");
            GUI_SetProperty(TEXT_VIDEO_PLAY_NETSPEED, "state", "show");
            memset(buf_tmp, 0, sizeof(buf_tmp));
            if(100 == netInfo.cache_buf_percent)
            {
                sprintf(buf_tmp, "%d%%", netInfo.cache_buf_percent);
            }
            else
            {
                sprintf(buf_tmp, "%d KB/s", netInfo.net_speed);
            }
            GUI_SetProperty(TEXT_VIDEO_PLAY_NETSPEED, "string", buf_tmp);
            GUI_SetProperty(CACHE_SLIDER_PROCESS, "value", &netInfo.cache_buf_percent);
        }
    }
    else
	{
        if(this.play_state == PLAY_STATE_LOAD || this.play_state == PLAY_STATE_PAUSE)
        {
            memset(buf_tmp, 0, sizeof(buf_tmp));
            if(100 == netInfo.cache_buf_percent)
            {
                sprintf(buf_tmp, "%d%%", netInfo.cache_buf_percent);
            }
            else
            {
                sprintf(buf_tmp, "%d KB/s", netInfo.net_speed);
            }
            GUI_SetProperty(TEXT_VIDEO_PLAY_NETSPEED, "string", buf_tmp);
            GUI_SetProperty(CACHE_SLIDER_PROCESS, "value", &netInfo.cache_buf_percent);
        }
	}
    return 0;
}

static int _epg_playback_netspeed_hide(void)
{
    GUI_SetProperty(CACHE_SLIDER_PROCESS, "state", "hide");
    GUI_SetProperty(TEXT_VIDEO_PLAY_NETSPEED, "state", "hide");
    GUI_SetProperty(IMG_EPG_PLAYBACK_PAUSE, "state", "hide");
    return 0;
}

static void _show_error_info(void (*msg_show_cb)(char *))
{
    if(this.play_error == PLAY_PLAYER_ERROR)
    {
        msg_show_cb(STR_ID_PLAY_ERROR);
    }
    else if(this.play_error == PLAY_CONNECT_ERROR)
    {
        msg_show_cb(STR_ID_SERVER_FAIL);
    }
    else if(this.play_error == PLAY_SYSTEM_BUSY)
    {
        msg_show_cb(STR_ID_BUSY_TRY_AGAIN);
    }
    else if((this.play_error == PLAY_USER_ERROR)&&this.play_error_str)
    {
        msg_show_cb(this.play_error_str);
    }
    else
    {
        msg_show_cb("Internal error!");
    }
}

static int _epg_playback_state_draw(void *usrdata)
{
	if(this.play_state == PLAY_STATE_LOAD)
	{
		if(GXCORE_SUCCESS != GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
			_epg_playback_netspeed_show();

        if(this.play_state == this.play_state_bak)
            return 0;

        GUI_SetProperty(IMG_EPG_PLAYBACK_PAUSE, "state", "hide");
        _hide_popup_msg();
		_epg_playback_show_gif();
	}
    else if(this.play_state == PLAY_STATE_PAUSE)
    {
		if(GXCORE_SUCCESS != GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
			_epg_playback_netspeed_show();

        if(this.play_state == this.play_state_bak)
            return 0;
        GUI_SetProperty(IMG_EPG_PLAYBACK_PAUSE, "state", "show");
        _epg_playback_hide_gif();
    }
	else
	{
        if(this.play_state == this.play_state_bak)
            return 0;

		_epg_playback_netspeed_hide();
		_epg_playback_hide_gif();
		if(this.play_state == PLAY_STATE_ERROR)
		{
            _show_error_info(_show_popup_msg);
		}
		else
		{
			_hide_popup_msg();
		}
	}

	this.play_state_bak = this.play_state;
	return 0;
}

static void _epg_playback_state_refresh_start(void)
{
    if(0 != reset_timer(this.refresh_timer))
	{
		this.refresh_timer = create_timer(_epg_playback_state_draw, 300, NULL, TIMER_REPEAT);
	}
}

static void _epg_playback_state_refresh_stop(void)
{
    timer_stop(this.refresh_timer);
    this.play_state_bak = PLAY_STATE_NONE;

    _hide_popup_msg();
    _epg_playback_hide_gif();
	_epg_playback_netspeed_hide();
}

static void _epg_playback_hide_infobar(void)
{
    app_play_bar_info_hide();
}

static void _epg_playback_show_infobar(void)
{
    PlayBarInfoOps play_info_ops = {0};
    int total = 0;

    if(this.play_ops.play_ctrl.play_list_total_get)
    {
        total = this.play_ops.play_ctrl.play_list_total_get();
        play_info_ops.channel_total = total;
        if(total > 1)
        {
            play_info_ops.channel_num = this.play_ops.list_focus_item+1;
        }
        else
        {
            play_info_ops.channel_num = 0;
        }
    }
    play_info_ops.name = this.play_ops.netvideo_title;
    if(NULL == this.play_ops.bar_title)
        play_info_ops.bar_title = "EPG";
    else
        play_info_ops.bar_title = this.play_ops.bar_title;

    memcpy(&(play_info_ops.epg_ctrl), &(this.play_ops.epg_ctrl), sizeof(NetappsEpgCtrl));
    play_info_ops.url = this.play_ops.netvideo_url;
    app_play_bar_info_show(&play_info_ops);
}

static void _epg_playback_mute_exec(void)
{
    GxMsgProperty_PlayerAudioMute player_mute;

    if(g_AppFullArb.state.mute == STATE_ON)
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

static void _epg_playback_mute_draw(void)
{
    if(g_AppFullArb.state.mute == STATE_ON)
    {
        GUI_SetProperty(IMG_MUTE, "state", "show");
        GUI_SetProperty(IMG_MUTE1, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_MUTE, "state", "osd_trans_hide");
        GUI_SetProperty(IMG_MUTE1, "state", "osd_trans_hide");
    }
    //flush与draw_now完全相同
    GUI_SetInterface("flush", NULL);
}

void app_iptv_playback_unmute_change(void)
{
    if(g_AppFullArb.state.mute == STATE_ON)
    {
        _epg_playback_mute_exec();
        _epg_playback_mute_draw();
    }
}

static void _epg_playback_get_url_thread(void* arg)
{
	GxCore_ThreadDetach();
	if(this.is_get_url_busy)
	{
		this.play_error = PLAY_SYSTEM_BUSY;
		app_epg_playback_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}
	else if(this.play_ops.get_url)
	{
		this.is_get_url_busy = 1;
		this.play_ops.get_url(this.play_ops.list_focus_item);
		this.is_get_url_busy = 0;
	}
}

static void _epg_playback_get_url(void)
{
	int thread_id = 0;

	GxCore_ThreadCreate("epg_playback_geturl", &thread_id, _epg_playback_get_url_thread,
		NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

status_t app_epg_playback_start_play(char *url, NetPlayState play_state, int64_t start, bool show_info)
{

	if(play_state == PLAY_STATE_ERROR)
	{
		this.play_state = play_state;
	}
	else
	{
		if(url == NULL)
			return GXCORE_ERROR;
        this.play_state = PLAY_STATE_LOAD;

		if(show_info)
        {
			if(strcmp(GUI_GetFocusWindow(), WND_EPG_PLAYBACK) == 0)
				_epg_playback_show_infobar();
        }

        if(1 == this.play_restart)
            GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
        else
            GxPlayer_MediaStop(PLAYER_FOR_IPTV);


        GxMsgProperty_PlayerPlay* video_play = NULL;
        video_play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
        if(video_play == NULL)
        {
            app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
            return GXCORE_ERROR;
        }
		sprintf(video_play->url, "%s", url);
        app_log_min("url:%s\n", url);
		video_play->player = PLAYER_FOR_IPTV;
		video_play->start = start;
		app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)video_play);
        GXCORE_FREE(video_play);

		int32_t audio_vol = 0;
		GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
		app_system_vol_set(audio_vol);
	}
	return GXCORE_SUCCESS;
}

static void app_epg_playback_end_dialog(void)
{
    if(GUI_CheckDialog(WND_EPG_PLAYBACK) == GXCORE_SUCCESS)
    {
        void (*exit_cb)(void);
        exit_cb = this.play_ops.play_ctrl.play_exit;
        GUI_EndDialog(WND_EPG_PLAYBACK);
        if(exit_cb)
            exit_cb();
    }
}

status_t app_epg_playback(NetVideoPlayOps *play_ops)
{
	if(play_ops == NULL)
		return GXCORE_ERROR;

    app_net_video_play_halt();
	_epg_playback_state_refresh_stop();
	memset(&this, 0, sizeof(struct ThisUICtrol));
	memcpy(&this.play_ops, play_ops, sizeof(NetVideoPlayOps));
	this.play_state = PLAY_STATE_LOAD;
	if(this.play_ops.source_num<1)
	{
		this.play_ops.source_num = 1;
	}
	if(GXCORE_ERROR == GUI_CheckDialog(WND_EPG_PLAYBACK))
	{
        if(true == this.play_ops.play_wnd_created)
        {
            this.play_ops.play_wnd_created = false;
            return GXCORE_ERROR;
        }
		GUI_CreateDialog(WND_EPG_PLAYBACK);
		_epg_playback_mute_draw();
	}
    else
    {
        GDI_StopImageNoClear("*.gif");
		_epg_playback_mute_draw();
    }
    GUI_SetInterface("flush", NULL);

	if(strcmp(GUI_GetFocusWindow(), WND_EPG_PLAYBACK) == 0)
	{
		_epg_playback_state_refresh_start();
	}

	_epg_playback_get_url();

    return GXCORE_SUCCESS;
}

static void app_epg_playback_status(GxMessage* msg)
{
    PlayerNetInfo info = {0};
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

	if((player_status == NULL) || (strcmp((char*)(player_status->player), PLAYER_FOR_IPTV) != 0))
	{
        return;
	}

	switch(player_status->status)
	{
		case PLAYER_STATUS_ERROR:
			app_log_flow("\n[%s]%d PLAYER_STATUS_ERROR, %d\n", __func__, __LINE__, player_status->error);
			this.play_state = PLAY_STATE_ERROR;
			if(player_status->error == PLAYER_ERROR_SOURCE_RESTART)
			{
				if(this.play_ops.play_ctrl.play_restart)
				{
                    int cur_time_ms = 0;
                    if(GXCORE_SUCCESS == GxPlayer_MediaGetNetInfo(PLAYER_FOR_IPTV, &info))
                    {
                        cur_time_ms = info.restart_time;
                    }
					this.play_restart = 1;
					this.play_ops.play_ctrl.play_restart(cur_time_ms);
				}
				else
				{
					this.play_restart = 0;
					this.play_error = PLAY_CONNECT_ERROR;
                    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
				}
			}
			else
			{
				this.play_restart = 0;
				if(player_status->error == PLAYER_ERROR_NO_DATA_SOURCE)
				{
					if((this.play_ops.source_num>1)&&(this.play_ops.list_focus_item<(this.play_ops.source_num-1)))
					{
						this.play_ops.list_focus_item++;
						GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
						this.play_state = PLAY_STATE_LOAD;
						_epg_playback_get_url();
					}
					else
					{
						this.play_error = PLAY_CONNECT_ERROR;
                        GxPlayer_MediaStop(PLAYER_FOR_IPTV);
					}
				}
				else if((player_status->error == PLAYER_ERROR_ACCESS_ERROR))
				{
					this.play_error = PLAY_CONNECT_ERROR;
                    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
				}
				else
				{
					this.play_error = PLAY_PLAYER_ERROR;
                    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
				}
			}
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT))
            {
                _show_error_info(_show_popdlg_msg);
            }
            else
            {
			    _epg_playback_state_draw(NULL);
            }
			break;

		case PLAYER_STATUS_PLAY_END:
			this.play_restart = 0;
			app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_END\n", __func__, __LINE__);

            if(GXCORE_SUCCESS == GUI_CheckDialog(IPTV_EPG_MENU))
            {
                GUI_EndDialog(IPTV_EPG_MENU);
            }
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME))
            {
                GUI_EndDialog(WND_VOLUME);
            }
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT))
            {
                GUI_EndDialog(WND_COMMON_SELECT);
            }
            if(this.play_ops.play_ctrl.play_next && this.play_ops.play_ctrl.play_list_total_get)
            {
                _epg_playback_hide_infobar();
                this.play_ops.play_ctrl.play_next(0);
                int total = this.play_ops.play_ctrl.play_list_total_get();
                if(total > 1)
                {
                    if(this.play_ops.play_ctrl.play_list_data_get)
                    {
                        this.play_ops.list_focus_item = (this.play_ops.list_focus_item + 1 == total)?0:this.play_ops.list_focus_item + 1;
                        this.play_ops.netvideo_title = this.play_ops.play_ctrl.play_list_data_get(this.play_ops.list_focus_item);
                    }
                    _epg_playback_show_infobar();
                }
            }
            else
            {
                app_epg_playback_end_dialog();
            }
            break;

		case PLAYER_STATUS_PLAY_RUNNING:
            {
                const char *widget = NULL;
                if(this.play_restart == 0)
                {
                    widget = GUI_GetFocusWidget();
                    if(widget && strcmp(widget, WND_EPG_PLAYBACK) == 0)
                        _epg_playback_show_infobar();
                }
                this.play_restart = 0;
                this.play_state = PLAY_STATE_RUNNING;
                app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_RUNNING\n", __func__, __LINE__);
            }
			break;

		case PLAYER_STATUS_PLAY_START:
			app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_START\n", __func__, __LINE__);
			break;

        case PLAYER_STATUS_STOPPED:
			app_log_flow("\n[%s]%d PLAYER_STATUS_STOPPED\n", __func__, __LINE__);
			break;

        case PLAYER_STATUS_START_FILL_BUF:
            if(this.play_state != PLAY_STATE_PAUSE)
            {
                this.play_state = PLAY_STATE_LOAD;
                app_log_flow("\n[%s]%d PLAYER_STATUS_START_FILL_BUF\n", __func__, __LINE__);
            }
            break;

        case PLAYER_STATUS_END_FILL_BUF:
            if(this.play_state != PLAY_STATE_PAUSE)
            {
                this.play_state = PLAY_STATE_RUNNING;
                app_log_flow("\n[%s]%d PLAYER_STATUS_END_FILL_BUF\n", __func__, __LINE__);
            }
            break;

		default:
			break;
	}
}

static void app_epg_playback_avcodec_status(GxMessage* msg)
{
	GxMsgProperty_PlayerAVCodecReport *avcodec_status = NULL;
    avcodec_status = (GxMsgProperty_PlayerAVCodecReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);

    if(AVCODEC_ERROR == avcodec_status->acodec.state|| AVCODEC_ERROR == avcodec_status->vcodec.state)
    {

    }
    else if(avcodec_status->vcodec.state == AVCODEC_RUNNING || avcodec_status->acodec.state == AVCODEC_RUNNING)
    {
        this.play_state = PLAY_STATE_RUNNING;
    }
}

int app_epg_playback_msg_proc(GxMessage *msg)
{
	if(msg == NULL)
		return EVENT_TRANSFER_STOP;

	switch(msg->msg_id)
	{
		case GXMSG_PLAYER_STATUS_REPORT:
			app_epg_playback_status(msg);
			break;

		case GXMSG_PLAYER_AVCODEC_REPORT:
			app_epg_playback_avcodec_status(msg);
			break;

		default:
		break;
	}

	return EVENT_TRANSFER_STOP;
}

int app_epg_playback_recovery_status(void)
{
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME))
	{
		GUI_EndDialog(WND_VOLUME);
	}
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG_PLAYBACK))
	{
        app_epg_playback_end_dialog();
	}

	return 0;
}

SIGNAL_HANDLER int app_epg_playback_create(GuiWidget *widget, void *usrdata)
{
    app_system_performace_dynamic_change(NET_PLAY_MODE);
    app_av_setting_init();
    _epg_playback_load_gif();
    if(this.play_ops.play_ctrl.play_create)
    {
        this.play_ops.play_ctrl.play_create();
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_playback_destroy(GuiWidget *widget, void *usrdata)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME))
    {
        GUI_EndDialog(WND_VOLUME);
    }
	_epg_playback_hide_infobar();
	_epg_playback_state_refresh_stop();
	remove_timer(this.refresh_timer);
	this.refresh_timer = NULL;
	GxPlayer_MediaStop(PLAYER_FOR_IPTV);
	memset(&this.play_ops, 0, sizeof(NetVideoPlayOps));
	_epg_playback_free_gif();
	app_epg_playback_free_error_str();
	this.is_get_url_busy = 0;
    app_net_video_play_resume();
    app_system_performace_dynamic_change(DVB_PLAY_MODE);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_playback_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

	int ret = EVENT_TRANSFER_STOP;
    int total  = 0;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_TV_RADIO:
                if(this.play_ops.play_mode == 1)
                {
                    void (*tv_radio_cb)(void);

                    tv_radio_cb = this.play_ops.play_ctrl.play_tv_radio;
                    if(tv_radio_cb)
                    {
                        _hide_popup_msg();
                        if(GUI_CheckDialog(WND_EPG_PLAYBACK) == GXCORE_SUCCESS)
                        {
                            GUI_EndDialog(WND_EPG_PLAYBACK);
                        }
                        tv_radio_cb();
                    }
                }
                break;

            case STBK_EXIT:
                if(this.play_ops.play_mode != 1)
                {
                    _hide_popup_msg();
                    app_epg_playback_end_dialog();
                }
                break;

            case STBK_MENU:
                if(this.play_ops.play_mode == 1)
                {
                    void (*menu_cb)(void);
                    menu_cb = this.play_ops.play_ctrl.play_menu;
                    if(menu_cb)
                    {
                        _hide_popup_msg();
                        if(GUI_CheckDialog(WND_EPG_PLAYBACK) == GXCORE_SUCCESS)
                        {
                            GUI_EndDialog(WND_EPG_PLAYBACK);
                        }
                        menu_cb();
                    }
                }
                else
                {
                    app_epg_playback_end_dialog();
                }
                break;

            case STBK_INFO:
                if(this.play_state == PLAY_STATE_RUNNING || this.play_state == PLAY_STATE_PAUSE || this.play_state == PLAY_STATE_LOAD)
                {
                    _epg_playback_show_infobar();
                }
                break;
            case STBK_MUTE:
                _epg_playback_mute_exec();
                _epg_playback_mute_draw();
                break;

            case STBK_LEFT:
            case STBK_VOLDOWN:
                event->key.sym = APPK_VOLDOWN;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _epg_playback_mute_draw();
                break;

            case STBK_RIGHT:
            case STBK_VOLUP:
                event->key.sym = APPK_VOLUP;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _epg_playback_mute_draw();
                break;

            case STBK_UP:
                if(this.play_ops.play_ctrl.play_next)
                {
                    _epg_playback_hide_infobar();
                    this.play_ops.play_ctrl.play_next(0);
                    if(this.play_ops.play_ctrl.play_list_total_get)
                    {
                        total = this.play_ops.play_ctrl.play_list_total_get();
                        if(total > 1)
                        {
                            if(this.play_ops.play_ctrl.play_list_data_get)
                            {
                                this.play_ops.list_focus_item = (this.play_ops.list_focus_item + 1 == total)?0:this.play_ops.list_focus_item + 1;
                                this.play_ops.netvideo_title = this.play_ops.play_ctrl.play_list_data_get(this.play_ops.list_focus_item);
                            }
                            _epg_playback_show_infobar();
                        }
                    }
                }
                break;

            case STBK_DOWN:
                if(this.play_ops.play_ctrl.play_prev)
                {
                    _epg_playback_hide_infobar();
                    this.play_ops.play_ctrl.play_prev();
                    if(this.play_ops.play_ctrl.play_list_total_get)
                    {
                        total = this.play_ops.play_ctrl.play_list_total_get();
                        if(total > 1)
                        {
                            if(this.play_ops.play_ctrl.play_list_data_get)
                            {
                                this.play_ops.list_focus_item = (this.play_ops.list_focus_item == 0 )?total-1:this.play_ops.list_focus_item - 1;
                                this.play_ops.netvideo_title = this.play_ops.play_ctrl.play_list_data_get(this.play_ops.list_focus_item);
                            }

                            _epg_playback_show_infobar();
                        }
                    }
                }
                break;

            case STBK_PLAY:
                if(this.play_state == PLAY_STATE_PAUSE)
                {
                    GxPlayer_MediaResume(PLAYER_FOR_IPTV);
                    this.play_state = PLAY_STATE_RUNNING;
                }
                break;
            case STBK_PAUSE:
                if(this.play_state == PLAY_STATE_NONE || this.play_state == PLAY_STATE_ERROR || this.play_state == PLAY_STATE_LOAD)
                    break;
                if(this.play_state == PLAY_STATE_PAUSE)
                {
                    GxPlayer_MediaResume(PLAYER_FOR_IPTV);
                    this.play_state = PLAY_STATE_RUNNING;
                }
                else
                {
                    GxPlayer_MediaPause(PLAYER_FOR_IPTV);
                    this.play_state = PLAY_STATE_PAUSE;
                }
                break;

            case STBK_OK:
                if(this.play_ops.play_ctrl.play_list_total_get != NULL && this.play_ops.play_ctrl.play_list_total_get() > 0)
                {
                    _hide_popup_msg();
                }
                else if(this.play_state == PLAY_STATE_RUNNING || this.play_state == PLAY_STATE_PAUSE || this.play_state == PLAY_STATE_LOAD)
                {
                    _epg_playback_show_infobar();
                }
                else
                {
                    _epg_playback_hide_infobar();
                    if(this.play_ops.play_ctrl.play_ok != NULL)
                        this.play_ops.play_ctrl.play_ok();
                }
                break;
            case STBK_AUDIO:
                {
                    extern int app_netvideo_audio_create_dialog(void);
                    app_netvideo_audio_create_dialog();
                }
                break;
            default:
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int app_epg_playback_got_focus(GuiWidget *widget, void *usrdata)
{
    this.play_state_bak = PLAY_STATE_NONE;
    _epg_playback_state_refresh_start();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_playback_lost_focus(GuiWidget *widget, void *usrdata)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK) || GXCORE_SUCCESS == GUI_CheckDialog(IPTV_EPG_MENU))
    {
		_epg_playback_netspeed_hide();
        _epg_playback_hide_gif();
        _hide_popup_msg();
    }
	return EVENT_TRANSFER_STOP;
}

app_netapp_event_class app_netapp_event_iptv_epg = {
	.name = "netapp iptv epg",
	.msg_wndName = WND_EPG_PLAYBACK,
	.msg_proc_callback = app_epg_playback_msg_proc,
	.msg_dev_out_callback = app_epg_playback_recovery_status,
	.key_net_open_check = NULL,
};

#endif
