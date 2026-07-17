#include "gx_demux.h"
#include "gx_options.h"
#include "stream_m3u8.h"
#include "textcode_convert.h"
#include "gx_avtick.h"

#define DEBUG_STREAM

#define GX_STREAM_CLASS_MAX (64)
static int stream_class_num = 0;
static GxStreamClass* gx_stream_classes[GX_STREAM_CLASS_MAX];
GxOptions              *public_stream_options = NULL;
GxStreamOptionCallback public_stream_callback = NULL;
GxStreamMpegtsCallback public_stream_mpegts_callback = NULL;
GxExternalNetTimeCallback external_net_time_callback = NULL;

static char *url_rec_proto(char* dsturl)
{
	int i=0, postfixlen=6;
	char* dot = dsturl;
	char* postfixptr = NULL;
	char  postfix[8];

	if (!dsturl)
		return 0;

	while (dot) {
		postfixptr = ++dot;
		dot = strchr(dot, '.');
	}

	if (postfixptr == dsturl)
		return 0;

	while (*postfixptr != '\0' && i< postfixlen)
		postfix[i++] = tolower(*(postfixptr++));

	postfix[i] = '\0';
	if (!strncmp(postfix, "dvr", 3))
		return "dvr";
	if (!strncmp(postfix, "pvr", 3))
		return "pvr";
	if (!strncmp(postfix, "lst", 3))
		return "pvr";
	if (!strncmp(postfix, "evt", 3))
		return "pvr";
	if (!strncmp(postfix, "seg", 3))
		return "pvr";

	return NULL;
}

static char* stream_parse_url(const char *url, char *proto, int proto_len, GxOptions* options)
{
	char *ptr = NULL, *dot = NULL, *new_url = NULL, *rec_proto = NULL;
	char *priv_whitelist[] = {"associationlibre:", "rtpsdp:", "compose:", "concat:"};
	int i = 0;

	if (!url) return NULL;

	memset(proto, 0, proto_len);

	ptr = strstr(url, " -H");
	if (ptr == NULL) {
		new_url = av_strdup(url);
		for (i = 0; i < sizeof(priv_whitelist)/sizeof(priv_whitelist[0]); i++) {
			if (strstr(new_url, priv_whitelist[i])) {
				memcpy(proto, priv_whitelist[i], strlen(priv_whitelist[i])-1);
				return new_url;
			}
		}

		dot = strstr(new_url, "://");
		if (dot == NULL)
			memcpy(proto, "file", 4);
		else if ((dot - new_url) > proto_len)
			return NULL;
		else
			memcpy(proto, new_url, dot - new_url);

		if (!strcasecmp(proto, "file") && (rec_proto = url_rec_proto(new_url)))
			memcpy(proto, rec_proto, (strlen(rec_proto) + 1));
	} else {
		int len = ptr-url;

		new_url = av_mallocz(len+1);
		if (!new_url)
			return NULL;

		memcpy(new_url, url, len);

		dot = strstr(new_url, "://");
		if (dot == NULL)
			memcpy(proto, "file", 4);
		else if ((dot - new_url) > proto_len)
			return NULL;
		else {
			for (i = 0; i < sizeof(priv_whitelist)/sizeof(priv_whitelist[0]); i++) {
				if (strstr(new_url, priv_whitelist[i])) {
					memcpy(proto, priv_whitelist[i], strlen(priv_whitelist[i])-1);
					if (strcasecmp(priv_whitelist[i], "associationlibre:")) {
						av_free(new_url);
						new_url = av_strdup(url);
					}
					break;
				}
			}
			if (i >= sizeof(priv_whitelist)/sizeof(priv_whitelist[0])) {
				memcpy(proto, new_url, dot - new_url);
			}
		}

		GxOptions_Set_By_Url(options, url);
		if (strstr(ptr, "enc:")) {
			if (NULL != GxOptions_Get_By_Id(public_stream_options, " -H", OPTIONS_ENC_DRM))
				snprintf (proto, proto_len, "%s", "drm");
			else if (!strcasecmp(proto, "file"))
				snprintf (proto, proto_len, "%s", "mtc");
			else if (!strncasecmp(proto, "http", 4))
				snprintf (proto, proto_len, "%s", "crypto");
		}
	}
	return new_url;
}

/*url,need to be converted to ffmpeg options
  * url: input url,for example: url -H User-Agent:***,bat ffmpeg use url user_agent
  * opts_type: example: url -H User-Agent:***, opts_type: user_agent
  * opts_value: value= ***
  */
char* stream_url_opts_info_to_lavf(const char *url, char *opts_type, char *opts_value, int opts_size)
{
	typedef struct
	{
		char access_opts[32];
		char lavf_opts[32];
	}URL_OPTIONS_INFO_TO_LAVF;
	URL_OPTIONS_INFO_TO_LAVF access_opts_to_lavf[] = {
		{"transport_type:", "rtsp_transport"},
		{"User-Agent:", "user_agent"},
		{"Connection:", "Connection"},
		{"Referer:", "Referer"},
		{"timeout:", "rw_timeout"},
		{"Set-Cookie:", "cookies"},
		{"ffprobesize:", "ffprobesize"},
		{"sdp_media_info:", "sdp_media_info"},
		{"ffanalyzeduration:", "analyzeduration"},
	};

	char protocol[32] = {0};
	char *noopt_url = NULL;
	char *value = NULL;
	int i = 0;
	GxOptions *options = GxOptions_Create();

	if (!options)
		return NULL;

	if (!(noopt_url = stream_parse_url(url, protocol, sizeof(protocol)/sizeof(char), options))) {
		GxOptions_Destory(options);
		return NULL;
	}

	for (i = 0; i < sizeof(access_opts_to_lavf)/sizeof(access_opts_to_lavf[0]); i++) {
		if ((value = GxOptions_Get_By_Name(options, " -H", (char *)access_opts_to_lavf[i].access_opts))) {
			break;
		}
	}
	if (i < sizeof(access_opts_to_lavf)/sizeof(access_opts_to_lavf[0])) {
		if (!opts_value) {
			snprintf (opts_type, opts_size-1, "%s", access_opts_to_lavf[i].lavf_opts);
			snprintf (opts_value, opts_size-1, "%s", value);
		}
	}
	GxOptions_Destory(options);
	return noopt_url;
}

static void stream_get_option_params(GxStream *stream, GxOptions *options)
{
	char *result = NULL;

	result = GxOptions_Get_By_Id(options, " -H", OPTIONS_AUDIO_DISABLE);
	if (result && (atoi(result) == 1))
		stream->audio_disable = atoi(result);/*audio disable(demux+acodec_disable).*/
	else if (result && (atoi(result) == 2))
		stream->audio_disable = atoi(result);/*audio codec disable.*/
	else
		stream->audio_disable = 0;

	result = GxOptions_Get_By_Id(options, " -H", OPTIONS_VIDEO_DISABLE);
	if (result && (atoi(result) == 1))
		stream->video_disable = atoi(result);/*video disable(demux+acodec_disable).*/
	else if (result && (atoi(result) == 2))
		stream->video_disable = atoi(result);/*video codec disable.*/
	else
		stream->video_disable = 0;

	result = GxOptions_Get_By_Id(options, " -H", OPTIONS_SUBTITLE_DISABLE);
	if (result && (atoi(result) == 1))
		stream->subtitle_disable = atoi(result);/*subtile disable(demux+acodec_disable).*/
	else
		stream->subtitle_disable = 0;

	return;
}

int GxStream_ConfigOptions(GxStreamOptionType type, const char *strings)
{
	if (public_stream_options == NULL)
		public_stream_options = GxOptions_Create();

	if (type == GX_STREAM_OPTION_DRM)
		GxOptions_Set(public_stream_options, " -H", "enc_drm:", "enable");
	else if (type == GX_STREAM_OPTION_USER_AGENT)
		GxOptions_Set(public_stream_options, " -H", "User-Agent:", (char*)strings);
	else if(type == GX_STREAM_OPTION_CONECTTION)
		GxOptions_Set(public_stream_options, " -H", "Connection:", (char*)strings);

	return 0;
}

int GxStream_ClearOptions(void)
{
	if (public_stream_options != NULL)
		GxOptions_Destory(public_stream_options);

	public_stream_options = NULL;

	return 0;
}

int GxStream_RegisterCallback(GxStreamOptionCallback cb)
{
	public_stream_callback = cb;

	return 0;
}

int GxStream_MpegtsRegisterCallback(GxStreamMpegtsCallback cb)
{
	public_stream_mpegts_callback = cb;
	return 0;
}

int GxStream_MpegtsUnregisterCallback(void)
{
	public_stream_mpegts_callback = NULL;
	return 0;
}

int GxExternalNetTime_RegisterCallback(GxExternalNetTimeCallback cb)
{
    external_net_time_callback = cb;
    return 0;
}

int GxExternalNetTime_UnregisterCallback(void)
{
    external_net_time_callback = NULL;
    return 0;
}

void GxStreamRegister(GxStreamClass* cls)
{
	int i;
	if (stream_class_num == 0)
		memset(&gx_stream_classes, 0, sizeof(gx_stream_classes));

	if (stream_class_num >= GX_STREAM_CLASS_MAX)
		return;

	for (i=0; i<stream_class_num; i++) {
		if (gx_stream_classes[i] == cls)
			return;
	}

	gx_stream_classes[stream_class_num++] = cls;

	if (cls->_inherit._inherit.init)
		cls->_inherit._inherit.init();
}

static void* streambase_create(GxObject * ob)
{
	GxStream* s = GXSTREAM(ob);

	if (s == NULL)
		return NULL;

	s->fd          = -2;
	s->file_format = GX_STREAMTYPE_UNKNOWN;
	s->buf_pos     = s->buf_len = 0;
	s->start_pos   = s->end_pos = 0;
	s->priv        = NULL;
	s->url         = NULL;
	s->original_url = NULL;
	s->pos          = 0;
	s->eof          = 0;

	return s;
}

static void streambase_release(GxObject*  ob)
{
	GxStream* sl = GXSTREAM(ob);
	if (sl && sl->url) {
		av_free(sl->url);
		sl->url = NULL;
	}

	if (sl && sl->original_url) {
		av_free(sl->original_url);
		sl->original_url = NULL;
	}

	if (sl && sl->redirect_url) {
		av_free(sl->redirect_url);
		sl->redirect_url = NULL;
	}
}

static int streambase_run(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

static int streambase_pause(GxMediaFilter *filter)
{

	return GX_PLAYER_OK;
}

static int streambase_resume(GxMediaFilter *filter)
{

	return GX_PLAYER_OK;
}


static int streambase_config(GxMediaFilter *filter)
{

	return GX_PLAYER_OK;
}

static int streambase_stop(GxMediaFilter *filter)
{
	return GX_PLAYER_OK;
}

uint64_t GxStream_GetTimeMs(void)
{
	struct timeval tv;
	//  float s;
	gettimeofday(&tv, NULL);
	//  s=tv.tv_usec;s*=0.000001;s+=tv.tv_sec;
	return (uint64_t)(tv.tv_sec*  1000 + tv.tv_usec / 1000);
}

void GxStream_Print(void* p)
{
	int i;
	GxStreamClass* sl = GXSTREAMCLASS(p);
	GxClass* obj = GXCLASS(p);

	gxlogd("CLASS: %s\n", obj->name);
	gxlogf("._inherit = {\n"
			"\t.name    = %s,\n"
			"\t.parent  = %p,\n"
			"\t.size    = %u,\n"
			"},\n\n", obj->name, obj->parent, obj->size );

	gxlogf(".protocols = {\"");
	for (i = 0; sl->protocols[i]; i++)
		gxlogf("%s\", ", sl->protocols[i]);
	gxlogf("NULL},\n\n");

#ifdef ENABLE_AUTHOR
	gxlogf(".author = {\n"
			"\t.info    = \"%s\",\n"
			"\t.name    = \"%s\",\n"
			"\t.author  = \"%s\",\n"
			"\t.comment = \"%s\",\n},\n\n", sl->author.info, sl->author.name, sl->author.author, sl->author.comment);
#endif

	gxlogf(".open    = %p,\n"
			".read    = %p,\n"
			".write   = %p,\n"
			".seek    = %p,\n"
			".control = %p,\n" ".close   = %p\n", sl->open, sl->read, sl->write, sl->seek, sl->control, sl->close);
}

GxStream* GxStream_NewMemory(unsigned char* data,int len)
{
	GxStream* s;

	if(len < 0)
		return NULL;
	s=av_malloc(sizeof(GxStream)+len);
	memset(s,0,sizeof(GxStream));
	s->fd=-1;
	s->file_format=GX_STREAMTYPE_MEMORY;
	s->buf_pos=0; s->buf_len=len;
	s->start_pos=0; s->end_pos=len;
	s->pos = 0; s->eof = 0;
	s->pos=len;
	memcpy(s->buffer,data,len);
	return s;
}

GxStream* GxStream_Probe(GxStream* stream)
{
	int i = 0, stream_buffer_size = 0, error_status = PLAYER_ERROR_NO_ERROR;
	void  *priv = NULL;
	off_t recv_size = 0;
	off_t size = 0;
	unsigned char *buffer = NULL;
	GxStreamClass* sl = GetGxStreamClassFromObject(stream);

	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);

	if (sl == NULL)
		goto probe_failure;

	if (sl->read == NULL) {
		goto probe_success;
	} else {
		if (!(sl->flags & GX_STREAM_NOT_CACHE)) {
			buffer = GxCore_PageMalloc(stream_buffer_size, 10);
			if (buffer == NULL)
				goto probe_failure;
		}
	}

	if (stream->ispvr || stream->isdvr)
		goto probe_success;

	if (!(sl->flags & GX_STREAM_NEED_PROBE))
		goto probe_success;

	GxStream_Control(stream, GX_STREAM_CTRL_GET_SIZE, &size);
	size = ((size < 0) || (size > stream_buffer_size)) ? stream_buffer_size : size;
	if (size <= 0)
		goto probe_success;

	for (i = 0; gx_stream_classes[i]; i++) {
		GxStreamClass* stream_class = GXSTREAMCLASS(gx_stream_classes[i]);
		if (stream_class->probe) {
			while (recv_size < size) {
				int recv_z = 0;
				if(stream->interrupt_cbk && stream->interrupt_cbk())
					goto probe_failure;
				recv_z = sl->read(stream, buffer + recv_size, size - recv_size);
				if (recv_z < 0)
					break;
				recv_size += recv_z;
			}

			if (stream_class->probe(stream, buffer, recv_size, &priv) == GX_PLAYER_OK) {
				GxStreamParam param;
				param.mode     = stream->mode;
				param.prog     = stream->prog_now;
				param.flags    = stream->flags;
				if (stream && stream->streaming_ctrl && stream->streaming_ctrl->cookies) {
					GxOptions_Set(stream->options, " -H", "Set-Cookie:", stream->streaming_ctrl->cookies);
				}
				if (stream->redirect_url) {
					GxOptions_Set(stream->options, " -H", "location:", stream->redirect_url);
				}
				param.options  = stream->options;
				param.protocol = (char*)stream_class->protocols[0];
				param.priv     = priv;
				recv_size      = 0;

				GxStream *s = GxStream_OpenByParam((const char*)stream->url, &param);
				if (s) {
					s->no_restart_stream = 0;
					GxStream_Close(stream);
					stream = s;
					break;
				} else
					goto probe_failure;
			} else {
				if (recv_size <= 0)//fix:fix http response return:Content-Length:0,read fail.
					goto probe_failure;
			}
		}
	}

probe_success:
	stream->pos     += (unsigned int)recv_size;
	stream->buf_pos += (unsigned int)recv_size;
	stream->buf_len  = (unsigned int)recv_size;
	stream->buffer   = buffer;
	return stream;

probe_failure:
	if (buffer)
		GxCore_PageFree(buffer);
	GxStream_Close(stream);
	GxPlayer_SystemGet(PSYS_PLAY_ERROR_STATUS, &error_status);
	if (error_status <= 0) {
		error_status = PLAYER_ERROR_URL_UNSUPPORT;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}
	return NULL;
}

int GxStream_URLISSupported(const char* url)
{
	int i, j, ret = 0;
	char protocol[32] = {0};
	char *noopt_url = NULL;
	GxStream* s = NULL;
	GxStreamClass* stream_class = NULL;
	GxOptions *options = GxOptions_Create();

	if (!(noopt_url = stream_parse_url(url, protocol, sizeof(protocol)/sizeof(char), options))) {
		GxOptions_Destory(options);
		return 0;
	}

	for (i = 0; gx_stream_classes[i]; i++) {
		stream_class = GXSTREAMCLASS(gx_stream_classes[i]);
		if (stream_class->protocols == NULL) {
			GxFatalError("Stream type has protocols = NULL, it's a bug.\n");
			continue;
		}

		for (j = 0; stream_class->protocols[j]; j++) {
			if (!strcasecmp(stream_class->protocols[j], protocol)) {
				ret = 1;
			}
		}
	}

	if (noopt_url)
		av_free(noopt_url);
	GxOptions_Destory(options);
	return ret;
}

GxStream *GxStream_Open(const char* url, int32_t mode, int prog)
{
	int i, j, error_status = PLAYER_ERROR_NO_ERROR;
	char protocol[32] = {0};
	char *noopt_url = NULL;
	GxStream* s = NULL;
	GxStreamClass* stream_class = NULL;
	GxOptions *options = GxOptions_Create();

	GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	if (!(noopt_url = stream_parse_url(url, protocol, sizeof(protocol)/sizeof(char), options))) {
		GxOptions_Destory(options);
		GxPlayer_SystemGet(PSYS_PLAY_ERROR_STATUS, &error_status);
		if (error_status <= 0) {
			error_status = PLAYER_ERROR_URL_UNSUPPORT;
			GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
		}
		return NULL;
	}

	for (i = 0; gx_stream_classes[i]; i++) {
		stream_class = GXSTREAMCLASS(gx_stream_classes[i]);
		if (stream_class->protocols == NULL) {
			GxFatalError("Stream type has protocols = NULL, it's a bug.\n");
			continue;
		}

		for (j = 0; stream_class->protocols[j]; j++) {
			if (!strcasecmp(stream_class->protocols[j], protocol)) {
				s = GxObjectNew(NULL, stream_class);

				s->flags         = 0;
				s->mode          = mode;
				s->buf_pos       = s->buf_len = 0;
				s->start_pos     = s->end_pos = 0;
				s->pos           = 0;
				s->eof           = 0;
				s->prog_now      = prog;
				s->prog_max      = 0;
				s->priv          = NULL;
				s->url           = noopt_url;
				s->original_url  = av_strdup(url);
				s->options       = options;
				s->redirect_url  = NULL;
				s->read_timeoutms= 500;
				s->demuxer_type  = GX_DEMUXER_TYPE_UNKNOWN;
				s->interrupt_cbk = NULL;
#ifdef CONFIG_STREAM_CACHE
				s->cache_pid     = 0;
#endif
				s->cur_seq_no    = -1;
				s->nobuffer      = 0;
				s->next          = NULL;
				s->hls_list      = NULL;
				s->hls_thread    = 0;
				s->debug_r_cost_time = 0;
				s->debug_r_data_size = 0;
				s->debug_w_cost_time = 0;
				s->debug_w_data_size = 0;
				s->is_hls_flg        = 0;
				memset(&s->err,  0, sizeof(GxStreamError));
				memset(&s->info, 0, sizeof(GxStreamInfo));
				stream_get_option_params(s, options);

				if (stream_class->open(s, mode) != GX_PLAYER_OK) {
					s->url = NULL;
					GxObjectDestroy(s);
				} else {
					if (stream_class->read)
						return GxStream_Probe(s);
					return s;
				}
			}
		}
	}
	if (noopt_url)
		av_free(noopt_url);
	GxOptions_Destory(options);
	GxPlayer_SystemGet(PSYS_PLAY_ERROR_STATUS, &error_status);
	if (error_status <= 0) {
		error_status = PLAYER_ERROR_URL_UNSUPPORT;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}
	return NULL;
}

GxStream* GxStream_OpenByParam(const char* url, GxStreamParam *param)
{
	int i, j;
	char *noopt_url = NULL;
	char protocol[32] = {0};
	GxStream *s = NULL;
	GxStreamClass *stream_class = NULL;
	GxOptions *options = GxOptions_Create();

	GxOptions_Copy((GxOptions*)param->options, options);

	if (param->protocol == NULL) {
		if (!(noopt_url = stream_parse_url(url, (char*)protocol, 32, options))) {
			GxOptions_Destory(options);
			return NULL;
		}
	} else {
		noopt_url = av_strdup(url);
		snprintf (protocol, sizeof(protocol)/sizeof(char)-1, "%s", param->protocol);
	}

	for (i = 0; gx_stream_classes[i]; i++) {
		stream_class = GXSTREAMCLASS(gx_stream_classes[i]);
		if (stream_class->protocols == NULL) {
			GxFatalError("Stream type has protocols = NULL, it's a bug.\n");
			continue;
		}

		for (j = 0; stream_class->protocols[j]; j++) {
			if (!strcasecmp(stream_class->protocols[j], protocol)) {
				s = GxObjectNew(NULL, stream_class);

				s->flags          = param->flags;
				s->mode           = param->mode;
				s->eof            = 0;
				s->prog_now       = param->prog;
				s->url            = noopt_url;
				s->original_url   = av_strdup(url);
				s->read_timeoutms = 500;
				s->options        = options;
				s->priv           = param->priv;
				s->cur_seq_no     = -1;
				s->next           = NULL;
				s->hls_list       = NULL;
				s->hls_thread     = 0;
				s->debug_r_cost_time = 0;
				s->debug_r_data_size = 0;
				s->debug_w_cost_time = 0;
				s->debug_w_data_size = 0;
				memset(&s->err,  0, sizeof(GxStreamError));
				memset(&s->info, 0, sizeof(GxStreamInfo));

				if (stream_class->open(s, s->mode) != GX_PLAYER_OK) {
					s->url = NULL;
					GxObjectDestroy(s);
				} else {
					return s;
				}
			}
		}
	}
	if (noopt_url)
		av_free(noopt_url);
	GxOptions_Destory(options);
	return NULL;
}

size_t GxStream_FillBuffer(GxStream *s)
{
	int len, rlen, stream_buffer_size;
	uint8_t* rbuffer;
	GxStreamClass* sl = GetGxStreamClassFromObject(s);
#ifdef DEBUG_STREAM
	unsigned int debug_stream_speed = 0;
	unsigned int debug_cost_ms = 0;

	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_SPEED, &debug_stream_speed);
	if (debug_stream_speed)
		debug_cost_ms = av_get_tick_ms(NULL, 0);
#endif
	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);
	if (s->eof) {
		s->buf_pos = s->buf_len = 0;
		return 0;
	}

	s->buf_pos %= stream_buffer_size;
	s->buf_len %= stream_buffer_size;
	rlen = stream_buffer_size - s->buf_len;
	rbuffer = s->buffer + s->buf_len;
	len = sl->read ? sl->read(s, rbuffer, rlen) : 0;

	if (len < 0) {
		s->eof = 1;
		s->buf_pos = s->buf_len = 0;
		return 0;
	}
	s->buf_len += len;
	s->pos     += len;

#ifdef DEBUG_STREAM
	if (debug_stream_speed) {
		debug_cost_ms = (av_get_tick_ms(NULL, 0) - debug_cost_ms);
		s->debug_r_cost_time += debug_cost_ms;
		s->debug_r_data_size += len;
		if (debug_cost_ms > DEBUG_ONCE_STREAM_TIME_MS) {
			gxlogi_raw("%s, r %7.2f KB/s, cost time %d",
					s->url, ((double)len / 1024.0 * 1000) / debug_cost_ms, debug_cost_ms);
		}
		if (s->debug_r_cost_time > DEBUG_STREAM_TIME_MS) {
			gxlogi_raw("%s, r %7.2f KB/s",
					s->url, ((double)s->debug_r_data_size / 1024.0 * 1000) / s->debug_r_cost_time);
			s->debug_r_cost_time = 0;
			s->debug_r_data_size = 0;
		}
	}
#endif

	return len;
}

size_t GxStream_Read(GxStream* s, uint8_t *buffer, size_t size)
{
	GxStreamClass* sl = GetGxStreamClassFromObject(s);
	int len = size;

	if (!buffer || !s)
		return 0;

	if (sl->flags & GX_STREAM_NOT_CACHE) {
		int len = 0;

		if (s->eof)
			return 0;
		len = sl->read ? sl->read(s, buffer, size) : 0;
		if (len < 0)
			s->eof = 1;
		s->pos += len;
		return len;
	} else {
		while (len > 0) {
			int x = s->buf_len - s->buf_pos;

			if (x > 0) {
				if (x > len)
					x = len;

				memcpy(buffer, &s->buffer[s->buf_pos], x);
				s->buf_pos += x;
			}
			else {
				if (!GxStream_CacheFillBuffer(s))
					return size - len;
				continue;
			}
			buffer += x;
			len -= x;
		}
	}

	return size - len;
}

size_t GxStream_Write(GxStream *s, uint8_t * buffer, size_t size)
{
	int rd = -1;
	GxStreamClass* sl = GetGxStreamClassFromObject(s);
#ifdef DEBUG_STREAM
	unsigned int debug_stream_speed = 0;
	unsigned int debug_cost_ms = 0;

	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_SPEED, &debug_stream_speed);
	if (debug_stream_speed)
		debug_cost_ms = av_get_tick_ms(NULL, 0);
#endif

	if (s->fd == -1)
		return -1;

	if(sl && sl->write)
		rd = sl->write(s, buffer, size);
	if (rd < 0)
		return -1;
	s->pos += rd;

#ifdef DEBUG_STREAM
	if (debug_stream_speed) {
		debug_cost_ms = (av_get_tick_ms(NULL, 0) - debug_cost_ms);
		s->debug_w_cost_time += debug_cost_ms;
		s->debug_w_data_size += rd;
		if (debug_cost_ms > 0) {
			gxlogi_raw("%s, w %7.2f KB/s, cost time %d",
					s->url, ((double)rd / 1024.0 * 1000) / debug_cost_ms, debug_cost_ms);
		}
		if (s->debug_w_cost_time > DEBUG_STREAM_TIME_MS) {
			gxlogi_raw("%s, w %7.2f KB/s",
					s->url, ((double)s->debug_w_data_size / 1024.0 * 1000) / s->debug_w_cost_time);
			s->debug_w_cost_time = 0;
			s->debug_w_data_size = 0;
		}
	}
#endif

	return rd;
}

int GxStream_SeekLong(GxStream *s, off_t pos)
{
	off_t newpos = 0, stream_buffer_size = 0;
	GxStreamClass* sl = GetGxStreamClassFromObject(s);

	s->buf_pos = s->buf_len = 0;
	if (s->mode == GX_STREAM_WRITE) {
		if (!sl->seek || sl->seek(s, pos) == GX_PLAYER_OK)
			return GX_PLAYER_OK;
		return GX_PLAYER_ERROR;
	}

	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);

	if (s->sector_size)
		newpos = (pos / s->sector_size)*  s->sector_size;
	else
		newpos = pos & (~((off_t) stream_buffer_size - 1));
	pos -= newpos;

	if (newpos == 0 || newpos != s->pos) {
		if (!sl->seek || sl->seek(s, newpos)!= GX_PLAYER_OK) {
			gx_msg(MSGT_STREAM, MSGL_ERR, "Seek failed\n");
			return GX_PLAYER_ERROR;
		}
	}

	while (GxStream_CacheFillBuffer(s) > 0 && pos >= 0) {
		if (pos <= s->buf_len) {
			s->buf_pos = pos;
			return GX_PLAYER_OK;
		}
		//pos -= s->buf_len;
	}

	return GX_PLAYER_ERROR;
}

int GxStream_Seek(GxStream *s, off_t pos)
{
	GxStreamClass* sl = GetGxStreamClassFromObject(s);

	if (sl->flags & GX_STREAM_NOT_CACHE) {
		if (sl->seek) {
			if (sl->seek(s, pos) == GX_PLAYER_OK) {
				s->pos = pos;
				return GX_PLAYER_OK;
			}
		}
		return GX_PLAYER_ERROR;
	}

	if (pos <= s->pos) {
		off_t x = pos - (s->pos - s->buf_len);
		if (x >= 0) {
			s->buf_pos = x;
			return GX_PLAYER_OK;
		}
	}

#ifdef CONFIG_STREAM_CACHE
	return GxStream_CacheSeekLong(s, pos, -1);
#else
	return GX_PLAYER_OK;
#endif
}

int GxStream_SeekTime(GxStream *stream, off_t time)
{
#ifdef CONFIG_STREAM_CACHE
	return GxStream_CacheSeekLong(stream, 0, time);
#else
	return GX_PLAYER_OK;
#endif

}

void GxStream_Reset(GxStream *s)
{
	GxStreamClass* sl = GetGxStreamClassFromObject(s);
	if (s->eof) {
		s->pos = 0;
		s->eof = 0;
	}
#ifdef CONFIG_STREAM_CACHE
	if (s->cache_pid){
		GxStream_CacheControl(s, GX_STREAM_CTRL_RESET, NULL);
		return;
	}
#endif
	if (sl && sl->control)
		sl->control(s, GX_STREAM_CTRL_RESET, NULL);
}

int GxStream_Control(GxStream *s, int cmd, void *args)
{
	GxStreamClass* sl = GetGxStreamClassFromObject(s);

#ifdef CONFIG_STREAM_CACHE
	if (s->cache_pid)
		return GxStream_CacheControl(s, cmd, args);
#endif

	switch(cmd){
	case GX_STREAM_CTRL_SET_READ_TIMEOUT:
		s->read_timeoutms = *(int*)args;
		return GX_PLAYER_OK;

	default:
		if (sl && sl->control)
			return sl->control(s, cmd, args);
		break;
	}

	return GX_PLAYER_ERROR;
}

void GxStream_Close(GxStream *stream)
{
	GxStreamClass* sl;

	if (stream == NULL)
		return;
#ifdef CONFIG_STREAM_CACHE
	if(stream->file_format == GX_STREAMTYPE_STREAM)
		GxStream_CacheDisable(stream);
#endif
	sl = GetGxStreamClassFromObject(stream);

	if (sl && sl->close)
		sl->close(stream);

	if (stream->buffer) {
		GxCore_PageFree(stream->buffer);
		stream->buffer = NULL;
	}

	GxOptions_Destory(stream->options);

	GxObjectDestroy(stream);
}

char* GxStream_ReadLine(GxStream * s, char *mem, int max)
{
	int len=0, l=0;
	char flags=0, x =0;
	unsigned char* end;
	char* ptr = mem;

	do {
		len = s->buf_len - s->buf_pos;

		if (len <= 0 && (!GxStream_CacheFillBuffer(s) || (len = s->buf_len - s->buf_pos) <= 0))
			break;

		if(s->encode == SUB_ENC_UCS)
		{
			unsigned char* over = s->buffer + s->buf_len;
			end = s->buffer + s->buf_pos;
			while(end <= over)
			{
				if ((x == 0) && (end[0] == 0x0D)) x = 1;
				else if ((x == 1) && (end[0] == 0x00)) x = 2;
				else if ((x == 2) && (end[0] == 0x0A)) x = 3;
				else if ((x == 3) && (end[0] == 0x00)) x = 4;
				else x = 0;

				end++;
				if (x == 4) {
					len = end -(s->buffer + s->buf_pos);
					flags = 1;
					break;
				}
			}
		}else{
			end = (unsigned char* )memchr((void *)(s->buffer + s->buf_pos), '\n', len);
			if (end) {
				len = end - (s->buffer + s->buf_pos) + 1;
				flags = 1;
			}
		}

		if (len >= max) {
			memcpy(ptr, s->buffer + s->buf_pos, max);
			ptr += max;
			l   += max;
			s->buf_pos += max;
			max = 0;
			break;
		} else {
			memcpy(ptr, s->buffer + s->buf_pos, len);
			ptr += len;
			l   += len;
			s->buf_pos += len;
			max -= len;
		}
	} while (!flags);

	if (s->eof && ptr == mem)
		return NULL;

	if (max > 0)
		ptr[0] = 0;

	if((s->encode == SUB_ENC_UCS)&&(l > 0))
	{
		int i;
		uint16_t* p=NULL;
		p = (uint16_t*)av_malloc(l);
		if(p == NULL)
			return NULL;
		memset(p,0,l);
		for(i=0; i< l/2; i++)
			p[i]=mem[2*i]|(mem[2*i+1]<<8);
		memset(mem,0,l);
		convert_unicode2utf8(p,l/2,mem,l);
		av_free(p);
	}
	return mem;
}

int GxStream_Skip(GxStream *s, off_t len)
{
	int stream_buffer_size = 0;
	GxStreamClass* stream_class = GetGxStreamClassFromObject(s);
	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);

	if ((len < 0 && (s->flags & GX_STREAM_SEEK_BW))
			|| (len > 2*  stream_buffer_size && (s->flags & GX_STREAM_SEEK_FW))) {
		return GxStream_Seek(s, GxStream_Tell(s) + len);
	}
	while (len > 0) {
		int x = s->buf_len - s->buf_pos;
		if (x == 0) {
			if (!GxStream_CacheFillBuffer(s))
				return GX_PLAYER_ERROR;
			x = s->buf_len - s->buf_pos;
		}
		if (x > len)
			x = len;
		s->buf_pos += x;
		len -= x;
#if 1
		if(stream_class && stream_class->seek && len >10240*1024)
		{
			if(stream_class->seek(s,len+s->pos) == GX_PLAYER_OK)
			{
				//s->pos = s->end_pos;
				//s->eof = 1;
				len = 0;
			}
		}
#endif
	}
	return GX_PLAYER_OK;
}

inline unsigned char GxStream_ReadChar(GxStream *s)
{
	unsigned char c = 0xff;
	GxStreamClass* sl = GetGxStreamClassFromObject(s);

	if (sl->flags & GX_STREAM_NOT_CACHE) {
		if (sl->read)
			sl->read(s, &c, 1);
		return c;
	} else {
		c = (s->buf_pos < s->buf_len) ? s->buffer[s->buf_pos++] :
		(GxStream_CacheFillBuffer(s) ? s->buffer[s->buf_pos++] : -255);
	}
	return c;
}

uint16_t GxStream_ReadWord(GxStream *s)
{
	uint16_t x = GxStream_ReadChar(s);
	uint16_t y = GxStream_ReadChar(s);

	return (x << 8) | y;
}

uint32_t GxStream_ReadDword(GxStream *s)
{
	uint32_t y = GxStream_ReadChar(s);

	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);

	return y;
}

uint16_t GxStream_ReadWord_Le(GxStream *s)
{
	uint16_t x = GxStream_ReadChar(s);
	uint16_t y = GxStream_ReadChar(s);

	return (y << 8) | x;
}

uint32_t GxStream_ReadDword_Le(GxStream *s)
{
	uint32_t y = GxStream_ReadChar(s);
	y |= GxStream_ReadChar(s) << 8;
	y |= GxStream_ReadChar(s) << 16;
	y |= GxStream_ReadChar(s) << 24;

	return y;
}

uint64_t GxStream_ReadQword(GxStream *s)
{
	uint64_t y = GxStream_ReadChar(s);

	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);

	return y;
}

uint64_t GxStream_ReadQword_Le(GxStream *s)
{
	uint64_t y = GxStream_ReadDword_Le(s);

	y |= (uint64_t) GxStream_ReadDword_Le(s) << 32;

	return y;
}

uint32_t GxStream_Read_Int24(GxStream *s)
{
	unsigned int y = GxStream_ReadChar(s);

	y = (y << 8) | GxStream_ReadChar(s);
	y = (y << 8) | GxStream_ReadChar(s);

	return y;
}

inline int GxStream_Eof(GxStream *s)
{
	return s->eof;
}

inline off_t GxStream_Tell(GxStream *s)
{
	return s->pos + s->buf_pos - s->buf_len;
}

void GxStreamClass_Init(void)
{
	GxClassRegister(&gx_streambase);
	GxClassInitTable((void*)gx_stream_classes);
}

void GxStreamClass_Destroy(void)
{
	int i;

	for (i = 0; gx_stream_classes[i]; i++)
		GxClassUnregister(gx_stream_classes[i]);
	GxClassUnregister(&gx_streambase);
}

int GxStream_RecordEnable(GxStream *stream, int size)
{
	if (stream && stream->outpin == NULL) {
		stream->outpin = GxPin_Create(GX_STREAM_PIN_NAME_OUTPUT, (void *)stream, GX_PINDIR_OUTPUT, size, GX_PINFLAG_SW);
		if (stream->outpin != NULL)
			return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

GxStreamClass gx_streambase = {
	._inherit = {
		._inherit = {
			.name    = "StreamBase",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxStream),
			.create  = streambase_create,
			.release = streambase_release,
		},
		.run    = streambase_run,
		.pause  = streambase_pause,
		.resume = streambase_resume,
		.config = streambase_config,
		.stop   = streambase_stop,
	},

	.protocols = {NULL},
	DEF_AUTHOR("Stream","base","No description","L.F","No comment"),

	.flags   = 0,
	.open    = NULL,
	.read    = NULL,
	.write   = NULL,
	.seek    = NULL,
	.control = NULL,
	.close   = NULL,
};

