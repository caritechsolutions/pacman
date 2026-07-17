/*
 * Raw FLAC picture parser
 * Copyright (c) 2001 Fabrice Bellard
 *
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

#include "avformat.h"
#include "flac_picture.h"
#include "internal.h"

typedef struct CodecMime{
    char str[32];
    enum CodecID id;
}CodecMime;
#define PNGSIG 0x89504e470d0a1a0a

#define AV_RB64(x)                                      \
    (((uint64_t)((const uint8_t*)(x))[0] << 56) |       \
     ((uint64_t)((const uint8_t*)(x))[1] << 48) |       \
     ((uint64_t)((const uint8_t*)(x))[2] << 40) |       \
     ((uint64_t)((const uint8_t*)(x))[3] << 32) |       \
     ((uint64_t)((const uint8_t*)(x))[4] << 24) |       \
     ((uint64_t)((const uint8_t*)(x))[5] << 16) |       \
     ((uint64_t)((const uint8_t*)(x))[6] <<  8) |       \
      (uint64_t)((const uint8_t*)(x))[7])

typedef struct GetByteContext {
    uint8_t *buffer, *buffer_end, *buffer_start;
} GetByteContext;

typedef struct AVBuffer {
    uint8_t *data; /**< data described by this buffer */
    size_t size; /**< size of data in bytes */

    /**
     *  number of existing AVBufferRef instances referring to this buffer
     */
    unsigned int refcount;

    /**
     * a callback for freeing the data
     */
    void (*free)(void *opaque, uint8_t *data);

    /**
     * an opaque pointer, to be used by the freeing callback
     */
    void *opaque;

    /**
     * A combination of AV_BUFFER_FLAG_*
     */
    int flags;

    /**
     * A combination of BUFFER_FLAG_*
     */
    int flags_internal;
}AVBuffer;

#define atomic_init(obj, value) \
do {                            \
    *(obj) = (value);           \
} while(0)

#define AV_RB32(x)  ((((uint8_t*)(x))[0] << 24) | \
                     (((uint8_t*)(x))[1] << 16) | \
                     (((uint8_t*)(x))[2] <<  8) | \
                      ((uint8_t*)(x))[3])

#define AV_INPUT_BUFFER_PADDING_SIZE 64
#define MAX_TRUNC_PICTURE_SIZE (500 * 1024 * 1024)
CodecMime flac_id3v2_mime_tags[] = {
    { "image/gif",  AV_CODEC_ID_GIF   },
    { "image/jpeg", AV_CODEC_ID_MJPEG },
    { "image/jpg",  AV_CODEC_ID_MJPEG },
    { "image/png",  AV_CODEC_ID_PNG   },
    { "image/tiff", AV_CODEC_ID_TIFF  },
    { "image/bmp",  AV_CODEC_ID_BMP   },
    { "JPG",        AV_CODEC_ID_MJPEG }, /* ID3v2.2  */
    { "PNG",        AV_CODEC_ID_PNG   }, /* ID3v2.2  */
    { "",           AV_CODEC_ID_NONE  },
};

char *flac_id3v2_picture_types[21] = {
    "Other",
    "32x32 pixels 'file icon'",
    "Other file icon",
    "Cover (front)",
    "Cover (back)",
    "Leaflet page",
    "Media (e.g. label side of CD)",
    "Lead artist/lead performer/soloist",
    "Artist/performer",
    "Conductor",
    "Band/Orchestra",
    "Composer",
    "Lyricist/text writer",
    "Recording Location",
    "During recording",
    "During performance",
    "Movie/video screen capture",
    "A bright coloured fish",
    "Illustration",
    "Band/artist logotype",
    "Publisher/Studio logotype",
};

static void bytestream2_init(GetByteContext *g,
                                              uint8_t *buf,
                                              int buf_size)
{
    av_assert0(buf_size >= 0);
    g->buffer       = buf;
    g->buffer_start = buf;
    g->buffer_end   = buf + buf_size;
}

static unsigned int bytestream_get_be32u(uint8_t **b)
{
    (*b) += 4;
    return AV_RB32(*b - 4);
}

static unsigned int bytestream2_get_be32u(GetByteContext *g)
{
    return bytestream_get_be32u(&g->buffer);
}

static unsigned int bytestream2_get_bytes_left(GetByteContext *g)
{
    return g->buffer_end - g->buffer;
}

static unsigned int bytestream2_get_bufferu(GetByteContext *g,
                                                             uint8_t *dst,
                                                             unsigned int size)
{
    memcpy(dst, g->buffer, size);
    g->buffer += size;
    return size;
}

static void bytestream2_skipu(GetByteContext *g,
                                               unsigned int size)
{
    g->buffer += size;
}

static int bytestream2_tell(GetByteContext *g)
{
    return (int)(g->buffer - g->buffer_start);
}

extern void free_stream(AVFormatContext *s, AVStream **pst);
void ff_remove_stream(AVFormatContext *s, AVStream *st)
{
    if (s->nb_streams < 0)
        return ;
    if (st != s->streams[ s->nb_streams - 1 ])
        return;
    free_stream(s, &(s->streams[--s->nb_streams]));
}

int ff_add_attached_pic(AVFormatContext *s, AVStream *st0, AVIOContext *pb,
                        uint8_t *buf, int size)
{
    AVStream *st = st0;
    AVPacket *pkt;
    int ret;

    if (!st && !(st = avformat_new_stream(s, NULL)))
        return AVERROR(ENOMEM);
    pkt = &st->attached_pic;
    if (buf) {
        av_assert1(*buf);
        av_free_packet(pkt);
        pkt->data = buf;
        pkt->size = size - AV_INPUT_BUFFER_PADDING_SIZE;
    } else {
        ret = av_get_packet(pb, pkt, size);
        if (ret < 0)
            goto fail;
    }
    st->disposition         |= AV_DISPOSITION_ATTACHED_PIC;
    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;

    pkt->stream_index = st->index;
    pkt->flags       |= AV_PKT_FLAG_KEY;

    return 0;
fail:
    if (!st0)
        ff_remove_stream(s, st);
    return ret;
}

int ff_flac_parse_picture(AVFormatContext *s, uint8_t **bufp, int buf_size,
                          int truncate_workaround)
{
    CodecMime *mime = flac_id3v2_mime_tags;
    enum AVCodecID id = AV_CODEC_ID_NONE;
    uint8_t *data = NULL;
    uint8_t mimetype[64], *buf = *bufp;
    uint8_t *desc = NULL;
    GetByteContext g;
    AVStream *st;
    int width, height, ret = 0, size = 0;
    unsigned int type;
    uint32_t len, left, trunclen = 0;

    if (buf_size < 34) {
        av_log(s, AV_LOG_ERROR, "Attached picture metadata block too short\n");
        if (s->error_recognition & AV_EF_EXPLODE)
            return AVERROR_INVALIDDATA;
        return 0;
    }

    bytestream2_init(&g, buf, buf_size);

    /* read the picture type */
    type = bytestream2_get_be32u(&g);
    if (type >= FF_ARRAY_ELEMS(flac_id3v2_picture_types)) {
        av_log(s, AV_LOG_ERROR, "Invalid picture type: %d.\n", type);
        if (s->error_recognition & AV_EF_EXPLODE) {
            return AVERROR_INVALIDDATA;
        }
        type = 0;
    }

    /* picture mimetype */
    len = bytestream2_get_be32u(&g);
    if (len <= 0 || len >= sizeof(mimetype)) {
        av_log(s, AV_LOG_ERROR, "Could not read mimetype from an attached "
               "picture.\n");
        if (s->error_recognition & AV_EF_EXPLODE)
            return AVERROR_INVALIDDATA;
        return 0;
    }
    if (len + 24 > bytestream2_get_bytes_left(&g)) {
        av_log(s, AV_LOG_ERROR, "Attached picture metadata block too short\n");
        if (s->error_recognition & AV_EF_EXPLODE)
            return AVERROR_INVALIDDATA;
        return 0;
    }
    bytestream2_get_bufferu(&g, mimetype, len);
    mimetype[len] = 0;

    while (mime->id != AV_CODEC_ID_NONE) {
        if (!strncmp(mime->str, (char *)mimetype, sizeof(mimetype))) {
            id = mime->id;
            break;
        }
        mime++;
    }
    if (id == AV_CODEC_ID_NONE) {
        gxloge ("Unknown attached picture mimetype: %s.\n",
               mimetype);
        if (s->error_recognition & AV_EF_EXPLODE)
            return AVERROR_INVALIDDATA;
        return 0;
    }

    /* picture description */
    len = bytestream2_get_be32u(&g);
    if (len > bytestream2_get_bytes_left(&g) - 20) {
        gxloge ("Attached picture metadata block too short\n");
        if (s->error_recognition & AV_EF_EXPLODE)
            return AVERROR_INVALIDDATA;
        return 0;
    }
    if (len > 0) {
        desc = g.buffer;
        bytestream2_skipu(&g, len);
    }

    /* picture metadata */
    width  = bytestream2_get_be32u(&g);
    ((uint8_t*)g.buffer)[-4] = '\0';   // NUL-terminate desc.
    height = bytestream2_get_be32u(&g);
    bytestream2_skipu(&g, 8);

    /* picture data */
    len = bytestream2_get_be32u(&g);

    left = bytestream2_get_bytes_left(&g);
    if (len <= 0 || len > left) {
        if (len > MAX_TRUNC_PICTURE_SIZE || len >= INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE) {
            av_log(s, AV_LOG_ERROR, "Attached picture metadata block too big %u\n", len);
            if (s->error_recognition & AV_EF_EXPLODE)
                return AVERROR_INVALIDDATA;
            return 0;
        }

        // Workaround bug for flac muxers that writs truncated metadata picture block size if
        // the picture size do not fit in 24 bits. lavf flacenc used to have the issue and based
        // on existing broken files other unknown flac muxers seems to truncate also.
        if (truncate_workaround &&
            len > left && (len & 0xffffff) == left) {
            gxlogd ("Correcting truncated metadata picture size from %u to %u\n", left, len);
            trunclen = len - left;
        } else {
            gxlogd("Attached picture metadata block too short\n");
            if (s->error_recognition & AV_EF_EXPLODE)
                return AVERROR_INVALIDDATA;
            return 0;
        }
    }
    if (trunclen == 0 && len >= buf_size - (buf_size >> 4)) {
        data = av_mallocz(len + AV_INPUT_BUFFER_PADDING_SIZE + bytestream2_tell(&g));
        if (!data)
            return AVERROR(ENOMEM);
        size  = len + AV_INPUT_BUFFER_PADDING_SIZE;
    } else {
        if (!(data = av_mallocz(len + AV_INPUT_BUFFER_PADDING_SIZE)))
            return AVERROR(ENOMEM);
        if (trunclen == 0) {
            bytestream2_get_bufferu(&g, data, len);
        } else {
            // If truncation was detected copy all data from block and
            // read missing bytes not included in the block size.
            bytestream2_get_bufferu(&g, data, left);
            if (avio_read(s->pb, data + len - trunclen, trunclen) < trunclen)
                RETURN_ERROR(AVERROR_INVALIDDATA);
        }
    }
    memset(data + len, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    if (AV_RB64(data) == PNGSIG)
        id = AV_CODEC_ID_PNG;
    ret = ff_add_attached_pic(s, NULL, NULL, data, size);
    if (ret < 0)
        RETURN_ERROR(ret);

    st = s->streams[s->nb_streams - 1];
    st->codec->codec_id   = id;
    st->codec->width      = width;
    st->codec->height     = height;
    av_dict_set(&st->metadata, "comment", flac_id3v2_picture_types[type], 0);
    if (desc)
        av_dict_set(&st->metadata, "title", (char *)desc, 0);
    if (data) {
        av_free(data);
        data = NULL;
    }
    return 0;

fail:
    if (data) {
        av_free(data);
        data = NULL;
    }
    return ret;
}
