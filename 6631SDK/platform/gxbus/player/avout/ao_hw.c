
#include "ao.h"

typedef struct {
	int        dev;
	handle_t   mod;
	GxFifo*    fifo_pcm;
	GxFifo*    fifo_adpcm;
	GxFifo*    fifo_ac3;
	GxFifo*    fifo_eac3;
	GxFifo*    fifo_dts;
	GxFifo*    fifo_aac;
	int        ac3mode;
	int        aacmode;
	int        spdsrc;
	int        hdmisrc;
	int        sync_flag;
	int        running;
}AoHwPriv;

static void ao_set_sync(AoHwPriv *priv, int sync)
{
	GxAudioOutProperty_ConfigSync audio_sync;
	audio_sync.sync = sync;
	GxPlayer_SystemGet(PSYS_AOUT_SYNC_TIMEMS,      &audio_sync.low_tolerance);
	GxPlayer_SystemGet(PSYS_AOUT_SYNC_HIGH_TIMEMS, &audio_sync.high_tolerance);
	GxAVSetProperty(priv->dev, priv->mod,   \
			GxAudioOutPropertyID_ConfigSync, (void*)&audio_sync, sizeof(GxAudioOutProperty_ConfigSync));
}

static int ao_hw_run(GxMediaFilter*  filter)
{
	AoHwPriv* priv;
	GxAudioOut* ao = GXAOUT(filter);

	if(ao)
	{
		priv = (AoHwPriv*)ao->priv;
		if(priv && priv->running)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAVSetProperty(priv->dev, priv->mod, GxAudioOutPropertyID_Run,NULL,0);
		}
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int ao_hw_pause(GxMediaFilter*  filter)
{
	AoHwPriv* priv;
	GxAudioOut* ao = GXAOUT(filter);

	if(ao)
	{
		priv = (AoHwPriv*)ao->priv;
		if(priv && priv->running)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAVSetProperty(priv->dev, priv->mod, GxAudioOutPropertyID_Pause, NULL,0);
		}
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int ao_hw_resume(GxMediaFilter*  filter)
{
	AoHwPriv* priv;
	GxAudioOut* ao = GXAOUT(filter);

	if(ao)
	{
		priv = (AoHwPriv*)ao->priv;
		if(priv && priv->running)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAVSetProperty(priv->dev, priv->mod, GxAudioOutPropertyID_Resume, NULL,0);
		}
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int ao_hw_config(GxMediaFilter*  filter)
{
	int ret;
	AoHwPriv* priv;
	GxPin* ao_pcm_pin,*ao_ac3_pin,*ao_eac3_pin, *ao_dts_pin, *ao_aac_pin;
	GxPin* ao_adpcm_pin;
	GxAudioOut* ao = GXAOUT(filter);
	GxFifoLink FifoLink;
	GxFifoConfig FifoConfig;
	GxStreamAudioHeader* AudioHeader = NULL;
	GxStreamAudioHeader* AudioHeader1 = NULL;
	if(ao)
	{
		priv = (AoHwPriv*)ao->priv;
		if(priv)
		{
			GxAudioOutProperty_ConfigSource source;
			memset(&source, 0 ,sizeof(GxAudioOutProperty_ConfigSource));

			GxPlayer_SystemGet(PSYS_AOUT_AC3MODE,   &priv->ac3mode);
			GxPlayer_SystemGet(PSYS_AOUT_AACMODE,   &priv->aacmode);
			GxPlayer_SystemGet(PSYS_AOUT_SPDSRC,    &priv->spdsrc);
			GxPlayer_SystemGet(PSYS_AOUT_HDMISRC,   &priv->hdmisrc);
			GxPlayer_SystemGet(PSYS_AOUT_SYNC_FLAG, &priv->sync_flag);

			ao_pcm_pin   = GxMediaFilter_FindPin(filter, GX_AO_PIN_NAME_PCM_IN);
			ao_ac3_pin   = GxMediaFilter_FindPin(filter, GX_AO_PIN_NAME_AC3_IN);
			ao_eac3_pin  = GxMediaFilter_FindPin(filter, GX_AO_PIN_NAME_EAC3_IN);
			ao_adpcm_pin = GxMediaFilter_FindPin(filter, GX_AO_PIN_NAME_ADPCM_IN);
			ao_dts_pin   = GxMediaFilter_FindPin(filter, GX_AO_PIN_NAME_DTS_IN);
			ao_aac_pin   = GxMediaFilter_FindPin(filter, GX_AO_PIN_NAME_AAC_IN);

			AudioHeader = ao->header;
			AudioHeader1 = ao->header1;
			gxlogf("[Player]: fmt = 0x%x samplefre = %d channelnum = %d bitwidth = %d endian = %d\n",
					AudioHeader->format,
					AudioHeader->samplerate,
					AudioHeader->channels,
					AudioHeader->samplesize,
					AudioHeader->sample_format);

			priv->sync_flag = AudioHeader->priv.pts_sync?priv->sync_flag:0;
			ao_set_sync(priv, priv->sync_flag);

			source.audiodec_id = 0;
			source.indata_type = AudioHeader->data_type;

			if (AudioHeader->data_type == PCM_TYPE) {
				priv->spdsrc  = PCM_OUTPUT;
				priv->hdmisrc = PCM_OUTPUT;
			}

			ret = GxAVSetProperty(priv->dev, priv->mod,   \
					GxAudioOutPropertyID_ConfigSource, (void*)&source, sizeof(GxAudioOutProperty_ConfigSource));
			CHECK_RETURN_VALUE(ret);

			if(AudioHeader1){
				source.audiodec_id = 1;
				ret = GxAVSetProperty(priv->dev, priv->mod,   \
						GxAudioOutPropertyID_ConfigSource, (void*)&source, sizeof(GxAudioOutProperty_ConfigSource));
				CHECK_RETURN_VALUE(ret);
			}

			GxAudioOutProperty_ConfigPort config;
			config.port_type = AUDIO_OUT_I2S_IN_DAC;
			config.data_type = PCM_OUTPUT;
			ret = GxAVSetProperty(priv->dev, priv->mod,   \
					GxAudioOutPropertyID_ConfigPort, (void*)&config, sizeof(GxAudioOutProperty_ConfigPort));
			CHECK_RETURN_VALUE(ret);

			config.port_type = AUDIO_OUT_SPDIF;
			config.data_type = priv->spdsrc;
			ret = GxAVSetProperty(priv->dev, priv->mod,   \
					GxAudioOutPropertyID_ConfigPort, (void*)&config, sizeof(GxAudioOutProperty_ConfigPort));
			CHECK_RETURN_VALUE(ret);

			config.port_type = AUDIO_OUT_HDMI;
			config.data_type = priv->hdmisrc;
			ret = GxAVSetProperty(priv->dev, priv->mod,   \
					GxAudioOutPropertyID_ConfigPort, (void*)&config, sizeof(GxAudioOutProperty_ConfigPort));
			CHECK_RETURN_VALUE(ret);

			if ((ao_adpcm_pin && ao_adpcm_pin->source) &&
					(AudioHeader1 && ((AudioHeader1->data_type & ADPCM_TYPE) == ADPCM_TYPE))) {
				if (ao_adpcm_pin->source->fifo) {

					FifoConfig.empty_gate = ao_adpcm_pin->source->fifo->size/4;
					FifoConfig.full_gate  = ao_adpcm_pin->source->fifo->size;
					ret = GxFifo_Config(ao_adpcm_pin->source->fifo, &FifoConfig);
					CHECK_RETURN_VALUE(ret);

					GxFifo_Reset(ao_adpcm_pin->source->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_ADPCM;
					FifoLink.direction = GXAV_PIN_INPUT;
					ret = GxFifo_Link(ao_adpcm_pin->source->fifo, &FifoLink);
					CHECK_RETURN_VALUE(ret);

					priv->fifo_adpcm = ao_adpcm_pin->source->fifo;
				}
			}

			if ((ao_pcm_pin && ao_pcm_pin->source) &&
					(AudioHeader && ((AudioHeader->data_type & PCM_TYPE) == PCM_TYPE))) {
				if (ao_pcm_pin->source->fifo) {

					FifoConfig.empty_gate = ao_pcm_pin->source->fifo->size/4;
					FifoConfig.full_gate  = ao_pcm_pin->source->fifo->size;
					ret = GxFifo_Config(ao_pcm_pin->source->fifo, &FifoConfig);
					CHECK_RETURN_VALUE(ret);

					GxFifo_Reset(ao_pcm_pin->source->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_PCM;
					FifoLink.direction = GXAV_PIN_INPUT;
					ret = GxFifo_Link(ao_pcm_pin->source->fifo, &FifoLink);
					CHECK_RETURN_VALUE(ret);

					priv->fifo_pcm = ao_pcm_pin->source->fifo;
					priv->running = 1;
				}
			}

			if (ao_ac3_pin && ao_ac3_pin->source &&
					((AudioHeader->data_type & AC3_TYPE) == AC3_TYPE)) {
				if (ao_ac3_pin->source->fifo) {

					GxFifo_Reset(ao_ac3_pin->source->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_AC3;
					FifoLink.direction = GXAV_PIN_INPUT;
					ret = GxFifo_Link(ao_ac3_pin->source->fifo, &FifoLink);
					CHECK_RETURN_VALUE(ret);

					priv->fifo_ac3 = ao_ac3_pin->source->fifo;
					priv->running = 1;
				}
			}

			if (ao_eac3_pin && ao_eac3_pin->source &&
					((AudioHeader->data_type & EAC3_TYPE) == EAC3_TYPE)) {
				if (ao_eac3_pin->source->fifo) {

					GxFifo_Reset(ao_eac3_pin->source->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_EAC3;
					FifoLink.direction = GXAV_PIN_INPUT;
					ret = GxFifo_Link(ao_eac3_pin->source->fifo, &FifoLink);
					CHECK_RETURN_VALUE(ret);

					priv->fifo_eac3 = ao_eac3_pin->source->fifo;
					priv->running = 1;
				}
			}

			if (ao_dts_pin && ao_dts_pin->source &&
					((AudioHeader->data_type & DTS_TYPE) == DTS_TYPE)) {
				if (ao_dts_pin->source->fifo) {

					GxFifo_Reset(ao_dts_pin->source->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_DTS;
					FifoLink.direction = GXAV_PIN_INPUT;
					ret = GxFifo_Link(ao_dts_pin->source->fifo, &FifoLink);
					CHECK_RETURN_VALUE(ret);

					priv->fifo_dts = ao_dts_pin->source->fifo;
					priv->running = 1;
				}
			}

			if (ao_aac_pin && ao_aac_pin->source &&
					((AudioHeader->data_type & AAC_TYPE) == AAC_TYPE)) {
				if (ao_aac_pin->source->fifo) {

					GxFifo_Reset(ao_aac_pin->source->fifo);
					FifoLink.module = priv->mod;
					FifoLink.pin_id = GXAV_PIN_AAC;
					FifoLink.direction = GXAV_PIN_INPUT;
					ret = GxFifo_Link(ao_aac_pin->source->fifo, &FifoLink);
					CHECK_RETURN_VALUE(ret);

					priv->fifo_aac = ao_aac_pin->source->fifo;
					priv->running = 1;
				}
			}

			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int ao_hw_stop(GxMediaFilter*  filter)
{
	AoHwPriv* priv;
	GxFifoLink FifoLink;
	GxAudioOut* ao = GXAOUT(filter);

	if(ao)
	{
		priv = (AoHwPriv*)ao->priv;
		if(priv && priv->running)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

			GxAVSetProperty(priv->dev, priv->mod, GxAudioOutPropertyID_Stop,NULL,0);

			FifoLink.module = priv->mod;
			if (priv->fifo_adpcm) {
				GxFifo_Unlink(priv->fifo_adpcm, &FifoLink);
				priv->fifo_adpcm = NULL;
			}
			if (priv->fifo_pcm) {
				GxFifo_Unlink(priv->fifo_pcm, &FifoLink);
				priv->fifo_pcm = NULL;
			}
			if (priv->fifo_ac3) {
				GxFifo_Unlink(priv->fifo_ac3, &FifoLink);
				priv->fifo_ac3 = NULL;
			}
			if (priv->fifo_eac3) {
				GxFifo_Unlink(priv->fifo_eac3, &FifoLink);
				priv->fifo_eac3 = NULL;
			}
			if (priv->fifo_dts) {
				GxFifo_Unlink(priv->fifo_dts, &FifoLink);
				priv->fifo_dts = NULL;
			}
			if (priv->fifo_aac) {
				GxFifo_Unlink(priv->fifo_aac, &FifoLink);
				priv->fifo_aac = NULL;
			}

			priv->running = 0;

			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		}
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int ao_hw_open(GxAudioOut*  ao)
{
	AoHwPriv* priv = NULL;

	priv = (void* )av_malloc(sizeof(AoHwPriv));
	if (priv == NULL)
		return GX_PLAYER_ERROR;

	ao->priv = (void* )priv;
	memset(ao->priv, 0, sizeof(AoHwPriv));

	priv->dev = GxAvdev_CreateDevice(0);
	if(priv->dev < 0){
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	priv->mod = GxAvdev_OpenModule(priv->dev, GXAV_MOD_AUDIO_OUT, 0);
	if(priv->mod < 0){
		GxAvdev_DestroyDevice(priv->dev);
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	return GX_PLAYER_OK;
}


static int ao_hw_close(GxAudioOut*  ao, int immed)
{
	AoHwPriv* priv = NULL;

	if(ao)
	{
		priv = (AoHwPriv*)ao->priv;
		if(priv)
		{
			ao_hw_stop((GxMediaFilter*)ao);
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAvdev_CloseModule(priv->dev, priv->mod);
			GxAvdev_DestroyDevice(priv->dev);
			av_free(ao->priv);
			return GX_PLAYER_OK;
		}
	}
	return GX_PLAYER_ERROR;
}

static int ao_hw_control(GxAudioOut*  ao, int cmd, void *args)
{
	AoHwPriv* priv = NULL;

	if(ao)
	{
		priv = (AoHwPriv*)ao->priv;
		if(priv)
		{
			switch (cmd) {
			case GX_AO_CTRL_SET_SPEED:
				{
					int ispeed = *(int*)args;

					if (GX_SPEED_NORMAL(ispeed)) {
						GxAudioOutProperty_Speed speed;
						int speed_mute = 0;

						if (ispeed != 1000)
							GxPlayer_SystemGet(PSYS_AOUT_SPEED_MUTE, &speed_mute);

						speed.speed      = ispeed/10;
						speed.speed_mute = speed_mute;
						GxAVSetProperty(priv->dev, priv->mod, GxAudioOutPropertyID_Speed, &speed, sizeof(speed));
					}
					break;
				}
			case GX_AO_CTRL_GET_FRAME_INFO:
				{
					int ret;
					GxAudioOutProperty_State state ;

					ret = GxAVGetProperty(priv->dev, priv->mod,
							GxAudioOutPropertyID_State, &state,sizeof(GxAudioOutProperty_State));
					if(ret == GXCORE_SUCCESS)
					{
						GxAvFrameStat *stat = &((GxStreamAudioHeader*)ao->header)->stat;
						stat->play_frame_cnt = state.frame_stat.play_frame_cnt;
						stat->lose_sync_cnt  = state.frame_stat.lose_sync_cnt;
						stat->synced_flag    = state.frame_stat.synced_flag;
						stat->synced_state   = state.frame_stat.synced_state;
					}
					return GX_PLAYER_ERROR;
				}
				break;
			case GX_AO_CTRL_SET_SYNC:
				((GxStreamAudioHeader*)ao->header)->priv.pts_sync = *(int*)args;
				ao_set_sync(priv, *(int*)args);
				break;
			case GX_AO_CTRL_SET_PCM_MIX:
				{
					int mix = *(int*)args;
					GxAudioOutProperty_PcmMix pcm_mix;

					if (mix == (0x1 << 0))
						pcm_mix.pcm_mix = MIX_PCM;
					else if (mix == (0x1 << 1))
						pcm_mix.pcm_mix = MIX_ADPCM;
					else
						pcm_mix.pcm_mix = MIX_PCM|MIX_ADPCM;
					GxAVSetProperty(priv->dev, priv->mod,   \
							GxAudioOutPropertyID_PcmMix, (void*)&pcm_mix, sizeof(GxAudioOutProperty_PcmMix));
				}
				break;
			case GX_AO_CTRL_GET_CUR_PTS:
				{
					int ret;
					GxAudioOutProperty_Pts pts;

					pts.pts = 0;
					ret = GxAVGetProperty(priv->dev, priv->mod,
							GxAudioOutPropertyID_Pts, &pts,sizeof(GxAudioOutProperty_Pts));
					if(ret == GXCORE_SUCCESS)
						*(int32_t*)args = pts.pts;
					else
						*(int32_t*)args = -1;

					return GX_PLAYER_OK;
				}
				break;
			case GX_AO_CTRL_DRAIN_WAIT:
				{
					GxAPlayProperty_Control drain_frame;
					int retry_times = 100;

					while(retry_times > 0) {
						int is_over = 0;

						drain_frame.cmd   = GX_APLAY_CONTROL_DRAIN;
						drain_frame.arg10 = 0;
						GxAVGetProperty(priv->dev, priv->mod, GxAPlayPropertyID_Control, &drain_frame, sizeof(GxAPlayProperty_Control));
						is_over = drain_frame.arg10;
						if(is_over)
							break;
						retry_times--;
						GxCore_ThreadDelay(10);
					}
					return GX_PLAYER_OK;
				}
				break;
			case GX_AO_CTRL_SET_DELAY:
				{
					GxAudioOutProperty_PtsOffset pts;

					pts.offset_ms = *(int32_t*)args;
					return GxAVSetProperty(priv->dev, priv->mod,
							GxAudioOutPropertyID_PtsOffset, &pts, sizeof(GxAudioOutProperty_PtsOffset));
				}
				break;
			default:
				return GX_PLAYER_ERROR;
			}
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_OK;
}


static int ao_hw_play(GxAudioOut*  ao, void *data, int len, int flags)
{
	return GX_PLAYER_OK;
}

static int ao_hw_reset(GxMediaFilter*  filter)
{
	if (filter) {
		ao_hw_stop(filter);
		ao_hw_config(filter);
		ao_hw_run(filter);

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

GxAudioOutClass gx_aout_hw = {
	._inherit = {              // GxMediaFilter
		._inherit     = {  // GxObject
			.name    = "Aout Base",
			.parent  = &gx_AudioOutBase,
			.size    = sizeof(GxAudioOut),
			.create  = NULL,
			.release = NULL,
		},
		.run          = ao_hw_run,
		.pause        = ao_hw_pause,
		.resume       = ao_hw_resume,
		.config       = ao_hw_config,
		.stop         = ao_hw_stop,
		.reset        = ao_hw_reset,
	},
	DEF_AUTHOR("AudioOut","hw","No description","L.F","No comment"),

	.name    = "HW AudioPlayer",
	.open    = ao_hw_open,
	.close   = ao_hw_close,
	.control = ao_hw_control,
	.play    = ao_hw_play,
};
