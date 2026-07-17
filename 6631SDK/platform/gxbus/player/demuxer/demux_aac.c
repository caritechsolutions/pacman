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

typedef struct {
	uint8_t* buf;
	uint64_t size;	/// amount of time of data packets pushed to demuxer->audio (in bytes)
	float time;	/// amount of time elapsed based upon samples_per_frame/sample_rate (in milliseconds)
	float last_pts; /// last pts seen
	int bitrate;	/// bitrate computed as size/time
	int frame_num;
	int duration;
} DemuxAACPriv;

/// \param srate (out) sample rate
/// \param num (out) number of audio frames in this ADTS frame
/// \return size of the ADTS frame in bytes
/// aac_parse_frames needs a buffer at least 8 bytes long

static int aac_samprate = -1;
static int aac_channels = -1;

int aac_parse_frame(uint8_t* buf, int *srate, int *num)
{
	int i = 0, sr, fl = 0, id;
	static int srates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 0, 0, 0};

	if((buf[i] != 0xFF) || ((buf[i+1] & 0xF6) != 0xF0))
		return 0;

	id = (buf[i+1] >> 3) & 0x01;	//id=1 mpeg2, 0: mpeg4
	sr = (buf[i+2] >> 2)  & 0x0F;
	if(sr > 11)
		return 0;
	*srate = srates[sr];

	if(aac_samprate == -1)
		aac_samprate =* srate;
	if(aac_channels == -1)
		aac_channels = ((buf[i+2] & 0x01)<<2)|((buf[i+3] & 0xc0)>>6);

	fl = ((buf[i+3] & 0x03) << 11) | (buf[i+4] << 3) | ((buf[i+5] >> 5) & 0x07);
	*num = (buf[i+6] & 0x02) + 1;

	return fl;
}

static int demux_aac_init(GxDemuxer* demuxer)
{
	DemuxAACPriv* priv;

	priv = av_calloc(1, sizeof(DemuxAACPriv));
	if(!priv)
		return 0;

	priv->buf = (uint8_t*) av_malloc(8);
	if(!priv->buf)
	{
		av_free(priv);
		return 0;
	}

	demuxer->priv = priv;
	return 1;
}

static void demux_aac_close(GxDemuxer* demuxer)
{
	DemuxAACPriv* priv = (DemuxAACPriv *) demuxer->priv;

	if(!priv)
		return;

	if(priv->buf)
		av_free(priv->buf);

	av_free(demuxer->priv);

	return;
}

static int demux_aac_check_file(GxDemuxer* demuxer)
{
	int cnt = 0, c, len, srate, num;
	off_t init, probed;
	DemuxAACPriv* priv;

	aac_samprate = -1;
	aac_channels = -1;

	if(! demux_aac_init(demuxer))
	{
		gxlogf( "COULDN'T INIT aac_demux, exit\n");
		return GX_DEMUXER_TYPE_UNKNOWN;
	}

	priv = (DemuxAACPriv* ) demuxer->priv;

	init = probed = GxStream_Tell(demuxer->stream);
	while(probed-init <= 32768 && cnt < 8)
	{
		c = 0;
		while(c != 0xFF)
		{
			c = GxStream_ReadChar(demuxer->stream);
			if ((c < 0) || (demuxer->stream->eof))
				goto fail;
		}
		priv->buf[0] = 0xFF;
		if(GxStream_Read(demuxer->stream, &(priv->buf[1]), 7) < 7)
			goto fail;

		len = aac_parse_frame(priv->buf, &srate, &num);
		if(len > 0)
		{
			cnt++;
			GxStream_Skip(demuxer->stream, len - 8);
		}
		probed = GxStream_Tell(demuxer->stream);
	}

	while(!GxStream_Eof(demuxer->stream) &&
			(demuxer->stream->file_format != GX_STREAMTYPE_STREAM))
	{
		if(GxStream_Read(demuxer->stream, &(priv->buf[0]), 8) < 8)
			break;
		if((priv->buf[0] != 0xFF) || ((priv->buf[1] & 0xF6) != 0xF0))
			break;

		len  = ((priv->buf[3] & 0x03) << 11) | (priv->buf[4] << 3) | ((priv->buf[5] >> 5) & 0x07);
		if(len > 0)
		{
			cnt++;
			GxStream_Skip(demuxer->stream, len - 8);
		}
		else
			break;
	}

	if(demuxer->stream->file_format == GX_STREAMTYPE_STREAM)
		priv->frame_num  = 0;
	else
		priv->frame_num  = cnt;

	GxStream_Seek(demuxer->stream, init);
	if(cnt < 8)
		goto fail;

	return GX_DEMUXER_TYPE_AAC;

fail:
	gxlogd("demux_aac_probe, failed to detect an AAC stream\n");
	return GX_DEMUXER_TYPE_UNKNOWN;
}

static GxDemuxer* demux_aac_open(GxDemuxer* demuxer)
{
	GxStreamAudioHeader* sh;
	DemuxAACPriv* priv = demuxer->priv;

	sh = GxStreamHeader_AudioNew(demuxer,0, 0);
	sh->ds = demuxer->audio;
	sh->format = GX_FOURCC('M', 'P', '4', 'A');
	sh->format = acodec_fourcc2std(sh->format);
	demuxer->audio->id = 0;
	demuxer->audio->sh = sh;
	sh->wf = av_malloc(sizeof(WAVEFORMATEX));
	sh->wf->nChannels = aac_channels;
	sh->wf->nSamplesPerSec = aac_samprate;
	sh->wf->nAvgBytesPerSec = 0;
	sh->wf->wBitsPerSample = 16;

	sh->samplerate = aac_samprate;
	if(aac_samprate)
		priv->duration = priv->frame_num*1024/(float)(aac_samprate/1000);
	else
		priv->duration = 0;

	demuxer->filepos = GxStream_Tell(demuxer->stream);

	demuxer->stream->eof = 0;

	return demuxer;
}

static int demux_aac_fill_buffer(GxDemuxer* demuxer, GxDemuxStream *ds)
{
	DemuxAACPriv* priv = (DemuxAACPriv *) demuxer->priv;
	GxDemuxPacket* dp;
	int c1, c2, len, srate, num;
	float tm = 0;

	if(demuxer->stream->eof ||
			(demuxer->stream->file_format != GX_STREAMTYPE_STREAM &&
			 demuxer->movi_end && GxStream_Tell(demuxer->stream) >= demuxer->movi_end))
		return GX_PLAYER_ERROR;

	while(! demuxer->stream->eof)
	{
		c1 = c2 = 0;
		while(c1 != 0xFF)
		{
			c1 = GxStream_ReadChar(demuxer->stream);
			if ((c1 < 0) || (demuxer->stream->eof))
				return GX_PLAYER_ERROR;
		}
		c2 = GxStream_ReadChar(demuxer->stream);
		if(c2 < 0)
			return GX_PLAYER_ERROR;
		if((c2 & 0xF6) != 0xF0)
			continue;

		priv->buf[0] = (unsigned char) c1;
		priv->buf[1] = (unsigned char) c2;
		if(GxStream_Read(demuxer->stream, &(priv->buf[2]), 6) < 6)
			return GX_PLAYER_ERROR;

		len = aac_parse_frame(priv->buf, &srate, &num);
#define LESS_LEN (8)
		if(len >= LESS_LEN) {
			//TODO liufei
			dp = GxDemuxPacket_Create(demuxer, NULL, len);
			if(! dp)
			{
				gxlogf( "fill_buffer, NEW_ADD_PACKET(%d)FAILED\n", len);
				return GX_PLAYER_ERROR;
			}

			memcpy(dp->buffer, priv->buf, LESS_LEN);
			if (len > LESS_LEN)
				GxStream_Read(demuxer->stream, &(dp->buffer[LESS_LEN]), len - LESS_LEN);
			if(srate)
				tm = (float) (num*  1024.0/srate);
			priv->last_pts += tm;
			dp->pts = priv->last_pts*1000;
			//gxlogd ( "\nPTS: %.3f\n", dp->pts);
			GxDemuxStream_AddPacket(demuxer->audio, dp);
			priv->size += len;
			priv->time += tm;

			priv->bitrate = (int) (priv->size / priv->time);
			demuxer->filepos = GxStream_Tell(demuxer->stream);

			return GX_PLAYER_OK;
		}
		else
			GxStream_Skip(demuxer->stream, -6);
	}

	return GX_PLAYER_ERROR;
}


//This is an almost verbatim copy of high_res_mp3_seek(), from demux_audio.c
static int demux_aac_seek(GxDemuxer* demuxer, int64_t  rel_seek_ms, int audio_delay, int flags)
{
	int ret = GX_PLAYER_OK;
	DemuxAACPriv* priv = (DemuxAACPriv *) demuxer->priv;
	GxDemuxStream* d_audio=demuxer->audio;
	GxStreamAudioHeader* sh_audio=d_audio->sh;
	float time;
	int  rel_seek_secs = rel_seek_ms/1000;

	GxDemuxStream_FreePacks(d_audio);

	time = (flags & GX_DEMUXER_SEEK_ABSOLUTE) ? rel_seek_secs - priv->last_pts : rel_seek_secs;
	if(time < 0)
	{
		GxStream_Seek(demuxer->stream, demuxer->movi_start);
		time = priv->last_pts + time;
		priv->last_pts = 0;
	}

	if(time > 0)
	{
		int len, nf, srate, num;

		nf = time*  sh_audio->samplerate/1024;

		while(nf > 0)
		{
			if(GxStream_Read(demuxer->stream,priv->buf, 8) < 8)
				break;
			len = aac_parse_frame(priv->buf, &srate, &num);
			if(len <= 0)
			{
				GxStream_Skip(demuxer->stream, -7);
				continue;
			}
			GxStream_Skip(demuxer->stream, len - 8);
			priv->last_pts += (float) (num*1024.0/srate);
			nf -= num;
		}
	}

	return ret;
}

static int demux_aac_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	DemuxAACPriv* priv = demuxer->priv;

	switch (cmd)
	{
		case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
			*((uint64_t* )arg) = priv->duration;
			return GX_DEMUXER_CTRL_OK;
		default:
			return GX_DEMUXER_CTRL_ERROR;
	}
}

GxDemuxerClass gx_demux_aac = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer AAC",
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
	DEF_AUTHOR("demuxer","aac","No description","L.F","No comment"),

	.name        = "Demuxer AAC",
	.type        = GX_DEMUXER_TYPE_AAC,
	.check_file  = demux_aac_check_file,
	.open        = demux_aac_open,
	.close       = demux_aac_close,
	.fill_buffer = demux_aac_fill_buffer,
	.seek        = demux_aac_seek,
	.control     = demux_aac_control,
};

