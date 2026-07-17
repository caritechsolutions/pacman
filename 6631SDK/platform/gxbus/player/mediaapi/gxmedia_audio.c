#include "gxmedia_module.h"
#include "gxmedia_common.h"

static int GxAudioDec_Open(GxMediaAudio* maudio, int modid)
{
	maudio->dec_dev = GxAvdev_CreateDevice(0);
	if (maudio->dec_dev < 0)
		return GX_MEDIAAPI_ERROR;

	maudio->dec_mod = GxAvdev_OpenModule(maudio->dec_dev, GXAV_MOD_AUDIO_DECODE, modid);
	if (maudio->dec_mod < 0)
		return GX_MEDIAAPI_ERROR;

	return GX_MEDIAAPI_SUCCESS;
}

static void GxAudioDec_Close(GxMediaAudio* maudio)
{
	if (maudio->dec_mod != -1)
		GxAvdev_CloseModule(maudio->dec_dev, maudio->dec_mod);

	if (maudio->dec_dev != -1)
		GxAvdev_DestroyDevice(maudio->dec_dev);

	maudio->dec_mod = -1;
	maudio->dec_dev = -1;

	return;
}

static int GxAudioDec_Config(GxMediaAudio* maudio)
{
	int ret = 0;
	GxAudioDecProperty_Config dec_config;
	GxAudioDecProperty_Capability dec_cap;
	GxAudioDecProperty_OutInfo out_info;
	GxAvGateInfo fifo_gate;

	if (maudio->mdemux == NULL) {
		maudio->es_buffer = GxMediaAPI_MemAlloc(AUDIO_ESA_SIZE, GX_PINFLAG_SESA);
		if (maudio->es_buffer == NULL) {
			GxMediaAPI_debug("[%s %d]: audio in hole create error\n", __FUNCTION__, __LINE__);
			return GX_MEDIAAPI_ERROR;
		}

		maudio->es_fifo = GxAVFifoCreate(maudio->dec_dev, maudio->es_buffer, AUDIO_ESA_SIZE, GXAV_PTS_FIFO);
		if(maudio->es_fifo == -1){
			GxMediaAPI_debug("[%s %d]: audio esa fifo create error\n", __FUNCTION__, __LINE__);
			GxMediaAPI_MemFree(maudio->es_buffer, GX_PINFLAG_SESA);
			maudio->es_buffer = NULL;
			return GX_MEDIAAPI_ERROR;
		}
	} else
		maudio->es_fifo = GxMedia_DemuxGetAfifo(maudio->mdemux);

	memset(&fifo_gate, 0, sizeof(GxAvGateInfo));
	fifo_gate.almost_empty = AUDIO_ESA_SIZE*7/8;
	fifo_gate.almost_full  = 1024;
	GxAVFifoConfig(maudio->dec_dev, maudio->es_fifo, &fifo_gate);

	memset(&dec_config, 0, sizeof(GxAudioDecProperty_Config));
	memset(&dec_cap,    0, sizeof(GxAudioDecProperty_Capability));
	dec_config.type = acodec_std2hw(maudio->params.codectype);
	if (dec_config.type == -1) {
		GxMediaAPI_debug("[%s %d]: config format %x error\n", __FUNCTION__, __LINE__, maudio->params.codectype);
		return GX_MEDIAAPI_ERROR;
	}

	dec_cap.type = dec_config.type;
	GxAVGetProperty(maudio->dec_dev, maudio->dec_mod, \
			GxAudioDecoderPropertyID_Capability, &dec_cap, sizeof(GxAudioDecProperty_Capability));

	if (dec_cap.workbuf_size) {
		maudio->work_size = dec_cap.workbuf_size;
		maudio->work_mem  = GxMediaAPI_MemAlloc(maudio->work_size, GX_PINFLAG_WKA);
		if (maudio->work_mem == NULL) {
			GxMediaAPI_debug("[%s %d]: audio men malloc failed\n", __FUNCTION__, __LINE__);
			return GX_MEDIAAPI_ERROR;
		}
	}

	dec_config.down_mix    = 1;
	dec_config.stream_type = (maudio->params.stream_data == GXMEDIA_AVSTREAM_TS) ? STREAM_TS : STREAM_ES;
	dec_config.stream_pid  = maudio->params.stream_pid;
	dec_config.mode        = (maudio->params.bypass == 0) ? DECODE_MODE : BYPASS_MODE;
	dec_config.ormem_size  = maudio->work_size;
	dec_config.ormem_start = maudio->work_mem;
	dec_config.enough_data = maudio->params.enough_data;
	ret = GxAVSetProperty(maudio->dec_dev, maudio->dec_mod,
			GxAudioDecoderPropertyID_Config, &dec_config,sizeof(GxAudioDecProperty_Config));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio decoder config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	if(maudio->params.codectype == AUDIO_CODEC_PCM) {
		GxAudioDecProperty_PcmInfo pcm_info;

		memset(&pcm_info, 0, sizeof(GxAudioDecProperty_PcmInfo));
		if((maudio->params.sample_rate == 0) ||
				(maudio->params.channel_num == 0) ||
				(maudio->params.sample_bits == 0))
			return GX_MEDIAAPI_ERROR;

		pcm_info.endian     = maudio->params.big_endian;
		pcm_info.samplefre  = maudio->params.sample_rate;
		pcm_info.channelnum = maudio->params.channel_num;
		pcm_info.bitwidth   = maudio->params.sample_bits;
		pcm_info.intelace   = 1;
		pcm_info.channelflag= 0;
		GxAVSetProperty(maudio->dec_dev, maudio->dec_mod,
				GxAudioDecoderPropertyID_PcmInfo, &pcm_info, sizeof(GxAudioDecProperty_PcmInfo));
	}
	GxAVFifoReset (maudio->dec_dev, maudio->es_fifo);

	GxAVGetProperty(maudio->dec_dev, maudio->dec_mod,
			GxAudioDecoderPropertyID_OutInfo, &out_info,sizeof(GxAudioDecProperty_OutInfo));
	maudio->dsp_outdata = out_info.outdata_type;

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_Start(GxMediaAudio* maudio)
{
	int ret = 0;

	GxAVLinkFifo(maudio->dec_dev,  maudio->dec_mod, maudio->es_fifo,  GXAV_PIN_ESA, GXAV_PIN_INPUT);
	if (maudio->dsp_outdata == PCM_TYPE)
		GxAVLinkFifo(maudio->dec_dev,  maudio->dec_mod, maudio->pcm_fifo, GXAV_PIN_PCM, GXAV_PIN_OUTPUT);
	else if (maudio->dsp_outdata == ADPCM_TYPE)
		GxAVLinkFifo(maudio->dec_dev,  maudio->dec_mod, maudio->pcm_fifo, GXAV_PIN_ADPCM, GXAV_PIN_OUTPUT);
	else if (maudio->dsp_outdata == AC3_TYPE)
		GxAVLinkFifo(maudio->dec_dev,  maudio->dec_mod, maudio->pcm_fifo, GXAV_PIN_AC3, GXAV_PIN_OUTPUT);
	else if (maudio->dsp_outdata == EAC3_TYPE)
		GxAVLinkFifo(maudio->dec_dev,  maudio->dec_mod, maudio->pcm_fifo, GXAV_PIN_EAC3, GXAV_PIN_OUTPUT);
	else if (maudio->dsp_outdata == DTS_TYPE)
		GxAVLinkFifo(maudio->dec_dev,  maudio->dec_mod, maudio->pcm_fifo, GXAV_PIN_DTS, GXAV_PIN_OUTPUT);
	else {
		GxMediaAPI_debug("[%s %d]: audio decoder fifo link failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	ret = GxAVSetProperty(maudio->dec_dev, maudio->dec_mod, GxAudioDecoderPropertyID_Run, NULL,0);
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio decoder run failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_Stop(GxMediaAudio* maudio)
{
	GxAVSetProperty(maudio->dec_dev, maudio->dec_mod, GxAudioDecoderPropertyID_Stop, NULL, 0);
	GxAVResetEvent(maudio->dec_dev, maudio->dec_mod, EVENT_AUDIO_FRAMEINFO);
	GxAVUnLinkFifo(maudio->dec_dev, maudio->dec_mod, maudio->es_fifo);
	GxAVUnLinkFifo(maudio->dec_dev, maudio->dec_mod, maudio->pcm_fifo);

	if (maudio->work_mem) {
		GxMediaAPI_MemFree(maudio->work_mem, GX_PINFLAG_WKA);
		maudio->work_mem = NULL;
	}

	if (maudio->mdemux == NULL) {
		if (maudio->es_fifo != -1)
			GxAVFifoDestroy(maudio->dec_dev, maudio->es_fifo);

		if (maudio->es_buffer)
			GxMediaAPI_MemFree(maudio->es_buffer , GX_PINFLAG_SESA);
	}
	maudio->es_fifo = -1;
	maudio->es_buffer = NULL;

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_Pause(GxMediaAudio* maudio)
{
	GxAVSetProperty(maudio->dec_dev, maudio->dec_mod, GxAudioDecoderPropertyID_Pause, NULL, 0);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_Resume(GxMediaAudio* maudio)
{
	GxAVSetProperty(maudio->dec_dev, maudio->dec_mod, GxAudioDecoderPropertyID_Resume, NULL, 0);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_GetEsaInfo(GxMediaAudio* maudio, GxMediaModuleAudioFifo *info)
{
	int esa_fifo_size = 0;
	int esa_used_size = 0;
	int pcm_fifo_size = 0;
	int pcm_used_size = 0;

	if ((maudio->es_fifo == -1) ||
			(maudio->pcm_fifo == -1))
		return GX_MEDIAAPI_ERROR;

	esa_fifo_size = GxAVFifoGetCap   (maudio->dec_dev, maudio->es_fifo);
	esa_used_size = GxAVFifoGetLength(maudio->dec_dev, maudio->es_fifo);
	pcm_fifo_size = GxAVFifoGetCap   (maudio->dec_dev, maudio->pcm_fifo);
	pcm_used_size = GxAVFifoGetLength(maudio->dec_dev, maudio->pcm_fifo);

	if (info) {
		info->esa_free_size = esa_fifo_size - esa_used_size;
		info->esa_used_size = esa_used_size;
		info->pcm_free_size = pcm_fifo_size - pcm_used_size;
		info->pcm_used_size = pcm_used_size;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_GetDecState(GxMediaAudio* maudio, GxMediaDecState *state, GxMediaDecErrCode *err_code)
{
	int ret = GXCORE_SUCCESS;
	GxAudioDecProperty_State dec_state ;

	ret = GxAVGetProperty(maudio->dec_dev, maudio->dec_mod,
			GxAudioDecoderPropertyID_State, &dec_state, sizeof(GxAudioDecProperty_State));
	if (ret != GXCORE_SUCCESS)
		return GX_MEDIAAPI_ERROR;

	switch (dec_state.state) {
	case AUDIODEC_READY:
	case AUDIODEC_RUNNING:
	case AUDIODEC_PAUSED:
		*state    = GXMEDIA_STATE_RUNNING;
		*err_code = GXMEDIA_ERR_NO_ERROR;
		break;
	case AUDIODEC_STOPED:
	case AUDIODEC_OVER:
		*state    = GXMEDIA_STATE_STOPPED;
		*err_code = GXMEDIA_ERR_NO_ERROR;
		break;
	case AUDIODEC_ERROR:
		*state    = GXMEDIA_STATE_ERROR;
		switch (dec_state.err_code) {
		case AUDIODEC_ERR_ESA_OVERFLOW:
			*err_code = GXMEDIA_ERR_ES_OVERFLOW;
			break;
		case AUDIODEC_ERR_UNSUPPORT_CODECTYPE:
			*err_code = GXMEDIA_ERR_DEC_UNSUPPORT;
			break;
		default:
			*err_code = GXMEDIA_ERR_UNKOWN_ERROR;
			break;
		}
		break;
	default:
		*state    = GXMEDIA_STATE_NO_DEC;
		*err_code = GXMEDIA_ERR_NO_ERROR;
		break;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_InjectData(GxMediaAudio* maudio, uint8_t* buffer, int len, int pts)
{
	int ret = 0;
	int write_size = 0;
	GxMediaModuleAudioFifo info;

	ret = GxAudioDec_GetEsaInfo(maudio, &info);
	if (ret != GX_MEDIAAPI_SUCCESS)
		return -1;

	if (info.esa_free_size <= len)
		return 0;

	write_size = GxAVFifoWriteDataPts(maudio->dec_dev, maudio->es_fifo, buffer, len, -1, pts);
	while (write_size < len) {
		ret = GxAVFifoWriteData(maudio->dec_dev, maudio->es_fifo, buffer+write_size, len-write_size, -1);
		if (ret < 0) {
			GxMediaAPI_debug("[%s %d]: audio decoder write data error\n", __FUNCTION__, __LINE__);
			return -1;
		}
		write_size += ret;
		if (write_size < len)
			GxCore_ThreadDelay(DELAY10MS);
	}

	return write_size;
}

static int GxAudioDec_Eos(GxMediaAudio* maudio)
{
	int ret = GxAVSetProperty(maudio->dec_dev, maudio->dec_mod, GxAudioDecoderPropertyID_Update, NULL, 0);

	return (ret == 0) ? GX_MEDIAAPI_SUCCESS : GX_MEDIAAPI_ERROR;
}

static int GxAudioDec_Finished(GxMediaAudio* maudio)
{
	int ret = 0;
	GxAudioDecProperty_State state;

	ret = GxAVSetProperty(maudio->dec_dev, maudio->dec_mod, GxAudioDecoderPropertyID_Update, NULL, 0);
	while (ret >= 0) {
		ret = GxAVGetProperty(maudio->dec_dev, maudio->dec_mod, GxAudioDecoderPropertyID_State, &state, sizeof(GxVideoDecoderProperty_State));
		if(state.state == AUDIODEC_OVER ||
				state.state == AUDIODEC_STOPED ||
				state.state == AUDIODEC_READY ||
				state.state == AUDIODEC_ERROR)
			break;
		GxCore_ThreadDelay(DELAY10MS);
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioDec_Speed(GxMediaAudio* maudio, float speed)
{
	int ret = 0;
	GxADSPProperty_Control actrl;

	if (speed < 0) return GX_MEDIAAPI_ERROR;

	actrl.cmd  = GX_ADSP_CONTROL_SET_SPEED_RATE;
	actrl.arg4 = (unsigned int)(100 * speed);
	ret = GxAVSetProperty(maudio->dec_dev, maudio->dec_mod,   \
			GxADSPPropertyID_Control, (void*)&actrl, sizeof(GxADSPProperty_Control));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio decoder set speed error\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioOut_Open(GxMediaAudio* maudio, int modid)
{
	GxAvGateInfo dec_output_info;

	maudio->out_dev = GxAvdev_CreateDevice(0);
	CHECK_RETURN_VALUE(maudio->out_dev);

	maudio->out_mod   = GxAvdev_OpenModule(maudio->out_dev, GXAV_MOD_AUDIO_OUT, modid);
	maudio->dsp_modid = modid;
	CHECK_RETURN_VALUE(maudio->out_mod);

	//创建输出数据通道以及配置其属性，连接解码器
	maudio->pcm_buffer = GxMediaAPI_MemAlloc(AUDIO_PCM_SIZE, GX_PINFLAG_PCM);
	if(maudio->pcm_buffer == NULL){
		GxMediaAPI_debug("[%s %d]: audio pcm hole create error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	maudio->pcm_fifo = GxAVFifoCreate(maudio->out_dev, maudio->pcm_buffer, AUDIO_PCM_SIZE, GXAV_NO_PTS_FIFO);
	if(maudio->pcm_fifo == -1){
		GxMediaAPI_debug("[%s %d]: audio pcm fifo create error\n", __FUNCTION__, __LINE__);
		GxMediaAPI_MemFree(maudio->pcm_buffer, GX_PINFLAG_PCM);
		maudio->pcm_buffer = NULL;
		return -1;
	}

	memset(&dec_output_info, 0, sizeof(GxAvGateInfo));
	dec_output_info.almost_empty = AUDIO_PCM_SIZE*7/8;
	dec_output_info.almost_full = 4096;
	GxAVFifoConfig(maudio->out_dev, maudio->pcm_fifo, &dec_output_info);
	return 0;
}

static void GxAudioOut_Close(GxMediaAudio* maudio)
{
	if (maudio->pcm_fifo != -1)
		GxAVFifoDestroy(maudio->out_dev, maudio->pcm_fifo);

	if (maudio->pcm_buffer)
		GxMediaAPI_MemFree(maudio->pcm_buffer, GX_PINFLAG_PCM);

	if (maudio->out_mod != -1)
		GxAvdev_CloseModule(maudio->out_dev, maudio->out_mod);

	if (maudio->out_dev != -1)
		GxAvdev_DestroyDevice(maudio->out_dev);

	maudio->pcm_fifo = -1;
	maudio->pcm_buffer = NULL;
	maudio->out_mod = -1;
	maudio->out_dev = -1;

	return;
}

static int GxAudioOut_Config(GxMediaAudio* maudio, GxMediaSyncParam *sync)
{
	int ret = 0;
	GxAudioOutProperty_ConfigPort   port_config;
	GxAudioOutProperty_ConfigSource source_config;
	GxAudioOutProperty_ConfigSync   sync_config;
	GxAudioOutProperty_Volume  volume;

	volume.right_now = maudio->params.volume.right_now;
	volume.volume    = maudio->params.volume.target_value;
	volume.step      = maudio->params.volume.step;
	GxAVSetProperty(maudio->out_dev, maudio->out_mod,
			GxAudioOutPropertyID_Volume, (void*)&volume, sizeof(GxAudioOutProperty_Volume));

	memset(&source_config, 0, sizeof(GxAudioOutProperty_ConfigSource));
	source_config.indata_type = maudio->dsp_outdata;
	source_config.audiodec_id = maudio->dsp_modid;
	ret = GxAVSetProperty(maudio->out_dev, maudio->out_mod,
			GxAudioOutPropertyID_ConfigSource, &source_config, sizeof(GxAudioOutProperty_ConfigSource));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio out config source failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	memset(&port_config, 0, sizeof(GxAudioOutProperty_ConfigPort));
	port_config.port_type = AUDIO_OUT_I2S_IN_DAC|AUDIO_OUT_HDMI|AUDIO_OUT_SPDIF;
	port_config.data_type = SMART_OUTPUT;
	ret = GxAVSetProperty(maudio->out_dev,  maudio->out_mod,
			GxAudioOutPropertyID_ConfigPort, &port_config, sizeof(GxAudioOutProperty_ConfigPort));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio out config port failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	sync_config.sync           = sync->enable;
	sync_config.low_tolerance  = sync->low_gate;
	sync_config.high_tolerance = sync->high_gate;
	ret = GxAVSetProperty(maudio->out_dev,  maudio->out_mod,
			GxAudioOutPropertyID_ConfigSync, &sync_config, sizeof(GxAudioOutProperty_ConfigSync));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio out config sync failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}
	GxAVFifoReset (maudio->out_dev, maudio->pcm_fifo);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioOut_Start(GxMediaAudio* maudio)
{
	int ret = 0;

	if (maudio->dsp_outdata == PCM_TYPE)
		GxAVLinkFifo(maudio->out_dev,  maudio->out_mod,  maudio->pcm_fifo, GXAV_PIN_PCM, GXAV_PIN_INPUT);
	else if (maudio->dsp_outdata == ADPCM_TYPE)
		GxAVLinkFifo(maudio->out_dev,  maudio->out_mod,  maudio->pcm_fifo, GXAV_PIN_ADPCM, GXAV_PIN_INPUT);
	else if (maudio->dsp_outdata == AC3_TYPE)
		GxAVLinkFifo(maudio->out_dev,  maudio->out_mod,  maudio->pcm_fifo, GXAV_PIN_AC3, GXAV_PIN_INPUT);
	else if (maudio->dsp_outdata == EAC3_TYPE)
		GxAVLinkFifo(maudio->out_dev,  maudio->out_mod,  maudio->pcm_fifo, GXAV_PIN_EAC3, GXAV_PIN_INPUT);
	else if (maudio->dsp_outdata == DTS_TYPE)
		GxAVLinkFifo(maudio->out_dev,  maudio->out_mod,  maudio->pcm_fifo, GXAV_PIN_DTS, GXAV_PIN_INPUT);
	else {
		GxMediaAPI_debug("[%s %d]: audio decoder fifo link failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	ret = GxAVSetProperty(maudio->out_dev, maudio->out_mod, GxAudioOutPropertyID_Run, NULL, 0);
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio out run failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioOut_Stop(GxMediaAudio* maudio)
{
	GxAVSetProperty(maudio->out_dev, maudio->out_mod, GxAudioOutPropertyID_Stop, NULL, 0);
	GxAVUnLinkFifo(maudio->out_dev, maudio->out_mod, maudio->pcm_fifo);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioOut_Pause(GxMediaAudio* maudio)
{
	int ret = 0;

	ret = GxAVSetProperty(maudio->out_dev, maudio->out_mod, GxAudioOutPropertyID_Pause, NULL, 0);
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: audio decoder pause failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioOut_Resume(GxMediaAudio* maudio)
{
	GxAVSetProperty(maudio->out_dev, maudio->out_mod, GxAudioOutPropertyID_Resume, NULL, 0);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxAudioOut_GetTime(GxMediaAudio* maudio, int64_t* adec_pts)
{
	int ret = -1;

	if (adec_pts != NULL) {
		GxAudioOutProperty_Pts pts;

		pts.pts = 0;
		ret = GxAVGetProperty(maudio->out_dev, maudio->out_mod,
				GxAudioOutPropertyID_Pts, &pts,sizeof(GxAudioOutProperty_Pts));
		*adec_pts = (int64_t)pts.sync_pts;
	}

	return ((ret == 0) ? GX_MEDIAAPI_SUCCESS : GX_MEDIAAPI_ERROR);
}

GxMediaAudio* GxMedia_AudioOpen(GxMediaAudioMod mod)
{
	int ret = 0;
	GxMediaAudio* media_audio = av_mallocz(sizeof(GxMediaAudio));

	if (media_audio == NULL) return NULL;

	media_audio->mdemux   = NULL;
	media_audio->dec_dev  = -1;
	media_audio->dec_mod  = -1;
	media_audio->out_dev  = -1;
	media_audio->out_mod  = -1;
	media_audio->es_fifo  = -1;
	media_audio->pcm_fifo = -1;
	media_audio->valid    = 1;

	ret = GxAudioDec_Open(media_audio, mod.adsp_modid);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxMediaAPI_debug("[%s %d]: audio decoder open failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	ret = GxAudioOut_Open(media_audio, mod.aplay_modid);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxMediaAPI_debug("[%s %d]: audio out open failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}
	media_audio->dsp_modid = mod.adsp_modid;

	return media_audio;

open_error:
	GxMedia_AudioClose(media_audio);
	return NULL;
}

int GxMedia_AudioBindDemux(GxMediaAudio* media_audio, GxMediaDemux *media_demux)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	media_audio->mdemux = media_demux;

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_AudioClose(GxMediaAudio* media_audio)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	GxMedia_AudioStop(media_audio);
	GxAudioOut_Close(media_audio);
	GxAudioDec_Close(media_audio);
	media_audio->valid = 0;
	av_free(media_audio);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_AudioConfig(GxMediaAudio* media_audio, GxMediaAudioPara param)
{
	int ret = GX_MEDIAAPI_ERROR;

	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	media_audio->params = param;
	ret = GxAudioDec_Config(media_audio);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxAudioDec_Stop(media_audio);
		GxAudioOut_Stop(media_audio);
		GxMediaAPI_debug("[%s %d]: audio decoder config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	ret = GxAudioOut_Config(media_audio, &param.sync);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxAudioDec_Stop(media_audio);
		GxAudioOut_Stop(media_audio);
		GxMediaAPI_debug("[%s %d]: audio decoder config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_AudioStart(GxMediaAudio* media_audio)
{
	int ret = GX_MEDIAAPI_ERROR;

	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	if (media_audio->running == 1)
		return GX_MEDIAAPI_SUCCESS;

	ret = GxAudioDec_Start(media_audio);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxAudioDec_Stop(media_audio);
		GxAudioOut_Stop(media_audio);
		GxMediaAPI_debug("[%s %d]: audio decoder config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	ret = GxAudioOut_Start(media_audio);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxAudioDec_Stop(media_audio);
		GxAudioOut_Stop(media_audio);
		GxMediaAPI_debug("[%s %d]: audio decoder config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	media_audio->running = 1;
	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_AudioStop(GxMediaAudio* media_audio)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	if (media_audio->running == 0)
		return GX_MEDIAAPI_SUCCESS;

	GxAudioDec_Stop(media_audio);
	GxAudioOut_Stop(media_audio);

	media_audio->running = 0;
	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_AudioPause(GxMediaAudio* media_audio)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	GxAudioDec_Pause(media_audio);
	GxAudioOut_Pause(media_audio);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_AudioResume(GxMediaAudio* media_audio)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	GxAudioOut_Resume(media_audio);
	GxAudioDec_Resume(media_audio);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_AudioInjectData(GxMediaAudio* media_audio, GxMediaSourceType type, uint8_t* buffer, int len, int pts)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	return GxAudioDec_InjectData(media_audio, buffer, len, pts);
}

int GxMedia_AudioEos(GxMediaAudio* media_audio)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	return GxAudioDec_Eos(media_audio);
}

int GxMedia_AudioFinished(GxMediaAudio* media_audio)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	return GxAudioDec_Finished(media_audio);
}

int GxMedia_AudioSpeed(GxMediaAudio* media_audio, float speed)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	return GxAudioDec_Speed(media_audio, speed);
}

int GxMedia_AudioGetPts(GxMediaAudio* media_audio, int64_t* apts)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	return GxAudioOut_GetTime(media_audio, apts);
}

int GxMedia_AudioGetFifoInfo(GxMediaAudio* media_audio, GxMediaModuleAudioFifo *info)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	return GxAudioDec_GetEsaInfo(media_audio, info);
}

int GxMedia_AudioGetDecState(GxMediaAudio* media_audio, GxMediaDecState *state, GxMediaDecErrCode *err_code)
{
	CHECK_MEDIA_MOLUDE_VALID(media_audio);

	return GxAudioDec_GetDecState(media_audio, state, err_code);
}
