#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_netvideo.h"
#include "app_netvideo_play.h"
#include "full_screen.h"
#include "app_volume_value.h"
#include "app_windows.h"
#include <gx_apps.h>
#include "app_play_bar_info.h"
#include "app_netapps_common.h"
#include "app_common_select.h"

#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#define IMG_FAIL_PATH WORK_PATH"theme/image/netapps/fail.jpg"

#define IMG_MUTE    "img_netvideo_play_mute"
#define IMG_MUTE1    "img_netvideo_play_mute1"
#define IMG_NETVIDEO_PAUSE "img_netvideo_play_pause"
#define IMG_GIF    "fimg_netvideo_play_gif"
#define TXT_POPUP    "txt_netvideo_play_popup"
#define IMG_POPUP    "img_netvideo_play_popup"
#define TEXT_SUBT "wnd_apps_netvideo_play_canvas_subt"
#define IMAGE_SUBT "wnd_apps_netvideo_play_pixel_subt"
#define TXT_PLAY_WAIT "text_netvideo_play_wait"
#define TEXT_VIDEO_PLAY_NETSPEED "text_netvideo_play_netspeed"
#define CACHE_SLIDER_PROCESS    "sliderbar_netvideo_play_cache"
#define KEY_IMG  "app_net_video_key"
#define DEFAULT_CMB_TITLE "[ALL]"
#ifdef ECOS_OS
#define TMP_PIC_DOWNLOAD_FILE "/mnt/play-tmp.png"
#define NETVIDEO_SUBTITLE_PATH "/mnt/netvideo_subtitle.txt"
#else
#define TMP_PIC_DOWNLOAD_FILE "/tmp/play-tmp.png"
#define NETVIDEO_SUBTITLE_PATH "/tmp/netvideo_subtitle.txt"
#endif

#define PLAY_DELAY_MS (300)
#define PLAY_DELAY_GATE_TIME (1000)

extern void app_av_setting_init(void);

struct ThisUICtrol{
    event_list *gif_timer;
    event_list *refresh_timer;
    event_list *geturl_timer;
    event_list *pic_play_timer;
    event_list* get_subtitle_timer;
    event_list* play_delay_timer;
    int32_t is_get_url_busy;
    int32_t play_restart;
    int pic_download_ok;
    uint32_t pic_download_handle;
    NetVideoSubtitleList *subtitle_list;
    unsigned int get_subtitle_handle;
    SubtState subtitle_status;
    unsigned int subtitle_download_handle;
    int subtitle_download_abort;
    int pic_download_abort;
    NetVideoPlayOps play_ops;
    NetPlayState play_state;
    NetPlayState play_state_bak;
    NetPlayErrorType play_error;
    GxMsgProperty_PlayerPlay video_play;
    char *play_error_str;
};

static struct ThisUICtrol this = {0};
static PlayerProgInfo* s_netvideo_info = NULL;
static void app_netvideo_subtitle_start(void);
typedef void(* netvideo_thread_func)(void);
#if APPLETS_SUPPORT
static int netvideo_work_finish = 0;
#endif


static void netvideo_thread_body(void* arg)
{
	netvideo_thread_func func = NULL;
	GxCore_ThreadDetach();
#if APPLETS_SUPPORT
	netvideo_work_finish++;
#endif
	func = (netvideo_thread_func)arg;
	if(func)
	{
		func();
	}
#if APPLETS_SUPPORT
	netvideo_work_finish--;
#endif
}

static void create_netvideo_thread(netvideo_thread_func func)
{
	static int thread_id = 0;

	GxCore_ThreadCreate("netvideo", &thread_id, netvideo_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}
#if APPLETS_SUPPORT
void app_netvideo_obj_exit(void)
{
    if(0 != netvideo_work_finish)
    {
        while(0 != netvideo_work_finish)
        {
            GxCore_ThreadDelay(100);
        }
    }
}
#endif

static void _net_video_playinfo_getforiptv(void)
{
    APP_FREE(s_netvideo_info);
    s_netvideo_info = (PlayerProgInfo *)GxCore_Malloc(sizeof(PlayerProgInfo));
    if(s_netvideo_info == NULL)
    {
        app_log_error("get PlayerProgInfo faild!!!!!!!!!!!\n");
        return;
    }
    memset(s_netvideo_info, 0, sizeof(PlayerProgInfo));
    if(GXCORE_SUCCESS != GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_IPTV,s_netvideo_info))
    {
        APP_FREE(s_netvideo_info);
    }
}

void  *app_get_netvideo_playinfo(void)
{
    if(s_netvideo_info == NULL)
        _net_video_playinfo_getforiptv();
    return s_netvideo_info;
}

static void app_net_video_free_error_str(void)
{
	APP_FREE(this.play_error_str);
}

void app_net_video_set_error_str(char *errstr)
{
	if(errstr)
	{
		app_net_video_free_error_str();
		this.play_error_str = GxCore_Strdup(errstr);
        this.play_state = PLAY_STATE_ERROR;
		this.play_error = PLAY_USER_ERROR;
	}
}

void app_net_video_set_play_error(NetPlayErrorType error)
{
	this.play_error = error;
}

static int _net_video_play_draw_gif(void *usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;

	if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_BOOK) && GXCORE_SUCCESS != GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
	{
		GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	}
	return 0;
}

static void _net_video_play_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _net_video_play_free_gif(void)
{
	APP_TIMER_REMOVE(this.gif_timer);
	this.gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _net_video_play_show_gif(void)
{
    GUI_SetProperty(IMG_GIF, "state", "show");
    if(NULL == this.gif_timer)//使用reset_timer会导致回调函数延时调用
        this.gif_timer = create_timer(_net_video_play_draw_gif, 200, NULL, TIMER_REPEAT);
}

static void _net_video_play_hide_gif(void)
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
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT) || GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK)
            || GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP) || GXCORE_SUCCESS == GUI_CheckDialog(IPTV_EPG_MENU))
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

static int _netvideo_play_netspeed_show(void)
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

static int _netvideo_play_netspeed_hide(void)
{
    GUI_SetProperty(CACHE_SLIDER_PROCESS, "state", "hide");
    GUI_SetProperty(TEXT_VIDEO_PLAY_NETSPEED, "state", "hide");
    GUI_SetProperty(IMG_NETVIDEO_PAUSE, "state", "hide");
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
        msg_show_cb(STR_ID_INTER_ERROR);
    }
}

static int _net_video_play_state_draw(void *usrdata)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(IPTV_EPG_MENU))
    {
        _net_video_play_hide_gif();
        return 0;
    }

	if(this.play_state == PLAY_STATE_LOAD)
	{
		if(GXCORE_SUCCESS != GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
			_netvideo_play_netspeed_show();

        if(this.play_state == this.play_state_bak)
            return 0;

        GUI_SetProperty(IMG_NETVIDEO_PAUSE, "state", "hide");
        _hide_popup_msg();
		_net_video_play_show_gif();
	}
    else if(this.play_state == PLAY_STATE_PAUSE)
    {
		if(GXCORE_SUCCESS != GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
			_netvideo_play_netspeed_show();

        if(this.play_state == this.play_state_bak)
            return 0;
        GUI_SetProperty(IMG_NETVIDEO_PAUSE, "state", "show");
        _net_video_play_hide_gif();
    }
	else
	{
        if(this.play_state == this.play_state_bak)
            return 0;

		_netvideo_play_netspeed_hide();
		_net_video_play_hide_gif();
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

static void _net_video_state_refresh_start(void)
{
    if(0 != reset_timer(this.refresh_timer))
	{
		this.refresh_timer = create_timer(_net_video_play_state_draw, 300, NULL, TIMER_REPEAT);
	}
}

static void _net_video_state_refresh_stop(void)
{
    if(this.refresh_timer != NULL)
    {
        timer_stop(this.refresh_timer);
    }
    this.play_state_bak = PLAY_STATE_NONE;

    _hide_popup_msg();
    _net_video_play_hide_gif();
	_netvideo_play_netspeed_hide();
}

static void _netvideo_play_hide_infobar(void)
{
    app_play_bar_info_hide();
}

static void _netvideo_play_show_infobar(void)
{
    PlayBarInfoOps play_info_ops = {0};
    int total = 0;

    if(this.play_ops.play_ctrl.play_list_total_get)
    {
        total = this.play_ops.play_ctrl.play_list_total_get();
        play_info_ops.channel_total = total;
        if(total > 0)
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
        play_info_ops.bar_title = "NET";
    else
        play_info_ops.bar_title = this.play_ops.bar_title;
#ifdef IPTV_EPG_SUPPORT
    memcpy(&(play_info_ops.epg_ctrl), &(this.play_ops.epg_ctrl), sizeof(NetappsEpgCtrl));
#endif
    play_info_ops.url = this.video_play.url;
    app_play_bar_info_show(&play_info_ops);
}

static void _net_video_mute_exec(void)
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

static void _net_video_mute_draw(void)
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

void app_net_video_unmute_change(void)
{
    if(g_AppFullArb.state.mute == STATE_ON)
    {
        _net_video_mute_exec();
        _net_video_mute_draw();
    }
}
static void _net_video_get_url_thread(void)
{
	if(this.is_get_url_busy)
	{
		this.play_error = PLAY_SYSTEM_BUSY;
		app_net_video_start_play(NULL, PLAY_STATE_ERROR, 0, false);
	}
	else if(this.play_ops.get_url)
	{
		this.is_get_url_busy = 1;
		this.play_ops.get_url(this.play_ops.list_focus_item);
		this.is_get_url_busy = 0;
	}
}

static void _net_video_get_url(void)
{
    create_netvideo_thread(_net_video_get_url_thread);
}

void app_net_video_subtitle_pause(void)
{
#if MEDIA_SUBTITLE_SUPPORT
    subtitle_pause();
#endif
}

void app_net_video_subtitle_resume(void)
{
#if MEDIA_SUBTITLE_SUPPORT
    subtitle_resume();
#endif
}

void app_net_video_subtitle_status_set(SubtState status,int index)
{
    this.subtitle_status = status;
    if(this.subtitle_list)
        this.subtitle_list->cur_subtitle = index;
}

static void app_net_video_subtitle_reset(void)
{
    GxBus_ConfigSetInt(NETAPPS_SUBTILE_LANGUAGE, 0);
}

static void app_netvideo_free_subtitle(void)
{
	if(this.subtitle_list)
	{
		app_net_video_free_subtitlelist(this.subtitle_list);
		this.subtitle_list = NULL;
	}
}

static void app_netvideo_get_subtitle_thread(void)
{
	NetVideoSubtitleList *subtitle = NULL;
	unsigned int handle = 0;
	this.get_subtitle_handle++;
	handle = this.get_subtitle_handle;
	if(this.play_ops.get_subtitle_list)
	{
		subtitle = this.play_ops.get_subtitle_list(0);
		if(handle == this.get_subtitle_handle)
		{
			if(subtitle)
			{
				this.subtitle_list = subtitle;
			}
		}
		else
		{
			if(subtitle)
			{
				app_net_video_free_subtitlelist(subtitle);
			}
		}
	}
}

static void app_netvideo_get_subtitle(void)
{
    create_netvideo_thread(app_netvideo_get_subtitle_thread);
}

static void app_netvideo_geturl_timer_stop(void)
{
	if(this.geturl_timer)
	{
		remove_timer(this.geturl_timer);
		this.geturl_timer = NULL;
	}
}

static int netvideo_geturl_timer_timeout(void *usrdata)
{
    this.geturl_timer = NULL;
	_net_video_get_url();
	return 0;
}

void app_net_video_free_subtitlelist(NetVideoSubtitleList *subtitlelist)
{
	int i = 0;

	if(subtitlelist)
	{
		if(subtitlelist->subtitle_num)
		{
			if(subtitlelist->subtitle_num>0)
			{
				for(i=0; i<subtitlelist->subtitle_num; i++)
				{
					APP_FREE(subtitlelist->subtitle_item[i].title);
					APP_FREE(subtitlelist->subtitle_item[i].url);
				}
			}
			APP_FREE(subtitlelist->subtitle_item);
		}
		APP_FREE(subtitlelist);
	}
}

static void netvideo_delete_subtitle(void)
{
	if(GXCORE_FILE_EXIST == GxCore_FileExists(NETVIDEO_SUBTITLE_PATH))
	{
		GxCore_FileDelete(NETVIDEO_SUBTITLE_PATH);
	}
}

static void netvideo_subtitle_download_stop(void)
{
	this.subtitle_download_abort = 1;
	if(this.subtitle_download_handle!=0)
	{
		gxapps_curl_async_abort(this.subtitle_download_handle);
		this.subtitle_download_handle = 0;
	}
	netvideo_delete_subtitle();
	this.subtitle_download_abort = 0;
}

static void netvideo_download_subtitle_cb(int message, int body)
{
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

	if(message == GXCURL_STATE_COMPLETE)
	{
		if(this.subtitle_download_abort)
			return;
		if(callback_data->handle>0)
		{
			if(this.subtitle_download_handle ==callback_data->handle)
			{
				if(callback_data->complete_data.retcode == GXAPPS_SUCCESS && this.subtitle_status == SUBT_STATE_DOWNLOADING)
				{
					this.subtitle_status = SUBT_STATE_NEED_DISPLAY;
				}
			}
		}
	}
}

static void netvideo_subtitle_download_start(void)
{
	netvideo_subtitle_download_stop();

	if(this.subtitle_list&&(this.subtitle_list->subtitle_num>0))
	{
		if(this.subtitle_download_abort == 0)
		{
			this.subtitle_download_handle = gxapps_curl_async_download(this.subtitle_list->subtitle_item[this.subtitle_list->cur_subtitle].url, NETVIDEO_SUBTITLE_PATH, netvideo_download_subtitle_cb);
		}
	}
}

static int netvideo_get_subtitle_timer_timeout(void *usrdata)
{
	if(this.play_ops.get_subtitle_list)
	{
		if((this.subtitle_status == SUBT_STATE_NEED_URL)
			&&(this.play_state == PLAY_STATE_RUNNING))
		{
			this.subtitle_status = SUBT_STATE_GET_URL;
			app_netvideo_free_subtitle();
			app_netvideo_get_subtitle();
		}
		else if(this.subtitle_status == SUBT_STATE_NEED_DOWNLOAD)
		{
			this.subtitle_status = SUBT_STATE_DOWNLOADING;
			netvideo_subtitle_download_start();
		}
		else if((this.subtitle_status == SUBT_STATE_NEED_DISPLAY)
			&&(this.play_state == PLAY_STATE_RUNNING))
		{
			this.subtitle_status = SUBT_STATE_RUNNING;
#if MEDIA_SUBTITLE_SUPPORT
            subtitle_start(NETVIDEO_SUBTITLE_PATH, PMP_SUBT_LOAD_OUTSIDE);
#endif
		}
	}
	else
	{
		APP_TIMER_REMOVE(this.get_subtitle_timer);
	}
	return 0;
}

static void app_netvideo_get_subtitle_timer_start(void)
{
	this.subtitle_status = SUBT_STATE_IDLE;
	APP_TIMER_REMOVE(this.get_subtitle_timer);
	this.get_subtitle_timer = create_timer(netvideo_get_subtitle_timer_timeout, 500, NULL, TIMER_REPEAT);
}

static void app_netvideo_subtitle_start(void)
{
	if(this.play_ops.get_subtitle_list)
	{
		this.subtitle_status = SUBT_STATE_NEED_URL;
	}
}
static void app_netvideo_geturl_timer_start(void)
{
	app_netvideo_geturl_timer_stop();
	this.geturl_timer = create_timer(netvideo_geturl_timer_timeout, 10, NULL, TIMER_ONCE);
}

void app_net_video_switch_source(int index)
{
	if((index>=0)&&(index<this.play_ops.source_num))
	{
		this.play_ops.list_focus_item = index;
		app_netvideo_geturl_timer_start();
	}
	else
	{
	    app_net_video_set_error_str(STR_ID_INTER_ERROR);
	}
}

void app_net_video_change_focus_item(int index)
{
    this.play_ops.list_focus_item = index;
}


static int _player_play_cb(void *usrdata)
{

    APP_TIMER_REMOVE(this.play_delay_timer);

    if(1 == this.play_restart)
        GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
    else
        GxPlayer_MediaStop(PLAYER_FOR_IPTV);

    app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)(&this.video_play));
    app_netvideo_subtitle_start();

    return GXCORE_SUCCESS;
}

static void _msg_net_video_start_play(GxMessage* msg)
{
    AppMsgVideoPlayer* play_msg = NULL;
    struct timeval tv;
    int cur_time_ms = 0;
    static int last_time_ms = 0;
    if(msg == NULL)
        return;

    play_msg = (AppMsgVideoPlayer*)GxBus_GetMsgPropertyPtr(msg,AppMsgVideoPlayer);

    if(play_msg->show_info)
    {
        if(strcmp(GUI_GetFocusWindow(), WND_NETVIDEO_PLAY) == 0)
            _netvideo_play_show_infobar();
    }
    if(strcmp(GUI_GetFocusWindow(), WND_NETVIDEO_PLAY) == 0 || strcmp(GUI_GetFocusWindow(), WND_COMMON_SELECT) == 0)
    {
        _net_video_state_refresh_stop();
        _net_video_state_refresh_start();
    }


    gettimeofday(&tv, NULL);
    cur_time_ms = tv.tv_sec*1000 + tv.tv_usec/1000;
    if(cur_time_ms - last_time_ms > PLAY_DELAY_GATE_TIME)
    {
        _player_play_cb(NULL);
    }
    else
    {
        if(0 != reset_timer(this.play_delay_timer))
        {
            this.play_delay_timer = create_timer(_player_play_cb, PLAY_DELAY_MS, NULL, TIMER_REPEAT);
        }
    }
    last_time_ms = cur_time_ms;

}

status_t app_net_video_start_play(char *url, NetPlayState play_state, int64_t start, bool show_info)
{
    AppMsgVideoPlayer param = {0};
  	if(play_state == PLAY_STATE_ERROR)
	{
		this.play_state = play_state;
	}
	else
	{
        if(url == NULL)
            return GXCORE_ERROR;
		this.play_state = PLAY_STATE_LOAD;
#if SOCKS5_SUPPORT
        extern void app_socks5_prxoy_url_get(char* resurl,char *url,int length);
        app_socks5_prxoy_url_get(this.video_play.url,url,sizeof(this.video_play.url));
#else
        snprintf(this.video_play.url, sizeof(this.video_play.url),"%s", url);
#endif
		this.video_play.player = PLAYER_FOR_IPTV;
		this.video_play.start = start;
		param.show_info = show_info;
		app_send_msg_exec(APPMSG_NETVIDEO_PLAY_START, &param);
	}
	return GXCORE_SUCCESS;
}


static status_t net_pic_update_timer_timeout(void* usrdata)
{
    int ret = GXCORE_SUCCESS;
    if(this.pic_download_ok == 0)
        return ret;
    _net_video_play_hide_gif();
    GUI_SetProperty(WND_NETVIDEO_PLAY, "back_ground", NULL);
	GUI_SetInterface("flush", NULL);
    if(this.pic_download_ok == 1)
    {
        ret = GDI_PlayImage(TMP_PIC_DOWNLOAD_FILE, -1, -1);
    }
    else
    {
        ret = GDI_PlayImage(IMG_FAIL_PATH, -1, -1);
    }
    if(ret != GXCORE_SUCCESS)
    {
        _show_popdlg_msg(STR_ID_PLAY_ERROR);
    }
    timer_stop(this.pic_play_timer);
    return GXCORE_SUCCESS;
}

static void pic_update_timer_reset(void)
{
	if (reset_timer(this.pic_play_timer) != 0)
	{
		this.pic_play_timer = create_timer(net_pic_update_timer_timeout, 300, NULL, TIMER_REPEAT);
	}
}

static void _pic_download_stop(void)
{
    APP_TIMER_REMOVE(this.pic_play_timer);
    this.pic_download_ok = 0;
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
                    this.pic_download_ok = 1;
                }
                else
                {
                    this.pic_download_ok = -1;
                }
            }
            else
            {
                this.pic_download_ok = -1;
            }
		}
	}
}

static void app_netvideo_play_end_dialog(void)
{
    if(GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_SUCCESS)
    {
        if(GUI_CheckDialog(WND_NETVIDEO_SUBT_SETTING) == GXCORE_SUCCESS)
            GUI_EndDialog(WND_NETVIDEO_SUBT_SETTING);
        popdlg_destroy();
        void (*exit_cb)(void);
        exit_cb = this.play_ops.play_ctrl.play_exit;
        if((GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY_NUMBER)))
            GUI_EndDialog(WND_NETVIDEO_PLAY_NUMBER);
        GUI_EndDialog(WND_NETVIDEO_PLAY);
        if(exit_cb)
            exit_cb();
    }
}

int app_netvideo_list_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NUM_LEN	    (10)
	ListItemPara* item = NULL;
	static char buffer[CHANNEL_NUM_LEN];

	item = (ListItemPara*)usrdata;
	if((NULL == item) || (NULL == this.play_ops.play_ctrl.play_list_data_get) || (0 > item->sel))
	{
        app_log_error("====%s, %d, err, %p, %p, %d\n", __FUNCTION__, __LINE__, item, this.play_ops.play_ctrl.play_list_data_get, item->sel);
		return EVENT_TRANSFER_STOP;
	}
    if(this.play_ops.play_ctrl.play_list_image_get)
    {
        //col-0: num
        item->x_offset = 2;
        item->image = this.play_ops.play_ctrl.play_list_image_get(item->sel);
        item->string = NULL;
    }
    else
    {
        //col-0: num
        item->x_offset = 2;
        item->image = NULL;
        memset(buffer, 0, CHANNEL_NUM_LEN);
        if(this.play_ops.play_ctrl.play_list_total_get == NULL)
        {
            sprintf(buffer, "%04d", (item->sel+1));
        }
        else
        {
            if(this.play_ops.play_ctrl.play_list_total_get() < 10000)
                sprintf(buffer, "%04d", (item->sel+1));
            else
                sprintf(buffer, "%05d", (item->sel+1));
        }
        item->string = buffer;
    }
	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = this.play_ops.play_ctrl.play_list_data_get(item->sel);
#ifdef IPTV_FAV_SUPPORT
    if(this.play_ops.play_ctrl.play_list_fav_check && this.play_ops.play_ctrl.str_green != NULL)
    {
        //col-2: fav
        item = item->next;
        item->x_offset = 0;
        if(this.play_ops.play_ctrl.play_list_fav_check(item->sel) > 0)
        {
            item->image = "s_icon_fav";
        }
        else
        {
            item->image = NULL;
        }
        item->string = NULL;
    }
#endif
	return EVENT_TRANSFER_STOP;
}

int app_netvideo_list_get_total(GuiWidget *widget, void *usrdata)
{
    if(this.play_ops.play_ctrl.play_list_total_get)
	    return this.play_ops.play_ctrl.play_list_total_get();
    else
        return 0;
}

int app_netvideo_list_create(GuiWidget *widget, void *usrdata)
{
    char *cmb_title = NULL;

    if(NULL != this.play_ops.play_ctrl.play_list_cmb_title_create)
    {
        this.play_ops.play_ctrl.play_list_cmb_title_create(&cmb_title);
    }
    else
    {
        cmb_title = DEFAULT_CMB_TITLE;
        this.play_ops.cmb_sel = 0;
    }

    //init cmb select
	GUI_SetProperty(g_CommonSelect.widget.group,"content", cmb_title);
	GUI_SetProperty(g_CommonSelect.widget.group, "select", &this.play_ops.cmb_sel);
	GUI_SetProperty(g_CommonSelect.widget.list, "active", &this.play_ops.list_focus_item);
	GUI_SetProperty(g_CommonSelect.widget.list, "select", &this.play_ops.list_focus_item);
    GUI_SetFocusWidget(g_CommonSelect.widget.list);
    if(NULL != this.play_ops.play_ctrl.play_list_cmb_title_free)
    {
        this.play_ops.play_ctrl.play_list_cmb_title_free(&cmb_title);
    }
    return EVENT_TRANSFER_STOP;
}

int app_netvideo_list_destroy(GuiWidget *widget, void *usrdata)
{
     if(NULL != this.play_ops.play_ctrl.play_list_destroy)
    {
        this.play_ops.play_ctrl.play_list_destroy();
    }
	return EVENT_TRANSFER_STOP;
}

int app_netvideo_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;

    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                default:
                    break;
            }
    }

    return EVENT_TRANSFER_STOP;
}

int app_netvideo_list_group_change(GuiWidget *widget, void *usrdata)
{

	int sel = -1;

	GUI_GetProperty(g_CommonSelect.widget.group, "select", &sel);
    if((this.play_ops.play_ctrl.play_list_change_group != NULL)&&(this.play_ops.play_ctrl.play_list_change_group(sel) > 0))
    {
            this.play_ops.list_focus_item = 0;
            if(this.play_ops.play_ctrl.play_list_data_get != NULL)
                this.play_ops.netvideo_title = this.play_ops.play_ctrl.play_list_data_get(this.play_ops.list_focus_item);
            GUI_SetProperty(g_CommonSelect.widget.list, "update_all", NULL);
            GUI_SetProperty(g_CommonSelect.widget.list, "select", &this.play_ops.list_focus_item);
            GUI_SetProperty(g_CommonSelect.widget.list, "active", &this.play_ops.list_focus_item);
    }
    return EVENT_TRANSFER_STOP;
}

int app_netvideo_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t sel=0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_DOWN:
				case STBK_UP:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				case STBK_LEFT:
				case STBK_RIGHT:
					GUI_SendEvent(g_CommonSelect.widget.group, event);
					break;

				case STBK_PAGE_UP:
					sel = 0;
					GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
					if(0 == sel)
					{
                        if(this.play_ops.play_ctrl.play_list_total_get && this.play_ops.play_ctrl.play_list_total_get() > 1)
                        {
						    sel = this.play_ops.play_ctrl.play_list_total_get() - 1;
						    GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
                        }
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;

				case STBK_PAGE_DOWN:
					sel = 0;
					GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                    if(this.play_ops.play_ctrl.play_list_total_get && this.play_ops.play_ctrl.play_list_total_get() > 1)
                    {
					    if(sel == (this.play_ops.play_ctrl.play_list_total_get() - 1))
					    {
					    	sel = 0;
					    	GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
					    	ret = EVENT_TRANSFER_STOP;
					    }
					    else
					    {
					    	ret = EVENT_TRANSFER_KEEPON;
					    }
                    }
					break;

				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(g_CommonSelect.widget.wnd);
					GUI_SetInterface("flush",NULL);
					break;

				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;

				case STBK_OK:
                    if(this.play_ops.play_ctrl.play_list_total_get)
                    {
                        if(this.play_ops.play_ctrl.play_list_total_get() > 1)
                        {
                            int active_sel = 0;
                            int sel = 0;
					        GUI_GetProperty(g_CommonSelect.widget.list, "active", &active_sel);
					        GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                            if(this.play_ops.play_ctrl.play_list_ok != NULL)
                            {
                                if(active_sel != sel)
                                {
                                    this.play_ops.list_focus_item = sel;
                                    this.play_ops.play_ctrl.play_list_ok(this.play_ops.list_focus_item);
                                    if(this.play_ops.play_ctrl.play_list_data_get != NULL)
                                    {
                                        this.play_ops.netvideo_title = this.play_ops.play_ctrl.play_list_data_get(this.play_ops.list_focus_item);
                                    }
                                    GUI_SetProperty(g_CommonSelect.widget.list, "active", &this.play_ops.list_focus_item);
                                    GUI_SetProperty(g_CommonSelect.widget.wnd, "update", NULL);
		                            _net_video_state_refresh_start();
                                }
                                else
					    		{
					    			GUI_EndDialog(g_CommonSelect.widget.wnd);
					    		}
                            }
                            else
                            {
					    		GUI_EndDialog(g_CommonSelect.widget.wnd);
                            }
                        }
                    }
                    break;

				case STBK_RED:
                    if(this.play_ops.play_ctrl.play_list_red && this.play_ops.play_ctrl.play_list_total_get)
                    {
                        if (this.play_ops.play_ctrl.play_list_total_get() > 1)
                        {
                            this.play_ops.play_ctrl.play_list_red(&this.play_ops.list_focus_item);
                        }
                    }
					break;

				case STBK_GREEN:
                    if(this.play_ops.play_ctrl.play_list_green && this.play_ops.play_ctrl.play_list_total_get)
                    {
                        if (this.play_ops.play_ctrl.play_list_total_get() > 1)
                        {
                            this.play_ops.play_ctrl.play_list_green(&this.play_ops.list_focus_item);
                        }
                    }
					break;

                case STBK_YELLOW:
                    if(this.play_ops.play_ctrl.play_list_yellow && this.play_ops.play_ctrl.play_list_total_get)
                    {
                        if (this.play_ops.play_ctrl.play_list_total_get() > 1)
                        {
                            this.play_ops.play_ctrl.play_list_yellow(&this.play_ops.list_focus_item);
                        }
                    }
					break;

                case STBK_BLUE:
                    if(this.play_ops.play_ctrl.play_list_blue && this.play_ops.play_ctrl.play_list_total_get)
                    {
                        if (this.play_ops.play_ctrl.play_list_total_get() > 1)
                        {
                            this.play_ops.play_ctrl.play_list_blue(&this.play_ops.list_focus_item);
                        }
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

int app_net_video_list_create_dialog(void)
{
	CommonSelectParas paras = {0};

	paras.choice = COMMON_SELECT_GROUP_ONLY;
	paras.content = "[All]";
    if(this.play_ops.play_ctrl.str_red != NULL)
        paras.tip_strs.str_red = this.play_ops.play_ctrl.str_red;
    if(this.play_ops.play_ctrl.str_green != NULL)
        paras.tip_strs.str_green = this.play_ops.play_ctrl.str_green;
    if(this.play_ops.play_ctrl.str_yellow != NULL)
        paras.tip_strs.str_yellow = this.play_ops.play_ctrl.str_yellow;
    if(this.play_ops.play_ctrl.str_blue != NULL)
        paras.tip_strs.str_blue = this.play_ops.play_ctrl.str_blue;

	paras.signal.create = app_netvideo_list_create;
	paras.signal.destroy = app_netvideo_list_destroy;
	paras.signal.keypress = app_netvideo_list_keypress;
	paras.signal.group_change = app_netvideo_list_group_change;
	paras.signal.list_get_total = app_netvideo_list_get_total;
	paras.signal.list_get_data = app_netvideo_list_get_data;
	paras.signal.list_keypress = app_netvideo_list_list_keypress;
	return app_common_select_create_dialog(&paras);
}

status_t app_net_video_stop_play(void)
{
	return GxPlayer_MediaStop(PLAYER_FOR_IPTV);
}

status_t app_net_picture_play(char *url)
{
    _pic_download_stop();
    GDI_StopImageNoClear("*.gif");
    GUI_SetInterface("clear_image", NULL);
    GUI_SetInterface("image_enable", NULL);
	GxPlayer_MediaStop(PLAYER_FOR_IPTV);
    app_update_window_back_ground(WND_NETVIDEO_PLAY, NULL, true);
	_net_video_state_refresh_stop();
	this.play_state = PLAY_STATE_NONE;
    _net_video_play_show_gif();
    pic_update_timer_reset();
    if(this.pic_download_abort == 0)
    {
        this.pic_download_handle = gxapps_curl_async_download(url, TMP_PIC_DOWNLOAD_FILE, _pic_download_cb);
    }
    return 0;
}

status_t app_net_set_music_background(char *url)
{
    gal_add_key_path(KEY_IMG, url);
    app_update_window_back_ground(WND_NETVIDEO_PLAY, KEY_IMG, true);
    return 0;
}

void app_net_set_video_mode(void)
{
    app_unset_window_back_ground(WND_NETVIDEO_PLAY, true, true);
}

NetVideoSubtitleList *app_net_video_get_subtitlelist(void)
{
    return this.subtitle_list;
}

static void _net_video_thisuictrol_init(void)
{
    _pic_download_stop();
	_net_video_state_refresh_stop();
    APP_TIMER_REMOVE(this.refresh_timer);
    APP_TIMER_REMOVE(this.play_delay_timer);
	this.get_subtitle_handle++;
	this.subtitle_status = SUBT_STATE_IDLE;
	APP_TIMER_REMOVE(this.get_subtitle_timer);
	app_netvideo_free_subtitle();
    app_net_video_subtitle_reset();
	netvideo_subtitle_download_stop();
	memset(&this.play_ops, 0, sizeof(NetVideoPlayOps));
    _net_video_play_hide_gif();
	app_net_video_free_error_str();
	app_netvideo_geturl_timer_stop();
	this.is_get_url_busy = 0;
}

status_t app_net_video_play(NetVideoPlayOps *play_ops)
{
	if(play_ops == NULL)
		return GXCORE_ERROR;

    GxPlayer_MediaStop(PLAYER_FOR_NORMAL);
    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
    _net_video_thisuictrol_init();
	memcpy(&this.play_ops, play_ops, sizeof(NetVideoPlayOps));
	this.play_state = PLAY_STATE_LOAD;
	if(this.play_ops.source_num<1)
	{
		this.play_ops.source_num = 1;
	}
	if(GXCORE_ERROR == GUI_CheckDialog(WND_NETVIDEO_PLAY))
	{
        if(true == this.play_ops.play_wnd_created)
        {
            this.play_ops.play_wnd_created = false;
            return GXCORE_ERROR;
        }
		GUI_CreateDialog(WND_NETVIDEO_PLAY);
		_net_video_mute_draw();
	}
    else
    {
		GDI_StopImageNoClear("*.gif");
		_net_video_mute_draw();
    }
    if(play_ops->netvideo_image != NULL)
    {
        gal_add_key_path(KEY_IMG, play_ops->netvideo_image);
        app_update_window_back_ground(WND_NETVIDEO_PLAY, KEY_IMG, true);
    }
    else
    {
        app_unset_window_back_ground(WND_NETVIDEO_PLAY, false, false);
    }

    if(strcmp(GUI_GetFocusWindow(), WND_NETVIDEO_PLAY) == 0)
    {
        _net_video_state_refresh_start();
    }

	_net_video_get_url();
	app_netvideo_get_subtitle_timer_start();

    return GXCORE_SUCCESS;
}

static void app_net_video_play_status(GxMessage* msg)
{
    PlayerNetInfo info = {0};
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

	if((player_status == NULL) || (strcmp((char*)(player_status->player), PLAYER_FOR_IPTV) != 0)||((this.video_play.url != NULL)&&(strcmp((char*)(player_status->url), this.video_play.url) != 0)))
	{
        return;
	}

	switch(player_status->status)
	{
		case PLAYER_STATUS_ERROR:
			app_log_flow("\n[%s]%d PLAYER_STATUS_ERROR, %d\n", __func__, __LINE__, player_status->error);
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_SUBT_SETTING))
            {
                GUI_EndDialog(WND_NETVIDEO_SUBT_SETTING);
            }
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
						_net_video_get_url();
						return;
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
            if(this.play_restart)
            {
                _net_video_play_state_draw(NULL);
            }
            else
            {
                _show_error_info(_show_popup_msg);
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
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_SUBT_SETTING))
            {
                GUI_EndDialog(WND_NETVIDEO_SUBT_SETTING);
            }
            if(this.play_ops.play_ctrl.play_next)
            {
                _netvideo_play_hide_infobar();
                if(this.play_ops.play_ctrl.play_next(0) == GXCORE_SUCCESS)
                {
                    if(this.play_ops.play_ctrl.play_list_total_get)
                    {
                        int total = this.play_ops.play_ctrl.play_list_total_get();
                        if(total > 1)
                        {
                            if(this.play_ops.play_ctrl.play_list_data_get)
                            {
                                this.play_ops.list_focus_item = (this.play_ops.list_focus_item + 1 == total)?0:this.play_ops.list_focus_item + 1;
                                this.play_ops.netvideo_title = this.play_ops.play_ctrl.play_list_data_get(this.play_ops.list_focus_item);
                            }
                            _netvideo_play_show_infobar();
                        }
                    }
                }
                else
                {
                    app_netvideo_play_end_dialog();
                }
            }
            else
            {
                app_netvideo_play_end_dialog();
            }
            break;

		case PLAYER_STATUS_PLAY_RUNNING:
            {
                PlayerStatusInfo player_status;
                const char *widget = NULL;
                if(this.play_restart == 0)
                {
                    widget = GUI_GetFocusWidget();
                    if(widget && strcmp(widget, WND_NETVIDEO_PLAY) == 0)
                        _netvideo_play_show_infobar();
                }
                this.play_restart = 0;
                GxPlayer_MediaGetStatus(PLAYER_FOR_IPTV, &player_status);
                if(player_status.vcodec.state == AVCODEC_RUNNING || player_status.acodec.state == AVCODEC_RUNNING)
                {
                    this.play_state = PLAY_STATE_RUNNING;
                }
                _net_video_playinfo_getforiptv();
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
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_SUBT_SETTING))
                {
                    GUI_EndDialog(WND_NETVIDEO_SUBT_SETTING);
                }
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

static void app_net_video_avcodec_status(GxMessage* msg)
{
	GxMsgProperty_PlayerAVCodecReport *avcodec_status = NULL;
    avcodec_status = (GxMsgProperty_PlayerAVCodecReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);

    if(AVCODEC_ERROR == avcodec_status->acodec.state|| AVCODEC_ERROR == avcodec_status->vcodec.state)
    {

    }
    else if(avcodec_status->vcodec.state == AVCODEC_RUNNING || avcodec_status->acodec.state == AVCODEC_RUNNING)
    {
        PlayerStatusInfo player_status;
        GxPlayer_MediaGetStatus(PLAYER_FOR_IPTV, &player_status);
        if(player_status.status == PLAYER_STATUS_PLAY_RUNNING)
            this.play_state = PLAY_STATE_RUNNING;
        else if(player_status.status == PLAYER_STATUS_START_FILL_BUF)
            this.play_state = PLAY_STATE_LOAD;
        else if(player_status.status == PLAYER_STATUS_PLAY_PAUSE)
            this.play_state = PLAY_STATE_PAUSE;
        else
            this.play_state = PLAY_STATE_RUNNING;
    }
}

int app_netvideo_play_msg_proc(GxMessage *msg)
{
	if(msg == NULL)
		return EVENT_TRANSFER_STOP;

	switch(msg->msg_id)
	{
		case GXMSG_PLAYER_STATUS_REPORT:
			app_net_video_play_status(msg);
			break;

		case GXMSG_PLAYER_AVCODEC_REPORT:
			app_net_video_avcodec_status(msg);
			break;

        case APPMSG_NETVIDEO_PLAY_START:
            _msg_net_video_start_play(msg);
            break;

		default:
		break;
	}

	return EVENT_TRANSFER_STOP;
}

int app_net_video_recovery_status(void)
{
    popdlg_destroy();
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME))
	{
		GUI_EndDialog(WND_VOLUME);
	}
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
    {
        GUI_EndDialog("after wnd_apps_netvideo_play");
        app_netvideo_play_end_dialog();
    }
    return 0;
}

SIGNAL_HANDLER int app_netvideo_play_create(GuiWidget *widget, void *usrdata)
{
    int32_t audio_vol = 0;
    int32_t value = 0;

    app_system_performace_dynamic_change(NET_PLAY_MODE);
    GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
    app_system_vol_set(audio_vol);
    value = pmpset_get_int(PMPSET_AUDIO_TRACK);
    pmpset_set_int(PMPSET_AUDIO_TRACK, value);
    app_netapps_aspect_init();
    app_player_femem_control(true);
    _net_video_play_load_gif();
#if MEDIA_SUBTITLE_SUPPORT
    subtitle_create(PLAYER_FOR_IPTV);
#endif
    if(this.play_ops.play_ctrl.play_create)
    {
        this.play_ops.play_ctrl.play_create();
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_destroy(GuiWidget *widget, void *usrdata)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME))
    {
        GUI_EndDialog(WND_VOLUME);
    }
    _pic_download_stop();
	_netvideo_play_hide_infobar();
	_net_video_state_refresh_stop();
    APP_TIMER_REMOVE(this.play_delay_timer);
    APP_TIMER_REMOVE(this.refresh_timer);
	this.get_subtitle_handle++;
	this.subtitle_status = SUBT_STATE_IDLE;
	APP_TIMER_REMOVE(this.get_subtitle_timer);
	app_netvideo_free_subtitle();
	netvideo_subtitle_download_stop();
	GxPlayer_MediaStop(PLAYER_FOR_IPTV);
    app_netapps_aspect_exit();
    APP_FREE(s_netvideo_info);
#if MEDIA_SUBTITLE_SUPPORT
    app_net_video_subtitle_reset();
    subtitle_destroy();
#endif
#ifdef IPTV_FAV_SUPPORT
    app_iptv_fav_list_dlg_clean();
#endif
	memset(&this.play_ops, 0, sizeof(NetVideoPlayOps));
	_net_video_play_free_gif();
	app_net_video_free_error_str();
	app_netvideo_geturl_timer_stop();
	app_net_video_time_timer_reset();
	this.is_get_url_busy = 0;

    GUI_SetInterface("flush", NULL);
    GDI_StopImageNoClear("*.gif");
    GUI_SetInterface("clear_image", NULL);
    app_player_femem_control(false);
    app_system_performace_dynamic_change(DVB_PLAY_MODE);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_keypress(GuiWidget *widget, void *usrdata)
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
                        if(GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_SUCCESS)
                        {
                            GUI_EndDialog(WND_NETVIDEO_PLAY);
                        }
                        tv_radio_cb();
                    }
                }
                break;

            case STBK_EXIT:
                if(this.play_ops.play_mode != 1)
                {
                    _hide_popup_msg();
                    app_netvideo_play_end_dialog();
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
                        if(GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_SUCCESS)
                        {
                            GUI_EndDialog(WND_NETVIDEO_PLAY);
                        }
                        menu_cb();
                    }
                }
                else
                {
                    app_netvideo_play_end_dialog();
                }
                break;

            case STBK_INFO:
                if(this.play_state == PLAY_STATE_RUNNING || this.play_state == PLAY_STATE_PAUSE || this.play_state == PLAY_STATE_LOAD)
                {
                    _netvideo_play_show_infobar();
                }
                break;
#ifdef IPTV_EPG_SUPPORT
            case STBK_EPG:
                {
                    if(this.play_ops.epg_ctrl.get_epg_days&&this.play_ops.epg_ctrl.get_full_epg
                            && this.play_ops.netvideo_title && this.play_ops.netvideo_url)
                    {
                        app_create_iptv_epg_menu(this.play_ops.netvideo_title, this.play_ops.netvideo_url, &this.play_ops.epg_ctrl);
                    }
                }
                break;
#endif
#ifdef IPTV_FAV_SUPPORT
			case STBK_FAV:
                app_iptv_create_fav_list(&(this.play_ops.list_focus_item));
				break;
#endif
#if MEDIA_SUBTITLE_SUPPORT
            case STBK_SUBT:
                if((this.play_state == PLAY_STATE_RUNNING) && (app_get_netvideo_playinfo() != NULL))
                {
                    _netvideo_play_hide_infobar();
                    if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
                    {
                        GUI_EndDialog(WND_VOLUME);
                    }
                    GUI_CreateDialog(WND_NETVIDEO_SUBT_SETTING);
                }
                break;
#endif
            case STBK_MUTE:
                _net_video_mute_exec();
                _net_video_mute_draw();
                break;

            case STBK_LEFT:
            case STBK_VOLDOWN:
                event->key.sym = APPK_VOLDOWN;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _net_video_mute_draw();
                break;

            case STBK_RIGHT:
            case STBK_VOLUP:
                event->key.sym = APPK_VOLUP;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _net_video_mute_draw();
                break;

            case STBK_UP:
                if(this.play_ops.play_ctrl.play_next)
                {
                    app_net_video_subtitle_reset();
                    _netvideo_play_hide_infobar();
                    this.play_ops.play_ctrl.play_next(1);
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
                            _netvideo_play_show_infobar();
                        }
                    }
                }
                break;

            case STBK_DOWN:
                if(this.play_ops.play_ctrl.play_prev)
                {
                    app_net_video_subtitle_reset();
                    _netvideo_play_hide_infobar();
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

                            _netvideo_play_show_infobar();
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
#ifdef IPTV_FAV_SUPPORT
                if(app_iptv_fav_list_dlg_check())
                {
                    app_iptv_create_fav_list(&(this.play_ops.list_focus_item));
                    break;
                }
#endif
                if(this.play_ops.play_ctrl.play_list_total_get != NULL && this.play_ops.play_ctrl.play_list_total_get() > 0)
                {
                    _hide_popup_msg();
                    app_net_video_list_create_dialog();
                }
                else if(this.play_state == PLAY_STATE_RUNNING || this.play_state == PLAY_STATE_PAUSE || this.play_state == PLAY_STATE_LOAD)
                {
                    if(app_netapps_bandwidth_get_num()>0)
                    {
                    	app_netapps_bandwidth_create_dialog();
                    }
                    else
                    	_netvideo_play_show_infobar();
                }
                else
                {
                    _netvideo_play_hide_infobar();
                    if(this.play_ops.play_ctrl.play_ok != NULL)
                        this.play_ops.play_ctrl.play_ok();
                }
                break;
			case STBK_1:
			case STBK_2:
			case STBK_3:
			case STBK_4:
			case STBK_5:
			case STBK_6:
			case STBK_7:
			case STBK_8:
			case STBK_9:
			case STBK_0:
                if(this.play_ops.play_ctrl.play_list_total_get != NULL && this.play_ops.play_ctrl.play_list_total_get() > 1)
                {
                    _hide_popup_msg();
                    app_netvideo_play_number_call(&this.play_ops);
                    GUI_SendEvent(WND_NETVIDEO_PLAY_NUMBER, event);
                }
                break;
            case STBK_AUDIO:
                {
                    if((this.play_state == PLAY_STATE_RUNNING) && (app_get_netvideo_playinfo() != NULL))
                    {
                        if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
                        {
                            GUI_EndDialog(WND_VOLUME);
                        }
                        extern int app_netvideo_audio_create_dialog(void);
                        app_netvideo_audio_create_dialog();
                    }
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
    return ret;
}

SIGNAL_HANDLER int app_netvideo_play_got_focus(GuiWidget *widget, void *usrdata)
{
    this.play_state_bak = PLAY_STATE_NONE;
    _net_video_state_refresh_start();
    app_net_video_subtitle_resume();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_lost_focus(GuiWidget *widget, void *usrdata)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK) || GXCORE_SUCCESS == GUI_CheckDialog(IPTV_EPG_MENU))
    {
        _hide_popup_msg();
    }
    app_net_video_subtitle_pause();
	return EVENT_TRANSFER_STOP;
}

void app_net_video_play_halt(void)
{
	_net_video_state_refresh_stop();
	app_net_video_stop_play();
}

void app_net_video_play_resume(void)
{
    _net_video_state_refresh_start();
	this.play_state_bak  = PLAY_STATE_LOAD;
    app_netvideo_geturl_timer_start();
}

#endif

#if VOICE_CONTROL_SUPPORT
// params: 0-stop, 1-pause; 2-play
// return: 0-support, 1-not support, 2-repeat, -1-failed
int app_vc_netvideo_play_flag(int flag)
{
#if NETAPPS_SUPPORT
    if((flag < 0) || (flag > 2))
        return -1;

    if(0 == flag)// stop
    {
        return 0;
    }
    else if(1 == flag)
    {
        if(this.play_state == PLAY_STATE_NONE || this.play_state == PLAY_STATE_ERROR || this.play_state == PLAY_STATE_LOAD)
            return 1;
        else if(this.play_state == PLAY_STATE_PAUSE)
            return 2;
        else
            return 0;
    }
    else
    {
        if(this.play_state == PLAY_STATE_NONE || this.play_state == PLAY_STATE_ERROR || this.play_state == PLAY_STATE_LOAD)
            return 1;
        else if(this.play_state == PLAY_STATE_RUNNING)
            return 2;
        else
            return 0;
    }
#endif
    return 0;
}

int app_vc_net_number_switch(unsigned int num, char* name, unsigned int name_len)
{
    int ret = 0;
#if NETAPPS_SUPPORT
    unsigned int len = 0;
#if VOICE_CONTROL_SUPPORT
    extern int app_vc_net_number_switch_url(NetVideoPlayOps* playops, unsigned int num);
    ret = app_vc_net_number_switch_url(&this.play_ops, num);
#endif
    if(GXCORE_SUCCESS == ret)
    {
        len = strlen(this.play_ops.netvideo_title);
        len = (name_len > len)?len:name_len;
        memcpy(name, this.play_ops.netvideo_title, len);
    }
#endif
    return ret;
}

#endif

