/*
 * Compose URL protocol
 */
#include "../avutil/avstring.h"
#include "avformat.h"

#define MAX_COMPOSE_SIZE 8192
#define AV_COMPOSE_SEPARATOR "|"
#define COMPOSE_FORMAT_DASH ""

#define COMPOSE_FREE(a) {         \
        if (a) {                  \
        av_free(a);a=NULL;}}

typedef struct Compose_Context {
    int compose_len;
    char *uri;
    char *composedec_file;
} Compose_Context;

static int compose_close(URLContext *h)
{
    Compose_Context  *s = h->priv_data;
    if (s && s->uri) {
        COMPOSE_FREE(s->uri);
        COMPOSE_FREE(s->composedec_file);
    }
    return 0;
}

/*!
 * \brief Appends a string to a buffer, substituting some characters by escape
 * sequences.
 */
static void av_copy_with_escape(const char *in_buf, char *out_buf)
{
    int i = 0, j = 0, plen = 0;

    if (in_buf == NULL)
        return;
    for (i = 0, j = 0; in_buf[i]; i++) {
        switch (in_buf[i]) {
        case '<':
            strncat(out_buf+j, "&lt;", strlen("&lt;"));
            j += strlen("&lt;");
            break;
        case '>':
            strncat(out_buf+j, "&gt;", strlen("&gt;"));
            j += strlen("&gt;");
            break;
        case '&':
            strncat(out_buf+j, "&amp;", strlen("&amp;"));
            j += strlen("&amp;");
            break;
        case '\'':
            strncat(out_buf+j, "&apos;", strlen("&apos;"));
            j += strlen("&apos;");
            break;
        case '\"':
            strncat(out_buf+j, "&quot;", strlen("&quot;"));
            j += strlen("&quot;");
            break;
        default:
            out_buf[j] = in_buf[i];
            j += 1;
            break;
        }
    }
}

static int compose_open(URLContext *h, const char *uri, int flags)
{
    char *node_uri = NULL, *escape_uri = NULL, *temp = NULL;
    int ret = 0, len = 0, i = 0, j = 0, compose_len = 0;
    Compose_Context  *s = h->priv_data;

    escape_uri = av_mallocz(MAX_COMPOSE_SIZE);
    if (!escape_uri) {
        return AVERROR(ENOMEM);
    }

    /* handle input */
    if (!*uri || !s) {
        ret = AVERROR(ENOENT);
        goto fail;
    }

    if (!av_strstart(uri, "compose:", &uri)) {
        COMPOSE_FREE(escape_uri);
        return AVERROR(EINVAL);
    }

    for (i = 0, len = 1; uri[i]; i++) {
        if (uri[i] == *AV_COMPOSE_SEPARATOR) {
            /* integer overflow */
            if (++len >= MAX_COMPOSE_SIZE) {
                COMPOSE_FREE(h->priv_data);
                COMPOSE_FREE(escape_uri);
                return AVERROR(ENAMETOOLONG);
            }
        }
    }

    av_copy_with_escape(uri, escape_uri);
    temp = escape_uri;
    if (!(s->uri = av_strdup(escape_uri))) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    if (!(s->composedec_file = av_mallocz(MAX_COMPOSE_SIZE))) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    memset (s->composedec_file, 0, MAX_COMPOSE_SIZE);

    compose_len += av_strlcatf(s->composedec_file + compose_len, MAX_COMPOSE_SIZE - compose_len,
        "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    compose_len += av_strlcatf(s->composedec_file + compose_len, MAX_COMPOSE_SIZE - compose_len,
        "<MPD xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
        "\t\txmlns=\"urn:mpeg:dash:schema:mpd:2011\"\n"
        "\t\txsi:schemaLocation=\"urn:mpeg:dash:schema:mpd:2011 DASH-MPD.xsd\"\n"
        "\t\ttype=\"%s\"\n"
        "\t\tprofiles=\"urn:mpeg:dash:profile:isoff-live:2011\"\n"
        "\t\tminimumUpdatePeriod=\"PT60S\"\n"
        "\t\tminBufferTime=\"PT12S\"\n"
        "\t\tavailabilityStartTime=\"2016-04-13T20:52:58\" >\n", "static");
    compose_len += av_strlcatf(s->composedec_file + compose_len, MAX_COMPOSE_SIZE - compose_len,
        "%s", "\t<Period start=\"PT0.0S\" id=\"1\">\n");

    for (i = 0, len = 0; *escape_uri; i++) {
        /* parsing uri */
        len = strcspn(escape_uri, AV_COMPOSE_SEPARATOR);
        if ((ret = av_reallocp(&node_uri, len + 1)) < 0) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        av_strlcpy(node_uri, escape_uri, len + 1);
        escape_uri += len + strspn(escape_uri + len, AV_COMPOSE_SEPARATOR);
        compose_len += av_strlcatf(s->composedec_file + compose_len, MAX_COMPOSE_SIZE - compose_len,
            "\t\t<AdaptationSet mimeType=\"%s/mp4\">\n"
            "\t\t\t<ContentComponent contentType=\"%s\" id=\"%d\"/>\n"
            "\t\t\t<SegmentTemplate timescale=\"1000\"\n"
            "\t\t\t\tduration=\"2000\"\n"
            "\t\t\t\tstartNumber=\"%d\"\n"
            "\t\t\t\t\tmedia=\"%s\"/>\n"
            "\t\t\t<Representation id=\"%d\" width=\"1920\" height=\"1080\">\n"
            "\t\t\t\t\t<SubRepresentation contentComponent=\"%d\"/>\n"
            "\t\t\t</Representation>\n"
            "\t\t</AdaptationSet>\n", ((i==0)?"video":"audio"), ((i==0)?"video":"audio"), i, i, node_uri, i, i);
    }
    compose_len += av_strlcatf(s->composedec_file + compose_len, MAX_COMPOSE_SIZE - compose_len,
        "%s%s", "\t</Period>\n", "</MPD>\n");

    COMPOSE_FREE(node_uri);
    s->compose_len = compose_len;
    escape_uri = temp;
    COMPOSE_FREE(escape_uri);
    return ret;
fail:
    escape_uri = temp;
    COMPOSE_FREE(escape_uri);
    COMPOSE_FREE(node_uri);
    COMPOSE_FREE(s->uri);
    COMPOSE_FREE(s->composedec_file);
    return ret;
}

static int compose_read(URLContext *h, unsigned char *buf, int size)
{
    Compose_Context *s = h->priv_data;
    if (s && s->composedec_file) {
        memcpy (buf, s->composedec_file, size);
        COMPOSE_FREE(s->composedec_file);
        s->composedec_file = NULL;
    } else {
        return -1;
    }
    return s->compose_len;
}

URLProtocol gx_compose_protocol = {
    .name           = "compose",
    .url_open       = compose_open,
    .url_read       = compose_read,
    .url_seek       = NULL,
    .url_close      = compose_close,
    .priv_data_size = sizeof(Compose_Context),
};

