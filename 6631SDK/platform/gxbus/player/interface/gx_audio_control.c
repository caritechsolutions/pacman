#include "gx_mediafilter.h"
#include "gx_stream.h"
#include "gx_config.h"
#include "gx_audio_control.h"
#include "gx_decoder.h"
#include "gx_avout.h"
#include "gx_system.h"
#include "gxmedia_api.h"
#include "audio_resample.h"
#include "libsmartvolume.h"

typedef enum {
	AUDIO_DUMPER_ADEC_PCM = 0x0,
	AUDIO_DUMPER_ADEC_ESA,
	AUDIO_DUMPER_AOUT_PCM,
	AUDIO_DUMPER_MAX,
} AudioDumperType;

typedef enum {
	AUDIO_DUMPER_RECORD = (0x1 << 0),
	AUDIO_DUMPER_VOLUME = (0x1 << 1),
} AudioDumperFlag;

typedef struct {
	char                *url;
	unsigned int         url_len;
	unsigned int         url_seq;
	unsigned int         frame_no;
	GxStream            *stream;
	unsigned char       *cache_addr;
	unsigned int         cache_size;
	unsigned int         cache_wptr;
} AudioDumperRecord;

//#define DEBUG_DUMP_RESAMPLER
#ifdef DEBUG_DUMP_RESAMPLER
static const char *debug_dump_resampler_url = "/mnt/usb01/resampler/smart_volume";
#endif
typedef struct {
#define GX_AUDIO_SMART_VOLUME_SAMPLEFRE (16000)
	unsigned int         frame_no;
	void                *resampler;
	unsigned int         src_samplefre;
	unsigned int         src_channels;
	unsigned int         dst_samplefre;
	unsigned int         dst_channels;
	GxSmartVolumeConfig  conf;
	void                *volumer;
	unsigned char       *buffer;
	unsigned int         cur_factor;
	unsigned int         cur_timems;
	unsigned int         dst_timems;
#ifdef DEBUG_DUMP_RESAMPLER
	AudioDumperRecord    record;
#endif
} AudioDumperVolume;

typedef struct {
	AudioDumperType      type;
	unsigned int         running;
	handle_t             thread_id;
	handle_t             mutex;
	AudioDumperFlag      flags;
	AudioDumperRecord    record;
	AudioDumperVolume    volume;
} AudioDumper;

static AudioDumper *_audio_dumper[AUDIO_DUMPER_MAX] = {NULL, NULL, NULL};
static int   _config_vaild    = 0;
static float _config_loudness = 0;

static size_t _dumper_record(AudioDumperRecord *rec, GxMediaApiOneFrame *frame)
{
	size_t size  = 0, wsize = 0, len = 0;
	size_t pos_w = rec->cache_wptr;
	unsigned char *buffer_w = rec->cache_addr;
	unsigned char *wptr = NULL;

	if ((frame != NULL) && (rec->frame_no > frame->frame_no)) {
		if (rec->stream != NULL) {
			if (pos_w > 0)
				len = GxStream_Write(rec->stream, buffer_w, pos_w);
			pos_w = 0;
			rec->cache_wptr = 0;
			GxStream_Close(rec->stream);
			rec->stream = NULL;
			rec->url[rec->url_len] = '\0';
		}
	}

	if (rec->stream == NULL) {
		sprintf(rec->url + rec->url_len, "-%08d.pcm", rec->url_seq);
		rec->stream = GxStream_Open(rec->url, GX_STREAM_TRUNC, 0);
		if (!rec->stream) {
			gxloge("%s file create fail\n", rec->url);
			return -1;
		}
		rec->url_seq++;
	}

	if (frame == NULL) {
		if (pos_w > 0)
			len = GxStream_Write(rec->stream, buffer_w, pos_w);
		rec->cache_wptr = 0;
		return len;
	} else {
		rec->frame_no = frame->frame_no;
		if (buffer_w == NULL) {
			if (frame->frame_size > 0)
				len = GxStream_Write(rec->stream, frame->user_frame_buf, frame->frame_size);
			return len;
		}
		size = frame->frame_size;
	}

	while (size) {
		if (size < (rec->cache_size - pos_w)) {
			memcpy(buffer_w + pos_w, frame->user_frame_buf + frame->frame_size - size, size);
			pos_w += size;
			break;
		} else {
			if (pos_w != 0) {
				memcpy(buffer_w + pos_w, frame->user_frame_buf + frame->frame_size - size, rec->cache_size - pos_w);
				wptr  = buffer_w;
				size -= (rec->cache_size - pos_w);
				pos_w = 0;
			}
			else {
				wptr = frame->user_frame_buf + frame->frame_size - size;
				size -= rec->cache_size;
			}

			len = GxStream_Write(rec->stream, wptr, rec->cache_size);
			if (len != rec->cache_size) {
				gxloge("write ret = %d\n", len);
				return 0;
			}
			wsize += len;
		}
	}
	rec->cache_wptr = pos_w;

	return frame->frame_size;
}

static int _dumper_record_open(AudioDumperRecord *rec, const char *url)
{
	rec->url_seq = 0;
	rec->url_len = strlen(url);
	rec->url     = av_malloc(rec->url_len + 32);
	if (rec->url == NULL) {
		gxloge("malloc fail\n");
		return -1;
	}
	memcpy(rec->url, url, rec->url_len);
	rec->url[rec->url_len] = '\0';

	rec->cache_wptr = 0;
	rec->cache_size = 64*1024;
	rec->cache_addr = av_malloc(rec->cache_size);
	if (rec->cache_addr == NULL) {
		av_free(rec->url);
		gxloge("malloc fail\n");
		return -1;
	}

	return 0;
}

static int _dumper_record_close(AudioDumperRecord *rec)
{
	_dumper_record(rec, NULL);
	if (rec->stream != NULL)
		GxStream_Close(rec->stream);
	rec->stream     = NULL;

	if (rec->cache_addr != NULL)
		av_free(rec->cache_addr);
	rec->cache_addr = NULL;
	rec->cache_wptr = 0;
	rec->cache_size = 0;

	if (rec->url != NULL)
		av_free(rec->url);
	rec->url     = NULL;
	rec->url_len = 0;
	rec->url_seq = 0;

	return 0;
}

static int _dumper_volume(AudioDumperVolume *vol, GxMediaApiOneFrame *frame)
{
	if ((vol->src_samplefre != frame->samplefre) ||
			(vol->src_channels != frame->channels) ||
			(vol->frame_no > frame->frame_no)) {
		if (vol->resampler != NULL) {
			gx_resample_free(vol->resampler);
			vol->resampler = NULL;
		}
		if (vol->volumer != NULL) {
			GxSmartVolumeDestroy(vol->volumer);
			vol->volumer = NULL;
		}
		vol->cur_factor = 100;
		GxPlayer_SystemSet(PSYS_AOUT_VOLUME_FACTOR, &vol->cur_factor);
		vol->cur_timems = 0;
	}

	if (vol->resampler == NULL) {
		vol->src_samplefre = frame->samplefre;
		vol->src_channels  = frame->channels;
		vol->resampler = gx_resample_alloc(
				vol->src_samplefre,
				vol->src_channels,
				vol->dst_samplefre,
				vol->dst_channels, frame->endian, 1);
		if (vol->resampler == NULL) {
			gxloge("resampler: %d %d - %d %d\n",
					vol->src_samplefre,
					vol->src_channels,
					vol->dst_samplefre,
					vol->dst_channels);
			return -1;
		}
	}
	if (vol->volumer == NULL) {
		vol->conf.init_shortterm = (_config_vaild == 0) ? vol->conf.target_loudness : _config_loudness;
		vol->volumer = GxSmartVolumeCreate(&vol->conf, vol->buffer);
		if (vol->volumer == NULL) {
			gxloge("smart volume error\n");
			return -1;
		}
		_config_vaild = 0;
	}

	if ((vol->dst_timems == 0) ||
			(vol->cur_timems < vol->dst_timems)) {
		int ret = 0;
		float gain = 0.0;
		unsigned char *frame_buf = NULL;
		unsigned int   frame_len = 0;
		GXAudioReSampleParam para;

		para.src_buf = frame->user_frame_buf;
		para.src_len = frame->frame_size;
		gx_resample(vol->resampler, &para);
		frame_buf = (unsigned char *)para.dst_buf;
		frame_len = para.dst_len;
		ret = GxSmartVolumeGetGain(vol->volumer, (short *)frame_buf, frame_len/sizeof(short), &gain);
		if (ret == 0) {
			unsigned int factor = (unsigned int)(gain * 100);

#define AUDIO_DUMPER_ADEC_VOLUME_STEP (200)
			if ((factor > vol->cur_factor) &&
					(factor - vol->cur_factor > AUDIO_DUMPER_ADEC_VOLUME_STEP))
				vol->cur_factor = vol->cur_factor + AUDIO_DUMPER_ADEC_VOLUME_STEP;
			else
				vol->cur_factor = factor;
			gxlogd("factor: %d %d\n", vol->cur_factor, factor);
			GxPlayer_SystemSet(PSYS_AOUT_VOLUME_FACTOR, &vol->cur_factor);
		}
#ifdef DEBUG_DUMP_RESAMPLER
		if (1) {
			GxMediaApiOneFrame rframe;

			rframe.frame_no = frame->frame_no;
			rframe.user_frame_buf = frame_buf;
			rframe.frame_size     = frame_len;
			_dumper_record(&vol->record, &rframe);
		}
#endif
		if (vol->dst_samplefre > 0)
			vol->cur_timems += (1000 * (frame_len / sizeof(short)) / vol->dst_samplefre);
	}
	vol->frame_no = frame->frame_no;

	return 0;
}

static void _dumper_thread(void *data)
{
	AudioDumper *dumper = (AudioDumper *)data;
	handle_t handle = 0;

	gxlogd("%s %d\n", __func__, __LINE__);
	if ((dumper->type == AUDIO_DUMPER_ADEC_PCM) ||
			(dumper->type == AUDIO_DUMPER_ADEC_ESA))
		handle = GxMediaApi_AdecOpen();
	else if (dumper->type == AUDIO_DUMPER_AOUT_PCM)
		handle = GxMediaApi_AoutOpen();

	while (dumper->running) {
		int ret = 0;
		GxMediaApiOneFrame frame;

		if (dumper->type == AUDIO_DUMPER_ADEC_PCM) {
			ret = GxMediaApi_AdecPCMFrameAlloc(handle, &frame, 1000);
			if (ret != 0)
				continue;
			GxCore_MutexLock(dumper->mutex);
			if ((dumper->flags & AUDIO_DUMPER_RECORD) == AUDIO_DUMPER_RECORD)
				_dumper_record(&dumper->record, &frame);
			if ((dumper->flags & AUDIO_DUMPER_VOLUME) == AUDIO_DUMPER_VOLUME)
				_dumper_volume(&dumper->volume, &frame);
			GxCore_MutexUnlock(dumper->mutex);
			GxMediaApi_AdecPCMFrameFree(handle, &frame);
		} else if (dumper->type == AUDIO_DUMPER_AOUT_PCM) {
			ret = GxMediaApi_AoutPCMFrameAlloc(handle, &frame, 1000);
			if (ret != 0)
				continue;
			GxCore_MutexLock(dumper->mutex);
			if ((dumper->flags & AUDIO_DUMPER_RECORD) == AUDIO_DUMPER_RECORD)
				_dumper_record(&dumper->record, &frame);
			GxCore_MutexUnlock(dumper->mutex);
			GxMediaApi_AoutPCMFrameFree(handle, &frame);
		} else if (dumper->type == AUDIO_DUMPER_ADEC_ESA) {
			ret = GxMediaApi_AdecESAFrameAlloc(handle, &frame, 1000);
			if (ret != 0)
				continue;
			GxCore_MutexLock(dumper->mutex);
			if ((dumper->flags & AUDIO_DUMPER_RECORD) == AUDIO_DUMPER_RECORD)
				_dumper_record(&dumper->record, &frame);
			GxCore_MutexUnlock(dumper->mutex);
			GxMediaApi_AdecESAFrameFree(handle, &frame);
		}
	}

	if ((dumper->type == AUDIO_DUMPER_ADEC_PCM) ||
			(dumper->type == AUDIO_DUMPER_ADEC_ESA))
		GxMediaApi_AdecClose(handle);
	else if (dumper->type == AUDIO_DUMPER_AOUT_PCM)
		GxMediaApi_AoutClose(handle);
	gxlogd("%s %d\n", __func__, __LINE__);

	return;
}

static int _audio_dumper_enable(AudioDumperType type, void *conf, AudioDumperFlag flag)
{
	int thread_flag = 0;
	AudioDumper *dumper = _audio_dumper[type];

	gxlogd("%s %d\n", __func__, __LINE__);
	if (dumper == NULL) {
		dumper = av_mallocz(sizeof(AudioDumper));
		if (dumper == NULL) {
			gxloge("malloc err\n");
			return -1;
		}
		GxCore_MutexCreate(&(dumper->mutex));
		dumper->type = type;
		_audio_dumper[type] = dumper;
	}

	GxCore_MutexLock(dumper->mutex);
	if (((flag & AUDIO_DUMPER_RECORD) == AUDIO_DUMPER_RECORD) &&
			(dumper->flags & AUDIO_DUMPER_RECORD) != AUDIO_DUMPER_RECORD) {
		_dumper_record_open(&dumper->record, (const char*)conf);
		dumper->flags |= AUDIO_DUMPER_RECORD;
	}

	if (((flag & AUDIO_DUMPER_VOLUME) == AUDIO_DUMPER_VOLUME) &&
		   (dumper->flags & AUDIO_DUMPER_VOLUME) != AUDIO_DUMPER_VOLUME) {
		AudioSmartConfig *config = (AudioSmartConfig *)conf;
		float      loudness = (config == NULL) ? -16.0 : config->target_loudness;
		unsigned int timems = (config == NULL) ?     0 : config->target_timems;

		dumper->volume.dst_timems = (config == NULL) ? 0 : config->target_timems;
		dumper->volume.cur_timems = 0;
		dumper->volume.frame_no = 0;
		dumper->volume.src_samplefre = 0;
		dumper->volume.src_channels  = 0;
		dumper->volume.dst_samplefre = GX_AUDIO_SMART_VOLUME_SAMPLEFRE;
		dumper->volume.dst_channels  = 1;
		dumper->volume.conf.sample_rate     = GX_AUDIO_SMART_VOLUME_SAMPLEFRE;
		dumper->volume.conf.target_loudness = loudness;//调整到合适音量
		dumper->volume.conf.target_lra      = 10;
		dumper->volume.conf.target_tp       = 0;
		dumper->volume.conf.init_frame_num  = 5;
		dumper->volume.conf.init_shortterm  = loudness;
		dumper->volume.buffer = av_malloc(GxSmartVolumeMemorySize(&dumper->volume.conf));
		if (dumper->volume.buffer == NULL)
			gxloge("malloc failed\n");
#ifdef DEBUG_DUMP_RESAMPLER
		_dumper_record_open(&dumper->volume.record, debug_dump_resampler_url);
#endif
		dumper->volume.cur_factor = 100;
		GxPlayer_SystemSet(PSYS_AOUT_VOLUME_FACTOR, &dumper->volume.cur_factor);
		dumper->flags |= AUDIO_DUMPER_VOLUME;
	}

	if (dumper->running == 0) {
		dumper->running = 1;
		thread_flag = 1;
	}
	GxCore_MutexUnlock(dumper->mutex);

	if (thread_flag == 1) {
		GxCore_ThreadCreate("audio.control",
				&dumper->thread_id,
				_dumper_thread,
				dumper, 32*1024,
				GXOS_DEFAULT_PRIORITY - 1);
	}
	gxlogd("%s %d\n", __func__, __LINE__);

	return 0;
}

static int _audio_dumper_disable(AudioDumperType type, AudioDumperFlag flag)
{
	int thread_flag = 1;
	AudioDumper *dumper = _audio_dumper[type];

	gxlogd("%s %d\n", __func__, __LINE__);
	if (dumper == NULL) {
		gxloge("dumper not exist\n");
		return -1;
	}

	GxCore_MutexLock(dumper->mutex);
	if (((flag & AUDIO_DUMPER_RECORD) == AUDIO_DUMPER_RECORD) &&
			((dumper->flags & AUDIO_DUMPER_RECORD) == AUDIO_DUMPER_RECORD)) {
		_dumper_record_close(&dumper->record);
		dumper->flags &= ~AUDIO_DUMPER_RECORD;
	}

	if (((flag & AUDIO_DUMPER_VOLUME) == AUDIO_DUMPER_VOLUME) &&
			((dumper->flags & AUDIO_DUMPER_VOLUME) == AUDIO_DUMPER_VOLUME)) {
		if (dumper->volume.resampler != NULL) {
			gx_resample_free(dumper->volume.resampler);
			dumper->volume.resampler = NULL;
		}
		if (dumper->volume.volumer != NULL) {
			GxSmartVolumeDestroy(dumper->volume.volumer);
			dumper->volume.volumer = NULL;
		}
		if (dumper->volume.buffer != NULL) {
			av_free(dumper->volume.buffer);
			dumper->volume.buffer = NULL;
		}
#ifdef DEBUG_DUMP_RESAMPLER
		_dumper_record_close(&dumper->volume.record);
#endif
		dumper->volume.cur_factor = 100;
		GxPlayer_SystemSet(PSYS_AOUT_VOLUME_FACTOR, &dumper->volume.cur_factor);
		dumper->flags &= ~AUDIO_DUMPER_VOLUME;
	}

	if ((dumper->flags & (AUDIO_DUMPER_RECORD | AUDIO_DUMPER_VOLUME)) == 0) {
		if (dumper->running)
			thread_flag = 0;
		dumper->running = 0;
	}
	GxCore_MutexUnlock(dumper->mutex);

	if (thread_flag == 0) {
		GxCore_ThreadJoin(dumper->thread_id);
		dumper->thread_id = 0;
		av_free(dumper);
		_audio_dumper[type] = NULL;
	}
	gxlogd("%s %d\n", __func__, __LINE__);

	return 0;
}

status_t gx_audio_record_adec_pcm(int enable, char *url)
{
	if (enable)
		return _audio_dumper_enable(AUDIO_DUMPER_ADEC_PCM, url, AUDIO_DUMPER_RECORD);
	else
		return _audio_dumper_disable(AUDIO_DUMPER_ADEC_PCM, AUDIO_DUMPER_RECORD);
}

status_t gx_audio_record_adec_esa(int enable, char *url)
{
	if (enable)
		return _audio_dumper_enable(AUDIO_DUMPER_ADEC_ESA, url, AUDIO_DUMPER_RECORD);
	else
		return _audio_dumper_disable(AUDIO_DUMPER_ADEC_ESA, AUDIO_DUMPER_RECORD);
}

status_t gx_audio_record_aout_pcm(int enable, char *url)
{
	if (enable)
		return _audio_dumper_enable(AUDIO_DUMPER_AOUT_PCM, url, AUDIO_DUMPER_RECORD);
	else
		return _audio_dumper_disable(AUDIO_DUMPER_AOUT_PCM, AUDIO_DUMPER_RECORD);
}

status_t gx_audio_smart_volume(int enable, AudioSmartConfig *config)
{
	if (enable)
		return _audio_dumper_enable(AUDIO_DUMPER_ADEC_PCM, config, AUDIO_DUMPER_VOLUME);
	else
		return _audio_dumper_disable(AUDIO_DUMPER_ADEC_PCM, AUDIO_DUMPER_VOLUME);
}

status_t gx_audio_smart_volume_set_loudness(float loudness)
{
	AudioDumper *dumper = _audio_dumper[AUDIO_DUMPER_ADEC_PCM];

	gxlogd("%s %d\n", __func__, __LINE__);
	if (dumper == NULL) {
		_config_vaild    = 1;
		_config_loudness = loudness;
		return -1;
	}

	GxCore_MutexLock(dumper->mutex);
	_config_vaild    = 1;
	_config_loudness = loudness;
	GxCore_MutexUnlock(dumper->mutex);

	return 0;
}

status_t gx_audio_smart_volume_get_loudness(float *loudness)
{
	AudioDumper *dumper = _audio_dumper[AUDIO_DUMPER_ADEC_PCM];

	gxlogd("%s %d\n", __func__, __LINE__);
	if ((dumper == NULL) || (loudness == NULL)) {
		gxloge("get loudness error\n");
		return -1;
	}

	GxCore_MutexLock(dumper->mutex);
	if (dumper->volume.volumer == NULL) {
		gxloge("volumer not exist\n");
		GxCore_MutexUnlock(dumper->mutex);
		return -1;
	}
	*loudness = GxSmartVolumeGetLoudness(dumper->volume.volumer);
	GxCore_MutexUnlock(dumper->mutex);

	return -1;
}
