#include "app.h"
#include "app_msg.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "gxhotplug.h"
#if USB_TEST_SUPPORT
#include "module/app_dir_module.h"
#include "app_usb_test.h"
#define USB_TEST_MAX_DIR_NUM    100
#define USB_TEST_COUNT_ARRAY    {1,2,5,10,20,50,100,200,500,1000}
#define _TERR(fmt, args...)     do { app_log_error("[TERR][%s:%d] ", __func__, __LINE__); app_log_error(fmt, ##args); } while(0)

typedef int (*change_cb_func)(int sel);
typedef int (*press_cb_func)(int key);

typedef enum {
    USB_TEST_ENTRY,
    USB_TEST_DIR,
    USB_TEST_TIMES,
    USB_TEST_START,
    USB_TEST_TOTAL
} UsbTestSettingItem;

typedef struct {
    int sel;
    int num;
    char **value;
    char *content;
    char *title;
    change_cb_func change_cb;
    press_cb_func press_cb;
} UsbTestParam;

typedef struct {
    UsbTestParam param_array[USB_TEST_START];
    SystemSettingItem item_array[USB_TEST_TOTAL];
    int entry_path_len;
    int current_focus_item ;
} UsbTestCtrl;

static UsbTestCtrl s_usb_test_ctrl;

typedef struct {
    char *src;
    char *dst;
    int max_count;
    int file_total;
    int stop_rq;
    handle_t handle;

    uint32_t total_size;
    uint32_t last_size;
    uint32_t start_msec;
    uint32_t last_msec;
    int last_percent;
} UsbTestMgr;

static UsbTestMgr s_usb_test_mgr;

/******** usb test control ********/

static void _usb_test_popup(char *tip)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.exit_cb = NULL;
    pop.str = tip;
    popdlg_create(&pop);
}

void app_usb_test_release(void)
{
    UsbTestMgr *mgr = &s_usb_test_mgr;
    APP_FREE(mgr->src);
    APP_FREE(mgr->dst);
    //mgr->max_count = 0; // linux SIGFPE
}

int app_usb_test_init(int times, const char *entry, const char *dir)
{
#define DST_SUFFIX "_dst"
    UsbTestMgr *mgr = &s_usb_test_mgr;
    int len = 0;

    if (mgr->handle) {
        _TERR("\033[31mwait last test stop!\033[0m\n");
        return -1;
    }
    app_usb_test_release();
    len = strlen(entry) + strlen(dir) + 2;
    mgr->src = GxCore_Calloc(1, len);
    if (!mgr->src) {
        _TERR("malloc failed!\n");
        return -1;
    }
    sprintf(mgr->src, "%s/%s", entry, dir);

    len += strlen(DST_SUFFIX);
    mgr->dst = GxCore_Calloc(1, len);
    if (!mgr->dst) {
        _TERR("malloc failed!\n");
        APP_FREE(mgr->src);
        return -1;
    }
    sprintf(mgr->dst, "%s/%s%s", entry, dir, DST_SUFFIX);

    mgr->max_count = times;

    return 0;
}

int app_usb_test_send_msg(UsbTestMsgType type, UsbTestMsgValue value)
{
    GxMsgProperty_UsbTest msg;
    msg.type = type;
    msg.value = value;
    return app_send_msg_exec(APPMSG_USB_TEST, &msg);
}

static int _usb_test_check(const char *path, int is_dir, size_t dir_count, size_t file_count, ssize_t file_size)
{
    UsbTestMgr *mgr = &s_usb_test_mgr;
    UsbTestMsgValue value = {0};
    uint32_t msec = 0;
    int interval = 0;
    int percent = 0;

    mgr->total_size += file_size;
    msec = app_tick_time_msec_get();
    interval = msec - mgr->last_msec;
    if (interval >= 1000) {
        mgr->last_msec = msec;
        value.speed = 1.0 * (mgr->total_size - mgr->last_size) / interval;
        mgr->last_size = mgr->total_size;
        app_usb_test_send_msg(MSG_USB_TEST_SPEED, value);
    }

    percent = (int)((dir_count + file_count) * 100 / mgr->file_total);
    if (mgr->last_percent != percent) {
        mgr->last_percent = percent;
        value.percent = percent;
        app_usb_test_send_msg(MSG_USB_TEST_PROGRESS, value);
    }

    if (is_dir) {
        app_log_info("\033[34m[%2d%%][%s] %s\033[0m\n", percent, "DIR", path);
    }
    return (mgr->stop_rq) ? -1 : 0;
}


static int _usb_test_check2(const char *path, int is_dir, size_t dir_count, size_t file_count, ssize_t file_size)
{
    UsbTestMgr *mgr = &s_usb_test_mgr;
    UsbTestMsgValue value = {0};
    int percent = 0;

    percent = (int)((dir_count + file_count) * 100 / mgr->file_total);
    if (mgr->last_percent != percent) {
        mgr->last_percent = percent;
        value.percent = percent;
        app_usb_test_send_msg(MSG_USB_TEST_PROGRESS, value);
    }

    if (is_dir) {
        app_log_info("\033[34m[%2d%%][%s] %s\033[0m\n", percent, "DIR", path);
    }
    return (mgr->stop_rq) ? -1 : 0;
}

static void _usb_test_task(void *arg)
{
    UsbTestMgr *mgr = &s_usb_test_mgr;
    UsbTestMsgValue value = {0};
    size_t dir_count = 0, file_count = 0;
    uint32_t msec = 0;
    int i = 0;

    GxCore_ThreadDetach();
    app_log_info("\033[31m--------%s enter--------\033[0m\n", __func__);
    if (app_dir_or_file_tell(mgr->src, 0, NULL, &dir_count, &file_count) < 0) {
        _TERR("\033[31mno files!\033[0m\n");
        goto end;
    }
    mgr->file_total = dir_count + file_count;
    if (mgr->file_total == 0) {
        _TERR("\033[31mno files!\033[0m\n");
        goto end;
    }

    for (i = 0; i < mgr->max_count; i++) {
        app_log_info("\033[32m--------%d/%d--------\033[0m\n", i+1, mgr->max_count);
        value.count = i;
        app_usb_test_send_msg(MSG_USB_TEST_COUNT, value);

        value.stage = USB_TEST_DEL_STAGE;
        app_usb_test_send_msg(MSG_USB_TEST_STAGE, value);
        if (app_dir_or_file_delete(mgr->dst, 0, _usb_test_check2) < 0) {
            app_usb_test_send_msg(MSG_USB_TEST_FAILURE, value);
            goto end;
        }

        mgr->total_size = 0;
        mgr->last_size = 0;
        mgr->start_msec = app_tick_time_msec_get();
        mgr->last_msec = mgr->start_msec;

        value.stage = USB_TEST_COPY_STAGE;
        app_usb_test_send_msg(MSG_USB_TEST_STAGE, value);
        if (app_dir_or_file_copy(mgr->src, mgr->dst, 0, 8192, _usb_test_check) < 0) {
            app_usb_test_send_msg(MSG_USB_TEST_FAILURE, value);
            goto end;
        }
        msec = app_tick_time_msec_get();
        if (msec > mgr->start_msec) {
            value.speed = 1.0 * mgr->total_size / (msec - mgr->start_msec);
            app_usb_test_send_msg(MSG_USB_TEST_SPEED, value);
        }

        value.stage = USB_TEST_CHECK_STAGE;
        app_usb_test_send_msg(MSG_USB_TEST_STAGE, value);
        if (app_dir_or_file_check(mgr->src, mgr->dst, 0, _usb_test_check2) < 0) {
            app_usb_test_send_msg(MSG_USB_TEST_FAILURE, value);
            goto end;
        }

        GxCore_ThreadDelay(1100);
    }

    value.count = mgr->max_count;
    app_usb_test_send_msg(MSG_USB_TEST_COUNT, value);
end:
    app_usb_test_release();
    mgr->stop_rq = 0;
    mgr->handle = 0;
    app_log_info("\033[31m--------%s exit--------\033[0m\n", __func__);
}

int app_usb_test_start(void)
{
    UsbTestMgr *mgr = &s_usb_test_mgr;
    if (mgr->handle) {
        _TERR("\033[31mwait last test stop!\033[0m\n");
        return -1;
    }
    GUI_CreateDialog(WND_USB_TEST);
    GxCore_ThreadCreate("usb_test", &mgr->handle, _usb_test_task, NULL, 16 * 1024, GXOS_DEFAULT_PRIORITY);
    return 0;
}

int app_usb_test_stop(void)
{
    UsbTestMgr *mgr = &s_usb_test_mgr;
    if (mgr->handle) {
        mgr->stop_rq = 1;
    } else {
        app_usb_test_release();
    }

    if (GUI_CheckDialog(WND_USB_TEST) == GXCORE_SUCCESS)
        GUI_EndDialog(WND_USB_TEST);
    return 0;
}

/******** usb test control window ********/

#define PERCENT_CNT     "text_usb_test_count_value"
#define PROGBAR_CNT     "progbar_usb_test_count"
#define PERCENT_PG      "text_usb_test_process_value"
#define PROGBAR_PG      "progbar_usb_test_process"
#define TXT_COUNT       "text_usb_test_count"
#define TXT_SPEED       "text_usb_test_speed"
#define TXT_INFO        "text_usb_test_info"

int app_usb_test_msg_handle(GxMsgProperty_UsbTest param)
{
    UsbTestMgr *mgr = &s_usb_test_mgr;
    int percent = 0;
    char *tip = NULL;
    char per[8] = {0};
    char buf[64] = {0};
    const char *focus = GUI_GetFocusWindow();

    if (!focus || strcmp(focus, WND_USB_TEST) != 0)
        return -1;

    switch (param.type)
    {
        case MSG_USB_TEST_STAGE:
            snprintf(per, sizeof(per), "%d%%", percent);
            GUI_SetProperty(PROGBAR_PG, "value", &percent);
            GUI_SetProperty(PERCENT_PG, "string", per);
            switch(param.value.stage)
            {
                case USB_TEST_DEL_STAGE:
                    tip = "Deleting ...";
                    break;
                case USB_TEST_COPY_STAGE:
                    tip = "Copying ...";
                    break;
                case USB_TEST_CHECK_STAGE:
                    tip = "Checking ...";
                    break;
                default:
                    return -1;
            }
            GUI_SetProperty(TXT_INFO, "string", tip);
            break;
        case MSG_USB_TEST_FAILURE:
            switch(param.value.failure)
            {
                case USB_TEST_DEL_STAGE:
                    tip = "Delete failed!";
                    break;
                case USB_TEST_COPY_STAGE:
                    tip = "Copy failed!";
                    break;
                case USB_TEST_CHECK_STAGE:
                    tip = "Check failed!";
                    break;
                default:
                    return -1;
            }
            GUI_SetProperty(TXT_INFO, "string", tip);
            break;
        case MSG_USB_TEST_SPEED:
            snprintf(buf, sizeof(buf), "%.3f KB/s", param.value.speed);
            GUI_SetProperty(TXT_SPEED, "string", buf);
            break;
        case MSG_USB_TEST_PROGRESS:
            percent = param.value.percent;
            snprintf(per, sizeof(per), "%d%%", percent);
            GUI_SetProperty(PROGBAR_PG, "value", &percent);
            GUI_SetProperty(PERCENT_PG, "string", per);
            break;
        case MSG_USB_TEST_COUNT:
            percent = param.value.count * 100 / mgr->max_count;
            snprintf(buf, sizeof(buf), "%d/%d", param.value.count + 1, mgr->max_count);
            snprintf(per, sizeof(per), "%d%%", percent);
            GUI_SetProperty(PROGBAR_CNT, "value", &percent);
            GUI_SetProperty(PERCENT_CNT, "string", per);
            if (param.value.count == mgr->max_count)
                GUI_SetProperty(TXT_INFO, "string", "USB test succeed!");
            else
                GUI_SetProperty(TXT_COUNT, "string", buf);
            break;
       default:
            return -1;
    }

    return 0;
}

SIGNAL_HANDLER int app_usb_test_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_usb_test_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_usb_test_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch (find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    app_usb_test_stop();
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    app_usb_test_stop();
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

/******** usb test setting window ********/

static char *_cmb_content_dump(char **array, int num)
{
    char *content_str = NULL;
    int content_len = 0;
    int i = 0, cnt = 0, len = 0;

    for (i = 0; i < num; i++) {
        content_len += strlen(array[i]);
    }
    content_len += 2 + num;
    content_str= GxCore_Malloc(content_len);
    if (!content_str) {
        _TERR("malloc failed!\n");
        return NULL;
    }

    content_str[cnt++] = '[';
    for (i = 0; i < num; i++) {
        len = strlen(array[i]);
        memcpy(content_str + cnt, array[i], len);
        cnt += len;
        content_str[cnt++] = ',';
    }
    content_str[cnt-1] = ']';
    content_str[cnt] = 0;

    return content_str;
}

static void _usb_param_free(UsbTestParam *param)
{
    int i = 0;

    for (i = 0; i < param->num; i++) {
        APP_FREE(param->value[i]);
    }
    APP_FREE(param->value);
    APP_FREE(param->content);
    param->sel = 0;
    param->num = 0;
}

static int _usb_test_entry_update(void)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + USB_TEST_ENTRY;
    HotplugPartitionList *list = NULL;
    int i = 0;

    _usb_param_free(param);
    list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (list && list->partition_num) {
        param->value = GxCore_Calloc(list->partition_num, sizeof(char*));
        if (!param->value) {
            goto err;
        }
        for (i = 0; i < list->partition_num; i++) {
            param->value[i] = GxCore_Strdup(list->partition[i].partition_entry);
            if (!param->value[i]) {
                goto err;
            }
            ++param->num;
        }

        param->content = _cmb_content_dump(param->value, param->num);
        if (!param->content) {
            goto err;
        }
        return 0;
    }

err:
    _usb_param_free(param);
    _TERR("malloc failed!\n");
    return -1;
}

static int _dir_update_cb(const char *path, int is_dir, size_t dir_count, size_t file_count, ssize_t file_size)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + USB_TEST_DIR;

    if (is_dir && param->num < USB_TEST_MAX_DIR_NUM && strlen(path) > ctrl->entry_path_len) {
        param->value[param->num] = GxCore_Strdup(path + ctrl->entry_path_len + 1);
        if (!param->value[param->num]) {
            _TERR("malloc failed!\n");
            return -1;
        }
        ++param->num;
    }
    return 0;
}

static int _usb_test_dir_update(void)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + USB_TEST_DIR;
    size_t dir_count = 0, file_count = 0;
    char *path = NULL;

    _usb_param_free(param);
    param->value = GxCore_Calloc(USB_TEST_MAX_DIR_NUM, sizeof(char*));
    if (!param->value) {
        goto err;
    }
    path = ctrl->param_array[USB_TEST_ENTRY].value[ctrl->param_array[USB_TEST_ENTRY].sel];
    ctrl->entry_path_len = strlen(path);
    if (app_dir_or_file_tell(path, 1, _dir_update_cb, &dir_count, &file_count) < 0) {
        goto err;
    }
    if (param->num > 0) {
        param->content = _cmb_content_dump(param->value, param->num);
        if (!param->content) {
            goto err;
        }
    }

    return 0;
err:
    _usb_param_free(param);
    _TERR("malloc failed!\n");
    return -1;
}

static int _usb_test_times_update(void)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + USB_TEST_TIMES;
    char buf[8] = {0};
    int value_array[] = USB_TEST_COUNT_ARRAY;
    int num = sizeof(value_array) / sizeof(value_array[0]);
    int i = 0;

    _usb_param_free(param);
    param->value= GxCore_Calloc(num, sizeof(char*));
    if (!param->value) {
        _TERR("malloc failed!\n");
        return -1;
    }

    for (i = 0; i < num; i++) {
        snprintf(buf, sizeof(buf), "%d", value_array[i]);
        param->value[i] = GxCore_Strdup(buf);
        if (!param->value[i]) {
            goto err;
        }
        ++param->num;
    }
    param->content = _cmb_content_dump(param->value, param->num);
    if (!param->content) {
        goto err;
    }

    return 0;
err:
    _usb_param_free(param);
    _TERR("malloc failed!\n");
    return -1;
}

static int _entry_change_cb(int sel)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + USB_TEST_ENTRY;
    param->sel = sel;
    _usb_test_dir_update();
    param = ctrl->param_array + USB_TEST_DIR;
    if (param->num > 0 && param->content != NULL)
        GUI_SetProperty(app_system_get_combobox(USB_TEST_DIR), "content", param->content);
    else
        GUI_SetProperty(app_system_get_combobox(USB_TEST_DIR), "content", "[None]");
    GUI_SetProperty(app_system_get_combobox(USB_TEST_DIR), "select", &param->sel);
    return EVENT_TRANSFER_KEEPON;
}

static int _dir_change_cb(int sel)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + USB_TEST_DIR;
    param->sel = sel;
    return EVENT_TRANSFER_KEEPON;
}

static int _times_change_cb(int sel)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + USB_TEST_TIMES;
    param->sel = sel;
    return EVENT_TRANSFER_KEEPON;
}

static int _app_av_standard_poplist_cb(int32_t ret_sel, unsigned short key)
{
    int item_sel ;
    item_sel = s_usb_test_ctrl.current_focus_item ;

    GUI_SetProperty(app_system_get_combobox(item_sel), "select", &ret_sel);
    return 0;
}

static int _choice_item_press_cb(int item_sel, int key)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + item_sel;
    PopList pop_list;

    if (key == STBK_OK) {
        if (param->num == 0)
            return EVENT_TRANSFER_STOP;

        app_system_set_item_data_update(item_sel);
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = param->title;
        pop_list.item_num = param->num;
        pop_list.item_content = param->value;
        pop_list.sel = ctrl->item_array[item_sel].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num = false;
        pop_list.mode = POP_MODE_UNBLOCK;
        pop_list.exit_cb = _app_av_standard_poplist_cb;
        ctrl->current_focus_item = item_sel;
        poplist_create(&pop_list);

        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static int _entry_press_cb(int key)
{
    return _choice_item_press_cb(USB_TEST_ENTRY, key);
}

static int _dir_press_cb(int key)
{
    return _choice_item_press_cb(USB_TEST_DIR, key);
}

static int _times_press_cb(int key)
{
    return _choice_item_press_cb(USB_TEST_TIMES, key);
}

static int _start_item_press_cb(unsigned short key)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = NULL;
    bool start_flag = true;
    int i = 0;

    if (key == STBK_OK) {
        for (i = 0; i < USB_TEST_START; i++) {
            param = ctrl->param_array + i;
            if (param->num == 0) {
                start_flag = false;
                break;
            }
            app_system_set_item_data_update(i);
            param->sel = ctrl->item_array[i].itemProperty.itemPropertyCmb.sel;
        }

        if (start_flag) {
            int count_array[] = USB_TEST_COUNT_ARRAY;
            if (app_usb_test_init(count_array[ctrl->param_array[USB_TEST_TIMES].sel],
                    ctrl->param_array[USB_TEST_ENTRY].value[ctrl->param_array[USB_TEST_ENTRY].sel],
                    ctrl->param_array[USB_TEST_DIR].value[ctrl->param_array[USB_TEST_DIR].sel]) < 0) {
                _usb_test_popup("Please wait last test stop!");
            } else {
                if (app_usb_test_start() < 0) {
                    _usb_test_popup("Please wait last test stop!");
                } else {
                    app_system_set_destroy(EXIT_NORMAL);
                }
            }
        } else {
            _usb_test_popup("Invalid test directory!");
        }
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _usb_test_choice_item_init(int item_sel)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = ctrl->param_array + item_sel;
    ctrl->item_array[item_sel].itemTitle = param->title;
    ctrl->item_array[item_sel].itemType = ITEM_CHOICE;
    ctrl->item_array[item_sel].itemProperty.itemPropertyCmb.content = \
        (param->num > 0 && param->content != NULL) ? param->content : "[None]";
    ctrl->item_array[item_sel].itemProperty.itemPropertyCmb.sel = param->sel;
    ctrl->item_array[item_sel].itemCallback.cmbCallback.CmbChange = param->change_cb;
    ctrl->item_array[item_sel].itemCallback.cmbCallback.CmbPress = param->press_cb;
    ctrl->item_array[item_sel].itemStatus = ITEM_NORMAL;
}

static void _usb_test_start_item_init(int item_sel)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    ctrl->item_array[item_sel].itemTitle = STR_ID_START;
    ctrl->item_array[item_sel].itemType = ITEM_PUSH;
    ctrl->item_array[item_sel].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    ctrl->item_array[item_sel].itemCallback.btnCallback.BtnPress = _start_item_press_cb;
    ctrl->item_array[item_sel].itemStatus = ITEM_NORMAL;
}

static void _usb_test_param_release(void)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    int i = 0;
    for (i = 0; i < USB_TEST_START; i++)
        _usb_param_free(ctrl->param_array + i);
}

static int _usb_test_param_init(void)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    UsbTestParam *param = NULL;

    param = ctrl->param_array + USB_TEST_ENTRY;
    if (_usb_test_entry_update() < 0) {
        return -1;
    }
    param->title = "Test Entry";
    param->change_cb = _entry_change_cb;
    param->press_cb = _entry_press_cb;

    param = ctrl->param_array + USB_TEST_DIR;
    _usb_test_dir_update();
    param->title = "Test Directory";
    param->change_cb = _dir_change_cb;
    param->press_cb = _dir_press_cb;

    param = ctrl->param_array + USB_TEST_TIMES;
    _usb_test_times_update();
    param->title = "Test Times";
    param->change_cb = _times_change_cb;
    param->press_cb = _times_press_cb;

    return 0;
}

static int _usb_test_exit_cb(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;
    _usb_test_param_release();
    return ret;
}

static void _usb_test_menu_exec(bool hide_img)
{
    UsbTestCtrl *ctrl = &s_usb_test_ctrl;
    static SystemSettingOpt s_setting_opt;
    int i = 0;

    if (_usb_test_param_init() < 0) {
        _usb_test_popup(STR_ID_INSERT_USB);
        return;
    }
    for (i = 0; i < USB_TEST_START; i++) {
        _usb_test_choice_item_init(i);
    }
    _usb_test_start_item_init(USB_TEST_START);

    s_setting_opt.menuTitle = "USB Test";
    s_setting_opt.itemNum = USB_TEST_TOTAL;
    s_setting_opt.timeDisplay = TIP_HIDE;
    if (hide_img)
    {
        s_setting_opt.topTipImgDisplay = TIP_HIDE;
        s_setting_opt.bottmTipDisplay = TIP_HIDE;
    }
    s_setting_opt.item = ctrl->item_array;
    s_setting_opt.exit = _usb_test_exit_cb;
    app_system_set_create(&s_setting_opt);
    GUI_SetInterface("flush", NULL);
}

void app_usb_test_menu_exec(void)
{
    _usb_test_menu_exec(false);
}

void app_dvbt_usb_test_menu_exec(void)
{
    _usb_test_menu_exec(true);
}
#endif
