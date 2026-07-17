#include "app.h"
#if RICHEPG_SUPPORT
#include "app_windows.h"
#include "app_str_id.h"
#include "richepg_json.h"
#include "richepg_json_api.h"
#include "richepg_store.h"
#include "richepg.h"
#include "app_eutel_database.h"
#include "app_eutel_common.h"
#include "app_book.h"

#define MORE_INFO_COLLECT_SUPPORT 0

#define EUTEL_UPDATE_LOG_PATH   "/home/gx/richepg_update_log.json"

#define TEXT_COLLECT_TITLE      "text_collect_title"
#define TEXT_COLLECT_PAGE       "text_collect_page"
#define TEXT_STORAGE            "text_collect_storage"
#define TEXT_USED_TITLE         "text_collect_used_title"
#define TEXT_USED_VALUE         "text_collect_used_value"
#define TEXT_FREE_TITLE         "text_collect_free_title"
#define TEXT_FREE_VALUE         "text_collect_free_value"
#define LISTVIEW_COLLECT        "listview_collect"
#define TEXT_COLLECT_TIME       "text_collect_time"
#define MAX_COLLECT_TIME_CNT    10
#define REALLOC_INCREASE_NUM    10
#define ITEMS_PER_PAGE          7

extern char *richepg_get_usbentry(char *path, int size);

typedef enum {
    EUTEL_COLLECT_GENRE_ADD = 0,
    EUTEL_COLLECT_CH_MOD,
    EUTEL_COLLECT_CH_ADD,
    EUTEL_COLLECT_REC_ADD,
    EUTEL_COLLECT_ECL_MAINT,
    EUTEL_COLLECT_EPG_MAINT,
    EUTEL_COLLECT_START_MAINT,
    EUTEL_COLLECT_FINISH_MAINT,
    EUTEL_COLLECT_FAIL_MAINT
} EutelCollectType;

typedef struct {
    int count;
    char *genres_name;
} EutelCollectGenreAdd;

typedef struct {
    int lcn;
    char *prog_name;
    char *genres_name;
} EutelCollectChAdd;

typedef struct {
    int ch_id;
    time_t rec_duration;
    char *rec_name;
    char *genres_name;
} EutelCollectRecAdd;

typedef struct {
    time_t maint_sec;
    char *sat_name;
} EutelCollectMaintAdd;

typedef struct {
    EutelCollectType type;
    union {
        EutelCollectGenreAdd genre_add;
        EutelCollectChAdd ch_add;
        EutelCollectRecAdd rec_add;
        EutelCollectMaintAdd maint_add;
    } value;
} EutelCollectItem;

typedef struct {
    int item_total;
    int item_num;
    EutelCollectItem *item_array;
    event_list *timer;
    int count;
    int s_index;
    int e_index;
} EutelCollectCtrl;

static EutelCollectCtrl s_eutel_collect_ctrl;

void app_eutel_collect_release(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    int i = 0;

    if (ctrl->item_array) {
        for (i = 0; i < ctrl->item_total; i++) {
            switch (ctrl->item_array[i].type)
            {
                case EUTEL_COLLECT_GENRE_ADD:
                    APP_FREE(ctrl->item_array[i].value.genre_add.genres_name);
                    break;
                case EUTEL_COLLECT_CH_MOD:
                case EUTEL_COLLECT_CH_ADD:
                    APP_FREE(ctrl->item_array[i].value.ch_add.prog_name);
                    APP_FREE(ctrl->item_array[i].value.ch_add.genres_name);
                    break;
                case EUTEL_COLLECT_REC_ADD:
                    APP_FREE(ctrl->item_array[i].value.rec_add.rec_name);
                    APP_FREE(ctrl->item_array[i].value.rec_add.genres_name);
                    break;
                case EUTEL_COLLECT_START_MAINT:
                case EUTEL_COLLECT_ECL_MAINT:
                case EUTEL_COLLECT_EPG_MAINT:
                case EUTEL_COLLECT_FINISH_MAINT:
                case EUTEL_COLLECT_FAIL_MAINT:
                    APP_FREE(ctrl->item_array[i].value.maint_add.sat_name);
                    break;
                default:
                    break;
            }
        }
        APP_FREE(ctrl->item_array);
        ctrl->item_num = 0;
        ctrl->item_total = 0;
        ctrl->s_index = -1;
        ctrl->e_index = -1;
    }
}

static int app_eutel_collect_increase(EutelCollectItem *item)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    int sel = ctrl->item_num;

    if (ctrl->item_num == ctrl->item_total) {
        int total = ctrl->item_total + REALLOC_INCREASE_NUM;
        EutelCollectItem *array = GxCore_Calloc(total, sizeof(EutelCollectItem));
        if (!array) {
            EUTEL_ERR("malloc failed!\n");
            return -1;
        }
        if (ctrl->item_total) {
            memcpy(array, ctrl->item_array, ctrl->item_total * sizeof(EutelCollectItem));
            APP_FREE(ctrl->item_array);
        }
        ctrl->item_array = array;
        ctrl->item_total = total;
    }

    ctrl->item_array[sel].type = item->type;
    switch (item->type)
    {
        case EUTEL_COLLECT_GENRE_ADD:
            if (item->value.genre_add.genres_name) {
                ctrl->item_array[sel].value.genre_add.genres_name = GxCore_Strdup(item->value.genre_add.genres_name);
                if (!ctrl->item_array[sel].value.genre_add.genres_name) {
                    EUTEL_ERR("malloc failed!\n");
                    return -1;
                }
            }
            ctrl->item_array[sel].value.genre_add.count = item->value.genre_add.count;
            break;
        case EUTEL_COLLECT_CH_MOD:
        case EUTEL_COLLECT_CH_ADD:
            if (item->value.ch_add.prog_name) {
                ctrl->item_array[sel].value.ch_add.prog_name = GxCore_Strdup(item->value.ch_add.prog_name);
                if (!ctrl->item_array[sel].value.ch_add.prog_name) {
                    EUTEL_ERR("malloc failed!\n");
                    return -1;
                }
            }
            if (item->value.ch_add.genres_name) {
                ctrl->item_array[sel].value.ch_add.genres_name = GxCore_Strdup(item->value.ch_add.genres_name);
                if (!ctrl->item_array[sel].value.ch_add.genres_name) {
                    APP_FREE(ctrl->item_array[sel].value.ch_add.prog_name);
                    EUTEL_ERR("malloc failed!\n");
                    return -1;
                }
            }
            ctrl->item_array[sel].value.ch_add.lcn = item->value.ch_add.lcn;
            break;
        case EUTEL_COLLECT_REC_ADD:
            if (item->value.rec_add.rec_name) {
                ctrl->item_array[sel].value.rec_add.rec_name = GxCore_Strdup(item->value.rec_add.rec_name);
                if (!ctrl->item_array[sel].value.rec_add.rec_name) {
                    EUTEL_ERR("malloc failed!\n");
                    return -1;
                }
            }
            if (item->value.rec_add.genres_name) {
                ctrl->item_array[sel].value.rec_add.genres_name = GxCore_Strdup(item->value.rec_add.genres_name);
                if (!ctrl->item_array[sel].value.rec_add.genres_name) {
                    APP_FREE(ctrl->item_array[sel].value.rec_add.rec_name);
                    EUTEL_ERR("malloc failed!\n");
                    return -1;
                }
            }
            ctrl->item_array[sel].value.rec_add.ch_id = item->value.rec_add.ch_id;
            ctrl->item_array[sel].value.rec_add.rec_duration = item->value.rec_add.rec_duration;
            break;
        case EUTEL_COLLECT_START_MAINT:
        case EUTEL_COLLECT_ECL_MAINT:
        case EUTEL_COLLECT_EPG_MAINT:
        case EUTEL_COLLECT_FINISH_MAINT:
        case EUTEL_COLLECT_FAIL_MAINT:
            if (item->value.maint_add.sat_name) {
                ctrl->item_array[sel].value.maint_add.sat_name = GxCore_Strdup(item->value.maint_add.sat_name);
                if (!ctrl->item_array[sel].value.maint_add.sat_name) {
                    EUTEL_ERR("malloc failed!\n");
                    return -1;
                }
            }
            ctrl->item_array[sel].value.maint_add.maint_sec = item->value.maint_add.maint_sec;
            break;
        default:
            return -1;
    }
    ++ctrl->item_num;

    return 0;
}

int app_eutel_collect_add_item_genre_add(int count, char *genres_name)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_GENRE_ADD;
    item.value.genre_add.count = count;
    item.value.genre_add.genres_name = genres_name;

    return app_eutel_collect_increase(&item);
}

int app_eutel_collect_add_item_ch_mod(int lcn, char *prog_name, char *genres_name)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_CH_MOD;
    item.value.ch_add.lcn = lcn;
    item.value.ch_add.prog_name = prog_name;
    item.value.ch_add.genres_name = genres_name;

    return app_eutel_collect_increase(&item);
}

int app_eutel_collect_add_item_ch_add(int lcn, char *prog_name, char *genres_name)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_CH_ADD;
    item.value.ch_add.lcn = lcn;
    item.value.ch_add.prog_name = prog_name;
    item.value.ch_add.genres_name = genres_name;

    return app_eutel_collect_increase(&item);
}

int app_eutel_collect_add_item_rec_add(int ch_id, time_t rec_duration, char *rec_name, char *genres_name)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_REC_ADD;
    item.value.rec_add.ch_id = ch_id;
    item.value.rec_add.rec_duration = rec_duration;
    item.value.rec_add.rec_name = rec_name;
    item.value.rec_add.genres_name = genres_name;

    return app_eutel_collect_increase(&item);
}

int app_eutel_collect_add_item_ecl_maint(char *sat_name, time_t seconds)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_ECL_MAINT;
    item.value.maint_add.sat_name = sat_name;
    item.value.maint_add.maint_sec = seconds;

    return app_eutel_collect_increase(&item);
}

int app_eutel_collect_add_item_epg_maint(char *sat_name, time_t seconds)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_EPG_MAINT;
    item.value.maint_add.sat_name = sat_name;
    item.value.maint_add.maint_sec = seconds;

    return app_eutel_collect_increase(&item);
}

void app_eutel_collect_start_index_record(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;

    ctrl->s_index = ctrl->item_num? ctrl->item_num - 1: 0;
}

void app_eutel_collect_finish_index_record(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;

    ctrl->e_index = ctrl->item_num? ctrl->item_num - 1: 0;
}

typedef enum {
    KEY_COLLECT_TYPE = 0,
    KEY_COLLECT_ITEM,
    KEY_COLLECT_GENRE_COUNT,
    KEY_COLLECT_GENRE_NAME,
    KEY_COLLECT_CH_LCN,
    KEY_COLLECT_CH_NAME,
    KEY_COLLECT_CH_ID,
    KEY_COLLECT_REC_DUR,
    KEY_COLLECT_REC_NAME,
    KEY_COLLECT_MAINT_SEC,
    KEY_COLLECT_SAT_NAME,
}CollectKeyType;

static char *_collect_get_key(CollectKeyType key)
{
    switch(key)
    {
        case KEY_COLLECT_TYPE:          return "0";
        case KEY_COLLECT_ITEM:          return "1";
        case KEY_COLLECT_GENRE_COUNT:   return "2";
        case KEY_COLLECT_GENRE_NAME:    return "3";
        case KEY_COLLECT_CH_LCN:        return "4";
        case KEY_COLLECT_CH_NAME:       return "5";
        case KEY_COLLECT_CH_ID:         return "6";
        case KEY_COLLECT_REC_DUR:       return "7";
        case KEY_COLLECT_REC_NAME:      return "8";
        case KEY_COLLECT_MAINT_SEC:     return "9";
        case KEY_COLLECT_SAT_NAME:      return "a";
        default:                        return NULL;
    }
    return NULL;
}

static cJSON *_collect_genre_object_get(EutelCollectGenreAdd *genre)
{
    cJSON *object = NULL;

    if(!genre)
        return NULL;

    if(NULL == (object = cJSON_CreateObject()))
        return NULL;

    cJSON_AddNumberToObject(object, _collect_get_key(KEY_COLLECT_GENRE_COUNT), genre->count);
    if(genre->genres_name)
        cJSON_AddStringToObject(object, _collect_get_key(KEY_COLLECT_GENRE_NAME), genre->genres_name);
    else
        cJSON_AddStringToObject(object, _collect_get_key(KEY_COLLECT_GENRE_NAME), STR_ID_CH_INSTALLED);

    return object;
}

static cJSON *_collect_ch_object_get(EutelCollectChAdd *ch)
{
    cJSON *object = NULL;

    if(!ch)
        return NULL;

    if(NULL == (object = cJSON_CreateObject()))
        return NULL;

    cJSON_AddNumberToObject(object, _collect_get_key(KEY_COLLECT_CH_LCN), ch->lcn);
    if(ch->genres_name)
        cJSON_AddStringToObject(object, _collect_get_key(KEY_COLLECT_GENRE_NAME), ch->genres_name);
    if(ch->prog_name)
        cJSON_AddStringToObject(object, _collect_get_key(KEY_COLLECT_CH_NAME), ch->prog_name);

    return object;
}

static cJSON *_collect_rec_object_get(EutelCollectRecAdd *rec)
{
    cJSON *object = NULL;

    if(!rec)
        return NULL;

    if(NULL == (object = cJSON_CreateObject()))
        return NULL;

    cJSON_AddNumberToObject(object, _collect_get_key(KEY_COLLECT_CH_ID), rec->ch_id);
    cJSON_AddNumberToObject(object, _collect_get_key(KEY_COLLECT_REC_DUR), rec->rec_duration);
    if(rec->rec_name)
        cJSON_AddStringToObject(object, _collect_get_key(KEY_COLLECT_REC_NAME), rec->rec_name);
    if(rec->genres_name)
        cJSON_AddStringToObject(object, _collect_get_key(KEY_COLLECT_GENRE_NAME), rec->genres_name);

    return object;
}

static cJSON *_collect_maint_object_get(EutelCollectMaintAdd *maint)
{
    cJSON *object = NULL;

    if(!maint)
        return NULL;

    if(NULL == (object = cJSON_CreateObject()))
        return NULL;

    cJSON_AddNumberToObject(object, _collect_get_key(KEY_COLLECT_MAINT_SEC), maint->maint_sec);
    if(maint->sat_name)
        cJSON_AddStringToObject(object, _collect_get_key(KEY_COLLECT_SAT_NAME), maint->sat_name);

    return object;
}

int app_eutel_collect_save(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    EutelCollectItem *item = NULL;
    int i = 0;
    cJSON *array = NULL;
    cJSON *object = NULL;
    cJSON *iobject = NULL;
    char *json_str = NULL;

    if(-1 == ctrl->s_index || -1 == ctrl->e_index || ctrl->s_index > ctrl->e_index || ctrl->e_index >= ctrl->item_total)
        return -1;

    if(NULL == (array = cJSON_CreateArray()))
        goto err;

    for(i = ctrl->s_index; i < ctrl->e_index + 1; i++)
    {
        item = &ctrl->item_array[i];

        if(NULL == (object = cJSON_CreateObject()))
            goto err;

        cJSON_AddNumberToObject(object, _collect_get_key(KEY_COLLECT_TYPE), item->type);

        if(EUTEL_COLLECT_GENRE_ADD == item->type)
            iobject = _collect_genre_object_get(&item->value.genre_add);
        else if(EUTEL_COLLECT_CH_MOD == item->type || EUTEL_COLLECT_CH_ADD == item->type)
            iobject = _collect_ch_object_get(&item->value.ch_add);
        else if(EUTEL_COLLECT_REC_ADD == item->type)
            iobject = _collect_rec_object_get(&item->value.rec_add);
        else if(EUTEL_COLLECT_START_MAINT == item->type
                || EUTEL_COLLECT_ECL_MAINT == item->type
                || EUTEL_COLLECT_EPG_MAINT == item->type
                || EUTEL_COLLECT_FINISH_MAINT == item->type
                || EUTEL_COLLECT_FAIL_MAINT == item->type)
            iobject = _collect_maint_object_get(&item->value.maint_add);

        if(NULL == iobject)
        {
            EUTEL_ERR("\033[31mget %d object failed\033[0m\n", item->type);
            goto err;
        }
        cJSON_AddItemToObject(object, _collect_get_key(KEY_COLLECT_ITEM), iobject);
        cJSON_AddItemToArray(array, object);
        object = NULL;
        iobject = NULL;
    }

    // cJSON_Print 格式化打印，cJSON_PrintUnformatted 非格式化打印
    if(NULL == (json_str = cJSON_PrintUnformatted(array)))
    {
        EUTEL_ERR("\033[31mcJSON_PrintUnformatted failed\033[0m\n");
        goto err;
    }
    _free_json_obj(array);

    if(richepg_copy_data_to_file((uint8_t*)json_str, strlen(json_str), EUTEL_UPDATE_LOG_PATH) < 0)
    {
        EUTEL_ERR("\033[31msave %s file failed\033[0m\n", EUTEL_UPDATE_LOG_PATH);
        goto err;
    }

    _free_json_str(json_str);
    return 0;
err:
    _free_json_obj(array);
    _free_json_obj(object);
    return -1;
}

static int _collect_genre_item_parse(cJSON *object)
{
    cJSON *item = NULL;
    int genre_count = 0;

    if(!object)
        return -1;

    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_GENRE_COUNT))) != NULL && item->type == cJSON_Number)
        genre_count = item->valueint;
    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_GENRE_NAME))) != NULL && item->type == cJSON_String)
        app_eutel_collect_add_item_genre_add(genre_count, item->valuestring);

    return 0;
}

static int _collect_ch_item_parse(cJSON *object, EutelCollectType type)
{
    int lcn = 0;
    cJSON *item = NULL;
    cJSON *gitem = NULL;
    char *p_name = NULL, *g_name = NULL;

    if(!object)
        return -1;

    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_CH_LCN))) != NULL && item->type == cJSON_Number)
        lcn = item->valueint;
    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_CH_NAME))) != NULL && item->type == cJSON_String)
        p_name = item->valuestring;
    if((gitem = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_GENRE_NAME))) != NULL && gitem->type == cJSON_String)
        g_name = gitem->valuestring;

    if(NULL == g_name || NULL == p_name)
        return -1;

    if(EUTEL_COLLECT_CH_MOD == type)
        app_eutel_collect_add_item_ch_mod(lcn, p_name, g_name);
    else if(EUTEL_COLLECT_CH_ADD == type)
        app_eutel_collect_add_item_ch_add(lcn, p_name, g_name);

    return 0;
}

static int _collect_rec_item_parse(cJSON *object)
{
    int ch_id = 0;
    time_t rec_duration = 0 ;
    char *r_name = NULL, *g_name = NULL;
    cJSON *item = NULL;
    cJSON *gitem = NULL;

    if(!object)
        return -1;

    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_CH_ID))) != NULL && item->type == cJSON_Number)
        ch_id = item->valueint;
    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_REC_DUR))) != NULL && item->type == cJSON_Number)
        rec_duration = item->valueint;
    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_REC_NAME))) != NULL && item->type == cJSON_String)
        r_name = item->valuestring;
    if((gitem = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_REC_NAME))) != NULL && gitem->type == cJSON_String)
        g_name = gitem->valuestring;

    if(r_name && g_name)
        app_eutel_collect_add_item_rec_add(ch_id, rec_duration, r_name, g_name);
    return 0;
}

static int _collect_maint_item_parse(cJSON *object, EutelCollectType type)
{
    time_t maint_sec = 0;
    cJSON *item = NULL;
    char *sat_name = NULL;

    if(!object)
        return -1;

    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_MAINT_SEC))) != NULL && item->type == cJSON_Number)
        maint_sec = (time_t)item->valueint;
    if((item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_SAT_NAME))) != NULL && item->type == cJSON_String)
        sat_name = item->valuestring;

    if(NULL == sat_name)
        return -1;

    if(EUTEL_COLLECT_START_MAINT == type)
        app_eutel_collect_add_item_start_maint(sat_name, maint_sec);
    else if(EUTEL_COLLECT_FINISH_MAINT == type)
        app_eutel_collect_add_item_finish_maint(sat_name, maint_sec);
    else if(EUTEL_COLLECT_FAIL_MAINT == type)
        app_eutel_collect_add_item_fail_maint(sat_name, maint_sec);
    else if(EUTEL_COLLECT_ECL_MAINT == type)
        app_eutel_collect_add_item_ecl_maint(sat_name, maint_sec);
    else if(EUTEL_COLLECT_EPG_MAINT == type)
        app_eutel_collect_add_item_epg_maint(sat_name, maint_sec);

    return 0;
}

int app_eutel_collect_parse(void)
{
    int total = 0, i = 0;
    cJSON *array = NULL;
    cJSON *object = NULL;
    cJSON *iobject = NULL;
    cJSON *item = NULL;

    app_eutel_collect_release();

    if(NULL == (array = richepg_get_json(true, EUTEL_UPDATE_LOG_PATH))|| array->type != cJSON_Array)
        goto err;

    if(0 == (total = cJSON_GetArraySize(array)))
        goto err;

    for(i = 0; i < total; i++)
    {
        object = cJSON_GetArrayItem(array, i);

        if(NULL == (item = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_TYPE))) || item->type != cJSON_Number)
            continue;
        if(NULL == (iobject = cJSON_GetObjectItem(object, _collect_get_key(KEY_COLLECT_ITEM))) || iobject->type != cJSON_Object)
            continue;

        if(EUTEL_COLLECT_GENRE_ADD == item->valueint)
            _collect_genre_item_parse(iobject);
        else if(EUTEL_COLLECT_CH_MOD == item->valueint || EUTEL_COLLECT_CH_ADD == item->valueint)
            _collect_ch_item_parse(iobject, item->valueint);
        else if(EUTEL_COLLECT_REC_ADD == item->valueint)
            _collect_rec_item_parse(iobject);
        else if(EUTEL_COLLECT_START_MAINT == item->valueint
                || EUTEL_COLLECT_ECL_MAINT == item->valueint
                || EUTEL_COLLECT_EPG_MAINT == item->valueint
                || EUTEL_COLLECT_FINISH_MAINT == item->valueint
                || EUTEL_COLLECT_FAIL_MAINT == item->valueint)
            _collect_maint_item_parse(iobject, item->valueint);
    }
    return 0;
err:
    _free_json_obj(array);
    return -1;
}

/*
   reset api 20221027
   0: success  1: fail
*/
int app_eutel_collect_reset(void)
{
    if(GxCore_FileExists(EUTEL_UPDATE_LOG_PATH) == GXCORE_FILE_EXIST)
    {
        GxCore_FileDelete(EUTEL_UPDATE_LOG_PATH);
    }
    return 0 ;
}

#if MORE_INFO_COLLECT_SUPPORT
int app_eutel_collect_add_item_start_maint(char *sat_name, time_t seconds)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_START_MAINT;
    item.value.maint_add.sat_name = sat_name;
    item.value.maint_add.maint_sec = seconds;

    return app_eutel_collect_increase(&item);
}

int app_eutel_collect_add_item_finish_maint(char *sat_name, time_t seconds)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_FINISH_MAINT;
    item.value.maint_add.sat_name = sat_name;
    item.value.maint_add.maint_sec = seconds;

    return app_eutel_collect_increase(&item);
}

int app_eutel_collect_add_item_fail_maint(char *sat_name, time_t seconds)
{
    EutelCollectItem item = {0};

    item.type = EUTEL_COLLECT_FAIL_MAINT;
    item.value.maint_add.sat_name = sat_name;
    item.value.maint_add.maint_sec = seconds;

    return app_eutel_collect_increase(&item);
}

#else
int app_eutel_collect_add_item_start_maint(char *sat_name, time_t seconds)
{
    return 0;
}

int app_eutel_collect_add_item_finish_maint(char *sat_name, time_t seconds)
{
    return 0;
}

int app_eutel_collect_add_item_fail_maint(char *sat_name, time_t seconds)
{
    return 0;
}
#endif

static void _eutel_collect_show_usage(void)
{
    GxDiskInfo disk = {0};
    char buffer[32] = {0};
    char *usb = richepg_get_usbentry(buffer, sizeof(buffer));

    if (usb != NULL && GxCore_DiskInfo(usb, &disk) == GXCORE_SUCCESS) {
        GUI_SetProperty(TEXT_STORAGE,    "state", "show");
        GUI_SetProperty(TEXT_USED_TITLE, "state", "show");
        GUI_SetProperty(TEXT_USED_VALUE, "state", "show");
        GUI_SetProperty(TEXT_FREE_TITLE, "state", "show");
        GUI_SetProperty(TEXT_FREE_VALUE, "state", "show");

        app_display_size_str_get(disk.used_size, buffer, sizeof(buffer));
        GUI_SetProperty(TEXT_USED_VALUE, "string", buffer);
        app_display_size_str_get(disk.total_size - disk.used_size, buffer, sizeof(buffer));
        GUI_SetProperty(TEXT_FREE_VALUE, "string", buffer);
    } else {
        GUI_SetProperty(TEXT_STORAGE,    "state", "hide");
        GUI_SetProperty(TEXT_USED_TITLE, "state", "hide");
        GUI_SetProperty(TEXT_USED_VALUE, "state", "hide");
        GUI_SetProperty(TEXT_FREE_TITLE, "state", "hide");
        GUI_SetProperty(TEXT_FREE_VALUE, "state", "hide");
    }
}

static int _eutel_collect_timer_exec(void *userdata)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    char buffer[8] = {0};

    --ctrl->count;
    if(ctrl->count == 0) {
        APP_TIMER_REMOVE(ctrl->timer);
        GUI_EndDialog(WND_EUTEL_COLLECT);
    } else {
        snprintf(buffer, sizeof(buffer), "%02d:%02d", ctrl->count / 60, ctrl->count % 60);
        GUI_SetProperty(TEXT_COLLECT_TIME, "string", buffer);
    }
    return 0;
}

static void _eutel_collect_timer_add(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    char buffer[8] = {0};

    ctrl->count = MAX_COLLECT_TIME_CNT;
    snprintf(buffer, sizeof(buffer), "%02d:%02d", ctrl->count / 60, ctrl->count % 60);
    GUI_SetProperty(TEXT_COLLECT_TIME, "string", buffer);
    APP_TIMER_ADD(ctrl->timer, _eutel_collect_timer_exec, 1000, TIMER_REPEAT);
}

static void _eutel_collect_timer_rm(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;

    GUI_SetProperty(TEXT_COLLECT_TIME, "string", STR_ID_BLANK);
    APP_TIMER_REMOVE(ctrl->timer);
}

static void _eutel_collect_show_page(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    int sel = 0;
    char buffer[16] = {0};

    if (ctrl->item_num <= ITEMS_PER_PAGE) {
        GUI_SetProperty(TEXT_COLLECT_PAGE, "string", STR_ID_BLANK);
    } else {
        GUI_GetProperty(LISTVIEW_COLLECT, "select", &sel);
        snprintf(buffer, sizeof(buffer), "%d / %d", sel / ITEMS_PER_PAGE + 1, (ctrl->item_num-1) / ITEMS_PER_PAGE + 1);
        GUI_SetProperty(TEXT_COLLECT_PAGE, "string", buffer);
    }
}

SIGNAL_HANDLER int app_eutel_collect_list_change(GuiWidget *widget, void *usrdata)
{
    _eutel_collect_show_page();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_collect_list_get_total(GuiWidget *widget, void *usrdata)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;

    return ctrl->item_num;
}

SIGNAL_HANDLER int app_eutel_collect_list_get_data(GuiWidget *widget, void *usrdata)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    static char buffer[64] = {0};
    time_t sec = 0;
    int csec = 0;
    ListItemPara *item = NULL;

    item = (ListItemPara *)usrdata;
    if(item == NULL || item->sel < 0 || item->sel >= ctrl->item_num)
        return EVENT_TRANSFER_STOP;

    switch (ctrl->item_array[item->sel].type)
    {
        case EUTEL_COLLECT_GENRE_ADD:
            snprintf(buffer, sizeof(buffer), "%d", ctrl->item_array[item->sel].value.genre_add.count);

            item->x_offset = 0;
            item->image = NULL;
            item->string = NULL;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            if (ctrl->item_array[item->sel].value.genre_add.genres_name)
                item->string = ctrl->item_array[item->sel].value.genre_add.genres_name;
            else
                item->string = STR_ID_CH_INSTALLED;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = NULL;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = buffer;
            break;

        case EUTEL_COLLECT_CH_MOD:
        case EUTEL_COLLECT_CH_ADD:
            snprintf(buffer, sizeof(buffer), "%04d", ctrl->item_array[item->sel].value.ch_add.lcn);

            item->x_offset = 0;
            item->image = NULL;
            if (ctrl->item_array[item->sel].value.ch_add.lcn > 0)
                item->string = buffer;
            else
                item->string = NULL;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = ctrl->item_array[item->sel].value.ch_add.prog_name;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = ctrl->item_array[item->sel].value.ch_add.genres_name;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            if (ctrl->item_array[item->sel].type == EUTEL_COLLECT_CH_MOD)
                item->string = STR_ID_NEW_CH_INSTALLED;
            else
                item->string = STR_ID_NEW_CH_ADDED;
            break;

        case EUTEL_COLLECT_REC_ADD:
            csec = (int)ctrl->item_array[item->sel].value.rec_add.rec_duration;
            snprintf(buffer, sizeof(buffer), "REC: %02dh%02dm%02ds", csec/3600, csec%3600 / 60, csec%3600 % 60);

            item->x_offset = 15;
            item->zoom = true;
            item->image = eutel_channel_logo_key_get_by_channel_id(ctrl->item_array[item->sel].value.rec_add.ch_id);
            item->string = NULL;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = ctrl->item_array[item->sel].value.rec_add.rec_name;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = ctrl->item_array[item->sel].value.rec_add.genres_name;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = buffer;
            break;

        case EUTEL_COLLECT_START_MAINT:
        case EUTEL_COLLECT_ECL_MAINT:
        case EUTEL_COLLECT_EPG_MAINT:
        case EUTEL_COLLECT_FINISH_MAINT:
        case EUTEL_COLLECT_FAIL_MAINT:
            sec = get_display_time_by_timezone(ctrl->item_array[item->sel].value.maint_add.maint_sec);
            app_common_data_time_edit_str(sec, LONG_DATE_EDIT | LONG_TIME_EDIT, buffer, sizeof(buffer));

            item->x_offset = 0;
            item->image = NULL;
            item->string = NULL;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = buffer;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            item->string = ctrl->item_array[item->sel].value.maint_add.sat_name;

            item = item->next;
            item->x_offset = 0;
            item->image = NULL;
            switch (ctrl->item_array[item->sel].type)
            {
                case EUTEL_COLLECT_START_MAINT: item->string = STR_ID_START_MAINT; break;
                case EUTEL_COLLECT_ECL_MAINT: item->string = STR_ID_CH_DATA_UPDATED; break;
                case EUTEL_COLLECT_EPG_MAINT: item->string = STR_ID_EPG_DATA_UPDATED; break;
                case EUTEL_COLLECT_FINISH_MAINT: item->string = STR_ID_FINISH_MAINT; break;
                case EUTEL_COLLECT_FAIL_MAINT: item->string = STR_ID_FAIL_MAINT; break;
                default: break;
            }
            break;

        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_collect_list_keypress(GuiWidget *widget, void *usrdata)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    int ret = EVENT_TRANSFER_STOP;
    int32_t sel = 0;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_EXIT:
                case STBK_MENU:
                case VK_BOOK_TRIGGER:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_UP:
                case STBK_LEFT:
                case STBK_PAGE_UP:
                    _eutel_collect_timer_rm();
                    GUI_GetProperty(LISTVIEW_COLLECT, "select", &sel);
                    if (sel == 0) {
                        sel = ctrl->item_num - 1;
                        GUI_SetProperty(LISTVIEW_COLLECT, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    } else {
                        event->key.sym = STBK_PAGE_UP;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_DOWN:
                case STBK_RIGHT:
                case STBK_PAGE_DOWN:
                    _eutel_collect_timer_rm();
                    GUI_GetProperty(LISTVIEW_COLLECT, "select", &sel);
                    if (ctrl->item_num == sel+1) {
                        sel = 0;
                        GUI_SetProperty(LISTVIEW_COLLECT, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    } else {
                        event->key.sym = STBK_PAGE_DOWN;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_eutel_collect_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_EXIT:
                case STBK_MENU:
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_EUTEL_COLLECT);
                    GUI_SetInterface("flush", NULL);
                    break;
                default:
                    break;
            }

        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_collect_create(GuiWidget *widget, void *usrdata)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    int i = 0;

    for (i = 0; i < ctrl->item_num; i++) {
        if (ctrl->item_array[i].type == EUTEL_COLLECT_GENRE_ADD) {
            GUI_SetProperty(TEXT_COLLECT_TITLE, "string", STR_ID_INSTALLATION);
            break;
        }
    }
    if (i == ctrl->item_num) {
        GUI_SetProperty(TEXT_COLLECT_TITLE, "string", STR_ID_UPDATES);
    }

    _eutel_collect_show_page();
    _eutel_collect_show_usage();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_collect_destroy(GuiWidget *widget, void *usrdata)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;

    APP_TIMER_REMOVE(ctrl->timer);
    app_eutel_collect_release();
    return EVENT_TRANSFER_STOP;
}

int app_eutel_collect_create_dialog(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    int i = 0;

    for (i = 0; i < ctrl->item_num; i++) {
        if (ctrl->item_array[i].type != EUTEL_COLLECT_START_MAINT
                && ctrl->item_array[i].type != EUTEL_COLLECT_FINISH_MAINT
                && ctrl->item_array[i].type != EUTEL_COLLECT_FAIL_MAINT) {
            break;
        }
    }
    if (i == ctrl->item_num) {
        app_eutel_collect_release();
        return -1;
    }

    GUI_CreateDialog(WND_EUTEL_COLLECT);
    app_unset_window_back_ground(WND_EUTEL_COLLECT, false, false);
    _eutel_collect_timer_add();
    return 0;
}

void app_eutel_collect_info_create_dialog(void)
{
    EutelCollectCtrl *ctrl = &s_eutel_collect_ctrl;
    PopDlg pop = {0};

    app_eutel_collect_release();
    app_eutel_collect_parse();
    if(0 == ctrl->item_num)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.timeout_sec = 2;
        pop.str = STR_ID_FTAEPG_NO_LOGS;
        popdlg_create(&pop);
        return;
    }

    GUI_CreateDialog(WND_EUTEL_COLLECT);
    //app_set_window_back_ground(WND_EUTEL_COLLECT, NULL, false);
    GUI_SetProperty(TEXT_COLLECT_TITLE, "string", STR_ID_UPDATES);
}

int app_eutel_collect_update_dialog(void)
{
    if(GUI_CheckDialog(WND_EUTEL_COLLECT) == GXCORE_SUCCESS)
    {
        GUI_SetProperty(LISTVIEW_COLLECT, "update_all", NULL);
    }
    return 0;
}
#endif

