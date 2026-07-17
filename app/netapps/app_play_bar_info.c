/*************************************************************************
	> File Name: app_plugin_play_info.c
	> Author:g
	> Mail:
	> Created Time: 2017年12月8日 星期五 10时12分45秒
    > Dec: new UI mode for plugin
 ************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_play_bar_info.h"
#include "app_windows.h"
#include "app_netvideo.h"
#include "app_netvideo_play.h"
#if NETAPPS_SUPPORT

struct ThisUICtrol{
    event_list *sp_info_timer;
    int s_timer_cnt;
    int seeking;
    uint64_t  total_time;
    bool seek_support;
    PlayBarInfoOps play_info_ops;
#ifdef IPTV_EPG_SUPPORT
    EpgEventList *short_epg;
#endif
};

enum{
    SEEK_NONE = 0,
    SEEKING,
    SEEKED
};

#define TEXT_BAR_TITLE   "text_plugin_play_info_bar_title"
#define SLIDER_CURSOR    "sliderbar_cursor_plugin_play_info"
#define SLIDER_PROCESS    "sliderbar_process_plugin_play_info"
#define TEXT_PLAY_INFO_NAME "text_plugin_play_info_name"
#define TEXT_PLAY_INFO_NAME_NUM "text_plugin_play_info_name_num"
#define TEXT_PLAY_INFO_SOLUTION "text_plugin_play_info_solution"
#define TEXT_PLAY_INFO_VFORMAT "text_plugin_play_info_vformat"
#define TEXT_PLAY_INFO_AFORMAT "text_plugin_play_info_aformat"
#define TEXT_PLAY_INFO_NETSPEED "text_plugin_play_info_netspeed"
#define TEXT_PLAY_INFO_TIME "text_plugin_play_info_time"
#define TEXT_PLAY_INFO_DATE "text_plugin_play_info_date"
#define TEXT_START_TIME    "text_plugin_play_start_time"
#define TEXT_SEEK_TIME    "text_plugin_play_seek_time"
#define TEXT_STOP_TIME    "text_plugin_play_stop_time"
#define INFO_TXT_EPG_CUR "text_epg_info_current"
#define INFO_TXT_EPG_NEXT "text_epg_info_next"


#define PLAY_INFOBAR_TIMEOUT (5)
#define INFO_EPG_NUM (2)

static struct ThisUICtrol this = {0};
static void _plugin_play_info_show(void);
static void timer_plugin_play_info_timeout_reset(void);

#ifdef IPTV_EPG_SUPPORT
extern void app_iptv_free_epglist(EpgEventList *epglist);

static void _play_epg_update(void)
{
    char buffer[128];
	if(this.short_epg&&(this.short_epg->epg_num>0))
	{
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer)-1, "%s-%s %s", this.short_epg->epg_item[0].start_time, this.short_epg->epg_item[0].end_time, this.short_epg->epg_item[0].title);
		GUI_SetProperty(INFO_TXT_EPG_CUR, "string", buffer);
		if(this.short_epg->epg_num>1)
				{
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer)-1, "%s-%s %s", this.short_epg->epg_item[1].start_time, this.short_epg->epg_item[1].end_time, this.short_epg->epg_item[1].title);
			GUI_SetProperty(INFO_TXT_EPG_NEXT, "string", buffer);
				}
				else
				{
			GUI_SetProperty(INFO_TXT_EPG_NEXT, "string", STR_ID_NO_INFO);
				}
			}
	else
	{
		GUI_SetProperty(INFO_TXT_EPG_CUR, "string", STR_ID_NO_INFO);
		GUI_SetProperty(INFO_TXT_EPG_NEXT, "string", STR_ID_NO_INFO);
	}
}

static void _free_short_epg(void)
{
	if(this.short_epg)
	{
		app_iptv_free_epglist(this.short_epg);
		this.short_epg = NULL;
	}
}
static void _get_short_epg_thread(void* arg)
{
	EpgEventList *epg = NULL;
	GxCore_ThreadDetach();

    if(this.play_info_ops.epg_ctrl.get_short_epg)
	{
		epg = this.play_info_ops.epg_ctrl.get_short_epg(this.play_info_ops.url, INFO_EPG_NUM);
        if(epg)
        {
            this.short_epg = epg;
        }
	}
}
static void _get_short_epg(void)
{
	int thread_id = 0;
	GxCore_ThreadCreate("iptv_getsepg", &thread_id, _get_short_epg_thread,
		NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}
#endif

int app_play_bar_info_show(PlayBarInfoOps *info_ops)
{
    int seek_bak = this.seeking;
    memset(&this, 0, sizeof(struct ThisUICtrol));
    memcpy(&this.play_info_ops, info_ops, sizeof(PlayBarInfoOps));
    this.seeking = seek_bak;
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME))
    {
        GUI_EndDialog(WND_VOLUME);
    }
    GUI_SetInterface("flush", NULL);//#347999，如已关闭WND_PLUGIN_PLAY_INFO窗口需先更新
    if(GUI_CheckDialog(WND_PLUGIN_PLAY_INFO) == GXCORE_SUCCESS)
    {
        _plugin_play_info_show();
        timer_plugin_play_info_timeout_reset();
    }
    else
    {
        if(GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS && GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS )
            GUI_CreateDialog(WND_PLUGIN_PLAY_INFO);
    }
#ifdef IPTV_EPG_SUPPORT
    if(this.play_info_ops.epg_ctrl.get_short_epg)
	{
		GUI_SetProperty(INFO_TXT_EPG_CUR, "state", "show");
		GUI_SetProperty(INFO_TXT_EPG_NEXT, "state", "show");
        GUI_SetProperty(TEXT_START_TIME, "state", "hide");
        GUI_SetProperty(TEXT_STOP_TIME, "state", "hide");
        GUI_SetProperty(SLIDER_CURSOR, "state", "hide");
        GUI_SetProperty(SLIDER_PROCESS, "state", "hide");
        _get_short_epg();
	}
    else
    {
		GUI_SetProperty(INFO_TXT_EPG_CUR, "state", "hide");
		GUI_SetProperty(INFO_TXT_EPG_NEXT, "state", "hide");
        GUI_SetProperty(TEXT_START_TIME, "state", "show");
        GUI_SetProperty(TEXT_STOP_TIME, "state", "show");
        GUI_SetProperty(SLIDER_CURSOR, "state", "show");
        GUI_SetProperty(SLIDER_PROCESS, "state", "show");
    }
#else
        GUI_SetProperty(TEXT_START_TIME, "state", "show");
        GUI_SetProperty(TEXT_STOP_TIME, "state", "show");
#endif
    GUI_SetProperty(TEXT_SEEK_TIME, "state", "hide");
    if(this.play_info_ops.bar_title != NULL)
        GUI_SetProperty(TEXT_BAR_TITLE, "string", this.play_info_ops.bar_title);


    return 0;
}

int app_play_bar_info_hide(void)
{
    int value = 0;

    if(this.seeking == SEEKING)
        this.seeking = SEEK_NONE;
    if (this.sp_info_timer)
        remove_timer(this.sp_info_timer);
    this.sp_info_timer = NULL;
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PLUGIN_PLAY_INFO))
    {
        GUI_SetProperty(SLIDER_PROCESS, "value", &value);
        GUI_SetProperty(SLIDER_CURSOR, "value", &value);
        GUI_EndDialog(WND_PLUGIN_PLAY_INFO);
        app_net_video_subtitle_resume();
    }
    return 0;
}

static bool _seekable_check(void)
{
    PlayerNetInfo netInfo = {0};

    if(this.total_time > 0 && GXCORE_SUCCESS == GxPlayer_MediaGetNetInfo(PLAYER_FOR_IPTV, &netInfo))
        return netInfo.seek_flag;
    else
        return false;
}

static int _video_codec_type_change(int video_type)
{
    int type_index = 0;

    switch(video_type)
    {
        case VIDEO_CODEC_MPEG12:
            type_index = 0;
            break;
        case VIDEO_CODEC_MPEG4:
            type_index = 1;
            break;
        case VIDEO_CODEC_H263:
            type_index = 2;
            break;
        case VIDEO_CODEC_H264:
            type_index = 3;
            break;
        case VIDEO_CODEC_REAL:
            type_index = 4;
            break;
        case VIDEO_CODEC_AVS:
            type_index = 5;
            break;
        case VIDEO_CODEC_H265:
            type_index = 6;
            break;
        case VIDEO_CODEC_VC1:
            type_index = 7;
            break;
        case VIDEO_CODEC_MJPEG:
            type_index = 8;
            break;
        case VIDEO_CODEC_DIV3:
            type_index = 9;
            break;
        default:
            type_index = 10;
            break;
    }
    return type_index;
}

static int _audio_codec_type_change(int audio_type)
{
    int type_index = 0;

    switch(audio_type)
    {
        case AUDIO_CODEC_MPEG1:
            type_index = 0;
            break;
        case AUDIO_CODEC_MPEG2:
            type_index = 1;
            break;
        case AUDIO_CODEC_AAC_LATM:
            type_index = 2;
            break;
        case AUDIO_CODEC_AAC_ADTS:
            type_index = 3;
            break;
        case AUDIO_CODEC_VORBIS:
            type_index = 4;
            break;
        case AUDIO_CODEC_AC3:
            type_index = 5;
            break;
        case AUDIO_CODEC_AVS:
            type_index = 6;
            break;
        case AUDIO_CODEC_PCM:
            type_index = 7;
            break;
        case AUDIO_CODEC_EAC3:
            type_index = 8;
            break;
        case AUDIO_CODEC_DTS:
            type_index = 9;
            break;
        case AUDIO_CODEC_DRA1:
            type_index = 10;
            break;
        case AUDIO_CODEC_DTS_HD:
            type_index = 11;
            break;
        case AUDIO_CODEC_REAL:
            type_index = 12;
            break;
        case AUDIO_CODEC_RA_AAC:
            type_index = 13;
            break;
        case AUDIO_CODEC_RA_RA8LBR:
            type_index = 14;
            break;
        case AUDIO_CODEC_FLAC:
            type_index = 16;
            break;
        default:
            type_index = 15;
            break;
    }
    return type_index;
}

static void _plugin_play_info_show(void)
{
    char Buffer[100];
    char time_str[24] = {0};
    char *tmp_str = NULL;
    char solution_str[15] = {0};
    uint64_t cur_time_ms = 0;
    PlayTimeInfo  tinfo = {0};
    PlayerStatusInfo status_info = {0};
    int start_play = 0;
    int cur_sliderbar = 0, seek_value = 0;
    char *video_type[] = {"MPEG2", "MPEG4", "H263", "H264", "REAL", "AVS", "H265", "VC1", "MJPEG", "DIV3", "UNKNOWN"};
    char *audio_type[] = {"MPEG1" , "MPEG2", "AAC_LATM", "AAC_ADT", "OGG ","AC3","AVS","PCM","EAC3","DTS","DRA1","DTS_HD","REAL","RA_AAC", "RA_RA8LBR", "UNKNOWN","FLAC"};

    memset(Buffer, 0, 100);
    if(this.play_info_ops.channel_total < 10000)
    {
        sprintf((char*)Buffer,"%04d", this.play_info_ops.channel_num);
        if( APP_THEME_CLASSIC_DARK ){
        GUI_SetProperty(TEXT_PLAY_INFO_NAME_NUM, "font", "font_main");
        }
    }
    else
    {
        sprintf((char*)Buffer,"%05d", this.play_info_ops.channel_num);
        if( APP_THEME_CLASSIC_DARK ){
        GUI_SetProperty(TEXT_PLAY_INFO_NAME_NUM, "font", "font_prog");
        }
    }
    GUI_SetProperty(TEXT_PLAY_INFO_NAME_NUM, "string", Buffer);
	if(GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS)
		GUI_SetFocusWidget(SLIDER_CURSOR);
    if(this.play_info_ops.channel_num == 0)
        GUI_SetProperty(TEXT_PLAY_INFO_NAME_NUM, "state", "hide");

    GUI_SetProperty(TEXT_PLAY_INFO_NAME, "string", this.play_info_ops.name);

    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));
    GUI_GetProperty(TEXT_PLAY_INFO_TIME, "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, time_str+MIN_SEC_OFFSET) != 0))
    {
        GUI_SetProperty(TEXT_PLAY_INFO_TIME, "string", time_str+MIN_SEC_OFFSET);
    }
    time_str[MIN_SEC_OFFSET] = '\0';
    GUI_GetProperty(TEXT_PLAY_INFO_DATE, "string", &tmp_str);
    if((tmp_str == NULL)
            || (strcmp(tmp_str, time_str) != 0))
    {
        GUI_SetProperty(TEXT_PLAY_INFO_DATE, "string", time_str);
    }
    PlayerProgInfo  *info = (PlayerProgInfo *)app_get_netvideo_playinfo();
    if(GXCORE_SUCCESS == GxPlayer_MediaGetStatus(PLAYER_FOR_IPTV, &status_info))
    {
        if(status_info.status == PLAYER_STATUS_PLAY_RUNNING || status_info.status == PLAYER_STATUS_PLAY_PAUSE \
          || status_info.status == PLAYER_STATUS_START_FILL_BUF || status_info.status == PLAYER_STATUS_END_FILL_BUF)
        {
            if(GXCORE_SUCCESS == GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_IPTV,info))
            {
                if((this.play_info_ops.url)&&(strcmp(info->url,this.play_info_ops.url)!=0))
                    start_play = 0;
                else
                    start_play = 1;
            }
        }
    }
    if(start_play && GxPlayer_MediaGetTime(PLAYER_FOR_IPTV, &tinfo) == GXCORE_SUCCESS)
	{
        cur_time_ms = tinfo.current;
        this.total_time = tinfo.totle;
		if(this.total_time == 0)
		{
		    sprintf(time_str, "00:00:00");
		}
		else
        {
            app_time_ms_to_edit_str(cur_time_ms,time_str,24);
        }
	    GUI_SetProperty(TEXT_START_TIME, "string", time_str);

        if(this.total_time == 0)
        {
		    sprintf(time_str, "  ");
        }
        else
        {
            app_time_ms_to_edit_str(this.total_time, time_str,24);
        }
		GUI_SetProperty(TEXT_STOP_TIME, "string", time_str);
        this.seek_support = _seekable_check();
        if(true == this.seek_support)
        {
            if(this.total_time > 0)
            {
                if(cur_time_ms > this.total_time)
                    cur_sliderbar = 100;
                else
                    cur_sliderbar = (cur_time_ms * 100)/this.total_time;

                GUI_SetProperty(SLIDER_PROCESS, "value", &cur_sliderbar);
                if(SEEKED == this.seeking || SEEK_NONE == this.seeking)
                {
                    this.seeking = SEEK_NONE;
                    seek_value = cur_sliderbar;
                    GUI_SetProperty(SLIDER_CURSOR, "value", &seek_value);
                }
                else
                {
                    GUI_GetProperty(SLIDER_CURSOR, "value", &seek_value);
                    GUI_SetProperty(SLIDER_CURSOR, "value", &seek_value);
                }
            }
        }
    }
    else
    {
        if(start_play == 0 && this.seeking == SEEK_NONE)
        {
            GUI_SetProperty(TEXT_START_TIME, "string", "00:00:00");
            GUI_SetProperty(TEXT_STOP_TIME, "string", "00:00:00");
            GUI_SetProperty(SLIDER_PROCESS, "value", &seek_value);
            GUI_SetProperty(SLIDER_CURSOR, "value", &seek_value);
        }
    }

    if(start_play)
    {
        if((info->video.width>0)&&(info->video.height>0))
        {
            sprintf(solution_str, "%d x %d", info->video.width, info->video.height);
            GUI_SetProperty(TEXT_PLAY_INFO_SOLUTION, "string", solution_str);
        }
        else
        {
            GUI_SetProperty(TEXT_PLAY_INFO_SOLUTION, "string", "NO RES");
        }
        GUI_SetProperty(TEXT_PLAY_INFO_VFORMAT, "string", video_type[_video_codec_type_change(info->video.codec_type)]);
        GUI_SetProperty(TEXT_PLAY_INFO_AFORMAT, "string", audio_type[_audio_codec_type_change(info->audio[0].codec_type)]);
        memset(Buffer, 0, 100);
        sprintf((char*)Buffer,"%d KB/s", info->net_speed);
        GUI_SetProperty(TEXT_PLAY_INFO_NETSPEED, "string", Buffer);
    }
    else
    {
        GUI_SetProperty(TEXT_PLAY_INFO_VFORMAT, "string", video_type[_video_codec_type_change(0xff)]);
        GUI_SetProperty(TEXT_PLAY_INFO_AFORMAT, "string", audio_type[_audio_codec_type_change(0xff)]);
        GUI_SetProperty(TEXT_PLAY_INFO_SOLUTION, "string", "NO RES");
        GUI_SetProperty(TEXT_PLAY_INFO_NETSPEED, "string", "0 KB/s");
    }
#ifdef IPTV_EPG_SUPPORT
    _play_epg_update();
#endif
}

static int timer_plugin_play_info_timeout(void *userdata)
{
    this.s_timer_cnt--;
    if(this.s_timer_cnt <= 0)
    {
        app_play_bar_info_hide();
    }
    else
    {
        _plugin_play_info_show();
    }
    return 0;
}

static void timer_plugin_play_info_timeout_reset(void)
{
	this.s_timer_cnt = PLAY_INFOBAR_TIMEOUT;
	if (reset_timer(this.sp_info_timer) != 0)
	{
		this.sp_info_timer = create_timer(timer_plugin_play_info_timeout, 1000, NULL, TIMER_REPEAT);
	}
}

SIGNAL_HANDLER int app_plugin_play_info_create(GuiWidget *widget, void *usrdata)
{
    _plugin_play_info_show();
    timer_plugin_play_info_timeout_reset();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_play_info_destroy(GuiWidget *widget, void *usrdata)
{
    if(this.sp_info_timer != NULL)
    {
        remove_timer(this.sp_info_timer);
        this.sp_info_timer = NULL;
    }
#ifdef IPTV_EPG_SUPPORT
    _free_short_epg();
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_play_info_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;

    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
		    case STBK_MENU:
            {
                app_play_bar_info_hide();
                break;
            }
            case STBK_UP:
		    case STBK_DOWN:
		    case STBK_OK:
            {
                app_play_bar_info_hide();
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY))
                    GUI_SendEvent(WND_PLUGIN_LISTVIEW_PLAY, event);
                else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG_PLAYBACK))
                    GUI_SendEvent(WND_EPG_PLAYBACK, event);
                else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                    GUI_SendEvent(WND_NETVIDEO_PLAY, event);
                break;
            }
            case STBK_TV_RADIO:
               if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG_PLAYBACK))
                    GUI_SendEvent(WND_EPG_PLAYBACK, event);

                else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                    GUI_SendEvent(WND_NETVIDEO_PLAY, event);
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
            {
                app_play_bar_info_hide();
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                    GUI_SendEvent(WND_NETVIDEO_PLAY, event);
                break;
            }
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}



static void _plugin_play_info_cursor_left_right_keypress(int key)
{
    int value=0, step=0;
	uint64_t cur_time_ms = 0;
    char time_str[24] = {0};
    PlayTimeInfo  tinfo = {0};

    if(true == this.seek_support)
    {
        this.seeking = SEEKING;
        timer_plugin_play_info_timeout_reset();
        GUI_GetProperty(SLIDER_CURSOR, "value", &value);
        GUI_GetProperty(SLIDER_CURSOR, "step", &step);
        if(key == STBK_RIGHT)
        {
            if(value + step > 100)
            {
                value = 100;
            }
            else
            {
                value += step;
            }
        }
        else
        {
            if(value - step < 0)
            {
                value = 0;
            }
            else
            {
                value -= step;
            }
        }
        cur_time_ms = value * this.total_time / 100;
        app_time_ms_to_edit_str(cur_time_ms,time_str,24);
        GUI_SetProperty(TEXT_SEEK_TIME, "string", time_str);
        if(GxPlayer_MediaGetTime(PLAYER_FOR_IPTV, &tinfo) == GXCORE_SUCCESS)
        {
            memset(time_str,0,sizeof(time_str));
            app_time_ms_to_edit_str(tinfo.current,time_str,24);
            GUI_SetProperty(TEXT_START_TIME, "string", time_str);
        }
        GUI_SetProperty(TEXT_SEEK_TIME, "state", "show");
        GUI_SetProperty(SLIDER_CURSOR, "value", &value);
    }
}

static void _plugin_play_info_cursor_ok_keypress(void)
{
	int cur_time_ms = 0;
	int value=0;
	GxMsgProperty_PlayerSeek seek;

    if(this.seeking == SEEKING)
    {
        timer_plugin_play_info_timeout_reset();
        GUI_GetProperty(SLIDER_CURSOR, "value", &value);
        cur_time_ms = this.total_time*value/100;

        memset(&seek, 0, sizeof(GxMsgProperty_PlayerSeek));
        seek.player = PLAYER_FOR_IPTV;
        seek.time = cur_time_ms;
        seek.flag = SEEK_ORIGIN_SET;

        app_send_msg_exec(GXMSG_PLAYER_SEEK, &seek);
        GUI_SetProperty(TEXT_SEEK_TIME, "state", "hide");
    }

    this.seeking = SEEKED;
    timer_plugin_play_info_timeout_reset();
}

SIGNAL_HANDLER int app_plugin_play_info_cursor_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;


	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);
	event = (GUI_Event *)usrdata;
	switch(event->key.sym)
	{
        case VK_BOOK_TRIGGER:
            GUI_EndDialog("after wnd_full_screen");
            break;
		case STBK_EXIT:
		case STBK_MENU:
        {
            app_play_bar_info_hide();
            break;
        }
		case STBK_UP:
		case STBK_DOWN:
        {
            app_play_bar_info_hide();
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY))
                GUI_SendEvent(WND_PLUGIN_LISTVIEW_PLAY, event);
            else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                GUI_SendEvent(WND_NETVIDEO_PLAY, event);
            break;
        }
		case STBK_TV_RADIO:
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                GUI_SendEvent(WND_NETVIDEO_PLAY, event);
            break;
		case STBK_LEFT:
		case STBK_RIGHT:
        {
            if(this.seek_support)
            {
                _plugin_play_info_cursor_left_right_keypress(event->key.sym);
            }
            break;
        }
		case STBK_OK:
        {
            if(this.seek_support && this.seeking == SEEKING)
            {
                _plugin_play_info_cursor_ok_keypress();
            }
            else
            {
                app_play_bar_info_hide();
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                    GUI_SendEvent(WND_NETVIDEO_PLAY, event);
            }
            break;
        }
        case STBK_MUTE:
        case STBK_PAUSE:
        {
            app_play_bar_info_hide();
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_DLNA_DMR))
                GUI_SendEvent(WND_DLNA_DMR, event);
            else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY))
                GUI_SendEvent(WND_PLUGIN_LISTVIEW_PLAY, event);
            else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                GUI_SendEvent(WND_NETVIDEO_PLAY, event);
            break;
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
        {
            app_play_bar_info_hide();
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
                GUI_SendEvent(WND_NETVIDEO_PLAY, event);
            break;
        }
		default:
			break;
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_play_info_got_focus(GuiWidget *widget, void *usrdata)
{
    timer_plugin_play_info_timeout_reset();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_play_info_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
