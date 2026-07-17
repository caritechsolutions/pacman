#include "app.h"
#if PANEL_SETTING_SUPPORT
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_utility.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_book.h"

enum PANEL_ITEM_NAME
{
	ITEM_BRIGHTNESS = 0,
	ITEM_DISPLAY,
#if STANDBY_SHOW_TIME_SHPPORT
    ITEM_STANDBY_SHOW_TIME,
#endif
	ITEM_PANEL_TOTAL
};

typedef enum
{
    PANEL_LED_BRIGHTNESS_LOW = 0,
    PANEL_LED_BRIGHTNESS_MOD,
    PANEL_LED_BRIGHTNESS_HIG,
    PANEL_LED_BRIGHTNESS_MAX,
} LedBrightnessSel;

typedef enum
{
    LED_DISPLAY_CHNO = 0,
    LED_DISPLAY_TIME,
    LED_DISPLAY_MAX,
} Led_State_t;

#if STANDBY_SHOW_TIME_SHPPORT
typedef enum
{
    TIME_OFF = 0,
    TIME_ON,
    TIME_MAX,
} StandbyShowTimeSel;
#endif

typedef struct
{
   LedBrightnessSel brightness;
   Led_State_t display;
#if STANDBY_SHOW_TIME_SHPPORT
   StandbyShowTimeSel standby_time;
#endif
} SysPanelPara;

static SystemSettingOpt s_panel_setting_opt;
static SystemSettingItem s_panel_item[ITEM_PANEL_TOTAL];
static SysPanelPara s_sys_panel_para;

static void app_panel_system_para_get(void)
{
    int init_value = 0;
    GxBus_ConfigGetInt(PANEL_LED_BRIGHTNESS_KEY, &init_value, PANEL_LED_BRIGHTNESS);
    if (init_value >= PANEL_LED_BRIGHTNESS_LOW && init_value < PANEL_LED_BRIGHTNESS_MAX)
    {
        s_sys_panel_para.brightness = init_value;
    }
    else
    {
        s_sys_panel_para.brightness = PANEL_LED_BRIGHTNESS_LOW;
    }

    GxBus_ConfigGetInt(PANEL_LED_DISPLAY_KEY, &init_value, PANEL_LED_DISPLAY);
    if (init_value >= LED_DISPLAY_CHNO && init_value < LED_DISPLAY_MAX)
    {
        s_sys_panel_para.display = init_value;
    }
    else
    {
        s_sys_panel_para.display = LED_DISPLAY_CHNO;
    }

#if STANDBY_SHOW_TIME_SHPPORT
    GxBus_ConfigGetInt(PANEL_LED_STANDBY_TIME_KEY, &init_value, PANEL_STANDBY_TIME);
    if (init_value >= TIME_OFF && init_value < TIME_MAX)
    {
        s_sys_panel_para.standby_time = init_value;
    }
    else
    {
        s_sys_panel_para.standby_time = TIME_ON;
    }
#endif
}

static void app_panel_result_para_get(SysPanelPara *ret_para)
{
    ret_para->brightness = s_panel_item[ITEM_BRIGHTNESS].itemProperty.itemPropertyCmb.sel;
    ret_para->display = s_panel_item[ITEM_DISPLAY].itemProperty.itemPropertyCmb.sel;
#if STANDBY_SHOW_TIME_SHPPORT
    ret_para->standby_time = s_panel_item[ITEM_STANDBY_SHOW_TIME].itemProperty.itemPropertyCmb.sel;
#endif
}

static int _brigthness_change(int sel)
{
    if(sel >= PANEL_LED_BRIGHTNESS_LOW && sel < PANEL_LED_BRIGHTNESS_MAX)
    {
        app_panel_ioctl_handle(PANEL_SHOW_BRIGHTNESS, &sel);
    }
    return 0;
}

static void app_panel_setting_brightness_item_init(void)
{
    s_panel_item[ITEM_BRIGHTNESS].itemTitle = STR_ID_LED_BRIGHTNESS;
	s_panel_item[ITEM_BRIGHTNESS].itemType = ITEM_CHOICE;
	s_panel_item[ITEM_BRIGHTNESS].itemProperty.itemPropertyCmb.content = "[Low,Middle,Full]";
	s_panel_item[ITEM_BRIGHTNESS].itemProperty.itemPropertyCmb.sel = s_sys_panel_para.brightness;
	s_panel_item[ITEM_BRIGHTNESS].itemCallback.cmbCallback.CmbChange = _brigthness_change;
	s_panel_item[ITEM_BRIGHTNESS].itemStatus = ITEM_NORMAL;
}

static void app_panel_setting_display_item_init(void)
{
    s_panel_item[ITEM_DISPLAY].itemTitle = STR_ID_LED_DISPLAY;
	s_panel_item[ITEM_DISPLAY].itemType = ITEM_CHOICE;
	s_panel_item[ITEM_DISPLAY].itemProperty.itemPropertyCmb.content = "[CH NO.,Time]";
	s_panel_item[ITEM_DISPLAY].itemProperty.itemPropertyCmb.sel = s_sys_panel_para.display;
	s_panel_item[ITEM_DISPLAY].itemCallback.cmbCallback.CmbChange = NULL;
	s_panel_item[ITEM_DISPLAY].itemStatus = ITEM_NORMAL;
}

#if STANDBY_SHOW_TIME_SHPPORT
static void app_panel_setting_standby_item_init(void)
{
    s_panel_item[ITEM_STANDBY_SHOW_TIME].itemTitle = STR_ID_LED_STANDBY;
	s_panel_item[ITEM_STANDBY_SHOW_TIME].itemType = ITEM_CHOICE;
	s_panel_item[ITEM_STANDBY_SHOW_TIME].itemProperty.itemPropertyCmb.content = "[Off,On]";
	s_panel_item[ITEM_STANDBY_SHOW_TIME].itemProperty.itemPropertyCmb.sel = s_sys_panel_para.standby_time;
	s_panel_item[ITEM_STANDBY_SHOW_TIME].itemCallback.cmbCallback.CmbChange = NULL;
	s_panel_item[ITEM_STANDBY_SHOW_TIME].itemStatus = ITEM_NORMAL;
}
#endif

static int app_panel_setting_para_save(SysPanelPara *ret_para)
{
    int set_value = 0;

    if (ret_para->brightness > PANEL_LED_BRIGHTNESS_MAX)
    {
        set_value = 0;
    }
    else
    {
        set_value = ret_para->brightness;
    }
    if (set_value != s_sys_panel_para.brightness)
        GxBus_ConfigSetInt(PANEL_LED_BRIGHTNESS_KEY, set_value);
    app_panel_ioctl_handle(PANEL_SHOW_BRIGHTNESS, &set_value);

    if (ret_para->display > LED_DISPLAY_MAX)
    {
        set_value = 0;
    }
    else
    {
        set_value = ret_para->display;
    }
    if (set_value != s_sys_panel_para.display)
        GxBus_ConfigSetInt(PANEL_LED_DISPLAY_KEY, set_value);
#if PANEL_SETTING_SUPPORT
    if(LED_DISPLAY_CHNO == set_value)
        app_panel_show_prog_num(PANEL_CLEAN_PROG_NUM);
    else
        app_panel_show_prog_num(PANEL_DISPLAY_TIME);
#endif

#if STANDBY_SHOW_TIME_SHPPORT
    if (ret_para->display > LED_DISPLAY_MAX)
    {
        set_value = 0;
    }
    else
    {
        set_value = ret_para->standby_time;
    }
    if (set_value != s_sys_panel_para.standby_time)
        GxBus_ConfigSetInt(PANEL_LED_STANDBY_TIME_KEY, set_value);
#endif
    return 0;
}

static int _panel_setting_save_pop_cb(PopDlgRet ret)
{
    SysPanelPara para_ret = {0};

    if(POP_VAL_OK == ret)
    {
        app_panel_result_para_get(&para_ret);
        app_panel_setting_para_save(&para_ret);
    }
    else
    {
        app_panel_ioctl_handle(PANEL_SHOW_BRIGHTNESS, &s_sys_panel_para.brightness);
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static bool app_panel_setting_para_change(SysPanelPara *ret_para)
{
	bool ret = FALSE;

	if(s_sys_panel_para.brightness != ret_para->brightness)
	{
		ret = TRUE;
	}
    if(s_sys_panel_para.display != ret_para->display)
	{
		ret = TRUE;
	}
#if STANDBY_SHOW_TIME_SHPPORT
    if(s_sys_panel_para.standby_time != ret_para->standby_time)
	{
		ret = TRUE;
	}
#endif
	return ret;
}

static bool _panel_setting_check_cb(void)
{
    bool ret = false;
    SysPanelPara para_ret = {0};

    app_panel_result_para_get(&para_ret);
    ret = app_panel_setting_para_change(&para_ret);

    return ret;
}

static int app_panel_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;
    SysPanelPara para_ret = {0};

    if(exit_type == EXIT_ABANDON)
    {
        app_panel_result_para_get(&para_ret);
        app_panel_setting_para_save(&para_ret);
    }
    else
    {
        app_system_set_callback_handler(_panel_setting_check_cb, _panel_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }
    return ret;
}

void app_panel_setting_menu_exec(void)
{
	memset(&s_panel_setting_opt, 0 ,sizeof(s_panel_setting_opt));
	app_panel_system_para_get();

	s_panel_setting_opt.menuTitle = STR_ID_PANEL_SET;
	s_panel_setting_opt.itemNum = ITEM_PANEL_TOTAL;
	s_panel_setting_opt.timeDisplay = TIP_HIDE;
	s_panel_setting_opt.item = s_panel_item;

	app_panel_setting_brightness_item_init();
	app_panel_setting_display_item_init();
#if STANDBY_SHOW_TIME_SHPPORT
	app_panel_setting_standby_item_init();
#endif
	s_panel_setting_opt.exit = app_panel_setting_exit_callback;
	app_system_set_create(&s_panel_setting_opt);
}

void app_dvbt_panel_setting_menu_exec(void)
{
	memset(&s_panel_setting_opt, 0 ,sizeof(s_panel_setting_opt));
	app_panel_system_para_get();

	s_panel_setting_opt.menuTitle = STR_ID_PANEL_SET;
	s_panel_setting_opt.itemNum = ITEM_PANEL_TOTAL;
    s_panel_setting_opt.topTipImgDisplay = TIP_HIDE;
    s_panel_setting_opt.bottmTipDisplay = TIP_HIDE;
	s_panel_setting_opt.timeDisplay = TIP_HIDE;
	s_panel_setting_opt.item = s_panel_item;

	app_panel_setting_brightness_item_init();
	app_panel_setting_display_item_init();
#if STANDBY_SHOW_TIME_SHPPORT
	app_panel_setting_standby_item_init();
#endif
	s_panel_setting_opt.exit = app_panel_setting_exit_callback;
	app_system_set_create(&s_panel_setting_opt);
}

#endif
