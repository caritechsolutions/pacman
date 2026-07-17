/*
 * Concat URL protocol
 */
#include "../avutil/avstring.h"
#include "avformat.h"

#define MAX_CONCAT_SIZE 4096
#define AV_CAT_SEPARATOR "|"
#define CONCAT_HEADER "ffconcat version 1.0"

typedef struct Concat_Context {
    int concat_len;
    char *uri;
    char *concatdec_file;
} Concat_Context;

static int concat_close(URLContext *h)
{
    Concat_Context  *s = h->priv_data;
    if (s && s->uri) {
        av_free(s->uri);
        av_free(s->concatdec_file);
    }
    return 0;
}

static int concat_open(URLContext *h, const char *uri, int flags)
{
    char *node_uri = NULL;
    int err = 0, len = 0, i = 0, concat_len = 0;
    Concat_Context  *s = h->priv_data;

    /* handle input */
    if (!*uri || !s)
        err = AVERROR(ENOENT);

    if (!av_strstart(uri, "concat:", &uri)) {
        return AVERROR(EINVAL);
    }

    for (i = 0, len = 1; uri[i]; i++) {
        if (uri[i] == *AV_CAT_SEPARATOR) {
            /* integer overflow */
            if (++len >= MAX_CONCAT_SIZE) {
                av_freep(&h->priv_data);
                return AVERROR(ENAMETOOLONG);
            }
        }
    }

    if (!(s->uri = av_strdup(uri)))
        return AVERROR(ENOMEM);

    if (!(s->concatdec_file = av_mallocz(MAX_CONCAT_SIZE)))
        return AVERROR(ENOMEM);
    memset (s->concatdec_file, 0, MAX_CONCAT_SIZE);

    concat_len += av_strlcatf(s->concatdec_file + concat_len, MAX_CONCAT_SIZE - concat_len, "%s\r\n", CONCAT_HEADER);
    for (i = 0; *uri; i++) {
        /* parsing uri */
        len = strcspn(uri, AV_CAT_SEPARATOR);
        if ((err = av_reallocp(&node_uri, len + 1)) < 0)
            break;
        av_strlcpy(node_uri, uri, len + 1);
        uri += len + strspn(uri + len, AV_CAT_SEPARATOR);
        concat_len += av_strlcatf(s->concatdec_file + concat_len, MAX_CONCAT_SIZE - concat_len, "test '%s'\r\n", node_uri);
    }

    if (node_uri) {
        av_free(node_uri);
        node_uri = NULL;
    }
    s->concat_len = concat_len;
    return err;
}

static int concat_read(URLContext *h, unsigned char *buf, int size)
{
    Concat_Context *s = h->priv_data;
    if (s && s->concatdec_file) {
        memcpy (buf, s->concatdec_file, size);
        av_free(s->concatdec_file);
        s->concatdec_file = NULL;
    } else {
        return -1;
    }
    return s->concat_len;
}

URLProtocol gx_concat_protocol = {
    .name           = "concat",
    .url_open       = concat_open,
    .url_read       = concat_read,
    .url_seek       = NULL,
    .url_close      = concat_close,
    .priv_data_size = sizeof(Concat_Context),
};

