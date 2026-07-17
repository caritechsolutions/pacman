#include "app.h"
#if (DEMOD_DVB_T || DEMOD_ISDBT) && 0 == DEMOD_DVB_S
#include "full_screen.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_t2_area_config.h"
#include "app_dvbt2_search.h"
#include "app_wnd_system_setting_opt.h"
#include "app_module.h"
#include "app_rf_channels.h"

#define GUIDE_LIST_VIEW         "list_installation_guide"
#define GUIDE_TIME              "txt_installation_guide_timeout"
#define TXT_GUIDE_OK            "txt_t2_installation_guide_ok"
#define IMG_GUIDE_OK            "img_t2_installation_guide_ok"

#define S_ARROW_L               "s_arrow_l"
#define S_ARROW_R               "s_arrow_r"

typedef enum _GUIDE_ITEM
{
    GUIDE_ITEM_LANGUAGE = 0,
    GUIDE_ITEM_COUNTRY,
    GUIDE_ITEM_FTAONLY,
    GUIDE_ITEM_MODE,
    GUIDE_ITEM_ANTPWR,
    GUIDE_ITEM_SEARCH,
    GUIDE_ITEM_TOTAL
} GUIDE_ITEM;

static int s_osd_language;
static int s_country;
static int s_ftaonly;
static event_list  *s_InsGdTimer = NULL;
static char *s_InsGdtime_str = NULL;
static int s_InsGd_timeout_sec = 0;

char *search_mode[] =
{
    STR_ID_FTA,
    STR_ID_CAS,
    STR_ID_ALL
};

int gFisrtPowerOn_flag = 0;

extern void app_dvbc_auto_search_entry(void);
extern void app_dvbt2_set_antenna_power(int ant_power);

extern int AppDvbt2GetCountry(void);
extern void AppDvbt2SetCountry(int s_country);
extern void app_region_value_init(void);

static int installation_guide_timeout_cb(void *usrdata)
{
    int dvb_search_mode = 0;

    s_InsGd_timeout_sec--;

    if(s_InsGdtime_str != NULL)
    {
        GxCore_Free(s_InsGdtime_str);
        s_InsGdtime_str = NULL;
    }
    s_InsGdtime_str = app_time_to_hms_str(s_InsGd_timeout_sec);
    GUI_SetProperty(GUIDE_TIME, "string", (void *)(s_InsGdtime_str));
    GUI_SetProperty(GUIDE_TIME, "draw_now", NULL);

    if (s_InsGd_timeout_sec == 0)
    {
        APP_TIMER_REMOVE(s_InsGdTimer);
#if DEMOD_DVB_C
        if(0 == dvb_search_mode)
            app_dvbc_auto_search_entry();
        else if(1 == dvb_search_mode)
#endif
            app_dvbt2_auto_search_entry();
    }

    return 0;
}

SIGNAL_HANDLER int app_installation_guide_create(const char *widgetname, void *usrdata)
{
    uint32_t sel = 0;

    g_AppFullArb.timer_stop();
    GUI_SetFocusWidget(GUIDE_LIST_VIEW);
    GUI_SetProperty(GUIDE_LIST_VIEW, "select", (void *)&sel);
    GxBus_ConfigGetInt(OSD_LANG_KEY, &s_osd_language, OSD_LANG);
    s_country = AppDvbt2GetCountry();
    GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &s_ftaonly, SEARCH_FTA_CAS_VALUE);

    if (gFisrtPowerOn_flag)
    {
        if (s_InsGdTimer != NULL)
        {
            APP_TIMER_REMOVE(s_InsGdTimer);
        }

        APP_TIMER_ADD(s_InsGdTimer, installation_guide_timeout_cb, 3000, TIMER_REPEAT);

        s_InsGd_timeout_sec = 10;
        s_InsGdtime_str = app_time_to_hms_str(s_InsGd_timeout_sec);
        GUI_SetProperty(GUIDE_TIME, "string", (void *)(s_InsGdtime_str));
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_installation_guide_destroy(const char *widgetname, void *usrdata)
{
    if (gFisrtPowerOn_flag)
    {
        if (s_InsGdtime_str != NULL)
        {
            GxCore_Free(s_InsGdtime_str);
            s_InsGdtime_str = NULL;
        }
        if (s_InsGdTimer != NULL)
        {
            remove_timer(s_InsGdTimer);
            s_InsGdTimer = NULL;
        }
        gFisrtPowerOn_flag = 0;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_installation_guide_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    uint32_t value = 0;

    if (gFisrtPowerOn_flag)
    {
        s_InsGd_timeout_sec = 10;
        reset_timer(s_InsGdTimer);
        s_InsGdtime_str = app_time_to_hms_str(s_InsGd_timeout_sec);
        GUI_SetProperty(GUIDE_TIME, "string", (void *)(s_InsGdtime_str));
    }

    GUI_GetProperty(GUIDE_LIST_VIEW, "select", (void *)&value);
    if (event->type == GUI_KEYDOWN)
    {
        switch (event->key.sym)
        {
            case STBK_MENU:
            case STBK_EXIT:
            {
                GUI_EndDialog(WND_INSTALLATION_GUIDE);
                if( APP_THEME_CLASSIC_PURPLE ){
                    app_unset_window_back_ground(WND_FULL_SCREEN, true, false);
                }
                g_AppFullArb.timer_stop();
                GUI_CreateDialog(WND_MAIN_MENU);
                return EVENT_TRANSFER_KEEPON;
            }
            default:
                return EVENT_TRANSFER_KEEPON;;
        }
    }
    return EVENT_TRANSFER_KEEPON;

}

SIGNAL_HANDLER  int app_installation_guide_server(const char *widgetname, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int list_installation_guide_get_total(GuiWidget *widget, void *usrdata)
{
    int32_t dvb_search_mode;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(dvb_search_mode == 1)
        return GUIDE_ITEM_TOTAL;
    else
        return GUIDE_ITEM_TOTAL - 1;
}

SIGNAL_HANDLER int list_installation_guide_get_data(GuiWidget *widget, void *usrdata)
{
#define MAX_LEN	30
    static char 	buffer1[MAX_LEN];
    ListItemPara	*item = NULL;
    uint32_t value = 0;

    GUI_GetProperty(GUIDE_LIST_VIEW, "select", (void *)&value);

    item = (ListItemPara *)usrdata;
    item->x_offset = 5;
    if (item->sel == GUIDE_ITEM_LANGUAGE) //osd language
    {
        memset(buffer1, 0, MAX_LEN);
        sprintf(buffer1, "%s", STR_ID_OSD_LANGUAGE);
        item->string = buffer1;
        item->image = NULL;

        item = item->next;
        if (item->sel == value)
            item->image = S_ARROW_L;
        else
            item->image = NULL;
        item->string = NULL;

        item = item->next;
        item->image = NULL;
        item->string = g_LangName[s_osd_language];

        item = item->next;
        if (item->sel == value)
            item->image = S_ARROW_R;
        else
            item->image = NULL;
        item->string = NULL;
    }
    else if (item->sel == GUIDE_ITEM_COUNTRY)
    {
        memset(buffer1, 0, MAX_LEN);
        sprintf(buffer1, "%s", STR_ID_COUNTRY);
        item->string = buffer1;
        item->image = NULL;

        item = item->next;
        if (item->sel == value)
            item->image = S_ARROW_L;
        else
            item->image = NULL;
        item->string = NULL;

        item = item->next;
        item->image = NULL;
        {
            item->string = g_CountryName[s_country];
        }
        item = item->next;
        if (item->sel == value)
            item->image = S_ARROW_R;
        else
            item->image = NULL;
        item->string = NULL;
    }
    else if (item->sel == GUIDE_ITEM_FTAONLY)
    {
        memset(buffer1, 0, MAX_LEN);
        sprintf(buffer1, "%s", STR_ID_FTA_ONLY);
        item->string = buffer1;
        item->image = NULL;

        item = item->next;
        if (item->sel == value)
            item->image = S_ARROW_L;
        else
            item->image = NULL;
        item->string = NULL;

        item = item->next;
        item->image = NULL;
        item->string = NULL;
        item->string = search_mode[s_ftaonly - 1];
        item = item->next;
        item->image = NULL;
        if (item->sel == value)
            item->image = S_ARROW_R;
        else
            item->image = NULL;
        item->string = NULL;
    }
    else if (item->sel == GUIDE_ITEM_MODE)
    {
        int32_t dvb_search_mode;
        GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);

        memset(buffer1, 0, MAX_LEN);
        sprintf(buffer1, "%s", STR_ID_SEARCH_MODE);
        item->string = buffer1;
        item->image = NULL;

        item = item->next;
        if (item->sel == value)
            item->image = S_ARROW_L;
        else
            item->image = NULL;
        item->string = NULL;

        item = item->next;
        item->image = NULL;
        item->string = NULL;
        if (0 == dvb_search_mode)
        {
            item->string = STR_ID_C;
        }
        else if (1 == dvb_search_mode)
        {
            item->string = STR_ID_T2;
        }

        item = item->next;
        item->image = NULL;
        if (item->sel == value)
            item->image = S_ARROW_R;
        else
            item->image = NULL;
        item->string = NULL;
    }
    else if (item->sel == GUIDE_ITEM_ANTPWR)
    {
        int32_t dvb_search_mode;
        GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);

        if (0 == dvb_search_mode)
        {
            memset(buffer1, 0, MAX_LEN);
            sprintf(buffer1, "%s", STR_ID_CHANNEL_SEARCH);
            item->string = buffer1;
            item->image = NULL;

            item = item->next;
            if (item->sel == value)
                item->image = S_ARROW_R;
            else
                item->image = NULL;
            item->string = NULL;

            item = item->next;
            item->image = NULL;
            item->string = NULL;
            item = item->next;
            item->image = NULL;
            item->string = NULL;
        }
        else
        {
            static int32_t antenna_config;
            GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);

            memset(buffer1, 0, MAX_LEN);
            sprintf(buffer1, "%s", STR_ID_ANTENNA_POWER);
            item->string = buffer1;
            item->image = NULL;

            item = item->next;
            if (item->sel == value)
                item->image = S_ARROW_L;
            else
                item->image = NULL;
            item->string = NULL;

            item = item->next;
            item->image = NULL;
            item->string = NULL;
            if (antenna_config == 0)//on
            {
                item->string = "Off";
            }
            else if (antenna_config == 1)//off
            {
                item->string = "On";
            }

            item = item->next;
            item->image = NULL;
            if (item->sel == value)
                item->image = S_ARROW_R;
            else
                item->image = NULL;
            item->string = NULL;
        }
    }

    else if (item->sel == GUIDE_ITEM_SEARCH)
    {

        int32_t dvb_search_mode;
        GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
        if (0 == dvb_search_mode)
        {
            ;
        }
        else
        {
            memset(buffer1, 0, MAX_LEN);
            sprintf(buffer1, "%s", STR_ID_CHANNEL_SEARCH);
            item->string = buffer1;
            item->image = NULL;

            item = item->next;
            if (item->sel == value)
                item->image = S_ARROW_R;
            else
                item->image = NULL;
            item->string = NULL;

            item = item->next;
            item->image = NULL;
            item->string = NULL;
            item = item->next;
            item->image = NULL;
            item->string = NULL;
        }
    }

    return 0;
}

void _leftright_key(unsigned short key)
{
    uint32_t value = 0;
    int dvb_search_mode = 0;

    GUI_GetProperty(GUIDE_LIST_VIEW, "select", (void *)&value);
    if (value == GUIDE_ITEM_LANGUAGE)
    {
        GxBus_ConfigGetInt(OSD_LANG_KEY, &s_osd_language, OSD_LANG);
#if (UI_MIRROR_SUPPORT > 0)
        if( app_menu_mirror_get())
        {

            if (key == STBK_RIGHT)
            {
                key = STBK_LEFT;
            }
            else  if (key == STBK_LEFT)
            {
                key = STBK_RIGHT;
            }
        }
#endif
        if (key == STBK_RIGHT)
        {
            if (s_osd_language >= (LANG_MAX - 1))
            {
                s_osd_language = 0;
            }
            else
            {
                s_osd_language++;
            }
        }
        else if (key == STBK_LEFT)
        {
            if (s_osd_language <= 0)
            {
                s_osd_language = (LANG_MAX - 1);
            }
            else
            {
                s_osd_language--;
            }
        }
        GxBus_ConfigSetInt(OSD_LANG_KEY, s_osd_language);
        GUI_SetInterface("osd_language", g_LangName[s_osd_language]);
#if (UI_MIRROR_SUPPORT > 0)
        app_menu_mirror_set(g_LangName[s_osd_language]);
#endif
        GUI_SetProperty(WND_INSTALLATION_GUIDE, "update", NULL);

    }
    else if (value == GUIDE_ITEM_COUNTRY)
    {

        s_country = AppDvbt2GetCountry();
        if (key == STBK_RIGHT)
        {

            if (s_country >= (COUNTRY_MAX - 1))
            {
                s_country = 0;
            }
            else
            {
                s_country++;
            }
        }
        else if (key == STBK_LEFT)
        {

            if (s_country <= 0)
            {
                s_country = (COUNTRY_MAX - 1);
            }
            else
            {
                s_country--;
            }

        }
        // GxBus_ConfigSetInt(T2_COUNTRY, s_country);
        AppDvbt2SetCountry(s_country);
        app_region_value_init();
        GUI_SetProperty(GUIDE_LIST_VIEW, "update_all", NULL);
        app_initial_channels_by_country(&Total_RfCh);
    }
    else if (value == GUIDE_ITEM_FTAONLY)
    {

        GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &s_ftaonly, SEARCH_FTA_CAS_VALUE);
        if (key == STBK_RIGHT)
        {
            if (s_ftaonly == GX_SEARCH_FTA_CAS_ALL)
            {
                s_ftaonly = GX_SEARCH_FTA;
            }
            else
            {
                s_ftaonly = GX_SEARCH_FTA_CAS_ALL;
            }
        }
        else if (key == STBK_LEFT)
        {
            if (s_ftaonly == GX_SEARCH_FTA_CAS_ALL)
            {
                s_ftaonly = GX_SEARCH_FTA;
            }
            else
            {
                s_ftaonly = GX_SEARCH_FTA_CAS_ALL;
            }
        }

        GxBus_ConfigSetInt(SEARCH_FTA_CAS_KEY, s_ftaonly);
        GUI_SetProperty(GUIDE_LIST_VIEW, "update_all", NULL);
    }
    else if (value == GUIDE_ITEM_MODE)
    {
#if DEMOD_DVB_C
        GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
        if ((key == STBK_RIGHT) || (key == STBK_LEFT))
        {

            if (0 == dvb_search_mode)
            {
                dvb_search_mode = 1;
            }
            else if (1 == dvb_search_mode)
            {
                dvb_search_mode = 0;
            }

        }

        GxBus_ConfigSetInt(SEARCH_MODE_TYPE_KEY, dvb_search_mode);

        GUI_SetProperty(GUIDE_LIST_VIEW, "update_all", NULL);
#endif
    }
    else if (value == GUIDE_ITEM_ANTPWR)
    {
        GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
#if DEMOD_DVB_C
        if(0 == dvb_search_mode && STBK_RIGHT == key)
        {
            app_dvbc_auto_search_entry();
        }
        else if (1 == dvb_search_mode)
#endif
        {
            if ((key == STBK_RIGHT)||(key == STBK_LEFT))
            {
                int antenna_config = 0;
                GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);

                if (antenna_config == 0)
                {
                    antenna_config = 1;
                }
                else
                {
                    antenna_config = 0;
                }
                app_dvbt2_set_antenna_power(antenna_config);
            }
        }
        GUI_SetProperty(GUIDE_LIST_VIEW, "update_all", NULL);
    }
    else if (value == GUIDE_ITEM_SEARCH)
    {
        GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
        if(1 == dvb_search_mode && STBK_RIGHT == key)
            app_dvbt2_auto_search_entry();
    }
}

static int installation_guide_select_search_mode(void)
{
    int dvb_search_mode = 0;
    int sel_val = 0;
    uint32_t value = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(1 == dvb_search_mode)
        sel_val = GUIDE_ITEM_SEARCH;
    else
        sel_val = GUIDE_ITEM_ANTPWR;

    GUI_GetProperty(GUIDE_LIST_VIEW, "select", (void *)&value);
    if(value == sel_val)
    {
#if DEMOD_DVB_C
        if(0 == dvb_search_mode)
            app_dvbc_auto_search_entry();
        else if(1 == dvb_search_mode)
#endif
            app_dvbt2_auto_search_entry();
    }
    return 0;
}

static int _installation_guide_search_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
    PasswdDlg passwd_dlg;
	if(ret->result ==  PASSWD_OK)
	{
        installation_guide_select_search_mode();
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_exit_cb = _installation_guide_search_passwd_dlg_cb;
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }

	return 0;
}

static int installation_guide_create_search_menu(void)
{
    int init_value = 0;
    PasswdDlg passwd_dlg;
    GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
    if(init_value == 1)
    {
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_exit_cb = _installation_guide_search_passwd_dlg_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {
        installation_guide_select_search_mode();
    }

    return 0;
}

SIGNAL_HANDLER int list_installation_guide_keypress(const char *widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;

    if (gFisrtPowerOn_flag)
    {
        s_InsGd_timeout_sec = 10;
        reset_timer(s_InsGdTimer);
        s_InsGdtime_str = app_time_to_hms_str(s_InsGd_timeout_sec);
        GUI_SetProperty(GUIDE_TIME, "string", (void *)(s_InsGdtime_str));
    }

    if (event->type == GUI_KEYDOWN)
    {
        switch (event->key.sym)
        {
            case STBK_RIGHT:
            case STBK_LEFT:
                _leftright_key(event->key.sym);
                return EVENT_TRANSFER_STOP;
            case STBK_OK:
                installation_guide_create_search_menu();
                return EVENT_TRANSFER_STOP;
            case STBK_PN:
            {
                if( APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE ){
                extern void app_t2_video_set_tv_format_shortcuts(void);
                extern void shortcuts_timer_start(void);
                shortcuts_timer_start();
                app_t2_video_set_tv_format_shortcuts();
                }else{
                extern status_t app_video_standard_hotkey(void);
                app_video_standard_hotkey();
                }
                break;
            }
            default:
                return EVENT_TRANSFER_KEEPON;
        }
    }

    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int list_installation_guide_change(const char *widgetname, void *usrdata)
{
    uint32_t value = 0;
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);

    GUI_GetProperty(GUIDE_LIST_VIEW, "select", (void *)&value);
    if(value == GUIDE_ITEM_SEARCH || (GUIDE_ITEM_ANTPWR == value && 0 == dvb_search_mode))
    {
        GUI_SetProperty(TXT_GUIDE_OK, "state", "show");
        GUI_SetProperty(IMG_GUIDE_OK, "state", "show");
    }
    else
    {
        GUI_SetProperty(TXT_GUIDE_OK, "state", "hide");
        GUI_SetProperty(IMG_GUIDE_OK, "state", "hide");
    }
    return EVENT_TRANSFER_STOP;
}
#endif
