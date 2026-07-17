#include "app.h"
#include "app_pop.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_str_id.h"
#include "app_windows.h"
#include "app_common_2nd_menu.h"
#include "app_main_menu.h"

extern void app_time_setting_menu_exec(void);
extern void app_timer_setting_menu_exec(void);
extern void app_lang_setting_menu_exec(void);
extern void app_lock_setting_menu_exec(void);
extern void app_osd_setting_menu_exec(void);
extern void app_color_setting_menu_exec(void);
extern void app_advanced_setting_menu_exec(void);
extern void app_reset_menu_exec(void);
extern void app_pvr_set_menu_exec(void);
extern void app_power_on_off_menu_exec(void);
extern void app_av_setting_menu_exec(void);
#if PROGRAM_NETUP_SUPPORT
extern void app_prog_netup_menu_create(void);
#endif
#if DEMOD_DVB_S
extern void app_antenna_setting_menu_exec(void);
extern void app_positioner_setting_menu_exec(void);
extern void app_sat_list_menu_exec(void);
extern void app_tp_list_menu_exec(void);

static void _installation_entry_init(void)
{
    int n = 0;
    memset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));
    g_Common2ndEntry[n].title = STR_ID_ATNE_SET;
    g_Common2ndEntry[n++].create = app_antenna_setting_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_POSI_SET;
    g_Common2ndEntry[n++].create = app_positioner_setting_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_SAT_LIST;
    g_Common2ndEntry[n++].create = app_sat_list_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_TP_LIST;
    g_Common2ndEntry[n++].create = app_tp_list_menu_exec;
    g_Common2ndEntryCount = n;
}

void app_installatio_menu_exec(void)
{
    _installation_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", STR_ID_INSTALLATION);
    return;
}
#endif

static void _setting_entry_init(void)
{
    int n = 0;
    memset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));
    g_Common2ndEntry[n].title = STR_ID_TIME_SET;
    g_Common2ndEntry[n++].create = app_time_setting_menu_exec;
#if OTT_SUPPORT
    g_Common2ndEntry[n].title = STR_ID_POWER_ON_OFF;
    g_Common2ndEntry[n++].create = app_power_on_off_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_AV_SET;
    g_Common2ndEntry[n++].create = app_av_setting_menu_exec;
#else
    g_Common2ndEntry[n].title = STR_ID_TIMER_SET;
    g_Common2ndEntry[n++].create = app_timer_setting_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_ADVANCED_SET;
    g_Common2ndEntry[n++].create = app_advanced_setting_menu_exec;
#endif
    g_Common2ndEntry[n].title = STR_ID_LANG;
    g_Common2ndEntry[n++].create = app_lang_setting_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_OSD_SET;
    g_Common2ndEntry[n++].create = app_osd_setting_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_LOCK_CTRL;
    g_Common2ndEntry[n++].create = app_lock_setting_menu_exec;
    g_Common2ndEntry[n].title = STR_ID_COLOR_SET;
    g_Common2ndEntry[n++].create = app_color_setting_menu_exec;
    g_Common2ndEntryCount = n;
}

void app_setting_menu_exec(void)
{
    _setting_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", STR_ID_SYS_SET);
    return;
}

static void _utility_entry_init(void)
{
    int n = 0;
    memset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));
    g_Common2ndEntry[n].title = STR_ID_FACTORY_RESET;
    g_Common2ndEntry[n++].create = app_reset_menu_exec;
#if !OTT_SUPPORT
    g_Common2ndEntry[n].title = STR_ID_PVR_MANAGE;
    g_Common2ndEntry[n++].create = app_pvr_set_menu_exec;
#if PROGRAM_NETUP_SUPPORT
    g_Common2ndEntry[n].title = STR_ID_PROGRAM_NETUP_TITLE;
    g_Common2ndEntry[n++].create = app_prog_netup_menu_create;
#endif
#endif
    g_Common2ndEntryCount = n;
}

void app_utility_menu_exec(void)
{
    _utility_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", STR_ID_UTILITY);
    return;
}
