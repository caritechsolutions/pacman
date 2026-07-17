#include "app.h"
#if FM_SUPPORT
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_fm.h"

enum FM_ITEM_NAME
{
	ITEM_BAND = 0,
	ITEM_ANT_GAIN,
	ITEM_FM_TOTAL
};

typedef enum
{
	FM_BAND_CHINA = 0,
	FM_BAND_AMERICA,
	FM_BAND_JAPAN,
	FM_BAND_RUSSIA,
	FM_BAND_MAX
} FmBandSel;

typedef enum
{
	FM_ANT_GAIN_NORMAL = 0,
	FM_ANT_GAIN_LOCAL,
	FM_ANT_GAIN_DX,
	FM_ANT_GAIN_MAX
} FmAntGainSel;

typedef struct
{
	FmBandSel band;
	FmAntGainSel ant_gain;
} SysFmPara;

static SystemSettingOpt s_fm_setting_opt;
static SystemSettingItem s_fm_item[ITEM_FM_TOTAL];
static SysFmPara s_sys_fm_para;

static void app_fm_system_para_get(void)
{
	int init_value = 0;

	GxBus_ConfigGetInt(BAND_KEY, &init_value, BAND_VALUE);
	if (init_value >= FM_BAND_CHINA && init_value < FM_BAND_MAX)
	{
	    s_sys_fm_para.band = init_value;
	}
	else
	{
	    s_sys_fm_para.band = FM_BAND_CHINA;
	}

	GxBus_ConfigGetInt(ANT_GAIN_KEY, &init_value, ANT_GAIN_VALUE);
	if (init_value >= FM_ANT_GAIN_NORMAL && init_value < FM_ANT_GAIN_MAX)
	{
	    s_sys_fm_para.ant_gain = init_value;
	}
	else
	{
	    s_sys_fm_para.ant_gain = FM_ANT_GAIN_NORMAL;
	}

}

static void app_fm_result_para_get(SysFmPara *ret_para)
{
    ret_para->band = s_fm_item[ITEM_BAND].itemProperty.itemPropertyCmb.sel;
    ret_para->ant_gain = s_fm_item[ITEM_ANT_GAIN].itemProperty.itemPropertyCmb.sel;
}

static void app_fm_setting_band_item_init(void)
{
	s_fm_item[ITEM_BAND].itemTitle = STR_ID_FM_BAND;
	s_fm_item[ITEM_BAND].itemType = ITEM_CHOICE;
	s_fm_item[ITEM_BAND].itemProperty.itemPropertyCmb.content = "[87-108 MHz,87.5-108 MHz,76-108 MHz,64-108 MHz]";
	s_fm_item[ITEM_BAND].itemProperty.itemPropertyCmb.sel = s_sys_fm_para.band;
	s_fm_item[ITEM_BAND].itemCallback.cmbCallback.CmbChange = NULL;
	s_fm_item[ITEM_BAND].itemStatus = ITEM_NORMAL;
}

static void app_fm_setting_ant_gain_item_init(void)
{
	s_fm_item[ITEM_ANT_GAIN].itemTitle = STR_ID_FM_ANT_GAIN;
	s_fm_item[ITEM_ANT_GAIN].itemType = ITEM_CHOICE;
	s_fm_item[ITEM_ANT_GAIN].itemProperty.itemPropertyCmb.content = "[Normal,Local,DX]";
	s_fm_item[ITEM_ANT_GAIN].itemProperty.itemPropertyCmb.sel = s_sys_fm_para.ant_gain;
	s_fm_item[ITEM_ANT_GAIN].itemCallback.cmbCallback.CmbChange = NULL;
	s_fm_item[ITEM_ANT_GAIN].itemStatus = ITEM_NORMAL;
}

static int app_fm_set_sensitivity(int value)
{
    GxFeFMSensitivity params = {0};
    int ret = -1;
    int handle = -1;

    handle = app_fm_get_handle();
    if(handle < 0)
        return -1;

    if(value == 0)
        params.sensitivity = 0x60;
    else if(value == 1)
        params.sensitivity = 0x40;
    if(value == 2)
        params.sensitivity = 0x80;

    ret = GxFeSetFMSensitivity(handle, &params);
    return ret;
}

static int app_fm_setting_para_save(SysFmPara *ret_para)
{
    int set_value = 0;

    if (ret_para->band > FM_BAND_MAX)
    {
        set_value = 0;
    }
    else
    {
        set_value = ret_para->band;
    }
    if (set_value != s_sys_fm_para.band)
        GxBus_ConfigSetInt(BAND_KEY, set_value);

    if (ret_para->ant_gain > FM_ANT_GAIN_MAX)
    {
        set_value = 0;
    }
    else
    {
        set_value = ret_para->ant_gain;
    }
    if (set_value != s_sys_fm_para.ant_gain)
    {
        app_fm_set_sensitivity(set_value);
        GxBus_ConfigSetInt(ANT_GAIN_KEY, set_value);
    }
    return 0;
}

static int _fm_setting_save_pop_cb(PopDlgRet ret)
{
    SysFmPara para_ret = {0};

    if(POP_VAL_OK == ret)
    {
        app_fm_result_para_get(&para_ret);
        app_fm_setting_para_save(&para_ret);
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static bool app_fm_setting_para_change(SysFmPara *ret_para)
{
	bool ret = FALSE;

	if(s_sys_fm_para.band != ret_para->band)
	{
		ret = TRUE;
	}
	if(s_sys_fm_para.ant_gain != ret_para->ant_gain)
	{
		ret = TRUE;
	}
	return ret;
}

static bool _fm_setting_check_cb(void)
{
    bool ret = false;
    SysFmPara para_ret = {0};

    app_fm_result_para_get(&para_ret);
    ret = app_fm_setting_para_change(&para_ret);

    return ret;
}

static int app_fm_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(padmux_check(34, 1) != 0)
        padmux_set(34, 1);

    if(exit_type == EXIT_ABANDON)
    {
	//do nothing
    }
    else
    {
        app_system_set_callback_handler(_fm_setting_check_cb, _fm_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }
    return ret;
}

void app_fm_setting_menu_exec(void)
{
	memset(&s_fm_setting_opt, 0 ,sizeof(s_fm_setting_opt));

	if(padmux_check(34, 4) != 0)
		padmux_set(34, 4);

	app_fm_system_para_get();

	s_fm_setting_opt.menuTitle = STR_ID_FM_SETTING;
	s_fm_setting_opt.itemNum = ITEM_FM_TOTAL;
	s_fm_setting_opt.topTipImgDisplay = TIP_HIDE;
	s_fm_setting_opt.bottmTipDisplay = TIP_HIDE;
	s_fm_setting_opt.timeDisplay = TIP_HIDE;
	s_fm_setting_opt.item = s_fm_item;

	app_fm_setting_band_item_init();
	app_fm_setting_ant_gain_item_init();

	s_fm_setting_opt.exit = app_fm_setting_exit_callback;

	app_system_set_create(&s_fm_setting_opt);
}

void app_fm_get_band_range(uint32_t* min, uint32_t* max)
{
    int init_value;

    *max = 108000;
    GxBus_ConfigGetInt(BAND_KEY, &init_value, BAND_VALUE);
    switch(init_value)
    {
        case FM_BAND_CHINA:
            *min = 87000;
            break;
        case FM_BAND_AMERICA:
            *min = 87500;
            break;
        case FM_BAND_JAPAN:
            *min = 76000;
            break;
        case FM_BAND_RUSSIA:
            *min = 64000;
            break;
        default:
            *min = 87000;
            break;
    }
}


#endif
