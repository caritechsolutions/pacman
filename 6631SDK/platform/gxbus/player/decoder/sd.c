#include "sd.h"

#define GX_SUB_DECODER_CLASS_MAX (16)
static int sub_decoder_class_num = 0;
static GxAVCodecClass* gx_sub_decoder_classes[GX_SUB_DECODER_CLASS_MAX];
void GxSubtitleDecoderRegister(GxAVCodecClass* cls)
{
	int i;
	if (sub_decoder_class_num == 0)
		memset(&gx_sub_decoder_classes, 0, sizeof(gx_sub_decoder_classes));

	if (sub_decoder_class_num >= GX_SUB_DECODER_CLASS_MAX)
		return;

	for (i=0; i<sub_decoder_class_num; i++) {
		if (gx_sub_decoder_classes[i] == cls)
			return;
	}
	gx_sub_decoder_classes[sub_decoder_class_num++] = cls;
}

static GxAVCodecClass* sub_get_class_from_type(int type)
{
	int i = 0, j = 0;
	for (i = 0; gx_sub_decoder_classes[i] != NULL; i++) {
		for (j = 0; j < GX_DECODER_SUPPORT_MAX; j++) {
			if(gx_sub_decoder_classes[i]->format[j] == -1)
				break;
			if (gx_sub_decoder_classes[i]->format[j] == type)
			{
				return gx_sub_decoder_classes[i];
				break;
			}
		}
	}
	return NULL;
}

GxFifo* sdfifo = NULL;

static void* sdbase_create(GxObject * obj)
{
	return obj;
}

static void sdbase_release(GxObject*  obj)
{
	return;
}

static void sdbase_run_thread(void* filter)
{
	GxFifo* fifo = NULL;
	size_t len;
	GxPin* in_pin, *out_pin;
	unsigned char* data;
	GxMediaFilter* mf = GXMEDIAFILTER(filter);

	if(filter == NULL)
		return  ;

	in_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_IN);
	out_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_OUT);

	gxlogf("[Player]%s[%d]:%s RUN\n",__FILE__,__LINE__,__FUNCTION__);
	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {
		if(mf->status != GX_MFT_STATE_RUNNING){
			GxCore_ThreadDelay(100);
			continue;
		}

		fifo  = in_pin->source->fifo;
		len = GxFifo_GetLength(fifo);

		if(len <= 0){
			if(in_pin->source->filter->status == GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				return ;
			}
			GxCore_ThreadDelay(100);
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
					GxCore_ThreadDelay(20);
				}
				av_free(data);
			}
			else
				gxlogf("[Player]%s[%d]:%s MALLOC ERROR\n",__FILE__,__LINE__,__FUNCTION__);
		}
		GxCore_ThreadDelay(100);
	}
}

static int sdbase_run(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);

	if(sd){
		if (GxCore_ThreadCreate("sdbase_run_thread", &sd->run_pthread,
				sdbase_run_thread, filter, 1024*8, GXOS_DEFAULT_PRIORITY) == 0){
			filter->status = GX_MFT_STATE_RUNNING;
			gxlogf("[Player]%s[%d]:%s SUCCESS\n",__FILE__,__LINE__,__FUNCTION__);
			return GX_PLAYER_OK;
		}
	}
	gxlogf("[Player]%s[%d]:%s FAILED\n",__FILE__,__LINE__,__FUNCTION__);
	return GX_PLAYER_ERROR;
}

static int sdbase_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sdbase_stop(GxMediaFilter*  filter)
{
	GxAVCodec* sd = GXDECODEROBJECT(filter);
	GxPin* in_pin;

	if(sd){
		GxCore_ThreadJoin(sd->run_pthread);
		in_pin = GxMediaFilter_FindPin(filter, GX_SD_PIN_NAME_IN);
		if(in_pin && in_pin->source && in_pin->source->fifo)
			GxFifo_Reset(in_pin->source->fifo);
	}
	gxlogf("[Player]%s[%d]:%s SUCCESS\n",__FILE__,__LINE__,__FUNCTION__);
	return GX_PLAYER_OK;
}

static int sdbase_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sdbase_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

GxSubCodec* GxSubDecoder_Open(GxStreamSubHeader* header)
{
	int i, j;
	GxAVCodecClass* pdec = NULL;
	GxSubCodec* sub_codec = NULL;

	if(header == NULL)
		return NULL;

	gxlogf("[Player]:%s[%d]:%s SUB_FMT = '%c'\n",__FILE__,__LINE__,__FUNCTION__,header->type);

	if (!(pdec = sub_get_class_from_type((int)header->type))) {
		gxlogf ("[Player]:%s[%d]:%s NOT FIND CODEC\n",__FILE__,__LINE__,__FUNCTION__);
		pdec = gx_sub_decoder_classes[0];//没有相应的解码字幕时，默认text
		//return NULL;
	}

	sub_codec = GxObjectNew(NULL, pdec);
	if (sub_codec != NULL) {
		sub_codec->parent.header = (void* )header;
		if(pdec->open){
			if(pdec->open(GXDECODEROBJECT(sub_codec))){
				GxObjectDestroy(sub_codec);
				return NULL ;
			}
		}
		gxlogf("[Player]:%s[%d]:%s SD = [%s]\n",__FILE__,__LINE__,__FUNCTION__,pdec->name);
		GxPin_Create(GX_SD_PIN_NAME_IN, sub_codec, GX_PINDIR_INPUT, 0, GX_PINFLAG_SW);
		GxPin_Create(GX_SD_PIN_NAME_OUT, sub_codec, GX_PINDIR_OUTPUT, 32*1024, GX_PINFLAG_SW);
		return (sub_codec);
	}
	return (sub_codec);
}

void GxSubDecoder_Close(GxSubCodec*  codec)
{
	GxAVCodecClass* pdec;

	if (codec )
	{
		if((pdec=GxGetDecoderClassFromObject(codec))!=NULL)
			pdec->close(GXDECODEROBJECT(codec));
		GxObjectDestroy(codec);
	}
}

int GxSubDecoder_Decode(GxSubCodec*  codec, unsigned char *buffer, int minlen, int maxlen)
{
	GxAVCodecClass* pdec ;

	if (!codec)
		return 0;

	if((pdec=GxGetDecoderClassFromObject(codec))!=NULL){
		return pdec->decode(&codec->parent, NULL, NULL, buffer, maxlen);
	}

	return 0;
}

int GxSubDecoder_Control(GxSubCodec*  codec, int cmd, void *arg)
{
	GxAVCodecClass* pdec ;

	if (!codec)
		return GX_PLAYER_ERROR;

	if((pdec=GxGetDecoderClassFromObject(codec))!=NULL){
		return pdec->control((GxAVCodec* )codec, cmd, arg);
	}

	return GX_PLAYER_ERROR;
}

void GxSubDecoderClass_Init(void)
{
	GxClassRegister(&gx_SubDecoderBase);
	GxClassInitTable((void* *)gx_sub_decoder_classes);
}

void GxSubDecoderClass_Destroy(void)
{
	int i;
	for (i = 0; gx_sub_decoder_classes[i]; i++)
		GxClassUnregister(gx_sub_decoder_classes[i]);
	GxClassUnregister(&gx_SubDecoderBase);
}

GxAVCodecClass gx_SubDecoderBase = {
	._inherit = {
		._inherit = {
			.name = "Decoder",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxSubCodec),
			.create  = sdbase_create,
			.release = sdbase_release,
		},
		.run    = sdbase_run,
		.pause  = sdbase_pause,
		.resume = sdbase_resume,
		.config = sdbase_config,
		.stop   = sdbase_stop,
	},
	DEF_AUTHOR("SubDecoder","base","No description","L.F","No comment"),

	.name       = "GxSubDecoder",
	.open       = NULL,
	.close      = NULL,
	.decode     = NULL,
	.control    = NULL,
};


