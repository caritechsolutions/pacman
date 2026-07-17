#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "app_cover_pvr.h"
#include "cJSON.h"

#define _fmalloc _cover_pvr_malloc
#define _ffree _cover_pvr_free
#define _fclose_fp(fp)  \
    do                  \
    {                   \
        if(fp)          \
            fclose(fp); \
        fp = NULL;      \
    } while(0)
#define _free_ptr(ptr)   \
    do                   \
    {                    \
        if(ptr)          \
            _ffree(ptr); \
        ptr = NULL;      \
    } while(0)

static int _free_read_file_data(uint8_t **data, int *size)
{
    if(data)
        _free_ptr(*data);
    if(size)
        *size = 0;
    return 0;
}

static int _read_file_to_data(const char *src, uint8_t **data, int *size)
{
    FILE *rfp = NULL;
    int total = 0;

    if(!src || !data)
        return -1;

    if(!size)
        size = &total;
    *data = NULL, *size = 0;

    if((rfp = fopen(src, "r")) == NULL)
        return -1;
    fseek(rfp, 0, SEEK_END);
    *size = ftell(rfp);
    fseek(rfp, 0, SEEK_SET);
    if(*size == 0)
        goto err;

    if((*data = _fmalloc(*size + 1)) == NULL)
        goto err;
    if(*size != fread(*data, 1, *size, rfp))
        goto err;

    (*data)[*size] = 0;
    _fclose_fp(rfp);
    return 0;
err:
    _fclose_fp(rfp);
    _free_ptr(*data);
    *size = 0;
    return -1;
}

static cJSON *_get_json(bool from_file, const char *json_data)
{
    cJSON *json = NULL;
    char *data = NULL;
    int len = 0;

    if(!json_data)
    {
        return NULL;
    }
    if(!from_file)
    {
        return cJSON_Parse(json_data);
    }
    if(access(json_data, F_OK) < 0)
    {
        return NULL;
    }
    if(_read_file_to_data(json_data, (uint8_t **)&data, &len) < 0)
    {
        return NULL;
    }
    json = cJSON_Parse(data);
    _free_read_file_data((uint8_t **)&data, &len);

    return json;
}

int cover_pvr_copy_file_to_file(const char *src, const char *dst, int unit_size)
{
    int ret = -1;
    FILE *rfp = NULL, *wfp = NULL;
    uint8_t *data = NULL;
    int size = 0;
    int total_size = 0;

    if(!src || !dst)
        return -1;
    if((rfp = fopen(src, "r")) == NULL)
        return -1;
    if((wfp = fopen(dst, "w+")) == NULL)
        goto end;
    if(unit_size < 512)
        unit_size = 8192;
    if((data = _fmalloc(unit_size)) == NULL)
        goto end;
    while((size = fread(data, 1, unit_size, rfp)) > 0)
    {
        if(size != fwrite(data, 1, size, wfp))
            goto end;
        total_size += size;
    }

    ret = total_size;
end:
    _fclose_fp(rfp);
    _fclose_fp(wfp);
    _free_ptr(data);
    if(ret < 0 && access(dst, F_OK) == 0)
        unlink(dst);
    return ret;
}

int cover_pvr_copy_data_to_file(uint8_t *data, int size, const char *dst)
{
    int ret = -1;
    FILE *wfp = NULL;

    if(!data || !dst)
        return -1;
    if((wfp = fopen(dst, "w+")) == NULL)
        return -1;
    if(size == fwrite(data, 1, size, wfp))
        ret = size;
    _fclose_fp(wfp);
    if(ret < 0 && access(dst, F_OK) == 0)
        unlink(dst);

    return ret;
}

static inline int _calloc(void **dst, size_t nmemb, size_t size)
{
    if(nmemb > 0)
    {
        *dst = _cover_pvr_calloc(nmemb, size);
    }

    if(nmemb > 0 && !(*dst))
    {
        return -1;
    }

    return 0;
}

int app_cover_pvr_free_epg_record(struct cover_pvr_epg_record *record)
{
    int i = 0;
    if(!record)
        return -1;

    COVER_PVR_FREE(record->fname);
    COVER_PVR_FREE(record->chname);
    for(i = 0; i < record->contents_num; i++)
    {
        COVER_PVR_FREE(record->contents[i]);
    }
    COVER_PVR_FREE(record->contents);
    COVER_PVR_FREE(record->name);
    COVER_PVR_FREE(record->description);
#if PVR_PREVIEW_SUPPORT
    COVER_PVR_FREE(record->preview.path);
#endif
    memset(record, 0, sizeof(struct cover_pvr_epg_record));

    return 0;
}

#if PVR_PREVIEW_SUPPORT
int app_cover_pvr_preview_info_record(struct cover_pvr_epg_record *record, PreviewInfo *info)
{
    if(!record || !info)
        return -1;

    record->preview.flag = info->flag;
    if (PREVIEW_FROM_PATH == info->flag) {
        if(record->preview.path)
            COVER_PVR_FREE(record->preview.path);
        if(info->path)
            record->preview.path = _cover_pvr_strdup(info->path);
    }
    else if (PREVIEW_FROM_TIME == info->flag) {
        record->preview.offset = info->offset;
    }

    return 0;
}
#endif

int app_cover_pvr_free_epg_record_all(struct cover_pvr_epg_record **record, int *num)
{
    int i = 0;
    if(!record || !num)
        return -1;

    for(i = 0; i < *num; i++)
    {
        app_cover_pvr_free_epg_record(&(*record)[i]);
    }
    COVER_PVR_FREE(*record);
    *record = NULL;
    *num = 0;

    return 0;
}

static int _parse_epg_record(cJSON *json, struct cover_pvr_epg_record *record)
{
    cJSON *item = NULL;
    cJSON *contents = NULL;
    cJSON *content = NULL;
    int i = 0;
    int contents_num = 0;

    if(!json || !record)
        return -1;

    item = cJSON_GetObjectItem(json, "filename");
    GET_OSTRING(record->fname, item);
    item = cJSON_GetObjectItem(json, "chname");
    GET_OSTRING(record->chname, item);
    item = cJSON_GetObjectItem(json, "startTime");
    GET_ONUMBER(record->starttime, item);
    item = cJSON_GetObjectItem(json, "finishTime");
    GET_ONUMBER(record->finishtime, item);
    item = cJSON_GetObjectItem(json, "recStart");
    GET_ONUMBER(record->recstart, item);
    item = cJSON_GetObjectItem(json, "duration");
    GET_ONUMBER(record->duration, item);

    contents = cJSON_GetObjectItem(json, "contentGenres");
    if(contents && contents->type == cJSON_Array)
    {
        contents_num = cJSON_GetArraySize(contents);
        if(contents_num > 0)
        {
            if(_calloc((void **)&record->contents, contents_num, sizeof(char *)) == 0)
            {
                for(i = 0; i < contents_num; i++)
                {
                    content = cJSON_GetArrayItem(contents, i);
                    GET_OSTRING(record->contents[record->contents_num], content);
                    if(record->contents[record->contents_num])
                        ++record->contents_num;
                }
            }
        }
    }

    item = cJSON_GetObjectItem(json, "parentalRating");
    GET_ONUMBER(record->parental, item);
    item = cJSON_GetObjectItem(json, "name");
    GET_OSTRING(record->name, item);
    item = cJSON_GetObjectItem(json, "description");
    GET_OSTRING(record->description, item);
    item = cJSON_GetObjectItem(json, "freeScan");
    GET_ONUMBER(record->from_freescan, item);
    item = cJSON_GetObjectItem(json, "previewFlag");
#if PVR_PREVIEW_SUPPORT
    GET_ONUMBER(record->preview.flag, item);
    item = cJSON_GetObjectItem(json, "previewOffset");
    GET_ONUMBER(record->preview.offset, item);
    item = cJSON_GetObjectItem(json, "previewPath");
    GET_OSTRING(record->preview.path, item);
#endif

    return 0;
}

static int _generate_epg_record(cJSON *json, struct cover_pvr_epg_record *record)
{
    cJSON *contents = NULL;

    if(!json || !record)
        return -1;

    if(!record->fname)
        return -1;
    cJSON_AddStringToObject(json, "filename", record->fname);
    if(record->chname)
        cJSON_AddStringToObject(json, "chname", record->chname);
    if(record->starttime)
        cJSON_AddNumberToObject(json, "startTime", record->starttime);
    if(record->finishtime)
        cJSON_AddNumberToObject(json, "finishTime", record->finishtime);
    if(record->recstart)
        cJSON_AddNumberToObject(json, "recStart", record->recstart);
    if(record->duration)
        cJSON_AddNumberToObject(json, "duration", record->duration);
    if(record->contents_num > 0 && record->contents)
    {
        if((contents = cJSON_CreateStringArray((const char **)record->contents, record->contents_num)) == NULL)
            return -1;
        cJSON_AddItemToObject(json, "contentGenres", contents);
    }
    if(record->parental)
        cJSON_AddNumberToObject(json, "parentalRating", record->parental);
    if(record->name)
        cJSON_AddStringToObject(json, "name", record->name);
    if(record->description)
        cJSON_AddStringToObject(json, "description", record->description);
    if(record->from_freescan)
        cJSON_AddNumberToObject(json, "freeScan", record->from_freescan);
#if PVR_PREVIEW_SUPPORT
    if(record->preview.offset && record->preview.flag == PREVIEW_FROM_TIME)
        cJSON_AddNumberToObject(json, "previewOffset", record->preview.offset);
    if(record->preview.path && record->preview.flag == PREVIEW_FROM_PATH)
        cJSON_AddStringToObject(json, "previewPath", record->preview.path);
    cJSON_AddNumberToObject(json, "previewFlag", record->preview.flag);
#endif

    return 0;
}

int app_cover_pvr_parse_epg_record_info(const char *path, struct cover_pvr_epg_record *record)
{
    int ret = -1;
    cJSON *json = NULL;

    if(!path || !record)
        return -1;

    if((json = _get_json(true, path)) == NULL)
    {
        COVER_PVR_ERR("Error before: [%s]\n", cJSON_GetErrorPtr());
        goto end;
    }
    ret = _parse_epg_record(json, record);

end:
    _free_json_obj(json);
    return ret;
}

int app_cover_pvr_parse_epg_record_all_info(const char *index_path, struct cover_pvr_epg_record **record, int *num)
{
    cJSON *json = NULL;
    cJSON *array = NULL;
    int total = 0;
    int i = 0;

    if(!index_path || !record || !num)
        return 0;

    *record = NULL, *num = 0;
    if((array = _get_json(true, index_path)) == NULL || array->type != cJSON_Array)
    {
        goto err;
    }

    total = cJSON_GetArraySize(array);
    if(_calloc((void **)record, total, sizeof(struct cover_pvr_epg_record)) < 0)
    {
        goto err;
    }

    for(i = 0; i < total; i++)
    {
        json = cJSON_GetArrayItem(array, i);
        if(_parse_epg_record(json, &(*record)[i]) == 0)
            (*num)++;
    }
    if(*num == 0)
        goto err;

    _free_json_obj(array);
    return *num;
err:
    app_cover_pvr_free_epg_record_all(record, num);
    _free_json_obj(array);
    return 0;
}

int app_cover_pvr_save_epg_record_info(const char *info_path, const char *index_path, struct cover_pvr_epg_record *record)
{
    int ret = -1;
    char *json_str = NULL;
    cJSON *json = NULL;
    cJSON *array = NULL;

    if(!info_path || !index_path || !record)
        return -1;

    if((json = cJSON_CreateObject()) == NULL)
    {
        return -1;
    }
    if(_generate_epg_record(json, record) < 0)
    {
        goto end;
    }

    // save info_path
    if((json_str = cJSON_PrintUnformatted(json)) == NULL)
    {
        goto end;
    }
    COVER_PVR_INFO("Path:%s; EpgPvrJson:\n\033[32m%s\033[0m\n", info_path, json_str);
    if(cover_pvr_copy_data_to_file((uint8_t *)json_str, strlen(json_str), info_path) < 0)
    {
        goto end;
    }

    // save index_path
    if((array = _get_json(true, index_path)) == NULL || array->type != cJSON_Array)
    {
        if((array = cJSON_CreateArray()) == NULL)
        {
            goto end;
        }
    }
    cJSON_AddItemToArray(array, json);
    json = NULL;
    _free_json_str(json_str);
    if((json_str = cJSON_PrintUnformatted(array)) == NULL)
    {
        goto end;
    }
    if(cover_pvr_copy_data_to_file((uint8_t *)json_str, strlen(json_str), index_path) < 0)
    {
        goto end;
    }

    ret = 0;
end:
    _free_json_str(json_str);
    _free_json_obj(json);
    _free_json_obj(array);
    return ret;
}

int app_cover_pvr_save_epg_record_all_info(const char *index_path, struct cover_pvr_epg_record *record, int record_num, char **extra_path, int extra_num, struct cover_pvr_epg_record_func *record_func)
{
    int ret = 0;
    char *json_str = NULL;
    char *fname = NULL;
    cJSON *json = NULL;
    cJSON *array = NULL;
    cJSON *item = NULL;
    int i = 0;
    int cnt = 0;

    if(!index_path)
        return ret;
    if((array = cJSON_CreateArray()) == NULL)
    {
        return ret;
    }

    if(record && record_num > 0)
    {
        for(i = 0; i < record_num; i++)
        {
            if((record[i].flag == EPG_RECORD_EXIST) || (record[i].flag == EPG_RECORD_ADD))
            {
                if((json = cJSON_CreateObject()) != NULL)
                {
                    if(_generate_epg_record(json, &record[i]) == 0)
                    {
                        cJSON_AddItemToArray(array, json);
                        cnt++;
                    }
                    else
                    {
                        _free_json_obj(json);
                    }
                }
            }
        }
    }

    if(extra_path && extra_num > 0)
    {
        for(i = 0; i < extra_num; i++)
        {
            if(record_func && record_func->stop_check && record_func->stop_check())
                break;
            if((json = _get_json(true, extra_path[i])) != NULL)
            {
                item = cJSON_GetObjectItem(json, "filename");
                if(!item || item->type != cJSON_String || !item->valuestring)
                {
                    if(record_func && record_func->fname_get && record_func->fname_free)
                    {
                        if((fname = record_func->fname_get(extra_path[i])) != NULL)
                        {
                            cJSON_AddStringToObject(json, "filename", fname);
                            record_func->fname_free(fname);
                            cJSON_AddItemToArray(array, json);
                            cnt++;
                        }
                        else
                        {
                            _free_json_obj(json);
                        }
                    }
                    else
                    {
                        _free_json_obj(json);
                    }
                }
                else
                {
                    cJSON_AddItemToArray(array, json);
                    cnt++;
                }
            }
        }
    }

    if((json_str = cJSON_PrintUnformatted(array)) == NULL)
    {
        goto end;
    }
    COVER_PVR_INFO("\033[33mRebuild %s, total = %d\033[0m\n", index_path, cnt);
    COVER_PVR_DBG("\033[33mRebuild %s, total = %d\n%s\033[0m\n", index_path, cnt, json_str);
    if(cover_pvr_copy_data_to_file((uint8_t *)json_str, strlen(json_str), index_path) < 0)
    {
        goto end;
    }

    ret = cnt;
end:
    _free_json_str(json_str);
    _free_json_obj(array);
    return ret;
}
