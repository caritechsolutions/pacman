#include "gx_stream.h"
#include "gx_media.h"
#include "gx_options.h"
#include "avstring.h"

typedef struct {
	GxStream parent;
} GxStreamAssociationLibre;

static int associationlibre_probe(GxStream *s, uint8_t* buffer, size_t size, void **priv)
{
	GxStreamAssociationLibre *stream = (GxStreamAssociationLibre*)s;
	if (!stream || (!stream->parent.url))
		return GX_PLAYER_ERROR;
	if (!(av_strstart(stream->parent.url, "associationlibre:", NULL))) {
		return GX_PLAYER_ERROR;
	}
	return GX_PLAYER_OK;
}

static int associationlibre_open(GxStream *s, int mode)
{
	GxStreamAssociationLibre *stream = (GxStreamAssociationLibre*)s;
	if (!stream || (!stream->parent.url))
		return GX_PLAYER_ERROR;
	if (!(av_strstart(stream->parent.url, "associationlibre:", NULL))) {
		return GX_PLAYER_ERROR;
	}
	stream->parent.file_format = GX_STREAMTYPE_STREAM;
	stream->parent.flags   = 0;
	stream->parent.end_pos = 0;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_ASSOCIATIONLIBRE;
	return GX_PLAYER_OK;
}

static void associationlibre_close(GxStream *s)
{
	return;
}

static int associationlibre_control(GxStream *s, int cmd, void *arg)
{
	return GX_PLAYER_ERROR;
}

static int associationlibre_seek(GxStream *s, off_t newpos)
{
	return GX_PLAYER_OK;
}

GxStreamClass gx_stream_associationlibre = {
	._inherit = {
		._inherit     = {
			.name    = "StreamMpegAssociationlibre",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStreamAssociationLibre),
		},
	},
	.protocols = {"associationlibre", NULL},
	DEF_AUTHOR("Stream","associationlibre","No description","L.F","No comment"),

	.flags   = 0,
	.probe   = associationlibre_probe,
	.open    = associationlibre_open,
	.read    = NULL,
	.write   = NULL,
	.seek    = associationlibre_seek,
	.control = associationlibre_control,
	.close   = associationlibre_close,
};


