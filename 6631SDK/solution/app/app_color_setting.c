#include "app.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "sys_setting_manage.h"
#include "app_pop.h"

enum COLOR_ITEM_NAME
{
	ITEM_BRIGHT = 0,
	ITEM_SATURATION,
	ITEM_CONTRAST,
#if APP_SHARPNESS_SUPPORT
	ITEM_SHARPNESS,
#endif
#if APP_HUE_SUPPORT
	ITEM_HUE,
#endif
	ITEM_COLOR_TOTAL,
};

static SystemSettingOpt s_color_setting_opt;
static SystemSettingItem s_color_item[ITEM_COLOR_TOTAL];
static VideoColor s_sys_color_para;
static VideoColor last_para;
static int s_color_menu_flag = 0;

#if WEBAPI_SUPPORT
static char* color_bright_str[] = {
	"20%",
	"30%",
	"40%",
	"50%",
	"60%",
	"70%",
	"80%",
	"90%",
	"100%"
};
#define COLOR_BRIGHT_TOTAL (sizeof(color_bright_str)/sizeof(char *))
static char* color_chroma_str[] = {
	"20%",
	"30%",
	"40%",
	"50%",
	"60%",
	"70%",
	"80%",
	"90%",
	"100%"
};
#define COLOR_CHROMA_TOTAL (sizeof(color_chroma_str)/sizeof(char *))
static char* color_contrast_str[] = {
	"20%",
	"30%",
	"40%",
	"50%",
	"60%",
	"70%",
	"80%",
	"90%",
	"100%"
};
#define COLOR_CONTRAST_TOTAL (sizeof(color_contrast_str)/sizeof(char *))
#endif

static int app_color_to_sel(int color)
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

////////// sel[0,10] <==> contrast[10, 100] ////////
static int app_contrast_to_sel(int contrast)
{
	int ret;
	ret = (contrast - 10) / 9;

	ret = app_color_to_sel(contrast);
	return ret;
}

static int app_sel_to_contrast(int sel)
{
	int ret;
	ret = sel * 9 + 10;
	ret = app_sel_to_color(sel);
	return ret;
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

static void app_color_system_para_get(void)
{
	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &s_sys_color_para.brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(SATURATION_KEY, &s_sys_color_para.saturation, SATURATION);
	GxBus_ConfigGetInt(CONTRAST_KEY, &s_sys_color_para.contrast, CONTRAST);
	GxBus_ConfigGetInt(SHARPNESS_KEY, &s_sys_color_para.sharpness, SHARPNESS);
	GxBus_ConfigGetInt(HUE_KEY, &s_sys_color_para.hue, HUE);
}

static void app_color_result_para_get(VideoColor *ret_para)
{
    ret_para->brightness = app_sel_to_color(s_color_item[ITEM_BRIGHT].itemProperty.itemPropertyCmb.sel);
    ret_para->saturation = app_sel_to_color(s_color_item[ITEM_SATURATION].itemProperty.itemPropertyCmb.sel);
    ret_para->contrast = app_sel_to_contrast(s_color_item[ITEM_CONTRAST].itemProperty.itemPropertyCmb.sel);
#if APP_SHARPNESS_SUPPORT
    ret_para->sharpness = app_sel_to_sharpness_hue(s_color_item[ITEM_SHARPNESS].itemProperty.itemPropertyCmb.sel);
#endif
#if APP_HUE_SUPPORT
    ret_para->hue = app_sel_to_sharpness_hue(s_color_item[ITEM_HUE].itemProperty.itemPropertyCmb.sel);
#endif
}

static bool app_color_setting_para_change(VideoColor *ret_para)
{
	bool ret = FALSE;

	if(s_sys_color_para.brightness != ret_para->brightness)
	{
		ret = TRUE;
	}

	if(s_sys_color_para.saturation != ret_para->saturation)
	{
		ret = TRUE;
	}

	if(s_sys_color_para.contrast!= ret_para->contrast)
	{
		ret = TRUE;
	}
#if APP_SHARPNESS_SUPPORT
	if(s_sys_color_para.sharpness != ret_para->sharpness)
	{
		ret = TRUE;
	}
#endif
#if APP_HUE_SUPPORT
	if(s_sys_color_para.hue!= ret_para->hue)
	{
		ret = TRUE;
	}
#endif
	return ret;
}

static int app_color_setting_para_save(VideoColor *ret_para)
{
	GxBus_ConfigSetInt(BRIGHTNESS_KEY, ret_para->brightness);
	GxBus_ConfigSetInt(SATURATION_KEY, ret_para->saturation);
	GxBus_ConfigSetInt(CONTRAST_KEY, ret_para->contrast);
#if APP_SHARPNESS_SUPPORT
	GxBus_ConfigSetInt(SHARPNESS_KEY, ret_para->sharpness);
#endif
#if APP_HUE_SUPPORT
	GxBus_ConfigSetInt(HUE_KEY, ret_para->hue);
#endif
	return 0;
}

static int _color_setting_save_pop_cb(PopDlgRet ret)
{
	VideoColor para_ret;

    if(POP_VAL_OK == ret)
    {
        app_color_result_para_get(&para_ret);
        app_color_setting_para_save(&para_ret);
    }
    else
    {
        system_setting_set_screen_color(s_sys_color_para.saturation ,s_sys_color_para.brightness, s_sys_color_para.contrast,s_sys_color_para.sharpness, s_sys_color_para.hue, false);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    s_color_menu_flag = 0;

    return 0;
}

static bool _color_setting_check_cb(void)
{
    bool ret = false;
    VideoColor para_ret;

    app_color_result_para_get(&para_ret);
    ret = app_color_setting_para_change(&para_ret);

    return ret;
}

static int app_color_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        system_setting_set_screen_color(s_sys_color_para.saturation ,s_sys_color_para.brightness, s_sys_color_para.contrast, s_sys_color_para.sharpness, s_sys_color_para.hue, false);
        s_color_menu_flag = 0;
    }
    else
    {
        if(false == app_system_set_callback_handler(_color_setting_check_cb, _color_setting_save_pop_cb))
        {
            s_color_menu_flag = 0;
        }
        ret = EXIT_RET_POP;
    }

	return ret;
}

static int app_color_change_brighness_cb(int sel)
{
	int cur_brightness = 0;

	app_system_set_item_data_update(ITEM_BRIGHT);
    cur_brightness = app_sel_to_color(s_color_item[ITEM_BRIGHT].itemProperty.itemPropertyCmb.sel);
    if(last_para.brightness != cur_brightness)
    {
        last_para.brightness = cur_brightness;
        system_setting_set_screen_color(last_para.saturation ,last_para.brightness, last_para.contrast ,last_para.sharpness, last_para.hue, false);
    }

    return EVENT_TRANSFER_KEEPON;
}
static int app_color_change_saturation_cb(int sel)
{
	int cur_saturation = 0;

	app_system_set_item_data_update(ITEM_SATURATION);
    cur_saturation = app_sel_to_color(s_color_item[ITEM_SATURATION].itemProperty.itemPropertyCmb.sel);
    if(last_para.saturation != cur_saturation)
    {
        last_para.saturation = cur_saturation;
        system_setting_set_screen_color(last_para.saturation ,last_para.brightness, last_para.contrast, last_para.sharpness, last_para.hue, false);
    }

	return EVENT_TRANSFER_KEEPON;
}

static int app_color_change_contrast_cb(int sel)
{
    int cur_contrast = 0;

    app_system_set_item_data_update(ITEM_CONTRAST);
    cur_contrast = app_sel_to_contrast(s_color_item[ITEM_CONTRAST].itemProperty.itemPropertyCmb.sel);
    if(last_para.contrast != cur_contrast)
    {
        last_para.contrast = cur_contrast;
        system_setting_set_screen_color(last_para.saturation ,last_para.brightness, last_para.contrast ,last_para.sharpness, last_para.hue, false);
    }

    return EVENT_TRANSFER_KEEPON;
}

#if APP_SHARPNESS_SUPPORT
static int app_color_change_sharpness_cb(int sel)
{
	int cur_sharpness = 0;

	app_system_set_item_data_update(ITEM_SHARPNESS);
    cur_sharpness = app_sel_to_sharpness_hue(s_color_item[ITEM_SHARPNESS].itemProperty.itemPropertyCmb.sel);
    if(last_para.sharpness != cur_sharpness)
    {
        last_para.sharpness = cur_sharpness;
        system_setting_set_screen_color(last_para.saturation ,last_para.brightness, last_para.contrast, last_para.sharpness, last_para.hue, false);
    }

	return EVENT_TRANSFER_KEEPON;
}
#endif
#if APP_HUE_SUPPORT
static int app_color_change_hue_cb(int sel)
{
    int cur_hue = 0;

    app_system_set_item_data_update(ITEM_HUE);
    cur_hue = app_sel_to_sharpness_hue(s_color_item[ITEM_HUE].itemProperty.itemPropertyCmb.sel);
    if(last_para.hue != cur_hue)
    {
        last_para.hue = cur_hue;
        system_setting_set_screen_color(last_para.saturation ,last_para.brightness, last_para.contrast ,last_para.sharpness, last_para.hue, false);
    }

    return EVENT_TRANSFER_KEEPON;
}
#endif
static void app_color_bright_item_init(void)
{
	s_color_item[ITEM_BRIGHT].itemTitle = STR_ID_BRIGHT;
	s_color_item[ITEM_BRIGHT].itemType = ITEM_CHOICE;
	s_color_item[ITEM_BRIGHT].itemProperty.itemPropertyCmb.content= "[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_BRIGHT].itemProperty.itemPropertyCmb.sel = app_color_to_sel(s_sys_color_para.brightness);
	s_color_item[ITEM_BRIGHT].itemCallback.cmbCallback.CmbChange= app_color_change_brighness_cb;
#if CVBS_TEST_SUPPORT
	s_color_item[ITEM_BRIGHT].itemStatus = ITEM_DISABLE;
#else
	s_color_item[ITEM_BRIGHT].itemStatus = ITEM_NORMAL;
#endif
}

static void app_color_saturation_item_init(void)
{
	s_color_item[ITEM_SATURATION].itemTitle = STR_ID_CHROMA;
	s_color_item[ITEM_SATURATION].itemType = ITEM_CHOICE;
	s_color_item[ITEM_SATURATION].itemProperty.itemPropertyCmb.content= "[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_SATURATION].itemProperty.itemPropertyCmb.sel = app_color_to_sel(s_sys_color_para.saturation);
	s_color_item[ITEM_SATURATION].itemCallback.cmbCallback.CmbChange = app_color_change_saturation_cb;
#if CVBS_TEST_SUPPORT
	s_color_item[ITEM_SATURATION].itemStatus = ITEM_DISABLE;
#else
	s_color_item[ITEM_SATURATION].itemStatus = ITEM_NORMAL;
#endif
}

static void app_color_contrast_item_init(void)
{
	s_color_item[ITEM_CONTRAST].itemTitle = STR_ID_CONTRAST;
	s_color_item[ITEM_CONTRAST].itemType = ITEM_CHOICE;
	s_color_item[ITEM_CONTRAST].itemProperty.itemPropertyCmb.content="[20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_CONTRAST].itemProperty.itemPropertyCmb.sel = app_contrast_to_sel(s_sys_color_para.contrast);
	s_color_item[ITEM_CONTRAST].itemCallback.cmbCallback.CmbChange = app_color_change_contrast_cb;
#if CVBS_TEST_SUPPORT
	s_color_item[ITEM_CONTRAST].itemStatus = ITEM_DISABLE;
#else
	s_color_item[ITEM_CONTRAST].itemStatus = ITEM_NORMAL;
#endif
}

#if APP_SHARPNESS_SUPPORT
static void app_color_sharpness_item_init(void)
{
	s_color_item[ITEM_SHARPNESS].itemTitle = STR_ID_SHARPNESS;
	s_color_item[ITEM_SHARPNESS].itemType = ITEM_CHOICE;
	s_color_item[ITEM_SHARPNESS].itemProperty.itemPropertyCmb.content= "[0%,10%,20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_SHARPNESS].itemProperty.itemPropertyCmb.sel = app_osd_sharpness_hue_to_sel(s_sys_color_para.sharpness);
	s_color_item[ITEM_SHARPNESS].itemCallback.cmbCallback.CmbChange = app_color_change_sharpness_cb;
#if CVBS_TEST_SUPPORT
	s_color_item[ITEM_SHARPNESS].itemStatus = ITEM_DISABLE;
# else
	s_color_item[ITEM_SHARPNESS].itemStatus = ITEM_NORMAL;
#endif
}
#endif

#if APP_HUE_SUPPORT
static void app_color_hue_item_init(void)
{
	s_color_item[ITEM_HUE].itemTitle = STR_ID_HUE;
	s_color_item[ITEM_HUE].itemType = ITEM_CHOICE;
	s_color_item[ITEM_HUE].itemProperty.itemPropertyCmb.content="[0%,10%,20%,30%,40%,50%,60%,70%,80%,90%,100%]";
	s_color_item[ITEM_HUE].itemProperty.itemPropertyCmb.sel = app_osd_sharpness_hue_to_sel(s_sys_color_para.hue);
	s_color_item[ITEM_HUE].itemCallback.cmbCallback.CmbChange = app_color_change_hue_cb;
#if CVBS_TEST_SUPPORT
	s_color_item[ITEM_HUE].itemStatus = ITEM_DISABLE;
#else
	s_color_item[ITEM_HUE].itemStatus = ITEM_NORMAL;
#endif
}
#endif
void app_color_setting_init(void)
{
	app_color_system_para_get();

	system_setting_set_screen_color(s_sys_color_para.saturation ,s_sys_color_para.brightness, s_sys_color_para.contrast ,s_sys_color_para.sharpness, s_sys_color_para.hue, false);
	app_log_info("get color setting init\n");
}

void app_color_setting_menu_exec(void)
{
	s_color_menu_flag = 1;
	memset(&s_color_setting_opt, 0 ,sizeof(s_color_setting_opt));
	app_color_system_para_get();
	memcpy(&last_para, &s_sys_color_para, sizeof(s_sys_color_para));

	s_color_setting_opt.menuTitle = STR_ID_COLOR_SET;
	s_color_setting_opt.itemNum = ITEM_COLOR_TOTAL;
	s_color_setting_opt.timeDisplay = TIP_HIDE;
	s_color_setting_opt.item = s_color_item;

	app_color_bright_item_init();
	app_color_saturation_item_init();
	app_color_contrast_item_init();
#if APP_SHARPNESS_SUPPORT
	app_color_sharpness_item_init();
#endif
#if APP_HUE_SUPPORT
	app_color_hue_item_init();
#endif
	s_color_setting_opt.exit = app_color_setting_exit_callback;

	app_system_set_create(&s_color_setting_opt);
}

#if WEBAPI_SUPPORT
#include <cJSON.h>

int app_webapi_color_setting_busy(void)
{
	return s_color_menu_flag;
}

void app_webapi_get_color_settings(int *bright, int *chroma, int *contrast)
{
	int init_value = 0;

	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &init_value, BRIGHTNESS);
	*bright = app_color_to_sel(init_value);

	GxBus_ConfigGetInt(SATURATION_KEY, &init_value, SATURATION);
	*chroma = app_color_to_sel(init_value);

	GxBus_ConfigGetInt(CONTRAST_KEY, &init_value, CONTRAST);
	*contrast = app_contrast_to_sel(init_value);
}

void app_webapi_make_color_data(cJSON *bright, cJSON *chroma, cJSON *contrast)
{
	int i = 0;

	for(i=0; i<COLOR_BRIGHT_TOTAL; i++)
	{
		cJSON_AddItemToArray(bright, cJSON_CreateString(color_bright_str[i]));
	}

	for(i=0; i<COLOR_CHROMA_TOTAL; i++)
	{
		cJSON_AddItemToArray(chroma, cJSON_CreateString(color_chroma_str[i]));
	}

	for(i=0; i<COLOR_CONTRAST_TOTAL; i++)
	{
		cJSON_AddItemToArray(contrast, cJSON_CreateString(color_contrast_str[i]));
	}

}

void app_webapi_save_color_settings(int bright, int chroma, int contrast)
{
	VideoColor cur_para = {0};
	int set = 0;

	GxBus_ConfigGetInt(BRIGHTNESS_KEY, &cur_para.brightness, BRIGHTNESS);
	GxBus_ConfigGetInt(SATURATION_KEY, &cur_para.saturation, SATURATION);
	GxBus_ConfigGetInt(CONTRAST_KEY,   &cur_para.contrast,   CONTRAST);
	GxBus_ConfigGetInt(SHARPNESS_KEY,  &cur_para.sharpness,  SHARPNESS);
	GxBus_ConfigGetInt(HUE_KEY,        &cur_para.hue,        HUE);

	if((bright>=0)&&bright<COLOR_BRIGHT_TOTAL)
	{
		if(bright != app_color_to_sel(cur_para.brightness))
		{
			set++;
			cur_para.brightness = app_sel_to_color(bright);
		}
	}

	if((chroma>=0)&&chroma<COLOR_CHROMA_TOTAL)
	{
		if(chroma != app_color_to_sel(cur_para.saturation))
		{
			set++;
			cur_para.saturation = app_sel_to_color(chroma);
		}
	}

	if((contrast>=0)&&contrast<COLOR_CONTRAST_TOTAL)
	{
		if(contrast != app_contrast_to_sel(cur_para.contrast))
		{
			set++;
			cur_para.contrast = app_sel_to_contrast(contrast);
		}
	}

	if(set>0)
	{
		app_color_setting_para_save(&cur_para);
		system_setting_set_screen_color(cur_para.saturation ,cur_para.brightness, cur_para.contrast, cur_para.sharpness, cur_para.hue, false);
	}

}

#endif


