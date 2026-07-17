#include "so.h"

#ifdef CONFIG_SUBTITLE_MEDIA
extern GxSubOutClass gx_sout_text;
extern GxSubOutClass gx_sout_image;
#endif

static GxSubOutClass* gx_sout_classes[] = {
#ifdef CONFIG_SUBTITLE_MEDIA
	&gx_sout_text,
	&gx_sout_image,
#endif
	NULL
};

static void* sobase_create(GxObject * obj)
{
	return obj;
}

static void sobase_release(GxObject*  obj)
{
	return;
}

static int sobase_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sobase_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sobase_config(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int sobase_stop(GxMediaFilter*  filter)
{
	GxSubOut* sub_out = GXSOUT(filter);

	GxCore_ThreadJoin(sub_out->run_pthread);

	return GX_PLAYER_OK;
}

static void sobase_run_thread(void* filter)
{
	GxSubOut* sub_out = GXSOUT(filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin *in_pin;
	GxFifo* fifo = NULL;
	GxSub sf;
	unsigned char* buf = NULL;
	size_t len;

	in_pin = GxMediaFilter_FindPin(filter, GX_SO_PIN_NAME_IN);

	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && in_pin->source && in_pin->source->fifo ) {
		if(mf->status != GX_MFT_STATE_RUNNING)
		{
			GxCore_ThreadDelay(100);
			continue;
		}

		fifo = in_pin->source->fifo;
		len  = GxFifo_GetLength(fifo);
		if(len <= 0){
			if(in_pin->source->filter->status == GX_MFT_STATE_STOPPED){
				GxMediaFilter_Stop(filter);
				GxSubOut_Close(sub_out);
				return  ;
			}
			GxCore_ThreadDelay(100);
			continue;
		}

		GxFifo_Read(in_pin->source->fifo, (unsigned char*)(&sf), sizeof(GxSub), -1);
		if(sf.size && (sf.magic==GX_SUB_MAGIC))
		{
			buf = av_malloc(sf.size);
			if(buf == NULL)
			{
				gxlogf("[Player]%s[%d]:%s MALLOC FAILED!",__FILE__,__LINE__,__FUNCTION__);
				continue;
			}
			GxFifo_Read(in_pin->source->fifo, buf, sf.size, -1);
			av_free(buf);
		}
		else
			gxlogf("[Player]%s[%d]:%s ERROR! magic=0x%x sf.size=0x%x\n",__FILE__,__LINE__,__FUNCTION__,sf.magic,sf.size);
		GxCore_ThreadDelay(100);
	}
}

static int sobase_run(GxMediaFilter*  filter)
{
	GxSubOut* d = GXSOUT(filter);
	if (GxCore_ThreadCreate("sobase_thread", &d->run_pthread, sobase_run_thread, filter, 8*1024, GXOS_DEFAULT_PRIORITY) == 0) {
		filter->status = GX_MFT_STATE_RUNNING;
		return 0;
	}

	return -1;
}

static int sobase_open(GxSubOut*  so)
{
	return GX_PLAYER_OK;
}

static int sobase_close(GxSubOut*  so)
{
	return GX_PLAYER_OK;
}

static int sobase_control(GxSubOut*  so, int cmd, void *args)
{
	return GX_PLAYER_OK;
}

static int sobase_play(GxSubOut*  so, void *data, int len, int flags)
{
	return GX_PLAYER_OK;
}

void GxSubOut_Print(void)
{
	int i = 0;
	gxlogf("GxSubOut Device Lists:\n");
	while (gx_sout_classes[i]) {
		gxlogf("\t%s", gx_sout_classes[i++]->name);
	}
	gxlogf("\n");
}

GxSubOut* GxSubOut_Open(GxStreamSubHeader * header, uint64_t (*cb)(void* args), void* args)
{
	int i;
	GxSubOutClass* sub_driver = NULL;
	GxSubOut* sub_out = NULL;

	for (i = 0; gx_sout_classes[i]; i++) {
		sub_driver = gx_sout_classes[i];
		sub_out = (GxSubOut*)GxObjectNew(NULL, sub_driver);
		if (sub_out == NULL)
			return NULL;
		sub_out->header = header;
		sub_out->type = header->type;
		if (sub_driver->open && sub_driver->open(sub_out)!=GX_PLAYER_OK) {
			GxObjectDestroy(sub_out);
			sub_out = NULL;
		} else
			break;
	}

	if(sub_out == NULL)
	{
		sub_driver = &gx_SubOutBase;
		sub_out = (GxSubOut*)GxObjectNew(NULL,sub_driver);
		if(sub_out == NULL)
			return NULL;
		sub_out->header = header;
		sub_out->type = header->type;
		if(sub_driver && sub_driver->open)
			sub_driver->open(sub_out);
	}

	if (sub_out) {
		sub_out->cb = cb;
		sub_out->args = args;
		GxPin_Create(GX_SO_PIN_NAME_IN, sub_out, GX_PINDIR_INPUT,0,0);
		gxlogf("[Player]: SO = [%s]\n", sub_driver->name);
	}

	return sub_out;
}

void GxSubOut_Close(GxSubOut*  so)
{
	if (so) {
		GxSubOutClass* so_class = GetGxSubOutClassFromObject(so);

		if (so_class && so_class->close)
			so_class->close(so);
		so->header = NULL;
		GxObjectDestroy(so);
	}
}

int GxSubOut_Control(GxSubOut*  so, int cmd, void *args )
{
	if (!so)
		return GX_PLAYER_ERROR;

	GxSubOutClass* so_class = GetGxSubOutClassFromObject(so);

	if (so_class && so_class->control) {
		return so_class->control(so, cmd, args);
	}

	return GX_PLAYER_ERROR;
}

int GxSubOut_Play(GxSubOut*  so, void *data, int len, int flags)
{
	if (!so)
		return -1;
	GxSubOutClass* so_class = GetGxSubOutClassFromObject(so);

	if (so_class && so_class->play)
		return so_class->play(so, data, len, flags);

	return GX_PLAYER_ERROR;
}

void GxSubOutClass_Init(void)
{
	GxClassRegister(&gx_SubOutBase);
	GxClassInitTable((void* *)gx_sout_classes);
}

void GxSubOutClass_Destroy(void)
{
	int i;

	for (i = 0; gx_sout_classes[i]; i++)
		GxClassUnregister(gx_sout_classes[i]);
	GxClassUnregister(&gx_SubOutBase);
}

GxSubOutClass gx_SubOutBase = {
	._inherit = {           // GxMediaFilter
		._inherit = {   // GxObject
			.name    = "Sout Base",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxSubOut),
			.create  = sobase_create,
			.release = sobase_release,
		},
		.run    = sobase_run,
		.pause  = sobase_pause,
		.resume = sobase_resume,
		.config = sobase_config,
		.stop   = sobase_stop,
	},
	DEF_AUTHOR("SubOut","base","No description","L.F","No comment"),

	.name    = "GxSubOut",
	.open    = sobase_open,
	.close   = sobase_close,
	.control = sobase_control,
	.play    = sobase_play,
};

