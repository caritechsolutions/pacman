#include "app.h"
#include "file_view.h"
#include "play_movie.h"
#include "pmp_psi.h"
#include "play_manage.h"
#include "pmp_dvb_subt.h"
#include "stdlib.h"
#include "app_windows.h"
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#endif

extern status_t app_movie_play_config(void);
extern status_t app_movie_prog_info_update(char *file_path);
extern status_t app_movie_psi_info_init(char *file_path);
extern PmpPsi *app_movie_psi_info_get(void);

#if MEDIA_SUBTITLE_SUPPORT
extern int app_movie_dvb_subt_cnt(void);
#endif

void play_get_psi_info(char* path,PmpPsi* psi_info)
{
    PmpPsi* info = NULL;
    app_movie_psi_info_init(path);
    info = app_movie_psi_info_get();
    memcpy(psi_info,info,sizeof(PmpPsi));
}

status_t play_movie(uint32_t file_no, int start_time_ms)
{
    play_list* list = NULL;
    char *path = NULL;

#if MEDIA_SUBTITLE_SUPPORT
    if(app_movie_dvb_subt_cnt() > 0)
        movie_dvb_subt_stop();
    else
        subtitle_destroy();
#endif

#if BLUETOOTH_SUPPORT
{
    extern int app_bluez_source_play(void);
    app_bluez_source_play();
}
#endif
    list = play_list_get(PLAY_LIST_TYPE_MOVIE);
    if(NULL == list) return GXCORE_ERROR;

    if(file_no >= list->nents)
    {
        return GXCORE_ERROR;
    }

    list->play_no = file_no;
    path = explorer_static_path_strcat(list->path, list->ents[file_no]);
    if(NULL == path) return GXCORE_ERROR;

    app_movie_prog_info_update(path);
    app_movie_psi_info_init(path);
    app_movie_play_config();
    app_log_info("[PLAY] movie: %s, start: %d\n", path, start_time_ms);
    GUI_SetProperty("movie_view_text_name1","string","");

    extern void movie_player_back_ground_set(void);
    movie_player_back_ground_set();
    play_av_by_url(path, start_time_ms);

#if MEDIA_SUBTITLE_SUPPORT
    pmpset_set_int(PMPSET_MOVIE_SUBT_VISIBILITY, PMPSET_TONE_ON);
    if(0 == app_movie_dvb_subt_cnt())
        subtitle_create(PMP_PLAYER_AV);
#endif

    return GXCORE_SUCCESS;
}

status_t play_movie_ctrol(play_movie_ctrol_state ctrol)
{
	status_t ret = GXCORE_SUCCESS;
	play_list* list = NULL;
	uint32_t file_no = 0;
	uint32_t file_count = 0;

	list = play_list_get(PLAY_LIST_TYPE_MOVIE);
	if(NULL == list) return GXCORE_ERROR;

	file_no = list->play_no;
	file_count = list->nents;
	if(0 == file_count)
	{
		return GXCORE_ERROR;
	}


	if(PLAY_MOVIE_CTROL_PREVIOUS == ctrol)
	{
		if(0 == file_count) return GXCORE_SUCCESS;

		if(1 == file_count)
		{
			file_no = 0;
		}
		else if(file_no <=0)
		{
			file_no = file_count  - 1;
		}
		else
		{
			file_no--;
		}
		ret = play_movie(file_no, 0);
	}
	else if(PLAY_MOVIE_CTROL_NEXT == ctrol)
	{
		if(0 == file_count) return GXCORE_SUCCESS;

		if(1 == file_count)
		{
			file_no = 0;
		}
		else if(file_no >= file_count -1)
		{
			file_no = 0;
		}
		else
		{
			file_no++;
		}
		ret = play_movie(file_no, 0);
	}
	else if(PLAY_MOVIE_CTROL_PLAY == ctrol)
	{
		play_movie(list->play_no, 0);
	}
	else if(PLAY_MOVIE_CTROL_PAUSE == ctrol)
	{
        play_av_by_pause();
	}
	else if(PLAY_MOVIE_CTROL_RESUME == ctrol)
	{
        play_av_by_resume();
	}
	else if(PLAY_MOVIE_CTROL_STOP == ctrol)
	{
        play_av_by_stop();
	}
	else if(PLAY_MOVIE_CTROL_RANDOM== ctrol)
	{
		int random_no = rand();
		random_no = random_no%file_count;
		if(random_no == list->play_no)
		{
            random_no += 1;
            random_no = (random_no < file_count)? random_no : 0;
		}
		ret = play_movie(random_no, 0);
	}
	else
	{
		return GXCORE_ERROR;
	}

	return ret;
}


status_t play_movie_speed(play_movie_ctrol_state ctrol, play_movie_speed_state speed)
{
	GxMsgProperty_PlayerSpeed player_speed;
	GxMessage *msg;

	if(PLAY_MOVIE_SPEED_X1 == speed)
	{
		player_speed.speed = (float)speed;
	}
	else
	{
		if(PLAY_MOVIE_CTROL_BACKWARD== ctrol)
		{
			player_speed.speed = (0 - (float)speed);
		}
		else if(PLAY_MOVIE_CTROL_FORWARD== ctrol)
		{
			player_speed.speed = (float)speed;
		}
		else
		{
			player_speed.speed = (float)(PLAY_MOVIE_SPEED_X1);
		}
	}

	app_log_debug("start speed %f\n", player_speed.speed);

	msg = GxBus_MessageNew(GXMSG_PLAYER_SPEED);
	APP_CHECK_P(msg, GXCORE_ERROR);

	GxMsgProperty_PlayerSpeed *spd;
	spd = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerSpeed);
	APP_CHECK_P(spd, GXCORE_ERROR);
	spd->player = PMP_PLAYER_AV;
	spd->speed = player_speed.speed;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);

#if MEDIA_SUBTITLE_SUPPORT
	if(PLAY_MOVIE_SPEED_X1 == speed)
	{
		if(app_movie_dvb_subt_cnt() > 0)
		{
			extern int app_movie_dvb_subt_index(void);
			int dvb_subt_index = app_movie_dvb_subt_index();
			movie_dvb_subt_start(dvb_subt_index);
		}
		else
		{
            subtitle_restart();
        }
        app_subt_select_restore();
        app_subt_visibility_restore();
#if OSD_SUBTITLE_SUPPORT
        if(PMPSET_TONE_ON == pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY))
            app_osd_subt_pause();
#endif
    }
	else
	{
		if(app_movie_dvb_subt_cnt() > 0)
		{
			movie_dvb_subt_stop();
		}
		else
		{
			subtitle_stop();
		}
	}
#endif

	return GXCORE_SUCCESS;
}

status_t play_movie_zoom(play_movie_zoom_state zoom)
{
	return GXCORE_SUCCESS;
}

status_t play_movie_zoom_by_path(char *path, PlayerWindow *window, time_t start_time_ms)
{
    GxMsgProperty_PlayerVideoWindow play_wnd;
    static PlayerWindow s_NormalRect = {0, 0, APP_XRES, APP_YRES};

    memcpy(&s_NormalRect, window, sizeof(PlayerWindow));
    play_wnd.player = PMP_PLAYER_AV;
    memcpy(&(play_wnd.rect), window, sizeof(PlayerWindow));

    app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW, (void *)(&play_wnd));
    return play_av_by_url(path, start_time_ms);
}

status_t play_movie_stop(void)
{
    return play_av_by_stop();
}

status_t play_movie_get_info(play_movie_info *info)
{
	play_list* list = NULL;

	list = play_list_get(PLAY_LIST_TYPE_MOVIE);
	if(NULL == list) return GXCORE_ERROR;

	// TODO:
	memset(info, 0, sizeof(play_movie_info));
	info->no = list->play_no;
	info->path = list->path;
	info->name = list->ents[list->play_no];

	return GXCORE_SUCCESS;
}




