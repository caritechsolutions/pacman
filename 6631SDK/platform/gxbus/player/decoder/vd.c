
#include <math.h>
#include "vd.h"
#include "gx_avutil.h"
#include "gx_image.h"
#include "vd_es2frame.h"

#ifdef CONFIG_VD_HW
extern GxAVCodecClass gx_decoder_vd_hw;
#endif
#ifdef CONFIG_VD_MPEG
extern GxAVCodecClass gx_decoder_mpeg_es;
#endif
#ifdef CONFIG_VD_DUMMY
extern GxAVCodecClass gx_decoder_vd_dummy;
#endif

static GxAVCodecClass* gx_video_decoder_classes[] = {
#ifdef CONFIG_VD_HW
	&gx_decoder_vd_hw,
#endif
#ifdef CONFIG_VD_MPEG
	&gx_decoder_mpeg_es,
#endif
#ifdef CONFIG_VD_DUMMY
	&gx_decoder_vd_dummy,
#endif

	NULL
};

static void* vdbase_create(GxObject * obj)
{
	return obj;
}

static void vdbase_release(GxObject*  obj)
{
	return;
}

static void vdbase_run_thread(void*  filter)
{
	GxFifo* fifo = NULL;
	size_t len;
	GxPin* in_pin, *out_pin;
	unsigned char* data;
	GxMediaFilter* mf = GXMEDIAFILTER(filter);

	if(filter == NULL)
		return  ;

	in_pin = GxMediaFilter_FindPin(filter, GX_VD_PIN_NAME_IN);
	out_pin = GxMediaFilter_FindPin(filter, GX_VD_PIN_NAME_OUT);

	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {
		fifo  = in_pin->source->fifo;
		len = GxFifo_GetLength(fifo);

		if (len <= 0) {
			if(in_pin->source->filter->status== GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				return  ;
			}
			GxCore_ThreadDelay(DELAY10MS);
			continue;
		}

		if (out_pin && out_pin->fifo) {
			data = av_malloc(len);
			if(data){
				unsigned int write_size = 0;
				GxFifo_Read(fifo,data ,len, -1);

				write_size =GxFifo_Write(out_pin->fifo, data, len, -1);

				while (write_size < len){
					write_size += GxFifo_Write(out_pin->fifo, data + write_size, len - write_size, -1);
					GxCore_ThreadDelay(DELAY10MS);
				}
				av_free(data);
			}
		}

		//GxCore_ThreadDelay(10);
	}
}

static int vdbase_run(GxMediaFilter*  filter)
{
	GxAVCodec* d = GXDECODEROBJECT(filter);
	if (GxCore_ThreadCreate("vdbase_run_thread", &d->run_pthread, vdbase_run_thread, filter,10240,GXOS_DEFAULT_PRIORITY) == 0) {
		filter->status = GX_MFT_STATE_RUNNING;
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int vdbase_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int vdbase_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int vdbase_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int vdbase_stop(GxMediaFilter*  filter)
{
	GxAVCodec* d = GXDECODEROBJECT(filter);
	GxCore_ThreadJoin(d->run_pthread);

	return GX_PLAYER_OK;
}

GxVideoCodec* GxVideoDecoder_Open(GxStreamVideoHeader * header)
{
	int i,j;
	GxAVCodecClass* pvdec = NULL;
	GxVideoCodec * video_codec = NULL;

	if(header == NULL)
		return NULL;

	gxlogf("[Player]: V_FMT = 0x%x\n",header->format);

	for (i = 0; gx_video_decoder_classes[i] != NULL; i++) {
		for (j = 0; j < GX_DECODER_SUPPORT_MAX; j++) {
			if (gx_video_decoder_classes[i]->format[j] == -1)
				break;
			else if (gx_video_decoder_classes[i]->format[j] == header->format ||
					gx_video_decoder_classes[i]->format[j] == PLAYER_VCODEC_DUMMY) {
				pvdec = gx_video_decoder_classes[i];
				break;
			}
		}

		if (pvdec == NULL)
			continue;

		video_codec = GxObjectNew(NULL, pvdec);
		if (video_codec != NULL) {
			video_codec->parent.header = (void* )header;
			if(pvdec->open){
				if(pvdec->open(GXDECODEROBJECT(video_codec))){
					GxObjectDestroy(video_codec);
					return NULL ;
				}
			}
			gxlogf("[Player]: VD = [%s]\n", pvdec->name);
			GxPin_Create(GX_VD_PIN_NAME_IN, video_codec, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);

#ifdef CONFIG_VD_DUMMY
			GxPin_Create(GX_VD_PIN_NAME_OUT, video_codec, GX_PINDIR_OUTPUT, 512 * 1024, 0);
#endif
			return (video_codec);
		}
	}

	return (video_codec);
}

void GxVideoDecoder_Close(GxVideoCodec*  codec)
{
	GxAVCodecClass* pvdec;

	if (codec) {
		if((pvdec=GxGetDecoderClassFromObject(codec))!=NULL)
			pvdec->close(GXDECODEROBJECT(codec));

		GxObjectDestroy(codec);
	}
}

int GxVideoDecoder_Control(GxVideoCodec*  codec, int cmd, void *arg)
{
	GxAVCodecClass* pvdec;

	if (codec == NULL)
		return GX_PLAYER_ERROR;
	if((pvdec=GxGetDecoderClassFromObject(codec))!=NULL)
		return (pvdec->control((GxAVCodec* ) codec, cmd, arg));

	return GX_PLAYER_ERROR;
}

void* GxVideoDecoder_Decode(GxVideoCodec * codec, unsigned char *start, int in_size, int drop_frame, double pts)
{
	int data_size = 0;
	void* out_data = NULL;
	GxAVCodecClass* pvdec;

	if (codec == NULL)
		return NULL;

	if((pvdec=GxGetDecoderClassFromObject(codec))!=NULL){
		if (!pvdec->decode(GXDECODEROBJECT(codec), &out_data, &data_size, start, in_size))
			return out_data;
	}
	return NULL;
}

void GxVideoDecoderClass_Init(void)
{
	GxClassRegister(&gx_VideoDecoderBase);
	GxClassInitTable((void* *)gx_video_decoder_classes);
}

void GxVideoDecoderClass_Destroy(void)
{
	int i;
	for (i = 0; gx_video_decoder_classes[i]; i++)
		GxClassUnregister(gx_video_decoder_classes[i]);
	GxClassUnregister(&gx_VideoDecoderBase);
}

GxAVCodecClass gx_VideoDecoderBase = {
	._inherit = {
		._inherit = {
			.name     = "Decoder",
			.parent   = &gx_mediafilter_base,
			.size     = sizeof(GxVideoCodec),
			.create   = vdbase_create,
			.release  = vdbase_release,
		},
		.run    = vdbase_run,
		.pause  = vdbase_pause,
		.resume = vdbase_resume,
		.config = vdbase_config,
		.stop   = vdbase_stop,
	},
	DEF_AUTHOR("VideoDecoder","base","No description","L.F","No comment"),

	.name       = "GxVideoDecoder",
	.open       = NULL,
	.close      = NULL,
	.decode     = NULL,
	.control    = NULL,
};


