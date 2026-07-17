#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "sys_setting_manage.h"

enum OSD_ITEM_NAME
{
	ITEM_OSD_TRANS = 0,
	ITEM_OSD_BRIGHT,
	ITEM_OSD_SATURATION,
	ITEM_OSD_CONTRAST,
#if APP_SHARPNESS_SUPPORT
	ITEM_OSD_SHARPNESS,
#endif
#if APP_HUE_SUPPORT
	ITEM_OSD_HUE,
#endif
	ITEM_OSD_BAR_TIMEOUT,
	ITEM_OSD_TOTAL
};

typedef enum
{
	OSD_TRANS_40 = 0,
	OSD_TRANS_50,
	OSD_TRANS_60,
	OSD_TRANS_70,
	OSD_TRANS_80,
	OSD_TRANS_90,
	OSD_TRANS_100
}OsdTransSel;

typedef enum
{
	OSD_TIMEOUT_3S,
	OSD_TIMEOUT_5S,
	OSD_TIMEOUT_8S
}OsdTimeOutSel;

typedef struct
{
	int osd_tran;
    VideoColor last_para;
	int bar_timeout;
}SysOsdPara;

static SystemSettingOpt s_osd_setting_opt;
static SystemSettingItem s_osd_item[ITEM_OSD_TOTAL];
static SysOsdPara s_sys_osd_para;

static int app_osd_color_to_sel(int color)
{
	int ret;
	if(color < 20)
      color = 20;
	ret = color/10-2;
	return ret;
}

static int app_sel_to_color(int sel)
{
	int ret;
	ret = sel*10+20;
	return ret;
}

static int app_contrast_to_sel(int contrast)
{
	int ret;
	ret = (contrast - 10) / 9;

	ret = app_osd_color_to_sel(contrast);
	return ret;
}

static int app_sel_to_contrast(int sel)
{
	int ret;
	ret = sel * 9 + 10;
	ret = app_sel_to_color(sel);
	return ret;
}

static OsdTransSel app_osd_trans_to_sel(int trans)
{
	OsdTransSel sel = OSD_TRANS_40;

	switch(trans)
	{
		case OSD_TRANS_40PERCENT:
			sel = OSD_TRANS_100;
			break;

		case OSD_TRANS_50PERCENT:
			sel = OSD_TRANS_90;
			break;

		case OSD_TRANS_60PERCENT:
			sel = OSD_TRANS_80;
			break;

		case OSD_TRANS_70PERCENT:
			sel = OSD_TRANS_70;
			break;

		case OSD_TRANS_80PERCENT:
			sel = OSD_TRANS_60;
			break;

		case OSD_TRANS_90PERCENT:
			sel = OSD_TRANS_50;
			break;

		case OSD_TRANS_100PERCENT:
			sel = OSD_TRANS_40;
			break;

		default:
			break;
	}

	return sel;
}

static int app_sel_to_osd_trans(OsdTransSel sel)
{
	int trans = OSD_TRANS_100PERCENT;

	switch(sel)
	{
		case OSD_TRANS_40:
			trans = OSD_TRANS_100PERCENT;
			break;

		case OSD_TRANS_50:
			trans = OSD_TRANS_90PERCENT;
			break;

		case OSD_TRANS_60:
			trans = OSD_TRANS_80PERCENT;
			break;

		case OSD_TRANS_70:
			trans = OSD_TRANS_70PERCENT;
			break;

		case OSD_TRANS_80:
			trans = OSD_TRANS_60PERCENT;
			break;

		case OSD_TRANS_90:
			trans = OSD_TRANS_50PERCENT;
			break;

		case OSD_TRANS_100:
			trans = OSD_TRANS_40PERCENT;
			break;

		default:
			break;
	}

	return trans;
}

static OsdTimeOutSel app_widget_timeout_to_sel(int sec)
{
	OsdTimeOutSel sel = OSD_TIMEOUT_3S;

	switch(sec)
	{
		case 3:
			sel = OSD_TIMEOUT_3S;
			break;

		case 5:
			sel = OSD_TIMEOUT_5S;
			break;

		case 8:
			sel = OSD_TIMEOUT_8S;
			break;

		default:
			break;
	}

	return sel;
}

static int app_sel_to_widget_timeout(OsdTimeOutSel sel)
{
	int sec = 3;

	switch(sel)
	{
		case OSD_TIMEOUT_3S:
			sec = 3;
			break;

		case OSD_TIMEOUT_5S:
			sec = 5;
			break;

		case OSD_TIMEOUT_8S:
			sec = 8;
			break;

		default:
			break;
	}

	return sec;
}

#if APP_SHARPNESS_SUPPORT || APP_HUE_SUPPORT
static int app_osd_sharpness_hue_to_sel(int color)
{
    int ret;
    ret = color/10;
    return ret;
}

static int app_sel_to_sharpness_hue(int sel)
{
    int ret;
    ret = sel*10;
    return ret;
}
#endif
static void app_osd_system_para_get(void)
{
	GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &s_sys_osd_para.osd_tran, TRANS_LEVEL);
	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &s_sys_osd_para.last_para.brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(SATURATION_KEY, &s_sys_osd_para.last_para.saturation, SATURATION);
	GxBus_ConfigGetInt(CONTRAST_KEY, &s_sys_osd_para.last_para.contrast, CONTRAST);
	GxBus_ConfigGetInt(SHARPNESS_KEY, &s_sys_osd_para.last_para.sharpness, SHARPNESS);
	GxBus_ConfigGetInt(HUE_KEY, &s_sys_osd_para.last_para.hue, HUE);
	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &s_sys_osd_para.bar_timeout, INFOBAR_TIMEOUT);
}

static int app_osd_trans_change_callback(int sel)
{
	int trans = OSD_TRANS_100PERCENT;

	trans = app_sel_to_osd_trans(sel);
	GUI_SetInterface("osd_alpha_global", &trans);
    GxBus_ConfigSetInt(TRANS_LEVEL_KEY, trans);

	return EVENT_TRANSFER_KEEPON;
}

static int app_osd_change_saturation_cb(int sel)
{
	int cur_saturation = 0;

    cur_saturation = app_sel_to_color(sel);
	GxBus_ConfigSetInt(SATURATION_KEY, cur_saturation);
    if(s_sys_osd_para.last_para.saturation != cur_saturation)
    {
        s_sys_osd_para.last_para.saturation = cur_saturation;
        system_setting_set_screen_color(s_sys_osd_para.last_para.saturation, s_sys_osd_para.last_para.brightness,s_sys_osd_para.last_para.contrast,
                s_sys_osd_para.last_para.sharpness, s_sys_osd_para.last_para.hue, false);
    }

	return EVENT_TRANSFER_KEEPON;
}


static int app_osd_change_contrast_cb(int sel)
{
    int cur_contrast = 0;

    cur_contrast = app_sel_to_contrast(sel);
	GxBus_ConfigSetInt(CONTRAST_KEY, cur_contrast);
    if(s_sys_osd_para.last_para.contrast != cur_contrast)
    {
        s_sys_osd_para.last_para.contrast = cur_contrast;
        system_setting_set_screen_color(s_sys_osd_para.last_para.saturation, s_sys_osd_para.last_para.brightness, s_sys_osd_para.last_para.contrast,
                s_sys_osd_para.last_para.sharpness, s_sys_osd_para.last_para.hue, false);
    }

    return EVENT_TRANSFER_KEEPON;
}

static int app_osd_bar_time_change_callback(int sel)
{
    int cur_bar_timeout = 0;

    cur_bar_timeout = app_sel_to_widget_timeout(sel);
    GxBus_ConfigSetInt(INFOBAR_TIMEOUT_KEY, cur_bar_timeout);
    return EVENT_TRANSFER_KEEPON;
}

static int app_osd_change_brighness_cb(int sel)
{
	int cur_brightness = 0;

    cur_brightness = app_sel_to_color(sel);
	GxBus_ConfigSetInt(BRIGHTNESS_KEY, cur_brightness);
    if(s_sys_osd_para.last_para.brightness != cur_brightness)
    {
        s_sys_osd_para.last_para.brightness = cur_brightness;
        system_setting_set_screen_color(s_sys_osd_para.last_para.saturation, s_sys_osd_para.last_para.brightness, s_sys_osd_para.last_para.contrast,
                s_sys_osd_para.last_para.sharpness, s_sys_osd_para.last_para.hue, false);
    }

    return EVENT_TRANSFER_KEEPON;
}

#if APP_SHARPNESS_SUPPORT
static int app_osd_change_sharpness_cb(int sel)
{
    int cur_sharpness = 0;

    cur_sharpness = app_sel_to_sharpness_hue(sel);
    GxBus_ConfigSetInt(SHARPNESS_KEY, cur_sharpness);
    if(s_sys_osd_para.last_para.sharpness!= cur_sharpness)
    {
        s_sys_osd_para.last_para.sharpness= cur_sharpness;
        system_setting_set_screen_color(s_sys_osd_para.last_para.saturation, s_sys_osd_para.last_para.brightness, s_sys_osd_para.last_para.contrast,
                s_sys_osd_para.last_para.sharpness, s_sys_osd_para.last_para.hue, false);
    }

    return EVENT_TRANSFER_KEEPON;
}
#endif

#if APP_HUE_SUPPORT
static int app_osd_change_hue_cb(int sel)
{
    int cur_hue = 0;

    cur_hue = app_sel_to_sharpness_hue(sel);
    GxBus_ConfigSetInt(HUE_KEY, cur_hue);
    if(s_sys_osd_para.last_para.hue!= cur_hue)
    {
        s_sys_osd_para.last_para.hue= cur_hue;
        system_setting_set_screen_color(s_sys_osd_para.last_para.saturation, s_sys_osd_para.last_para.brightness, s_sys_osd_para.last_para.contrast,
                s_sys_osd_para.last_para.sharpness, s_sys_osd_para.last_para.hue, false);
    }

    return EVENT_TRANSFER_KEEPON;
}
#endif
static void app_osd_setting_trans_item_init(void)
{
	s_osd_item[ITEM_OSD_TRANS].itemTitle = STR_ID_TRAN;
	s_osd_item[ITEM_OSD_TRANS].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_OSD_TRANS].itemProperty.itemPropertyCmb.content= "[0%,10%,20%,30%,40%,50%,60%]";
	s_osd_item[ITEM_OSD_TRANS].itemProperty.itemPropertyCmb.sel = app_osd_trans_to_sel(s_sys_osd_para.osd_tran);
	s_osd_item[ITEM_OSD_TRANS].itemCallback.cmbCallback.CmbChange = app_osd_trans_change_callback;
	s_osd_item[ITEM_OSD_TRANS].itemStatus = ITEM_NORMAL;
}

static void app_osd_setting_bright_item_init(void)
{
	s_osd_item[ITEM_OSD_BRIGHT].itemTitle = STR_ID_BRIGHT;
	s_osd_item[ITEM_OSD_BRIGHT].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_OSD_BRIGHT].itemProperty.itemPropertyCmb.content= "[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_osd_item[ITEM_OSD_BRIGHT].itemProperty.itemPropertyCmb.sel = app_osd_color_to_sel(s_sys_osd_para.last_para.brightness);
	s_osd_item[ITEM_OSD_BRIGHT].itemCallback.cmbCallback.CmbChange = app_osd_change_brighness_cb;
#if CVBS_TEST_SUPPORT
	s_osd_item[ITEM_OSD_BRIGHT].itemStatus = ITEM_DISABLE;
#else
	s_osd_item[ITEM_OSD_BRIGHT].itemStatus = ITEM_NORMAL;
#endif
}

static void app_osd_setting_saturation_item_init(void)
{
	s_osd_item[ITEM_OSD_SATURATION].itemTitle = STR_ID_CHROMA;
	s_osd_item[ITEM_OSD_SATURATION].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_OSD_SATURATION].itemProperty.itemPropertyCmb.content= "[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_osd_item[ITEM_OSD_SATURATION].itemProperty.itemPropertyCmb.sel = app_osd_color_to_sel(s_sys_osd_para.last_para.saturation);
	s_osd_item[ITEM_OSD_SATURATION].itemCallback.cmbCallback.CmbChange = app_osd_change_saturation_cb;
#if CVBS_TEST_SUPPORT
	s_osd_item[ITEM_OSD_SATURATION].itemStatus = ITEM_DISABLE;
#else
	s_osd_item[ITEM_OSD_SATURATION].itemStatus = ITEM_NORMAL;
#endif
}

static void app_osd_setting_contrast_item_init(void)
{
	s_osd_item[ITEM_OSD_CONTRAST].itemTitle = STR_ID_CONTRAST;
	s_osd_item[ITEM_OSD_CONTRAST].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_OSD_CONTRAST].itemProperty.itemPropertyCmb.content="[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_osd_item[ITEM_OSD_CONTRAST].itemProperty.itemPropertyCmb.sel = app_contrast_to_sel(s_sys_osd_para.last_para.contrast);
	s_osd_item[ITEM_OSD_CONTRAST].itemCallback.cmbCallback.CmbChange = app_osd_change_contrast_cb;
#if CVBS_TEST_SUPPORT
	s_osd_item[ITEM_OSD_CONTRAST].itemStatus = ITEM_DISABLE;
#else
	s_osd_item[ITEM_OSD_CONTRAST].itemStatus = ITEM_NORMAL;
#endif
}

#if APP_SHARPNESS_SUPPORT
static void app_osd_setting_sharpness_item_init(void)
{
	s_osd_item[ITEM_OSD_SHARPNESS].itemTitle = STR_ID_SHARPNESS;
	s_osd_item[ITEM_OSD_SHARPNESS].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_OSD_SHARPNESS].itemProperty.itemPropertyCmb.content="[0%,10%,20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_osd_item[ITEM_OSD_SHARPNESS].itemProperty.itemPropertyCmb.sel = app_osd_sharpness_hue_to_sel(s_sys_osd_para.last_para.sharpness);
	s_osd_item[ITEM_OSD_SHARPNESS].itemCallback.cmbCallback.CmbChange = app_osd_change_sharpness_cb;
#if CVBS_TEST_SUPPORT
	s_osd_item[ITEM_OSD_SHARPNESS].itemStatus = ITEM_DISABLE;
#else
	s_osd_item[ITEM_OSD_SHARPNESS].itemStatus = ITEM_NORMAL;
#endif
}
#endif

#if APP_HUE_SUPPORT
static void app_osd_setting_hue_item_init(void)
{
	s_osd_item[ITEM_OSD_HUE].itemTitle = STR_ID_HUE;
	s_osd_item[ITEM_OSD_HUE].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_OSD_HUE].itemProperty.itemPropertyCmb.content="[0%,10%,20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_osd_item[ITEM_OSD_HUE].itemProperty.itemPropertyCmb.sel = app_osd_sharpness_hue_to_sel(s_sys_osd_para.last_para.hue);
	s_osd_item[ITEM_OSD_HUE].itemCallback.cmbCallback.CmbChange = app_osd_change_hue_cb;
#if CVBS_TEST_SUPPORT
	s_osd_item[ITEM_OSD_HUE].itemStatus = ITEM_DISABLE;
#else
	s_osd_item[ITEM_OSD_HUE].itemStatus = ITEM_NORMAL;
#endif
}
#endif
static void app_osd_setting_bar_timeout_item_init(void)
{
	s_osd_item[ITEM_OSD_BAR_TIMEOUT].itemTitle = STR_ID_OSD_TIMEOUT;
	s_osd_item[ITEM_OSD_BAR_TIMEOUT].itemType = ITEM_CHOICE;
	s_osd_item[ITEM_OSD_BAR_TIMEOUT].itemProperty.itemPropertyCmb.content= "[3s,5s,8s]";
	s_osd_item[ITEM_OSD_BAR_TIMEOUT].itemProperty.itemPropertyCmb.sel = app_widget_timeout_to_sel(s_sys_osd_para.bar_timeout);
	s_osd_item[ITEM_OSD_BAR_TIMEOUT].itemCallback.cmbCallback.CmbChange = app_osd_bar_time_change_callback;
	s_osd_item[ITEM_OSD_BAR_TIMEOUT].itemStatus = ITEM_NORMAL;
}

void app_t2_osd_setting_menu_exec(void)
{
	memset(&s_osd_setting_opt, 0 ,sizeof(s_osd_setting_opt));

	app_osd_system_para_get();

	s_osd_setting_opt.menuTitle = STR_ID_OSD_SET;
	s_osd_setting_opt.itemNum = ITEM_OSD_TOTAL;
    s_osd_setting_opt.topTipImgDisplay = TIP_HIDE;
	s_osd_setting_opt.bottmTipDisplay= TIP_HIDE;
	s_osd_setting_opt.timeDisplay = TIP_HIDE;
	s_osd_setting_opt.item = s_osd_item;
	app_osd_setting_trans_item_init();
    app_osd_setting_bright_item_init();
    app_osd_setting_saturation_item_init();
    app_osd_setting_contrast_item_init();
#if APP_SHARPNESS_SUPPORT
    app_osd_setting_sharpness_item_init();
#endif
#if APP_HUE_SUPPORT
    app_osd_setting_hue_item_init();
#endif
    app_osd_setting_bar_timeout_item_init();
	s_osd_setting_opt.exit = NULL;

	app_system_set_create(&s_osd_setting_opt);
}

