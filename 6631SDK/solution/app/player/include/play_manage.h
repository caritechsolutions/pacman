#ifndef __PLAY_MANAGE_H__
#define __PLAY_MANAGE_H__

#include "gxtype.h"
#include "gxcore.h"
#include "file_view.h"
#include "pmp_explorer.h"
#include "gxbus.h"
#include "gxavdev.h"
#include "gxmsg.h"



#ifdef __cplusplus
extern "C" {
#endif

#define PMP_PLAYER_AV	"player_av"
#define PMP_PLAYER_PIC	"player_pic"

#define PLAY_Printf(...)	app_log_debug(__VA_ARGS__)
#define PLAY_FREE(x)	if(x){GxCore_Free(x);x=NULL;}

typedef enum
{
	PLAY_LIST_TYPE_MOVIE,
	PLAY_LIST_TYPE_MUSIC,
	PLAY_LIST_TYPE_PIC,
	PLAY_LIST_TYPE_TEXT
}play_list_type;


typedef struct
{
	play_list_type type;
	int	play_no;
	char* path;
	int nents;
	char** ents;
}play_list;

void play_av_display_size_str(uint64_t size, char* str);
void play_av_window_set(char* wnd_name);
void play_av_window_set_full_screen(void);
void play_pic_window_set(char* wnd_name);
void play_pic_window_set_full_screen(void);
status_t play_init(void);
status_t play_av_by_url(const char* url, int start_time_ms);
status_t play_av_by_stop(void);
status_t play_av_by_pause(void);
status_t play_av_by_resume(void);
status_t play_av_by_path_file(const char* path, const char* file, int start_time_ms);

//status_t play_list_init(file_view_group group, int start_play_file);
//status_t play_list_get(file_view_group group, play_list** list);
//status_t play_list_clear(file_view_group group);



status_t play_list_clear(play_list* list);
status_t play_list_init(play_list_type type, explorer_para* explorer, int select_file_no);
play_list* play_list_get(play_list_type type);

#ifdef __cplusplus
}
#endif

#endif /* __PLAY_MANAGE_H__ */

