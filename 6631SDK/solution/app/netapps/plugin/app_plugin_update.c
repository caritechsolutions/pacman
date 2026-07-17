/*************************************************************************
	> File Name: app_plugin_update.c
	> Author:
	> Mail:
	> Created Time: 2016年10月20日 星期四 13时40分11秒
 ************************************************************************/

#include "app.h"
#if PLUGINSTORE_SUPPORT
#include "jansson.h"
#include "app_plugin_update.h"
#include <gx_apps.h>
#include <dirent.h>
#include <sys/stat.h>

bool thiz_PluginPrintEnable = false;

#if PLUGINONLINE_SUPPORT
#define PLUGIN_LIST_JSON_FILE_NAME "plugin_list.json"
#define PLUGIN_LIST_JSON_FILE_URL  "http://192.168.110.133:88/plugins/plugin_list.json.zip"

static void _plugin_classification_list_free(PLuginClassificationList *head);

enum DOWNLOAD_STATUS_TYPE{
    DOWNLOAD_OK,
    DOWNLOAD_FAIL,
    DOWNLOAD_WAIT
};

struct VersionClass{
    int major;
    int minor;
    int modified;
};

extern PluginInfoList *Pluginstore_focus_list_get(void);
static handle_t plugin_download_thread_handle = -1;
static int this_download_status = DOWNLOAD_FAIL;
static int plugin_need_update_counter = 0;

static void _plugin_download_cb(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

    if(message == GXCURL_STATE_COMPLETE)
    {
        if(callback_data->handle > 0)
        {
            if(plugin_download_thread_handle == callback_data->handle)
            {
                if(callback_data->complete_data.retcode == GXAPPS_SUCCESS && callback_data->complete_data.size > 0)
                {
                    this_download_status = DOWNLOAD_OK;
                    PLUGIN_INFO("*****download****ok*****");
                }
                else
                {
                    this_download_status = DOWNLOAD_FAIL;
                    PLUGIN_ERR("*****download****error*****");
                }
            }
        }
    }
}

static void _plugin_download_stop(void)
{
    if(plugin_download_thread_handle >= 0)
    {
        gxapps_curl_async_abort(plugin_download_thread_handle);
        plugin_download_thread_handle = -1;
    }
}

void Plugin_download_stop(void)
{
    this_download_status = DOWNLOAD_FAIL;
    GxCore_ThreadDelay(50); //delay for exit
}

static int _plugin_download_start(char *url, char *name, int mode)
{
    int i = 0;
    uint32_t length = 0;
    uint32_t command_length = 0;
    char *path = NULL;
    char *new_path = NULL;
    char *command = NULL;

    if(NULL == url || NULL == name)
        return -1;

    length = strlen(TMP_DOWNLOAD_PATH) + strlen(name) + 20;
    if(length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return -1;
    }

    path = GxCore_Calloc(length, sizeof(char));
    if(NULL == path)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }

    snprintf(path, length, "%s/%s.zip", TMP_DOWNLOAD_PATH, name);
    this_download_status = DOWNLOAD_WAIT;
    _plugin_download_stop();
    plugin_download_thread_handle = gxapps_curl_async_download(url, path, _plugin_download_cb);
    while(this_download_status != DOWNLOAD_OK)
    {
        i++;
        GxCore_ThreadDelay(10);
        if(i > 800 || this_download_status == DOWNLOAD_FAIL)
        {
            PLUGIN_ERR("!!!!download fail!!!!\n");
            _plugin_download_stop();
            goto err;
        }
    }

    if(mode == 1)
    {
        length = strlen(app_plugin_installed_path()) + strlen(name) + 20;
        if(length >= MAX_PATH_LEN)
        {
            PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
            goto err;
        }

        new_path = GxCore_Calloc(length, sizeof(char));
        if(NULL == new_path)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            goto err;
        }

        snprintf(new_path, length, "%s/%s.zip", app_plugin_installed_path(), name);

        command_length = strlen(path) + strlen(new_path) + 20;
        if(command_length >= 1024)
        {
            PLUGIN_ERR("path length exceeds the maximnum(1024) limit\n");
            goto err;
        }

        command = GxCore_Calloc(command_length, sizeof(char));
        if(NULL == command)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            goto err;
        }

        snprintf(command, command_length, "mv %s %s ", path, new_path);
        system(command);
        system("sync");
        if(GXCORE_FILE_EXIST != GxCore_FileExists(new_path))
        {
            PLUGIN_ERR("copy file error\n");
            goto err;
        }

        APP_FREE(new_path);
        APP_FREE(path);
    }

    APP_FREE(path);
    return 0;
err:
    APP_FREE(path);
    APP_FREE(new_path);
    APP_FREE(command);
    return -1;
}

static int _version_check(PLuginClassificationList *head,  PluginUpdateNode *node, int total_number)
{
    PluginUpdateNode *pos = NULL;
    PluginClassificationNode *classification_node = NULL;

    classification_node = list_entry(head->list.next, typeof(*head), list);
    if(total_number > classification_node->update_list_head->counter)
        return 0;

    list_for_each_entry(pos, &(classification_node->update_list_head->list), list)
    {
        if(strcmp(pos->id, node->id) == 0)
        {
            pos->repo_ver = GxCore_Strdup(node->repo_ver);
            node->ins_ver = GxCore_Strdup(pos->ins_ver);
            if(Plugin_update_check(node) >= 0)
            {
                node->status = STR_ID_UPGRADE;
                pos->status = STR_ID_UPGRADE;
                plugin_need_update_counter++;
            }
            else
            {
                node->status = pos->status;
            }

            if(node->url)
                pos->url = GxCore_Strdup(node->url);

            pos->link = node;  //for update and uninstall link
            node->link = pos;
            return -1;
        }
    }
    return 0;
}

static int plugin_list_json_parse(char *file, PLuginClassificationList *head)
{
    json_error_t error;
    json_t *object = NULL;
    json_t *obj_value = NULL;
    json_t *array_value = NULL;
    json_t *info_value = NULL;
    const char *obj_key = NULL;
    const char *info_key = NULL;
    uint32_t index = 0;
    uint32_t i = 0;

    PluginClassificationNode *classification_node = NULL;
    PluginUpdateNode *update_node = NULL;

    object = json_load_file(file, JSON_DISABLE_EOF_CHECK, &error);
    if(NULL == object)
        return -1;

    if(!json_is_object(object))
    {
        json_decref(object);
        return -1;
    }

    json_object_foreach(object, obj_key, obj_value)
    {
        classification_node = (PluginClassificationNode *)GxCore_Calloc(1, sizeof(PluginClassificationNode));
        if(classification_node == NULL)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            goto err;
        }
        head->counter++;
        classification_node->name = GxCore_Strdup(obj_key);
        list_add_tail(&(classification_node->list), &(head->list));

        classification_node->update_list_head = (PluginUpdateList *)GxCore_Calloc(1, sizeof(PluginUpdateList));
        if(classification_node->update_list_head == NULL)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            goto err;
        }
        INIT_DLIST_HEAD(&(classification_node->update_list_head->list));

        index = 0;
        while(1)
        {
            array_value = json_array_get(obj_value, index);
            if(array_value == NULL)
                break;

            if(!json_is_object(array_value))
            {
                PLUGIN_ERR("json parse err\n");
                goto err;
            }

            update_node = (PluginUpdateNode *)GxCore_Calloc(1, sizeof(PluginUpdateNode));
            if(update_node == NULL)
            {
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                goto err;
            }
            json_object_foreach(array_value, info_key, info_value)
            {
                if(strcmp(info_key, "id") == 0)
                    update_node->id = GxCore_Strdup(json_string_value(info_value));
                else if(strcmp(info_key, "name") == 0)
                    update_node->name = GxCore_Strdup(json_string_value(info_value));
                else if(strcmp(info_key, "version") == 0)
                    update_node->repo_ver = GxCore_Strdup(json_string_value(info_value));
                else if(strcmp(info_key, "icon") == 0)
                    update_node->icon = GxCore_Strdup(json_string_value(info_value));
                else if(strcmp(info_key, "url") == 0)
                    update_node->url = GxCore_Strdup(json_string_value(info_value));
                else if(strcmp(info_key, "describe") == 0)
                    update_node->shortdesc = GxCore_Strdup(json_string_value(info_value));
                else
                    PLUGIN_ERR("value: %s is invalid, please check\n", json_string_value(info_value));
            }

            if(update_node->repo_ver)
                update_node->status = STR_ID_NOT_INSTALLED;

            if(_version_check(head, update_node, i) < 0)
                i++;

            index++;
            update_node->counter = index;

            list_add_tail(&(update_node->list), &(classification_node->update_list_head->list));
        }
        classification_node->update_list_head->counter = index;
    }
    json_decref(object);
    return 0;
err:
    json_decref(object);
    _plugin_classification_list_free(head);
    return -1;
}
#endif

static void _plugin_info_node_free(PluginInfoNode *node)
{
    if(node != NULL)
    {
        APP_FREE(node->id);
        APP_FREE(node->title);
        APP_FREE(node->version);
        APP_FREE(node->synopsis);
        APP_FREE(node->ui_type);
        APP_FREE(node->icon_url);
        APP_FREE(node->folder);
        APP_FREE(node->setting);
        APP_FREE(node);
    }
    return;
}

static int _plugin_zip_decompression(char *path, char *name)
{
    char *zip_src = NULL;
    uint32_t zip_src_length = 0;

    if(NULL == path || NULL == name)
        return -1;

    zip_src_length = strlen(path) + 20;
    if(zip_src_length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return -1;
    }

    zip_src = GxCore_Calloc(zip_src_length, sizeof(char));
    if(NULL == zip_src)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }

    snprintf(zip_src, zip_src_length, "%s/%s", path, "plugin.json");
    snprintf(name, MAX_PATH_LEN, "%s/%s", TMP_DOWNLOAD_PATH, "plugin.json");
    PLUGIN_INFO("decompression path: %s", zip_src);

    if(Plugin_zip_decompression(zip_src, name) < 0)
    {
        APP_FREE(zip_src);
        return -1;
    }

    APP_FREE(zip_src);
    return 0;
}

static int _plugin_info_node_content_get(PluginInfoNode *node, char *name, char *path)
{
    json_error_t error;
    uint32_t length = 0;
    json_t *object = NULL;
    json_t *obj_value = NULL;
    const char *obj_key = NULL;

    if(NULL == node || NULL == name || NULL == path)
        return -1;

    object = json_load_file(name, JSON_DISABLE_EOF_CHECK, &error);
    if(NULL == object)
        return -1;

    if(!json_is_object(object))
    {
        json_decref(object);
        return -1;
    }

    json_object_foreach(object, obj_key, obj_value)
    {
        if(strcmp(obj_key, "id") == 0)
            node->id = GxCore_Strdup(json_string_value(obj_value));
        else if(strcmp(obj_key, "title") == 0)
            node->title = GxCore_Strdup(json_string_value(obj_value));
        else if(strcmp(obj_key, "version") == 0)
            node->version = GxCore_Strdup(json_string_value(obj_value));
        else if(strcmp(obj_key, "synopsis") == 0)
            node->synopsis = GxCore_Strdup(json_string_value(obj_value));
        else if(strcmp(obj_key, "ui_type") == 0)
            node->ui_type = GxCore_Strdup(json_string_value(obj_value));
        else if(strcmp(obj_key, "setting") == 0)
            node->setting = json_dumps(obj_value, 0);
        else if(strcmp(obj_key, "icon") == 0)
        {
            if(strncmp(json_string_value(obj_value), "http://", 7) == 0)
            {
                node->icon_url = GxCore_Strdup(json_string_value(obj_value));
            }
            else
            {
                memset(name, 0, MAX_PATH_LEN);

                length = strlen(path) + strlen(json_string_value(obj_value)) + 20;
                if(length >= MAX_PATH_LEN)
                {
                    PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
                    json_decref(object);
                    return -1;
                }
                snprintf(name, MAX_PATH_LEN, "%s/%s", path, json_string_value(obj_value));
                node->icon_url = GxCore_Strdup(name);
            }
        }
    }

    json_decref(object);
    return 0;
}

static PluginInfoNode *plugin_info_parse(char *path)
{
    char *name = NULL;
    uint32_t length = 0;
    PluginInfoNode *node = NULL;

    if(0 == strncmp(path, "zip://", 6))
        length = strlen(TMP_DOWNLOAD_PATH) + 20;
    else
        length = strlen(path) + 20;

    if(length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return NULL;
    }

    name = GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == name)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return NULL;
    }

    if(strncmp(path, "zip://", 6) == 0)
    {
        if(-1 == _plugin_zip_decompression(path, name))
            goto err;
    }
    else
    {
        snprintf(name, MAX_PATH_LEN, "%s/%s", path, "plugin.json");
    }

    node = (PluginInfoNode *)GxCore_Calloc(1, sizeof(PluginInfoNode));
    if(NULL == node)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        goto err;
    }

    if(-1 == _plugin_info_node_content_get(node, name, path))
        goto err;

    if(NULL == node->id)
        goto err;

    APP_FREE(name);
    return node;
err:
    _plugin_info_node_free(node);
    APP_FREE(name);
    return NULL;
}

int Plugin_info_list_get(PluginInfoList **head)
{
    int i = 0;
    DIR *dir = NULL;
    DIR *dir_child = NULL;
    struct dirent *ent = NULL;
    struct dirent *ent_child = NULL;
    struct stat buf;
    char *path = NULL;
    char *tmp_name = NULL;
    uint32_t length = 0;
    PluginInfoNode *node = NULL;
    PluginInfoNode *node_plugin = NULL;

    if(NULL == *head)
    {
        *head = (PluginInfoList *)GxCore_Calloc(1, sizeof(PluginInfoList));
        if(NULL == *head)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            return -1;
        }
        INIT_DLIST_HEAD(&((*head)->list));
    }

    dir = opendir(app_plugin_installed_path());
    if(dir == NULL)
        return -1;

    path = GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == path)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        goto err;
    }

    while (NULL != (ent = readdir(dir)))
    {
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        node = NULL;
        memset(path, 0, MAX_PATH_LEN);

        length = strlen(app_plugin_installed_path()) + strlen(ent->d_name) + 20;
        if(length >= MAX_PATH_LEN)
        {
            PLUGIN_ERR("%s path length exceeds the maximnum(%d) limit\n", ent->d_name, MAX_PATH_LEN);
            goto err;
        }

        snprintf(path, MAX_PATH_LEN, "%s/%s", app_plugin_installed_path(), ent->d_name);
        lstat(path, &buf);
        if(S_ISREG(buf.st_mode))
        {
            tmp_name = ent->d_name;
            while(*(++tmp_name) != '\0');
            if(strncmp(tmp_name - 4, ".zip", 4) == 0)
            {
                memset(path, 0, MAX_PATH_LEN);
                length = strlen(app_plugin_installed_path()) + strlen(ent->d_name) + 20;
                if(length >= MAX_PATH_LEN)
                {
                    PLUGIN_ERR("%s path length exceeds the maximnum(%d) limit\n", ent->d_name, MAX_PATH_LEN);
                    goto err;
                }
                snprintf(path, MAX_PATH_LEN, "zip://file://%s/%s", app_plugin_installed_path(), ent->d_name);
                node = plugin_info_parse(path);
            }
        }
        else if(S_ISDIR(buf.st_mode))
        {
            dir_child = opendir(path);
            if(dir_child == NULL)
                continue;
            while (NULL != (ent_child = readdir(dir_child)))
            {
                if(strcmp(ent_child->d_name, ".") == 0 || strcmp(ent_child->d_name, "..") == 0)
                    continue;

                memset(path, 0, MAX_PATH_LEN);
                snprintf(path, MAX_PATH_LEN, "%s/%s", app_plugin_installed_path(), ent->d_name);
                if(strcmp(ent_child->d_name, "plugin.json") == 0)
                    node = plugin_info_parse(path);
            }
            closedir(dir_child);
        }
        if(node != NULL)
        {
            node->folder = GxCore_Strdup(ent->d_name);
            if(strcmp(node->id, STR_PLUGIN_MANAGE_ID) == 0)
            {
                node_plugin = node;
                continue;
            }
            i++;
            node->counter = i;
            list_add_tail(&(node->list), &((*head)->list));
        }
    }
    closedir(dir);
#if PLUGINONLINE_SUPPORT
    if(node_plugin != NULL)
    {
        i++;
        node_plugin->counter = i;
        list_add(&(node_plugin->list), &((*head)->list));
    }
#endif
    (*head)->counter = i; //save total node number
    APP_FREE(path);
    return 0;
err:
    closedir(dir);
    APP_FREE(path);
    return -1;
}

#if PLUGINONLINE_SUPPORT
static int _plugin_install_list_get(PLuginClassificationList *head)
{
    int index = 0;
    PluginInfoList *pl_head = NULL;
    PluginInfoNode *pos = NULL;
    PluginClassificationNode *classification_node = NULL;
    PluginUpdateNode *update_node = NULL;
    char *classification_name = CLASSIFICATION_INSTALL_NAME;

    if(NULL == head)
        return -1;

    classification_node = (PluginClassificationNode *)GxCore_Calloc(1, sizeof(PluginClassificationNode));
    if(classification_node == NULL)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }
    head->counter++;
    classification_node->name = GxCore_Strdup(classification_name);
    list_add_tail(&(classification_node->list), &(head->list));

    classification_node->update_list_head = (PluginUpdateList *)GxCore_Calloc(1, sizeof(PluginUpdateList));
    if(classification_node->update_list_head == NULL)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        goto err;
    }
    INIT_DLIST_HEAD(&(classification_node->update_list_head->list));

    pl_head = Pluginstore_focus_list_get();
    list_for_each_entry(pos, &(pl_head->list), list)
    {
        if(strcmp(pos->id, STR_PLUGIN_MANAGE_ID) == 0)
            continue;

        update_node = (PluginUpdateNode *)GxCore_Calloc(1, sizeof(PluginUpdateNode));
        if(update_node == NULL)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            goto err;
        }
        update_node->id = GxCore_Strdup(pos->id);
        update_node->name = GxCore_Strdup(pos->title);
        update_node->ins_ver = GxCore_Strdup(pos->version);
        update_node->status = STR_INSTALLED;
        update_node->icon = GxCore_Strdup(pos->icon_url);
        update_node->shortdesc = GxCore_Strdup(pos->synopsis);
        update_node->url = NULL;
        index++;
        update_node->counter = index;
        list_add_tail(&(update_node->list), &(classification_node->update_list_head->list));
    }
    classification_node->update_list_head->counter = index;
    return 0;
err:
    _plugin_classification_list_free(head);
    return -1;
}

char *_plugin_list_download_url_parse(void)
{
    json_error_t error;
    json_t *object = NULL;
    json_t *obj_value = NULL;
    json_t *array_value = NULL;
    char *path = NULL;
    char *url = NULL;
    char *ret_url = NULL;
    const char *obj_key = NULL;
    uint32_t length = 0;
    uint32_t url_length = 0;
    int i = 0;
    int status = 0;

    length = strlen(PLUGIN_SETTING_FILE_PATH) + 20;
    if(length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return NULL;
    }

    path = GxCore_Calloc(length, sizeof(char));
    if(NULL == path)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return NULL;
    }

    snprintf(path, length, "%s/%s", PLUGIN_SETTING_FILE_PATH, "plugin.cfg");
    object = json_load_file(path, JSON_DISABLE_EOF_CHECK, &error);
    if(NULL == object)
    {
        APP_FREE(path);
        return NULL;
    }

    if(!json_is_array(object))
        goto err;

    url = GxCore_Calloc(MAX_ARRAY_LEN, sizeof(char));
    if(NULL == url)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        goto err;
    }

    while(1)
    {
        array_value = json_array_get(object, i);
        i++;
        if(array_value == NULL)
            break;

        if(!json_is_object(array_value))
            goto err;

        json_object_foreach(array_value, obj_key, obj_value)
        {
            if(strcmp(obj_key,"status") == 0)
            {
                if(strcmp(json_string_value(obj_value), "enable") == 0)
                    status = 1;
            }
            if(strcmp(obj_key, "url") == 0)
            {
                memset(url, 0, MAX_ARRAY_LEN);
                url_length = strlen(json_string_value(obj_value)) + 50;
                if(url_length >= MAX_ARRAY_LEN)
                {
                    PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_ARRAY_LEN);
                    goto err;
                }

                if(strncmp("http", json_string_value(obj_value), 4) == 0)
                    snprintf(url, MAX_ARRAY_LEN, "%s/plugin_list.json.zip", json_string_value(obj_value));
                else
                    snprintf(url, MAX_ARRAY_LEN, "http://%s/plugin_list.json.zip", json_string_value(obj_value));
            }
        }
    }

    if(status)
        ret_url = GxCore_Strdup(url);
    else
        ret_url = GxCore_Strdup(PLUGIN_LIST_JSON_FILE_URL);

    json_decref(object);
    APP_FREE(path);
    APP_FREE(url);
    return ret_url;
err:
    json_decref(object);
    APP_FREE(path);
    APP_FREE(url);
    return NULL;
}

static int _plugin_list_download_from_server(PLuginClassificationList **head)
{
    char *url = NULL;
    char *path = NULL;
    char *zip_src = NULL;
    uint32_t length = 0;
    uint32_t zip_src_length = 0;

    url = _plugin_list_download_url_parse();
    if(url == NULL)
        return -1;

    PLUGIN_INFO("plugin server:%s", url);
    if(_plugin_download_start(url, PLUGIN_LIST_JSON_FILE_NAME, 0) < 0)
    {
        APP_FREE(url);
        return -1;
    }
    APP_FREE(url);

    length = strlen(TMP_DOWNLOAD_PATH) + strlen(PLUGIN_LIST_JSON_FILE_NAME) + 20;
    zip_src_length = strlen(TMP_DOWNLOAD_PATH) + 2 * strlen(PLUGIN_LIST_JSON_FILE_NAME) + 20;
    if(length >= MAX_PATH_LEN || zip_src_length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return -1;
    }

    path = GxCore_Calloc(length, sizeof(char));
    if(NULL == path)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }

    zip_src = GxCore_Calloc(zip_src_length, sizeof(char));
    if(NULL == zip_src)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        goto err;
    }

    snprintf(zip_src, zip_src_length, "zip://file://%s/%s.zip/%s", TMP_DOWNLOAD_PATH, PLUGIN_LIST_JSON_FILE_NAME, PLUGIN_LIST_JSON_FILE_NAME);
    snprintf(path, length, "%s/%s.zip", TMP_DOWNLOAD_PATH, PLUGIN_LIST_JSON_FILE_NAME);
    if(Plugin_zip_decompression(zip_src, path) < 0)
        goto err;

    plugin_need_update_counter = 0;
    plugin_list_json_parse(path, *head);

    APP_FREE(path);
    APP_FREE(zip_src);
    return 0;
err:
    APP_FREE(path);
    APP_FREE(zip_src);
    return -1;
}

void Plugin_classification_list_get(PLuginClassificationList **head)
{
    if(*head == NULL)
    {
        *head = (PLuginClassificationList *)GxCore_Calloc(1, sizeof(PLuginClassificationList));
        if(*head == NULL)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            return;
        }
        INIT_DLIST_HEAD(&((*head)->list));
    }

    if(-1 == _plugin_install_list_get(*head))
        return;

    _plugin_list_download_from_server(head);
}

static void _plugin_classification_node_content_free(PluginUpdateNode *node)
{
    if(node != NULL)
    {
        APP_FREE(node->id);
        APP_FREE(node->name);
        APP_FREE(node->ins_ver);
        APP_FREE(node->repo_ver);
        APP_FREE(node->icon);
        APP_FREE(node->shortdesc);
        APP_FREE(node->url);
    }
    return;
}

static void _plugin_classification_list_free(PLuginClassificationList *head)
{
    PluginUpdateNode *pos2 = NULL;
    PluginUpdateNode *pos_back2 = NULL;
    PluginClassificationNode *pos1 = NULL;
    PluginClassificationNode *pos_back1 = NULL;

    if(head == NULL)
        return;

    list_for_each_entry_safe(pos1, pos_back1, &(head->list), list)
    {
        if(pos1->update_list_head != NULL)
        {
            list_for_each_entry_safe(pos2, pos_back2, &((pos1->update_list_head)->list), list)
            {
                _plugin_classification_node_content_free(pos2);
                list_del(&(pos2->list));
                APP_FREE(pos2);
            }
            APP_FREE(pos1->update_list_head);
        }
        APP_FREE(pos1->name);
        list_del(&(pos1->list));
        APP_FREE(pos1);
    }

    head->counter = 0;
    return;
}


void Plugin_classification_list_free(PLuginClassificationList **head)
{
    _plugin_classification_list_free(*head);
    APP_FREE(*head);
    return;
}

void Plugin_installed_check(PluginClassificationNode *node)

{
    PluginUpdateNode *pos;
    PluginUpdateNode *pos_back;

    if(strncmp(node->name, CLASSIFICATION_INSTALL_NAME, 9) != 0)
        return;

    list_for_each_entry_safe(pos, pos_back, &((node->update_list_head)->list), list)
    {
        if(pos->ins_ver == NULL)
        {
            APP_FREE(pos->id);
            APP_FREE(pos->name);
            APP_FREE(pos->repo_ver);
            APP_FREE(pos->icon);
            APP_FREE(pos->shortdesc);
            APP_FREE(pos->url);

            if(pos->link != NULL)
                pos->link->link = NULL;

            list_del(&(pos->list));
            APP_FREE(pos);
            node->update_list_head->counter--;
        }
    }
}

static int _version_parse(char *version, struct VersionClass *data)
{
    char major[10] = {0};
    char minor[10] = {0};
    char modified[10] = {0};
    int i = 0;
    char *string;

    string = version;
    while(1)
    {
        major[i] = *string++;
        if(*string == '.')
            break;
        i++;
        if(i > 10)
            return -1;
    }
    i = 0;
    string++;
    while(1)
    {
        minor[i] = *string++;
        if(*string == '.')
            break;
        i++;
        if(i > 10)
            return -1;
    }
    i = 0;
    string++;
    while(1)
    {
        modified[i] = *string++;
        if(*string == '\0')
            break;
        i++;
        if(i > 10)
            return -1;
    }

    data->major = atoi(major);
    data->minor = atoi(minor);
    data->modified = atoi(modified);
    PLUGIN_INFO("version:%d.%d.%d", data->major, data->minor, data->modified);
    return 0;
}

int Plugin_update_check(PluginUpdateNode *node)
{
    struct VersionClass ins_data;
    struct VersionClass repo_data;

    if(node->ins_ver == NULL || node->repo_ver == NULL)
        return -1;

    if(_version_parse(node->ins_ver, &ins_data) < 0)
        return -1;
    if(_version_parse(node->repo_ver, &repo_data) < 0)
        return -1;

    if(repo_data.major == ins_data.major)
    {
        if(repo_data.minor == ins_data.minor)
        {
            if(repo_data.modified == ins_data.modified)
                return -1;
            else if(repo_data.modified > ins_data.modified)
                return 0;
            else
                return -1;
        }
        else if(repo_data.minor > ins_data.minor)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else if(repo_data.major > ins_data.major)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int Plugin_download_update(PluginUpdateNode *node)
{
    char *path = NULL;
    uint32_t length = 0;
    PluginInfoList *pl_head = NULL;
    PluginInfoNode *pl_node = NULL;
    PluginInfoNode *pos = NULL;
    PluginInfoNode *pos_back = NULL;

    path = GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == path)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }

    pl_head = Pluginstore_focus_list_get();
    list_for_each_entry_safe(pos, pos_back, &((pl_head)->list), list)
    {
        memset(path, 0, MAX_PATH_LEN);
        if(strcmp(node->id, pos->id) == 0)
        {
            PLUGIN_INFO("download soft:%s", node->url);
            if(_plugin_download_start(node->url, pos->id, 1) < 0)
                goto err;

            length = strlen(app_plugin_installed_path()) + strlen(pos->id) + 50;
            if(length >= MAX_PATH_LEN)
            {
                PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
                goto err;
            }

            snprintf(path, length, "zip://file://%s/%s.zip", app_plugin_installed_path(), pos->id);
            pl_node = plugin_info_parse(path);
            if(NULL == pl_node)
                goto err;

            pl_node->counter = pos->counter;
            pl_node->folder = GxCore_Strdup(pos->folder);
            list_add_tail(&(pl_node->list), &(pos->list));

            APP_FREE(pos->id);
            APP_FREE(pos->version);
            APP_FREE(pos->title);
            APP_FREE(pos->folder);
            APP_FREE(pos->ui_type);
            APP_FREE(pos->setting);
            APP_FREE(pos->synopsis);
            APP_FREE(pos->icon_url);

            list_del(&(pos->list));
            APP_FREE(pos);
            APP_FREE(node->ins_ver);
            node->ins_ver = GxCore_Strdup(pl_node->version);
            node->status = STR_INSTALLED;
            if(node->link != NULL)
            {
                APP_FREE(node->link->ins_ver);
                node->link->ins_ver = GxCore_Strdup(pl_node->version);
                node->link->status = STR_INSTALLED;
            }
            plugin_need_update_counter--;
            APP_FREE(path);
            return 0;
        }
    }

err:
    APP_FREE(path);
    return -1;
}

int Plugin_download_install(PluginUpdateNode *node, PluginClassificationNode *classification_node)
{
    char *path = NULL;
    uint32_t length = 0;
    PluginInfoList *pl_head = NULL;
    PluginInfoNode *pl_node = NULL;
    PluginUpdateNode *update_node = NULL;
    PluginClassificationNode *installed_node = NULL;
    PluginClassificationNode *tmp = NULL;

    if(NULL == node || NULL == node->url || NULL == classification_node)
        return -1;
    PLUGIN_INFO("download soft:%s", node->url);
    if(_plugin_download_start(node->url, node->id, 1) < 0)
        return -1;
    pl_head = Pluginstore_focus_list_get();

    path = GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == path)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }

    length = strlen(app_plugin_installed_path()) + strlen(node->id) + 50;
    if(length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        goto err;
    }

    snprintf(path, MAX_PATH_LEN, "zip://file://%s/%s.zip", app_plugin_installed_path(), node->id);
    pl_node = plugin_info_parse(path);
    if(pl_node != NULL)
    {
        length = strlen(node->id) + 50;
        if(length >= MAX_PATH_LEN)
        {
            PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
            APP_FREE(pl_node);
            goto err;
        }

        snprintf(path, MAX_PATH_LEN, "%s.zip", node->id);
        pl_node->folder = path;
        pl_node->counter = pl_head->counter;
        list_add_tail(&(pl_node->list), &((pl_head)->list));
        pl_head->counter++;
        node->ins_ver = GxCore_Strdup(pl_node->version);
        node->status = STR_INSTALLED;
        if(node->link != NULL)
        {
            node->link->status = STR_INSTALLED;
            APP_FREE(node->link->ins_ver);
            node->link->ins_ver = GxCore_Strdup(node->ins_ver);
        }
        if(strncmp(classification_node->name, CLASSIFICATION_INSTALL_NAME, 9) != 0)
        {
            update_node = (PluginUpdateNode *)GxCore_Calloc(1, sizeof(PluginUpdateNode));
            if(update_node == NULL)
            {
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                return -1;
            }
            update_node->id = GxCore_Strdup(pl_node->id);
            update_node->name = GxCore_Strdup(pl_node->title);
            update_node->ins_ver = GxCore_Strdup(pl_node->version);
            update_node->status = STR_INSTALLED;
            update_node->icon = GxCore_Strdup(pl_node->icon_url);
            update_node->shortdesc = GxCore_Strdup(pl_node->synopsis);
            update_node->url = GxCore_Strdup(node->url);
            update_node->link = node;
            node->link = update_node;
            tmp = classification_node;
            do{
                installed_node = list_entry(tmp->list.prev, typeof(PluginClassificationNode), list);
                tmp = installed_node;
            }while(strncmp(installed_node->name, CLASSIFICATION_INSTALL_NAME, 9) != 0);
            (installed_node->update_list_head->counter)++;
            update_node->counter = installed_node->update_list_head->counter;
            list_add_tail(&(update_node->list), &(installed_node->update_list_head->list));
        }
        PLUGIN_INFO("ok");
        return 0;
    }

err:
    APP_FREE(path);
    return -1;
}

int Plugin_download_uninstall(PluginUpdateNode *node, PluginClassificationNode *classification_node)
{
    PluginInfoList *head = NULL;
    PluginInfoNode *pos1 = NULL;
    PluginInfoNode *pos_back1 = NULL;
    PluginUpdateNode *pos2 = NULL;
    PluginUpdateNode *pos_back2 = NULL;
    PluginClassificationNode *tmp = NULL;
    PluginClassificationNode *installed_node = NULL;
    char *path = NULL;
    uint32_t length = 0;

    head = Pluginstore_focus_list_get();
    if(NULL == head || NULL == node || NULL == classification_node)
        return -1;

    list_for_each_entry_safe(pos1, pos_back1, &((head)->list), list)
    {
        if(strcmp(node->id, pos1->id) == 0)
        {
            length = strlen(app_plugin_installed_path()) + strlen(pos1->folder) + 20;
            if(length >= MAX_PATH_LEN)
            {
                PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
                return -1;
            }

            path = GxCore_Calloc(length, sizeof(char));
            if(NULL == path)
            {
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                return -1;
            }

            snprintf(path, length, "%s/%s", app_plugin_installed_path(), pos1->folder);
            if(GXCORE_FILE_EXIST == GxCore_FileExists(path))
                GxCore_FileDelete(path);
            else
                PLUGIN_ERR("plugin file not exit\n");

            list_del(&(pos1->list));
            APP_FREE(pos1);
            head->counter--;

            APP_FREE(node->ins_ver);

            if(strcmp(node->status, STR_ID_UPGRADE) == 0)
                plugin_need_update_counter--;

            node->status = STR_ID_NOT_INSTALLED;

            if(node->link != NULL)
            {
                node->link->status = STR_ID_NOT_INSTALLED;
                APP_FREE(node->link->ins_ver);
            }

            if(strncmp(classification_node->name, CLASSIFICATION_INSTALL_NAME, 9) != 0)
            {
                tmp = classification_node;
                do{
                    installed_node = list_entry(tmp->list.prev, typeof(PluginClassificationNode), list);
                    tmp = installed_node;
                }while(strncmp(installed_node->name, CLASSIFICATION_INSTALL_NAME, 9) != 0);

                list_for_each_entry_safe(pos2, pos_back2, &((installed_node->update_list_head)->list), list)
                {
                    if(strcmp(node->id, pos2->id) == 0)
                    {
                        APP_FREE(pos2->id);
                        APP_FREE(pos2->name);
                        APP_FREE(pos2->ins_ver);
                        APP_FREE(pos2->repo_ver);
                        APP_FREE(pos2->icon);
                        APP_FREE(pos2->shortdesc);
                        APP_FREE(pos2->url);

                        if(pos2->link != NULL)
                            pos2->link->link = NULL;

                        list_del(&(pos2->list));
                        APP_FREE(pos2);
                        installed_node->update_list_head->counter--;
                        break;
                    }
                }
            }
            PLUGIN_INFO("plugin uninstall ok");
            APP_FREE(path);
            return 0;
        }
    }
    return -1;
}

int Plugin_need_update_counter_get(void)
{
    return plugin_need_update_counter;
}
#endif
#endif
