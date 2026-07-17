#include "gxmedia_api.h"
#include "gxmedia_module.h"
#include "gxmedia_common.h"

typedef struct {
	int       dev;
	int       mod;
	int       fifotype;
	int       volumedb;
	int       channels;
	int       mute_en;
	unsigned int   kern_base_addr;
	unsigned char *user_base_addr;
	unsigned int   kern_base_size;
} GxMediaAdec;
#define MIN_VOLUME (32)

static unsigned char *resample_filter_addr = NULL;
static unsigned int   resample_filter_size = 0;

static int _adec_alloc_one_frame(GxMediaAdec *adec, GxMediaApiOneFrame *frame, int fifotype, int timeout_ms)
{
	int ret = -1;
	GxAudioDecProperty_DumpFrame one_frame;
	int wait_event = 0;

	if (adec->fifotype == -1) {
		GxAudioDecProperty_DumpConfig conf;

		conf.fifotype = fifotype;
		conf.resample_table_addr = resample_filter_addr;
		conf.resample_table_size = resample_filter_size;
		conf.i_volumedb = adec->mute_en ? MIN_VOLUME : adec->volumedb;
		conf.i_channels = adec->channels;
		GxAVSetProperty(adec->dev, adec->mod,
				GxAudioDecoderPropertyID_DumpEnable, &conf, sizeof(GxAudioDecProperty_DumpConfig));
		adec->fifotype = fifotype;
	}

	switch (fifotype) {
	case AUDIO_DEC_PCM_OFIFO:
		wait_event = EVENT_AUDIODEC_PCM_FRAME_0;
		break;
	case AUDIO_DEC_ESA_IFIFO:
		wait_event = EVENT_AUDIODEC_ESA_FRAME;
		break;
	case AUDIO_DEC_PCM_OADSP:
		wait_event = EVENT_AUDIODEC_PCM_FRAME_1;
		break;
	case AUDIO_DEC_ESA_OFIFO:
		wait_event = EVENT_AUDIODEC_ESA_FRAME_1;
		break;
	default:
		break;
	}

	one_frame.fifotype   = adec->fifotype;
	one_frame.i_volumedb = adec->mute_en ? MIN_VOLUME : adec->volumedb;
	one_frame.i_channels = adec->channels;
	ret = GxAVGetProperty(adec->dev, adec->mod,
			GxAudioDecoderPropertyID_DumpAlloc, &one_frame, sizeof(GxAudioDecProperty_DumpFrame));
	if (ret != 0) {
		unsigned int event = 0;

		if (timeout_ms != 0) {
			int retval = GXCORE_SUCCESS;
			int timeout_us = (timeout_ms > 0) ? (timeout_ms * 1000) : -1;

wait_one_frame:
			retval = GxAVWaitEvents(adec->dev, adec->mod, wait_event, timeout_us, &event);
			if ((GXCORE_SUCCESS == retval) &&
					(event & wait_event)) {
				one_frame.i_volumedb = adec->mute_en ? MIN_VOLUME : adec->volumedb;
				one_frame.i_channels = adec->channels;
				ret = GxAVGetProperty(adec->dev, adec->mod,
						GxAudioDecoderPropertyID_DumpAlloc, &one_frame, sizeof(GxAudioDecProperty_DumpFrame));
				if (ret != 0) {
					goto wait_one_frame;
				}
			}
		}
	}
	if (ret == 0) {
		if (adec->user_base_addr == NULL) {
			adec->kern_base_addr = (unsigned int)one_frame.k_baseaddr;
			adec->kern_base_size = one_frame.k_basesize;
			adec->user_base_addr = (unsigned char *)GxCore_Map(adec->dev, adec->kern_base_addr, adec->kern_base_size);
		}
		frame->kern_frame_buf = one_frame.buf_addr;
		frame->user_frame_buf = adec->user_base_addr + ((unsigned int)one_frame.buf_addr - adec->kern_base_addr);
		frame->frame_size = one_frame.buf_size;
		frame->samplefre  = one_frame.samplefre;
		frame->channels   = one_frame.channels;
		frame->bitwidth   = one_frame.bitwidth;
		frame->endian     = one_frame.endian;
		frame->interlace  = one_frame.interlace;
		frame->frame_no   = one_frame.frame_no;
		frame->sample_val = one_frame.sample_val;
		frame->volume_db  = one_frame.volume_db;
		frame->down_mix   = one_frame.down_mix;
		frame->codectype  = acodec_hw2std(one_frame.codectype);
	}

	return ret;
}

static int _adec_free_one_frame(GxMediaAdec *adec, GxMediaApiOneFrame *frame)
{
	GxAudioDecProperty_DumpFrame pcm_frame;

	pcm_frame.fifotype  = adec->fifotype;
	pcm_frame.buf_addr  = frame->kern_frame_buf;
	pcm_frame.buf_size  = frame->frame_size;
	pcm_frame.samplefre = frame->samplefre;
	pcm_frame.channels  = frame->channels;
	pcm_frame.bitwidth  = frame->bitwidth;
	pcm_frame.endian    = frame->endian;
	pcm_frame.interlace = frame->interlace;
	pcm_frame.frame_no  = frame->frame_no;

	return GxAVSetProperty(adec->dev, adec->mod,
			GxAudioDecoderPropertyID_DumpFree, &pcm_frame, sizeof(GxAudioDecProperty_DumpFrame));
}

static unsigned int _adec_get_version(void)
{
	int dev = GxAvdev_CreateDevice(0);
	int mod = GxAvdev_OpenModule(dev, GXAV_MOD_AUDIO_DECODE, 0);
	GxADSPProperty_Control actrl;

	actrl.cmd  = GX_ADSP_CONTROL_GET_VERSION;
	actrl.arg1 = 0;
	GxAVGetProperty(dev, mod, GxADSPPropertyID_Control, &actrl, sizeof(GxADSPProperty_Control));
	return (unsigned int)actrl.arg1;
}

handle_t GxMediaApi_AdecOpen(void)
{
	GxMediaAdec *adec = av_mallocz(sizeof(GxMediaAdec));

	adec->dev = GxAvdev_CreateDevice(0);
	CHECK_RETURN_VALUE(adec->dev);

	adec->mod = GxAvdev_OpenModule(adec->dev, GXAV_MOD_AUDIO_DECODE, 0);
	CHECK_RETURN_VALUE(adec->mod);

	adec->fifotype = -1;
	adec->volumedb =  0;
	adec->channels =  2;
	adec->mute_en  =  0;

	return (handle_t)adec;
}

status_t GxMediaApi_AdecClose(handle_t handle)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	if (adec->fifotype != -1) {
		GxAudioDecProperty_DumpConfig conf;

		conf.fifotype = adec->fifotype;
		GxAVSetProperty(adec->dev, adec->mod,
				GxAudioDecoderPropertyID_DumpDisable, &conf, sizeof(GxAudioDecProperty_DumpConfig));
	}

	if (adec->user_base_addr != NULL) {
		GxCore_UnMap(adec->dev, adec->user_base_addr, adec->kern_base_size);
		adec->user_base_addr = NULL;
		adec->kern_base_addr = 0;
		adec->kern_base_size = 0;
	}

	if (adec->mod != -1)
		GxAvdev_CloseModule(adec->dev, adec->mod);

	if (adec->dev != -1)
		GxAvdev_DestroyDevice(adec->dev);

	av_free(adec);

	return 0;
}

status_t GxMediaApi_AdecPCMFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	return _adec_alloc_one_frame(adec, frame, AUDIO_DEC_PCM_OFIFO, timeout_ms);
}

status_t GxMediaApi_AdecPCMFrameFree(handle_t handle, GxMediaApiOneFrame *frame)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	return _adec_free_one_frame(adec, frame);
}

status_t GxMediaApi_AdecESAFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	return _adec_alloc_one_frame(adec, frame, AUDIO_DEC_ESA_IFIFO, timeout_ms);
}

status_t GxMediaApi_AdecESAFrameFree(handle_t handle, GxMediaApiOneFrame *frame)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	return _adec_free_one_frame(adec, frame);
}

void GxMediaApi_AdecResampleRegister(void)
{
	extern unsigned char _resample_filter_441_buf[16*1176];
	if (_adec_get_version() >= 0x00020000) {
		gxloge("chip unspport register 44.kHz resample param\n");
		return;
	}
	resample_filter_addr = (unsigned char *)_resample_filter_441_buf;
	resample_filter_size = sizeof(_resample_filter_441_buf);
}

void GxMediaApi_AdecResampleAllRegister(void)
{
	extern short _resample_filter_all_buf[1024*16];
	if (_adec_get_version() < 0x00020000) {
		gxloge("chip unspport register all samplefreq resample param\n");
		return;
	}
	resample_filter_addr = (unsigned char *)_resample_filter_all_buf;
	resample_filter_size = sizeof(_resample_filter_all_buf);
}

status_t GxMediaApi_AdecResamplePCMFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	if (resample_filter_addr == NULL)
		return -1;

	return _adec_alloc_one_frame(adec, frame, AUDIO_DEC_PCM_OADSP, timeout_ms);
}

status_t GxMediaApi_AdecResamplePCMFrameFree(handle_t handle, GxMediaApiOneFrame *frame)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	return _adec_free_one_frame(adec, frame);
}

status_t GxMediaApi_AdecResampleSetChannels(handle_t handle, int channels)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	adec->channels = ((channels > 2) ? 2 : ((channels == 0) ? 1 : channels));

	return 0;
}

status_t GxMediaApi_AdecResampleSetVolume(handle_t handle, int volume)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	volume = (volume >= 100) ? 100 : volume;
	volume = (volume <=   0) ?   0 : volume;
	adec->volumedb = MIN_VOLUME - ((volume * MIN_VOLUME) / 100);

	return 0;
}

status_t GxMediaApi_AdecResampleSetMute(handle_t handle, int mute)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	adec->mute_en = mute ? 1 : 0;

	return 0;
}

status_t GxMediaApi_AdecDecESAFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	return _adec_alloc_one_frame(adec, frame, AUDIO_DEC_ESA_OFIFO, timeout_ms);
}

status_t GxMediaApi_AdecDecESAFrameFree(handle_t handle, GxMediaApiOneFrame *frame)
{
	GxMediaAdec *adec = (GxMediaAdec *)handle;

	if (adec == NULL)
		return -1;

	return _adec_free_one_frame(adec, frame);
}
