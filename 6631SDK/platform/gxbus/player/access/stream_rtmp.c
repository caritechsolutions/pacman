#include "gx_common.h"
#include "gx_demux.h"
#include "gx_stream.h"
#include "gx_demux.h"
#include "librtmp/rtmp.h"
#include "librtmp/log.h"
#include "stream_m3u8.h"
#include "gx_system.h"

#define DEF_BUFTIME  (3 * 1000)/* 10 hours default */

typedef struct gx_stream_rtmp{
        GxStream parent;
        GxStreamingCtrl *streaming_ctrl;
        int timeout;
}GxStreamRtmp;

#define RTMP_STREAM_FILE_DEBUG 0
#if RTMP_STREAM_FILE_DEBUG
char* rtmp_stream_file = "/media/sda1/net_pvr/01_rtmp.flv";
FILE* rtmp_file_fd = NULL;
#endif

static int _rtmp_read(RTMP* rtmp, uint8_t* buffer, int size)
{
	double duration = 0.0;
	unsigned int  bufferTime = DEF_BUFTIME;
	int nRead = 0;

	nRead = RTMP_Read(rtmp, (char*)buffer, size);
	if (nRead > 0){
		if (duration <= 0)	// if duration unknown try to get it from the stream (onMetaData)
			duration = RTMP_GetDuration(rtmp);
		if (duration > 0){
			// make sure we claim to have enough buffer time!
			if (bufferTime < (duration * 1000.0)){
				bufferTime = (uint32_t) (duration * 1000.0) + 5000;	// extra 5sec to make sure we've got enough
				RTMP_Log(RTMP_LOGDEBUG,
					"Detected that buffer time is less than duration, resetting to: %dms",
					bufferTime);
				RTMP_SetBufferMS(rtmp, bufferTime);
				RTMP_UpdateBufferMS(rtmp);
			}
		}
		//gxlogd("download at: %.3f kB\n",(double) nRead / 1024.0);
	}
	return nRead;
}

static int _rtmp_start(GxStream*  s)
{
	GxStreamRtmp* stream = (GxStreamRtmp *) (s);
	unsigned int port = 0;
	int blive = 1;
	int protocol = RTMP_PROTOCOL_RTMP;
	RTMP* rtmp = NULL;
	AVal tcurl = { 0, 0 };
	AVal app = { 0, 0 };
	AVal hostname = { 0, 0 };
	AVal playpath = { 0, 0 };
	AVal swfurl = {0, 0};
	AVal pageurl = {0, 0};

	rtmp = (RTMP*)av_malloc(sizeof(RTMP));
	if(!rtmp){
		gxlogd("error: %s %d\n",__FUNCTION__,__LINE__);
		return GX_PLAYER_ERROR;
	}

	GxPlayer_SystemGet(PSYS_NETWORK_TIMEOUT, &stream->timeout);

	RTMP_Init(rtmp);
	if (!RTMP_ParseURL(s->url, &protocol, &hostname, &port,&playpath, &app,
				&tcurl, &pageurl, &swfurl, &blive)){
		gxlogd("error: %s %d\n",__FUNCTION__,__LINE__);
		av_free(rtmp);
		return GX_PLAYER_ERROR;
	}

	RTMP_SetupStream(rtmp, protocol, &hostname, port, NULL, &playpath,
		   &tcurl, &swfurl, &pageurl, &app, NULL, NULL, -1, NULL, NULL, 0, 0, blive, stream->timeout, s->interrupt_cbk);

	RTMP_SetBufferMS(rtmp, DEF_BUFTIME);
	if (!RTMP_Connect(rtmp, NULL)){
		gxlogd("error: %s %d\n",__FUNCTION__,__LINE__);
		RTMP_Close(rtmp);
		RTMP_FreeURL(&hostname, &playpath, &app, &tcurl, &pageurl, &swfurl);
		av_free(rtmp);
		return GX_PLAYER_ERROR;
	}
	if (!RTMP_ConnectStream(rtmp, 0)) {
		gxlogd("error: %s %d\n",__FUNCTION__,__LINE__);
		RTMP_Close(rtmp);
		RTMP_FreeURL(&hostname, &playpath, &app, &tcurl, &pageurl, &swfurl);
		av_free(rtmp);
		return GX_PLAYER_ERROR;
	}

	//预留一部份数据在buffer,以便于数据探测
	if (!stream->parent.nobuffer)
	{
		int read_size = 0;
		uint8_t buffer[512] = {0};

		while(read_size < sizeof(buffer)/sizeof(uint8_t)){
			if(s->interrupt_cbk && s->interrupt_cbk())
				goto RTMP_START_ERROR;

			int nread = _rtmp_read(rtmp, buffer+read_size, sizeof(buffer)/sizeof(uint8_t)-read_size);
			if(nread < 0)
				goto RTMP_START_ERROR;
			read_size += nread;
		}
		streaming_bufferize(stream->streaming_ctrl, (char*)buffer, read_size);
	}

	s->flags |= GX_STREAM_SEEK;
	s->flags |= GX_STREAM_SEEK_TIME;

	stream->streaming_ctrl->data = rtmp;
	stream->streaming_ctrl->streaming_read = NULL;
	stream->streaming_ctrl->streaming_seek = NULL;
	return GX_PLAYER_OK;

RTMP_START_ERROR:
	RTMP_Close(rtmp);
	RTMP_FreeURL(&hostname, &playpath, &app, &tcurl, &pageurl, &swfurl);
	av_free(rtmp);
	return GX_PLAYER_ERROR;
}

static int _rtmp_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;

	if(!streaming_ctrl){
		return GX_PLAYER_ERROR;
	}
	if((size <= streaming_ctrl->buffer_pos)&&(streaming_ctrl->buffer)){
		streaming_ctrl->buffer_pos -= size;
	}else
		streaming_bufferize(streaming_ctrl, buffer, size);

#if RTMP_STREAM_FILE_DEBUG
	if(rtmp_file_fd)
		fclose(rtmp_file_fd);
	rtmp_file_fd = fopen(rtmp_stream_file, "w+");
#endif
	return GX_PLAYER_OK;
}

static int _rtmp_seek(GxStream*  s, int64_t time)
{
	if (!s->streaming_ctrl)
		return GX_PLAYER_ERROR;

	if (!RTMP_SendSeek(s->streaming_ctrl->data, (int)time)) {
		gxlogd("error: %s %d\n",__FUNCTION__,__LINE__);
		return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}

static int rtmp_stream_read(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	size_t recv_size = 0;

	if(s->streaming_ctrl == NULL)
		return -1;

	if (s->streaming_ctrl->buffer_size != 0) {
		int buffer_len = s->streaming_ctrl->buffer_size - s->streaming_ctrl->buffer_pos;
		recv_size = (max_len < buffer_len) ? max_len : buffer_len;
		memcpy(buffer, (s->streaming_ctrl->buffer) + (s->streaming_ctrl->buffer_pos), recv_size);
		s->streaming_ctrl->buffer_pos += recv_size;
		if (s->streaming_ctrl->buffer_pos >= s->streaming_ctrl->buffer_size) {
			streaming_buffer_free(s->streaming_ctrl);
		}
		goto read_complete;
	}

	recv_size = _rtmp_read(s->streaming_ctrl->data, buffer, max_len);
	if (recv_size > 0)
		s->err.recv_err = 0;
	else if (recv_size == 0) {
		s->err.recv_err++;
		if(s->err.recv_err > 50){
			recv_size = -1;
			s->err.need_restart = 1;
		}
	}

read_complete:
#if RTMP_STREAM_FILE_DEBUG
	if(rtmp_file_fd && recv_size>0){
		fwrite(buffer, recv_size, 1, rtmp_file_fd);
	}
#endif
	gxlogf("Rtmp read size: %.02f(KB)\n",(double)recv_size/1024.0);
	return recv_size;
}

static int rtmp_stream_open(GxStream *s, int mode)
{
	GxStreamRtmp* stream = (GxStreamRtmp *) (s);
	char *cache_value = NULL;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &s->interrupt_cbk);
	stream->streaming_ctrl = streaming_ctrl_new();
	if (!stream->streaming_ctrl)
		return GX_PLAYER_ERROR;
	cache_value = GxOptions_Get_By_Name(((GxStream*)stream)->options, " -H", "no_cache_flag:");
	stream->parent.nobuffer = cache_value?atoi(cache_value):0;

	if (_rtmp_start(s) < 0) {
		streaming_ctrl_free(stream->streaming_ctrl);
		s->streaming_ctrl = stream->streaming_ctrl = NULL;
		return GX_PLAYER_ERROR;
	}

	s->file_format = GX_STREAMTYPE_STREAM;
	s->demuxer_type = GX_DEMUXER_TYPE_LAVF;
	s->streaming_ctrl = stream->streaming_ctrl;
#if RTMP_STREAM_FILE_DEBUG
	if(rtmp_file_fd)
		fclose(rtmp_file_fd);
	rtmp_file_fd = fopen(rtmp_stream_file, "w+");
#endif
	return GX_PLAYER_OK;
}

static void rtmp_stream_close(GxStream*  s)
{
	GxStreamRtmp* stream = (GxStreamRtmp *) (s);
	RTMP* rtmp = stream->streaming_ctrl->data;

#if RTMP_STREAM_FILE_DEBUG
	if(rtmp_file_fd)
		fclose(rtmp_file_fd);
	rtmp_file_fd = NULL;
#endif

	if(rtmp){
		RTMP_Close(rtmp);
		RTMP_FreeURL(&rtmp->Link.hostname, &rtmp->Link.playpath,
				&rtmp->Link.app, &rtmp->Link.tcUrl, &rtmp->Link.pageUrl, &rtmp->Link.swfUrl);
		av_free(rtmp);
		stream->streaming_ctrl->data = NULL;
	}
	if(stream->streaming_ctrl){
		streaming_ctrl_free(stream->streaming_ctrl);
		s->streaming_ctrl = stream->streaming_ctrl = NULL;
	}
	s->interrupt_cbk = NULL;
}

static int rtmp_stream_control(GxStream *s, int cmd, void *arg)
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
			return _rtmp_seek(s, *(int64_t*)arg);
		}
		case GX_STREAM_CTRL_ROLL_BACK:
		{
			GxStream_RollBack* rol = arg;
			if(!rol)
				return GX_PLAYER_ERROR;
			_rtmp_rollback(s, rol->buffer, rol->size);
			return GX_PLAYER_OK;
		}

		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

GxStreamClass gx_stream_rtmp = {
	._inherit = {
		._inherit = {
			.name = "StreamRtmp",
			.parent = &gx_streambase,
			.size = sizeof(GxStreamRtmp),
		},
	},
	.protocols = {"rtmp", "rtmps", NULL},
	DEF_AUTHOR("Stream","rtmp","No description","L.F","No comment"),

	.flags   = 0,
	.open    = rtmp_stream_open,
	.read    = rtmp_stream_read,
	.write   = NULL,
	.seek    = NULL,
	.control = rtmp_stream_control,
	.close   = rtmp_stream_close,
};
