#include "app.h"
#include "gx_apps.h"
#include "app_windows.h"
#include "app_iptv_list_service.h"
#include "app_common_select.h"
#include "app_module.h"

#ifdef IPTV_CLOUD_SUPPORT
#define IPTV_CLOUD_LIST_SELECT_KEY   "iptvcloud->active_sel"
#define IPTV_CLOUD_LIST_SELECT_VALUE   0

#define IPTV_CLOUD_LIST     g_CommonSelect.widget.list

#define IPTV_SERVER_URL_LENGHT   (256)
#define IPTV_USER_SERVER_PATH "/home/gx/iptv_userserver.xml"

typedef enum {
    ADD_MODE = 0,
    EDIT_MODE,
} InputMode;

static bool s_force_update_list_flag = false;
static int iptv_source_select = 0;
static int iptv_source_active = 0;
static int input_pop_mode = ADD_MODE;

char iptv_server_url_orig[IPTV_SERVER_URL_LENGHT]={0};
char iptv_server_key[IPTV_SERVER_URL_LENGHT]={0};
char iptv_server_url[IPTV_SERVER_URL_LENGHT]={0};
char iptv_server_type[IPTV_SERVER_URL_LENGHT]={0};
gx_list_t  *_get_remote_iptv_addserver(void);
extern char *_get_remote_server_name(int value);
extern char *_get_remote_server_url(int value);
extern void app_iptv_cloud_source_change_notice(void);
extern gx_list_t *_load_remote_iptv_server_list(void);
extern int app_iptv_server_usb_add(void);

void set_input_pop_mode(int value)
{
    input_pop_mode = value;
}

int get_input_pop_mode(void)
{
    return input_pop_mode;
}

void set_iptv_source_select(int value)
{
    iptv_source_select = value;
}

int get_iptv_source_select(void)
{
    return iptv_source_select;
}

void set_active_item(int value)
{
    iptv_source_active = value;
    GxBus_ConfigSetInt(IPTV_CLOUD_LIST_SELECT_KEY, iptv_source_active);
}

int get_active_item(void)
{
    GxBus_ConfigGetInt(IPTV_CLOUD_LIST_SELECT_KEY, (int*)&iptv_source_active, IPTV_CLOUD_LIST_SELECT_VALUE);

    return iptv_source_active;
}

void app_iptv_cloud_green_key_func(void)
{
    int sel = 0;
    int value = 0;
    GUI_GetProperty(IPTV_CLOUD_LIST, "select", &sel);
    gx_list_delete_item(_get_remote_iptv_addserver(), sel);
    gx_list_save_to_file(_get_remote_iptv_addserver(), IPTV_USER_SERVER_PATH);
    _load_remote_iptv_server_list();

    GUI_SetProperty(IPTV_CLOUD_LIST, "update_all", NULL);
    if ((sel == get_active_item()) && (_get_remote_channels_total() > 0))
    {
        value = 0;
        set_active_item(0);//the first iptv server
        GUI_SetProperty(IPTV_CLOUD_LIST, "select", &value);
        GUI_SetProperty(IPTV_CLOUD_LIST, "active", &value);
    }
    else if (sel < get_active_item())
    {
        set_active_item(get_active_item() - 1); //the first iptv server
        value = get_active_item();
        GUI_SetProperty(IPTV_CLOUD_LIST, "active", &value);
        GUI_SetProperty(IPTV_CLOUD_LIST, "select", &value);
    }
    else
    {
        sel = sel - 1;
        GUI_SetProperty(IPTV_CLOUD_LIST, "select", &sel);
        value = get_active_item();
        GUI_SetProperty(IPTV_CLOUD_LIST, "active", &value);
    }
}

void app_iptv_cloud_blue_key_func(void)
{
    int ret = -1;
    ret = app_iptv_server_usb_add();
    if (ret == -1)
        return;
    _load_remote_iptv_server_list();
    set_active_item(0);
    GUI_SetProperty(IPTV_CLOUD_LIST, "update_all", NULL);
}

int app_iptv_cloud_create(GuiWidget *widget, void *usrdata)
{
    int value = 0;
    char *url = NULL;

    if (_get_remote_iptv_addserver() == NULL)
        _load_remote_iptv_server_list();

    s_force_update_list_flag = false;
    value = get_active_item();
    memset(iptv_server_url_orig, 0, IPTV_SERVER_URL_LENGHT);
    if ((url = _get_remote_server_url(value)) != NULL)
    {
        strncpy(iptv_server_url_orig, url, IPTV_SERVER_URL_LENGHT-1);
    }
    GUI_SetProperty(IPTV_CLOUD_LIST, "select", &value);
    GUI_SetProperty(IPTV_CLOUD_LIST, "active", &value);

    return EVENT_TRANSFER_STOP;
}

int app_iptv_cloud_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    int value = 0;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch (find_virtualkey_ex(event->key.scancode, event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_UP:
                case STBK_DOWN:
                case STBK_PAGE_DOWN:
                case STBK_PAGE_UP:
                    break;

                case STBK_RED:
                    {
                        set_input_pop_mode(ADD_MODE);
                        GUI_CreateDialog(WND_IPTV_CLOUD_INPUT);
                    }
                    break;
                case STBK_YELLOW:
                    {
                        if (_get_remote_channels_total() > 0)
                        {
                            GUI_GetProperty(IPTV_CLOUD_LIST, "select", &value);
                            if (value < 0)
                            {
                                value = 0;
                                GUI_SetProperty(IPTV_CLOUD_LIST, "select", &value);
                            }
                            set_iptv_source_select(value);
                            set_input_pop_mode(EDIT_MODE);
                            GUI_CreateDialog(WND_IPTV_CLOUD_INPUT);
                        }
                    }
                    break;
                case STBK_GREEN:
                    {
                        if (_get_remote_channels_total() > 0)
                        {
                            app_iptv_cloud_green_key_func();
                        }
                    }
                    break;
                case STBK_BLUE:
                    app_iptv_cloud_blue_key_func();
                    break;

                default:
                    break;
            }

        default:
            break;
    }
    return ret;
}

int app_iptv_cloud_list_keypress(GuiWidget *widget, void *usrdata)
{
    int  ret = EVENT_TRANSFER_STOP;
    int  sel = 0;
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;

    if (event->type == GUI_KEYDOWN)
    {
        switch (find_virtualkey_ex(event->key.scancode, event->key.sym))
        {
            case STBK_MENU:
            case STBK_EXIT:
                {
                    int value = get_active_item();
                    char *url = _get_remote_server_url(value);

                    if (s_force_update_list_flag
                            || (url && strcmp(iptv_server_url_orig, url) != 0)
                            || (!url && strlen(iptv_server_url_orig) == 0))
                    {
                        app_iptv_cloud_source_change_notice();
                    }
                    GUI_EndDialog(g_CommonSelect.widget.wnd);
                }
                break;

            case STBK_OK:
                GUI_GetProperty(IPTV_CLOUD_LIST, "select", &sel);
                if (sel < 0)
                {
                    sel = 0;
                    GUI_SetProperty(IPTV_CLOUD_LIST, "select", &sel);
                }
                GUI_SetProperty(IPTV_CLOUD_LIST, "active", &sel);
                set_active_item(sel);
                s_force_update_list_flag = true;
                break;

            case STBK_UP:
            case STBK_DOWN:
            case STBK_PAGE_DOWN:
            case STBK_PAGE_UP:
            case STBK_RED:
            case STBK_YELLOW:
            case STBK_GREEN:
            case STBK_BLUE:
                ret = EVENT_TRANSFER_KEEPON;
                break;

            default:
                break;
        }
    }

    return ret;
}

int app_iptv_cloud_list_get_total(GuiWidget *widget, void *usrdata)
{
    return _get_remote_channels_total();
}

int app_iptv_cloud_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    static char buffer[10];
    item = (ListItemPara *)usrdata;

    if (NULL == item)
        return GXCORE_ERROR;
    if (0 > item->sel)
        return GXCORE_ERROR;

    //col-0: channel num
    item->x_offset = 2;
    item->image = NULL;
    memset(buffer, 0, 10);
    sprintf(buffer, "%03d", (item->sel + 1));
    item->string = buffer;

    //col-1: channel name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    item->string = _get_remote_server_name(item->sel);

    return EVENT_TRANSFER_STOP;
}
int app_iptv_cloud_got_focus(GuiWidget *widget, void *usrdata)
{
    int sel = get_active_item();
    GUI_EndDialog(WND_IPTV_CLOUD_INPUT);
    GUI_SetProperty(IPTV_CLOUD_LIST, "update_all", NULL);
    GUI_GetProperty(IPTV_CLOUD_LIST, "select", &sel);
    GUI_SetProperty(IPTV_CLOUD_LIST, "active", &sel);

    return EVENT_TRANSFER_STOP;
}

#define __IPTV_CLOUD_INPUT__
#define TXT_IPTV_CLOUD_INPUT_NAME       "text_iptv_cloud_input_name_content"
#define TXT_IPTV_CLOUD_INPUT_URL        "text_iptv_cloud_input_url_content"
#define TXT_IPTV_CLOUD_INPUT_TITLE      "text_iptv_cloud_input_title"
#define BOX_IPTV_CLOUD_INPUT             "box_iptv_cloud_input"

//===================iptv edit==================================
static void app_iptv_url_save(void)
{
    memset(&iptv_server_type, 0x0, sizeof(iptv_server_type));
    strcpy(iptv_server_type, "ALL");
    if (get_input_pop_mode() == ADD_MODE)
    {
        gx_list_add_item(_get_remote_iptv_addserver(), iptv_server_key, iptv_server_url, iptv_server_type);
    }
    else if (get_input_pop_mode() == EDIT_MODE)
    {
        gx_list_edit_item(_get_remote_iptv_addserver(), get_iptv_source_select(), iptv_server_key, iptv_server_url, iptv_server_type);
    }

    gx_list_save_to_file(_get_remote_iptv_addserver(), IPTV_USER_SERVER_PATH);

    _load_remote_iptv_server_list();
}

void app_iptv_server_url_manual_add(void)
{
    memset(&iptv_server_key, 0x0, sizeof(iptv_server_key));
    memset(&iptv_server_url, 0x0, sizeof(iptv_server_url));
    memset(&iptv_server_type, 0x0, sizeof(iptv_server_type));
    strcpy(iptv_server_url, "http://");
    GUI_SetProperty(TXT_IPTV_CLOUD_INPUT_TITLE, "string", STR_ID_IPTV_ADD_LINK);
}

void app_iptv_server_url_manual_edit(int value)
{
    memset(&iptv_server_key, 0x0, sizeof(iptv_server_key));
    memset(&iptv_server_url, 0x0, sizeof(iptv_server_url));
    memset(&iptv_server_type, 0x0, sizeof(iptv_server_type));
    strcpy(iptv_server_key, _get_remote_server_name(value));
    strcpy(iptv_server_url, _get_remote_server_url(value));
    GUI_SetProperty(TXT_IPTV_CLOUD_INPUT_NAME, "string", iptv_server_key);
    GUI_SetProperty(TXT_IPTV_CLOUD_INPUT_URL, "string", iptv_server_url);
    GUI_SetProperty(TXT_IPTV_CLOUD_INPUT_TITLE, "string", STR_ID_IPTV_ED_LINK);
}

static void _iptv_edit_key_release_cb(PopKeyboard *data)
{

    if (data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;
    strcpy(iptv_server_key, data->out_name);

    GUI_SetProperty(TXT_IPTV_CLOUD_INPUT_NAME, "string", data->out_name);
}

static void _iptv_edit_url_release_cb(PopKeyboard *data)
{

    if (data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;
    strcpy(iptv_server_url, data->out_name);

    GUI_SetProperty(TXT_IPTV_CLOUD_INPUT_URL, "string", data->out_name);
}

static void _iptv_edit_type_release_cb(PopKeyboard *data)
{

    if (data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;
    strcpy(iptv_server_type, data->out_name);

    GUI_SetProperty(TXT_IPTV_CLOUD_INPUT_NAME, "string", data->out_name);
}

static void _iptv_edit_ok_keypress(void)
{
#define WND_KEYBOARD_X 330
#define WND_KEYBOARD_Y 130

    int sel = 0;
    static PopKeyboard keyboard;
    memset(&keyboard, 0, sizeof(PopKeyboard));

    keyboard.pos.x = WND_KEYBOARD_X;
    keyboard.pos.y = WND_KEYBOARD_Y;

    GUI_GetProperty(TXT_IPTV_CLOUD_INPUT_URL, "string", &keyboard.in_name);
    GUI_GetProperty(BOX_IPTV_CLOUD_INPUT, "select", &sel);

    keyboard.max_num    = IPTV_SERVER_URL_LENGHT - 1;
    keyboard.out_name	= NULL;
    keyboard.change_cb	= NULL;
    if (sel == 0)
    {
        keyboard.in_name	= iptv_server_key;
        keyboard.release_cb = _iptv_edit_key_release_cb;
    }
    else if (sel == 1)
    {
        keyboard.in_name	= iptv_server_url;
        keyboard.release_cb = _iptv_edit_url_release_cb;
    }
    else
    {
        keyboard.in_name	= iptv_server_type;
        keyboard.release_cb = _iptv_edit_type_release_cb;
    }
    keyboard.usr_data	= NULL;

    multi_language_keyboard_create(&keyboard);
}

static int app_iptv_cloud_input_pop_exit(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        app_iptv_url_save();
        GUI_EndDialog(WND_IPTV_CLOUD_INPUT);
    }

    return 0;
}

SIGNAL_HANDLER int app_iptv_cloud_input_create(GuiWidget *widget, void *usrdata)
{
    if (get_input_pop_mode() == EDIT_MODE)
    {
        app_iptv_server_url_manual_edit(get_iptv_source_select());
    }
    else if (get_input_pop_mode() == ADD_MODE)
    {
        app_iptv_server_url_manual_add();
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_cloud_input_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_iptv_cloud_input_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;

    if (GUI_KEYDOWN == event->type)
    {
        switch (find_virtualkey_ex(event->key.scancode, event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                {
                    GUI_EndDialog("after wnd_full_screen");
                }
                break;
            case STBK_YELLOW:
            case STBK_EXIT:
                {
                    GUI_EndDialog(WND_IPTV_CLOUD_INPUT);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;
            case STBK_BLUE:
                {
                    if ((strlen(iptv_server_key) == 0) || (strlen(iptv_server_url) == 0))
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = "server name or server url input is empty, do you want to exit?";
                        pop.exit_cb = app_iptv_cloud_input_pop_exit;
                        popdlg_create(&pop);

                        ret = EVENT_TRANSFER_STOP;
                        break ;
                    }

                    app_iptv_url_save();
                    GUI_EndDialog(WND_IPTV_CLOUD_INPUT);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;
            case STBK_OK:
                {
                    _iptv_edit_ok_keypress();
                    ret = EVENT_TRANSFER_STOP;
                }
                break;
            case STBK_UP:
            case STBK_DOWN:
                {
                    ret = EVENT_TRANSFER_KEEPON;
                }
                break;
            default:
                break;
        }
    }

    return ret;
}

int app_iptv_cloud_create_dialog(void)
{
    CommonSelectParas paras = {0};

    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = STR_ID_IPTV_LOCAL_LIST;
    paras.tip_strs.str_red = STR_ID_ADD;
    paras.tip_strs.str_green = STR_ID_DELETE;
    paras.tip_strs.str_yellow = STR_ID_EDIT;
    paras.tip_strs.str_blue = "Import";
    paras.signal.got_focus = app_iptv_cloud_got_focus;
    paras.signal.create = app_iptv_cloud_create;
    paras.signal.keypress = app_iptv_cloud_keypress;
    paras.signal.list_get_total = app_iptv_cloud_list_get_total;
    paras.signal.list_get_data = app_iptv_cloud_list_get_data;
    paras.signal.list_keypress = app_iptv_cloud_list_keypress;
    app_common_select_create_dialog(&paras);

    return 0;
}

#endif
