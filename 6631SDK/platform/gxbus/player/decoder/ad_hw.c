
#include "ad.h"

typedef struct {
	int dev;
	handle_t mod;
	GxFifo* esa_in;
	GxFifo* pcm_out;
	GxFifo* ac3_out;
	GxFifo* eac3_out;
	GxFifo* dts_out;
	GxFifo* aac_out;
	GxFifo* adesa_in;
	GxFifo* adpcm_out;
	handle_t firmware;
	GxAudioDecProperty_Config config;
	GxAudioDecProperty_Capability capability;
	int ac3mode;
	int aacmode;
	int downmix;
	int hdmisrc;
	int running;
	int enable_ac3;
	int enable_dts;
	int enable_aac;
	int anti_error_code;
	void* ormem;
	unsigned int orsize;
	unsigned int boost_volume;
	int support_speed;
}AdHwPriv;

static int _set_audio_speed(AdHwPriv* priv, int ispeed)
{
	GxADSPProperty_Control actrl;

	actrl.cmd  = GX_ADSP_CONTROL_SET_SPEED_RATE;
	actrl.arg4 = ispeed/10;
	return GxAVSetProperty(priv->dev, priv->mod,   \
			GxADSPPropertyID_Control, (void*)&actrl, sizeof(GxADSPProperty_Control));
}

static int ad_hw_run(GxMediaFilter*  filter)
{
	AdHwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);
	GxStreamAudioHeader* header;

	if (ad)
	{
		priv = (AdHwPriv*)ad->priv;
		header = (GxStreamAudioHeader*)ad->header;
		if (priv && priv->running)
		{
			gxlogi("[Player]: %s in.\n", __func__);
			GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Run, NULL,0);
			gxlogi("[Player]: %s out.\n", __func__);

			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_OK;
}

static int  ad_hw_pause(GxMediaFilter*  filter)
{
	AdHwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);
	if (ad)
	{
		priv = (AdHwPriv*)ad->priv;
		if (priv && priv->running)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAVSetProperty(priv->dev, priv->mod,GxAudioDecoderPropertyID_Pause, NULL,0);
			return GX_PLAYER_OK;
		}
	}
	return GX_PLAYER_ERROR;
}

static int ad_hw_wait(GxMediaFilter*  filter)
{
	int ret;
	uint32_t event;
	AdHwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);

	if (ad)
	{
		priv = (AdHwPriv*)ad->priv;
		if (priv)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			ret = GxAVWaitEvents(priv->dev, priv->mod, EVENT_AUDIO_FRAMEINFO,2000000,&event);
			gxlogf("a_event = 0x%x\n",event);
			return ret;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ad_hw_abort(GxMediaFilter*  filter)
{
	AdHwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);

	if (ad)
	{
		priv = (AdHwPriv*)ad->priv;
		if (priv)
		{
			GxAudioDecProperty_State state ;
			GxAVGetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_State, &state,sizeof(GxAudioDecProperty_State));
			if (state.state == AUDIODEC_STOPED || state.state == AUDIODEC_OVER)
			{
				gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
				GxAVSetEvent(priv->dev, priv->mod, EVENT_AUDIO_FRAMEINFO);
			}
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ad_hw_resume(GxMediaFilter*  filter)
{
	AdHwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);
	if (ad)
	{
		priv = (AdHwPriv*)ad->priv;
		if (priv && priv->running)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAVSetProperty(priv->dev, priv->mod,GxAudioDecoderPropertyID_Resume, NULL,0);
			return GX_PLAYER_OK;
		}
	}
	return GX_PLAYER_ERROR;
}

static int ad_hw_stop(GxMediaFilter*  filter)
{
	AdHwPriv* priv;
	GxAVCodec* ad = GXDECODEROBJECT(filter);
	GxFifoLink FifoLink;

	if (ad)
	{
		priv = (AdHwPriv*)ad->priv;
		if (priv && priv->running)
		{
			gxlogi("[Player]: %s in.\n", __func__);
			GxAVSetProperty(priv->dev, priv->mod,GxAudioDecoderPropertyID_Stop, NULL,0);
			gxlogi("[Player]: %s out.\n", __func__);

			GxAVResetEvent(priv->dev, priv->mod, EVENT_AUDIO_FRAMEINFO);

			FifoLink.module = priv->mod;
			if (ad->decoder_id == 0) {
				if (priv->pcm_out) {
					GxFifo_Reset(priv->pcm_out);
					GxFifo_Unlink(priv->pcm_out, &FifoLink);
					priv->pcm_out = NULL;
				}
				if (priv->ac3_out) {
					GxFifo_Reset(priv->ac3_out);
					GxFifo_Unlink(priv->ac3_out, &FifoLink);
					priv->ac3_out = NULL;
				}
				if (priv->eac3_out) {
					GxFifo_Reset(priv->eac3_out);
					GxFifo_Unlink(priv->eac3_out, &FifoLink);
					priv->eac3_out = NULL;
				}
				if (priv->dts_out) {
					GxFifo_Reset(priv->dts_out);
					GxFifo_Unlink(priv->dts_out, &FifoLink);
					priv->dts_out = NULL;
				}
				if (priv->aac_out) {
					GxFifo_Reset(priv->aac_out);
					GxFifo_Unlink(priv->aac_out, &FifoLink);
					priv->aac_out = NULL;
				}
				if (priv->esa_in) {
					GxFifo_Unlink(priv->esa_in, &FifoLink);
					priv->esa_in = NULL;
				}
				if (priv->ormem) {
					av_memhole_free(priv->ormem, GX_PINFLAG_WKA);
					priv->orsize = 0;
					priv->ormem  = NULL;
				}
			}else if(ad->decoder_id == 1){
				if (priv->adpcm_out) {
					GxFifo_Reset(priv->adpcm_out);
					GxFifo_Unlink(priv->adpcm_out, &FifoLink);
					priv->adpcm_out = NULL;
				}
				if (priv->adesa_in) {
					GxFifo_Unlink(priv->adesa_in, &FifoLink);
					priv->adesa_in = NULL;
				}
			}

			priv->running = 0;
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

#define CHECK_CONFIG_RET(ret);  \
	if (ret < 0) { \
		if (priv->ormem) { \
			av_memhole_free(priv->ormem, GX_PINFLAG_WKA); \
			priv->orsize = 0; \
			priv->ormem  = NULL; \
		} \
		gxloge("%s[%d]:%s###################error##########\n", __FILE__, __LINE__, __FUNCTION__);\
		return ret; \
	}

static int ad_hw_config(GxMediaFilter*  filter)
{
	int ret;
	AdHwPriv* priv;
	GxPin* esa_in_pin, *pcm_out_pin, *ac3_out_pin, *eac3_out_pin, *dts_out_pin, *aac_out_pin;
	GxPin* adesa_in_pin, *adpcm_out_pin;
	GxAVCodec* ad = GXDECODEROBJECT(filter);
	GxAudioDecProperty_Config audiodec_config;
	GxAudioDecProperty_OutInfo audiodec_outinfo;
	GxAudioDecProperty_BoostVolume audiodec_boost;
	GxStreamAudioHeader* header;
	GxFifoLink FifoLink;
	GxADSPProperty_Control actrl;

	memset(&audiodec_config, 0 , sizeof(audiodec_config));
	if(ad)
	{
		int codec_mask = 0;
		header = (GxStreamAudioHeader*)ad->header;
		priv = (AdHwPriv*)ad->priv;
		if(priv)
		{
			GxPlayer_SystemGet(PSYS_AOUT_AC3MODE, &priv->ac3mode);
			GxPlayer_SystemGet(PSYS_AOUT_AACMODE, &priv->aacmode);
			GxPlayer_SystemGet(PSYS_AOUT_DOWNMIX, &priv->downmix);
			GxPlayer_SystemGet(PSYS_AOUT_HDMISRC, &priv->hdmisrc);
			GxPlayer_SystemGet(PSYS_AUDIO_ANTI_ERROR_CODE, &priv->anti_error_code);
			GxPlayer_SystemGet(PSYS_AUDIOCODEC_MASK, &codec_mask);
			if (ad->decoder_id == 1)
				GxPlayer_SystemGet(PSYS_ADEC_DESCRIPTOR_BOOST_VOLUME, &priv->boost_volume);
			else
				GxPlayer_SystemGet(PSYS_ADEC_BOOST_VOLUME,            &priv->boost_volume);

			if (header->format & codec_mask) {
				gxloge("[Player]: acodec [0x%x] not support ! \n", header->format);
				return GX_PLAYER_ERROR;
			}

			audiodec_config.type = acodec_std2hw(header->format);
			if (audiodec_config.type == -1) {
				gxloge("[Player]: acodec [0x%x] not support ! \n", header->format);
				return GX_PLAYER_ERROR;
			}

			switch (header->format) {
			case AUDIO_CODEC_AAC_LATM:
				audiodec_config.u.mpeg4_aac.format = LATM_FORMAT;
				if(header->priv.para.data) {
					audiodec_config.u.mpeg4_aac.header_size = ((header->priv.para.len > 2048) ? 2048 : header->priv.para.len );
					memcpy(audiodec_config.u.mpeg4_aac.header,header->priv.para.data,audiodec_config.u.mpeg4_aac.header_size);
				}
				break;
			case AUDIO_CODEC_AAC_ADTS:
				audiodec_config.u.mpeg4_aac.format = ADTS_FORMAT;
				break;
			case AUDIO_CODEC_RA_AAC:
			case AUDIO_CODEC_RA_RA8LBR:
				if (header->priv.para.data) {
					audiodec_config.u.real.header_size = ((header->priv.para.len > 2048) ? 2048 : header->priv.para.len );
					memcpy(audiodec_config.u.real.header,header->priv.para.data,audiodec_config.u.real.header_size);
				}
				break;
			case AUDIO_CODEC_EAC3:
				if (header->priv.para.data) {
					audiodec_config.u.dolby.header_size = ((header->priv.para.len > 2048) ? 2048 : header->priv.para.len );
					memcpy(audiodec_config.u.dolby.header,header->priv.para.data,audiodec_config.u.dolby.header_size);
				}
				break;
			default:
				break;
			}

			if (!priv->enable_aac && audiodec_config.type == CODEC_MPEG4_AAC)
				return GX_PLAYER_ERROR;

			if (!priv->enable_ac3 && (audiodec_config.type == CODEC_AC3 || audiodec_config.type == CODEC_EAC3))
				return GX_PLAYER_ERROR;

			if (!priv->enable_dts && audiodec_config.type == CODEC_DTS)
				return GX_PLAYER_ERROR;

			if (audiodec_config.type == CODEC_PCM) {
				if ((header->samplerate == 0) || (header->channels == 0) || (header->samplesize == 0))
					return GX_PLAYER_ERROR;
			}

			if (audiodec_config.type == CODEC_AC3 ||
					audiodec_config.type == CODEC_EAC3) {
				if ((priv->ac3mode & AUDIO_AC3_BYPASS_MODE) == AUDIO_AC3_BYPASS_MODE)
					audiodec_config.mode |= BYPASS_MODE;

				if ((priv->ac3mode & AUDIO_AC3_DECODE_MODE) == AUDIO_AC3_DECODE_MODE)
					audiodec_config.mode |= DECODE_MODE;
			} else if (audiodec_config.type == CODEC_MPEG4_AAC) {
				audiodec_config.mode |= DECODE_MODE;

				if ((priv->aacmode & AUDIO_AAC_CONVERT_MODE) == AUDIO_AAC_CONVERT_MODE) {
					if ((header->samplerate >= 32000) || (header->samplerate <= 48000))
						audiodec_config.mode |= CONVERT_MODE;
				}

				if ((priv->aacmode & AUDIO_AAC_BYPASS_MODE) == AUDIO_AAC_BYPASS_MODE)
					audiodec_config.mode |= BYPASS_MODE;
			} else if (audiodec_config.type == CODEC_DTS) {
					audiodec_config.mode = BYPASS_MODE;
			} else
				audiodec_config.mode |= DECODE_MODE;

			if (ad->decoder_id == 0) {
				priv->capability.type = audiodec_config.type;
				GxAVGetProperty(priv->dev, priv->mod, \
						GxAudioDecoderPropertyID_Capability, &priv->capability, sizeof(GxAudioDecProperty_Capability));
				if (priv->capability.workbuf_size) {
					priv->orsize = priv->capability.workbuf_size;
					priv->ormem = av_memhole_malloc(priv->orsize, GX_PINFLAG_WKA);
					if (priv->ormem == NULL)
						return GX_PLAYER_ERROR;
				}
			} else {
				if (header->stream_type == AVSTREAM_TS)
					audiodec_config.stream_type = STREAM_TS;
				else
					audiodec_config.stream_type = STREAM_ES;
				audiodec_config.stream_pid = header->stream_pid;
				priv->orsize = 0;
				priv->ormem  = NULL;
			}

			audiodec_config.down_mix = (audiodec_config.type==CODEC_PCM)?0:priv->downmix;
			audiodec_config.sink = AUDIO_TO_AVOUT;
			audiodec_config.audioout_id = 0;
			audiodec_config.ormem_start = priv->ormem;
			audiodec_config.ormem_size  = priv->orsize;
			audiodec_config.hdmisrc     = priv->hdmisrc;
			gxlogf("[Player]: [acodec = 0x%x, id = %d]\n", audiodec_config.type, ad->decoder_id);

			if (header->ds->demuxer->type == GX_DEMUXER_TYPE_HW_TS || header->ds->demuxer->type == GX_DEMUXER_TYPE_HW) {
				audiodec_config.enough_data     = 4*1024;
				audiodec_config.anti_error_code = priv->anti_error_code;
			} else {
				audiodec_config.enough_data     = 0;
				audiodec_config.anti_error_code = 0;
			}
			ret = GxAVSetProperty(priv->dev, priv->mod, \
					GxAudioDecoderPropertyID_Config, &audiodec_config,sizeof(GxAudioDecProperty_Config));
			CHECK_CONFIG_RET(ret);

			_set_audio_speed(priv, 1000);

			if (audiodec_config.type == CODEC_PCM) {
				GxAudioDecProperty_PcmInfo pcm_info;

				pcm_info.endian     = (header->big_endian == 1) ? PCM_BIG_ENDIAN : PCM_LITTLE_ENDIAN;
				pcm_info.samplefre  =  header->samplerate;
				pcm_info.channelnum =  header->channels;
				pcm_info.bitwidth   =  header->samplesize;
				pcm_info.intelace   =  1;
				pcm_info.float_en   =  0;
				pcm_info.channelflag=  0;
				GxAVSetProperty(priv->dev, priv->mod,   \
						GxAudioDecoderPropertyID_PcmInfo, &pcm_info, sizeof(GxAudioDecProperty_PcmInfo));
			}

			audiodec_boost.index = priv->boost_volume;
			GxAVSetProperty(priv->dev, priv->mod,   \
					GxAudioDecoderPropertyID_BoostVolume, &audiodec_boost, sizeof(GxAudioDecProperty_BoostVolume));

			GxAVGetProperty(priv->dev, priv->mod, \
					GxAudioDecoderPropertyID_OutInfo, &audiodec_outinfo, sizeof(GxAudioDecProperty_OutInfo));
			header->data_type = audiodec_outinfo.outdata_type;

			actrl.cmd  = GX_ADSP_CONTROL_SUPPORT_SPEED;
			actrl.arg5 = 0;
			GxAVGetProperty(priv->dev, priv->mod,   \
					GxADSPPropertyID_Control, (void*)&actrl, sizeof(GxADSPProperty_Control));
			priv->support_speed = actrl.arg5;

			priv->config = audiodec_config;
			if(ad->decoder_id == 1){
				adesa_in_pin  = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_ADESA_IN);
				adpcm_out_pin = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_ADPCM_OUT);

				if (adesa_in_pin && adesa_in_pin->source) {
					if (adesa_in_pin->source->fifo) {
						FifoLink.module = priv->mod;
						FifoLink.pin_id = GXAV_PIN_ADESA;
						FifoLink.direction = GXAV_PIN_INPUT;
						ret = GxFifo_Link(adesa_in_pin->source->fifo, &FifoLink);
						CHECK_CONFIG_RET(ret);

						priv->adesa_in = adesa_in_pin->source->fifo;
						priv->running = 1;
					}
				}

				if (adpcm_out_pin && adpcm_out_pin->fifo &&
						((header->data_type & ADPCM_TYPE) == ADPCM_TYPE)) {
					GxFifo_Reset(adpcm_out_pin->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_ADPCM;
					FifoLink.direction = GXAV_PIN_OUTPUT;
					ret = GxFifo_Link(adpcm_out_pin->fifo, &FifoLink);
					CHECK_CONFIG_RET(ret);

					priv->adpcm_out = adpcm_out_pin->fifo;
				}
			}else if(ad->decoder_id == 0){
				esa_in_pin   = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_ESA_IN);
				pcm_out_pin  = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_PCM_OUT);
				ac3_out_pin  = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_AC3_OUT);
				eac3_out_pin = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_EAC3_OUT);
				dts_out_pin  = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_DTS_OUT);
				aac_out_pin  = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_AAC_OUT);

				if (esa_in_pin && esa_in_pin->source) {
					if (esa_in_pin->source->fifo) {
						FifoLink.module = priv->mod;
						FifoLink.pin_id = GXAV_PIN_ESA;
						FifoLink.direction = GXAV_PIN_INPUT;
						ret = GxFifo_Link(esa_in_pin->source->fifo, &FifoLink);
						CHECK_CONFIG_RET(ret);

						priv->esa_in = esa_in_pin->source->fifo;
						priv->running = 1;
					}
				}

				if (pcm_out_pin && pcm_out_pin->fifo &&
						((header->data_type & PCM_TYPE) == PCM_TYPE)) {
					GxFifo_Reset(pcm_out_pin->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_PCM;
					FifoLink.direction = GXAV_PIN_OUTPUT;
					ret = GxFifo_Link(pcm_out_pin->fifo, &FifoLink);
					CHECK_CONFIG_RET(ret);

					priv->pcm_out = pcm_out_pin->fifo;
				}

				if (ac3_out_pin && ac3_out_pin->fifo &&
						((header->data_type & AC3_TYPE) == AC3_TYPE)) {
					GxFifo_Reset(ac3_out_pin->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_AC3;
					FifoLink.direction = GXAV_PIN_OUTPUT;
					ret = GxFifo_Link(ac3_out_pin->fifo, &FifoLink);
					CHECK_CONFIG_RET(ret);

					priv->ac3_out = ac3_out_pin->fifo;
				}

				if (eac3_out_pin && eac3_out_pin->fifo &&
						((header->data_type & EAC3_TYPE) == EAC3_TYPE)) {
					GxFifo_Reset(eac3_out_pin->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_EAC3;
					FifoLink.direction = GXAV_PIN_OUTPUT;
					ret = GxFifo_Link(eac3_out_pin->fifo, &FifoLink);
					CHECK_CONFIG_RET(ret);

					priv->eac3_out = eac3_out_pin->fifo;
				}

				if (dts_out_pin && dts_out_pin->fifo &&
						((header->data_type & DTS_TYPE) == DTS_TYPE)) {
					GxFifo_Reset(dts_out_pin->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_DTS;
					FifoLink.direction = GXAV_PIN_OUTPUT;
					ret = GxFifo_Link(dts_out_pin->fifo, &FifoLink);
					CHECK_CONFIG_RET(ret);

					priv->dts_out = dts_out_pin->fifo;
				}

				if (aac_out_pin && aac_out_pin->fifo &&
						((header->data_type & AAC_TYPE) == AAC_TYPE)) {
					GxFifo_Reset(aac_out_pin->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_AAC;
					FifoLink.direction = GXAV_PIN_OUTPUT;
					ret = GxFifo_Link(aac_out_pin->fifo, &FifoLink);
					CHECK_CONFIG_RET(ret);

					priv->aac_out = aac_out_pin->fifo;
				}
			}
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;

}

static int ad_hw_probe(AudioCodecType codec_type, int need_decode)
{
	int score = 0, i = 0;
	extern GxAVCodecClass gx_audio_decoder_HW;
	GxAVCodecClass *cls = &gx_audio_decoder_HW;

	for (i = 0; i < GX_DECODER_SUPPORT_MAX; i++) {
		if (cls->format[i] == codec_type)
			score = (score > 50) ? score : 50;
		else if (cls->format[i] == PLAYER_ACODEC_DUMMY)
			score = (score > 20) ? score : 20;
	}

	return score;
}

static int ad_hw_open(GxAVCodec*  ad)
{
	AdHwPriv* priv = NULL;
	GxStreamAudioHeader* header = (GxStreamAudioHeader*)ad->header;

	priv = (void* )av_malloc(sizeof(AdHwPriv));
	if (priv == NULL)
		return GX_PLAYER_ERROR;

	ad->priv = (void* )priv;
	memset(ad->priv, 0, sizeof(AdHwPriv));

	priv->dev = GxAvdev_CreateDevice(0);
	if(priv->dev < 0){
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	priv->mod = GxAvdev_OpenModule(priv->dev, GXAV_MOD_AUDIO_DECODE, ad->decoder_id);
	if(priv->mod < 0){
		GxAvdev_DestroyDevice(priv->dev);
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	GxPlayer_SystemGet(PSYS_ENABLE_AC3, &priv->enable_ac3);
	GxPlayer_SystemGet(PSYS_ENABLE_DTS, &priv->enable_dts);
	GxPlayer_SystemGet(PSYS_ENABLE_AAC, &priv->enable_aac);

	if(header){
		header->dec_dev = priv->dev;
		header->dec_mod = priv->mod;
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_hw_control(GxAVCodec*  ad, int cmd, void* args)
{
	int ret;
	AdHwPriv* priv = NULL;

	if(ad)
	{
		priv = (AdHwPriv*)ad->priv;
		if(priv)
		{
			switch (cmd)
			{
			case GX_AD_CTRL_PAUSE:
				{
					ret = GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Pause, NULL, 0);
					CHECK_RETURN_VALUE(ret);
				}
				break;
			case GX_AD_CTRL_RESUME:
				{
					ret = GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Resume, NULL, 0);
					CHECK_RETURN_VALUE(ret);
				}
				break;
			case GX_AD_CTRL_UPDATE:
				{
					ret = GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Update, NULL, 0);
					CHECK_RETURN_VALUE(ret);
				}
				break;
			case GX_AD_CTRL_REFRESH:
				{
					ret = GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Update, NULL, 0);
					CHECK_RETURN_VALUE(ret);
				}
				break;
			case GX_AD_CTRL_GET_FRAME_INFO:
				{
					int ret;
					GxAudioDecProperty_State state ;
					if(((GxStreamAudioHeader*)ad->header)->format == PLAYER_ACODEC_DUMMY)
						return GX_PLAYER_OK;

					ret = GxAVGetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_State, &state,sizeof(GxAudioDecProperty_State));
					if(ret == GXCORE_SUCCESS)
					{
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
									sh->i_bps = (sh->i_bps == 0) ? frame.bitrate / 8 : sh->i_bps;
									sh->samplerate = (sh->samplerate == 0) ? frame.samplefre : sh->samplerate;
									sh->channels = (sh->channels == 0) ? frame.channelnum : sh->channels;
									sh->dual_mono  = frame.dual_mono;
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
							case AUDIODEC_ERR_CODEC_ERROR:
								pstate->err_code = AVCODEC_ERR_CODEC_ERROR;
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
			case GX_AD_CTRL_GET_PCM_SIZE:
				{
					if (priv && priv->running) {
						*(int*)args = GxFifo_GetLength(priv->pcm_out);
					}else{
						*(int*)args = 0;
					}
					return GX_PLAYER_OK;
				}
			case GX_AD_CTRL_RESET:
				{
					ret = GxAVSetProperty(priv->dev, priv->mod, GxAudioDecoderPropertyID_Reset, NULL, 0);
					CHECK_RETURN_VALUE(ret);
				}
				break;
			case GX_AD_CTRL_SET_SPEED:
				{
					int ispeed = *(int*)args;
					return _set_audio_speed(priv, ispeed);
				}
			case GX_AD_CTRL_SUPPORT_SPEED:
				{
					*(int *)args = priv->support_speed;
					return GX_PLAYER_OK;
				}

			default:
				return GX_PLAYER_ERROR;
			}
		}
	}

	return GX_PLAYER_OK;
}

static int ad_hw_close(GxAVCodec*  ad)
{
	AdHwPriv* priv = NULL;

	if(ad)
	{
		priv = (AdHwPriv*)ad->priv;
		if(priv)
		{
			ad_hw_stop((GxMediaFilter*)ad);
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAvdev_CloseModule(priv->dev, priv->mod);
			GxAvdev_DestroyDevice(priv->dev);
			av_free(ad->priv);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ad_hw_decode(GxAVCodec* ad, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	return 100;
}

static int ad_hw_reset(GxMediaFilter*  filter)
{
	if(filter)
	{
		ad_hw_stop(filter);
		ad_hw_config(filter);
		ad_hw_run(filter);

		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

GxAVCodecClass gx_audio_decoder_HW = {
	._inherit = {
		._inherit = {
			.name    = "AudioDecoderHW",
			.parent  = (void* )&gx_AudioDecoderBase,
			.size    = sizeof(GxAudioCodec),
		},
		.run    = ad_hw_run,
		.pause  = ad_hw_pause,
		.resume = ad_hw_resume,
		.config = ad_hw_config,
		.stop   = ad_hw_stop,
		.wait   = ad_hw_wait,
		.reset  = ad_hw_reset,
		.abort  = ad_hw_abort,
	},
	DEF_AUTHOR("AudioDecoder","hw","No description","L.F","No comment"),

	.name    = "HW  AudioDecoder",
	.format  = {
		AUDIO_CODEC_MPEG1,
		AUDIO_CODEC_MPEG2,
		AUDIO_CODEC_AAC_LATM,
		AUDIO_CODEC_AAC_ADTS,
		AUDIO_CODEC_VORBIS,
		AUDIO_CODEC_AC3,
		AUDIO_CODEC_AVS,
		AUDIO_CODEC_PCM,
		AUDIO_CODEC_EAC3,
		AUDIO_CODEC_DTS,
		AUDIO_CODEC_DRA1,
		AUDIO_CODEC_DTS_HD,
		AUDIO_CODEC_RA_AAC,
		AUDIO_CODEC_RA_RA8LBR,
		AUDIO_CODEC_FLAC,
		AUDIO_CODEC_OPUS,
		AUDIO_CODEC_SBC,
		PLAYER_ACODEC_DUMMY,
		-1 },
	.probe   = ad_hw_probe,
	.open    = ad_hw_open,
	.close   = ad_hw_close,
	.control = ad_hw_control,
	.decode  = ad_hw_decode,
};

