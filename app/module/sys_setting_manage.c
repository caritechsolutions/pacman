#include "app_send_msg.h"
#include "module/app_log.h"
#include "app_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "sys_setting_manage.h"
#include "app_main_menu_theme_type.h"

void system_setting_set_full_gate(int init_value)
{
	GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};
	GxBus_ConfigSetInt(PVR_FILE_SIZE_KEY, init_value);

	pvrConfig.volume_sizemb = init_value;

	app_send_msg_exec(GXMSG_PLAYER_PVR_CONFIG,&pvrConfig);

}

void system_setting_set_screen_ratio(int32_t screenRatio)
{
    GxMsgProperty_PlayerDisplayScreen screen_ratio;

    screen_ratio.aspect = screenRatio;

    screen_ratio.xres = APP_THEME_XRES;
    screen_ratio.yres = APP_THEME_YRES;

    app_send_msg_exec(GXMSG_PLAYER_DISPLAY_SCREEN, &screen_ratio);
}

void system_setting_set_screen_type(int32_t screenType)
{
    GxMsgProperty_PlayerVideoAspect aspect;

    aspect = screenType;

    app_send_msg_exec(GXMSG_PLAYER_VIDEO_ASPECT, &aspect);
}

void system_setting_set_screen_color(uint32_t chroma,uint32_t brightness,uint32_t contrast,
         uint32_t sharpness, uint32_t hue, bool realtime)
{
#define MAX_COLOR_VALUE 100
    GxMsgProperty_PlayerVideoColor color = {0};

    if(MAX_COLOR_VALUE < chroma)
        chroma = MAX_COLOR_VALUE;
    if(MAX_COLOR_VALUE < brightness)
        brightness = MAX_COLOR_VALUE;
    if(MAX_COLOR_VALUE < contrast)
        contrast = MAX_COLOR_VALUE;
    if(MAX_COLOR_VALUE < sharpness)
        sharpness = MAX_COLOR_VALUE;
    if(MAX_COLOR_VALUE < hue)
        hue = MAX_COLOR_VALUE;

	//color.vout_id = 0; // default for hdmi
	color.interface = GX_VIDEO_OUTPUT_HDMI;
    color.brightness = brightness;
    color.saturation = chroma;
    color.contrast = contrast;
    color.sharpness = sharpness;
    color.hue = hue;

    if(realtime){
        GxPlayer_SetVideoColors(color);
    } else {
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_COLOR, &color);
    }
	//color.vout_id = 1; //for cvbs
	color.interface = GX_VIDEO_OUTPUT_RCA;
    if(realtime){
        GxPlayer_SetVideoColors(color);
    } else {
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_COLOR, &color);
    }
}

void system_setting_set_tv_type_no_auto(uint32_t tvType,uint32_t output)
{
	GxMsgProperty_PlayerVideoModeConfig  modeConfig;
	if(TvPal == tvType)
	{
	    modeConfig.mode = GX_VIDEO_OUTPUT_PAL;
	}
	else if(TvNtsc == tvType)
	{
	    modeConfig.mode = GX_VIDEO_OUTPUT_NTSC_M;
	}
	else
	{
	    APP_PRINT("modeconfig error tvType = %d\n",tvType);
	    modeConfig.mode = GX_VIDEO_OUTPUT_PAL;
	}
	modeConfig.interface = output;
	app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&modeConfig);
}

