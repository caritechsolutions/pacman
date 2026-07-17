#include "app.h"
#include "app_key.h"
#include "app_pop.h"
#include "app_epg.h"
#include "module/app_nim.h"
#include "module/app_psi.h"
#include "pmp_psi.h"
#include "pmp_dvb_subt.h"
#include "media_info.h"
#include "app_volume_value.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "app_windows.h"
#include "widget.h"
#if PARENTAL_LOCK_SUPPORT
#include"app_parent_lock.h"
#endif
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#endif
#if OTA_SUPPORT
#include "downloader/gxota_api.h"
#endif
//resourse
#define IMG_POP_STOP					"s_pvr_stop"
#define IMG_BUTTON2_PLAY_FOCUS		    "s_mpicon_play_focus"
#define IMG_BUTTON2_PLAY_UNFOCUS		"s_mpicon_play_unfocus"
#define IMG_BUTTON2_PAUSE_FOCUS		    "s_mpicon_pause_focus"
#define IMG_BUTTON2_PAUSE_UNFOCUS	    "s_mpicon_pause_unfocus"

#define IMG_POP_PREVIOUS				"MP_BUTTON_FRONT"
#define IMG_POP_PLAY					"MP_BUTTON_PLAY"
#define IMG_POP_NEXT					"MP_BUTTON_NEXT"
#define IMG_FF                          "s_pvr_ff"
#define IMG_FB                          "s_pvr_fb"
#define IMG_SPEED                          "s_pvr_speed"
#define IMG_POP_PAUSE					"s_pause"
//widget
#define IMAGE_MOVIE_TITLE               "img_movie_view_back"
#define TEXT_SUBT                 		"movie_view_canvas_subt"
#define IMAGE_POPMSG					"movie_view_image_popmsg"
#define TEXT_POPMSG					    "movie_view_text_popmsg"
#define IMAGE_BOX_BACK					"movie_view_image_boxback"
#define BOX_MOVIE_CTROL					"movie_view_box"
#define BUTTON_PLAY_STOP				"movie_view_boxitem3_button"
#define TEXT_MOVIE_NAME1				"movie_view_text_name1"
#define TEXT_START_TIME					"movie_view_text_start_time"
#define TEXT_STOP_TIME					"movie_view_text_stop_time"
#define SLIDERBAR_MOVICE				"movie_view_sliderbar"
#define SLIDER_CURSOR_MOVICE			"movie_view_sliderbar_cursor"
#define IMG_SLIDER_BACK			        "movie_view_img_sliderback"
#define COMBOBOX_POPMSG                 "movie_view_combo_popmsg"
#define IMAGE_MUTE                      "movie_view_imag_mute"
#define TEXT_MOVIE_STATUS				"movie_view_text_status"

static event_list* box_timer_hide = NULL;
static event_list* box_sliderbar_update_timer = NULL;
static event_list* box_sliderbar_active_timer = NULL;
static event_list* popmsg_topright_timer_hide = NULL;
static event_list* popmsg_centre_timer_hide = NULL;
static event_list* delay_play_timer = NULL;

static bool s_movie_video_ok = true;//mark of video decode, if err, can't support play_movie_speed
static bool s_is_movie_playing = false;

static play_movie_speed_state movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
static play_movie_ctrol_state movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
static play_movie_speed_state movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
static play_movie_zoom_state movie_box_button6_zoom = PLAY_MOVIE_ZOOM_X1;
static int64_t s_movie_total_time=0;
static int64_t s_movie_seek_min_time = 0;
static status_t s_media_playlock_state = GXCORE_ERROR;

#if MEDIA_SUBTITLE_SUPPORT
static int s_sub_delayms = 0;
static int *subt_select = NULL;
#endif
static bool s_movie_manual_stop = false;
static bool s_frist_play = true;
static bool s_pause_to_speed_play = false;// recover from pause to speed
static pmpset_movie_play_sequence s_movie_play_sequence;
static PlayerProgInfo  s_player_info;
static enum
{
    MOVIE_SEEK_NONE = 0,
    MOVIE_SEEKING,
    MOVIE_SEEKED
}s_seek_flag = MOVIE_SEEK_NONE;

static int s_start_time_ms = 0;
static int s_start_play_no = -1;
static bool s_ctl_box_show = false;

static PmpPsi s_moview_psi_info;
static int s_dvb_subt_index = 0;
extern bool music_play_state(void);
extern void music_status_init(void);
static status_t movie_box_timer_stop(void);
static status_t movie_sliderbar_timer_stop(void);
static void _media_lock_play(void);
static int movie_box_show(void);
static int app_update_psi_subt_info(void);
static status_t popmsg_topright_ctrl(play_movie_ctrol_state button, uint32_t value);
static void _movie_box_button_select(int sel);

typedef enum
{
	MOVIE_BOX_BUTTON_PREVIOUS,
	MOVIE_BOX_BUTTON_SPEED_BACKWARD,
	MOVIE_BOX_BUTTON_PLAY_PAUSE,
	MOVIE_BOX_BUTTON_SPEED_FORWARD,
	MOVIE_BOX_BUTTON_NEXT,
	MOVIE_BOX_BUTTON_STOP,
	MOVIE_BOX_BUTTON_SET,
#if PLAYBACK_SPEED_SUPPORT
    MOVIE_BOX_BUTTON_PLAYBACK_SPEED,
#else
	MOVIE_BOX_BUTTON_VOL,
#endif
#if MEDIA_SUBTITLE_SUPPORT
    MOVIE_BOX_BUTTON_SUB,
#else
	MOVIE_BOX_BUTTON_INFO,
#endif
	MOVIE_BOX_BUTTON_ZOOM,				// TODO: later
	MOVIE_BOX_BUTTON_PAUSE,
	MOVIE_BOX_BUTTON_PLAY,
}movie_box_button;

typedef enum
{
	MOVIE_COMBO_TRACK,
	MOVIE_COMBO_RATIO
}movie_combo;

typedef enum
{
	MOVE_A_DECODE_WORKING = 0,
	MOVE_V_DECODE_WORKING ,
	MOVE_A_V_DECODE_WORKING,
	MOVE_A_V_DECODE_NOT_WORKING
}avdecode_status;

int sg_AVDecodeStatus = 0;;

static int path_usb_check(const char* path)
{
    if (NULL == path)
        return GXCORE_ERROR;

    int32_t i = 0;
    char *partition_entry = NULL;
    int32_t partition_entry_len = 0;
    HotplugPartitionList *usb_partition_list = NULL;
    usb_partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);

    if(NULL == usb_partition_list || 0 == usb_partition_list->partition_num)
        return GXCORE_ERROR;

    for(i = 0; i < usb_partition_list->partition_num; i++)
    {
        partition_entry = usb_partition_list->partition[i].partition_entry;
        partition_entry_len = strlen(usb_partition_list->partition[i].partition_entry);
        if(0 == strncmp(path, partition_entry, partition_entry_len))
            return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static bool _check_playspeed_sub_support(void)
{
    if(false == s_is_movie_playing)
    {
        if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_STOP)
        {
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_INVALID;
            pop.mode = POP_MODE_UNBLOCK;
            pop.timeout_sec = 6;
            popdlg_create(&pop);
        }
        return false;
    }

    if(movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1
            || movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1
            || movie_box_button2_ctrol == PLAY_MOVIE_CTROL_STOP)
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_MODE_NOT_SUPPORT;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 6;
        popdlg_create(&pop);
        return false;
    }
    return true;
}

#if PLAYBACK_SPEED_SUPPORT
#define PLAYBACK_SPEED_DEFAULT_VALUE 2
#if (defined SCORPIO || defined SCORPIO_TAURUS)
#define PLAYBACK_SPEED_DEFAULT_MAX 4
#define PLAYBACK_SPEED_MAX_SPEED 1.5
static char* strArray[] = {"0.5X","0.75X","1X","1.25X","1.5X"};
#else
#define PLAYBACK_SPEED_DEFAULT_MAX 6
#define PLAYBACK_SPEED_MAX_SPEED 1.99
static char* strArray[] = {"0.5X","0.75X","1X","1.25X","1.5X","1.75X","2X"};
#endif

static int s_playback_speed_sel = PLAYBACK_SPEED_DEFAULT_VALUE;
static void _movie_playback_set_select(int sel)
{
    s_playback_speed_sel = sel;
}

static int _movie_playback_get_select(void)
{
    return s_playback_speed_sel;
}

static int _movie_playback_set_speed(int select)
{
	GxMsgProperty_PlayerSpeed player_speed;
	GxMessage *msg;

    if(select >= PLAYBACK_SPEED_DEFAULT_MAX)
        player_speed.speed = PLAYBACK_SPEED_MAX_SPEED; //The fastest playback speed is currently 1.99 times the original speed
    else
        player_speed.speed = (select+2)*0.25;

    msg = GxBus_MessageNew(GXMSG_PLAYER_SPEED);
    APP_CHECK_P(msg, GXCORE_ERROR);
    GxMsgProperty_PlayerSpeed *spd;
    spd = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerSpeed);
    APP_CHECK_P(spd, GXCORE_ERROR);
    spd->player = PMP_PLAYER_AV;
    spd->speed = player_speed.speed;
    GxBus_MessageSendWait(msg);
    GxBus_MessageFree(msg);
    return 0;
}

static int _playback_speed_poplist_exit_cb(int select,unsigned short key)
{
    if(key == STBK_OK)
    {
        if(_movie_playback_get_select() != select)
        {
            _movie_playback_set_speed(select);
            popmsg_topright_ctrl(PLAY_MOVIE_PLAYBACK_SPEED,select);
            _movie_playback_set_select(select);
        }
    }
    return 0;
}

static int _playback_speed_poplist(int sel)
{
    PopList pop_list;
    int ret =-1;

    memset(&pop_list, 0, sizeof(PopList));
    pop_list.title = STR_ID_PLAYBACK_SPEED;
    pop_list.item_num = (PLAYBACK_SPEED_DEFAULT_MAX+1);
    pop_list.item_content = strArray;
    pop_list.sel = sel;
    pop_list.show_num= false;
    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _playback_speed_poplist_exit_cb;
    pop_list.pos.x = 560;
    pop_list.pos.y = 160;
    ret = poplist_create(&pop_list);
    return ret;
}
#endif

#if MEDIA_SUBTITLE_SUPPORT
int app_movie_dvb_subt_cnt(void)
{
    int cnt = 0;
    if(s_moview_psi_info.valid_flag > 0)
    {
        cnt = s_moview_psi_info.subt_info.num;
    }
    return cnt;
}

static int app_get_subt_delay_time(void)
{
    pmp_subt_para* subt_para = NULL;
    subt_para = subtitle_get();
    if(subt_para != NULL)
    {
        return subt_para->delay_ms;
    }
    return 0;
}

static int app_set_subt_delay_time(int delay_ms)
{
    pmp_subt_para* subt_para = NULL;
    subt_para = subtitle_get();
    if(subt_para != NULL)
    {
        subt_para->delay_ms = delay_ms;
        GxPlayer_MediaSubSync(PMP_PLAYER_AV, subt_para->list,  subt_para->delay_ms);
    }
    return 0;
}

int app_movie_dvb_subt_index(void)
{
    return s_dvb_subt_index;
}

status_t app_movie_dvb_subt_switch(int index)
{
    if(index >= s_moview_psi_info.subt_info.num)
        return GXCORE_ERROR;

    s_dvb_subt_index = index;
    movie_dvb_subt_switch(s_dvb_subt_index);

    return GXCORE_SUCCESS;
}

int app_get_subt_real_select(int select)
{
    if(subt_select != NULL)
        return subt_select[select];
    else
        return 0;
}

int app_get_subt_real_type(pmp_subt_load_type type, int select)
{
    int i = 0;
    if(type == PMP_SUBT_LOAD_OUTSIDE)
    {
        PlayerProgInfo *player_info = app_player_prog_info_get();
        if(GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, player_info) == GXCORE_SUCCESS) //success
        {
            memcpy(&s_player_info, player_info, sizeof(PlayerProgInfo));
            app_update_psi_subt_info();
        }
        for(i = player_info->sub_num; i > 0 ; i-- )
        {
            if(player_info->sub[i-1].type == PLAYER_SUB_TYPE_FILE)
            {
                if( player_info->sub[i-1].encode != PLAYER_SUB_ENC_SPU)
                    return 1;
                else
                    return 0;
            }
        }
    }
    else
    {
        if((s_player_info.sub_num == 0) || (subt_select == NULL))
            return 0;
        if(s_player_info.sub[subt_select[select]].type == PLAYER_SUB_TYPE_FILE)
            return 0;
        if(s_player_info.sub[subt_select[select]].encode != PLAYER_SUB_ENC_SPU)
            return 1;
    }
    return 0;
}

void app_clean_subt_select(void)
{
    APP_FREE(subt_select);
}

status_t app_update_subt_info(PlayerProgInfo *player_info)
{
    uint32_t sub_num = 0;
    int i = 0;
    if(player_info == NULL || player_info->sub_num == 0)
    {
        return GXCORE_ERROR;
    }
    if(subt_select == NULL)
    {
        subt_select = (int*)GxCore_Calloc(PLAYER_MAX_TRACK_SUB, sizeof(int));
    }
    else
    {
        memset(subt_select, 0, PLAYER_MAX_TRACK_SUB*sizeof(int));
    }
    PlayerSubTrack *subt_info = (PlayerSubTrack*)GxCore_Calloc(player_info->sub_num, sizeof(PlayerSubTrack));
    if(subt_info == NULL || subt_select == NULL)
        return GXCORE_ERROR;
    memcpy(subt_info,player_info->sub, player_info->sub_num*sizeof(PlayerSubTrack));
    for(i = 0; i < player_info->sub_num; i++)
    {
        if(subt_info[i].type == PLAYER_SUB_TYPE_DVB_TTX || subt_info[i].type == PLAYER_SUB_TYPE_DVB_MAG)
        {
            continue;
        }
        else
        {
            memcpy(&player_info->sub[sub_num], &subt_info[i], sizeof(PlayerSubTrack));
            subt_select[sub_num] = i;
            sub_num++;
        }
    }
    player_info->sub_num = sub_num;
    GxCore_Free(subt_info);
    return GXCORE_SUCCESS;
}

#endif

void app_movie_psi_info_clean(void)
{
    app_audio_lang_free(&s_moview_psi_info.audio_info);

    if(s_moview_psi_info.subt_info.data != NULL)
    {
        GxCore_Free(s_moview_psi_info.subt_info.data);
    }

    if(s_moview_psi_info.ttx_info.data != NULL)
    {
        GxCore_Free(s_moview_psi_info.ttx_info.data);
    }

    memset(&s_moview_psi_info, 0 , sizeof(s_moview_psi_info));
}

static int app_update_psi_subt_info(void)
{
    char valid_flag = s_moview_psi_info.valid_flag;

    if(s_moview_psi_info.subt_info.data != NULL)
    {
        GxCore_Free(s_moview_psi_info.subt_info.data);
    }

    memset(&(s_moview_psi_info.subt_info), 0 , sizeof(SubtTTxInfo));
    if(s_moview_psi_info.ttx_info.data != NULL)
    {
        GxCore_Free(s_moview_psi_info.ttx_info.data);
    }
    memset(&(s_moview_psi_info.ttx_info), 0 , sizeof(SubtTTxInfo));

#if MEDIA_SUBTITLE_SUPPORT
    app_update_subt_info(&s_player_info);
#endif
    s_moview_psi_info.subt_info.num = s_player_info.sub_num;
    if(s_moview_psi_info.subt_info.num > 0)
    {
        s_moview_psi_info.subt_info.data = (ttx_subt*)GxCore_Calloc(s_moview_psi_info.subt_info.num, sizeof(ttx_subt));
        if(s_moview_psi_info.subt_info.data != NULL)
        {
            int i;
            for(i = 0; i < s_moview_psi_info.subt_info.num; i++)
            {
                s_moview_psi_info.subt_info.data[i].elem_pid = s_player_info.sub[i].pid.pid;
                strncpy((char*)s_moview_psi_info.subt_info.data[i].lang, s_player_info.sub[i].lang, sizeof(s_moview_psi_info.subt_info.data[i].lang));
                s_moview_psi_info.subt_info.data[i].type = PLAYER_SUB_TYPE_DVB;
                s_moview_psi_info.subt_info.data[i].subt.composite_page_id = s_player_info.sub[i].pid.major;
                s_moview_psi_info.subt_info.data[i].subt.ancillary_page_id = s_player_info.sub[i].pid.minor;
            }
        }
    }

    s_moview_psi_info.valid_flag = valid_flag;
    return 0;
}

void movie_player_back_ground_set(void)
{
    if(s_player_info.is_radio == 1)
        app_set_window_back_ground(WIN_MOVIE_VIEW, NULL, true);
    else
        app_unset_window_back_ground(WIN_MOVIE_VIEW, true, true);
}

status_t app_movie_prog_info_update(char *file_path)
{
    if(file_path == NULL)
        return GXCORE_ERROR;
    if(GxPlayer_MediaGetProgInfoByUrl(file_path, &s_player_info) != 0)
    {
        app_log_error("[%s]get prog info err !!!\n", __func__);
        return GXCORE_ERROR;
    }
    return GXCORE_SUCCESS;
}

PlayerProgInfo* app_movie_player_info_get()
{
    return &s_player_info;
}

status_t app_movie_psi_info_init(char *file_path)
{
    SectionParseCb pat_parse_cb;
    PATInfo pat_info = {0};
    SectionParseCb pmt_parse_cb;
    PMTInfo pmt_info = {0};
    PATProgData *cur_prog = NULL;
    int cnt = 0;

    app_movie_psi_info_clean();
    if(file_path == NULL)
        return GXCORE_ERROR;

    memset(&s_moview_psi_info, 0 , sizeof(s_moview_psi_info));

    pat_parse_cb.cb_data = (void*)&pat_info;
    pat_parse_cb.cb_proc = pat_section_proc;
    if(app_tsfile_section_parse(file_path, 0, &pat_parse_cb) == GXCORE_ERROR)
        return GXCORE_ERROR;

    if((pat_info.prog_num == 0) || (pat_info.prog_data == NULL))
        return GXCORE_ERROR;

    for(cnt = 0; cnt < pat_info.prog_num; cnt++)
    {
        cur_prog = &pat_info.prog_data[cnt];
        pmt_info.service_id =  cur_prog->service_id;
        pmt_parse_cb.cb_data = (void*)&pmt_info;
        pmt_parse_cb.cb_proc = pmt_section_proc;
        if(app_tsfile_section_parse(file_path, cur_prog->pmt_pid, &pmt_parse_cb) == GXCORE_SUCCESS)
        {
            app_parse_pmt_video(&pmt_info, &s_moview_psi_info.video_info.video_pid, &s_moview_psi_info.video_info.video_type);
            app_parse_pmt_audio(&pmt_info, &s_moview_psi_info.audio_info);
            app_parse_pmt_private(&pmt_info, &s_moview_psi_info.private_info);
            pmt_data_release(&pmt_info);
            app_update_psi_subt_info();
            s_moview_psi_info.valid_flag = 1;
            break;
        }
    }
    s_moview_psi_info.ts_id = pat_info.ts_id;
    s_moview_psi_info.service_id = cur_prog->service_id;
    pat_data_release(&pat_info);
    cur_prog = NULL;

    return GXCORE_SUCCESS;
}

PmpPsi *app_movie_psi_info_get(void)
{
    return &s_moview_psi_info;
}

status_t app_movie_play_config(void)
{
    PlayerPlayConfig config;
    if(s_moview_psi_info.valid_flag > 0)
    {
        memset(&config, 0, sizeof(PlayerPlayConfig));
        config.tsfilter.vpid = s_moview_psi_info.video_info.video_pid;
        if(s_moview_psi_info.audio_info.audioLangCnt > 0)
        {
            config.tsfilter.apid = s_moview_psi_info.audio_info.audioLangList[0].id;
        }

        s_dvb_subt_index = 0;
        GxPlayer_MediaPlayConfig(PMP_PLAYER_AV, &config);
    }

    return GXCORE_SUCCESS;
}

static int app_movie_pause_allowed(void)
{
	int ret = 0;
	PlayTimeInfo time_info = {0};
	if(s_is_movie_playing)
	{
		ret = 1;
		memset(&time_info, 0, sizeof(PlayTimeInfo));
		if(GXCORE_SUCCESS == GxPlayer_MediaGetTime(PMP_PLAYER_AV, &time_info))
		{
			if((time_info.current>0)&&(time_info.totle>0)&&(time_info.current<=time_info.totle))
			{
				if((time_info.totle-time_info.current)<1000)
				{
					ret = 0;
				}
			}
		}
	}
	return ret;
}

static status_t movie_box_button2_setimg(play_movie_ctrol_state state);

static void app_movie_view_create(void)
{
	if(GUI_CheckDialog(WIN_MEDIA_BAR) != GXCORE_SUCCESS)
	{
		GUI_CreateDialog(WIN_MEDIA_BAR);
		if((movie_box_button3_fspeed == PLAY_MOVIE_SPEED_X1)
				&& (movie_box_button1_bspeed == PLAY_MOVIE_SPEED_X1))
		{
			s_pause_to_speed_play = false;
			movie_box_button2_setimg(movie_box_button2_ctrol);
		}

		if(box_timer_hide != NULL)
		{
			reset_timer(box_timer_hide);
		}

	}
	return ;
}

static void app_movie_view_end_dialog(void)
{
	if(GUI_CheckDialog(WIN_MEDIA_BAR) == GXCORE_SUCCESS)
	{
        GUI_SetProperty(TEXT_MOVIE_NAME1, "rolling_stop", NULL);
        GUI_EndDialog(WIN_MEDIA_BAR);
        GUI_SetInterface("flush", NULL);
	}
#if OTA_SUPPORT
    if(GxOTA_isTrigger() != true)
    {
        popdlg_destroy();
    }
#else
	popdlg_destroy();
#endif
	return ;
}

static int popmsg_topright_hide(void *userdata)
{
	GUI_SetProperty(IMAGE_POPMSG, "state", "osd_trans_hide");
	GUI_SetProperty(TEXT_POPMSG, "state", "hide");
	APP_TIMER_REMOVE(popmsg_topright_timer_hide);
#if PLAYBACK_SPEED_SUPPORT
    if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_RESUME || movie_box_button2_ctrol == PLAY_MOVIE_CTROL_PLAY)
        popmsg_topright_ctrl(PLAY_MOVIE_PLAYBACK_SPEED,_movie_playback_get_select());
#endif
	return 0;
}

static status_t popmsg_topright_ctrl(play_movie_ctrol_state button, uint32_t value)
{
	int timeout = 3000;

	if(popmsg_topright_timer_hide)
	{
		app_log_info("[MOVIE] popmsg_topright_timer_hide remove it\n");
		APP_TIMER_REMOVE(popmsg_topright_timer_hide);
	}

	GUI_SetProperty(IMAGE_POPMSG, "state", "show");
    GUI_SetProperty(TEXT_POPMSG, "state", "hide");

	switch(button)
	{
		case PLAY_MOVIE_CTROL_PREVIOUS:
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PREVIOUS);
			break;

		case  PLAY_MOVIE_CTROL_BACKWARD:
			if(PLAY_MOVIE_SPEED_X1== value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X1");
				GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PLAY);
				break;
			}

			timeout = 0;
			if(PLAY_MOVIE_SPEED_X2 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X2");
			}
			else if(PLAY_MOVIE_SPEED_X4 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X4");
			}
			else if(PLAY_MOVIE_SPEED_X8 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X8");
			}
			else if(PLAY_MOVIE_SPEED_X16 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X16");
			}
			else if(PLAY_MOVIE_SPEED_X32 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X32");
			}
            GUI_SetProperty(IMAGE_POPMSG, "img", IMG_FB);
            GUI_SetProperty(TEXT_POPMSG, "state", "show");
			break;

		case PLAY_MOVIE_CTROL_RESUME:
		case PLAY_MOVIE_CTROL_PLAY:
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PLAY);
			break;


		case PLAY_MOVIE_CTROL_PAUSE:
			timeout = 0;
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PAUSE);
			break;


		case PLAY_MOVIE_CTROL_FORWARD:
			if(PLAY_MOVIE_SPEED_X1== value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X1");
				GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PLAY);
				break;
			}

			timeout = 0;
			if(PLAY_MOVIE_SPEED_X2 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X2");
			}
			else if(PLAY_MOVIE_SPEED_X4 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X4");
			}
			else if(PLAY_MOVIE_SPEED_X8 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X8");
			}
			else if(PLAY_MOVIE_SPEED_X16 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X16");
			}
			else if(PLAY_MOVIE_SPEED_X32 == value)
			{
				GUI_SetProperty(TEXT_POPMSG, "string", "X32");
			}
            GUI_SetProperty(IMAGE_POPMSG, "img", IMG_FF);
            GUI_SetProperty(TEXT_POPMSG, "state", "show");
			break;


		case PLAY_MOVIE_CTROL_NEXT:
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_NEXT);
			break;

		case PLAY_MOVIE_CTROL_STOP:
			timeout = 0;
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_STOP);
			break;

#if PLAYBACK_SPEED_SUPPORT
        case PLAY_MOVIE_PLAYBACK_SPEED:
			if(value == PLAYBACK_SPEED_DEFAULT_VALUE)
			{
                GUI_SetProperty(IMAGE_POPMSG, "state", "hide");
                GUI_SetProperty(TEXT_POPMSG, "state", "hide");
			}
            else
            {
                timeout = 0;
                if(value == 0)
                    GUI_SetProperty(TEXT_POPMSG, "string", "0.5X");
                else if(value == 1)
                    GUI_SetProperty(TEXT_POPMSG, "string", "0.75X");
                else if(value == 3)
                    GUI_SetProperty(TEXT_POPMSG, "string", "1.25X");
                else if(value == 4)
                    GUI_SetProperty(TEXT_POPMSG, "string", "1.5X");
                else if(value == 5)
                    GUI_SetProperty(TEXT_POPMSG, "string", "1.75X");
                else
                    GUI_SetProperty(TEXT_POPMSG, "string", "2X");
                GUI_SetProperty(IMAGE_POPMSG, "img", IMG_SPEED);
                GUI_SetProperty(TEXT_POPMSG, "state", "show");
            }
            break;
#endif

		case  PLAY_MOVIE_CTROL_ZOOM:
			timeout = 0;
			break;

		default:
			break;
	}
// add in 20121023
	//GUI_SetInterface("flush", NULL);
	if(timeout)
	{
		if(popmsg_topright_timer_hide)
		{
			app_log_error("[MOVIE] popmsg_topright_timer_hide is already created\n");
			return GXCORE_ERROR;
		}

		popmsg_topright_timer_hide = create_timer(popmsg_topright_hide, timeout, 0, TIMER_REPEAT);
	}

	return GXCORE_SUCCESS;
}

static int popmsg_centre_hide(void* usrdata)
{
	GUI_SetProperty(COMBOBOX_POPMSG, "state", "osd_trans_hide");
	APP_TIMER_REMOVE(popmsg_centre_timer_hide);
	return 0;
}

status_t popmsg_centre_ctrl(movie_combo value_sel)
{
	uint32_t value = 0;

	switch (value_sel)
	{
		case MOVIE_COMBO_TRACK:
			value = pmpset_get_int(PMPSET_AUDIO_TRACK);
			if(value==PMPSET_AUDIO_TRACK_RIGHT)
			{
				value=PMPSET_AUDIO_TRACK_STEREO;
			}
			else
			{
				value+=1;
			}
			pmpset_set_int(PMPSET_AUDIO_TRACK, value);
			GUI_SetProperty(COMBOBOX_POPMSG, "select", (void*)&value);
			break;

		case MOVIE_COMBO_RATIO:
			value = pmpset_get_int(PMPSET_ASPECT_RATIO);
			if(value==PMPSET_ASPECT_RATIO_16_9)
			{
				value=PMPSET_ASPECT_RATIO_AUTO;
			}
			else
			{
				value+=1;
			}
			pmpset_set_int(PMPSET_ASPECT_RATIO, value);
			value+=3;
			GUI_SetProperty(COMBOBOX_POPMSG, "select", (void*)&value);
			break;

		default:
			return GXCORE_ERROR;
			break;
	}

	GUI_SetProperty(COMBOBOX_POPMSG, "state", "show");

	/*timer*/
	if(popmsg_centre_timer_hide)
	{
		reset_timer(popmsg_centre_timer_hide);
	}
	else
	{
		popmsg_centre_timer_hide = create_timer(popmsg_centre_hide, 3000, 0, TIMER_REPEAT);
	}
	return GXCORE_SUCCESS;
}

status_t popmsg_mute_init(void)
{
	uint32_t value = 0;

	value=pmpset_get_int(PMPSET_MUTE);

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

status_t popmsg_mute_ctrl(void)
{
	uint32_t value = 0;

	value=pmpset_get_int(PMPSET_MUTE);

	switch (value)
	{
		case PMPSET_MUTE_ON:
			pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
			GUI_SetProperty(IMAGE_MUTE, "state", "hide");
			break;

		case PMPSET_MUTE_OFF:
			pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_ON);
			GUI_SetProperty(IMAGE_MUTE, "state", "show");
			break;

		default:
			return GXCORE_ERROR;
			break;
	}
	return GXCORE_SUCCESS;
}

void app_movie_unmute_change(void)
{
    uint32_t value = 0;
	value=pmpset_get_int(PMPSET_MUTE);
    if(value == PMPSET_MUTE_ON)
    {
        pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
        GUI_SetProperty(IMAGE_MUTE, "state", "hide");
    }
}

static status_t movie_box_setinfo(play_movie_info* info)
{
	uint64_t cur_time_ms = 0;
	uint64_t total_time_ms = 0;
	int cur_sliderbar = 0;
	char buf_tmp[20] = {0};
	status_t ret = GXCORE_ERROR;
	PlayTimeInfo  tinfo = {0};

	if(GXCORE_ERROR == GUI_CheckDialog(WIN_MEDIA_BAR))
		return GXCORE_ERROR;
	/*file name*/
	if(NULL == info)return GXCORE_ERROR;

    char *old_movie_name = NULL;
    GUI_GetProperty(TEXT_MOVIE_NAME1, "string", &old_movie_name);
    if ((old_movie_name == NULL) || (strcmp(old_movie_name, info->name) != 0))
    {
        GUI_SetProperty(TEXT_MOVIE_NAME1, "string", info->name);
    }

	ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &tinfo);
	if(GXCORE_SUCCESS == ret)
	{
        s_movie_seek_min_time = tinfo.seek_min;
		cur_time_ms = tinfo.current - s_movie_seek_min_time;
		total_time_ms = tinfo.totle - s_movie_seek_min_time;
		s_movie_total_time = total_time_ms;
		if(0 >= s_movie_total_time)
		{
			s_movie_total_time = 0;
            s_movie_seek_min_time = 0;
			cur_time_ms = 0;
			cur_sliderbar = 0;
		}
		else if(cur_time_ms > s_movie_total_time)
		{
			cur_time_ms = s_movie_total_time;
			cur_sliderbar = 100;
		}
		else
			cur_sliderbar = (cur_time_ms * 100)/ s_movie_total_time ;

        GUI_SetProperty(SLIDERBAR_MOVICE, "value", &cur_sliderbar);


		/*sliderbar*/
        if((s_seek_flag == MOVIE_SEEK_NONE) || (s_seek_flag == MOVIE_SEEKED))
        {
            if(s_seek_flag == MOVIE_SEEK_NONE)
            {
                GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &cur_sliderbar);
            }
            else
            {
                int seeked_val = 0;
                GUI_GetProperty(SLIDER_CURSOR_MOVICE, "value", &seeked_val);
                GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &seeked_val);
            }
            /*cur time*/
            app_time_ms_to_edit_str(cur_time_ms,buf_tmp,20);
			GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);

        }

        /*total time*/
        app_time_ms_to_edit_str(s_movie_total_time,buf_tmp,20);
		GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);
    }
	else
	{
		if(delay_play_timer != NULL)
		{
			int seeked_val = 0;
			GUI_SetProperty(SLIDERBAR_MOVICE, "value", &seeked_val);
			GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &seeked_val);
		}
	}

	return GXCORE_SUCCESS;
}


static status_t movie_box_button2_setimg(play_movie_ctrol_state state)
{
	if((PLAY_MOVIE_CTROL_PAUSE == state)||(PLAY_MOVIE_CTROL_STOP == state))
	{
		GUI_SetProperty(BUTTON_PLAY_STOP, "unfocus_img", IMG_BUTTON2_PLAY_UNFOCUS);
		GUI_SetProperty(BUTTON_PLAY_STOP, "focus_img", IMG_BUTTON2_PLAY_FOCUS);
	}
	else if((PLAY_MOVIE_CTROL_PLAY== state)||(PLAY_MOVIE_CTROL_RESUME== state))
	{
		GUI_SetProperty(BUTTON_PLAY_STOP, "unfocus_img", IMG_BUTTON2_PAUSE_UNFOCUS);
		GUI_SetProperty(BUTTON_PLAY_STOP, "focus_img", IMG_BUTTON2_PAUSE_FOCUS);
	}
	else
	{
		return GXCORE_ERROR;
	}
	GUI_SetProperty(BOX_MOVIE_CTROL, "update", NULL); //平台boxitem里的button重置focus图片的bug，修改后可删除此行代码

	return GXCORE_SUCCESS;
}



static status_t movie_box_timer_restart(void)
{
	reset_timer(box_timer_hide);
	return GXCORE_SUCCESS;
}

static int movie_sliderbar_update(void* userdata)
{
	play_movie_info cur_info;

	/*set info*/
	play_movie_get_info(&cur_info);
	movie_box_setinfo(&cur_info);
	return 0;
}

static status_t movie_sliderbar_timer_start(void)
{
	APP_TIMER_ADD(box_sliderbar_update_timer, movie_sliderbar_update, 200, TIMER_REPEAT);
	return GXCORE_SUCCESS;
}

static status_t movie_sliderbar_timer_stop(void)
{
	APP_TIMER_REMOVE(box_sliderbar_update_timer);
	return GXCORE_SUCCESS;
}

static status_t movie_box_timer_stop(void)
{
	APP_TIMER_REMOVE(box_timer_hide);
	return GXCORE_SUCCESS;
}

int movie_box_hide(void* userdata)
{
	int ret_value = 0;

    if(false == s_ctl_box_show)
        return 0;

	s_seek_flag = MOVIE_SEEK_NONE;
	APP_TIMER_REMOVE(box_sliderbar_active_timer);
	movie_sliderbar_update(NULL);
	movie_box_timer_stop();
	movie_sliderbar_timer_stop();
	if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_STOP)
	{
		int sliderbar_init = 0;
		GUI_SetProperty(SLIDERBAR_MOVICE, "value", &sliderbar_init);
		GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &sliderbar_init);
	}
	app_movie_view_end_dialog();
    ret_value = EVENT_TRANSFER_KEEPON;
    s_ctl_box_show = false;

	return ret_value;
}

static status_t movie_box_timer_start(void)
{
	APP_TIMER_ADD(box_timer_hide, movie_box_hide, 10000, TIMER_REPEAT);
	return GXCORE_SUCCESS;
}

static void movie_view_info_get(MediaInfo *media_info)
{
	static char str_size[20] = {0};
	static char str_res[20] = {0};
	static char str_bit[20] = {0};
	static char str_fps[20] = {0};
#if HIGH_DYNAMIC_RANGE_SUPPORT
	#define APP_MOVIE_VIEW_HDR_TYPE_TOTAL  (5)  // same as app_channel_info.c #define APP_HDR_TYPE_TOTAL  (5)
	static char str_code_hdr[40] = {0};
	char* hdr_type_str[APP_MOVIE_VIEW_HDR_TYPE_TOTAL] = {" ", "HLG", "HDR10","HDR10_PLUS","DOLBYVISION"};//0 is sdr , do not display
	int type = 0 ;
	int flag = 0 ;
    extern int app_get_hdr_enable(void);
#endif

	play_movie_info info;
	play_movie_get_info(&info);

	PlayerProgInfo *play_info = app_player_prog_info_get();

	GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, play_info);
	if(s_movie_manual_stop&&(s_player_info.file_size>=0))
	{
		memcpy(play_info, &s_player_info, sizeof(PlayerProgInfo));
	}

	media_info->line[0].subt = "File name:";
	media_info->line[0].info = info.name;

	media_info->line[1].subt = "File size:";
	if(play_info->file_size)
	{
		memset(str_size,0,sizeof(str_size));
        play_av_display_size_str(play_info->file_size,str_size);
        media_info->line[1].info = str_size;
	}

	media_info->line[2].subt = "Resolution:";
	if(play_info->video.width)
	{
		memset(str_res,0,sizeof(str_res));
		sprintf(str_res, "%d x %d", play_info->video.width,play_info->video.height);
		media_info->line[2].info = str_res;
	}

	media_info->line[3].subt = "Code format:";
#if HIGH_DYNAMIC_RANGE_SUPPORT
    if (app_get_hdr_enable())
    {
        app_log_info("[app] HDR display: type=%d, flag=%d \n",play_info->video.dr.type,play_info->video.dr.dolby_flag);
        type = play_info->video.dr.type ;
        if ( (type > (APP_MOVIE_VIEW_HDR_TYPE_TOTAL-1) ) || (type < 0 ) )
            type = 0 ;
#if (DOLBY_SUPPORT == 0)
        if ( type == (APP_MOVIE_VIEW_HDR_TYPE_TOTAL-1) ) //DOLBYVISION need copyright
        {
            type = 0 ;
        }
#endif

        if ( type > 0 )
        {
            memset(str_code_hdr,0,sizeof(str_code_hdr));
            if(play_info->video.codec_id)
            {
                flag = play_info->video.codec_id[0] ;
                if (flag == 0)
                {
                    sprintf(str_code_hdr, "%s",hdr_type_str[type]);
                }
                else
                {
                    sprintf(str_code_hdr, "%s  %s",play_info->video.codec_id,hdr_type_str[type]);
                }
            }
            else
            {
                sprintf(str_code_hdr, "%s",hdr_type_str[type]);
            }
            media_info->line[3].info = str_code_hdr;
        }
        else
        {
            if(play_info->video.codec_id)
                media_info->line[3].info = play_info->video.codec_id;
        }
    }
    else
    {
	    if(play_info->video.codec_id)
		    media_info->line[3].info = play_info->video.codec_id;
    }
#else
	if(play_info->video.codec_id)
		media_info->line[3].info = play_info->video.codec_id;
#endif

	media_info->line[4].subt = "Bit rate:";
	if(play_info->video.bitrate)
	{
		memset(str_bit,0,sizeof(str_bit));
		sprintf(str_bit, "%d bps",play_info->bitrate);
		media_info->line[4].info = str_bit;
	}

	media_info->line[5].subt = "Frame rate:";
	if(play_info->video.fps)
	{
		memset(str_fps,0,sizeof(str_fps));
		sprintf(str_fps, "%3.1f fps", play_info->video.fps);
		media_info->line[5].info = str_fps;
	}
}

static char* app_movie_view_get_movie_path(void)
{
    char* path = NULL;
    play_list* list = NULL;

    list = play_list_get(PLAY_LIST_TYPE_MOVIE);
    if((list != NULL)&&(list->play_no < list->nents))
    {
        path = explorer_static_path_strcat(list->path, list->ents[list->play_no]);
    }
    return path;
}

static bool _check_ff_rew_support(void)
{
    PopDlg pop;
    if(!s_is_movie_playing)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_INVALID;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 6;
        popdlg_create(&pop);
        return false;
    }
    if(PLAY_MOVIE_CTROL_STOP == movie_box_button2_ctrol)
        return false;
#if PLAYBACK_SPEED_SUPPORT
    if(_movie_playback_get_select() != PLAYBACK_SPEED_DEFAULT_VALUE)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_MODE_NOT_SUPPORT;
        popdlg_create(&pop);
        return false;
    }
#endif
    if(s_player_info.is_radio == 1 || s_movie_video_ok == false)//radio not support
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_FILE_NOT_SUPPORT;
        popdlg_create(&pop);
        return false;
    }
    return true;
}

static void _close_media_bar_for_submenu(void)
{
    movie_box_hide(NULL);
    if(popmsg_centre_timer_hide)
    {
        APP_TIMER_REMOVE(popmsg_centre_timer_hide);
        GUI_SetProperty(COMBOBOX_POPMSG, "state", "osd_trans_hide");
        GUI_SetProperty(COMBOBOX_POPMSG, "draw_now", NULL);
    }
}

static void _create_media_set_dlg(void)
{
    if(movie_box_button1_bspeed == PLAY_MOVIE_SPEED_X1
            &&movie_box_button3_fspeed == PLAY_MOVIE_SPEED_X1//Normal Play
            &&(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_PLAY
#if MEDIA_SUBTITLE_SUPPORT
                ||movie_box_button2_ctrol == PLAY_MOVIE_CTROL_RESUME
#endif
              ))//Don't support setting When pause
    {

        _close_media_bar_for_submenu();
        GUI_CreateDialog(WIN_MOVIE_SET);
    }
    else
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_INVALID;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 6;
        popdlg_create(&pop);
    }
}

static void _create_media_info_dlg(void)
{
    MediaInfo media_info;
    _close_media_bar_for_submenu();
    memset(&media_info, 0, sizeof(media_info));

    movie_view_info_get(&media_info);
    media_info.change_cb = movie_view_info_get;
    media_info.destroy_cb = NULL;
    media_info_create(&media_info);
}

static void _create_media_volume_dlg(void)
{
     _close_media_bar_for_submenu();
    if(pmpset_get_int(PMPSET_MUTE) == PMPSET_MUTE_ON)
    {
        pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
        GUI_SetProperty(IMAGE_MUTE, "state", "osd_trans_hide");
    }
    GUI_CreateDialog(WND_VOLUME);
}

static void _create_media_sub_dlg(void)
{
    if(_check_playspeed_sub_support() == false)
        return;
    _close_media_bar_for_submenu();
    GUI_CreateDialog(WIN_MOVIE_SUBT);
}

status_t movie_box_ctrl(movie_box_button button)
{
	int box_sel = 0;
	status_t ret = GXCORE_SUCCESS;
    PlayTimeInfo time_info = {0};
    char* path = NULL;

	uint32_t sliderbar_value = 0;

	switch(button)
	{
		case MOVIE_BOX_BUTTON_PREVIOUS:
			APP_TIMER_REMOVE(delay_play_timer);
			GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "hide");
			GUI_SetProperty(SLIDERBAR_MOVICE, "value", &sliderbar_value);
			/*reset speed state*/
			movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
			movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
			/*reset play state*/
			if(PLAY_MOVIE_CTROL_PLAY != movie_box_button2_ctrol)
			{
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
				movie_box_button2_setimg(movie_box_button2_ctrol);
			}
			/*reset zoom state*/
			if(PLAY_MOVIE_ZOOM_X1 != movie_box_button6_zoom)
			{
				movie_box_button6_zoom = PLAY_MOVIE_ZOOM_X1;
				//TODO: if gxplayer not reset, reset it
				play_movie_zoom(movie_box_button6_zoom);
			}


			/*previous*/
            path = app_movie_view_get_movie_path();
            ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &time_info);
            if((ret != GXCORE_ERROR) &&
                (0 < time_info.current - time_info.seek_min) && (0 <  s_movie_total_time) && (time_info.current - time_info.seek_min != s_movie_total_time))
            {
                pmp_save_tag(path, time_info.current);
            }
			s_movie_total_time=0;
            s_movie_seek_min_time = 0;
            s_frist_play = true;
			play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
#if PARENTAL_LOCK_SUPPORT
            if(s_moview_psi_info.private_info.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE)
            {
                app_pvr_file_parental_rating_stop();
            }
#endif
            ret =play_movie_ctrol(PLAY_MOVIE_CTROL_PREVIOUS);
            popmsg_topright_ctrl(PLAY_MOVIE_CTROL_PREVIOUS, 0);
            s_start_time_ms = 0;
            pmp_save_tag(path, s_start_time_ms);
            _media_lock_play();
            break;

		case MOVIE_BOX_BUTTON_SPEED_BACKWARD:
            if(_check_ff_rew_support() == false)
                break;

            _movie_box_button_select(MOVIE_BOX_BUTTON_SPEED_BACKWARD);
			/*turn to playing, then speed*/
			if(PLAY_MOVIE_CTROL_PAUSE == movie_box_button2_ctrol)
			{
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
				movie_box_button2_setimg(movie_box_button2_ctrol);
				goto BACKSPEED;
			}
			else
			{
				/*speed*/
BACKSPEED:		movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
				if(PLAY_MOVIE_SPEED_X32 <= movie_box_button1_bspeed)
				{
					movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
					movie_box_button2_setimg(PLAY_MOVIE_CTROL_PLAY);
				}
				else
				{
                    movie_box_button1_bspeed *= 2;
                    movie_box_button2_setimg(PLAY_MOVIE_CTROL_PAUSE);
				}
				popmsg_topright_ctrl(PLAY_MOVIE_CTROL_BACKWARD, movie_box_button1_bspeed);
#if MEDIA_SUBTITLE_SUPPORT
                s_sub_delayms = app_get_subt_delay_time();
#endif

				play_movie_speed(PLAY_MOVIE_CTROL_BACKWARD, movie_box_button1_bspeed);
			}
			break;


		case MOVIE_BOX_BUTTON_PLAY_PAUSE:
			if(PLAY_MOVIE_CTROL_PLAY == movie_box_button2_ctrol)
			{
				if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
					|| (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
				{
					movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
					movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;

					popmsg_topright_ctrl(PLAY_MOVIE_CTROL_FORWARD, PLAY_MOVIE_SPEED_X1);
					play_movie_speed(PLAY_MOVIE_CTROL_FORWARD, PLAY_MOVIE_SPEED_X1);
				}
				else
				{
					if((delay_play_timer!= NULL)||(0==app_movie_pause_allowed()))
					{
						break;
					}
					movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PAUSE;
					play_movie_ctrol(PLAY_MOVIE_CTROL_PAUSE);
				}
			}
#if MEDIA_SUBTITLE_SUPPORT
            else if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_RESUME)
            {
                if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
					|| (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
				{
					movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
					movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;

					popmsg_topright_ctrl(PLAY_MOVIE_CTROL_FORWARD, PLAY_MOVIE_SPEED_X1);
					play_movie_speed(PLAY_MOVIE_CTROL_FORWARD, PLAY_MOVIE_SPEED_X1);
				}
				else
				{
					if((delay_play_timer!= NULL)||(0==app_movie_pause_allowed()))
					{
						break;
					}
					movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PAUSE;
					play_movie_ctrol(PLAY_MOVIE_CTROL_PAUSE);
				}
            }
#endif
			else if(PLAY_MOVIE_CTROL_PAUSE == movie_box_button2_ctrol)
			{
				if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
					|| (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
				{
					movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
					movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
					play_movie_speed(PLAY_MOVIE_CTROL_FORWARD, PLAY_MOVIE_SPEED_X1);

				}

#if MEDIA_SUBTITLE_SUPPORT
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_RESUME;//PLAY_MOVIE_CTROL_PLAY;
#else
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
#endif
				play_movie_ctrol(PLAY_MOVIE_CTROL_RESUME);
			}
			else if(PLAY_MOVIE_CTROL_STOP == movie_box_button2_ctrol)
			{
				int seeked_val = 0;
                GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &seeked_val);
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
				play_movie_ctrol(PLAY_MOVIE_CTROL_PLAY);
                _media_lock_play();
                if(false == s_ctl_box_show)
				    movie_box_show();
#if OSD_SUBTITLE_SUPPORT
                else
                    app_osd_subt_pause();
#endif
			}
			movie_box_button2_setimg(movie_box_button2_ctrol);
			popmsg_topright_ctrl(movie_box_button2_ctrol, 0);
			break;

		// add the pause mode for pause key
		case MOVIE_BOX_BUTTON_PAUSE:
			if(PLAY_MOVIE_CTROL_PLAY == movie_box_button2_ctrol)
			{
				if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1) || (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
				{
					s_pause_to_speed_play = true;
				}

				if((delay_play_timer!= NULL)||(0==app_movie_pause_allowed()))
				{
					break;
				}
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PAUSE;
				play_movie_ctrol(PLAY_MOVIE_CTROL_PAUSE);
				movie_box_button2_setimg(movie_box_button2_ctrol);
				popmsg_topright_ctrl(movie_box_button2_ctrol, 0);

			}
#if MEDIA_SUBTITLE_SUPPORT
            else if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_RESUME)
            {
                if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1) || (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
				{
					s_pause_to_speed_play = true;
				}
                if((delay_play_timer!= NULL)||(0==app_movie_pause_allowed()))
				{
					break;
				}
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PAUSE;
				play_movie_ctrol(PLAY_MOVIE_CTROL_PAUSE);
				movie_box_button2_setimg(movie_box_button2_ctrol);
				popmsg_topright_ctrl(movie_box_button2_ctrol, 0);
            }
#endif
			else if(PLAY_MOVIE_CTROL_PAUSE == movie_box_button2_ctrol)
			{
				if((movie_box_button1_bspeed == PLAY_MOVIE_SPEED_X1) && (movie_box_button3_fspeed == PLAY_MOVIE_SPEED_X1))
				{
#if MEDIA_SUBTITLE_SUPPORT
                        movie_box_button2_ctrol = PLAY_MOVIE_CTROL_RESUME;//PLAY_MOVIE_CTROL_PLAY;
#else
                        movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
#endif
					play_movie_ctrol(PLAY_MOVIE_CTROL_RESUME);
					movie_box_button2_setimg(movie_box_button2_ctrol);
					popmsg_topright_ctrl(movie_box_button2_ctrol, 0);
				}
                else
                {
                    if((delay_play_timer!= NULL)||(0==app_movie_pause_allowed()))
                    {
                        break;
                    }
					if(s_pause_to_speed_play)
					{
						s_pause_to_speed_play = false;
						if(movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
						{
							popmsg_topright_ctrl(PLAY_MOVIE_CTROL_BACKWARD, movie_box_button1_bspeed);
							play_movie_speed(PLAY_MOVIE_CTROL_BACKWARD, movie_box_button1_bspeed);
							if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MEDIA_BAR))
							{
								GUI_SetFocusWidget(BOX_MOVIE_CTROL);
								box_sel = MOVIE_BOX_BUTTON_SPEED_BACKWARD;
								GUI_SetProperty(BOX_MOVIE_CTROL, "select", &box_sel);
							}
						}
						else if(movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1)
						{
							popmsg_topright_ctrl(PLAY_MOVIE_CTROL_FORWARD, movie_box_button3_fspeed);
							play_movie_speed(PLAY_MOVIE_CTROL_FORWARD, movie_box_button3_fspeed);
							if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MEDIA_BAR))
							{
								GUI_SetFocusWidget(BOX_MOVIE_CTROL);
								box_sel = MOVIE_BOX_BUTTON_SPEED_FORWARD;
								GUI_SetProperty(BOX_MOVIE_CTROL, "select", &box_sel);
							}
						}
					}
					else
					{
						s_pause_to_speed_play = true;
#if MEDIA_SUBTITLE_SUPPORT
						if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1) || (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
							movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PAUSE;
						else
							movie_box_button2_ctrol = PLAY_MOVIE_CTROL_RESUME;
#else
						movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
#endif
						play_movie_ctrol(PLAY_MOVIE_CTROL_PAUSE);
						movie_box_button2_setimg(movie_box_button2_ctrol);
						popmsg_topright_ctrl(movie_box_button2_ctrol, 0);
					}
                }
            }
            else
            {
                break;
            }
			//recover the last play mode display
			if(PLAY_MOVIE_CTROL_PLAY == movie_box_button2_ctrol)
			{
				if(movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
					popmsg_topright_ctrl(PLAY_MOVIE_CTROL_BACKWARD, movie_box_button1_bspeed);
				else if(movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1)
					popmsg_topright_ctrl(PLAY_MOVIE_CTROL_FORWARD, movie_box_button3_fspeed);

			}
			break;

		case MOVIE_BOX_BUTTON_PLAY:
			if(PLAY_MOVIE_CTROL_PAUSE== movie_box_button2_ctrol)
			{
				if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
					|| (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
				{
					movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
					movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
					play_movie_speed(PLAY_MOVIE_CTROL_FORWARD, PLAY_MOVIE_SPEED_X1);
				}
#if MEDIA_SUBTITLE_SUPPORT
                movie_box_button2_ctrol = PLAY_MOVIE_CTROL_RESUME;//PLAY_MOVIE_CTROL_PLAY;
#else
                movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
#endif
				play_movie_ctrol(PLAY_MOVIE_CTROL_RESUME);
			}
			else if(PLAY_MOVIE_CTROL_STOP== movie_box_button2_ctrol)
			{
				int seeked_val = 0;
                GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &seeked_val);
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
				play_movie_ctrol(PLAY_MOVIE_CTROL_PLAY);
                _media_lock_play();
                // modify by gosta 20220323 , issue 312569
                if(false == s_ctl_box_show)
                {
                    if(GUI_CheckDialog(WND_PASSWD) != GXCORE_SUCCESS)
                    {
                        movie_box_show();
                    }
                }
#if OSD_SUBTITLE_SUPPORT
                else
                    app_osd_subt_pause();
#endif
			}
			else if((PLAY_MOVIE_CTROL_PLAY == movie_box_button2_ctrol
                    || movie_box_button2_ctrol == PLAY_MOVIE_CTROL_RESUME)
					&& ((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
					|| (movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1)))
			{
					movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
					movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
                    movie_box_button2_ctrol = PLAY_MOVIE_CTROL_RESUME;
                    play_movie_speed(PLAY_MOVIE_CTROL_FORWARD, movie_box_button3_fspeed);
#if OSD_SUBTITLE_SUPPORT
                    if(PMPSET_TONE_OFF == pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY))
                        app_osd_subt_pause();
#endif
			}
			else
			{
				break;
			}

			// modify by gosta 20220323 , issue 312569
			if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PASSWD)&&(!s_ctl_box_show))
			{
				popmsg_topright_hide(NULL);
			}
			else
			{
				movie_box_button2_setimg(movie_box_button2_ctrol);
				popmsg_topright_ctrl(movie_box_button2_ctrol, 0);
			}

			break;

		case MOVIE_BOX_BUTTON_SPEED_FORWARD:
            if(_check_ff_rew_support() == false)
                break;
            _movie_box_button_select(MOVIE_BOX_BUTTON_SPEED_FORWARD);
			/*turn to playing, then speed*/
			if(PLAY_MOVIE_CTROL_PAUSE == movie_box_button2_ctrol)
			{
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
				movie_box_button2_setimg(movie_box_button2_ctrol);
				goto FORWARDSPEED;
			}
			else
			{
				/*speed*/
FORWARDSPEED:	movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
				if(PLAY_MOVIE_SPEED_X32 <= movie_box_button3_fspeed)
				{
					movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
					movie_box_button2_setimg(PLAY_MOVIE_CTROL_PLAY);
				}
				else
				{
					movie_box_button3_fspeed *= 2;
					movie_box_button2_setimg(PLAY_MOVIE_CTROL_PAUSE);
				}
				popmsg_topright_ctrl(PLAY_MOVIE_CTROL_FORWARD, movie_box_button3_fspeed);
#if MEDIA_SUBTITLE_SUPPORT
                s_sub_delayms = app_get_subt_delay_time();
#endif
				play_movie_speed(PLAY_MOVIE_CTROL_FORWARD, movie_box_button3_fspeed);
			}
			break;

		case MOVIE_BOX_BUTTON_NEXT:
			APP_TIMER_REMOVE(delay_play_timer);
			GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "hide");
			GUI_SetProperty(SLIDERBAR_MOVICE, "value", &sliderbar_value);
			/*reset speed state*/
			movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
			movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
			/*reset play state*/
            movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
            movie_box_button2_setimg(movie_box_button2_ctrol);
			/*reset zoom state*/
			if(PLAY_MOVIE_ZOOM_X1 != movie_box_button6_zoom)
			{
				movie_box_button6_zoom = PLAY_MOVIE_ZOOM_X1;
				//TODO: if gxplayer not reset, reset it
				play_movie_zoom(movie_box_button6_zoom);
			}

			/*next*/
            path = app_movie_view_get_movie_path();
            ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &time_info);
            if((ret != GXCORE_ERROR) &&
                    (0 < time_info.current - time_info.seek_min) && (0 <  s_movie_total_time) && (time_info.current - time_info.seek_min != s_movie_total_time))
            {
                pmp_save_tag(path, time_info.current);
            }
            s_movie_total_time=0;
            s_movie_seek_min_time = 0;
            s_frist_play = true;
            play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
#if PARENTAL_LOCK_SUPPORT
            if(s_moview_psi_info.private_info.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE)
            {
                app_pvr_file_parental_rating_stop();
            }
#endif
            ret =play_movie_ctrol(PLAY_MOVIE_CTROL_NEXT);
            popmsg_topright_ctrl(PLAY_MOVIE_CTROL_NEXT, 0);
            s_start_time_ms = 0;
            pmp_save_tag(path, s_start_time_ms);
            _media_lock_play();
            break;

		case MOVIE_BOX_BUTTON_STOP:
			/*reset speed state*/
			movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
			movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
			s_is_movie_playing = false;
			s_movie_total_time=0;
            s_movie_seek_min_time = 0;
            s_frist_play = true;
#if MEDIA_SUBTITLE_SUPPORT
            s_sub_delayms = 0;
#endif
			APP_TIMER_REMOVE(delay_play_timer);
			int bcheck = 1 ;
			if( APP_THEME_CLASSIC_DARK ){
				if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MEDIA_BAR))
					bcheck = 1 ;
				else
					bcheck = 0 ;
			}
			if (bcheck)
			{
				//#error 10559
				int sliderbar_init = 0;
				char buf_tmp[20]= "00:00:00";
				GUI_SetProperty(SLIDERBAR_MOVICE, "value", &sliderbar_init);
				GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &sliderbar_init);
				GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
				GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);
			}
			/*reset zoom state*/
			if(PLAY_MOVIE_ZOOM_X1 != movie_box_button6_zoom)
			{
				movie_box_button6_zoom = PLAY_MOVIE_ZOOM_X1;
				//TODO: if gxplayer not reset, reset it
				play_movie_zoom(movie_box_button6_zoom);
			}

			/*stop*/
			if(PLAY_MOVIE_CTROL_STOP != movie_box_button2_ctrol)
			{
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_STOP;
				movie_box_button2_setimg(movie_box_button2_ctrol);
			}
			{
			    int tag_ms = 0;
			    char* path = NULL;
			    path = app_movie_view_get_movie_path();

			    /*save tag*/
			    pmp_save_tag(path, tag_ms);
			}
			popmsg_topright_ctrl(PLAY_MOVIE_CTROL_STOP, 0);

			play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);

			break;

		case MOVIE_BOX_BUTTON_ZOOM:
			if(PLAY_MOVIE_ZOOM_X16 <= movie_box_button6_zoom)
			{
				movie_box_button6_zoom = PLAY_MOVIE_ZOOM_X1;
			}
			else
			{
				movie_box_button6_zoom++;
			}
			popmsg_topright_ctrl(PLAY_MOVIE_CTROL_ZOOM, movie_box_button6_zoom);

			play_movie_zoom(movie_box_button6_zoom);
			break;
		case MOVIE_BOX_BUTTON_SET:
            _create_media_set_dlg();
			break;
#if MEDIA_SUBTITLE_SUPPORT
		case MOVIE_BOX_BUTTON_SUB:
            _create_media_sub_dlg();
			break;
#else
		case MOVIE_BOX_BUTTON_INFO:
            _create_media_info_dlg();
			break;
#endif

#if PLAYBACK_SPEED_SUPPORT
        case MOVIE_BOX_BUTTON_PLAYBACK_SPEED:
#if MEDIA_SUBTITLE_SUPPORT
            s_sub_delayms = app_get_subt_delay_time();
#endif
             if(_check_playspeed_sub_support() == false){
                 break;
             }
            _movie_box_button_select(MOVIE_BOX_BUTTON_PLAYBACK_SPEED);
            _playback_speed_poplist(_movie_playback_get_select());
            break;
#else
		case MOVIE_BOX_BUTTON_VOL:
            _create_media_volume_dlg();
			break;
#endif
		default:
			ret =  GXCORE_ERROR;
			break;
	}

	return ret;
}

static int _movie_play_pre_or_next(movie_box_button button)
{
    int sliderbar_init = 0;
    char buf_tmp[20]= "00:00:00";
    if(button != MOVIE_BOX_BUTTON_PREVIOUS && button != MOVIE_BOX_BUTTON_NEXT)
        return 0;
    popmsg_centre_hide(NULL);
    app_movie_view_end_dialog();
    movie_box_ctrl(button);
    movie_box_show();

    GUI_SetProperty(SLIDERBAR_MOVICE, "value", &sliderbar_init);
    GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &sliderbar_init);
    GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
    GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);
    GUI_SetProperty(WIN_MEDIA_BAR, "update", NULL);
    GUI_SetProperty(WIN_MOVIE_VIEW, "update", NULL);

    if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
        GUI_SetFocusWidget(WND_PASSWD);
    else
        GUI_SetFocusWidget(SLIDER_CURSOR_MOVICE);

    return 0;
}

status_t app_seek_set_mediaplay(int seek_ms,SeekFlag flag)
{
	GxMessage *msg;

	GxMsgProperty_PlayerSeek *seek;
	msg = GxBus_MessageNew(GXMSG_PLAYER_SEEK);
	APP_CHECK_P(msg, 1);
	seek = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerSeek);
	APP_CHECK_P(seek, 1);
	seek->player = PMP_PLAYER_AV;
	seek->time = seek_ms;
	seek->flag = flag;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);

    movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
    movie_box_button2_setimg(movie_box_button2_ctrol);
#if PLAYBACK_SPEED_SUPPORT
    if(_movie_playback_get_select() != PLAYBACK_SPEED_DEFAULT_VALUE)
    {
        _movie_playback_set_speed(_movie_playback_get_select());
        popmsg_topright_ctrl(PLAY_MOVIE_PLAYBACK_SPEED,_movie_playback_get_select());
    }
    else
    {
        popmsg_topright_ctrl(movie_box_button2_ctrol, 0);

    }
#else
    popmsg_topright_ctrl(movie_box_button2_ctrol, 0);
#endif
    return GXCORE_SUCCESS;
}

static status_t movie_sliderbar_okkey(void)
{
	long cur_time_ms = 0;
	char buf_tmp[20] = {0};
	int value=0;

	GUI_GetProperty(SLIDER_CURSOR_MOVICE, "value", &value);
	cur_time_ms= s_movie_seek_min_time + s_movie_total_time*value/100;
	if((cur_time_ms - s_movie_seek_min_time > s_movie_total_time)||(0 >= s_movie_total_time))
		return GXCORE_ERROR;

	if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_STOP)
	{
		play_list* list = NULL;
		char* path = NULL;

		/*play*/
		list = play_list_get(PLAY_LIST_TYPE_MOVIE);
		if(NULL == list) return GXCORE_ERROR;

		path = explorer_static_path_strcat(list->path, list->ents[list->play_no]);
		if(NULL == path) return GXCORE_ERROR;

		app_log_info("[PLAY] movie: %s\n", path);
		play_av_by_url(path, cur_time_ms);

		GUI_SetProperty(TEXT_MOVIE_STATUS, "string", "Seeking...");

		GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "show");
		GUI_SetProperty(TEXT_MOVIE_STATUS, "draw_now", NULL);

		GUI_SetProperty(IMAGE_POPMSG, "state", "osd_trans_hide");
		GUI_SetProperty(TEXT_POPMSG, "state", "hide");
		GUI_SetProperty(TEXT_POPMSG, "string", "X1");
	}
	else
	{
		GUI_SetProperty(TEXT_MOVIE_STATUS, "string", "Seeking...");
		GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "show");
		GUI_SetProperty(TEXT_MOVIE_STATUS, "draw_now", NULL);
        if(s_movie_total_time>0)
            app_seek_set_mediaplay(cur_time_ms,SEEK_ORIGIN_SET);
        else
            app_seek_set_mediaplay(value,SEEK_ORIGIN_PERCENT);

#if MEDIA_SUBTITLE_SUPPORT
        if(PLAY_MOVIE_SPEED_X1 != movie_box_button1_bspeed || movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1)
        {
            if(app_movie_dvb_subt_cnt() > 0)
            {
                int dvb_subt_index = app_movie_dvb_subt_index();
                movie_dvb_subt_start(dvb_subt_index);
            }
            else
            {
                subtitle_restart();
            }
            app_subt_select_restore();
        }
        s_sub_delayms = app_get_subt_delay_time();
        app_subt_visibility_restore();
#if OSD_SUBTITLE_SUPPORT
        if(PMPSET_TONE_ON == pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY))
            app_osd_subt_pause();
#endif
#endif
        movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
		movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;

		GUI_SetProperty(SLIDERBAR_MOVICE, "value", &value);
		GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "hide");
		GUI_SetProperty(TEXT_MOVIE_STATUS, "draw_now", NULL);
        app_time_ms_to_edit_str((cur_time_ms-s_movie_seek_min_time),buf_tmp,20);
		GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
	}

	s_seek_flag = MOVIE_SEEK_NONE;
	APP_TIMER_ADD(box_sliderbar_update_timer, movie_sliderbar_update, 200, TIMER_REPEAT);

	return GXCORE_SUCCESS;
}


static status_t movie_sliderbar_active(void* usrdata)
{
    box_sliderbar_active_timer = NULL;
    s_seek_flag = MOVIE_SEEKED;
	APP_TIMER_ADD(box_sliderbar_update_timer, movie_sliderbar_update, 200, TIMER_REPEAT);

	return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int  movie_view_sliderbar_keypress(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int  movie_view_cursor_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int64_t cur_time_ms = 0;
	int value=0,step=0;
	char buf_tmp[20];

	play_movie_info info;

	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	movie_box_timer_restart();

	event = (GUI_Event *)usrdata;
	switch(event->key.sym)
	{
		case APPK_BACK:
		case APPK_MENU:
			movie_box_hide(NULL);
			return EVENT_TRANSFER_STOP;

		case APPK_LEFT:
			if(0 >= s_movie_total_time)
				return EVENT_TRANSFER_STOP;

          APP_TIMER_REMOVE(box_sliderbar_update_timer);
			s_seek_flag = MOVIE_SEEKING;

			GUI_GetProperty(SLIDER_CURSOR_MOVICE, "value", &value);
			GUI_GetProperty(SLIDER_CURSOR_MOVICE, "step", &step);
			if(value-step<0)
			{
				value=0;
			}
			else
			{
				value-=step;
			}
			cur_time_ms=s_movie_total_time*value/100;
            app_time_ms_to_edit_str(cur_time_ms,buf_tmp,20);
			GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
			GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &value);

			movie_sliderbar_update(0);
			APP_TIMER_ADD(box_sliderbar_active_timer, movie_sliderbar_active, 1000, TIMER_ONCE);
			return EVENT_TRANSFER_STOP;

		case APPK_RIGHT:
			if(0 >= s_movie_total_time)
				return EVENT_TRANSFER_STOP;

          APP_TIMER_REMOVE(box_sliderbar_update_timer);
			s_seek_flag = MOVIE_SEEKING;

			GUI_GetProperty(SLIDER_CURSOR_MOVICE, "value", &value);
			GUI_GetProperty(SLIDER_CURSOR_MOVICE, "step", &step);
			if(value+step>100)
			{
				value=100;
			}
			else
			{
				value+=step;
			}
			cur_time_ms=s_movie_total_time*value/100;
            app_time_ms_to_edit_str(cur_time_ms,buf_tmp,20);
			GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
			GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &value);

			movie_sliderbar_update(0);
			APP_TIMER_ADD(box_sliderbar_active_timer, movie_sliderbar_active, 1000, TIMER_ONCE);
			return EVENT_TRANSFER_STOP;

		case APPK_OK:
            if(s_seek_flag == MOVIE_SEEKED  || s_seek_flag == MOVIE_SEEKING)
            {
			    APP_TIMER_REMOVE(box_sliderbar_active_timer);
			    movie_sliderbar_okkey();
            }
			return EVENT_TRANSFER_STOP;

		case APPK_UP:
			return EVENT_TRANSFER_STOP;

		case APPK_DOWN:
			GUI_SetFocusWidget(BOX_MOVIE_CTROL);
			s_seek_flag = MOVIE_SEEK_NONE;
			play_movie_get_info(&info);
			movie_box_setinfo(&info);
			APP_TIMER_REMOVE(box_sliderbar_active_timer);
			APP_TIMER_ADD(box_sliderbar_update_timer, movie_sliderbar_update, 200, TIMER_REPEAT);
			GUI_SetProperty(WIN_MEDIA_BAR, "update", NULL);
			return EVENT_TRANSFER_STOP;
		default:
			break;
	}
	return EVENT_TRANSFER_KEEPON;
}

static int movie_box_show(void)
{
	play_movie_info info;

	/*show*/
	app_movie_view_create();

	if(s_frist_play == true)
	{
		int sliderbar_init = 0;
		GUI_SetProperty(SLIDERBAR_MOVICE, "value", &sliderbar_init);
		GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &sliderbar_init);
	}
	/*set info*/
	play_movie_get_info(&info);
	movie_box_setinfo(&info);

    if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_STOP)
    {
        char buf_tmp[20]= "00:00:00";
        GUI_SetProperty(TEXT_START_TIME, "string", buf_tmp);
        GUI_SetProperty(TEXT_STOP_TIME, "string", buf_tmp);
    }
    /*timer start*/
	movie_box_timer_start();
	movie_sliderbar_timer_start();
	s_seek_flag = MOVIE_SEEK_NONE;

    if (movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1 || movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1)
    {
		movie_box_button2_setimg(PLAY_MOVIE_CTROL_PAUSE);
    }

	int item_sel = MOVIE_BOX_BUTTON_PLAY_PAUSE;
	GUI_SetProperty(BOX_MOVIE_CTROL, "select", (void*)&item_sel);
	GUI_SetFocusWidget(SLIDER_CURSOR_MOVICE);
	GUI_SetProperty(WIN_MEDIA_BAR, "update", NULL);
    s_ctl_box_show = true;

	return 0;
}

SIGNAL_HANDLER int movie_view_box_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	uint32_t item_sel = 0;
	int ret_value = 0;


	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			movie_box_timer_restart();

			switch(event->key.sym)
			{
				case APPK_BACK:
				case APPK_MENU:
					ret_value = movie_box_hide(NULL);
					return ret_value;
				case APPK_OK:
					GUI_GetProperty(BOX_MOVIE_CTROL, "select", (void*)&item_sel);
					if(MOVIE_BOX_BUTTON_SPEED_BACKWARD == item_sel || MOVIE_BOX_BUTTON_SPEED_FORWARD == item_sel)
					{
						if(delay_play_timer != NULL)
						{
							return EVENT_TRANSFER_STOP;
						}
					}

					if(MOVIE_BOX_BUTTON_STOP == item_sel)
					{
						s_movie_manual_stop = TRUE;
					}
					if(MOVIE_BOX_BUTTON_PREVIOUS == item_sel)
					{
                        _movie_play_pre_or_next(MOVIE_BOX_BUTTON_PREVIOUS);
						return EVENT_TRANSFER_STOP;

					}
					if(MOVIE_BOX_BUTTON_NEXT == item_sel)
					{
                        _movie_play_pre_or_next(MOVIE_BOX_BUTTON_NEXT);
						return EVENT_TRANSFER_STOP;
					}
					movie_box_ctrl(item_sel);
					return EVENT_TRANSFER_STOP;
					break;
				case APPK_SEEK:
					GUI_SetFocusWidget(SLIDER_CURSOR_MOVICE);
					return EVENT_TRANSFER_STOP;
				case APPK_DOWN:
					return EVENT_TRANSFER_STOP;
				case APPK_UP:
					GUI_SetFocusWidget(SLIDER_CURSOR_MOVICE);
					return EVENT_TRANSFER_STOP;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER  int movie_view_got_focus(const char* widgetname, void *usrdata)
{

	PlayerStatusInfo info = {0};
	if(GXCORE_SUCCESS == GxPlayer_MediaGetStatus(PMP_PLAYER_AV, &info))
	{
		if(info.status == PLAYER_STATUS_PLAY_PAUSE)
		{
			GUI_SetProperty(IMAGE_POPMSG, "state", "show");
			GUI_SetProperty(TEXT_POPMSG, "state", "hide");
			GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PAUSE);
		}
	}

#if OSD_SUBTITLE_SUPPORT
    if( (PMPSET_TONE_ON == pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY))
	    && (info.status == PLAYER_STATUS_PLAY_RUNNING) )
    {
        //GUI_EndDialog() will call this, subt will show dirty dialog data. so flush 20230208
        GUI_SetInterface("flush", NULL);
        app_osd_subt_resume();
    }
#endif

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int movie_view_lost_focus(const char* widgetname, void *usrdata)
{
#if MEDIA_SUBTITLE_SUPPORT
	movie_box_timer_stop();
#endif
#if OSD_SUBTITLE_SUPPORT
    if(PMPSET_TONE_ON == pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY))
        app_osd_subt_pause();
#endif

	return EVENT_TRANSFER_STOP;
}

static int s_delay_play_cnt = 0;
static int _delay_play(void* userdata)
{
	++s_delay_play_cnt;
	APP_TIMER_REMOVE(delay_play_timer);
	if(0 == strcasecmp(WND_POP_BOOK, GUI_GetFocusWindow()))
	{
		return 0;
	}
	pmpset_movie_play_sequence sequence = 0;
	sequence = pmpset_get_int(PMPSET_MOVIE_PLAY_SEQUENCE);
	if(PMPSET_MOVIE_PLAY_SEQUENCE_SEQUENCE == sequence && s_delay_play_cnt < 3)
	{
		play_list* list = NULL;
		list = play_list_get(PLAY_LIST_TYPE_MOVIE);

		if(NULL == list) return GXCORE_ERROR;

		APP_Printf("play sequence\n");
		if(list->play_no >= list->nents -1)
		{
			movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
			app_movie_view_end_dialog();
            app_enddialog_wnd(WIN_MOVIE_VIEW);
		}
		else
		{
			app_movie_view_end_dialog();
            app_enddialog_after_wnd("after win_movie_view");
			movie_box_ctrl(MOVIE_BOX_BUTTON_NEXT);
		}
	}
	else
	{
		movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
        app_movie_view_end_dialog();
        app_enddialog_wnd(WIN_MOVIE_VIEW);
	}
	return 0;
}

static int movie_play_status_prompt_info(GxMsgProperty_PlayerStatusReport* player_status)
{
	if(player_status == NULL)
		return -1;
	s_movie_video_ok = true; //video err, can't support play_movie_speed
	if(player_status->error == PLAYER_ERROR_AUDIO_DECODER_ERROR)
	{
	    PopDlg pop;
	    memset(&pop, 0, sizeof(PopDlg));
	    pop.type = POP_TYPE_OK;
	    pop.format = POP_FORMAT_DLG;
	    pop.str = STR_ID_CAN_NOT_PLAY_AUDIO;
	    pop.mode = POP_MODE_UNBLOCK;
	    pop.timeout_sec = 5;
	    popdlg_create(&pop);
	}
	else if(player_status->error == PLAYER_ERROR_VIDEO_DECODER_ERROR)
	{
	    PopDlg pop;
	    memset(&pop, 0, sizeof(PopDlg));
	    pop.type = POP_TYPE_OK;
	    pop.format = POP_FORMAT_DLG;
	    pop.str = STR_ID_CAN_NOT_PLAY_VIDEO;
	    pop.mode = POP_MODE_UNBLOCK;
	    pop.timeout_sec = 5;
	    popdlg_create(&pop);
		s_movie_video_ok = false; //video err, can't support play_movie_speed
	}
	//app_log_debug("---%s:%d player_status->error = %d\n",__func__,__LINE__,player_status->error);
	return 0;
}

#if AUDIO_DESCRIPTOR_SUPPORT
static int app_audio_descriptor_enable(PlayerProgInfo *player_info)
{
    int i = 0;
    int32_t ad_mode = 1;
    GxMsgProperty_PlayerAdAudio ad_audio = {0};

    ad_audio.player = PMP_PLAYER_AV;
    GxBus_ConfigGetInt(AD_CONF_KEY, &ad_mode, AD_CONFIG);
    if(ad_mode)
    {
        for(i = 0; i< player_info->audio_num; i++)
        {
            if(player_info->audio[i].track_type == AUDIO_DESCRIPTOR_TRACK)
            {
                ad_audio.apid = i; //播放码流文件用索引，大网节目用pid
                ad_audio.admxid = 1;
                ad_audio.atype = player_info->audio[i].codec_type;
                app_send_msg_exec(GXMSG_PLAYER_AD_AUDIO_ENABLE,(void*)&ad_audio);
                break;
            }
        }
    }
    return 0;
}
#endif

static int movie_service_status(GxMsgProperty_PlayerStatusReport* player_status)
{
	play_list* list = NULL;
	char* path = NULL;
	pmpset_movie_play_sequence sequence = 0;
	uint32_t cur_sliderbar = 0;

	APP_CHECK_P(player_status, 1);
	if(strcmp(player_status->player,PMP_PLAYER_AV) != 0)
		return 1;

	/*check this msg owner*/
	path = app_movie_view_get_movie_path();
	if((path != NULL) && (strcmp(path, player_status->url) != 0))
	{
		if(music_play_state() == TRUE)
		{
			music_status_init();
		}
		return 0;
	}

//	app_log_debug("@@@@@@@@@@@@@@status = %2d, url =%s@@@@@\n", player_status->status, player_status->url);
	switch(player_status->status)
	{
		case PLAYER_STATUS_ERROR:
			APP_Printf("\n[SERVICE] PLAYER_STATUS_ERROR, %d\n", player_status->error);
			s_is_movie_playing = false;
            app_movie_view_end_dialog();
			GUI_SetProperty(TEXT_MOVIE_STATUS, "string", STR_ID_CAN_NOT_PLAY_FILE);
			GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "show");
			GUI_SetProperty(TEXT_MOVIE_STATUS, "draw_now", NULL);

			play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
			int sliderbar_init = 0;
			GUI_SetProperty(SLIDERBAR_MOVICE, "value", &sliderbar_init);

            s_dvb_subt_index = 0;
			s_movie_total_time=0;
            s_movie_seek_min_time = 0;
            s_frist_play = true;
			APP_TIMER_ADD(delay_play_timer, _delay_play, 5000, TIMER_REPEAT);
			break;

		case PLAYER_STATUS_PLAY_END:
			s_is_movie_playing = false;
			APP_Printf("\n[SERVICE] PLAYER_STATUS_PLAY_END\n");
			GUI_SetInterface("flush", NULL);//#77894
			GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "hide");
            s_dvb_subt_index = 0;
			s_movie_total_time=0;
            s_movie_seek_min_time = 0;
			s_start_time_ms = 0;
			pmp_save_tag(path, s_start_time_ms);

			if(GXCORE_ERROR == path_usb_check(path))
			{
				APP_Printf("usb err,play end!!!\n");
				movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
				app_movie_view_end_dialog();
				app_enddialog_wnd(WIN_MOVIE_VIEW);
				break;
			}

			/*sequence*/
			sequence = pmpset_get_int(PMPSET_MOVIE_PLAY_SEQUENCE);
			if(PMPSET_MOVIE_PLAY_SEQUENCE_ONLY_ONCE == sequence)
			{
				APP_Printf("play only once\n");
				movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
				app_movie_view_end_dialog();
				app_enddialog_wnd(WIN_MOVIE_VIEW);
				break;
			}
			else if(PMPSET_MOVIE_PLAY_SEQUENCE_REPEAT_ONE == sequence)
			{
			    popdlg_destroy();
				if(s_movie_manual_stop == TRUE)
				{
					movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
                    app_movie_view_end_dialog();
					app_enddialog_wnd(WIN_MOVIE_VIEW);
				}
				else
				{
					APP_Printf("play repeat one\n");
					if(GUI_CheckDialog(WIN_MEDIA_BAR) == GXCORE_SUCCESS)
					{
						GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &cur_sliderbar);//#39566
					}
					if((movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
							||(movie_box_button3_fspeed != PLAY_MOVIE_SPEED_X1))
					{
						movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
						movie_box_ctrl(MOVIE_BOX_BUTTON_PLAY);
						app_movie_view_end_dialog();
					}
					else
					{
                        if(GUI_CheckDialog(WND_FILE_LIST)==GXCORE_SUCCESS)
                        {
                            file_list_destroy();
                        }
                        movie_box_hide(NULL);
						app_enddialog_after_wnd("after win_movie_view");
						movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
						movie_box_ctrl(MOVIE_BOX_BUTTON_PLAY);
					}
				}
				break;
			}
			else if(PMPSET_MOVIE_PLAY_SEQUENCE_SEQUENCE == sequence)
			{
				list = play_list_get(PLAY_LIST_TYPE_MOVIE);
				if(NULL == list) return GXCORE_ERROR;

				APP_Printf("play sequence\n");
				if(movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1)
				{
					movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
					movie_box_ctrl(MOVIE_BOX_BUTTON_PLAY);
				}
				else
				{
					if(GUI_CheckDialog(WIN_MEDIA_BAR) == GXCORE_SUCCESS)
					{
						GUI_SetProperty(SLIDER_CURSOR_MOVICE, "value", &cur_sliderbar);//#39566
					}
					if(list->play_no >= list->nents -1)
					{
						movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
						app_movie_view_end_dialog();
						app_enddialog_wnd(WIN_MOVIE_VIEW);
					}
					else if(s_movie_manual_stop == TRUE)
					{
						movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
					}
					else
					{
                        movie_box_hide(NULL);
						app_enddialog_after_wnd("after win_movie_view");
						movie_box_ctrl(MOVIE_BOX_BUTTON_NEXT);
					}
				}
				app_movie_view_end_dialog();
				break;
			}
			else
			{
				APP_Printf("play none\n");
				movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
				break;
			}
			break;

		case PLAYER_STATUS_PLAY_RUNNING:
			s_is_movie_playing = true;
			s_seek_flag = MOVIE_SEEK_NONE;//#42494
            APP_Printf("\n[SERVICE] PLAYER_STATUS_PLAY_RUNNING\n");
            PlayerProgInfo *player_info = app_player_prog_info_get();
#if PLAYBACK_SPEED_SUPPORT
            if(_movie_playback_get_select() != PLAYBACK_SPEED_DEFAULT_VALUE)
            {
                _movie_playback_set_speed(_movie_playback_get_select());
                popmsg_topright_ctrl(PLAY_MOVIE_PLAYBACK_SPEED,_movie_playback_get_select());
            }
#endif
            if(GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, player_info) == GXCORE_SUCCESS) //success
            {
                int lang_value = 0;
                GxBus_ConfigSetInt(MEDIA_SUBTILE_LANGUAGE, lang_value);
                memcpy(&s_player_info, player_info, sizeof(PlayerProgInfo));
                if(PLAY_MOVIE_SPEED_X1 == movie_box_button3_fspeed && PLAY_MOVIE_SPEED_X1 == movie_box_button1_bspeed)
                {
                    movie_box_button2_setimg(PLAY_MOVIE_CTROL_PLAY);
                }
                app_update_psi_subt_info();
                GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "hide");
#if AUDIO_DESCRIPTOR_SUPPORT
                app_audio_descriptor_enable(player_info);
#endif
            }
            else
            {
                app_log_error("get prog info err !!!\n");
            }
            /*set info*/
            if(GXCORE_ERROR == GUI_CheckDialog(WIN_MOVIE_SET)
                    && GXCORE_ERROR == GUI_CheckDialog(WIN_MOVIE_SUBT))
            {
				if(s_frist_play == true)
				{
					if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VOLUME))
					{
						GUI_EndDialog(WND_VOLUME);
						GUI_SetProperty(WND_VOLUME, "draw_now", NULL);
					}
                    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_BOOK)&&(!s_ctl_box_show))
                    {
						popdlg_destroy();
                        movie_box_show();
                    }
				}

            }
            movie_play_status_prompt_info(player_status);
#if MEDIA_SUBTITLE_SUPPORT
            if(PLAY_MOVIE_CTROL_RESUME == movie_box_button2_ctrol)
            {
                movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
            }
            else
            {
                if(movie_box_button1_bspeed == PLAY_MOVIE_SPEED_X1 && movie_box_button3_fspeed == PLAY_MOVIE_SPEED_X1)
                {
                    pmp_subt_para* subt_para = NULL;
                    subt_para = subtitle_get();

                    if(app_movie_dvb_subt_cnt() > 0)
                    {
                        movie_dvb_subt_start(s_dvb_subt_index);
#if OSD_SUBTITLE_SUPPORT
                        if(strcmp(GUI_GetFocusWindow(), WIN_MOVIE_VIEW) && PMPSET_TONE_ON == pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY))
                            app_osd_subt_pause();
#endif
                    }
                    else
                    {
                        if(subt_para != NULL && (s_frist_play == true))
                        {
                            extern char* play_movie_sub_get(void);
                            if(0 == player_info->sub_num && play_movie_sub_get() != NULL)
                                subt_para->para.type = PLAYER_SUB_TYPE_FILE;
                        }
                        subtitle_restart();
                    }

                    if(!s_frist_play)
                    {
                        app_subt_select_restore();
                    }
                    else
                    {
                        app_subt_select_init();
                        s_sub_delayms = 0;
                    }
                    app_set_subt_delay_time(s_sub_delayms);
                }
            }
#endif
			if(s_frist_play == true)
			{
				s_frist_play = false;
			}
			s_movie_manual_stop = FALSE;
			GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "hide");
			break;

		case PLAYER_STATUS_PLAY_START:
			s_is_movie_playing = false;
			APP_Printf("\n[SERVICE] PLAYER_STATUS_PLAY_START\n");
			// add in 20121128--for audio delay status recover
			// driver will clear too, so app must do following code.
			{
				extern int audio_delay_ms;
				audio_delay_ms = 0;
				movie_box_button2_ctrol = PLAY_MOVIE_CTROL_PLAY;
				movie_box_button2_setimg(movie_box_button2_ctrol);
				if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_SET))
					GUI_SetProperty("movie_set_button_delay", "string", "0ms");
			}
			break;

		default:
			break;
	}

	return 0;
}

void player_speed_change(GxMsgProperty_PlayerSpeedReport *spd)
{
    if(spd->speed == 1)
    {
#if MEDIA_SUBTITLE_SUPPORT
        app_set_subt_delay_time(s_sub_delayms);
#endif
    }
    if(spd->speed == 1 && (movie_box_button1_bspeed != PLAY_MOVIE_SPEED_X1))
	{
		popdlg_destroy();
		GUI_SetProperty(IMAGE_POPMSG, "state", "osd_trans_hide");
		GUI_SetProperty(TEXT_POPMSG, "state", "hide");
		movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;

        if(GUI_CheckDialog(WIN_MEDIA_INFO) == GXCORE_SUCCESS)
            GUI_EndDialog(WIN_MEDIA_INFO);

		if((GUI_CheckDialog(WIN_MEDIA_BAR) == GXCORE_SUCCESS)&&(GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
		{
			GUI_SetFocusWidget(SLIDER_CURSOR_MOVICE);
		}
		PlayerStatusInfo info;
		if(GXCORE_SUCCESS == GxPlayer_MediaGetStatus(PMP_PLAYER_AV, &info))
		{
			if(info.status == PLAYER_STATUS_PLAY_PAUSE)
			{
				GUI_SetProperty(IMAGE_POPMSG, "state", "show");
				GUI_SetProperty(TEXT_POPMSG, "state", "hide");
				GUI_SetProperty(IMAGE_POPMSG, "img", IMG_POP_PAUSE);
			}
		}

	}
}

SIGNAL_HANDLER  int movie_view_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	GxMsgProperty_PlayerAVCodecReport* avcodec_status = NULL;
	GxMsgProperty_PlayerSpeedReport *player_spd = NULL;

	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event *)usrdata;
	msg = (GxMessage*)event->msg.service_msg;
	switch(msg->msg_id)
	{
		case GXMSG_PLAYER_STATUS_REPORT:
			player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
			movie_service_status(player_status);

			break;

		case GXMSG_PLAYER_AVCODEC_REPORT:
			avcodec_status = (GxMsgProperty_PlayerAVCodecReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerAVCodecReport);
			APP_CHECK_P(avcodec_status, EVENT_TRANSFER_STOP);
			if(AVCODEC_ERROR == avcodec_status->acodec.state || AVCODEC_ERROR == avcodec_status->vcodec.state
                    || AVCODEC_UNSUPPORT == avcodec_status->acodec.state || AVCODEC_UNSUPPORT == avcodec_status->vcodec.state)
			{
				player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
				APP_CHECK_P(player_status, EVENT_TRANSFER_STOP);
				player_status->status = PLAYER_STATUS_ERROR;
				movie_service_status(player_status);
			}
			else
			{
			}
			break;
        case GXMSG_PLAYER_SPEED_REPORT:
			{
				player_spd = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerSpeedReport);
				player_speed_change(player_spd);
			}
            break;
		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

static int _movie_error_cb(PopDlgRet ret)
{
    GUI_EndDialog(WIN_MOVIE_VIEW);
    return 0;
}


static int _media_lock_play_cb(PopPwdRet *ret, void *usr_data)
{
    play_list* list = NULL;
    int start_time_ms = 0;
    char* path = NULL;

    if(ret->result == PASSWD_OK)
    {
        list = play_list_get(PLAY_LIST_TYPE_MOVIE);
        path = app_movie_view_get_movie_path();
        start_time_ms = pmp_load_tag(path);
        // modify by gosta 20220323 , issue 312569
        popmsg_topright_ctrl(PLAY_MOVIE_CTROL_PLAY, 0);
        play_movie(list->play_no, start_time_ms);
        s_media_playlock_state = GXCORE_SUCCESS;
    }
    else if(ret->result == PASSWD_ERROR)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_exit_cb = _media_lock_play_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {
        popmsg_centre_hide(NULL);
        popmsg_topright_hide(NULL);
        app_movie_view_end_dialog();
        GUI_SetProperty(IMAGE_MUTE, "state", "osd_trans_hide");

        GUI_EndDialog(WIN_MOVIE_VIEW);
        app_log_info("Cancel passwd wnd!!!!!");
    }
    return 0;
}

static void _media_lock_play(void)
{
    s_media_playlock_state = GXCORE_ERROR;
    int32_t lock_state = 0;
    GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &lock_state, CHANNEL_LOCK);

    if(lock_state == 1 && s_moview_psi_info.private_info.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        PasswdDlg passwd_dlg;
        s_frist_play = true;
        play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_exit_cb = _media_lock_play_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }
#if PARENTAL_LOCK_SUPPORT
    else
    {
        app_pvr_file_parental_rating_start(1,s_moview_psi_info.ts_id,s_moview_psi_info.service_id);
    }
#endif

}

static int _play_cb(PopDlgRet ret)
{
    if(ret != POP_VAL_OK)
    {
        s_start_time_ms = 0;
        char* path = NULL;
        path = app_movie_view_get_movie_path();

        /*save tag*/
        pmp_save_tag(path, s_start_time_ms);
    }

    /*play start*/
    play_movie(s_start_play_no, s_start_time_ms);
    _media_lock_play();
	GUI_SetInterface("flush",NULL);//pp enable together with osd

	/*set volume*/
	int32_t audio_vol = 0;
	GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
	app_system_vol_set(audio_vol);

	/*popmsg mute*/
	popmsg_mute_init();
    return 0;
}

#if PARENTAL_LOCK_SUPPORT
int app_media_parent_lock_play(void)
{
    if(s_media_playlock_state == GXCORE_SUCCESS)
         return 0;
    PasswdDlg passwd_dlg;
	char* path = NULL;
    PlayTimeInfo time_info = {0};
    status_t ret = GXCORE_ERROR;
	path = app_movie_view_get_movie_path();
    ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &time_info);
    if(GXCORE_SUCCESS == ret)
    {
        pmp_save_tag(path, time_info.current);
        s_frist_play = true;
        play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = STR_ID_PARENT_GUID;
        passwd_dlg.passwd_exit_cb = _media_lock_play_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
        return 0;
    }
    return -1;
}
#endif

SIGNAL_HANDLER int movie_view_init(const char* widgetname, void *usrdata)
{
	uint32_t value = 0;
	status_t ret = GXCORE_SUCCESS;

#if USB_MICROPHONE_SUPPORT
    {
        extern int app_microphone_play(void);
        app_microphone_play();
    }
#endif
	app_player_femem_control(true);
    app_system_performace_dynamic_change(FILE_PLAY_MODE);
	// clear mp3 file name
	clean_music_path_bak();
	// clean music play tag .
	clean_music_file_no_bak();

	s_frist_play = true;
	s_movie_video_ok = true;
	s_is_movie_playing = false;
	/*list get*/
	play_list* list = NULL;
	char* path = NULL;
	list = play_list_get(PLAY_LIST_TYPE_MOVIE);
	if(list == NULL){
		ret = GXCORE_ERROR;
		goto TO_MOVIE_INIT_ERR;
	}
	if(list->play_no >= list->nents){
		ret = GXCORE_ERROR;
		goto TO_MOVIE_INIT_ERR;
	}
	path = explorer_static_path_strcat(list->path, list->ents[list->play_no]);
	if(path == NULL){
		ret = GXCORE_ERROR;
		goto TO_MOVIE_INIT_ERR;
	}

	GUI_SetInterface("image_disable", NULL);
	GUI_SetProperty(WIN_MOVIE_VIEW, "back_ground", (void*)"null");
	GUI_SetInterface("flush", NULL);

	memset(&s_player_info, 0, sizeof(PlayerProgInfo));
	// for SPP display control
	if(GxPlayer_MediaGetProgInfoByUrl(path, &s_player_info) == 0) //success
	{
		// add 20121112, for background display
		if(s_player_info.is_radio == 1)
		{
			GUI_SetInterface("image_enable", NULL);
		}
	}

	movie_box_button1_bspeed = PLAY_MOVIE_SPEED_X1;
	movie_box_button3_fspeed = PLAY_MOVIE_SPEED_X1;
	movie_box_button6_zoom = PLAY_MOVIE_ZOOM_X1;
	s_movie_total_time=0;
    s_movie_seek_min_time = 0;
	s_movie_manual_stop= false;
	s_seek_flag = MOVIE_SEEK_NONE;
	s_start_play_no = list->play_no;

	if(GUI_CheckDialog(WND_PVR_MEIDA) == GXCORE_SUCCESS)
	{
		s_movie_play_sequence = pmpset_get_int(PMPSET_MOVIE_PLAY_SEQUENCE);
	//#40799 close init when pvr media
	}
	/*popmsg topright*/
	GUI_SetProperty(IMAGE_POPMSG, "state", "osd_trans_hide");
	GUI_SetProperty(TEXT_POPMSG, "state", "hide");

	/*popmsg centre*/
	GUI_SetProperty(COMBOBOX_POPMSG, "state", "osd_trans_hide");

	/*popmsg status*/
	GUI_SetProperty(TEXT_MOVIE_STATUS, "state", "osd_trans_hide");

	/*box*/
    movie_box_hide(NULL);
    s_start_time_ms = pmp_load_tag(path);
#if PLAYBACK_SPEED_SUPPORT
    _movie_playback_set_select(PLAYBACK_SPEED_DEFAULT_VALUE);
#endif
    if(0 < s_start_time_ms)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_CONTINUE_VIEW;
        pop.title = NULL;
        pop.exit_cb = _play_cb;
        pop.timeout_sec = 6;
        popdlg_create(&pop);
    }
    else
    {
        _play_cb(POP_VAL_CANCEL);
    }

	/*popmsg aspect*/
	value = pmpset_get_int(PMPSET_ASPECT_RATIO);
	pmpset_set_int(PMPSET_ASPECT_RATIO, value);


TO_MOVIE_INIT_ERR:
	if(ret == GXCORE_ERROR)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.mode = POP_MODE_UNBLOCK;
		pop.format = POP_FORMAT_DLG;
        pop.exit_cb = _movie_error_cb;
		pop.str = STR_ID_CAN_NOT_PLAY_FILE;
		pop.title = NULL;
		pop.timeout_sec = 6;
		popdlg_create(&pop);
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int movie_view_destroy(const char* widgetname, void *usrdata)
{
#if PARENTAL_LOCK_SUPPORT
	app_pvr_file_parental_rating_stop();
#endif
	s_delay_play_cnt = 0;
	/*popmsg topright*/
	APP_TIMER_REMOVE(popmsg_topright_timer_hide);

	/*popmsg centre*/
	APP_TIMER_REMOVE(popmsg_centre_timer_hide);

	/*box*/
	APP_TIMER_REMOVE(box_sliderbar_update_timer);
	APP_TIMER_REMOVE(box_sliderbar_active_timer);

	s_seek_flag = MOVIE_SEEK_NONE;
	s_is_movie_playing = false;

	APP_TIMER_REMOVE(delay_play_timer);

	/*list get*/
	char* path = NULL;
    path = app_movie_view_get_movie_path();

    /*save tag*/
    PlayTimeInfo time_info = {0};
    if (GxCore_FileExists(path) == GXCORE_FILE_UNEXIST)
    {
            pmp_save_tag(path, 0);
    }
    else
    {
        status_t ret = GXCORE_ERROR;
        ret = GxPlayer_MediaGetTime(PMP_PLAYER_AV, &time_info);
        if(GXCORE_SUCCESS == ret)
        {
            s_movie_total_time = time_info.totle - time_info.seek_min;
            if((0 != time_info.current)&& (0!= s_movie_total_time) &&(time_info.current != s_movie_total_time))
                pmp_save_tag(path, time_info.current);
        }
    }

	/*play stop*/
	play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);

#if MEDIA_SUBTITLE_SUPPORT
    if(app_movie_dvb_subt_cnt() == 0)
        subtitle_destroy();
    else
        movie_dvb_subt_stop();
    app_clean_subt_select();
#endif
#if PLAYBACK_SPEED_SUPPORT
    _movie_playback_set_select(PLAYBACK_SPEED_DEFAULT_VALUE);
#endif
    movie_box_hide(NULL);

    app_movie_psi_info_clean();
	s_frist_play = true;
	s_movie_video_ok = true;

	app_player_femem_control(false);
    app_system_performace_dynamic_change(DVB_PLAY_MODE);
	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int movie_media_view_keypress(const char* widgetname, void *usrdata)
{

	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
		GUI_SendEvent(WIN_MOVIE_VIEW, event);
	return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER  int movie_media_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event *)usrdata;

	if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
		GUI_SendEvent(WIN_MOVIE_VIEW, event);
	return EVENT_TRANSFER_STOP;
}

static void _movie_box_button_select(int sel)
{
    const char *focus_widget = GUI_GetFocusWidget();

    if(false == s_ctl_box_show)
        movie_box_show();
    if(focus_widget && strcmp(focus_widget, BOX_MOVIE_CTROL))
        GUI_SetFocusWidget(BOX_MOVIE_CTROL);
    GUI_SetProperty(BOX_MOVIE_CTROL, "select", &sel);
}


SIGNAL_HANDLER int movie_view_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event *)usrdata;
	switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
	{
		case VK_BOOK_TRIGGER:
			GUI_EndDialog("after wnd_full_screen");
			break;
		case APPK_BACK:
		case APPK_MENU:
			popmsg_centre_hide(NULL);
			popmsg_topright_hide(NULL);
			app_movie_view_end_dialog();
			GUI_SetProperty(IMAGE_MUTE, "state", "osd_trans_hide");

			GUI_EndDialog(WIN_MOVIE_VIEW);
			return EVENT_TRANSFER_STOP;

		case APPK_OK:
			popmsg_centre_hide(NULL);
			movie_box_show();
			return EVENT_TRANSFER_STOP;

        case APPK_INFO:
            {
                extern int app_fuse_common_pvr_info_create_dialog(void);
                popmsg_centre_hide(NULL);
                if(app_fuse_common_pvr_info_create_dialog() == -1)
                    _create_media_info_dlg();
            }
            return EVENT_TRANSFER_STOP;

		case APPK_VOLDOWN:
		case APPK_LEFT:
            _create_media_volume_dlg();
			event->key.sym = APPK_VOLDOWN;
			GUI_SendEvent(WND_VOLUME, event);
			return EVENT_TRANSFER_STOP;

		case APPK_VOLUP:
		case APPK_RIGHT:
             _create_media_volume_dlg();
			event->key.sym = APPK_VOLUP;
			GUI_SendEvent(WND_VOLUME, event);
			return EVENT_TRANSFER_STOP;

			/*shortcut keys
			*/
		case APPK_PREVIOUS:
            _movie_play_pre_or_next(MOVIE_BOX_BUTTON_PREVIOUS);
			return EVENT_TRANSFER_STOP;

		case APPK_REW:
            movie_box_ctrl(MOVIE_BOX_BUTTON_SPEED_BACKWARD);
			return EVENT_TRANSFER_STOP;

		case APPK_PAUSE_PLAY:
			popmsg_centre_hide(NULL);
            _movie_box_button_select(MOVIE_BOX_BUTTON_PLAY_PAUSE);
			movie_box_ctrl(MOVIE_BOX_BUTTON_PLAY_PAUSE);
			return EVENT_TRANSFER_STOP;

			// add in 20120206, for single key for pause
		case APPK_PAUSE:
			if(!s_is_movie_playing || PLAY_MOVIE_CTROL_STOP == movie_box_button2_ctrol)
				return EVENT_TRANSFER_STOP;

            popmsg_centre_hide(NULL);
            _movie_box_button_select(MOVIE_BOX_BUTTON_PLAY_PAUSE);
            movie_box_ctrl(MOVIE_BOX_BUTTON_PAUSE);
			return EVENT_TRANSFER_STOP;

		case APPK_PLAY:
			popmsg_centre_hide(NULL);
            _movie_box_button_select(MOVIE_BOX_BUTTON_PLAY_PAUSE);
			movie_box_ctrl(MOVIE_BOX_BUTTON_PLAY);
			return EVENT_TRANSFER_STOP;

		case APPK_FF:
            movie_box_ctrl(MOVIE_BOX_BUTTON_SPEED_FORWARD);
			return EVENT_TRANSFER_STOP;

        case APPK_NEXT:
            _movie_play_pre_or_next(MOVIE_BOX_BUTTON_NEXT);
            return EVENT_TRANSFER_STOP;

		case APPK_STOP:
			popmsg_centre_hide(NULL);
            _movie_box_button_select(MOVIE_BOX_BUTTON_STOP);
			s_movie_manual_stop = TRUE;
			movie_box_ctrl(MOVIE_BOX_BUTTON_STOP);
			return EVENT_TRANSFER_STOP;

        case APPK_YELLOW:
		case APPK_SUBT:
#if MEDIA_SUBTITLE_SUPPORT
            _create_media_sub_dlg();
#else
            _create_media_info_dlg();
#endif
			return EVENT_TRANSFER_STOP;
        case APPK_GREEN:
#if PLAYBACK_SPEED_SUPPORT
            movie_box_ctrl(MOVIE_BOX_BUTTON_PLAYBACK_SPEED);
#else
             _create_media_volume_dlg();
#endif
            return EVENT_TRANSFER_STOP;
		case APPK_SET:
        case APPK_SEEK:
            _create_media_set_dlg();
			return EVENT_TRANSFER_STOP;

		case APPK_AUDIO:
			popmsg_centre_ctrl(MOVIE_COMBO_TRACK);
			return EVENT_TRANSFER_STOP;

		case APPK_RATIO:
			if(PLAY_MOVIE_CTROL_PLAY == movie_box_button2_ctrol
					&& s_player_info.is_radio == 0)
			{
				popmsg_centre_ctrl(MOVIE_COMBO_RATIO);
			}
			return EVENT_TRANSFER_STOP;

		case APPK_MUTE:
			if(delay_play_timer != NULL)
			{
				return EVENT_TRANSFER_STOP;
			}
			popmsg_mute_ctrl();
			return EVENT_TRANSFER_STOP;
		case APPK_REPEAT:
			{
				int value;
				int select = 2;

				value = pmpset_get_int(PMPSET_MOVIE_PLAY_SEQUENCE);
				if(value == PMPSET_MOVIE_PLAY_SEQUENCE_SEQUENCE)
				{
					value = PMPSET_MOVIE_PLAY_SEQUENCE_ONLY_ONCE;
				}
				else
				{
					value++;
				}
				pmpset_set_int(PMPSET_MOVIE_PLAY_SEQUENCE, value);

				GUI_CreateDialog(WIN_MOVIE_SET);
				GUI_SetProperty("movie_set_box", "select", &select);

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

	return EVENT_TRANSFER_KEEPON;
}

#if HDMI_CEC_SUPPORT
play_movie_ctrol_state app_get_play_movie_ctrol_state(void)
{
    return movie_box_button2_ctrol;
}
#endif
#if VOICE_CONTROL_SUPPORT
int app_movie_play_status_get(void)
{
    if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_PLAY)
        return 2;// playing
    else if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_PAUSE)
        return 1;// pause
    else if(movie_box_button2_ctrol == PLAY_MOVIE_CTROL_STOP)
        return 0;// stop
    return -1;
}
#endif

