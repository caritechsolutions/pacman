#include "gx_common.h"
#include "gx_demux.h"
#include "gx_stream.h"
#include "gx_system.h"

static int vod_stream_read(GxStream*  stream, uint8_t * buffer, size_t max_len)
{
	return GX_PLAYER_OK;
}

static int vod_stream_seek(GxStream*  stream, off_t newpos)
{
	return GX_PLAYER_OK;
}

static int vod_stream_pause(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static int vod_stream_resume(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static int vod_stream_open(GxStream* stream, int mode)
{
	gxlogd("vod stream url %s\n", stream->url);
	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &stream->interrupt_cbk);
	stream->file_format = GX_STREAMTYPE_VODSYSTEM;
	stream->demuxer_type = GX_DEMUXER_TYPE_VOD;
	return GX_PLAYER_OK;
}

static void vod_stream_close(GxStream*  stream)
{
	stream->interrupt_cbk = NULL;
}

static int vod_stream_control(GxStream *s, int cmd, void *arg)
{
	return GX_PLAYER_ERROR;
}

GxStreamClass gx_stream_vod = {
	._inherit = {
		._inherit = {
			.name = "StreamVod",
			.parent = &gx_streambase,
			.size = sizeof(GxStream),
		},
		.pause  = vod_stream_pause,
		.resume = vod_stream_resume,
	},
	.protocols = {"vod", NULL},
	DEF_AUTHOR("Stream","vod","No description","L.F","No comment"),

	.flags   = 0,
	.open    = vod_stream_open,
	.read    = vod_stream_read,
	.write   = NULL,
	.seek    = vod_stream_seek,
	.control = vod_stream_control,
	.close   = vod_stream_close,
};
