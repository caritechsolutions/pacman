#include "gx_stream.h"
#include "gx_mediafilter.h"

// /1/2/3/4/5/6/7

#define FIFO_CAP_SIZE ~(1LL<<63)
#define FIFO_TIMEOUT  (10)


static int fifo_open(GxStream *s, int mode)
{
	//GxFifo *f = GxFifo_Create(FIFO_SIZE, GX_PINFLAG_SW);

	int32_t fd;
	int32_t size;

	GxUrl_GetItem(s->url, "fd", &fd);
	GxUrl_GetItem(s->url, "size", &size);

	gxlogd("[access] fifo fd = 0x%x, size = 0x%x\n", fd, size);
	s->fd = (int)fd;
	s->file_format = GX_STREAMTYPE_FIFO;

	return GX_PLAYER_OK;
}

static int fifo_read(GxStream *s, uint8_t *buffer, size_t len)
{
	int r = GxFifo_Read((GxFifo *)s->fd, buffer, len, FIFO_TIMEOUT);

	return (r <= 0) ? -1 : r;
}

static int fifo_write(GxStream *s, uint8_t *buffer, size_t len)
{
	int w = GxFifo_Write((GxFifo *)s->fd, buffer, len, FIFO_TIMEOUT);

	return (w <= 0) ? -1 : w;
}

static int fifo_seek(GxStream *s, off_t newpos)
{
	return GX_PLAYER_OK;
}

static int fifo_control(GxStream *s, int cmd, void *arg)
{
	switch (cmd) {
		case GX_STREAM_CTRL_GET_SIZE:
		{
			off_t* size = (off_t*)arg;
			*size = GxFifo_GetLength((GxFifo *)s->fd);
			return GX_PLAYER_OK;
		}

		case GX_STREAM_CTRL_GET_CAP_SIZE:
		{
			int64_t *size = (int64_t*)arg;

			*size = FIFO_CAP_SIZE;
			if (*size < 0)
				return GX_PLAYER_ERROR;

			return GX_PLAYER_OK;
		}

		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

/*
 * in open fd has openned by caller. Means "GxFifo_Create" do by otherone.
 * so in close & stop, needn't call "GxFifo_Destroy", it will call by otherone.
 */
static void fifo_close(GxStream *s)
{
	if(s && s->fd >= 0){
//		GxFifo_Destroy((GxFifo *)s->fd);
		s->fd  = -1;
	}
}

static int fifo_stop(GxMediaFilter *filter){
	GxStream*  s = GXSTREAM(filter);

	if(s && s->fd >= 0){
//		GxFifo_Destroy((GxFifo *)s->fd);
		s->fd  = -1;
	}
	return GX_PLAYER_OK;
}

GxStreamClass gx_stream_fifo = {
	._inherit = {
		._inherit = {
			.name   = "StreamFifo",
			.parent = &gx_streambase,
			.size   = sizeof(GxStream),
		},
		.run     = NULL,
		.stop    = fifo_stop,
		.pause   = NULL,
		.resume  = NULL,
		.config  = NULL,
	},
	.protocols = {"fifo", NULL},
	DEF_AUTHOR("Stream","fifo","No description","S.B","No comment"),

	.flags   = 0,
	.open    = fifo_open,
	.read    = fifo_read,
	.write   = fifo_write,
	.seek    = fifo_seek,
	.control = fifo_control,
	.close   = fifo_close,
};
