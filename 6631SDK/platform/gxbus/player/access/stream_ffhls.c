#include "gx_stream.h"
#include "gx_media.h"
#include "gx_options.h"

typedef struct {
	GxStream parent;
} GxStreamFFHLS;

static int ffhls_open(GxStream *s, int mode)
{
	GxStreamFFHLS *stream = (GxStreamFFHLS*)s;

	stream->parent.file_format = GX_STREAMTYPE_DEMUXER;
	stream->parent.flags   = 0;
	stream->parent.end_pos = 0;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_LAVF;
	return GX_PLAYER_OK;
}

static void ffhls_close(GxStream *s)
{
	return;
}

static int ffhls_control(GxStream *s, int cmd, void *arg)
{
	return GX_PLAYER_OK;
}

static int ffhls_seek(GxStream *s, off_t newpos)
{
	return GX_PLAYER_OK;
}

static int ffhls_probe(GxStream *s, uint8_t *buffer, size_t size, void **priv)
{
	int iStatuFlg = GX_PLAYER_ERROR;

    if ((0 == strncmp((const char *)buffer, "#EXTM3U", 7)) ||
		strstr((const char *)buffer, "#EXT-X-STREAM-INF:")     ||
        strstr((const char *)buffer, "#EXT-X-TARGETDURATION:") ||
        strstr((const char *)buffer, "#EXT-X-MEDIA-SEQUENCE:")) {
			iStatuFlg = GX_PLAYER_OK;
    }

	return iStatuFlg;
}

GxStreamClass gx_stream_ffhls = {
	._inherit = {
		._inherit     = {
			.name    = "StreamFFHLS",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStreamFFHLS),
		},
	},
	.protocols = {"ffhls", NULL},
	DEF_AUTHOR("Stream","ffhls","No description","L.F","No comment"),

	.flags   = 0,
	.probe   = ffhls_probe,
	.open    = ffhls_open,
	.read    = NULL,
	.write   = NULL,
	.seek    = ffhls_seek,
	.control = ffhls_control,
	.close   = ffhls_close,
};
