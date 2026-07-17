#ifndef __APP_COVER_PVR_H__
#define __APP_COVER_PVR_H__
#include <time.h>
#include <gxtype.h>
#include "cJSON.h"
#include "module/app_log.h"

#ifdef MEMORY_DEBUG
#define _cover_pvr_malloc     GxCore_Malloc
#define _cover_pvr_calloc     GxCore_Calloc
#define _cover_pvr_realloc    GxCore_Realloc
#define _cover_pvr_strdup     GxCore_Strdup
#define _cover_pvr_free       GxCore_Free
#else
#define _cover_pvr_malloc     malloc
#define _cover_pvr_calloc     calloc
#define _cover_pvr_realloc    realloc
#define _cover_pvr_strdup     strdup
#define _cover_pvr_free       free
#endif

#define COVER_PVR_FREE(x)     do { if(x) _cover_pvr_free(x); x=NULL; } while(0)

#define GET_MNUMBER(number, item) do {                  \
    if (item && item->type == cJSON_Number) {           \
        number = item->valueint;                        \
    } else {                                            \
        goto err;                                       \
    }                                                   \
} while(0)

#define GET_ONUMBER(number, item) do {                  \
    if (item && item->type == cJSON_Number) {           \
        number = item->valueint;                        \
    }                                                   \
} while(0)

#define GET_MSTRING(string, item) do {                  \
    if (item && item->type == cJSON_String) {           \
        if (item->valuestring) {                        \
            string = _cover_pvr_strdup(item->valuestring);\
        } else {                                        \
            string = NULL;                              \
        }                                               \
    }                                                   \
    else {                                              \
        goto err;                                       \
    }                                                   \
} while(0)

#define GET_OSTRING(string, item) do {                  \
    if (item && item->type == cJSON_String) {           \
        if (item->valuestring) {                        \
            string = _cover_pvr_strdup(item->valuestring);\
        } else {                                        \
            string = NULL;                              \
        }                                               \
    }                                                   \
} while(0)

#define GET_BOOL(val, parent, key) do {                 \
    cJSON *sitem = cJSON_GetObjectItem(parent, key);    \
    val = (sitem && sitem->type == cJSON_True);         \
} while(0)

#define _free_json_obj(obj) do { if(obj) cJSON_Delete(obj); obj=NULL; } while(0);
#define _free_json_str(ptr) do { if(ptr) free(ptr); ptr=NULL; } while(0);

#define COVER_PVR_DBG_PRINT   0

#define COVER_PVR_ERR(fmt, args...)  do {                     \
    app_log_min("[eERR][%s:%d]: ", __func__, __LINE__);          \
    app_log_min(fmt, ##args);                                    \
} while(0)

#define COVER_PVR_INFO(fmt, args...)  do {                    \
    app_log_min("[eINFO][%s:%d]: ", __func__, __LINE__);         \
    app_log_min(fmt, ##args);                                    \
} while(0)

#if COVER_PVR_DBG_PRINT
#define COVER_PVR_DBG(fmt, args...)  do {                     \
    app_log_min("[eDBG][%s:%d]: ", __func__, __LINE__);          \
    app_log_min(fmt, ##args);                                    \
} while(0)
#else
#define COVER_PVR_DBG(fmt, args...)  do { } while (0)
#endif

#define PVR_JSON_NAME           "info.json"
#define PVR_LOGO_NAME           "logo.jpg"
#define PVR_CHLOGO_NAME         "ch.jpg"
#define PVR_INDEX_NAME          "index.json"
#define COVER_PVR_EVENT_KEY_DF  "cover_pvr"
#define COVER_PVR_EVENT_KEY     "cover_pvr_event_key"

#define WND_COVER_PVR            "wnd_cover_pvr"
#define WND_COVER_PVR_INFO       "wnd_cover_pvr_info"

typedef enum {
    EPG_RECORD_IDLE = 0,
    EPG_RECORD_EXIST,
    EPG_RECORD_ADD,
    EPG_RECORD_DEL
} CoverPvrEpgRecordFlag;

#if PVR_PREVIEW_SUPPORT
typedef enum {
    PREVIEW_FROM_PATH = 0,
    PREVIEW_FROM_TIME,
}PrevFlag;

typedef struct {
    int flag;
    time_t offset;
    char *path;
}PreviewInfo;
#endif

typedef struct
{
	time_t start_time;
	time_t finish_time;
	int parent_rate;
	char *event_title;
	char *event_desc;
	char *event_logo;
	char *genres_str;
} play_record_info;

struct cover_pvr_epg_record
{
    char *fname;
    char *chname;
    time_t starttime;
    time_t finishtime;
    time_t recstart;
    time_t duration;
    unsigned int contents_num;
    char **contents;
    int parental;
    char *name;
    char *description;
    int from_freescan;
#if PVR_PREVIEW_SUPPORT
    PreviewInfo preview;
#endif
    uint32_t flag; // flag needn't save to json
};

typedef enum {
    PVR_RECORD_SAVE_NONE = 0,
    PVR_RECORD_SAVE_CH,
    PVR_RECORD_SAVE_ALL
} PvrRecordSaveFlag;

typedef struct {
    PvrRecordSaveFlag save_flag;
    time_t start_sec;
    int channel_id;
    char *dir_path;
    char *info_path;
    char *logo_path;
    char *chlogo_path;
    char *index_path;
    char *ori_logo_path;
    char *ori_chlogo_path;
    struct cover_pvr_epg_record record;
} PvrRecordInfo;

struct cover_pvr_epg_record_func
{
    bool (*stop_check)(void);
    void* (*fname_get)(const char *info_path);
    void (*fname_free)(void *fname);
};

int cover_pvr_copy_file_to_file(const char *src, const char *dst, int unit_size);
int cover_pvr_copy_data_to_file(uint8_t *data, int size, const char *dst);
int app_cover_pvr_free_epg_record(struct cover_pvr_epg_record *record);
int app_cover_pvr_free_epg_record_all(struct cover_pvr_epg_record **record, int *num);
int app_cover_pvr_parse_epg_record_info(const char *path, struct cover_pvr_epg_record *record);
int app_cover_pvr_parse_epg_record_all_info(const char *index_path, struct cover_pvr_epg_record **record, int *num);
int app_cover_pvr_save_epg_record_info(const char *info_path, const char *index_path, struct cover_pvr_epg_record *record);
int app_cover_pvr_save_epg_record_all_info(const char *index_path,
        struct cover_pvr_epg_record *record, int num, char **extra_path, int extra_num,
        struct cover_pvr_epg_record_func *record_func);
status_t app_cover_pvr_hotplug(char *hotplug_dev);
int app_cover_pvr_create_dialog(void);
void app_cover_pvr_init(void);
char *app_cover_pvr_get_full_name(const char *path, int plen, const char *fname, int flen);
char *app_cover_pvr_get_full_name2(const char *path, const char *fname);
int app_cover_pvr_manage_init(bool on_fullscreen, bool reverse_flag, const char *menu_logo);
int app_cover_pvr_ctrl_build(void);
int app_cover_pvr_event_info_get(int num, const char *fname, bool get_all, play_record_info *rec_info);
int app_cover_pvr_event_info_free(play_record_info *rec_info);
#if PVR_PREVIEW_SUPPORT
int app_cover_pvr_preview_info_record(struct cover_pvr_epg_record *record, PreviewInfo *info);
#endif

#endif
