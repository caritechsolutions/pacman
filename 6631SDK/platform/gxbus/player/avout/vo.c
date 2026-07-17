
#include "vo.h"

#ifdef CONFIG_VO_HW
extern GxVideoOutClass gx_vout_hw;
#endif
#ifdef CONFIG_VO_FBDEV
extern GxVideoOutClass gx_vout_fbdev;
#endif
#ifdef CONFIG_VO_DUMMY
extern GxVideoOutClass gx_vout_dummy;
#endif

static GxVideoOutClass* gx_vout_classes[] = {
#ifdef CONFIG_VO_HW
	&gx_vout_hw,
#endif
#ifdef CONFIG_VO_FBDEV
	&gx_vout_fbdev,
#endif
#ifdef CONFIG_VO_DUMMY
	&gx_vout_dummy,
#endif
	NULL
};

static void* vobase_create(GxObject * obj)
{
	return obj;
}

static void vobase_release(GxObject*  obj)
{
	return;
}

static void vobase_run_thread(void* filter)
{
	GxVideoOut* video_out = GXVOUT(filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin* in_pin;
	GxFifo* fifo = NULL;

	size_t len;
	unsigned char* data ;
	in_pin = GxMediaFilter_FindPin(filter, GX_VO_PIN_NAME_IN);

	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {

		fifo = in_pin->source->fifo;
		len  = GxFifo_GetLength(fifo);

		if(len <= 0){
			if(in_pin->source->filter->status== GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				GxVideoOut_Close(video_out);
				return  ;
			}
			GxCore_ThreadDelay(DELAY10MS);
			continue;
		}

		if (in_pin->source && in_pin->source->fifo && len) {
			if ((data = (unsigned char*)av_malloc(len)) != NULL) {
				GxFifo_Read(in_pin->source->fifo, data, len, -1);
				GxVideoOut_DrawFrame(video_out, data, len, 0);
				av_free(data);
			}
		}
		//GxCore_ThreadDelay(10);
	}
}

static int vobase_run(GxMediaFilter *filter)
{
	GxVideoOut* d = GXVOUT(filter);
	if (GxCore_ThreadCreate("vobase_thread", &d->run_pthread, vobase_run_thread, filter, 10240, GXOS_DEFAULT_PRIORITY) == 0) {
		filter->status = GX_MFT_STATE_RUNNING;
		return (int)d->run_pthread;
	}
	return -1;
}

static int vobase_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int vobase_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int vobase_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int vobase_stop(GxMediaFilter*  filter)
{
	GxVideoOut* d = GXVOUT(filter);

	GxCore_ThreadJoin(d->run_pthread);

	return GX_PLAYER_OK;
}

void GxVideoOut_Print(void)
{
	int i = 0;
	gxlogf("GxVideoOut Device Lists:\n");
	while (gx_vout_classes[i]) {
		gxlogf("\t%s\n", gx_vout_classes[i++]->name);
	}
}

GxVideoOut* GxVideoOut_Open(GxStreamVideoHeader * header,int index)
{
	int i;
	GxVideoOutClass* video_driver = NULL;
	GxVideoOut* video_out = NULL;

	for (i = 0; gx_vout_classes[i]; i++) {
		video_driver = gx_vout_classes[i];
		video_out = (GxVideoOut*)GxObjectNew(NULL, video_driver);
		if (video_out == NULL)
			return NULL;
		video_out->header = header;
		video_out->index  = index;
		if (video_driver->open && video_driver->open(video_out)!=GX_PLAYER_OK) {
			GxObjectDestroy(video_out);
			video_out = NULL;
		} else
			break;

	}

	if (video_out) {
		GxPin_Create(GX_VO_PIN_NAME_IN, video_out, GX_PINDIR_INPUT,0,GX_PINFLAG_NO_PTS_FIFO);
		gxlogf("[Player]: VO = [%s]\n", video_driver->name);
	}

	return video_out;
}

void GxVideoOut_Close(GxVideoOut*  vo)
{
	if (vo)
	{
		GxVideoOutClass* vo_class = GetGxVideoOutClassFromObject(vo);

		if (vo_class && vo_class->close)
			vo_class->close(vo);

		GxObjectDestroy(vo);
	}
}

int GxVideoOut_DrawFrame(GxVideoOut*  vo, uint8_t * frame, int len, int type)
{
	if (!vo)
		return GX_PLAYER_ERROR;

	GxVideoOutClass* vo_class = GetGxVideoOutClassFromObject(vo);

	if (vo_class && vo_class->draw_frame)
		return vo_class->draw_frame(vo, frame, len, type);
	else
		return GX_PLAYER_ERROR;
}

int GxVideoOut_DrawSub(GxVideoOut*  vo, uint8_t * data, int len, int type)
{
	if (!vo)
		return GX_PLAYER_ERROR;

	GxVideoOutClass* vo_class = GetGxVideoOutClassFromObject(vo);

	if (vo_class && vo_class->draw_sub)
		return vo_class->draw_sub(vo, data, len, type);
	else
		return GX_PLAYER_ERROR;
}

int GxVideoOut_Control(GxVideoOut*  vo, int cmd, void *args)
{
	if (!vo)
		return GX_PLAYER_ERROR;

	GxVideoOutClass* vo_class = GetGxVideoOutClassFromObject(vo);

	if (vo_class && vo_class->control) {
		return vo_class->control(vo, cmd, args);
	}

	return GX_PLAYER_ERROR;
}


void GxVideoOutClass_Init(void)
{
	GxClassRegister(&gx_VideoOutBase);
	GxClassInitTable((void* *)gx_vout_classes);
}

void GxVideoOutClass_Destroy(void)
{
	int i;
	for (i = 0; gx_vout_classes[i]; i++)
		GxClassUnregister(gx_vout_classes[i]);
	GxClassUnregister(&gx_VideoOutBase);
}

GxVideoOutClass gx_VideoOutBase = {
	._inherit = {
		._inherit = {
			.name    = "Vout Base",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxVideoOut),
			.create  = vobase_create,
			.release = vobase_release,
		},
		.run    = vobase_run,
		.pause  = vobase_pause,
		.resume = vobase_resume,
		.config = vobase_config,
		.stop   = vobase_stop,
	},
	DEF_AUTHOR("VideoOut","base","No description","L.F","No comment"),

	.name       = "GxVideoOut",
	.open       = NULL,
	.close      = NULL,
	.draw_frame = NULL,
	.draw_sub   = NULL,
	.control    = NULL,
};

