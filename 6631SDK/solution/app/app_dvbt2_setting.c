/*****************************************************************************
 * 			  CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2009-2014, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	app_dvbt2_search.c
 * Author    : 	baiyingshan
 * Project   :	dvbt2 Foundation
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 1.0  	2014.06	          		 Dugan Du    	Create
 *****************************************************************************/
//public header files
#include "app.h"

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_windows.h"
#include "app_wnd_system_setting_opt.h"
#include "app_dvbt2_search.h"
#include "app_volume_value.h"
#include "app_t2_area_config.h"

enum ITEM_DVBT2_NAME
{
    ITEM_DVBT2_COUNTRY = 0,
    ITEM_DVBT2_FTA,
    ITEM_DVBT2_AUTO,
    ITEM_DVBT2_MANUAL,
    ITEM_DVBT2_ANNTENA,
    ITEM_DVBT2_TOTAL
};

static SystemSettingOpt s_dvbt2_opt;
static SystemSettingItem s_dvbt2_item[ITEM_DVBT2_TOTAL];
static char sCountrybuffer[MAX_COUNTRY_NAME_LEN + 8] = {0};

#if (DVBT2_ANT_POWER > 0)
static event_list *s_ant_power_check_timer = NULL;

static int check_exit(PopDlgRet Ret)
{
    int32_t antenna_config = 0;
    char *title = app_system_get_title();

    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_SYSTEM_SETTING) && title && 0 == strcmp(STR_ID_SEARCH_SETTING, title))
    {
        GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);
        s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemProperty.itemPropertyCmb.sel = antenna_config;
        app_system_set_item_property(ITEM_DVBT2_ANNTENA, &s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemProperty);
    }

    return 0;
}

void app_dvbt2_set_antenna_power(int ant_power)
{
    GxBus_ConfigSetInt(T2_ANTENNA_POWER_KEY, ant_power);
    app_gpio_ant_power_set(ant_power);
}

static int _ant_power_check_timer(void *usrdata)
{
    int32_t antenna_config;

    GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);

    if(0 == antenna_config || 1 == app_gpio_ant_power_get())
        return 0;

    if(0 == app_gpio_ant_power_get())
    {
        app_dvbt2_set_antenna_power(0);

        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_SHORT_CIRCUIT;
        pop.title = STR_ID_ANTENNA_POWER;
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = check_exit;
        popdlg_create(&pop);
        app_log_info("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^LNB short\n");
    }
    return 0;
}

int app_dvbt2_antenna_power_init(void)
{
    int32_t antenna_config;

    GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);
    app_gpio_ant_power_set(antenna_config);

    APP_TIMER_ADD(s_ant_power_check_timer, _ant_power_check_timer, 30, TIMER_REPEAT);
    return 0;
}
#endif

static int _app_dvbt2_country_poplist_cb(int32_t ret_sel, unsigned short key)
{
    int    country = 0;

    country = AppDvbt2GetCountry();
    if (country != ret_sel)
    {
        country = ret_sel ;
        AppDvbt2SetCountry(country);
        sprintf(sCountrybuffer, "[%s]", AppDvbt2GetCountryName(country));
        app_system_set_item_property(ITEM_DVBT2_COUNTRY, &s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemProperty);
    }
    poplist_destory_cb_free_item_content();
    return 0;
}

static int  app_dvbt2_country_pop(int country_sel)
{
    int i = 0;
    int country_ret = 0;
    char **country_name = NULL;

    if (1 >= AppDvbt2GetCountryNum())
    {
        return country_sel;
    }

    country_name = (char **)GxCore_Malloc(AppDvbt2GetCountryNum() * sizeof(char *));
    if (country_name == NULL)
    {
        app_log_error("\nt2_search_country_pop_failed, malloc failed...%s,%d\n", __FILE__, __LINE__);
        return country_sel;
    }
    else
    {
        for (i = 0; i < AppDvbt2GetCountryNum(); i++)
        {
            country_name[i] = (char *)GxCore_Malloc(MAX_COUNTRY_NAME_LEN);
            if (country_name[i] != NULL)
            {
                memset(country_name[i], 0, MAX_COUNTRY_NAME_LEN);
                strcpy(country_name[i], (char *)AppDvbt2GetCountryName(i));
            }
        }
    }

    PopList pop_list;
    memset(&pop_list, 0, sizeof(PopList));
    pop_list.title = STR_ID_COUNTRY;
    pop_list.item_num = AppDvbt2GetCountryNum();
    pop_list.item_content = country_name;
    pop_list.sel = country_sel;
    pop_list.show_num = true;
    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _app_dvbt2_country_poplist_cb;

    country_ret = poplist_create(&pop_list);
    return country_ret;
}

static int app_dvbt2_country_change_callback(int sel)
{
    int32_t country = 0;
    country = AppDvbt2GetCountry();
    sprintf(sCountrybuffer, "[%s]", AppDvbt2GetCountryName(country));
    app_system_set_item_property(ITEM_DVBT2_COUNTRY, &s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemProperty);

    return EVENT_TRANSFER_KEEPON;
}
static int app_dvbt2_country_cmb_press_callback(int key)
{
    int    country = 0;

    country = AppDvbt2GetCountry();
    if (key == STBK_RIGHT)
    {
        if (country >= (COUNTRY_MAX - 1))
        {
            country = 0;
        }
        else
        {
            country++;
        }
        AppDvbt2SetCountry(country);
    }
    else if (key == STBK_LEFT)
    {
        if (country <= 0)
        {
            country = COUNTRY_MAX - 1;
        }
        else
        {
            country--;
        }
        AppDvbt2SetCountry(country);
    }
    else if (key == STBK_OK)
    {
        country = app_dvbt2_country_pop(country);
    }

    return EVENT_TRANSFER_KEEPON;
}
static int app_dvbt2_fta_change_callback(int sel)
{
    int32_t fta_cas_mode;

    if (sel == 0)
    {
        fta_cas_mode = GX_SEARCH_FTA;
    }
    else
    {
        fta_cas_mode = GX_SEARCH_FTA_CAS_ALL;
    }
    GxBus_ConfigSetInt(SEARCH_FTA_CAS_KEY, fta_cas_mode);

    return EVENT_TRANSFER_KEEPON;
}

static int _app_dvbt2_auto_button_press_popdlg_exit_cb(PopDlgRet ret)
{
    int32_t sat_id = 0;

    if ( ret == POP_VAL_OK )
    {
        sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);
        app_all_channels_del_by_sat_id(sat_id);
        app_all_tp_del_by_sat_id(sat_id);
        app_dvbt2_auto_search_entry();
    }

    return 0 ;
}

static int app_dvbt2_auto_button_press_callback(unsigned short key)
{
    int32_t fta_cas_mode;
    uint32_t SearchTvNum = 0;
    uint32_t SearchRadioNum = 0;

    int32_t sat_id = 0;
    if(key != STBK_OK)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &fta_cas_mode, SEARCH_FTA_CAS_VALUE);

    sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);
    app_channel_num_check_by_sat_id(sat_id, &SearchTvNum, &SearchRadioNum);

    if((SearchTvNum == 0) && (SearchRadioNum == 0))
    {
        app_all_channels_del_by_sat_id(sat_id);
        app_all_tp_del_by_sat_id(sat_id);
        app_dvbt2_auto_search_entry();
    }
    else
    {
        PopDlg  pop;

        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_DEL_ALLCHANNEL;
        pop.exit_cb = _app_dvbt2_auto_button_press_popdlg_exit_cb;
        popdlg_create(&pop);
    }

    return EVENT_TRANSFER_KEEPON;
}
static int app_dvbt2_manual_button_press_callback(unsigned short key)
{
    if (key == STBK_OK)
    {
        extern void app_dvbt_manual_search_menu_exec(void);
        app_dvbt_manual_search_menu_exec();
    }
    return EVENT_TRANSFER_KEEPON;
}
static int app_dvbt2_anntenna_change_callback(int sel)
{
#if (DVBT2_ANT_POWER > 0)
    app_dvbt2_set_antenna_power(sel);
#endif
    return EVENT_TRANSFER_KEEPON;
}
static void app_dvbt2_country_item_init(void)
{
    int32_t country = 0;

    country = AppDvbt2GetCountry();
    sprintf(sCountrybuffer, "[%s]", AppDvbt2GetCountryName(country));
    s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemTitle = STR_ID_COUNTRY;
    s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemType = ITEM_POP_CHOICE;
    s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemProperty.itemPropertyCmb.content = sCountrybuffer;
    s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemProperty.itemPropertyCmb.sel = 0;
    s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemCallback.cmbCallback.CmbChange = app_dvbt2_country_change_callback;
    s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemCallback.cmbCallback.CmbPress = app_dvbt2_country_cmb_press_callback;
    s_dvbt2_item[ITEM_DVBT2_COUNTRY].itemStatus = ITEM_NORMAL;
}
static void app_dvbt2_fta_item_init(void)
{
    int cmb_sel = -1;

    int32_t fta_cas_mode;
    GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &fta_cas_mode, SEARCH_FTA_CAS_VALUE);
    if (fta_cas_mode == GX_SEARCH_FTA)
    {
        cmb_sel = 0;
    }
    else
    {
        cmb_sel = 1;
    }

    s_dvbt2_item[ITEM_DVBT2_FTA].itemTitle = STR_ID_FTA_ONLY;
    s_dvbt2_item[ITEM_DVBT2_FTA].itemType = ITEM_POP_CHOICE;
    s_dvbt2_item[ITEM_DVBT2_FTA].itemProperty.itemPropertyCmb.content = "[FTA,ALL]"; //"[English,Chinese]";
    s_dvbt2_item[ITEM_DVBT2_FTA].itemProperty.itemPropertyCmb.sel = cmb_sel;
    s_dvbt2_item[ITEM_DVBT2_FTA].itemCallback.cmbCallback.CmbChange = app_dvbt2_fta_change_callback;
    s_dvbt2_item[ITEM_DVBT2_FTA].itemCallback.cmbCallback.CmbPress = NULL;
    s_dvbt2_item[ITEM_DVBT2_FTA].itemStatus = ITEM_NORMAL;
}
static void app_dvbt2_auto_item_init(void)
{
    s_dvbt2_item[ITEM_DVBT2_AUTO].itemTitle = STR_ID_AUTO_SEARCH;
    s_dvbt2_item[ITEM_DVBT2_AUTO].itemType = ITEM_PUSH;
    s_dvbt2_item[ITEM_DVBT2_AUTO].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    s_dvbt2_item[ITEM_DVBT2_AUTO].itemCallback.btnCallback.BtnPress = app_dvbt2_auto_button_press_callback;
    s_dvbt2_item[ITEM_DVBT2_AUTO].itemStatus = ITEM_NORMAL;
}
static void app_dvbt2_manual_item_init(void)
{
    s_dvbt2_item[ITEM_DVBT2_MANUAL].itemTitle = STR_ID_MANUAL_SEARCH;
    s_dvbt2_item[ITEM_DVBT2_MANUAL].itemType = ITEM_PUSH;
    s_dvbt2_item[ITEM_DVBT2_MANUAL].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    s_dvbt2_item[ITEM_DVBT2_MANUAL].itemCallback.btnCallback.BtnPress = app_dvbt2_manual_button_press_callback;
    s_dvbt2_item[ITEM_DVBT2_MANUAL].itemStatus = ITEM_NORMAL;
}
static void app_dvbt2_anntenna_item_init(void)
{
    int32_t antenna_config;
    GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);

    s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemTitle = STR_ID_ANTENNA_POWER;
    s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemType = ITEM_POP_CHOICE;
    s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemProperty.itemPropertyCmb.content = "[Off,On]";
    s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemProperty.itemPropertyCmb.sel = antenna_config;
    s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemCallback.cmbCallback.CmbChange = app_dvbt2_anntenna_change_callback;
    s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemCallback.cmbCallback.CmbPress = NULL;
    s_dvbt2_item[ITEM_DVBT2_ANNTENA].itemStatus = ITEM_NORMAL;
}
static int app_dvbt2_setting_exit_callback(ExitType exit_type)
{
    return EXIT_RET_DEFAULT;
}

void app_dvbt2_setting(void)
{
    app_dvbt2_country_item_init();
    app_dvbt2_fta_item_init();
    app_dvbt2_auto_item_init();
    app_dvbt2_manual_item_init();
    app_dvbt2_anntenna_item_init();
    memset(&s_dvbt2_opt, 0, sizeof(s_dvbt2_opt));

    s_dvbt2_opt.menuTitle = STR_ID_SEARCH_SETTING;
    s_dvbt2_opt.titleImage = "s_title_left_utility";
    s_dvbt2_opt.itemNum = ITEM_DVBT2_TOTAL;
    s_dvbt2_opt.timeDisplay = TIP_HIDE;
    s_dvbt2_opt.item = s_dvbt2_item;
    s_dvbt2_opt.exit = app_dvbt2_setting_exit_callback;

    app_system_set_create(&s_dvbt2_opt);

    return;
}

#endif//DEMOD_DVB_T
