#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#if EX_SHIFT_SUPPORT
enum
{
	ITEM_EXSHIFT_CONTROL = 0,
	ITEM_EXSHIFT_TIME,
	ITEM_EXSHIFT_TOTAL
};
typedef enum
{
	_DISABLE = 0,
	_ENABLE ,
}EXSHIFT_CONTROL;



typedef struct
{
	EXSHIFT_CONTROL     EXShiftControlSwitch;
	int				  	EXShiftTime;
}EXShiftPara;

static SystemSettingOpt sg_EXShiftSettingOpt;
static SystemSettingItem sg_EXShiftSettingItem[ITEM_EXSHIFT_TOTAL];
static EXShiftPara sg_EXShiftPara;


static void app_exshift_setting_para_get(void)
{
	int init_value = 0;

	GxBus_ConfigGetInt(EXSHIFT_KEY, &init_value, EXSHIFT_DEFAULT);
	sg_EXShiftPara.EXShiftControlSwitch = init_value;

	GxBus_ConfigGetInt(EXSHIFT_TIME, &init_value, EXSHIFT_TIME_DEFAULT);
	sg_EXShiftPara.EXShiftTime = init_value/500;
}

static void app_exshift_setting_result_para_get(EXShiftPara *ret_para)
{
    ret_para->EXShiftControlSwitch = sg_EXShiftSettingItem[ITEM_EXSHIFT_CONTROL].itemProperty.itemPropertyCmb.sel;
    ret_para->EXShiftTime = sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemProperty.itemPropertyCmb.sel;
}

static bool app_exshift_setting_para_change(EXShiftPara *ret_para)
{
	bool ret = FALSE;

	if(sg_EXShiftPara.EXShiftControlSwitch != ret_para->EXShiftControlSwitch)
	{
		ret = TRUE;
	}

	if(sg_EXShiftPara.EXShiftTime != ret_para->EXShiftTime)
	{
		ret = TRUE;
	}

	return ret;
}

static int app_exshift_setting_para_save(EXShiftPara *ret_para)
{
	int init_value = 0;

    init_value = ret_para->EXShiftControlSwitch;
    GxBus_ConfigSetInt(EXSHIFT_KEY,init_value);

    init_value = ret_para->EXShiftTime*500;
    GxBus_ConfigSetInt(EXSHIFT_TIME,init_value);

	return 0;
}

static int _exshift_setting_save_pop_cb(PopDlgRet ret)
{
    EXShiftPara para_ret;
    if(POP_VAL_OK == ret)
    {
        app_exshift_setting_result_para_get(&para_ret);
        app_exshift_setting_para_save(&para_ret);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static bool _exshift_setting_check_cb(void)
{
    bool ret = false;
    EXShiftPara para_ret;

    app_exshift_setting_result_para_get(&para_ret);
    ret = app_exshift_setting_para_change(&para_ret);

    return ret;
}

static int app_exshift_setting_exit_callback(ExitType Type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == Type)
    {
        ;
    }
    else
    {
        app_system_set_callback_handler(_exshift_setting_check_cb, _exshift_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }

	return ret;
}

static int app_exshift_setting_change_callback(int sel)
{
	if(_DISABLE == sel)
	{
		app_system_set_item_state_update(ITEM_EXSHIFT_TIME,ITEM_DISABLE);
	}
	else
	{
		app_system_set_item_state_update(ITEM_EXSHIFT_TIME,ITEM_NORMAL);
	}
	return 0;
}
static void app_exshift_setting_switch_item_init(void)
{
	sg_EXShiftSettingItem[ITEM_EXSHIFT_CONTROL].itemTitle = "EX-Shift Enable";
	sg_EXShiftSettingItem[ITEM_EXSHIFT_CONTROL].itemType = ITEM_CHOICE;
	sg_EXShiftSettingItem[ITEM_EXSHIFT_CONTROL].itemProperty.itemPropertyCmb.content= "[Off,On]";
	sg_EXShiftSettingItem[ITEM_EXSHIFT_CONTROL].itemProperty.itemPropertyCmb.sel = sg_EXShiftPara.EXShiftControlSwitch;
	sg_EXShiftSettingItem[ITEM_EXSHIFT_CONTROL].itemCallback.cmbCallback.CmbChange= app_exshift_setting_change_callback;
	sg_EXShiftSettingItem[ITEM_EXSHIFT_CONTROL].itemStatus = ITEM_NORMAL;
}

static void app_exshift_setting_time_item_init(void)
{
	sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemTitle = "EX-Shift Time";
	sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemType = ITEM_CHOICE;
	sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemProperty.itemPropertyCmb.content= "[0s,0.5s,1s,1.5s,2s,2.5s,3s,3.5s,4s,4.5s,5s]";
	sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemProperty.itemPropertyCmb.sel = sg_EXShiftPara.EXShiftTime;
	sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemCallback.cmbCallback.CmbChange= NULL;

	if(_DISABLE == sg_EXShiftPara.EXShiftControlSwitch)
		sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemStatus = ITEM_DISABLE;
	else
		sg_EXShiftSettingItem[ITEM_EXSHIFT_TIME].itemStatus = ITEM_NORMAL;
}


void app_exshift_menu_exec(void)
{

	memset(&sg_EXShiftSettingOpt, 0 ,sizeof(sg_EXShiftSettingOpt));

	app_exshift_setting_para_get();

	sg_EXShiftSettingOpt.menuTitle = STR_ID_EXSHIFT_SET;
	sg_EXShiftSettingOpt.titleImage = "s_title_left_utility";
	sg_EXShiftSettingOpt.itemNum = ITEM_EXSHIFT_TOTAL;
	sg_EXShiftSettingOpt.timeDisplay = TIP_HIDE;
	sg_EXShiftSettingOpt.item = sg_EXShiftSettingItem;

	app_exshift_setting_switch_item_init();
	app_exshift_setting_time_item_init();

	sg_EXShiftSettingOpt.exit = app_exshift_setting_exit_callback;

	app_system_set_create(&sg_EXShiftSettingOpt);
}
#endif


