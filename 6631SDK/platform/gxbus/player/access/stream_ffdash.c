#include "gx_stream.h"
#include "gx_media.h"
#include "gx_options.h"
#include "avstring.h"

typedef struct {
	GxStream parent;
} GxStreamFFDash;

static int ffdash_open(GxStream *s, int mode)
{
	GxStreamFFDash *stream = (GxStreamFFDash*)s;

	stream->parent.file_format = GX_STREAMTYPE_DEMUXER;
	stream->parent.flags   = 0;
	stream->parent.end_pos = 0;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_LAVF;

	return GX_PLAYER_OK;
}

static void ffdash_close(GxStream *s)
{
	return;
}

static int ffdash_control(GxStream *s, int cmd, void *arg)
{
	return GX_PLAYER_OK;
}

static int ffdash_seek(GxStream *s, off_t newpos)
{
	return GX_PLAYER_OK;
}

static int ffdash_probe(GxStream *s, uint8_t *buffer, size_t size, void **priv)
{
	if (!av_stristr((char *)buffer, "<MPD"))
		return GX_PLAYER_ERROR;

	if (av_stristr((char *)buffer, "dash:profile:isoff-on-demand:2011") ||
		av_stristr((char *)buffer, "dash:profile:isoff-live:2011") ||
		av_stristr((char *)buffer, "dash:profile:isoff-live:2012") ||
		av_stristr((char *)buffer, "dash:profile:isoff-main:2011") ||
		av_stristr((char *)buffer, "3GPP:PSS:profile:DASH1")) {
			return GX_PLAYER_OK;
	}
	if (av_stristr((char *)buffer, "dash:profile")) {
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

GxStreamClass gx_stream_ffmpegdash = {
	._inherit = {
		._inherit     = {
			.name    = "StreamFFDash",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStreamFFDash),
		},
	},
	.protocols = {"mpegdash", NULL},
	DEF_AUTHOR("Stream","ffdash","No description","L.F","No comment"),

	.flags   = 0,
	.probe   = ffdash_probe,
	.open    = ffdash_open,
	.read    = NULL,
	.write   = NULL,
	.seek    = ffdash_seek,
	.control = ffdash_control,
	.close   = ffdash_close,
};
