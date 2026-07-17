#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#if CA_SUPPORT

#if EX_SHIFT_SUPPORT
extern void app_exshift_menu_exec(void);
#endif
enum TIME_NAME
{
#if EX_SHIFT_SUPPORT
	ITEM_EXSHIFT,
#endif
	ITEM_SPECIAL_TOTAL
};



static SystemSettingOpt sg_SpecialSettingOpt;
static SystemSettingItem sg_SpecialSettingItem[ITEM_SPECIAL_TOTAL];

static int app_special_exit_callback(ExitType exit_type)
{
    return EXIT_RET_DEFAULT;
}

#if EX_SHIFT_SUPPORT
static int app_special_delay_item_press_callback(unsigned short key)
{
	int ret = EVENT_TRANSFER_KEEPON;

	switch(key)
	{
		case STBK_OK:
		{
			app_exshift_menu_exec();
			break;
		}

		default:
			break;
	}

	return ret;
}
static void app_special_delay_item_init(void)
{
	sg_SpecialSettingItem[ITEM_EXSHIFT].itemTitle = "EX-Shift Setting";
	sg_SpecialSettingItem[ITEM_EXSHIFT].itemType = ITEM_PUSH;
	sg_SpecialSettingItem[ITEM_EXSHIFT].itemProperty.itemPropertyBtn.string = "Press OK";
	sg_SpecialSettingItem[ITEM_EXSHIFT].itemCallback.btnCallback.BtnPress = app_special_delay_item_press_callback;
	sg_SpecialSettingItem[ITEM_EXSHIFT].itemStatus = ITEM_NORMAL;
}
#endif


void app_special_menu_exec(void)
{
	if(ITEM_SPECIAL_TOTAL == 0)
	{
		app_log_error("\n-------no one need to setting----\n");
		return;
	}
	memset(&sg_SpecialSettingOpt, 0 ,sizeof(sg_SpecialSettingOpt));

	sg_SpecialSettingOpt.menuTitle = STR_ID_SPECIAL;
	sg_SpecialSettingOpt.titleImage = "s_title_left_utility";
	sg_SpecialSettingOpt.itemNum = ITEM_SPECIAL_TOTAL;
	sg_SpecialSettingOpt.timeDisplay = TIP_HIDE;
	sg_SpecialSettingOpt.item = sg_SpecialSettingItem;
#if EX_SHIFT_SUPPORT
	app_special_delay_item_init();
#endif
	sg_SpecialSettingOpt.exit = app_special_exit_callback;

	app_system_set_create(&sg_SpecialSettingOpt);
}
#endif


