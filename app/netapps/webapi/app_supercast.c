#include "app_default_params.h"
#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_windows.h"
#include <cJSON.h>
#include <gx_apps.h>
#include "app_main_menu_tip.h"
#include "app_keyboard_language.h"
#include "app_wnd_system_setting_opt.h"

#if WEBAPI_SUPPORT
#include "app_webapi.h"
#define SUPERCAST_ITEM_NUM (2)

#ifdef ECOS_OS
#define APK_PIC_PATH "/mnt/apk_supercast.png"
#define SERVER_PIC_PATH "/mnt/server_supercast.png"
#else
#define APK_PIC_PATH "/tmp/apk_supercast.png"
#define SERVER_PIC_PATH "/tmp/server_supercast.png"
#endif
#define APPK_URL "https://file.dvbcloud.com/supercast/images/apk_download_qrcode.png"
#define VIDEO_URL "https://file.dvbcloud.com/supercast/images/apk_description_qrcode.png"

#define MOED_INFO "txt_supercast_mode"
#define SUPPORT_ON  "SuperCast Support On"
#define SUPPORT_OFF "SuperCast support off, Press red key to configure the function"

enum SUPERCAST_SETTING_ITEM_NUM
{
    ITEM_SETTING_MODE = 0,
    ITEM_SETTING_NAME,
    ITEM_SETTING_TOTAL,
};
typedef struct{
    int mode;
    char name[SUPERCAST_SERVER_NAME_LEN];
}SysSupercastSetting;

static SystemSettingOpt  s_supercast_setting_opt;
static SystemSettingItem s_supercast_item[ITEM_SETTING_TOTAL];
static SysSupercastSetting s_supercast_setting;
extern void app_supercast_setting_menu_exec(void);

static char *supercast_key_img[SUPERCAST_ITEM_NUM]=
{
    "supercast_img_key1",
    "supercast_img_key2",
};

static unsigned int supercast_pic_download_handle[SUPERCAST_ITEM_NUM];
static int supercast_pic_download_abort = 0;
static event_list* suoercas_picupdate_timer = NULL;


static void _supercast_send_ui_msg(NetappsDownState state,uint32_t code, const char* filepath,uint32_t handle)
{
    GxMessage   *new_msg = NULL;
    AppMsgNetappsDownReport* param = NULL;
    new_msg = GxBus_MessageNew(APPMSG_NETVIDEO_DOWN_STATE_REPORT);
    param = (AppMsgNetappsDownReport*)GxBus_GetMsgPropertyPtr(new_msg, AppMsgNetappsDownReport);
    if(param != NULL)
    {
        param->state = state;
        param->errorcode = code;
        param->filepath = (char *)filepath;
        param->handle = handle;
        GxBus_MessageSend(new_msg);
    }
    return;
}

static int _supercast_update_timer_stop(void* data)
{
    APP_TIMER_REMOVE(suoercas_picupdate_timer);
    app_pop_reset_timeout(3);
    app_pop_set_string(STR_ID_TIMEOUT);
    app_update_pop_dlg(WND_POP_BOOK);
    return 0;
}

static void _supercast_download_pic_cb(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
    unsigned int handle = 0;
    int i = 0;

    if(message == GXCURL_STATE_COMPLETE)
    {
        if(supercast_pic_download_abort)
            return;
        handle = callback_data->handle;
        if(handle>0)
        {
            for(i=0;i<SUPERCAST_ITEM_NUM;i++)
            {
                if(supercast_pic_download_handle[i] == handle)
                {
                    if(callback_data->complete_data.retcode == GXAPPS_SUCCESS && callback_data->complete_data.size > 0)
                    {
                        if(i == 0)
                            _supercast_send_ui_msg(NETAPPS_DOWN_STATE_SUCCESS,0,APK_PIC_PATH,handle);
                        else
                            _supercast_send_ui_msg(NETAPPS_DOWN_STATE_SUCCESS,0,SERVER_PIC_PATH,handle);
                    }
                    else
                    {
                        _supercast_send_ui_msg(NETAPPS_DOWN_STATE_FAILED,0,NULL,handle);
                    }
                    break;
                }
            }
        }
    }
}

static void _supercast_pic_download_stop(void)
{
    int i = 0;
    supercast_pic_download_abort = 1;
    for(i=0; i<SUPERCAST_ITEM_NUM; i++)
    {
        if(supercast_pic_download_handle[i]!=0)
        {
            gxapps_curl_async_abort(supercast_pic_download_handle[i]);
            supercast_pic_download_handle[i] = 0;
        }
    }
    GxCore_FileDelete(APK_PIC_PATH);
    GxCore_FileDelete(SERVER_PIC_PATH);
    supercast_pic_download_abort = 0;
}

static void _supercast_pic_download_start(void)
{
    GxCore_FileDelete(APK_PIC_PATH);
    GxCore_FileDelete(SERVER_PIC_PATH);
    _supercast_pic_download_stop();
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec= 12;
    pop.str = STR_ID_UPDATING;
    popdlg_create(&pop);
    APP_TIMER_ADD(suoercas_picupdate_timer, _supercast_update_timer_stop, 10000, TIMER_REPEAT);
    supercast_pic_download_handle[0] = gxapps_curl_async_download(APPK_URL, APK_PIC_PATH, _supercast_download_pic_cb);
    supercast_pic_download_handle[1] = gxapps_curl_async_download(VIDEO_URL, SERVER_PIC_PATH, _supercast_download_pic_cb);
    return;
}

int app_supercast_start(char *dev_name)
{
    if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
    {
        app_webapi_start(dev_name);
        return 0;
    }
    else
    {
         PopDlg pop;
         memset(&pop, 0, sizeof(PopDlg));
         pop.type = POP_TYPE_OK;
         pop.format = POP_FORMAT_DLG;
         pop.mode = POP_MODE_UNBLOCK;
         pop.pos.x = APP_POP_DLG_POS_X;
         pop.pos.y = APP_POP_DLG_POS_Y;
         pop.str = STR_ID_CONNECT_TIP;
         popdlg_create(&pop);
    }
    return -1;
}

void app_supercast_stop(void)
{
    app_webapi_stop();
}

SIGNAL_HANDLER int app_supercast_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_supercast_destroy(GuiWidget *widget, void *usrdata)
{
    APP_TIMER_REMOVE(suoercas_picupdate_timer);
    _supercast_pic_download_stop();
    GxCore_FileDelete(APK_PIC_PATH);
    GxCore_FileDelete(SERVER_PIC_PATH);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_supercast_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    int key = 0;
    GUI_Event *event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_KEYDOWN:
            key = find_virtualkey_ex(event->key.scancode,event->key.sym);
            switch(key)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_RED:
                    {
                        app_supercast_setting_menu_exec();
                    }
                    break;
                case STBK_GREEN:
                    {
                        _supercast_pic_download_start();
                    }
                    break;
                case STBK_MENU:
                case STBK_EXIT:
                    GUI_EndDialog(WND_SUPERCAST);
                    break;
                default:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
            }
            break;
        default:
            ret = EVENT_TRANSFER_KEEPON;
            break;
    }
    return ret;
}

static void app_supercast_support_reflash(void)
{
    int config = 0;
    GxBus_ConfigGetInt(WEBAPI_SWITCH, &config, WEBAPI_SWITCH_ON);
    if(config == 0) //ON
    {
        GUI_SetProperty(MOED_INFO, "string", SUPPORT_ON);
    }else{
        GUI_SetProperty(MOED_INFO, "string", SUPPORT_OFF);
    }
}

void app_create_supercast_menu_exec(void)
{
    GUI_CreateDialog(WND_SUPERCAST);
    _supercast_pic_download_start();
    app_supercast_support_reflash();
}

int app_supercast_update_msg_proc(GxMessage *msg)
{
    AppMsgNetappsDownReport* msg_state = GxBus_GetMsgPropertyPtr(msg, AppMsgNetappsDownReport);
    switch(msg_state->state)
    {
        case NETAPPS_DOWN_STATE_FAILED:
            {
                APP_TIMER_REMOVE(suoercas_picupdate_timer);
                _supercast_pic_download_stop();
                GUI_SetProperty("img_download_app", "load_scal_img", NULL);
                GUI_SetProperty("img_download_app", "load_scal_img", NULL);
                GUI_SetProperty("img_video_introduction", "state", "hide");
                GUI_SetProperty("img_video_introduction", "state", "hide");
                if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
                {
                    app_update_pop_dlg(WND_POP_BOOK);
                    break;
                }
                PopDlg pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.timeout_sec= 3000;
                pop.str = STR_ID_SERVER_FAIL;
                popdlg_create(&pop);
            }
            break;
        case NETAPPS_DOWN_STATE_SUCCESS:
            {
                popdlg_destroy();
                APP_TIMER_REMOVE(suoercas_picupdate_timer);
                if(msg_state->handle == supercast_pic_download_handle[0])
                {
                    gal_add_key_path(supercast_key_img[0], msg_state->filepath);
                    GUI_SetProperty("img_download_app", "img", supercast_key_img[0]);
                    GUI_SetProperty("img_download_app", "state", "show");
                }
                else
                {
                    gal_add_key_path(supercast_key_img[1], msg_state->filepath);
                    GUI_SetProperty("img_video_introduction", "img", supercast_key_img[1]);
                    GUI_SetProperty("img_video_introduction", "state", "show");
                }
                app_update_pop_dlg(WND_POP_BOOK);
            }
            break;
        default:
            break;
    }
     return 0;

}

static void app_supcast_para_get(void)
{
    memset(&s_supercast_setting, 0, sizeof(s_supercast_setting));
    GxBus_ConfigGetInt(WEBAPI_SWITCH, &s_supercast_setting.mode, WEBAPI_SWITCH_ON);
    GxBus_ConfigGet(SUPERCAST_SERVER_NAME, s_supercast_setting.name, SUPERCAST_SERVER_NAME_LEN, SUPERCAST_SERVER_NAME_DEFAULT);
}

static void app_supercast_setting_result_para_get(SysSupercastSetting *ret_para)
{
    char *name = NULL;
    ret_para->mode = s_supercast_item[ITEM_SETTING_MODE].itemProperty.itemPropertyCmb.sel;

    GUI_GetProperty(app_system_get_button(ITEM_SETTING_NAME), "string", &name);
    sprintf(ret_para->name, "%s", name);
}


static int app_supercast_setting_para_save(SysSupercastSetting *ret_para)
{
    int ret = 0;
    if(ret_para->mode == 0) // ON
    {
        if(s_supercast_setting.mode == 0)
        {
            app_supercast_stop();
            GxCore_ThreadDelay(1000);
        }
        ret = app_supercast_start(ret_para->name);
    }else{
        app_supercast_stop();
    }
    if(ret == 0)
        GxBus_ConfigSetInt(WEBAPI_SWITCH, ret_para->mode);
    GxBus_ConfigSet(SUPERCAST_SERVER_NAME, ret_para->name);
    return 0;
}

static bool app_supercast_setting_para_change(SysSupercastSetting *ret_para)
{
   bool ret = FALSE;

   if(s_supercast_setting.mode != ret_para->mode)
   {
      ret = TRUE;
   }

   if(0 != strcmp(s_supercast_setting.name, ret_para->name))
   {
      ret = TRUE;
   }

   return ret;
}

static int _supercast_setting_save_pop_cb(PopDlgRet ret)
{
    SysSupercastSetting ret_para = {};
    if(POP_VAL_OK == ret)
    {
        app_supercast_setting_result_para_get(&ret_para);
        app_supercast_setting_para_save(&ret_para);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    app_supercast_support_reflash();
    return 0;
}

static bool _supercast_setting_check_cb(void)
{
    bool ret = false;
    SysSupercastSetting para_ret = {};

    app_supercast_setting_result_para_get(&para_ret);
    ret = app_supercast_setting_para_change(&para_ret);

    return ret;
}

static int app_supercast_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_NORMAL == exit_type)
    {
        if(false == app_system_set_callback_handler(_supercast_setting_check_cb, _supercast_setting_save_pop_cb))
        {
        }
        ret = EXIT_RET_POP;
    }

    return ret;
}

static void _supercast_setting_rename_release_cb(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    GUI_SetProperty(app_system_get_button(ITEM_SETTING_NAME), "string", data->out_name);
}

static int app_supercast_name_press_ok_cb(unsigned short key)
{
    char *in_name = NULL;
    if (key == STBK_OK) {
        static PopKeyboard keyboard;
        memset(&keyboard, 0, sizeof(PopKeyboard));
        GUI_GetProperty(app_system_get_button(ITEM_SETTING_NAME), "string", &in_name);
        keyboard.in_name    = in_name;
        keyboard.max_num = SUPERCAST_SERVER_NAME_LEN - 1;
        keyboard.out_name   = NULL;
        keyboard.change_cb  = NULL;
        keyboard.release_cb = _supercast_setting_rename_release_cb;
        keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;
        multi_language_keyboard_create(&keyboard);
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void app_supercast_mode_item_init(void)
{
    s_supercast_item[ITEM_SETTING_MODE].itemTitle = "SuperCast Mode";
    s_supercast_item[ITEM_SETTING_MODE].itemType = ITEM_CHOICE;
    s_supercast_item[ITEM_SETTING_MODE].itemProperty.itemPropertyCmb.content= "[On,Off]";
    s_supercast_item[ITEM_SETTING_MODE].itemProperty.itemPropertyCmb.sel = s_supercast_setting.mode;
    s_supercast_item[ITEM_SETTING_MODE].itemCallback.cmbCallback.CmbChange= NULL;
    s_supercast_item[ITEM_SETTING_MODE].itemStatus = ITEM_NORMAL;
}

static void app_supercast_name_item_init(void)
{
    s_supercast_item[ITEM_SETTING_NAME].itemTitle = "Box Name";
    s_supercast_item[ITEM_SETTING_NAME].itemType = ITEM_PUSH;
    s_supercast_item[ITEM_SETTING_NAME].itemProperty.itemPropertyBtn.string = s_supercast_setting.name;
    s_supercast_item[ITEM_SETTING_NAME].itemCallback.btnCallback.BtnPress = app_supercast_name_press_ok_cb;
    s_supercast_item[ITEM_SETTING_NAME].itemStatus = ITEM_NORMAL;
}

void app_supercast_setting_menu_exec(void)
{
    memset(&s_supercast_setting_opt, 0 ,sizeof(s_supercast_setting_opt));
    app_supcast_para_get();

    s_supercast_setting_opt.menuTitle = "SuperCast Setting";
    s_supercast_setting_opt.itemNum = ITEM_SETTING_TOTAL;
    s_supercast_setting_opt.timeDisplay = TIP_HIDE;
    s_supercast_setting_opt.item = s_supercast_item;
    s_supercast_setting_opt.exit = app_supercast_setting_exit_callback;

    app_supercast_mode_item_init();
    app_supercast_name_item_init();
    app_system_set_create(&s_supercast_setting_opt);
}
#endif
