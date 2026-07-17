#ifndef __APP_IPTV_H__
#define __APP_IPTV_H__

#include "app_config.h"
#if NETAPPS_SUPPORT
#include "app_wnd_system_setting_opt.h"
#include "app_netvideo_epg.h"
#include <gx_apps.h>
#include <app_iptv_list_api.h>
#include "gxtype.h"
#endif

#if MINI_MEM_SUPPORT
#define LIST_MAX_ITEM_NUM 8000
#else
#define LIST_MAX_ITEM_NUM 30000
#endif
#if NETAPPS_SUPPORT
#define MAX_IPTV_ITEM_NUM       4
#define MAX_IPTV_GROUP_LENGTH   128
#define MAX_IPTV_URL_LENGTH     1024
#define MAX_IPTV_NAME_LENGTH    512
#define TEXT_VIDEO_FOCUS_COLOR "[net_video_focus_color,net_video_focus_color,net_video_focus_color]"
#define TEXT_VIDEO_UNFOCUS_COLOR "[gui_trans,gui_trans,gui_trans]"

enum IPTV_LOGIN_STATUS
{
	IPTV_LOGIN_NONE = 0,
	IPTV_LOGIN_PLEASE,
	IPTV_LOGIN_RUNNING,
	IPTV_LOGIN_SUCCESS,
	IPTV_LOGIN_FAILED
};
typedef enum {
	IPTV_PLAY_POINT = 0,
	IPTV_PLAY_PREV,
	IPTV_PLAY_NEXT,
	IPTV_PLAY_LIST,
	IPTV_PLAY_LAST,
	IPTV_PLAY_PAGE
} IPTVPlayChoice;

typedef struct
{
	char *item_name;
	char *item_pic;
	void (*item_func)(void);
}IptvItem;

typedef struct _IPTVSetCtrl
{
	int server_type;
	const char *name;
	int param_num;
	int param_len[MAX_IPTV_ITEM_NUM];
	char *param1_name;
	char *param2_name;
	char *param3_name;
	char *param4_name;
	void (*setting_get)(char *param1, char *param2, char *param3, char *param4);
	void (*setting_set)(char *param1, char *param2, char *param3, char *param4);
	void (*save_func)(void);
	int (*get_status)(void);
	char* (*get_message)(void);
	void (*funckey)(MenuFuncKey *key);
}IPTVSetCtrl;

typedef struct
{
	char* group;
	char* url;
	char* name;
}IPTVChannel;

typedef struct _IPTVOps
{
	int server_type;
	char *server_title;
	int source_num;
	int auto_play;
	int remote_fastget;
	int fullscreen_flag;
	int iptv_lite_flag;

	void (*set_save_channel)(IPTVChannel*);
	IPTVChannel* (*get_save_channel)(void);
	gx_list_t* (*get_fav_list)(void);
	void (*save_fav_list)(gx_list_t*);
	char* (*get_url)(char *);
	int  (*check_lock)(char *);
	NetappsEpgCtrl epg_ctrl;
}IPTVOps;

void app_iptv_tip_show(char *buf);
void app_iptv_show_status(int server_type);

void app_iptv_factory_reset(void);
status_t app_create_iptv_menu(IPTVOps *ops);
void app_iptv_free_daylist(EpgDayList *daylist);
void app_iptv_free_epglist(EpgEventList *epglist);
char *app_iptv_get_time_str(char *timestamp);
void app_iptv_create_setting_menu(IPTVSetCtrl *set);
void app_create_iptv_browser_menu(char *title_txt, char *logo, int item_num, IptvItem *item_data);
void app_create_iptv_account_infor_menu(gx_list_t *list);

void app_create_iptv_epg_menu(char *prog_name, char *prog_url, NetappsEpgCtrl *ops);

#ifdef IPTV_FAV_SUPPORT
void app_iptv_create_fav_list(int* focus_item);
int app_iptv_fav_play_next(void);
int app_iptv_fav_play_prev(void);
int app_fav_item_to_all_index(int item);
int app_iptv_fav_list_dlg_check(void);
int app_iptv_fav_list_get_cur_play(void);
void app_iptv_fav_list_dlg_clean(void);
#endif


#endif
#endif
