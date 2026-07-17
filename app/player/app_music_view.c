#include "app.h"
#include "pmp_lyrics.h"
#include "stdlib.h"
#include "app_default_params.h"
#include "media_info.h"
#include "app_volume_value.h"
#include "app_windows.h"

//resourse
#define IMG_POP_STOP					"s_pvr_stop"
#define IMG_BUTTON2_PLAY_FOCUS		"s_mpicon_play_focus"
#define IMG_BUTTON2_PLAY_UNFOCUS		"s_mpicon_play_unfocus"
#define IMG_BUTTON2_PAUSE_FOCUS		"s_mpicon_pause_focus"
#define IMG_BUTTON2_PAUSE_UNFOCUS	"s_mpicon_pause_unfocus"

#define IMG_POP_PREVIOUS				"MP_BUTTON_FRONT"
#define IMG_POP_PLAY					"MP_BUTTON_PLAY"
#define IMG_POP_NEXT					"MP_BUTTON_NEXT"
#define IMG_POP_PAUSE					"s_pause"
//widget
#define CANVAS_SPECTRUM                 "music_view_canvas_spectrum"
#define NOTEPAD_LRC                     "music_view_notepad_lrc"
#define IMAGE_POPMSG					"music_view_image_popmsg"
#define IMAGE_BOX_BACK					"music_view_image_boxback"
#define BOX_MUSIC_CTROL                 "music_view_box"
#define BUTTON_PLAY_STOP				"music_view_boxitem2_button"
#define TEXT_STATUS                     "music_view_text_status"
#define TEXT_MUSIC_NAME1				"music_view_text_name1"
#define TEXT_START_TIME                 "music_view_text_start_time"
#define TEXT_STOP_TIME					"music_view_text_stop_time"
#define IMAGE_MUTE                      "music_view_imag_mute"
#define SLIDERBAR_MUSIC                 "music_view_sliderbar"
#define SLIDER_CURSOR_MUSIC             "music_view_sliderbar_cursor"
#define IMG_SLIDER_BACK                 "music_view_img_sliderback"

static event_list* music_popmsg_topright_timer = NULL;
static event_list* music_box_refresh_timer = NULL;
static event_list* music_sliderbar_active_timer = NULL;
static event_list* delay_play_timer = NULL;

static play_music_speed_state music_box_button1_bspeed = PLAY_MUSIC_SPEED_X0;
static play_music_ctrol_state music_box_button2_ctrol = PLAY_MUSIC_CTROL_STOP;
static play_music_speed_state music_box_button3_fspeed = PLAY_MUSIC_SPEED_X0;
static uint64_t s_music_total_time=0;
static bool s_music_manual_stop = false;
static uint32_t s_view_mode_bak = PMPSET_MUSIC_VIEW_MODE_SPECTRUM;
static bool s_musicErrPlayState = false; //用于当第一次播放状态为err时，第二次进入还需重新播放

static enum
{
    MUSIC_SEEK_NONE = 0,
    MUSIC_SEEKING,
    MUSIC_SEEKED
}s_music_seek_flag = MUSIC_SEEK_NONE;

typedef enum
{
	MUSIC_BOX_BUTTON_PREVIOUS,
	MUSIC_BOX_BUTTON_PLAY_PAUSE,
	MUSIC_BOX_BUTTON_NEXT,
	MUSIC_BOX_BUTTON_STOP,
	MUSIC_BOX_BUTTON_LRC,
	MUSIC_BOX_BUTTON_SET,
	MUSIC_BOX_BUTTON_VOL,
	MUSIC_BOX_BUTTON_INFO,
	MUSIC_BOX_BUTTON_PAUSE,
	MUSIC_BOX_BUTTON_PLAY,

	MUSIC_BOX_BUTTON_SPEED_BACKWARD,	// TODO: later
	MUSIC_BOX_BUTTON_SPEED_FORWARD	// TODO: later
}music_box_button;
extern struct SpectrumView  SpectrumViewData;

extern void music_status_init(void);
extern status_t app_send_msg_exec(uint32_t msg_id,void* params);
static int _sequence_play(void* userdata);

void music_view_spectrum_create(void)
{
#define SPECTRUM_X_OFFSET	0
#define SPECTRUM_Y_YOFFSET  50
	int x,y,w,h = 0;
	sscanf((widget_get_property(gui_get_widget(NULL,CANVAS_SPECTRUM),"rect")),"[%d,%d,%d,%d]",&x,&y,&w,&h);
	if(w>SPECTRUM_X_OFFSET)
	{
		w = w-SPECTRUM_X_OFFSET;
	}
	if(h>SPECTRUM_Y_YOFFSET)
	{
		h = h-SPECTRUM_Y_YOFFSET;
	}
	spectrum_create(CANVAS_SPECTRUM, SPECTRUM_X_OFFSET, SPECTRUM_Y_YOFFSET, w, h);

}

void music_view_mode_create(void)
{
	pmpset_music_view_mode view_mode = 0;
	view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);

	GUI_SetProperty(CANVAS_SPECTRUM, "state", "hide");
	GUI_SetProperty(NOTEPAD_LRC, "state", "hide");

	if(PMPSET_MUSIC_VIEW_MODE_LRC == view_mode)
	{
		lyrics_create(NOTEPAD_LRC);
	}
	else if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode)
	{
		GUI_SetProperty("image_win_music_view_back", "state", "show");
		music_view_spectrum_create();
		if(PLAY_MUSIC_CTROL_PLAY == music_box_button2_ctrol && false == s_musicErrPlayState)
			spectrum_start();
	}
}

void music_view_mode_destroy(void)
{
	pmpset_music_view_mode view_mode = 0;
	view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
	if(PMPSET_MUSIC_VIEW_MODE_LRC == view_mode)
	{
		lyrics_destroy();
	}
	else if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode)
	{
		spectrum_destroy();
	}
}

void music_view_mode_start(void)
{
	pmpset_music_view_mode view_mode = 0;
	view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);

	if(PMPSET_MUSIC_VIEW_MODE_LRC == view_mode)
	{
		if(!s_musicErrPlayState)
			lyrics_start(NULL);
	}
	else if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode)
	{
		char* focus_win = NULL;
		focus_win = (char*)GUI_GetFocusWindow();
		if(NULL == focus_win) return;
		if(0 == strcasecmp(focus_win, WIN_MUSIC_VIEW))
		{
			spectrum_start();
		}
	}
}

static void music_view_mode_stop(void)
{
	pmpset_music_view_mode view_mode = 0;
	view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
	if(PMPSET_MUSIC_VIEW_MODE_LRC == view_mode)
	{
		lyrics_stop();
	}
	else if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode)
	{
		spectrum_stop();
	}
}

void music_view_mode_pause(void)
{
	pmpset_music_view_mode view_mode = 0;
	view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
	if(PMPSET_MUSIC_VIEW_MODE_LRC == view_mode)
	{
		lyrics_pause();
	}
	else if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode)
	{
//		spectrum_pause();
		spectrum_stop();
	}
}

void music_view_mode_resume(void)
{
	pmpset_music_view_mode view_mode = 0;
	view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
	if(PMPSET_MUSIC_VIEW_MODE_LRC == view_mode)
	{
		if((PLAY_MUSIC_CTROL_PLAY == music_box_button2_ctrol)
		||(PLAY_MUSIC_CTROL_PAUSE == music_box_button2_ctrol))
		{
			if(s_view_mode_bak == PMPSET_MUSIC_VIEW_MODE_LRC)
			{
				lyrics_resume();
			}
			else
			{
				if(!s_musicErrPlayState)
					lyrics_start(NULL);
			}
		}
	}
	else if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode)
	{
		if((PLAY_MUSIC_CTROL_PLAY == music_box_button2_ctrol)
                &&(GXCORE_SUCCESS != GUI_CheckDialog(WIN_MUSIC_SET)
                    &&GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_BOOK)))
		{
            if(spectrum_get_FlagForStop() == true)
            {
                spectrum_start();
            }
            else
            {
                spectrum_resume();
            }
		}
	}
}

extern void spectrum_volumebak_clear(void);
static int music_popmsg_topright_hide(void* usrdata)
{
    pmpset_music_view_mode view_mode = 0;
    view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);

	GUI_SetProperty(IMAGE_POPMSG, "state", "hide");
	APP_TIMER_REMOVE(music_popmsg_topright_timer);

    //for avoiding bug 146690
    if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode)
    {
        GUI_SetProperty(WIN_MUSIC_VIEW, "update", NULL);//for bug 146717
        spectrum_volumebak_clear();
        GUI_SetInterface("flush", 0);
    }

	return 0;
}

static void _create_music_info_dlg(void)
{
    MediaInfo media_info;
    memset(&media_info, 0, sizeof(media_info));
    music_info_get(&media_info);
    media_info.change_cb = music_info_get;
    media_info.destroy_cb = music_info_destroy;
    media_info_create(&media_info);
}

static void _create_music_volume_dlg(void)
{
    uint32_t value;
    value=pmpset_get_int(PMPSET_MUTE);

    if(value==PMPSET_MUTE_ON)
    {
        pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
        GUI_SetProperty(IMAGE_MUTE, "state", "hide");
        GUI_SetProperty(WIN_MUSIC_VIEW, "update", NULL);
        GUI_SetInterface("flush", NULL);
        spectrum_volumebak_clear();
    }

    int32_t pos_y = 40;
    GUI_CreateDialog(WND_VOLUME);
    GUI_SetProperty(WND_VOLUME, "move_window_y", &pos_y);
}

static status_t music_popmsg_topright_ctrl(play_music_ctrol_state button, uint32_t value)
{
	int timeout = 3000;

	APP_TIMER_REMOVE(music_popmsg_topright_timer);

	if(GUI_CheckDialog(WIN_MUSIC_VIEW) != GXCORE_SUCCESS)
		return GXCORE_ERROR;

	GUI_SetProperty(IMAGE_POPMSG, "state", "show");

	switch(button)
	{
		case PLAY_MUSIC_CTROL_PREVIOUS:
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PREVIOUS);
			break;

		case  PLAY_MUSIC_CTROL_BACKWARD:
			if(PLAY_MUSIC_SPEED_X0== value)
			{
				GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PLAY);
				break;
			}

			timeout = 0;
			break;


		case PLAY_MUSIC_CTROL_PLAY:
			timeout = 200;
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PLAY);
			break;


		case PLAY_MUSIC_CTROL_PAUSE:
			timeout = 0;
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PAUSE);
			break;


		case PLAY_MUSIC_CTROL_FORWARD:
			if(PLAY_MUSIC_SPEED_X0== value)
			{
				GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PLAY);
				break;
			}
			timeout = 0;
			break;


		case PLAY_MUSIC_CTROL_NEXT:
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_NEXT);
			break;


		case PLAY_MUSIC_CTROL_STOP:
			timeout = 0;
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_STOP);
			break;

		default:
			break;
	}

	if(timeout)
	{
		APP_TIMER_ADD(music_popmsg_topright_timer, music_popmsg_topright_hide, timeout, TIMER_REPEAT);
	}

	return GXCORE_SUCCESS;
}

status_t music_popmsg_mute_init(void)
{
	uint32_t value = 0;

	value=pmpset_get_int(PMPSET_MUTE);
#if USB_MICROPHONE_SUPPORT
    extern int app_microphone_get_mute_flag(void);
    if((app_microphone_get_mute_flag())||(value == PMPSET_MUTE_ON))
    {
        value = PMPSET_MUTE_ON;
    }
#endif
	switch (value)
	{
		case PMPSET_MUTE_ON:
			GUI_SetProperty(IMAGE_MUTE, "state", "show");
			break;

		case PMPSET_MUTE_OFF:
			GUI_SetProperty(IMAGE_MUTE, "state", "hide");
			break;

		default:
			return GXCORE_ERROR;
			break;
	}
	return GXCORE_SUCCESS;
}

status_t music_popmsg_mute_ctrl(void)
{
	uint32_t value = 0;

	value=pmpset_get_int(PMPSET_MUTE);
	switch (value)
	{
		case PMPSET_MUTE_ON:
			pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
			GUI_SetProperty(IMAGE_MUTE, "state", "hide");
            GUI_SetProperty(WIN_MUSIC_VIEW, "update", NULL);
            GUI_SetInterface("flush", NULL);
            spectrum_volumebak_clear();
			//spectrum_resume();
			break;

		case PMPSET_MUTE_OFF:
			pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_ON);
			GUI_SetProperty(IMAGE_MUTE, "state", "show");
			break;

		default:
			return GXCORE_ERROR;
			break;
	}
#if USB_MICROPHONE_SUPPORT
            extern int app_microphone_get_mute_flag(void);
            int mute = app_microphone_get_mute_flag();
            if(mute){
                GUI_SetProperty(IMAGE_MUTE, "state", "show");
            }
#endif
	return GXCORE_SUCCESS;
}

void app_music_unmute_change(void)
{
    uint32_t value = 0;
    value=pmpset_get_int(PMPSET_MUTE);
    if(value == PMPSET_MUTE_ON)
    {
        pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
        GUI_SetProperty(IMAGE_MUTE, "state", "hide");
        GUI_SetProperty(WIN_MUSIC_VIEW, "update", NULL);
        GUI_SetInterface("flush", NULL);
        spectrum_volumebak_clear();
    }
}

static status_t music_box_button2_setimg(play_music_ctrol_state state)
{
	if(PLAY_MUSIC_CTROL_PLAY == state)
	{
		GUI_SetProperty(BUTTON_PLAY_STOP, "unfocus_img", IMG_BUTTON2_PAUSE_UNFOCUS);
		GUI_SetProperty(BUTTON_PLAY_STOP, "focus_img", IMG_BUTTON2_PAUSE_FOCUS);
	}
	else if((PLAY_MUSIC_CTROL_PAUSE == state)||(PLAY_MUSIC_CTROL_STOP == state))
	{
		GUI_SetProperty(BUTTON_PLAY_STOP, "unfocus_img", IMG_BUTTON2_PLAY_UNFOCUS);
		GUI_SetProperty(BUTTON_PLAY_STOP, "focus_img", IMG_BUTTON2_PLAY_FOCUS);
	}
	else
	{
		return GXCORE_ERROR;
	}

	GUI_SetProperty(BOX_MUSIC_CTROL, "update", NULL);

	return GXCORE_SUCCESS;
}

static uint64_t music_total_time_get(void)
{
	uint64_t duration = 0;
	PlayTimeInfo  tinfo = {0};

    GxPlayer_MediaGetTime(PMP_PLAYER_AV, &tinfo);
	duration = tinfo.totle;

	return duration;
}


static status_t music_box_refreshinfo(void* usrdata)
{
	uint64_t cur_time_ms = 0;
	int cur_sliderbar = 0;
	char buf_tmp[20];
	status_t ret = GXCORE_ERROR;
	PlayTimeInfo  tinfo = {0};
    uint64_t total_time_ms = 0;
	ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &tinfo);
	if(GXCORE_SUCCESS == ret)
	{
        total_time_ms = tinfo.totle;
		cur_time_ms = tinfo.current;
		if((cur_time_ms > total_time_ms)||(0 == total_time_ms))
		{
			cur_time_ms = 0;
			cur_sliderbar = 0;
		}
		else
			cur_sliderbar = (cur_time_ms * 100)/ total_time_ms;
	}
	else
	{
		cur_sliderbar = 0;
		return GXCORE_ERROR;// 20140707_
	}
	/*sliderbar*/
	if((s_music_seek_flag == MUSIC_SEEK_NONE) || (s_music_seek_flag == MUSIC_SEEKED))
	{
		GUI_SetProperty(SLIDERBAR_MUSIC, "value", &cur_sliderbar);
        if(s_music_seek_flag == MUSIC_SEEK_NONE)
        {
		    GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &cur_sliderbar);
        }
        else
        {
            int seeked_val = 0;
            GUI_GetProperty(SLIDER_CURSOR_MUSIC, "value", &seeked_val);
            GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &seeked_val);
        }
		/*cur time*/
        app_time_ms_to_edit_str(cur_time_ms,buf_tmp,20);
		GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
	}
	/*total time*/
    app_time_ms_to_edit_str(total_time_ms,buf_tmp,20);
	GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);
	return GXCORE_SUCCESS;
}

static status_t music_box_setinfo(play_music_info* info)
{
	uint64_t cur_time_ms = 0;
	int cur_progbar = 0;
	char buf_tmp[20] = {0};
	status_t ret = GXCORE_ERROR;
	PlayTimeInfo  tinfo = {0};

	/*file name*/
	if(NULL == info)return GXCORE_ERROR;

    char *old_music_name = NULL;
    GUI_GetProperty(TEXT_MUSIC_NAME1, "string", &old_music_name);
    if ((old_music_name == NULL) || (strcmp(old_music_name, info->name) != 0))  //增加的条件
    {
        GUI_SetProperty(TEXT_MUSIC_NAME1, "string", info->name);
    }

	/*progbar*/
	ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &tinfo);
	cur_time_ms = tinfo.current;
	if(GXCORE_SUCCESS == ret)
    {
        if((cur_time_ms > s_music_total_time)||(0 == s_music_total_time))
        {
            cur_progbar = 0;
            cur_time_ms = 0;
        }
        else
            cur_progbar = cur_time_ms * 100 / s_music_total_time;

        if((s_music_seek_flag == MUSIC_SEEK_NONE) || (s_music_seek_flag == MUSIC_SEEKED))
        {
            GUI_SetProperty(SLIDERBAR_MUSIC, "value", &cur_progbar);
            if(s_music_seek_flag == MUSIC_SEEK_NONE)
            {
                GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &cur_progbar);
            }
            else
            {
                int seeked_val = 0;
                GUI_GetProperty(SLIDER_CURSOR_MUSIC, "value", &seeked_val);
                GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &seeked_val);
            }
        }

        /*cur time*/
        app_time_ms_to_edit_str(cur_time_ms,buf_tmp,20);
        GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
    }

	return GXCORE_SUCCESS;
}


status_t music_box_ctrl(music_box_button button)
{
	status_t ret = GXCORE_ERROR;
	play_music_info info;
	int sliderbar_init = 0;
	switch(button)
	{
		case MUSIC_BOX_BUTTON_PREVIOUS:
			APP_TIMER_REMOVE(delay_play_timer);

			/*reset speed state*/
			music_box_button1_bspeed = PLAY_MUSIC_SPEED_X0;
			music_box_button3_fspeed = PLAY_MUSIC_SPEED_X0;

			/*reset play state*/
			if(PLAY_MUSIC_CTROL_PLAY != music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				music_box_button2_setimg(music_box_button2_ctrol);
			}
			GUI_SetProperty(TEXT_STATUS, "state", "hide");
			GUI_SetProperty(SLIDERBAR_MUSIC, "value", &sliderbar_init);
			GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &sliderbar_init);
			GUI_SetProperty(TEXT_START_TIME, "string", "00:00:00");
			GUI_SetProperty(TEXT_STOP_TIME, "string", "00:00:00");
			music_view_mode_stop();

			/*previous*/
			music_popmsg_topright_ctrl(PLAY_MUSIC_CTROL_PREVIOUS, 0);
			ret =play_music_ctrol(PLAY_MUSIC_CTROL_PREVIOUS);
			if(GXCORE_ERROR == ret)
			{
				break;
			}
			play_music_get_info(&info);
			GUI_SetProperty(TEXT_MUSIC_NAME1, "string", info.name);
			break;

		case MUSIC_BOX_BUTTON_SPEED_BACKWARD:
			/*turn to playing, then speed*/
			if(PLAY_MUSIC_CTROL_PAUSE == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				music_box_button2_setimg(music_box_button2_ctrol);
				play_music_ctrol(PLAY_MUSIC_CTROL_RESUME);
				music_view_mode_resume();
			}
			else if(PLAY_MUSIC_CTROL_STOP == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				music_box_button2_setimg(music_box_button2_ctrol);
				play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
			}


			/*speed*/
			music_box_button3_fspeed = PLAY_MUSIC_SPEED_X0;
			if(PLAY_MUSIC_SPEED_X8 <= music_box_button1_bspeed)
			{
				music_box_button1_bspeed = PLAY_MUSIC_SPEED_X0;
			}
			else
			{
				music_box_button1_bspeed++;
			}
			music_popmsg_topright_ctrl(MUSIC_BOX_BUTTON_SPEED_BACKWARD, music_box_button1_bspeed);
			play_music_speed(PLAY_MUSIC_CTROL_BACKWARD, music_box_button1_bspeed);
			break;


		case MUSIC_BOX_BUTTON_PLAY_PAUSE:
			if(PLAY_MUSIC_CTROL_PLAY == music_box_button2_ctrol && false == s_musicErrPlayState)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PAUSE;
				play_music_ctrol(PLAY_MUSIC_CTROL_PAUSE);
				music_view_mode_pause();
				music_popmsg_topright_ctrl(music_box_button2_ctrol, 0);
			}
			else if(PLAY_MUSIC_CTROL_PAUSE == music_box_button2_ctrol)
			{
                if(s_musicErrPlayState != true)
                {
                    music_popmsg_topright_ctrl(PLAY_MUSIC_CTROL_PLAY, 0);
                }
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				play_music_ctrol(PLAY_MUSIC_CTROL_RESUME);
				music_view_mode_resume();
            }
			else if(PLAY_MUSIC_CTROL_STOP == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
                if(s_musicErrPlayState != true)
                {
                    music_popmsg_topright_ctrl(music_box_button2_ctrol, 0);
                }
			}
			music_box_button2_setimg(music_box_button2_ctrol);
			break;

		case MUSIC_BOX_BUTTON_PAUSE:
			if(PLAY_MUSIC_CTROL_PLAY == music_box_button2_ctrol && false == s_musicErrPlayState)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PAUSE;
				play_music_ctrol(PLAY_MUSIC_CTROL_PAUSE);
				music_view_mode_pause();
			}
			else if(PLAY_MUSIC_CTROL_PAUSE == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				play_music_ctrol(PLAY_MUSIC_CTROL_RESUME);
				music_view_mode_resume();
			}
			else
			{
				GUI_SetProperty(WIN_MUSIC_VIEW, "update", NULL);
				break;
			}
			music_popmsg_topright_ctrl(music_box_button2_ctrol, 0);
			music_box_button2_setimg(music_box_button2_ctrol);
			break;

		case MUSIC_BOX_BUTTON_PLAY:
			if(PLAY_MUSIC_CTROL_STOP == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
				play_music_get_info(&info);
				music_box_setinfo(&info);
			}
			else if(PLAY_MUSIC_CTROL_PAUSE == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				play_music_ctrol(PLAY_MUSIC_CTROL_RESUME);
				music_view_mode_resume();
			}
			else
			{
				break;
			}
			music_popmsg_topright_ctrl(music_box_button2_ctrol, 0);
			music_box_button2_setimg(music_box_button2_ctrol);
			break;

		case MUSIC_BOX_BUTTON_SPEED_FORWARD:
			/*turn to playing, then speed*/
			if(PLAY_MUSIC_CTROL_PAUSE == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				music_box_button2_setimg(music_box_button2_ctrol);
				play_music_ctrol(PLAY_MUSIC_CTROL_RESUME);
				music_view_mode_resume();
			}
			else if(PLAY_MUSIC_CTROL_STOP == music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				music_box_button2_setimg(music_box_button2_ctrol);
				play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
			}


			/*speed*/
			music_box_button1_bspeed = PLAY_MUSIC_SPEED_X0;
			if(PLAY_MUSIC_SPEED_X8 <= music_box_button3_fspeed)
			{
				music_box_button3_fspeed = PLAY_MUSIC_SPEED_X0;
			}
			else
			{
				music_box_button3_fspeed++;
			}
			music_popmsg_topright_ctrl(PLAY_MUSIC_CTROL_FORWARD, music_box_button3_fspeed);
			play_music_speed(PLAY_MUSIC_CTROL_FORWARD, music_box_button3_fspeed);
			break;


		case MUSIC_BOX_BUTTON_NEXT:
#if 1
			if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
			{
				 GUI_EndDialog(WND_VOLUME);
			}
#endif
			APP_TIMER_REMOVE(delay_play_timer);
			/*reset speed state*/
			music_box_button1_bspeed = PLAY_MUSIC_SPEED_X0;
			music_box_button3_fspeed = PLAY_MUSIC_SPEED_X0;
			/*reset play state*/
			if(PLAY_MUSIC_CTROL_PLAY != music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
				music_box_button2_setimg(music_box_button2_ctrol);
			}
			/*next*/
			GUI_SetProperty(TEXT_STATUS, "state", "hide");//same as MUSIC_BOX_BUTTON_PREVIOUS
			GUI_SetProperty(SLIDERBAR_MUSIC, "value", &sliderbar_init);
			GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &sliderbar_init);
			GUI_SetProperty(TEXT_START_TIME, "string", "00:00:00");
			GUI_SetProperty(TEXT_STOP_TIME, "string", "00:00:00");

			music_view_mode_stop();
			music_popmsg_topright_ctrl(PLAY_MUSIC_CTROL_NEXT, 0);
			ret =play_music_ctrol(PLAY_MUSIC_CTROL_NEXT);
			if(GXCORE_ERROR == ret)
			{
				break;
			}

			play_music_get_info(&info);
			GUI_SetProperty(TEXT_MUSIC_NAME1, "string", info.name);

			//music_box_setinfo(&info);
			break;

		case MUSIC_BOX_BUTTON_STOP:
			APP_TIMER_REMOVE(delay_play_timer);
			/*reset speed state*/
			music_box_button1_bspeed = PLAY_MUSIC_SPEED_X0;
			music_box_button3_fspeed = PLAY_MUSIC_SPEED_X0;//#error 10559

			char buf_tmp[20]= "00:00:00";
			GUI_SetProperty(SLIDERBAR_MUSIC, "value", &sliderbar_init);
			GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &sliderbar_init);
			GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
			GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);

			/*stop*/
			if(PLAY_MUSIC_CTROL_STOP != music_box_button2_ctrol)
			{
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_STOP;
				music_box_button2_setimg(music_box_button2_ctrol);
			}

			play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
			music_popmsg_topright_ctrl(PLAY_MUSIC_CTROL_STOP, 0);
			music_view_mode_stop();
			break;

		case MUSIC_BOX_BUTTON_LRC:
			{
				if(delay_play_timer !=NULL)
				{
					break;
				}
				uint32_t value_sel = 0;
				value_sel = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);

				if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == value_sel)
				{
					value_sel = PMPSET_MUSIC_VIEW_MODE_LRC;

					spectrum_destroy();

					if(!s_musicErrPlayState)
					{
						lyrics_create(NOTEPAD_LRC);
						if(music_box_button2_ctrol != PLAY_MUSIC_CTROL_STOP)//错误 #44120
						{
							lyrics_start(NULL);
						}
					}
				}
				else if(PMPSET_MUSIC_VIEW_MODE_LRC == value_sel)
				{
					value_sel = PMPSET_MUSIC_VIEW_MODE_SPECTRUM;

					lyrics_destroy();

					music_view_spectrum_create();
					if(PLAY_MUSIC_CTROL_PLAY == music_box_button2_ctrol && false == s_musicErrPlayState)
					{
						spectrum_start();
					}
					else
					{
						showSpectrumView();
						spectrum_stop();
					}
				}
				else
				{
					value_sel = PMPSET_MUSIC_VIEW_MODE_SPECTRUM;
				}
				pmpset_set_int(PMPSET_MUSIC_VIEW_MODE, value_sel);
				s_view_mode_bak = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
			}
			break;


		case MUSIC_BOX_BUTTON_SET:
            {
                s_view_mode_bak = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);

                GUI_CreateDialog(WIN_MUSIC_SET);
            }
			break;

		case MUSIC_BOX_BUTTON_INFO:
            {
                _create_music_info_dlg();
            }
			break;

		case MUSIC_BOX_BUTTON_VOL:
			{
                _create_music_volume_dlg();
				break;
			}


		default:
			return GXCORE_ERROR;
			break;
	}

	return GXCORE_SUCCESS;
}


static status_t music_sliderbar_okkey(void)
{
	int cur_time_ms = 0;
	int value=0;
	GxMessage *msg;
	GxMsgProperty_PlayerSeek *seek;

	GUI_GetProperty(SLIDER_CURSOR_MUSIC, "value", &value);
	cur_time_ms=s_music_total_time*value/100;
	if((cur_time_ms > s_music_total_time)||(0 == s_music_total_time))
		return GXCORE_ERROR;
    music_view_mode_pause();
	if(music_box_button2_ctrol==PLAY_MUSIC_CTROL_STOP)
	{
		play_list* list = NULL;
		char* path = NULL;

		/*play*/
		list = play_list_get(PLAY_LIST_TYPE_MUSIC);
		if(NULL == list) return GXCORE_ERROR;

		path = explorer_static_path_strcat(list->path, list->ents[list->play_no]);
		if(NULL == path) return GXCORE_ERROR;

		app_log_info("[PLAY] music: %s, start: %dms\n", path, cur_time_ms);
        if(cur_time_ms == s_music_total_time)//stop情况下直接播放最后时间不会发出play_end消息
            cur_time_ms -= 3000;
		play_av_by_url(path, cur_time_ms);

		GUI_SetProperty(IMAGE_POPMSG, "state", "hide");
		music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
		music_box_button2_setimg(music_box_button2_ctrol);
	}
	else
	{

		app_log_info("[SEEK] music seek time: %dms\n", cur_time_ms);
		msg = GxBus_MessageNew(GXMSG_PLAYER_SEEK);
		APP_CHECK_P(msg, 1);
		seek = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerSeek);
		APP_CHECK_P(seek, 1);
		seek->player = PMP_PLAYER_AV;
		seek->time = cur_time_ms;
		seek->flag = SEEK_ORIGIN_SET;
		GxBus_MessageSendWait(msg);
		GxBus_MessageFree(msg);
		if(PLAY_MUSIC_CTROL_PLAY == music_box_button2_ctrol)
		{
			music_box_button2_setimg(music_box_button2_ctrol);
		}
		else
		{
			music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
			music_box_button2_setimg(music_box_button2_ctrol);
			music_popmsg_topright_ctrl(music_box_button2_ctrol, 0);
		}
        GUI_SetProperty(SLIDERBAR_MUSIC, "value", &cur_time_ms);
	}

	music_view_mode_resume();
	s_music_seek_flag = MUSIC_SEEK_NONE;
    APP_TIMER_ADD(music_box_refresh_timer, music_box_refreshinfo, 200, TIMER_REPEAT);

	return GXCORE_SUCCESS;
}


static status_t music_sliderbar_active(void* usrdata)
{
    APP_TIMER_REMOVE(music_sliderbar_active_timer);
    s_music_seek_flag = MUSIC_SEEKED;
    APP_TIMER_ADD(music_box_refresh_timer, music_box_refreshinfo, 200, TIMER_REPEAT)

	return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int  music_view_sliderbar_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int  music_view_cursor_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int64_t cur_time_ms = 0;
	int value=0,step=0;
	char buf_tmp[20];
	play_music_info info;

	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event *)usrdata;
	switch(event->key.sym)
	{
		case APPK_LEFT:
			if(0 == s_music_total_time)
				return EVENT_TRANSFER_STOP;
			APP_TIMER_REMOVE(music_box_refresh_timer);
			s_music_seek_flag = MUSIC_SEEKING;

			GUI_GetProperty(SLIDER_CURSOR_MUSIC, "value", &value);
			GUI_GetProperty(SLIDER_CURSOR_MUSIC, "step", &step);
			if(value-step<0)
			{
				value=0;
			}
			else
			{
				value-=step;
			}
			cur_time_ms=s_music_total_time*value/100;
            app_time_ms_to_edit_str(cur_time_ms,buf_tmp,20);
			GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
			GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &value);

			APP_TIMER_ADD(music_sliderbar_active_timer, music_sliderbar_active, 100, TIMER_REPEAT);
			return EVENT_TRANSFER_STOP;

		case APPK_RIGHT:
			if(0 == s_music_total_time)
				return EVENT_TRANSFER_STOP;
            APP_TIMER_REMOVE(music_box_refresh_timer);
			s_music_seek_flag = MUSIC_SEEKING;

			GUI_GetProperty(SLIDER_CURSOR_MUSIC, "value", &value);
			GUI_GetProperty(SLIDER_CURSOR_MUSIC, "step", &step);
			if(value+step>100)
			{
				value=100;
			}
			else
			{
				value+=step;
			}
			cur_time_ms=s_music_total_time*value/100;
            app_time_ms_to_edit_str(cur_time_ms,buf_tmp,20);
			GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
			GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &value);

			APP_TIMER_ADD(music_sliderbar_active_timer, music_sliderbar_active, 100, TIMER_REPEAT);
			return EVENT_TRANSFER_STOP;

		case APPK_OK:
            APP_TIMER_REMOVE(music_sliderbar_active_timer);
            music_sliderbar_okkey();
			return EVENT_TRANSFER_STOP;

		case APPK_UP:
			return EVENT_TRANSFER_STOP;

		case APPK_DOWN:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			s_music_seek_flag = MUSIC_SEEK_NONE;
			play_music_get_info(&info);
			music_box_setinfo(&info);
			APP_TIMER_REMOVE(music_sliderbar_active_timer);
			APP_TIMER_ADD(music_box_refresh_timer, music_box_refreshinfo, 200, TIMER_REPEAT);
			if(music_box_button2_ctrol==PLAY_MUSIC_CTROL_STOP)
			{
				GUI_SetProperty(WIN_MUSIC_VIEW, "update", NULL);
			}
			return EVENT_TRANSFER_STOP;

		default:
			break;
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int music_view_box_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	uint32_t item_sel = 0;

	if(NULL == usrdata) return EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	switch(event->key.sym)
	{
		case APPK_OK:
			GUI_GetProperty(BOX_MUSIC_CTROL, "select", (void*)&item_sel);
			if(item_sel == MUSIC_BOX_BUTTON_STOP)
			{
				s_music_manual_stop = true;
			}
			music_box_ctrl(item_sel);
			return EVENT_TRANSFER_STOP;
			break;

		case APPK_DOWN:
			return EVENT_TRANSFER_STOP;

		case APPK_UP:
			GUI_SetFocusWidget(SLIDER_CURSOR_MUSIC);
			return EVENT_TRANSFER_STOP;

		default:
			break;
	}


	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER  int music_view_got_focus(const char* widgetname, void *usrdata)
{
	PlayerStatusInfo info;
	if(GXCORE_SUCCESS == GxPlayer_MediaGetStatus(PMP_PLAYER_AV, &info))
	{
		if(info.status == PLAYER_STATUS_PLAY_END)
		{
		}
		else if(info.status == PLAYER_STATUS_ERROR)
		{
		}
		else
		{
			music_view_mode_resume();
		}
	}
	// music set will set view mode,update it. 20230508
	s_view_mode_bak = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int music_view_lost_focus(const char* widgetname, void *usrdata)
{
	if (GXCORE_SUCCESS != GUI_CheckDialog(WND_VOLUME))
	{
		music_view_mode_pause();
	}

	return EVENT_TRANSFER_STOP;
}

static int _sequence_play(void* userdata)
{
	pmpset_music_play_sequence sequence = 0;

	APP_TIMER_REMOVE(delay_play_timer);
	if(0 == strcasecmp(WND_POP_BOOK, GUI_GetFocusWindow()))
	{
		return 0;
	}

	sequence = pmpset_get_int(PMPSET_MUSIC_PLAY_SEQUENCE);

	if(PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE == sequence)
	{
		play_list* list = NULL;

		list = play_list_get(PLAY_LIST_TYPE_MUSIC);
		if(list->play_no >= list->nents -1)
		{
			music_box_ctrl(MUSIC_BOX_BUTTON_STOP);
			GUI_EndDialog("after win_music_view");
			GUI_EndDialog(WIN_MUSIC_VIEW);
		}
		else
		{
			GUI_EndDialog("after win_music_view");
			APP_Printf("play next\n");
			music_box_ctrl(MUSIC_BOX_BUTTON_NEXT);
		}
	}
	else
	{
		music_box_ctrl(MUSIC_BOX_BUTTON_STOP);
		GUI_EndDialog("after win_music_view");
		GUI_EndDialog(WIN_MUSIC_VIEW);
	}
	return 0;
}

static int music_view_service_status(GxMsgProperty_PlayerStatusReport* player_status)
{
	char buf_tmp[20];
	play_list* list = NULL;
	char* path = NULL;
	pmpset_music_play_sequence sequence = 0;
	const char *wnd = NULL;

	APP_CHECK_P(player_status, 1);

	/*check this msg owner*/
	list = play_list_get(PLAY_LIST_TYPE_MUSIC);
	if(NULL == list) return 1;
	path = explorer_static_path_strcat(list->path, list->ents[list->play_no]);
	if(NULL == path || strcmp(path, player_status->url)) return 1;

	switch(player_status->status)
	{
		case PLAYER_STATUS_ERROR:
			s_musicErrPlayState = true;
			APP_Printf("[SERVICE] PLAYER_STATUS_ERROR, %d\n", player_status->error);
			APP_TIMER_REMOVE(music_box_refresh_timer);
			clean_music_file_no_bak();
			music_view_mode_stop();
			/*popmsg centre*/
			GUI_SetProperty(TEXT_STATUS, "string", STR_ID_CAN_NOT_PLAY_FILE);
			GUI_SetProperty(TEXT_STATUS, "state", "show");
			GUI_SetProperty(TEXT_STATUS, "draw_now", NULL);
			APP_TIMER_ADD(delay_play_timer, _sequence_play, 3000, TIMER_REPEAT);
			break;

		case PLAYER_STATUS_PLAY_END:
			music_box_refreshinfo(0);
			s_music_total_time = 0;
			/*sequence*/
			sequence = pmpset_get_int(PMPSET_MUSIC_PLAY_SEQUENCE);
			if(PMPSET_MUSIC_PLAY_SEQUENCE_ONLY_ONCE == sequence)
			{
				APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_END\n");
				APP_TIMER_REMOVE(music_box_refresh_timer);
				APP_Printf("play only once\n");
				music_box_ctrl(MUSIC_BOX_BUTTON_STOP);
                app_enddialog_wnd(WIN_MUSIC_VIEW);
				break;
			}
			else if(PMPSET_MUSIC_PLAY_SEQUENCE_REPEAT_ONE == sequence)
			{
                app_enddialog_after_wnd("after win_music_view");
				APP_Printf("play repeat one\n");
				music_view_mode_stop();
				if(s_music_manual_stop == true)
				{
					APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_END\n");
					APP_TIMER_REMOVE(music_box_refresh_timer);
					APP_Printf("play stop\n");
					music_box_ctrl(MUSIC_BOX_BUTTON_STOP);
				}
				else
				{
					s_music_seek_flag = MUSIC_SEEK_NONE;
					int sliderbar_init = 0;
					GUI_SetProperty(SLIDERBAR_MUSIC, "value", &sliderbar_init);
					GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &sliderbar_init);
					play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
                    music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
                    music_popmsg_topright_ctrl(music_box_button2_ctrol, 0);
                    music_box_button2_setimg(music_box_button2_ctrol);
				}
				//break;
			}
			else if(PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE == sequence)
			{
				list = play_list_get(PLAY_LIST_TYPE_MUSIC);
				 if((list->play_no >= list->nents -1)
				 	|| (s_music_manual_stop == true))
				{
					APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_END\n");
					APP_TIMER_REMOVE(music_box_refresh_timer);
					APP_Printf("play stop\n");
					music_box_ctrl(MUSIC_BOX_BUTTON_STOP);
                    if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_SET))
                    {
                        GUI_EndDialog(WIN_MUSIC_SET);
                    }
                    app_enddialog_wnd(WIN_MUSIC_VIEW);
				}
				else
				{
					APP_Printf("play next\n");
                    if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_SET))
                    {
                        GUI_EndDialog(WIN_MUSIC_SET);
                    }
                    app_enddialog_after_wnd("after win_music_view");
					s_music_seek_flag = MUSIC_SEEK_NONE;
					music_box_ctrl(MUSIC_BOX_BUTTON_NEXT);
				}
			}
			else
			{
				APP_Printf("play none\n");
				APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_END\n");
				APP_TIMER_REMOVE(music_box_refresh_timer);

				music_box_ctrl(MUSIC_BOX_BUTTON_STOP);
			}
			break;

		case PLAYER_STATUS_PLAY_RUNNING:
			s_musicErrPlayState = false;
			APP_Printf("[SERVICE] PLAYER_STATUS_PLAY_RUNNING\n");

			wnd = GUI_GetFocusWindow();
			if((wnd != NULL)
				&& (strcmp(WIN_MUSIC_VIEW, wnd) == 0)
				&&  (music_box_button2_ctrol==PLAY_MUSIC_CTROL_PLAY))
			{
				music_view_mode_start();
				lrc_timer_string_set();
				s_view_mode_bak = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
			}
			else
			{
				s_view_mode_bak = PMPSET_MUSIC_VIEW_MODE_SPECTRUM;
			}

			/*get total_time here, because get this need long time*/
			list = play_list_get(PLAY_LIST_TYPE_MUSIC);
			s_music_total_time = music_total_time_get();
            app_time_ms_to_edit_str(s_music_total_time,buf_tmp,20);
			GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);

			s_music_manual_stop = false;
			GUI_SetProperty(TEXT_STATUS, "string", " ");
			GUI_SetProperty(TEXT_STATUS, "draw_now", NULL);

			break;

		case PLAYER_STATUS_PLAY_START:
			/*popmsg centre*/
			GUI_SetProperty(TEXT_STATUS, "string", " ");
			GUI_SetProperty(TEXT_STATUS, "draw_now", NULL);
                APP_TIMER_ADD(music_box_refresh_timer, music_box_refreshinfo, 500, TIMER_REPEAT);//#39577
			break;

		default:
			break;
	}

	return 0;
}

SIGNAL_HANDLER  int music_view_canvas_init(const char* widgetname, void *usrdata)
{
	/*bean: canvas redraw lost static action*/
	spectrum_redraw();
	return EVENT_TRANSFER_STOP;
}

int music_view_spectrum_refresh(void)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
    {
        pmpset_music_view_mode view_mode = 0;
        view_mode = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);

        if((PMPSET_MUSIC_VIEW_MODE_SPECTRUM == view_mode) && (!spectrum_get_FlagForStop()))
        {
            spectrum_resume();
        }
    }
        return 0;
}

SIGNAL_HANDLER  int music_view_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	GxMsgProperty_PlayerAVCodecReport* avcodec_status = NULL;

	if(NULL == usrdata) return EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	msg = (GxMessage*)event->msg.service_msg;
	switch(msg->msg_id)
	{
		case GXMSG_PLAYER_STATUS_REPORT:
			player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
			music_view_service_status(player_status);
			break;

		case GXMSG_PLAYER_AVCODEC_REPORT:
			avcodec_status = (GxMsgProperty_PlayerAVCodecReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);
			if(AVCODEC_ERROR == avcodec_status->acodec.state)
			{
				GxMsgProperty_PlayerStatusReport player_status_fromavcodec;
				player_status_fromavcodec.status = PLAYER_STATUS_ERROR;
				music_view_service_status(&player_status_fromavcodec);
			}
            break;
		default:
			break;
	}


	return EVENT_TRANSFER_STOP;
}

extern char path_bak[255];
SIGNAL_HANDLER int music_view_init(const char* widgetname, void *usrdata)
{
	play_music_info info;
	play_list* list = NULL;
	int item_sel = 0;
	char *Path = NULL;
	bool bmode_flag = false;

#if USB_MICROPHONE_SUPPORT
    {
        extern int app_microphone_play(void);
        app_microphone_play();
    }
#endif
	/*play*/
	list = play_list_get(PLAY_LIST_TYPE_MUSIC);
	if(NULL == list)
		return GXCORE_ERROR;

	s_music_seek_flag = MUSIC_SEEK_NONE;

	/*popmsg topright*/
	GUI_SetProperty(IMAGE_POPMSG, "state", "hide");

	/*popmsg centre*/
	GUI_SetProperty(TEXT_STATUS, "state", "hide");
	app_log_debug("\n###########file_no_bak = %x ; list->play_no = %x \n", play_music_file_no_bak(), list->play_no);

	Path = explorer_static_path_strcat(list->path, list->ents[list->play_no]);
	APP_CHECK_P(Path, GXCORE_ERROR);
	if(NULL != Path)
	{
		if(0 != strncmp(path_bak, Path, 254) || ((0==strncmp(path_bak,Path,254))&&(s_musicErrPlayState==true)))
		{
			s_music_total_time=0;
			music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
			play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
		}
		else
		{
			if(PLAY_MUSIC_CTROL_PAUSE == music_box_button2_ctrol)
			{
				play_music_ctrol(PLAY_MUSIC_CTROL_RESUME);
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
			}
			else if(PLAY_MUSIC_CTROL_STOP== music_box_button2_ctrol)
			{
				s_music_total_time=0;
				play_music_ctrol(PLAY_MUSIC_CTROL_PLAY);
				music_box_button2_ctrol = PLAY_MUSIC_CTROL_PLAY;
			}
			else //playing
			{
				bmode_flag = true;
				s_music_total_time = music_total_time_get();
			}
		}
	}
    else
    {
        s_musicErrPlayState = true;
        play_av_by_stop();
        GUI_SetProperty(TEXT_STATUS, "string", STR_ID_CAN_NOT_PLAY_FILE);
        GUI_SetProperty(TEXT_STATUS, "state", "show");
        APP_TIMER_ADD(delay_play_timer, _sequence_play, 3000, TIMER_REPEAT);
    }

	play_music_get_info(&info);
	music_box_setinfo(&info);

	/*set volume*/
	int32_t audio_vol = 0;
	GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
	app_system_vol_set(audio_vol);

	/*popmsg_mute*/
	music_popmsg_mute_init();

	/*box refresh*/
	int sliderbar_init = 0;
	char buf_tmp[20]= "00:00:00";
	GUI_SetProperty(SLIDERBAR_MUSIC, "value", &sliderbar_init);//ERROT #10346
	GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &sliderbar_init);
	GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);//ERROT #14007
//	GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);

    if(false == s_musicErrPlayState)
    {
	    music_box_refresh_timer = create_timer(music_box_refreshinfo, 500, 0, TIMER_REPEAT);
    }
	item_sel = MUSIC_BOX_BUTTON_PLAY_PAUSE;
	// init music_view_boxitem2_button image state
	GUI_SetProperty(BUTTON_PLAY_STOP, "unfocus_img", IMG_BUTTON2_PAUSE_UNFOCUS);
	GUI_SetProperty(BUTTON_PLAY_STOP, "focus_img", IMG_BUTTON2_PAUSE_FOCUS);
	GUI_SetProperty(BOX_MUSIC_CTROL, "select", (void*)&item_sel);
	GUI_SetFocusWidget(BOX_MUSIC_CTROL);

	s_view_mode_bak = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
	/*view mode*/
	music_view_mode_create();
	if(bmode_flag == true) //添加这个变量因为BUG43644
	{
		music_view_mode_start();
		lrc_timer_string_set();
	}

	s_music_manual_stop = false;

// add in 20121112
	GUI_SetInterface("image_enable", NULL);
	return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int music_view_destroy(const char* widgetname, void *usrdata)
{
	/*popmsg topright*/
	APP_TIMER_REMOVE(music_popmsg_topright_timer);

	/*box refresh*/
	APP_TIMER_REMOVE(music_box_refresh_timer);

	/*sliderbar active*/
	APP_TIMER_REMOVE(music_sliderbar_active_timer);

	APP_TIMER_REMOVE(delay_play_timer);

	/*view mode*/
	music_view_mode_destroy();
	lrc_timer_string_Remove();

	s_music_seek_flag = MUSIC_SEEK_NONE;
#if MULTIMEDIA_PREVIEW_SUPPORT
    music_box_button2_ctrol = PLAY_MUSIC_CTROL_STOP;
    play_music_ctrol(PLAY_MUSIC_CTROL_STOP);
#endif

	return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int music_view_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int box_sel;
	int seeked_val = 0;

	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event *)usrdata;
	switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
	{
		case VK_BOOK_TRIGGER:
            GUI_SetProperty(TEXT_MUSIC_NAME1, "string", NULL);
			GUI_EndDialog("after wnd_full_screen");
			break;
		case APPK_BACK:
		case APPK_MENU:

			GUI_EndDialog(WIN_MUSIC_VIEW);
			return EVENT_TRANSFER_STOP;

		/*shortcut keys
*/
		case APPK_PREVIOUS:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_PREVIOUS;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
			s_music_seek_flag = MUSIC_SEEK_NONE;
			music_box_ctrl(MUSIC_BOX_BUTTON_PREVIOUS);
			return EVENT_TRANSFER_STOP;
		/*case APPK_REW:
			music_box_ctrl(MUSIC_BOX_BUTTON_SPEED_BACKWARD);
			return EVENT_TRANSFER_STOP;*/
		case APPK_PAUSE_PLAY:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_PLAY_PAUSE;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
			music_box_ctrl(MUSIC_BOX_BUTTON_PLAY_PAUSE);
			return EVENT_TRANSFER_STOP;

		case APPK_PAUSE:
			if(PLAY_MUSIC_CTROL_STOP != music_box_button2_ctrol)
			{
				GUI_SetFocusWidget(BOX_MUSIC_CTROL);
				box_sel = MUSIC_BOX_BUTTON_PLAY_PAUSE;
				GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
				music_box_ctrl(MUSIC_BOX_BUTTON_PAUSE);
			}
			return EVENT_TRANSFER_STOP;

		case APPK_PLAY:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			s_music_seek_flag = MUSIC_SEEK_NONE;
			APP_TIMER_REMOVE(music_sliderbar_active_timer);
			music_box_refreshinfo(NULL);//bug#61205
			APP_TIMER_ADD(music_box_refresh_timer, music_box_refreshinfo, 500, TIMER_REPEAT);//#39577
			box_sel = MUSIC_BOX_BUTTON_PLAY_PAUSE;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
			music_box_ctrl(MUSIC_BOX_BUTTON_PLAY);
			return EVENT_TRANSFER_STOP;
		/*case APPK_FF:
			music_box_ctrl(MUSIC_BOX_BUTTON_SPEED_FORWARD);
			return EVENT_TRANSFER_STOP;*/
		case APPK_NEXT:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_NEXT;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
			s_music_seek_flag = MUSIC_SEEK_NONE;
			music_box_ctrl(MUSIC_BOX_BUTTON_NEXT);
			return EVENT_TRANSFER_STOP;

		case APPK_STOP:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
            seeked_val = 0;
			s_music_manual_stop = true;
			box_sel = MUSIC_BOX_BUTTON_STOP;
			GUI_SetProperty(SLIDER_CURSOR_MUSIC, "value", &seeked_val);
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
			music_box_ctrl(MUSIC_BOX_BUTTON_STOP);
			return EVENT_TRANSFER_STOP;
		case APPK_RATIO:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_LRC;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
			s_music_seek_flag = MUSIC_SEEK_NONE;
			music_box_ctrl(MUSIC_BOX_BUTTON_LRC);
			return EVENT_TRANSFER_STOP;
		case APPK_SET:
			if(delay_play_timer !=NULL)
			{
				return EVENT_TRANSFER_STOP;
			}
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_SET;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
			s_music_seek_flag = MUSIC_SEEK_NONE;
			music_box_ctrl(MUSIC_BOX_BUTTON_SET);
			return EVENT_TRANSFER_STOP;
		case APPK_MUTE:
			if(delay_play_timer !=NULL)
			{
				return EVENT_TRANSFER_STOP;
			}
			music_popmsg_mute_ctrl();
			return EVENT_TRANSFER_STOP;
		case APPK_SEEK:
			GUI_SetFocusWidget(SLIDER_CURSOR_MUSIC);
			return EVENT_TRANSFER_STOP;
		case APPK_VOLUP:
		case APPK_VOLDOWN:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_VOL;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
            _create_music_volume_dlg();
			return EVENT_TRANSFER_STOP;
        case APPK_YELLOW:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_INFO;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
            _create_music_info_dlg();
            break;

		case APPK_GREEN:
			GUI_SetFocusWidget(BOX_MUSIC_CTROL);
			box_sel = MUSIC_BOX_BUTTON_VOL;
			GUI_SetProperty(BOX_MUSIC_CTROL, "select", &box_sel);
            _create_music_volume_dlg();
            return EVENT_TRANSFER_STOP;

		case APPK_REPEAT:
		{
			int value;

			value = pmpset_get_int(PMPSET_MUSIC_PLAY_SEQUENCE);
			if(value == PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE)
			{
				value = PMPSET_MUSIC_PLAY_SEQUENCE_ONLY_ONCE;
			}
			else
			{
				value++;
			}
			pmpset_set_int(PMPSET_MUSIC_PLAY_SEQUENCE, value);

			GUI_CreateDialog(WIN_MUSIC_SET);

			return EVENT_TRANSFER_STOP;
		}
#if USB_MICROPHONE_SUPPORT
        case APPK_0:
        {
            int32_t pos_y = 40;
            void app_create_mike_menu(void);
            extern int app_microphone_set_mute(int mute);
            extern int app_microphone_get_mute_flag(void);
            int value = app_microphone_get_mute_flag();
            if(value == PMPSET_MUTE_ON)
            {
                app_microphone_set_mute(value);
                spectrum_volumebak_clear();
            }

            app_create_mike_menu();
			GUI_SetProperty(WND_MIKE_VIEW, "move_window_y", &pos_y);
        }
        break;
        case APPK_1:
        {
            extern int app_microphone_set_mute(int mute);
            extern int app_microphone_get_mute_flag(void);
            int value = app_microphone_get_mute_flag();
            app_microphone_set_mute(value);
        }
        break;
        case APPK_2:
        {
            extern int app_mircophone_voice_stop(void);
            app_mircophone_voice_stop();
        }
        break;

#endif
		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

#define MUSIC_EXTERN______________________
bool music_play_state(void)
{
	if((music_box_button2_ctrol == PLAY_MUSIC_CTROL_PLAY)
		||(music_box_button2_ctrol == PLAY_MUSIC_CTROL_PAUSE))
		return true;
	return false;
}

void music_status_init(void)
{
	music_box_button2_ctrol = PLAY_MUSIC_CTROL_STOP;
}

#if HDMI_CEC_SUPPORT
play_music_ctrol_state app_get_play_music_ctrol_state(void)
{
    return music_box_button2_ctrol;
}
#endif

#if VOICE_CONTROL_SUPPORT
int app_music_play_status_get(void)
{
    if(music_box_button2_ctrol == PLAY_MUSIC_CTROL_PLAY)
        return 2;// playing
    else if(music_box_button2_ctrol == PLAY_MUSIC_CTROL_PAUSE)
        return 1;
    else if(music_box_button2_ctrol == PLAY_MUSIC_CTROL_STOP)
        return 0;
    return -1;
}
#endif


