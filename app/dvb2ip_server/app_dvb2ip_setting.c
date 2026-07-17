#include "app_dvb2ip_platform.h"
#if DVB2IP_SERVER_SUPPORT
#include "app.h"
#include "app_wnd_system_setting_opt.h"

typedef enum {
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    ITEM_DVB2IP_MODE,
    ITEM_DVB2IP_SWITCH,
#endif
    ITEM_DVB2IP_PRIO,
    ITEM_DVB2IP_FOLLOW,
    ITEM_DVB2IP_SAVE,
    ITEM_DVB2IP_TOTAL
} ITEM_DVB2IP;

static char *s_dvb2ip_item_title[ITEM_DVB2IP_TOTAL] = {
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    STR_ID_DVB2IP_MODE,
    STR_ID_SAT2IP_CONFLICT_OPERAT,
#endif
    STR_ID_DVB2IP_PRIORITY,
    STR_ID_CLIENT_CONTROL,
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    STR_ID_SAVE
#else
    STR_ID_DVB2IP_START
#endif
};

static char *s_dvb2ip_item_content[ITEM_DVB2IP_TOTAL] = {
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    "[Off,On]",
    "[Disable,Enable]",
#endif
    "[Low,High]",
    "[Independent,Following,Master]",
    "Press OK"
};

static SystemSettingOpt  s_Dvb2ipOpt;
static SystemSettingItem s_Dvb2ipItem[ITEM_DVB2IP_TOTAL];
static int s_dvb2ip_item_flag[ITEM_DVB2IP_TOTAL-1];

static bool _dvb2ip_check_cb(void)
{
    int i = 0;
    int flag[ITEM_DVB2IP_TOTAL-1] = {0};
    bool change_flag = false;


    for (i = 0; i < ITEM_DVB2IP_TOTAL-1; i++)
    {
        flag[i] = s_Dvb2ipItem[i].itemProperty.itemPropertyCmb.sel;
    }

    for (i = 0; i < ITEM_DVB2IP_TOTAL-1; i++)
    {
        if (s_dvb2ip_item_flag[i] != flag[i])
        {
            change_flag = true;
            break;
        }
    }

    return change_flag;
}

static int _dvb2ip_save_pop_public(void)
{
    int i = 0;
    int flag[ITEM_DVB2IP_TOTAL-1] = {0};


    for (i = 0; i < ITEM_DVB2IP_TOTAL-1; i++)
    {
        flag[i] = s_Dvb2ipItem[i].itemProperty.itemPropertyCmb.sel;
    }
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    PopDlg pop = {0};
    popdlg_destroy();
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.str = STR_ID_SAVING;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.exit_cb = NULL;
    popdlg_create(&pop);

    for (i = ITEM_DVB2IP_TOTAL-2; i >= 0; i--)
    {
        s_dvb2ip_item_flag[i] = flag[i];
        switch(i)
        {
            case ITEM_DVB2IP_MODE:
                 app_dvb2ip_use_flag_set(flag[i]);
                 break;
            case ITEM_DVB2IP_SWITCH:
                 app_dvb2ip_switch_flag_set(flag[i]);
                 break;
            case ITEM_DVB2IP_PRIO:
                 app_dvb2ip_prio_flag_set(flag[i]);
                 break;
            case ITEM_DVB2IP_FOLLOW:
                 app_dvb2ip_follow_flag_set(flag[i]);
                 break;
            default:
                    break;
        }
    }

#else
   for (i = 0; i < ITEM_DVB2IP_TOTAL-1; i++)
        app_system_set_item_data_update(i);
   for (i = 0; i < ITEM_DVB2IP_TOTAL-1; i++)
   {
       if (s_dvb2ip_item_flag[i] != flag[i])
       {
           s_dvb2ip_item_flag[i] = flag[i];
           switch(i)
           {
                case ITEM_DVB2IP_PRIO:
                     app_dvb2ip_prio_flag_set(flag[i]);
                     break;
                case ITEM_DVB2IP_FOLLOW:
                     app_dvb2ip_follow_flag_set(flag[i]);
                     break;
                default:
                     break;
           }
       }
   }

#endif

    return 0;
}

#if !DVB2IP_SERVER_USE_STATIC_SCREEN
static int app_dvb2ip_set_item_data_update(void)
{
    int i = 0;

    for (i = 0; i < ITEM_DVB2IP_TOTAL-1; i++)
    {
        app_system_set_item_data_update(i);
    }

    return 0;
}

static int app_dvb2ip_ok_item_press_cb(unsigned short key)
{
    if(key == STBK_OK)
    {
        bool change_flag = false;

        app_dvb2ip_set_item_data_update();
        change_flag = _dvb2ip_check_cb();
        if (!change_flag)
            return EVENT_TRANSFER_STOP;

        _dvb2ip_save_pop_public();

        return EVENT_TRANSFER_STOP;
    }

    return EVENT_TRANSFER_KEEPON;
}

#else
static int app_dvb2ip_ok_item_press_cb(unsigned short key)
{
    if(key == STBK_OK)
    {
        _dvb2ip_save_pop_public();
        app_dvb2ip_play_entry();
        app_system_set_destroy(EXIT_NORMAL);
    }

    return EVENT_TRANSFER_KEEPON;
}
#endif

static void app_dvb2ip_choice_item_init(ITEM_DVB2IP sel)
{
    s_Dvb2ipItem[sel].itemTitle = s_dvb2ip_item_title[sel];
    s_Dvb2ipItem[sel].itemType = ITEM_CHOICE;
    s_Dvb2ipItem[sel].itemProperty.itemPropertyCmb.content= s_dvb2ip_item_content[sel];
    s_Dvb2ipItem[sel].itemProperty.itemPropertyCmb.sel = s_dvb2ip_item_flag[sel];
    s_Dvb2ipItem[sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_Dvb2ipItem[sel].itemStatus = ITEM_NORMAL;
}

static void app_dvb2ip_save_item_init(ITEM_DVB2IP sel)
{
    s_Dvb2ipItem[sel].itemTitle = s_dvb2ip_item_title[sel];
    s_Dvb2ipItem[sel].itemType = ITEM_PUSH;
    s_Dvb2ipItem[sel].itemProperty.itemPropertyBtn.string= s_dvb2ip_item_content[sel];
    s_Dvb2ipItem[sel].itemCallback.btnCallback.BtnPress = app_dvb2ip_ok_item_press_cb;
    s_Dvb2ipItem[sel].itemStatus = ITEM_NORMAL;
}


static bool app_dvb2ip_check_cb(void)
{
    return _dvb2ip_check_cb();
}

static int app_dvb2ip_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
		_dvb2ip_save_pop_public();
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static int app_dvb2ip_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(false == app_system_set_callback_handler(app_dvb2ip_check_cb, app_dvb2ip_save_pop_cb))
    {
        	//do nothing
    }
    ret = EXIT_RET_POP;

    return ret;
}

void app_dvb2ip_setting_menu_exec(void)
{
    int i = 0;
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    s_dvb2ip_item_flag[ITEM_DVB2IP_MODE] = app_dvb2ip_use_flag_get();
    s_dvb2ip_item_flag[ITEM_DVB2IP_SWITCH] = app_dvb2ip_switch_flag_get();
#endif
    s_dvb2ip_item_flag[ITEM_DVB2IP_PRIO] = app_dvb2ip_prio_flag_get();
    s_dvb2ip_item_flag[ITEM_DVB2IP_FOLLOW] = app_dvb2ip_follow_flag_get();

    memset(&s_Dvb2ipOpt, 0 ,sizeof(SystemSettingOpt));
    s_Dvb2ipOpt.menuTitle = STR_ID_DVB2IP_SETTING;
    //s_Dvb2ipOpt.titleImage = "s_title_left_utility";
    s_Dvb2ipOpt.itemNum = ITEM_DVB2IP_TOTAL;
    s_Dvb2ipOpt.item = s_Dvb2ipItem;
    s_Dvb2ipOpt.timeDisplay = TIP_HIDE;

    for (i = 0; i < ITEM_DVB2IP_TOTAL-1; i++)
        app_dvb2ip_choice_item_init(i);
    app_dvb2ip_save_item_init(ITEM_DVB2IP_SAVE);
    s_Dvb2ipOpt.exit = app_dvb2ip_exit_callback;

    app_system_set_create(&s_Dvb2ipOpt);
}
#endif

