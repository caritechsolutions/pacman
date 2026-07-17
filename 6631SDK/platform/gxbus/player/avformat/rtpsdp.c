/*
 * rtpsdp URL protocol
 */
#include "../avutil/avstring.h"
#include "avformat.h"

#define MAX_RTPSDP_SIZE 2048

typedef struct RtpSdp_Context {
    int rtpsdp_len;
    char *uri;
    char *rtpsdp_file;
} RtpSdp_Context;

static int rtpsdp_close(URLContext *h)
{
    RtpSdp_Context  *s = h->priv_data;
    if (s && s->uri) {
        av_free(s->uri);
        av_free(s->rtpsdp_file);
    }
    return 0;
}

void av_replacestr(char *src, char match, char *dec)
{
    int i = 0, k = 0;

    while(src[i] != '\0') {
        if(src[i] == match) {
            dec[k++] = '\n';
        } else {
            dec[k++] = src[i];
        }
        i++;
    }
}

static int rtpsdp_open(URLContext *h, const char *uri, int flags)
{
    int err = 0, rtpsdp_len = 0;
    AVDictionaryEntry *e = NULL;
    RtpSdp_Context  *s = h->priv_data;

    /* handle input */
    if (!*uri || !s)
        err = AVERROR(ENOENT);

    if (!av_strstart(uri, "rtpsdp:", &uri)) {
        return AVERROR(EINVAL);
    }

    if (!(s->uri = av_strdup(uri)))
        return AVERROR(ENOMEM);

    if (!(s->rtpsdp_file = av_mallocz(MAX_RTPSDP_SIZE))) {
        av_free(s->uri);
        return AVERROR(ENOMEM);
    }
    memset (s->rtpsdp_file, 0, MAX_RTPSDP_SIZE);
#if 0 // only use test..
    rtpsdp_len += av_strlcatf(s->rtpsdp_file, MAX_RTPSDP_SIZE,
        "v=0\n"
        "o=- 0 0 IN IP4 127.0.0.1\n"
        "s=No Name\n"
        "c=IN IP4 238.123.46.66\n"
        "t=0 0\n"
        "a=tool:libavformat 58.29.100\n"
        "m=video 8001 RTP/AVP 96\n"
        "a=rtpmap:96 H264/90000\n"
        "a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAKKzZQHgCJ+XAWoCAgKAAAAMAIAAABlHjBjLA,aOvjyyLAAA==; profile-level-id=640028");
#else
    if ((e = av_dict_get(h->url_context_options, "sdp_media_info", NULL, 0))) {
        av_replacestr(e->value, '&', s->rtpsdp_file);
        rtpsdp_len = strlen(s->rtpsdp_file);
    } else {
        av_free(s->uri);
        av_free(s->rtpsdp_file);
        return AVERROR(EINVAL);
    }
#endif
    s->rtpsdp_len = rtpsdp_len;
    return err;
}

static int rtpsdp_read(URLContext *h, unsigned char *buf, int size)
{
    RtpSdp_Context *s = h->priv_data;
    if (s && s->rtpsdp_file) {
        memcpy (buf, s->rtpsdp_file, size);
        av_free(s->rtpsdp_file);
        s->rtpsdp_file = NULL;
    } else {
        return -1;
    }
    return s->rtpsdp_len;
}

URLProtocol gx_rtpsdp_protocol = {
    .name           = "rtpsdp",
    .url_open       = rtpsdp_open,
    .url_read       = rtpsdp_read,
    .url_seek       = NULL,
    .url_close      = rtpsdp_close,
    .priv_data_size = sizeof(RtpSdp_Context),
};
