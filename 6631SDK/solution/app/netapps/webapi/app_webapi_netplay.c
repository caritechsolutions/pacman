#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_default_params.h"
#include "app_wnd_system_setting_opt.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include "full_screen.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"
#include "netapps/app_netapps_common.h"
#include "app_volume_value.h"

#ifdef ECOS_OS
#define WEBAPI_IMAGE_PATH "/mnt/webapi.jpg"
#define WEBAPI_MUSIC_BG "bg"
#else
#define WEBAPI_IMAGE_PATH "/tmp/webapi.jpg"
#define WEBAPI_MUSIC_BG "music_back"
#endif

#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#define IMG_GIF                 "img_webapi_netplay_gif"
#define IMG_INFOBAR_BACK    "img_webapi_netplay_barback"
#define SLIDER_CURSOR    "sliderbar_cursor_webapi_netplay"
#define SLIDER_PROCESS    "sliderbar_process_netplay_barback"
#define TEXT_START_TIME    "text_webapi_netplay_start_time"
#define TEXT_STOP_TIME    "text_webapi_netplay_stop_time"
#define IMG_MUTE              "img_webapi_netplay_mute"
#define TXT_POPUP    "txt_webapi_netplay_popup"
#define IMG_POPUP    "img_webapi_netplay_popup"
#define IMG_PAUSE    "img_webapi_netplay_pause"
#define WEBAPI_NETPLAY_INFO_SHOW_SEC (7)

#define WEBAPI_PLAYER_AV   "player_webapi_av"
#define WEBAPI_PLAYER_PIC  "player_webapi_pic"

#define TEXT_VIDEO_PLAY_NETSPEED "text_webapi_play_netspeed"
#define CACHE_SLIDER_PROCESS    "sliderbar_webapi_play_cache"
static enum
{
	VIDEO_SEEK_NONE,
	VIDEO_SEEKING,
	VIDEO_SEEKED
}s_seek_flag = VIDEO_SEEK_NONE;

typedef enum _WebapiPlayState
{
    PLAY_STATE_NONE,
    PLAY_STATE_LOAD,
    PLAY_STATE_RUNNING,
    PLAY_STATE_ERROR,
    PLAY_STATE_PAUSE
}WebapiPlayState;

typedef enum _WebapiAudioType
{
    WEBAPI_AUDIO_MPEG1,
    WEBAPI_AUDIO_MPEG2,
    WEBAPI_AUDIO_AAC_LATM,
    WEBAPI_AUDIO_AAC_ADTS,
    WEBAPI_AUDIO_OGG,
    WEBAPI_AUDIO_AC3,
    WEBAPI_AUDIO_AVS,
    WEBAPI_AUDIO_PCM,
    WEBAPI_AUDIO_EAC3,
    WEBAPI_AUDIO_DTS,
    WEBAPI_AUDIO_DRA1,
    WEBAPI_AUDIO_DTS_HD,
    WEBAPI_AUDIO_REAL,
    WEBAPI_AUDIO_RA_AAC,
    WEBAPI_AUDIO_RA_RA8LBR,
    WEBAPI_AUDIO_UNKNOWN,
}WebapiAudioType;

typedef enum _WebapiVideoType
{
    WEBAPI_VIDEO_MPEG2,
    WEBAPI_VIDEO_MPEG4,
    WEBAPI_VIDEO_H263,
    WEBAPI_VIDEO_H264,
    WEBAPI_VIDEO_REAL,
    WEBAPI_VIDEO_AVS,
    WEBAPI_VIDEO_H265,
    WEBAPI_VIDEO_VC1,
    WEBAPI_VIDEO_MJPEG,
    WEBAPI_VIDEO_DIV3,
    WEBAPI_VIDEO_UNKNOWN,
}WebapiVideoType;

static void webapi_show_popup_msg(char* msg);

static event_list* s_webapi_picupdate_timer = NULL;
static event_list *s_webapi_time_timer = NULL;
static event_list *s_webapi_state_refresh_timer = NULL;
static event_list *s_webapi_inforbar_refresh_timer = NULL;
static event_list *s_webapi_sliderbar_seek_timer = NULL;
static event_list *s_webapi_netplay_gif_timer = NULL;
static int s_webapi_infobar_timer_cnt = 0;
static int s_webapi_infobar_show_flag = 0;
static int s_webapi_total_video_dur = 0;
static uint64_t s_webapi_cur_time_ms_bak = 0;
static int s_webapi_cur_sliderbar_bak = 0;
static int s_webapi_paused = 0;

static WebapiMediaType media_type = WEBAPI_MEDIA_UNKNOWN;
static int webapi_ui_exits = 0;
static unsigned int webapi_download_pic_handle = 0;
static int webapi_pic_download_abort = 0;
static int webapi_pic_download_ok = 0;
static int webapi_pic_show_ok = 0;
static WebapiPlayState webapi_play_state = PLAY_STATE_NONE;
static WebapiPlayState webapi_play_state_bak = PLAY_STATE_NONE;

static char *webapi_audio_type[] =
{
	"MPEG1", //WEBAPI_AUDIO_MPEG1
	"MPEG2", //WEBAPI_AUDIO_MPEG2
	"AAC_LATM", //WEBAPI_AUDIO_AAC_LATM
	"AAC_ADTS",  //WEBAPI_AUDIO_AAC_ADTS
	"OGG ", //WEBAPI_AUDIO_OGG
	"AC3", //WEBAPI_AUDIO_AC3
	"AVS", //WEBAPI_AUDIO_AVS
	"PCM", //WEBAPI_AUDIO_PCM
	"EAC3", //WEBAPI_AUDIO_EAC3
	"DTS", //WEBAPI_AUDIO_DTS
	"DRA1", //WEBAPI_AUDIO_DRA1
	"DTS_HD", //WEBAPI_AUDIO_DTS_HD
	"REAL", //WEBAPI_AUDIO_REAL
	"RA_AAC", //WEBAPI_AUDIO_RA_AAC
	"RA_RA8LBR", //WEBAPI_AUDIO_RA_RA8LBR
	"UNKNOWN", //WEBAPI_AUDIO_UNKNOWN
};

static char *webapi_video_type[] =
{
	"MPEG2", //WEBAPI_VIDEO_MPEG2
	"MPEG4", //WEBAPI_VIDEO_MPEG4
	"H263", //WEBAPI_VIDEO_H263
	"H264", //WEBAPI_VIDEO_H264
	"REAL", //WEBAPI_VIDEO_REAL
	"AVS", //WEBAPI_VIDEO_AVS
	"H265", //WEBAPI_VIDEO_H265
	"VC1", //WEBAPI_VIDEO_VC1
	"MJPEG", //WEBAPI_VIDEO_MJPEG
	"DIV3", //WEBAPI_VIDEO_DIV3
	"UNKNOWN", //WEBAPI_AUDIO_UNKNOWN
};

static int _webapi_netplay_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;

	GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
	return 0;
}

static void _webapi_netplay_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;

	GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
	GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _webapi_netplay_free_gif(void)
{
    APP_TIMER_REMOVE(s_webapi_netplay_gif_timer);
	s_webapi_netplay_gif_timer = NULL;
	GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _webapi_netplay_show_gif(void)
{
	GUI_SetProperty(IMG_GIF, "state", "show");
	APP_TIMER_ADD(s_webapi_netplay_gif_timer, _webapi_netplay_draw_gif, 100, TIMER_REPEAT);
}

static void _webapi_netplay_hide_gif(void)
{
	APP_TIMER_REMOVE(s_webapi_netplay_gif_timer);
	GUI_SetProperty(IMG_GIF, "state", "hide");
}

static int _webapi_play_netspeed_show(void)
{
	char buf_tmp[16] = {0};
	PlayerNetInfo netInfo = {0};

    if(GXCORE_SUCCESS == GxPlayer_MediaGetNetInfo(WEBAPI_PLAYER_AV, &netInfo))
    {
        if(webapi_play_state == PLAY_STATE_LOAD || (webapi_play_state == PLAY_STATE_PAUSE && netInfo.eof != 1))
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
        if(webapi_play_state == PLAY_STATE_LOAD || webapi_play_state == PLAY_STATE_PAUSE)
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

static int _webapi_play_netspeed_hide(void)
{
    GUI_SetProperty(CACHE_SLIDER_PROCESS, "state", "hide");
    GUI_SetProperty(TEXT_VIDEO_PLAY_NETSPEED, "state", "hide");
    return 0;
}

static WebapiAudioType webapi_get_audio_type(int codec_type)
{
	WebapiAudioType audio_type = WEBAPI_AUDIO_UNKNOWN;

	switch(codec_type)
	{
		case AUDIO_CODEC_MPEG1:
			audio_type = WEBAPI_AUDIO_MPEG1;
			break;
		case AUDIO_CODEC_MPEG2:
			audio_type = WEBAPI_AUDIO_MPEG2;
			break;
		case AUDIO_CODEC_AAC_LATM:
			audio_type = WEBAPI_AUDIO_AAC_LATM;
			break;
		case AUDIO_CODEC_AAC_ADTS:
			audio_type = WEBAPI_AUDIO_AAC_ADTS;
			break;
		case AUDIO_CODEC_VORBIS:
			audio_type = WEBAPI_AUDIO_OGG;
			break;
		case AUDIO_CODEC_AC3:
			audio_type = WEBAPI_AUDIO_AC3;
			break;
		case AUDIO_CODEC_AVS:
			audio_type = WEBAPI_AUDIO_AVS;
			break;
		case AUDIO_CODEC_PCM:
			audio_type = WEBAPI_AUDIO_PCM;
			break;
		case AUDIO_CODEC_EAC3:
			audio_type = WEBAPI_AUDIO_EAC3;
			break;
		case AUDIO_CODEC_DTS:
			audio_type = WEBAPI_AUDIO_DTS;
			break;
		case AUDIO_CODEC_DRA1:
			audio_type = WEBAPI_AUDIO_DRA1;
			break;
		case AUDIO_CODEC_DTS_HD:
			audio_type = WEBAPI_AUDIO_DTS_HD;
			break;
		case AUDIO_CODEC_REAL:
			audio_type = WEBAPI_AUDIO_REAL;
			break;
		case AUDIO_CODEC_RA_AAC:
			audio_type = WEBAPI_AUDIO_RA_AAC;
			break;
		case AUDIO_CODEC_RA_RA8LBR:
			audio_type = WEBAPI_AUDIO_RA_RA8LBR;
			break;
		default:
			audio_type = WEBAPI_AUDIO_UNKNOWN;
			break;
	}

	return audio_type;
}

static WebapiVideoType webapi_get_video_type(int codec_type)
{
	WebapiVideoType video_type = WEBAPI_VIDEO_UNKNOWN;

	switch(codec_type)
	{
		case VIDEO_CODEC_MPEG12:
			video_type = WEBAPI_VIDEO_MPEG2;
			break;
		case VIDEO_CODEC_MPEG4:
			video_type = WEBAPI_VIDEO_MPEG4;
			break;
		case VIDEO_CODEC_H263:
			video_type = WEBAPI_VIDEO_H263;
			break;
		case VIDEO_CODEC_H264:
			video_type = WEBAPI_VIDEO_H264;
			break;
		case VIDEO_CODEC_REAL:
			video_type = WEBAPI_VIDEO_REAL;
			break;
		case VIDEO_CODEC_AVS:
			video_type = WEBAPI_VIDEO_AVS;
			break;
		case VIDEO_CODEC_H265:
			video_type = WEBAPI_VIDEO_H265;
			break;
		case VIDEO_CODEC_VC1:
			video_type = WEBAPI_VIDEO_VC1;
			break;
		case VIDEO_CODEC_MJPEG:
			video_type = WEBAPI_VIDEO_MJPEG;
			break;
		case VIDEO_CODEC_DIV3:
			video_type = WEBAPI_VIDEO_DIV3;
			break;
		default:
			video_type = WEBAPI_VIDEO_UNKNOWN;
			break;
	}

	return video_type;
}

static void webapi_netplay_prepare(WebapiMediaType type)
{
	if(type == WEBAPI_MEDIA_VIDEO)
	{
		WEBAPI_PRINTF("%s[%d] video\n",__FUNCTION__, __LINE__);
		GUI_SetProperty(WND_WEBAPI_NETPLAY, "back_ground", "null");
		GUI_SetInterface("draw_now", NULL);
		GUI_SetInterface("video_enable", NULL);
	}
	else if(type == WEBAPI_MEDIA_AUDIO)
	{
		WEBAPI_PRINTF("%s[%d] music\n",__FUNCTION__, __LINE__);
		GUI_SetProperty(WND_WEBAPI_NETPLAY, "back_ground", WEBAPI_MUSIC_BG);
		GUI_SetInterface("flush", NULL);
		GUI_SetInterface("video_disable", NULL);
	}
}

static void webapi_net_player_play(const char* url, WebapiMediaType type, uint32_t start)
{
	GxMsgProperty_PlayerPlay* player_play = NULL;

	if((NULL == url)||(type == WEBAPI_MEDIA_UNKNOWN)||(type == WEBAPI_MEDIA_PICTURE))
	{
		return;
	}
    player_play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
    if(player_play == NULL)
    {
        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
        return;
    }
	GxPlayer_MediaStop(WEBAPI_PLAYER_AV);
	webapi_netplay_prepare(type);
	strcpy(player_play->url, url);
	player_play->player = WEBAPI_PLAYER_AV;
	player_play->start = 1000*start;

	WEBAPI_PRINTF("%s[%d]%s,time=%lld\n",__FUNCTION__, __LINE__, player_play->url, player_play->start);
	app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)player_play);
    GXCORE_FREE(player_play);
}

static void webapi_net_player_stop(void)
{
	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	GxPlayer_MediaStop(WEBAPI_PLAYER_AV);
}

static void webapi_net_player_pause(void)
{
	GxMsgProperty_PlayerPause player_play = {0};

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	player_play.player = WEBAPI_PLAYER_AV;
	app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_play));
}

static void webapi_net_player_resume(void)
{
	GxMsgProperty_PlayerResume player_play = {0};

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	player_play.player = WEBAPI_PLAYER_AV;
	app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_play));
}

static void webapi_net_player_seek(int64_t time_ms)
{
	GxMsgProperty_PlayerSeek seek;

	WEBAPI_PRINTF("%s[%d]:%lld ms\n",__FUNCTION__, __LINE__, time_ms);
	memset(&seek, 0, sizeof(GxMsgProperty_PlayerSeek));
	seek.player = WEBAPI_PLAYER_AV;
	seek.time = time_ms;
	seek.flag = SEEK_ORIGIN_SET;
	app_send_msg_exec(GXMSG_PLAYER_SEEK, &seek);
}

static void webapi_net_pic_player_preplay(void)
{
	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	GUI_SetInterface("clear_image", NULL);
	GUI_SetProperty(WND_WEBAPI_NETPLAY, "back_ground", "null");
	GUI_SetInterface("flush", NULL);
	GUI_SetInterface("video_disable", NULL);
}

static void webapi_net_pic_player_play(const char* url)
{
	int ret = GXCORE_SUCCESS;
	WEBAPI_PRINTF("%s[%d]:%s\n",__FUNCTION__, __LINE__, url);
	ret = GDI_PlayImage(url,-1,-1);
	if(ret != GXCORE_SUCCESS && GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS)
	{
		webapi_show_popup_msg(STR_ID_PLAY_ERROR);
	}
}

static void webapi_show_popup_msg(char* msg)
{
	GUI_SetProperty(TXT_POPUP, "string", msg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");
}

static void webapi_hide_popup_msg(void)
{
	GUI_SetProperty(IMG_POPUP, "state", "hide");
	GUI_SetProperty(TXT_POPUP, "state", "hide");
}

static int app_webapi_pic_update_timer_timeout(void *userdata)
{
	if(webapi_pic_show_ok == 0)
	{
		if(webapi_pic_download_ok == 1)
		{
			if(GXCORE_FILE_UNEXIST != GxCore_FileExists(WEBAPI_IMAGE_PATH))
			{
				_webapi_netplay_hide_gif();
                GUI_SetInterface("flush",NULL);
				webapi_net_pic_player_play(WEBAPI_IMAGE_PATH);
			}
			webapi_pic_show_ok = 1;
		}
		else if(webapi_pic_download_ok == -1)
		{
			_webapi_netplay_hide_gif();
			webapi_show_popup_msg(STR_ID_PLAY_ERROR);
			webapi_pic_show_ok = 1;
		}
	}

	if(webapi_pic_show_ok == 1)
	{
		GUI_SetInterface("flush",NULL);
		timer_stop(s_webapi_picupdate_timer);
	}

	return 0;
}

static void app_webapi_pic_update_timer_stop(void)
{
	if(s_webapi_picupdate_timer)
	{
		remove_timer(s_webapi_picupdate_timer);
		s_webapi_picupdate_timer = NULL;
	}
}

static void app_webapi_pic_update_timer_reset(void)
{
	if(reset_timer(s_webapi_picupdate_timer) != 0)
	{
		s_webapi_picupdate_timer = create_timer(app_webapi_pic_update_timer_timeout, 300, NULL, TIMER_REPEAT);
	}
}

static void app_webapi_download_pic_cb(int message, int body)
{
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

	if(message == GXCURL_STATE_COMPLETE)
	{
		if(webapi_pic_download_abort)
		{
			return;
		}
		if(callback_data->handle>0 && webapi_download_pic_handle == callback_data->handle)
		{
            if(callback_data->complete_data.size > 0)
            {
                if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
                {
                    WEBAPI_PRINTF("%s[%d]success\n",__FUNCTION__, __LINE__);
                    webapi_pic_download_ok = 1;
                }
                else
                {
                    WEBAPI_PRINTF("%s[%d]failed\n",__FUNCTION__, __LINE__);
                    webapi_pic_download_ok = -1;
                }
            }
            else
            {
                WEBAPI_PRINTF("%s[%d]failed\n",__FUNCTION__, __LINE__);
                webapi_pic_download_ok = -1;
            }
		}
	}
}

static void app_webapi_delete_pic_files(void)
{
	if(GXCORE_FILE_EXIST == GxCore_FileExists(WEBAPI_IMAGE_PATH))
	{
		GxCore_FileDelete(WEBAPI_IMAGE_PATH);
	}
}

static void app_webapi_pic_download_stop(void)
{
	app_webapi_pic_update_timer_stop();
	webapi_pic_download_ok = 0;
	webapi_pic_show_ok = 0;
	webapi_pic_download_abort = 1;
	if(webapi_download_pic_handle!=0)
	{
		gxapps_curl_async_abort(webapi_download_pic_handle);
		webapi_download_pic_handle = 0;
	}
	app_webapi_delete_pic_files();
	webapi_pic_download_abort= 0;
}

static void app_webapi_pic_download_start(char *url)
{
	WEBAPI_PRINTF("%s[%d]%s\n",__FUNCTION__, __LINE__, url);
	app_webapi_pic_update_timer_reset();
	if(webapi_pic_download_abort == 0)
	{
		webapi_download_pic_handle = gxapps_curl_async_download(url, WEBAPI_IMAGE_PATH, app_webapi_download_pic_cb);
	}
}

static int app_webapi_play_state_draw(void *usrdata)
{
	const char *wnd = NULL;
	static char *wnd_bak = NULL;
	wnd = GUI_GetFocusWindow();

	if((media_type == WEBAPI_MEDIA_UNKNOWN)||(media_type == WEBAPI_MEDIA_PICTURE))
	{
		return 0;
	}

	if(NULL != wnd_bak && 0 == strcasecmp(wnd_bak, wnd))
	{
		if(webapi_play_state == webapi_play_state_bak)
		{
			return 0;
		}
	}

	webapi_play_state_bak = webapi_play_state;
	APP_FREE(wnd_bak);
	int len = strlen(wnd);
	wnd_bak =  (char*)GxCore_Malloc(len + 1);
	memset(wnd_bak, 0, len + 1);
	memcpy(wnd_bak, wnd, len);

	if(webapi_play_state == PLAY_STATE_LOAD)
	{
		if(GXCORE_SUCCESS != GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
			_webapi_play_netspeed_show();
		webapi_hide_popup_msg();
		_webapi_netplay_show_gif();
	}
    else if(webapi_play_state == PLAY_STATE_PAUSE)
    {
		if(GXCORE_SUCCESS != GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
			_webapi_play_netspeed_show();

        GUI_SetProperty(IMG_PAUSE, "state", "show");
		_webapi_netplay_hide_gif();
    }
	else
	{
		_webapi_play_netspeed_hide();
		if(0 == strcasecmp(WND_WEBAPI_NETPLAY, wnd))
		{
			if(webapi_play_state == PLAY_STATE_ERROR)
			{
				_webapi_netplay_hide_gif();
				webapi_show_popup_msg(STR_ID_PLAY_ERROR);
			}
			else if(webapi_play_state == PLAY_STATE_NONE)
			{
				_webapi_netplay_hide_gif();
				webapi_show_popup_msg(STR_ID_PLAY_END);
			}
			else
			{
				_webapi_netplay_hide_gif();
				webapi_hide_popup_msg();
			}
		}
	}

	return 0;
}

static void app_webapi_state_refresh_start(void)
{
	if(0 != reset_timer(s_webapi_state_refresh_timer))
	{
		s_webapi_state_refresh_timer = create_timer(app_webapi_play_state_draw, 300, NULL, TIMER_REPEAT);
	}
}

static void app_webapi_state_refresh_stop(void)
{
	webapi_hide_popup_msg();
	_webapi_netplay_hide_gif();
	_webapi_play_netspeed_hide();
	timer_stop(s_webapi_state_refresh_timer);
	webapi_play_state_bak = PLAY_STATE_NONE;
}

static int app_webapi_update_time(void *userdata)
{
	uint64_t cur_time_ms = 0;
	uint64_t total_time_ms = 0;
	int cur_sliderbar = 0;
	status_t ret = GXCORE_ERROR;
	PlayTimeInfo  tinfo = {0};

	if(webapi_play_state == PLAY_STATE_RUNNING)
	{
		if((ret=GxPlayer_MediaGetTime(WEBAPI_PLAYER_AV,  &tinfo)) == GXCORE_SUCCESS)
		{
			cur_time_ms = tinfo.current;
			total_time_ms = tinfo.totle;
			s_webapi_total_video_dur = total_time_ms;
			if(s_webapi_total_video_dur < 0)
			{
				s_webapi_total_video_dur = 0;
			}
			else if(s_webapi_total_video_dur > 0)
			{
				if(cur_time_ms > s_webapi_total_video_dur)
				{
					cur_time_ms = s_webapi_total_video_dur;
					cur_sliderbar = 100;
				}
				else
				{
					cur_sliderbar = (cur_time_ms * 100)/ s_webapi_total_video_dur ;
				}
			}
		}
		else
		{
			return 0;
		}
		s_webapi_cur_time_ms_bak = cur_time_ms;
		s_webapi_cur_sliderbar_bak = cur_sliderbar;
	}

	return 0;
}

static void app_webapi_update_infobar(void)
{
	char buf_tmp[20] = {0};

	if(s_seek_flag != VIDEO_SEEKING)
	{
		GUI_SetProperty(SLIDER_PROCESS, "value", &s_webapi_cur_sliderbar_bak);
		if(s_seek_flag == VIDEO_SEEK_NONE)
		{
			GUI_SetProperty(SLIDER_CURSOR, "value", &s_webapi_cur_sliderbar_bak);
		}
		else
		{
			int seeked_val = 0;
			GUI_GetProperty(SLIDER_CURSOR, "value", &seeked_val);
			GUI_SetProperty(SLIDER_CURSOR, "value", &seeked_val);
		}
		/*cur time*/
		if((s_webapi_total_video_dur>0)||(s_webapi_cur_time_ms_bak>0))
		{
            app_time_ms_to_edit_str(s_webapi_cur_time_ms_bak,buf_tmp,20);
		}
		else
		{
			sprintf(buf_tmp, "  ");
		}
		GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
	}

	/*total time*/
    app_time_ms_to_edit_str(s_webapi_total_video_dur,buf_tmp,20);
	GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);
}

static void app_webapi_infobar_timer_stop(void)
{
	APP_TIMER_REMOVE(s_webapi_inforbar_refresh_timer);
	s_webapi_infobar_timer_cnt = 0;
}

static void app_webapi_hide_infobar(void)
{
	app_webapi_infobar_timer_stop();

	GUI_SetProperty(IMG_INFOBAR_BACK, "state", "hide");
	GUI_SetProperty(SLIDER_PROCESS, "state", "hide");
	GUI_SetProperty(SLIDER_CURSOR, "state", "hide");
	GUI_SetProperty(TEXT_START_TIME, "state", "hide");
	GUI_SetProperty(TEXT_STOP_TIME, "state", "hide");
	GUI_SetProperty(SLIDER_CURSOR, "state", "hide");

	APP_TIMER_REMOVE(s_webapi_sliderbar_seek_timer);
	s_seek_flag = VIDEO_SEEK_NONE;
	s_webapi_infobar_show_flag = 0;
}

static int app_webapi_infobar_timer_cb(void *userdata)
{
	s_webapi_infobar_timer_cnt++;
	if(s_webapi_infobar_timer_cnt == WEBAPI_NETPLAY_INFO_SHOW_SEC)
	{
		app_webapi_hide_infobar();
	}
	else
	{
		app_webapi_update_infobar();
	}

	return 0;
}

static void app_webapi_infobar_timer_reset(void)
{
	s_webapi_infobar_timer_cnt = 0;
	if(reset_timer(s_webapi_inforbar_refresh_timer) != 0)
	{
		s_webapi_inforbar_refresh_timer = create_timer(app_webapi_infobar_timer_cb, 1000, NULL, TIMER_REPEAT);
	}
}

static void app_webapi_show_infobar(void)
{
	GUI_SetProperty(IMG_INFOBAR_BACK, "state", "show");
	GUI_SetProperty(SLIDER_PROCESS, "state", "show");
	GUI_SetProperty(TEXT_START_TIME, "state", "show");
	GUI_SetProperty(TEXT_STOP_TIME, "state", "show");
	GUI_SetProperty(SLIDER_PROCESS, "state", "show");
	GUI_SetProperty(SLIDER_CURSOR, "state", "show");
	GUI_SetFocusWidget(SLIDER_CURSOR);
	app_webapi_update_infobar();
	app_webapi_infobar_timer_reset();

	s_webapi_infobar_show_flag = 1;
}

void app_webapi_netplay_message_send(int action, char *url, WebapiMediaType type, uint32_t time)
{
	AppMsg_WebapiUIControl param = {0};
	WebapiNetplayParam *play_param = NULL;

	WEBAPI_PRINTF("%s[%d][%d/%d/%u]%s\n",__FUNCTION__, __LINE__, action, type, time, url);
	play_param = GxCore_Malloc(sizeof(WebapiNetplayParam));
	memset(play_param, 0, sizeof(WebapiNetplayParam));
	play_param->action = action;
	play_param->media_type = type;
	if(url)
	{
		play_param->url = GxCore_Strdup(url);
	}
	play_param->time = time;

	memset(&param, 0, sizeof(AppMsg_WebapiUIControl));
	param.type = WEBAPI_MSG_NETPLAY;
	param.result = (int)play_param;
	app_send_msg_exec(APPMSG_WEBAPI_UI_CONTROL, &param);
}

int app_webapi_netplay_exist(void)
{
	return webapi_ui_exits;
}

static void app_webapi_netplay_free_param(WebapiNetplayParam *param)
{
	if(param)
	{
		APP_FREE(param->url);
		APP_FREE(param);
	}
}

static void app_webapi_netplay_dvb_stop(void)
{
	g_AppFullArb.timer_stop();
	g_AppPlayOps.program_stop();
}

static void app_webapi_netplay_dvb_resume(void)
{
	g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	if(g_AppPlayOps.normal_play.play_total != 0)
	{
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
		g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
		g_AppFullArb.state.pause = STATE_OFF;
		g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
		if(g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
			g_AppFullArb.tip = FULL_STATE_LOCKED;
		else
			g_AppFullArb.tip = FULL_STATE_DUMMY;
	}
}
static void app_webapi_msg_play_func(char* url, WebapiMediaType type, uint32_t start)
{
	if(g_AppPvrOps.state != PVR_DUMMY)
	{
		PopDlg pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.format = POP_FORMAT_DLG;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_STOP_PVR_FIRST;
		pop.timeout_sec = 3;
		popdlg_create(&pop);
		return;
	}

	if((!url)||(type == WEBAPI_MEDIA_UNKNOWN))
	{
		return;
	}

	WEBAPI_PRINTF("%s[%d][%d/%u]%s\n",__FUNCTION__, __LINE__, type, start, url);
	if(GUI_CheckDialog(WND_WEBAPI_NETPLAY) == GXCORE_ERROR)
	{
		if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SYSTEM_SETTING))
			app_system_reset_unfocus_image();
		GUI_EndDialog("after wnd_full_screen");
		g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt,STBK_EXIT);
		app_webapi_netplay_dvb_stop();
		GUI_CreateDialog(WND_WEBAPI_NETPLAY);
	}
    else
    {
        GUI_SetProperty(IMG_PAUSE, "state", "hide");
    }

	media_type = type;
	s_webapi_cur_time_ms_bak = 0;
	s_webapi_cur_sliderbar_bak = 0;
	s_webapi_total_video_dur = 0;
	s_webapi_paused = 0;
	app_webapi_infobar_timer_stop();
	app_webapi_pic_download_stop();
	if(type == WEBAPI_MEDIA_PICTURE)
	{
		webapi_play_state = PLAY_STATE_NONE;
		app_webapi_hide_infobar();
		app_webapi_state_refresh_stop();
		webapi_net_player_stop();
		GDI_StopImageNoClear("xxx.gif");
		webapi_net_pic_player_preplay();
		if(url[0] == '/')
		{
			webapi_net_pic_player_play(url);
		}
		else
		{
			_webapi_netplay_show_gif();
			app_webapi_pic_download_start(url);
		}
	}
	else
	{
		webapi_play_state = PLAY_STATE_LOAD;
		app_webapi_state_refresh_start();
		webapi_net_player_play(url, type, start);
		app_webapi_show_infobar();
	}
}

static void app_webapi_msg_stop_func(void)
{
	if(webapi_ui_exits>0)
	{
        GUI_EndDialog(WND_VOLUME);
        GUI_SetProperty(IMG_PAUSE, "state", "show");
        GUI_SetProperty(IMG_PAUSE, "img", "s_pvr_stop");
		webapi_play_state = PLAY_STATE_NONE;
		if(media_type == WEBAPI_MEDIA_PICTURE)
			GDI_StopImageNoClear("xxx.gif");
		media_type = WEBAPI_MEDIA_UNKNOWN;
		s_webapi_cur_time_ms_bak = 0;
		s_webapi_cur_sliderbar_bak = 0;
		s_webapi_total_video_dur = 0;
		s_webapi_paused = 0;
		app_webapi_state_refresh_stop();
		webapi_net_player_stop();
	}

}

static void app_webapi_msg_pause_func(void)
{
	if((webapi_ui_exits>0)&&((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
		&&(s_webapi_total_video_dur>0)&&(s_webapi_cur_time_ms_bak>0))
	{
		if(s_webapi_paused>0)
		{
            GUI_SetProperty(IMG_PAUSE, "state", "hide");
            webapi_net_player_resume();
			s_webapi_paused = 0;
		}
		else
		{
            GUI_SetProperty(IMG_PAUSE, "state", "show");
            GUI_SetProperty(IMG_PAUSE, "img", "s_pause");
			webapi_net_player_pause();
			s_webapi_paused = 1;
		}
	}
}

static void app_webapi_msg_seek_func(uint32_t time_s)
{
	if((webapi_ui_exits>0)&&((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
		&&(time_s>=0)&&(s_webapi_total_video_dur>0)&&(time_s*1000<=s_webapi_total_video_dur))
	{
        GUI_SetProperty(IMG_PAUSE, "state", "hide");
		webapi_net_player_seek(time_s*1000);
        s_webapi_paused = 0;
	}
}

static void app_webapi_netplay_control(WebapiNetplayParam *param)
{
	switch(param->action)
	{
		case WEBAPI_PLAY_ACTION_START:
			app_webapi_msg_play_func(param->url, param->media_type, param->time);
			break;

		case WEBAPI_PLAY_ACTION_STOP:
			app_webapi_msg_stop_func();
			break;

		case WEBAPI_PLAY_ACTION_PAUSE:
			app_webapi_msg_pause_func();
			break;

		case WEBAPI_PLAY_ACTION_SEEK:
			app_webapi_msg_seek_func(param->time);
			break;

		default:
			break;
	}
}

void app_webapi_set_net_play(int param)
{
	WebapiNetplayParam *netplay_param = (WebapiNetplayParam *)param;

	if(netplay_param)
	{
		app_webapi_netplay_control(netplay_param);
		app_webapi_netplay_free_param(netplay_param);
	}
}

char *app_webapi_netplay_get_play_data(void)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *out = NULL;
	PlayerProgInfo *info = NULL;
	char buffer[64];
	char *status = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if((webapi_ui_exits>0)&&((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO)))
	{
		root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
		json1 = cJSON_CreateObject();
		cJSON_AddItemToObject(root, WEBAPI_PLAYINFO_STR, json1);

		if((webapi_play_state == PLAY_STATE_LOAD)||(webapi_play_state == PLAY_STATE_RUNNING))
		{
			if(s_webapi_paused>0)
			{
				status = "pause";
			}
			else
			{
				status = "play";
			}
		}
		else if(webapi_play_state == PLAY_STATE_ERROR)
		{
			status = "error";
		}
		else
		{
			status = "stop";
		}
		cJSON_AddStringToObject(json1, WEBAPI_STATUS_STR, status);
		if(s_webapi_cur_time_ms_bak>0)
		{
			info = GxCore_Calloc(1, sizeof(PlayerProgInfo));
			if(NULL == info)
			{
				app_log_error("calloc failed\n");
				return NULL;
			}
			if(GXCORE_SUCCESS == GxPlayer_MediaGetProgInfoByName(WEBAPI_PLAYER_AV, info))
			{
				cJSON_AddStringToObject(json1, WEBAPI_ACODEC_STR, webapi_audio_type[webapi_get_audio_type(info->audio[info->cur_audio_id].codec_type)]);
				cJSON_AddStringToObject(json1, WEBAPI_VCODEC_STR, webapi_video_type[webapi_get_video_type(info->video.codec_type)]);
				if(info->video.width)
				{
					cJSON_AddNumberToObject(json1, WEBAPI_VWIDTH_STR, info->video.width);
				}
				if(info->video.height)
				{
					cJSON_AddNumberToObject(json1, WEBAPI_VHEIGHT_STR, info->video.height);
				}
				if(info->video.bitrate)
				{
					cJSON_AddNumberToObject(json1, WEBAPI_BITRATE_STR, info->video.bitrate);
				}
				if(info->video.fps)
				{
					memset(buffer,0,sizeof(buffer));
					sprintf(buffer, "%3.1f fps",info->video.fps);
					cJSON_AddStringToObject(json1, WEBAPI_FPS_STR, buffer);
				}
			}
			cJSON_AddNumberToObject(json1, WEBAPI_CURRENT_TIME_STR, s_webapi_cur_time_ms_bak);
			cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_TIME_STR, s_webapi_total_video_dur);
			GxCore_Free(info);
		}
		out = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
	}

	return out;
}

static void app_webapi_mute_exec(void)
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

static void app_webapi_mute_draw(void)
{
	if(g_AppFullArb.state.mute == STATE_ON)
	{
		GUI_SetProperty(IMG_MUTE, "state", "show");
	}
	else
	{
		GUI_SetProperty(IMG_MUTE, "state", "osd_trans_hide");
	}
	GUI_SetInterface("flush", NULL);
}

void app_webapi_unmute_change(void)
{
    if(g_AppFullArb.state.mute == STATE_ON)
    {
        app_webapi_mute_exec();
        app_webapi_mute_draw();
    }
}

static status_t app_webapi_sliderbar_seek_timercb(void* usrdata)
{
	s_webapi_sliderbar_seek_timer = NULL;
	s_seek_flag = VIDEO_SEEKED;

	return GXCORE_SUCCESS;
}

static status_t app_webapi_sliderbar_okkey(void)
{
	int cur_time_ms = 0;
	int value=0;

	GUI_GetProperty(SLIDER_CURSOR, "value", &value);
	cur_time_ms = s_webapi_total_video_dur*value/100;

	if((cur_time_ms > s_webapi_total_video_dur)||(0 == s_webapi_total_video_dur))
	{
		return GXCORE_ERROR;
	}

    GUI_SetProperty(IMG_PAUSE, "state", "hide");
	webapi_net_player_seek(cur_time_ms);
	s_seek_flag = VIDEO_SEEK_NONE;
	s_webapi_cur_time_ms_bak = cur_time_ms;
	s_webapi_cur_sliderbar_bak = value;

	return GXCORE_SUCCESS;
}

int app_webapi_netplay_menu_is_show(void)
{
    if ( (GUI_CheckDialog(WND_WEBAPI_NETPLAY)==GXCORE_SUCCESS) && s_webapi_infobar_show_flag )
    {
        return 1 ;
    }

    return 0 ;
}

int app_webapi_netplay_avcodec_status(GxMessage* msg)
{
	GxMsgProperty_PlayerAVCodecReport *avcodec_status = NULL;
	avcodec_status = (GxMsgProperty_PlayerAVCodecReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);

	if(AVCODEC_ERROR == avcodec_status->acodec.state|| AVCODEC_ERROR == avcodec_status->vcodec.state)
	{

	}
	else if(avcodec_status->vcodec.state == AVCODEC_RUNNING || avcodec_status->acodec.state == AVCODEC_RUNNING)
	{
        PlayerStatusInfo player_status;
        GxPlayer_MediaGetStatus(WEBAPI_PLAYER_AV, &player_status);
        if(player_status.status == PLAYER_STATUS_PLAY_RUNNING)
            webapi_play_state = PLAY_STATE_RUNNING;
        else if(player_status.status == PLAYER_STATUS_START_FILL_BUF)
            webapi_play_state = PLAY_STATE_LOAD;
        else if(player_status.status == PLAYER_STATUS_PLAY_PAUSE)
            webapi_play_state = PLAY_STATE_PAUSE;
        else
            webapi_play_state = PLAY_STATE_RUNNING;
	}

	return EVENT_TRANSFER_STOP;
}

int app_webapi_netplay_msg_proc(GxMessage *msg)
{
	GxMsgProperty_PlayerStop player_stop = {0};
	GxMsgProperty_PlayerStatusReport* player_status = NULL;

	player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);

	if((player_status == NULL) || (strcmp((char*)(player_status->player), WEBAPI_PLAYER_AV) != 0))
	{
		return EVENT_TRANSFER_STOP;
	}

	popdlg_destroy();
	switch(player_status->status)
	{
		case PLAYER_STATUS_ERROR:
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_ERROR\n",__FUNCTION__, __LINE__);
			webapi_play_state = PLAY_STATE_ERROR;
			s_webapi_cur_time_ms_bak = 0;
			s_webapi_cur_sliderbar_bak = 0;
			s_webapi_total_video_dur = 0;
			break;

		case PLAYER_STATUS_PLAY_END:
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_PLAY_END\n",__FUNCTION__, __LINE__);
			player_stop.player = WEBAPI_PLAYER_AV;
			app_send_msg_exec(GXMSG_PLAYER_STOP, (void *)(&player_stop));
			webapi_play_state = PLAY_STATE_NONE;
			s_webapi_cur_time_ms_bak = 0;
			s_webapi_cur_sliderbar_bak = 0;
			s_webapi_total_video_dur = 0;
			break;

		case PLAYER_STATUS_PLAY_RUNNING:
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_PLAY_RUNNING\n",__FUNCTION__, __LINE__);
			break;

		case PLAYER_STATUS_PLAY_START:
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_PLAY_START\n",__FUNCTION__, __LINE__);
			break;

		case PLAYER_STATUS_PLAY_PAUSE:
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_PLAY_PAUSE\n",__FUNCTION__, __LINE__);
			break;

		case PLAYER_STATUS_STOPPED:
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_STOPPED\n",__FUNCTION__, __LINE__);
			break;

		case PLAYER_STATUS_START_FILL_BUF:
			webapi_play_state = PLAY_STATE_LOAD;
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_START_FILL_BUF\n",__FUNCTION__, __LINE__);
			break;

		case PLAYER_STATUS_END_FILL_BUF:
			webapi_play_state = PLAY_STATE_RUNNING;
			WEBAPI_PRINTF("%s[%d]PLAYER_STATUS_END_FILL_BUF\n",__FUNCTION__, __LINE__);
			break;

		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int webapi_netplay_init(GuiWidget *widget, void *usrdata)
{
    uint32_t audio_val_sys = 0;
    int32_t audio_val_config = 0;
    app_system_performace_dynamic_change(NET_PLAY_MODE);
	webapi_ui_exits = 1;
	webapi_download_pic_handle = 0;
	webapi_pic_download_ok = 0;
	webapi_pic_show_ok = 0;
	webapi_pic_download_abort = 0;
	s_webapi_cur_time_ms_bak = 0;
	s_webapi_cur_sliderbar_bak = 0;
	webapi_hide_popup_msg();
	app_webapi_mute_draw();
	app_netapps_aspect_init();
	_webapi_netplay_load_gif();
    GDI_RegisterJPEG();
	gal_img_threshod_size(PIC_SHOW_MAX_SIZE);//MINI_MEM_SUPPORT limit malloc memory 20230425
	s_webapi_time_timer = create_timer(app_webapi_update_time, 1000, NULL, TIMER_REPEAT);

    audio_val_sys = app_system_vol_get();
    GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_val_config, AUDIO_VOLUME);
    if ((audio_val_sys != audio_val_config) && (audio_val_config > 0) )
    {
        app_system_vol_set(audio_val_config);
    }

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int webapi_netplay_destroy(GuiWidget *widget, void *usrdata)
{
	webapi_ui_exits = 0;
	webapi_play_state = PLAY_STATE_NONE;
	if(media_type == WEBAPI_MEDIA_PICTURE)
		GDI_StopImageNoClear("xxx.gif");
    GDI_UnRegisterJPEG();
	media_type = WEBAPI_MEDIA_UNKNOWN;
	s_webapi_cur_time_ms_bak = 0;
	s_webapi_cur_sliderbar_bak = 0;
	s_webapi_total_video_dur = 0;
	s_webapi_paused = 0;
	APP_TIMER_REMOVE(s_webapi_time_timer);
	APP_TIMER_REMOVE(s_webapi_state_refresh_timer);
	app_netapps_aspect_exit();
	app_webapi_state_refresh_stop();
	app_webapi_hide_infobar();
	webapi_net_player_stop();
	_webapi_netplay_hide_gif();
	_webapi_netplay_free_gif();
	app_webapi_pic_download_stop();
	app_webapi_netplay_dvb_resume();
	app_system_performace_dynamic_change(DVB_PLAY_MODE);
	gal_img_threshod_size(0);//resevser to limit malloc memory

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int webapi_netplay_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_KEYDOWN:
		{
			switch(event->key.sym)
			{
				case STBK_EXIT:
					if(media_type == WEBAPI_MEDIA_VIDEO)
					{
						if((s_webapi_infobar_show_flag == 0) || (webapi_play_state != PLAY_STATE_RUNNING))
						{
							_webapi_netplay_hide_gif();
							webapi_hide_popup_msg();
							GUI_SetInterface("flush", NULL);
							GUI_EndDialog(WND_WEBAPI_NETPLAY);
						}
						else
						{
							app_webapi_hide_infobar();
						}

					}
					else
					{
						GUI_EndDialog(WND_WEBAPI_NETPLAY);
					}
					break;

				case STBK_INFO:
				case STBK_OK:
					if(((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))&&(s_webapi_infobar_show_flag == 0))
					{
						app_webapi_show_infobar();
					}
					break;

				case STBK_VOLDOWN:
				case STBK_VOLUP:
				case STBK_LEFT:
				case STBK_RIGHT:
					GUI_CreateDialog("wnd_volume_value");
					GUI_SendEvent("wnd_volume_value", event);
					app_webapi_mute_draw();
					break;

				case STBK_MUTE:
					app_webapi_mute_exec();
					app_webapi_mute_draw();
					break;
				case STBK_ASPECT:
					app_netapps_aspect_hotkey();
					break;
				default:
					break;
			}
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_webapi_netplay_cursor_keypress(const char* widgetname, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	int64_t cur_time_ms = 0;
	int value=0,step=0;
	char buf_tmp[20] = {0};

	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);
	event = (GUI_Event *)usrdata;
	switch(event->key.sym)
	{
		case STBK_EXIT:
			app_webapi_hide_infobar();
			break;

		case STBK_LEFT:
			if(s_webapi_total_video_dur>0)
			{
				s_seek_flag = VIDEO_SEEKING;
				if((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
				{
					app_webapi_infobar_timer_reset();
				}
				GUI_GetProperty(SLIDER_CURSOR, "value", &value);
				GUI_GetProperty(SLIDER_CURSOR, "step", &step);
				if(value-step<0)
				{
					value=0;
				}
				else
				{
					value-=step;
				}
				cur_time_ms = s_webapi_total_video_dur*value/100;
                app_time_ms_to_edit_str(cur_time_ms, buf_tmp, 20);
				GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
				GUI_SetProperty(SLIDER_CURSOR, "value", &value);

				APP_TIMER_ADD(s_webapi_sliderbar_seek_timer, app_webapi_sliderbar_seek_timercb, 100, TIMER_ONCE);
			}
			break;

		case STBK_RIGHT:
			if(s_webapi_total_video_dur>0)
			{
				s_seek_flag = VIDEO_SEEKING;
				if((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
				{
					app_webapi_infobar_timer_reset();
				}
				GUI_GetProperty(SLIDER_CURSOR, "value", &value);
				GUI_GetProperty(SLIDER_CURSOR, "step", &step);
				if(value+step>100)
				{
					value=100;
				}
				else
				{
					value+=step;
				}
				cur_time_ms = s_webapi_total_video_dur*value/100;
                app_time_ms_to_edit_str(cur_time_ms, buf_tmp, 20);
				GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
				GUI_SetProperty(SLIDER_CURSOR, "value", &value);
				APP_TIMER_ADD(s_webapi_sliderbar_seek_timer, app_webapi_sliderbar_seek_timercb, 100, TIMER_ONCE);
			}
			break;

		case STBK_OK:
			if((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
			{
				app_webapi_infobar_timer_reset();
			}
			webapi_play_state = PLAY_STATE_LOAD;
			app_webapi_sliderbar_okkey();
			break;;

		default:
			ret = EVENT_TRANSFER_KEEPON;
			break;
	}

	return ret;
}


#endif

