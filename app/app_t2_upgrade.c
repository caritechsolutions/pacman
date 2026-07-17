#include "app_config.h"
#include "app_windows.h"
#include "app_str_id.h"
#include "gui_core.h"
#include "app_common_2nd_menu.h"

extern void app_t2_usb_upgrade_menu_exec(void);
#if NET_UPGRADE_SUPPORT
extern void app_create_netupgrade_menu(void);
#endif
#if OTA_SUPPORT
extern void app_ota_menu_exec(void);
#endif

static void _software_menu_entry_init(void)
{
    g_Common2ndEntryCount = 0;
    memset(g_Common2ndEntry, 0, sizeof(g_Common2ndEntry));

    g_Common2ndEntry[g_Common2ndEntryCount].title = STR_ID_USB_UPGRADE;
    g_Common2ndEntry[g_Common2ndEntryCount].create = app_t2_usb_upgrade_menu_exec;
    g_Common2ndEntryCount++;
#if NET_UPGRADE_SUPPORT
    g_Common2ndEntry[g_Common2ndEntryCount].title = "Net Upgrade";
    g_Common2ndEntry[g_Common2ndEntryCount].create = app_create_netupgrade_menu;
    g_Common2ndEntryCount++;
#endif
#if OTA_SUPPORT
    g_Common2ndEntry[g_Common2ndEntryCount].title = "OTA Upgrade";
    g_Common2ndEntry[g_Common2ndEntryCount].create = app_ota_menu_exec;
    g_Common2ndEntryCount++;
#endif

    return;
}

void app_t2_upgrade_menu_exec(void)
{
    _software_menu_entry_init();
    GUI_CreateDialog(WND_COMMON_2ND_MENU);
    GUI_SetProperty(TEXT_COMMON_2ND_TITLE, "string", STR_ID_FIRMWARE_UPGRADE);
    return;
}
