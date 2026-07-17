#include "ad.h"
#include "gxplayer_decoder.h"
#include "audio/audio_dec.h"

typedef struct {
	int       dev;
	handle_t  mod;
	GxFifo*   esaInFifo;
	GxFifo*   pcmOutFifo;
	GxFifo*   esaFifo;
	int       downmix;
	handle_t  mutex;
	int       running;
	int       paused;
	int       threadID;
	GxAudioDecoderContext *ctx;
	GxAudioDecoderStatus status;
	void         *ormem;
	unsigned int orsize;
	unsigned int bit_rate;
} AdSwPriv;

static int _sw_read(void *swpriv, unsigned char *buf, unsigned int size, int *pts)
{
	AdSwPriv *priv = (AdSwPriv *)swpriv;

	if (priv->esaInFifo) {
		unsigned int in_size = GxFifo_GetLength(priv->esaInFifo);

		size = (in_size > size) ? size : in_size;
		if (size > 0) {
			GxFifo_ReadWithPts(priv->esaInFifo, buf, size, pts, 0);
		}

		return size;
	}

	return -1;
}

static int _sw_peek(void *swpriv, unsigned char *buf, unsigned int size)
{
	AdSwPriv *priv = (AdSwPriv *)swpriv;

	if (priv->esaInFifo) {
		unsigned int in_size = GxFifo_GetLength(priv->esaInFifo);

		size = (in_size > size) ? size : in_size;
		if (size > 0) {
			GxFifo_Peek(priv->esaInFifo, buf, size);
		}

		return size;
	}

	return -1;
}

static int _sw_skip(void *swpriv, unsigned int size)
{
	AdSwPriv *priv = (AdSwPriv *)swpriv;

	if (priv->esaInFifo) {
		unsigned int in_size = GxFifo_GetLength(priv->esaInFifo);

		size = (in_size > size) ? size : in_size;
		if (size > 0) {
			GxFifo_Skip(priv->esaInFifo, size);
		}

		return size;
	}

	return -1;
}

static int _sw_write(void *swpriv, unsigned char *buf, unsigned int size, int pts)
{
	AdSwPriv *priv = (AdSwPriv *)swpriv;

	if (priv) {
		unsigned int out_cap  = GxFifo_GetCap(priv->esaFifo);
		unsigned int out_size = GxFifo_GetLength(priv->esaFifo);
		unsigned int out_space_size = (out_cap - out_size);

		if (out_space_size < size)
			gxlogd("warning: esa space full (%d, %d)\n", out_space_size, out_size);

		GxFifo_WriteWithPts(priv->esaFifo, buf, size, pts, 0);
		return size;
	}

	return -1;
}

static int _sw_control(void *swpriv, GxAudioDecoderCommand cmd, void *args)
{
	AdSwPriv *priv = (AdSwPriv *)swpriv;

	if (priv) {
		switch (cmd) {
		case GX_AUDIO_DECODER_SET_CDD_INFO:
			{
				GxAudioDecoderCddInfo *info = (GxAudioDecoderCddInfo *)args;

				priv->bit_rate = info->bit_rate;
				break;
			}
		case GX_AUDIO_DECODER_SET_PCM_INFO:
			{
				GxAudioDecoderPcmInfo *info = (GxAudioDecoderPcmInfo *)args;
				GxAudioDecProperty_PcmInfo pcm_info;

				pcm_info.endian     = (info->endian == 1) ? PCM_BIG_ENDIAN : PCM_LITTLE_ENDIAN;
				pcm_info.samplefre  =  info->sample_rate;
				pcm_info.channelnum =  info->channel_num;
				pcm_info.bitwidth   =  info->bits_per_sample;
				pcm_info.channelflag=  info->channel_flag;
				pcm_info.intelace   =  1;
				pcm_info.float_en   =  info->float_en;
				GxAVSetProperty(priv->dev, priv->mod,   \
						GxAudioDecoderPropertyID_PcmInfo, &pcm_info, sizeof(GxAudioDecProperty_PcmInfo));
			}
			break;
		case GX_AUDIO_DECODER_GET_RBUF_INFO:
			{
				GxAudioDecoderBufInfo *info = (GxAudioDecoderBufInfo *)args;

				memset(info,0,sizeof(GxAudioDecoderBufInfo));
				info->cap  = GxFifo_GetCap(priv->esaInFifo);
				if(info->cap != 0) {
					info->size = GxFifo_GetLength(priv->esaInFifo);
					info->free = GxFifo_GetFreeSize(priv->esaInFifo);
				}
			}
			break;
		case GX_AUDIO_DECODER_GET_RBUF_PTS_INFO:
			{
				GxAudioDecoderBufInfo *info = (GxAudioDecoderBufInfo *)args;

				memset(info,0,sizeof(GxAudioDecoderBufInfo));
				info->cap  = GxFifo_GetPtsCap(priv->esaInFifo);
				if(info->cap != 0) {
					info->size = GxFifo_GetPtsLength(priv->esaInFifo);
					info->free = GxFifo_GetPtsFreeSize(priv->esaInFifo);
				}
			}
			break;
		case GX_AUDIO_DECODER_GET_WBUF_INFO:
			{
				GxAudioDecoderBufInfo *info = (GxAudioDecoderBufInfo *)args;

				memset(info,0,sizeof(GxAudioDecoderBufInfo));
				info->cap  = GxFifo_GetCap   (priv->esaFifo);
				info->size = GxFifo_GetLength(priv->esaFifo);
				info->free = GxFifo_GetFreeSize(priv->esaFifo);
			}
			break;
		}
	}

	return 0;
}

static void _sw_decode(void* data)
{
	GxMediaFilter *filter = (GxMediaFilter*)data;
	GxAVCodec *ad = GXDECODEROBJECT(filter);
	AdSwPriv *priv = ad->priv;
	GxStreamAudioHeader* header = (GxStreamAudioHeader*)ad->header;
	GxAudioDecoderParam param;

	param.priv = (void *)priv;
	param.rc   = _sw_read;
	param.pc   = _sw_peek;
	param.sc   = _sw_skip;
	param.wc   = _sw_write;
	param.cc   = _sw_control;
	priv->ctx = GxAudioDecoderCreate(header->format, &param);
	if (!priv->ctx) return;

	while (priv->running) {
		if (priv->paused) {
			GxCore_ThreadDelay(100);
			continue;
		}
#if 0
		gxlogi("time: %d esaIn : [%08d]%08d  - [%08d]%08d - esa :[%08d]%08d - [%08d]%08d\n",
			get_current_time(),
			GxFifo_GetLength(priv->esaInFifo),GxFifo_GetFreeSize(priv->esaInFifo),
			GxFifo_GetPtsLength(priv->esaInFifo),GxFifo_GetPtsFreeSize(priv->esaInFifo),
			GxFifo_GetLength(priv->esaFifo),GxFifo_GetFreeSize(priv->esaFifo),
			GxFifo_GetPtsLength(priv->esaFifo),GxFifo_GetPtsFreeSize(priv->esaFifo));
#endif
		priv->status = GxAudioDecoderDecode(priv->ctx);
		if (priv->status == GX_AUDIO_DECODER_EOF) {
			GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Update, NULL, 0);
		}

		if (priv->status == GX_AUDIO_DECODER_EOF ||
			priv->status == GX_AUDIO_DECODER_RBUF_EMPTY ||
			priv->status == GX_AUDIO_DECODER_ERROR)
			GxCore_ThreadDelay(20);
		else if(priv->status == GX_AUDIO_DECODER_WBUF_FULL)
			GxCore_ThreadDelay(10);
		else
			GxCore_ThreadYield();
	}

	GxAudioDecoderDestory(priv->ctx);
	priv->ctx = NULL;
}

static int ad_sw_config(GxMediaFilter*  filter)
{
	GxAVCodec* ad = GXDECODEROBJECT(filter);

	if (ad) {
		GxStreamAudioHeader* header = (GxStreamAudioHeader*)ad->header;
		AdSwPriv* priv = (AdSwPriv*)ad->priv;
		GxAudioDecProperty_Config     audiodec_config;
		GxAudioDecProperty_OutInfo    audiodec_outinfo;
		GxAudioDecProperty_Capability audiodec_capability;

		GxPlayer_SystemGet(PSYS_AOUT_DOWNMIX, &priv->downmix);

		memset(&audiodec_config, 0 , sizeof(audiodec_config));
		if (ad->decoder_id == 0) {
			GxPin* esa_in_pin  = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_ESA_IN);
			GxPin* pcm_out_pin = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_PCM_OUT);

			audiodec_capability.type = CODEC_PCM;
			GxAVGetProperty(priv->dev, priv->mod, \
					GxAudioDecoderPropertyID_Capability, &audiodec_capability, sizeof(GxAudioDecProperty_Capability));
			if (audiodec_capability.workbuf_size) {
				priv->orsize = audiodec_capability.workbuf_size;
				priv->ormem  = av_memhole_malloc(priv->orsize, GX_PINFLAG_WKA);
				if (priv->ormem == NULL)
					return GX_PLAYER_ERROR;
			}

			audiodec_config.type = CODEC_PCM;
			audiodec_config.down_mix = priv->downmix;
			audiodec_config.sink = AUDIO_TO_AVOUT;
			audiodec_config.mode = DECODE_MODE;
			audiodec_config.audioout_id = 0;
			audiodec_config.ormem_start = priv->ormem;
			audiodec_config.ormem_size  = priv->orsize;
			GxAVSetProperty(priv->dev, priv->mod, \
					GxAudioDecoderPropertyID_Config, &audiodec_config,sizeof(GxAudioDecProperty_Config));

			GxAVGetProperty(priv->dev, priv->mod, \
					GxAudioDecoderPropertyID_OutInfo, &audiodec_outinfo, sizeof(GxAudioDecProperty_OutInfo));
			header->data_type = audiodec_outinfo.outdata_type;

			if (esa_in_pin && esa_in_pin->source)
				priv->esaInFifo = esa_in_pin->source->fifo;

			if (priv->esaFifo) {
				GxFifoLink FifoLink;
				FifoLink.module    = priv->mod;
				FifoLink.pin_id    = GXAV_PIN_ESA;
				FifoLink.direction = GXAV_PIN_INPUT;
				GxFifo_Link(priv->esaFifo, &FifoLink);
			}

			if (pcm_out_pin && pcm_out_pin->fifo) {
				GxFifoLink FifoLink;
				GxFifo_Reset(pcm_out_pin->fifo);
				FifoLink.module    = priv->mod;
				FifoLink.pin_id    = GXAV_PIN_PCM;
				FifoLink.direction = GXAV_PIN_OUTPUT;
				GxFifo_Link(pcm_out_pin->fifo, &FifoLink);
				priv->pcmOutFifo = pcm_out_pin->fifo;
			}

			return GX_PLAYER_OK;
		} else if (ad->decoder_id == 1) {
			header->data_type = ADPCM_TYPE;
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ad_sw_run(GxMediaFilter*  filter)
{
	AdSwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);

	if (ad) {
		priv = (AdSwPriv*)ad->priv;
		if (priv && !priv->running) {
			priv->running = 1;
			priv->paused  = 0;
			GxCore_ThreadCreate("sw audio decoder",
					&priv->threadID, _sw_decode, filter, 1024*64, GXOS_DEFAULT_PRIORITY - 2);
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

			GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Run, NULL,0);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ad_sw_stop(GxMediaFilter*  filter)
{
	AdSwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);

	if (ad) {
		priv = (AdSwPriv*)ad->priv;

		if (priv) {
			if (priv->running) {
				priv->running = 0;
				priv->paused  = 0;
				GxCore_ThreadJoin(priv->threadID);

				GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Stop, NULL,0);
				GxAVResetEvent (priv->dev, priv->mod, EVENT_AUDIO_FRAMEINFO);

				if (priv->ormem) {
					av_memhole_free(priv->ormem, GX_PINFLAG_WKA);
					priv->orsize = 0;
					priv->ormem  = NULL;
				}
			}

			if (priv->pcmOutFifo) {
				GxFifoLink FifoLink;
				GxFifo_Reset(priv->pcmOutFifo);
				FifoLink.module = priv->mod;
				GxFifo_Unlink(priv->pcmOutFifo, &FifoLink);
				priv->pcmOutFifo = NULL;
			}

			if (priv->esaInFifo)
				priv->esaInFifo = NULL;

			if (priv->esaFifo) {
				GxFifoLink FifoLink;
				GxFifo_Reset(priv->esaFifo);
				FifoLink.module = priv->mod;
				GxFifo_Unlink(priv->esaFifo, &FifoLink);
			}

			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ad_sw_pause(GxMediaFilter*  filter)
{
	AdSwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);

	if (ad) {
		priv = (AdSwPriv*)ad->priv;
		if (priv && priv->running) {
			GxAVSetProperty(priv->dev, priv->mod,GxAudioDecoderPropertyID_Pause, NULL,0);
			priv->paused = 1;
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_sw_resume(GxMediaFilter*  filter)
{
	AdSwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);

	if (ad) {
		priv = (AdSwPriv*)ad->priv;
		if (priv && priv->running) {
			priv->paused = 0;
			GxAVSetProperty(priv->dev, priv->mod,GxAudioDecoderPropertyID_Resume, NULL,0);
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_sw_abort(GxMediaFilter*  filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_sw_wait(GxMediaFilter*  filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_sw_reset(GxMediaFilter*  filter)
{
	if (filter) {
		ad_sw_stop(filter);
		ad_sw_config(filter);
		ad_sw_run(filter);

		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int ad_sw_probe(AudioCodecType codec_type, int need_decode)
{
	return need_decode ? GxAudioDecoderProbe(codec_type) : 0;
}

static int ad_sw_open(GxAVCodec* ad)
{
	AdSwPriv* priv = NULL;
	GxStreamAudioHeader* header = (GxStreamAudioHeader*)ad->header;
	unsigned int esasize = 0;;

	priv = (void* )av_mallocz(sizeof(AdSwPriv));
	if (priv == NULL)
		return GX_PLAYER_ERROR;

	ad->priv = (void* )priv;
	GxPlayer_SystemGet(PSYS_BufSizeESA, &esasize);
	priv->esaFifo = GxFifo_Create(esasize, GX_PINFLAG_PTS_FIFO);
	if(!priv->esaFifo)
		goto error_out1;

	priv->dev = GxAvdev_CreateDevice(0);
	if (priv->dev < 0)
		goto error_out1;

	priv->mod = GxAvdev_OpenModule(priv->dev, GXAV_MOD_AUDIO_DECODE, ad->decoder_id);
	if (priv->mod < 0)
		goto error_out2;

	if (header) {
		header->dec_dev = priv->dev;
		header->dec_mod = priv->mod;
	}

	GxCore_MutexCreate(&priv->mutex);
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;

error_out2:
	GxAvdev_DestroyDevice(priv->dev);

error_out1:
	if (priv->esaFifo)
		GxFifo_Destroy(priv->esaFifo);
	if (priv)
		av_free(priv);

	ad->priv = NULL;
	return GX_PLAYER_ERROR;
}

static int ad_sw_close(GxAVCodec*  ad)
{
	AdSwPriv* priv = NULL;

	if (ad) {
		priv = (AdSwPriv*)ad->priv;
		if (priv) {
			ad_sw_stop((GxMediaFilter*)ad);

			GxCore_MutexDelete(priv->mutex);
			GxAvdev_CloseModule(priv->dev, priv->mod);
			GxAvdev_DestroyDevice(priv->dev);
			if (priv->esaFifo)
				GxFifo_Destroy(priv->esaFifo);
			av_free(priv);
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ad_sw_control(GxAVCodec*  ad, int cmd, void* args)
{
	AdSwPriv* priv = (AdSwPriv*)ad->priv;

	switch (cmd) {
	case GX_AD_CTRL_PAUSE:
		break;
	case GX_AD_CTRL_RESUME:
		break;
	case GX_AD_CTRL_UPDATE:
	case GX_AD_CTRL_REFRESH:
		GxAudioDecoderUpdate(priv->ctx);
		break;
	case GX_AD_CTRL_GET_FRAME_INFO:
		{
			int ret;
			GxAudioDecProperty_State state ;
			if(((GxStreamAudioHeader*)ad->header)->format == PLAYER_ACODEC_DUMMY)
				return GX_PLAYER_OK;

			ret = GxAVGetProperty(priv->dev, priv->mod,
					GxAudioDecoderPropertyID_State, &state, sizeof(GxAudioDecProperty_State));
			if(ret == GXCORE_SUCCESS) {
				switch(state.state)
				{
				case AUDIODEC_RUNNING:
				case AUDIODEC_PAUSED:
					{
						GxAudioDecProperty_FrameInfo frame;
						ret = GxAVGetProperty(priv->dev, priv->mod,
								GxAudioDecoderPropertyID_FrameInfo, &frame,sizeof(GxAudioDecProperty_FrameInfo));
						if(ret == GXCORE_SUCCESS)
						{
							GxStreamAudioHeader *sh = (GxStreamAudioHeader*)ad->header;
							sh->stat.decode_frame_cnt = frame.decode_frame_cnt;
							sh->i_bps = (sh->i_bps == 0) ? priv->bit_rate / 8 : sh->i_bps;
							sh->samplerate = frame.samplefre;
							sh->channels = (sh->channels == 0) ? frame.channelnum : sh->channels;
							if (args)
								*(GxAudioDecProperty_FrameInfo *)args = frame;
							return GX_PLAYER_OK;
						}
						return GX_PLAYER_ERROR;
					}
					break;
				default:
					return GX_PLAYER_ERROR;
				}
			}
			return GX_PLAYER_ERROR;
		}

		break;
	case GX_AD_CTRL_GET_STATUS:
		{
			int ret;
			AVCodecStatus* pstate = (AVCodecStatus*)args;
			GxAudioDecProperty_State state ;
			if(((GxStreamAudioHeader*)ad->header)->format == PLAYER_ACODEC_DUMMY)
			{
				pstate->state = AVCODEC_RUNNING;
				return GX_PLAYER_OK;
			}
			ret = GxAVGetProperty(priv->dev, priv->mod,
					GxAudioDecoderPropertyID_State, &state, sizeof(GxAudioDecProperty_State));
			if(ret == GXCORE_SUCCESS)
			{
				switch(state.state){
				case AUDIODEC_READY:
					pstate->state = AVCODEC_READY;
					pstate->err_code = AVCODEC_ERR_NONE;
					break;
				case AUDIODEC_RUNNING:
					pstate->state = AVCODEC_RUNNING;
					if (state.err_code == AUDIODEC_ERR_UNSUPPORT_CODECTYPE)
						pstate->err_code = AVCODEC_ERR_CODECTYPE_UNSUPPORT;
					else
						pstate->err_code = AVCODEC_ERR_NONE;
					break;
				case AUDIODEC_PAUSED:
					pstate->state = AVCODEC_PAUSED;
					pstate->err_code = AVCODEC_ERR_NONE;
					break;
				case AUDIODEC_STOPED:
					pstate->state = AVCODEC_STOPPED;
					pstate->err_code = AVCODEC_ERR_NONE;
					break;
				case AUDIODEC_OVER:
					pstate->state = AVCODEC_FINISHED;
					pstate->err_code = AVCODEC_ERR_NONE;
					break;
				case AUDIODEC_ERROR:
					pstate->state = AVCODEC_ERROR;
					switch(state.err_code)
					{
					case AUDIODEC_ERR_NONE:
						pstate->err_code = AVCODEC_ERR_NONE;
						break;
					case AUDIODEC_ERR_ESA_OVERFLOW:
						pstate->err_code = AVCODEC_ERR_ESBUF_OVERFLOW;
						break;
					case AUDIODEC_ERR_UNSUPPORT_CODECTYPE:
						pstate->err_code = AVCODEC_ERR_CODECTYPE_UNSUPPORT;
						break;
					default:
						break;
					}
					break;
				default:
					pstate->state = AVCODEC_STOPPED;
					pstate->err_code = AVCODEC_ERR_NONE;
					break;
				}
				return GX_PLAYER_OK;
			}
			return GX_PLAYER_ERROR;
		}
		break;
	case GX_AD_CTRL_GET_PCM_SIZE:
		if (priv && priv->running)
			*(int*)args = GxFifo_GetLength(priv->pcmOutFifo);
		else
			*(int*)args = 0;
		return GX_PLAYER_OK;
	default:
		break;
	}

	return GX_PLAYER_ERROR;
}

static int ad_sw_decode(GxAVCodec* ad, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	return GX_PLAYER_ERROR;
}

GxAVCodecClass gx_audio_decoder_SW = {
	._inherit = {
		._inherit = {
			.name    = "AudioDecoderSW",
			.parent  = (void* )&gx_AudioDecoderBase,
			.size    = sizeof(GxAudioCodec),
		},
		.run    = ad_sw_run,
		.pause  = ad_sw_pause,
		.resume = ad_sw_resume,
		.config = ad_sw_config,
		.stop   = ad_sw_stop,
		.wait   = ad_sw_wait,
		.reset  = ad_sw_reset,
		.abort  = ad_sw_abort,
	},
	DEF_AUTHOR("AudioDecoder","SW","No description","linxsh","No comment"),

	.name    = "SW AudioDecoder",
	.format  = {
		AUDIO_CODEC_AMR_NB,
		AUDIO_CODEC_AMR_WB,
		AUDIO_CODEC_APE,
		-1 },
	.probe   = ad_sw_probe,
	.open    = ad_sw_open,
	.close   = ad_sw_close,
	.control = ad_sw_control,
	.decode  = ad_sw_decode,
};

