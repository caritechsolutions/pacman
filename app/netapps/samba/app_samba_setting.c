#include "app.h"

#if SAMBA_SUPPORT
#include "app_module.h"
#include "app_samba_login.h"
#include "app_windows.h"

#define LV_SAMBA_LIST    "list_samba_setting_samba"
#define TXT_BLUE_TIP    "text_samba_setting_tip_blue"
#define IMG_BLUE_TIP    "img_samba_setting_tip_blue"

#define IMG_NETWORK_CONNECT    "network_connect"
#define IMG_LOCK    "s_lock"

#define SAMBA_DEFAULT_ADDR "//192.168.0.1/share"

static AppSambaListClass *thiz_samba_setting_list = NULL;
static AppSambaInfoClass thiz_samba_login_info;
static event_list *thiz_samba_check_timer = NULL;
AppSambaConnectStatus thiz_samba_connect_status[MAX_SAMBA_SVR_NUM];

static void _samba_setting_pop_msg(char *msg)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.str = msg;
    pop.title = NULL;
    pop.mode = POP_MODE_UNBLOCK;
    pop.create_cb = NULL;
    pop.exit_cb = NULL;
    pop.timeout_sec = 1;
    popdlg_create(&pop);
}

static int _samba_check_network_dev(void)
{
    ubyte64 cur_dev;
    int ret = 1;

    memset(cur_dev, 0, sizeof(ubyte64));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
    if(strlen(cur_dev) == 0)
    {
        ret = 0;
    }
    return ret;
}
static void _samba_setting_info_update(void)
{
    thiz_samba_setting_list = app_get_samba_List();
}

static int _samba_setting_info_num(void)
{
    int num = 0;

    _samba_setting_info_update();
    if(thiz_samba_setting_list != NULL)
        num = thiz_samba_setting_list->samba_num;

    return num;
}

static AppSambaInfoClass *_samba_setting_info_get(uint32_t index)
{
    int num = _samba_setting_info_num();

    if((num > 0) && (index < num))
        return &thiz_samba_setting_list->samba_list[index];

    return NULL;
}

static void _samba_setting_blue_tip_draw(AppSambaConnectStatus samba_status)
{
    if(samba_status == SAMBA_STATUS_DISCONNECTED)
    {
        GUI_SetProperty(IMG_BLUE_TIP, "state", "show");
        GUI_SetProperty(TXT_BLUE_TIP, "state", "show");
        GUI_SetProperty(TXT_BLUE_TIP, "string", STR_ID_CONNECT);
    }
    else if(samba_status == SAMBA_STATUS_CONNECTED)
    {
        GUI_SetProperty(IMG_BLUE_TIP, "state", "show");
        GUI_SetProperty(TXT_BLUE_TIP, "state", "show");
        GUI_SetProperty(TXT_BLUE_TIP, "string", STR_ID_DISCONNECT);
    }
    else
    {
        GUI_SetProperty(IMG_BLUE_TIP, "state", "hide");
        GUI_SetProperty(TXT_BLUE_TIP, "state", "hide");
    }
}

static void _samba_setting_status_draw(uint32_t index, AppSambaConnectStatus samba_status)
{
    int sel;

    GUI_SetProperty(LV_SAMBA_LIST, "update_row", &index);
    GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
    if(sel == index)
    {
        _samba_setting_blue_tip_draw(samba_status);
    }
}

static void _samba_setting_status_update(uint32_t index)
{
    AppSambaInfoClass *samba_inf = NULL;
    AppSambaConnectStatus samba_status;

    samba_inf = _samba_setting_info_get(index);
    if(samba_inf != NULL)
    {
        samba_status = app_samba_check_connect(samba_inf->samba_id);
        if(samba_status != thiz_samba_connect_status[index])
        {
            thiz_samba_connect_status[index] = samba_status;
            _samba_setting_status_draw(index, samba_status);
        }
    }
}

static void _samba_setting_status_update_all(void)
{
    int samba_num, i;

    samba_num = _samba_setting_info_num();
    for(i = 0; i < samba_num; i++)
    {
        _samba_setting_status_update(i);
    }
}

static AppSambaConnectStatus _samba_setting_status_get(uint32_t index)
{
    if(index < MAX_SAMBA_SVR_NUM)
        return thiz_samba_connect_status[index];

    return SAMBA_STATUS_DISCONNECTED;
}

static int _samba_check_timer_cb(void* usrdata)
{
    _samba_setting_status_update_all();
    return 0;
}

static void _samba_check_timer_reset(void)
{
    if(reset_timer(thiz_samba_check_timer) != 0)
    {
        thiz_samba_check_timer = create_timer(_samba_check_timer_cb, 1000, NULL, TIMER_REPEAT);
    }
}

static void _samba_check_timer_stop(void)
{
    remove_timer(thiz_samba_check_timer);
    thiz_samba_check_timer = NULL;
}

SIGNAL_HANDLER int On_samba_setting_create(const char* widgetname, void *userdata)
{
    memset(thiz_samba_connect_status, 0, sizeof(thiz_samba_connect_status));

    _samba_setting_status_update_all();
    _samba_check_timer_reset();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_samba_setting_destroy(const char* widgetname, void *userdata)
{
    _samba_check_timer_stop();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_samba_setting_got_focus(const char* widgetname, void *userdata)
{
    int sel = -1;

    GUI_SetProperty(LV_SAMBA_LIST, "active", &sel);
    _samba_check_timer_reset();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_samba_setting_lost_focus(const char* widgetname, void *userdata)
{
    int sel;

    _samba_check_timer_stop();
    GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
    GUI_SetProperty(LV_SAMBA_LIST, "active", &sel);

    return EVENT_TRANSFER_STOP;
}

static void _samba_setting_blue_key_proc(void)
{
    //connect/disconnect
    int sel;
    AppSambaInfoClass *samba_inf = NULL;
    AppSambaConnectStatus status;

    GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
    samba_inf = _samba_setting_info_get(sel);
    if(samba_inf != NULL)
    {
        status = _samba_setting_status_get(sel);
        if(status != SAMBA_STATUS_CONNECTING)
        {
            if(status == SAMBA_STATUS_CONNECTED)
            {
                app_samba_disconnect(samba_inf->samba_id);
            }
            else
            {
                if(_samba_check_network_dev())
                {
                    app_samba_connect(samba_inf->samba_id);
                }
                else
                {
                    _samba_setting_pop_msg(STR_ID_SERVER_FAIL);
                }
            }
            _samba_setting_status_update(sel);
        }
    }
}

static int _samba_delete_pop_exit_cb(PopDlgRet pop_ret)
{
    int sel;
    AppSambaInfoClass *samba_inf = NULL;
    int samba_num = 0;

    if(pop_ret == POP_VAL_OK)
    {
        GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
        samba_inf = _samba_setting_info_get(sel);
        if(samba_inf != NULL)
        {
            app_samba_delete(samba_inf->samba_id);
            samba_num = _samba_setting_info_num();
            if((samba_num>0)&&(sel>(samba_num-1)))
            {
                sel = samba_num-1;
                GUI_SetProperty(LV_SAMBA_LIST, "select", &sel);
            }
            GUI_SetProperty(LV_SAMBA_LIST, "update_all", NULL);
            _samba_setting_status_update_all();
        }
    }

    return 0;
}

static void _samba_setting_red_key_proc(void)
{
    //delete
    int sel;
    AppSambaInfoClass *samba_inf = NULL;
    PopDlg  pop = {0};

    GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
    samba_inf = _samba_setting_info_get(sel);
    if(samba_inf != NULL)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_SURE_DELETE;
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = _samba_delete_pop_exit_cb;
        popdlg_create(&pop);
    }
}

static bool _samba_info_data_cmp(AppSambaInfoClass *info1, AppSambaInfoClass *info2)
{
    if((strcmp(info1->samba_svr_path, info2->samba_svr_path) == 0)
            && (strcmp(info1->samba_username, info2->samba_username) == 0)
            && (strcmp(info1->samba_passwd, info2->samba_passwd) == 0))
        return true;

    return false;
}

static void _samba_edit_login_exit_cb(bool save_ret, AppSambaInfoClass *samba_out)
{
    int sel = -1;
    AppSambaInfoClass *samba_inf = NULL;

    GUI_SetProperty(LV_SAMBA_LIST, "active", &sel);
    if((save_ret == true) && (samba_out != NULL))
    {
        GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
        samba_inf = _samba_setting_info_get(sel);
        if((samba_inf != NULL) && (_samba_info_data_cmp(samba_inf, samba_out) == false))
        {
            if((strcmp(samba_inf->samba_svr_path, samba_out->samba_svr_path) != 0)
                &&(app_samba_svr_exist(samba_out->samba_svr_path) > 0))
            {
                _samba_setting_pop_msg(STR_ID_DIR_EXISTS);
            }
            else
            {
                if(app_samba_check_connect(samba_inf->samba_id) == true)
                    app_samba_disconnect(samba_inf->samba_id);
                app_samba_modify(samba_inf->samba_id, samba_out);
                app_samba_connect(samba_inf->samba_id);
                _samba_setting_status_update(sel);
            }
        }
    }
}

static void _samba_setting_yellow_key_proc(void)
{
    //edit
    int sel;
    AppSambaInfoClass *samba_inf = NULL;

    GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
    samba_inf = _samba_setting_info_get(sel);
    if(samba_inf != NULL)
    {
        GUI_SetProperty(LV_SAMBA_LIST, "active", &sel);
		GUI_SetInterface("flush",NULL);
        memset(&thiz_samba_login_info, 0, sizeof(AppSambaInfoClass));
        app_create_samba_login(samba_inf, &thiz_samba_login_info, _samba_edit_login_exit_cb);
    }
}

static void _samba_add_login_exit_cb(bool save_ret, AppSambaInfoClass *samba_out)
{
    uint32_t samba_id = 0;
    int sel = 0;
    int samba_num = 0;
    if(save_ret == true)
    {
        if((samba_id = app_samba_new(samba_out)) > 0)
        {
            app_samba_connect(samba_id);
            GUI_SetProperty(LV_SAMBA_LIST, "update_all", NULL);
            samba_num = _samba_setting_info_num();
            if(samba_num>0)
            {
                sel = samba_num-1;
            }
            GUI_SetProperty(LV_SAMBA_LIST, "select", &sel);
            _samba_setting_status_update(sel);
        }
        else
        {
            _samba_setting_pop_msg(STR_ID_DIR_EXISTS);
        }
    }
}

static void _samba_setting_green_key_proc(void)
{
    //add
    if(thiz_samba_setting_list&&(thiz_samba_setting_list->samba_num>=MAX_SAMBA_SVR_NUM))
    {
        _samba_setting_pop_msg(STR_ID_MAX_DIR);
    }
    else
    {
        memset(&thiz_samba_login_info, 0, sizeof(AppSambaInfoClass));
        strlcpy(thiz_samba_login_info.samba_svr_path, SAMBA_DEFAULT_ADDR, sizeof(thiz_samba_login_info.samba_svr_path));
        app_create_samba_login(&thiz_samba_login_info, &thiz_samba_login_info, _samba_add_login_exit_cb);
    }
}

SIGNAL_HANDLER int On_samba_setting_keypress(const char* widgetname, void *userdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)userdata;
    switch(event->type)
    {
        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog(WND_SAMBA);
                    }
                    break;

                case STBK_RED:
                    _samba_setting_red_key_proc();
                    break;

                case STBK_BLUE:
                    _samba_setting_blue_key_proc();
                    break;

                case STBK_YELLOW:
                    _samba_setting_yellow_key_proc();
                    break;

                case STBK_GREEN:
                    _samba_setting_green_key_proc();
                    break;

                case STBK_OK:
                    break;

                default:
                    break;
            }

        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_samba_list_get_total(const char* widgetname, void *userdata)
{
    return _samba_setting_info_num();
}

SIGNAL_HANDLER int On_samba_list_get_data(const char* widgetname, void *userdata)
{
    ListItemPara *item = NULL;
    AppSambaInfoClass *samba_inf = NULL;
    AppSambaConnectStatus status;
    //static char buf[8];

    item = (ListItemPara*)userdata;
    if((NULL == item) ||(0 > item->sel))
        return EVENT_TRANSFER_STOP;

    if((samba_inf = _samba_setting_info_get(item->sel)) != NULL)
    {
        //col-0: need username & password
        item->x_offset = 0;
        if((strlen(samba_inf->samba_username) > 0)
            && (strlen(samba_inf->samba_passwd) > 0))
            item->image = IMG_LOCK;
        else
            item->image = NULL;
        item->string = NULL;

        //col-1: samba svr
        item = item->next;
        item->x_offset = 0;
        item->image = NULL;
        item->string = samba_inf->samba_svr_path;

        //col-2: status
        item = item->next;
        item->x_offset = 0;
        item->image = NULL;
        item->string = NULL;

        status = _samba_setting_status_get(item->sel);
        if(status == SAMBA_STATUS_CONNECTED)
            item->image = IMG_NETWORK_CONNECT;
        else if(status == SAMBA_STATUS_CONNECTING)
            item->string = "......";
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int On_samba_list_change(const char* widgetname, void *userdata)
{
    int sel;
    AppSambaConnectStatus samba_status;

    GUI_GetProperty(LV_SAMBA_LIST, "select", &sel);
    samba_status = _samba_setting_status_get(sel);
    _samba_setting_blue_tip_draw(samba_status);

    return EVENT_TRANSFER_STOP;
}

void app_create_samba_menu(void)
{
    GUI_CreateDialog(WND_SAMBA);
}
#endif

