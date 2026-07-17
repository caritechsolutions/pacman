/*************************************************************************
	> File Name: app_plugin_listview_play.c
	> Author:
	> Mail:
	> Created Time: 2017年11月16日 星期四 10时12分45秒
    > Dec: new UI mode for plugin
 ************************************************************************/

#include "app.h"
#if PLUGINSTORE_SUPPORT
#include "gx_pluginstore_api.h"
#include "app_plugin_server.h"
#include "../app_netapps_service.h"
#include "app_module.h"
#include "full_screen.h"
#include "../app_play_bar_info.h"
#include "gx_apps.h"
#include "app_pop.h"
#include "app_plugin_update.h"
#include "jansson.h"
#include "app_keyboard_language.h"
#include "app_book.h"
#include "app_windows.h"

#define TXT_STATE               "txt_plugin_listview_state"
#define IMG_STATE               "img_plugin_listview_state"

#define WND_LIST_GROUP1         "list_plugin_listview1_play_group"
#define IMG_LIST_GROUP1_BACK    "list_plugin_listview1_play_group_back_img"

#define WND_LIST_GROUP2         "list_plugin_listview2_play_group"
#define IMG_LIST_GROUP2_BACK    "list_plugin_listview2_play_group_back_img"

#define WND_LIST_GROUP3         "list_plugin_listview3_play_group"
#define IMG_LIST_GROUP3_BACK    "list_plugin_listview3_play_group_back_img"

#define LISTVIEW_FOCUS_IMG      "new_focus_item"
#define LISTVIEW_UNFOCUS_IMG    "null"

#define IMG_MUTE                "img_plugin_listview_play_mute"
#define IMG_GIF                 "fimg_plugin_listview_play_gif"
#define GIF_PATH                WORK_PATH"theme/image/netapps/loading.gif"


#define CHANNEL_NUM_LEN	    (8)
#define MAX_FILE_NAME_LEN   (128)
#define USE_THREAD          1
#define NO_USE_THREAD       0
#define MAX_PAGE_NUM        (10)
#define MAX_COUNT           (1000)

enum UPDATE_STATUS{
    UPDATE_CLASSIFICATION,
    UPDATE_SOURCE,
    UPDATE_SOURCE_CHILD,
    UPDATE_SOURCE_URL,
    UPDATE_SOURCE_PLAY,
    UPDATE_END,
};

enum MSG_CONTROL_STATUS{
    MSG_DIG_END,
    MSG_DIG_UPDATE,
    MSG_SHOW_TIP,
    MSG_PLAY,
};

typedef struct _ListUIDataNode{
    int counter;
    char *type;
    char *name;
    struct dlist_head list;
}ListUIDataNode, ListUIDataList;

struct ListFocusInfo{
    int total_number;
    int focus_item;
    int active_item;
    ListUIDataNode *node;
    ListUIDataList *head;
    struct ListFocusInfo *child;
};

struct ThisUICtrol{
    int flag;
    char press_play;
    char first_play_init;
    char *search_str;
    char play_url[PLAYER_URL_LONG + 1];
    int search_enable;
    bool wnd_created;
    bool exit_flag;
    bool login;
    event_list *first_play_timer;
    event_list *gif_timer;
    event_list *play_record_url_timer;
    handle_t mutex_id;
    handle_t list_update_thread_handle;
    PlayBarInfoOps play_info_ops;
    PlayerStatus play_status;
    struct ListFocusInfo focus_info;
};

extern PluginInfoNode *Pluginstore_focus_node_get(void);

static void _plugin_listview_play_exit_keypress(void);

static struct ThisUICtrol this = {0};
static event_list *thiz_tip_timer = NULL;

static void _plugin_show_tip(char *str, int32_t time_out)
{
    if(NULL == str || time_out < 0)
        return;

    PopDlg pop;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = str;
    pop.timeout_sec = time_out;
    popdlg_create(&pop);

    app_update_pop_dlg(WND_POP_BOOK);

    return;
}

static int32_t _plugin_get_data_slow_tip(void *arg)
{
    if((0 == this.focus_info.total_number && UPDATE_CLASSIFICATION == this.flag)
            || ((NULL == this.focus_info.child || 0 == this.focus_info.child->total_number) && UPDATE_SOURCE == this.flag)
            || ((NULL == this.focus_info.child || NULL == this.focus_info.child->child || 0 == this.focus_info.child->child->total_number) && UPDATE_SOURCE_CHILD == this.flag))
        _plugin_show_tip(STR_ID_PLUGIN_GET_DATA_SLOW, 2);

    return 0;
}

static void _plugin_play_info_save(char *url, char *name, int channel_num)
{
    int i;
    int ret;
    char channel_num_str[10];
    char file[MAX_FILE_NAME_LEN] = {0};
    json_t *object = {NULL};
    PluginInfoNode *node;

    if(url == NULL || name == NULL)
        return;

    if(GxCore_FileExists(PLUGIN_SETTING_FILE_PATH) == GXCORE_FILE_UNEXIST)
    {
        if(GxCore_Mkdir(PLUGIN_SETTING_FILE_PATH) < 0)
        {
            PLUGIN_ERR("create plugin cfg error\n");
            return;
        }
    }
    sprintf(channel_num_str, "%d", channel_num);
    object = json_object();
    for(i = 0; i < 3; i++)
    {
        json_object_set_new(object, "url", json_string(url));
        json_object_set_new(object, "name", json_string(name));
        json_object_set_new(object, "channel_num", json_string(channel_num_str));
    }

    node = Pluginstore_focus_node_get();
    snprintf(file, MAX_FILE_NAME_LEN, "%s/%s.play", PLUGIN_SETTING_FILE_PATH, node->id);

    ret = json_dump_file(object, file, 0);
    if(ret < 0)
    {
        if(GxCore_FileExists(file) == GXCORE_FILE_EXIST)
            GxCore_FileDelete(file);
    }
    json_decref(object);
}

static int _plugin_play_info_get(char **url, char **name, int *channel_num)
{
    char file[MAX_FILE_NAME_LEN] = {0};
    json_error_t error;
    json_t *object = NULL;
    const char *obj_key = NULL;
    json_t *obj_value = NULL;
    PluginInfoNode *node;

    node = Pluginstore_focus_node_get();
    snprintf(file, MAX_FILE_NAME_LEN, "%s/%s.play", PLUGIN_SETTING_FILE_PATH, node->id);

    if(GXCORE_FILE_EXIST == GxCore_FileExists(file))
    {
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
            if(strcmp(obj_key, "url") == 0)
                *url = GxCore_Strdup(json_string_value(obj_value));

            if(strcmp(obj_key, "name") == 0)
                *name = GxCore_Strdup(json_string_value(obj_value));

            if(strcmp(obj_key, "channel_num") == 0)
                sscanf(json_string_value(obj_value), "%d", channel_num);
        }
        json_decref(object);
        if(*name == NULL || *url == NULL || 0 == strcmp(*url, "null"))
        {
            APP_FREE(*name);
            APP_FREE(*url);
            return -1;
        }
        return 0;
    }
    return -1;
}

static void _no_program_state_show(void)
{
    if(this.play_status != PLAYER_STATUS_PLAY_RUNNING && this.play_status != PLAYER_STATUS_PLAY_START)
    {
        GUI_SetProperty(TXT_STATE, "string", STR_ID_NO_PROGRAM);
        GUI_SetProperty(TXT_STATE, "state", "show");
        GUI_SetProperty(IMG_STATE, "state", "show");
    }

    app_update_pop_dlg(WND_POP_BOOK);
}

static void _no_program_state_hide(void)
{
    char state[5] = {0};

    GUI_GetProperty(IMG_STATE, "state", &state);
    if(0 == strcmp(state, "show"))
    {
        GUI_SetProperty(TXT_STATE, "state", "hide");
        GUI_SetProperty(IMG_STATE, "state", "hide");
        GUI_SetInterface("flush", NULL);
    }
}

static int _plugin_listview_play_draw_gif(void* usrdata)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
    return 0;
}

static void _plugin_source_load_gif(void)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
    GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _plugin_source_free_gif(void)
{
    APP_TIMER_REMOVE(this.gif_timer);
    this.gif_timer = NULL;
    GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void _plugin_listview_play_show_gif(void)
{
    GUI_SetProperty(IMG_GIF, "state", "show");
    APP_TIMER_ADD(this.gif_timer, _plugin_listview_play_draw_gif, 100, TIMER_REPEAT);
}

static void _plugin_listview_play_hide_gif(void)
{
    APP_TIMER_REMOVE(this.gif_timer);
    GUI_SetProperty(IMG_GIF, "state", "hide");
}

static int _plugin_listview_play_popdlg_exit_cb(PopDlgRet ret)
{
    char state[5] = {0};

    GUI_GetProperty(WND_LIST_GROUP1, "state", &state);
    if(strcmp(state, "hide") == 0)
        _no_program_state_show();

    if(true == this.exit_flag)
        _plugin_listview_play_exit_keypress();

    return 0;
}

static void _plugin_listview_play_popdlg(char *msg, int time_sec)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_WAIT;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = _plugin_listview_play_popdlg_exit_cb;
    pop.str = msg;
    pop.timeout_sec = time_sec;
    popdlg_create(&pop);

    app_update_pop_dlg(WND_POP_BOOK);
}

static void _plugin_listview_play_mute_exec(void)
{
    GxMsgProperty_PlayerAudioMute player_mute;

    if (g_AppFullArb.state.mute == STATE_ON)
    {
        player_mute = 0;
        g_AppFullArb.state.mute = STATE_OFF;
        app_set_hardware_unmute();
    }
    else
    {
        player_mute = 1;
        g_AppFullArb.state.mute = STATE_ON;
        app_set_hardware_mute();
    }
    app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
}

static void _plugin_listview_play_mute_draw(void)
{
    if (g_AppFullArb.state.mute == STATE_ON)
        GUI_SetProperty(IMG_MUTE, "state", "show");
    else
        GUI_SetProperty(IMG_MUTE, "state", "osd_trans_hide");

    GUI_SetProperty(IMG_MUTE, "draw_now", NULL);
}

void app_plugin_listview_play_unmute_change(void)
{
    if(g_AppFullArb.state.mute == STATE_ON)
    {
        _plugin_listview_play_mute_exec();
        _plugin_listview_play_mute_draw();
    }
}

static void _plugin_listview_play_show_tip(char *str)
{
    struct PlugiaSourceUIMsg param = {0};

    if(NULL == str)
        return;

    param.type = MSG_SHOW_TIP;
    param.status = str;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    return;
}

static void _plugin_listview_play_list_update(int id, int active)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_DIG_UPDATE;
    param.item = id;
    param.active = active;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    return;
}

void _list_ui_data_free(ListUIDataList **head)
{
    ListUIDataNode *pos;
    ListUIDataNode *pos_back;

    if(head == NULL || *head == NULL)
        return;

    list_for_each_entry_safe(pos, pos_back, &((*head)->list), list)
    {
        list_del(&(pos->list));
        APP_FREE(pos->type);
        APP_FREE(pos->name);
        APP_FREE(pos);
    }
    APP_FREE(*head);
}

static int _list_update_group1(PluginInfoNode *node, ClassificationNode **classification_node, SourceInfoNode **source_node, ClassificationNode **classification_pos_back, int *update_flag)
{
    int update_ui_flag = 0;
    ListUIDataNode *ui_node;
    ClassificationNode *classification_pos = NULL;

    if(*update_flag == UPDATE_SOURCE_PLAY)
        Source_url_refresh_close();

    if(*update_flag != UPDATE_CLASSIFICATION)
    {
        *update_flag = UPDATE_CLASSIFICATION;
        if(*source_node != NULL && (*source_node)->child_list != NULL)
        {
            Source_list_free(&(*source_node)->child_list);
            *source_node = NULL;
        }
        *source_node = NULL;
        if(*classification_node != NULL && (*classification_node)->source_list_head != NULL)
        {
            Source_list_free(&(*classification_node)->source_list_head);
            *classification_node = NULL;
        }
        *classification_node = NULL;
    }
    else
    {
        update_ui_flag = 0;
        list_from_node(classification_pos, &((*classification_pos_back)->list), list, node->classification_list_head)
        {
            ui_node = (ListUIDataNode *)GxCore_Calloc(1, sizeof(ListUIDataNode));
            if(ui_node == NULL)
            {
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                return -1;
            }
            ui_node->counter = classification_pos->counter;
            ui_node->type = GxCore_Strdup(classification_pos->type);
            ui_node->name = GxCore_Strdup(classification_pos->title);
            this.focus_info.total_number++;
            list_add_tail(&(ui_node->list), &(this.focus_info.head->list));
            update_ui_flag = 1;
        }
        if(update_ui_flag)
        {
            *classification_pos_back = list_entry(classification_pos->list.prev, typeof(*classification_pos), list);
            if(this.focus_info.node == NULL)
                this.focus_info.node = list_entry(this.focus_info.head->list.next, typeof(*this.focus_info.head), list);
            _plugin_listview_play_list_update(1, this.focus_info.active_item);
        }
    }
    return 0;
}

static int _list_update_group2(PluginInfoNode *node, ClassificationNode **classification_node, SourceInfoNode **source_node, SourceInfoNode **source_pos_back, int *update_flag)
{
    int update_ui_flag = 0;
    ListUIDataNode *ui_node;
    ClassificationNode *classification_pos = NULL;
    SourceInfoNode *source_pos = NULL;

    if(*update_flag == UPDATE_SOURCE_PLAY)
        Source_url_refresh_close();

    *update_flag = UPDATE_SOURCE;
    if(*source_node != NULL && (*source_node)->child_list != NULL)
    {
        Source_list_free(&(*source_node)->child_list);
        *source_node = NULL;
    }
    *source_node = NULL;
    if(*classification_node == NULL || (*classification_node)->counter != this.focus_info.node->counter)
    {
        if(*classification_node != NULL && (*classification_node)->source_list_head != NULL)
        {
            Source_list_free(&(*classification_node)->source_list_head);
            *classification_node = NULL;
        }
        list_from_node(classification_pos, &(node->classification_list_head->list), list, node->classification_list_head)
        {
            if(classification_pos->counter == this.focus_info.node->counter)
            {
                *classification_node = classification_pos;
                *source_pos_back = (*classification_node)->source_list_head;
                break;
            }
        }
        if(*classification_node == NULL)
        {
            PLUGIN_ERR("error!\n");
            return -1;
        }
    }
    else
    {
        if(this.focus_info.child->total_number == 0)
        {
            *source_pos_back = (*classification_node)->source_list_head;
        }
        update_ui_flag = 0;
        list_from_node(source_pos, &((*source_pos_back)->list), list, (*classification_node)->source_list_head)
        {
            ui_node = (ListUIDataNode *)GxCore_Calloc(1, sizeof(ListUIDataNode));
            if(ui_node == NULL)
            {
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                return -1;
            }
            ui_node->counter = source_pos->counter;
            ui_node->type = GxCore_Strdup(source_pos->type);
            ui_node->name = GxCore_Strdup(source_pos->title);
            this.focus_info.child->total_number++;
            list_add_tail(&(ui_node->list), &(this.focus_info.child->head->list));
            update_ui_flag = 1;
        }
        if(update_ui_flag)
        {
            *source_pos_back = list_entry(source_pos->list.prev, typeof(*source_pos), list);
            if(this.focus_info.child->node == NULL)
                this.focus_info.child->node = list_entry(this.focus_info.child->head->list.next, typeof(*this.focus_info.child->head), list);
            _plugin_listview_play_list_update(2, this.focus_info.child->active_item);
        }
    }
    return 0;
}

static int _list_update_group3(ClassificationNode *classification_node, SourceInfoNode **source_node, SourceInfoNode **source_child_pos_back, int *update_flag)
{
    int update_ui_flag = 0;
    ListUIDataNode *ui_node;
    SourceInfoNode *source_pos = NULL;
    SourceInfoNode *source_child_pos = NULL;

    if(*update_flag == UPDATE_SOURCE_PLAY)
        Source_url_refresh_close();

    *update_flag = UPDATE_SOURCE_CHILD;
    if(*source_node == NULL || (*source_node)->counter != this.focus_info.child->node->counter)
    {
        if(*source_node != NULL && (*source_node)->child_list != NULL)
        {
            Source_list_free(&(*source_node)->child_list);
            *source_node = NULL;
        }
        list_from_node(source_pos, &(classification_node->source_list_head->list), list, classification_node->source_list_head)
        {
            if(source_pos->counter == this.focus_info.child->node->counter)
            {
                *source_node = source_pos;
                *source_child_pos_back = (*source_node)->child_list;
                break;
            }
        }
        if(*source_node == NULL)
        {
            PLUGIN_ERR("error!\n");
            return -1;
        }
    }
    else
    {
        if(this.focus_info.child->child->total_number == 0)
        {
            *source_child_pos_back = (*source_node)->child_list;
        }
        update_ui_flag = 0;
        list_from_node(source_child_pos, &((*source_child_pos_back)->list), list, (*source_node)->child_list)
        {
            ui_node = (ListUIDataNode *)GxCore_Calloc(1, sizeof(ListUIDataNode));
            if(ui_node == NULL)
            {
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                return -1;
            }
            ui_node->counter = source_child_pos->counter;
            ui_node->type = GxCore_Strdup(source_child_pos->type);
            ui_node->name = GxCore_Strdup(source_child_pos->title);
            this.focus_info.child->child->total_number++;
            list_add_tail(&(ui_node->list), &(this.focus_info.child->child->head->list));
            update_ui_flag = 1;
        }
        if(update_ui_flag)
        {
            *source_child_pos_back = list_entry(source_child_pos->list.prev, typeof(*source_child_pos), list);
            if(this.focus_info.child->child->node == NULL)
                this.focus_info.child->child->node = list_entry(this.focus_info.child->child->head->list.next, typeof(*this.focus_info.child->child->head), list);
            _plugin_listview_play_list_update(3, this.focus_info.child->child->active_item);
        }
    }
    return 0;
}

static int _list_update_url(ClassificationNode *classification_node, SourceInfoNode **source_node, SourceInfoNode **source_child_node, int *update_flag)
{
    SourceInfoNode *source_pos = NULL;
    SourceInfoNode *source_child_pos = NULL;

    if(*update_flag == UPDATE_SOURCE_PLAY)
        Source_url_refresh_close();

    *update_flag = UPDATE_SOURCE_URL;

    if(*source_node != NULL && (*source_node)->child_list != NULL)
    {
        *source_child_node = NULL;
        list_from_node(source_child_pos, &((*source_node)->child_list->list), list, (*source_node)->child_list)
        {
            if(source_child_pos->counter == this.focus_info.child->child->node->counter)
            {
                *source_child_node = source_child_pos;
                break;
            }
        }
        if(*source_child_node == NULL)
        {
            PLUGIN_ERR("error!\n");
            return -1;
        }
    }
    else
    {
        *source_node = NULL;
        list_from_node(source_pos, &(classification_node->source_list_head->list), list, classification_node->source_list_head)
        {
            if(source_pos->counter == this.focus_info.child->node->counter)
            {
                *source_node = source_pos;
                break;
            }
        }
        if(*source_node == NULL)
        {
            PLUGIN_ERR("error!\n");
            return -1;
        }
    }
    return 0;
}

static void _list_update_thread_end(PluginInfoNode *node, ClassificationNode *classification_node, SourceInfoNode *source_node)
{
    if(source_node != NULL && source_node->child_list != NULL)
        Source_list_free(&source_node->child_list);

    if(classification_node != NULL && classification_node->source_list_head != NULL)
        Source_list_free(&classification_node->source_list_head);

    if(node->classification_list_head != NULL)
        Classification_list_free(&node->classification_list_head);

    PLUGIN_INFO("=====>");
}

static void _list_update_thread(void *argc)
{
    int ret = -1;
    int update_flag = UPDATE_CLASSIFICATION;
    PluginInfoNode *node;
    ClassificationNode *classification_node = NULL;
    ClassificationNode *classification_pos_back = NULL;
    SourceInfoNode *source_node = NULL;
    SourceInfoNode *source_pos_back = NULL;
    SourceInfoNode *source_child_node = NULL;
    SourceInfoNode *source_child_pos_back = NULL;
    char *url = NULL;
    struct PlugiaSourceUIMsg param = {0};
    char search_str[MAX_FILE_NAME_LEN];
    int search_enable = 0;

    GxCore_ThreadDetach();
    node = Pluginstore_focus_node_get();
    Plugin_store_init();
    Plugin_install(node->folder);
    ret = Classification_list_get(node);
    if(ret < 0)
    {
        this.login = false;
        PLUGIN_ERR("load error\n");
        goto ERROR;
    }
    else
    {
        this.login = true;
    }

    classification_pos_back = node->classification_list_head;

    while(1)
    {
        switch(update_flag)
        {
            case UPDATE_CLASSIFICATION:
                {
                    ret += Classification_list_refresh(node->classification_list_head);
                    break;
                }
            case UPDATE_SOURCE:
                {
                    if(NULL == classification_node)
                        break;

                    if(classification_node->source_list_head == NULL)
                    {
                        PLUGIN_INFO("UPDATE_SOURCE->counter:%d",classification_node->counter);
                        if(search_enable == 1)
                        {
                            search_enable = 0;
                            ret += Source_list_search_get(&classification_node->source_list_head, classification_node->counter, USE_THREAD, search_str);
                        }
                        else
                        {
                            ret += Source_list_get(&classification_node->source_list_head, classification_node->counter, USE_THREAD);
                        }
                        source_pos_back = classification_node->source_list_head;
                    }
                    else
                    {
                        GxCore_ThreadDelay(100);
                        ret += Source_list_refresh(classification_node->source_list_head);
                    }
                    break;
                }
            case UPDATE_SOURCE_CHILD:
                {
                    if(NULL == source_node)
                        break;

                    if(source_node->child_list == NULL)
                    {
                        PLUGIN_INFO("UPDATE_SOURCE_CHILD->counter: %d", source_node->counter);
                        ret += Source_list_get(&source_node->child_list, source_node->counter, NO_USE_THREAD);
                        source_child_pos_back = source_node->child_list;
                    }
                    else
                    {
                        GxCore_ThreadDelay(100);
                        ret += Source_list_refresh(source_node->child_list);
                    }
                    break;
                }
            case UPDATE_SOURCE_URL:
                {
                    if(source_child_node != NULL)
                    {
                        PLUGIN_INFO("UPDATE_SOURCE_URL: %d", source_child_node->counter);
                        ret += Source_url_get(source_child_node->counter);
                    }
                    else
                    {
                        if(NULL == source_node)
                            break;

                        PLUGIN_INFO("UPDATE_SOURCE_URL: %d", source_node->counter);
                        ret += Source_url_get(source_node->counter);
                    }
                    update_flag = UPDATE_SOURCE_PLAY;
                    break;
                }
            case UPDATE_SOURCE_PLAY:
                {
                    ret += Source_url_refresh(&(url));
                    if(url != NULL)
                    {
                        if(source_child_node != NULL)
                        {
                            update_flag = UPDATE_SOURCE_CHILD;
                            source_child_node = NULL;
                        }
                        else
                        {
                            update_flag = UPDATE_SOURCE;
                            source_node = NULL;
                        }
                        Source_url_refresh_close();
                        param.type = MSG_PLAY;
                        param.status = source_url_json_parse(url);
                        APP_FREE(url);
                        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
                    }
                    break;
                }
            default:
                break;
        }
        if(ret < 0)
        {
            PLUGIN_ERR("error!\n");
            goto ERROR;
        }

        if(GxCore_MutexTrylock(this.mutex_id) >= 0)
        {
            switch(this.flag)
            {
                case UPDATE_CLASSIFICATION:
                    {
                        ret += _list_update_group1(node, &classification_node, &source_node, &classification_pos_back, &update_flag);
                        break;
                    }
                case UPDATE_SOURCE:
                    {
                        if(this.search_enable == 1)
                        {
                            this.search_enable = 0;
                            search_enable = 1;
                            strcpy(search_str, this.search_str);
                        }
                        ret += _list_update_group2(node, &classification_node, &source_node, &source_pos_back, &update_flag);
                        break;
                    }
                case UPDATE_SOURCE_CHILD:
                    {
                        ret += _list_update_group3(classification_node, &source_node, &source_child_pos_back, &update_flag);
                        break;
                    }
                case UPDATE_SOURCE_URL:
                    {
                        this.flag = UPDATE_SOURCE_PLAY;
                        ret += _list_update_url(classification_node, &source_node, &source_child_node, &update_flag);
                        break;
                    }
                case UPDATE_END:
                    {
                        GxCore_MutexUnlock(this.mutex_id);
                        goto END;
                    }
                default:
                    {
                        break;
                    }
            }
            GxCore_MutexUnlock(this.mutex_id);
            if(ret < 0)
            {
                PLUGIN_ERR("error!\n");
                goto ERROR;
            }
        }
        GxCore_ThreadDelay(20);
    }

END:
    _list_update_thread_end(node, classification_node, source_node);
    return;
ERROR:
    this.exit_flag = true;
    this.flag = UPDATE_END;
    _list_update_thread_end(node, classification_node, source_node);
    _plugin_listview_play_show_tip(STR_ID_PLUGIN_ERR);
    return;
}

static void _classification_list_get(void)
{
    GxCore_ThreadCreate("list_update", &this.list_update_thread_handle, _list_update_thread,  NULL, 8*1024, GXOS_DEFAULT_PRIORITY);
}

status_t app_create_plugin_listview_play_menu(void)
{
    memset(&this, 0, sizeof(struct ThisUICtrol));
    this.wnd_created = true;
    this.exit_flag = false;
    this.first_play_init = 1;
    this.focus_info.active_item = -1;
    this.focus_info.head = (ListUIDataList *)GxCore_Calloc(1, sizeof(ListUIDataList));
    if(this.focus_info.head == NULL)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }
    this.search_str = GxCore_Calloc(1, MAX_FILE_NAME_LEN);
    if(this.search_str == NULL)
    {
        APP_FREE(this.focus_info.head);
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }
    INIT_DLIST_HEAD(&(this.focus_info.head->list));
    GxCore_MutexCreate(&this.mutex_id);
    this.flag = UPDATE_CLASSIFICATION;
    _classification_list_get();

    if(GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY) == GXCORE_ERROR)
        GUI_CreateDialog(WND_PLUGIN_LISTVIEW_PLAY);

    GUI_SetInterface("flush", NULL);
    _plugin_listview_play_mute_draw();
    return 0;
}

void app_destroy_plugin_listview_play_menu(void)
{
    APP_TIMER_REMOVE(thiz_tip_timer);
    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
    this.wnd_created = false;
    this.exit_flag = false;
    GxCore_MutexLock(this.mutex_id);
    if(this.flag != UPDATE_END) //for book msg
    {
        this.flag = UPDATE_END;
        Plugin_store_exit();
    }
    if(this.focus_info.child != NULL && this.focus_info.child->child != NULL)
    {
        _list_ui_data_free(&this.focus_info.child->child->head);
        APP_FREE(this.focus_info.child->child);
    }
    if(this.focus_info.child != NULL)
    {
        _list_ui_data_free(&this.focus_info.child->head);
        APP_FREE(this.focus_info.child);
    }

    APP_FREE(this.play_info_ops.name);
    APP_FREE(this.search_str);
    _list_ui_data_free(&this.focus_info.head);
    GxCore_MutexUnlock(this.mutex_id);
    GxCore_MutexDelete(this.mutex_id);
}

static int  _plugin_first_play(void *usrdata)
{
    static int i = 0;

    if(this.first_play_init == 0)
    {
        APP_TIMER_REMOVE(this.first_play_timer);
        return 0;
    }

    GxCore_MutexLock(this.mutex_id);
    switch(this.flag)
    {
        case UPDATE_CLASSIFICATION:
            {
                if(this.focus_info.total_number > 0)
                {
                    if(this.focus_info.child == NULL)
                    {
                        this.focus_info.child = (struct ListFocusInfo*)GxCore_Calloc(1, sizeof(struct ListFocusInfo));
                        if(this.focus_info.child == NULL)
                        {
                            PLUGIN_ERR("No memory to GxCore_Calloc\n");
                            GxCore_MutexUnlock(this.mutex_id);
                            return -1;
                        }
                        this.focus_info.child->active_item = -1;
                        this.focus_info.child->head = (ListUIDataList *)GxCore_Calloc(1, sizeof(ListUIDataList));
                        if(this.focus_info.child->head == NULL)
                        {
                            PLUGIN_ERR("No memory to GxCore_Calloc\n");
                            APP_FREE(this.focus_info.child);
                            GxCore_MutexUnlock(this.mutex_id);
                            return -1;
                        }
                        INIT_DLIST_HEAD(&(this.focus_info.child->head->list));
                    }

                    if(strcmp("search", this.focus_info.node->type) == 0)
                    {
                        this.focus_info.focus_item += 1;
                        this.focus_info.active_item = this.focus_info.focus_item;
                        this.focus_info.node = list_entry(this.focus_info.node->list.next, typeof(*this.focus_info.node), list);
                        GUI_SetProperty(WND_LIST_GROUP1, "select", &this.focus_info.focus_item);
                    }
                    this.flag = UPDATE_SOURCE;
                    GUI_SetProperty(WND_LIST_GROUP1, "item_active_image", LISTVIEW_UNFOCUS_IMG);
                    GUI_SetProperty(WND_LIST_GROUP1, "active", NULL);
                    GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
                }
                break;
            }
        case UPDATE_SOURCE:
            {
                if(this.focus_info.child->total_number > 0)
                {
                    if(this.focus_info.child->total_number < 12 && i < 3)
                    {
                        i++;
                        break;
                    }
                    if(this.press_play == 0 && strcmp("directory", this.focus_info.child->node->type) != 0)
                    {
                        this.first_play_init = 0;
                        this.press_play = 1;
                        this.flag = UPDATE_SOURCE_URL;
                        APP_FREE(this.play_info_ops.name);
                        this.focus_info.child->active_item = this.focus_info.child->focus_item;
                        this.play_info_ops.channel_num = this.focus_info.child->focus_item + 1;
                        this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->node->name);
                    }
                    else
                    {
                        if(this.focus_info.child->child == NULL)
                        {
                            this.focus_info.child->child = (struct ListFocusInfo*)GxCore_Calloc(1, sizeof(struct ListFocusInfo));
                            if(this.focus_info.child->child == NULL)
                            {
                                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                                GxCore_MutexUnlock(this.mutex_id);
                                return -1;
                            }
                            this.focus_info.child->child->active_item = -1;
                            this.focus_info.child->child->head = (ListUIDataList *)GxCore_Calloc(1, sizeof(ListUIDataList));
                            if(this.focus_info.child->child->head == NULL)
                            {
                                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                                APP_FREE(this.focus_info.child->child);
                                GxCore_MutexUnlock(this.mutex_id);
                                return -1;
                            }
                            INIT_DLIST_HEAD(&(this.focus_info.child->child->head->list));
                        }
                        this.flag = UPDATE_SOURCE_CHILD;
                        GUI_SetProperty(WND_LIST_GROUP2, "item_active_image", LISTVIEW_UNFOCUS_IMG);
                        GUI_SetProperty(WND_LIST_GROUP2, "active", NULL);
                        GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
                    }
                }
                break;
            }
        case UPDATE_SOURCE_CHILD:
            {
                if(this.focus_info.child->child->total_number > 0)
                {
                    if(this.focus_info.child->child->total_number < 12 && i < 3)
                    {
                        i++;
                        break;
                    }
                    if(this.press_play == 0 && strcmp("directory", this.focus_info.child->child->node->type) != 0)
                    {
                        this.first_play_init = 0;
                        this.press_play = 1;
                        this.flag = UPDATE_SOURCE_URL;
                        APP_FREE(this.play_info_ops.name);
                        this.focus_info.child->child->active_item = this.focus_info.child->child->focus_item;
                        this.play_info_ops.channel_num = this.focus_info.child->child->focus_item + 1;
                        this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->child->node->name);
                    }
                    else
                    {
                        this.first_play_init = 0;
                    }
                }
                break;
            }
        default:
            break;
    }
    GxCore_MutexUnlock(this.mutex_id);
    return 0;
}

static int _plugin_play_record_url(void *arg)
{
    struct PlugiaSourceUIMsg param = {0};

    if(false == this.login || NULL == arg)
        return -1;

    param.type = MSG_PLAY;
    param.status = (char *)arg;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    APP_TIMER_REMOVE(this.play_record_url_timer);

    return 0;
}

static void _plugin_first_play_start(void)
{
    char *url = NULL;
    char *name = NULL;
    int channel_num = 0;
    int ret = -1;

    ret = _plugin_play_info_get(&url, &name, &channel_num);
    if(0 == ret)
    {
        GxCore_MutexLock(this.mutex_id);
        this.play_info_ops.channel_num = channel_num;
        this.play_info_ops.name = name;
        this.first_play_init = 0;
        GxCore_MutexUnlock(this.mutex_id);
        if(NULL == this.play_record_url_timer)
            this.play_record_url_timer = create_timer(_plugin_play_record_url, 200, url, TIMER_REPEAT);
    }
    else
    {
        APP_TIMER_ADD(this.first_play_timer, _plugin_first_play, 100, TIMER_REPEAT);
    }
}

SIGNAL_HANDLER int app_plugin_listview_play_create(GuiWidget *widget, void *usrdata)
{
    app_system_performace_dynamic_change(NET_PLAY_MODE);
    _plugin_source_load_gif();
    _plugin_listview_play_show_gif();
    _plugin_first_play_start();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview_play_destroy(GuiWidget *widget, void *usrdata)
{
    APP_TIMER_REMOVE(this.first_play_timer);
    APP_TIMER_REMOVE(this.play_record_url_timer);
    _plugin_source_free_gif();
    app_destroy_plugin_listview_play_menu();
    app_system_performace_dynamic_change(DVB_PLAY_MODE);
    return EVENT_TRANSFER_STOP;
}

static void _plugin_listview_play_exit_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    _no_program_state_hide();
    this.flag = UPDATE_END;
    Plugin_store_exit();
    GxCore_MutexUnlock(this.mutex_id);
    popdlg_destroy();
    app_play_bar_info_hide();
    GUI_EndDialog(WND_PLUGIN_LISTVIEW_PLAY);
}

static void _plugin_listview_play_ok_keypress(void)
{
    if(this.first_play_init)
        return;
    APP_TIMER_ADD(thiz_tip_timer, _plugin_get_data_slow_tip, 30000, TIMER_REPEAT);
    GxCore_MutexLock(this.mutex_id);
    _no_program_state_hide();
    if(this.focus_info.child != NULL && this.focus_info.child->child != NULL)
    {
        GUI_SetProperty(WND_LIST_GROUP1, "state", "show");
        GUI_SetProperty(IMG_LIST_GROUP1_BACK, "state", "show");
        GUI_SetProperty(WND_LIST_GROUP2, "state", "show");
        GUI_SetProperty(IMG_LIST_GROUP2_BACK, "state", "show");
        GUI_SetProperty(WND_LIST_GROUP3, "state", "show");
        GUI_SetProperty(IMG_LIST_GROUP3_BACK, "state", "show");
        GUI_SetFocusWidget(WND_LIST_GROUP3);
        if(1 == this.press_play)
        {
            GUI_SetProperty(WND_LIST_GROUP1, "active", &this.focus_info.active_item);
            GUI_SetProperty(WND_LIST_GROUP2, "active", &this.focus_info.child->active_item);
            GUI_SetProperty(WND_LIST_GROUP3, "active", &this.focus_info.child->child->active_item);
        }
    }
    else if(this.focus_info.child != NULL)
    {
        GUI_SetProperty(WND_LIST_GROUP1, "state", "show");
        GUI_SetProperty(IMG_LIST_GROUP1_BACK, "state", "show");
        GUI_SetProperty(WND_LIST_GROUP2, "state", "show");
        GUI_SetProperty(IMG_LIST_GROUP2_BACK, "state", "show");
        GUI_SetFocusWidget(WND_LIST_GROUP2);
        if(1 == this.press_play)
        {
            GUI_SetProperty(WND_LIST_GROUP1, "active", &this.focus_info.active_item);
            GUI_SetProperty(WND_LIST_GROUP2, "active", &this.focus_info.child->active_item);
        }
    }
    else
    {
        GUI_SetProperty(WND_LIST_GROUP1, "state", "show");
        GUI_SetProperty(IMG_LIST_GROUP1_BACK, "state", "show");
        GUI_SetFocusWidget(WND_LIST_GROUP1);
    }
    GxCore_MutexUnlock(this.mutex_id);
    app_update_pop_dlg(WND_POP_BOOK);
}

static void _plugin_listview_play_down_keypress(void)
{
    if(this.first_play_init)
        return;
    GxCore_MutexLock(this.mutex_id);
    if(this.flag == UPDATE_CLASSIFICATION)
    {
        GxCore_MutexUnlock(this.mutex_id);
        return;
    }
    _no_program_state_hide();
    if(this.focus_info.child->child == NULL)
    {
        if(this.focus_info.child->total_number > 0 && strcmp("directory", this.focus_info.child->node->type) != 0)
        {
            _plugin_listview_play_show_gif();
            popdlg_destroy();
            this.press_play = 1;
            this.flag = UPDATE_SOURCE_URL;
            this.focus_info.child->focus_item -=1;
            if(this.focus_info.child->focus_item < 0)
            {
                this.focus_info.child->focus_item = this.focus_info.child->total_number - 1;
                this.focus_info.child->node = list_entry(this.focus_info.child->head->list.prev, typeof(*this.focus_info.child->head), list);
            }
            else
            {
                this.focus_info.child->node = list_entry(this.focus_info.child->node->list.prev, typeof(*this.focus_info.child->node), list);
            }

            APP_FREE(this.play_info_ops.name);
            this.focus_info.child->active_item = this.focus_info.child->focus_item;
            this.play_info_ops.channel_num = this.focus_info.child->focus_item + 1;
            this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->node->name);

            GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
            GUI_SetProperty(WND_LIST_GROUP2, "select", &this.focus_info.child->focus_item);
            GUI_SetProperty(WND_LIST_GROUP2, "active", &this.focus_info.child->active_item);
        }
    }
    else
    {
        if(this.focus_info.child->child->total_number > 0 && strcmp("directory", this.focus_info.child->child->node->type) != 0)
        {
            _plugin_listview_play_show_gif();
            popdlg_destroy();
            this.press_play = 1;
            this.flag = UPDATE_SOURCE_URL;
            this.focus_info.child->child->focus_item -=1;
            if(this.focus_info.child->child->focus_item < 0)
            {
                this.focus_info.child->child->focus_item = this.focus_info.child->child->total_number - 1;
                this.focus_info.child->child->node = list_entry(this.focus_info.child->child->head->list.prev, typeof(*this.focus_info.child->child->head), list);
            }
            else
            {
                this.focus_info.child->child->node = list_entry(this.focus_info.child->child->node->list.prev, typeof(*this.focus_info.child->child->node), list);
            }

            APP_FREE(this.play_info_ops.name);
            this.focus_info.child->child->active_item = this.focus_info.child->child->focus_item;
            this.play_info_ops.channel_num = this.focus_info.child->child->focus_item + 1;
            this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->child->node->name);

            GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
            GUI_SetProperty(WND_LIST_GROUP3, "select", &this.focus_info.child->child->focus_item);
            GUI_SetProperty(WND_LIST_GROUP3, "active", &this.focus_info.child->child->active_item);
        }
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _plugin_listview_play_up_keypress(void)
{
    if(this.first_play_init)
        return;
    GxCore_MutexLock(this.mutex_id);
    if(this.flag == UPDATE_CLASSIFICATION)
    {
        GxCore_MutexUnlock(this.mutex_id);
        return;
    }
    _no_program_state_hide();
    if(this.focus_info.child->child == NULL)
    {
        if(this.focus_info.child->total_number > 0 && strcmp("directory", this.focus_info.child->node->type) != 0)
        {
            _plugin_listview_play_show_gif();
            popdlg_destroy();
            this.press_play = 1;
            this.flag = UPDATE_SOURCE_URL;
            this.focus_info.child->focus_item += 1;
            if(this.focus_info.child->focus_item > (this.focus_info.child->total_number - 1))
            {
                this.focus_info.child->focus_item = 0;
                this.focus_info.child->node = list_entry(this.focus_info.child->head->list.next, typeof(*this.focus_info.child->head), list);
            }
            else
            {
                this.focus_info.child->node = list_entry(this.focus_info.child->node->list.next, typeof(*this.focus_info.child->node), list);
            }

            APP_FREE(this.play_info_ops.name);
            this.focus_info.child->active_item = this.focus_info.child->focus_item;
            this.play_info_ops.channel_num = this.focus_info.child->focus_item + 1;
            this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->node->name);

            GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
            GUI_SetProperty(WND_LIST_GROUP2, "select", &this.focus_info.child->focus_item);
            GUI_SetProperty(WND_LIST_GROUP2, "active", &this.focus_info.child->active_item);
        }
    }
    else
    {
        if(this.focus_info.child->child->total_number > 0 && strcmp("directory", this.focus_info.child->child->node->type) != 0)
        {
            _plugin_listview_play_show_gif();
            popdlg_destroy();
            this.press_play = 1;
            this.flag = UPDATE_SOURCE_URL;
            this.focus_info.child->child->focus_item += 1;
            if(this.focus_info.child->child->focus_item > (this.focus_info.child->child->total_number - 1))
            {
                this.focus_info.child->child->focus_item = 0;
                this.focus_info.child->child->node = list_entry(this.focus_info.child->child->head->list.next, typeof(*this.focus_info.child->child->head), list);
            }
            else
            {
                this.focus_info.child->child->node = list_entry(this.focus_info.child->child->node->list.next, typeof(*this.focus_info.child->child->node), list);
            }

            APP_FREE(this.play_info_ops.name);
            this.focus_info.child->child->active_item = this.focus_info.child->child->focus_item;
            this.play_info_ops.channel_num = this.focus_info.child->child->focus_item + 1;
            this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->child->node->name);

            GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
            GUI_SetProperty(WND_LIST_GROUP3, "select", &this.focus_info.child->child->focus_item);
            GUI_SetProperty(WND_LIST_GROUP3, "active", &this.focus_info.child->child->active_item);
        }
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _plugin_listview_play_info_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.play_status == PLAYER_STATUS_PLAY_RUNNING && this.play_info_ops.name != NULL)
    {
        this.play_info_ops.bar_title = "IPTV";
        this.play_info_ops.url = this.play_url;
        app_play_bar_info_show(&this.play_info_ops);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

SIGNAL_HANDLER int app_plugin_listview_play_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;

    if(this.flag == UPDATE_END)
        return EVENT_TRANSFER_STOP;

    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                _plugin_listview_play_exit_keypress();
                break;
            case STBK_OK:
                _plugin_listview_play_ok_keypress();
                break;
            case STBK_UP:
                _plugin_listview_play_up_keypress();
                break;
            case STBK_DOWN:
                _plugin_listview_play_down_keypress();
                break;
            case STBK_MUTE:
                _plugin_listview_play_mute_exec();
                _plugin_listview_play_mute_draw();
                break;
            case STBK_LEFT:
            case STBK_VOLDOWN:
                event->key.sym = APPK_VOLDOWN;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _plugin_listview_play_mute_draw();
                break;
            case STBK_RIGHT:
            case STBK_VOLUP:
                event->key.sym = APPK_VOLUP;
                GUI_CreateDialog(WND_VOLUME);
                GUI_SendEvent(WND_VOLUME, event);
                _plugin_listview_play_mute_draw();
                break;
            case STBK_PAGE_UP:
                break;
            case STBK_PAGE_DOWN:
                break;
            case STBK_INFO:
                _plugin_listview_play_info_keypress();
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview_play_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview_play_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static void app_plugin_listview_play_keyboard_proc(PopKeyboard *data)
{
    if(POP_VAL_CANCEL == data->in_ret || NULL == data->out_name)
        return;

    GxCore_MutexLock(this.mutex_id);
    strlcpy(this.search_str, data->out_name, MAX_FILE_NAME_LEN);
    this.search_enable = 1;
    if(this.focus_info.child == NULL)
    {
        this.focus_info.child = (struct ListFocusInfo*)GxCore_Calloc(1, sizeof(struct ListFocusInfo));
        if(this.focus_info.child == NULL)
        {
            GxCore_MutexUnlock(this.mutex_id);
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            return;
        }
        memset(this.focus_info.child, 0, sizeof(struct ListFocusInfo));
        this.focus_info.child->head = (ListUIDataList *)GxCore_Calloc(1, sizeof(ListUIDataList));
        if(this.focus_info.child->head == NULL)
        {
            APP_FREE(this.focus_info.child);
            GxCore_MutexUnlock(this.mutex_id);
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            return;
        }
        INIT_DLIST_HEAD(&(this.focus_info.child->head->list));
    }
    this.flag = UPDATE_SOURCE;
    GUI_SetProperty(WND_LIST_GROUP1, "item_active_image", LISTVIEW_UNFOCUS_IMG);
    GUI_SetProperty(WND_LIST_GROUP1, "active", NULL);
    GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
    GUI_SetProperty(WND_LIST_GROUP2, "state", "show");
    GUI_SetProperty(IMG_LIST_GROUP2_BACK, "state", "show");
    GUI_SetProperty(WND_LIST_GROUP2, "update_all", NULL);
    GUI_SetFocusWidget(WND_LIST_GROUP2);
    app_update_pop_dlg(WND_POP_TIP);
    GxCore_MutexUnlock(this.mutex_id);
}

static void app_plugin_listview_play_keyboard_create(void)
{
    static PopKeyboard keyboard;

    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = NULL;
    keyboard.max_num = 120;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = NULL;
    keyboard.release_cb = app_plugin_listview_play_keyboard_proc;
    keyboard.usr_data = NULL;
    keyboard.pos.x = 500;

    multi_language_keyboard_create(&keyboard);
}

static void _group_list1_ok_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.total_number > 0)
    {
        if(strcmp("search", this.focus_info.node->type) == 0)
        {
            app_plugin_listview_play_keyboard_create();
            GxCore_MutexUnlock(this.mutex_id);
            return;
        }
        APP_TIMER_ADD(thiz_tip_timer, _plugin_get_data_slow_tip, 30000, TIMER_REPEAT);
        if(this.focus_info.child == NULL)
        {
            this.focus_info.child = (struct ListFocusInfo*)GxCore_Calloc(1, sizeof(struct ListFocusInfo));
            if(this.focus_info.child == NULL)
            {
                GxCore_MutexUnlock(this.mutex_id);
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                return;
            }
            this.focus_info.child->active_item = -1;
            this.focus_info.child->head = (ListUIDataList *)GxCore_Calloc(1, sizeof(ListUIDataList));
            if(this.focus_info.child->head == NULL)
            {
                APP_FREE(this.focus_info.child);
                GxCore_MutexUnlock(this.mutex_id);
                PLUGIN_ERR("No memory to GxCore_Calloc\n");
                return;
            }
            INIT_DLIST_HEAD(&(this.focus_info.child->head->list));
        }
        this.flag = UPDATE_SOURCE;
        this.focus_info.active_item = this.focus_info.focus_item;
        GUI_SetProperty(WND_LIST_GROUP1, "item_active_image", LISTVIEW_UNFOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP1, "active", &this.focus_info.active_item);
        GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP2, "state", "show");
        GUI_SetProperty(IMG_LIST_GROUP2_BACK, "state", "show");
        GUI_SetProperty(WND_LIST_GROUP2, "update_all", NULL);
        GUI_SetFocusWidget(WND_LIST_GROUP2);
        app_update_pop_dlg(WND_POP_BOOK);
        app_update_pop_dlg(WND_POP_TIP);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list1_exit_keypress(void)
{
    APP_TIMER_REMOVE(thiz_tip_timer);
    GUI_SetProperty(WND_LIST_GROUP1, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP1_BACK, "state", "hide");
    _no_program_state_show();
}

static void _group_list1_up_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.total_number > 0)
    {
        this.focus_info.focus_item -=1;
        if(this.focus_info.focus_item < 0)
        {
            this.focus_info.focus_item = this.focus_info.total_number - 1;
            this.focus_info.node = list_entry(this.focus_info.head->list.prev, typeof(*this.focus_info.head), list);
        }
        else
        {
            this.focus_info.node = list_entry(this.focus_info.node->list.prev, typeof(*this.focus_info.node), list);
        }
        GUI_SetProperty(WND_LIST_GROUP1, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP1, "select", &this.focus_info.focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}


static void _group_list1_down_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.total_number > 0)
    {
        this.focus_info.focus_item += 1;
        if(this.focus_info.focus_item > (this.focus_info.total_number - 1))
        {
            this.focus_info.focus_item = 0;
            this.focus_info.node = list_entry(this.focus_info.head->list.next, typeof(*this.focus_info.head), list);
        }
        else
        {
            this.focus_info.node = list_entry(this.focus_info.node->list.next, typeof(*this.focus_info.node), list);
        }
        GUI_SetProperty(WND_LIST_GROUP1, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP1, "select", &this.focus_info.focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list1_page_up_keypress(void)
{
    ListUIDataNode *pos = NULL;
    int i = 0;
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.total_number >= MAX_PAGE_NUM)
    {
        if(this.focus_info.focus_item == 0)
        {
            this.focus_info.focus_item = this.focus_info.total_number - 1;
            this.focus_info.node = list_entry(this.focus_info.head->list.prev, typeof(*this.focus_info.head), list);
        }
        else if(this.focus_info.focus_item > 0 && this.focus_info.focus_item < MAX_PAGE_NUM)
        {
            this.focus_info.focus_item = 0;
            this.focus_info.node = list_entry(this.focus_info.head->list.next, typeof(*this.focus_info.head), list);
        }
        else
        {
            this.focus_info.focus_item -= MAX_PAGE_NUM;
            list_from_node_reverse(pos, &(this.focus_info.node->list), list, this.focus_info.head)
            {
                i++;
                if(i >= MAX_PAGE_NUM)
                    break;
            }
            this.focus_info.node = pos;
        }
        GUI_SetProperty(WND_LIST_GROUP1, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP1, "select", &this.focus_info.focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}


static void _group_list1_page_down_keypress(void)
{
    ListUIDataNode *pos = NULL;
    int i = 0;
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.total_number >= MAX_PAGE_NUM)
    {
        if(this.focus_info.focus_item == this.focus_info.total_number -1)
        {
            this.focus_info.focus_item = 0;
            this.focus_info.node = list_entry(this.focus_info.head->list.next, typeof(*this.focus_info.head), list);
        }
        else if((this.focus_info.focus_item < this.focus_info.total_number - 1)
                && (this.focus_info.focus_item >  this.focus_info.total_number - 1 - MAX_PAGE_NUM))
        {
            this.focus_info.focus_item = this.focus_info.total_number - 1;
            this.focus_info.node = list_entry(this.focus_info.head->list.prev, typeof(*this.focus_info.head), list);
        }
        else
        {
            this.focus_info.focus_item += MAX_PAGE_NUM;
            list_from_node(pos, &(this.focus_info.node->list), list, this.focus_info.head)
            {
                i++;
                if(i >= MAX_PAGE_NUM)
                    break;
            }
            this.focus_info.node = pos;
        }
        GUI_SetProperty(WND_LIST_GROUP1, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP1, "select", &this.focus_info.focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

SIGNAL_HANDLER int app_plugin_listview1_play_group_list_keypress(GuiWidget *widget, void *usrdata)
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
            case STBK_EXIT:
            case STBK_MENU:
                _group_list1_exit_keypress();
                break;
            case STBK_OK:
            case STBK_RIGHT:
                _group_list1_ok_keypress();
                break;
            case STBK_UP:
                _group_list1_up_keypress();
                break;
            case STBK_DOWN:
                _group_list1_down_keypress();
                break;
            case STBK_LEFT:
                break;
            case STBK_PAGE_UP:
                _group_list1_page_up_keypress();
                break;
            case STBK_PAGE_DOWN:
                _group_list1_page_down_keypress();
                break;
            default:
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview1_play_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    if(this.focus_info.total_number == 0)
        return 1; //for show wait msg
    else
        return this.focus_info.total_number;
}

SIGNAL_HANDLER int app_plugin_listview1_play_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;
    int i = 0;
    ListUIDataNode *pos = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return EVENT_TRANSFER_STOP;
    if(0 > item->sel)
        return EVENT_TRANSFER_STOP;

    while(GxCore_MutexTrylock(this.mutex_id) < 0)
    {
        GxCore_ThreadDelay(10);
    }

    if(this.focus_info.total_number == 0) //show wait msg
    {
        item->image = NULL;
        item->string = STR_ID_WAITING;
        GxCore_MutexUnlock(this.mutex_id);
        return EVENT_TRANSFER_STOP;
    }

    list_from_node(pos, &(this.focus_info.head->list), list, this.focus_info.head)
    {
        i++;
        if(i > item->sel)
            break;
    }
    item->string = pos->name;
    item->image = NULL;
    GxCore_MutexUnlock(this.mutex_id);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview1_play_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static void _group_list2_exit_keypress(void)
{
    APP_TIMER_REMOVE(thiz_tip_timer);
    GUI_SetProperty(WND_LIST_GROUP1, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP1_BACK, "state", "hide");
    GUI_SetProperty(WND_LIST_GROUP2, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP2_BACK, "state", "hide");
    _no_program_state_show();
}

static void _group_list2_ok_keypress(int32_t keycode)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->total_number > 0)
    {
        if(this.press_play == 1)
        {
            _group_list2_exit_keypress();
        }
        else if(this.press_play == 0 && strcmp("directory", this.focus_info.child->node->type) != 0)
        {
            if(STBK_RIGHT == keycode)
            {
                GxCore_MutexUnlock(this.mutex_id);
                return;
            }
            this.press_play = 1;
            this.flag = UPDATE_SOURCE_URL;
            APP_FREE(this.play_info_ops.name);
            this.play_info_ops.channel_num = this.focus_info.child->focus_item + 1;
            this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->node->name);
            this.focus_info.child->active_item = this.focus_info.child->focus_item;
            GUI_SetProperty(WND_LIST_GROUP2, "active", &this.focus_info.child->active_item);
            _plugin_listview_play_show_gif();
        }
        else
        {
            if(this.focus_info.child->child == NULL)
            {
                this.focus_info.child->child = (struct ListFocusInfo*)GxCore_Calloc(1, sizeof(struct ListFocusInfo));
                if(this.focus_info.child->child == NULL)
                {
                    GxCore_MutexUnlock(this.mutex_id);
                    PLUGIN_ERR("No memory to GxCore_Calloc\n");
                    return;
                }
                this.focus_info.child->child->active_item = -1;
                this.focus_info.child->child->head = (ListUIDataList *)GxCore_Calloc(1, sizeof(ListUIDataList));
                if(this.focus_info.child->child->head == NULL)
                {
                    APP_FREE(this.focus_info.child->child);
                    GxCore_MutexUnlock(this.mutex_id);
                    PLUGIN_ERR("No memory to GxCore_Calloc\n");
                    return;
                }
                INIT_DLIST_HEAD(&(this.focus_info.child->child->head->list));
            }

            APP_TIMER_ADD(thiz_tip_timer, _plugin_get_data_slow_tip, 30000, TIMER_REPEAT);
            this.flag = UPDATE_SOURCE_CHILD;
            this.focus_info.child->active_item = this.focus_info.child->active_item;
            GUI_SetProperty(WND_LIST_GROUP2, "item_active_image", LISTVIEW_UNFOCUS_IMG);
            GUI_SetProperty(WND_LIST_GROUP2, "active", &this.focus_info.child->active_item);
            GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
            GUI_SetProperty(WND_LIST_GROUP3, "state", "show");
            GUI_SetProperty(IMG_LIST_GROUP3_BACK, "state", "show");
            GUI_SetProperty(WND_LIST_GROUP3, "update_all", NULL);
            GUI_SetFocusWidget(WND_LIST_GROUP3);
            app_update_pop_dlg(WND_POP_BOOK);
            app_update_pop_dlg(WND_POP_TIP);
        }
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list2_left_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child != NULL)
    {
        _list_ui_data_free(&this.focus_info.child->head);
        APP_FREE(this.focus_info.child);
    }
    this.press_play = 0;
    this.flag = UPDATE_CLASSIFICATION;
    GUI_SetProperty(WND_LIST_GROUP2, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP2_BACK, "state", "hide");
    GUI_SetProperty(WND_LIST_GROUP1, "focus_img", LISTVIEW_FOCUS_IMG);
    GUI_SetFocusWidget(WND_LIST_GROUP1);
    GUI_SetProperty(WND_LIST_GROUP1, "update_all", NULL);
    GUI_SetProperty(WND_LIST_GROUP1, "active", &this.focus_info.active_item);
    _plugin_listview_play_hide_gif();
    app_update_pop_dlg(WND_POP_TIP);
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list2_up_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->total_number > 0)
    {
        this.press_play = 0;
        this.flag = UPDATE_SOURCE;
        this.focus_info.child->focus_item -= 1;
        if(this.focus_info.child->focus_item < 0)
        {
            this.focus_info.child->focus_item = this.focus_info.child->total_number - 1;
            this.focus_info.child->node = list_entry(this.focus_info.child->head->list.prev, typeof(*this.focus_info.child->head), list);
        }
        else
        {
            this.focus_info.child->node = list_entry(this.focus_info.child->node->list.prev, typeof(*this.focus_info.child->node), list);
        }
        GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP2, "select", &this.focus_info.child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}


static void _group_list2_down_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->total_number > 0)
    {
        if(this.focus_info.child->total_number >= MAX_COUNT
                && this.focus_info.child->focus_item == this.focus_info.child->total_number - 1)
        {
            _plugin_show_tip(STR_ID_NO_RESOURCE, 2);
            GxCore_MutexUnlock(this.mutex_id);
            return;
        }
        this.press_play = 0;
        this.flag = UPDATE_SOURCE;
        this.focus_info.child->focus_item += 1;
        if(this.focus_info.child->focus_item > (this.focus_info.child->total_number - 1))
        {
            this.focus_info.child->focus_item = 0;
            this.focus_info.child->node = list_entry(this.focus_info.child->head->list.next, typeof(*this.focus_info.child->head), list);
        }
        else
        {
            this.focus_info.child->node = list_entry(this.focus_info.child->node->list.next, typeof(*this.focus_info.child->node), list);
        }
        GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP2, "select", &this.focus_info.child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list2_page_up_keypress(void)
{
    ListUIDataNode *pos = NULL;
    int i = 0;

    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->total_number > MAX_PAGE_NUM)
    {
        this.press_play = 0;
        this.flag = UPDATE_SOURCE;
        if(this.focus_info.child->focus_item == 0)
        {
            this.focus_info.child->focus_item = this.focus_info.child->total_number - 1;
            this.focus_info.child->node = list_entry(this.focus_info.child->head->list.prev, typeof(*this.focus_info.child->head), list);
        }
        else if(this.focus_info.child->focus_item > 0 && this.focus_info.child->focus_item < MAX_PAGE_NUM)
        {
            this.focus_info.child->focus_item = 0;
            this.focus_info.child->node = list_entry(this.focus_info.child->head->list.next, typeof(*this.focus_info.child->head), list);
        }
        else
        {
            this.focus_info.child->focus_item -= MAX_PAGE_NUM;
            list_from_node_reverse(pos, &(this.focus_info.child->node->list), list, this.focus_info.child->head)
            {
                i++;
                if(i >= MAX_PAGE_NUM)
                    break;
            }
            this.focus_info.child->node = pos;
        }
        GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP2, "select", &this.focus_info.child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list2_page_down_keypress(void)
{
    ListUIDataNode *pos = NULL;
    int i = 0;

    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->total_number > MAX_PAGE_NUM)
    {
        if(this.focus_info.child->total_number >= MAX_COUNT
                && this.focus_info.child->focus_item == this.focus_info.child->total_number - 1)
        {
            _plugin_show_tip(STR_ID_NO_RESOURCE, 2);
            GxCore_MutexUnlock(this.mutex_id);
            return;
        }
        this.press_play = 0;
        this.flag = UPDATE_SOURCE;
        if(this.focus_info.child->focus_item == this.focus_info.child->total_number - 1)
        {
            this.focus_info.child->focus_item = 0;
            this.focus_info.child->node = list_entry(this.focus_info.child->head->list.next, typeof(*this.focus_info.child->head), list);
        }
        else if(this.focus_info.child->focus_item < this.focus_info.child->total_number - 1 && this.focus_info.child->focus_item > this.focus_info.child->total_number - 1 - MAX_PAGE_NUM)
        {
            this.focus_info.child->focus_item = this.focus_info.child->total_number - 1;
            this.focus_info.child->node = list_entry(this.focus_info.child->head->list.prev, typeof(*this.focus_info.child->head), list);
        }
        else
        {
            this.focus_info.child->focus_item += MAX_PAGE_NUM;
            list_from_node(pos, &(this.focus_info.child->node->list), list, this.focus_info.child->head)
            {
                i++;
                if(i >= MAX_PAGE_NUM)
                    break;
            }
            this.focus_info.child->node = pos;
        }
        GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP2, "select", &this.focus_info.child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

SIGNAL_HANDLER int app_plugin_listview2_play_group_list_keypress(GuiWidget *widget, void *usrdata)
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
            case STBK_EXIT:
            case STBK_MENU:
                _group_list2_exit_keypress();
                break;
            case STBK_OK:
            case STBK_RIGHT:
                _group_list2_ok_keypress(find_virtualkey_ex(event->key.scancode,event->key.sym));
                break;
            case STBK_UP:
                _group_list2_up_keypress();
                break;
            case STBK_DOWN:
                _group_list2_down_keypress();
                break;
            case STBK_LEFT:
                _group_list2_left_keypress();
                break;
            case STBK_PAGE_UP:
                _group_list2_page_up_keypress();
                break;
            case STBK_PAGE_DOWN:
                _group_list2_page_down_keypress();
                break;
            default:
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview2_play_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    if(this.focus_info.child == NULL || this.focus_info.child->total_number == 0)
        return 1; //for show wait msg
    else
        return this.focus_info.child->total_number;
}

SIGNAL_HANDLER int app_plugin_listview2_play_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    int i = 0;
    ListItemPara* item = NULL;
    static char buffer[CHANNEL_NUM_LEN];
    ListUIDataNode *pos = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return EVENT_TRANSFER_STOP;
    if(0 > item->sel)
        return EVENT_TRANSFER_STOP;

    while(GxCore_MutexTrylock(this.mutex_id) < 0)
    {
        GxCore_ThreadDelay(10);
    }

    if(this.focus_info.child == NULL || this.focus_info.child->total_number == 0)
    {
        item->x_offset = 2;
        item = item->next;
        item->image = NULL;
        item->string = STR_ID_WAITING;
        GxCore_MutexUnlock(this.mutex_id);
        return EVENT_TRANSFER_STOP;
    }

    sprintf(buffer, "%04d", (item->sel+1));
    item->image = NULL;
    item->string = buffer;
    item->x_offset = 2;
    item = item->next;
    item->image = NULL;
    list_from_node(pos, &(this.focus_info.child->head->list), list, this.focus_info.child->head)
    {
        i++;
        if(i > item->sel)
            break;
    }
    item->string = pos->name;
    GxCore_MutexUnlock(this.mutex_id);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview2_play_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static void _group_list3_exit_keypress(void)
{
    GUI_SetProperty(WND_LIST_GROUP1, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP1_BACK, "state", "hide");
    GUI_SetProperty(WND_LIST_GROUP2, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP2_BACK, "state", "hide");
    GUI_SetProperty(WND_LIST_GROUP3, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP3_BACK, "state", "hide");
    _no_program_state_show();
}

static void _group_list3_ok_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);

    if(this.focus_info.child->child->total_number > 0)
    {
        if(this.press_play == 1)
        {
            _group_list3_exit_keypress();
        }
        else if(this.press_play == 0 && strcmp("directory", this.focus_info.child->child->node->type) != 0)
        {
            this.press_play = 1;
            this.flag = UPDATE_SOURCE_URL;
            APP_FREE(this.play_info_ops.name);
            this.play_info_ops.channel_num = this.focus_info.child->child->focus_item + 1;
            this.play_info_ops.name = GxCore_Strdup(this.focus_info.child->child->node->name);
            this.focus_info.child->child->active_item = this.focus_info.child->child->focus_item;
            GUI_SetProperty(WND_LIST_GROUP3, "active", &this.focus_info.child->child->active_item);
            _plugin_listview_play_show_gif();
        }
    }

    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list3_left_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->child != NULL)
    {
        _list_ui_data_free(&this.focus_info.child->child->head);
        APP_FREE(this.focus_info.child->child);
    }
    this.press_play = 0;
    this.flag = UPDATE_SOURCE;
    GUI_SetProperty(WND_LIST_GROUP3, "state", "hide");
    GUI_SetProperty(IMG_LIST_GROUP3_BACK, "state", "hide");
    GUI_SetProperty(WND_LIST_GROUP2, "focus_img", LISTVIEW_FOCUS_IMG);
    GUI_SetFocusWidget(WND_LIST_GROUP2);
    GUI_SetProperty(WND_LIST_GROUP2, "update_all", NULL);
    GUI_SetProperty(WND_LIST_GROUP2, "active", &this.focus_info.child->active_item);//after update all, need reset active sel
    _plugin_listview_play_hide_gif();
    app_update_pop_dlg(WND_POP_TIP);
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list3_up_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->child->total_number > 0)
    {
        this.press_play = 0;
        this.flag = UPDATE_SOURCE_CHILD;
        this.focus_info.child->child->focus_item -= 1;
        if(this.focus_info.child->child->focus_item < 0)
        {
            this.focus_info.child->child->focus_item = this.focus_info.child->child->total_number - 1;
            this.focus_info.child->child->node = list_entry(this.focus_info.child->child->head->list.prev, typeof(*this.focus_info.child->child->head), list);
        }
        else
        {
            this.focus_info.child->child->node = list_entry(this.focus_info.child->child->node->list.prev, typeof(*this.focus_info.child->child->node), list);
        }
        GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP3, "select", &this.focus_info.child->child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list3_down_keypress(void)
{
    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->child->total_number > 0)
    {
        this.press_play = 0;
        this.flag = UPDATE_SOURCE_CHILD;
        this.focus_info.child->child->focus_item += 1;
        if(this.focus_info.child->child->focus_item > (this.focus_info.child->child->total_number - 1))
        {
            this.focus_info.child->child->focus_item = 0;
            this.focus_info.child->child->node = list_entry(this.focus_info.child->child->head->list.next, typeof(*this.focus_info.child->child->head), list);
        }
        else
        {
            this.focus_info.child->child->node = list_entry(this.focus_info.child->child->node->list.next, typeof(*this.focus_info.child->child->node), list);
        }
        GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP3, "select", &this.focus_info.child->child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list3_page_up_keypress(void)
{
    ListUIDataNode *pos = NULL;
    int i = 0;

    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->child->total_number > 0)
    {
        this.press_play = 0;
        this.flag = UPDATE_SOURCE_CHILD;
        this.focus_info.child->child->focus_item -= MAX_PAGE_NUM;
        if(this.focus_info.child->child->focus_item < 0)
        {
            this.focus_info.child->child->focus_item = this.focus_info.child->child->total_number - 1;
            this.focus_info.child->child->node = list_entry(this.focus_info.child->child->head->list.prev, typeof(*this.focus_info.child->child->head), list);
        }
        else
        {
            list_from_node_reverse(pos, &(this.focus_info.child->child->node->list), list, this.focus_info.child->child->head)
            {
                i++;
                if(i >= MAX_PAGE_NUM)
                    break;
            }
            this.focus_info.child->child->node = pos;
        }
        GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP3, "select", &this.focus_info.child->child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

static void _group_list3_page_down_keypress(void)
{
    ListUIDataNode *pos = NULL;
    int i = 0;

    GxCore_MutexLock(this.mutex_id);
    if(this.focus_info.child->child->total_number > 0)
    {
        this.press_play = 0;
        this.flag = UPDATE_SOURCE_CHILD;
        this.focus_info.child->child->focus_item += MAX_PAGE_NUM;
        if(this.focus_info.child->child->focus_item > (this.focus_info.child->child->total_number - 1))
        {
            this.focus_info.child->child->focus_item = 0;
            this.focus_info.child->child->node = list_entry(this.focus_info.child->child->head->list.next, typeof(*this.focus_info.child->child->head), list);
        }
        else
        {
            list_from_node(pos, &(this.focus_info.child->child->node->list), list, this.focus_info.child->child->head)
            {
                i++;
                if(i >= MAX_PAGE_NUM)
                    break;
            }
            this.focus_info.child->child->node = pos;
        }
        GUI_SetProperty(WND_LIST_GROUP3, "focus_img", LISTVIEW_FOCUS_IMG);
        GUI_SetProperty(WND_LIST_GROUP3, "select", &this.focus_info.child->child->focus_item);
    }
    GxCore_MutexUnlock(this.mutex_id);
}

SIGNAL_HANDLER int app_plugin_listview3_play_group_list_keypress(GuiWidget *widget, void *usrdata)
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
            case STBK_EXIT:
            case STBK_MENU:
                _group_list3_exit_keypress();
                break;
            case STBK_OK:
                _group_list3_ok_keypress();
                break;
            case STBK_UP:
                _group_list3_up_keypress();
                break;
            case STBK_DOWN:
                _group_list3_down_keypress();
                break;
            case STBK_LEFT:
                _group_list3_left_keypress();
                break;
            case STBK_RIGHT:
                break;
            case STBK_PAGE_UP:
                _group_list3_page_up_keypress();
                break;
            case STBK_PAGE_DOWN:
                _group_list3_page_down_keypress();
                break;
            default:
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview3_play_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    if(this.focus_info.child == NULL || this.focus_info.child->child == NULL || this.focus_info.child->child->total_number == 0)
        return 1; //for show wait msg
    else
        return this.focus_info.child->child->total_number;
}

SIGNAL_HANDLER int app_plugin_listview3_play_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    int i = 0;
    ListItemPara* item = NULL;
    static char buffer[CHANNEL_NUM_LEN];
    ListUIDataNode *pos = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return EVENT_TRANSFER_STOP;
    if(0 > item->sel)
        return EVENT_TRANSFER_STOP;

    while(GxCore_MutexTrylock(this.mutex_id) < 0)
    {
        GxCore_ThreadDelay(10);
    }

    if(this.focus_info.child == NULL || this.focus_info.child->child == NULL || this.focus_info.child->child->total_number == 0)
    {
        item->x_offset = 2;
        item = item->next;
        item->image = NULL;
        item->string = STR_ID_WAITING;
        GxCore_MutexUnlock(this.mutex_id);
        return EVENT_TRANSFER_STOP;
    }

    sprintf(buffer, "%02d", (item->sel+1));
    item->image = NULL;
    item->string = buffer;
    item->x_offset = 2;
    item = item->next;
    item->image = NULL;
    list_from_node(pos, &(this.focus_info.child->child->head->list), list, this.focus_info.child->child->head)
    {
        i++;
        if(i > item->sel)
            break;
    }
    item->string = pos->name;
    GxCore_MutexUnlock(this.mutex_id);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_listview3_play_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static void _plugin_listview_play_ui_contral(struct PlugiaSourceUIMsg *msg)
{
    GxMsgProperty_PlayerPlay* video_play = NULL;

    if(msg == NULL)
        return;

    switch(msg->type)
    {
        case MSG_DIG_END:
            {
                popdlg_destroy();
                GUI_EndDialog(WND_PLUGIN_LISTVIEW_PLAY);
                break;
            }
        case MSG_SHOW_TIP:
            {
                if(GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY) == GXCORE_SUCCESS)
                {
                    _plugin_listview_play_hide_gif();
                    _plugin_listview_play_popdlg(msg->status, 0);
                }
                break;
            }
        case MSG_DIG_UPDATE:
            {
                if(msg->item == 1)
                {
                    GUI_SetProperty(WND_LIST_GROUP1, "update_all", NULL);
                    if(msg->active != -1)
                        GUI_SetProperty(WND_LIST_GROUP1, "active", &msg->active);
                }
                else if(msg->item == 2)
                {
                    GUI_SetProperty(WND_LIST_GROUP2, "update_all", NULL);
                    if(msg->active != -1)
                        GUI_SetProperty(WND_LIST_GROUP2, "active", &msg->active);
                }
                else if(msg->item == 3)
                {
                    GUI_SetProperty(WND_LIST_GROUP3, "update_all", NULL);
                    if(msg->active != -1)
                        GUI_SetProperty(WND_LIST_GROUP3, "active", &msg->active);
                }
                app_update_pop_dlg(WND_POP_BOOK);
                app_update_pop_dlg(WND_POP_TIP);
                break;
            }
        case MSG_PLAY:
            {
                if(msg->status != NULL)
                {
                    PLUGIN_INFO("play url:%s", msg->status);
                    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
                    video_play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
                    if(video_play == NULL)
                    {
                        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
                        return;
                    }
                    strlcpy(video_play->url, msg->status, sizeof(video_play->url));

                    GxCore_MutexLock(this.mutex_id);
                    memset(this.play_url, 0, sizeof(this.play_url));
                    strlcpy(this.play_url, msg->status, sizeof(this.play_url));
                    GxCore_MutexUnlock(this.mutex_id);

                    _plugin_play_info_save(video_play->url, this.play_info_ops.name, this.play_info_ops.channel_num);
                    APP_FREE(msg->status);
                    video_play->player = PLAYER_FOR_IPTV;
                    app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)video_play);
                    GXCORE_FREE(video_play);
                }
                else
                {
                    _plugin_listview_play_popdlg(STR_ID_PLAY_URL_NULL, 1);
                }
            }
        default:
            break;
    }
}

static int _plugin_listview_play_status(GxMessage* msg)
{
    GxMsgProperty_PlayerStatusReport* player_status = NULL;
    player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
    char state[5];

    if((player_status == NULL) || (strcmp((char*)(player_status->player), PLAYER_FOR_IPTV) != 0))
    {
        return EVENT_TRANSFER_STOP;
    }

    switch(player_status->status)
    {
        case PLAYER_STATUS_ERROR:
            _plugin_listview_play_hide_gif();
            PLUGIN_INFO("PLAYER_STATUS_ERROR, %d", player_status->error);
            GxCore_MutexLock(this.mutex_id);
            if(PLAYER_ERROR_SOURCE_RESTART == player_status->error)
            {
                GxMsgProperty_PlayerPlay* video_play = NULL;
                video_play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
                if(video_play == NULL)
                {
                    app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
                    return GXCORE_ERROR;
                }
                strlcpy(video_play->url, this.play_url, sizeof(video_play->url));
                PLUGIN_INFO("play restart url: %s", this.play_url);
                GxCore_MutexUnlock(this.mutex_id);
                GxPlayer_MediaStop(PLAYER_FOR_IPTV);
                video_play->player = PLAYER_FOR_IPTV;
                app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)video_play);
                GXCORE_FREE(video_play);
            }
            else
            {
                this.play_status = PLAYER_STATUS_ERROR;
                this.press_play = 0;
                GxCore_MutexUnlock(this.mutex_id);
                _plugin_listview_play_popdlg(STR_ID_PLAY_ERROR, 1);
            }
            break;

        case PLAYER_STATUS_PLAY_END:
            _plugin_listview_play_hide_gif();
            _plugin_listview_play_popdlg(STR_ID_PLAY_END, 1);
            PLUGIN_INFO("PLAYER_STATUS_PLAY_END");
            GxCore_MutexLock(this.mutex_id);
            this.play_status = PLAYER_STATUS_PLAY_END;
            this.press_play = 0;
            GxCore_MutexUnlock(this.mutex_id);
            break;

        case PLAYER_STATUS_PLAY_RUNNING:
            _plugin_listview_play_hide_gif();
            PLUGIN_INFO("PLAYER_STATUS_PLAY_RUNNING");
            GxCore_MutexLock(this.mutex_id);
            this.play_status = PLAYER_STATUS_PLAY_RUNNING;
            GUI_GetProperty(WND_LIST_GROUP1, "state", &state);
            if(strcmp(state,"hide") == 0)
            {
                if(this.play_info_ops.name != NULL)
                {
                    this.play_info_ops.bar_title = "IPTV";
                    this.play_info_ops.url = this.play_url;
                    app_play_bar_info_show(&this.play_info_ops);
                }
            }
            GxCore_MutexUnlock(this.mutex_id);
            break;

        case PLAYER_STATUS_PLAY_START:
            GxCore_MutexLock(this.mutex_id);
            _no_program_state_hide();
            this.play_status = PLAYER_STATUS_PLAY_START;
            GxCore_MutexUnlock(this.mutex_id);
            PLUGIN_INFO("PLAYER_STATUS_PLAY_START");
            break;

        case PLAYER_STATUS_STOPPED:
            _plugin_listview_play_hide_gif();
            PLUGIN_INFO("PLAYER_STATUS_STOPPED");
            break;

        case PLAYER_STATUS_START_FILL_BUF:
            _plugin_listview_play_show_gif();
            PLUGIN_INFO("PLAYER_STATUS_START_FILL_BUF");
            break;

        case PLAYER_STATUS_END_FILL_BUF:
            _plugin_listview_play_hide_gif();
            PLUGIN_INFO("PLAYER_STATUS_END_FILL_BUF");
            break;

        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

int app_plugin_listview_play_msg_proc(GxMessage *msg)
{
    struct PlugiaSourceUIMsg *ui_msg;

    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    switch(msg->msg_id)
    {
        case APPMSG_PLUGIN_SOURCE_UI_CONTROL:
            {
                ui_msg = GxBus_GetMsgPropertyPtr(msg, struct PlugiaSourceUIMsg);
                _plugin_listview_play_ui_contral(ui_msg);
                break;
            }
        case GXMSG_PLAYER_STATUS_REPORT:
            {
                _plugin_listview_play_status(msg);
                break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

void app_plugin_listview_play_deal_err_from_ecmascript(void)
{
    if(true == this.wnd_created && false == this.exit_flag)
        _plugin_listview_play_show_tip(STR_ID_PLUGIN_ERR);
}

#endif
