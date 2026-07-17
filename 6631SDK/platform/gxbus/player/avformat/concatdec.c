/*
 * Copyright (c) 2012 Nicolas George
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with FFmpeg; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "riff.h"
#include "../avutil/avstring.h"
#include "../avutil/intreadwrite.h"
#include "../avutil/opt.h"
#include "avformat.h"
#include "internal.h"
#include "gx_stream.h"
#include "avio.h"
#include "string.h"
#include "avio.h"
#include "../avutil/dict.h"

#define AV_INPUT_BUFFER_PADDING_SIZE 64

typedef enum ConcatMatchMode {
    MATCH_ONE_TO_ONE,
    MATCH_EXACT_ID,
} ConcatMatchMode;

typedef struct ConcatStream {
    int out_stream_index;
} ConcatStream;

typedef struct {
    char *url;
    int64_t start_time;
    int64_t file_start_time;
    int64_t file_inpoint;
    int64_t duration;
    int64_t user_duration;
    int64_t next_dts;
    int64_t inpoint;
    int64_t outpoint;
    int nb_streams;
    ConcatStream *streams;
    AVDictionary *metadata;
    AVDictionary *opts;
} ConcatFile;

typedef struct {
    unsigned int nb_files;
    int safe;
    int seekable;
    int eof;
    ConcatMatchMode stream_match_mode;
    unsigned int auto_convert;
    int segment_time_metadata;
    ConcatFile *files;
    ConcatFile *cur_file;
    AVFormatContext *avf;
} ConcatContext;

static int concat_probe(AVProbeData *probe)
{
    return memcmp(probe->buf, "ffconcat version 1.0", 20) ? 0 : AVPROBE_SCORE_MAX;
}

static char *get_keyword(char **cursor)
{
    char *ret = *cursor += strspn(*cursor, SPACE_CHARS);
    *cursor += strcspn(*cursor, SPACE_CHARS);
    if (**cursor) {
        *((*cursor)++) = 0;
        *cursor += strspn(*cursor, SPACE_CHARS);
    }
    return ret;
}

static int safe_filename(const char *f)
{
    const char *start = f;

    for (; *f; f++) {
        /* A-Za-z0-9_- */
        if (!((unsigned)((*f | 32) - 'a') < 26 ||
              (unsigned)(*f - '0') < 10 || *f == '_' || *f == '-')) {
            if (f == start)
                return 0;
            else if (*f == '/')
                start = f + 1;
            else if (*f != '.')
                return 0;
        }
    }
    return 1;
}

#define FAIL(retcode) do { ret = (retcode); goto fail; } while(0)

static int add_file(AVFormatContext *avf, char *filename, ConcatFile **rfile,
                    unsigned int *nb_files_alloc)
{
    ConcatContext *cat = avf->priv_data;
    char *url = NULL, *proto = NULL;
    size_t url_len, proto_len;
    int ret = -1;

    if (!cat)
        return AVERROR(EINVAL);
    if (cat->safe > 0 && !safe_filename(filename)) {
        FAIL(AVERROR(EPERM));
    }

    proto = avio_find_protocol_name(filename);
    proto_len = proto ? strlen(proto) : 0;
    if (proto && !memcmp(filename, proto, proto_len) &&
        (filename[proto_len] == ':' || filename[proto_len] == ',')) {
        url = av_strdup(filename);
        if (!url)
            FAIL(AVERROR(ENOMEM));
    } else {
        if (avf->url) {
            url_len = strlen(avf->url) + strlen(filename) + 16;
            if (!(url = av_mallocz(url_len)))
                FAIL(AVERROR(ENOMEM));
            ff_make_absolute_url(url, url_len, avf->url, filename);
        }
    }

    if (cat->nb_files >= *nb_files_alloc) {
        size_t n = FFMAX(*nb_files_alloc * 2, 16);
        ConcatFile *new_files = NULL;
        if (n <= cat->nb_files || n > SIZE_MAX / sizeof(*cat->files) ||
            !(new_files = av_realloc(cat->files, n * sizeof(*cat->files))))
            FAIL(AVERROR(ENOMEM));
        cat->files = new_files;
        *nb_files_alloc = n;
    }

    if (proto)
        av_freep(&proto);
    *rfile = &cat->files[cat->nb_files];

    cat->files[cat->nb_files].url        = av_strdup(url);
    if (url)
        av_free(url);

    cat->files[cat->nb_files].start_time = AV_NOPTS_VALUE;
    cat->files[cat->nb_files].duration   = AV_NOPTS_VALUE;
    cat->files[cat->nb_files].next_dts   = AV_NOPTS_VALUE;
    cat->files[cat->nb_files].inpoint    = AV_NOPTS_VALUE;
    cat->files[cat->nb_files].outpoint   = AV_NOPTS_VALUE;
    cat->files[cat->nb_files].user_duration = AV_NOPTS_VALUE;
    cat->files[cat->nb_files].opts       = NULL;
    cat->files[cat->nb_files].streams    = NULL;
    cat->files[cat->nb_files].metadata   = NULL;
    cat->nb_files++;

    return 0;

fail:
    av_free(url);
    return ret;
}

static int copy_stream_props(AVStream *st, AVStream *source_st)
{
    int ret;

    if (st->codec->codec_id || !source_st->codec->codec_id) {
        if (st->codec->extradata_size < source_st->codec->extradata_size) {
            if (st->codec->extradata) {
                av_freep(&st->codec->extradata);
                st->codec->extradata_size = 0;
            }
            ret = ff_alloc_extradata(st->codec,
                                     source_st->codec->extradata_size);
            if (ret < 0)
                return ret;
        }
        memcpy(st->codec->extradata, source_st->codec->extradata,
               source_st->codec->extradata_size);
        return 0;
    }

    st->codec->codec_type = source_st->codec->codec_type;
    st->codec->codec_id = source_st->codec->codec_id;
    st->codec->bit_rate = source_st->codec->bit_rate;
    st->codec->sample_rate = source_st->codec->sample_rate;
    if (source_st->codec->extradata) {
        if (!st->codec->extradata) {
            st->codec->extradata = av_mallocz(source_st->codec->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        }
        if (!st->codec->extradata) {
            return AVERROR(ENOMEM);
        }
        memcpy(st->codec->extradata, source_st->codec->extradata, source_st->codec->extradata_size);
        st->codec->extradata_size = source_st->codec->extradata_size;
    }

    st->r_frame_rate        = source_st->r_frame_rate;
    st->avg_frame_rate      = source_st->avg_frame_rate;
    st->sample_aspect_ratio = source_st->sample_aspect_ratio;
    avpriv_set_pts_info(st, AV_INPUT_BUFFER_PADDING_SIZE, source_st->time_base.num, source_st->time_base.den);

    av_dict_copy(&st->metadata, source_st->metadata, 0);
    return 0;
}

static int detect_stream_specific(AVFormatContext *avf, int idx)
{
    return 0;
}

static int match_streams_one_to_one(AVFormatContext *avf)
{
    ConcatContext *cat = avf->priv_data;
    AVStream *st;
    int i, ret;

    for (i = cat->cur_file->nb_streams; i < cat->avf->nb_streams; i++) {
        if (i < avf->nb_streams) {
            st = avf->streams[i];
        } else {
            if (!(st = avformat_new_stream(avf, NULL)))
                return AVERROR(ENOMEM);
        }
        if ((ret = copy_stream_props(st, cat->avf->streams[i])) < 0)
            return ret;
        cat->cur_file->streams[i].out_stream_index = i;
    }
    return 0;
}

static int match_streams_exact_id(AVFormatContext *avf)
{
    ConcatContext *cat = avf->priv_data;
    AVStream *st;
    int i, j, ret;

    for (i = cat->cur_file->nb_streams; i < cat->avf->nb_streams; i++) {
        st = cat->avf->streams[i];
        for (j = 0; j < avf->nb_streams; j++) {
            if (avf->streams[j]->id == st->id) {
                if ((ret = copy_stream_props(avf->streams[j], st)) < 0)
                    return ret;
                cat->cur_file->streams[i].out_stream_index = j;
            }
        }
    }
    return 0;
}

static int match_streams(AVFormatContext *avf)
{
    ConcatContext *cat = avf->priv_data;
    ConcatStream *map = NULL;
    int i, ret;

    if (cat->cur_file->nb_streams >= cat->avf->nb_streams)
        return 0;
    map = av_realloc(cat->cur_file->streams, cat->avf->nb_streams * sizeof(*map));
    if (!map) {
        return AVERROR(ENOMEM);
    }
    cat->cur_file->streams = map;
    memset(map + cat->cur_file->nb_streams, 0,
           (cat->avf->nb_streams - cat->cur_file->nb_streams) * sizeof(*map));

    for (i = cat->cur_file->nb_streams; i < cat->avf->nb_streams; i++) {
        map[i].out_stream_index = -1;
        if ((ret = detect_stream_specific(avf, i)) < 0) {
            return ret;
        }
    }
    switch (cat->stream_match_mode) {
    case MATCH_ONE_TO_ONE:
        ret = match_streams_one_to_one(avf);
        break;
    case MATCH_EXACT_ID:
        ret = match_streams_exact_id(avf);
        break;
    default:
        ret = AVERROR_BUG;
    }
    if (ret < 0) {
        return ret;
    }
    cat->cur_file->nb_streams = cat->avf->nb_streams;
    return 0;
}

static int64_t get_best_effort_duration(ConcatFile *file, AVFormatContext *avf)
{
    if (file->user_duration != AV_NOPTS_VALUE)
        return file->user_duration;
    if (file->outpoint != AV_NOPTS_VALUE)
        return file->outpoint - file->file_inpoint;
    if (avf->duration > 0)
        return avf->duration - (file->file_inpoint - file->file_start_time);
    if (file->next_dts != AV_NOPTS_VALUE)
        return file->next_dts - file->file_inpoint;
    return AV_NOPTS_VALUE;
}

extern int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags);
static int open_file(AVFormatContext *avf, unsigned int fileno)
{
    ConcatContext *cat = avf->priv_data;
    ConcatFile *file = &cat->files[fileno];
    char lavf_opts_value[256] = "";
    char lavf_opts_type[256] = "";
    int ret = 0;
    char *file_url = NULL;

    cat->segment_time_metadata = 0;
    if (cat->avf)
        avformat_close_input(&cat->avf);

    cat->avf = avformat_alloc_context();
    if (!cat->avf)
        return AVERROR(ENOMEM);

    GxPlayer_SystemGet(PSYS_INDEX_CACHE, &cat->avf->index_limit);
    if (!cat->avf)
        return AVERROR(ENOMEM);

    cat->avf->flags |= avf->flags & ~AVFMT_FLAG_CUSTOM_IO;

	/*url1 url2,need to be converted to ffmpeg options*/
    file_url = stream_url_opts_info_to_lavf(file->url, lavf_opts_type, lavf_opts_value, 256);
    if (lavf_opts_value[0]) {
        av_dict_set(&file->opts, lavf_opts_type, lavf_opts_value, 0);
    }
    av_freep(&file->url);
    file->url = av_strdup(file_url);
    av_free(file_url);
    cat->avf->file_format = avf->file_format;

    if ((ret = avformat_open_input(&cat->avf, file->url, NULL, &file->opts)) < 0 ||
        (ret = avformat_find_stream_info(cat->avf, NULL)) < 0) {
        if (cat->avf)
            avformat_close_input(&cat->avf);
        return ret;
    }
    cat->cur_file = file;
    file->start_time = !fileno ? 0 :
                       cat->files[fileno - 1].start_time +
                       cat->files[fileno - 1].duration;
    file->file_start_time = (cat->avf->start_time == AV_NOPTS_VALUE) ? 0 : cat->avf->start_time;
    file->file_inpoint = (file->inpoint == AV_NOPTS_VALUE) ? file->file_start_time : file->inpoint;
    file->duration = get_best_effort_duration(file, cat->avf);

    if (cat->segment_time_metadata) {
        av_dict_set_int(&file->metadata, "lavf.concatdec.start_time", file->start_time, 0);
        if (file->duration != AV_NOPTS_VALUE)
            av_dict_set_int(&file->metadata, "lavf.concatdec.duration", file->duration, 0);
    }
    if ((ret = match_streams(avf)) < 0)
        return ret;
    if (file->inpoint != AV_NOPTS_VALUE) {
       if ((ret = avformat_seek_file(cat->avf, -1, INT64_MIN, file->inpoint, file->inpoint, 0)) < 0) {
           return ret;
       }
    }
    return 0;
}

static int concat_read_close(AVFormatContext *avf)
{
    ConcatContext *cat = avf->priv_data;
    unsigned int i;
    if (!cat)
        return 0;

    for (i = 0; i < cat->nb_files; i++) {
        if (cat->files) {
            if (cat->files[i].opts)
                av_dict_free(&cat->files[i].opts);
            if (cat->files[i].url) {
                av_freep(&cat->files[i].url);
            }
            if (cat->files[i].streams)
                av_freep(&cat->files[i].streams);
            if (cat->files[i].metadata)
                av_dict_free(&cat->files[i].metadata);
        }
    }
    if (cat->avf)
        avformat_close_input(&cat->avf);
    av_freep(&cat->files);
    return 0;
}

extern int64_t gx_read_line_to_overwrite(AVIOContext *s, char *bp, int len);
static int concat_read_header(AVFormatContext *avf, AVFormatParameters *ap)
{
    ConcatContext *cat = avf->priv_data;
    ConcatFile *file = NULL;
    char *cursor = NULL, *keyword = NULL;
    int line = 0, i = 0;
    unsigned int nb_files_alloc = 0;
    int64_t ret = -1, time = 0;
    char bp_buf[2048];

    memset (bp_buf, 0, sizeof(bp_buf)/sizeof(char));
    cat->nb_files = 0;
    while ((ret = gx_read_line_to_overwrite(avf->pb, bp_buf, sizeof(bp_buf)/sizeof(char))) >= 0) {
        line++;
        cursor = bp_buf;
        keyword = get_keyword(&cursor);
        if (!*keyword || *keyword == '#')
            continue;

        if (!strcmp(keyword, "test")) {
            char *filename = NULL;
            filename = av_get_token((const char **)&cursor, SPACE_CHARS);
            if (!filename) {
                FAIL(AVERROR_INVALIDDATA);
            }
            if ((ret = add_file(avf, filename, &file, &nb_files_alloc)) < 0) {
				if (filename)
					av_freep(&filename);
                goto fail;
            }
            if (filename)
                av_freep(&filename);
        } else if (!strcmp(keyword, "duration") || !strcmp(keyword, "inpoint") || !strcmp(keyword, "outpoint")) {
            char *dur_str = get_keyword(&cursor);
            int64_t dur;
            if (!file) {
                FAIL(AVERROR_INVALIDDATA);
            }
            if ((ret = av_parse_time(&dur, dur_str, 1)) < 0) {
                goto fail;
            }
            if (!strcmp(keyword, "duration"))
                file->user_duration = dur;
            else if (!strcmp(keyword, "inpoint"))
                file->inpoint = dur;
            else if (!strcmp(keyword, "outpoint"))
                file->outpoint = dur;
        } else if (!strcmp(keyword, "file_packet_metadata")) {
            char *metadata = NULL;
            if (!file) {
                FAIL(AVERROR_INVALIDDATA);
            }
            metadata = av_get_token((const char **)&cursor, SPACE_CHARS);
            if (!metadata) {
                FAIL(AVERROR_INVALIDDATA);
            }
            if ((ret = av_dict_parse_string(&file->metadata, metadata, "=", "", 0)) < 0) {
                if (metadata)
                    av_freep(&metadata);
                FAIL(AVERROR_INVALIDDATA);
            }
            if (metadata)
                av_freep(&metadata);
        } else if (!strcmp(keyword, "stream")) {
            if (!avformat_new_stream(avf, NULL))
                FAIL(AVERROR(ENOMEM));
        } else if (!strcmp(keyword, "exact_stream_id")) {
            if (!avf->nb_streams) {
                FAIL(AVERROR_INVALIDDATA);
            }
            avf->streams[avf->nb_streams - 1]->id = strtol(get_keyword(&cursor), NULL, 0);
        } else if (!strcmp(keyword, "ffconcat")) {
            char *ver_kw  = get_keyword(&cursor);
            char *ver_val = get_keyword(&cursor);
            if (strcmp(ver_kw, "version") || strcmp(ver_val, "1.0")) {
                FAIL(AVERROR_INVALIDDATA);
            }

            if (cat->safe < 0)
                cat->safe = 1;
        } else {
            FAIL(AVERROR_INVALIDDATA);
        }
    }

    if (!cat->nb_files)
        FAIL(AVERROR_INVALIDDATA);

    for (i = 0; i < cat->nb_files; i++) {
        if (cat->files[i].start_time == AV_NOPTS_VALUE)
            cat->files[i].start_time = time;
        else
            time = cat->files[i].start_time;
        if (cat->files[i].user_duration == AV_NOPTS_VALUE) {
            if (cat->files[i].inpoint == AV_NOPTS_VALUE || cat->files[i].outpoint == AV_NOPTS_VALUE)
                break;
            cat->files[i].user_duration = cat->files[i].outpoint - cat->files[i].inpoint;
        }
        cat->files[i].duration = cat->files[i].user_duration;
        time += cat->files[i].user_duration;
    }

    if (i == cat->nb_files) {
        avf->duration = time;
        cat->seekable = 1;
    }

    cat->stream_match_mode = avf->nb_streams ? MATCH_EXACT_ID :MATCH_ONE_TO_ONE;
    if ((ret = open_file(avf, 0)) < 0)
        goto fail;
    return 0;

fail:
    concat_read_close(avf);
    return ret;
}

static int open_next_file(AVFormatContext *avf)
{
    ConcatContext *cat = avf->priv_data;
    unsigned int fileno = cat->cur_file - cat->files;

    cat->cur_file->duration = get_best_effort_duration(cat->cur_file, cat->avf);

    if (cat->files[fileno].opts)
        av_dict_free(&cat->files[fileno].opts);
    if (++fileno >= cat->nb_files) {
        cat->eof = 1;
        return AVERROR_EOF;
    }
    return open_file(avf, fileno);
}

/* Returns true if the packet dts is greater or equal to the specified outpoint. */
static int packet_after_outpoint(ConcatContext *cat, AVPacket *pkt)
{
    if (cat->cur_file->outpoint != AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE) {
        return av_compare_ts(pkt->dts, cat->avf->streams[pkt->stream_index]->time_base,
                             cat->cur_file->outpoint, AV_TIME_BASE_Q) >= 0;
    }
    return 0;
}

static int concat_read_packet(AVFormatContext *avf, AVPacket *pkt)
{
    ConcatContext *cat = avf->priv_data;
    int ret = -1;
    int64_t delta;
    ConcatStream *cs;
    AVStream *st;

    if (cat->eof)
        return AVERROR_EOF;

    if (!cat->avf)
        return AVERROR(EIO);

    while (1) {
        ret = av_read_frame(cat->avf, pkt);
        if (ret == AVERROR_EOF) {
            if ((ret = open_next_file(avf)) < 0) {
                return ret;
            }
            avf->concat_eof = 1;
            avf->duration = cat->avf->duration;
            avf->start_time = cat->avf->start_time;
            continue;
        }
        if (ret < 0)
            return ret;
        if ((ret = match_streams(avf)) < 0) {
            av_free_packet(pkt);
            return ret;
        }
        if (packet_after_outpoint(cat, pkt)) {
            av_free_packet(pkt);
            if ((ret = open_next_file(avf)) < 0)
                return ret;
            continue;
        }
        cs = &cat->cur_file->streams[pkt->stream_index];
        if (cs && cs->out_stream_index < 0) {
            if (pkt)
                av_free_packet(pkt);
            continue;
        }
        break;
    }

    st = cat->avf->streams[pkt->stream_index];

    delta = av_rescale_q(cat->cur_file->start_time - cat->cur_file->file_inpoint, AV_TIME_BASE_Q,
                         cat->avf->streams[pkt->stream_index]->time_base);

    if (pkt->pts != AV_NOPTS_VALUE)
        pkt->pts += delta;
    if (pkt->dts != AV_NOPTS_VALUE)
        pkt->dts += delta;
    if (cat->cur_file->metadata) {
        uint8_t* metadata;
        int metadata_len;
        char* packed_metadata = (char *)av_packet_pack_dictionary(cat->cur_file->metadata, &metadata_len);
        if (!packed_metadata)
            return AVERROR(ENOMEM);
        if (!(metadata = av_packet_new_side_data(pkt, AV_PKT_DATA_STRINGS_METADATA, metadata_len))) {
            av_freep(&packed_metadata);
            return AVERROR(ENOMEM);
        }
        memcpy(metadata, packed_metadata, metadata_len);
        av_freep(&packed_metadata);
    }

    if (cat->cur_file->duration == AV_NOPTS_VALUE && st->cur_dts != AV_NOPTS_VALUE) {
        int64_t next_dts = av_rescale_q(st->cur_dts, st->time_base, AV_TIME_BASE_Q);
        if (cat->cur_file->next_dts == AV_NOPTS_VALUE || next_dts > cat->cur_file->next_dts) {
            cat->cur_file->next_dts = next_dts;
        }
    }

    if (cs)
        pkt->stream_index = cs->out_stream_index;
    avf->streams[pkt->stream_index]->time_base.num = cat->avf->streams[pkt->stream_index]->time_base.num;
    avf->streams[pkt->stream_index]->time_base.den = cat->avf->streams[pkt->stream_index]->time_base.den;
    return ret;
}

static void rescale_interval(AVRational tb_in, AVRational tb_out,
                             int64_t *min_ts, int64_t *ts, int64_t *max_ts)
{
    *ts     = av_rescale_q    (*    ts, tb_in, tb_out);
    *min_ts = av_rescale_q_rnd(*min_ts, tb_in, tb_out,
                               AV_ROUND_UP   | AV_ROUND_PASS_MINMAX);
    *max_ts = av_rescale_q_rnd(*max_ts, tb_in, tb_out,
                               AV_ROUND_DOWN | AV_ROUND_PASS_MINMAX);
}

static int try_seek(AVFormatContext *avf, int stream,
                    int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
{
    ConcatContext *cat = avf->priv_data;
    int64_t t0 = cat->cur_file->start_time - cat->cur_file->file_inpoint;

    ts -= t0;
    min_ts = min_ts == INT64_MIN ? INT64_MIN : min_ts - t0;
    max_ts = max_ts == INT64_MAX ? INT64_MAX : max_ts - t0;
    if (stream >= 0) {
        if (stream >= cat->avf->nb_streams)
            return AVERROR(EIO);
        rescale_interval(AV_TIME_BASE_Q, cat->avf->streams[stream]->time_base,
                         &min_ts, &ts, &max_ts);
    }
    return avformat_seek_file(cat->avf, stream, min_ts, ts, max_ts, flags);
}

static int real_seek(AVFormatContext *avf, int stream,
                     int64_t min_ts, int64_t ts, int64_t max_ts, int flags, AVFormatContext *cur_avf)
{
    ConcatContext *cat = avf->priv_data;
    int ret = -1, left = 0, right = cat->nb_files;
    if (!cat)
        return AVERROR(EINVAL);

    if (stream >= 0) {
        if (stream >= avf->nb_streams)
            return AVERROR(EINVAL);
        rescale_interval(avf->streams[stream]->time_base, AV_TIME_BASE_Q,
                         &min_ts, &ts, &max_ts);
    }

    /* Always support seek to start */
    if (ts <= 0)
        right = 1;
    else if (!cat->seekable)
        return AVERROR(ESPIPE); /* XXX: can we use it? */

    while (right - left > 1) {
        int mid = (left + right) / 2;
        if (ts < cat->files[mid].start_time)
            right = mid;
        else
            left  = mid;
    }

    if (cat->cur_file != &cat->files[left]) {
        if ((ret = open_file(avf, left)) < 0)
            return ret;
    } else {
        cat->avf = cur_avf;
    }

    ret = try_seek(avf, stream, min_ts, ts, max_ts, flags);
    if (ret < 0 &&
        left < cat->nb_files - 1 &&
        cat->files[left + 1].start_time < max_ts) {
        if (cat->cur_file == &cat->files[left])
            cat->avf = NULL;
        if ((ret = open_file(avf, left + 1)) < 0)
            return ret;
        ret = try_seek(avf, stream, min_ts, ts, max_ts, flags);
    }
    return ret;
}

static int concat_seek(AVFormatContext *avf, int stream,
                       int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
{
    ConcatContext *cat = avf->priv_data;
    if (!cat)
        return AVERROR(EINVAL);
    ConcatFile *cur_file_saved = cat->cur_file;
    AVFormatContext *cur_avf_saved = cat->avf;
    int ret;

    if (flags & (AVSEEK_FLAG_BYTE | AVSEEK_FLAG_FRAME))
        return AVERROR(ENOSYS);
    cat->avf = NULL;
    if ((ret = real_seek(avf, stream, min_ts, ts, max_ts, flags, cur_avf_saved)) < 0) {
        if (cat->cur_file != cur_file_saved) {
            if (cat->avf)
                avformat_close_input(&cat->avf);
        }
        cat->avf      = cur_avf_saved;
        cat->cur_file = cur_file_saved;
    } else {
        if (cat->cur_file != cur_file_saved) {
            avformat_close_input(&cur_avf_saved);
        }
        cat->eof = 0;
    }
    return ret;
}

AVInputFormat concat_demuxer = {
    .name           = "concat",
    .priv_data_size = sizeof(ConcatContext),
    .read_probe     = concat_probe,
    .read_header    = concat_read_header,
    .read_packet    = concat_read_packet,
    .read_close     = concat_read_close,
    .read_seek2     = concat_seek,
};


