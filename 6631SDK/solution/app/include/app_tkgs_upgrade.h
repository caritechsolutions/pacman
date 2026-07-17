#ifndef __APP_TKGS_UPGRADE_H__
#define __APP_TKGS_UPGRADE_H__

#if TKGS_SUPPORT

typedef enum{
	TKGS_UPGRADE_MODE_MENU = 0,
	TKGS_UPGRADE_MODE_BACKGROUND,
	TKGS_UPGRADE_MODE_FACTORY,
	TKGS_UPGRADE_MODE_STANDBY
}APP_TKGS_UPGRADE_MODE;

void app_tkgs_set_upgrade_mode(APP_TKGS_UPGRADE_MODE mode);

void _tkgsupgradeprocess_upgrade_version_check(uint16_t pid);
int app_tkgs_upgrade_process_menu_exec(void);
#endif
#endif
