#include "app_default_params.h"
#include "app_module.h"
#include "app_wnd_search.h"
#include "app_pop.h"
#include "app_windows.h"
#if DEMOD_DVB_T || DEMOD_ISDBT
#include "app_dvbt2_search.h"
#include "app_t2_area_config.h"

extern void app_dvbt2_set_antenna_power(int ant_power);
extern void app_main_menu_listview_update(void);
extern void app_region_value_init(void);

typedef enum
{
    ITEM_SEARCH_C  = 0,
    ITEM_SEARCH_T2 = 1,
} SEARCH_MODE_ITEM;

static int _t2_search_country_poplist_cb(int32_t ret_sel, unsigned short key)
{
    int country;
    country = AppDvbt2GetCountry();

    if (country != ret_sel )
    {
        country = ret_sel ;
        AppDvbt2SetCountry(country);
        app_region_value_init();
        app_main_menu_listview_update();
    }
    poplist_destory_cb_free_item_content();
    return 0;
}

static int _t2_search_country_pop(int country_sel)
{
    int i = 0;
    int country_ret = 0;
    char **country_name = NULL;

    if (1 >= AppDvbt2GetCountryNum())
        return country_sel;

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
    pop_list.title = "Country";
    pop_list.item_num = AppDvbt2GetCountryNum();
    pop_list.item_content = country_name;
    pop_list.sel = country_sel;
    pop_list.show_num = true;
    pop_list.pos.x = APP_MAINMENU_POP_LIST_X;
    pop_list.pos.y = APP_MAINMENU_POP_LIST_Y;
    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _t2_search_country_poplist_cb;

    country_ret = poplist_create(&pop_list);
    return country_ret;
}

int app_search_mode_setting_keypress(int key)
{
#if (DEMOD_DVB_T) && (DEMOD_DVB_C)
    int dvb_search_mode = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);

    if(ITEM_SEARCH_C == dvb_search_mode)
        dvb_search_mode = ITEM_SEARCH_T2;
    else if(ITEM_SEARCH_T2 == dvb_search_mode)
        dvb_search_mode = ITEM_SEARCH_C;

    GxBus_ConfigSetInt(SEARCH_MODE_TYPE_KEY, dvb_search_mode);
    app_main_menu_listview_update();
#endif
    return EVENT_TRANSFER_KEEPON;
}

char *app_search_mode_setting_content_get(void)
{
    char *string = NULL;
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(ITEM_SEARCH_C == dvb_search_mode)
        string = STR_ID_C;
    else if(ITEM_SEARCH_T2 == dvb_search_mode)
        string = STR_ID_T2;

    return string;
}

int app_search_fta_only_setting_keypress(int key)
{
    int32_t fta_cas_mode;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &fta_cas_mode, SEARCH_FTA_CAS_VALUE);

    if(GX_SEARCH_FTA_CAS_ALL == fta_cas_mode)
        fta_cas_mode = GX_SEARCH_FTA;
    else
        fta_cas_mode = GX_SEARCH_FTA_CAS_ALL;

    GxBus_ConfigSetInt(SEARCH_FTA_CAS_KEY, fta_cas_mode);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_search_fta_only_setting_content_get(void)
{
    char *string = NULL;
    int32_t fta_cas_mode = 0;

    GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &fta_cas_mode, SEARCH_FTA_CAS_VALUE);

    if(GX_SEARCH_FTA_CAS_ALL == fta_cas_mode)
        string = STR_ID_ALL;
    else
        string = STR_ID_FTA;

    return string;
}

int app_dvbt_country_setting_check(void)
{
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(ITEM_SEARCH_T2 == dvb_search_mode)
        return 1;

    return 0;
}

int app_dvbt_country_setting_keypress(int key)
{
    int country;
    if(STBK_LEFT != key && STBK_RIGHT != key && STBK_OK != key)
        return EVENT_TRANSFER_KEEPON;

    country = AppDvbt2GetCountry();

    if(key == STBK_RIGHT)
    {
        if(country >= (COUNTRY_MAX - 1))
            country = 0;
        else
            country++;
    }
    else if(key == STBK_LEFT)
    {
        if(country <= 0)
            country = COUNTRY_MAX - 1;
        else
            country--;
    }
    else
    {
        _t2_search_country_pop(country);
        return EVENT_TRANSFER_STOP;
    }
    AppDvbt2SetCountry(country);
    app_region_value_init();
    app_main_menu_listview_update();
    return EVENT_TRANSFER_KEEPON;
}

char *app_dvbt_country_setting_content_get(void)
{
    int32_t country = 0;
    country = AppDvbt2GetCountry();
    return AppDvbt2GetCountryName(country);
}

int app_antenna_power_setting_check(void)
{
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(ITEM_SEARCH_T2 == dvb_search_mode)
        return 1;

    return 0;
}

int app_antenna_power_setting_keypress(int key)
{
    int antenna_config = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);
    if(0 == antenna_config)
        antenna_config = 1;
    else
        antenna_config = 0;

    app_dvbt2_set_antenna_power(antenna_config);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_antenna_power_setting_content_get(void)
{
    char *string = NULL;
    int antenna_config = 0;

    GxBus_ConfigGetInt(T2_ANTENNA_POWER_KEY, &antenna_config, T2_ANTENNA_POWER_VALUE);
    if(0 == antenna_config)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
#endif
