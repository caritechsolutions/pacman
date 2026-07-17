/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Reliable Internet Streaming Transport protocol
 */

#include "avformat.h"
#include "../avutil/avstring.h"
#include "../avutil/opt.h"
#include "network.h"
#include <sys/time.h>


#include "librist/librist.h"
#include <librist/version.h>

// RIST_MAX_PACKET_SIZE - 28 minimum protocol overhead
#define MAX_PAYLOAD_SIZE (10000-28)
#define FIFO_SIZE_DEFAULT 8192
#define STREAM_LIBRIST_PVR_DEBUG 0

#if STREAM_LIBRIST_PVR_DEBUG
    static char* stream_librist_pvr_file = "/media/sda/stb_librist_downlod.ts"; /*linux*/
#endif

typedef struct RISTContext {
    int profile;
    int buffer_size;
    int packet_size;
    int log_level;
    int encryption;
    int fifo_size;
    int overrun_nonfatal;
    char *secret;
#if STREAM_LIBRIST_PVR_DEBUG
    FILE *download_stream_fp;
#endif
    struct rist_logging_settings logging_settings;
    struct rist_peer_config peer_config;

    struct rist_peer *peer;
    struct rist_ctx *ctx;
} RISTContext;

static int risterr2ret(int err)
{
    switch (err) {
    case RIST_ERR_MALLOC:
        return AVERROR_NOMEM;
    default:
        return AVERROR_EXIT;//AVERROR_EXTERNAL;
    }
}

static int log_cb(void *arg, enum rist_log_level log_level, const char *msg)
{
    int level  = 0;
    return 0;

    switch (log_level) {
    case RIST_LOG_ERROR:    level = AV_LOG_ERROR;   break;
    case RIST_LOG_WARN:     level = AV_LOG_WARNING; break;
    case RIST_LOG_NOTICE:   level = AV_LOG_INFO;    break;
    case RIST_LOG_INFO:     level = AV_LOG_VERBOSE; break;
    case RIST_LOG_DEBUG:    level = AV_LOG_DEBUG;   break;
    case RIST_LOG_DISABLE:  level = AV_LOG_QUIET;   break;
    default: level = AV_LOG_WARNING;
    }

    av_log(arg, level, "%s", msg);

    return 0;
}

static int librist_close(URLContext *h)
{
    RISTContext *s = h->priv_data;
    int ret = 0;
#if STREAM_LIBRIST_PVR_DEBUG
    fclose (s->download_stream_fp);
#endif
    if (s->secret) {
        av_free(s->secret);
        s->secret = NULL;
    }
    s->peer = NULL;

    if (s->ctx)
        ret = rist_destroy(s->ctx);
    s->ctx = NULL;

    return risterr2ret(ret);
}

static int init_librist_parameters(URLContext *h, RISTContext *s)
{
    AVDictionaryEntry *e = NULL;
    if (!s)
        return AVERROR(EINVAL);

    s->profile = RIST_PROFILE_MAIN;
    s->buffer_size = 0;
    s->encryption = 0;
    s->fifo_size = FIFO_SIZE_DEFAULT;
    s->overrun_nonfatal = 0;
    s->secret = NULL;

    if ((e = av_dict_get(h->url_context_options, "fifo_size", NULL, 0))) {
        s->fifo_size = strtol(e->value, NULL, 10);
    }
    if ((e = av_dict_get(h->url_context_options, "rist_secret", NULL, 0))) {
        s->secret = av_strdup(e->value);
    }
    if ((e = av_dict_get(h->url_context_options, "rist_encryption_type", NULL, 0))) {
        s->encryption = strtol(e->value, NULL, 10);
    }
    if ((e = av_dict_get(h->url_context_options, "rist_profile", NULL, 0))) {
        s->profile = strtol(e->value, NULL, 10);
        if (s->profile == 0)
            s->profile = RIST_PROFILE_SIMPLE;
        else if (s->profile == 2)
            s->profile = RIST_PROFILE_ADVANCED;
        else
            s->profile = RIST_PROFILE_MAIN;
    }

    return 0;
}

static int librist_open(URLContext *h, const char *uri, int flags)
{
    RISTContext *s = h->priv_data;
    struct rist_logging_settings *logging_settings = &s->logging_settings;
    struct rist_peer_config *peer_config = &s->peer_config;
    int ret = 0;

    if (init_librist_parameters(h, s) < 0) {
        return AVERROR(EINVAL);
    }

    if ((flags & AVIO_FLAG_READ_WRITE) == AVIO_FLAG_READ_WRITE) {
        return AVERROR(EINVAL);
    }

    s->logging_settings = (struct rist_logging_settings)LOGGING_SETTINGS_INITIALIZER;
    ret = rist_logging_set(&logging_settings, s->log_level, log_cb, h, NULL, NULL);

    if (ret < 0) {
        return risterr2ret(ret);
    }

    if (flags & AVIO_FLAG_WRITE) {
        h->max_packet_size = s->packet_size;
        ret = rist_sender_create(&s->ctx, s->profile, 0, logging_settings);
    }
    if (ret < 0) {
        goto err;
    }

    if (flags & AVIO_FLAG_READ) {
        h->max_packet_size = MAX_PAYLOAD_SIZE;
        ret = rist_receiver_create(&s->ctx, s->profile, logging_settings);
    }
    if (ret < 0) {
        goto err;
    }

    ret = rist_peer_config_defaults_set(peer_config);
    if (ret < 0) {
        goto err;
    }

    ret = rist_parse_address2(uri, &peer_config);
    if (ret < 0) {
        goto err;
    }

    if (flags & AVIO_FLAG_READ) {
        ret = rist_receiver_set_output_fifo_size(s->ctx, s->fifo_size);
        if (ret != 0)
            goto err;
    }

    if (((s->encryption == 128 || s->encryption == 256) && !s->secret) ||
        ((peer_config->key_size == 128 || peer_config->key_size == 256) && !peer_config->secret[0])) {
        gxloge ("secret is mandatory if encryption is enabled\n");
        librist_close(h);
        return AVERROR(EINVAL);
    }

    if (s->secret && peer_config->secret[0] == 0)
        av_strlcpy(peer_config->secret, s->secret, RIST_MAX_STRING_SHORT);

    if (s->secret && (s->encryption == 128 || s->encryption == 256))
        peer_config->key_size = s->encryption;

    if (s->buffer_size) {
        peer_config->recovery_length_min = s->buffer_size;
        peer_config->recovery_length_max = s->buffer_size;
    }

    ret = rist_peer_create(s->ctx, &s->peer, &s->peer_config);
    if (ret < 0) {
        goto err;
    }

    ret = rist_start(s->ctx);
    if (ret < 0) {
        goto err;
    }
#if STREAM_LIBRIST_PVR_DEBUG
    s->download_stream_fp = fopen(stream_librist_pvr_file,"wb+");
#endif
    return 0;
err:
    librist_close(h);
    return risterr2ret(ret);
}

static int librist_read(URLContext *h, uint8_t *buf, int size)
{
    RISTContext *s = h->priv_data;
    int ret;

    struct rist_data_block *data_block;
    ret = rist_receiver_data_read2(s->ctx, &data_block, POLLING_TIME);

    if (ret < 0)
        return risterr2ret(ret);

    if (ret == 0)
        return AVERROR(EAGAIN);

    if (data_block->payload_len > MAX_PAYLOAD_SIZE) {
        rist_receiver_data_block_free2(&data_block);
        return AVERROR_EXIT;
    }

    if (data_block->flags & RIST_DATA_FLAGS_OVERFLOW) {
        if (!s->overrun_nonfatal) {
            gxloge ("Fifo buffer overrun. To avoid, increase fifo_size option.\n");
            size = AVERROR(EIO);
            goto out_free;
        }
    }

    size = data_block->payload_len;
    memcpy(buf, data_block->payload, size);
#if STREAM_LIBRIST_PVR_DEBUG
    fwrite(buf, size, 1, s->download_stream_fp);
#endif
out_free:
    rist_receiver_data_block_free2(&data_block);
    return size;
}

static int librist_write(URLContext *h, uint8_t *buf, int size)
{
    RISTContext *s = h->priv_data;
    struct rist_data_block data_block = { 0 };
    int ret;

    data_block.ts_ntp = 0;
    data_block.payload = buf;
    data_block.payload_len = size;

    ret = rist_sender_data_write(s->ctx, &data_block);
    if (ret < 0)
        return risterr2ret(ret);

    return ret;
}

URLProtocol gx_librist_protocol = {
    .name                = "rist",
    .url_open            = librist_open,
    .url_read            = librist_read,
    .url_write           = librist_write,
    .url_close           = librist_close,
    .priv_data_size      = sizeof(RISTContext),
};

