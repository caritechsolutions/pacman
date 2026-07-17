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
  int bps;
  int dummy;
} DemuxDRAPriv;

static void demux_dra_close(GxDemuxer* demuxer)
{
	DemuxDRAPriv* priv = (DemuxDRAPriv *) demuxer->priv;

	if(!priv)
		return;

	av_free(demuxer->priv);

	return;
}

static int demux_dra_check_file(GxDemuxer* demuxer)
{
    return GX_DEMUXER_TYPE_DRA;
}

static GxDemuxer* demux_dra_open(GxDemuxer * demuxer)
{
	GxStreamAudioHeader* sh_audio = NULL;
	demuxer->priv = av_malloc(sizeof(DemuxDRAPriv));

    if(!demuxer->priv)
        return NULL;

    ((DemuxDRAPriv*)(demuxer->priv))->bps = 128;

	demuxer->audio->sh = GxStreamHeader_AudioNew(demuxer, 0, 0);
	sh_audio = demuxer->audio->sh;
	sh_audio->ds = demuxer->audio;
    sh_audio->format = AUDIO_CODEC_DRA;

	return demuxer;
}

static int demux_dra_fill_buffer(GxDemuxer* demuxer, GxDemuxStream *ds)
{
    int packet_size = 128;
    unsigned char buf[128];
	DemuxDRAPriv* priv = (DemuxDRAPriv *) demuxer->priv;
	GxDemuxPacket* dp;

	if(demuxer->stream->eof)
        	return GX_PLAYER_ERROR;

	if(!demuxer->stream->eof)
	{
		dp = GxDemuxPacket_Create(demuxer, NULL, packet_size);
	    if(!dp)
	    {
	    	gxlogf( "fill_buffer, NEW_ADD_PACKET(%d)FAILED\n", packet_size);
	    	return GX_PLAYER_ERROR;
	    }

        GxStream_Read(demuxer->stream, buf, packet_size);
        GxDemuxPacket_Write(dp, buf, packet_size);
	    GxDemuxStream_AddPacket(demuxer->audio, dp);

        dp->pts = demuxer->filepos/(priv->bps/8);
	    demuxer->filepos = GxStream_Tell(demuxer->stream);
    }

    return GX_PLAYER_OK;
}

static void demux_dra_seek(GxDemuxer* demuxer, int64_t  rel_seek_ms, int audio_delay, int flags)
{
	DemuxDRAPriv* priv = (DemuxDRAPriv *) demuxer->priv;
	GxDemuxStream* d_audio=demuxer->audio;
	uint64_t pos;

	GxDemuxStream_FreePacks(d_audio);

    pos = rel_seek_ms*priv->bps/8;

    GxStream_Seek(demuxer->stream, pos);
}

static int demux_dra_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	DemuxDRAPriv* priv = (DemuxDRAPriv *) demuxer->priv;

    switch (cmd)
	{
		case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
			*((int64_t* )arg) = demuxer->stream->end_pos*8/priv->bps;
			return GX_DEMUXER_CTRL_OK;
		default:
			return GX_DEMUXER_CTRL_ERROR;
	}
    return GX_DEMUXER_CTRL_ERROR;
}

GxDemuxerClass gx_demux_dra = {
	._inherit = {		    // GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer DRA",
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
	DEF_AUTHOR("demuxer","dra","No description","L.F","No comment"),

	.name        = "Demuxer DRA",
	.type        = GX_DEMUXER_TYPE_DRA,
	.check_file  = demux_dra_check_file,
	.open        = demux_dra_open,
	.close       = demux_dra_close,
	.fill_buffer = demux_dra_fill_buffer,
	.seek        = demux_dra_seek,
	.control     = demux_dra_control,
};

