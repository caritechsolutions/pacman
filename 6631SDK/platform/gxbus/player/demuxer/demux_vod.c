#include "gx_demux.h"
#include "stheader.h"
#include "../avformat/avformat.h"
#include "../avformat/riff.h"

static int demux_vod_check_file(GxDemuxer*  demuxer)
{
	return GX_DEMUXER_TYPE_VOD;
}

static GxDemuxer* demux_vod_open(GxDemuxer * demuxer)
{
	return NULL;
}

static int demux_vod_fill_buffer(GxDemuxer* demuxer, GxDemuxStream* dsds)
{
	return GX_PLAYER_OK;
}

static int demux_vod_seek(GxDemuxer*  demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	return GX_PLAYER_OK;
}

static int demux_vod_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	return GX_DEMUXER_CTRL_OK;
}

static void demux_vod_close(GxDemuxer*  demuxer)
{
}

static int demuxer_vod_run(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int demuxer_vod_stop(GxMediaFilter* filter)
{
	return GX_PLAYER_OK;
}

static int demuxer_vod_pause(GxMediaFilter* filter)
{
	return GX_PLAYER_OK;
}

static int demuxer_vod_resume(GxMediaFilter* filter)
{
	return GX_PLAYER_OK;
}

GxDemuxerClass gx_demux_vod = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Vod demuxer",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = demuxer_vod_run,
		.pause  = demuxer_vod_pause,
		.resume = demuxer_vod_resume,
		.stop   = demuxer_vod_stop,
	},
	DEF_AUTHOR("demuxer","mp4","No description","L.F","No comment"),

	.name        = "Demuxer MP4",
	.type        = GX_DEMUXER_TYPE_VOD,
	.check_file  = demux_vod_check_file,
	.open        = demux_vod_open,
	.close       = demux_vod_close,
	.fill_buffer = demux_vod_fill_buffer,
	.seek        = demux_vod_seek,
	.control     = demux_vod_control,
};

