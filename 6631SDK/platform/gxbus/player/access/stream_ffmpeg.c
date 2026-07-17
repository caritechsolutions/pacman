#include "gx_stream.h"
#include "gx_media.h"
#include "gx_options.h"
#include "avstring.h"

typedef struct {
	GxStream parent;
} GxStreamFFmpeg;

static int ff_support_stream_media_protocol(const char *uri)
{
	int ret = GX_PLAYER_ERROR;
	int index = 0;
	char *ff_sup_protocol[]={"compose:", "concat:", "rtsp://", "rtsps://", "sdp://", "rtp://", \
                                 "srtp://", "udp://", "udplite://", "rtmp://",      \
                                 "rtmpt://", "rtmpts://", "rtmps://", "mmsh://",    \
                                 "mmst://", "data:", ".mpd", ".m3u8", "http://",    \
                                 "https://", "rtpsdp://", "associationlibre:", "rist://"};
	if (!uri) {
		return ret;
	}

	for (index = 0; index < sizeof(ff_sup_protocol)/sizeof(ff_sup_protocol[0]); index++) {
		if (av_strstart(uri, ff_sup_protocol[index], NULL)) {
			ret = GX_PLAYER_OK;
			break;
		} else if ((strstr(uri, ".m3u8") || strstr(uri, ".M3U8") ||strstr(uri, ".mpd") || strstr(uri, ".MPD")) && (!av_strstart(uri, "http://", NULL)) && (!av_strstart(uri, "https://", NULL))) {
#ifdef CONFIG_STREAM_FFFILE
			ret = GX_PLAYER_OK;
			break;
#endif
		}
	}
	return ret;
}

static int ff_stream_media_probe(GxStream *stream, uint8_t* buffer, size_t size, void **priv)
{
#ifdef CONFIG_STREAM_FFFILE
	/*support hls.*/
    if (av_stristr((const char *)buffer, "#EXTM3U")                                                  ||
		av_stristr((const char *)buffer, "#EXT-X-STREAM-INF:")                                       ||
        av_stristr((const char *)buffer, "#EXT-X-TARGETDURATION:")                                   ||
        (av_stristr((const char *)buffer, "#EXTM3U") &&  strstr((const char *)buffer, "#EXTINF:"))   ||
        av_stristr((const char *)buffer, "#EXT-X-MEDIA-SEQUENCE:"))
        return GX_PLAYER_OK;
#endif
#ifdef CONFIG_STREAM_MPEGDASH
	/*support dash.*/
    if (av_stristr((char *)buffer, "<MPD")                              ||
		av_stristr((char *)buffer, "dash:profile:isoff-on-demand:2011") ||
        av_stristr((char *)buffer, "dash:profile:isoff-live:2011")      ||
        av_stristr((char *)buffer, "dash:profile:isoff-live:2012")      ||
        av_stristr((char *)buffer, "dash:profile:isoff-main:2011")      ||
        av_stristr((char *)buffer, "dash:profile")                      ||
        av_stristr((char *)buffer, "3GPP:PSS:profile:DASH1")) {
        return GX_PLAYER_OK;
    }
#endif
	return GX_PLAYER_ERROR;
}

static int ff_stream_media_open(GxStream *s, int mode)
{
	GxStreamFFmpeg *stream = (GxStreamFFmpeg*)s;
	if (!stream || (!stream->parent.url))
		return GX_PLAYER_ERROR;

	if (GX_PLAYER_ERROR == ff_support_stream_media_protocol(stream->parent.url))
		return GX_PLAYER_ERROR;

	stream->parent.file_format = GX_STREAMTYPE_DEMUXER;
	stream->parent.flags   = 0;
	stream->parent.end_pos = 0;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_LAVF;
	return GX_PLAYER_OK;
}

static void ff_stream_media_close(GxStream *s)
{
	return;
}

static int ff_stream_media_control(GxStream *s, int cmd, void *arg)
{
	return GX_PLAYER_ERROR;
}

static int ff_stream_media_seek(GxStream *s, off_t newpos)
{
	return GX_PLAYER_OK;
}

#define FF_STREAM_MEDIA_PROTOCOL(flavor, protocols1, protocols2, protocols3, protocols4)        \
GxStreamClass gx_stream_##flavor = {                                                            \
	._inherit = {                                                                               \
		._inherit     = {                                                                       \
			.name    = "GxStreamFFmpeg",                                                        \
			.parent  = &gx_streambase,                                                          \
			.size    = sizeof(GxStreamFFmpeg),                                                  \
		},                                                                                      \
	},                                                                                          \
	.protocols = {protocols1, protocols2, protocols3, protocols4, NULL},                        \
	DEF_AUTHOR("Stream","ff_stream_media","No description","L.F","No comment"),                 \
	.flags   = 0,                       \
	.probe   = ff_stream_media_probe,   \
	.open    = ff_stream_media_open,    \
	.read    = NULL,                    \
	.write   = NULL,                    \
	.seek    = ff_stream_media_seek,    \
	.control = ff_stream_media_control, \
	.close   = ff_stream_media_close,   \
};

#if defined(CONFIG_STREAM_FFUDP) ||  defined(CONFIG_STREAM_FFRTP) ||  defined(CONFIG_STREAM_FFSRTP) ||  defined(CONFIG_STREAM_FFRTSP)
	FF_STREAM_MEDIA_PROTOCOL(ffudp, "udp", "udplite", "", "")
#endif
#ifdef CONFIG_STREAM_FFRTSP
	FF_STREAM_MEDIA_PROTOCOL(ffrtsp, "rtsp", "rtsps", "", "")
#endif
#if defined(CONFIG_STREAM_FFRTP) ||  defined(CONFIG_STREAM_FFRTSP)
	FF_STREAM_MEDIA_PROTOCOL(ffrtp, "rtp", "sdp", "", "")
#endif
#if defined(CONFIG_STREAM_FFSRTP) ||  defined(CONFIG_STREAM_FFRTSP)
	FF_STREAM_MEDIA_PROTOCOL(ffsrtp, "srtp", "", "", "")
#endif
#ifdef CONFIG_STREAM_FFRTMP
	FF_STREAM_MEDIA_PROTOCOL(ffrtmp, "rtmp", "rtmpt", "rtmps", "rtmpts")
#endif
#ifdef CONFIG_STREAM_FFMMS
	FF_STREAM_MEDIA_PROTOCOL(ffmms, "mms", "mmsh", "mmst", "")
#endif
#ifdef CONFIG_STREAM_FFDATA
	FF_STREAM_MEDIA_PROTOCOL(ffdata, "data", "", "", "")
#endif
#ifdef CONFIG_STREAM_FFHTTP
	FF_STREAM_MEDIA_PROTOCOL(ffhttp, "http", "https", "httpproxy", "http_proxy")
#endif
#if defined(CONFIG_STREAM_FFHLS) || defined(CONFIG_STREAM_FFCRYPTO)
	FF_STREAM_MEDIA_PROTOCOL(ffcrypto, "crypto", "", "", "")
#endif
#ifdef CONFIG_STREAM_FFHLS
	FF_STREAM_MEDIA_PROTOCOL(ffhls, "ffhls", "m3u8", "", "")
#endif
#ifdef CONFIG_STREAM_FFHTTP
	FF_STREAM_MEDIA_PROTOCOL(ffdash, "dash", "", "", "")
#endif
#if defined(CONFIG_STREAM_FFRTP) ||  defined(CONFIG_STREAM_FFRTSP)
    FF_STREAM_MEDIA_PROTOCOL(ffrtpsdp, "rtpsdp", "", "", "")
#endif
#ifdef CONFIG_STREAM_FFCOMPOSE
    FF_STREAM_MEDIA_PROTOCOL(ffcompose, "compose", "", "", "")
#endif
#ifdef CONFIG_STREAM_FFCONCAT
    FF_STREAM_MEDIA_PROTOCOL(ffconcat, "concat", "", "", "")
#endif
#ifdef CONFIG_STREAM_FFFILE
    FF_STREAM_MEDIA_PROTOCOL(fffile, "m3u8", "dash", "", "")
#endif
#ifdef CONFIG_STREAM_FFASSOCIATIONLIBRE
    FF_STREAM_MEDIA_PROTOCOL(ffassociationlibre, "associationlibre", "", "", "")
#endif
#ifdef CONFIG_STREAM_FFRIST
    FF_STREAM_MEDIA_PROTOCOL(ffrist, "rist", "", "", "")
#endif

