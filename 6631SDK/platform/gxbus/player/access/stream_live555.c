/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gx_url.h"
#include "gx_stream.h"
#include"gx_demux.h"
#include"gx_common.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

typedef struct {
	GxStream parent;
	GxStreamingCtrl *streaming_ctrl;
}GxStreamLive555;

extern int network_bandwidth;

static int _rtsp_streaming_seek(int fd, off_t pos, GxStreamingCtrl* streaming_ctrl)
{
	return -1; // For now, we don't handle RTSP stream seeking
}

static int rtsp_streaming_start(GxStream* stream)
{
	stream->streaming_ctrl->streaming_seek = _rtsp_streaming_seek;
	return 0;
}

static int open_live_rtsp_sip(GxStream *s,int mode)
{
	GxStreamLive555* stream = (GxStreamLive555 *) (s);
	GxURL *url;

	stream->streaming_ctrl = streaming_ctrl_new();
	s->streaming_ctrl = stream->streaming_ctrl;

	if( stream->streaming_ctrl==NULL ) {
		return GX_PLAYER_ERROR;
	}
	stream->streaming_ctrl->bandwidth = network_bandwidth;
	url = url_new(stream->parent.url);
	stream->streaming_ctrl->url = check4proxies(url);

	gxlogf("STREAM_LIVE555, URL: %s\n", stream->parent.url);

	if(rtsp_streaming_start(s) < 0) {
		gxlogf("rtsp_streaming_start failed\n");
		goto fail;
	}

	stream->parent.file_format = GX_STREAMTYPE_STREAM;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_RTP;

	return GX_PLAYER_OK;

fail:
	streaming_ctrl_free( stream->streaming_ctrl );
	stream->streaming_ctrl = NULL;
	return GX_PLAYER_ERROR;
}

static int open_live_sdp(GxStream *stream,int mode)
{
	int f;
	char *filename = stream->url;
	off_t len;
	char* sdpDescription;
	ssize_t numBytesRead;

	if(strncmp("sdp://",filename,6) == 0) {
		filename += 6;
		f = open(filename,O_RDONLY|O_BINARY);
		if(f < 0) {
			gxlogf("open_live_sdp failed\n");
			return GX_PLAYER_ERROR;
		}

		len=lseek(f,0,SEEK_END);
		lseek(f,0,SEEK_SET);
		if(len == -1)
			return GX_PLAYER_ERROR;
		if(len > SIZE_MAX - 1)
			return GX_PLAYER_ERROR;

		sdpDescription = av_malloc(len+1);
		if(sdpDescription == NULL)
			return GX_PLAYER_ERROR;
		numBytesRead = read(f, sdpDescription, len);
		if(numBytesRead != len) {
			av_free(sdpDescription);
			return GX_PLAYER_ERROR;
		}
		sdpDescription[len] = '\0'; // to be safe
		stream->priv = sdpDescription;
		stream->file_format = GX_STREAMTYPE_SDP;
		stream->demuxer_type = GX_DEMUXER_TYPE_RTP;

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

GxStreamClass gx_stream_rtsp_sip = {
	._inherit = {
		._inherit = {
			.name   = "StreamRtspSip",
			.parent = &gx_streambase,
			.size   = sizeof(GxStream),
		},
	},
	.protocols = {"rtsp", "sip", NULL},
	DEF_AUTHOR("Stream","RTSP","No description","L.F","No comment"),

	.open    = open_live_rtsp_sip,
	.read    = NULL,
	.write   = NULL,
	.seek    = NULL,
	.control = NULL,
	.close   = NULL,
};

GxStreamClass gx_stream_sdp = {
	._inherit = {
		._inherit = {
			.name   = "StreamSdp",
			.parent = &gx_streambase,
			.size   = sizeof(GxStreamLive555),
		},
	},
	.protocols = {"sdp", NULL},
	DEF_AUTHOR("Stream","SDP","No description","L.F","No comment"),

	.flags   = 0,
	.open    = open_live_sdp,
	.read    = NULL,
	.write   = NULL,
	.seek    = NULL,
	.control = NULL,
	.close   = NULL,
};

