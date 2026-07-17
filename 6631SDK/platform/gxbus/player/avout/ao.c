#include "ao.h"
#include "gx_device.h"

#ifdef CONFIG_AO_HW
extern GxAudioOutClass gx_aout_hw;
#endif
#ifdef CONFIG_AO_SDL
extern GxAudioOutClass gx_aout_sdl;
#endif
#ifdef CONFIG_AO_OSS
extern GxAudioOutClass gx_aout_oss;
#endif
#ifdef CONFIG_AO_DUMMY
extern GxAudioOutClass gx_aout_dummy;
#endif

static GxAudioOutClass* gx_aout_classes[] = {
#ifdef CONFIG_AO_HW
	&gx_aout_hw,
#endif
#ifdef CONFIG_AO_SDL
	&gx_aout_sdl,
#endif
#ifdef CONFIG_AO_OSS
	&gx_aout_oss,
#endif
#ifdef CONFIG_AO_DUMMY
	&gx_aout_dummy,
#endif

	NULL
};

static void* aobase_create(GxObject * obj)
{
	return obj;
}

static void aobase_release(GxObject*  obj)
{
	return;
}

static int aobase_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int aobase_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int aobase_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int aobase_stop(GxMediaFilter*  filter)
{
	GxAudioOut* audio_out = GXAOUT(filter);

	GxCore_ThreadJoin(audio_out->run_pthread);

	return GX_PLAYER_OK;
}

static void aobase_run_thread(void* filter)
{
	GxAudioOut* audio_out = GXAOUT(filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin *in_pin;
	GxFifo* fifo = NULL;

	int playflags = 0;
	static int aout_need_size = 0;
	unsigned char* data = NULL;
	size_t len;

	in_pin = GxMediaFilter_FindPin(filter, GX_AO_PIN_NAME_PCM_IN);

	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {

		fifo = in_pin->source->fifo;
		len  = GxFifo_GetLength(fifo);

		if(len <= 0){
			if(in_pin->source->filter->status == GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				GxAudioOut_Close(audio_out,1);
				return  ;
			}
			GxCore_ThreadDelay(DELAY10MS);
			continue;
		}

		if (in_pin->source && in_pin->source->fifo && len >0) {
			aout_need_size = len;
			//if(aout_need_size == 0)
				//GxAudioOut_Control(audio_out, GX_AO_GET_SPACE, &aout_need_size);
			//	aout_need_size = 10240;

			//if (aout_need_size <=0)
			//	continue;
			//if (len >= aout_need_size) {
				if ((data = av_malloc(aout_need_size)) != NULL) {
					GxFifo_Read(in_pin->source->fifo, data, aout_need_size, -1);
					GxAudioOut_Play(audio_out, data, aout_need_size, playflags);
					av_free(data);
					aout_need_size =0;
				}
			//}
		}
		//GxCore_ThreadDelay(10);
	}
}

static int aobase_run(GxMediaFilter*  filter)
{
	GxAudioOut* d = GXAOUT(filter);
	if (GxCore_ThreadCreate("aobase_thread", &d->run_pthread, aobase_run_thread, filter, 10240, GXOS_DEFAULT_PRIORITY) == 0) {
		filter->status = GX_MFT_STATE_RUNNING;
		return 0;
	}

	return -1;
}

void GxAudioOut_Print(void)
{
	int i = 0;
	gxlogf("GxAudioOut Device Lists:\n");
	while (gx_aout_classes[i]) {
		gxlogf("\t%s", gx_aout_classes[i++]->name);
	}
	gxlogf("\n");
}

GxAudioOut* GxAudioOut_Open(GxStreamAudioHeader* header, GxStreamAudioHeader *header1)
{
	int i, enable_ac3, enable_dts, enable_aac;;
	GxAudioOutClass* audio_driver = NULL;
	GxAudioOut* audio_out = NULL;

	for (i = 0; gx_aout_classes[i]; i++) {
		audio_driver = gx_aout_classes[i];
		audio_out = GxObjectNew(NULL, audio_driver);
		if (audio_out == NULL)
			return NULL;
		audio_out->header  = header;
		audio_out->header1 = header1;
		if (audio_driver->open && audio_driver->open(audio_out) != GX_PLAYER_OK) {
			GxObjectDestroy(audio_out);
			audio_out = NULL;
		} else
			break;

	}

	GxPlayer_SystemGet(PSYS_ENABLE_AC3,  &enable_ac3);
	GxPlayer_SystemGet(PSYS_ENABLE_DTS,  &enable_dts);
	GxPlayer_SystemGet(PSYS_ENABLE_AAC,  &enable_aac);

	if (audio_out) {
		if(audio_out->header){
			GxPin_Create(GX_AO_PIN_NAME_PCM_IN,  audio_out,  GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
			if (header->format == AUDIO_CODEC_AC3 || header->format == AUDIO_CODEC_EAC3) {
				if (enable_ac3) {
					GxPin_Create(GX_AO_PIN_NAME_AC3_IN,  audio_out,  GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
					GxPin_Create(GX_AO_PIN_NAME_EAC3_IN, audio_out,  GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
				}
			} else if (header->format == AUDIO_CODEC_DTS || header->format == AUDIO_CODEC_DTS_HD) {
				if (enable_dts)
					GxPin_Create(GX_AO_PIN_NAME_DTS_IN,  audio_out,  GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
			} else if (header->format == AUDIO_CODEC_AAC_LATM || header->format == AUDIO_CODEC_AAC_ADTS) {
				if (enable_aac) {
					GxPin_Create(GX_AO_PIN_NAME_AC3_IN,  audio_out,  GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
					GxPin_Create(GX_AO_PIN_NAME_AAC_IN,  audio_out,  GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
				}
			}
		}

		if(audio_out->header1)
			GxPin_Create(GX_AO_PIN_NAME_ADPCM_IN, audio_out, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
		gxlogf("[Player]: AO = [%s]\n", audio_driver->name);
	}

	return audio_out;
}

void GxAudioOut_Close(GxAudioOut*  ao, int immed)
{
	if (ao) {
		GxAudioOutClass* ao_class = GetGxAudioOutClassFromObject(ao);

		if (ao_class && ao_class->close)
			ao_class->close(ao, immed);

		GxObjectDestroy(ao);
	}
}

int GxAudioOut_Control(GxAudioOut*  ao, int cmd, void *args )
{
	if (!ao)
		return GX_PLAYER_ERROR;

	GxAudioOutClass* ao_class = GetGxAudioOutClassFromObject(ao);

	if (cmd ==  GX_AO_CTRL_SWITCH_HEADER) {
		GxStreamAudioHeader* header = args;

		if (header->format == AUDIO_CODEC_AC3 || header->format == AUDIO_CODEC_EAC3) {
			int enable_ac3;

			GxPlayer_SystemGet(PSYS_ENABLE_AC3,  &enable_ac3);
			if (enable_ac3) {
				GxPin *ac3_in_pin  = GxMediaFilter_FindPin(ao, GX_AO_PIN_NAME_AC3_IN);
				GxPin *eac3_in_pin = GxMediaFilter_FindPin(ao, GX_AO_PIN_NAME_EAC3_IN);

				if (!ac3_in_pin)
					GxPin_Create(GX_AO_PIN_NAME_AC3_IN,  ao, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
				if (!eac3_in_pin)
					GxPin_Create(GX_AO_PIN_NAME_EAC3_IN, ao, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
			}
		} else if (header->format == AUDIO_CODEC_DTS || header->format == AUDIO_CODEC_DTS_HD) {
			int enable_dts;

			GxPlayer_SystemGet(PSYS_ENABLE_DTS,  &enable_dts);
			if (enable_dts) {
				GxPin *dts_in_pin = GxMediaFilter_FindPin(ao, GX_AO_PIN_NAME_DTS_IN);

				if (!dts_in_pin)
					GxPin_Create(GX_AO_PIN_NAME_DTS_IN, ao, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
			}
		} else if (header->format == AUDIO_CODEC_AAC_LATM || header->format == AUDIO_CODEC_AAC_ADTS) {
			int enable_aac;

			GxPlayer_SystemGet(PSYS_ENABLE_AAC,  &enable_aac);
			if (enable_aac) {
				GxPin *aac_in_pin = GxMediaFilter_FindPin(ao, GX_AO_PIN_NAME_AAC_IN);
				GxPin *ac3_in_pin = GxMediaFilter_FindPin(ao, GX_AO_PIN_NAME_AC3_IN);

				if (!aac_in_pin)
					GxPin_Create(GX_AO_PIN_NAME_AAC_IN, ao, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
				if (!ac3_in_pin)
					GxPin_Create(GX_AO_PIN_NAME_AC3_IN, ao, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
			}
		}
		ao->header = header;
		return GX_PLAYER_OK;
	}

	if (ao_class && ao_class->control) {
		return ao_class->control(ao, cmd, args);
	}

	return GX_PLAYER_ERROR;
}

int GxAudioOut_Play(GxAudioOut*  ao, void *data, int len, int flags)
{
	if (!ao)
		return -1;
	GxAudioOutClass* ao_class = GetGxAudioOutClassFromObject(ao);

	if (ao_class && ao_class->play)
		return ao_class->play(ao, data, len, flags);

	return GX_PLAYER_ERROR;
}

void GxAudioOutClass_Init(void)
{
	GxClassRegister(&gx_AudioOutBase);
	GxClassInitTable((void* *)gx_aout_classes);
}

void GxAudioOutClass_Destroy(void)
{
	int i;

	for (i = 0; gx_aout_classes[i]; i++)
		GxClassUnregister(gx_aout_classes[i]);
	GxClassUnregister(&gx_AudioOutBase);
}

GxAudioOutClass gx_AudioOutBase = {
	._inherit = {           // GxMediaFilter
		._inherit = {   // GxObject
			.name    = "Vout Base",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxAudioOut),
			.create  = aobase_create,
			.release = aobase_release,
		},
		.run    = aobase_run,
		.pause  = aobase_pause,
		.resume = aobase_resume,
		.config = aobase_config,
		.stop   = aobase_stop,
	},
	DEF_AUTHOR("AudioOut","base","No description","L.F","No comment"),

	.name    = "GxAudioOut",
	.open    = NULL,
	.close   = NULL,
	.control = NULL,
	.play    = NULL,
};

