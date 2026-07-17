/*************************************************************************
	> File Name: app/include/app_fm_play.h
	> Author:
	> Mail:
	> Created Time: 2021年11月26日 星期五 11时04分34秒
 ************************************************************************/
#ifndef APP_FM_PLAY_H
#define APP_FM_PLAY_H

#include "dvbhal/gxfe_hal.h"
#include "cygnus_padmux.h"

#define FM_FREQ_NAME_LEN 32
#define FM_FREQ_DATA "/home/gx/fm_freq_data"
#define FM_DEFAULT_FREQ_NAME_LIST_PATH "/dvb/theme/fm_name_list.xml"
#define FM_USER_FREQ_NAME_LIST_PATH "/home/gx/fm_name_list.xml"

typedef enum{
    STORA_MODE =0,
    FREQ_MODE
}Fm_mode;

typedef enum{
    STATE_PAUSE =0,
    STATE_PLAY,
    STATE_STOP
}Fm_play_state;

typedef struct
{
    uint32_t freq;
    uint32_t del_flag;
    char name[FM_FREQ_NAME_LEN];
}FmListData;

typedef struct _IptvItemClass
{
	char *fm_freq;
	char *fm_name;
}FmItemClass;

typedef struct _FmListClass
{
	int fm_item_num;
	FmItemClass *fm_item_list;
}FmListClass;
FmListClass *_fm_list_new(void);
void fm_list_free(FmListClass *list);
FmListClass *_get_list_from_xml_file(const char *path);

int app_fm_get_handle(void);
void app_fm_data_init(void);
void fm_freq_name_list_destroy(void);
char* app_fm_get_freq_name(uint32_t freq);
void app_fm_update_freq_name(FmListData* data);
void app_fm_play_mode_set(Fm_mode mode);
Fm_mode app_fm_play_mode_get(void);
extern int app_fm_freq_num_get(void);
extern int app_fm_freq_array_get(uint32_t* freq_array);
extern int app_fm_freq_del(FmListData* freq_list_data);
extern bool app_fm_freq_check_exist(uint32_t freq);
extern int app_fm_freq_add_one(uint32_t freq);
extern bool app_fm_freq_check_exist(uint32_t freq);
extern int app_fm_freq_list_get(FmListData* freq_list_data);
extern void app_fm_freq_del_file(void);
extern int app_fm_play_get_index(void);
extern int app_fm_play_set_index(int index);
extern void app_create_fm_freq_list(void);
extern void app_fm_get_band_range(uint32_t* min,uint32_t* max);
extern int app_fm_freq_save(uint32_t* freq_array, int freq_num);
extern void app_fm_play_prog(uint32_t freq);
extern void app_fm_play_exec(void);
#endif
