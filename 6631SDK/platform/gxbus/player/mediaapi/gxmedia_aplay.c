#include "gxmedia_api.h"
#include "gxmedia_module.h"
#include "gxmedia_common.h"

typedef struct {
	int             dev;
	int             mod;
	int             is_config;
	int             codec;
	unsigned char  *kern_baseaddr;
	unsigned char  *user_baseaddr;
	unsigned int    kern_basesize;
} GxMediaAPlay;

handle_t GxMediaApi_APlayOpen(int modid)
{
	GxMediaAPlay *aplay = av_mallocz(sizeof(GxMediaAPlay));
	GxAudioOutProperty_ConfigSource source_config;
	GxAudioOutProperty_ConfigPort   port_config;

	aplay->dev = GxAvdev_CreateDevice(0);
	CHECK_RETURN_VALUE(aplay->dev);

	aplay->mod = GxAvdev_OpenModule(aplay->dev, GXAV_MOD_AUDIO_OUT, modid);
	CHECK_RETURN_VALUE(aplay->mod);

	memset(&source_config, 0, sizeof(GxAudioOutProperty_ConfigSource));
	source_config.indata_type = PCM_TYPE;
	source_config.audiodec_id = -1;
	GxAVSetProperty(aplay->dev, aplay->mod,
			GxAudioOutPropertyID_ConfigSource, &source_config, sizeof(GxAudioOutProperty_ConfigSource));

	memset(&port_config, 0, sizeof(GxAudioOutProperty_ConfigPort));
	port_config.port_type = AUDIO_OUT_I2S_IN_DAC|AUDIO_OUT_HDMI|AUDIO_OUT_SPDIF;
	port_config.data_type = SMART_OUTPUT;
	GxAVSetProperty(aplay->dev, aplay->mod,
			GxAudioOutPropertyID_ConfigPort, &port_config, sizeof(GxAudioOutProperty_ConfigPort));

	aplay->codec = GX_APLAY_CODEC_PCM;

	return (handle_t)aplay;
}

status_t GxMediaApi_APlayClose(handle_t handle)
{
	GxMediaAPlay *aplay = (GxMediaAPlay *)handle;

	if (aplay == NULL)
		return -1;

	if (aplay->is_config == 1) {
		GxAVSetProperty(aplay->dev, aplay->mod, GxAudioOutPropertyID_Stop, NULL, 0);
        aplay->is_config = 0;
    }

    if (aplay->kern_baseaddr != NULL) {
		GxCore_UnMap(aplay->dev, aplay->kern_baseaddr, aplay->kern_basesize);
        aplay->kern_baseaddr = NULL;
        aplay->user_baseaddr = NULL;
        aplay->kern_basesize = 0;
    }

	if (aplay->mod != -1)
		GxAvdev_CloseModule(aplay->dev, aplay->mod);

	if (aplay->dev != -1)
		GxAvdev_DestroyDevice(aplay->dev);

	av_free(aplay);

	return 0;
}

status_t GxMediaApi_APlayGetPCMFrame(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms)
{
	GxMediaAPlay *aplay = (GxMediaAPlay *)handle;
	GxAPlayProperty_Control ctrl;

	if (aplay == NULL)
		return -1;

	if (aplay->is_config == 0) {
		GxAPlayProperty_ControlFrameConfig *config = &ctrl.arg1;

		memset(&ctrl, 0, sizeof(GxAPlayProperty_Control));
		ctrl.cmd = GX_APLAY_CONTROL_FRAME_CONFIG;
		config->codec = aplay->codec;
		config->pcm.samplefreq = frame->samplefre;
		config->pcm.channelnum = frame->channels;
		config->pcm.bitwidth   = frame->bitwidth;
		config->pcm.endian     = frame->endian;
		config->pcm.interlace  = frame->interlace;
		GxAVSetProperty(aplay->dev, aplay->mod,
				GxAPlayPropertyID_Control, &ctrl, sizeof(GxAPlayProperty_Control));

		GxAVSetProperty(aplay->dev, aplay->mod, GxAudioOutPropertyID_Run, NULL, 0);
		aplay->is_config = 1;
	}

	if (1) {
		GxAPlayProperty_ControlFrameAlloc *config = &ctrl.arg2;
		unsigned int k_baseaddr = 0;
		unsigned int k_curraddr = 0;

		memset(&ctrl, 0, sizeof(GxAPlayProperty_Control));
		ctrl.cmd = GX_APLAY_CONTROL_FRAME_ALLOC;
		config->codec = aplay->codec;
		config->frame_size = frame->frame_size;
		if (GxAVGetProperty(aplay->dev, aplay->mod,
					GxAPlayPropertyID_Control, &ctrl, sizeof(GxAPlayProperty_Control)) < 0)
			return -1;
		k_baseaddr = (unsigned int)config->buf.kernel_baseaddr;
		k_curraddr = (unsigned int)config->buf.kernel_addr;
		if (aplay->user_baseaddr == NULL) {
			aplay->kern_baseaddr = (unsigned char *)k_baseaddr;
			aplay->kern_basesize = config->buf.kernel_basesize;
			aplay->user_baseaddr = GxCore_Map(aplay->dev, k_baseaddr, aplay->kern_basesize);
		}
		frame->kern_frame_buf = (unsigned char *)k_curraddr;
		frame->user_frame_buf = aplay->user_baseaddr + (k_curraddr - k_baseaddr);
		frame->frame_size     = config->buf.kernel_size;
		frame->ch_num         = config->buf.ch_num;
		frame->ch_size        = config->buf.ch_size;
		frame->frame_no++;
	}

	return 0;
}

status_t GxMediaApi_APlayPushPCMFrame(handle_t handle, GxMediaApiOneFrame *frame)
{
	GxMediaAPlay *aplay = (GxMediaAPlay *)handle;
	GxAPlayProperty_Control ctrl;

	if (aplay == NULL)
		return -1;

	if (1) {
		GxAPlayProperty_ControlFramePush *config = &ctrl.arg3;

		memset(&ctrl, 0, sizeof(GxAPlayProperty_Control));
		ctrl.cmd = GX_APLAY_CONTROL_FRAME_PUSH;
		config->codec = aplay->codec;
		config->buf.kernel_addr  = frame->kern_frame_buf;
		config->buf.kernel_size  = frame->frame_size;
		config->buf.ch_num       = frame->ch_num;
		config->buf.ch_size      = frame->ch_size;
		GxAVSetProperty(aplay->dev, aplay->mod,
				GxAPlayPropertyID_Control, &ctrl, sizeof(GxAPlayProperty_Control));
	}

	return 0;
}

status_t GxMediaApi_APlaySetChannel(handle_t handle, GxMediaApiAPlayChannel channel)
{
	GxAudioOutProperty_Channel ch;
	GxMediaAPlay *aplay = (GxMediaAPlay *)handle;

	if (aplay == NULL)
		return -1;

	ch.lfe                = AUDIO_CHANNEL_M;
	ch.center             = AUDIO_CHANNEL_M;
	ch.left_surround      = AUDIO_CHANNEL_M;
	ch.right_surround     = AUDIO_CHANNEL_M;
	ch.left_backsurround  = AUDIO_CHANNEL_M;
	ch.right_backsurround = AUDIO_CHANNEL_M;
	ch.mono = 0;
	switch (channel) {
	case GXMEDIA_APLAY_STERO:
		ch.left  = AUDIO_CHANNEL_0;
		ch.right = AUDIO_CHANNEL_1;
		break;
	case GXMEDIA_APLAY_LEFT:
		ch.left  = AUDIO_CHANNEL_0;
		ch.right = AUDIO_CHANNEL_0;
		break;
	case GXMEDIA_APLAY_RIGHT:
		ch.left  = AUDIO_CHANNEL_1;
		ch.right = AUDIO_CHANNEL_1;
		break;
	case GXMEDIA_APLAY_MULTI:
		ch.left               = AUDIO_CHANNEL_0;
		ch.right              = AUDIO_CHANNEL_1;
		ch.lfe                = AUDIO_CHANNEL_2;
		ch.center             = AUDIO_CHANNEL_3;
		ch.left_surround      = AUDIO_CHANNEL_4;
		ch.right_surround     = AUDIO_CHANNEL_5;
		ch.left_backsurround  = AUDIO_CHANNEL_6;
		ch.right_backsurround = AUDIO_CHANNEL_7;
		break;
	default:
		break;
	}
	return GxAVSetProperty(aplay->dev, aplay->mod,
			GxAudioOutPropertyID_Channel, &ch, sizeof(GxAudioOutProperty_Channel));
}

status_t GxMediaApi_APlaySetVolume(handle_t handle, int volume)
{
	GxAudioOutProperty_Volume _vol;
	GxMediaAPlay *aplay = (GxMediaAPlay *)handle;

	if (aplay == NULL)
		return -1;

	_vol.volume    = (volume > 100) ? 100 : ((volume < 0) ? 0 : volume);
	_vol.right_now = 1;
	_vol.step      = 0;
	return GxAVSetProperty(aplay->dev, aplay->mod,
			GxAudioOutPropertyID_Volume, &_vol, sizeof(GxAudioOutProperty_Volume));
}

status_t GxMediaApi_APlaySetMute(handle_t handle, int mute)
{
	GxAudioOutProperty_Mute _mute;
	GxMediaAPlay *aplay = (GxMediaAPlay *)handle;

	if (aplay == NULL)
		return -1;

	_mute.mute = mute ? 1 :0;
	return GxAVSetProperty(aplay->dev, aplay->mod,
			GxAudioOutPropertyID_Mute, &_mute, sizeof(GxAudioOutProperty_Mute));
}
