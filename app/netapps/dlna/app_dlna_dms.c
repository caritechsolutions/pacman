#include "app.h"
#if DLNADMS_SUPPORT
#include "app_windows.h"
#include "app_keyboard_language.h"
#include "dlna.h"
#include "app_module.h"
#include "full_screen.h"
#include "app_dlna.h"
#include <gx_apps.h>
#include "file_manage.h"

#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#define DLNA_DMS_UUID    "dlna>dmsuuid"
#define TXT_POPUP            "txt_dlna_dms_popup"
#define IMG_POPUP            "img_dlna_dms_popup"

#define USER_ID 55


struct ThisUICtrol {
    int server_running;
    ubyte32 ip;
    AppMsg_DlnaUIControl ui_msg;
    DMSCtrlCallback dms_ctrl;
    event_list *msg_timer;
};

static struct ThisUICtrol this;

static int _get_ip(char *ip)
{
    GxMsgProperty_NetDevIpcfg s_ip_cfg;

    memset(&s_ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &s_ip_cfg.dev_name);
    if(s_ip_cfg.dev_name[0] == 0)
        return -1;
    app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_ip_cfg);
    strcpy(ip, s_ip_cfg.ip_addr);
    return 0;
}

static void _dms_show_poptips(char *msg)
{
	GUI_SetProperty(TXT_POPUP, "string", msg);
	GUI_SetProperty(IMG_POPUP, "state", "show");
	GUI_SetProperty(TXT_POPUP, "state", "show");
	GUI_SetInterface("flush", NULL);
}

static void _dms_hide_poptips(void)
{
	GUI_SetProperty(TXT_POPUP, "state", "hide");
	GUI_SetProperty(IMG_POPUP, "state", "hide");
}
static void _dms_show_popdlg(char *str, int32_t time_out)
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

static int _object_get_cur_id(const char* obj_id)
{
    int id = 0;
    char *cur_str = NULL;

    cur_str = strrchr(obj_id, '$');
    if(NULL != cur_str)
        id = atoi(cur_str+1);
    else
        id = atoi(obj_id);

    return id;
}

static char _dec2hex(short int c)
{
    if(0 <= c && c <= 9)
    {
        return c + '0';
    }
    else if(10 <= c && c <= 15)
    {
        return c + 'A' - 10;
    }
    else
    {
        app_log_error("dms _dec2hex error\n");
        return -1;
    }
}

static int _encode_uri(char *url, int max_len)
{
    int i = 0;
    int j, i0, i1;
    int len = strlen(url);
    int res_len = 0;
    char *res;
    char c;

    if(max_len <= 0)
    {
        app_log_error("dms _encode_uri error, max_len<=0\n");
        return -1;
    }

    res = (char *) GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == res)
    {
        app_log_error("%s,%d malloc error\n", __func__, __LINE__);
        return -1;
    }

    for(i = 0; i < len; ++i)
    {
        c = url[i];
        if(('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '/' || c == '.')
        {
            res[res_len++] = c;
        }
        else
        {
            j = (short int)c;
            if(j < 0)
                j += 256;
            i1 = j / 16;
            i0 = j - i1 * 16;
            res[res_len++] = '%';
            res[res_len++] = _dec2hex(i1);
            res[res_len++] = _dec2hex(i0);
        }

        if(res_len > max_len)
        {
            app_log_error("dms _encode_uri error, filename too long\n");
            GxCore_Free(res);
            return -1;
        }
    }
    res[res_len] = '\0';
    strlcpy(url, res, max_len);

    GxCore_Free(res);
    return 0;
}

static int app_dlna_dms_get_content_array(const char *object_id, DMSContentInfo **info_arry, int *total, int *update_id)
{
    int count = 0;
    int cur_id = 0;
    int i = 0;
    int ret = -1;
    int len = 0;
    char *path = NULL;
    char *tmp_path = NULL;
    char tmp[MAX_ID_STR_LEN];
    static  DMSContentInfo *p_info = NULL;
    char *ptitle = NULL;
    FileDirectoryNode *dir_node = NULL;

    path = (char *) GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == path)
    {
        app_log_error("%s,%d malloc error\n", __func__, __LINE__);
        return -1;
    }

    tmp_path = (char *) GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == tmp_path)
    {
        app_log_error("%s,%d malloc error\n", __func__, __LINE__);
        goto error;
    }


    cur_id = _object_get_cur_id(object_id);
    if(0 == cur_id)
    {
        count = file_get_device_total();
        p_info = (DMSContentInfo *)GxCore_Calloc(count, sizeof(DMSContentInfo));
        if(NULL == p_info)
        {
            app_log_error("dms callback malloc error\n");
            goto error;
        }

        for(i = 0; i < count; i++)
        {
            FileDeviceManage *dev_manage = file_get_device_info(i);
            p_info[i].media_type = DLNA_CONTAINER;
            p_info[i].child_count = dev_manage->dir_tree->nents;
            snprintf(tmp, MAX_ID_STR_LEN, "%d", i+USER_ID);
            memcpy(p_info[i].id, tmp, MAX_ID_STR_LEN);
            memcpy(p_info[i].parent_id, "0", MAX_ID_STR_LEN);
            ptitle = strdup(dev_manage->device_name);
            p_info[i].title = ptitle;

        }
    }
    else
    {
        ret = file_get_directory_info_by_object_id(USER_ID, object_id, &dir_node, path);
        if(ret < 0)
        {
            app_log_error("get directory info error\n");
            goto error;
        }
        count = dir_node->nents;
        p_info = (DMSContentInfo *)GxCore_Calloc(count, sizeof(DMSContentInfo));
        if(NULL == p_info)
        {
            app_log_error("dms callback malloc error\n");
            goto error;
        }

        ret =  _encode_uri(path, MAX_PATH_LEN);
        if(ret < 0)
        {
            goto error;
        }

        snprintf(tmp_path, MAX_PATH_LEN, "http://%s:8085%s/", this.ip, path);
        len = strlen(tmp_path);
        i = 0;
        while(count--)
        {
            GxDirent* ent = &(dir_node->ents[i]);
            if(GX_FILE_DIRECTORY == ent->ftype)
            {
                p_info[i].media_type = DLNA_CONTAINER;
                p_info[i].child_count = dir_node->child[i].nents;

            }
            else if(GX_FILE_REGULAR == ent->ftype)
            {
                snprintf(tmp_path+len, MAX_PATH_LEN-len, "%s", ent->fname);
                ret = _encode_uri(tmp_path+len, MAX_PATH_LEN-len);
                if(ret < 0)
                {
                    continue;
                }
                p_info[i].media_type = file_get_dlna_media_type(ent->fname);
                p_info[i].child_count = 0;
                p_info[i].size = ent->fsize;
                p_info[i].url = strdup(tmp_path);
            }
            snprintf(tmp, MAX_ID_STR_LEN, "%s$%d",object_id, i+1);
            memcpy(p_info[i].id, tmp, MAX_ID_STR_LEN);
            snprintf(tmp, MAX_ID_STR_LEN, "%s", object_id);
            memcpy(p_info[i].parent_id, tmp, MAX_ID_STR_LEN);
            p_info[i].title = strdup(ent->fname);
            i++;
        }
    }

    *info_arry = p_info;
    *total = i;
    *update_id = USER_ID;
    GxCore_Free(path);
    GxCore_Free(tmp_path);

    return 0;

error:
    if(path)
        GxCore_Free(path);
    if(tmp_path)
        GxCore_Free(tmp_path);
    if(p_info)
        GxCore_Free(p_info);

    return -1;
}

void app_create_dlna_dms_menu(void)
{
    ubyte32 ip_address = {0};

    if(_get_ip(ip_address) < 0)
    {
        _dms_show_popdlg(STR_ID_LINK_NETWORK, 2);
        return;
    }

    if(GUI_CheckDialog(WND_DLNA_DMS) == GXCORE_ERROR)
    {
#if SAT2IP_SERVER_SUPPORT
		if (app_sat2ip_operate_tip_get_force() < 0)
			return;
		app_sat2ip_unset_gui_ok_fast();
#endif
#if DVB2IP_SERVER_SUPPORT
		if (app_dvb2ip_operate_tip_get_force() < 0)
			return;
		app_dvb2ip_gui_flag_set(0);
#endif
		memset(&this, 0, sizeof(struct ThisUICtrol));
        GUI_CreateDialog(WND_DLNA_DMS);
    }
}

static int app_dlna_dms_error(DLNAErrorNoEnum err_no, const char *err_str)
{
    AppMsg_DlnaUIControl ui_msg = {0};

    ui_msg.type = UI_DLNA_ERROR;
    ui_msg.err_str = (char *)err_str;
    app_send_msg_exec(APPMSG_DLNA_UI_CONTROL, &ui_msg);
    app_log_error("app_dlan_dms_error:%s\n", err_str);
    return 0;
}

char *_dms_get_uuid(void)
{
    static char uuid[42];
    int i = 0;

	GxBus_ConfigGet(DLNA_DMS_UUID, uuid, sizeof(uuid), "NULL");
    if(strncmp(uuid, "NULL", 4) == 0)
    {
        srand((unsigned int)time(NULL));
        sprintf(uuid,"%s", "uuid:");
        for(i = 5; i < 41; i++)
        {
            sprintf(uuid + i, "%x", rand()%0xf);
            if(i == 12 || i == 17 || i == 22 || i == 27)
            {
                i++;
                sprintf(uuid + i, "%s", "-");
            }
        }
        GxBus_ConfigSet(DLNA_DMS_UUID, uuid);
    }
    app_log_debug("dms %s\n", uuid);
	return uuid;
}

extern void Http_web_server_start(void);
int app_dms_media_server_start(char *host_ip)
{
    Http_web_server_start();
    this.server_running = 1;
	return 0;
}

extern void Http_web_server_stop(void);
void app_dms_media_server_stop(void)
{
    if(1 == this.server_running)
    {
        this.server_running = 0;
        Http_web_server_stop();
    }
}

static int app_dlna_dms_init(void)
{
    DMSConfig config = {0};
    int ret = 0;

    popdlg_destroy();
    if(file_manage_init() <= 0)
    {
        app_log_info("no usb devices \n");
        _dms_show_poptips(STR_ID_DMS_USB_ERR);
        return EVENT_TRANSFER_STOP;
    }

    if(file_directory_tree_create() < 0)
    {
        app_log_error("file tree create error \n");
        _dms_show_poptips(STR_ID_DMS_FILE_ERR);
        return EVENT_TRANSFER_STOP;
    }

    if(_get_ip(this.ip) < 0)
    {
        app_log_error("ip address get error\n");
        _dms_show_poptips(STR_ID_LINK_NETWORK);
        file_directory_tree_free();
        return EVENT_TRANSFER_STOP;
    }

    config.ip_address = this.ip;
    config.port = 23333;
    config.friendly_name = "Lucky DMS";
    config.udn = _dms_get_uuid();
    config.logo_path = "/dvb/theme/image/netapps/media_server.png"; //只支持png

    this.dms_ctrl.get_content_array = app_dlna_dms_get_content_array;
    this.dms_ctrl.error = app_dlna_dms_error;

    ret = DMS_start(&config, &this.dms_ctrl);
    if(ret != 0)
    {
        _dms_show_poptips(STR_ID_DMS_START_ERR);
    }

    _dms_show_poptips(STR_ID_DMS_START);
    app_dms_media_server_start(this.ip);

    return EVENT_TRANSFER_STOP;

}

static int app_dlna_dms_source_release(void)
{
    if(this.server_running == 1)
    {
        DMS_stop();
        file_directory_tree_free();
        app_dms_media_server_stop();
    }
    return 0;
}

SIGNAL_HANDLER int app_dlna_dms_create(GuiWidget *widget, void *usrdata)
{
    app_dlna_dms_init();
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_dlna_dms_destroy(GuiWidget *widget, void *usrdata)
{
    app_dlna_dms_source_release();
    _dms_hide_poptips();

#if SAT2IP_SERVER_SUPPORT
	app_sat2ip_set_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
	return EVENT_TRANSFER_STOP;
}

static int _dms_exit_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        GUI_EndDialog(WND_DLNA_DMS);
    }
    return 0;
}

SIGNAL_HANDLER int app_dlna_dms_keypress(GuiWidget *widget, void *usrdata)
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
                {
                    PopDlg pop = {0};
                    pop.type = POP_TYPE_YES_NO;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_STOP_DMS_INFO;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.exit_cb = _dms_exit_pop_cb;
                    popdlg_create(&pop);
                }
                break;
            default:
                break;
        }
    }
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dlna_dms_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_dlna_dms_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static int app_dlna_dms_ui_msg_proc_exec(void *usrdata)
{
    AppMsg_DlnaUIControl *ui_msg = &this.ui_msg;

    APP_TIMER_REMOVE(this.msg_timer);
    switch(ui_msg->type)
    {
        case UI_DLNA_ERROR:
        {
            _dms_show_popdlg(ui_msg->err_str, 1);
            break;
        }
        default:
            break;
    }

    return 0;
}

int app_dlna_dms_ui_msg_proc(GxMessage *msg)
{
    AppMsg_DlnaUIControl *ui_msg;

    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    ui_msg = GxBus_GetMsgPropertyPtr(msg, AppMsg_DlnaUIControl);
    switch(ui_msg->type)
    {
        case UI_DLNA_ERROR:
            memcpy(&this.ui_msg, ui_msg, sizeof(this.ui_msg));
            APP_TIMER_ADD(this.msg_timer, app_dlna_dms_ui_msg_proc_exec, 10, TIMER_REPEAT);
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

int app_dlna_dms_hotplug_msg_proc(GxMessage *msg)
{
    switch(msg->msg_id)
    {
        case GXMSG_HOTPLUG_IN:
            if(this.server_running == 0)
            {
                if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
                    app_dlna_dms_init();
                else
                    _dms_show_poptips(STR_ID_LINK_NETWORK);
            }
            break;
        case GXMSG_HOTPLUG_OUT:
            {
                if(this.server_running == 1)
                {
                    HotplugPartitionList *partition_list = NULL;
                    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
                    if(partition_list->partition_num <=0)
                    {
                        app_dlna_dms_source_release();
                        popdlg_destroy();
                        _dms_show_poptips(STR_ID_DMS_USB_ERR);
                    }
                }
            }
            break;
        case GXMSG_NETWORK_PLUG_OUT:
            if(this.server_running == 1)
            {
                ubyte32 ip_address = {0};
                if(_get_ip(ip_address) < 0)
                {
                    app_dlna_dms_source_release();
                    popdlg_destroy();
                    _dms_show_poptips(STR_ID_LINK_NETWORK);
                }
            }
            break;
        case GXMSG_NETWORK_STATE_REPORT:
            {
                if(this.server_running == 0)
                {
                    GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                    if(dev_state->dev_state == IF_STATE_CONNECTED)
                    {
                        app_dlna_dms_init();
                    }
                }
            }
            break;
    }
    return EVENT_TRANSFER_STOP;
}
#endif
