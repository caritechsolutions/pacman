#include "gxmedia_api.h"
#include "gxaudio_dongle.h"
#include "gxplayer_module.h"
#include "gx_mem.h"
#include "audio_resample.h"

//#define DEBUG_AUDIO_DONGLE_RECORD
typedef enum {
	GX_AUDIO_DONGLE_SOURCE = (0x1 << 0),
	GX_AUDIO_DONGLE_SINK   = (0x1 << 1),
} GxAudioDongleType;

typedef enum {
	GX_AUDIO_DONGLE_RES_PCM = 0, //dongle 的后缀 "dec.pcm.frame"
	GX_AUDIO_DONGLE_DEC_CDD = 1, //dongle 的后缀 "dec.cdd.frame"
	GX_AUDIO_DONGLE_OUT_PCM = 2, //dongle 的后缀 "out.pcm.frame"
} GxAudioDongleDataType;

typedef struct {
	GxAudioDongleType     type;
	GxAudioDongleDataType data_type;
	handle_t              source_handle;
	handle_t              sink_handle;
	int                   sink_sync;
	GxAudioDongleCap      cap;
	GxAudioDongleParam    param;
	int                   thread_running;
	int                   thread_id;
	unsigned char        *rbuf;
	unsigned int          rsize;
#ifdef DEBUG_AUDIO_DONGLE_RECORD
	unsigned char        *rec_buf;
	unsigned int          rec_size;
	unsigned int          rec_pos;
#endif
	int                   need_resample;
	int                   resample_freq;
	void                 *resampler;
} GxAudioDongleCtx;

typedef struct {
	GxAudioDongleOps *ops;
	GxAudioDongleCtx *ctx;
} GxAudioDongle;

static GxAudioDongle dongles[] = {
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
};

static GxAudioDongle* _find_dongle(const char *name)
{
	unsigned int i = 0;

	if (!name) return NULL;
	for (i = 0; i < sizeof(dongles)/sizeof(dongles[0]); i++) {
		if (dongles[i].ops && (!strcmp(name, dongles[i].ops->name))) {
			return &dongles[i];
		}
	}

	return NULL;
}

static int _add_dongle_ops(GxAudioDongleOps *ops)
{
	unsigned int i = 0;
	GxAudioDongle *dongle = _find_dongle(ops->name);

	if (dongle) return -1;
	for (i = 0; i < sizeof(dongles)/sizeof(dongles[0]); i++) {
		if (!dongles[i].ops && !dongles[i].ctx)
			break;
	}

	if (i >= sizeof(dongles)/sizeof(dongles[0]))
		return -1;

	dongles[i].ops = ops;
	return 0;
}

static int _del_dongle_ops(GxAudioDongleOps *ops)
{
	GxAudioDongle *dongle = _find_dongle(ops->name);

	if (!dongle || dongle->ctx) return -1;

	dongle->ops = NULL;
	return 0;
}

static int _get_dongle_datatype(GxAudioDongle *dongle)
{
	if (dongle->ops->private_name) {
		if (strcmp(dongle->ops->private_name, GX_AUDIO_DONGLE_DEC_CDD_FRAME_NAME) == 0)
			return GX_AUDIO_DONGLE_DEC_CDD;

		if (strcmp(dongle->ops->private_name, GX_AUDIO_DONGLE_OUT_PCM_FRAME_NAME) == 0)
			return GX_AUDIO_DONGLE_OUT_PCM;
	}

	return GX_AUDIO_DONGLE_RES_PCM;
}

static void _dongle_push(GxAudioDongle *dongle)
{
	GxAudioDongleCtx *ctx = dongle->ctx;
	GxAudioDongleOps *ops = dongle->ops;
	GxMediaApiOneFrame frame;
	int ret = 0;
	unsigned char *frame_buf = NULL;
	unsigned int   frame_len = 0;

	if (ctx->data_type == GX_AUDIO_DONGLE_RES_PCM)
		ret = GxMediaApi_AdecResamplePCMFrameAlloc(ctx->source_handle, &frame, 100);
	else if (ctx->data_type == GX_AUDIO_DONGLE_DEC_CDD)
		ret = GxMediaApi_AdecDecESAFrameAlloc(ctx->source_handle, &frame, 100);
	else if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
		ret = GxMediaApi_AoutPCMFrameAlloc(ctx->source_handle, &frame, 100);
	}
	if (ret != 0) return;

	if (ctx->need_resample) {
		GXAudioReSampleParam para;

		if (ctx->resample_freq != frame.samplefre) {
			if (ctx->resampler) {
				gx_resample_free(ctx->resampler);
				ctx->resampler = NULL;
			}
			ctx->resample_freq = frame.samplefre;
		}
		if (ctx->resampler == NULL) {
			ctx->resampler = gx_resample_alloc(frame.samplefre,
					frame.channels,
					ctx->cap.pcm.sample_freq,
					ctx->cap.pcm.channel_num, frame.endian, 0);
			if (ctx->resampler == NULL) {
				gxlogi("resampler: %d %d - %d %d\n",
						frame.samplefre,
						frame.channels,
						ctx->cap.pcm.sample_freq,
						ctx->cap.pcm.channel_num);
				return;
			}
		}
		para.src_buf = frame.user_frame_buf;
		para.src_len = frame.frame_size;
		gx_resample(ctx->resampler, &para);
		frame_buf = (unsigned char *)para.dst_buf;
		frame_len = para.dst_len;
	} else {
		frame_buf = frame.user_frame_buf;
		frame_len = frame.frame_size;
	}

	if (ops && ops->push) {
		GxAudioDongleData data;

		data.buf_ptr    = frame_buf;
		data.buf_size   = frame_len;
		data.codec      = frame.codectype;
		data.frame_no   = frame.frame_no;
		data.sample_val = frame.sample_val;
		data.down_mix   = frame.down_mix;
		data.volume_db  = frame.volume_db;
		if (data.codec == AUDIO_CODEC_PCM) {
			if (ctx->cap.pcm.endian == 0) {
				unsigned int i = 0;
				for (i = 0; i < (data.buf_size / 4); i++) {
					unsigned char xswap = 0;

					if (ctx->param.channel == GXAUDIO_DONGLE_STEREO_CHANNEL) {
						xswap = data.buf_ptr[i * 4 + 1];
						data.buf_ptr[i * 4 + 1] = data.buf_ptr[i * 4 + 0];
						data.buf_ptr[i * 4 + 0] = xswap;
						xswap = data.buf_ptr[i * 4 + 3];
						data.buf_ptr[i * 4 + 3] = data.buf_ptr[i * 4 + 2];
						data.buf_ptr[i * 4 + 2] = xswap;
					} else if (ctx->param.channel == GXAUDIO_DONGLE_LEFT_CHANNEL) {
						xswap = data.buf_ptr[i * 4 + 1];
						data.buf_ptr[i * 4 + 1] = data.buf_ptr[i * 4 + 0];
						data.buf_ptr[i * 4 + 0] = xswap;
						data.buf_ptr[i * 4 + 3] = data.buf_ptr[i * 4 + 0];
						data.buf_ptr[i * 4 + 2] = xswap;
					} else if (ctx->param.channel == GXAUDIO_DONGLE_RIGHT_CHANNEL) {
						xswap = data.buf_ptr[i * 4 + 3];
						data.buf_ptr[i * 4 + 3] = data.buf_ptr[i * 4 + 2];
						data.buf_ptr[i * 4 + 2] = xswap;
						data.buf_ptr[i * 4 + 1] = data.buf_ptr[i * 4 + 2];
						data.buf_ptr[i * 4 + 0] = xswap;
					}
				}
			}
		}
#ifdef DEBUG_AUDIO_DONGLE_RECORD
		if ((ctx->rec_pos + data.buf_size) <= ctx->rec_size) {
			memcpy(ctx->rec_buf + ctx->rec_pos, data.buf_ptr, data.buf_size);
			ctx->rec_pos += data.buf_size;
			gxlogi("%d %d %d %d %d: %p %d\n",
					frame.samplefre,
					frame.channels,
					frame.bitwidth,
					frame.interlace,
					frame.endian,
					ctx->rec_buf, ctx->rec_pos);
		}
#endif
		ops->push(&data);
	}

	if (ctx->data_type == GX_AUDIO_DONGLE_RES_PCM)
		GxMediaApi_AdecResamplePCMFrameFree(ctx->source_handle, &frame);
	else if (ctx->data_type == GX_AUDIO_DONGLE_DEC_CDD)
		GxMediaApi_AdecDecESAFrameFree(ctx->source_handle, &frame);
	else if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM)
		GxMediaApi_AoutPCMFrameFree(ctx->source_handle, &frame);

	return;
}

static void _dongle_pull(GxAudioDongle *dongle)
{
	GxAudioDongleCtx *ctx = dongle->ctx;
	GxAudioDongleOps *ops = dongle->ops;
	GxAudioDongleData data;
	unsigned int copy_size = 0;
	int recv_size = 0;
    GxMediaApiOneFrame frame;

    if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
        int ret = 0;

        memset(&frame, 0, sizeof(GxMediaApiOneFrame));
        frame.samplefre = ctx->cap.pcm.sample_freq;
        frame.channels  = ctx->cap.pcm.channel_num;
        frame.bitwidth  = ctx->cap.pcm.bits;
        frame.interlace = ctx->cap.pcm.interlace;
        frame.endian    = ctx->cap.pcm.endian;
        ret = GxMediaApi_APlayGetPCMFrame(ctx->sink_handle, &frame, 0);
		if (ret < 0) {
			GxCore_ThreadDelay(10);
			return;
		}
    }

	if (ctx->cap.keep_pulse && (ctx->sink_sync == 0)) {//首次清空数据
		GxAudioDongleBufInfo buf;

		buf.datalen = 0;
		buf.freelen = 0;
		if (ops->get_bufinfo)
			ops->get_bufinfo(&buf);
		while (buf.datalen > 0) {
			data.buf_size = (ctx->rsize >= buf.datalen) ? buf.datalen : ctx->rsize;
			data.buf_ptr  = ctx->rbuf;
			if (ops->pull)
				recv_size = ops->pull(&data);
			if (recv_size <= 0)
				break;
			buf.datalen -= recv_size;
		}
		ctx->sink_sync = 1;
	}

	if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
        int data_len = 0;

		while (ctx->thread_running) {
			data.buf_ptr  = frame.user_frame_buf + data_len;
			data.buf_size = frame.frame_size     - data_len;
			if (ops->pull)
				recv_size = ops->pull(&data);
            if (recv_size < 0) {
			    GxCore_ThreadDelay(10);
                return;
            } else if (recv_size == 0) {
				GxCore_ThreadDelay(10);
				continue;
			}
			data_len += recv_size;
            if (data_len >= frame.frame_size) {
                recv_size     = frame.frame_size;
                data.buf_ptr  = frame.user_frame_buf;
                data.buf_size = frame.frame_size;
                break;
            }
		}
	} else {
		data.buf_size = ctx->rsize;
		data.buf_ptr  = ctx->rbuf;
		if (ops->pull)
			recv_size = ops->pull(&data);

		if (recv_size <= 0) {
			GxCore_ThreadDelay(10);
			return;
		}
	}

#ifdef DEBUG_AUDIO_DONGLE_RECORD
	if ((ctx->rec_pos + recv_size) <= ctx->rec_size) {
		memcpy(ctx->rec_buf + ctx->rec_pos, data.buf_ptr, recv_size);
		ctx->rec_pos += recv_size;
		gxlogi("%p %d %d\n",ctx->rec_buf, ctx->rec_pos, recv_size);
	}
#endif
	if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
		GxMediaApi_APlayPushPCMFrame(ctx->sink_handle, &frame);
	} else {
		while (ctx->thread_running) {
			int len = GxMediaApi_AudioInjectData(ctx->sink_handle,
					GXMEDIA_SOURCE_ES,
					data.buf_ptr + copy_size,
					recv_size - copy_size, -1);
			if (len < 0)
				break;
			else if (len == 0)
				GxCore_ThreadDelay(10);
			copy_size += len;
			if (copy_size >= recv_size)
				break;
		}
	}

	return;
}

static void _dongle_thread(void *data)
{
	GxAudioDongle *dongle = (GxAudioDongle *)data;
	GxAudioDongleCtx *ctx = dongle->ctx;

#ifdef DEBUG_AUDIO_DONGLE_RECORD
	ctx->rec_size = 4*1024*1024;
	ctx->rec_buf  = av_malloc(ctx->rec_size);
	ctx->rec_pos  = 0;
#endif
	while (ctx->thread_running) {
		if (ctx->type & GX_AUDIO_DONGLE_SOURCE)
			_dongle_push(dongle);
		if (ctx->type & GX_AUDIO_DONGLE_SINK)
			_dongle_pull(dongle);
	}
#ifdef DEBUG_AUDIO_DONGLE_RECORD
	ctx->rec_size = 0;
	if (ctx->rec_buf)
		av_free(ctx->rec_buf);
	ctx->rec_buf  = NULL;
	ctx->rec_pos  = 0;
#endif

	return;
}

int GxAudioRegisterDongle(GxAudioDongleOps *ops)
{
	if ((!ops) || (!ops->name))
		return -1;

	return _add_dongle_ops(ops);
}

int GxAudioUnRegisterDongle(GxAudioDongleOps *ops)
{
	if ((!ops) || (!ops->name))
		return -1;

	return _del_dongle_ops(ops);
}

int GxAudioDongleEnable(const char *name, GxAudioDongleParam *param)
{
	GxAudioDongle *dongle = _find_dongle(name);
	GxAudioDongleCtx *ctx = NULL;
	GxAudioDongleOps *ops = NULL;

	if (!dongle || dongle->ctx || !dongle->ops) {
		gxloge("Audio Dongle [%s] Not Find\n", name);
		return -1;
	}

	ops = dongle->ops;
	ctx = av_mallocz(sizeof(GxAudioDongleCtx));
	if (ops->get_cap)
		ops->get_cap(&ctx->cap);

	ctx->type  |= ops->push ? GX_AUDIO_DONGLE_SOURCE : 0;
	ctx->type  |= ops->pull ? GX_AUDIO_DONGLE_SINK   : 0;
	ctx->data_type = _get_dongle_datatype(dongle);
	if (ctx->type & GX_AUDIO_DONGLE_SOURCE) {
		if (ctx->data_type == GX_AUDIO_DONGLE_RES_PCM) {
			ctx->source_handle = GxMediaApi_AdecOpen();
			GxMediaApi_AdecResampleSetChannels(ctx->source_handle, ctx->cap.pcm.channel_num);
			GxMediaApi_AdecResampleSetVolume  (ctx->source_handle, param->volume);
			GxMediaApi_AdecResampleSetMute    (ctx->source_handle, param->mute);
			if (param == NULL) {
				gxloge("Audio Dongle [%d] Param is NULL\n", ctx->data_type);
			} else
				ctx->param  = *param;
		} else if (ctx->data_type == GX_AUDIO_DONGLE_DEC_CDD) {
			ctx->source_handle = GxMediaApi_AdecOpen();
			if (param == NULL) {
				gxloge("Audio Dongle [%d] Param is NULL\n", ctx->data_type);
			} else
				ctx->param  = *param;
		} else if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
			ctx->source_handle = GxMediaApi_AoutOpen();
			ctx->param.channel = GXAUDIO_DONGLE_STEREO_CHANNEL;
			ctx->resample_freq = 0;
			ctx->need_resample = 1;
		}
	}

	if (ctx->type & GX_AUDIO_DONGLE_SINK) {
		GxMediaAudioPara para;

		if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
			ctx->sink_handle = GxMediaApi_APlayOpen(1);

			GxMediaApi_APlaySetVolume(ctx->sink_handle, param->volume);
			GxMediaApi_APlaySetMute  (ctx->sink_handle, param->mute);
		} else {
			ctx->rsize = 4 * 1024;
			ctx->rbuf  = av_malloc(ctx->rsize);
			if (ctx->rbuf == NULL) {
				av_free(ctx);
				return -1;
			}

			ctx->sink_handle = GxMediaApi_AudioOpen(0);
			memset(&para, 0, sizeof(GxMediaAudioPara));
			if (ctx->cap.format == GXAUDIO_DONGLE_PCM) {
				para.codectype   = AUDIO_CODEC_PCM;
				para.sample_rate = ctx->cap.pcm.sample_freq;
				para.sample_bits = ctx->cap.pcm.bits;
				para.channel_num = ctx->cap.pcm.channel_num;
				para.big_endian  = ctx->cap.pcm.endian;
			} else if (ctx->cap.format == GXAUDIO_DONGLE_SBC)
				para.codectype   = AUDIO_CODEC_SBC;
			para.volume.right_now = 1;
			para.volume.target_value = param->volume;
			para.volume.step = 0;
			GxMediaApi_AudioConfig(ctx->sink_handle, para);

			GxPlayer_SetAudioVolume(param->volume);
			GxPlayer_SetAudioMute(param->mute);

			if (param->channel == GXAUDIO_DONGLE_STEREO_CHANNEL)
				GxPlayer_SetAudioTrack(AUDIO_TRACK_STEREO);
			else if (param->channel == GXAUDIO_DONGLE_LEFT_CHANNEL)
				GxPlayer_SetAudioTrack(AUDIO_TRACK_LEFT);
			else if (param->channel == GXAUDIO_DONGLE_RIGHT_CHANNEL)
				GxPlayer_SetAudioTrack(AUDIO_TRACK_RIGHT);

			GxMediaApi_AudioStart(ctx->sink_handle);
		}
	}

	ctx->thread_running = 1;

	dongle->ctx = ctx;
	GxCore_ThreadCreate(name, &ctx->thread_id,
			_dongle_thread,
			dongle, 32*1024,
			GXOS_DEFAULT_PRIORITY - 1);

	return 0;
}

int GxAudioDongleDisable(const char *name)
{
	GxAudioDongle *dongle = _find_dongle(name);
	GxAudioDongleCtx *ctx = NULL;

	if (!dongle || !dongle->ctx)
		return -1;

	ctx = dongle->ctx;
	ctx->thread_running = 0;
	GxCore_ThreadJoin(ctx->thread_id);

	if (ctx->need_resample) {
		if (ctx->resampler) {
			gx_resample_free(ctx->resampler);
			ctx->resampler = NULL;
		}
	}

	if (ctx->type & GX_AUDIO_DONGLE_SOURCE) {
		if (ctx->data_type == GX_AUDIO_DONGLE_RES_PCM ||
				ctx->data_type == GX_AUDIO_DONGLE_DEC_CDD) {
			GxMediaApi_AdecClose(ctx->source_handle);
		} else if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
			GxMediaApi_AoutClose(ctx->source_handle);
		}
	}

	if (ctx->type & GX_AUDIO_DONGLE_SINK) {
		if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM) {
			GxMediaApi_APlayClose(ctx->sink_handle);
		} else {
			GxMediaApi_AudioStop (ctx->sink_handle);
			GxMediaApi_AudioClose(ctx->sink_handle);
			if (ctx->rbuf)
				av_free(ctx->rbuf);
			ctx->rbuf  = NULL;
			ctx->rsize = 0;
		}
	}

	av_free(dongle->ctx);
	dongle->ctx = NULL;

	return 0;
}

int GxAudioDongleSetVolume(const char *name, int volume)
{
	GxAudioDongle *dongle = _find_dongle(name);
	GxAudioDongleCtx *ctx = NULL;

	if (!dongle || !dongle->ctx)
		return -1;

	ctx = dongle->ctx;
	if (ctx->type & GX_AUDIO_DONGLE_SOURCE)
		GxMediaApi_AdecResampleSetVolume(ctx->source_handle, volume);

	if (ctx->type & GX_AUDIO_DONGLE_SINK) {
		if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM)
			GxMediaApi_APlaySetVolume(ctx->sink_handle, volume);
		else
			GxPlayer_SetAudioVolume(volume);
	}

	ctx->param.volume = volume;
	return 0;
}

int GxAudioDongleSetMute(const char *name, int mute)
{
	GxAudioDongle *dongle = _find_dongle(name);
	GxAudioDongleCtx *ctx = NULL;

	if (!dongle || !dongle->ctx)
		return -1;

	ctx = dongle->ctx;
	if (ctx->type & GX_AUDIO_DONGLE_SOURCE)
		GxMediaApi_AdecResampleSetMute(ctx->source_handle, mute);

	if (ctx->type & GX_AUDIO_DONGLE_SINK) {
		if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM)
			GxMediaApi_APlaySetMute(ctx->sink_handle, mute);
		else
			GxPlayer_SetAudioMute(mute);
	}

	ctx->param.mute = mute;
	return 0;
}

int GxAudioDongleSetChannel(const char *name, GxAudioDongleChannel channel)
{
	GxAudioDongle *dongle = _find_dongle(name);
	GxAudioDongleCtx *ctx = NULL;

	if (!dongle || !dongle->ctx)
		return -1;

	ctx = dongle->ctx;
	if (ctx->type & GX_AUDIO_DONGLE_SINK) {
		if (channel == GXAUDIO_DONGLE_STEREO_CHANNEL) {
			if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM)
				GxMediaApi_APlaySetChannel(ctx->sink_handle, GXMEDIA_APLAY_STERO);
			else
				GxPlayer_SetAudioTrack(AUDIO_TRACK_STEREO);
		} else if (channel == GXAUDIO_DONGLE_LEFT_CHANNEL) {
			if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM)
				GxMediaApi_APlaySetChannel(ctx->sink_handle, GXMEDIA_APLAY_LEFT);
			else
				GxPlayer_SetAudioTrack(AUDIO_TRACK_LEFT);
		} else if (channel == GXAUDIO_DONGLE_RIGHT_CHANNEL) {
			if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM)
				GxMediaApi_APlaySetChannel(ctx->sink_handle, GXMEDIA_APLAY_RIGHT);
			else
				GxPlayer_SetAudioTrack(AUDIO_TRACK_RIGHT);
		} else {
			if (ctx->data_type == GX_AUDIO_DONGLE_OUT_PCM)
				GxMediaApi_APlaySetChannel(ctx->sink_handle, GXMEDIA_APLAY_MULTI);
		}
	}

	ctx->param.channel = channel;
	return 0;
}
