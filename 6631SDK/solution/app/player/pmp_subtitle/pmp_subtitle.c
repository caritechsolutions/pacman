#include "app_config.h"
#if MEDIA_SUBTITLE_SUPPORT
#include "play_movie.h"
#include "pmp_subtitle.h"
#include "gdi_core.h"
#include "app_windows.h"
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#endif

static pmp_subt_para subt_para = {0};

static void play_movie_info_and_sub_bak_clear(void);

static bool check_base_name(const char* check_file, const char* play_file)
{
	int check_file_len = 0;
	int play_file_len = 0;
	int base_name_len = 0;
	int i = 0;

	if(NULL == check_file || NULL == play_file)
		return false;

	check_file_len = strlen(check_file);
	play_file_len = strlen(play_file);

	for(i = play_file_len - 1; i >= 0 ; i--)
	{
		if('.' == play_file[i])
			break;
	}

	if(0 == i)
        return false;

	base_name_len = i;

	if(check_file_len <= base_name_len)
		return false;

	if(0 == strncasecmp((const char*)check_file, play_file, base_name_len))
		return true;

	return false;
}

static char* subt_getfile(void)
{
	GxDirent* ent = NULL;
	int count = 0;
	int i = 0;
	play_movie_info movie_info;
	explorer_para* explorer_subt = NULL;

	if(NULL == explorer_view) return NULL;
	explorer_subt = explorer_opendir(explorer_view->path, SUFFIX_SUBT);
	if(NULL == explorer_subt) return NULL;

	play_movie_get_info(&movie_info);

	count = explorer_subt->nents;
	for(i = 0; i < count; i++)
	{
		ent = explorer_subt->ents + i;
		if(NULL == ent)
		{
			explorer_closedir(explorer_subt);
			return NULL;
		}

		if(GX_FILE_REGULAR == ent->ftype)
		{
			if(check_base_name(ent->fname, movie_info.name))
			{
				char* path = NULL;
				path = explorer_static_path_strcat(movie_info.path, ent->fname);
				explorer_closedir(explorer_subt);
				return path;
			}
		}
	}

	explorer_closedir(explorer_subt);
	return NULL;
}

int subtitle_create(char *player)
{
	app_log_debug("subtitle_create\n");
    if(NULL == player)
        return -1;

	memset(&subt_para, 0, sizeof(pmp_subt_para));

	subt_para.cur_subt = 0;
	subt_para.para.pid.pid = 0;
	subt_para.para.pid.major = 0;
	subt_para.para.pid.minor = 0;

    subt_para.player = GxCore_Strdup(player);
    if(NULL == subt_para.player)
        return -1;

#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_init();
#endif

	return 0;
}

void subtitle_destroy(void)
{
	app_log_debug("subtitle_destroy\n");

    app_clear_movie_subt_path();
    play_movie_info_and_sub_bak_clear();

	//stop
	subtitle_stop();
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_deinit();
#endif
	//para
    APP_FREE(subt_para.file);
    APP_FREE(subt_para.player);
    subt_para.para.file_name = NULL;
    return;
}


typedef struct {
	char* sub_file;
	play_movie_info movie_info;
}play_movie_info_and_sub_t;

static play_movie_info_and_sub_t  s_movie_info_sub_bak = {
	.sub_file = NULL,
	.movie_info = {
		.no = 0,
		.path = NULL,
		.name = NULL
	}
};

void play_movie_info_and_sub_bak_set(char* file)
{
    play_movie_info movie_info;
    play_movie_get_info(&movie_info);

    APP_FREE(s_movie_info_sub_bak.sub_file);
    s_movie_info_sub_bak.sub_file = GxCore_Strdup(file);
    s_movie_info_sub_bak.movie_info.no = movie_info.no;

    APP_FREE(s_movie_info_sub_bak.movie_info.path);
    s_movie_info_sub_bak.movie_info.path = GxCore_Strdup(movie_info.path);

    APP_FREE(s_movie_info_sub_bak.movie_info.name);
    s_movie_info_sub_bak.movie_info.name = GxCore_Strdup(movie_info.name);
}

static void play_movie_info_and_sub_bak_clear(void)
{
    APP_FREE(s_movie_info_sub_bak.sub_file);
    APP_FREE(s_movie_info_sub_bak.movie_info.path);
    APP_FREE(s_movie_info_sub_bak.movie_info.name);
}

char *play_movie_sub_get(void)
{
    status_t ret = GXCORE_ERROR;
	play_movie_info movie_info;

	ret = play_movie_get_info(&movie_info);
	if((GXCORE_SUCCESS == ret
            && NULL != s_movie_info_sub_bak.movie_info.path)
			&& 0 == strcmp(s_movie_info_sub_bak.movie_info.path, movie_info.path)
			&& (NULL != s_movie_info_sub_bak.movie_info.name)
			&& 0 == strcmp(s_movie_info_sub_bak.movie_info.name, movie_info.name))
	{
		if(NULL != s_movie_info_sub_bak.sub_file)
            return s_movie_info_sub_bak.sub_file;
		else
			return subt_getfile();
	}
	else
	{
		return subt_getfile();
	}
}

int subtitle_start(char *file, pmp_subt_load_type type)
{
	char* file_load = NULL;

	//unload old
	subtitle_stop();

	if(type == PMP_SUBT_LOAD_OUTSIDE || type == PMP_SUBT_LOAD_INIT)
	{
		//get file
		if(NULL == file)
			file_load = play_movie_sub_get();
		else
			file_load = file;

		app_log_debug("file: %s\n", file_load);
	}

	if(file_load != NULL)//outside subt
	{
		subt_para.para.type = PLAYER_SUB_TYPE_FILE;
        APP_FREE(subt_para.file);
		subt_para.file = GxCore_Strdup(file_load);
		if(NULL == subt_para.file)
			return -1;
		subt_para.para.file_name = subt_para.file;
        subt_para.list = GxPlayer_MediaSubLoad(subt_para.player, &(subt_para.para));
	}
	else//inside subt
	{
		if(type == PMP_SUBT_LOAD_OUTSIDE)
		{
			subt_para.para.type = PLAYER_SUB_TYPE_FILE;
			subt_para.para.file_name = NULL;
			return 0;
		}

		subt_para.para.type = PLAYER_SUB_TYPE_INSIDE;
		subt_para.para.file_name = NULL;
        PlayerProgInfo *player_info = NULL;
        player_info = app_movie_player_info_get();
#if NETAPPS_SUPPORT
        if(strcmp(subt_para.player,PMP_PLAYER_AV) != 0)
        {
            extern void* app_get_netvideo_playinfo(void);
            player_info = (PlayerProgInfo *)app_get_netvideo_playinfo();
        }
#endif
        if(player_info->sub_num>0)
        {
            subt_para.para.pid.pid = app_get_subt_real_select(0);
#if NETAPPS_SUPPORT
            subt_para.para.pid.pid = 0;
#endif
            subt_para.list = GxPlayer_MediaSubLoad(subt_para.player, &(subt_para.para));
        }
	}

	app_log_debug("load: %s,player: %s\n", subt_para.file, subt_para.player);

	return 0;
}

void subtitle_stop(void)
{
	if(subt_para.file)
		app_log_debug("%s\n", subt_para.file);
	if(subt_para.player)
		app_log_debug("%s\n", subt_para.player);

	//unload
	if(subt_para.list)
	{
		GxPlayer_MediaSubUnLoad(subt_para.player, subt_para.list);
		subt_para.list = NULL;
	}
}

void subtitle_pause(void)
{
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_pause();
#endif
}

void subtitle_resume(void)
{
#if OSD_SUBTITLE_SUPPORT
    app_osd_subt_resume();
#endif
}

pmp_subt_para *subtitle_get(void)
{
	return &subt_para;
}

pmp_subt_load_type app_bustype_to_apptype(PlayerSubType sub_type)
{
    pmp_subt_load_type type;
    switch (sub_type)
    {
        case PLAYER_SUB_TYPE_INSIDE:
            type = PMP_SUBT_LOAD_INSIDE;
            break;
        case PLAYER_SUB_TYPE_FILE:
            type = PMP_SUBT_LOAD_OUTSIDE;
            break;
        default:
            type = PMP_SUBT_LOAD_INSIDE;
            break;
    }
    return type;
}

int subtitle_restart(void)
{
    pmp_subt_load_type type;
    if(NULL == subt_para.player)
        return -1;

    type = app_bustype_to_apptype(subt_para.para.type);
    return subtitle_start(NULL, type);
}
#endif
