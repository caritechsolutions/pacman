#include "app.h"
#include "app_pop.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_str_id.h"
#include "app_windows.h"
#include "app_common_2nd_menu.h"
#include "app_main_menu.h"

#if DLNADMP_SUPPORT
extern void app_create_dlna_dmp_menu(void);
#endif
#if DLNADMR_SUPPORT
extern void app_create_dlna_dmr_menu(void);
#endif
#if DLNASAT2IP_SUPPORT
extern void app_create_dlna_sat2ip_menu(void);
#endif

#if FM_SUPPORT
extern void app_fm_setting_menu_exec(void);
extern void app_fm_auto_search_exec(void);
extern void app_fm_play_exec(void);
#endif

extern void app_media_file_menu_exec(void);
extern void app_media_video_menu_exec(void);
extern void app_media_music_menu_exec(void);
extern void app_media_picture_menu_exec(void);

extern void _internet_entry_init(void);
extern int _internet_entry_count_get(void);
extern void app_netapps_event_key_check(int key);
static void _media_entry_init(void)
{
    g_Common2ndEntryCount = 0;
    memset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));

    g_Common2ndEntry[0].title = STR_ID_FILE;
    g_Common2ndEntry[1].title = STR_ID_VIDEO;
    g_Common2ndEntry[2].title = STR_ID_MUSIC;
    g_Common2ndEntry[3].title = STR_ID_PICTURE;
    g_Common2ndEntry[0].create = app_media_file_menu_exec;
    g_Common2ndEntry[1].create = app_media_video_menu_exec;
    g_Common2ndEntry[2].create = app_media_music_menu_exec;
    g_Common2ndEntry[3].create = app_media_picture_menu_exec;
    g_Common2ndEntryCount = 4;
}

static void _fm_entry_init(void)
{
    g_Common2ndEntryCount = 0;
#if FM_SUPPORT
    uint32_t tuner = 0;
    AppFrontend_Monitor monitor;

    memset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));

    g_Common2ndEntry[0].title = STR_ID_FM_SETTING;
    g_Common2ndEntry[1].title = STR_ID_FM_AUTO_SEARCH;
    g_Common2ndEntry[2].title = STR_ID_FM_PLAY;
    g_Common2ndEntry[0].create = app_fm_setting_menu_exec;
    g_Common2ndEntry[1].create = app_fm_auto_search_exec;
    g_Common2ndEntry[2].create = app_fm_play_exec;
    g_Common2ndEntryCount = 3;

    tuner = app_tuner_cur_tuner_get();
    monitor = FRONTEND_MONITOR_OFF;
    app_ioctl(tuner, FRONTEND_MONITOR_SET, &monitor);
    GxFrontend_SetUnlock(tuner);
    GxFrontend_CleanRecordPara(tuner);
#endif
}

static void _dlna_entry_init(void)
{
#if DLNADMP_SUPPORT || DLNADMR_SUPPORT || DLNASAT2IP_SUPPORT
    int32_t netapp_enable = 0;
#endif

    g_Common2ndEntryCount = 0;
    memset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));

#if DLNADMP_SUPPORT
    if(app_oem_get_int(OEM_DLNA_DMP, &netapp_enable) == GXCORE_SUCCESS)
    {
        if(netapp_enable == 1)
        {
            g_Common2ndEntry[g_Common2ndEntryCount].title = "DMP";
            g_Common2ndEntry[g_Common2ndEntryCount].create = app_create_dlna_dmp_menu;
            g_Common2ndEntry[g_Common2ndEntryCount].attr = MENU_NETWORK_CHECK;
            g_Common2ndEntryCount++;
        }
    }
#endif
#if DLNADMR_SUPPORT
    if(app_oem_get_int(OEM_DLNA_DMR, &netapp_enable) == GXCORE_SUCCESS)
    {
        if(netapp_enable == 1)
        {
            g_Common2ndEntry[g_Common2ndEntryCount].title = "DMR";
            g_Common2ndEntry[g_Common2ndEntryCount].create = app_create_dlna_dmr_menu;
            g_Common2ndEntry[g_Common2ndEntryCount].attr = MENU_NETWORK_CHECK;
            g_Common2ndEntryCount++;
        }
    }
#endif
#if DLNASAT2IP_SUPPORT
    if(app_oem_get_int(OEM_DLNA_SAT2IP, &netapp_enable) == GXCORE_SUCCESS)
    {
        if(netapp_enable == 1)
        {
            g_Common2ndEntry[g_Common2ndEntryCount].title = "Sat2IP";
            g_Common2ndEntry[g_Common2ndEntryCount].create = app_create_dlna_sat2ip_menu;
            g_Common2ndEntry[g_Common2ndEntryCount].attr = MENU_NETWORK_CHECK;
            g_Common2ndEntryCount++;
        }
    }
#endif
    return;
}

static int _dlna_entry_count_get(void)
{
    static int ret = -1;
#if DLNADMP_SUPPORT || DLNADMR_SUPPORT || DLNASAT2IP_SUPPORT
    int32_t netapp_enable = 0;
#endif

    if(ret >= 0)
        return ret;
    ret = 0;

#if DLNADMP_SUPPORT
    if(app_oem_get_int(OEM_DLNA_DMP, &netapp_enable) == GXCORE_SUCCESS)
    {
        if(netapp_enable == 1)
            ret++;
    }
#endif
#if DLNADMR_SUPPORT
    if(app_oem_get_int(OEM_DLNA_DMR, &netapp_enable) == GXCORE_SUCCESS)
    {
        if(netapp_enable == 1)
            ret++;
    }
#endif
#if DLNASAT2IP_SUPPORT
    if(app_oem_get_int(OEM_DLNA_SAT2IP, &netapp_enable) == GXCORE_SUCCESS)
    {
        if(netapp_enable == 1)
            ret++;
    }
#endif
    return ret;
}

int app_internet_menu_check(void)
{
    int ret = 0;

    if(_internet_entry_count_get() > 0)
        ret = 1;
    else
        ret = 0;
    return ret;
}

void app_internet_menu_exec(void)
{
    _internet_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", STR_ID_INTERNET);
    return;
}
void app_adult_video_control(int key)
{
    char *title_buf = NULL;
    GUI_GetProperty(TEXT_COMMON_2ND_TITLE, "string", &title_buf);
    if(strcmp(title_buf,STR_ID_INTERNET) == 0)
    {
#if NETAPPS_SUPPORT
        app_netapps_event_key_check(key);
#endif
        _internet_entry_init();
        GUI_SetProperty(LISTVIEW_COMMON2ND_MENU, "update_all", NULL);
    }
    return;
}

void app_dvbt_media_centre_menu_exec(void)
{
    _media_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", STR_ID_MEDIA_CENTER);
    return;
}

int app_dvbt_dlna_menu_check(void)
{
    int ret = 0;

    if(_dlna_entry_count_get() > 0)
        ret = 1;
    else
        ret = 0;
    return ret;
}
void app_dvbt_fm_exec(void)
{
    _fm_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", "FM");
    return;
}

void app_dvbt_dlna_menu_exec(void)
{
    _dlna_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", STR_ID_DLNA);
    return;
}
