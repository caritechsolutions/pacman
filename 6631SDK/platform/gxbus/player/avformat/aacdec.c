/*
 * raw ADTS AAC demuxer
 * Copyright (c) 2008 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2009 Robert Swain ( rob opendot cl )
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
#include "../avutil/intreadwrite.h"
#include "avformat.h"
#include "../avutil/internal.h"
#include "../avutil/avstring.h"
#include "rawdec.h"
#include "avio.h"
#include "id3v1.h"
#include "id3v2.h"
#define ADTS_HEADER_SIZE 7

static int adts_aac_probe(AVProbeData *p)
{
    int max_frames = 0, first_frames = 0;
    int fsize, frames;
    const uint8_t *buf0 = p->buf;
    const uint8_t *buf2;
    const uint8_t *buf;
    const uint8_t *end = buf0 + p->buf_size - 7;

    buf = buf0;

    for (; buf < end; buf = buf2 + 1) {
        buf2 = buf;

        for (frames = 0; buf2 < end; frames++) {
            uint32_t header = AV_RB16(buf2);
            if ((header & 0xFFF6) != 0xFFF0) {
                if (buf != buf0) {
                    // Found something that isn't an ADTS header, starting
                    // from a position other than the start of the buffer.
                    // Discard the count we've accumulated so far since it
                    // probably was a false positive.
                    frames = 0;
                }
                break;
            }
            fsize = (AV_RB32(buf2 + 3) >> 13) & 0x1FFF;
            if (fsize < 7)
                break;
            fsize = FFMIN(fsize, end - buf2);
            buf2 += fsize;
        }
        max_frames = FFMAX(max_frames, frames);
        if (buf == buf0)
            first_frames = frames;
    }

    if (first_frames >= 3)
        return AVPROBE_SCORE_EXTENSION + 1;
    else if (max_frames > 100)
        return AVPROBE_SCORE_EXTENSION;
    else if (max_frames >= 3)
        return AVPROBE_SCORE_EXTENSION / 2;
    else if (first_frames >= 1)
        return 1;
    else
        return 0;
}

static int adts_aac_resync(AVFormatContext *s)
{
    uint16_t state;

    // skip data until an ADTS frame is found
    state = get_byte(s->pb);

    while (!avio_feof(s->pb) && (avio_tell(s->pb) < s->probesize)) {
        state = (state << 8) | avio_r8(s->pb);
        if ((state >> 4) != 0xFFF)
            continue;
        avio_seek(s->pb, -2, SEEK_CUR);
        break;
    }
    if (s->pb->eof_reached) {
        return AVERROR_EOF;
    }
    if ((state >> 4) != 0xFFF) {
        return AVERROR_INVALIDDATA;
    }
    return 0;
}

static int av_aac_parse_frame(AVFormatContext *s, AVStream *st, uint8_t* buf)
{
    int i = 0, sr, frameSize = 0;
    static const int srates[] =
    {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000, 0, 0, 0
    };

    if((buf[i] != 0xFF) || ((buf[i+1] & 0xF6) != 0xF0))
        return 0;

    sr = (buf[i+2] >> 2)  & 0x0F;
    if(sr > 11)
        return 0;
    if (st && st->codec) {
        st->codec->sample_rate = srates[sr];
        st->codec->channels = ((buf[i+2] & 0x1)<<2)|((buf[i+3] & 0xc0)>>6);
    }
    frameSize = ((buf[i+3] & 0x03) << 11) | (buf[i+4] << 3) | ((buf[i+5] >> 5) & 0x07);
    //*num = (buf[i+6] & 0x02) + 1;
    return frameSize;
}

static int adts_aac_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    AVStream *st;
    int ret, frameSize = 0, filesize = 0;
    uint8_t header[7];
    int64_t offset = 0, mFrameDurationUs = 0, numFrames = 0;

    st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);

    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codec->codec_id   = s->iformat->raw_codec_id;
    st->need_parsing         = AVSTREAM_PARSE_FULL_RAW;
    if ((s->pb->seekable & AVIO_SEEKABLE_NORMAL) &&
        !av_dict_get(s->metadata, "", NULL, AV_DICT_IGNORE_SUFFIX)) {
        int64_t cur = avio_tell(s->pb);
        avio_seek(s->pb, cur, SEEK_SET);
    }


    ret = adts_aac_resync(s);
    if (ret < 0)
        return ret;

    avio_seek(s->pb, 0, SEEK_SET);
    filesize = avio_size(s->pb);
    while (filesize > 0 && !avio_feof(s->pb)) {
        avio_seek(s->pb, offset, SEEK_SET);
        if (avio_read(s->pb, &header, 8) < 8) {
            return 0;
        }
        if ((frameSize = av_aac_parse_frame(s, st, header))==0) {
            return 0;
        }
        offset += frameSize;
        numFrames++;
        if (offset >= filesize) {
            break;
        }
    }
    avpriv_set_pts_info(st, 64, 1, st->codec->sample_rate);
    if (st->codec->sample_rate > 0)
        mFrameDurationUs = (1024*AV_TIME_BASE + (st->codec->sample_rate-1))/st->codec->sample_rate;
    st->duration = numFrames * mFrameDurationUs;
    st->duration = av_rescale_q(st->duration, AV_TIME_BASE_Q, st->time_base);
    avio_seek(s->pb, 0, SEEK_SET);
    return 0;
}

static int handle_id3(AVFormatContext *s, AVPacket *pkt)
{
    AVDictionary *metadata = NULL;
    AVIOContext ioctx;
    ID3v2ExtraMeta *id3v2_extra_meta = NULL;
    int ret;

    ret = av_append_packet(s->pb, pkt, ff_id3v2_tag_len(pkt->data) - pkt->size);
    if (ret < 0) {
        return ret;
    }

    ffio_init_context(&ioctx, pkt->data, pkt->size, 0, NULL, NULL, NULL, NULL);
    ff_id3v2_read_dict(&ioctx, &metadata, ID3v2_DEFAULT_MAGIC, &id3v2_extra_meta);
    if (metadata) {
        if ((ret = av_dict_copy(&s->metadata, metadata, 0)) < 0)
            goto error;
        s->event_flags |= AVFMT_EVENT_FLAG_METADATA_UPDATED;
    }

error:
    av_free_packet(pkt);
    ff_id3v2_free_extra_meta(&id3v2_extra_meta);
    av_dict_free(&metadata);
    return ret;
}

static int ff_aac_id3v2_match(const uint8_t *buf, const char *magic)
{
    return  buf[0]         == magic[0] &&
            buf[1]         == magic[1] &&
            buf[2]         == magic[2] &&
            buf[3]         != 0xff     &&
            buf[4]         != 0xff     &&
           (buf[6] & 0x80) == 0        &&
           (buf[7] & 0x80) == 0        &&
           (buf[8] & 0x80) == 0        &&
           (buf[9] & 0x80) == 0;
}

static int adts_aac_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    int ret = -1, fsize = -1;

retry:
    ret = av_get_packet(s->pb, pkt, ADTS_HEADER_SIZE);
    if (ret < 0) {
		if (avio_feof(s->pb)) {
			return AVERROR_EOF;
		} else {
        	return ret;
		}
    }

    if (ret < ADTS_HEADER_SIZE) {
		gxlogd ("aacdec.c. read fail. ret=%d\n", ret);
        return AVERROR(EIO);
    }

    if ((AV_RB16(pkt->data) >> 4) != 0xfff) {
        // Parse all the ID3 headers between frames
        int append = ID3v2_HEADER_SIZE - ADTS_HEADER_SIZE;
        if (append <= 0)
			return ret;
        ret = av_append_packet(s->pb, pkt, append);
        if (ret != append) {
			gxlogd ("aacdec.c. fail.[%d,%d]\n", ret, append);
            return AVERROR(EIO);
        }
        if (!ff_aac_id3v2_match(pkt->data, "ID3")) {
            av_free_packet(pkt);
			ret = 0;
            ret = adts_aac_resync(s);
        } else {
            ret = handle_id3(s, pkt);
        }
        if (ret < 0) {
			gxlogd ("aacdec.c. fail.ret=%d\n", ret);
            return ret;
        }

        goto retry;
    }

    fsize = (AV_RB32(pkt->data + 3) >> 13) & 0x1FFF;
    if (fsize < ADTS_HEADER_SIZE) {
        return AVERROR_INVALIDDATA;
    }

    ret = av_append_packet(s->pb, pkt, fsize - pkt->size);

    return ret;
}

static int64_t av_clip64(int64_t a, int64_t amin, int64_t amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}

static int adts_aac_seek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)
{
    int64_t offset = 0, mFrameDurationUs = 0, numFrames = 0, pos = 0;
    int frameSize = 0, ret = 0;
    uint8_t header[8];
    AVStream *st = s->streams[stream_index];
    if ((ret = avio_seek(s->pb, offset, SEEK_SET)) < 0) {
        return ret;
    }

    timestamp = av_clip64(timestamp, 0, st->duration);
    pos       = av_rescale(timestamp, avio_size(s->pb), st->duration) + s->data_offset;

    do {
         if ((ret = avio_seek(s->pb, offset, SEEK_SET)) < 0) {
             return ret;
         }
         if (avio_read(s->pb, &header, 8) < 8) {
             return -1;
         }
         if ((frameSize = av_aac_parse_frame(s, NULL, header))<= 0) {
             avio_skip(s->pb, -7);
             continue;
         }
         offset += frameSize;
    } while (offset <= pos && !avio_feof(s->pb));

    avio_seek(s->pb, offset, SEEK_SET);
    ff_read_frame_flush(s);
    return 0;
}


AVInputFormat aac_demuxer = {
    .name         = "aac",
    .read_probe   = adts_aac_probe,
    .read_header  = adts_aac_read_header,
    .read_packet  = adts_aac_read_packet,
    .read_seek    = adts_aac_seek,
    .flags        = AVFMT_GENERIC_INDEX,
    .extensions   = "aac",
    .raw_codec_id = AV_CODEC_ID_AAC,
};

