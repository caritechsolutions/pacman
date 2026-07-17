#include "gxmedia_api.h"
#include "gxmedia_module.h"
#include "gxmedia_common.h"
#include "audio_resample.h"

typedef struct {
	int       dev;
	int       mod;
} GxMediaAout;

void GxMediaApi_AoutResampleRegister(void)
{
	gx_resample_register();
}

handle_t GxMediaApi_AoutOpen(void)
{
	GxMediaAout *aout = av_mallocz(sizeof(GxMediaAout));
	GxAudioOutProperty_DumpPinID pin;

	aout->dev = GxAvdev_CreateDevice(0);
	CHECK_RETURN_VALUE(aout->dev);

	aout->mod = GxAvdev_OpenModule(aout->dev, GXAV_MOD_AUDIO_OUT, 0);
	CHECK_RETURN_VALUE(aout->mod);

	pin.fifotype = AUDIO_OUT_PCM_OFIFO;
	GxAVSetProperty(aout->dev, aout->mod,
			GxAudioOutPropertyID_DumpEnable, &pin, sizeof(GxAudioOutProperty_DumpPinID));

	return (handle_t)aout;
}

status_t GxMediaApi_AoutPCMFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms)
{
	int ret = -1;
	GxMediaAout *aout = (GxMediaAout *)handle;
	GxAudioOutProperty_DumpFrame pcm_frame;

	if (aout == NULL)
		return -1;

	pcm_frame.fifotype = AUDIO_OUT_PCM_OFIFO;
	ret = GxAVGetProperty(aout->dev, aout->mod,
			GxAudioOutPropertyID_DumpAlloc, &pcm_frame, sizeof(GxAudioOutProperty_DumpFrame));
	if (ret != 0) {
		unsigned int event = 0;

		if (timeout_ms != 0) {
			int retval = GXCORE_SUCCESS;
			int timeout_us = (timeout_ms > 0) ? (timeout_ms * 1000) : -1;

wait_pcm_frame:
			retval = GxAVWaitEvents(aout->dev, aout->mod, EVENT_AUDIOOUT_PCM_FRAME, timeout_us, &event);
			if ((GXCORE_SUCCESS == retval) &&
					(event & EVENT_AUDIOOUT_PCM_FRAME)) {
				ret = GxAVGetProperty(aout->dev, aout->mod,
						GxAudioOutPropertyID_DumpAlloc, &pcm_frame, sizeof(GxAudioOutProperty_DumpFrame));
				if (ret != 0) {
					goto wait_pcm_frame;
				}
			}
		}
	}
	if (ret == 0) {
		frame->kern_frame_buf = pcm_frame.buf_addr;
		frame->user_frame_buf = (unsigned char *)GxCore_Map(aout->dev,
				(unsigned int)pcm_frame.buf_addr, pcm_frame.buf_size);
		frame->frame_size = pcm_frame.buf_size;
		frame->samplefre  = pcm_frame.samplefre;
		frame->channels   = pcm_frame.channels;
		frame->bitwidth   = pcm_frame.bitwidth;
		frame->endian     = pcm_frame.endian;
		frame->interlace  = pcm_frame.interlace;
		frame->codectype  = AUDIO_CODEC_PCM;
	}

	return ret;
}

status_t GxMediaApi_AoutPCMFrameFree(handle_t handle, GxMediaApiOneFrame *frame)
{
	GxMediaAout *aout = (GxMediaAout *)handle;
	GxAudioOutProperty_DumpFrame pcm_frame;

	if (aout == NULL)
		return -1;

	pcm_frame.fifotype  = AUDIO_OUT_PCM_OFIFO;
	pcm_frame.buf_addr  = frame->kern_frame_buf;
	pcm_frame.buf_size  = frame->frame_size;
	pcm_frame.samplefre = frame->samplefre;
	pcm_frame.channels  = frame->channels;
	pcm_frame.bitwidth  = frame->bitwidth;
	pcm_frame.endian    = frame->endian;
	pcm_frame.interlace = frame->interlace;

	return GxAVSetProperty(aout->dev, aout->mod,
			GxAudioOutPropertyID_DumpFree, &pcm_frame, sizeof(GxAudioOutProperty_DumpFrame));
}

status_t GxMediaApi_AoutClose(handle_t handle)
{
	GxMediaAout *aout = (GxMediaAout *)handle;
	GxAudioOutProperty_DumpPinID pin;

	pin.fifotype = AUDIO_OUT_PCM_OFIFO;
	GxAVSetProperty(aout->dev, aout->mod,
			GxAudioOutPropertyID_DumpDisable, &pin, sizeof(GxAudioOutProperty_DumpPinID));
	if (aout->mod != -1)
		GxAvdev_CloseModule(aout->dev, aout->mod);

	if (aout->dev != -1)
		GxAvdev_DestroyDevice(aout->dev);

	av_free(aout);

	return 0;
}
