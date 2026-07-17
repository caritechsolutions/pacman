/*************************************************************************
	> File Name: app_plugin_manage.c
	> Author:
	> Mail:
	> Created Time: 2016年08月26日 星期五 14时19分54秒
 ************************************************************************/

#include "app.h"
#if PLUGINSTORE_SUPPORT&&PLUGINONLINE_SUPPORT
#include "app_plugin_update.h"
#include "gx_pluginstore_api.h"
#include "app_plugin_server.h"
#include "../app_netapps_service.h"
#include "gx_apps.h"
#include "app_windows.h"

/*--------------------widget macro---------------------------*/
#define WND_PLUGIN_MANAGE_PAGE_INFO    "text_plugin_manage_page_info"
#define WND_PLUGIN_MANAGE_TITLE        "text_plugin_manage_title"
#define WND_LIST_GROUP                 "list_plugin_manage_group"
#define MAX_PAGE_ITEM                  (6)
#define MAX_PAGE_LINE                  (2)
#define MAX_FILE_NAME_LEN              (128)
#define IMAGE_DOWN_PATH                TMP_DOWNLOAD_PATH
#define IMG_DEFAULT                    WORK_PATH"theme/image/netapps/default.png"
#define WND_SHORT_DESC                 "text_plugin_manage_item_shortdesc"
#define IMG_GIF                        "img_plugin_manage_gif"
#define GIF_PATH                       WORK_PATH"theme/image/netapps/loading.gif"
#define LISTVIEW_FOCUS_IMG             "feeds_item"
#define LISTVIEW_UNFOCUS_IMG           "feeds_itembg_sel"
#define TXT_POPUP                      "txt_plugin_manage_popup"
#define IMG_POPUP                      "img_plugin_manage_popup"

enum UPDATE_STATUS{
    UPDATE_CLASSIFICATION,
    UPDATE_SOURCE,
};

enum MSG_CONTROL_STATUS{
    MSG_FOCUS_ITEM,
    MSG_UNFOCUS_ITEM,
    MSG_UNFOCUS_ALL_ITEM,
    MSG_CLEAN_ALL_ITEM,
    MSG_UPDATE_ITEM,
    MSG_UPDATE_ITEM_IMAGE,
    MSG_UPDATE_PAGE_INFO,
    MSG_GROUP_LIST_FOCUS,
    MSG_GROUP_LIST_UPDATE,
    MSG_SHOW_GIF,
    MSG_HIDE_GIF,
    MSG_SHOW_POPUP,
    MSG_HIDE_POPUP,
};

struct UpdateNodeFocusInfo{
    int total_number;
    int focus_item;
    PluginUpdateNode *node;
};

struct PLuginClassificationFocusInfo{
    int total_number;
    int focus_item;
    int flag;
    int pic_download_abort;
    handle_t mutex;
    int wait_abort;
    int data_get_abort;
    struct UpdateNodeFocusInfo source;
    PluginClassificationNode *node;
    PLuginClassificationList *head; //need lock
};

static char *this_pic_path[MAX_PAGE_ITEM] = {NULL};
extern char *get_fname_suffix_from_url(char *url);
extern PluginInfoNode *Pluginstore_focus_node_get(void);
static struct PLuginClassificationFocusInfo this_focus_info = {0}; //set .source data by app_netapps_service_push
static int _plugin_manage_info_draw(void);
static handle_t item_image_update_thread_handle[MAX_PAGE_ITEM] = {-1};
static event_list* s_plugin_manage_gif_timer = NULL;
static event_list* s_plugin_manage_popup_timer = NULL;
static bool pop_msg_on = false;
static event_list* s_plugin_update_timer = NULL;

static void _plugin_manage_source_exit(void);

static char *s_text_manage_title[MAX_PAGE_ITEM] =
{
    "text_plugin_manage_item_title1",
    "text_plugin_manage_item_title2",
    "text_plugin_manage_item_title3",
    "text_plugin_manage_item_title4",
    "text_plugin_manage_item_title5",
    "text_plugin_manage_item_title6",
};

static char *s_img_manage_focus[MAX_PAGE_ITEM] =
{
    "img_plugin_manage_item_focus1",
    "img_plugin_manage_item_focus2",
    "img_plugin_manage_item_focus3",
    "img_plugin_manage_item_focus4",
    "img_plugin_manage_item_focus5",
    "img_plugin_manage_item_focus6",
};

static char *s_img_manage_unfocus[MAX_PAGE_ITEM] =
{
    "img_plugin_manage_item_unfocus1",
    "img_plugin_manage_item_unfocus2",
    "img_plugin_manage_item_unfocus3",
    "img_plugin_manage_item_unfocus4",
    "img_plugin_manage_item_unfocus5",
    "img_plugin_manage_item_unfocus6",
};

static char *s_text_manage_status[MAX_PAGE_ITEM] =
{
    "text_plugin_manage_item_status1",
    "text_plugin_manage_item_status2",
    "text_plugin_manage_item_status3",
    "text_plugin_manage_item_status4",
    "text_plugin_manage_item_status5",
    "text_plugin_manage_item_status6",
};

static char *key_img[MAX_PAGE_ITEM]=
{
    "plugin_manage_img_key1",
    "plugin_manage_img_key2",
    "plugin_manage_img_key3",
    "plugin_manage_img_key4",
    "plugin_manage_img_key5",
    "plugin_manage_img_key6",
};

static void _app_plugin_manage_hide_popup_msg(void)
{
    APP_TIMER_REMOVE(s_plugin_manage_popup_timer);

    pop_msg_on = false;
    GUI_SetProperty(TXT_POPUP, "state", "hide");
    GUI_SetProperty(IMG_POPUP, "state", "hide");
}

static void app_plugin_manage_hide_popup_msg(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_HIDE_POPUP;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static int _plugin_manage_popup_timer_timeout(void *userdata)
{
    APP_TIMER_REMOVE(s_plugin_manage_popup_timer);
    app_plugin_manage_hide_popup_msg();
    return 0;
}

static void _app_plugin_manage_show_popup_msg(char* pMsg, int time_out)
{
    GUI_SetInterface("flush", NULL);
    pop_msg_on = true;
    GUI_SetProperty(TXT_POPUP, "string", pMsg);
    GUI_SetProperty(IMG_POPUP, "state", "show");
    GUI_SetProperty(TXT_POPUP, "state", "show");

    if(time_out > 0)
    {
        APP_TIMER_ADD(s_plugin_manage_popup_timer, _plugin_manage_popup_timer_timeout, time_out, TIMER_REPEAT);
    }
}

static void app_plugin_manage_show_popup_msg(char* pMsg, int time_out)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_SHOW_POPUP;
    param.status = pMsg;
    param.item = time_out;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static int app_plugin_manage_draw_gif(void* usrdata)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
    return 0;
}

static void _plugin_manage_load_gif(void)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
    GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _plugin_manage_free_gif(void)
{
    APP_TIMER_REMOVE(s_plugin_manage_gif_timer);
    GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void app_plugin_manage_show_gif(void)
{
    GUI_SetProperty(IMG_GIF, "state", "show");
    APP_TIMER_ADD(s_plugin_manage_gif_timer, app_plugin_manage_draw_gif, 100, TIMER_REPEAT);
}

static void app_plugin_manage_hide_gif(void)
{
    if(s_plugin_manage_gif_timer)
        timer_stop(s_plugin_manage_gif_timer);

    GUI_SetProperty(IMG_GIF, "state", "hide");
}

static void _plugin_manage_focus_item(int index)
{
    struct PlugiaSourceUIMsg param = {0};

    if(index >= 0)
    {
        param.type = MSG_FOCUS_ITEM;
        param.item = index;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    }
}

static void _plugin_manage_unfocus_all_item(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_UNFOCUS_ALL_ITEM;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_manage_show_state(void)
{
    struct PlugiaSourceUIMsg param = {0};

    this_focus_info.wait_abort = 1;
    param.type = MSG_SHOW_GIF;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static void _plugin_manage_hide_state(void)
{
    struct PlugiaSourceUIMsg param = {0};

    this_focus_info.wait_abort = 0;
    param.type = MSG_HIDE_GIF;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static void _plugin_manage_unfocus_item(int index)
{
    struct PlugiaSourceUIMsg param = {0};

    if(index >= 0)
    {
        param.type = MSG_UNFOCUS_ITEM;
        param.item = index;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
    }
}

static void _plugin_manage_group_list_update(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_GROUP_LIST_UPDATE;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_manage_group_list_focus(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_GROUP_LIST_FOCUS;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _plugin_manage_clean_all_item(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_CLEAN_ALL_ITEM;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);

    return;
}

static void _list_view_focus_info_item_init(void)
{
    this_focus_info.focus_item = 0;
    if(this_focus_info.head != NULL && this_focus_info.head->list.next != NULL)
        this_focus_info.node = list_entry(this_focus_info.head->list.next, typeof(*this_focus_info.head), list);
}

static void _update_focus_info_item_init(void)
{
    this_focus_info.source.focus_item = 0;
    this_focus_info.source.total_number = 0;
    if(this_focus_info.node->update_list_head != NULL && this_focus_info.node->update_list_head->list.next != NULL)
    {
        this_focus_info.source.node = list_entry(this_focus_info.node->update_list_head->list.next, typeof(*this_focus_info.node->update_list_head), list);
        this_focus_info.source.total_number = this_focus_info.node->update_list_head->counter;
        PLUGIN_INFO("total_number:%d", this_focus_info.source.total_number);
        _plugin_manage_info_draw();
    }
}

static void _plugin_manage_update_page_info(void)
{
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_UPDATE_PAGE_INFO;
    param.item = this_focus_info.source.focus_item;
    param.total = this_focus_info.source.total_number;
    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static void _plugin_manage_show_title(void)
{
    PluginInfoNode *node;

    node = Pluginstore_focus_node_get();
    if(node->title != NULL)
        GUI_SetProperty(WND_PLUGIN_MANAGE_TITLE, "string", node->title);
}

static void _plugin_get_update_data(void *usrdata)
{
    GxCore_ThreadDetach();

    this_focus_info.data_get_abort = 0;
    GxCore_MutexLock(this_focus_info.mutex);
    if(this_focus_info.head != NULL)
        Plugin_classification_list_free(&this_focus_info.head);
    Plugin_classification_list_get(&this_focus_info.head);
    GxCore_MutexUnlock(this_focus_info.mutex);
    this_focus_info.data_get_abort = 1;
}

static int _plugin_get_update_data_wrap(void *data)
{
    static int thread_id = 0;

    GxCore_ThreadCreate("get_update_data", &thread_id, _plugin_get_update_data,
            NULL, 64 * 1024, GXOS_DEFAULT_PRIORITY);
    return 0;
}
void Plugin_update_timer_start(void)
{
    if(s_plugin_update_timer != NULL)
    {
        this_focus_info.data_get_abort = 1;
        return;
    }
    if(0 == this_focus_info.mutex)
        GxCore_MutexCreate(&this_focus_info.mutex);
    s_plugin_update_timer = create_timer(_plugin_get_update_data_wrap, 12*60*60*1000, NULL, TIMER_REPEAT_NOW);
}

void Plugin_update_timer_stop(void)
{
    APP_TIMER_REMOVE(s_plugin_update_timer);
}

void Plugin_update_timer_delete(void)
{
    APP_TIMER_REMOVE(s_plugin_update_timer);
    Plugin_download_stop();
    GxCore_MutexLock(this_focus_info.mutex);
    if(this_focus_info.head != NULL)
        Plugin_classification_list_free(&this_focus_info.head);
    GxCore_MutexUnlock(this_focus_info.mutex);
}

static void _plugin_manage_data_get(void)
{
    _plugin_manage_show_state();
    while(!this_focus_info.data_get_abort)
    {
        GxCore_ThreadDelay(10);
    }
    Plugin_update_timer_stop();
    _plugin_manage_hide_state();
    if(this_focus_info.head != NULL)
    {
        this_focus_info.total_number = this_focus_info.head->counter;
        if(this_focus_info.total_number <= 1)
        {
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.mode = POP_MODE_UNBLOCK;
            pop.str = STR_ID_PLUGIN_ERR;
            pop.timeout_sec = 2;
            popdlg_create(&pop);
            app_update_pop_dlg(WND_POP_BOOK);
        }
        _list_view_focus_info_item_init();
        _update_focus_info_item_init();

        _plugin_manage_group_list_focus();
        _plugin_manage_group_list_update();
        this_focus_info.flag = UPDATE_CLASSIFICATION;
    }
}

static void _pic_download_cb(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
    int i = 0;
    struct PlugiaSourceUIMsg param = {0};

    if(message == GXCURL_STATE_COMPLETE)
    {
        if(this_focus_info.pic_download_abort)
            return;
        if(callback_data->handle > 0)
        {
            for(i = 0; i < MAX_PAGE_ITEM; i++)
            {
                if(item_image_update_thread_handle[i] == callback_data->handle && callback_data->complete_data.size > 0)
                {
                    if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
                    {
                        PLUGIN_INFO("*****%d****ok*****", i);
                        memset(&param, 0, sizeof(struct PlugiaSourceUIMsg));
                        param.type = MSG_UPDATE_ITEM_IMAGE;
                        param.item = i;
                        param.image_path = GxCore_Strdup(this_pic_path[i]);
                        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
                    }
                    else
                    {
                        PLUGIN_ERR("*****%d****error*****", i);
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

static void _item_image_update(char *url, int index)
{
    char *path = NULL;
    uint32_t length = 0;

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
        return;
    }

    snprintf(path, length, "%s/%d%s", IMAGE_DOWN_PATH, index, get_fname_suffix_from_url(url));

    APP_FREE(this_pic_path[index]);
    this_pic_path[index] = path;
    item_image_update_thread_handle[index] = gxapps_curl_async_download(url, path, _pic_download_cb);

    return;
}

static void _draw_image(char *url, int index)
{
    char *path = NULL;
    uint32_t length = 0;
    struct PlugiaSourceUIMsg param = {0};

    param.type = MSG_UPDATE_ITEM_IMAGE;
    param.item = index;
    if(strncmp(url, "zip://", 6) == 0)
    {
        length = strlen(TMP_DOWNLOAD_PATH) + strlen(get_fname_suffix_from_url(url)) + 20;
        if(length >= MAX_PATH_LEN)
        {
            PLUGIN_ERR("path length exceeds the maximnum(%d) limit\n", MAX_PATH_LEN);
            return;
        }

        path = GxCore_Calloc(sizeof(char), length);
        if(NULL == path)
        {
            PLUGIN_ERR("No memory to GxCore_Calloc\n");
            return;
        }

        snprintf(path, length, "%s/%d%s", TMP_DOWNLOAD_PATH, index, get_fname_suffix_from_url(url));
        Plugin_zip_decompression(url, path);
        param.image_path = path;
    }
    else if(strncmp(url, "http", 4) == 0)
    {
        param.image_path = GxCore_Strdup(IMG_DEFAULT);
        _item_image_update(url, index);
    }
    else
    {
        param.image_path = GxCore_Strdup(url);
    }

    app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
}

static int _plugin_manage_info_draw(void)
{
    PluginUpdateNode *pos;
    PluginUpdateNode *node;
    PluginUpdateList **head;
    int i = 0;
    struct PlugiaSourceUIMsg param = {0};

    _pic_download_stop();
    head = &this_focus_info.node->update_list_head;

    node = list_entry(this_focus_info.source.node->list.prev, typeof(*(this_focus_info.source.node)), list);
    list_from_node(pos, &(node->list), list, *head)
    {
        if(i >= MAX_PAGE_ITEM)
            break;

        memset(&param, 0, sizeof(struct PlugiaSourceUIMsg));
        param.type = MSG_UPDATE_ITEM;
        param.node.title = pos->name;
        param.item = i;
        param.status = pos->status;
        app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
        _draw_image(pos->icon, i);
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
    _plugin_manage_update_page_info();

    return 0;
}

static void _plugin_manage_up_keypress(void)
{
    PluginUpdateNode *pos;
    PluginUpdateNode *node;
    PluginUpdateList *head;
    int i, j, total_number, focus_item;

    node = this_focus_info.source.node;
    head = this_focus_info.node->update_list_head;
    total_number = this_focus_info.source.total_number;
    focus_item = i = this_focus_info.source.focus_item;
    j = 0;

    if(((i%MAX_PAGE_ITEM)-MAX_PAGE_ITEM/MAX_PAGE_LINE) < 0)
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i > total_number-1)
            return;
        list_from_node(pos, &(node->list), list, head)
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
        list_from_node_reverse(pos, &(node->list), list, head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
            return;
    }

    _plugin_manage_unfocus_item(focus_item);
    this_focus_info.source.focus_item = i;
    this_focus_info.source.node = pos;
    _plugin_manage_focus_item(i);
}

static void _plugin_manage_down_keypress(void)
{
    PluginUpdateNode *pos;
    PluginUpdateNode *node;
    PluginUpdateList *head;
    int i, j, total_number, focus_item;

    node = this_focus_info.source.node;
    head = this_focus_info.node->update_list_head;
    total_number = this_focus_info.source.total_number;
    focus_item = i = this_focus_info.source.focus_item;
    j = 0;

    if(((i%MAX_PAGE_ITEM)+MAX_PAGE_ITEM/MAX_PAGE_LINE) < MAX_PAGE_ITEM)
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i > total_number-1)
            return;
        list_from_node(pos, &(node->list), list, head)
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
        list_from_node_reverse(pos, &(node->list), list, head)
        {
            j++;
            if(j >= MAX_PAGE_ITEM/MAX_PAGE_LINE)
                break;
        }
        if(j < MAX_PAGE_ITEM/MAX_PAGE_LINE)
            return;
    }

    _plugin_manage_unfocus_item(focus_item);
    this_focus_info.source.focus_item = i;
    this_focus_info.source.node = pos;
    _plugin_manage_focus_item(i);
}

static void _plugin_manage_left_keypress(void)
{
    PluginUpdateNode *node;
    PluginUpdateList *head;
    PluginUpdateNode *pos;
    int i = 0, j = 0;
    int page, focus_item;

    node = this_focus_info.source.node;
    head = this_focus_info.node->update_list_head;
    focus_item = i = this_focus_info.source.focus_item;

    if(0 == i%(MAX_PAGE_ITEM/MAX_PAGE_LINE))
    {
        page = (i - i % MAX_PAGE_ITEM) / MAX_PAGE_ITEM;
        if((page-1) < 0)
        {
            _plugin_manage_source_exit();
            return;
        }
        else
        {
            page -= 1;
        }
        this_focus_info.source.focus_item = 0;

        i = page * MAX_PAGE_ITEM;
        list_from_node(pos, &(head->list), list, head)
        {
            j++;
            if(j > i)
                break;
        }
        this_focus_info.source.node = pos;
        this_focus_info.source.focus_item = i;
        if(strncmp(this_focus_info.node->name, CLASSIFICATION_INSTALL_NAME, 9) == 0)
        {
            Plugin_installed_check(this_focus_info.node);
            this_focus_info.source.total_number = this_focus_info.node->update_list_head->counter;
        }
        _plugin_manage_info_draw();
        if(this_focus_info.source.total_number <= 0)
        {
            this_focus_info.source.focus_item = focus_item;
            _plugin_manage_source_exit();
            return;
        }
    }
    else
    {
        i -= 1;
        this_focus_info.source.node = list_entry(node->list.prev, typeof(*node), list);
        this_focus_info.source.focus_item = i;
    }

    _plugin_manage_unfocus_item(focus_item);
    _plugin_manage_focus_item(i);
}

static void _plugin_manage_right_keypress(void)
{
    PluginUpdateNode *node;
    PluginUpdateList *head;
    PluginUpdateNode *pos;
    int i = 0, j = 0;
    int max_page, page, total_number, focus_item;

    node = this_focus_info.source.node;
    head = this_focus_info.node->update_list_head;
    total_number = this_focus_info.source.total_number;
    focus_item = i = this_focus_info.source.focus_item;

    if(0 == (i+1)%(MAX_PAGE_ITEM/MAX_PAGE_LINE) || i == (total_number - 1))
    {
        if(total_number <= MAX_PAGE_ITEM)
        {
            return;
        }
        else
        {
            max_page = ((total_number-1) - ((total_number-1) % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
            page = (i - i % MAX_PAGE_ITEM) / MAX_PAGE_ITEM;
            if((page + 1) > max_page)
                page = 0;
            else
                page += 1;

            i = page * MAX_PAGE_ITEM;
            list_from_node(pos, &(head->list), list, head)
            {
                j++;
                if(j > i)
                    break;
            }
            this_focus_info.source.node = pos;
            this_focus_info.source.focus_item = i;
            if((page == 0) && strncmp(this_focus_info.node->name, CLASSIFICATION_INSTALL_NAME, 9) == 0)
            {
                Plugin_installed_check(this_focus_info.node);
                this_focus_info.source.total_number = this_focus_info.node->update_list_head->counter;
                _update_focus_info_item_init();
            }
            _plugin_manage_unfocus_item(focus_item);
            _plugin_manage_info_draw();
            if(this_focus_info.source.total_number <= 0)
            {
                this_focus_info.source.focus_item = focus_item;
                _plugin_manage_source_exit();
                return;
            }

        }
    }
    else
    {
        i += 1;
        if(i > total_number-1)
            return;
        this_focus_info.source.node = list_entry(node->list.next, typeof(*node), list);
        this_focus_info.source.focus_item = i;
        _plugin_manage_unfocus_item(focus_item);
    }

    _plugin_manage_focus_item(i);
}

status_t app_create_plugin_manage_menu(void)
{
    if(GUI_CheckDialog(WND_PLUGIN_MANAGE) == GXCORE_ERROR)
    {
        GUI_CreateDialog(WND_PLUGIN_MANAGE);
    }
    GUI_SetInterface("flush", NULL);
    return 0;
}

static void _plugin_manage_ok_keypress(void)
{
    int ret = -1;
    struct PlugiaSourceUIMsg param = {0};

    if(this_focus_info.source.node->ins_ver == NULL)
    {
        app_plugin_manage_show_popup_msg(STR_ID_PLUGIN_INSTALING_TIP, 0);
        ret = Plugin_download_install(this_focus_info.source.node, this_focus_info.node);
        if(ret == 0)
        {
            pop_msg_on = true;
            app_plugin_manage_show_popup_msg(STR_ID_INSTALLED, 1000);
            while(pop_msg_on)
            {
                GxCore_ThreadDelay(10);
            }

            param.type = MSG_UPDATE_ITEM;
            param.node.title = this_focus_info.source.node->name;
            param.item = this_focus_info.source.focus_item;
            param.status = this_focus_info.source.node->status;
            app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
        }
        else
        {
            app_plugin_manage_show_popup_msg("Install to fail!", 0);
        }
    }
    else if(strncmp(this_focus_info.source.node->status, "Upgrade", 7) == 0)
    {
        app_plugin_manage_show_popup_msg("Updating ,Wait Please!", 0);
        ret = Plugin_download_update(this_focus_info.source.node);
        if(ret == 0)
        {
            pop_msg_on = true;
            app_plugin_manage_show_popup_msg("Updated!", 1000);
            while(pop_msg_on)
            {
                GxCore_ThreadDelay(10);
            }

            param.type = MSG_UPDATE_ITEM;
            param.node.title = this_focus_info.source.node->name;
            param.item = this_focus_info.source.focus_item;
            param.status = this_focus_info.source.node->status;
            app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
        }
        else
        {
            app_plugin_manage_show_popup_msg("Updating fail!", 0);
        }
    }
}

static void _plugin_manage_update_after_uninstall(void)
{
    int32_t focus_item = 0;
    int32_t i = 0, j = 0, page = 0;
    PluginUpdateNode *pos = NULL;
    PluginUpdateNode *focus_node = NULL;
    PluginUpdateList *head = NULL;

    focus_item = this_focus_info.source.focus_item;
    Plugin_installed_check(this_focus_info.node);
    if(this_focus_info.source.focus_item <= 0)
        this_focus_info.source.focus_item = 0;
    else
        this_focus_info.source.focus_item--;

    if(this_focus_info.node->update_list_head != NULL && this_focus_info.node->update_list_head->list.next != NULL)
    {
        head = this_focus_info.node->update_list_head;
        //get focus node
        i = this_focus_info.source.focus_item;
        list_from_node(pos, &(head->list), list, head)
        {
            j++;
            if(j > i)
                break;
        }
        focus_node = pos;

        //firset set this_focus_info.source.node is the first node in cur page
        //because _plugin_manage_info_draw update ui need from the first one in cur page
        page = (i - i % MAX_PAGE_ITEM) / MAX_PAGE_ITEM;
        i = page * MAX_PAGE_ITEM;
        j = 0;
        list_from_node(pos, &(head->list), list, head)
        {
            j++;
            if(j > i)
                break;
        }

        this_focus_info.source.total_number = this_focus_info.node->update_list_head->counter;

        _plugin_manage_unfocus_item(focus_item);
        if(this_focus_info.node->update_list_head->counter <= 0)
        {
            this_focus_info.source.node = focus_node;
            _plugin_manage_clean_all_item();
            _plugin_manage_source_exit();
        }
        else
        {
            this_focus_info.source.node = pos;
            _plugin_manage_info_draw();
            this_focus_info.source.node = focus_node;
            _plugin_manage_focus_item(this_focus_info.source.focus_item);
        }
    }

    return;
}

static void _plugin_download_uninstall(void)
{
    int ret = -1;
    struct PlugiaSourceUIMsg param = {0};

    if(this_focus_info.source.node->ins_ver == NULL)
        return;

    ret = Plugin_download_uninstall(this_focus_info.source.node, this_focus_info.node);
    if(ret == 0)
    {
        pop_msg_on = true;
        app_plugin_manage_show_popup_msg(STR_ID_PLUGIN_UNINSTALL_FIN, 1000);
        while(pop_msg_on)
            GxCore_ThreadDelay(10);

        if(0 == strncmp(this_focus_info.node->name, CLASSIFICATION_INSTALL_NAME, 9))
            _plugin_manage_update_after_uninstall();
        else
        {
            param.type = MSG_UPDATE_ITEM;
            param.node.title = this_focus_info.source.node->name;
            param.item = this_focus_info.source.focus_item;
            param.status = this_focus_info.source.node->status;
            app_send_msg_exec(APPMSG_PLUGIN_SOURCE_UI_CONTROL, &param);
        }
    }
    else
    {
        app_plugin_manage_show_popup_msg("Uninstall fail!", 0);
    }
}

static void _plugin_manage_source_exit(void)
{
    _plugin_manage_unfocus_item(this_focus_info.source.focus_item);
    this_focus_info.flag = UPDATE_CLASSIFICATION;
    _plugin_manage_group_list_focus();
}

SIGNAL_HANDLER int app_plugin_manage_create(GuiWidget *widget, void *usrdata)
{
    _plugin_manage_unfocus_all_item();
    _plugin_manage_show_title();
    _plugin_manage_load_gif();
    app_netapps_service_push(_plugin_manage_data_get);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_manage_destroy(GuiWidget *widget, void *usrdata)
{
    _plugin_manage_free_gif();
    Plugin_download_stop();
    Plugin_update_timer_start();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_manage_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_OK:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_manage_ok_keypress);
                break;
            case STBK_EXIT:
            case STBK_MENU:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_manage_source_exit);
                else
                    app_plugin_manage_hide_popup_msg();
                break;
            case STBK_UP:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_manage_up_keypress);
                break;
            case STBK_DOWN:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_manage_down_keypress);
                break;
            case STBK_LEFT:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_manage_left_keypress);
                break;
            case STBK_RIGHT:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_manage_right_keypress);
                break;
            case STBK_RED:
                if(!pop_msg_on)
                    app_netapps_service_push(_plugin_download_uninstall);
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

static void _group_list_up_keypress(void)
{
    Plugin_installed_check(this_focus_info.node);
    if(this_focus_info.total_number > 1)
    {
        _plugin_manage_clean_all_item();
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

        _plugin_manage_group_list_focus();
        _update_focus_info_item_init();
    }
}

static void _group_list_down_keypress(void)
{
    Plugin_installed_check(this_focus_info.node);
    if(this_focus_info.total_number > 1)
    {
        _plugin_manage_clean_all_item();
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

        _plugin_manage_group_list_focus();
        _update_focus_info_item_init();
    }
}

SIGNAL_HANDLER int app_plugin_manage_group_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(this_focus_info.flag == UPDATE_SOURCE)
    {
        GUI_SendEvent(WND_PLUGIN_MANAGE, event);
        return ret;
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
                GUI_EndDialog(WND_PLUGIN_MANAGE);
                break;
            case STBK_OK:
            case STBK_RIGHT:
                if(this_focus_info.source.total_number > 0)
                {
                    this_focus_info.flag = UPDATE_SOURCE;
                    _plugin_manage_focus_item(this_focus_info.source.focus_item);
                    GUI_SetProperty(WND_LIST_GROUP, "focus_img", LISTVIEW_UNFOCUS_IMG);
                }
                break;
            case STBK_UP:
                app_netapps_service_push(_group_list_up_keypress);
                break;
            case STBK_DOWN:
                app_netapps_service_push(_group_list_down_keypress);
                break;
            default:
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int app_plugin_manage_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    return this_focus_info.total_number;
}

SIGNAL_HANDLER int app_plugin_manage_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;
    PluginClassificationNode *pos;
    int i = 0;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return EVENT_TRANSFER_STOP;
    if(0 > item->sel)
        return EVENT_TRANSFER_STOP;

    list_from_node(pos, &(this_focus_info.head->list), list, this_focus_info.head)
    {
        i++;
        if(i > item->sel)
            break;
    }
    item->image = NULL;
    item->string = pos->name;

    return EVENT_TRANSFER_STOP;
}

static void _plugin_manage_ui_contral(struct PlugiaSourceUIMsg *msg)
{
    char page_info[20] = {0};
    int page, max_page;

    if(msg == NULL)
        return;

    switch(msg->type)
    {
        case MSG_FOCUS_ITEM:
            {
                GUI_SetProperty(s_img_manage_unfocus[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                GUI_SetProperty(s_img_manage_focus[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[#E78818,#E78818,#E78818]");
                GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[#ee3300,#ee3300,#ee3300]");
                GUI_SetProperty(WND_SHORT_DESC, "string", this_focus_info.source.node->shortdesc);
                break;
            }
        case MSG_UNFOCUS_ITEM:
            {
                GUI_SetProperty(s_img_manage_focus[msg->item%MAX_PAGE_ITEM], "state", "hide");
                GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[text_color,text_color,text_color]");
                GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[#005577,#005577,#005577]");
                break;
            }
        case MSG_UNFOCUS_ALL_ITEM:
            {
                int32_t i;

                for(i = 0; i < MAX_PAGE_ITEM; i++)
                {
                    GUI_SetProperty(s_text_manage_title[i], "forecolor", "[text_color,text_color,text_color]");
                    GUI_SetProperty(s_text_manage_status[i], "forecolor", "[#005577,#005577,#005577]");
                }
                break;
            }
        case MSG_CLEAN_ALL_ITEM:
            {
                int32_t i;
                for(i = 0;i < MAX_PAGE_ITEM; i++)
                {
                    GUI_SetProperty(s_img_manage_unfocus[i], "state", "hide");
                    GUI_SetProperty(s_text_manage_title[i], "state", "hide");
                    GUI_SetProperty(s_text_manage_status[i], "state", "hide");
                }
                break;
            }
        case MSG_UPDATE_ITEM:
            {
                if(msg->node.title != NULL)
                {
                    GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "string", msg->node.title);
                    GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                    GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "string", msg->status);
                    GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                }
                else
                {
                    GUI_SetProperty(s_img_manage_unfocus[(msg->item)%MAX_PAGE_ITEM], "state", "hide");
                    GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "state", "hide");
                    GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "state", "hide");
                }
                break;
            }
        case MSG_UPDATE_ITEM_IMAGE:
            {
                if(msg->image_path != NULL)
                {
                    gal_add_key_path(key_img[msg->item], msg->image_path);
                    GUI_SetProperty(s_img_manage_unfocus[msg->item], "img",key_img[msg->item]);
                    GUI_SetProperty(s_img_manage_unfocus[msg->item], "state", "show");
                    if(pop_msg_on)
                    {
                        GUI_SetProperty(TXT_POPUP, "update", NULL);
                        GUI_SetProperty(IMG_POPUP, "update", NULL);
                    }
                    APP_FREE(msg->image_path);
                }
                break;
            }
        case MSG_GROUP_LIST_FOCUS:
            {
                GUI_SetProperty(WND_LIST_GROUP, "focus_img", LISTVIEW_FOCUS_IMG);
                GUI_SetProperty(WND_LIST_GROUP, "select", &this_focus_info.focus_item);
                GUI_SetProperty(WND_SHORT_DESC, "string", NULL);
                break;
            }
        case MSG_UPDATE_PAGE_INFO:
            {
                page = 1 + (msg->item - (msg->item % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
                max_page = 1 + ((msg->total-1) - ((msg->total-1) % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
                snprintf(page_info, sizeof(page_info), "%d/%d", page, max_page);
                GUI_SetProperty(WND_PLUGIN_MANAGE_PAGE_INFO, "string", page_info);
                GUI_SetProperty(WND_PLUGIN_MANAGE_PAGE_INFO, "state", "show");
                break;
            }
        case MSG_GROUP_LIST_UPDATE:
            {
                GUI_SetProperty(WND_LIST_GROUP, "update_all", NULL);
                break;
            }
        case MSG_SHOW_GIF:
            {
                app_plugin_manage_show_gif();
                break;
            }
        case MSG_HIDE_GIF:
            {
                app_plugin_manage_hide_gif();
                break;
            }
        case MSG_SHOW_POPUP:
            {
                _app_plugin_manage_show_popup_msg(msg->status, msg->item);
                break;
            }
        case MSG_HIDE_POPUP:
            {
                _app_plugin_manage_hide_popup_msg();
                break;
            }
        default:
            break;
    }
}

int app_plugin_manage_msg_proc(GxMessage *msg)
{
    struct PlugiaSourceUIMsg *ui_msg;
    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    switch(msg->msg_id)
    {
        case APPMSG_PLUGIN_SOURCE_UI_CONTROL:
            {
                ui_msg = GxBus_GetMsgPropertyPtr(msg, struct PlugiaSourceUIMsg);
                _plugin_manage_ui_contral(ui_msg);
                app_update_pop_dlg(WND_POP_TIP);
                app_update_pop_dlg(WND_POP_BOOK);
                break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_manage_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_plugin_manage_lost_focus(GuiWidget *widget, void *usrdata)
{
    PLUGIN_INFO("=====>");
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_plugin_manage_got_focus(GuiWidget *widget, void *usrdata)
{
    PLUGIN_INFO("=====>");
    return EVENT_TRANSFER_STOP;
}
#endif
