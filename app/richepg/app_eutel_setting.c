#include "app.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/app_time.h"
#include "app_config.h"
#if RICHEPG_SUPPORT
#include "richepg.h"
#include "richepg_store.h"
#include "app_eutel_common.h"
#include "app_eutel_database.h"

typedef int (*_get_func)(void);
typedef void (*_item_init_func)(int item_sel);
typedef void (*_save_func)(int new_value, int *pold_value);

typedef enum {
    ITEM_MAINT,
    ITEM_LINEUP_SELECT,
    ITEM_SATTV_INFOMATION,
    ITEM_SATTV_UPDATES,
    ITEM_UPDATE_SWITCH2,
    ITEM_EPG_VIEW_STYLE,
#if EPG_VIEW_DAYS_SET_SUPPORT
    ITEM_EPG_VIEW_DAYS,
#endif
    ITEM_EPG_EVENING_START,
    ITEM_EPG_KEEP_SEC,
    ITEM_CHFILTER_KEEP_SEC,
    ITEM_LINEUP_SWITCH2,
    ITEM_UPDATE_ATTACH2,
    ITEM_RICHEPG_SYSTEM_CONFIG,
    ITEM_RICHEPG_ITEM_TOTAL,
} RichepgSettingItem;

/*
 *  如果调整菜单显示顺序，只需要调整上面的参数
 */

static int s_richepg_value_array[ITEM_RICHEPG_ITEM_TOTAL];
static SystemSettingItem s_richepg_item_array[ITEM_RICHEPG_ITEM_TOTAL];

static int _richepg_setting_save_before_btn_exec(void);
extern void app_eutel_manual_maint_menu_exec(void);
extern void app_eutel_lineup_select_menu_exec(void);
extern void app_richepginfo_menu_exec(void);


/**************** lineup switch mode ****************/

#define RICHEPG_LINEUP_MODE_CONTENT    "[Fast Switch,Full Switch]"

static int _richepg_lineup_mode_get(void)
{
    int init_value = app_richepg_lineup_mode_get();
    return init_value;
}

static void _richepg_lineup_mode_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_lineup_mode_set(new_value);
    }
}

static void _richepg_lineup_mode2_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_LINEUP_SWITCH;
    s_richepg_item_array[item_sel].itemType = ITEM_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = RICHEPG_LINEUP_MODE_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** maintenance update mode ****************/

#define RICHEPG_UPDATE_MODE_CONTENT    "[Current Sat,All Sats]"

static int _richepg_update_mode_get(void)
{
    int init_value = app_richepg_update_mode_get();
    return init_value;
}

static void _richepg_update_mode_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_update_mode_set(new_value);
    }
}

static void _richepg_update_mode2_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_MAINT_SATS;
    s_richepg_item_array[item_sel].itemType = ITEM_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = RICHEPG_UPDATE_MODE_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** maintenance attach update time ****************/

#define UPDATE_ATTACH_CONTENT    "[Off,12:00,13:00,14:00,15:00,16:00,17:00,18:00,19:00,20:00,21:00]"
static char *s_update_attach_str_array[] = {"Off", "12:00", "13:00", "14:00", "15:00", "16:00", "17:00", "18:00", "19:00", "20:00", "21:00"};
static int s_update_attach_val_array[] = {-1, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};

static int _richepg_update_attach_get(void)
{
    int init_value = app_richepg_update_attach_get();
    int total = sizeof(s_update_attach_val_array)/sizeof(s_update_attach_val_array[0]);
    int i = 0;

    for (i = 0; i < total; i++) {
        if (init_value == s_update_attach_val_array[i]) {
            return i;
        }
    }
    return 0;
}

static void _richepg_update_attach_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_update_attach_set(s_update_attach_val_array[new_value]);
    }
}

static int _update_attach2_press_cb(int key)
{
    int cmb_sel = -1;
    PopList pop_list;

    if (key == STBK_OK) {
        app_system_set_item_data_update(ITEM_UPDATE_ATTACH2);
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_SUPER_ACCURACY;
        pop_list.item_num = sizeof(s_update_attach_str_array)/sizeof(s_update_attach_str_array[0]);
        pop_list.item_content = s_update_attach_str_array;
        pop_list.sel = s_richepg_item_array[ITEM_UPDATE_ATTACH2].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num = false;
        cmb_sel = poplist_create(&pop_list);
        GUI_SetProperty(app_system_get_combobox(ITEM_UPDATE_ATTACH2), "select", &cmb_sel);
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_update_attach2_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_SUPER_ACCURACY;
    s_richepg_item_array[item_sel].itemType = ITEM_POP_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = UPDATE_ATTACH_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbPress = _update_attach2_press_cb;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** epg view style ****************/

#define RICHEPG_EPG_VIEW_STYLE_CONTENT  "[Program View,Channel View]"

static int _richepg_epg_view_style_get(void)
{
    int init_value = app_richepg_epg_view_style_get();
    return init_value;
}

static void _richepg_epg_view_style_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_epg_view_style_set(new_value);
    }
}

static void _richepg_epg_view_style_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_EPG_VIEW_STYLE;
    s_richepg_item_array[item_sel].itemType = ITEM_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = RICHEPG_EPG_VIEW_STYLE_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** new epg show days ****************/

#if EPG_VIEW_DAYS_SET_SUPPORT
static char *s_epg_view_days_str_array[] = RICHEPG_EPG_VIEW_DAYS_STR_ARRAY;
static int _richepg_epg_view_days_get(void)
{
    int init_value = app_richepg_epg_view_days_get();
    return (init_value - RICHEPG_EPG_VIEW_DAYS_MIN);
}

static void _richepg_epg_view_days_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_epg_view_days_set(new_value + RICHEPG_EPG_VIEW_DAYS_MIN);
    }
}

static int _epg_view_days_press_cb(int key)
{
    int cmb_sel = -1;
    PopList pop_list;

    if (key == STBK_OK) {
        app_system_set_item_data_update(ITEM_EPG_VIEW_DAYS);
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_EPG_VIEW_DAYS;
        pop_list.item_num = sizeof(s_epg_view_days_str_array)/sizeof(s_epg_view_days_str_array[0]);
        pop_list.item_content = s_epg_view_days_str_array;
        pop_list.sel = s_richepg_item_array[ITEM_EPG_VIEW_DAYS].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num = false;
        cmb_sel = poplist_create(&pop_list);
        GUI_SetProperty(app_system_get_combobox(ITEM_EPG_VIEW_DAYS), "select", &cmb_sel);
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_epg_view_days_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_EPG_VIEW_DAYS;
    s_richepg_item_array[item_sel].itemType = ITEM_POP_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = RICHEPG_EPG_VIEW_DAYS_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbPress = _epg_view_days_press_cb;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}
#endif

/**************** new epg show days ****************/
#define EPG_EVENING_TOTAL_HOURS 24
static char **s_epg_evening_time_str_array = NULL;
static char *s_epg_evening_time_content = NULL;

static int _richepg_epg_evening_start_get(void)
{
    int init_value = app_richepg_epg_evening_start_get();
    return init_value;
}

static void _richepg_epg_evening_start_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_epg_evening_start_set(new_value);
    }
}

static int _epg_evening_start_press_cb(int key)
{
    int cmb_sel = -1;
    PopList pop_list;

    if (key == STBK_OK) {
        app_system_set_item_data_update(ITEM_EPG_EVENING_START);
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_EPG_EVENING_START;
        pop_list.item_num = EPG_EVENING_TOTAL_HOURS;
        pop_list.item_content = s_epg_evening_time_str_array;
        pop_list.sel = s_richepg_item_array[ITEM_EPG_EVENING_START].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num = false;
        cmb_sel = poplist_create(&pop_list);
        GUI_SetProperty(app_system_get_combobox(ITEM_EPG_EVENING_START), "select", &cmb_sel);
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_epg_evening_start_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_EPG_EVENING_START;
    s_richepg_item_array[item_sel].itemType = ITEM_POP_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = s_epg_evening_time_content;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbPress = _epg_evening_start_press_cb;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** program view epg keep_sec ****************/

#define EPG_KEEP_SEC_CONTENT    "[Off,1 min,2 min,5 min,10 min,30 min,1 hour]"
static char *s_epg_keep_sec_str_array[] = {"Off", "1 min", "2 min", "5 min", "10 min", "30 min", "1 hour"};
static int s_epg_keep_sec_val_array[] = {0, 60, 120, 300, 600, 1800, 3600};

static int _richepg_epg_keep_sec_get(void)
{
    int init_value = app_richepg_epg_keep_sec_get();
    int total = sizeof(s_epg_keep_sec_val_array)/sizeof(s_epg_keep_sec_val_array[0]);
    int i = 0;

    for (i = 0; i < total; i++) {
        if (init_value == s_epg_keep_sec_val_array[i]) {
            return i;
        }
    }
    return 0;
}

static void _richepg_epg_keep_sec_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_epg_keep_sec_set(s_epg_keep_sec_val_array[new_value]);
        app_eutel_epg2_bak_timer_stop();//reset the epg program timeout 20221020  r:333193
    }
}

static int _epg_keep_sec_press_cb(int key)
{
    int cmb_sel = -1;
    PopList pop_list;

    if (key == STBK_OK) {
        app_system_set_item_data_update(ITEM_EPG_KEEP_SEC);
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_EPG_KEEP_SEC;
        pop_list.item_num = sizeof(s_epg_keep_sec_str_array)/sizeof(s_epg_keep_sec_str_array[0]);
        pop_list.item_content = s_epg_keep_sec_str_array;
        pop_list.sel = s_richepg_item_array[ITEM_EPG_KEEP_SEC].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num = false;
        cmb_sel = poplist_create(&pop_list);
        GUI_SetProperty(app_system_get_combobox(ITEM_EPG_KEEP_SEC), "select", &cmb_sel);
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_epg_keep_sec_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_EPG_KEEP_SEC;
    s_richepg_item_array[item_sel].itemType = ITEM_POP_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = EPG_KEEP_SEC_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbPress = _epg_keep_sec_press_cb;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** Channel Filter keep_sec ****************/

#define CHFILTER_KEEP_SEC_CONTENT    "[Off,0.5 min,2 min,10 min,30 min,1 hour,Always]"
static char *s_chfilter_keep_sec_str_array[] = {"Off", "0.5 min", "2 min", "10 min", "30 min", "1 hour", "Always"};
static int s_chfilter_keep_sec_val_array[] = {0, 30, 120, 600, 1800, 3600, -1};

static int _richepg_chfilter_keep_sec_get(void)
{
    int init_value = app_richepg_chfilter_keep_sec_get();
    int total = sizeof(s_chfilter_keep_sec_val_array)/sizeof(s_chfilter_keep_sec_val_array[0]);
    int i = 0;

    for (i = 0; i < total; i++) {
        if (init_value == s_chfilter_keep_sec_val_array[i]) {
            return i;
        }
    }
    return 0;
}

static void _richepg_chfilter_keep_sec_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
        *pold_value = new_value;
        app_richepg_chfilter_keep_sec_set(s_chfilter_keep_sec_val_array[new_value]);
        app_richepg_chfilter_keep_sec_clear(true);
    }
}

static int _chfilter_keep_sec_press_cb(int key)
{
    int cmb_sel = -1;
    PopList pop_list;

    if (key == STBK_OK) {
        app_system_set_item_data_update(ITEM_CHFILTER_KEEP_SEC);
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_KEEP_CHANNEL_FILTER;
        pop_list.item_num = sizeof(s_chfilter_keep_sec_str_array)/sizeof(s_chfilter_keep_sec_str_array[0]);
        pop_list.item_content = s_chfilter_keep_sec_str_array;
        pop_list.sel = s_richepg_item_array[ITEM_CHFILTER_KEEP_SEC].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num = false;
        cmb_sel = poplist_create(&pop_list);
        GUI_SetProperty(app_system_get_combobox(ITEM_CHFILTER_KEEP_SEC), "select", &cmb_sel);
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_chfilter_keep_sec_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_KEEP_CHANNEL_FILTER;
    s_richepg_item_array[item_sel].itemType = ITEM_POP_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = CHFILTER_KEEP_SEC_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbPress = _chfilter_keep_sec_press_cb;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/*------------- free ctrl build flag */
#define FREE_CTRL_BUILD_CONTENT    "[Off,Always]"
static int _richepg_memory_free_flag_get(void)
{
    int init_value = app_eutel_get_memory_free_flag();
    return init_value;
}

static void _richepg_memory_free_flag_save(int new_value, int *pold_value)
{
    if (new_value != *pold_value) {
		*pold_value = new_value;
        app_eutel_set_memory_free_flag(new_value);
    }
}

static void _richepg_memory_free_flag_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = STR_ID_MEMORY_FREE;
    s_richepg_item_array[item_sel].itemType = ITEM_POP_CHOICE;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.content = FREE_CTRL_BUILD_CONTENT;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyCmb.sel = s_richepg_value_array[item_sel];
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbChange = NULL;
    s_richepg_item_array[item_sel].itemCallback.cmbCallback.CmbPress = NULL;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** maint ****************/

static int _richepg_maint_exec_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        _richepg_setting_save_before_btn_exec();
        app_eutel_manual_maint_menu_exec();
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_maint_exec_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = "Maintenance";
    s_richepg_item_array[item_sel].itemType = ITEM_PUSH;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_richepg_item_array[item_sel].itemCallback.btnCallback.BtnPress= _richepg_maint_exec_press_callback;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** lineup select ****************/

static int _richepg_lineup_select_exec_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        _richepg_setting_save_before_btn_exec();
        app_eutel_lineup_select_menu_exec();
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_lineup_select_exec_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = "Lineup Select";
    s_richepg_item_array[item_sel].itemType = ITEM_PUSH;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_richepg_item_array[item_sel].itemCallback.btnCallback.BtnPress= _richepg_lineup_select_exec_press_callback;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** information ****************/

static int _richepg_information_exec_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        app_richepginfo_menu_exec();
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_information_exec_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = "Sat.tv Information";
    s_richepg_item_array[item_sel].itemType = ITEM_PUSH;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_richepg_item_array[item_sel].itemCallback.btnCallback.BtnPress= _richepg_information_exec_press_callback;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

/**************** updates ****************/

static int _richepg_updates_exec_press_callback(unsigned short key)
{
    if(key == STBK_OK)
    {
        app_eutel_collect_info_create_dialog();
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _richepg_updates_exec_item_init(int item_sel)
{
    s_richepg_item_array[item_sel].itemTitle = "Sat.tv Updates";
    s_richepg_item_array[item_sel].itemType = ITEM_PUSH;
    s_richepg_item_array[item_sel].itemProperty.itemPropertyBtn.string= STR_ID_PRESS_OK;
    s_richepg_item_array[item_sel].itemCallback.btnCallback.BtnPress= _richepg_updates_exec_press_callback;
    s_richepg_item_array[item_sel].itemStatus = ITEM_NORMAL;
}

static int _richepg_setting_save_value(int sel,int new_value, int *pold_value)
{
    if ( sel == ITEM_UPDATE_SWITCH2 ){
        _richepg_update_mode_save(new_value, pold_value);
    }
    else if ( sel == ITEM_EPG_VIEW_STYLE ){
        _richepg_epg_view_style_save(new_value, pold_value);
    }
#if EPG_VIEW_DAYS_SET_SUPPORT
    else if ( sel == ITEM_EPG_VIEW_DAYS ){
        _richepg_epg_view_days_save(new_value, pold_value);
    }
#endif
    else if ( sel == ITEM_EPG_EVENING_START ){
        _richepg_epg_evening_start_save(new_value, pold_value);
    }
    else if ( sel == ITEM_EPG_KEEP_SEC ){
        _richepg_epg_keep_sec_save(new_value, pold_value);
    }
    else if ( sel == ITEM_CHFILTER_KEEP_SEC ){
        _richepg_chfilter_keep_sec_save(new_value, pold_value);
    }
    else if ( sel == ITEM_LINEUP_SWITCH2 ){
        _richepg_lineup_mode_save(new_value, pold_value);
    }
    else if ( sel == ITEM_UPDATE_ATTACH2 ){
        _richepg_update_attach_save(new_value, pold_value);
    }
    else if ( sel == ITEM_RICHEPG_SYSTEM_CONFIG ){
        _richepg_memory_free_flag_save(new_value, pold_value);
    }
    else{
        app_log_error("richepg setting can not find sel = %d \n",sel);
    }
    return 0 ;
}

/**************** ftaepg Setting ****************/

static int _richepg_setting_save_before_btn_exec(void)
{
    int value_array[ITEM_RICHEPG_ITEM_TOTAL] = {0};
    int i = 0;

    for (i = 0; i < ITEM_RICHEPG_ITEM_TOTAL; i++) {
        if ( ( s_richepg_item_array[i].itemType != ITEM_POP_CHOICE ) &&
             ( s_richepg_item_array[i].itemType != ITEM_CHOICE ) ) {
            continue;
        }

        app_system_set_item_data_update(i);
        value_array[i] = s_richepg_item_array[i].itemProperty.itemPropertyCmb.sel;
        if (value_array[i] != s_richepg_value_array[i]) {
           _richepg_setting_save_value(i,value_array[i], &s_richepg_value_array[i]);
        }
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);

    APP_FREE(s_epg_evening_time_str_array);
    APP_FREE(s_epg_evening_time_content);

    return 0;
}

static bool _richepg_setting_check_cb(void)
{
    int value_array[ITEM_RICHEPG_ITEM_TOTAL] = {0};
    int i = 0;

    for (i = 0; i < ITEM_RICHEPG_ITEM_TOTAL; i++) {
        if ( ( s_richepg_item_array[i].itemType != ITEM_POP_CHOICE ) &&
             ( s_richepg_item_array[i].itemType != ITEM_CHOICE ) ) {
            continue;
        }

        app_system_set_item_data_update(i);
        value_array[i] = s_richepg_item_array[i].itemProperty.itemPropertyCmb.sel;
        if (value_array[i] != s_richepg_value_array[i]) {
            return true;
        }
    }
    return false;
}

static int _richepg_setting_save_pop_cb(PopDlgRet ret)
{
    int value_array[ITEM_RICHEPG_ITEM_TOTAL] = {0};
    int i = 0;

    if(POP_VAL_OK == ret) {
        for (i = 0; i < ITEM_RICHEPG_ITEM_TOTAL; i++) {
            if ( ( s_richepg_item_array[i].itemType != ITEM_POP_CHOICE ) &&
                 ( s_richepg_item_array[i].itemType != ITEM_CHOICE ) ) {
                continue;
            }

            app_system_set_item_data_update(i);
            value_array[i] = s_richepg_item_array[i].itemProperty.itemPropertyCmb.sel;
            if (value_array[i] != s_richepg_value_array[i]) {
                _richepg_setting_save_value(i,value_array[i], &s_richepg_value_array[i]);
            }
        }
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static int _richepg_setting_exit_cb(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;
    if (EXIT_ABANDON != exit_type)
    {
        app_system_set_callback_handler(_richepg_setting_check_cb, _richepg_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }
    APP_FREE(s_epg_evening_time_str_array);
    APP_FREE(s_epg_evening_time_content);

    return ret;
}

static void app_eutel_richepg_setting_init(int sel)
{
    if ( sel == ITEM_MAINT ){
        s_richepg_value_array[ITEM_MAINT] = 0 ;
        _richepg_maint_exec_item_init(ITEM_MAINT);
    }
    else if ( sel == ITEM_LINEUP_SELECT ){
        s_richepg_value_array[ITEM_LINEUP_SELECT] = 0 ;
        _richepg_lineup_select_exec_item_init(ITEM_LINEUP_SELECT);
    }
    else if ( sel == ITEM_SATTV_INFOMATION ){
        s_richepg_value_array[ITEM_SATTV_INFOMATION] = 0 ;
        _richepg_information_exec_item_init(ITEM_SATTV_INFOMATION);
    }
    else if ( sel == ITEM_SATTV_UPDATES ){
        s_richepg_value_array[ITEM_SATTV_UPDATES] = 0 ;
        _richepg_updates_exec_item_init(ITEM_SATTV_UPDATES);
    }
    else if ( sel == ITEM_UPDATE_SWITCH2 ){
        s_richepg_value_array[ITEM_UPDATE_SWITCH2] = _richepg_update_mode_get();
        _richepg_update_mode2_item_init(ITEM_UPDATE_SWITCH2);
    }
    else if ( sel == ITEM_EPG_VIEW_STYLE ){
        s_richepg_value_array[ITEM_EPG_VIEW_STYLE] = _richepg_epg_view_style_get();
        _richepg_epg_view_style_item_init(ITEM_EPG_VIEW_STYLE);
    }
#if EPG_VIEW_DAYS_SET_SUPPORT
    else if ( sel == ITEM_EPG_VIEW_DAYS ){
        s_richepg_value_array[ITEM_EPG_VIEW_DAYS] = _richepg_epg_view_days_get();
        _richepg_epg_view_days_item_init(ITEM_EPG_VIEW_DAYS);
    }
#endif
    else if ( sel == ITEM_EPG_EVENING_START ){
        s_richepg_value_array[ITEM_EPG_EVENING_START] = _richepg_epg_evening_start_get();
        _richepg_epg_evening_start_item_init(ITEM_EPG_EVENING_START);
    }
    else if ( sel == ITEM_EPG_KEEP_SEC ){
        s_richepg_value_array[ITEM_EPG_KEEP_SEC] = _richepg_epg_keep_sec_get();
        _richepg_epg_keep_sec_item_init(ITEM_EPG_KEEP_SEC);
    }
    else if ( sel == ITEM_CHFILTER_KEEP_SEC ){
        s_richepg_value_array[ITEM_CHFILTER_KEEP_SEC] = _richepg_chfilter_keep_sec_get();
        _richepg_chfilter_keep_sec_item_init(ITEM_CHFILTER_KEEP_SEC);
    }
    else if ( sel == ITEM_LINEUP_SWITCH2 ){
        s_richepg_value_array[ITEM_LINEUP_SWITCH2] = _richepg_lineup_mode_get();//lineup switch
        _richepg_lineup_mode2_item_init(ITEM_LINEUP_SWITCH2);
    }
    else if ( sel == ITEM_UPDATE_ATTACH2 ){
        s_richepg_value_array[ITEM_UPDATE_ATTACH2] = _richepg_update_attach_get();//super accuracy
        _richepg_update_attach2_item_init(ITEM_UPDATE_ATTACH2);
    }
    else if ( sel == ITEM_RICHEPG_SYSTEM_CONFIG ){
        s_richepg_value_array[ITEM_RICHEPG_SYSTEM_CONFIG] = _richepg_memory_free_flag_get();
        _richepg_memory_free_flag_item_init(ITEM_RICHEPG_SYSTEM_CONFIG);
    }
    else {
        app_log_error("richepg setting_init, can not find sel=%d \n",sel);
    }
}

void app_eutel_richepg_setting_menu_exec(void)
{
    static SystemSettingOpt s_setting_opt;
    int i = 0;
    char *tmp1 = NULL, *tmp2 = NULL;
    char buf[6] = {0};

    APP_FREE(s_epg_evening_time_str_array);
    APP_FREE(s_epg_evening_time_content);
    s_epg_evening_time_str_array = GxCore_Calloc(1, EPG_EVENING_TOTAL_HOURS * (sizeof(char*) + 6));
    s_epg_evening_time_content = GxCore_Calloc(1, EPG_EVENING_TOTAL_HOURS * 6 + 2);
    if (!s_epg_evening_time_str_array || !s_epg_evening_time_content)
        return;
    tmp1 = (char*)s_epg_evening_time_str_array + EPG_EVENING_TOTAL_HOURS * sizeof(char*);
    tmp2 = s_epg_evening_time_content;
    *tmp2++ = '[';
    for (i = 0; i < EPG_EVENING_TOTAL_HOURS; i++) {
        sprintf(buf, "%02d:00", i);
        s_epg_evening_time_str_array[i] = tmp1;
        memcpy(tmp1, buf, 5);
        tmp1 += 6;

        memcpy(tmp2, buf, 5);
        tmp2 += 5;
        *tmp2++ = ',';
    }
    *(--tmp2) = ']';

    for(i=0;i<ITEM_RICHEPG_ITEM_TOTAL;i++){
        app_eutel_richepg_setting_init(i);
    }

    s_setting_opt.menuTitle = STR_ID_FTAEPG_SET;
    s_setting_opt.itemNum = ITEM_RICHEPG_ITEM_TOTAL;
    s_setting_opt.timeDisplay = TIP_HIDE;
    s_setting_opt.item = s_richepg_item_array;
    s_setting_opt.forbidExitKey = false;
    s_setting_opt.exit = _richepg_setting_exit_cb;
    app_system_set_create(&s_setting_opt);
    GUI_SetProperty("img_system_setting_company_logo", "state", "show");
    GUI_SetProperty("img_system_setting_company_logo", "img", (void*)"richepg_sattv_logo_b");
    GUI_SetInterface("flush", NULL);
}

#endif
