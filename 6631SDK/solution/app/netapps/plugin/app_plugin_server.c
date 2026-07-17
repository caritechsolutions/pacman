/*************************************************************************
	> File Name: app_plugin_server.c
	> Author:
	> Mail:
	> Created Time: 2016年08月30日 星期二 09时24分18秒
 ************************************************************************/

#include "app.h"
#if PLUGINSTORE_SUPPORT
#include "app_plugin_server.h"
#include "jansson.h"

void Plugin_msg_register(void)
{
    GxBus_MessageRegister(APPMSG_PLUGIN_SOURCE_UI_CONTROL, sizeof(struct PlugiaSourceUIMsg));
}

void Plugin_msg_unregister(void)
{
    GxBus_MessageUnregister(APPMSG_PLUGIN_SOURCE_UI_CONTROL);
}

char *source_url_json_parse(char *string)
{
    json_error_t error;
    json_t *object = NULL;
    const char *obj_key = NULL;
    json_t *obj_value = NULL;
    json_t *array_value = NULL;
    const char *info_key = NULL;
    json_t *info_value = NULL;
    char *url;
    int index = 0;

    while(*(string++) != ':' && *string != '\0');
    object = json_loads(string, JSON_DISABLE_EOF_CHECK, &error);
    if(NULL == object)
        return NULL;

    if(!json_is_object(object))
    {
        json_decref(object);
        return NULL;
    }

    json_object_foreach(object, obj_key, obj_value)
    {
        if(strcmp(obj_key, "sources") == 0)
        {
            while(1)
            {
                array_value = json_array_get(obj_value, index);
                if(array_value == NULL)
                    break;
                if(!json_is_object(array_value))
                {
                    json_decref(object);
                    return NULL;
                }
                json_object_foreach(array_value, info_key, info_value)
                {
                    if(strcmp(info_key, "url") == 0)
                    {
                        url = GxCore_Strdup(json_string_value(info_value));
                        json_decref(object);
                        return url;
                    }
                }
                index++;
            }
            break;
        }
    }
    json_decref(object);
    return NULL;
}

int fs_read_permission_check(char *name)
{
    if(name == NULL)
        return -1;
    if(strncmp(name, "/home/gx/plugincfg", 18) == 0)
        return 0 ;
    return -1;
}

const char *app_dataroot(void)
{
    return "/home/gx/plugin_env";
}
const char *app_plugin_installed_path(void)
{
    return "/home/gx/plugin_env/plugins";
}

void app_plugin_source_deal_err_from_ecmascript(void)
{
    app_plugin_source_list_deal_err_from_ecmascript();
    app_plugin_listview_play_deal_err_from_ecmascript();
}
#endif
