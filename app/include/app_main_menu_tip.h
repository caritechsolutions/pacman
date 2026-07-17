#ifndef __APP_MAIN_MENU_TIP_H__
#define __APP_MAIN_MENU_TIP_H__

typedef enum
{
	MM_DEL_ALL_NO_CH = 0,
	MM_DEL_ALL_HAVE_CH,
	MM_DEL_ALL,
	MM_NO_CHANNEL,
	MM_NO_SAT,
	MM_MAX_SAT,
	MM_MAX_TP,
	MM_APPLETS_OPEN,
}MainMenuPopupType;

extern unsigned int app_check_channel_num(GxBusPmDataProgType type);
extern void app_mainmenu_tip_exec(MainMenuPopupType type);
extern void _delete_all_channels(void);
#endif
