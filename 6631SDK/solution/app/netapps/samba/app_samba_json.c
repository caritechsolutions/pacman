#include "app_config.h"

#if SAMBA_SUPPORT
#include "module/app_log.h"
#include "app_samba_db.h"
#include "gxcore.h"
#include "jansson.h"

#define SAMBA_DB_DEBUG
#ifdef SAMBA_DB_DEBUG
#define SAMBA_DB_PRINT(...)    app_log_debug(__VA_ARGS__)
#else
#define SAMBA_DB_PRINT(...)
#endif

#define SAMBA_DB_PATH    "/usr/share/app/samba/samba_db"
#define PATH_KEY_STR    "svr_path"
#define USER_KEY_STR    "username"
#define PASSWD_KEY_STR    "password"
#define ENABLE_KEY_STR    "enable"

static AppSambaIDListClass samba_id_list;

static status_t _samba_db_parse_info(const char *info_key, json_t *info_value, AppSambaInfoClass *samba_info)
{
    status_t ret = GXCORE_ERROR;

    if((info_key == NULL) || (info_value == NULL) ||(samba_info == NULL))
        return GXCORE_ERROR;

    SAMBA_DB_PRINT("[%s][%d]info_key = %s, type = %d\n", __func__, __LINE__, info_key, json_typeof(info_value));

    if((strcmp(info_key, PATH_KEY_STR) == 0) && (json_is_string(info_value)))
    {
        strlcpy(samba_info->samba_svr_path, json_string_value(info_value), sizeof(samba_info->samba_svr_path));
        ret = GXCORE_SUCCESS;
    }

    if((strcmp(info_key, USER_KEY_STR) == 0) && (json_is_string(info_value)))
    {
        strlcpy(samba_info->samba_username, json_string_value(info_value), sizeof(samba_info->samba_username));
        ret = GXCORE_SUCCESS;
    }

    if((strcmp(info_key, PASSWD_KEY_STR) == 0) && (json_is_string(info_value)))
    {
        strlcpy(samba_info->samba_passwd, json_string_value(info_value), sizeof(samba_info->samba_passwd));
        ret = GXCORE_SUCCESS;
    }

    if((strcmp(info_key, ENABLE_KEY_STR) == 0) && (json_is_integer(info_value)))
    {
        samba_info->samba_enable = json_integer_value(info_value);
        ret = GXCORE_SUCCESS;
    }

    return ret;
}

static status_t _samba_new_db(const char *file)
{
    status_t ret = GXCORE_ERROR;
    json_t *new_object = NULL;

    if((file != NULL) && ((new_object = json_object()) != NULL))
    {
        json_dump_file(new_object, file, 0);
        json_decref(new_object);
        ret = GXCORE_SUCCESS;
    }

    return ret;
}

int app_samba_db_int(AppSambaInfoClass (*samba_list)[MAX_SAMBA_SVR_NUM])
{
    json_t *samba_object = NULL;
    const char *obj_key = NULL;
    json_t *obj_value = NULL;
    const char *info_key = NULL;
    json_t *info_value = NULL;
    json_error_t error;
    unsigned int id = 0;
    int cnt = 0;
    AppSambaInfoClass *samba_array = NULL;

    memset(&samba_id_list, 0, sizeof(AppSambaIDListClass));
    if(GxCore_FileExists(SAMBA_DB_PATH) == GXCORE_FILE_UNEXIST)
    {
        _samba_new_db(SAMBA_DB_PATH);
        return 0;
    }

    if(samba_list == NULL)
        return 0;

    samba_array = *samba_list;

    samba_object = json_load_file(SAMBA_DB_PATH, 0, &error);
    SAMBA_DB_PRINT("[%s]source %s\n", __func__, error.source);
    SAMBA_DB_PRINT("[%s]text %s\n", __func__, error.text);

    if(!json_is_object(samba_object))
    {
        json_decref(samba_object);
        GxCore_FileDelete(SAMBA_DB_PATH);
        _samba_new_db(SAMBA_DB_PATH);
        return 0;
    }

    SAMBA_DB_PRINT("[%s]%d obj type = %d\n", __func__, __LINE__, json_typeof(samba_object));

    char *result = json_dumps(samba_object, JSON_PRESERVE_ORDER);
    SAMBA_DB_PRINT("[%s]result %s\n", __func__, result);
    GxCore_Free(result);

    json_object_foreach(samba_object, obj_key, obj_value)
    {
        if(json_typeof(obj_value) == JSON_OBJECT)
        {
            SAMBA_DB_PRINT("[%s][%d]obj_key = %s, type = %d\n", __func__, __LINE__, obj_key, json_typeof(obj_value));
            id = atoi(obj_key);
            if((id > 0)
                    && (id <= MAX_SAMBA_SVR_NUM)
                    && (samba_array[id - 1].samba_id != id))
            {
                samba_id_list.id_list[samba_id_list.id_num++] = id;
                samba_array[id - 1].samba_id = id;
                json_object_foreach(obj_value, info_key, info_value)
                {
                    _samba_db_parse_info(info_key, info_value, &samba_array[id - 1]);
                }
                cnt++;
                if(cnt >= MAX_SAMBA_SVR_NUM)
                    break;
            }
        }
    }

    json_decref(samba_object);

    return cnt;
}

status_t app_samba_db_add(AppSambaInfoClass *samba_info)
{
    json_t *samba_object = NULL;
    json_t *new_samba = NULL;
    json_error_t error;
    char samba_id[20] = {0};
    status_t ret = GXCORE_SUCCESS;

    if(samba_info == NULL)
        goto samba_db_add_ret;

    if(GxCore_FileExists(SAMBA_DB_PATH) == GXCORE_FILE_UNEXIST)
    {
        _samba_new_db(SAMBA_DB_PATH);
    }

    samba_object = json_load_file(SAMBA_DB_PATH, 0, &error);
    SAMBA_DB_PRINT("[%s]source %s\n", __func__, error.source);
    SAMBA_DB_PRINT("[%s]text %s\n", __func__, error.text);

    if(!json_is_object(samba_object))
        goto samba_db_add_ret;

    if((new_samba = json_object()) == NULL)
        goto samba_db_add_ret;

    SAMBA_DB_PRINT("[%s]%d: obj type = %d\n", __func__, __LINE__, json_typeof(samba_object));

    char *result = json_dumps(samba_object, JSON_PRESERVE_ORDER);
    SAMBA_DB_PRINT("[%s]%d: result %s\n", __func__, __LINE__, result);
    GxCore_Free(result);

    if(strlen(samba_info->samba_svr_path) > 0)
        json_object_set_new(new_samba, PATH_KEY_STR, json_string(samba_info->samba_svr_path));
    if(strlen(samba_info->samba_username) > 0)
        json_object_set_new(new_samba, USER_KEY_STR, json_string(samba_info->samba_username));
    if(strlen(samba_info->samba_passwd) > 0)
        json_object_set_new(new_samba, PASSWD_KEY_STR, json_string(samba_info->samba_passwd));
    json_object_set_new(new_samba, ENABLE_KEY_STR, json_integer(samba_info->samba_enable));

    snprintf(samba_id, sizeof(samba_id), "%d", samba_info->samba_id);
    json_object_set_new(samba_object, samba_id, new_samba);

    result = json_dumps(samba_object, JSON_PRESERVE_ORDER);
    SAMBA_DB_PRINT("[%s]%d: result %s\n", __func__, __LINE__, result);
    GxCore_Free(result);

    json_dump_file(samba_object, SAMBA_DB_PATH, 0);
    json_decref(new_samba);

    ret = GXCORE_SUCCESS;

    if(samba_id_list.id_num<MAX_SAMBA_SVR_NUM)
    {
        samba_id_list.id_list[samba_id_list.id_num++] = samba_info->samba_id;
    }

samba_db_add_ret:
    if(samba_object != NULL)
        json_decref(samba_object);

    return ret;
}

status_t app_samba_db_delete(uint32_t samba_id)
{
    json_t *samba_object = NULL;
    json_error_t error;
    char id_str[20] = {0};
    int i = 0;

    samba_object = json_load_file(SAMBA_DB_PATH, 0, &error);
    if(!json_is_object(samba_object))
    {
        json_decref(samba_object);
        return GXCORE_ERROR;
    }

    snprintf(id_str, sizeof(id_str), "%d", samba_id);
    json_object_del(samba_object, id_str);
    json_dump_file(samba_object, SAMBA_DB_PATH, 0);

    json_decref(samba_object);

    for(i=0; i<samba_id_list.id_num; i++)
    {
    	 if(samba_id_list.id_list[i] == samba_id)
        {
            if(i==(samba_id_list.id_num-1))
            {
                samba_id_list.id_list[i] = 0;
            }
	     else
	     {
                memmove(&(samba_id_list.id_list[i]),
                    &(samba_id_list.id_list[i+1]), sizeof(int)*(samba_id_list.id_num-(i+1)));
	     }
	     samba_id_list.id_num--;
    	    break;
        }
    }
    return GXCORE_SUCCESS;
}

status_t app_samba_db_modify(uint32_t samba_id, AppSambaInfoClass *samba_info)
{
    json_t *samba_obj = NULL;
    json_t *info_obj = NULL;
    json_error_t error;
    char id_str[20] = {0};

    if(samba_info == NULL)
        return GXCORE_ERROR;

    samba_obj = json_load_file(SAMBA_DB_PATH, 0, &error);
    if(!json_is_object(samba_obj))
    {
        json_decref(samba_obj);
        return GXCORE_ERROR;
    }

    snprintf(id_str, sizeof(id_str), "%d", samba_id);
    info_obj = json_object_get(samba_obj, id_str);
    if(!json_is_object(info_obj))
    {
        json_decref(info_obj);
        json_decref(samba_obj);
        return GXCORE_ERROR;
    }

    if(strlen(samba_info->samba_svr_path) > 0)
        json_object_set_new(info_obj, PATH_KEY_STR, json_string(samba_info->samba_svr_path));
    else
        json_object_del(info_obj, PATH_KEY_STR);

    if(strlen(samba_info->samba_username) > 0)
        json_object_set_new(info_obj, USER_KEY_STR, json_string(samba_info->samba_username));
    else
        json_object_del(info_obj, USER_KEY_STR);

    if(strlen(samba_info->samba_passwd) > 0)
        json_object_set_new(info_obj, PASSWD_KEY_STR, json_string(samba_info->samba_passwd));
    else
        json_object_del(info_obj, PASSWD_KEY_STR);

    json_object_set_new(info_obj, ENABLE_KEY_STR, json_integer(samba_info->samba_enable));

    json_dump_file(samba_obj, SAMBA_DB_PATH, 0);

    json_decref(info_obj);
    json_decref(samba_obj);

    return GXCORE_SUCCESS;
}

AppSambaIDListClass *app_samba_db_get_idlist(void)
{
	return &samba_id_list;
}

#endif

