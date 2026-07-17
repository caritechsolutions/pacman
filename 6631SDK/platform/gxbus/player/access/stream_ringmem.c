#include "gx_stream.h"
#include "recfs.h"

typedef struct record_file* pfile;

typedef struct _PrivTable {
	int hdr_writed;
}PrivTable;

static int ringmem_open(GxStream *s, int mode)
{
	pfile rec = recfile_open(s->url, mode);
	if (rec == NULL)
		return GX_PLAYER_ERROR;

	s->fd = (int)rec;
	s->file_format = GX_STREAMTYPE_RINGMEM;
	s->priv = av_mallocz(sizeof(PrivTable));

	return GX_PLAYER_OK;
}

static int ringmem_read(GxStream *s, uint8_t *buffer, size_t len)
{
	int r = recfile_read((pfile)s->fd, buffer, len);

	return (r < 0) ? -1 : r;
}

static int ringmem_write(GxStream *s, uint8_t *buffer, size_t len)
{
	int w = 0;

	w = recfile_write((pfile)s->fd, buffer, len);
	return (w < 0) ? -1 : w;
}

static int ringmem_seek(GxStream *s, off_t newpos)
{
	if (recfile_seek((pfile)s->fd, s->pos, SEEK_SET) < 0)
		return GX_PLAYER_ERROR;

	return GX_PLAYER_OK;
}

static int ringmem_control(GxStream *s, int cmd, void *arg)
{
	switch (cmd) {
		case GX_STREAM_CTRL_GET_SIZE:
			{
				off_t* size = (off_t*)arg;
				*size = recfile_getsize((pfile)s->fd);
				return GX_PLAYER_OK;
			}

		case GX_STREAM_CTRL_GET_CAP_SIZE:
			gxlogf("[Player]: recfile not support get cap size\n");
			return GX_PLAYER_OK;

		case GX_STREAM_CTRL_WRITE_HDR:
			return GX_PLAYER_OK;

		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

static void ringmem_close(GxStream *s)
{
	if(s){
		if(s->priv) {
			av_free(s->priv);
			s->priv = NULL;
		}

		if(s->fd) {
			recfile_close((pfile)s->fd);
			s->fd  = -1;
		}
	}
}

static int ringmem_stop(GxMediaFilter *filter){
	GxStream*  s = GXSTREAM(filter);

	if(s){
		if(s->priv) {
			av_free(s->priv);
			s->priv = NULL;
		}

		if(s->fd >= 0) {
			recfile_close((pfile)s->fd);
			s->fd  = -1;
		}
	}

	return GX_PLAYER_OK;
}

GxStreamClass gx_stream_ringmem = {
	._inherit = {
		._inherit = {
			.name    = "StreamRingMem",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStream),
		},
		.run     = NULL,
		.stop    = ringmem_stop,
		.pause   = NULL,
		.resume  = NULL,
		.config  = NULL,
	},
	.protocols = {"ringmem", "", NULL},
	DEF_AUTHOR("Stream","mem","No description","shenbin","No comment"),

	.flags   = 0,
	.open    = ringmem_open,
	.read    = ringmem_read,
	.write   = ringmem_write,
	.seek    = ringmem_seek,
	.control = ringmem_control,
	.close   = ringmem_close,
};
