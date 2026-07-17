/*
*   Copyright (C) 2006 Benjamin Zores
*    based on previous Real RTSP support from Roberto Togni and xine team.
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

#include <sys/socket.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "gx_common.h"
#include "gx_demux.h"
#include "gx_stream.h"
#include "gx_system.h"
#include "tcp.h"
#include "librtsp/rtsp.h"
#include "librtsp/rtsp_session.h"

extern int network_bandwidth;
#define RTSP_PLAY_RUNING   1
#define RTSP_PLAY_PAUSE    2
#define RTSP_PLAY_STOPPED  3

static int rtsp_player_status = RTSP_PLAY_STOPPED;
static int rtsp_time_over = 0;

static int rtsp_stream_read(GxStream*  stream, uint8_t * buffer, size_t max_len)
{
	int recv_size = 0;

	if(stream->file_format == GX_STREAMTYPE_DEMUXER)
		return GX_PLAYER_OK;

	if((stream->streaming_ctrl == NULL) ||
			(stream->interrupt_cbk && stream->interrupt_cbk()))
		return -1;

	if(rtsp_player_status == RTSP_PLAY_PAUSE){
		GxCore_ThreadDelay(100);
		return 0;
	}

	recv_size = rtsp_session_read(stream->streaming_ctrl->data, (char*)buffer, max_len, rtsp_time_over);

	return recv_size;
}

static int rtsp_stream_start(GxStream*  s)
{
	GxStreamRtsp* stream = (GxStreamRtsp *) (s);
	int fd;
	rtsp_session_t* rtsp;
	char* mrl;
	char* file;
	int port;
	int redirected, temp;

	if (!stream)
		return -1;

	/* counter so we don't get caught in infinite redirections*/
	temp = 5;

	do {
		redirected = 0;

		fd = connect_2_server(stream->streaming_ctrl->url->hostname,
				      port = (stream->streaming_ctrl->url->port ?
					      stream->streaming_ctrl->url->port : 554), 1, s->interrupt_cbk);
		gxlogf("connect server first fd is:%d\n", fd);
		if (fd < 0 && !stream->streaming_ctrl->url->port){
			fd = connect_2_server(stream->streaming_ctrl->url->hostname, port = 7070, 1, s->interrupt_cbk);
			gxlogf("connect server second fd is:%d\n", fd);
		}
		gxlogf("port:%d hostname:%s\n", port, stream->streaming_ctrl->url->hostname);
		if (fd < 0) {
			gxlogf("after second connect fd is smaller than zero\n");
			if (-1 != fd)
				return fd;
			else
				return -1;
		}
		file = stream->streaming_ctrl->url->file;
		if (file[0] == '/')
			file++;

		mrl = av_malloc(strlen(stream->streaming_ctrl->url->hostname)
			     + strlen(file) + 16);

		snprintf(mrl, strlen(stream->streaming_ctrl->url->hostname) + strlen(file) + 16, "rtsp://%s:%i/%s", stream->streaming_ctrl->url->hostname, port, file);
		//gxlogf("fd:%d \n mrl:%s\n file:%s\n hostname:%s\n bandwidth:%d\n username:%s\n password:%s\n");
		rtsp = rtsp_session_start(fd, &mrl, file,
					  stream->streaming_ctrl->url->hostname,
					  port, &redirected,
					  stream->streaming_ctrl->bandwidth,
					  stream->streaming_ctrl->url->username, stream->streaming_ctrl->url->password);

		if (redirected == 1) {
			url_free(stream->streaming_ctrl->url);
			stream->streaming_ctrl->url = url_new(mrl);
			closesocket(fd);
		}

		av_free(mrl);
		temp--;
	} while ((redirected != 0) && (temp > 0));

	if (!rtsp) {
		gxlogf("rtsp_session_start fail\n");
		return -1;
	}
	stream->parent.fd = fd;
	stream->streaming_ctrl->data = rtsp;
	stream->streaming_ctrl->streaming_read = NULL;
	stream->streaming_ctrl->streaming_seek = NULL;
	stream->streaming_ctrl->status = streaming_playing_e;

	return 0;
}

static int rtsp_stream_pause(GxMediaFilter *filter)
{
	GxStream* s = (GxStream*)GXSTREAM(filter);
	GxStreamRtsp* stream = (GxStreamRtsp*)s;

	if(s->file_format == GX_STREAMTYPE_DEMUXER)
		return GX_PLAYER_OK;

	rtsp_session_pause(stream->streaming_ctrl->data);
	rtsp_player_status = RTSP_PLAY_PAUSE;
	return GX_PLAYER_OK;
}

static int rtsp_stream_resume(GxMediaFilter *filter)
{
	GxStream* s = (GxStream*)GXSTREAM(filter);
	GxStreamRtsp* stream = (GxStreamRtsp*)s;

	if(s->file_format == GX_STREAMTYPE_DEMUXER)
		return GX_PLAYER_OK;

	rtsp_session_resume(stream->streaming_ctrl->data);
	rtsp_player_status = RTSP_PLAY_RUNING;
	return GX_PLAYER_OK;
}

static int rtsp_stream_open(GxStream*  s, int mode)
{
	GxStreamRtsp* stream = (GxStreamRtsp *) (s);
	GxURL* url;
	int retflg = -1;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &s->interrupt_cbk);
	stream->streaming_ctrl = streaming_ctrl_new();
	if (!stream->streaming_ctrl)
		return GX_PLAYER_ERROR;

	stream->streaming_ctrl->bandwidth = network_bandwidth;
	url = url_new(stream->parent.url);
	stream->streaming_ctrl->url = check4proxies(url);

	if ((retflg = rtsp_stream_start(s)) < 0) {
		if (-1 != retflg)
			return GX_PLAYER_ERROR;
		stream->parent.file_format = GX_STREAMTYPE_DEMUXER;
		stream->parent.demuxer_type = GX_DEMUXER_TYPE_LAVF;
	}else{
		stream->parent.file_format = GX_STREAMTYPE_STREAM;
		stream->parent.demuxer_type = GX_DEMUXER_TYPE_UNKNOWN;
		if (rtsp_get_duration(stream->streaming_ctrl->data) > 0) {
			stream->parent.flags |= GX_STREAM_SEEK;
			stream->parent.flags |= GX_STREAM_SEEK_TIME;
		} else {
			stream->parent.flags &= ~GX_STREAM_SEEK;
			stream->parent.flags &= ~GX_STREAM_SEEK_TIME;
		}
		rtsp_time_over = 0;
		rtsp_player_status = RTSP_PLAY_RUNING;
	}
	url_free(url);
	stream->parent.streaming_ctrl = stream->streaming_ctrl;
	return GX_PLAYER_OK;
}

static void rtsp_stream_close(GxStream*  stream)
{
	GxStreamRtsp* s = (GxStreamRtsp *) (stream);

	if(s->streaming_ctrl){
		if(s->streaming_ctrl->data)
			rtsp_session_end(s->streaming_ctrl->data);
		if(s->streaming_ctrl->buffer)
			av_free(s->streaming_ctrl->buffer);
		if(s->parent.fd > 0)
			closesocket(s->parent.fd);
		s->parent.fd = -1;
		s->streaming_ctrl->data = NULL;
		s->streaming_ctrl->buffer = NULL;
		s->streaming_ctrl->streaming_read = NULL;
		s->streaming_ctrl->streaming_seek = NULL;
		s->streaming_ctrl->status = streaming_stopped_e;
		streaming_ctrl_free(s->streaming_ctrl);
		s->streaming_ctrl = NULL;
	}
	rtsp_time_over = 0;
	rtsp_player_status = RTSP_PLAY_STOPPED;
	stream->interrupt_cbk = NULL;
}

static int rtsp_stream_control(GxStream *s, int cmd, void *arg)
{
	if(s->file_format == GX_STREAMTYPE_DEMUXER)
		return GX_PLAYER_OK;

	switch (cmd) {
		case GX_STREAM_CTRL_RESET:
		{
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_SEEK_TO_TIME:
		{
			GxStreamingCtrl* stream_ctrl = s->streaming_ctrl;
			if(stream_ctrl)
				rtsp_session_seek(stream_ctrl->data, *(uint64_t*)arg);
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_TOTAL_TIME:
		{
			if(s->streaming_ctrl)
				*(int*)arg = rtsp_get_duration(s->streaming_ctrl->data);
			else
				*(int*)arg = 0;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_SEND_ALIVE:
		{
			if(s->streaming_ctrl)
				rtsp_session_send_alive(s->streaming_ctrl->data);
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_TIME_OVER:
		{
			rtsp_time_over = 1;
			break;
		}
		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

GxStreamClass gx_stream_rtsp = {
	._inherit = {
		._inherit = {
			.name = "StreamRtsp",
			.parent = &gx_streambase,
			.size = sizeof(GxStreamRtsp),
		},
		.pause  = rtsp_stream_pause,
		.resume = rtsp_stream_resume,
	},
	.protocols = {"rtsp", NULL},
	DEF_AUTHOR("Stream","rtsp","No description","L.F","No comment"),

	.flags   = 0,
	.open    = rtsp_stream_open,
	.read    = rtsp_stream_read,
	.write   = NULL,
	.seek    = NULL,
	.control = rtsp_stream_control,
	.close   = rtsp_stream_close,
};
