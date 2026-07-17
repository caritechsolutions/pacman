#ifndef __PMP_SUBTITLE_H__
#define __PMP_SUBTITLE_H__

#include "gxcore.h"
#include "module/player/gxplayer_module.h"
#include "gui_timer.h"
#include "gui_core.h"
#include "app_data_type.h"
#include "app.h"
#define SUFFIX_SUBT		"idx;IDX;srt;SRT;ssa;SSA;ass;ASS;smi;SMI;sub;SUB;vtt;VTT"

typedef struct
{
    int cur_subt;
    int delay_ms;
    char *file;
    PlayerSubtitle *list;
    PlayerSubPara para;
    char *player;
}pmp_subt_para;

typedef enum{
	PMP_SUBT_LOAD_INIT,
	PMP_SUBT_LOAD_OUTSIDE,
	PMP_SUBT_LOAD_INSIDE,
	PMP_SUBT_LOAD_ERROR
}pmp_subt_load_type;

int subtitle_create(char *player);
void subtitle_destroy(void);
int subtitle_start(char* file, pmp_subt_load_type type);
void subtitle_stop(void);
int subtitle_restart(void);
pmp_subt_para *subtitle_get(void);
void subtitle_pause(void);
void subtitle_resume(void);

int app_clear_movie_subt_path(void);
int app_subt_visibility_restore(void);
int app_subt_select_restore(void);
int app_subt_select_init(void);
int app_get_subt_real_select(int select);
int app_get_subt_real_type(pmp_subt_load_type type, int select);
PlayerProgInfo* app_movie_player_info_get(void);
void play_movie_info_and_sub_bak_set(char* file);
#endif

