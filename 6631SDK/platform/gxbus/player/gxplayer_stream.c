#include "gxplayer_module.h"
#include "gx_common.h"
#include "gx_stream.h"
#include "avformat.h"
#include "network.h"

#define CHECK_GXPLAYER_STREAM_NULL(ptr) do {   \
	if (NULL == ptr) {                         \
		gxloge("%s %d\n", __func__, __LINE__); \
		return -1;                             \
	}                                          \
} while(0);

typedef struct {
	off_t         filesize;
	off_t         rdoff;
	uint64_t      duration;
	uint32_t      bitrate;
	GxStream     *stream;
	URLContext   *ff_ctx;
} GxPlayerStream;

void *GxPlayer_StreamOpen(const char *url, GxPlayerStreamMode mode)
{
	GxPlayerStream *ps = av_mallocz(sizeof(GxPlayerStream));
	int flags0 = 0, flags1 = 0;

	if (NULL == ps) {
		gxloge("%s %d\n", __func__, __LINE__);
		return NULL;
	}

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, url);
	switch (mode) {
	case GX_PLAYER_STREAM_RONLY:
		flags0 = GX_STREAM_READ;
		flags1 = URL_RDONLY;
		break;
	case GX_PLAYER_STREAM_WONLY:
		flags0 = GX_STREAM_WRITE;
		flags1 = URL_WRONLY;
		break;
	case GX_PLAYER_STREAM_TRUNC:
		flags0 = GX_STREAM_TRUNC;
		flags1 = URL_RDWR;
		break;
	case GX_PLAYER_STREAM_APPEND:
		flags0 = GX_STREAM_APPEND;
		flags1 = URL_RDWR;
		break;
	default:
		gxloge("%s %d\n", __func__, __LINE__);
		return NULL;
	}
	ps->stream = GxStream_Open(url, flags0, 0);
	if (NULL == ps->stream) {
		av_free(ps);
		gxloge("%s %d\n", __func__, __LINE__);
		return NULL;
	}

#ifdef CONFIG_FFMPEG_ENABLE
	if (ps->stream->file_format == GX_STREAMTYPE_DEMUXER) {
		AVDictionary *ff_opts = NULL;
		int ret = -1, timeout_ms = 0;
		extern int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags);

	//      delete for time out for 20s
	//	GxPlayer_SystemGet(PSYS_NETWORK_TIMEOUT, &timeout_ms);
	//	av_dict_set_int(&ff_opts, "rw_timeout", timeout_ms, 0);
		if (ps->stream->options) {
			char *strings = NULL;

			strings = GxOptions_Get_By_Id(ps->stream->options, "-H", OPTIONS_USER_AGENT);
			av_dict_set(&ff_opts, "user_agent", strings, 0);
			strings = GxOptions_Get_By_Id(ps->stream->options, "-H", OPTIONS_REFERER);
			av_dict_set(&ff_opts, "referer", strings, 0);
			strings = GxOptions_Get_By_Id(ps->stream->options, "-H", OPTIONS_CONNECTION);
			av_dict_set(&ff_opts, "connection", strings, 0);
		}
		ret = url_open(&ps->ff_ctx, url, flags1, &ff_opts);
		if (ff_opts)
			av_dict_free(&ff_opts);
		if (ret < 0) {
			GxStream_Close(ps->stream);
			av_free(ps);
			gxloge("%s %d\n", __func__, __LINE__);
			return NULL;
		}
	}
#endif
	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);

	return ((void*)ps);
}

int GxPlayer_StreamClose(void *handle)
{
	GxPlayerStream *ps = (GxPlayerStream *)handle;

	CHECK_GXPLAYER_STREAM_NULL(ps);

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	if (ps->stream) {
		GxStream_Close(ps->stream);
		ps->stream = NULL;
	}
#ifdef CONFIG_FFMPEG_ENABLE
	if (ps->ff_ctx) {
		url_close(ps->ff_ctx);
		ps->ff_ctx = NULL;
	}
#endif
	av_free(ps);
	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);

	return 0;
}

size_t GxPlayer_StreamRead(void *handle, uint8_t *buffer, size_t size)
{
	size_t read_size = 0;
	GxPlayerStream *ps = (GxPlayerStream *)handle;

	CHECK_GXPLAYER_STREAM_NULL(ps);

	if (ps->ff_ctx) {
#ifdef CONFIG_FFMPEG_ENABLE
		read_size = url_read(ps->ff_ctx, buffer, size);
		if (read_size > 0) {
			ps->rdoff += read_size;
		} else if (read_size == 0) {
			off_t tol_size = url_filesize(ps->ff_ctx);
			if (ps->rdoff >= tol_size)
				read_size = AVERROR_EOF;
		}
#endif
	} else if (ps->stream) {
		read_size =  GxStream_Read(ps->stream, buffer, size);
		if (read_size == 0) {
			off_t tol_size = 0;
			off_t res_size = 0;

			GxStream_Control(ps->stream, GX_STREAM_CTRL_GET_SIZE,     &tol_size);
			GxStream_Control(ps->stream, GX_STREAM_CTRL_GET_RES_SIZE, &res_size);
			if (ps->stream->eof || ((tol_size > 0) && (res_size == 0)))
				read_size = AVERROR_EOF;
		}
	}

	av_flow_log("[FLOW] - [%s %d] %d\n", __func__, __LINE__, read_size);
	return read_size;
}

size_t GxPlayer_StreamWrite(void *handle, uint8_t *buffer, size_t size)
{
	av_flow_log("[FLOW] - [%s %d] %d\n", __func__, __LINE__, size);
	gxloge("%s %d: not support write\n", __func__, __LINE__);
	return -1;
}

off_t GxPlayer_StreamSeek(void *handle, off_t pos)
{
	off_t ret_pos = -1;
	GxPlayerStream *ps = (GxPlayerStream *)handle;

	CHECK_GXPLAYER_STREAM_NULL(ps);

	if (ps->ff_ctx) {
#ifdef CONFIG_FFMPEG_ENABLE
		ret_pos = url_seek(ps->ff_ctx, pos, SEEK_CUR);
#endif
	} else if (ps->stream) {
		if (GX_PLAYER_OK == GxStream_Seek(ps->stream, pos))
			ret_pos = GxStream_Tell(ps->stream);
	}

	av_flow_log("[FLOW] - [%s %d] %lld\n", __func__, __LINE__, ret_pos);
	return ret_pos;
}

int GxPlayer_StreamControl(void *handle, GxPlayerStreamCmd cmd, void *arg)
{
	GxPlayerStream *ps = (GxPlayerStream *)handle;

	CHECK_GXPLAYER_STREAM_NULL(ps);

	switch (cmd) {
	case GX_PLAYER_STREAM_GET_TOTLE_SIZE:
		*(off_t *)arg = ps->filesize;
		break;
	case GX_PLAYER_STREAM_GET_DURATION:
		*(uint64_t *)arg = ps->duration;
		break;
	case GX_PLAYER_STREAM_GET_BITRATE:
		*(uint32_t *)arg = ps->bitrate;
		break;
	case GX_PLAYER_STREAM_GET_CURPOS:
		if (ps->ff_ctx) {
#ifdef CONFIG_FFMPEG_ENABLE
			*(off_t *)arg = url_seek(ps->ff_ctx, 0, SEEK_CUR);
#endif
		} else {
			*(off_t *)arg = GxStream_Tell(ps->stream);
		}
		av_flow_log("[FLOW] - [%s %d] %lld\n", __func__, __LINE__, *(off_t *)arg);
		break;
	default:
		return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}
