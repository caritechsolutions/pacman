#ifndef __APP_NETVIDEO_H__
#define __APP_NETVIDEO_H__
#include "app_common_view.h"

#define NETVIDEO_MAX_PAGE_ITEM      COMMON_VIEW_MAX_PAGE_ITEM

typedef enum
{
	OBJ_VIDEO_GROUP = 0,
	OBJ_VIDEO_PAGE,
	OBJ_VIDEO_TOTAL,
}NetVideoObj;

typedef struct
{
	char *video_duration;
	char *video_title;
	char *video_author;
	char *video_viewcnt;
	char *video_pic;
}NetVideoItem;

typedef struct
{
	int (*video_busy)(void);
	void (*video_page_change)(int page_num);
	void (*video_exit)(void);
	int (*get_video_depth)(void);
	void (*set_video_depth)(int);
	void (*video_sel_change)(int video_sel);
	void (*video_group_change)(int group_sel);
	void (*obj_change)(NetVideoObj obj);
	void (*ok_press)(void);
	void (*service)(void *usrdata);
}NetVideoMenuCb;

status_t app_net_video_create(char *menu_title, char *menu_logo, NetVideoMenuCb *video_cb, int group_sel);
status_t app_net_video_page_item_update(int video_num, NetVideoItem *video_item);
void app_net_video_time_timer_stop(void);
void app_net_video_time_timer_reset(void);
int app_net_video_draw_gif(void* usrdata);
void app_net_video_show_gif(void);
void app_net_video_hide_gif(void);
bool app_net_video_get_popup_msg_on(void);
void app_net_video_hide_popup_msg(void);
void app_net_video_show_popup_msg(char* pMsg, int time_out);
void app_net_video_page_info_update(int page_total, int page_num);
void app_net_video_enable_group_roll(void);
void app_net_video_disable_group_roll(void);
void app_net_video_set_group_sel(unsigned int sel);
void app_net_video_set_group_active(unsigned int sel);
void app_net_video_set_menukey_string(char *str);
void app_net_video_enable_menu_key(char *tip, void (*func)(void));
void app_net_video_disable_menu_key(void);
void app_net_video_enable_red_key(char *tip, void (*func)(void));
void app_net_video_disable_red_key(void);
void app_net_video_enable_green_key(char *tip, void (*func)(void));
void app_net_video_disable_green_key(void);
void app_net_video_enable_yellow_key(char *tip, void (*func)(void));
void app_net_video_disable_yellow_key(void);
void app_net_video_enable_blue_key(char *tip, void (*func)(void));
void app_net_video_disable_blue_key(void);
void app_net_video_clear_all_item(void);
void app_net_video_page_item_set_focus(int focus);
void app_net_video_group_update(int group_total, char **group_str);
status_t app_net_video_set_obj(NetVideoObj obj);
void app_net_video_pic_download_stop(void);
void app_net_video_pic_download_start(void);
int app_net_video_get_max_page_item(void);
void app_net_video_subtitle_pause(void);
void app_net_video_subtitle_resume(void);
#endif

