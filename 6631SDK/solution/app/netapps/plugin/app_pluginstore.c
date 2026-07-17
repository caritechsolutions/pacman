/*************************************************************************
	> File Name: app_pluginstore.c
	> Author:
	> Mail:
	> Created Time: 2016年08月26日 星期五 14时19分54秒
 ************************************************************************/

#include "app.h"
#include "app_windows.h"
#if PLUGINSTORE_SUPPORT
#include "gx_pluginstore_api.h"
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "app_wnd_system_setting_opt.h"
#include "app_plugin_update.h"
#include "jansson.h"
#include "app_keyboard_language.h"

#define WND_PLUGIN_SYNOPSIS          "text_pluginstore_synopsis"
#define WND_PLUGINSTORE_PAGE_INFO    "text_pluginstore_page_info"
#define WND_PLUGINSTORE_IMAGE_RED    "img_pluginstore_red"
#define WND_PLUGINSTORE_TEXT_RED     "text_pluginstore_red"
#define MAX_PAGE_ITEM                (10)
#define MAX_PAGE_LINE                (2)
#define MAX_FILE_NAME_LEN            (128)
#define MAX_CFG_GROUP                (3)
#define MAX_CFG_ITEM                 (4)
#define UI_TYPE_LISTVIEW             "listview"

static int _plugin_main_menu_info_draw(int mode);
static void plugin_draw_plugin_store_pic_timer_delete(void);
#if PLUGINONLINE_SUPPORT
extern void Plugin_update_timer_start(void);
extern void Plugin_update_timer_delete(void);
extern status_t app_create_plugin_manage_menu(void);
#endif
extern status_t app_create_plugin_source_menu(void);
extern status_t app_create_plugin_listview_play_menu(void);
void app_pluginstore_exit(void);

struct PluginFocusInfo{
    int total_number;
    int focus_item;
    int cfg_file_check_status;
    PluginInfoNode *node;
    PluginInfoList *head;
};

struct PluginSettingData{
    char *name;
    char *value;
};

static struct PluginSettingData cfg_data[MAX_CFG_GROUP][MAX_CFG_ITEM];
static struct PluginSettingData cfg_data_temp[MAX_CFG_GROUP][MAX_CFG_ITEM];
static struct PluginFocusInfo this_focus_info= {0, 0, 0, NULL, NULL};
#if PLUGINONLINE_SUPPORT
static event_list* s_draw_plugin_store_pic_timer = NULL;
#endif
extern char *get_fname_suffix_from_url(char *url);
static void plugin_draw_plugin_store_pic_timer_start(void);
static void plugin_draw_plugin_store_pic_timer_stop(void);
static SystemSettingOpt  plugin_setting_opt;
static SystemSettingItem *plugin_setting_item = NULL;
static int cfg_sel_mode = 0;
static int cfg_sel_mode_base = 0;
static bool thiz_cfg_format_newer = false;

static char *s_text_plugin_title[MAX_PAGE_ITEM] =
{
    "text_pluginstore_item_title1",
    "text_pluginstore_item_title2",
    "text_pluginstore_item_title3",
    "text_pluginstore_item_title4",
    "text_pluginstore_item_title5",
    "text_pluginstore_item_title6",
    "text_pluginstore_item_title7",
    "text_pluginstore_item_title8",
    "text_pluginstore_item_title9",
    "text_pluginstore_item_title10",
};

static char *s_img_plugin_focus[MAX_PAGE_ITEM] =
{
    "img_pluginstore_item_focus1",
    "img_pluginstore_item_focus2",
    "img_pluginstore_item_focus3",
    "img_pluginstore_item_focus4",
    "img_pluginstore_item_focus5",
    "img_pluginstore_item_focus6",
    "img_pluginstore_item_focus7",
    "img_pluginstore_item_focus8",
    "img_pluginstore_item_focus9",
    "img_pluginstore_item_focus10",
};

static char *s_img_plugin_unfocus[MAX_PAGE_ITEM] =
{
    "img_pluginstore_item_unfocus1",
    "img_pluginstore_item_unfocus2",
    "img_pluginstore_item_unfocus3",
    "img_pluginstore_item_unfocus4",
    "img_pluginstore_item_unfocus5",
    "img_pluginstore_item_unfocus6",
    "img_pluginstore_item_unfocus7",
    "img_pluginstore_item_unfocus8",
    "img_pluginstore_item_unfocus9",
    "img_pluginstore_item_unfocus10",
};

static char *key_img[MAX_PAGE_ITEM]=
{
    "pluginstore_img_key1",
    "pluginstore_img_key2",
    "pluginstore_img_key3",
    "pluginstore_img_key4",
    "pluginstore_img_key5",
    "pluginstore_img_key6",
    "pluginstore_img_key7",
    "pluginstore_img_key8",
    "pluginstore_img_key9",
    "pluginstore_img_key10",
};

PluginInfoNode *Pluginstore_focus_node_get(void)
{
    return this_focus_info.node;
}

PluginInfoList *Pluginstore_focus_list_get(void)
{
    return this_focus_info.head;
}

static int plugin_setting_get(void)
{
    char file[MAX_FILE_NAME_LEN] = {0};
    json_error_t error;
    json_t *object = NULL;
    const char *obj_key = NULL;
    json_t *obj_value = NULL;
    json_t *array_value = NULL;
    int i = 0, j = 0;

    memset(cfg_data, 0, sizeof(sizeof(struct PluginSettingData) * MAX_CFG_ITEM * MAX_CFG_GROUP));
    memset(cfg_data_temp, 0, sizeof(sizeof(struct PluginSettingData) * MAX_CFG_ITEM * MAX_CFG_GROUP));
    snprintf(file, MAX_FILE_NAME_LEN, "%s/%s.cfg", PLUGIN_SETTING_FILE_PATH, this_focus_info.node->id);
    if(GXCORE_FILE_EXIST == GxCore_FileExists(file))
    {
        object = json_load_file(file, JSON_DISABLE_EOF_CHECK, &error);
        if(NULL == object)
            return -1;

        if(!json_is_array(object))
        {
            json_decref(object);
            return -1;
        }

        while(1)
        {
            j = 0;
            array_value = json_array_get(object, i);
            if(array_value == NULL)
                break;

            if(!json_is_object(array_value))
            {
                json_decref(object);
                return -1;
            }
            json_object_foreach(array_value, obj_key, obj_value)
            {
                if(strcmp(obj_key,"status") == 0)
                {
                    if(strcmp(json_string_value(obj_value), "enable") == 0)
                        cfg_sel_mode = cfg_sel_mode_base = i;
                    continue;
                }
                if(json_is_object(obj_value))// new format
                {
                    int index = 0;

                    if(NULL == json_object_get(obj_value, "index")
                            || NULL == json_object_get(obj_value, "value"))
                    {
                        PLUGIN_ERR("the format of config file is error\n");
                        json_decref(object);
                        return -1;
                    }

                    index = json_integer_value(json_object_get(obj_value, "index"));
                    if(index >= MAX_CFG_ITEM)
                    {
                        PLUGIN_ERR("The max number of configuration items supported are %d\n", MAX_CFG_ITEM);
                        json_decref(object);
                        return -1;
                    }

                    thiz_cfg_format_newer = true;

                    APP_FREE(cfg_data[i][index].name);
                    cfg_data[i][index].name = GxCore_Strdup(obj_key);

                    APP_FREE(cfg_data_temp[i][index].name);
                    cfg_data_temp[i][index].name = GxCore_Strdup(obj_key);

                    APP_FREE(cfg_data[i][index].value);
                    cfg_data[i][index].value = GxCore_Strdup(json_string_value(json_object_get(obj_value, "value")));

                    APP_FREE(cfg_data_temp[i][index].value);
                    cfg_data_temp[i][index].value = GxCore_Strdup(json_string_value(json_object_get(obj_value, "value")));
                }
                else //compatible with old format
                {
                    thiz_cfg_format_newer = false;

                    cfg_data[i][j].name = GxCore_Strdup(obj_key);
                    cfg_data_temp[i][j].name = GxCore_Strdup(obj_key);
                    if(json_string_value(obj_value) != NULL)
                    {
                        cfg_data[i][j].value = GxCore_Strdup(json_string_value(obj_value));
                        cfg_data_temp[i][j].value = GxCore_Strdup(json_string_value(obj_value));
                    }
                }
                j++;
                if(j >= MAX_CFG_ITEM)
                    break;
            }
            i++;
            if(i >= MAX_CFG_GROUP)
                break;
        }
    }
    else
    {
        object = json_loads(this_focus_info.node->setting, JSON_DISABLE_EOF_CHECK, &error);
        if(json_is_object(object))
        {
            json_object_foreach(object, obj_key, obj_value)
            {
                if(json_is_object(obj_value))// new format
                {
                    int index = 0;

                    if(NULL == json_object_get(obj_value, "index")
                            || NULL == json_object_get(obj_value, "value"))
                    {
                        PLUGIN_ERR("the format of config file is error\n");
                        json_decref(object);
                        return -1;
                    }

                    index = json_integer_value(json_object_get(obj_value, "index"));
                    if(index >= MAX_CFG_ITEM)
                    {
                        PLUGIN_ERR("The max number of configuration items supported are %d\n", MAX_CFG_ITEM);
                        json_decref(object);
                        return -1;
                    }

                    thiz_cfg_format_newer = true;

                    cfg_data[0][index].name = GxCore_Strdup(obj_key);
                    cfg_data_temp[0][index].name = GxCore_Strdup(obj_key);
                    if(json_string_value(json_object_get(obj_value, "value")) != NULL
                            && json_string_value(json_object_get(obj_value, "value"))[0] != '\0')
                    {
                        cfg_data[0][index].value = GxCore_Strdup(json_string_value(json_object_get(obj_value, "value")));
                        cfg_data_temp[0][index].value = GxCore_Strdup(json_string_value(json_object_get(obj_value, "value")));
                    }
                    else
                    {
                        cfg_data[0][j].value = GxCore_Strdup("null");
                        cfg_data_temp[0][j].value = GxCore_Strdup("null");
                    }
                }
                else
                {
                    thiz_cfg_format_newer = false;

                    cfg_data[0][j].name = GxCore_Strdup(obj_key);
                    cfg_data_temp[0][j].name = GxCore_Strdup(obj_key);
                    if(json_string_value(obj_value) != NULL && json_string_value(obj_value)[0] != '\0')
                    {
                        cfg_data[0][j].value = GxCore_Strdup(json_string_value(obj_value));
                        cfg_data_temp[0][j].value = GxCore_Strdup(json_string_value(obj_value));
                    }
                    else
                    {
                        cfg_data[0][j].value = GxCore_Strdup("null");
                        cfg_data_temp[0][j].value = GxCore_Strdup("null");
                    }
                }

                j++;
                if(j >= MAX_CFG_ITEM)
                    break;
            }
        }
        else
        {
            json_decref(object);
            return -1;
        }

        for(i = 1; i < MAX_CFG_GROUP; i++)
        {
            for(j = 0; j < MAX_CFG_ITEM; j++)
            {
                if(cfg_data_temp[0][j].name == NULL)
                    break;
                cfg_data[i][j].name = GxCore_Strdup(cfg_data_temp[0][j].name);
                cfg_data_temp[i][j].name = GxCore_Strdup(cfg_data_temp[0][j].name);
                cfg_data[i][j].value = GxCore_Strdup(cfg_data_temp[0][j].value);
                cfg_data_temp[i][j].value = GxCore_Strdup(cfg_data_temp[0][j].value);
            }
        }
    }
    json_decref(object);
    return j;
}

static int app_plugin_mode_change_callback(int sel)
{
    int i;

    if(cfg_sel_mode == sel)
        return EVENT_TRANSFER_KEEPON;

    for(i = 0; i < MAX_CFG_ITEM; i++)
    {
        if(cfg_data[cfg_sel_mode][i].name == NULL)
            break;

        if(strcmp(cfg_data[cfg_sel_mode][i].value, cfg_data_temp[cfg_sel_mode][i].value) != 0)
        {
            APP_FREE(cfg_data_temp[cfg_sel_mode][i].value);
            cfg_data_temp[cfg_sel_mode][i].value = GxCore_Strdup(cfg_data[cfg_sel_mode][i].value);
        }
    }

    cfg_sel_mode = sel;
    for(i = 1; i <= MAX_CFG_ITEM; i++)
    {
        if(cfg_data_temp[cfg_sel_mode][i-1].name == NULL)
            break;
        plugin_setting_item[i].itemProperty.itemPropertyEdit.string = cfg_data_temp[cfg_sel_mode][i-1].value;
        app_system_set_item_property(i, &plugin_setting_item[i].itemProperty);
    }
    return EVENT_TRANSFER_KEEPON;
}

static void app_plugin_keyboard_proc(PopKeyboard *data)
{
    int sel;

    sel = app_system_get_sel();
    if(POP_VAL_CANCEL == data->in_ret || NULL == data->out_name)
        return;

    APP_FREE(cfg_data_temp[cfg_sel_mode][sel-1].value);
    cfg_data_temp[cfg_sel_mode][sel-1].value = GxCore_Strdup(data->out_name);
    plugin_setting_item[sel].itemProperty.itemPropertyBtn.string = cfg_data_temp[cfg_sel_mode][sel-1].value;
    app_system_set_item_property(sel, &plugin_setting_item[sel].itemProperty);
}

static int app_plugin_item_btn_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    static PopKeyboard keyboard;
    int sel;

    sel = app_system_get_sel();
    if(key == STBK_OK)
    {
        memset(&keyboard, 0, sizeof(PopKeyboard));
        keyboard.in_name    = cfg_data_temp[cfg_sel_mode][sel-1].value;
        keyboard.max_num = 120;
        keyboard.out_name   = NULL;
        keyboard.change_cb  = NULL;
        keyboard.release_cb = app_plugin_keyboard_proc;
        keyboard.usr_data = NETWORK_CHAR_NAME_INVALID;
        keyboard.pos.x = 500;

        multi_language_keyboard_create(&keyboard);
        ret = EVENT_TRANSFER_STOP;
    }
    return ret;
}

static bool app_plugin_cfg_change(void)
{
    int i;
    int ret = false;

    for(i = 0; i < MAX_CFG_ITEM; i++)
    {
        if(NULL == cfg_data_temp[cfg_sel_mode][i].name
                && NULL == cfg_data_temp[cfg_sel_mode][i].value)
            continue;

        if(NULL == cfg_data_temp[cfg_sel_mode][i].name
                || 0 == memcmp(cfg_data_temp[cfg_sel_mode][i].value, "null", 4))
            return false;
    }

    for(i = 0; i < MAX_CFG_ITEM; i++)
    {
        if(NULL == cfg_data_temp[cfg_sel_mode][i].name
                && NULL == cfg_data_temp[cfg_sel_mode][i].value)
            continue;

        if(strcmp(cfg_data[cfg_sel_mode][i].value, cfg_data_temp[cfg_sel_mode][i].value) != 0)
            ret = true;
    }

    if(false == ret && cfg_sel_mode != cfg_sel_mode_base)
        ret = true;

    return ret;
}

void app_plugin_cfg_save(void)
{
    int i,j;
    int ret;
    json_t *array = NULL;
    char file[MAX_FILE_NAME_LEN] = {0};
    json_t *object[MAX_CFG_GROUP] = {NULL};
    json_t *temp_object[MAX_CFG_GROUP][MAX_CFG_ITEM] = {{NULL}, {NULL}, {NULL}};

    if(GxCore_FileExists(PLUGIN_SETTING_FILE_PATH) == GXCORE_FILE_UNEXIST)
    {
        if(GxCore_Mkdir(PLUGIN_SETTING_FILE_PATH) < 0)
        {
            PLUGIN_ERR("creat plugin cfg error\n");
            return;
        }
    }
    array = json_array();
    cfg_sel_mode_base = cfg_sel_mode;
    for(i = 0; i < MAX_CFG_GROUP; i++)
    {
        object[i] = json_object();
        for(j = 0; j < MAX_CFG_ITEM; j++)
        {
            if(cfg_data_temp[i][j].name == NULL)
                break;

            if(strcmp(cfg_data[i][j].value, cfg_data_temp[i][j].value) != 0)
            {
                APP_FREE(cfg_data[i][j].value);
                cfg_data[i][j].value = GxCore_Strdup(cfg_data_temp[i][j].value);
            }
            if(thiz_cfg_format_newer)
            {
                temp_object[i][j] = json_object();
                json_object_set_new(temp_object[i][j], "value", json_string(cfg_data_temp[i][j].value));
                json_object_set_new(temp_object[i][j], "index", json_integer(j));
                json_object_set_new(object[i], cfg_data_temp[i][j].name, temp_object[i][j]);
            }
            else
                json_object_set_new(object[i], cfg_data_temp[i][j].name, json_string(cfg_data_temp[i][j].value));
        }
        if(cfg_sel_mode == i)
            json_object_set_new(object[i], "status", json_string("enable"));
        else
            json_object_set_new(object[i], "status", json_string("disable"));
        json_array_append_new(array, object[i]);
    }
    snprintf(file, MAX_FILE_NAME_LEN, "%s/%s.cfg", PLUGIN_SETTING_FILE_PATH, this_focus_info.node->id);
    ret = json_dump_file(array, file, 0);
    PLUGIN_INFO("json dump file %s:%d", file, ret);
    if(ret < 0)
    {
        if(GxCore_FileExists(file) == GXCORE_FILE_EXIST)
            GxCore_FileDelete(file);
    }
    for(i = 0; i < MAX_CFG_GROUP; i++)
    {
        for(j = 0; j < MAX_CFG_ITEM; j++)
            json_decref(temp_object[i][j]);

        json_decref(object[i]);
    }

    json_decref(array);
}

static void _plugin_setting_cfg_data_free(void)
{
    int i,j;

    for(i = 0; i < MAX_CFG_GROUP; i++)
    {
        for(j = 0; j < MAX_CFG_ITEM; j++)
        {
            APP_FREE(cfg_data[i][j].name);
            APP_FREE(cfg_data[i][j].value);
            APP_FREE(cfg_data_temp[i][j].name);
            APP_FREE(cfg_data_temp[i][j].value);
        }
    }
    return;
}

static int _plugin_setting_save_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        app_plugin_cfg_save();
#if PLUGINONLINE_SUPPORT
        if(strcmp(this_focus_info.node->id, "plugin") == 0)
        {
            Plugin_update_timer_delete();
            Plugin_update_timer_start();
        }
#endif
    }

    _plugin_setting_cfg_data_free();
    app_system_reset_unfocus_image();
    APP_FREE(plugin_setting_opt.menuTitle);
    APP_FREE(plugin_setting_item);
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static void plugin_setting_item_init(int max_item)
{
    int i = 0;

    plugin_setting_item[i].itemTitle = "Group";
    plugin_setting_item[i].itemType = ITEM_CHOICE;
    plugin_setting_item[i].itemProperty.itemPropertyBtn.string = "[1,2,3]";
    plugin_setting_item[i].itemProperty.itemPropertyCmb.sel = cfg_sel_mode;
    plugin_setting_item[i].itemCallback.cmbCallback.CmbChange = app_plugin_mode_change_callback;
    plugin_setting_item[i].itemStatus = ITEM_NORMAL;
    for(i = 1; i <= max_item; i++)
    {
        plugin_setting_item[i].itemTitle = cfg_data_temp[cfg_sel_mode][i-1].name;
        plugin_setting_item[i].itemType = ITEM_PUSH;
        plugin_setting_item[i].itemProperty.itemPropertyEdit.string = cfg_data_temp[cfg_sel_mode][i-1].value;
        plugin_setting_item[i].itemCallback.btnCallback.BtnPress = app_plugin_item_btn_callback;
        plugin_setting_item[i].itemStatus = ITEM_NORMAL;
    }
}

static int _plugin_setting_exit_cb(ExitType Type)
{
    int ret = EXIT_RET_DEFAULT;
    if(EXIT_NORMAL == Type)
    {
        ret = EXIT_RET_POP;
        if(true == app_system_set_callback_handler(app_plugin_cfg_change, _plugin_setting_save_cb))
            return ret;
    }
    _plugin_setting_cfg_data_free();
    APP_FREE(plugin_setting_opt.menuTitle);
    APP_FREE(plugin_setting_item);
    return ret;
}

static int _plugin_setting_file_check(void)
{
    char *file = NULL;
    uint32_t length = 0;

    if(this_focus_info.node->setting == NULL)
        return 0;

    length = strlen(PLUGIN_SETTING_FILE_PATH) + strlen(this_focus_info.node->id) + 20;
    if(length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return -1;
    }

    file = GxCore_Calloc(length, sizeof(char));
    if(NULL == file)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }

    snprintf(file, length, "%s/%s.cfg", PLUGIN_SETTING_FILE_PATH, this_focus_info.node->id);
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(file))
    {
        this_focus_info.cfg_file_check_status = 1;
        APP_FREE(file);
        return -1;
    }
    else
    {
        APP_FREE(file);
        return 0;
    }
}

static void _plugin_create_setting_menu(void)
{
    char *name = NULL;
    uint32_t length = 0;
    int max_item = -1;

    plugin_draw_plugin_store_pic_timer_delete();
    max_item = plugin_setting_get();
    if(max_item <= 0)
    {
        PLUGIN_ERR("error\n");
        return;
    }

    length = strlen(this_focus_info.node->title) + 20;
    if(length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return;
    }

    name = GxCore_Calloc(length,sizeof(char));
    if(NULL == name)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return;
    }

    plugin_setting_item = (SystemSettingItem *)GxCore_Malloc(sizeof(SystemSettingItem) * (max_item + 1));
    if(plugin_setting_item == NULL)
    {
        PLUGIN_ERR("No memory to GxCore_Malloc\n");
        return;
    }
    memset(&plugin_setting_opt, 0 ,sizeof(SystemSettingOpt));
    memset(plugin_setting_item, 0 ,sizeof(SystemSettingItem) * (max_item + 1));
    plugin_setting_item_init(max_item);
    snprintf(name, length, "%s Setting", this_focus_info.node->title);
    plugin_setting_opt.menuTitle = name;
    plugin_setting_opt.itemNum = max_item + 1;
    plugin_setting_opt.item = plugin_setting_item;
    plugin_setting_opt.timeDisplay = TIP_HIDE;
    plugin_setting_opt.exit = _plugin_setting_exit_cb;
    app_system_set_create(&plugin_setting_opt);

    return;
}

static void _focus_info_item_init(void)
{
    this_focus_info.focus_item = 0;
    if(this_focus_info.head != NULL && this_focus_info.head->list.next != NULL)
        this_focus_info.node = list_entry(this_focus_info.head->list.next, typeof(*this_focus_info.head), list);
}

static void _pluginstore_update_page_info(void)
{
    char page_info[20] = {0};
    int page, max_page;
    int item,total;

    item = this_focus_info.focus_item;
    total = this_focus_info.total_number;
    page = 1 + (item - (item % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
    max_page = 1 + ((total-1) - ((total-1) % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
    snprintf(page_info, sizeof(page_info), "%d/%d", page, max_page);
    GUI_SetProperty(WND_PLUGINSTORE_PAGE_INFO, "string", page_info);
    GUI_SetProperty(WND_PLUGINSTORE_PAGE_INFO, "state", "show");
}

static void _pluginstore_focus_item(int index)
{
    int i;
    if(index >= 0)
    {
        i = index % MAX_PAGE_ITEM;
        GUI_SetProperty(s_img_plugin_unfocus[i], "state", "show");
        GUI_SetProperty(s_img_plugin_focus[i], "state", "show");
        GUI_SetProperty(s_text_plugin_title[i], "forecolor", "[#E78818,#E78818,#E78818]");
        GUI_SetProperty(WND_PLUGIN_SYNOPSIS, "string", this_focus_info.node->synopsis);
        if(this_focus_info.node->setting == NULL)
        {
            GUI_SetProperty(WND_PLUGINSTORE_IMAGE_RED, "state", "hide");
            GUI_SetProperty(WND_PLUGINSTORE_TEXT_RED, "state", "hide");
        }
        else
        {
            GUI_SetProperty(WND_PLUGINSTORE_IMAGE_RED, "state", "show");
            GUI_SetProperty(WND_PLUGINSTORE_TEXT_RED, "state", "show");
        }

    }
}

static void _pluginstore_unfocus_item(int index)
{
    if(index >= 0)
    {
        GUI_SetProperty(s_img_plugin_focus[index%MAX_PAGE_ITEM], "state", "hide");
        GUI_SetProperty(s_text_plugin_title[index%MAX_PAGE_ITEM], "forecolor", "[text_color,text_color,text_color]");
    }
}

static void _pluginstore_unfocus_all_item(void)
{
    int i;

    for(i = 0; i < MAX_PAGE_ITEM; i++)
        _pluginstore_unfocus_item(i);

    return;
}

static void _pluginstore_up_keypress(void)
{
    PluginInfoNode *pos;
    int i, j;

    i = this_focus_info.focus_item;
    j = 0;
    if(((i%MAX_PAGE_ITEM)-MAX_PAGE_ITEM/MAX_PAGE_LINE) < 0)
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i > this_focus_info.total_number-1)
            return;
        list_from_node(pos, &(this_focus_info.node->list), list, this_focus_info.head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
            return;
    }
    else
    {
        i -= MAX_PAGE_ITEM / MAX_PAGE_LINE;
        list_from_node_reverse(pos, &(this_focus_info.node->list), list, this_focus_info.head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
            return;
    }
    _pluginstore_unfocus_item(this_focus_info.focus_item);
    this_focus_info.focus_item = i;
    this_focus_info.node = pos;
    _pluginstore_focus_item(this_focus_info.focus_item);
    PLUGIN_DBG("%d, id: %s\n", i, this_focus_info.node->id);
}

static void _pluginstore_down_keypress(void)
{
    PluginInfoNode *pos;
    int i, j;

    i = this_focus_info.focus_item;
    j = 0;
    if(((i%MAX_PAGE_ITEM)+MAX_PAGE_ITEM/MAX_PAGE_LINE) < MAX_PAGE_ITEM)
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i > this_focus_info.total_number-1)
            return;
        list_from_node(pos, &(this_focus_info.node->list), list, this_focus_info.head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
            return;
    }
    else
    {
        i -= MAX_PAGE_ITEM / MAX_PAGE_LINE;
        list_from_node_reverse(pos, &(this_focus_info.node->list), list, this_focus_info.head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
            return;
    }
    _pluginstore_unfocus_item(this_focus_info.focus_item);
    this_focus_info.focus_item = i;
    this_focus_info.node = pos;
    _pluginstore_focus_item(this_focus_info.focus_item);
    PLUGIN_DBG("%d, id: %s\n", i, this_focus_info.node->id);
}

static void _pluginstore_left_keypress(void)
{
    PluginInfoNode *pos;
    int i = 0, j = 0;
    int page;

    i = this_focus_info.focus_item;
    if(0 == i%(MAX_PAGE_ITEM/MAX_PAGE_LINE))
    {
        if(this_focus_info.total_number <= MAX_PAGE_ITEM)
        {
            return;
        }
        else
        {
            page = (i - i % MAX_PAGE_ITEM) / MAX_PAGE_ITEM;
            if((page-1) < 0)
                return;
            else
                page -= 1;

            i = page * MAX_PAGE_ITEM;
            list_from_node(pos, &(this_focus_info.head->list), list, this_focus_info.head)
            {
                j++;
                if(j > i)
                    break;
            }
            this_focus_info.node = pos;
            if(page == 0)
                plugin_draw_plugin_store_pic_timer_start();
            else
                plugin_draw_plugin_store_pic_timer_stop();
            _plugin_main_menu_info_draw(0);
        }
    }
    else
    {
        i -= 1;
        this_focus_info.node = list_entry(this_focus_info.node->list.prev, typeof(*this_focus_info.node), list);
    }
    _pluginstore_unfocus_item(this_focus_info.focus_item);
    this_focus_info.focus_item = i;
    _pluginstore_focus_item(this_focus_info.focus_item);
    _pluginstore_update_page_info();
    PLUGIN_DBG("%d, id: %s\n", i, this_focus_info.node->id);
}

static void _pluginstore_right_keypress(void)
{
    PluginInfoNode *pos;
    int i = 0, j = 0;
    int max_page, page;

    i = this_focus_info.focus_item;
    if(0 == (i+1)%(MAX_PAGE_ITEM/MAX_PAGE_LINE) || i == (this_focus_info.total_number - 1))
    {
        if(this_focus_info.total_number <= MAX_PAGE_ITEM)
        {
            return;
        }
        else
        {
            max_page = (this_focus_info.total_number - (this_focus_info.total_number % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
            page = (i - i % MAX_PAGE_ITEM) / MAX_PAGE_ITEM;
            if((page + 1) > max_page)
                page = 0;
            else
                page += 1;

            i = page * MAX_PAGE_ITEM;
            list_from_node(pos, &(this_focus_info.head->list), list, this_focus_info.head)
            {
                j++;
                if(j > i)
                    break;
            }
            this_focus_info.node = pos;
            if(page == 0)
                plugin_draw_plugin_store_pic_timer_start();
            else
                plugin_draw_plugin_store_pic_timer_stop();
            _plugin_main_menu_info_draw(0);
        }
    }
    else
    {
        i += 1;
        if(i > this_focus_info.total_number-1)
            return;
        this_focus_info.node = list_entry(this_focus_info.node->list.next, typeof(*this_focus_info.node), list);
    }
    _pluginstore_unfocus_item(this_focus_info.focus_item);
    this_focus_info.focus_item = i;
    _pluginstore_focus_item(this_focus_info.focus_item);
    _pluginstore_update_page_info();
    PLUGIN_DBG("%d, id: %s", i, this_focus_info.node->id);
}

static void _draw_image(char *url, int index)
{
    char *name = NULL;
    uint32_t length = 0;

    if(strncmp(url, "zip://", 6) == 0)
    {
        length = strlen(TMP_DOWNLOAD_PATH) + strlen(get_fname_suffix_from_url(url)) + 20;
        if(length >= MAX_PATH_LEN)
        {
            PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
            return;
        }

        name = GxCore_Calloc(length,sizeof(char));
        if(NULL == name)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            return;
        }
        snprintf(name, length, "%s/%d%s", TMP_DOWNLOAD_PATH, index, get_fname_suffix_from_url(url));
        Plugin_zip_decompression(url, name);
        gal_add_key_path(key_img[index], name);
        APP_FREE(name);
    }
    else
    {
        gal_add_key_path(key_img[index], url);
    }

    GUI_SetProperty(s_img_plugin_unfocus[index], "img",key_img[index]);
    GUI_SetProperty(s_img_plugin_unfocus[index], "state", "show");

    return;
}

#if PLUGINONLINE_SUPPORT
static int _draw_plugin_store_pic(void *data)
{
    PluginInfoNode *node;
    int len = 0;
    int item, page;

    item = this_focus_info.focus_item;
    page = 1 + (item - (item % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
    if(page > 1)
        return -1;
    if(this_focus_info.node == NULL)
        return -1;
    node = list_entry(this_focus_info.head->list.next, typeof(*this_focus_info.head), list);
    if(node == NULL ||node->icon_url == NULL)
        return -1;
    len = strlen(node->icon_url);
    if(Plugin_need_update_counter_get() > 0)
        node->icon_url[len - 7] = 'x';
    else
        node->icon_url[len - 7] = 'g';
    _draw_image(node->icon_url, 0);
    return 0;
}
#endif

static void plugin_draw_plugin_store_pic_timer_start(void)
{
#if PLUGINONLINE_SUPPORT
    APP_TIMER_ADD(s_draw_plugin_store_pic_timer, _draw_plugin_store_pic, 2 * 1000, TIMER_REPEAT_NOW);
#endif
}

static void plugin_draw_plugin_store_pic_timer_stop(void)
{
#if PLUGINONLINE_SUPPORT
    if(s_draw_plugin_store_pic_timer)
        timer_stop(s_draw_plugin_store_pic_timer);
#endif
}

static void plugin_draw_plugin_store_pic_timer_delete(void)
{
#if PLUGINONLINE_SUPPORT
    APP_TIMER_REMOVE(s_draw_plugin_store_pic_timer);
#endif
}

static int _plugin_main_menu_info_draw(int mode)
{
    PluginInfoNode *pos = NULL;
    PluginInfoList *head = NULL;
    int i = 0;
    int j = 0;

    if(NULL == this_focus_info.head)
    {
        if(-1 == Plugin_info_list_get(&this_focus_info.head))
        {
            APP_FREE(this_focus_info.head);
            PLUGIN_ERR("Plugin_info_list_get error\n");
            return -1;
        }

        head = this_focus_info.head;
        this_focus_info.total_number = this_focus_info.head->counter;
        PLUGIN_INFO("total_number:%d", this_focus_info.total_number);
        if(this_focus_info.total_number <= 0)
        {
            APP_FREE(this_focus_info.head);
            return -1;
        }
        _focus_info_item_init();
    }
    else
    {
        if(mode == 1)
        {
            list_from_node_reverse(pos, &(this_focus_info.node->list), list, this_focus_info.head)
            {
                j++;
                if(j > (this_focus_info.focus_item % MAX_PAGE_ITEM))
                    break;
            }
            head = pos;
        }
        else
        {
            head = list_entry(this_focus_info.node->list.prev, typeof(*this_focus_info.node), list);
        }
    }

    list_from_node(pos, &(head->list), list, this_focus_info.head)
    {
        if(i >= MAX_PAGE_ITEM)
            break;

        GUI_SetProperty(s_text_plugin_title[i], "string", pos->title);
        GUI_SetProperty(s_text_plugin_title[i], "state", "show");
        if(strcmp(pos->id, "plugin") != 0) //draw plugin store picture
            _draw_image(pos->icon_url, i);
        i++;
    }
    for(; i < MAX_PAGE_ITEM; i++)
    {
        GUI_SetProperty(s_img_plugin_unfocus[i], "state", "hide");
        GUI_SetProperty(s_text_plugin_title[i], "state", "hide");
    }

    return 0;
}

void app_create_pluginstore_menu(void)
{
    int ret;

    if (GxCore_FileExists(TMP_DOWNLOAD_PATH) == GXCORE_FILE_UNEXIST)
    {
        if(GxCore_Mkdir(TMP_DOWNLOAD_PATH) < 0)
            return;
    }
    if(GUI_CheckDialog(WND_PLUGINSTORE) == GXCORE_ERROR)
        GUI_CreateDialog(WND_PLUGINSTORE);

    _pluginstore_unfocus_all_item();
    GUI_SetInterface("flush", NULL);
    _focus_info_item_init();
    ret = _plugin_main_menu_info_draw(0);
    if(ret < 0)
        return;
    _pluginstore_focus_item(this_focus_info.focus_item);
    _pluginstore_update_page_info();
#if PLUGINONLINE_SUPPORT
    Plugin_update_timer_start();
#endif
    plugin_draw_plugin_store_pic_timer_start();
}

static int _rm_pathfile(const char* name)
{
    struct stat st;
    DIR *dir;
    struct dirent *de;
    int fail = 0;

    if (lstat(name, &st) < 0)
        return -1;

    if (!S_ISDIR(st.st_mode))
        return unlink(name);

    dir = opendir(name);
    if (dir == NULL)
        return -1;

    errno = 0;
    while((de = readdir(dir)) != NULL) {
        char dn[PATH_MAX];
        if (!strcmp(de->d_name, "..") || !strcmp(de->d_name, "."))
            continue;
        sprintf(dn, "%s/%s", name, de->d_name);
        if (_rm_pathfile(dn) < 0) {
            fail = 1;
            break;
        }
        errno = 0;
    }
    if (fail || errno < 0) {
        int save = errno;
        closedir(dir);
        errno = save;
        return -1;
    }
    if (closedir(dir) < 0)
        return -1;

    return rmdir(name);
}

void app_pluginstore_exit(void)
{
    plugin_draw_plugin_store_pic_timer_delete();
    _rm_pathfile(TMP_DOWNLOAD_PATH);
#if PLUGINONLINE_SUPPORT
    Plugin_update_timer_delete();
#endif
}

SIGNAL_HANDLER int app_pluginstore_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pluginstore_destroy(GuiWidget *widget, void *usrdata)
{
    app_pluginstore_exit();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pluginstore_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_OK:
                plugin_draw_plugin_store_pic_timer_delete();
                if(strcmp(this_focus_info.node->id, STR_PLUGIN_MANAGE_ID) == 0)
                {
#if PLUGINONLINE_SUPPORT
                    app_create_plugin_manage_menu();
#endif
                }
                else
                {
                    if(_plugin_setting_file_check() < 0)
                    {
                        _plugin_create_setting_menu();
                    }
                    else
                    {
                        if(this_focus_info.node->ui_type != NULL && strcmp(UI_TYPE_LISTVIEW, this_focus_info.node->ui_type) == 0)
                            app_create_plugin_listview_play_menu();
                        else
                            app_create_plugin_source_menu();
                    }
                }
                break;
            case STBK_UP:
                _pluginstore_up_keypress();
                break;
            case STBK_DOWN:
                _pluginstore_down_keypress();
                break;
            case STBK_LEFT:
                _pluginstore_left_keypress();
                break;
            case STBK_RIGHT:
                _pluginstore_right_keypress();
                break;
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_PLUGINSTORE);
                break;
            case STBK_RED:
                if(this_focus_info.node->setting != NULL)
                    _plugin_create_setting_menu();
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pluginstore_lost_focus(GuiWidget *widget, void *usrdata)
{
    PLUGIN_INFO("=====>");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pluginstore_got_focus(GuiWidget *widget, void *usrdata)
{
    PLUGIN_INFO("=====>");
    this_focus_info.total_number = this_focus_info.head->counter;
    plugin_draw_plugin_store_pic_timer_start();
    _plugin_main_menu_info_draw(1);
    _pluginstore_update_page_info();

    return EVENT_TRANSFER_STOP;
}

#endif
