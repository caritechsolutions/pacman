#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "module/channel_edit.h"

#if LONG_PASSWORD_SUPPORT
#define MAX_PASSWD_LEN 6
#else
#define MAX_PASSWD_LEN 4
#endif

enum LOCK_ITEM_NAME
{
    ITEM_MENU_LOCK = 0,
    ITEM_CHANNEL_LOCK,
    ITEM_PASSWD_BUTTON,
    ITEM_OLD_PASSWD,
    ITEM_NEW_PASSWD,
    ITEM_CONFIRM_PASSWD,
    ITEM_LOCK_TOTAL
};

typedef enum
{
    MENU_LOCK_OFF,
    MENU_LOCK_ON
}MenuLock;

typedef enum
{
    CHANNEL_LOCK_OFF,
    CHANNEL_LOCK_ON
}ChannelLock;

typedef struct
{
    MenuLock menu_lock;
    ChannelLock channel_lock;
    char passwd[MAX_PASSWD_LEN+1];
}SysLockPara;

static SystemSettingOpt s_lock_setting_opt;
static SystemSettingItem s_lock_item[ITEM_LOCK_TOTAL];
static SysLockPara s_sys_lock_para;
static char *s_old_pwd = NULL;
static char *s_new_pwd = NULL;
static char *s_confirm_pwd = NULL;

static void app_lock_system_para_get(void)
{
    int init_value = 0;

    GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
    s_sys_lock_para.menu_lock= init_value;

    GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &init_value, CHANNEL_LOCK);
    s_sys_lock_para.channel_lock= init_value;

    memset(s_sys_lock_para.passwd, '\0', sizeof(s_sys_lock_para.passwd));
}

static void app_lock_result_para_get(SysLockPara *ret_para)
{
    ret_para->menu_lock= s_lock_item[ITEM_MENU_LOCK].itemProperty.itemPropertyCmb.sel;
    ret_para->channel_lock= s_lock_item[ITEM_CHANNEL_LOCK].itemProperty.itemPropertyCmb.sel;
}

static bool app_lock_setting_para_change(SysLockPara *ret_para)
{
    bool ret = FALSE;

    if(s_sys_lock_para.menu_lock!= ret_para->menu_lock)
    {
        ret = TRUE;
    }

    if(s_sys_lock_para.channel_lock!= ret_para->channel_lock)
    {
        ret = TRUE;
    }

    return ret;
}

static int app_lock_setting_para_save(SysLockPara *ret_para)
{
    int init_value = 0;

    init_value = ret_para->menu_lock;
    GxBus_ConfigSetInt(MENU_LOCK_KEY,init_value);

    init_value = ret_para->channel_lock;
    GxBus_ConfigSetInt(CHANNEL_LOCK_KEY,init_value);

    return 0;
}

static void app_lock_exit_passwd_edit(int cur_sel)
{
    app_system_set_item_state_update(ITEM_MENU_LOCK, ITEM_NORMAL);
    app_system_set_item_state_update(ITEM_CHANNEL_LOCK, ITEM_NORMAL);
    app_system_set_item_state_update(ITEM_PASSWD_BUTTON, ITEM_NORMAL);
    app_system_set_item_state_update(ITEM_OLD_PASSWD, ITEM_HIDE);
    app_system_set_item_state_update(ITEM_NEW_PASSWD, ITEM_HIDE);
    app_system_set_item_state_update(ITEM_CONFIRM_PASSWD, ITEM_HIDE);

    app_system_set_unfocus_item(cur_sel);
    app_system_set_focus_item(ITEM_PASSWD_BUTTON);
}

static int _lock_setting_save_pop_cb(PopDlgRet ret)
{
    SysLockPara para_ret;

    if(POP_VAL_OK == ret)
    {
        app_lock_result_para_get(&para_ret);
        app_lock_setting_para_save(&para_ret);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static bool _lock_setting_check_cb(void)
{
    bool ret = false;
    SysLockPara para_ret;

    app_lock_result_para_get(&para_ret);
    ret = app_lock_setting_para_change(&para_ret);

    return ret;
}

static int app_lock_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(exit_type == EXIT_ABANDON)//error:#40662
    {
        ;
    }
    else
    {
        app_system_set_callback_handler(_lock_setting_check_cb, _lock_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }

    return ret;
}

static int app_lock_passwd_button_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;
    int init_value = 0;

    switch(key)
    {
        case STBK_OK:

            app_system_set_item_state_update(ITEM_MENU_LOCK, ITEM_DISABLE);
            app_system_set_item_state_update(ITEM_CHANNEL_LOCK, ITEM_DISABLE);
            app_system_set_item_state_update(ITEM_PASSWD_BUTTON, ITEM_DISABLE);

            app_system_set_item_property(ITEM_OLD_PASSWD, &s_lock_item[ITEM_OLD_PASSWD].itemProperty);
            app_system_set_item_state_update(ITEM_OLD_PASSWD, ITEM_NORMAL);

            app_system_set_item_property(ITEM_NEW_PASSWD, &s_lock_item[ITEM_NEW_PASSWD].itemProperty);
            app_system_set_item_state_update(ITEM_NEW_PASSWD, ITEM_NORMAL);

            app_system_set_item_property(ITEM_CONFIRM_PASSWD, &s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty);
            app_system_set_item_state_update(ITEM_CONFIRM_PASSWD, ITEM_NORMAL);

            app_system_set_unfocus_item(ITEM_PASSWD_BUTTON);
            app_system_set_focus_item(ITEM_OLD_PASSWD);

            GxBus_ConfigGetInt(PASSWORD_KEY0, &init_value, PASSWORD_1);
            s_sys_lock_para.passwd[0] = init_value;
            GxBus_ConfigGetInt(PASSWORD_KEY1, &init_value, PASSWORD_2);
            s_sys_lock_para.passwd[1] = init_value;
            GxBus_ConfigGetInt(PASSWORD_KEY2, &init_value, PASSWORD_3);
            s_sys_lock_para.passwd[2] = init_value;
            GxBus_ConfigGetInt(PASSWORD_KEY3, &init_value, PASSWORD_4);
            s_sys_lock_para.passwd[3] = init_value;
#if LONG_PASSWORD_SUPPORT
            GxBus_ConfigGetInt(PASSWORD_KEY2, &init_value, PASSWORD_5);
            s_sys_lock_para.passwd[4] = init_value;
            GxBus_ConfigGetInt(PASSWORD_KEY3, &init_value, PASSWORD_6);
            s_sys_lock_para.passwd[5] = init_value;
#endif

            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int app_lock_old_passwd_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;

    switch(key)
    {
        case STBK_EXIT:
            app_lock_exit_passwd_edit(ITEM_OLD_PASSWD);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_UP:
        case STBK_DOWN:
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int app_lock_new_passwd_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;

    switch(key)
    {
        case STBK_EXIT:
            app_lock_exit_passwd_edit(ITEM_NEW_PASSWD);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_UP:
        case STBK_DOWN:
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int app_lock_confirm_passwd_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;

    switch(key)
    {
        case STBK_EXIT:
            app_lock_exit_passwd_edit(ITEM_CONFIRM_PASSWD);
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_UP:
        case STBK_DOWN:
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

static int _old_passwd_error_pop_cb(PopDlgRet ret)
{
    app_system_set_item_property(ITEM_OLD_PASSWD, &s_lock_item[ITEM_OLD_PASSWD].itemProperty);
    app_system_set_focus_item(ITEM_OLD_PASSWD);
    return 0;
}

static int app_lock_old_passwd_reachend_callback(char *string)
{
    char general[8] = {0};

    if(GXCORE_ERROR == app_oem_get(OEM_PASSWORD, general, 8))
    {
        memset(general, 0x0, 8);
        sprintf(general, "%s", GRNERAL_PASSWD);
    }

   if(string != NULL)
    {
        s_old_pwd = string;

        if (strlen(string) == MAX_PASSWD_LEN)
        {
            if((strcmp(s_old_pwd, s_sys_lock_para.passwd) == 0)
                || (strcmp(s_old_pwd, GRNERAL_PASSWD) == 0))
            {
                app_system_set_unfocus_item(ITEM_OLD_PASSWD);
                app_system_set_focus_item(ITEM_NEW_PASSWD);
            }
            else
            {
                PopDlg  pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_OK;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_ERR_PASSWD;
                pop.pos.x = APP_POP_DLG_POS_X;
                pop.pos.y = APP_POP_DLG_POS_Y;
                pop.exit_cb = _old_passwd_error_pop_cb;
                popdlg_create(&pop);

                if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
                {
                    GUI_SetFocusWidget(WND_POP_BOOK);
                }

            }
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

static int app_lock_new_passwd_reachend_callback(char *string)
{
    if((string != NULL) && (strlen(string) == MAX_PASSWD_LEN))
    {
        s_new_pwd = string;

        app_system_set_unfocus_item(ITEM_NEW_PASSWD);
        app_system_set_focus_item(ITEM_CONFIRM_PASSWD);
    }

    return EVENT_TRANSFER_KEEPON;
}

static int _invalid_passwd_pop_cb(PopDlgRet ret)
{
    GUI_SetInterface("flush", NULL);
    app_system_set_item_property(ITEM_NEW_PASSWD, &s_lock_item[ITEM_NEW_PASSWD].itemProperty);
    app_system_set_item_property(ITEM_CONFIRM_PASSWD, &s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty);
    app_system_set_unfocus_item(ITEM_CONFIRM_PASSWD);
    app_system_set_focus_item(ITEM_NEW_PASSWD);

    return 0;
}

static int _success_modify_passwd_pop_cb(PopDlgRet ret)
{
    app_lock_exit_passwd_edit(ITEM_CONFIRM_PASSWD);

    return 0;
}

static int _common_passwd_pop_cb(PopDlgRet ret)
{
    GUI_SetInterface("flush", NULL);
    app_system_set_item_property(ITEM_NEW_PASSWD, &s_lock_item[ITEM_NEW_PASSWD].itemProperty);
    app_system_set_item_property(ITEM_CONFIRM_PASSWD, &s_lock_item[ITEM_CONFIRM_PASSWD].itemProperty);
    app_system_set_unfocus_item(ITEM_CONFIRM_PASSWD);
    app_system_set_focus_item(ITEM_NEW_PASSWD);

    return 0;
}

static int app_lock_confirm_passwd_reachend_callback(char *string)
{
    PopDlg  pop;

    memset(&pop, 0, sizeof(PopDlg));

    if((string != NULL) && (s_new_pwd != NULL))
    {
        s_confirm_pwd = string;
        if(strlen(string) != MAX_PASSWD_LEN)
        {
            return EVENT_TRANSFER_KEEPON;
        }
        //check pwd
        if(strcmp(s_new_pwd, s_confirm_pwd) == 0)
        {
            if(strcmp(s_old_pwd,s_new_pwd) == 0)
            {
                pop.type = POP_TYPE_OK;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_RE_ENTER_PASSWD;
                pop.pos.x = APP_POP_DLG_POS_X;
                pop.pos.y = APP_POP_DLG_POS_Y;
                pop.exit_cb = _common_passwd_pop_cb;
                popdlg_create(&pop);

                if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
                {
                    GUI_SetFocusWidget(WND_POP_BOOK);
                }

            }
            else
            {
                GxBus_ConfigSetInt(PASSWORD_VERIFY, PASSWD_VERIFY_ERR);
                GxBus_ConfigSetInt(PASSWORD_KEY0, s_confirm_pwd[0]);
                GxBus_ConfigSetInt(PASSWORD_KEY1, s_confirm_pwd[1]);
                GxBus_ConfigSetInt(PASSWORD_KEY2, s_confirm_pwd[2]);
                GxBus_ConfigSetInt(PASSWORD_KEY3, s_confirm_pwd[3]);
#if LONG_PASSWORD_SUPPORT
                GxBus_ConfigSetInt(PASSWORD_KEY4, s_confirm_pwd[4]);
                GxBus_ConfigSetInt(PASSWORD_KEY5, s_confirm_pwd[5]);
#endif
                GxBus_ConfigSetInt(PASSWORD_VERIFY, PASSWD_VERIFY_OK);

                pop.type = POP_TYPE_NO_BTN;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_SUCCESS;
                pop.timeout_sec = 1;
                pop.pos.x = APP_POP_DLG_POS_X;
                pop.pos.y = APP_POP_DLG_POS_Y;
                pop.exit_cb = _success_modify_passwd_pop_cb;
                popdlg_create(&pop);

            }
        }
        else
        {
            pop.type = POP_TYPE_OK;
            pop.mode = POP_MODE_UNBLOCK;
            pop.str = STR_ID_ERR_PASSWD;
            pop.pos.x = APP_POP_DLG_POS_X;
            pop.pos.y = APP_POP_DLG_POS_Y;
            pop.exit_cb = _invalid_passwd_pop_cb;
            popdlg_create(&pop);

            if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
            {
                GUI_SetFocusWidget(WND_POP_BOOK);
            }
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

static void _lock_choice_item_init(enum LOCK_ITEM_NAME sel, char *title, char *content, int cmb_sel,
        int (*CmbChange)(int), int (*CmbPress)(int))
{

    s_lock_item[sel].itemTitle = title;
    s_lock_item[sel].itemType = ITEM_CHOICE;
    s_lock_item[sel].itemProperty.itemPropertyCmb.content= content;
    s_lock_item[sel].itemProperty.itemPropertyCmb.sel = cmb_sel;
    s_lock_item[sel].itemCallback.cmbCallback.CmbChange= CmbChange;
    s_lock_item[sel].itemCallback.cmbCallback.CmbPress= CmbPress;
    s_lock_item[sel].itemStatus = ITEM_NORMAL;
}

static void app_lock_setting_menu_item_init(void)
{
    _lock_choice_item_init(ITEM_MENU_LOCK, STR_ID_MENU_LOCK, "[Off,On]", s_sys_lock_para.menu_lock, NULL, NULL);
}

static void app_lock_setting_channel_item_init(void)
{
    /*only play program need input password*/
    _lock_choice_item_init(ITEM_CHANNEL_LOCK, STR_ID_CH_LOCK, "[Off,On]", s_sys_lock_para.channel_lock, NULL, NULL);
}

static void app_lock_setting_passwd_button_item_init(void)
{
    s_lock_item[ITEM_PASSWD_BUTTON].itemTitle = STR_ID_CHANGE_PASSWD;
    s_lock_item[ITEM_PASSWD_BUTTON].itemType = ITEM_PUSH;
    s_lock_item[ITEM_PASSWD_BUTTON].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_lock_item[ITEM_PASSWD_BUTTON].itemCallback.btnCallback.BtnPress= app_lock_passwd_button_press_callback;
    s_lock_item[ITEM_PASSWD_BUTTON].itemStatus = ITEM_NORMAL;
}

static void _lock_passwd_item_init(enum LOCK_ITEM_NAME sel, char *title,
        int (*EditReachEnd)(char *), int (*EditPress)(unsigned short))
{
    int pwd_len = MAX_PASSWD_LEN;
    static char edit_len[2] = {0};
    sprintf(edit_len , "%d", pwd_len);

    s_lock_item[sel].itemTitle = title;
    s_lock_item[sel].itemType = ITEM_EDIT;
    s_lock_item[sel].itemProperty.itemPropertyEdit.string= NULL;
    s_lock_item[sel].itemProperty.itemPropertyEdit.maxlen = edit_len;
    s_lock_item[sel].itemProperty.itemPropertyEdit.format = "edit_digit";
    s_lock_item[sel].itemProperty.itemPropertyEdit.intaglio = "*";
    s_lock_item[sel].itemProperty.itemPropertyEdit.default_intaglio = "-";
    s_lock_item[sel].itemCallback.editCallback.EditReachEnd= EditReachEnd;
    s_lock_item[sel].itemCallback.editCallback.EditPress= EditPress;
    s_lock_item[sel].itemStatus = ITEM_HIDE;
}

static void app_lock_setting_old_passwd_item_init(void)
{
    _lock_passwd_item_init(ITEM_OLD_PASSWD, STR_ID_OLD_PASSWD,
            app_lock_old_passwd_reachend_callback, app_lock_old_passwd_press_callback);
}

static void app_lock_setting_new_passwd_item_init(void)
{
    _lock_passwd_item_init(ITEM_NEW_PASSWD, STR_ID_NEW_PASSWD,
            app_lock_new_passwd_reachend_callback, app_lock_new_passwd_press_callback);
}

static void app_lock_setting_confirm_passwd_item_init(void)
{
    _lock_passwd_item_init(ITEM_CONFIRM_PASSWD, STR_ID_CON_PASSWD,
            app_lock_confirm_passwd_reachend_callback, app_lock_confirm_passwd_press_callback);
}

static void _lock_setting_menu_exec(bool tip_hide)
{
    memset(&s_lock_setting_opt, 0 ,sizeof(s_lock_setting_opt));
    app_lock_system_para_get();

    s_lock_setting_opt.menuTitle = STR_ID_LOCK_CTRL;
    s_lock_setting_opt.itemNum = ITEM_LOCK_TOTAL;
    if(tip_hide)
        s_lock_setting_opt.topTipImgDisplay = TIP_HIDE;
    s_lock_setting_opt.timeDisplay = TIP_HIDE;
    s_lock_setting_opt.item = s_lock_item;

    app_lock_setting_menu_item_init();
    app_lock_setting_channel_item_init();
    app_lock_setting_passwd_button_item_init();
    app_lock_setting_old_passwd_item_init();
    app_lock_setting_new_passwd_item_init();
    app_lock_setting_confirm_passwd_item_init();

    s_lock_setting_opt.exit = app_lock_setting_exit_callback;

    app_system_set_create(&s_lock_setting_opt);
}

void app_lock_setting_menu_exec(void)
{
    _lock_setting_menu_exec(false);
}

void app_dvbt_lock_setting_menu_exec(void)
{
    _lock_setting_menu_exec(true);
}
