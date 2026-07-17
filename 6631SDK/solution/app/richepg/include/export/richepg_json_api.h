#ifndef __RICHEPG_JSON_API_H__
#define __RICHEPG_JSON_API_H__
#include "cJSON.h"
#include "richepg_common.h"

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
            string = _richepg_strdup(item->valuestring);\
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
            string = _richepg_strdup(item->valuestring);\
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
cJSON *richepg_get_json(bool from_file, const char *json_data);

#endif

