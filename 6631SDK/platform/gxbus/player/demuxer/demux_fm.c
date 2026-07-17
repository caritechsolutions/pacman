#include "gx_demux.h"
#include "stheader.h"

#define FM_SAMPLE_MS       (64)

static int demux_fm_check_file(GxDemuxer* demuxer)
{
	return GX_DEMUXER_TYPE_FM;
}

static GxDemuxer* demux_fm_open(GxDemuxer* demuxer)
{
	GxStreamAudioHeader *sh_audio=NULL;
	GxStreamDataInfo data_info;

	GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_DATA_INFO, (void *)(&data_info));
	sh_audio = GxStreamHeader_AudioNew(demuxer, 0, 0xff);
	sh_audio->channels   = data_info.channels;
	sh_audio->big_endian = data_info.endian;
	sh_audio->samplerate = data_info.samplerate;
	sh_audio->samplesize = data_info.bitwidth;
	sh_audio->format     = AUDIO_CODEC_PCM;

	demuxer->video->id = 0;
	demuxer->video->sh = NULL;
	demuxer->audio->id = 0;
	demuxer->audio->sh = sh_audio;

	return demuxer;
}

static void demux_fm_close(GxDemuxer* demuxer)
{
	return;
}

static int demux_fm_fill_buffer(GxDemuxer* demuxer,GxDemuxStream* dsds)
{
	GxDemuxStream* ds = demuxer->audio;
	GxDemuxPacket* dp = NULL;
	GxStreamAudioHeader *sh_audio = demuxer->audio->sh;
	unsigned int samples = (FM_SAMPLE_MS * (sh_audio->samplerate / 1000));
	unsigned int sample_size = samples * (sh_audio->samplesize / 8) * sh_audio->channels;
	unsigned int read_size = 0;

	dp = GxDemuxPacket_Create(demuxer, NULL, sample_size);
	if (dp == NULL){
		demuxer->audio->eof = 1;
		return 0;
	}

	read_size = GxStream_Read(demuxer->stream, dp->buffer, sample_size);
	if (read_size < sample_size) {
		GxDemuxPacket_Destroy(dp);
		return GX_PLAYER_OK;
	}

	GxDemuxStream_AddPacket(ds, dp);
	return GX_PLAYER_OK;
}

static int demux_fm_seek(GxDemuxer* demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	return 0;
}

static int demux_fm_control(GxDemuxer* demuxer, int cmd, void* arg)
{
	return GX_PLAYER_ERROR;
}

GxDemuxerClass gx_demux_fm = {
	._inherit = {
		._inherit = {
			.name    = "Demuxer FM",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.config = NULL,
		.stop   = NULL,
	},
	.name        = "Demuxer FM",
	.type        = GX_DEMUXER_TYPE_FM,
	.check_file  = demux_fm_check_file,
	.open        = demux_fm_open,
	.close       = demux_fm_close,
	.fill_buffer = demux_fm_fill_buffer,
	.seek        = demux_fm_seek,
	.control     = demux_fm_control,
};

