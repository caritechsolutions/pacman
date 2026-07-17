/*
*   Copyright (C) 2006 Benjamin Zores
*    Stream layer for MPEG over UDP, based on previous work from Dave Chapman
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

#include "gx_common.h"
#include "gx_network.h"
#include "gx_stream.h"
#include "stream_m3u8.h"
#include "gx_demux.h"
#include "gx_url.h"
#include "udp.h"

#define STREAM_UDP_PVR_DEBUG 0
#if STREAM_UDP_PVR_DEBUG
static FILE* stream_udp_pvr_fd = NULL;
static char* stream_udp_pvr_file = "/media/sda1/net_pvr/1-udp.ts";
#endif

typedef struct gx_stream_udp {

	GxStream parent;
	GxStreamingCtrl* streaming_ctrl;
} GxStreamUdp;

extern int GxUDP_Open_Socket(GxURL*  url);

static int udp_streaming_start(GxStream*  s)
{
	int stream_buffer_size = 0;
	GxStreamUdp* stream = (GxStreamUdp *) (s);
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;
	UDPContext* udp = NULL;

	udp = Gx_UDP_Open(streaming_ctrl->url);
	if (udp == NULL)
			return GX_PLAYER_ERROR;

	//预留一部份数据在buffer,以便于数据探测
	{
		uint8_t buffer[STREAM_UDP_MAX_PKT_SIZE];

		while(1){
			if (s->interrupt_cbk && s->interrupt_cbk()){
				Gx_UDP_Close(udp);
				return GX_PLAYER_ERROR;
			}

			int nread = Gx_UDP_Read(udp, buffer, STREAM_UDP_MAX_PKT_SIZE);
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

static int udp_streaming_start4server(GxStream*  s)
{
	int stream_buffer_size = 0;
	GxStreamUdp* stream = (GxStreamUdp *) (s);
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;
	UDPContext* udp = NULL;

	udp = Gx_UDP_Open4Server(streaming_ctrl->url);
	if (udp == NULL)
		return GX_PLAYER_ERROR;
	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);

	streaming_ctrl->priv = udp;
	streaming_ctrl->streaming_read = NULL;
	streaming_ctrl->streaming_seek = NULL;
	streaming_ctrl->prebuffer_size = stream_buffer_size;
	streaming_ctrl->buffering = 0;
	streaming_ctrl->status = streaming_playing_e;

	return GX_PLAYER_OK;
}

static int udp_stream_open(GxStream*  s, int mode)
{
	GxStreamUdp* stream = (GxStreamUdp *) (s);
	GxURL* url;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &s->interrupt_cbk);
	gxlogf("STREAM_UDP, URL: %s\n", stream->parent.url);
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

	if (mode == GX_STREAM_TRUNC) {
		if (udp_streaming_start4server(&stream->parent) < 0) {
			streaming_ctrl_free(stream->streaming_ctrl);
			stream->streaming_ctrl = NULL;
			return GX_PLAYER_ERROR;
		}
	} else {
		if (udp_streaming_start(&stream->parent) < 0) {
			streaming_ctrl_free(stream->streaming_ctrl);
			stream->streaming_ctrl = NULL;
			return GX_PLAYER_ERROR;
		}
	}

	stream->parent.file_format = GX_STREAMTYPE_STREAM;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_UNKNOWN;
	s->streaming_ctrl = stream->streaming_ctrl;
#if STREAM_UDP_PVR_DEBUG
		if(stream_udp_pvr_fd)
			fclose(stream_udp_pvr_fd);
		stream_udp_pvr_fd = fopen(stream_udp_pvr_file, "w+");
#endif
	return GX_PLAYER_OK;
}

static int udp_stream_read(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	int recv_size = -1;
	GxStreamUdp* stream = (GxStreamUdp*)s;
	GxStreamingCtrl* stream_ctrl = stream->streaming_ctrl;

read_again:
	if(stream->parent.interrupt_cbk && stream->parent.interrupt_cbk())
		return -1;

	if (stream_ctrl->buffer_size > 0) {
		int buffer_len = stream_ctrl->buffer_size - stream_ctrl->buffer_pos;
		recv_size = (max_len < buffer_len) ? max_len : buffer_len;
		memcpy(buffer, (stream_ctrl->buffer + stream_ctrl->buffer_pos), recv_size);
		stream_ctrl->buffer_pos += recv_size;
		if (stream_ctrl->buffer_pos >= stream_ctrl->buffer_size) {
			streaming_buffer_free(stream_ctrl);
		}
		goto read_complete;
	}

	if(max_len < STREAM_UDP_MAX_PKT_SIZE){
		uint8_t tmp_buffer[STREAM_UDP_MAX_PKT_SIZE];
		int nread = Gx_UDP_Read(stream_ctrl->priv, tmp_buffer, STREAM_UDP_MAX_PKT_SIZE);
		if(nread > 0){
			streaming_bufferize(stream_ctrl, (char*)tmp_buffer, nread);
			goto read_again;
		}
		return nread;
	}

	if(stream->streaming_ctrl->priv)
		recv_size = Gx_UDP_Read(stream->streaming_ctrl->priv, buffer, max_len);

read_complete:
#if STREAM_UDP_PVR_DEBUG
	if(stream_udp_pvr_fd && recv_size > 0){
		fwrite(buffer, recv_size, 1, stream_udp_pvr_fd);
	}
#endif

	return recv_size;
}

static int udp_stream_write(GxStream* s, uint8_t * buffer, size_t size)
{
	int send_size = -1;
	GxStreamUdp* stream = (GxStreamUdp*)s;
	GxStreamingCtrl* stream_ctrl = stream->streaming_ctrl;

	if(stream->parent.interrupt_cbk && stream->parent.interrupt_cbk())
		return -1;

	if (size > 0) {
        if(stream->streaming_ctrl->priv)
		send_size = Gx_UDP_Write(stream->streaming_ctrl->priv, buffer, size);
	}

	return send_size;
}

static int udp_streaming_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;

	if(!streaming_ctrl){
		return GX_PLAYER_ERROR;
	}
	if((size <= streaming_ctrl->buffer_pos)&&(streaming_ctrl->buffer)){
		streaming_ctrl->buffer_pos -= size;
	}else
		streaming_bufferize(streaming_ctrl, buffer, size);

#if STREAM_UDP_PVR_DEBUG
	if(stream_udp_pvr_fd)
		fclose(stream_udp_pvr_fd);
	stream_udp_pvr_fd = fopen(stream_udp_pvr_file, "w+");
#endif
	return GX_PLAYER_OK;
}

static int udp_stream_control(GxStream *s, int cmd, void *arg)
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
			udp_streaming_rollback(s, rol->buffer, rol->size);
			return GX_PLAYER_OK;
		}

		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

static void udp_stream_close(GxStream* s)
{
	GxStreamUdp* stream = (GxStreamUdp*)s;

#if STREAM_UDP_PVR_DEBUG
	if(stream_udp_pvr_fd)
		fclose(stream_udp_pvr_fd);
	stream_udp_pvr_fd = NULL;
#endif
	if(stream->streaming_ctrl){
		if(stream->streaming_ctrl->priv){
			Gx_UDP_Close(stream->streaming_ctrl->priv);
			stream->streaming_ctrl->priv = NULL;
		}
		streaming_ctrl_free(stream->streaming_ctrl);
		stream->streaming_ctrl = NULL;
	}
}

GxStreamClass gx_stream_udp = {
	._inherit = {
		._inherit = {
			.name   = "StreamUdp",
			.parent = &gx_streambase,
			.size   = sizeof(GxStreamUdp),
		},
	},
	.protocols = {"udp", NULL},
	DEF_AUTHOR("Stream","udp","No description","L.F","No comment"),

	.flags   = 0,
	.open    = udp_stream_open,
	.read    = udp_stream_read,
	.write   = udp_stream_write,
	.seek    = NULL,
	.control = udp_stream_control,
	.close   = udp_stream_close,
};
