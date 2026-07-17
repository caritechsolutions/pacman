#include "app.h"
#include <common/gxpm.h>
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "sys_setting_manage.h"

#if POWER_ON_CONTROL_SUPPORT

#define POWER_ON_STANDBY_POP_SEC (15)

enum POWERONCONTROL_ITEM_NAME
{
	ITEM_POWERONCONTROL = 0,
	ITEM_POWERONCONTROL_TOTAL,
};

static SystemSettingOpt s_poweroncontrol_setting_opt;
static SystemSettingItem s_poweroncontrol_item[ITEM_POWERONCONTROL_TOTAL];
static int32_t s_PowerControl = 0;
extern int top_stbk_halt(uint32_t scan_code);
extern void app_main_menu_listview_update(void);

static void app_poweroncontrol_system_para_get(void)
{
    GxBus_ConfigGetInt(POWER_CONTROL_KEY, &s_PowerControl, POWER_CONTROL);
}

static int app_poweroncontrol_setting_exit_callback(ExitType exit_type)
{
    if(s_PowerControl != s_poweroncontrol_item[ITEM_POWERONCONTROL].itemProperty.itemPropertyCmb.sel)
    {
        GxBus_ConfigSetInt(POWER_CONTROL_KEY, s_poweroncontrol_item[ITEM_POWERONCONTROL].itemProperty.itemPropertyCmb.sel);
    }
	return 0;
}

static void app_poweroncontrol_item_init(void)
{

	s_poweroncontrol_item[ITEM_POWERONCONTROL].itemTitle = STR_ID_POWERONCONTROL;
	s_poweroncontrol_item[ITEM_POWERONCONTROL].itemType = ITEM_CHOICE;
	s_poweroncontrol_item[ITEM_POWERONCONTROL].itemProperty.itemPropertyCmb.content= "[Standby, Last state, Power On]";
	s_poweroncontrol_item[ITEM_POWERONCONTROL].itemProperty.itemPropertyCmb.sel = s_PowerControl;
	s_poweroncontrol_item[ITEM_POWERONCONTROL].itemCallback.cmbCallback.CmbChange= NULL;
	s_poweroncontrol_item[ITEM_POWERONCONTROL].itemStatus = ITEM_NORMAL;
}

void app_poweroncontrol_setting_menu_exec(void)
{
	memset(&s_poweroncontrol_setting_opt, 0 ,sizeof(s_poweroncontrol_setting_opt));
	app_poweroncontrol_system_para_get();

	s_poweroncontrol_setting_opt.menuTitle = STR_ID_POWERONCONTROL;
	s_poweroncontrol_setting_opt.itemNum = ITEM_POWERONCONTROL_TOTAL;
	s_poweroncontrol_setting_opt.timeDisplay = TIP_HIDE;
	s_poweroncontrol_setting_opt.item = s_poweroncontrol_item;

	app_poweroncontrol_item_init();
	s_poweroncontrol_setting_opt.exit = app_poweroncontrol_setting_exit_callback;
	app_system_set_create(&s_poweroncontrol_setting_opt);

}

static int standy_pop_exec(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        top_stbk_halt(0);
    }
    else
    {
        GxBus_ConfigSetInt(POWER_STANDBY_KEY, 1);
        app_set_popdlg_display_type(POP_DISPLAY_ON_BOT);
    }

    return 0;
}
static void app_standy(void)
{
    PopDlg standy_pop;

    memset(&standy_pop, 0, sizeof(PopDlg));
    standy_pop.type = POP_TYPE_YES_NO;
    standy_pop.mode = POP_MODE_UNBLOCK;
    standy_pop.str = STR_ID_POWER_OFF;
    standy_pop.timeout_sec = POWER_ON_STANDBY_POP_SEC;
    standy_pop.default_ret = POP_VAL_OK;
    standy_pop.show_time = true;
    standy_pop.exit_cb = standy_pop_exec;
    app_set_popdlg_display_type(POP_DISPLAY_ON_TOP);
    popdlg_create(&standy_pop);
}

void app_system_setting_power_control(void)
{
    unsigned int sWakeFlag = 0;
    int powerctrl_flag = 0;
    int powerstandby_flag = 0;

    GxBus_ConfigGetInt(POWER_CONTROL_KEY, &powerctrl_flag, POWER_CONTROL);
    GxBus_ConfigGetInt(POWER_STANDBY_KEY, &powerstandby_flag, POWER_STANDBY);
    GxCore_GetWakeFlag(&sWakeFlag);

    if(((powerctrl_flag == 1)&&(sWakeFlag == WAKE_FROM_POWER_OFF)&&(powerstandby_flag == 0))
        ||((powerctrl_flag == 0)&&(sWakeFlag == WAKE_FROM_POWER_OFF)))
    {
        app_standy();
    }
    else
    {
        GxBus_ConfigSetInt(POWER_STANDBY_KEY, 1);
    }

    return ;
}

int app_poweroncontrol_setting_keypress(int key)
{
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    int32_t PowerControl = 0;
    GxBus_ConfigGetInt(POWER_CONTROL_KEY, &PowerControl, POWER_CONTROL);
    if (STBK_LEFT == key)
    {
        if (PowerControl <= 0)
        {
            PowerControl = 2;
        }
        else
        {
            PowerControl --;
        }

    }
    else if (STBK_RIGHT == key)
    {
        if (PowerControl >= 2)
        {
            PowerControl = 0;
        }
        else
        {
            PowerControl ++;
        }

    }
    GxBus_ConfigSetInt(POWER_CONTROL_KEY, PowerControl);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_poweroncontrol_setting_content_get(void)
{
    int32_t PowerControl = 0;
    static char *Power_control[3] = {"Standby", "Last state", "Power On"};
    GxBus_ConfigGetInt(POWER_CONTROL_KEY, &PowerControl, POWER_CONTROL);
    return Power_control[PowerControl];
}

#endif
