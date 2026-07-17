#include "gx_muxer.h"

#ifdef CONFIG_FFMPEG_ENABLE
extern GxMuxerClass gx_muxer_ffmpeg;
#endif

static GxMuxerClass *gx_muxer_class[] = {
#ifdef CONFIG_FFMPEG_ENABLE
	&gx_muxer_ffmpeg,
#endif
	NULL
};

static GxMuxerClass *muxer_get_class_from_type(int type)
{
	int i;

	for (i = 0; gx_muxer_class[i]; i++)
		if (type == gx_muxer_class[i]->type)
			return gx_muxer_class[i];

	return NULL;
}

void GxMuxerPacket_Destroy(GxMuxerPacket *pkt)
{
	if (pkt && pkt->data) {
		av_free(pkt->data);
		av_free(pkt);
	}
}

GxMuxerPacket *GxMuxerPacket_New(int size)
{
	GxMuxerPacket *pkt = av_mallocz(sizeof(GxMuxerPacket));

	if (!pkt)
		return NULL;

	pkt->data = av_malloc(size);
	if (!pkt->data) {
		av_free(pkt);
		return NULL;
	}

	pkt->size = size;
	pkt->stream_index = -1;

	return pkt;
}

static void *muxerbase_create(GxObject *obj)
{
	return obj;
}

static void muxerbase_release(GxObject *obj)
{
	return;
}

static int muxerbase_pause(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static int muxerbase_resume(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static int muxerbase_stop(GxMediaFilter *filter)
{
	GxMuxer *muxer = GXMUXER(filter);
	GxMuxerClass *cls = GetGxMuxerClassFromObject(muxer);

	if (!muxer->flushed) {
		GxCore_ThreadJoin(muxer->thread_mux);
		if (cls && cls->write_trailer)
			cls->write_trailer(muxer);

		muxer->flushed = 1;
	}

	return GX_PLAYER_OK;
}

static int muxerbase_config(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static void muxerbase_thread(void *arg)
{
	GxMuxer *muxer = arg;
	GxFifo *fifo = muxer->inpin->source->fifo;
	GxMuxerClass *cls = GetGxMuxerClassFromObject(muxer);
	GxMediaFilter *mf = GXMEDIAFILTER(muxer);

	while (mf->status == GX_MFT_STATE_RUNNING) {
		size_t size;
		GxMuxerPacket *pkt;

		size = GxFifo_Read(fifo, (uint8_t *)&pkt, sizeof(pkt), 20000);
		if (size == sizeof(pkt)) {
			if (muxer->have_error || muxer->flushed) {
				break;
			}

			cls->write_frame(muxer, pkt->stream_index, pkt->data, pkt->size, pkt->pts);
			GxMuxerPacket_Destroy(pkt);
		}
		else {
			GxCore_ThreadDelay(20);
		}
	}
}


static int muxerbase_run(GxMediaFilter *filter)
{
	GxMuxer *muxer = GXMUXER(filter);
	GxCore_ThreadCreate("MUXER_thread", &muxer->thread_mux, muxerbase_thread, muxer, 16*1024, GXOS_DEFAULT_PRIORITY);
	return GX_PLAYER_OK;
}

GxMuxer *GxMuxer_Open(GxMuxerOpenInfo *info)
{
	GxMuxer *muxer = NULL;
	GxMuxerClass *cls = NULL;

	if(info == NULL)
		return NULL;

	cls = muxer_get_class_from_type(GX_MUXER_TYPE_FFMPEG);

	if (cls) {
		muxer = GxObjectNew(NULL, cls);
		if (muxer && cls->open) {
			muxer->info = *info;
			if (cls->open(muxer) != NULL) {
				muxer->inpin  = GxPin_Create(GX_MUXER_PIN_NAME_INPUT,  (void *)muxer, GX_PINDIR_INPUT, 0, GX_PINFLAG_SW);
				muxer->outpin = GxPin_Create(GX_MUXER_PIN_NAME_OUTPUT, (void *)muxer,
						GX_PINDIR_OUTPUT, MUXER_OUT_FIFO_SIZE, GX_PINFLAG_SW);
				if (muxer->outpin != NULL) {
					return muxer;
				}
			}
		}
	}

	GxMuxer_Close(muxer);
	return NULL;
}

void GxMuxer_Close(GxMuxer *muxer)
{
	if(muxer)
	{
		GxMuxerClass *cls = GetGxMuxerClassFromObject(muxer);

		muxerbase_stop(GXMEDIAFILTER(muxer));

		if(cls && cls->close)
			cls->close(muxer);

		GxObjectDestroy(muxer);
	}
}

int GxMuxer_Control(GxMuxer *muxer, int cmd, void *args)
{
	if (muxer)
	{
		GxMuxerClass *cls = GetGxMuxerClassFromObject(muxer);

		if (cls && cls->control)
			return cls->control(muxer, cmd, args);
		else
			return GX_PLAYER_ERROR;
	}
	return GX_PLAYER_OK;
}

GxMuxerClass gx_MuxerBase = {
	._inherit = {
		._inherit = {
			.name    = "Muxer",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxMuxer),
			.create  = muxerbase_create,
			.release = muxerbase_release,
			.event   = NULL,
		},
		.run     = muxerbase_run,
		.pause   = muxerbase_pause,
		.resume  = muxerbase_resume,
		.stop    = muxerbase_stop,
		.config  = muxerbase_config,
	},
	DEF_AUTHOR("Muxer", "base", "No description", "L.F", "No comment"),
	.type       = GX_MUXER_TYPE_UNKNOWN
};


