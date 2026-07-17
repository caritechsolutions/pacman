#ifndef __APP_EUTEL_CONTROL_H__
#define __APP_EUTEL_CONTROL_H__

#include "app_config.h"
#include "app_default_params.h"
#if RICHEPG_SUPPORT
#include "gxtype.h"
#include "common/list.h"
#include "common/gxmalloc.h"
#include "module/pm/gxpm_manage.h"
#include "module/config/gxconfig.h"
#include "gxos/gxcore_os.h"
#include "gxos/gxcore_os_filesys.h"
#include "richepg_block_mem.h"

extern int s_eutel_printf_en;
#define EUTEL_ERR(fmt, args...)  do {                               \
	printf("[EUTEL_ERR][%s:%d]: ", __func__, __LINE__);             \
	printf(fmt, ##args);                                            \
} while(0)

#define EUTEL_INFO(fmt, args...)  do {                              \
	printf("[EUTEL_INFO][%s:%d]: ", __func__, __LINE__);            \
	printf(fmt, ##args);                                            \
} while(0)

#define EUTEL_SINFO  printf

#define EUTEL_DBG(fmt, args...)  do {                               \
	if (s_eutel_printf_en) {                                        \
		printf("[EUTEL_DBG][%s:%d]: ", __func__, __LINE__);         \
		printf(fmt, ##args);                                        \
	}                                                               \
} while(0)

#define EUTEL_CH_KEY                "chkey%d"  // %d为channel id
#define EUTEL_CH_KEY_DFTV           "richepg_chtv"
#define EUTEL_CH_KEY_DFRA           EUTEL_CH_KEY_DFTV  // "richepg_chra"
#define EUTEL_EPG_EVENT_KEY         "eutel_epg_event_key"
#define EUTEL_EPG_EVENT_KEY_DF      "richepg_epg"
#define EUTEL_LINEUP_LOGO_KEY       "richepg_lineup_logo"

#define EUTEL_BOOKMARK_PROG_POS     31
#define EUTEL_MEM_BLOCK_SIZE        10240
#define EUTEL_INVALID_LCN           0xffff

typedef enum {
	EUTEL_CHANNEL_GENRE = 0,
	EUTEL_CHANNEL_TYPE,
	EUTEL_CONTENT_GENRE,
} EutelArrayType;

typedef struct {
	int lcn;
	int pos;
} EutelChannelLcnArg;

typedef struct {
	int id;
	char *name;
} EutelChannelGenreArg;

typedef struct {
	int id;
	char *name;
} EutelChannelTypeArg;

typedef struct {
	int id;
	char *name;
} EutelContentGenreArg;

typedef struct {
	int channel_id;
	char *channel_logo;
	char *channel_name;
	char *ear_logo;

	int lcn_num;
	int *lcn_ids;
	int genre_num;
	int *genre_ids;
	int type_num;
	int *type_ids;

	uint16_t prog_id;
	uint32_t stream_flag:1; // 0, tv; 1, radio
	uint32_t scramble_flag:1;
	uint32_t logo_flag:1; // 0, default; 1, EUTEL_CH_KEY
	uint32_t mark_flag:1;
	uint32_t mark_flag_bak:1;
	uint32_t lock_flag:1;
	uint32_t lock_flag_bak:1;
	uint32_t zero:9;
} EutelChannelArg;

typedef struct {
	BlockMemMgr struct_mgr;
	BlockMemMgr string_mgr;
	BlockMemMgr ear_mgr;

	int genre_total;
	EutelChannelGenreArg *genre_array;
	int type_total;
	EutelChannelTypeArg *type_array;
	int content_total;
	EutelContentGenreArg *content_array;

	int arg_total;
	EutelChannelArg *arg_array;
	int sort_total;
	EutelChannelLcnArg *sort_array;
	int use_total;
	EutelChannelLcnArg *use_array;

#define CHANNEL_FILTER_START_SEL  2
	int filter_sel; // 0: all; 1: Mark List; filter_sel-2: genre_sel
	int filter_sel_bak;
	int cur_stream_type; // 0, tv; 1, radio
	int cur_tv_lcn;
	int cur_ra_lcn;
	int prev_lcn;
	int ops_pos;
	int ops_lcn;

	uint32_t filter_sec;
	event_list *filter_timer;
} EutelChannelCtrl;

extern EutelChannelCtrl s_eutel_ch_ctrl;

int app_richepg_init_all(void);
int eutel_windows_link_init(void);
int eutel_channel_ctrl_init(void);
int eutel_channel_ctrl_release(void);
char *eutel_genre_name_get(int id, EutelArrayType type);
char *eutel_array_name_get(int num, int *id, EutelArrayType type);
int eutel_channel_ctrl_build(void);
int eutel_channel_logo_key_refresh(void);
int eutel_channel_ear_logo_update(void);
char *eutel_cur_ear_logo_path_get(void);
char *eutel_cur_channel_name_get(void);
int eutel_cur_channel_id_get(void);
int eutel_cur_channel_prog_id_get(void);
void eutel_lineup_logo_key_add(void);
char *eutel_lineup_logo_key_get(void);
char *eutel_channel_logo_key_get(int use_pos);
char *eutel_channel_logo_key_get_by_channel_id(int channel_id);

int eutel_channel_group_all_build(void);
int eutel_channel_group_mark_build(void);
int eutel_channel_group_filter_build(int sel);
int eutel_channel_group_build(int sel);
char *eutel_channel_group_title(const char *widget);
int eutel_channel_group_find_build(char *find_str);
int eutel_channel_mark_change(int sel, bool save_flag);
int eutel_channel_mark_save_all(void);
int eutel_channel_lock_change(int sel, bool save_flag);
int eutel_channel_lock_save_all(void);
int eutel_channel_update_current_row(const char *widget);
int app_eutel_chlist_filter_pop(const char *widget);
int app_eutel_evlist_filter_pop(const char *widget, int orig_sel);

int eutel_channel_play_use_sel_get(void);
char *eutel_channel_name_get_by_prog_id(int prog_id);
int eutel_channel_use_sel_get_by_prog_id(int prog_id);
int eutel_channel_use_sel_get_by_lcn(int lcn);

int eutel_channel_set_play_type(bool is_radio);
int eutel_channel_get_play_lcn(void);
int eutel_channel_set_play_lcn(int lcn);
int eutel_channel_play_by_lcn(uint32_t mode, int lcn);
int eutel_channel_play_check_num(int num);
int eutel_channel_play_by_num(uint32_t mode, int num);
int eutel_channel_play_prev(uint32_t mode);
int eutel_channel_play_next(uint32_t mode);
int eutel_channel_play_current(uint32_t mode);
int eutel_channel_play_recall(uint32_t mode);
int eutel_channel_play_tvradio(uint32_t mode);
int eutel_channel_play_progid(uint32_t mode, int prog_id);
int eutel_channel_find_lcn_by_prog_id(int prog_id);
int eutel_channel_find_lcn_by_channel_id(int channel_id, int *prog_id);
int eutel_channel_play_first(void);
int eutel_channel_play_set_lcn_first(void);

int eutel_channel_prog_public_get_total(void);
int eutel_channel_prog_public_get_next_pos(int pos);
int eutel_channel_prog_public_get_prev_pos(int pos);
int eutel_channel_prog_public_get_cur_pos(int pos);
int eutel_channel_prog_public_real_pos(int sel);
int eutel_channel_prog_public_real_select(int pos);
int eutel_channel_prog_public_get_id(int sel);
int eutel_channel_prog_public_set_lcn(int pos);
int eutel_channel_prog_public_is_full_mode(int sat_id);
int eutel_channel_prog_public_get_lcn(int sel);
int eutel_channel_ctrl_is_already_build(void);
int eutel_channel_ctrl_data_easy_save(void);

#endif
#endif

