#include "gxavdev.h"
#include "gxplayer_module.h"
#include "module/config/gxconfig.h"

#define SCREEN_WIDTH	1920
#define SCREEN_HEIGHT	1080

#define PLATFORM 0 //"v1.9.6-4"
void play_init(void)
{
	GxBus_ConfigSetInt(PLAYER_CONFIG_AUDIO_AC3_MODE, AUDIO_AC3_FULL_MODE);
	GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_SYNC_FLAG, 0);
#if PLATFORM
	GxPlayer_ModuleInit(0);
#else
	GxPlayer_ModuleInit();
#endif
	GxPlayer_ModuleRegisterALL();
	GxPlayer_SetVideoPPZoom(0);
	//PlayerVideoAutoAdapt config = {0};
	//GxPlayer_SetVideoAutoAdapt(&config);
}

void vout_init(int mode)
{
	VideoColor color = {1, 50, 50, 50};
	VideoOutputConfig vo_cfg;
	DisplayScreen scr = {DISPLAY_SCREEN_16X9, SCREEN_WIDTH, SCREEN_HEIGHT};
	GxPlayer_SetVideoColors(color);
	GxPlayer_SetVideoAspect(ASPECT_NORMAL);
#if PLATFORM
	GxPlayer_SetVideoOutputSelect(VIDEO_OUTPUT_HDMI|VIDEO_OUTPUT_RCA);
	vo_cfg.interface = VIDEO_OUTPUT_HDMI, vo_cfg.mode = VIDEO_OUTPUT_HDMI_1080I_50HZ;
	GxPlayer_SetVideoOutputConfig(vo_cfg);
	vo_cfg.interface = VIDEO_OUTPUT_RCA, vo_cfg.mode = VIDEO_OUTPUT_PAL;
#else
	GxPlayer_SetVideoOutputSelect(GX_VIDEO_OUTPUT_HDMI|GX_VIDEO_OUTPUT_RCA);
	vo_cfg.interface = GX_VIDEO_OUTPUT_HDMI, vo_cfg.mode = GX_VIDEO_OUTPUT_1080I_50HZ;
	GxPlayer_SetVideoOutputConfig(vo_cfg);
	vo_cfg.interface = GX_VIDEO_OUTPUT_RCA, vo_cfg.mode = GX_VIDEO_OUTPUT_PAL;
#endif
	GxPlayer_SetVideoOutputConfig(vo_cfg);
	GxPlayer_SetDisplayScreen(scr);
}

void aout_init(int mode)
{
	AudioOutputConfig ao_cfg;
	GxPlayer_SetAudioOutputSelect(AUDIO_OUTPUT_DAC_IN | AUDIO_OUTPUT_SPDIF | AUDIO_OUTPUT_HDMI);
	ao_cfg.output_data = AUDIO_OUTPUT_PCM;
	ao_cfg.output_port = AUDIO_OUTPUT_DAC_IN;
	GxPlayer_SetAudioOutputConfig(ao_cfg);
	ao_cfg.output_data = AUDIO_OUTPUT_SAFE;
	ao_cfg.output_port = AUDIO_OUTPUT_SPDIF;
	GxPlayer_SetAudioOutputConfig(ao_cfg);
	ao_cfg.output_data = AUDIO_OUTPUT_SAFE;
	ao_cfg.output_port = AUDIO_OUTPUT_HDMI;
	GxPlayer_SetAudioOutputConfig(ao_cfg);
	GxPlayer_SetAudioTrack(AUDIO_TRACK_STEREO);
	GxPlayer_SetAudioMute(0);
	GxPlayer_SetAudioVolume(85);
}

void play(const char *url)
{
	GxPlayer_MediaPlay("gxcasplayer", url, 0, 85, NULL);
	printf("%s\n",url);
}

