/*
*   Copyright (C) 2006 Benjamin Zores
*    Stream layer for MPEG over RTP, based on previous work from Dave Chapman
*
*    This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software Foundation,
*   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "gx_url.h"
#include "udp.h"
#include "rtp.h"
#include "gx_stream.h"
#include "gx_demux.h"
#include "gx_common.h"
#include "stream_m3u8.h"

typedef struct gx_stream_rtp {
	GxStream parent;
	GxStreamingCtrl* streaming_ctrl;
} GxStreamRtp;

static int rtp_streaming_start(GxStream*  s)
{
	int stream_buffer_size = 0;
	GxStreamRtp* stream = (GxStreamRtp *) (s);
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;
	UDPContext* udp = NULL;

	udp = Gx_UDP_Open(streaming_ctrl->url);
	if(udp == NULL)
		return GX_PLAYER_ERROR;
	init_rtp_cache();

	//预留一部份数据在buffer,以便于数据探测
	{
		char buffer[MAX_RTP_PACKET_SIZE];

		while(1){
			if(s->interrupt_cbk && s->interrupt_cbk()) {
				Gx_UDP_Close(udp);
				return GX_PLAYER_ERROR;
			}

			int nread = read_rtp_from_server(udp->udp_fd, buffer, MAX_RTP_PACKET_SIZE, 0);
			if(nread < 0){
				Gx_UDP_Close(udp);
				return GX_PLAYER_ERROR;
			}else if(nread > 0){
				streaming_bufferize(streaming_ctrl, (char*)buffer, nread);
				break;
			}
		}
	}

	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);
	streaming_ctrl->priv = udp;
	streaming_ctrl->streaming_read = NULL;
	streaming_ctrl->streaming_seek = NULL;
	streaming_ctrl->prebuffer_size = stream_buffer_size;
	streaming_ctrl->buffering = 0;
	streaming_ctrl->status = streaming_playing_e;

	return GX_PLAYER_OK;
}

static int rtp_stream_open(GxStream*  s, int mode)
{
	GxStreamRtp* stream = (GxStreamRtp *) (s);
	GxURL* url;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &s->interrupt_cbk);
	stream->streaming_ctrl = streaming_ctrl_new();
	if (!stream->streaming_ctrl)
		return GX_PLAYER_ERROR;

	url = url_new(stream->parent.url);
	stream->streaming_ctrl->url = check4proxies(url);
	url_free(url);

	if (url->port == 0) {
		streaming_ctrl_free(stream->streaming_ctrl);
		stream->streaming_ctrl = NULL;
		return GX_PLAYER_ERROR;
	}

	if (rtp_streaming_start(&stream->parent) < 0) {
		streaming_ctrl_free(stream->streaming_ctrl);
		stream->streaming_ctrl = NULL;
		return GX_PLAYER_ERROR;
	}

	stream->parent.file_format = GX_STREAMTYPE_STREAM;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_UNKNOWN;
	s->streaming_ctrl = stream->streaming_ctrl;

	return GX_PLAYER_OK;
}

static int rtp_stream_read(GxStream* s, uint8_t * buffer, size_t max_len)
{
	int recv_size = -1;
	GxStreamRtp* stream = (GxStreamRtp*)s;
	GxStreamingCtrl* stream_ctrl = stream->streaming_ctrl;
	UDPContext* udp = NULL;

	udp = stream_ctrl->priv;
	if(udp == NULL)
		return -1;

read_again:
	if(stream->parent.interrupt_cbk && stream->parent.interrupt_cbk())
		return -1;

	if (stream_ctrl->buffer_size > 0) {
		int buffer_len = stream_ctrl->buffer_size - stream_ctrl->buffer_pos;
		recv_size = (max_len < buffer_len) ? max_len : buffer_len;
		memcpy(buffer, (stream_ctrl->buffer) + (stream_ctrl->buffer_pos), recv_size);
		stream_ctrl->buffer_pos += recv_size;
		if (stream_ctrl->buffer_pos >= stream_ctrl->buffer_size) {
			streaming_buffer_free(stream_ctrl);
		}
		goto read_complete;
	}

	if(max_len < MAX_RTP_PACKET_SIZE){
		char tmp_buffer[MAX_RTP_PACKET_SIZE];
		int nread = read_rtp_from_server(udp->udp_fd, tmp_buffer, MAX_RTP_PACKET_SIZE, 0);
		if(nread > 0){
			streaming_bufferize(stream_ctrl, (char*)tmp_buffer, nread);
			goto read_again;
		}
		return nread;
	}

	recv_size = read_rtp_from_server(udp->udp_fd, (char*)buffer, max_len, 0);
read_complete:
	return recv_size;
}

static int rtp_streaming_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;

	if(!streaming_ctrl){
		return GX_PLAYER_ERROR;
	}
	if((size <= streaming_ctrl->buffer_pos)&&(streaming_ctrl->buffer)){
		streaming_ctrl->buffer_pos -= size;
	}else
		streaming_bufferize(streaming_ctrl, buffer, size);

	return GX_PLAYER_OK;
}

static int rtp_stream_control(GxStream *s, int cmd, void *arg)
{
	switch (cmd) {
		case GX_STREAM_CTRL_RESET:
		{
			s->buf_pos = 0;
			s->buf_len = 0;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_SEEK_TO_TIME:
		{
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_ROLL_BACK:
		{
			GxStream_RollBack* rol = arg;
			if(!rol)
				return GX_PLAYER_ERROR;
			rtp_streaming_rollback(s, rol->buffer, rol->size);
			return GX_PLAYER_OK;
		}

		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

static void rtp_stream_close(GxStream* s)
{
	GxStreamRtp* stream = (GxStreamRtp*)s;

	if(stream->streaming_ctrl){
		if(stream->streaming_ctrl->priv){
			Gx_UDP_Close(stream->streaming_ctrl->priv);
			stream->streaming_ctrl->priv = NULL;
		}
		streaming_ctrl_free(stream->streaming_ctrl);
		stream->streaming_ctrl = NULL;
	}

	uninit_rtp_cache();
}

GxStreamClass gx_stream_rtp = {
	._inherit = {
		._inherit = {
			.name   = "StreamRnm",
			.parent = &gx_streambase,
			.size   = sizeof(GxStreamRtp),
		},
	},
	.protocols = {"rtp", NULL},
	DEF_AUTHOR("Stream","rtp","No description","L.F","No comment"),

	.flags   = 0,
	.open    = rtp_stream_open,
	.read    = rtp_stream_read,
	.write   = NULL,
	.seek    = NULL,
	.control = rtp_stream_control,
	.close   = rtp_stream_close,
};
