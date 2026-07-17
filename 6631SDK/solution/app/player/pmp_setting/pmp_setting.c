#include "pmp_setting.h"
#include "gui_core.h"
#include "gxbus.h"
#include "gxavdev.h"
#include "gxmsg.h"
#include "play_pic.h"
#include "app.h"
#include "full_screen.h"
#include "app_default_params.h"
#include "app_volume_value.h"
#include "app_module.h"
#define PMPSET_ACTIVE					"pmpset_active_flag"

extern int app_audio_prog_track_get_mode(void);
extern int app_audio_prog_track_set_mode(int value,uint8_t mode);
extern void app_save_mute_state(int value);
extern int app_get_mute_state(void);

#define DO_NOT_USE_SAME_CONFIG
#define FULL_ASPECT

// 0-40 映射成0-100
#define AUDIO_MAP_40(audio)	(((audio)*5)>>1)

//3: 0-25 映射成0-75 4: 0-25 映射成0-100
#define VOLUME_NUM (4)
#define AUDIO_MAP_25(audio)	((audio) * VOLUME_NUM)

// 0-50 to 0-75
 #define AUDIO_MAP_50(audio) ((audio)/2 + (audio))


#ifdef  DO_NOT_USE_SAME_CONFIG
#if CVBS_TEST_SUPPORT
pmpset_aspect_ratio RecordRATIO =  PMPSET_ASPECT_RATIO_ORIGINAL_SIZE;
#else
pmpset_aspect_ratio RecordRATIO =  PMPSET_ASPECT_RATIO_FULL_SCREEN;// PMPSET_ASPECT_RATIO_AUTO;
#endif

status_t pmpset_exit(void)
{
	int RecordSetting  = 0;
	GxBus_ConfigGetInt(PMPSET_SAVE_ASPECT_RATIO, &RecordSetting, 0);

#if 0
	if(ASPECT_RAW_RATIO == RecordSetting)
	{
		RecordSetting = PMPSET_ASPECT_RATIO_AUTO;
	}
	else if(ASPECT_NORMAL == RecordSetting)
	{
		RecordSetting = PMPSET_ASPECT_RATIO_FULL_SCREEN;
	}
	else if(ASPECT_RAW_SIZE == RecordSetting)
	{
		RecordSetting = PMPSET_ASPECT_RATIO_ORIGINAL_SIZE;
	}
	else if(ASPECT_4X3_CUT == RecordSetting || ASPECT_4X3_PULL== RecordSetting)
	{
		RecordSetting = PMPSET_ASPECT_RATIO_4_3;
	}
	else if(ASPECT_16X9_CUT == RecordSetting || ASPECT_16X9_PULL == RecordSetting)
	{
		RecordSetting = PMPSET_ASPECT_RATIO_16_9;
	}
	pmpset_set_int(PMPSET_ASPECT_RATIO, RecordSetting);
#endif
	GxMessage *msg_aspect;
	GxMsgProperty_PlayerVideoAspect *config_aspect ;
	msg_aspect = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_ASPECT);
	APP_CHECK_P(msg_aspect, GXCORE_ERROR);
	config_aspect = GxBus_GetMsgPropertyPtr(msg_aspect, GxMsgProperty_PlayerVideoAspect);
	APP_CHECK_P(config_aspect, GXCORE_ERROR);

    	*config_aspect = RecordSetting;
	GxBus_MessageSendWait(msg_aspect);
	GxBus_MessageFree(msg_aspect);



	return 0;
}

#else
status_t pmpset_exit(void)
{
	return 0;
}
#endif

status_t pmpset_init(void)
{
	int32_t value = 0;

#ifdef  DO_NOT_USE_SAME_CONFIG
	//PMPSET_ASPECT_RATIO,
#ifdef FULL_ASPECT
	pmpset_set_int(PMPSET_ASPECT_RATIO, PMPSET_ASPECT_RATIO_FULL_SCREEN);//PMPSET_ASPECT_RATIO_AUTO
#else
	pmpset_set_int(PMPSET_ASPECT_RATIO, PMPSET_ASPECT_RATIO_AUTO);//PMPSET_ASPECT_RATIO_AUTO
#endif
#endif

	GxBus_ConfigGetInt(PMPSET_ACTIVE, &value, 0);
	if(0 == value)
	{
		pmpset_factory_default();
		return GXCORE_SUCCESS;
	}

	APP_Printf_Blue("\n[APP] pmpset\n");
	APP_Printf("---------------------------------------\n");

#ifndef PMP_ATTACH_DVB
	/*GLOBAL*/
	//PMPSET_LANG,
	value = pmpset_get_int(PMPSET_LANG);
	pmpset_set_int(PMPSET_LANG, value);
	APP_Printf("PMPSET_LANG:			%d\n", value);

	//PMPSET_DISPLAY_SCREEN
	value = pmpset_get_int(PMPSET_DISPLAY_SCREEN);
	pmpset_set_int(PMPSET_DISPLAY_SCREEN, value);
	APP_Printf("PMPSET_DISPLAY_SCREEN:		%d\n", value);

	/*AV*/
	//PMPSET_VOLUME,
	value = pmpset_get_int(PMPSET_VOLUME);
	pmpset_set_int(PMPSET_VOLUME, value);
	APP_Printf("PMPSET_VOLUME:			%d\n", value);

	//PMPSET_OUTPUT_MODE,
	value = pmpset_get_int(PMPSET_OUTPUT_MODE);
	pmpset_set_int(PMPSET_OUTPUT_MODE, value);
	APP_Printf("PMPSET_OUTPUT_MODE:		%d\n", value);

	//PMPSET_ASPECT_RATIO,
	value = pmpset_get_int(PMPSET_ASPECT_RATIO);
	pmpset_set_int(PMPSET_ASPECT_RATIO, value);
	APP_Printf("PMPSET_ASPECT_RATIO:		%d\n", value);

	//PMPSET_VIDEO_FORMAT,
	value = pmpset_get_int(PMPSET_VIDEO_FORMAT);
	pmpset_set_int(PMPSET_VIDEO_FORMAT, value);
	APP_Printf("PMPSET_VIDEO_FORMAT:		%d\n", value);

	//PMPSET_AUDIO_TRACK,
	value = pmpset_get_int(PMPSET_AUDIO_TRACK);
	pmpset_set_int(PMPSET_AUDIO_TRACK, value);
	APP_Printf("PMPSET_AUDIO_TRACK:		%d\n", value);

	//PMPSET_MUTE,
	value = pmpset_get_int(PMPSET_MUTE);
	pmpset_set_int(PMPSET_MUTE, value);
	APP_Printf("PMPSET_MUTE:			%d\n", value);

	/*MOVIE*/

	/*MUSIC*/

	/*PIC*/

	/*TEXT*/

	/*NETWORK*/


	//PMPSET_POWER_ON_PALY
	//TODO:
#endif

	APP_Printf("---------------------------------------\n");

	return GXCORE_SUCCESS;
}

status_t pmpset_factory_default(void)
{
	int32_t ret = 0;

	APP_Printf("[PMPSET] factory default\n");

	/*GLOBAL*/
	//PMPSET_LANG,
#ifndef PMP_ATTACH_DVB
	pmpset_set_int(PMPSET_LANG, PMPSET_LANG_CHINESE);
#else
	//pmpset_set_int(PMPSET_LANG, PMPSET_LANG_ENGLISH);
#endif

	//PMPSET_DISPLAY_SCREEN
	pmpset_set_int(PMPSET_DISPLAY_SCREEN, PMPSET_DISPLAY_SCREEN_1280_720);
	//PMPSET_POWER_ON_PALY
	pmpset_set_int(PMPSET_POWER_ON_PALY, PMPSET_TONE_OFF);
	pmpset_set_int(PMPSET_SAVE_TAGS, PMPSET_TONE_ON);

	//PMPSET_MUTE
    if(g_AppFullArb.state.mute == STATE_OFF)
    {
        pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
    }
    else
    {
        pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_ON);
    }
	/*MOVIE*/
	pmpset_set_int(PMPSET_MOVIE_PLAY_SEQUENCE, PMPSET_MOVIE_PLAY_SEQUENCE_ONLY_ONCE);
	pmpset_set_int(PMPSET_MOVIE_SUBT_VISIBILITY, PMPSET_TONE_ON);

	/*MUSIC*/
	pmpset_set_int(PMPSET_MUSIC_PLAY_SEQUENCE, PMPSET_MUSIC_PLAY_SEQUENCE_ONLY_ONCE);
	pmpset_set_int(PMPSET_MUSIC_VIEW_MODE, PMPSET_MUSIC_VIEW_MODE_SPECTRUM);

	/*PIC*/
	pmpset_set_int(PMPSET_PIC_SWITCH_DURATION, PMPSET_PIC_SWITCH_DURATION_3s);
	pmpset_set_int(PMPSET_PIC_SWITCH_MODE, PMPSET_PIC_SWITCH_MODE_DEFAULT);
	pmpset_set_int(PMPSET_PIC_PLAY_SEQUENCE, PMPSET_PIC_PLAY_SEQUENCE_SEQUENCE);
	pmpset_set_int(PMPSET_PIC_SPECIAL_EFFECTS, PMPSET_PIC_EFFECT_NONE);

	/*TEXT*/
	pmpset_set_int(PMPSET_TEXT_ROLL_LINES, 1);
	pmpset_set_int(PMPSET_TEXT_AUTO_ROLL, PMPSET_TONE_OFF);
	/*NETWORK*/

#ifndef PMP_ATTACH_DVB
	/*AV*/
	//PMPSET_VOLUME,
	pmpset_set_int(PMPSET_VOLUME, 20);

	//PMPSET_OUTPUT_MODE,
	pmpset_set_int(PMPSET_OUTPUT_MODE, PMPSET_OUTPUT_MODE_SCART);

	//PMPSET_VIDEO_FORMAT,
	pmpset_set_int(PMPSET_VIDEO_FORMAT, PMPSET_VIDEO_FORMAT_AUTO);

	//PMPSET_ASPECT_RATIO,
	pmpset_set_int(PMPSET_ASPECT_RATIO, PMPSET_ASPECT_RATIO_AUTO);

	//PMPSET_AUDIO_TRACK,
	pmpset_set_int(PMPSET_AUDIO_TRACK, PMPSET_AUDIO_TRACK_STEREO);
#endif

	/*pmpset_active_flag*/
	ret = GxBus_ConfigSetInt(PMPSET_ACTIVE, 1);
	if(GXCONFIG_FAILURE == ret) return GXCORE_ERROR;

	pmp_init_tags();

	return GXCORE_SUCCESS;
}

/*value 0-25*/
status_t pmpset_send_volume(int value)
{
    uint32_t audio_vol = 0;
    int32_t volume_scope = AUDIO_SCOPE;
    // player do sth
    GxBus_ConfigGetInt(AUDIO_VOLUME_SCOPE, (int32_t*)(&volume_scope), AUDIO_SCOPE);
    if(AUDIO_SCOPE == volume_scope)
    {
        audio_vol = AUDIO_MAP_25(value);
        app_system_vol_set(audio_vol);
    }
    return GXCORE_SUCCESS;
}

status_t pmpset_send_mute(int value)
{
	GxMessage *msg;
	GxMsgProperty_PlayerAudioMute *para;
#if BLUETOOTH_SUPPORT
{
    extern int app_bluetooth_source_set_mute(int mute);
    int ret_value = 1;
    ret_value = app_bluetooth_source_set_mute(value);
    if(ret_value == 0)
    {
        msg = GxBus_MessageNew(GXMSG_PLAYER_AUDIO_MUTE);
        APP_CHECK_P(msg, GXCORE_ERROR);
        para = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerAudioMute);
        APP_CHECK_P(para, GXCORE_ERROR);
        *para = value;
        GxBus_MessageSend(msg);
    }

    if(value == 0)
        app_set_hardware_unmute();
    else
        app_set_hardware_mute();
}
#else
	msg = GxBus_MessageNew(GXMSG_PLAYER_AUDIO_MUTE);
	APP_CHECK_P(msg, GXCORE_ERROR);
	para = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerAudioMute);
	APP_CHECK_P(para, GXCORE_ERROR);
	*para = value;
	GxBus_MessageSend(msg);
#endif
#ifdef PMP_ATTACH_DVB
      app_save_mute_state(value);
#else
	GxBus_ConfigSetInt(PMPSET_SAVE_MUTE, value);
#endif
	return GXCORE_SUCCESS;
}

status_t pmpset_send_track(int value)
{
	GxMessage *msg;
	GxMsgProperty_PlayerAudioTrack* audio_config;

	msg = GxBus_MessageNew(GXMSG_PLAYER_AUDIO_TRACK);
	APP_CHECK_P(msg, GXCORE_ERROR);
	audio_config = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerAudioTrack);
	APP_CHECK_P(audio_config, GXCORE_ERROR);

	if(PMPSET_AUDIO_TRACK_STEREO == value)
		*audio_config= AUDIO_TRACK_STEREO;
	else if(PMPSET_AUDIO_TRACK_LEFT == value)
		*audio_config = AUDIO_TRACK_LEFT;
	else if(PMPSET_AUDIO_TRACK_RIGHT == value)
		*audio_config = AUDIO_TRACK_RIGHT;
    else if(PMPSET_AUDIO_TRACK_MONO == value)
		*audio_config = AUDIO_TRACK_MONO;

	GxBus_MessageSend(msg);
#if 0
       app_audio_prog_track_set_mode(*audio_config,0);
#else
	GxBus_ConfigSetInt(PMPSET_SAVE_AUDIO_TRACK, *audio_config);
#endif
	return GXCORE_SUCCESS;
}

status_t pmpset_send_output_mode(int value)
{
    int init_value = 0;
	GxMessage *msg_interface;
	GxMsgProperty_PlayerVideoInterface *config_interface;
	msg_interface = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_INTERFACE);
	APP_CHECK_P(msg_interface, GXCORE_ERROR);
	config_interface = GxBus_GetMsgPropertyPtr(msg_interface, GxMsgProperty_PlayerVideoInterface);
	APP_CHECK_P(config_interface, GXCORE_ERROR);



	GxMessage *msg_video_mode;
	GxMsgProperty_PlayerVideoModeConfig *config_video_mode;
	msg_video_mode = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_MODE_CONFIG);
	APP_CHECK_P(msg_video_mode, GXCORE_ERROR);
	config_video_mode = GxBus_GetMsgPropertyPtr(msg_video_mode, GxMsgProperty_PlayerVideoModeConfig);
	APP_CHECK_P(config_video_mode, GXCORE_ERROR);

	pmpset_video_format video_format = 0;

	if(PMPSET_OUTPUT_MODE_RCA ==value)
	{
		*config_interface = GX_VIDEO_OUTPUT_RCA;

		config_video_mode->interface = GX_VIDEO_OUTPUT_RCA;
		video_format = pmpset_get_int(PMPSET_VIDEO_FORMAT);
		if(video_format >= PMPSET_VIDEO_FORMAT_PAL && video_format <= PMPSET_VIDEO_FORMAT_NTSC_443)
		{
			config_video_mode->mode = video_format;
		}
		else
		{
			config_video_mode->mode = GX_VIDEO_OUTPUT_PAL;
		}
	}
	//else if(PMPSET_OUTPUT_MODE_VGA ==value)
	//{
	//	*config_interface = GX_VIDEO_OUTPUT_VGA;

	//	config_video_mode->interface = GX_VIDEO_OUTPUT_VGA;
	//	config_video_mode->mode = GX_VIDEO_OUTPUT_VGA_480P;
	//}
	else if(PMPSET_OUTPUT_MODE_YUV == value)
	{
		*config_interface = GX_VIDEO_OUTPUT_YUV;

		config_video_mode->interface = GX_VIDEO_OUTPUT_YUV;
		config_video_mode->mode = GX_VIDEO_OUTPUT_480I;
	}
	//else if (PMPSET_OUTPUT_MODE_DVI == value)
	//{
	//	*config_interface = GX_VIDEO_OUTPUT_DVI;

	//	config_video_mode->interface = GX_VIDEO_OUTPUT_DVI;
	//	// TODO: config_video_mode->mode = ;
	//}
	else if (PMPSET_OUTPUT_MODE_HDMI == value)
	{
		*config_interface = GX_VIDEO_OUTPUT_HDMI;

		config_video_mode->interface = GX_VIDEO_OUTPUT_HDMI;
		config_video_mode->mode = GX_VIDEO_OUTPUT_1080I_50HZ;
	}
	else if (PMPSET_OUTPUT_MODE_SCART == value)
	{
		*config_interface = GX_VIDEO_OUTPUT_SCART;

		config_video_mode->interface = GX_VIDEO_OUTPUT_SCART;
		video_format = pmpset_get_int(PMPSET_VIDEO_FORMAT);
		if(video_format >= PMPSET_VIDEO_FORMAT_PAL && video_format <= PMPSET_VIDEO_FORMAT_NTSC_443)
		{
			config_video_mode->mode = video_format;
		}
		else
		{
			config_video_mode->mode = GX_VIDEO_OUTPUT_PAL;
		}
	}
	else if (PMPSET_OUTPUT_MODE_SVIDEO == value)
	{
		*config_interface = GX_VIDEO_OUTPUT_SVIDEO;

		config_video_mode->interface = GX_VIDEO_OUTPUT_SVIDEO;
		video_format = pmpset_get_int(PMPSET_VIDEO_FORMAT);
		if(video_format >= PMPSET_VIDEO_FORMAT_PAL && video_format <= PMPSET_VIDEO_FORMAT_NTSC_443)
		{
			config_video_mode->mode = video_format;
		}
		else
		{
			config_video_mode->mode = GX_VIDEO_OUTPUT_PAL;
		}
	}
	//else if (PMPSET_OUTPUT_MODE_LCD == value)
	//{
	//	*config_interface = GX_VIDEO_OUTPUT_LCD;

	//	config_video_mode->interface = GX_VIDEO_OUTPUT_LCD;
	//	config_video_mode->mode = GX_VIDEO_OUTPUT_DIGITAL_RGB_720x480_0_255;
	//}


	GxBus_MessageSend(msg_interface);
	GxBus_ConfigSetInt(PMPSET_SAVE_OUTPUT_MODE, *config_interface);

	GxBus_MessageSend(msg_video_mode);
	GxBus_ConfigSetInt(PMPSET_SAVE_VIDEO_FORMAT, config_video_mode->mode);
    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
    app_av_set_videoout_mode(init_value);
	return GXCORE_SUCCESS;
}

status_t pmpset_send_video_format(int value)
{
	GxMessage *msg_auto_adapt;
    int32_t init_value = 0;
	GxMsgProperty_PlayerVideoAutoAdapt *config_auto_adapt;
	msg_auto_adapt = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_AUTO_ADAPT);
	APP_CHECK_P(msg_auto_adapt, GXCORE_ERROR);
	config_auto_adapt = GxBus_GetMsgPropertyPtr(msg_auto_adapt, GxMsgProperty_PlayerVideoAutoAdapt);
	APP_CHECK_P(config_auto_adapt, GXCORE_ERROR);

	pmpset_output_mode output_mode = pmpset_get_int(PMPSET_OUTPUT_MODE);
	if(PMPSET_VIDEO_FORMAT_AUTO == value)
	{
		config_auto_adapt->enable = 1;
		config_auto_adapt->pal = GX_VIDEO_OUTPUT_PAL;
		config_auto_adapt->ntsc = GX_VIDEO_OUTPUT_NTSC_M;
		GxBus_MessageSend(msg_auto_adapt);
		GxBus_ConfigSetInt(PMPSET_SAVE_VIDEO_AUTO_ADAPT, config_auto_adapt->enable);
	}
	else
	{
		GxMessage *msg_video_mode;
		GxMsgProperty_PlayerVideoModeConfig *config_video_mode;
		msg_video_mode = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_MODE_CONFIG);
		APP_CHECK_P(msg_video_mode, GXCORE_ERROR);
		config_video_mode = GxBus_GetMsgPropertyPtr(msg_video_mode, GxMsgProperty_PlayerVideoModeConfig);
		APP_CHECK_P(config_video_mode, GXCORE_ERROR);

		config_auto_adapt->enable = 0;
		GxBus_MessageSend(msg_auto_adapt);
		GxBus_ConfigSetInt(PMPSET_SAVE_VIDEO_AUTO_ADAPT, config_auto_adapt->enable);


		if(PMPSET_OUTPUT_MODE_RCA ==output_mode)
		{
			config_video_mode->interface = GX_VIDEO_OUTPUT_RCA;
			config_video_mode->mode = value;
		}
		//else if(PMPSET_OUTPUT_MODE_VGA ==output_mode)
		//{
		//	config_video_mode->interface = GX_VIDEO_OUTPUT_VGA;
		//	config_video_mode->mode = value;
		//}
		else if(PMPSET_OUTPUT_MODE_YUV == output_mode)
		{
			config_video_mode->interface = GX_VIDEO_OUTPUT_YUV;
			config_video_mode->mode = value;
		}
		//else if (PMPSET_OUTPUT_MODE_DVI == output_mode)
		//{
		//	config_video_mode->interface = GX_VIDEO_OUTPUT_DVI;
		//	config_video_mode->mode = value;
		//}
		else if (PMPSET_OUTPUT_MODE_HDMI == output_mode)
		{
			config_video_mode->interface = GX_VIDEO_OUTPUT_HDMI;
			config_video_mode->mode = value;
		}
		else if (PMPSET_OUTPUT_MODE_SCART == output_mode)
		{
			config_video_mode->interface = GX_VIDEO_OUTPUT_SCART;
			config_video_mode->mode = value;
		}
		else if (PMPSET_OUTPUT_MODE_SVIDEO == output_mode)
		{
			config_video_mode->interface = GX_VIDEO_OUTPUT_SVIDEO;
			config_video_mode->mode = value;
		}
		//else if (PMPSET_OUTPUT_MODE_LCD == output_mode)
		//{
		//	config_video_mode->interface = GX_VIDEO_OUTPUT_LCD;
		//	config_video_mode->mode = value;
		//}

		GxBus_MessageSend(msg_video_mode);
		GxBus_ConfigSetInt(PMPSET_SAVE_VIDEO_FORMAT, config_video_mode->mode);
	}

    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
    app_av_set_videoout_mode(init_value);
	return GXCORE_SUCCESS;
}


status_t pmpset_send_ratio(int value)
{
#if 0
	GxMessage *msg_video_hide;
	GxMsgProperty_PlayerVideoHide *config_video_hide;
	msg_video_hide = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_HIDE);
	APP_CHECK_P(msg_video_hide, GXCORE_ERROR);
	config_video_hide = GxBus_GetMsgPropertyPtr(msg_video_hide, GxMsgProperty_PlayerVideoHide);
	APP_CHECK_P(config_video_hide, GXCORE_ERROR);
	config_video_hide->player = PMP_PLAYER_AV;


	GxMessage *msg_video_show;
	GxMsgProperty_PlayerVideoShow *config_video_show;
	msg_video_show = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_SHOW);
	APP_CHECK_P(msg_video_show, GXCORE_ERROR);
	config_video_show = GxBus_GetMsgPropertyPtr(msg_video_show, GxMsgProperty_PlayerVideoShow);
	APP_CHECK_P(config_video_show, GXCORE_ERROR);
	config_video_show->player = PMP_PLAYER_AV;
#endif

	GxMessage *msg_aspect;
	GxMsgProperty_PlayerVideoAspect *config_aspect;
	msg_aspect = GxBus_MessageNew(GXMSG_PLAYER_VIDEO_ASPECT);
	APP_CHECK_P(msg_aspect, GXCORE_ERROR);
	config_aspect = GxBus_GetMsgPropertyPtr(msg_aspect, GxMsgProperty_PlayerVideoAspect);
	APP_CHECK_P(config_aspect, GXCORE_ERROR);

	switch (value)
	{
		case PMPSET_ASPECT_RATIO_AUTO://原始比例
			*config_aspect=ASPECT_RAW_RATIO;//
			break;
		case PMPSET_ASPECT_RATIO_FULL_SCREEN://全屏
			*config_aspect=ASPECT_NORMAL;
			break;
		case PMPSET_ASPECT_RATIO_ORIGINAL_SIZE:
			*config_aspect=ASPECT_RAW_SIZE;
			break;
		case PMPSET_ASPECT_RATIO_4_3:
			*config_aspect=ASPECT_4X3_CUT;
			break;
		case PMPSET_ASPECT_RATIO_16_9:
			*config_aspect=ASPECT_16X9_CUT;
			break;
		default:
			break;
	}

#if 0
	GxBus_MessageSendWait(msg_video_hide);
	GxBus_MessageFree(msg_video_hide);
#endif

	//GxBus_MessageSendWait(msg_aspect);
	//GxBus_MessageFree(msg_aspect);
	GxBus_MessageSend(msg_aspect);

#if 0
	GxBus_MessageSendWait(msg_video_show);
	GxBus_MessageFree(msg_video_show);
#endif

#ifdef  DO_NOT_USE_SAME_CONFIG
	RecordRATIO = value;
#else
	GxBus_ConfigSetInt(PMPSET_SAVE_ASPECT_RATIO, *config_aspect);
#endif

	return GXCORE_SUCCESS;
}


status_t pmpset_send_movie_play_sequence(int value)
{
	switch (value)
	{
		case PMPSET_MOVIE_PLAY_SEQUENCE_ONLY_ONCE:
			GxBus_ConfigSetInt(PMPSET_SAVE_MOVIE_PLAY_SEQUENCE,PMPSET_MOVIE_PLAY_SEQUENCE_ONLY_ONCE);
			break;

		case PMPSET_MOVIE_PLAY_SEQUENCE_REPEAT_ONE:
			GxBus_ConfigSetInt(PMPSET_SAVE_MOVIE_PLAY_SEQUENCE,PMPSET_MOVIE_PLAY_SEQUENCE_REPEAT_ONE);
			break;

		/*case PMPSET_MOVIE_PLAY_SEQUENCE_REPEAT_ALL:
			GxBus_ConfigSetInt(PMPSET_SAVE_MOVIE_PLAY_SEQUENCE,PMPSET_MOVIE_PLAY_SEQUENCE_REPEAT_ALL);
			break;*/

		case PMPSET_MOVIE_PLAY_SEQUENCE_SEQUENCE:
			GxBus_ConfigSetInt(PMPSET_SAVE_MOVIE_PLAY_SEQUENCE,PMPSET_MOVIE_PLAY_SEQUENCE_SEQUENCE);
			break;

		/*case PMPSET_MOVIE_PLAY_SEQUENCE_RANDOM:
			GxBus_ConfigSetInt(PMPSET_SAVE_MOVIE_PLAY_SEQUENCE,PMPSET_MOVIE_PLAY_SEQUENCE_RANDOM);
			break;*/

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_send_music_play_sequence(int value)
{
	switch (value)
	{
		case PMPSET_MUSIC_PLAY_SEQUENCE_ONLY_ONCE:
			GxBus_ConfigSetInt(PMPSET_SAVE_MUSIC_PLAY_SEQUENCE,PMPSET_MUSIC_PLAY_SEQUENCE_ONLY_ONCE);
			break;

		case PMPSET_MUSIC_PLAY_SEQUENCE_REPEAT_ONE:
			GxBus_ConfigSetInt(PMPSET_SAVE_MUSIC_PLAY_SEQUENCE,PMPSET_MUSIC_PLAY_SEQUENCE_REPEAT_ONE);
			break;

		/*case PMPSET_MUSIC_PLAY_SEQUENCE_REPEAT_ALL:
			GxBus_ConfigSetInt(PMPSET_SAVE_MUSIC_PLAY_SEQUENCE,PMPSET_MUSIC_PLAY_SEQUENCE_REPEAT_ALL);
			break;
			*/
		case PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE:
			GxBus_ConfigSetInt(PMPSET_SAVE_MUSIC_PLAY_SEQUENCE,PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE);
			break;

		/*case PMPSET_MUSIC_PLAY_SEQUENCE_RANDOM:
			GxBus_ConfigSetInt(PMPSET_SAVE_MUSIC_PLAY_SEQUENCE,PMPSET_MUSIC_PLAY_SEQUENCE_RANDOM);
			break;*/

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_send_music_view_mode(int value)
{
	switch (value)
	{
		case PMPSET_MUSIC_VIEW_MODE_LRC:
			GxBus_ConfigSetInt(PMPSET_SAVE_MUSIC_VIEW_MODE,PMPSET_MUSIC_VIEW_MODE_LRC);
			break;

		case PMPSET_MUSIC_VIEW_MODE_SPECTRUM:
			GxBus_ConfigSetInt(PMPSET_SAVE_MUSIC_VIEW_MODE,PMPSET_MUSIC_VIEW_MODE_SPECTRUM);
			break;

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_send_pic_switch_duration(int value)
{
	switch (value)
	{
		case PMPSET_PIC_SWITCH_DURATION_3s:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SWITCH_DURATION,PMPSET_PIC_SWITCH_DURATION_3s);
			break;

		case PMPSET_PIC_SWITCH_DURATION_5s:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SWITCH_DURATION,PMPSET_PIC_SWITCH_DURATION_5s);
			break;

		case PMPSET_PIC_SWITCH_DURATION_7s:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SWITCH_DURATION,PMPSET_PIC_SWITCH_DURATION_7s);
			break;

		case PMPSET_PIC_SWITCH_DURATION_10s:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SWITCH_DURATION,PMPSET_PIC_SWITCH_DURATION_10s);
			break;

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_send_pic_switch_mode(int value)
{
	switch (value)
	{
		case PMPSET_PIC_SWITCH_MODE_DEFAULT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SWITCH_MODE,PMPSET_PIC_SWITCH_MODE_DEFAULT);
			break;

		case PMPSET_PIC_SWITCH_MODE_LR:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SWITCH_MODE,PMPSET_PIC_SWITCH_MODE_LR);
			break;

		case PMPSET_PIC_SWITCH_MODE_RL:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SWITCH_MODE,PMPSET_PIC_SWITCH_MODE_RL);
			break;

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_send_pic_play_sequence(int value)
{
	switch (value)
	{
		case PMPSET_PIC_PLAY_SEQUENCE_SEQUENCE:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_PLAY_SEQUENCE,PMPSET_PIC_PLAY_SEQUENCE_SEQUENCE);
			break;

		case PMPSET_PIC_PLAY_SEQUENCE_RANDOM:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_PLAY_SEQUENCE,PMPSET_PIC_PLAY_SEQUENCE_RANDOM);
			break;

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_send_pic_special_effects(int value)
{
	switch (value)
	{
		case PMPSET_PIC_EFFECT_NONE:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_EFFECT_NONE);
			break;

		case PMPSET_PIC_SLIDE_LEFT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_SLIDE_LEFT);
			break;

		case PMPSET_PIC_SLIDE_RIGHT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_SLIDE_RIGHT);
			break;

		case PMPSET_PIC_SLIDE_TOP:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_SLIDE_TOP);
			break;

		case PMPSET_PIC_SLIDE_BOTTOM:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_SLIDE_BOTTOM);
			break;

		case PMPSET_PIC_DIAGONAL_TOP_LEFT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_DIAGONAL_TOP_LEFT);
			break;

		case PMPSET_PIC_DIAGONAL_TOP_RIGHT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_DIAGONAL_TOP_RIGHT);
			break;

		case PMPSET_PIC_DIAGONAL_BOTTOM_LEFT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_DIAGONAL_BOTTOM_LEFT);
			break;

		case PMPSET_PIC_DIAGONAL_BOTTOM_RIGHT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_DIAGONAL_BOTTOM_RIGHT);
			break;

		case PMPSET_PIC_HSTEP_TOP_LEFT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_HSTEP_TOP_LEFT);
			break;

		case PMPSET_PIC_HSTEP_TOP_RIGHT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_HSTEP_TOP_RIGHT);
			break;

		case PMPSET_PIC_HSTEP_BOTTOM_LEFT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_HSTEP_BOTTOM_LEFT);
			break;

		case PMPSET_PIC_HSTEP_BOTTOM_RIGHT:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_HSTEP_BOTTOM_RIGHT);
			break;

		case PMPSET_PIC_CHECKER_BOARD:
			GxBus_ConfigSetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS,PMPSET_PIC_CHECKER_BOARD);
			break;

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_send_display_screen(int value)
{
	GxMessage *msg;
	GxMsgProperty_PlayerDisplayScreen* display_screen;

	msg = GxBus_MessageNew(GXMSG_PLAYER_DISPLAY_SCREEN);
	APP_CHECK_P(msg, GXCORE_ERROR);
	display_screen = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerDisplayScreen);
	APP_CHECK_P(display_screen, GXCORE_ERROR);

/*
	switch (value)
	{
		case PMPSET_DISPLAY_SCREEN_720_576:
			display_screen->aspect = DISPLAY_SCREEN_4X3;
			display_screen->xres = 720;
			display_screen->yres = 576;
			break;

		case PMPSET_DISPLAY_SCREEN_1280_720:
			display_screen->aspect = DISPLAY_SCREEN_4X3;
			display_screen->xres = 1280;
			display_screen->yres = 720;
			break;

		default:
			break;
	}
*/
	display_screen->aspect = DISPLAY_SCREEN_4X3;
	display_screen->xres = APP_THEME_XRES;
	display_screen->yres = APP_THEME_YRES;

	GxBus_MessageSend(msg);

	GxBus_ConfigSetInt(PMPSET_SAVE_DISPLAY_SCREEN,value);

	return GXCORE_SUCCESS;
}

status_t pmpset_send_save_tags(int value)
{
	switch (value)
	{
		case PMPSET_TONE_OFF:
			GxBus_ConfigSetInt(PMPSET_SAVE_SAVE_TAGS,PMPSET_TONE_OFF);
			break;

		case PMPSET_TONE_ON:
			GxBus_ConfigSetInt(PMPSET_SAVE_SAVE_TAGS,PMPSET_TONE_ON);
			break;

		default:
			break;
	}
	return GXCORE_SUCCESS;
}

status_t pmpset_set_int(pmpset_property property, int value)
{
	switch(property)
	{
		/*AV*/
		case PMPSET_VOLUME:
			pmpset_send_volume(value);
			break;
		case PMPSET_MUTE://zfz 20101014
			pmpset_send_mute(value);
			break;


//#ifndef PMP_ATTACH_DVB
		/*GLOBAL*/
		case PMPSET_LANG:
			if(PMPSET_LANG_ENGLISH == value)
			{
				GxBus_ConfigSetInt(PMPSET_SAVE_LANG, PMPSET_LANG_ENGLISH);
				GUI_SetInterface("osd_language", "English");
			}
			else if(PMPSET_LANG_CHINESE == value)
			{
				GxBus_ConfigSetInt(PMPSET_SAVE_LANG, PMPSET_LANG_CHINESE);
				GUI_SetInterface("osd_language", "Chinese");
			}
			break;

		case PMPSET_DISPLAY_SCREEN:
			pmpset_send_display_screen(value);
			break;

		case PMPSET_POWER_ON_PALY:
			break;
		case PMPSET_LAST_PLAY_PATH:
			break;
		case PMPSET_SAVE_TAGS:
			pmpset_send_save_tags(value);
			break;
		case PMPSET_FACTORY_DEFAULT:
			pmpset_factory_default();
			return GXCORE_SUCCESS;

		case PMPSET_OUTPUT_MODE:
			pmpset_send_output_mode(value);
			break;

		case PMPSET_VIDEO_FORMAT:
			pmpset_send_video_format(value);
			break;

		case PMPSET_ASPECT_RATIO:
			pmpset_send_ratio(value);
			break;

		case PMPSET_AUDIO_TRACK:
			pmpset_send_track(value);
			break;

		/*MOVIE*/
		case PMPSET_MOVIE_PLAY_SEQUENCE:
			pmpset_send_movie_play_sequence(value);
			break;
		case	PMPSET_MOVIE_SUBT_VISIBILITY:
			GxBus_ConfigSetInt(PMPSET_SAVE_MOVIE_SUBT,value);
			break;
		/*MUSIC*/
		case PMPSET_MUSIC_PLAY_SEQUENCE:
			pmpset_send_music_play_sequence(value);
			break;
		case PMPSET_MUSIC_VIEW_MODE:
			pmpset_send_music_view_mode(value);
			break;

		/*PIC*/
		case PMPSET_PIC_SWITCH_DURATION:
			pmpset_send_pic_switch_duration(value);
			break;

		case PMPSET_PIC_SWITCH_MODE:
			pmpset_send_pic_switch_mode(value);
			break;

		case PMPSET_PIC_PLAY_SEQUENCE:
			pmpset_send_pic_play_sequence(value);
			break;

		case PMPSET_PIC_SPECIAL_EFFECTS:
			pmpset_send_pic_special_effects(value);
			break;

		/*TEXT*/
		case PMPSET_TEXT_ROLL_LINES:
			GxBus_ConfigSetInt(PMPSET_SAVE_TEXT_ROLL_LINES,value);
			break;

		case PMPSET_TEXT_AUTO_ROLL:
			GxBus_ConfigSetInt(PMPSET_SAVE_TEXT_AUTO_ROLL,value);
			break;
		/*NETWORK*/
//#endif
		default:
			break;
	}

	return GXCORE_SUCCESS;
}

int32_t pmpset_get_int(pmpset_property property)
{
	int value_return = 0;
	int value_get = 0;

	switch(property)
	{
		/*AV*/
		case PMPSET_VOLUME:
            value_get = app_system_vol_get();
			value_return = value_get / VOLUME_NUM;
			break;
		case PMPSET_MUTE://zfz 20101014
#ifdef PMP_ATTACH_DVB
			value_return = app_get_mute_state();
#else
			GxBus_ConfigGetInt(PMPSET_SAVE_MUTE, &value_return, 0);
#endif
			break;

		/*GLOBAL*/
		case PMPSET_LANG:
			GxBus_ConfigGetInt(PMPSET_SAVE_LANG, &value_return, 0);
			break;

		case PMPSET_DISPLAY_SCREEN:
			GxBus_ConfigGetInt(PMPSET_SAVE_DISPLAY_SCREEN,&value_return, 0);
			break;

		case PMPSET_LAST_PLAY_PATH:
			break;

		case PMPSET_SAVE_TAGS:
			GxBus_ConfigGetInt(PMPSET_SAVE_SAVE_TAGS, &value_return, 1);
			break;

		case PMPSET_FACTORY_DEFAULT:
			return GXCORE_SUCCESS;

		case PMPSET_OUTPUT_MODE:
			GxBus_ConfigGetInt(PMPSET_SAVE_OUTPUT_MODE, &value_get, 0);
			if(GX_VIDEO_OUTPUT_RCA == value_get)
			{
				value_return = PMPSET_OUTPUT_MODE_RCA;
			}
			//else if(GX_VIDEO_OUTPUT_VGA == value_get)
			//{
			//	value_return = PMPSET_OUTPUT_MODE_VGA;
			//}
			else if(GX_VIDEO_OUTPUT_YUV == value_get)
			{
				value_return = PMPSET_OUTPUT_MODE_YUV;
			}
			//else if(GX_VIDEO_OUTPUT_DVI == value_get)
			//{
			//	value_return = PMPSET_OUTPUT_MODE_DVI;
			//}
			else if(GX_VIDEO_OUTPUT_HDMI == value_get)
			{
				value_return = PMPSET_OUTPUT_MODE_HDMI;
			}
			else if(GX_VIDEO_OUTPUT_SCART == value_get)
			{
				value_return = PMPSET_OUTPUT_MODE_SCART;
			}
			else if(GX_VIDEO_OUTPUT_SVIDEO == value_get)
			{
				value_return = PMPSET_OUTPUT_MODE_SVIDEO;
			}
			//else if(GX_VIDEO_OUTPUT_LCD == value_get)
			//{
			//	value_return = PMPSET_OUTPUT_MODE_LCD;
			//}
			break;

		case PMPSET_VIDEO_FORMAT:
			GxBus_ConfigGetInt(PMPSET_SAVE_VIDEO_AUTO_ADAPT, &value_get, 0);
			if(1 == value_get)
			{
				value_return = PMPSET_VIDEO_FORMAT_AUTO;
			}
			else
			{
				GxBus_ConfigGetInt(PMPSET_SAVE_VIDEO_FORMAT, &value_return, 0);
			}
			break;

		case PMPSET_ASPECT_RATIO:
#ifdef  DO_NOT_USE_SAME_CONFIG
			value_return = RecordRATIO;
#else
			GxBus_ConfigGetInt(PMPSET_SAVE_ASPECT_RATIO, &value_get, 0);
			if(ASPECT_RAW_RATIO == value_get)
			{
				value_return = PMPSET_ASPECT_RATIO_AUTO;
			}
			else if(ASPECT_NORMAL == value_get)
			{
				value_return = PMPSET_ASPECT_RATIO_FULL_SCREEN;
			}
			else if(ASPECT_RAW_SIZE == value_get)
			{
				value_return = PMPSET_ASPECT_RATIO_ORIGINAL_SIZE;
			}
			else if(ASPECT_4X3_CUT == value_get || ASPECT_4X3_PULL== value_get)
			{
				value_return = PMPSET_ASPECT_RATIO_4_3;
			}
			else if(ASPECT_16X9_CUT == value_get || ASPECT_16X9_PULL == value_get)
			{
				value_return = PMPSET_ASPECT_RATIO_16_9;
			}
#endif

			break;
		case PMPSET_AUDIO_TRACK:
			#if 0
			value_get = app_audio_prog_track_get_mode();
			#else
			GxBus_ConfigGetInt(PMPSET_SAVE_AUDIO_TRACK, &value_get, 0);
			#endif
			if(AUDIO_TRACK_STEREO==value_get)
			{
				value_return = PMPSET_AUDIO_TRACK_STEREO;
			}
			else if(AUDIO_TRACK_LEFT==value_get)
			{
				value_return = PMPSET_AUDIO_TRACK_LEFT;
			}
			else if(AUDIO_TRACK_RIGHT ==value_get)
			{
				value_return = PMPSET_AUDIO_TRACK_RIGHT;
			}
			break;

		/*MOVIE*/
		case PMPSET_MOVIE_PLAY_SEQUENCE:
			GxBus_ConfigGetInt(PMPSET_SAVE_MOVIE_PLAY_SEQUENCE, &value_return, 0);
			break;
		case	PMPSET_MOVIE_SUBT_VISIBILITY:
			GxBus_ConfigGetInt(PMPSET_SAVE_MOVIE_SUBT,&value_return, PMPSET_TONE_ON);
			break;
		/*MUSIC*/
		case PMPSET_MUSIC_PLAY_SEQUENCE:
			GxBus_ConfigGetInt(PMPSET_SAVE_MUSIC_PLAY_SEQUENCE, &value_return, 0);
			break;

		case PMPSET_MUSIC_VIEW_MODE:
			GxBus_ConfigGetInt(PMPSET_SAVE_MUSIC_VIEW_MODE, &value_return, 0);
			break;

		/*PIC*/
		case PMPSET_PIC_SWITCH_DURATION:
			GxBus_ConfigGetInt(PMPSET_SAVE_PIC_SWITCH_DURATION, &value_return, 0);
			break;

		case PMPSET_PIC_SWITCH_MODE:
			GxBus_ConfigGetInt(PMPSET_SAVE_PIC_SWITCH_MODE, &value_return, 0);
			break;

		case PMPSET_PIC_PLAY_SEQUENCE:
			GxBus_ConfigGetInt(PMPSET_SAVE_PIC_PLAY_SEQUENCE, &value_return, 0);
			break;

		case PMPSET_PIC_SPECIAL_EFFECTS:
			GxBus_ConfigGetInt(PMPSET_SAVE_PIC_SPECIAL_EFFECTS, &value_return, 0);
			break;

		/*TEXT*/
		case PMPSET_TEXT_ROLL_LINES:
			GxBus_ConfigGetInt(PMPSET_SAVE_TEXT_ROLL_LINES,&value_return, 0);
			break;

		case PMPSET_TEXT_AUTO_ROLL:
			GxBus_ConfigGetInt(PMPSET_SAVE_TEXT_AUTO_ROLL,&value_return, 0);
			break;
		/*NETWORK*/
		default:
			break;
	}

	return value_return;
}
status_t pmpset_set_str(pmpset_property property, char* str)
{
	return GXCORE_SUCCESS;
}


status_t pmpset_get_str(const char* key, char** str, int str_len)
{
	return GXCORE_SUCCESS;
}

