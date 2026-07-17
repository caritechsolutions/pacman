#include "gxplayer_internal.h"
#include "gx_audio_control.h"

status_t GxPlayer_SetCvbsViewport(PlayerWindow window)
{
	av_flow_log("[Flow] - [%s %d]: %u, %u, %u, %u\n",
			__func__, __LINE__, window.x, window.y, window.width, window.height);
	return GxPlayer_SystemSet(PSYS_VOUT_CVBSVIDWPORT, &window);
}

status_t GxPlayer_SetVideoPlayBypass(int bypass)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, bypass);
	return GxPlayer_SystemSet(PSYS_VDEC_PLAY_BYPASS, &bypass);
}

status_t GxPlayer_SetScartMode(VideoOutputScartMode mode)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, mode);
	return GxPlayer_SystemSet(PSYS_VOUT_SCARTMODE, &mode);
}

status_t GxPlayer_SetDisplayScreen(DisplayScreen scr)
{
	av_flow_log("[Flow] - [%s %d]: [a]:%d, [x]:%d, [y]:%d\n",
			__func__, __LINE__, scr.aspect, scr.xres, scr.yres);
	if(scr.aspect != DISPLAY_SCREEN_4X3 &&
	   scr.aspect != DISPLAY_SCREEN_16X9)
		return GXCORE_ERROR;
	return GxPlayer_SystemSet(PSYS_VOUT_SCREEN, &scr);
}

status_t GxPlayer_SetVideoAspect(VideoAspect aspect )
{
	av_flow_log("[Flow] - [%s %d]: [a]:%d\n", __func__, __LINE__, aspect);
	if(aspect<ASPECT_NORMAL || aspect>ASPECT_16X9)
		return GXCORE_ERROR;
	return GxPlayer_SystemSet(PSYS_VOUT_ASPECT, &aspect);
}

status_t GxPlayer_SetVideoDrcMode(VideoDrcMode mode)
{
	av_flow_log("[Flow] - [%s %d]: [a]:%d\n", __func__, __LINE__, mode);
	if(mode < GX_VDRC_AUTO || mode > GX_VDRC_TOSDR)
		return GXCORE_ERROR;
	return GxPlayer_SystemSet(PSYS_VDRC_MODE, &mode);
}

status_t GxPlayer_SetVideoSyncMode(VideoSyncMode mode)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, mode);
	return GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &mode);
}

status_t GxPlayer_SetVideoSyncTime(unsigned int timems)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, timems);
	return GxPlayer_SystemSet(PSYS_VDEC_SYNC_TIMEMS, &timems);
}

status_t GxPlayer_SetVideoSyncHighTime(unsigned int timems)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, timems);
	return GxPlayer_SystemSet(PSYS_VDEC_SYNC_HIGH_TIMEMS, &timems);
}

status_t GxPlayer_SetVideoColors(VideoColor color)
{
	av_flow_log("[Flow] - [%s %d]: %d %d %d\n",
			__func__, __LINE__, color.brightness, color.saturation, color.contrast);
	if((color.brightness  >100 || color.brightness < 0) ||
		(color.saturation >100 || color.saturation < 0) ||
		(color.contrast   >100 || color.contrast   < 0))
		return GXCORE_ERROR;
	return GxPlayer_SystemSet(PSYS_VOUT_COLOR, &color);
}

status_t GxPlayer_GetVideoColors(VideoColor *color)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemGet(PSYS_VOUT_COLOR, color);
}

status_t GxPlayer_GetVideoUsableIface(int *iface)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemGet(PSYS_VOUT_USABLE_IFACE, iface);
}

status_t GxPlayer_SetVideoOutputSelect(int interface)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, interface);
	return GxPlayer_SystemSet(PSYS_VOUT_INTERFACE, &interface);
}

status_t GxPlayer_GetVideoOutputConfig(VideoOutputConfig *config)
{
	av_flow_log("[Flow] - [%s %d]: %d\n",
			__func__, __LINE__, config->interface);
	if(config->interface < GX_VIDEO_OUTPUT_RCA ||
			config->interface > GX_VIDEO_OUTPUT_HDMI)
		return GXCORE_ERROR;
	return GxPlayer_SystemGet(PSYS_VOUT_RESOLUTION, config);
}

status_t GxPlayer_SetVideoOutputConfig(VideoOutputConfig config)
{
	av_flow_log("[Flow] - [%s %d]: %d\n",
			__func__, __LINE__, config.interface, config.mode);
	if(config.mode < GX_VIDEO_OUTPUT_PAL ||
			config.mode >= GX_VIDEO_OUTPUT_MODE_MAX)
		return GXCORE_ERROR;
	return GxPlayer_SystemSet(PSYS_VOUT_RESOLUTION, &config);
}

status_t GxPlayer_SetVideoOutputDef(VideoOutDefault outdef)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemSet(PSYS_VOUT_DEF, &outdef);
}

status_t GxPlayer_SetVideoOutputPower(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	return GxPlayer_SystemSet(PSYS_VOUT_POWER, &enable);
}

status_t GxPlayer_SetVideoOutputPower2(VideoOutPower power)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemSet(PSYS_VOUT_POWER2, &power);
}

status_t GxPlayer_SetVideoAutoAdapt(PlayerVideoAutoAdapt* Auto)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, Auto->enable);
	return GxPlayer_SystemSet(PSYS_VOUT_AUTOADAPT, Auto);
}

status_t GxPlayer_SetVideoMosaicDrop(int number)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, number);
	return GxPlayer_SystemSet(PSYS_VDEC_MOSAIC_DROP, &number);
}

status_t GxPlayer_SetVideoMosaicGate(int percent)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, percent);
	return GxPlayer_SystemSet(PSYS_VDEC_MOSAIC_GATE, &percent);
}

status_t GxPlayer_SetVideoUserdataConfig(VideoUserdataConfig *config)
{
	av_flow_log("[Flow] - [%s %d]: %d %d %d\n",
			__func__, __LINE__, config->cc_enable, config->cc_display, config->hdr_enable);
	GxPlayer_SystemSet(PSYS_VDEC_CC_ENABLE,  &config->cc_enable );
	GxPlayer_SystemSet(PSYS_VDEC_CC_DISPLAY, &config->cc_display);
	GxPlayer_SystemSet(PSYS_VDEC_HDR_ENABLE, &config->hdr_enable );

	return GX_PLAYER_OK;
}

status_t GxPlayer_SetAudioDolbyCertify(int enable)
{
	int esa_size = 512*1024;
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	GxPlayer_SystemSet(PSYS_BufSizeESA, &esa_size);
	return GxPlayer_SystemSet(PSYS_AUDIO_DOLBY_CERTIFY, &enable);
}

status_t GxPlayer_SetAudioPureRecoveryMode(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	return GxPlayer_SystemSet(PSYS_AUDIO_PURE_RECOVERY, &enable);
}

status_t GxPlayer_SetAudioOutputSelect(int interface)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, interface);
	return GxPlayer_SystemSet(PSYS_AOUT_INTERFACE, &interface);
}

status_t GxPlayer_SetAudioOutputConfig(AudioOutputConfig config)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, config.output_port);
	if(config.output_port == AUDIO_OUTPUT_SPDIF)
		GxPlayer_SystemSet(PSYS_AOUT_SPDSRC, &config.output_data);
	if(config.output_port == AUDIO_OUTPUT_HDMI)
		GxPlayer_SystemSet(PSYS_AOUT_HDMISRC, &config.output_data);
	return GxPlayer_SystemSet(PSYS_AOUT_CONFIGPORT, &config);
}

status_t GxPlayer_SetAudioSyncEnable(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	return GxPlayer_SystemSet(PSYS_AOUT_SYNC_FLAG, &enable);
}

status_t GxPlayer_SetAudioSyncTime(unsigned int timems)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, timems);
	return GxPlayer_SystemSet(PSYS_AOUT_SYNC_TIMEMS, &timems);
}

status_t GxPlayer_SetAudioSyncHighTime(unsigned int timems)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, timems);
	return GxPlayer_SystemSet(PSYS_AOUT_SYNC_HIGH_TIMEMS, &timems);
}

status_t GxPlayer_SetAudioAC3Mode(AudioAc3Mode mode)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, mode);
	return GxPlayer_SystemSet(PSYS_AOUT_AC3MODE, &mode);
}

status_t GxPlayer_SetAudioAACMode(AudioAACMode mode)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, mode);
	return GxPlayer_SystemSet(PSYS_AOUT_AACMODE, &mode);
}

status_t GxPlayer_SetAudioAC3Enable(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	return GxPlayer_SystemSet(PSYS_ENABLE_AC3, &enable);
}

status_t GxPlayer_SetAudioDownMix(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	return GxPlayer_SystemSet(PSYS_AOUT_DOWNMIX, &enable);
}

status_t GxPlayer_SetAudioVolume(unsigned int vol)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, vol);
	return GxPlayer_SystemSet(PSYS_AOUT_VOLUME, &vol);
}

status_t GxPlayer_GetAudioVolume(unsigned int *vol)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, vol);
	return GxPlayer_SystemGet(PSYS_AOUT_VOLUME, vol);
}

status_t GxPlayer_SetAudioTrack(AudioTrack tarck)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemSet(PSYS_AOUT_TRACK, &tarck);
}

status_t GxPlayer_SetAudioDescriptorTrack(AudioDecriptorTrack tarck)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemSet(PSYS_AOUT_DESCRIPTOR_TRACK, &tarck);
}

status_t GxPlayer_SetAudioBoostVolume(unsigned int volume)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, volume);
	return GxPlayer_SystemSet(PSYS_ADEC_BOOST_VOLUME, &volume);
}

status_t GxPlayer_SetAudioDescriptorBoostVolume(unsigned int volume)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, volume);
	return GxPlayer_SystemSet(PSYS_ADEC_DESCRIPTOR_BOOST_VOLUME, &volume);
}

status_t GxPlayer_SetAudioDACVolume(int voldb)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemSet(PSYS_AOUT_SET_DAC_VOLUME, &voldb);
}

status_t GxPlayer_SetAudioMute(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	enable = enable ? -1: 0;
	return GxPlayer_SystemSet(PSYS_AOUT_MUTE, &enable);
}

status_t GxPlayer_GetAudioMute(int *enable)
{
	if (enable) {
		GxPlayer_SystemGet(PSYS_AOUT_MUTE, enable);
		*enable = *enable ? 1 : 0;
	}
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, *enable);
	return GX_PLAYER_OK;
}

status_t GxPlayer_SetAudioPowerMute(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	enable = enable ? 1: 0;
	return GxPlayer_SystemSet(PSYS_AOUT_POWER_MUTE, &enable);
}

status_t GxPlayer_SetAudioPort(AudioOutPortSet port)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemSet(PSYS_AOUT_SET_PORT, &port);
}

status_t GxPlayer_TurnAudioPort(AudioOutPortTurn port)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemSet(PSYS_AOUT_TURN_PORT, &port);
}

status_t GxPlayer_GetAudioDescriptorVaild(int *vaild)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);

	*vaild = 1;
	return GX_PLAYER_OK;
}

status_t GxPlayer_GetEncrypt(EncryptKey *key)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxPlayer_SystemGet(PSYS_ENCRYPT_KEY, key);
}

status_t GxPlayer_SetAudioDmxFullGate(int full_gate)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, full_gate);
	return GxPlayer_SystemSet(PSYS_AUDIO_DMX_FULL_GATE, &full_gate);
}

status_t GxPlayer_SetFreezeFrameSwitch(int enable)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	return GxPlayer_SystemSet(PSYS_FREEZE_ENABLE, &enable);
}

status_t GxPlayer_SetAudioSmartVolume(unsigned int enable, AudioSmartConfig *config)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, enable);
	return gx_audio_smart_volume(enable, config);
}

status_t GxPlayer_SetAudioSmartVolumeLoudness(float loudness)
{
	av_flow_log("[Flow] - [%s %d]: %f\n", __func__, __LINE__, loudness);
	return gx_audio_smart_volume_set_loudness(loudness);
}

status_t GxPlayer_GetAudioSmartVolumeLoudness(float *loudness)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return gx_audio_smart_volume_get_loudness(loudness);
}

status_t GxPlayer_SetAudioSpeedMode(AudioOutputSpeedMode mode)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, mode);
	return GxPlayer_SystemSet(PSYS_AUDIO_SPEED_MODE, &mode);
}

void GxPlayer_SetStreamCallback(PlayerStreamPortCallback *cbk)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_CBK_STREAM_PORT, (void*)cbk);
}

void GxPlayer_SetEventCallback(PLAYER_EVENT_REPORT report)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_CBK_EVENT_REPORT, (void*)report);
}

void GxPlayer_SetFreezeCallback(PLAYER_FREEZE_FRAME freeze,PLAYER_UNFREEZE_FRAME unfreeze)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_CBK_FREEZE, (void*)freeze);
	GxPlayer_SystemSet(PSYS_CBK_UNFREEZE, (void*)unfreeze);
}

void GxPlayer_SetSeekCallback(PLAYER_SEEK_CBK cbk)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_CBK_SEEK, (void*)cbk);
}

void GxPlayer_SetVideoPPZoom(int enable)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_VDEC_ENABLE_PPZOOM, (void*)(&enable));
}

void GxPlayer_SetVideoFpsSource(VideoFpsSource src)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_VDEC_FPS_SOURCE, (void*)(&src));
}

void GxPlayer_SetVideoDeinterlace(int enable)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_VDEC_ENABLE_DEINTERLACE, (void*)(&enable));
}

void GxPlayer_SetVideoDeinterlaceParam(VideoDeinterlaceParam *param)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_VDEC_ENABLE_DEINTERLACE, (void*)(&param->flag));
	GxPlayer_SystemSet(PSYS_VDEC_DEINTERLACE_MAX_WIDTH,  (void*)(&param->max_width));
	GxPlayer_SystemSet(PSYS_VDEC_DEINTERLACE_MAX_HEIGHT, (void*)(&param->max_height));
}

void GxPlayer_SetVideoExtraFrameBuffer(VideoCodecType mask)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_VDEC_EXTRA_FB, (void*)(&mask));
}

void GxPlayer_AudioCodecMask(AudioCodecType mask)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_AUDIOCODEC_MASK, &mask);
}

void GxPlayer_VideoCodecMask(VideoCodecType mask)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_VIDEOCODEC_MASK, &mask);
}

void GxPlayer_SetTP(const char *url)
{
	av_flow_log("[Flow] - [%s %d]: %s\n", __func__, __LINE__, url);
	GxStream *stream = GxStream_Open(url, GX_STREAM_READ, 0);
	if (stream) {
		GxMediaFilter_Config(stream);
		GxStream_Close(stream);
	}
}

extern int gxplayer_heap_register(unsigned int start, unsigned int size);
extern int gxplayer_heap_unregister(unsigned int start);
extern int gxplayer_heap_modify(PlayerHeapInfo *info);

int  GxPlayer_HeapRegister(unsigned int start, unsigned int size)
{
	av_flow_log("[Flow] - [%s %d]: 0x%x 0x%x\n", __func__, __LINE__, start, size);
	return gxplayer_heap_register(start, size);
}

int  GxPlayer_HeapUnregister(unsigned int start)
{
	av_flow_log("[Flow] - [%s %d]: 0x%x\n", __func__, __LINE__, start);
	return gxplayer_heap_unregister(start);
}

int  GxPlayer_HeapModify(PlayerHeapInfo *info)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return gxplayer_heap_modify(info);
}

void GxPlayer_SetCustomSSLCipherSuiteCallback(CUSTOM_SSL_CHPHER_SUITE_CBK report)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_NETWORK_CUSTOM_SSL_CIPHER_SUITE, (char*)report);
}

void GxPlayer_SetNetworkMulitBandwidthResolution(NetworkResolution *config)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxPlayer_SystemSet(PSYS_NETWORK_RESOLUTION_WIDTH, &(config->width));
	GxPlayer_SystemSet(PSYS_NETWORK_RESOLUTION_HEIGHT, &(config->height));
}

status_t GxPlayer_SetNetAudioDelay(unsigned int vol)
{
	printf ("[Flow] - [%s %d]: %d\n", __func__, __LINE__, vol);
	return GxPlayer_SystemSet(PSYS_NETWORK_AUDIO_DELAY, &vol);
}
