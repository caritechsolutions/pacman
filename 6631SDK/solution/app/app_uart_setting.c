#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"

#if (UART_SUPPORT > 0)
enum TIME_AV_NAME
{
	ITEM_UART_CONTROL = 0,
	ITEM_SERIAL_BAUD_RATE,
	ITEM_UART_TOTAL
};
typedef enum
{
	UART_PROTOCOL_DISABLE = 0,
	UART_PROTOCOL_ENABLE ,
}UART_CONTROL;

typedef enum
{
	LOWER_LIMIT = 0,// if the baud is 115200, the ouput is 112500
	UPPER_LIMIT,	// if the baud is 115200, the ouput is 120550
}BAUD_RATE_ADJUST;

typedef struct
{
	UART_CONTROL     UartControlSwitch;
	BAUD_RATE_ADJUST BaudRateAdjust;// only support two level to adjust
}UARTPara;

static SystemSettingOpt sg_UartSettingOpt;
static SystemSettingItem sg_UartSettingItem[ITEM_UART_TOTAL];
static UARTPara sg_UartPara;


static void app_uart_setting_para_get(void)
{
	int init_value = 0;

	GxBus_ConfigGetInt(UART_MODE_KEY, &init_value, UART_PROTOCOL_SWITCH);
	sg_UartPara.UartControlSwitch = init_value;

	GxBus_ConfigGetInt(UART_BAUD_KEY, &init_value, UART_BAUD_RATE_LIMIT);
	sg_UartPara.BaudRateAdjust = init_value;
}

static void app_uart_setting_result_para_get(UARTPara *ret_para)
{
    ret_para->UartControlSwitch = sg_UartSettingItem[ITEM_UART_CONTROL].itemProperty.itemPropertyCmb.sel;
    ret_para->BaudRateAdjust = sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemProperty.itemPropertyCmb.sel;
}

static bool app_uart_setting_para_change(UARTPara *ret_para)
{
	bool ret = FALSE;

	if(sg_UartPara.UartControlSwitch != ret_para->UartControlSwitch)
	{
		ret = TRUE;
	}

	if(sg_UartPara.BaudRateAdjust != ret_para->BaudRateAdjust)
	{
		ret = TRUE;
	}

	return ret;
}

static int app_uart_setting_para_save(UARTPara *ret_para)
{
	int init_value = 0;

    init_value = ret_para->UartControlSwitch;
    GxBus_ConfigSetInt(UART_MODE_KEY,init_value);

    init_value = ret_para->BaudRateAdjust;
    GxBus_ConfigSetInt(UART_BAUD_KEY,init_value);

	if(UART_PROTOCOL_DISABLE == ret_para->UartControlSwitch)
	{
        // TODO: 20160302
	}
	else
	{
        // TODO: 20160302
	}

	return 0;
}

static int _uart_setting_save_pop_cb(PopDlgRet ret)
{
    UARTPara para_ret;

    if(POP_VAL_OK == ret)
    {
        app_uart_setting_result_para_get(&para_ret);
        app_uart_setting_para_save(&para_ret);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static bool _uart_setting_check_cb(void)
{
    bool ret = false;
	UARTPara para_ret;

    app_uart_setting_result_para_get(&para_ret);
    ret = app_uart_setting_para_change(&para_ret);

    return ret;
}

static int app_uart_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        ;
    }
    else
    {
        app_system_set_callback_handler(_uart_setting_check_cb, _uart_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }

    return ret;
}


static void app_uart_setting_uart_switch_item_init(void)
{
	sg_UartSettingItem[ITEM_UART_CONTROL].itemTitle = "Uart Protocol Enable";
	sg_UartSettingItem[ITEM_UART_CONTROL].itemType = ITEM_CHOICE;
	sg_UartSettingItem[ITEM_UART_CONTROL].itemProperty.itemPropertyCmb.content= "[Off,On]";
	sg_UartSettingItem[ITEM_UART_CONTROL].itemProperty.itemPropertyCmb.sel = sg_UartPara.UartControlSwitch;
	sg_UartSettingItem[ITEM_UART_CONTROL].itemCallback.cmbCallback.CmbChange= NULL;
	sg_UartSettingItem[ITEM_UART_CONTROL].itemStatus = ITEM_NORMAL;
}

static void app_uart_setting_baud_limit_item_init(void)
{
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemTitle = STR_ID_BAUD_RATE_ADJUST;
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemType = ITEM_CHOICE;
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemProperty.itemPropertyCmb.content= "[Lower Limit,Upper Limit]";
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemProperty.itemPropertyCmb.sel = sg_UartPara.BaudRateAdjust;
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemCallback.cmbCallback.CmbChange= NULL;
	sg_UartSettingItem[ITEM_SERIAL_BAUD_RATE].itemStatus = ITEM_NORMAL;
}


void app_uart_menu_exec(void)
{

	memset(&sg_UartSettingOpt, 0 ,sizeof(sg_UartSettingOpt));

	app_uart_setting_para_get();

	sg_UartSettingOpt.menuTitle = STR_ID_UART;
	sg_UartSettingOpt.titleImage = "s_title_left_utility";
	sg_UartSettingOpt.itemNum = ITEM_UART_TOTAL;
	sg_UartSettingOpt.timeDisplay = TIP_HIDE;
	sg_UartSettingOpt.item = sg_UartSettingItem;

	app_uart_setting_uart_switch_item_init();
	app_uart_setting_baud_limit_item_init();

	sg_UartSettingOpt.exit = app_uart_setting_exit_callback;

	app_system_set_create(&sg_UartSettingOpt);
}
#endif


