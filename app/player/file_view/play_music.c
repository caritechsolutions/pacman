#include "app.h"
#include "play_manage.h"
#include "play_music.h"
#include "stdlib.h"

static uint32_t file_no_bak = ~(0);
char path_bak[255]={0};

status_t play_music(uint32_t file_no)
{
	play_list* list = NULL;
	char* path = NULL;

	list = play_list_get(PLAY_LIST_TYPE_MUSIC);
	if(NULL == list) return GXCORE_ERROR;

	if(file_no >= list->nents)
	{
		return GXCORE_ERROR;
	}

	list->play_no = file_no;
	path = explorer_static_path_strcat(list->path, list->ents[file_no]);
	if(NULL == path) return GXCORE_ERROR;

	app_log_info("[PLAY] music: %s\n", path);
	play_av_by_url(path, 0);

	file_no_bak = list->play_no;
	memset(path_bak, 0, 255);
	strncpy(path_bak, path, 254);
#if BLUETOOTH_SUPPORT
{
    extern int app_bluez_source_play(void);
    app_bluez_source_play();
}
#endif

	return GXCORE_SUCCESS;
}

status_t play_music_ctrol(play_music_ctrol_state ctrol)
{
	status_t ret = GXCORE_SUCCESS;
	play_list* list = NULL;
	uint32_t file_no = 0;
	uint32_t file_count = 0;

	list = play_list_get(PLAY_LIST_TYPE_MUSIC);
	if(NULL == list) return GXCORE_ERROR;

	file_no = list->play_no;
	file_count = list->nents;
	if(0 == file_count)
	{
		return GXCORE_ERROR;
	}


	if(PLAY_MUSIC_CTROL_PREVIOUS == ctrol)
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
		ret = play_music(file_no);
	}
	else if(PLAY_MUSIC_CTROL_NEXT == ctrol)
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
		ret = play_music(file_no);
	}
	else if(PLAY_MUSIC_CTROL_RANDOM== ctrol)
	{
		int random_no = rand();
		random_no = random_no%file_count;
		if(random_no == list->play_no)
		{
            random_no += 1;
            random_no = (random_no < file_count)? random_no : 0;
		}
		ret = play_music(random_no);
	}
	else if(PLAY_MUSIC_CTROL_PLAY == ctrol)
	{
		play_music(list->play_no);;
	}
	else if(PLAY_MUSIC_CTROL_PAUSE == ctrol)
	{
#if BLUETOOTH_SUPPORT
    extern int app_bluez_source_pause(void);
    app_bluez_source_pause();
#endif
        play_av_by_pause();
	}
	else if(PLAY_MUSIC_CTROL_RESUME == ctrol)
	{
		//TODO: after stop, resume change to play
		//play_music(list->play_no, 0);
        play_av_by_resume();
#if BLUETOOTH_SUPPORT
{
    extern int app_bluez_source_play(void);
    app_bluez_source_play();
}
#endif
	}
	else if(PLAY_MUSIC_CTROL_STOP == ctrol)
	{
        play_av_by_stop();
	}
	else
	{
		return GXCORE_ERROR;
	}

	return ret;
}


status_t play_music_speed(play_music_ctrol_state ctrol, play_music_speed_state speed)
{
	if(PLAY_MUSIC_SPEED_X0 == speed)
	{
		return GXCORE_SUCCESS;
	}

	if(PLAY_MUSIC_CTROL_BACKWARD== ctrol)
	{
		;
	}
	else if(PLAY_MUSIC_CTROL_FORWARD== ctrol)
	{
		;
	}
	else
	{
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

status_t play_music_get_info(play_music_info *info)
{
	play_list* list = NULL;

	list = play_list_get(PLAY_LIST_TYPE_MUSIC);
	if(NULL == list) return GXCORE_ERROR;

	// TODO:
	memset(info, 0, sizeof(play_music_info));
	info->no = list->play_no;
	info->path = list->path;
	info->name = list->ents[list->play_no];

	return GXCORE_SUCCESS;
}

PlayerID3Info* play_music_get_id3_info(void)
{
	play_list* list = NULL;
	char* path = NULL;
	PlayerID3Info *id3_info = NULL;

	list = play_list_get(PLAY_LIST_TYPE_MUSIC);
	if(NULL == list) return 0;

	// TODO:

	path = explorer_static_path_strcat(list->path, list->ents[list->play_no]);
	if(NULL == path) return 0;
	app_log_info("[ID3] get start: %s\n", path);
	id3_info = GxPlayer_MediaGetID3Info((const char*)path);
	app_log_info("[ID3] get finish\n");

	return id3_info;
}

int play_music_file_no_bak(void)
{
	return file_no_bak;
}

void clean_music_file_no_bak(void)
{
    file_no_bak = ~(0);
}
void clean_music_path_bak(void)
{
	memset(path_bak, 0, 255);
}
