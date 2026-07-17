/*
 *  This file is part of MPlayer.
 *
 *  MPlayer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  MPlayer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with MPlayer; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "gx_demux.h"
#include "gx_common.h"
#include "stheader.h"

#define READ_UNIT (16*1024)

typedef struct
{
	char buf[READ_UNIT];
}DemuxESPriv;

static void demux_es_close(GxDemuxer* demuxer)
{
	DemuxESPriv* priv = (DemuxESPriv *) demuxer->priv;

	if(!priv)
		return;

	av_free(demuxer->priv);

	return;
}

static int demux_es_check_file(GxDemuxer* demuxer)
{
	DemuxESPriv* priv;

	priv = av_calloc(1, sizeof(DemuxESPriv));

	if(!priv)
		return GX_DEMUXER_TYPE_UNKNOWN;

	demuxer->priv = priv;

	return GX_DEMUXER_TYPE_ES;
}

int probe_video_codec(GxDemuxer *demuxer)
{
#define JPEG_SYNC_CODE  0xFFD8FF
#define VIDEO_SYNC_CODE 0x000001
#define MPEG2_SPS_CODE  0xB3
#define AVS_SPS_CODE    0xB0
#define H264_SPS_CODE   0x07
#define H265_VPS_CODE   0x40
	int fmt = 0;
	unsigned int  word = 0x0;
	unsigned char bytes[4] = {0xff,0xff,0xff,0xff};
	unsigned int  cnt = 0;

	while(!GxStream_Eof(demuxer->stream) && fmt == 0) {
		bytes[0] = bytes[1];
		bytes[1] = bytes[2];
		bytes[2] = bytes[3];
		bytes[3] = GxStream_ReadChar(demuxer->stream);
		word = (bytes[0]<<24) | (bytes[1]<<16) | (bytes[2]<<8) | bytes[3];
		cnt++;

		if (((word>>8) == JPEG_SYNC_CODE) && (cnt == 4)) {
			fmt = VIDEO_CODEC_UNKNOWN;
			break;
		} else if ((word>>8) == VIDEO_SYNC_CODE) {
			if ((bytes[3] & 0xff) == MPEG2_SPS_CODE) {
				fmt = VIDEO_CODEC_MPEG12;
				break;
			} else if ((bytes[3] & 0xff) == AVS_SPS_CODE) {
				fmt = VIDEO_CODEC_AVS;
				break;
			} else if ((bytes[3] & 0x7e) == H265_VPS_CODE) {
				fmt = VIDEO_CODEC_H265;
				break;
			} else if ((bytes[3] & 0x1f) == H264_SPS_CODE) {
				fmt = VIDEO_CODEC_H264;
				break;
			}
		}
	}

	return fmt;
}

static GxDemuxer* demux_es_open(GxDemuxer* demuxer)
{
	GxStreamAudioHeader* ash = NULL;
	GxStreamVideoHeader* vsh = NULL;

	if(strstr(demuxer->stream->url, "esv") || strstr(demuxer->stream->url, "bin")) {
		vsh = GxStreamHeader_VideoNew(demuxer,0, 0);
		vsh->ds = demuxer->video;
		demuxer->video->sh = vsh;
		demuxer->video->id = 0;
		vsh->format = probe_video_codec(demuxer);
		if (vsh->format == VIDEO_CODEC_UNKNOWN) {
			av_free(demuxer->priv);
			demuxer->priv = NULL;
			return NULL;
		}
	} else if(strstr(demuxer->stream->url,"esa")) {
		ash = GxStreamHeader_AudioNew(demuxer,0, 0);
		ash->ds = demuxer->audio;
		demuxer->audio->sh = ash;
		demuxer->audio->id = 0;

		if(strstr(demuxer->stream->url,".mp3"))
			ash->format = AUDIO_CODEC_MPEG1;
		else if(strstr(demuxer->stream->url,".latm"))
			ash->format = AUDIO_CODEC_AAC_LATM;
		else if(strstr(demuxer->stream->url,".adts"))
			ash->format = AUDIO_CODEC_AAC_ADTS;
		else if(strstr(demuxer->stream->url,".dts"))
			ash->format = AUDIO_CODEC_DTS;
		else if(strstr(demuxer->stream->url,".aac"))
			ash->format = AUDIO_CODEC_AAC_LATM;
		else if(strstr(demuxer->stream->url,".ac3"))
			ash->format = AUDIO_CODEC_AC3;
		else if(strstr(demuxer->stream->url,".eac3"))
			ash->format = AUDIO_CODEC_EAC3;
		else if(strstr(demuxer->stream->url,".sbc"))
			ash->format = AUDIO_CODEC_SBC;
		else if(strstr(demuxer->stream->url,".dra"))
			ash->format = AUDIO_CODEC_DRA1;
		else {
			av_free(demuxer->priv);
			demuxer->priv = NULL;
			return NULL;
		}
	} else if (demuxer->stream->file_format == GX_STREAMTYPE_MEM) {
		int aformat = -1, vformat = -1;

		if (!demuxer->stream->prog_max) {
			vformat = probe_video_codec(demuxer);
			aformat = -1;
		} else {
			vformat = vcodec_fourcc2std(demuxer->stream->prog[demuxer->stream->prog_now].vcodec);
			aformat = acodec_fourcc2std(demuxer->stream->prog[demuxer->stream->prog_now].acodec);
		}

		if (vformat != -1) {
			vsh = GxStreamHeader_VideoNew(demuxer, 0, 0);
			vsh->ds = demuxer->video;
			vsh->format = vformat;
			demuxer->video->sh = vsh;
			demuxer->video->id = 0;
		}

		if (aformat != -1) {
			ash = GxStreamHeader_AudioNew(demuxer, 0, 0);
			ash->ds = demuxer->audio;
			ash->format = aformat;
			demuxer->audio->sh = ash;
			demuxer->audio->id = 0;
		}

		if ((vsh == NULL) && (ash == NULL)) {
			av_free(demuxer->priv);
			demuxer->priv = NULL;
			return NULL;
		}
	}

	GxStream_Reset(demuxer->stream);
	GxStream_Seek (demuxer->stream, 0);
	demuxer->filepos = GxStream_Tell(demuxer->stream);
	demuxer->stream->eof = 0;

	return demuxer;
}

static int demux_es_fill_buffer(GxDemuxer* demuxer, GxDemuxStream *ds)
{
	int len;
	GxDemuxPacket* dp;
	DemuxESPriv* priv = demuxer->priv;

	if(demuxer->stream->eof || (demuxer->movi_end && GxStream_Tell(demuxer->stream) >= demuxer->movi_end))
		return GX_PLAYER_ERROR;

	len = GxStream_Read(demuxer->stream,(uint8_t *)priv->buf, READ_UNIT);
	if(len > 0) {
		// TODO liufei
		dp = GxDemuxPacket_Create(demuxer, NULL, len);
		if(! dp)
		{
			gxlogf( "fill_buffer, NEW_ADD_PACKET(%d)FAILED\n", len);
			return GX_PLAYER_ERROR;
		}

		GxDemuxPacket_Write(dp,(uint8_t *)priv->buf, len);

		if(demuxer->audio->sh)
			GxDemuxStream_AddPacket(demuxer->audio, dp);
		else
			GxDemuxStream_AddPacket(demuxer->video, dp);

		return GX_PLAYER_OK;
	}

	demuxer->audio->eof = 1;
	demuxer->stream->eof = 1;

	return GX_PLAYER_ERROR;
}


static int demux_es_seek(GxDemuxer* demuxer, int64_t  rel_seek_ms, int audio_delay, int flags)
{
	return GX_PLAYER_OK;
}

static int demux_es_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	switch (cmd)
	{
		case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
			return GX_DEMUXER_CTRL_OK;
		default:
			return GX_DEMUXER_CTRL_ERROR;
	}
}

GxDemuxerClass gx_demux_es = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer ES",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("demuxer","es","No description","zhaogf","No comment"),

	.name        = "Demuxer es",
	.type        = GX_DEMUXER_TYPE_ES,
	.check_file  = demux_es_check_file,
	.open        = demux_es_open,
	.close       = demux_es_close,
	.fill_buffer = demux_es_fill_buffer,
	.seek        = demux_es_seek,
	.control     = demux_es_control,
};
