#ifndef APP_NETVIDEO_PLAY_H
#define APP_NETVIDEO_PLAY_H
#include "app_config.h"
#include "app_netvideo_epg.h"

typedef struct _NetVideoSubtitleItem
{
  char *title;
  char *url;
}NetVideoSubtitleItem;

typedef struct _NetVideoSubtitleList
{
  int subtitle_num;
  int cur_subtitle;
  NetVideoSubtitleItem *subtitle_item;
}NetVideoSubtitleList;

typedef struct _AppMsgVideoPlayer
{
    bool show_info;
}AppMsgVideoPlayer;

typedef struct
{
  void (*play_ok)(void);
  void (*play_exit)(void);
  void (*play_create)(void);
  void (*play_tv_radio)(void);
  void (*play_menu)(void);
  void (*play_prev)(void);
  status_t (*play_next)(int force);
  void (*play_restart)(uint64_t cur_time);
  int (*play_list_total_get)(void);
  char *(*play_list_data_get)(int sel);
  char *(*play_list_image_get)(int sel);
  int (*play_list_ok)(int item);
  int (*play_list_red)(int *item);
  int (*play_list_green)(int *item);
  int (*play_list_yellow)(int *item);
  int (*play_list_blue)(int *item);
  int (*play_list_cmb_title_create)(char **cmb_title);
  void (*play_list_cmb_title_free)(char **cmb_title);
  int (*play_list_change_group)(int sel);
  int (*play_list_fav_check)(int sel);
  int (*play_list_destroy)(void);
  char* str_red;
  char* str_green;
  char* str_yellow;
  char* str_blue;
}NetVideoPlayCtrl;

typedef struct
{
  void (*get_url)(int);
  NetVideoSubtitleList *(*get_subtitle_list)(int);
  char *bar_title;
  char *netvideo_title;
  char *netvideo_url;
  char *netvideo_image;
  int source_num;
  int cmb_sel;
  int play_mode;
  int list_focus_item;
  bool play_wnd_created;
  NetVideoPlayCtrl play_ctrl;
  NetappsEpgCtrl epg_ctrl;
}NetVideoPlayOps;

typedef enum
{
    PLAY_STATE_NONE,
    PLAY_STATE_LOAD,
    PLAY_STATE_RUNNING,
    PLAY_STATE_ERROR,
    PLAY_STATE_PAUSE,
}NetPlayState;

typedef enum
{
    PLAY_PLAYER_ERROR,
    PLAY_CONNECT_ERROR,
    PLAY_URL_NONE,
    PLAY_SYSTEM_BUSY,
    PLAY_USER_ERROR,
}NetPlayErrorType;

typedef enum
{
    SUBT_STATE_IDLE = 0,
    SUBT_STATE_NEED_URL,
    SUBT_STATE_GET_URL,
    SUBT_STATE_NEED_DOWNLOAD,
    SUBT_STATE_DOWNLOADING,
    SUBT_STATE_NEED_DISPLAY,
    SUBT_STATE_RUNNING,
}SubtState;

void app_net_video_free_subtitlelist(NetVideoSubtitleList *subtitlelist);
void app_net_video_subtitle_pause(void);
void app_net_video_subtitle_resume(void);
void app_net_video_subtitle_status_set(SubtState status,int index);
status_t app_net_picture_play(char *url);
status_t app_net_video_play(NetVideoPlayOps *play_ops);
status_t app_net_set_music_background(char *url);
void app_net_set_video_mode(void);
status_t app_net_video_start_play(char *url, NetPlayState play_state, int64_t start, bool show_info);
void app_net_video_set_error_str(char *errstr);
void app_net_video_switch_source(int index);
void *app_get_netvideo_playinfo(void);


status_t app_netvideo_play_listview_call(NetVideoPlayOps *play_ops);
status_t app_netvideo_play_number_call(NetVideoPlayOps *play_ops);
status_t app_net_video_stop_play(void);
status_t app_epg_playback(NetVideoPlayOps *play_ops);
void app_net_video_play_halt(void);
void app_net_video_play_resume(void);

#endif
