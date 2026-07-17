
#include "ad.h"
#include "gx_device.h"
#include "gxplayer_decoder.h"

#ifdef CONFIG_AD_MP3
extern GxAVCodecClass gx_audio_decoder_MP3;
#endif
#ifdef CONFIG_AD_HW
extern GxAVCodecClass gx_audio_decoder_HW;
#endif
#ifdef CONFIG_AD_SW
extern GxAVCodecClass gx_audio_decoder_SW;
#endif
#ifdef CONFIG_AD_DUMMY
extern GxAVCodecClass gx_audio_decoder_dummy;
#endif

static GxAVCodecClass* gx_audio_decoder_classes[] = {
#ifdef CONFIG_AD_HW
	&gx_audio_decoder_HW,
#endif
#ifdef CONFIG_AD_SW
	&gx_audio_decoder_SW,
#endif
#ifdef CONFIG_AD_MP3
	&gx_audio_decoder_MP3,
#endif
#ifdef CONFIG_AD_DUMMY
	&gx_audio_decoder_dummy,
#endif
	NULL
};

void GxPlayerSwDecoderRegister(GxAudioDecoder *adec)
{
#ifdef CONFIG_AD_SW
	GxAudioRegisterDecoder(adec);
#endif
}

static void* adbase_create(GxObject * obj)
{
	return obj;
}

static void adbase_release(GxObject*  obj)
{
	return;
}

static void adbase_run_thread(void* filter)
{
	GxFifo* fifo = NULL;
	size_t len;
	GxPin* in_pin, *out_pin;
	unsigned char* data;
	GxMediaFilter* mf = GXMEDIAFILTER(filter);

	if(filter == NULL)
		return  ;

	in_pin = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_ESA_IN);
	out_pin = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_PCM_OUT);

	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {

		fifo  = in_pin->source->fifo;
		len = GxFifo_GetLength(fifo);

		if(len <= 0){
			if(in_pin->source->filter->status== GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				return  ;
			}
			GxCore_ThreadDelay(DELAY10MS);
			continue;
		}

		if(out_pin && out_pin->fifo){
			data = av_malloc(len);
			if(data){
				unsigned int write_size = 0;
				GxFifo_Read(fifo,data ,len, -1);

				write_size =GxFifo_Write(out_pin->fifo,data ,len, -1);

				while (write_size < len){
					write_size +=GxFifo_Write(out_pin->fifo,data+write_size ,len-write_size, -1);
					GxCore_ThreadDelay(DELAY10MS);
				}
				av_free(data);
			}
		}

		//GxCore_ThreadDelay(10);
	}
}

static int adbase_run(GxMediaFilter*  filter)
{
	GxAVCodec* d = GXDECODEROBJECT(filter);
	if (GxCore_ThreadCreate("adbase_run_thread", &d->run_pthread, adbase_run_thread, filter, 1024*10, GXOS_DEFAULT_PRIORITY) == 0) {
		filter->status = GX_MFT_STATE_RUNNING;
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int adbase_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int adbase_stop(GxMediaFilter*  filter)
{
	GxAVCodec* d = GXDECODEROBJECT(filter);
	GxCore_ThreadJoin(d->run_pthread);

	return GX_PLAYER_OK;
}

static int adbase_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int adbase_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

int GxAudioDecoder_BufferInit(GxAudioCodec*  AudioCodec)
{

	if (AudioCodec == NULL)
		return GX_PLAYER_ERROR;

	AudioCodec->out_minsize = 4608;
	if (AudioCodec->out_minsize > 0) {
		AudioCodec->out_buffer_size = 65535;	/* worst case calc.*/
		AudioCodec->out_buffer = av_malloc(AudioCodec->out_buffer_size);
		if (!AudioCodec->out_buffer) {
			gxlogf("MSGTR_CantAllocAudioBuf");
			return GX_PLAYER_ERROR;
		}
	}

	memset(AudioCodec->out_buffer, 0, AudioCodec->out_buffer_size);
	AudioCodec->out_buffer_len = 0;

	return GX_PLAYER_OK;
}

static GxAVCodecClass*  _find_audio_codec(AudioCodecType codec)
{
	unsigned int i = 0;
	GxAVCodecClass* padec = NULL;
	int score = 0, need_decode = 1;

	if (codec == AUDIO_CODEC_AC3 || codec == AUDIO_CODEC_EAC3) {
		AudioAc3Mode ac3mode;
		GxPlayer_SystemGet(PSYS_AOUT_AC3MODE, &ac3mode);
		need_decode = (ac3mode & AUDIO_AC3_DECODE_MODE) ? 1 : 0;
	} else if (codec == AUDIO_CODEC_AAC_LATM || codec == AUDIO_CODEC_AAC_ADTS) {
		AudioAACMode aacmode;
		GxPlayer_SystemGet(PSYS_AOUT_AACMODE, &aacmode);
		need_decode = (aacmode & AUDIO_AAC_DECODE_MODE) ? 1 : 0;
	}

	for (i = 0; gx_audio_decoder_classes[i] != NULL; i++) {
		if (gx_audio_decoder_classes[i]->probe) {
			int probe_score = gx_audio_decoder_classes[i]->probe(codec, need_decode);
			if (probe_score > score) {
				padec = gx_audio_decoder_classes[i];
				score = probe_score;
			}
		}
	}

	return padec;
}

int GxAudioDecoder_CheckReOpen(AudioCodecType codec1, AudioCodecType codec2)
{
	GxAVCodecClass* padec1 = _find_audio_codec(codec1);
	GxAVCodecClass* padec2 = _find_audio_codec(codec2);

	return (padec1 != padec2) ? 1 : 0;
}

GxAudioCodec* GxAudioDecoder_Open(GxStreamAudioHeader * header)
{
	int i, enable_ac3, enable_dts, enable_aac;
	int pcm, ac3, eac3, dts, aac;
	int score = 0;

	GxAVCodecClass* padec = NULL;
	GxAudioCodec* audio_codec = NULL;
	if(header == NULL)
		return NULL;

	GxPlayer_SystemGet(PSYS_ENABLE_AC3,  &enable_ac3);
	GxPlayer_SystemGet(PSYS_ENABLE_DTS,  &enable_dts);
	GxPlayer_SystemGet(PSYS_ENABLE_AAC,  &enable_aac);
	GxPlayer_SystemGet(PSYS_BufSizePCM,  &pcm);
	GxPlayer_SystemGet(PSYS_BufSizeAC3,  &ac3);
	GxPlayer_SystemGet(PSYS_BufSizeEAC3, &eac3);
	GxPlayer_SystemGet(PSYS_BufSizeDTS,  &dts);
	GxPlayer_SystemGet(PSYS_BufSizeAAC,  &aac);

	gxlogf("[Player]: A_FMT = 0x%x\n",header->format);
	padec = _find_audio_codec(header->format);

	if (padec) {
		audio_codec = GxObjectNew(NULL, padec);
		if(header->audio_id == 1)
			audio_codec->parent.decoder_id = 1;
		else
			audio_codec->parent.decoder_id = 0;

		if (audio_codec != NULL) {
			audio_codec->parent.header = (void* )header;
			if(padec->open){
				if(padec->open(GXDECODEROBJECT(audio_codec))){
					GxObjectDestroy(audio_codec);
					return NULL ;
				}
			}
			gxlogf("[Player]: AD = [%s]\n", padec->name);
			if(header->audio_id == 1){
				GxPin_Create(GX_AD_PIN_NAME_ADESA_IN, audio_codec, GX_PINDIR_INPUT,  0,   GX_PINFLAG_NO_PTS_FIFO);
				GxPin_Create(GX_AD_PIN_NAME_ADPCM_OUT,audio_codec, GX_PINDIR_OUTPUT, pcm, GX_PINFLAG_PCM1);
			}else{
				GxPin_Create(GX_AD_PIN_NAME_ESA_IN,  audio_codec, GX_PINDIR_INPUT,  0,   GX_PINFLAG_NO_PTS_FIFO);
				GxPin_Create(GX_AD_PIN_NAME_PCM_OUT, audio_codec, GX_PINDIR_OUTPUT, pcm, GX_PINFLAG_PCM);
				if (header->format == AUDIO_CODEC_AC3 || header->format == AUDIO_CODEC_EAC3) {
					if (enable_ac3) {
						GxPin_Create(GX_AD_PIN_NAME_AC3_OUT,  audio_codec, GX_PINDIR_OUTPUT, ac3,  GX_PINFLAG_AC3);
						GxPin_Create(GX_AD_PIN_NAME_EAC3_OUT, audio_codec, GX_PINDIR_OUTPUT, eac3, GX_PINFLAG_EAC3);
					}
				} else if (header->format == AUDIO_CODEC_DTS || header->format == AUDIO_CODEC_DTS_HD) {
					if (enable_dts)
						GxPin_Create(GX_AD_PIN_NAME_DTS_OUT, audio_codec, GX_PINDIR_OUTPUT, dts, GX_PINFLAG_DTS);
				} else if (header->format == AUDIO_CODEC_AAC_LATM || header->format == AUDIO_CODEC_AAC_ADTS) {
					if (enable_aac) {
						GxPin_Create(GX_AD_PIN_NAME_AC3_OUT, audio_codec, GX_PINDIR_OUTPUT, ac3, GX_PINFLAG_AC3);
						GxPin_Create(GX_AD_PIN_NAME_AAC_OUT, audio_codec, GX_PINDIR_OUTPUT, aac, GX_PINFLAG_AAC);
					}
				}
			}
			return (audio_codec);
		}

	}

	return (audio_codec);
}

void GxAudioDecoder_Close(GxAudioCodec*  codec)
{
	GxAVCodecClass* padec;

	if (codec )
	{
		if((padec=GxGetDecoderClassFromObject(codec))!=NULL)
			padec->close(GXDECODEROBJECT(codec));

		GxObjectDestroy(codec);
	}
}

int GxAudioDecoder_Decode(GxAudioCodec*  codec, unsigned char *buffer, int minlen, int maxlen)
{
	GxAVCodecClass* padec ;

	if (!codec)
		return 0;

	if((padec=GxGetDecoderClassFromObject(codec))!=NULL){
		return padec->decode(&codec->parent, NULL, NULL, buffer, maxlen);
	}

	return 0;
}

int GxAudioDecoder_Control(GxAudioCodec*  codec, int cmd, void *arg)
{
	GxAVCodecClass* padec ;

	if (!codec)
		return GX_PLAYER_ERROR;

	if (GX_AD_CTRL_SWITCH_HEADER == cmd) {
		GxStreamAudioHeader* header = arg;

		if (header->format == AUDIO_CODEC_AC3 || header->format == AUDIO_CODEC_EAC3) {
			int enable_ac3;

			GxPlayer_SystemGet(PSYS_ENABLE_AC3,  &enable_ac3);
			if (enable_ac3) {
				GxPin *ac3_out_pin  = GxMediaFilter_FindPin(codec, GX_AD_PIN_NAME_AC3_OUT);
				GxPin *eac3_out_pin = GxMediaFilter_FindPin(codec, GX_AD_PIN_NAME_EAC3_OUT);
				int ac3, eac3;

				GxPlayer_SystemGet(PSYS_BufSizeAC3,  &ac3);
				GxPlayer_SystemGet(PSYS_BufSizeEAC3, &eac3);
				if (!ac3_out_pin)
					GxPin_Create(GX_AD_PIN_NAME_AC3_OUT,  codec, GX_PINDIR_OUTPUT, ac3,  GX_PINFLAG_AC3);
				if (!eac3_out_pin)
					GxPin_Create(GX_AD_PIN_NAME_EAC3_OUT, codec, GX_PINDIR_OUTPUT, eac3, GX_PINFLAG_EAC3);
			}
		} else if (header->format == AUDIO_CODEC_DTS || header->format == AUDIO_CODEC_DTS_HD) {
			int enable_dts;

			GxPlayer_SystemGet(PSYS_ENABLE_DTS,  &enable_dts);
			if (enable_dts) {
				GxPin *dts_out_pin  = GxMediaFilter_FindPin(codec, GX_AD_PIN_NAME_DTS_OUT);
				int dts;

				GxPlayer_SystemGet(PSYS_BufSizeDTS, &dts);
				if (!dts_out_pin)
					GxPin_Create(GX_AD_PIN_NAME_DTS_OUT, codec, GX_PINDIR_OUTPUT, dts, GX_PINFLAG_DTS);
			}
		} else if (header->format == AUDIO_CODEC_AAC_LATM || header->format == AUDIO_CODEC_AAC_ADTS) {
			int enable_aac;

			GxPlayer_SystemGet(PSYS_ENABLE_AAC,  &enable_aac);
			if (enable_aac) {
				GxPin *aac_out_pin  = GxMediaFilter_FindPin(codec, GX_AD_PIN_NAME_AAC_OUT);
				GxPin *ac3_out_pin  = GxMediaFilter_FindPin(codec, GX_AD_PIN_NAME_AC3_OUT);
				int aac, ac3;

				GxPlayer_SystemGet(PSYS_BufSizeAAC, &aac);
				GxPlayer_SystemGet(PSYS_BufSizeAC3, &ac3);
				if (!aac_out_pin)
					GxPin_Create(GX_AD_PIN_NAME_AAC_OUT, codec, GX_PINDIR_OUTPUT, aac, GX_PINFLAG_AAC);
				if (!ac3_out_pin)
					GxPin_Create(GX_AD_PIN_NAME_AC3_OUT, codec, GX_PINDIR_OUTPUT, ac3, GX_PINFLAG_AC3);
			}
		}

		codec->parent.header = (void* )header;
		return GX_PLAYER_OK;
	}

	if((padec=GxGetDecoderClassFromObject(codec))!=NULL){
		return padec->control((GxAVCodec* )codec, cmd, arg);
	}

	return GX_PLAYER_ERROR;
}

void GxAudioDecoderClass_Init(void)
{
	GxClassRegister(&gx_AudioDecoderBase);
	GxClassInitTable((void* *)gx_audio_decoder_classes);
}

void GxAudioDecoderClass_Destroy(void)
{
	int i;
	for (i = 0; gx_audio_decoder_classes[i]; i++)
		GxClassUnregister(gx_audio_decoder_classes[i]);
	GxClassUnregister(&gx_AudioDecoderBase);
}

GxAVCodecClass gx_AudioDecoderBase = {
	._inherit = {
		._inherit = {
			.name    = "Decoder",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxAudioCodec),
			.create  = adbase_create,
			.release = adbase_release,
		},
		.run    = adbase_run,
		.pause  = adbase_pause,
		.resume = adbase_resume,
		.config = adbase_config,
		.stop   = adbase_stop,
	},
	DEF_AUTHOR("AudioDecoder","base","No description","L.F","No comment"),

	.name       = "GxAudioDecoder",
	.open       = NULL,
	.close      = NULL,
	.decode     = NULL,
	.control    = NULL,
};


