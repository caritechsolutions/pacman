/*************************************************************************
	> File Name: app_plugin_source.c
	> Author:
	> Mail:
	> Created Time: 2016年08月26日 星期五 14时19分54秒
 ************************************************************************/

#include "app.h"
#if PLUGINSTORE_SUPPORT
#include "gx_pluginstore_api.h"
#include "app_plugin_server.h"
#include "app_plugin_update.h"
#include "gxos/gxcore_os_core.h"
#include "../app_netvideo_play.h"
#include "jansson.h"
#include "gx_apps.h"
#include "../app_netapps_service.h"
#include "app_keyboard_language.h"
#include "app_windows.h"

#define WND_PLUGIN_SOURCE_PAGE_INFO    "text_plugin_source_page_info"
#define WND_PLUGIN_SOURCE_TOTAL_INFO   "text_plugin_source_total_info"
#define WND_PLUGIN_SOURCE_TITLE        "text_plugin_source_title"
#define WND_LIST_GROUP                 "list_plugin_source_group"
#define WND_PLUGIN_SOURCE_NAME         "text_plugin_source_item_name"
#define WND_PLUGIN_SOURCE_SHORTDESC    "text_plugin_source_shortdesc"
#define WND_PLUGIN_LISTVIEW_TITLE      "text_plugin_source_listview"
#define IMG_GIF                        "img_plugin_source_gif"
#define TXT_POPUP                      "txt_plugin_source_popup"
#define IMG_POPUP                      "img_plugin_source_popup"

#define GIF_PATH                       WORK_PATH"theme/image/netapps/loading.gif"
#define IMG_DEFAULT                    WORK_PATH"theme/image/netapps/default.png"
#define IMG_MUSIC                      WORK_PATH"theme/image/netapps/music_back.jpg"
#define MAX_PAGE_ITEM                  (8)
#define MAX_PAGE_LINE                  (2)
#define MAX_FILE_NAME_LEN              (128)
#define IMAGE_DOWN_PATH                TMP_DOWNLOAD_PATH
#define STR_SHORTDESC                  "Description"
#define UI_TYPE_SIGNLE                 "signle"
#define LISTVIEW_FOCUS_IMG             "feeds_item"
#define LISTVIEW_UNFOCUS_IMG           "feeds_itembg_sel"
#define TXT_BACKCOLOR_UNFOCUS   "[net_bg_color,net_bg_color,net_bg_color]"
#define TXT_FORECOLOR_FOCUS     "[net_video_focus_color,net_video_focus_color,net_video_focus_color]"
#define TXT_FOCUS_COLOR         "[net_title_focus_color,net_title_focus_color,net_title_focus_color]"
#define TXT_UNFOCUS_COLOR       "[net_title_unfocus_color,net_title_unfocus_color,net_title_unfocus_color]"
#define PLUGIN_STR_BLANK        " "
#define PLUGIN_IMG_NULL         "null"
#define MAX_COUNT                      (1000)

enum UPDATE_STATUS{
    UPDATE_CLASSIFICATION,
    UPDATE_CLASSIFICATION_OK,
    UPDATE_SOURCE,
    UPDATE_SOURCE_CHILD,
    UPDATE_SOURCE_URL,
    UPDATE_SOURCE_PLAY,
    UPDATE_END,
};

enum MSG_CONTROL_STATUS{
    MSG_UPDATE_ITEM,
    MSG_CLEAN_ITEM,
    MSG_UPDATE_ITEM_IMAGE,
    MSG_FOCUS_ITEM,
    MSG_UNFOCUS_ITEM,
    MSG_UNFOCUS_ALL_ITEM,
    MSG_UPDATE_PAGE_INFO,
    MSG_GROUP_LIST_FOCUS,
    MSG_GROUP_LIST_UNFOCUS,
    MSG_GROUP_LIST_SHOW,
    MSG_GROUP_LIST_HIDE,
    MSG_GROUP_LIST_UPDATE,
    MSG_SOURCE_DIG_END,
    MSG_PLAY_DIG_CREAT,
    MSG_PLAY_DIG_END,
    MSG_SHOW_GIF,
    MSG_HIDE_GIF,
    MSG_SHOW_POPUP,
    MSG_HIDE_POPUP,
    MSG_PLAY,
    MSG_CREATE_KEYBOARD,
};

struct SourceFocusInfo{
    int total_number;
    int focus_item;
    int old_item;
    int child_index;
    SourceInfoNode *node;
    struct SourceFocusInfo *child;
};

struct ListFocusInfo{
    int total_number;
    int focus_item;
    char *search_str;
    int search_enable;
    int update_flag;
    bool book_flag;
    int parent_update_flag;
    int init_abort;
    int wait_abort;
    int gif_status;
    int pic_download_abort;
    handle_t mutex_id;
    struct SourceFocusInfo source;
    ClassificationNode *node;
    ClassificationList *head;
};

static handle_t list_update_thread_handle = -1;
static handle_t item_image_update_thread_handle[MAX_PAGE_ITEM] = {-1};
static struct ListFocusInfo this_focus_info;
static char *this_pic_path[MAX_PAGE_ITEM] = {NULL};
static event_list* s_plugin_source_gif_timer = NULL;
static int app_exit_abort = 0;
static int play_exit_abort = 0;
static int mutex_group_list_exit_id = 0;
static char *source_url = NULL;
static bool pop_msg_on = false;
static bool thiz_wnd_created = false;
static event_list* s_plugin_source_popup_timer = NULL;
static event_list* s_plugin_source_timeout_monitor_timer = NULL;
static bool exit_only_once = false;
extern PluginInfoNode *Pluginstore_focus_node_get(void);
extern int gdi_lock(void);
extern int gdi_unlock(void);
static struct SourceFocusInfo * _source_child_find_parent_node(void);
static int _plugin_source_info_draw(int mode);
static int _source_child_list_init(void);
static void app_plugin_source_hide_gif(void);
static void app_plugin_source_show_gif(void);
static void plugin_source_exit_abort(void);
static bool _plugin_check_err_type(void);

static char *s_text_source_title[MAX_PAGE_ITEM] =
{
    "text_plugin_source_item_title1",
    "text_plugin_source_item_title2",
    "text_plugin_source_item_title3",
    "text_plugin_source_item_title4",
    "text_plugin_source_item_title5",
    "text_plugin_source_item_title6",
    "text_plugin_source_item_title7",
    "text_plugin_source_item_title8",
};

static char *s_text_source_focus[MAX_PAGE_ITEM] =
{
    "text_plugin_source_item_focus1",
    "text_plugin_source_item_focus2",
    "text_plugin_source_item_focus3",
    "text_plugin_source_item_focus4",
    "text_plugin_source_item_focus5",
    "text_plugin_source_item_focus6",
    "text_plugin_source_item_focus7",
    "text_plugin_source_item_focus8",
};

static char *s_img_source_unfocus[MAX_PAGE_ITEM] =
{
    "img_plugin_source_item_unfocus1",
    "img_plugin_source_item_unfocus2",
    "img_plugin_source_item_unfocus3",
    "img_plugin_source_item_unfocus4",
    "img_plugin_source_item_unfocus5",
    "img_plugin_source_item_unfocus6",
    "img_plugin_source_item_unfocus7",
    "img_plugin_source_item_unfocus8",
};

static char *key_img_default= "plugin_source_default_key";

static char *key_img[MAX_PAGE_ITEM]=
{
    "plugin_source_img_key1",
    "plugin_source_img_key2",
    "plugin_source_img_key3",
    "plugin_source_img_key4",
    "plugin_source_img_key5",
    "plugin_source_img_key6",
    "plugin_source_img_key7",
    "plugin_source_img_key8",
};

static void _plugin_source_update_page_info(int index)
{
    struct PlugiaSourceUIMsg param = {0};
    struct SourceFocusInfo *parent_node = NULL;
    SourceInfoNode *tail_node = NULL;

    if(index >= 0)
    {
        param.type = MSG_UPDATE_PAGE_INFO;
        param.item = index;
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node = _source_child_find_parent_node();
            param.total = parent_node->child->total_number;
            tail_node = list_entry(parent_node->node->child_list->list.prev, typeof(*(parent_node->child->node)), list);
        }
        else
        {
            param.total = this_focus_info.source.total_number;
            tail_node = list_entry(this_focus_info.node->source_list_head->list.prev, typeof(*(parent_node->child->node)), list);
        }

        if(tail_node->extra_data != NULL &&!strncmp(tail_node->extra_data,"total",5))
            param.node.title = GxCore_Strdup(tail_node->extra_data);
        else
            param.node.title = NULL;

        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    }
}

static void _app_plugin_source_hide_popup_msg(void)
{
    if(s_plugin_source_popup_timer)
    {
        remove_timer(s_plugin_source_popup_timer);
        s_plugin_source_popup_timer = NULL;
    }

    pop_msg_on = false;
    GUI_SetProperty(TXT_POPUP, "state", "hide");
    GUI_SetProperty(IMG_POPUP, "state", "hide");
    if(this_focus_info.gif_status)
        app_plugin_source_show_gif();
}

static void app_plugin_source_hide_popup_msg(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_HIDE_POPUP;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static int _plugin_source_popup_timer_timeout(void *userdata)
{
    if(s_plugin_source_popup_timer)
    {
        s_plugin_source_popup_timer = NULL;
    }
    app_plugin_source_hide_popup_msg();
    return 0;
}

static void _app_plugin_source_show_popup_msg(char* pMsg, int time_out)
{
    if(true == this_focus_info.book_flag)
        return;
    pop_msg_on = true;
    if(this_focus_info.gif_status)
        app_plugin_source_hide_gif();
    GUI_SetInterface("flush", NULL);
    GUI_SetProperty(TXT_POPUP, "string", pMsg);
    GUI_SetProperty(IMG_POPUP, "state", "show");
    GUI_SetProperty(TXT_POPUP, "state", "show");

    if(time_out > 0)
    {
        if (reset_timer(s_plugin_source_popup_timer) != 0)
        {
            s_plugin_source_popup_timer = create_timer(_plugin_source_popup_timer_timeout, time_out, NULL, TIMER_ONCE);
        }
    }
}

static void app_plugin_source_show_popup_msg(char* pMsg, int time_out)
{
    struct PlugiaSourceUIMsg param = {0};

    if(NULL == pMsg || time_out < 0)
        return;

    param.type = MSG_SHOW_POPUP;
    param.status = pMsg;
    param.item = time_out;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static int _plugin_timeout_popup_draw(void *userdata)
{
    if(!pop_msg_on)
        app_plugin_source_show_popup_msg("Getting data is slow,  please wait or try again", 0);
    return 0;
}

static void plugin_timeout_monitor_start(int time_out)
{
    if (reset_timer(s_plugin_source_timeout_monitor_timer) != 0)
        s_plugin_source_timeout_monitor_timer = create_timer(_plugin_timeout_popup_draw, time_out, NULL, TIMER_REPEAT);
}

static void plugin_timeout_monitor_stop(void)
{
    if(s_plugin_source_timeout_monitor_timer)
        timer_stop(s_plugin_source_timeout_monitor_timer);
}

static void plugin_timeout_monitor_delete(void)
{
    if(s_plugin_source_timeout_monitor_timer)
    {
        remove_timer(s_plugin_source_timeout_monitor_timer);
        s_plugin_source_timeout_monitor_timer = NULL;
    }
}

static int app_plugin_source_draw_gif(void* usrdata)
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
    APP_TIMER_REMOVE(s_plugin_source_gif_timer);
    s_plugin_source_gif_timer = NULL;
    GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void app_plugin_source_show_gif(void)
{
    this_focus_info.gif_status = 1;
    GUI_SetProperty(IMG_GIF, "state", "show");
    if(0 != reset_timer(s_plugin_source_gif_timer))
        s_plugin_source_gif_timer = create_timer(app_plugin_source_draw_gif, 100, NULL, TIMER_REPEAT);
}

static void app_plugin_source_hide_gif(void)
{
    if(!pop_msg_on)
        this_focus_info.gif_status = 0;
    APP_TIMER_REMOVE(s_plugin_source_gif_timer);
    GUI_SetProperty(IMG_GIF, "state", "hide");
}

static void _plugin_source_focus_item(int index)
{
    struct PlugiaSourceUIMsg param = {0};
    struct SourceFocusInfo *parent_node = NULL;

    if(index >= 0)
    {
        param.type = MSG_FOCUS_ITEM;
        param.item = index;
        param.node.title = this_focus_info.source.node->title;
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node = _source_child_find_parent_node();
            if(parent_node->child->node != NULL)
                param.node.shortdesc = parent_node->child->node->shortdesc;
        }
        else
            param.node.shortdesc = NULL;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    }
}

static void _plugin_source_unfocus_item(int index)
{
    struct PlugiaSourceUIMsg param = {0};

    if(index >= 0)
    {
        param.type = MSG_UNFOCUS_ITEM;
        param.item = index;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    }
}

static void _plugin_source_unfocus_all_item(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_UNFOCUS_ALL_ITEM;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_show_state(void)
{
    struct PlugiaSourceUIMsg param = {0};

    if(true == this_focus_info.book_flag || true == pop_msg_on)
        return;
    this_focus_info.wait_abort = 1;
    param.type = MSG_SHOW_GIF;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static void _plugin_source_hide_state(void)
{
    struct PlugiaSourceUIMsg param = {0};

    this_focus_info.wait_abort = 0;
    param.type = MSG_HIDE_GIF;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static void _plugin_source_clean_item(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_CLEAN_ITEM;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static void _plugin_source_create_keyboard(void)
{
    struct PlugiaSourceUIMsg param = {0};
    param.type = MSG_CREATE_KEYBOARD;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static void _plugin_source_group_list_update(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_GROUP_LIST_UPDATE;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_group_list_show(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_GROUP_LIST_SHOW;
    param.item = this_focus_info.focus_item;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_group_list_hide(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_GROUP_LIST_HIDE;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_group_list_focus(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_GROUP_LIST_FOCUS;
    param.item = this_focus_info.focus_item;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_group_list_unfocus(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_GROUP_LIST_UNFOCUS;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_play_dig_create(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_PLAY_DIG_CREAT;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_dig_end(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_SOURCE_DIG_END;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    return;
}

static void _plugin_source_play(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_PLAY;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_source_play_get_url(int source_index)
{
    char *url;

    url = source_url_json_parse(source_url);
    if(url == NULL)
        return;
    PLUGIN_INFO("play url:%s", url);
    app_net_video_start_play(url, PLAY_STATE_LOAD, 0, false);
    APP_FREE(url);
}

static void _plugin_source_play_restart_ctrl(uint64_t cur_time)
{
    char *url;

    url = source_url_json_parse(source_url);
    if(url == NULL)
        return;

    PLUGIN_INFO("play url:%s", url);
    app_net_video_start_play(url, PLAY_STATE_LOAD, cur_time, false);
    APP_FREE(url);
}

static int _plugin_source_play_list_total_get_ctrl(void)
{
    struct SourceFocusInfo *parent_node = NULL;
    int total = 0;
    SourceInfoNode *pos;
    SourceInfoList *head;

    if(this_focus_info.parent_update_flag == UPDATE_SOURCE)
    {
        head = this_focus_info.node->source_list_head;
    }
    else if(this_focus_info.parent_update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        head = parent_node->node->child_list;
    }
    else
        return 0;

    list_from_node(pos, &(head->list), list, head)
    {
        if(strcmp("video", pos->type) == 0)
        {
            total++;
        }
    }
    return total;
}

static char *_plugin_source_play_list_data_get_ctrl(int sel)
{
    struct SourceFocusInfo *parent_node = NULL;
    SourceInfoNode *pos;
    SourceInfoList *head;
    int i = 0;

    if(this_focus_info.parent_update_flag == UPDATE_SOURCE)
    {
        head = this_focus_info.node->source_list_head;
    }
    else if(this_focus_info.parent_update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        head = parent_node->node->child_list;
    }
    else
        return NULL;
    list_from_node(pos, &(head->list), list, head)
    {
        if(strcmp("video", pos->type) != 0)
        {
            continue;
        }
        i++;
        if(i > sel)
            break;
    }

    if(i != 0)
        return pos->title;
    else
        return NULL;
}

static void _plugin_source_play_list_info_get(char **prog_name, int *focus_item)
{
    struct SourceFocusInfo *parent_node = NULL;
    SourceInfoNode *pos;
    SourceInfoNode *node;
    SourceInfoList *head;
    int i = 0;

    if(this_focus_info.parent_update_flag == UPDATE_SOURCE)
    {
        head = this_focus_info.node->source_list_head;
        node = this_focus_info.source.node;
    }
    else if(this_focus_info.parent_update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        head = parent_node->node->child_list;
        node = parent_node->child->node;
    }
    else
        return;
    list_from_node(pos, &(head->list), list, head)
    {
        if(strcmp("video", pos->type) != 0)
        {
            continue;
        }
        if(pos == node)
            break;
        i++;
    }
    if(prog_name != NULL && *prog_name == NULL)
    {
        *prog_name = pos->title;
    }

    if(focus_item != NULL)
        *focus_item = i;
}

static int _plugin_source_play_list_ok(int item)
{
    struct SourceFocusInfo *parent_node = NULL;
    SourceInfoNode *pos;
    SourceInfoList *head;
    int i = 0, j = 0;
    int old_item;

    _plugin_source_play_list_info_get(NULL, &old_item);
    if(old_item == item)
        return 0;

    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return 0;
    if(this_focus_info.parent_update_flag == UPDATE_SOURCE)
    {
        head = this_focus_info.node->source_list_head;
    }
    else if(this_focus_info.parent_update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        head = parent_node->node->child_list;
    }
    else
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return -1;
    }

    list_from_node(pos, &(head->list), list, head)
    {
        j++;
        if(strcmp("video", pos->type) != 0)
        {
            continue;
        }
        if(i == item)
            break;
        i++;
    }
    if(i == item)
    {
        Source_url_refresh_close();
        Source_url_get(pos->counter);
        this_focus_info.update_flag = UPDATE_SOURCE_URL;
        if(this_focus_info.parent_update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node->child->focus_item = j - 1;
            parent_node->child->node = pos;
        }
        else if(this_focus_info.parent_update_flag == UPDATE_SOURCE)
        {
            this_focus_info.source.focus_item = j - 1;
            this_focus_info.source.node = pos;
        }
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return 0;
    }
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    return -1;
}

static void _plugin_source_play_prev(void)
{
    int list_focus_item = 0;

    _plugin_source_play_list_info_get(NULL, &list_focus_item);
    list_focus_item--;
    if(list_focus_item < 0)
        list_focus_item = _plugin_source_play_list_total_get_ctrl() - 1;
    _plugin_source_play_list_ok(list_focus_item);
}

static status_t _plugin_source_play_next(int force)
{
    int list_focus_item = 0;

    _plugin_source_play_list_info_get(NULL, &list_focus_item);
    list_focus_item++;
    if(list_focus_item > _plugin_source_play_list_total_get_ctrl() - 1)
        list_focus_item = 0;
    _plugin_source_play_list_ok(list_focus_item);
    return GXCORE_SUCCESS;
}

static status_t app_plugin_source_play(char *image_url)
{
    NetVideoPlayOps play_ops;
    memset(&play_ops, 0, sizeof(NetVideoPlayOps));

    play_ops.get_url             = _plugin_source_play_get_url;
    play_ops.play_wnd_created    = true;
    play_ops.source_num = 1;
    _plugin_source_play_list_info_get(&play_ops.netvideo_title, &play_ops.list_focus_item);
    play_ops.play_ctrl.play_ok   = NULL;
    play_ops.play_ctrl.play_exit = NULL;
    play_ops.play_ctrl.play_prev = _plugin_source_play_prev;
    play_ops.play_ctrl.play_next = _plugin_source_play_next;
    play_ops.play_ctrl.play_restart = _plugin_source_play_restart_ctrl;
    play_ops.play_ctrl.play_list_total_get = _plugin_source_play_list_total_get_ctrl;
    play_ops.play_ctrl.play_list_data_get = _plugin_source_play_list_data_get_ctrl;
    play_ops.play_ctrl.play_list_ok = _plugin_source_play_list_ok;
    play_ops.netvideo_image = image_url;
    return app_net_video_play(&play_ops);
}

static char * image_url_json_parse(char *string)
{
    json_error_t error;
    json_t *object = NULL;
    const char *obj_key = NULL;
    json_t *obj_value = NULL;
    json_t *array_value = NULL;
    const char *info_key = NULL;
    json_t *info_value = NULL;
    char *url = NULL;
    int index = 0;

    if(strncasecmp(string, "http", 4) == 0)
    {
        url = GxCore_Strdup(string);
        return url;
    }
    else
    {
        while(*(string++) != ':' && *string != '\0');
        object = json_loads(string, JSON_DISABLE_EOF_CHECK, &error);
        if(json_is_array(object))
        {
            while(1)
            {
                array_value = json_array_get(object, index);
                if(array_value == NULL || !json_is_object(array_value))
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
        }
        else if(json_is_object(object))
        {
            json_object_foreach(object, obj_key, obj_value)
            {
                if(strcmp(obj_key, "default") == 0)
                {
                    json_object_foreach(obj_value, info_key, info_value)
                    {
                        if(strcmp(info_key, "url") == 0)
                        {
                            url = GxCore_Strdup(json_string_value(info_value));
                            json_decref(object);
                            return url;
                        }
                    }
                }
            }
        }
        else
        {
            json_decref(object);
            return NULL;
        }
    }
    return NULL;
}

char *get_fname_suffix_from_url(char *url)
{
    char *p = url;
    int i = 0;

    while(*p)
    {
        i++;
        p++;
    }

    while(i--) {
        if(*p == '.')
            break;
        else
            p--;
    }
    if(i == 0)
        return ".png";
    if(strncasecmp(p, ".bmp", 4) == 0)
        return ".bmp";
    else if(strncasecmp(p, ".jpg", 4) == 0)
        return ".jpg";
    else if(strncasecmp(p, ".jpe", 4) == 0)
        return ".jpeg";
    else if(strncasecmp(p, ".gif", 4) == 0)
        return ".gif";
    else if(strncasecmp(p, ".ico", 4) == 0)
        return ".ico";
    else
        return ".png";
}

static void _pic_download_cb(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
    struct PlugiaSourceUIMsg param = {0};
    int i = 0;

    if(message == GXCURL_STATE_COMPLETE)
    {
        if(this_focus_info.pic_download_abort)
            return;
        if(callback_data->handle > 0)
        {
            for(i = 0; i < MAX_PAGE_ITEM; i++)
            {
                if(item_image_update_thread_handle[i] == callback_data->handle)
                {
                    //retcode 只是http协议通信成功或者失败的返回值,不是下载成功或者失败的返回值,需要引入额外的参数进行判断
                    if(callback_data->complete_data.retcode == GXAPPS_SUCCESS && callback_data->complete_data.size > 0)
                    {
                        PLUGIN_INFO("*****%d****ok*****", i);
                        memset(&param, 0, sizeof(struct PlugiaSourceUIMsg));
                        param.type = MSG_UPDATE_ITEM_IMAGE;
                        param.item = i;
                        param.image_path = this_pic_path[i];
                        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
                    }
                    else
                    {
                        PLUGIN_ERR("*****%d****error*****\n", i);
                    }
                }
            }
        }
    }
}

static void _gxapp_curl_async_abort(void *data)
{
    int handle = (int )data;

    GxCore_ThreadDetach();
    gxapps_curl_async_abort(handle);
}

static void _pic_download_stop(void)
{
    int i = 0;
    static handle_t curl_async_abort_handle[MAX_PAGE_ITEM];

    PLUGIN_INFO("stop download");
    this_focus_info.pic_download_abort = 1;
    for(i = 0; i < MAX_PAGE_ITEM; i++)
    {
        if(item_image_update_thread_handle[i] >= 0)
        {
            GxCore_ThreadCreate("async_abort", &curl_async_abort_handle[i], _gxapp_curl_async_abort, (void *)item_image_update_thread_handle[i], 10*1024, GXOS_DEFAULT_PRIORITY);
            item_image_update_thread_handle[i] = -1;
        }
    }
    this_focus_info.pic_download_abort = 0;
}

static void _item_image_update(char *icon_url, int index)
{
    char *url = NULL;
    uint32_t length = 0;
    char *path = NULL;

    url = image_url_json_parse(icon_url);
    if(url == NULL)
        return;

    length = strlen(IMAGE_DOWN_PATH) + strlen(get_fname_suffix_from_url(url)) + 20;
    if(length >= MAX_PATH_LEN)
    {
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return;
    }

    path = GxCore_Calloc(sizeof(char), length);
    if(NULL == path)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        APP_FREE(url);
        return;
    }

    snprintf(path, MAX_FILE_NAME_LEN, "%s/%d%s", IMAGE_DOWN_PATH, index, get_fname_suffix_from_url(url));
    APP_FREE(this_pic_path[index]);

    this_pic_path[index] = path;
    item_image_update_thread_handle[index] = gxapps_curl_async_download(url, path, _pic_download_cb);

    APP_FREE(url);
}

static void _list_update_thread(void *argc)
{
    int i = 0;
    int ret;
    char *url;

    GxCore_ThreadDetach();
    while(1)
    {
        GxCore_MutexLock(this_focus_info.mutex_id);
        if(this_focus_info.update_flag == UPDATE_CLASSIFICATION)
        {
            ret = Classification_list_refresh(this_focus_info.head);
            if(ret < 0)
            {
                app_exit_abort = 1;
                this_focus_info.update_flag = UPDATE_END;
                GxCore_MutexUnlock(this_focus_info.mutex_id);
                app_plugin_source_show_popup_msg(STR_ID_PLUGIN_ERR, 0);
                break;
            }
            if(this_focus_info.total_number < this_focus_info.head->counter || (i > 50 && this_focus_info.total_number > 0))
            {
                if(pop_msg_on && 0 == app_exit_abort)
                    app_plugin_source_hide_popup_msg();
                if(this_focus_info.total_number < this_focus_info.head->counter)
                    GxCore_ThreadDelay(300);
                ret = Classification_list_refresh(this_focus_info.head);
                if(ret < 0)
                {
                    app_exit_abort = 1;
                    this_focus_info.update_flag = UPDATE_END;
                    GxCore_MutexUnlock(this_focus_info.mutex_id);
                    app_plugin_source_show_popup_msg(STR_ID_PLUGIN_ERR, 0);
                    break;
                }
                this_focus_info.total_number = this_focus_info.head->counter;
                _plugin_source_group_list_update();
                this_focus_info.update_flag = UPDATE_CLASSIFICATION_OK;
                PLUGIN_INFO("update classification of.");
            }
            i++;
        }
        else if(this_focus_info.update_flag == UPDATE_SOURCE
                || this_focus_info.update_flag == UPDATE_CLASSIFICATION_OK
                || this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            ret = _plugin_source_info_draw(0);
            if(ret < 0)
            {
                app_exit_abort = 1;
                this_focus_info.update_flag = UPDATE_END;
                GxCore_MutexUnlock(this_focus_info.mutex_id);
                app_plugin_source_show_popup_msg(STR_ID_PLUGIN_ERR, 0);
                break;
            }
        }
        else if(this_focus_info.update_flag == UPDATE_SOURCE_URL)
        {
            ret = Source_url_refresh(&(url));
            if(ret < 0)
            {
                app_exit_abort = 1;
                this_focus_info.update_flag = UPDATE_END;
                GxCore_MutexUnlock(this_focus_info.mutex_id);
                app_plugin_source_show_popup_msg(STR_ID_PLUGIN_ERR, 0);
                break;
            }
            if(url != NULL)
            {
                this_focus_info.update_flag = UPDATE_SOURCE_PLAY;
                APP_FREE(source_url);
                source_url = GxCore_Strdup(url);
                APP_FREE(url);
                _plugin_source_play();
            }
        }
        else if(this_focus_info.update_flag == UPDATE_END)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            break;
        }
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        GxCore_ThreadDelay(20);
    }
}

static void _list_view_focus_info_item_init(void)
{
    this_focus_info.focus_item = 0;
    if(this_focus_info.head != NULL && this_focus_info.head->list.next != NULL)
        this_focus_info.node = list_entry(this_focus_info.head->list.next, typeof(*this_focus_info.head), list);
}

static void _source_focus_info_item_init(void)
{
    struct SourceFocusInfo *parent_node = NULL;

    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        parent_node->child->focus_item = 0;
        if(parent_node->node->child_list != NULL && parent_node->node->child_list->list.next != NULL)
            parent_node->child->node = list_entry(parent_node->node->child_list->list.next, typeof(*parent_node->node->child_list), list);
    }
    else
    {
        this_focus_info.source.focus_item = 0;
        if(this_focus_info.node->source_list_head != NULL && this_focus_info.node->source_list_head->list.next != NULL)
            this_focus_info.source.node = list_entry(this_focus_info.node->source_list_head->list.next, typeof(*this_focus_info.node->source_list_head), list);
    }
}

static void _plugin_source_show_title(void)
{
    PluginInfoNode *node;

    node = Pluginstore_focus_node_get();
    if(node->title != NULL)
        GUI_SetProperty(WND_PLUGIN_SOURCE_TITLE, "string", node->title);
}

static void _classification_list_get(void)
{
    PluginInfoNode *node;
    SourceInfoNode *virtual_node = NULL;
    int ret;

    this_focus_info.search_str = GxCore_Calloc(1, MAX_FILE_NAME_LEN);
    app_exit_abort = 0;
    this_focus_info.init_abort = 1;
    _plugin_source_show_state();
    node = Pluginstore_focus_node_get();
    Plugin_store_init();
    this_focus_info.init_abort = 0;
    Plugin_install(node->folder);
    ret = Classification_list_get(node);
    if(ret < 0)
    {
        PLUGIN_ERR("load error\n");
        app_exit_abort = 1;
        app_plugin_source_show_popup_msg(STR_ID_PLUGIN_ERR, 0);
        return;
    }

    if(app_exit_abort)
    {
        PLUGIN_ERR("load error, app_exit_abort\n");
        return;
    }

    GxCore_MutexLock(this_focus_info.mutex_id);
    this_focus_info.source.child_index = 0;
    if(node->ui_type != NULL && strcmp(UI_TYPE_SIGNLE, node->ui_type) == 0)
    {
        PLUGIN_INFO("ok");
        virtual_node = (SourceInfoNode *)GxCore_Calloc(1, sizeof(SourceInfoNode));
        if(virtual_node == NULL)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            return;
        }
        INIT_DLIST_HEAD(&virtual_node->list);
        ret = Source_list_get((SourceInfoList **)&virtual_node, 1, 0);
        if(ret < 0)
        {
            PLUGIN_ERR("load error\n");
            APP_FREE(virtual_node);
            _plugin_source_dig_end();
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
        virtual_node->counter = 1;
        this_focus_info.source.node = virtual_node;
        this_focus_info.node = NULL;
        _source_child_list_init();
        this_focus_info.head = NULL;
    }
    else
    {
        _plugin_source_group_list_show();

        this_focus_info.head = node->classification_list_head;
        this_focus_info.total_number = this_focus_info.head->counter;
        this_focus_info.focus_item = 0;

        _plugin_source_group_list_focus();
        _plugin_source_group_list_update();

        _list_view_focus_info_item_init();

        if(this_focus_info.node != NULL
                && this_focus_info.node->type != NULL
                && 0 == strcmp(this_focus_info.node->type, "search"))
        {
            plugin_timeout_monitor_stop();
            app_plugin_source_hide_popup_msg();
        }

        this_focus_info.update_flag = UPDATE_CLASSIFICATION;
    }
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    GxCore_ThreadCreate("list_update", &list_update_thread_handle, _list_update_thread,  NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
}

static void _classification_list_free(void)
{
    PluginInfoNode *node;

    node = Pluginstore_focus_node_get();
    GxCore_MutexLock(mutex_group_list_exit_id);
    Classification_list_free(&(node->classification_list_head));
    GxCore_MutexUnlock(mutex_group_list_exit_id);
    this_focus_info.head = NULL;
}

static struct SourceFocusInfo * _source_child_find_last_node(void)
{
    struct SourceFocusInfo *node = NULL;

    node = &(this_focus_info.source);
    while(node->child != NULL)
        node= node->child;
    return node;
}

static struct SourceFocusInfo * _source_child_find_parent_node(void)
{
    struct SourceFocusInfo *node = NULL;
    int index;

    index = this_focus_info.source.child_index - 1;
    node = &(this_focus_info.source);
    while(index > 0)
    {
        node= node->child;
        index--;
    }
    return node;
}

static int _source_child_list_init(void)
{
    struct SourceFocusInfo *node;

    plugin_timeout_monitor_start(20000);
    _plugin_source_clean_item();
    node = _source_child_find_last_node();
    node->child = (struct SourceFocusInfo *)GxCore_Calloc(1, sizeof(struct SourceFocusInfo));
    if(node->child == NULL)
    {
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return -1;
    }

    if(this_focus_info.source.child_index == 0)
    {
        _plugin_source_group_list_hide();
        _plugin_source_unfocus_item(this_focus_info.source.focus_item);
        this_focus_info.update_flag = UPDATE_SOURCE_CHILD;
    }
    else
    {
        _plugin_source_unfocus_item(node->focus_item);
    }

    this_focus_info.source.child_index++;
    return 0;
}

static void _source_child_list_exit(void)
{
    struct SourceFocusInfo *node;

    this_focus_info.gif_status = 0;
    PLUGIN_INFO("====>");
    node = _source_child_find_parent_node();
    if(node == NULL)
    {
        PLUGIN_ERR("error\n");
        return;
    }
    if(node->child->child == NULL)
    {
        Source_list_free(&(node->node->child_list));
        _plugin_source_unfocus_item(node->child->focus_item);
        _plugin_source_clean_item();

        if(this_focus_info.source.child_index > 0)
            this_focus_info.source.child_index--;
        APP_FREE(node->child);
        if(this_focus_info.source.child_index == 0)
        {
            this_focus_info.update_flag = UPDATE_SOURCE;
            _plugin_source_group_list_show();
            _plugin_source_info_draw(2);
            _plugin_source_focus_item(this_focus_info.source.focus_item);
        }
        else
        {
            _plugin_source_info_draw(2);
            _plugin_source_focus_item(node->focus_item);
        }
    }
}

static int group_list_keypress_timer_abort = 0;
static int _group_list_keypress_timer(int ms)
{
    uint64_t tick = GxCore_TickStart(ms);

    group_list_keypress_timer_abort = 0;
    do{
        if(group_list_keypress_timer_abort)
            return -1;
        GxCore_ThreadDelay(10);
    }while(!GxCore_TickEnd(tick));
    return 0;
}

static int _plugin_source_info_draw(int mode)
{
    SourceInfoNode *pos;
    SourceInfoNode *node;
    struct SourceFocusInfo *parent_node = NULL;
    SourceInfoList **head;
    int i = 0, j = 0;
    int index, total_number;
    struct PlugiaSourceUIMsg param = {0};
    int ret;
    int use_thread;
    int search = 0;
    bool IsNeedUpdateItem=false;
    int cur_page_last_item_num=0;

    if(mode)
    {
        _pic_download_stop();
    }

    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        head = &parent_node->node->child_list;
        index = parent_node->node->counter;
        total_number = parent_node->child->total_number;
        use_thread = 0;

        if(parent_node->node->type != NULL //for bug 93969
                && strcmp("search", parent_node->node->type) == 0)
        {
            search = 1;
        }
    }
    else
    {
        head = &this_focus_info.node->source_list_head;
        index = this_focus_info.node->counter;
        total_number = this_focus_info.source.total_number;
        use_thread = 1;
        if(this_focus_info.node->type != NULL
                && strcmp("search", this_focus_info.node->type) == 0)
        {
            search = 1;
        }
    }

    if(*head == NULL)
    {
        if(_group_list_keypress_timer(10) < 0)
            return 0;

        if(search)
        {
            if(this_focus_info.search_enable == 0 || this_focus_info.search_str == NULL)
            {
                _plugin_source_hide_state();
                return 0;
            }
            _plugin_source_show_state();
            ret = Source_list_search_get(head, index, use_thread, this_focus_info.search_str);
            this_focus_info.search_enable = 0;
        }
        else
        {
            _plugin_source_show_state();
            ret = Source_list_get(head, index, use_thread);
        }
        if(ret < 0)
            return -1;
        PLUGIN_INFO("total_number:%d", (*head)->counter);

        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
            parent_node->child->total_number = (*head)->counter;
        else
            this_focus_info.source.total_number = (*head)->counter;

        if((*head)->counter <= 0)
            return 0;

        plugin_timeout_monitor_stop();

        if(pop_msg_on && 0 == app_exit_abort)
            app_plugin_source_hide_popup_msg();

        _source_focus_info_item_init();
        node = *head;

        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
            _plugin_source_update_page_info(parent_node->child->focus_item);
        else
            _plugin_source_update_page_info(this_focus_info.source.focus_item);
    }
    else
    {
        ret = Source_list_refresh(*head);
        if(ret < 0)
            return -1;
        if(!mode && total_number >= (*head)->counter)
            return 0;

        plugin_timeout_monitor_stop();

        if(pop_msg_on && 0 == app_exit_abort)
            app_plugin_source_hide_popup_msg();

        PLUGIN_DBG("update total_number:%d\n", (*head)->counter);
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
            parent_node->child->total_number = (*head)->counter;
        else
            this_focus_info.source.total_number = (*head)->counter;

        if(total_number <= 0 && (*head)->counter > 0)
            _source_focus_info_item_init();

        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
            _plugin_source_update_page_info(parent_node->child->focus_item);
        else
            _plugin_source_update_page_info(this_focus_info.source.focus_item);

        if(!mode)
        {
            if((total_number<this_focus_info.source.total_number)
                    && total_number-this_focus_info.source.focus_item/MAX_PAGE_ITEM*MAX_PAGE_ITEM < MAX_PAGE_ITEM)
                //on last page and not fillup
            {
                IsNeedUpdateItem=true;
                cur_page_last_item_num=total_number%MAX_PAGE_ITEM;
            }
            else
            {
                return 0;
            }
        }

        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            if(parent_node->child->focus_item != 0)
            {
                list_from_node_reverse(pos, &(parent_node->child->node->list), list, *head)
                {
                    j++;
                    if(j > (parent_node->child->focus_item % MAX_PAGE_ITEM))
                        break;
                }
                node = pos;
            }
            else
                node = list_entry(parent_node->child->node->list.prev, typeof(*(parent_node->child->node)), list);
        }
        else
        {
            if(this_focus_info.source.focus_item != 0)
            {
                list_from_node_reverse(pos, &(this_focus_info.source.node->list), list, *head)
                {
                    j++;
                    if(j > (this_focus_info.source.focus_item % MAX_PAGE_ITEM))
                        break;
                }
                node = pos;
            }
            else
                node = list_entry(this_focus_info.source.node->list.prev, typeof(*(this_focus_info.source.node)), list);
        }
    }

    _plugin_source_hide_state();
    list_from_node(pos, &(node->list), list, *head)
    {
        if(i >= MAX_PAGE_ITEM)
            break;
        if(IsNeedUpdateItem && i< cur_page_last_item_num)
        {
            i++;
            continue;
        }

        memset(&param, 0, sizeof(struct PlugiaSourceUIMsg));
        param.type = MSG_UPDATE_ITEM;
        param.node.title = pos->title;
        param.draw_mode = mode;
        param.image_path = IMG_DEFAULT;
        param.item = i;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
        if(pos->icon_url != NULL)
            _item_image_update(pos->icon_url, i);
        i++;
    }
    for(; i < MAX_PAGE_ITEM; i++)
    {
        memset(&param, 0, sizeof(struct PlugiaSourceUIMsg));
        param.type = MSG_UPDATE_ITEM;
        param.node.title = NULL;
        param.item = i;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    }

    return 0;
}

status_t app_create_plugin_source_menu(void)
{
    PLUGIN_INFO("=====>");
    if(GUI_CheckDialog(WND_PLUGIN_SOURCE_LIST) == GXCORE_ERROR)
        GUI_CreateDialog(WND_PLUGIN_SOURCE_LIST);

    GUI_SetInterface("flush", NULL);
    return 0;
}

static void _plugin_source_classification_free(void)
{
    PluginInfoNode *node;

    PLUGIN_INFO("=====>");
    this_focus_info.update_flag = UPDATE_END;
    node = Pluginstore_focus_node_get();
    if(node->classification_list_head != NULL)
    {
        if(this_focus_info.node != NULL && this_focus_info.node->source_list_head != NULL)
            Source_list_free(&(this_focus_info.node->source_list_head));
        _classification_list_free();
        this_focus_info.total_number = 0;
        this_focus_info.source.total_number = 0;
    }
}


void app_plugin_source_menu_exit(void)
{
    APP_FREE(this_focus_info.search_str);
    exit_only_once = true;
    thiz_wnd_created = false;
    app_exit_abort = 1;
    Plugin_store_exit();
    Plugin_exit_abort(1);
    GxCore_MutexLock(this_focus_info.mutex_id);
    this_focus_info.update_flag = UPDATE_END;
    _pic_download_stop();
    while(this_focus_info.source.child_index > 1)
    {
        _source_child_list_exit();
    }
    if(this_focus_info.head == NULL)
    {
        _plugin_source_classification_free();
        if(this_focus_info.source.node != NULL && this_focus_info.source.node->child_list != NULL)
            Source_list_free(&(this_focus_info.source.node->child_list));
        APP_FREE(this_focus_info.source.child);
        APP_FREE(this_focus_info.source.node);
    }
    else
    {
        if(1 == this_focus_info.source.child_index)
        {
            _source_child_list_exit();
        }
        _plugin_source_classification_free();
    }
    Plugin_exit_abort(0);
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    GxCore_MutexDelete(this_focus_info.mutex_id);
    GxCore_MutexDelete(mutex_group_list_exit_id);
    exit_only_once = false;
    return;
}

static void wraper_app_plugin_source_exit(void)
{
    exit_only_once = true;
    _plugin_source_dig_end();
    exit_only_once = false;
}


SIGNAL_HANDLER int app_plugin_source_create(GuiWidget *widget, void *usrdata)
{
    int i = 0;

    app_log_debug("=====>%s\n", __func__);
    thiz_wnd_created = true;
    for(i = 0;i < MAX_PAGE_ITEM; i++)
    {
        GUI_SetProperty(s_text_source_focus[i], "backcolor", TXT_BACKCOLOR_UNFOCUS);
        GUI_SetProperty(s_img_source_unfocus[i], "img", PLUGIN_IMG_NULL);
        GUI_SetProperty(s_text_source_title[i], "string", PLUGIN_STR_BLANK);
    }
    memset(&this_focus_info, 0, sizeof(struct ListFocusInfo));
    GxCore_MutexCreate(&mutex_group_list_exit_id);
    GxCore_MutexCreate(&this_focus_info.mutex_id);
    _plugin_source_unfocus_all_item();
    _plugin_source_show_title();
    _plugin_source_load_gif();
    plugin_timeout_monitor_start(20000);
    app_netapps_service_push(_classification_list_get);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_source_destroy(GuiWidget *widget, void *usrdata)
{
    PLUGIN_INFO("=====>");
    plugin_timeout_monitor_delete();
    app_plugin_source_menu_exit();
    return EVENT_TRANSFER_STOP;
}

static void _plugin_source_up_keypress(void)
{
    SourceInfoNode *pos;
    SourceInfoNode *node;
    SourceInfoList *head;
    struct SourceFocusInfo *parent_node = NULL;
    int i, j, total_number, focus_item;

    if(this_focus_info.wait_abort || this_focus_info.update_flag == UPDATE_END)
        return;
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        node = parent_node->child->node;
        head = parent_node->node->child_list;
        total_number = parent_node->child->total_number;
        focus_item = i = parent_node->child->focus_item;
    }
    else
    {
        node = this_focus_info.source.node;
        head = this_focus_info.node->source_list_head;
        total_number = this_focus_info.source.total_number;
        focus_item = i = this_focus_info.source.focus_item;
    }
    j = 0;

    if(total_number == 0 || node == NULL || head == NULL)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(((i%MAX_PAGE_ITEM)-MAX_PAGE_ITEM/MAX_PAGE_LINE) < 0)
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i > total_number-1)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
        list_from_node(pos, &(node->list), list, head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
    }
    else
    {
        i -= MAX_PAGE_ITEM / MAX_PAGE_LINE;
        list_from_node_reverse(pos, &(node->list), list, head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
    }

    _plugin_source_unfocus_item(focus_item);
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node->child->focus_item = i;
        parent_node->child->node = pos;
        PLUGIN_INFO("%d,child type:%s", i, parent_node->child->node->type);
    }
    else
    {
        this_focus_info.source.focus_item = i;
        this_focus_info.source.node = pos;
        PLUGIN_INFO("%d,type:%s", i, this_focus_info.source.node->type);
    }
    _plugin_source_focus_item(i);
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void _plugin_source_down_keypress(void)
{
    SourceInfoNode *pos;
    SourceInfoNode *node;
    SourceInfoList *head;
    struct SourceFocusInfo *parent_node = NULL;
    int i, j, total_number, focus_item;

    if(this_focus_info.wait_abort || this_focus_info.update_flag == UPDATE_END)
        return;
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        node = parent_node->child->node;
        head = parent_node->node->child_list;
        total_number = parent_node->child->total_number;
        focus_item = i = parent_node->child->focus_item;
    }
    else
    {
        node = this_focus_info.source.node;
        head = this_focus_info.node->source_list_head;
        total_number = this_focus_info.source.total_number;
        focus_item = i = this_focus_info.source.focus_item;
    }
    j = 0;

    if(total_number == 0 || node == NULL || head == NULL)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(((i%MAX_PAGE_ITEM)+MAX_PAGE_ITEM/MAX_PAGE_LINE) < MAX_PAGE_ITEM)
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i > total_number-1)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
        list_from_node(pos, &(node->list), list, head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
    }
    else
    {
        i -= MAX_PAGE_ITEM / MAX_PAGE_LINE;
        list_from_node_reverse(pos, &(node->list), list, head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
    }

    _plugin_source_unfocus_item(focus_item);
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node->child->focus_item = i;
        parent_node->child->node = pos;
        PLUGIN_INFO("%d,child type:%s", i, parent_node->child->node->type);
    }
    else
    {
        this_focus_info.source.focus_item = i;
        this_focus_info.source.node = pos;
        PLUGIN_INFO("%d,type:%s", i, this_focus_info.source.node->type);
    }
    _plugin_source_focus_item(i);
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void _plugin_source_left_keypress(void)
{
    SourceInfoNode *node;
    SourceInfoList *head;
    SourceInfoNode *pos;
    int i = 0, j = 0;
    struct SourceFocusInfo *parent_node = NULL;
    int page, total_number, focus_item;

    if(this_focus_info.wait_abort || this_focus_info.update_flag == UPDATE_END)
        return;
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        node = parent_node->child->node;
        head = parent_node->node->child_list;
        total_number = parent_node->child->total_number;
        focus_item = i = parent_node->child->focus_item;
    }
    else
    {
        node = this_focus_info.source.node;
        head = this_focus_info.node->source_list_head;
        total_number = this_focus_info.source.total_number;
        focus_item = i = this_focus_info.source.focus_item;
    }

    if(total_number == 0 || node == NULL || head == NULL)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(0 == i%(MAX_PAGE_ITEM/MAX_PAGE_LINE))
    {
        _plugin_source_unfocus_item(focus_item);
        page = (i - i % MAX_PAGE_ITEM) / MAX_PAGE_ITEM;
        if((page-1) < 0)
        {
            if(this_focus_info.update_flag == UPDATE_SOURCE)
            {
                this_focus_info.update_flag = UPDATE_CLASSIFICATION_OK;
                _plugin_source_unfocus_item(this_focus_info.source.focus_item);
                _plugin_source_group_list_focus();
                GxCore_MutexUnlock(this_focus_info.mutex_id);
                return;
            }
            else
            {
                if(total_number <= MAX_PAGE_ITEM)
                {
                    _plugin_source_focus_item(focus_item);
                    GxCore_MutexUnlock(this_focus_info.mutex_id);
                    return;
                }
                i = total_number - 1;
            }
        }
        else
            i -= (MAX_PAGE_ITEM / MAX_PAGE_LINE + 1);

        list_from_node(pos, &(head->list), list, head)
        {
            j++;
            if(j > i)
                break;
        }
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node->child->node = pos;
            parent_node->child->focus_item = i;
        }
        else
        {
            this_focus_info.source.node = pos;
            this_focus_info.source.focus_item = i;
        }
        _plugin_source_info_draw(1);
    }
    else
    {
        i -= 1;
        _plugin_source_unfocus_item(focus_item);
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node->child->node = list_entry(node->list.prev, typeof(*node), list);
            parent_node->child->focus_item = i;
        }
        else
        {
            this_focus_info.source.node = list_entry(node->list.prev, typeof(*node), list);
            this_focus_info.source.focus_item = i;
        }
    }

    _plugin_source_focus_item(i);
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void _plugin_source_right_keypress(void)
{
    SourceInfoNode *node;
    SourceInfoList *head;
    SourceInfoNode *pos;
    int i = 0, j = 0;
    struct SourceFocusInfo *parent_node = NULL;
    int total_number, focus_item;

    if(this_focus_info.wait_abort || this_focus_info.update_flag == UPDATE_END)
        return;
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        node = parent_node->child->node;
        head = parent_node->node->child_list;
        total_number = parent_node->child->total_number;
        focus_item = i = parent_node->child->focus_item;
    }
    else
    {
        node = this_focus_info.source.node;
        head = this_focus_info.node->source_list_head;
        total_number = this_focus_info.source.total_number;
        focus_item = i = this_focus_info.source.focus_item;
    }

    if(total_number >= MAX_COUNT && focus_item == total_number - 1)
    {
        app_plugin_source_show_popup_msg(STR_ID_NO_RESOURCE, 2000);
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }

    if(total_number == 0 || node == NULL || head == NULL)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(0 == (i+1)%(MAX_PAGE_ITEM/MAX_PAGE_LINE) || i == (total_number - 1))
    {
        if(total_number <= MAX_PAGE_ITEM)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
        else
        {
            _plugin_source_unfocus_item(focus_item);
            if(i == total_number - 1)
            {
                i = 0;
            }
            else if(total_number - 1 - i >= (MAX_PAGE_ITEM / MAX_PAGE_LINE + 1))
            {
                i += (MAX_PAGE_ITEM / MAX_PAGE_LINE + 1);
            }
            else
            {
                i = total_number - 1;
            }

            list_from_node(pos, &(head->list), list, head)
            {
                j++;
                if(j > i)
                    break;
            }
            if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
            {
                parent_node->child->node = pos;
                parent_node->child->focus_item = i;
            }
            else
            {
                this_focus_info.source.node = pos;
                this_focus_info.source.focus_item = i;
            }
            Plugin_source_want_more_data();
            _plugin_source_info_draw(1);
        }
    }
    else
    {
        i += 1;
        if(i > total_number-1)
        {
            GxCore_MutexUnlock(this_focus_info.mutex_id);
            return;
        }
        _plugin_source_unfocus_item(focus_item);
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node->child->node = list_entry(node->list.next, typeof(*node), list);
            parent_node->child->focus_item = i;
        }
        else
        {
            this_focus_info.source.node = list_entry(node->list.next, typeof(*node), list);
            this_focus_info.source.focus_item = i;
        }
    }

    _plugin_source_focus_item(i);
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void _plugin_source_page_up_keypress(void)
{
    SourceInfoNode *node;
    SourceInfoList *head;
    SourceInfoNode *pos;
    int i = 0, j = 0;
    struct SourceFocusInfo *parent_node = NULL;
    int total_number, focus_item;

    if(this_focus_info.wait_abort || this_focus_info.update_flag == UPDATE_END)
        return;
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        node = parent_node->child->node;
        head = parent_node->node->child_list;
        total_number = parent_node->child->total_number;
        focus_item = i = parent_node->child->focus_item;
    }
    else
    {
        node = this_focus_info.source.node;
        head = this_focus_info.node->source_list_head;
        total_number = this_focus_info.source.total_number;
        focus_item = i = this_focus_info.source.focus_item;
    }

    if(total_number == 0 || node == NULL || head == NULL)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }

    if(total_number <= MAX_PAGE_ITEM)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    else
    {
        _plugin_source_unfocus_item(focus_item);

        if(0 == i)
        {
            i = total_number - 1;
        }
        else
        {
            if(i >= MAX_PAGE_ITEM)
            {
                i -= MAX_PAGE_ITEM;
            }
            else
            {
                i = 0;
            }
        }
        list_from_node(pos, &(head->list), list, head)
        {
            j++;
            if(j > i)
                break;
        }
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node->child->node = pos;
            parent_node->child->focus_item = i;
        }
        else
        {
            this_focus_info.source.node = pos;
            this_focus_info.source.focus_item = i;
        }
        _plugin_source_info_draw(1);
    }

    _plugin_source_focus_item(i);
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    return;
}
static void _plugin_source_page_down_keypress(void)
{
    SourceInfoNode *node;
    SourceInfoList *head;
    SourceInfoNode *pos;
    int i = 0, j = 0;
    struct SourceFocusInfo *parent_node = NULL;
    int total_number, focus_item;

    if(this_focus_info.wait_abort || this_focus_info.update_flag == UPDATE_END)
        return;
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        parent_node = _source_child_find_parent_node();
        node = parent_node->child->node;
        head = parent_node->node->child_list;
        total_number = parent_node->child->total_number;
        focus_item = i = parent_node->child->focus_item;
    }
    else
    {
        node = this_focus_info.source.node;
        head = this_focus_info.node->source_list_head;
        total_number = this_focus_info.source.total_number;
        focus_item = i = this_focus_info.source.focus_item;
    }

    if(total_number >= MAX_COUNT && focus_item == total_number - 1)
    {
        app_plugin_source_show_popup_msg(STR_ID_NO_RESOURCE, 2000);
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }

    if(total_number == 0 || node == NULL || head == NULL)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(total_number <= MAX_PAGE_ITEM)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    else
    {
        _plugin_source_unfocus_item(focus_item);

        if(i == total_number - 1)
        {
            i = 0;
        }
        else if(total_number - 1 - i >= MAX_PAGE_ITEM)
        {
            i += MAX_PAGE_ITEM;
        }
        else
        {
            i = total_number -1;
        }
        list_from_node(pos, &(head->list), list, head)
        {
            j++;
            if(j > i)
                break;
        }
        if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node->child->node = pos;
            parent_node->child->focus_item = i;
        }
        else
        {
            this_focus_info.source.node = pos;
            this_focus_info.source.focus_item = i;
        }
        _plugin_source_info_draw(1);
    }

    _plugin_source_focus_item(i);
    Plugin_source_want_more_data();
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    return;
}

static void plugin_source_exit_abort(void)
{
    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION || this_focus_info.update_flag ==  UPDATE_CLASSIFICATION_OK || this_focus_info.update_flag == UPDATE_END)
    {
        app_exit_abort = 1;
        Plugin_store_exit();
    }
    else if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        if(this_focus_info.head == NULL && this_focus_info.source.child_index <= 1)
        {
            app_exit_abort = 1;
            Plugin_store_exit();
        }
    }
}

static void _plugin_source_exit_keypress(void)
{
    PLUGIN_INFO("=====>");
    exit_only_once = true;
    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION
            || this_focus_info.update_flag ==  UPDATE_CLASSIFICATION_OK
            || this_focus_info.update_flag == UPDATE_END
            || (this_focus_info.update_flag == UPDATE_SOURCE_CHILD
                && this_focus_info.head == NULL
                && this_focus_info.source.child_index <= 1))
    {
        _plugin_source_dig_end();
        exit_only_once = false;
        return;
    }

    Plugin_exit_abort(1);
    GxCore_MutexLock(this_focus_info.mutex_id);
    if(this_focus_info.update_flag == UPDATE_SOURCE)
    {
        this_focus_info.update_flag = UPDATE_CLASSIFICATION_OK;
        _plugin_source_unfocus_item(this_focus_info.source.focus_item);
        _plugin_source_group_list_focus();
    }
    else if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        _pic_download_stop();
        _source_child_list_exit();
    }
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    Plugin_exit_abort(0);
    exit_only_once = false;
}

static void _plugin_source_ok_keypress(void)
{
    int ret;
    struct SourceFocusInfo *parent_node = NULL;
    PLUGIN_INFO("=====>");

    if(this_focus_info.wait_abort || this_focus_info.update_flag == UPDATE_END)
        return;
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    if(this_focus_info.update_flag == UPDATE_SOURCE)
    {
        _pic_download_stop();
        if(strcmp("directory", this_focus_info.source.node->type) == 0)
        {
            _source_child_list_init();
        }
        else
        {
            this_focus_info.update_flag = UPDATE_SOURCE_URL;
            this_focus_info.parent_update_flag = UPDATE_SOURCE;
            _plugin_source_play_dig_create();
            ret = Source_url_get(this_focus_info.source.node->counter);
            if(ret < 0)
                PLUGIN_ERR("Source_url_get error\n");

            play_exit_abort = 1;
            this_focus_info.source.old_item = this_focus_info.source.focus_item;
        }
    }
    else if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD)
    {
        _pic_download_stop();
        parent_node = _source_child_find_parent_node();
        if(parent_node->child->total_number > 0)
        {
            PLUGIN_INFO("node type: %s", parent_node->child->node->type);
            if(strcmp("directory", parent_node->child->node->type) == 0)
            {
                _source_child_list_init();
            }
            else
            {
                this_focus_info.update_flag = UPDATE_SOURCE_URL;
                this_focus_info.parent_update_flag = UPDATE_SOURCE_CHILD;
                _plugin_source_play_dig_create();
                ret = Source_url_get(parent_node->child->node->counter);
                if(ret < 0)
                    PLUGIN_ERR("Source_url_get error\n");

                play_exit_abort = 1;
                parent_node->child->old_item = parent_node->child->focus_item;
            }
        }
    }
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

SIGNAL_HANDLER int app_plugin_source_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(this_focus_info.init_abort
            && VK_BOOK_TRIGGER != find_virtualkey_ex(event->key.scancode,event->key.sym))
        return EVENT_TRANSFER_STOP;
    else
    {
        while(this_focus_info.init_abort)
            GxCore_ThreadDelay(10);
    }

    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                if(pop_msg_on)
                {
                    app_plugin_source_hide_popup_msg();
                    plugin_timeout_monitor_start(20000);//reset
                }
                if(!pop_msg_on || (app_exit_abort && true == _plugin_check_err_type()))
                {
                    plugin_source_exit_abort();
                    while(!exit_only_once && app_netapps_service_push(_plugin_source_exit_keypress) < 0)
                    {
                        GxCore_ThreadDelay(10);
                    }
                }
                else
                {
                    app_plugin_source_hide_popup_msg();
                    plugin_timeout_monitor_start(20000);//reset
                }
                break;
            case STBK_OK:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_source_ok_keypress);
                break;
            case STBK_UP:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_source_up_keypress);
                break;
            case STBK_DOWN:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_source_down_keypress);
                break;
            case STBK_LEFT:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_source_left_keypress);
                break;
            case STBK_RIGHT:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_source_right_keypress);
                break;
            case STBK_PAGE_UP:
                if(!pop_msg_on)
                {
                    app_netapps_service_push(_plugin_source_page_up_keypress);
                }
                break;
            case STBK_PAGE_DOWN:
                if(!pop_msg_on)
                {
                    app_netapps_service_push(_plugin_source_page_down_keypress);
                }
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_source_lost_focus(GuiWidget *widget, void *usrdata)
{
    if((GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
            ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP))
            ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE)))
    {
        struct PlugiaSourceUIMsg param = {0};

        param.type = MSG_HIDE_GIF;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
        this_focus_info.book_flag = true;
    }
    PLUGIN_INFO("=====>");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_source_got_focus(GuiWidget *widget, void *usrdata)
{
    struct SourceFocusInfo *parent_node = NULL;

    if(UPDATE_CLASSIFICATION_OK == this_focus_info.update_flag
            || UPDATE_CLASSIFICATION == this_focus_info.update_flag)
    {
        _plugin_source_group_list_show();
    }
    if(GXCORE_ERROR == GUI_CheckDialog(WND_POP_BOOK) && true == this_focus_info.book_flag)
    {
        this_focus_info.book_flag = false;
        if((this_focus_info.wait_abort) && (!pop_msg_on))
            app_plugin_source_show_gif();

        return EVENT_TRANSFER_STOP;
    }
    GxCore_MutexLock(this_focus_info.mutex_id);
    if(this_focus_info.update_flag == UPDATE_SOURCE_URL || this_focus_info.update_flag == UPDATE_SOURCE_PLAY)
    {
        if(this_focus_info.update_flag == UPDATE_SOURCE_URL)
        {
            while(play_exit_abort == 0)
            {
                GxCore_ThreadDelay(10);
            }
        }
        Source_url_refresh_close();
        this_focus_info.update_flag = this_focus_info.parent_update_flag;
        _plugin_source_info_draw(2);
        if(this_focus_info.parent_update_flag == UPDATE_SOURCE)
        {
            if(this_focus_info.source.old_item != this_focus_info.source.focus_item)
            {
                _plugin_source_unfocus_item(this_focus_info.source.old_item);
                _plugin_source_focus_item(this_focus_info.source.focus_item);
            }
        }
        else if(this_focus_info.parent_update_flag == UPDATE_SOURCE_CHILD)
        {
            parent_node = _source_child_find_parent_node();
            if(parent_node->child->old_item != parent_node->child->focus_item)
            {
                _plugin_source_unfocus_item(parent_node->child->old_item);
                _plugin_source_focus_item(parent_node->child->focus_item);
            }
        }
        play_exit_abort = 0;
    }
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    PLUGIN_INFO("=====>");
    return EVENT_TRANSFER_STOP;
}

static void _group_list_up_keypress(void)
{
    while(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
    {
        if(app_exit_abort > 0)
            return;
        GxCore_ThreadDelay(10);
    }
    _plugin_source_hide_state();
    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION_OK)
    {
        _pic_download_stop();
        this_focus_info.source.total_number = 0;
        Source_list_free(&(this_focus_info.node->source_list_head));
    }
    this_focus_info.focus_item -= 1;
    if(this_focus_info.focus_item < 0)
    {
        this_focus_info.focus_item = this_focus_info.total_number - 1;
        this_focus_info.node = list_entry(this_focus_info.head->list.prev, typeof(*this_focus_info.head), list);
    }
    else
    {
        this_focus_info.node = list_entry(this_focus_info.node->list.prev, typeof(*this_focus_info.node), list);
    }

    if(this_focus_info.node->type != NULL
            && 0 == strcmp(this_focus_info.node->type, "search"))
    {
        plugin_timeout_monitor_stop();
        app_plugin_source_hide_popup_msg();
    }

    _plugin_source_group_list_focus();
    _plugin_source_clean_item();
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void _group_list_down_keypress(void)
{
    while(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
    {
        if(app_exit_abort > 0)
            return;
        GxCore_ThreadDelay(10);
    }
    _plugin_source_hide_state();
    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION_OK)
    {
        _pic_download_stop();
        this_focus_info.source.total_number = 0;
        Source_list_free(&(this_focus_info.node->source_list_head));
    }

    this_focus_info.focus_item += 1;
    if(this_focus_info.focus_item > (this_focus_info.total_number - 1))
    {
        this_focus_info.focus_item = 0;
        this_focus_info.node = list_entry(this_focus_info.head->list.next, typeof(*this_focus_info.head), list);
    }
    else
    {
        this_focus_info.node = list_entry(this_focus_info.node->list.next, typeof(*this_focus_info.node), list);
    }

    if(this_focus_info.node->type != NULL
            && 0 == strcmp(this_focus_info.node->type, "search"))
    {
        plugin_timeout_monitor_stop();
        app_plugin_source_hide_popup_msg();
    }

    _plugin_source_group_list_focus();
    _plugin_source_clean_item();
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void _group_list_page_up_keypress(void)
{
    int i = 0;
    ClassificationNode *pos = NULL;

    while(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
    {
        if(app_exit_abort > 0)
            return;
        GxCore_ThreadDelay(10);
    }
    _plugin_source_hide_state();
    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION_OK)
    {
        _pic_download_stop();
        this_focus_info.source.total_number = 0;
        Source_list_free(&(this_focus_info.node->source_list_head));
    }

    if(this_focus_info.focus_item == 0)
    {
        this_focus_info.focus_item = this_focus_info.total_number - 1;
        this_focus_info.node = list_entry(this_focus_info.head->list.prev, typeof(*this_focus_info.head), list);
    }
    else
    {
        this_focus_info.focus_item -= MAX_PAGE_ITEM;
        if(this_focus_info.focus_item < 0)
        {
            this_focus_info.focus_item = 0;
            this_focus_info.node = list_entry(this_focus_info.head->list.next, typeof(*this_focus_info.head), list);
        }
        else
        {
            list_for_each_entry(pos, &this_focus_info.head->list, list)
            {
                i++;
                if(i > this_focus_info.focus_item)
                    break;
            }
            this_focus_info.node = pos;
        }
    }

    if(this_focus_info.node->type != NULL
            && 0 == strcmp(this_focus_info.node->type, "search"))
    {
        plugin_timeout_monitor_stop();
        app_plugin_source_hide_popup_msg();
    }

    _plugin_source_group_list_focus();
    _plugin_source_clean_item();
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void _group_list_page_down_keypress(void)
{
    int i = 0;
    ClassificationNode *pos = NULL;

    while(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
    {
        if(app_exit_abort > 0)
            return;
        GxCore_ThreadDelay(10);
    }
    _plugin_source_hide_state();
    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION_OK)
    {
        _pic_download_stop();
        this_focus_info.source.total_number = 0;
        Source_list_free(&(this_focus_info.node->source_list_head));
    }

    if(this_focus_info.focus_item == this_focus_info.total_number - 1)
    {
        this_focus_info.focus_item = 0;
        this_focus_info.node = list_entry(this_focus_info.head->list.next, typeof(*this_focus_info.head), list);
    }
    else
    {
        this_focus_info.focus_item += MAX_PAGE_ITEM;
        if(this_focus_info.focus_item > (this_focus_info.total_number - 1))
        {
            this_focus_info.focus_item = this_focus_info.total_number - 1;
            this_focus_info.node = list_entry(this_focus_info.head->list.prev, typeof(*this_focus_info.head), list);
        }
        else
        {
            list_for_each_entry(pos, &this_focus_info.head->list, list)
            {
                i++;
                if(i > this_focus_info.focus_item)
                    break;
            }
            this_focus_info.node = pos;
        }
    }

    if(this_focus_info.node->type != NULL
            && 0 == strcmp(this_focus_info.node->type, "search"))
    {
        plugin_timeout_monitor_stop();
        app_plugin_source_hide_popup_msg();
    }

    _plugin_source_group_list_focus();
    _plugin_source_clean_item();
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void app_plugin_source_keyboard_proc(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;
    GxCore_MutexLock(this_focus_info.mutex_id);
    strncpy(this_focus_info.search_str, data->out_name, MAX_FILE_NAME_LEN - 1);
    this_focus_info.search_enable = 1;
    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION_OK)
    {
        _pic_download_stop();
        this_focus_info.source.total_number = 0;
        Source_list_free(&(this_focus_info.node->source_list_head));
    }
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

static void app_plugin_source_keyboard_create(void)
{
    static PopKeyboard keyboard;

    app_keyboard_save_cb(app_input_save_space_cb);
    memset(&keyboard, 0, sizeof(PopKeyboard));
    keyboard.in_name    = NULL;
    keyboard.max_num = 120;
    keyboard.out_name   = NULL;
    keyboard.change_cb  = NULL;
    keyboard.release_cb = app_plugin_source_keyboard_proc;
    keyboard.usr_data = NULL;
    keyboard.pos.x = 500;

    multi_language_keyboard_create(&keyboard);
}

static void _group_list_ok_keypress(void)
{
    if(app_exit_abort > 0)
        return;

    if(this_focus_info.update_flag == UPDATE_CLASSIFICATION_OK && strcmp("search", this_focus_info.node->type) == 0)
    {
        _pic_download_stop();
        GxCore_MutexLock(this_focus_info.mutex_id);
        this_focus_info.source.total_number = 0;

        if(this_focus_info.node->source_list_head)
            Source_list_free(&(this_focus_info.node->source_list_head));
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        _plugin_source_create_keyboard();
    }
}

static void _group_list_right_keypress(void)
{
    if(GxCore_MutexTrylock(this_focus_info.mutex_id) < 0)
        return;
    if(app_exit_abort > 0)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        return;
    }
    this_focus_info.update_flag = UPDATE_SOURCE;
    if(this_focus_info.node->source_list_head == NULL)
        _source_focus_info_item_init();

    _plugin_source_group_list_unfocus();
    _plugin_source_focus_item(this_focus_info.source.focus_item);
    GxCore_MutexUnlock(this_focus_info.mutex_id);
}

SIGNAL_HANDLER int app_plugin_source_group_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    if(this_focus_info.init_abort)
        return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    if(this_focus_info.update_flag == UPDATE_SOURCE)
    {
        GUI_SendEvent(WND_PLUGIN_SOURCE_LIST, event);
        return ret;
    }
    group_list_keypress_timer_abort = 1;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                if(pop_msg_on)
                {
                    app_plugin_source_hide_popup_msg();
                    plugin_timeout_monitor_start(20000);//reset
                }
                if(!pop_msg_on || (app_exit_abort && true == _plugin_check_err_type()))
                {
                    Plugin_store_exit();
                    while(!exit_only_once && app_netapps_service_push(wraper_app_plugin_source_exit) < 0)
                    {
                        GxCore_ThreadDelay(10);
                    }
                }
                break;
            case STBK_OK:
                if(!pop_msg_on)
                    app_netapps_service_push(_group_list_ok_keypress);
                break;
            case STBK_RIGHT:
                if(this_focus_info.source.total_number != 0 && !pop_msg_on)
                    app_netapps_service_push(_group_list_right_keypress);
                break;
            case STBK_UP:
                if(!pop_msg_on)
                {
                    plugin_timeout_monitor_start(20000);
                    app_netapps_service_push(_group_list_up_keypress);
                }
                break;
            case STBK_DOWN:
                if(!pop_msg_on)
                {
                    plugin_timeout_monitor_start(20000);
                    app_netapps_service_push(_group_list_down_keypress);
                }
                break;
            case STBK_PAGE_UP:
                if(!pop_msg_on)
                {
                    plugin_timeout_monitor_start(20000);
                    app_netapps_service_push(_group_list_page_up_keypress);
                }
                break;
            case STBK_PAGE_DOWN:
                if(!pop_msg_on)
                {
                    plugin_timeout_monitor_start(20000);
                    app_netapps_service_push(_group_list_page_down_keypress);
                }
                break;
            default:
                break;
        }
    }

    return ret;
}

SIGNAL_HANDLER int app_plugin_source_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    return this_focus_info.total_number;
}

SIGNAL_HANDLER int app_plugin_source_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;
    ClassificationNode *pos;
    int i = 0;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return EVENT_TRANSFER_STOP;
    if(0 > item->sel)
        return EVENT_TRANSFER_STOP;

    if(GxCore_MutexTrylock(mutex_group_list_exit_id) < 0)
        return EVENT_TRANSFER_STOP;

    list_from_node(pos, &(this_focus_info.head->list), list, this_focus_info.head)
    {
        if(app_exit_abort && true == _plugin_check_err_type())
            break;

        if(++i > item->sel)
        {
            item->image = NULL;
            item->string = pos->title;
            break;
        }
    }
    GxCore_MutexUnlock(mutex_group_list_exit_id);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_source_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static void _plugin_source_path_show(void)
{
    char *path = NULL;
    uint32_t length = 0;
    struct SourceFocusInfo *node = NULL;
    int index, offest;

    GxCore_MutexLock(this_focus_info.mutex_id);
    index = this_focus_info.source.child_index;
    node = &(this_focus_info.source);

    if(this_focus_info.head)
        length = strlen(node->node->title) + 10;
    else
        length = 10;

    if(length >= MAX_PATH_LEN)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
        return;
    }

    path = GxCore_Calloc(sizeof(char), MAX_PATH_LEN);
    if(NULL == path)
    {
        GxCore_MutexUnlock(this_focus_info.mutex_id);
        PLUGIN_ERR("No memory to GxCore_Calloc\n");
        return;
    }

    if(this_focus_info.head != NULL)
        snprintf(path, MAX_PATH_LEN, "PATH: %s", node->node->title);
    else
        snprintf(path, MAX_PATH_LEN, "PATH: ");

    offest = strlen(path);
    while(index > 0)
    {
        node = node->child;
        snprintf(path + offest, MAX_PATH_LEN - offest, "/%s", node->node->title);
        offest = strlen(path);
        if(offest >= MAX_PATH_LEN)
        {
            offest = MAX_PATH_LEN;
            break;
        }
        index--;
    }
    GxCore_MutexUnlock(this_focus_info.mutex_id);
    GUI_SetProperty(WND_PLUGIN_SOURCE_NAME, "string", path);

    APP_FREE(path);
}

static void _plugin_source_ui_contral(struct PlugiaSourceUIMsg *msg)
{
    char page_info[20] = {0};
    int page, max_page;
    int i = 0;

    if(msg == NULL)
        return;

    switch(msg->type)
    {
        case MSG_UPDATE_ITEM:
            {
                this_focus_info.gif_status = 0;
                if(pop_msg_on && 0 == app_exit_abort)
                    _app_plugin_source_hide_popup_msg();
                gdi_lock();
                if(msg->node.title != NULL)
                {
                    GUI_SetProperty(s_text_source_title[msg->item], "string", msg->node.title);
                    gal_add_key_path(key_img_default, msg->image_path);
                    GUI_SetProperty(s_img_source_unfocus[msg->item], "img",key_img_default);
                }
                else
                {
                    GUI_SetProperty(s_img_source_unfocus[msg->item], "img", PLUGIN_IMG_NULL);
                    GUI_SetProperty(s_text_source_title[msg->item], "string", PLUGIN_STR_BLANK);
                }
                GUI_SetProperty(s_text_source_focus[msg->item], "update", NULL);
                gdi_unlock();
                if(this_focus_info.update_flag == UPDATE_SOURCE_CHILD && msg->item == 0)
                {
                    if(msg->draw_mode == 0)
                        _plugin_source_focus_item(0);
                }
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        case MSG_CLEAN_ITEM:
            {
                this_focus_info.gif_status = 0;
                if(pop_msg_on)
                    _app_plugin_source_hide_popup_msg();
                GUI_SetProperty(WND_PLUGIN_SOURCE_PAGE_INFO, "string", PLUGIN_STR_BLANK);
                GUI_SetProperty(WND_PLUGIN_SOURCE_TOTAL_INFO, "string", PLUGIN_STR_BLANK);
                for(i = 0;i < MAX_PAGE_ITEM; i++)
                {
                    GUI_SetProperty(s_text_source_focus[i], "backcolor", TXT_BACKCOLOR_UNFOCUS);
                    GUI_SetProperty(s_img_source_unfocus[i], "img", PLUGIN_IMG_NULL);
                    GUI_SetProperty(s_text_source_title[i], "string", PLUGIN_STR_BLANK);
                }
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        case MSG_UPDATE_ITEM_IMAGE:
            {
                this_focus_info.gif_status = 0;
                if(pop_msg_on)
                    _app_plugin_source_hide_popup_msg();
                if(msg->image_path != NULL)
                {
                    gal_add_key_path(key_img[msg->item], msg->image_path);
                    GUI_SetProperty(s_img_source_unfocus[msg->item], "img", key_img[msg->item]);
                    GUI_SetProperty(s_img_source_unfocus[msg->item], "state", "show");
                }
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        case MSG_PLAY:
            {
                if(strncmp(source_url, "audio", 5) == 0)
                    app_plugin_source_play(IMG_MUSIC);
                else
                    app_plugin_source_play(NULL);
                break;
            }
        case MSG_GROUP_LIST_FOCUS:
            {
                GUI_SetProperty(WND_LIST_GROUP, "focus_img", LISTVIEW_FOCUS_IMG);
                GUI_SetProperty(WND_LIST_GROUP, "select", &msg->item);
                GUI_SetProperty(WND_PLUGIN_SOURCE_NAME, "string", PLUGIN_STR_BLANK);
                break;
            }
        case MSG_GROUP_LIST_UNFOCUS:
            {
                GUI_SetProperty(WND_LIST_GROUP, "focus_img", LISTVIEW_UNFOCUS_IMG);
                break;
            }
        case MSG_GROUP_LIST_SHOW:
            {
                char *text = NULL;
                GUI_GetProperty(WND_PLUGIN_SOURCE_SHORTDESC, "string", &text);
                if(text && strcmp(text, PLUGIN_STR_BLANK))
                    GUI_SetProperty(WND_PLUGIN_SOURCE_SHORTDESC, "string", PLUGIN_STR_BLANK);
                GUI_SetProperty(WND_LIST_GROUP, "state", "show");
                GUI_SetProperty(WND_PLUGIN_LISTVIEW_TITLE, "string", STR_ID_LIST_VIEW_TITLE);
                if((GXCORE_ERROR == GUI_CheckDialog(WND_POP_BOOK))&&(GXCORE_ERROR == GUI_CheckDialog(WND_POP_TIP)))
                {
                    GUI_SetFocusWidget(WND_LIST_GROUP);
                    GUI_SetProperty(WND_LIST_GROUP, "select", &msg->item);
                }
                app_update_pop_dlg(WND_POP_TIP);
                break;
            }
        case MSG_GROUP_LIST_HIDE:
            {
                GUI_SetProperty(WND_LIST_GROUP, "state", "hide");
                GUI_SetProperty(WND_PLUGIN_LISTVIEW_TITLE, "string", STR_SHORTDESC);
                GUI_SetProperty(WND_PLUGIN_SOURCE_PAGE_INFO, "string", PLUGIN_STR_BLANK);
                break;
            }
        case MSG_UPDATE_PAGE_INFO:
            {
                page = 1 + (msg->item - (msg->item % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
                max_page = 1 + ((msg->total-1) - ((msg->total-1) % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
                snprintf(page_info, sizeof(page_info), "%d/%d", page, max_page);
                GUI_SetProperty(WND_PLUGIN_SOURCE_PAGE_INFO, "string", page_info);
                if(msg->node.title != NULL)
                {
                    GUI_SetProperty(WND_PLUGIN_SOURCE_TOTAL_INFO, "string", msg->node.title);
                    APP_FREE(msg->node.title);
                }
                else
                {
                    GUI_SetProperty(WND_PLUGIN_SOURCE_TOTAL_INFO, "string", PLUGIN_STR_BLANK);
                }
                app_update_pop_dlg(WND_POP_TIP);
                break;
            }
        case MSG_FOCUS_ITEM:
            {
                gdi_lock();
                GUI_SetProperty(s_text_source_focus[(msg->item)%MAX_PAGE_ITEM], "backcolor", TXT_FORECOLOR_FOCUS);
                GUI_SetProperty(s_text_source_title[(msg->item)%MAX_PAGE_ITEM], "forecolor", TXT_FOCUS_COLOR);
                GUI_SetProperty(s_img_source_unfocus[(msg->item)%MAX_PAGE_ITEM], "update", NULL);
                gdi_unlock();
                _plugin_source_path_show();
                if(UPDATE_SOURCE_CHILD == this_focus_info.update_flag)
                    GUI_SetProperty(WND_PLUGIN_SOURCE_SHORTDESC, "string", msg->node.shortdesc);
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        case MSG_UNFOCUS_ITEM:
            {
                gdi_lock();
                GUI_SetProperty(s_text_source_focus[msg->item%MAX_PAGE_ITEM], "backcolor", TXT_BACKCOLOR_UNFOCUS);
                GUI_SetProperty(s_text_source_title[(msg->item)%MAX_PAGE_ITEM], "forecolor", TXT_UNFOCUS_COLOR);
                GUI_SetProperty(s_img_source_unfocus[(msg->item)%MAX_PAGE_ITEM], "update", NULL);
                gdi_unlock();
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        case MSG_UNFOCUS_ALL_ITEM:
            {
                int32_t i;

                for(i = 0; i < MAX_PAGE_ITEM; i++)
                {
                    GUI_SetProperty(s_text_source_focus[i], "backcolor", TXT_BACKCOLOR_UNFOCUS);
                    GUI_SetProperty(s_text_source_title[i], "forecolor", TXT_UNFOCUS_COLOR);
                    GUI_SetProperty(s_img_source_unfocus[i], "update", NULL);
                }
                break;
            }
        case MSG_SOURCE_DIG_END:
            {
                _plugin_source_free_gif();
                GUI_EndDialog(WND_PLUGIN_SOURCE_LIST);
                break;
            }
        case MSG_PLAY_DIG_CREAT:
            {
                GUI_CreateDialog(WND_NETVIDEO_PLAY);
                break;
            }
        case MSG_PLAY_DIG_END:
            {
                GUI_EndDialog(WND_NETVIDEO_PLAY);
                break;
            }
        case MSG_GROUP_LIST_UPDATE:
            {
                GUI_SetProperty(WND_LIST_GROUP, "update_all", NULL);
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        case MSG_SHOW_GIF:
            {
                if(this_focus_info.update_flag == UPDATE_CLASSIFICATION || this_focus_info.update_flag ==  UPDATE_CLASSIFICATION_OK)
                {
                    GUI_SetProperty(WND_PLUGIN_SOURCE_NAME, "string", "Getting data from network,Please wait!");
                }
                app_plugin_source_show_gif();
                break;
            }
        case MSG_HIDE_GIF:
            {
                if(this_focus_info.update_flag == UPDATE_CLASSIFICATION || this_focus_info.update_flag ==  UPDATE_CLASSIFICATION_OK)
                {
                    GUI_SetProperty(WND_PLUGIN_SOURCE_NAME, "string", PLUGIN_STR_BLANK);
                }
                app_plugin_source_hide_gif();
                break;
            }
        case MSG_SHOW_POPUP:
            {
                _app_plugin_source_show_popup_msg(msg->status, msg->item);
                break;
            }
        case MSG_HIDE_POPUP:
            {
                _app_plugin_source_hide_popup_msg();
                break;
            }
        case MSG_CREATE_KEYBOARD:
            {
                app_plugin_source_hide_gif();
                if(pop_msg_on)
                    _app_plugin_source_hide_popup_msg();
                GUI_SetProperty(WND_PLUGIN_SOURCE_PAGE_INFO, "string", PLUGIN_STR_BLANK);
                GUI_SetProperty(WND_PLUGIN_SOURCE_TOTAL_INFO, "string", PLUGIN_STR_BLANK);
                for(i = 0;i < MAX_PAGE_ITEM; i++)
                {
                    GUI_SetProperty(s_text_source_focus[i], "backcolor", TXT_BACKCOLOR_UNFOCUS);
                    GUI_SetProperty(s_img_source_unfocus[i], "img", PLUGIN_IMG_NULL);
                    GUI_SetProperty(s_text_source_title[i], "string", PLUGIN_STR_BLANK);
                }
                app_plugin_source_keyboard_create();
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        default:
            break;
    }
}

int app_plugin_source_msg_proc(GxMessage *msg)
{
    struct PlugiaSourceUIMsg *ui_msg;

    if(msg == NULL || GXCORE_ERROR == GUI_CheckDialog(WND_PLUGIN_SOURCE_LIST))
        return EVENT_TRANSFER_STOP;

    switch(msg->msg_id)
    {
        case APPMSG_PLUGIN_SOURCE_UI_CONTROL:
            {
                ui_msg = GxBus_GetMsgPropertyPtr(msg, struct PlugiaSourceUIMsg);
                _plugin_source_ui_contral(ui_msg);
                break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

static bool _plugin_check_err_type(void)
{
    PluginInfoNode *node;

    node = Pluginstore_focus_node_get();
    if(UPDATE_CLASSIFICATION == this_focus_info.update_flag
            || UPDATE_END == this_focus_info.update_flag
            || (node->ui_type != NULL
                && 0 == strcmp(UI_TYPE_SIGNLE, node->ui_type)
                && UPDATE_SOURCE_CHILD == this_focus_info.update_flag))
    {
        return true;
    }
    return false;
}

void app_plugin_source_list_deal_err_from_ecmascript(void)
{
    if(false == thiz_wnd_created)
        return;

    if(this_focus_info.update_flag != UPDATE_SOURCE_PLAY)
        app_plugin_source_show_popup_msg(STR_ID_PLUGIN_ERR, 0);

    if(true == _plugin_check_err_type())
        app_exit_abort = 1;
}

#endif
