/*
 * Apple HTTP Live Streaming demuxer
 * Copyright (c) 2010 Martin Storsjo
 * Copyright (c) 2013 Anssi Hannula
 * Copyright (c) 2021 Nachiket Tarate
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

/**
 * @file
 * Apple HTTP Live Streaming demuxer
 * http://tools.ietf.org/html/draft-pantos-http-live-streaming
 */
#include "gx_stream.h"
#include "gx_media.h"
#include "gx_options.h"
#include "../avutil/avstring.h"
#include "../avutil/intreadwrite.h"
#include "../avutil/mathematics.h"
#include "../avutil/opt.h"
#include "../avutil/dict.h"
#include "avformat.h"
#include "../avutil/internal.h"
#include "riff.h"
#include "avio.h"
#include "http.h"
#include "id3v2.h"
#include "hls_sample_encryption.h"
#include "abr.h"

#define MAX_FIELD_LEN 64
#define MAX_CHARACTERISTICS_LEN 512
#define MPEG_TIME_BASE 90000
#define MPEG_TIME_BASE_Q (AVRational){1, MPEG_TIME_BASE}
#define AV_INPUT_BUFFER_PADDING_SIZE 64

#define HLS_MAX_RELOAD 200

#define HLS_AV_DICT_GET(s, e, f, h) {                       \
    if ((e = av_dict_get(s->url_options, h, NULL, 0))) {   \
        f = av_strdup(e->value);}}

/*
 * An apple http stream consists of a playlist with media segment files,
 * played sequentially. There may be several playlists with the same
 * video content, in different bandwidth variants, that are played in
 * parallel (preferably only one bandwidth variant at a time). In this case,
 * the user supplied the url to a main playlist that only lists the variant
 * playlists.
 *
 * If the main playlist doesn't point at any variants, we still create
 * one anonymous toplevel variant for this, to maintain the structure.
 */
enum KeyType {
    KEY_NONE,
    KEY_AES_128,
    KEY_SAMPLE_AES
};

struct segment {
    int64_t duration;
    int64_t url_offset;
    int64_t size;
    char *url;
    char *key;
    enum KeyType key_type;
    uint8_t iv[16];
    /* associated Media Initialization Section, treated as a segment */
    struct segment *init_section;
};

struct rendition;

enum PlaylistType {
    PLS_TYPE_UNSPECIFIED,
    PLS_TYPE_EVENT,
    PLS_TYPE_VOD
};

enum MEDIA_TYPE {
    MEDIA_TYPE_UNKNOWN  = 0, // 0000
    MEDIA_TYPE_VIDEO    = 1, // 0001
    MEDIA_TYPE_AUDIO    = 2, // 0010
    MEDIA_TYPE_DATA     = 4, // 0100
    MEDIA_TYPE_SUBTITLE = 8, // 1000
};


/*
 * Each playlist has its own demuxer. If it currently is active,
 * it has an open AVIOContext too, and potentially an AVPacket
 * containing the next packet from this stream.
 */
struct playlist {
    char *url;
    char *seg_base_url;
    char *seg_key_base_url;
    int total_url_size;
    AVIOContext pb;
    uint8_t* read_buffer;
    AVIOContext *input;
    int input_read_done;
    int is_restart_needed;
    AVIOContext *input_next;
    int input_next_requested;
    AVFormatContext *parent;
    int index;
    AVFormatContext *ctx;
    AVPacket pkt;
    int has_noheader_flag;

    /* main demuxer streams associated with this playlist
     * indexed by the subdemuxer stream indexes */
    AVStream **main_streams;
    int n_main_streams;

    int finished;
    enum PlaylistType type;
    int64_t target_duration;
    int64_t start_seq_no;
    int n_segments;
    struct segment **segments;
    int needed;
    int broken;
    int64_t cur_seq_no;
    int64_t last_seq_no;
    int64_t cur_seg_offset;
    int64_t last_load_time;
    int m3u8_hold_counters;

    /* Currently active Media Initialization Section */
    struct segment *cur_init_section;
    uint8_t *init_sec_buf;
    unsigned int init_sec_buf_size;
    unsigned int init_sec_data_len;
    unsigned int init_sec_buf_read_offset;
    char *key_url;
    uint8_t key[16];
    /* ID3 timestamp handling (elementary audio streams have ID3 timestamps
     * (and possibly other ID3 tags) in the beginning of each segment) */
    int is_id3_timestamped; /* -1: not yet known */
    int64_t id3_mpegts_timestamp; /* in mpegts tb */
    int64_t id3_offset; /* in stream original tb */
    uint8_t* id3_buf; /* temp buffer for id3 parsing */
    unsigned int id3_buf_size;
    AVDictionary *id3_initial; /* data from first id3 tag */
    int id3_found; /* ID3 tag found at some point */
    int id3_changed; /* ID3 tag data has changed at some point */
    ID3v2ExtraMeta *id3_deferred_extra; /* stored here until subdemuxer is opened */
    HLSAudioSetupInfo audio_setup_info;
    int64_t seek_timestamp;
    int seek_flags;
    int seek_stream_index; /* into subdemuxer stream array */

    /* Renditions associated with this playlist, if any.
     * Alternative rendition playlists have a single rendition associated
     * with them, and variant main Media Playlists may have
     * multiple (playlist-less) renditions associated with them. */
    int n_renditions;
    struct rendition **renditions;

    int n_variants;
    /* Media Initialization Sections (EXT-X-MAP) associated with this
     * playlist, if any. */
    int n_init_sections;
    int select_flag;
    int av_segregate_flg;
    struct segment **init_sections;
    uint64_t http_req_start_time;
    uint64_t http_req_end_time;
    uint64_t http_req_last_time;
    uint64_t internal_http_read_size;
};

/*
 * Renditions are e.g. alternative subtitle or audio streams.
 * The rendition may either be an external playlist or it may be
 * contained in the main Media Playlist of the variant (in which case
 * playlist is NULL).
 */
struct rendition {
    enum AVMediaType type;
    struct playlist *playlist;
    char group_id[MAX_FIELD_LEN];
    char language[MAX_FIELD_LEN];
    char name[MAX_FIELD_LEN];
    int disposition;
};

struct variant {
    int bandwidth;

    /* every variant contains at least the main Media Playlist in index 0 */
    int n_playlists;
    struct playlist **playlists;
    char reame_rate[20];
    char codecs[MAX_FIELD_LEN];
    char audio_group[MAX_FIELD_LEN];
    char video_group[MAX_FIELD_LEN];
    char subtitles_group[MAX_FIELD_LEN];
    char resolution[MAX_FIELD_LEN];
};

typedef struct HLSContext {
    AVFormatContext *ctx;
    int prog_now;/*use mulit .m3u8 url. current .m3u8 url.*/
    int n_variants;
    int is_av_split_streams;
    struct variant **variants;
    int n_playlists;
    struct playlist **playlists;
    int n_renditions;
    struct rendition **renditions;

    int64_t cur_seq_no;
    int live_start_index;
    int first_packet;
    int64_t first_timestamp;
    int64_t cur_timestamp;
    char *referer;                       ///< holds HTTP referer set as an AVOption to the HTTP protocol context
    char *user_agent;                    ///< holds HTTP user agent set as an AVOption to the HTTP protocol context
    char *cookies;                       ///< holds HTTP cookie values set in either the initial response or as an AVOption to the HTTP protocol context
    char *headers;                       ///< holds HTTP headers set as an AVOption to the HTTP protocol context
    char *http_proxy;                    ///< holds the address of the HTTP proxy server
    char *Authorization;
    char *CustomHeaders;
    char *socks5_proxy;
    char *avio_flags;
    AVDictionary *avio_opts;
    //int strict_std_compliance;//no use.
    char *allowed_extensions;
    int max_reload;
    int http_persistent;
    int http_multiple;
    int cur_m3u8_idx;
    int av_segregate_flg;
    int http_seekable;
    int is_switch_track;
    int switch_idx;
    AVIOContext *playlist_pb;
    HLSCryptoContext  crypto_ctx;
    ABRContext* video_abr;
    int is_in_reopen;
    handle_t abr_thread;
} HLSContext;

struct variant_info {
    char bandwidth[20];
    char reame_rate[20];
    char codecs[MAX_FIELD_LEN];
    /* variant group ids: */
    char audio[MAX_FIELD_LEN];
    char video[MAX_FIELD_LEN];
    char subtitles[MAX_FIELD_LEN];
    char resolution[MAX_FIELD_LEN];
};

static void free_segment_dynarray(struct segment **segments, int n_segments)
{
    int i = 0;
    for (i = 0; i < n_segments; i++) {
        if (segments && segments[i]) {
            if (segments[i]->key)
                av_freep(&segments[i]->key);
            if (segments[i]->url)
                av_freep(&segments[i]->url);
            if (segments[i])
                av_freep(&segments[i]);
        }
    }
}

static int auto_filter_decoder_unsupporrt_type(struct variant_info *info, struct variant *v)
{
    char resolution[MAX_FIELD_LEN+1] = {0};
    char codecs[MAX_FIELD_LEN+1] = {0};
    char *vid_width = NULL, *vid_height = NULL;

    if (info) {
        av_strlcpy(resolution, info->resolution, sizeof(resolution));
        av_strlcpy(codecs, info->codecs, sizeof(codecs));
        if (atoi(info->reame_rate) > 60) {
            gxlogi_raw_l ("The current bandwidth reame-rate:%s is not supported by the decoder.\n", info->reame_rate);
            return 1;
        }
    } else if (v) {
        av_strlcpy(resolution, v->resolution, sizeof(resolution));
        av_strlcpy(codecs, v->codecs, sizeof(codecs));
        if (atoi(v->reame_rate) > 60) {
            gxlogi_raw_l ("The current bandwidth reame-rate:%s is not supported by the decoder.\n", info->reame_rate);
            return 1;
        }
    }
    vid_width = av_strtok(resolution, "x", &vid_height);
    if ((vid_width && (atoi(vid_width) > 1920)) || (vid_height && (atoi(vid_height) > 1080))) {
        gxlogi_raw_l ("The current bandwidth resolution:%s x %s is not supported by the decoder.\n", vid_width, vid_height);
        return 1;
    }

    if ((av_stristr(codecs, "avc1.")) /*H.264 (AVC)*/
    || (av_stristr(codecs, "mp4a.")) /*AAC-LC/HE-AAC*/
    || (av_stristr(codecs, "hvc1.")) /*H.265 (HEVC)*/
    || (av_stristr(codecs, "hev1.")) /*H.265 (HEVC)*/
    || (av_stristr(codecs, "ac-3")) /*AC-3 (Dolby Digital)*/
    || (av_stristr(codecs, "ec-3")) /*EC-3 (Dolby Digital Plus)*/
    || (av_stristr(codecs, "opus"))) /*Opus*/ {
        if (av_stristr(codecs, "vp09.") || av_stristr(codecs, "av01.")) {
            gxlogi_raw_l("Current codecs:%s. The decoder does not support it.\n", codecs);
                return 1;
            }
            return 0;
    } else if ((av_stristr(codecs, "vp09.")) || (av_stristr(codecs, "av01.")) || (strlen(codecs) > 0)) {
        gxlogi_raw_l("Current codecs:%s. The decoder does not support it.\n", codecs);
            return 1;
    }
    return 0;
}

static int init_hls_abr(ABRContext** http_abr)
{
    ABRContext* abr = NULL;
    if (!(abr = (ABRContext* )av_mallocz(sizeof(ABRContext))))
        return AVERROR(ENOMEM);
    abr->max_history_len = 2;
    abr->throughput_history = NULL;
    abr->algorithm = SimpleThroughput; //AlwaysFirst;
    *http_abr = abr;
    return 0;
}

static void close_hls_abr(ABRContext* abr)
{
    if (abr) {
        if (abr->throughput_history)
            av_free(abr->throughput_history);
        av_free(abr);
    }
}


static void hls_add_metric(HLSContext* c, enum AVMediaType type, float tpt,
        int buffer_max, int buffer_r, int buffer_c)
{
    if (type != AVMEDIA_TYPE_VIDEO) {
        gxloge ("mediatype not found when adding metric.\n");
        return;
    }
    abr_add_metric(c->video_abr, tpt, buffer_max, buffer_r, buffer_c);
}

static int hls_get_stream(HLSContext* c, enum AVMediaType type)
{
    int *bandwidth, ai, i = 0;

    if (type == AVMEDIA_TYPE_VIDEO) {
        if (c->n_variants <= 0)
            return AVERROR(EINVAL);
        bandwidth = (int*)av_mallocz(sizeof(int) * c->n_variants);
        if (!bandwidth)
            return AVERROR(ENOMEM);
        for (i = 0; i < c->n_variants; ++i) {
            bandwidth[i] = c->variants[i]->bandwidth;
        }
        ai = abr_get_stream(c->video_abr, bandwidth, c->n_variants);
        av_free(bandwidth);
        return ai;
    }
    return AVERROR(EINVAL);
}

/*
   BUPT
   this function is for abr schedule
   tpt(Mbps), buffer(KB)
 */
static int hls_abr_update(AVFormatContext *s, enum AVMediaType type)
{
    HLSContext *c = s->priv_data;
    float tpt = -1;
    uint64_t t1 = av_gettime();
    int i = 0, swit_idx = -1, timeout = 0, cur_idx = -1, buffer_max = 240, buffer_r = 90, buffer_c = 126;
    struct playlist *pls;

    if (!c || (c->n_variants <= 1) || (c->n_playlists <= 1) || (type != AVMEDIA_TYPE_VIDEO)) {
        return AVERROR(EINVAL);
    }

    for (cur_idx = 0; cur_idx < c->n_playlists; cur_idx++) {
        pls = c->playlists[cur_idx];
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
        if (!pls || !pls->ctx || (!pls->n_segments)) {
            continue;
        }
        if (pls->n_variants && !pls->n_renditions) {
            break;
        }
    }
    if (cur_idx >= c->n_playlists) {
        return AVERROR(EINVAL);
    }

    while (pls && pls->http_req_end_time == 0) {
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
        GxCore_ThreadDelay(100);
        if (((av_gettime() - t1)/1000) > 5000) {
            timeout = 1;
            break;
        }
    }
    if (pls && pls->http_req_end_time > 0) {
        tpt = 8 * (float)pls->internal_http_read_size / (pls->http_req_end_time - pls->http_req_start_time);
    } else if (pls && pls->internal_http_read_size > 0) {
        tpt = 8 * (float)pls->internal_http_read_size / (pls->http_req_last_time - pls->http_req_start_time);
    } else {
        return 0;
    }

    hls_add_metric(c, type, tpt, buffer_max, buffer_r, buffer_c);
    swit_idx = hls_get_stream(c, type);
    if (swit_idx < 0 || swit_idx >= c->n_variants)
        return AVERROR(EINVAL);
    struct variant *v = c->variants[swit_idx];
    if (auto_filter_decoder_unsupporrt_type(NULL, v)) {
        return AVERROR_NOTSUPP;
    }

    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if ((!pls) || (pls->n_segments <= 0))
            continue;
        if (pls->n_variants && !pls->n_renditions) {
            if ((swit_idx != c->prog_now) && (!c->is_in_reopen)) {
                c->switch_idx = swit_idx;
                c->is_switch_track = 1;
                c->is_in_reopen = 1;
                GxCore_ThreadDelay((pls->segments[0] && pls->segments[0]->duration)?(pls->segments[0]->duration/1000):5000);
                break;
            }
        }
    }
    return 0;
}

static void hls_abr_thread(void* arg)
{
    int ret = 0;
    AVFormatContext *s = arg;
    if (!s)
        return ;
    while (!url_check_interrupt_cb()) {
        if ((ret = hls_abr_update(s, AVMEDIA_TYPE_VIDEO)) < 0)
            break;
        GxCore_ThreadDelay(500);
    }
    return ;
}

extern int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags);
static void free_segment_list(struct playlist *pls)
{
    free_segment_dynarray(pls->segments, pls->n_segments);
    if (pls->n_segments > 0 && pls->segments)
        av_freep(&pls->segments);
    pls->n_segments = 0;
}

static void free_init_section_list(struct playlist *pls)
{
    int i = 0;
    for (i = 0; i < pls->n_init_sections; i++) {
        if (pls->init_sections[i]->key) {
            av_free(pls->init_sections[i]->key);
            pls->init_sections[i]->key = NULL;
        }
        if (pls->init_sections[i]->url) {
            av_free(pls->init_sections[i]->url);
            pls->init_sections[i]->url = NULL;
        }
        if (pls->init_sections[i]) {
            av_freep(&pls->init_sections[i]);
        }
    }
    if (pls->n_init_sections > 0 && pls->init_sections) {
        av_freep(&pls->init_sections);
    }
    pls->n_init_sections = 0;
}

extern void ff_format_io_close(AVFormatContext *s, AVIOContext **pb);
static void free_playlist_list(HLSContext *c)
{
    int i = 0, j = 0;
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (!pls)
            continue;
        free_segment_list(pls);
        free_init_section_list(pls);
        for (j = 0; j < pls->n_main_streams; j++) {
            AVStream *st = pls->main_streams[j];
            if (st)
                av_dict_free(&st->metadata);
        }
        pls->n_main_streams = 0;
        if (pls->main_streams) {
            av_free(pls->main_streams);
            pls->main_streams = NULL;
        }
        if (pls->renditions)
            av_freep(&pls->renditions);
        if (pls->id3_buf)
            av_freep(&pls->id3_buf);
        if (pls->id3_initial)
            av_dict_free(&pls->id3_initial);
        if (pls->id3_deferred_extra)
            ff_id3v2_free_extra_meta(&pls->id3_deferred_extra);
        if (pls->init_sec_buf)
            av_freep(&pls->init_sec_buf);
        av_free_packet(&pls->pkt);
        if (pls->pb.buffer)
            av_freep(&pls->pb.buffer);
        if (pls->input)
            ff_format_io_close(c->ctx, &pls->input);
        pls->input_read_done = 0;
        if (pls->input_next)
            ff_format_io_close(c->ctx, &pls->input_next);
        pls->input_next_requested = 0;
        if (pls->ctx) {
            pls->ctx->pb = NULL;
            avformat_close_input(&pls->ctx);
        }
        if (pls->key_url) {
            av_free(pls->key_url);
            pls->key_url = NULL;
        }
        if (pls->seg_base_url) {
            av_free(pls->seg_base_url);
            pls->seg_base_url = NULL;
        }
        if (pls->seg_key_base_url) {
            av_free(pls->seg_key_base_url);
            pls->seg_key_base_url = NULL;
        }
        if (pls->url) {
            av_free(pls->url);
            pls->url = NULL;
        }
        if (pls) {
            av_free(pls);
        }
    }
    if (c->playlists)
        av_freep(&c->playlists);
    if (c->cookies)
        av_freep(&c->cookies);
    if (c->user_agent)
        av_freep(&c->user_agent);
    if (c->Authorization)
        av_freep(&c->Authorization);
    if (c->CustomHeaders)
        av_freep(&c->CustomHeaders);
    if (c->socks5_proxy)
        av_freep(&c->socks5_proxy);
    if (c->referer)
        av_freep(&c->referer);
    if (c->headers)
        av_freep(&c->headers);
    if (c->http_proxy)
        av_freep(&c->http_proxy);
    if (c->avio_flags)
        av_freep(&c->avio_flags);
    c->n_playlists = 0;
}

static void free_variant_list(HLSContext *c)
{
    int i = 0;
    for (i = 0; i < c->n_variants; i++) {
        struct variant *var = c->variants[i];
        av_freep(&var->playlists);
        av_free(var);
    }
    av_freep(&c->variants);
    c->n_variants = 0;
}

static void free_rendition_list(HLSContext *c)
{
    int i;
    for (i = 0; i < c->n_renditions; i++)
        av_freep(&c->renditions[i]);
    av_freep(&c->renditions);
    c->n_renditions = 0;
}

/*
 * Used to reset a statically allocated AVPacket to a clean slate,
 * containing no data.
 */
static void reset_packet(AVPacket *pkt)
{
    av_init_packet(pkt);
    pkt->data = NULL;
}

extern void ff_make_absolute_url(char *buf, int size, const char *base,
        const char *rel);
static struct playlist *new_playlist(HLSContext *c, const char *url,
        const char *base)
{
    int def_max_url_size = 0;
    char *tmp_url = NULL;
    struct playlist *pls = NULL;
    if (!(pls = av_mallocz(sizeof(struct playlist)))) {
        goto fail;
    }
    reset_packet(&pls->pkt);
    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    if (!(tmp_url = av_mallocz(def_max_url_size))) {
        goto fail;
    }
    ff_make_absolute_url(tmp_url, def_max_url_size-1, base, url);
    if (!tmp_url[0]) {
        goto fail;
    }
    if (!(pls->url = av_strdup(tmp_url))) {
        goto fail;
    }
    if (tmp_url) {
        av_free(tmp_url);
        tmp_url = NULL;
    }
    pls->seek_timestamp = AV_NOPTS_VALUE;
    pls->is_id3_timestamped = -1;
    pls->id3_mpegts_timestamp = AV_NOPTS_VALUE;
    dynarray_add(&c->playlists, &c->n_playlists, pls);
    return pls;
fail:
    if (tmp_url) {
        av_free(tmp_url);
        tmp_url = NULL;
    }
    if (pls) {
        av_free(pls);
        pls = NULL;
    }
    return NULL;
}

static struct variant *new_variant(HLSContext *c, struct variant_info *info,
        const char *url, const char *base)
{
    struct variant *var;
    struct playlist *pls;

    pls = new_playlist(c, url, base);
    if (!pls)
        return NULL;

    var = av_mallocz(sizeof(struct variant));
    if (!var)
        return NULL;

    if (info) {
        var->bandwidth = atoi(info->bandwidth);
        av_strlcpy(var->reame_rate, info->reame_rate, sizeof(var->reame_rate));
        av_strlcpy(var->codecs, info->codecs, sizeof(var->codecs));
        av_strlcpy(var->audio_group, info->audio, sizeof(var->audio_group));
        av_strlcpy(var->video_group, info->video, sizeof(var->video_group));
        av_strlcpy(var->subtitles_group, info->subtitles, sizeof(var->subtitles_group));
        av_strlcpy(var->resolution, info->resolution, sizeof(var->resolution));
        pls->n_variants = 1;
    }

    dynarray_add(&c->variants, &c->n_variants, var);
    dynarray_add(&var->playlists, &var->n_playlists, pls);
    return var;
}

static void handle_variant_args(struct variant_info *info, const char *key,
        int key_len, char **dest, int *dest_len)
{
    if (!strncmp(key, "BANDWIDTH=", key_len)) {
        *dest     =        info->bandwidth;
        *dest_len = sizeof(info->bandwidth);
    } else if (!strncmp(key, "AUDIO=", key_len)) {
        *dest     =        info->audio;
        *dest_len = sizeof(info->audio);
    } else if (!strncmp(key, "VIDEO=", key_len)) {
        *dest     =        info->video;
        *dest_len = sizeof(info->video);
    } else if (!strncmp(key, "SUBTITLES=", key_len)) {
        *dest     =        info->subtitles;
        *dest_len = sizeof(info->subtitles);
    } else if (!strncmp(key, "CODECS=", key_len)) {
        *dest     =        info->codecs;
        *dest_len = sizeof(info->codecs);
    } else if (!strncmp(key, "RESOLUTION=", key_len)) {
        *dest     =        info->resolution;
        *dest_len = sizeof(info->resolution);
    } else if (!strncmp(key, "FRAME-RATE=", key_len)) {
        *dest     =        info->reame_rate;
        *dest_len = sizeof(info->reame_rate);
    }
}

struct key_info {
    char *uri;
    char method[11];
    char iv[35];
};

static void handle_key_args(struct key_info *info, const char *key,
        int key_len, char **dest, int *dest_len)
{
    int def_max_url_size = 0;
    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    if (!strncmp(key, "METHOD=", key_len)) {
        *dest     =        info->method;
        *dest_len = sizeof(info->method);
    } else if (!strncmp(key, "URI=", key_len)) {
        *dest     =        info->uri;
        *dest_len = def_max_url_size;
    } else if (!strncmp(key, "IV=", key_len)) {
        *dest     =        info->iv;
        *dest_len = sizeof(info->iv);
    }
}

struct init_section_info {
    char *uri;
    char byterange[32];
};

static struct segment *new_init_section(struct playlist *pls,
        struct init_section_info *info,
        const char *url_base)
{
    struct segment *sec;
    int def_max_url_size = 0;
    char *tmp_str = NULL, *ptr;

    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    if (!(tmp_str = av_mallocz(def_max_url_size))) {
        return NULL;
    }
    ptr = tmp_str;

    if (!info->uri[0])
        goto fail;

    sec = av_mallocz(sizeof(*sec));
    if (!sec)
        goto fail;

    ff_make_absolute_url(tmp_str, def_max_url_size-1, url_base, info->uri);
    if (!tmp_str[0]) {
        goto fail;
    }
    sec->url = av_strdup(ptr);
    if (!sec->url) {
        goto fail;
    }
    if (tmp_str) {
        av_free(tmp_str);
        tmp_str = NULL;
    }

    if (info->byterange[0]) {
        sec->size = strtoll(info->byterange, NULL, 10);
        ptr = strchr(info->byterange, '@');
        if (ptr)
            sec->url_offset = strtoll(ptr+1, NULL, 10);
    } else {
        /* the entire file is the init section */
        sec->size = -1;
    }

    dynarray_add(&pls->init_sections, &pls->n_init_sections, sec);

    return sec;
fail:
    if (sec) {
        av_free(sec);
        sec = NULL;
    }
    if (tmp_str) {
        av_free(tmp_str);
        tmp_str = NULL;
    }
    return NULL;
}

static void handle_init_section_args(struct init_section_info *info, const char *key,
        int key_len, char **dest, int *dest_len)
{
    int def_max_url_size = 0;
    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    if (!strncmp(key, "URI=", key_len)) {
        *dest     =        info->uri;
        *dest_len = def_max_url_size;
    } else if (!strncmp(key, "BYTERANGE=", key_len)) {
        *dest     =        info->byterange;
        *dest_len = sizeof(info->byterange);
    }
}

struct rendition_info {
    char *uri;
    char type[16];
    char group_id[MAX_FIELD_LEN];
    char language[MAX_FIELD_LEN];
    char assoc_language[MAX_FIELD_LEN];
    char name[MAX_FIELD_LEN];
    char defaultr[4];
    char forced[4];
    char characteristics[MAX_CHARACTERISTICS_LEN];
};

static struct rendition *new_rendition(HLSContext *c, struct rendition_info *info,
        const char *url_base)
{
    struct rendition *rend;
    enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    char *characteristic = NULL, *chr_ptr = NULL, *saveptr = NULL;

    if (!strcmp(info->type, "AUDIO"))
        type = AVMEDIA_TYPE_AUDIO;
    else if (!strcmp(info->type, "VIDEO"))
        type = AVMEDIA_TYPE_VIDEO;
    else if (!strcmp(info->type, "SUBTITLES"))
        type = AVMEDIA_TYPE_SUBTITLE;
    else if (!strcmp(info->type, "CLOSED-CAPTIONS"))
        /* CLOSED-CAPTIONS is ignored since we do not support CEA-608 CC in
         * AVC SEI RBSP anyway */
        return NULL;

    if (type == AVMEDIA_TYPE_UNKNOWN || type == AVMEDIA_TYPE_SUBTITLE)
        return NULL;

    /* URI is mandatory for subtitles as per spec */
    //if (type == AVMEDIA_TYPE_SUBTITLE && !info->uri[0])
    //    return NULL;

    rend = av_mallocz(sizeof(struct rendition));
    if (!rend)
        return NULL;

    dynarray_add(&c->renditions, &c->n_renditions, rend);

    rend->type = type;
    av_strlcpy(rend->group_id, info->group_id, sizeof(rend->group_id));
    av_strlcpy(rend->language, info->language, sizeof(rend->language));
    av_strlcpy(rend->name, info->name, sizeof(rend->name));

    /* add the playlist if this is an external rendition */
    if (info->uri[0]) {
        rend->playlist = new_playlist(c, info->uri, url_base);
        if (rend->playlist) {
            dynarray_add(&rend->playlist->renditions,
                    &rend->playlist->n_renditions, rend);
        }
    }

    if (info->assoc_language[0]) {
        int langlen = strlen(rend->language);
        if (langlen < sizeof(rend->language) - 3) {
            size_t assoc_len;
            rend->language[langlen] = ',';
            assoc_len = av_strlcpy(rend->language + langlen + 1, info->assoc_language,
                    sizeof(rend->language) - langlen - 1);
            if (langlen + assoc_len + 2 > sizeof(rend->language)) // truncation occurred
                gxlogi_raw_l ("Truncated rendition language: %s\n", info->assoc_language);

        }
    }

    if (!strcmp(info->defaultr, "YES"))
        rend->disposition |= AV_DISPOSITION_DEFAULT;
    if (!strcmp(info->forced, "YES"))
        rend->disposition |= AV_DISPOSITION_FORCED;

    chr_ptr = info->characteristics;
    while ((characteristic = av_strtok(chr_ptr, ",", &saveptr))) {
        if (!strcmp(characteristic, "public.accessibility.describes-music-and-sound"))
            rend->disposition |= AV_DISPOSITION_HEARING_IMPAIRED;
        else if (!strcmp(characteristic, "public.accessibility.describes-video"))
            rend->disposition |= AV_DISPOSITION_VISUAL_IMPAIRED;

        chr_ptr = NULL;
    }
    return rend;
}

static void handle_rendition_args(struct rendition_info *info, const char *key,
        int key_len, char **dest, int *dest_len)
{
    int def_max_url_size = 0;
    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    if (!strncmp(key, "TYPE=", key_len)) {
        *dest     =        info->type;
        *dest_len = sizeof(info->type);
    } else if (!strncmp(key, "URI=", key_len)) {
        *dest     =        info->uri;
        *dest_len = def_max_url_size;
    } else if (!strncmp(key, "GROUP-ID=", key_len)) {
        *dest     =        info->group_id;
        *dest_len = sizeof(info->group_id);
    } else if (!strncmp(key, "LANGUAGE=", key_len)) {
        *dest     =        info->language;
        *dest_len = sizeof(info->language);
    } else if (!strncmp(key, "ASSOC-LANGUAGE=", key_len)) {
        *dest     =        info->assoc_language;
        *dest_len = sizeof(info->assoc_language);
    } else if (!strncmp(key, "NAME=", key_len)) {
        *dest     =        info->name;
        *dest_len = sizeof(info->name);
    } else if (!strncmp(key, "DEFAULT=", key_len)) {
        *dest     =        info->defaultr;
        *dest_len = sizeof(info->defaultr);
    } else if (!strncmp(key, "FORCED=", key_len)) {
        *dest     =        info->forced;
        *dest_len = sizeof(info->forced);
    } else if (!strncmp(key, "CHARACTERISTICS=", key_len)) {
        *dest     =        info->characteristics;
        *dest_len = sizeof(info->characteristics);
    }
    /*
     * ignored:
     * - AUTOSELECT: client may autoselect based on e.g. system language
     * - INSTREAM-ID: EIA-608 closed caption number ("CC1".."CC4")
     */
}

/* used by parse_playlist to allocate a new variant+playlist when the
 * playlist is detected to be a Media Playlist (not Master Playlist)
 * and we have no parent Master Playlist (parsing of which would have
 * allocated the variant and playlist already)
 * *pls == NULL  => Master Playlist or parentless Media Playlist
 * *pls != NULL => parented Media Playlist, playlist+variant allocated */
static int ensure_playlist(HLSContext *c, struct playlist **pls, const char *url)
{
    if (*pls)
        return 0;
    if (!new_variant(c, NULL, url, NULL))
        return AVERROR_NOMEM;
    *pls = c->playlists[c->n_playlists - 1];
    return 0;
}

extern int ff_http_do_new_request(URLContext *h, const char *uri, AVDictionary **opts);
static int open_url_keepalive(AVFormatContext *s, AVIOContext **pb,
        const char *url, AVDictionary **options)
{
    int ret = -1;
    URLContext *uc = ffio_geturlcontext(*pb);
    if (!uc) {
        return ret;
    }
    (*pb)->eof_reached = 0;

    ret = ff_http_do_new_request(uc, url, options);
    if (ret < 0) {
        ff_format_io_close(s, pb);
    }
    return ret;
}

static int open_url(AVFormatContext *s, AVIOContext **pb, char *url,
        AVDictionary *opts, AVDictionary *opts2, int *is_http_out)
{
    HLSContext *c = s->priv_data;
    AVDictionary *tmp = NULL;
    char *proto_name = NULL;
    int ret;
    int is_http = 0, flags = 0;

    av_dict_copy(&tmp, opts, 0);
    av_dict_copy(&tmp, opts2, 0);

    if (av_strstart(url, "crypto", NULL)) {
        if (url[6] == '+' || url[6] == ':')
            proto_name = avio_find_protocol_name(url + 7);
    } else if (av_strstart(url, "data", NULL)) {
        if (url[4] == '+' || url[4] == ':')
            proto_name = avio_find_protocol_name(url + 5);
    }

    if (!proto_name)
        proto_name = avio_find_protocol_name(url);

    if (!proto_name) {
        if (tmp)
            av_dict_free(&tmp);
        return AVERROR(EINVAL);
    }

    // only http(s) & file are allowed
    if (av_strstart(proto_name, "file", NULL)) {
        char *allowed_extensions = "3gp,aac,avi,ac3,eac3,flac,mkv,m3u8,m4a,m4s,m4v,mpg,mov,mp2,mp3,mp4,mpeg,mpegts,ogg,ogv,oga,ts,vob,wav,m3u";
        if (strcmp(allowed_extensions, "ALL") && !av_match_ext(url, allowed_extensions)) {
            gxlogd ("Filename extension of::%s\n",url);
            goto err;
        }
    } else if (av_strstart(proto_name, "http", NULL)) {
        is_http = 1;
    } else {
        goto err;
    }

    if (!strncmp(proto_name, url, strlen(proto_name)) && url[strlen(proto_name)] == ':')
        ;
    else if (av_strstart(url, "crypto", NULL) && !strncmp(proto_name, url + 7, strlen(proto_name)) && url[7 + strlen(proto_name)] == ':')
        ;
    else if (av_strstart(url, "data", NULL) && !strncmp(proto_name, url + 5, strlen(proto_name)) && url[5 + strlen(proto_name)] == ':')
        ;
    else if (strcmp(proto_name, "file") || !strncmp(url, "file,", 5)) {
        goto err;
    }

    if (c->avio_flags)
        flags = atoi(c->avio_flags);

    if (is_http && c->http_persistent && *pb && !c->is_switch_track) {
        ret = open_url_keepalive(c->ctx, pb, url, &tmp);
        if (ret == AVERROR_EXIT) {
            goto err;
        } else if (ret < 0) {
            if (ret != AVERROR_EOF)
                gxlogd ("keepalive request failed for '%s'\n", url);
            ret = avio_open2(pb, url, AVIO_FLAG_READ|flags, &tmp);
        }
    } else {
        ret = avio_open2(pb, url, AVIO_FLAG_READ|flags, &tmp);
    }
    av_dict_free(&tmp);

    if (is_http_out)
        *is_http_out = is_http;

    if (proto_name) {
        av_freep(&proto_name);
    }
    return ret;

err:
    if (proto_name) {
        av_freep(&proto_name);
    }
    if (tmp)
        av_dict_free(&tmp);
    return AVERROR(EINVAL);
}

static int find_segment_url_common_prefix_length(const char* url1, const char* url2)
{
    if (!url1 || !url2) return 0;

#define MAX_HOST_NAME_SIZE (1024)

    const char *p1 = url1, *p2 = url2;
    char *hostname1 = NULL, *hostname2 = NULL, *tmp_hostname = NULL;
    int last_pos = 0, current_pos = 0, continue_flg = 0, port1 = -1, port2 = -1;

    if (!(tmp_hostname = av_mallocz(2 * MAX_HOST_NAME_SIZE))) {
        return AVERROR(ENOMEM);
    }
    hostname1 = tmp_hostname;
    hostname2 = tmp_hostname+MAX_HOST_NAME_SIZE;

    av_url_split(NULL, 0, NULL, 0, hostname1, MAX_HOST_NAME_SIZE-1, &port1, NULL, 0, url1);
    av_url_split(NULL, 0, NULL, 0, hostname2, MAX_HOST_NAME_SIZE-1, &port2, NULL, 0, url2);

    if ((0 != av_strcasecmp(hostname1, hostname2)) || (port1 != port2)) {
        av_free(tmp_hostname);
        return AVERROR(EINVAL);
    }
    av_free(tmp_hostname);

    while (*p1 && *p2 && tolower(*p1) == tolower(*p2)) {
        if (*p1 == '/') {
            continue_flg = (*(p1+1) == '/') ? 1 : 0;
            last_pos = current_pos;
        }
        if (!continue_flg || (*p1 != '/' || *(p1-1) != '/')) {
            current_pos++;
        }
        p1++; p2++;
    }
    if (!*p1 && !*p2) {
        while (p1 > url1 && *(p1-1) != '/') p1--;
        return p1 - url1;
    }
    while (last_pos > 0 && url1[last_pos-1] == '/') {
        last_pos--;
    }
    return (last_pos > 0 || *url1 == '/') ? last_pos + 1 : 0;
}

extern int ff_get_chomp_line(AVIOContext *s, char *buf, int maxlen);
static int parse_playlist(HLSContext *c, const char *url,
        struct playlist *pls, AVIOContext *in, int prog_now, int is_reset_location)
{
    int ret = 0, is_segment = 0, is_variant = 0, close_in = 0, has_iv = 0, def_max_url_size = 0, same_size = 0, is_seg_base_flg = 1, is_seg_key_base_flg = 1;
    int64_t duration = 0, seg_offset = 0, seg_size = -1;
    enum KeyType key_type = KEY_NONE;
    uint8_t iv[16] = "";
    char *tmp_buf = NULL, *key = NULL, *line = NULL, *tmp_str = NULL, *tmp_seg_url = NULL, *tmp_seg_key_url = NULL;
    const char *ptr;
    struct variant_info variant_info;
    struct segment *cur_init_section = NULL;
    int is_http = av_strstart(url, "http", NULL);

    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    if (!(tmp_buf = av_mallocz(2*def_max_url_size))) {
        return AVERROR(ENOMEM);
    }
    line = tmp_buf;
    tmp_str = tmp_buf+def_max_url_size;

    if (is_http && !in && c->http_persistent && c->playlist_pb && !c->is_switch_track) {
        in = c->playlist_pb;
        ret = open_url_keepalive(c->ctx, &c->playlist_pb, url, &c->avio_opts);
        if (ret == AVERROR_EXIT) {
            av_free (tmp_buf);
            return ret;
        } else if (ret < 0) {
            if (ret != AVERROR_EOF)
                gxlogi_raw_l ("keepalive request failed for '%s' with error: '%s' when parsing playlist.\n", url, av_err2str(ret));
            in = NULL;
        }
    }

    if (!in) {
        AVDictionary *opts = NULL;
        /* Some HLS servers don't like being sent the range header */
        av_dict_set_int(&opts, "seekable", c->http_seekable, 0);
        // broker prior HTTP options that should be consistent across requests
        av_dict_metadata(&opts, "user_agent", c->user_agent, 0);
        av_dict_metadata(&opts, "Authorization", c->Authorization, 0);
        av_dict_metadata(&opts, "CustomHeaders", c->CustomHeaders, 0);
        av_dict_metadata(&opts, "socks5_proxy", c->socks5_proxy, 0);
        av_dict_metadata(&opts, "ffcookies", c->cookies, 0);
        av_dict_metadata(&opts, "headers", c->headers, 0);
        av_dict_metadata(&opts, "http_proxy", c->http_proxy, 0);
        av_dict_metadata(&opts, "referer", c->referer, 0);
        if (c->avio_opts)
            av_dict_copy(&opts, c->avio_opts, 0);
        av_dict_metadata(&opts, "avioflags", c->avio_flags, 0);
        av_dict_metadata(&opts, "multiple_requests", "1", 0);
        av_dict_set_int(&opts, "is_reconnect", 1, 0);
        ret = avio_open2(&in, url, c->avio_flags?((atoi(c->avio_flags)>=0)?AVIO_FLAG_READ|(atoi(c->avio_flags)):AVIO_FLAG_READ):AVIO_FLAG_READ, &opts);
        av_dict_free(&opts);
        if (ret < 0) {
            av_free (tmp_buf);
            return ret;
        }

        if (is_http && c->http_persistent)
            c->playlist_pb = in;
        else
            close_in = 1;

        if (is_reset_location && in && in->opaque) {
            AVDictionaryEntry *e = NULL;
            AVIOInternal *internal = in->opaque;
            if (internal && internal->h) {
                URLContext *h = internal->h;
                if ((e = av_dict_get(h->url_context_options, "location", NULL, 0))) {
                    if (pls->url)
                        av_free(pls->url);
                    if (!(pls->url = av_strdup(e->value))) {
                        ret = AVERROR(ENOMEM);
                        goto fail;
                    }
                    url = pls->url;
                }
            }
        }
    }

    ff_get_chomp_line(in, line, def_max_url_size-1);
    if (strcmp(line, "#EXTM3U")) {
        ret = AVERROR(EINVAL);
        goto fail;
    }

    if (pls) {
        free_segment_list(pls);
        pls->finished = 0;
        pls->total_url_size = 0;
        pls->type = PLS_TYPE_UNSPECIFIED;
    }

    while (!avio_feof(in)) {
        ff_get_chomp_line(in, line, def_max_url_size-1);
        if (pls) {
            int sysMemSize = 0, max_url_size = 0;
            GxPlayer_SystemGet(PSYS_PACKET_CACHE, &sysMemSize);
            if (sysMemSize <= 6*1024*1024)
                sysMemSize = 2*1024*1024;
            max_url_size = (c&&c->is_av_split_streams)?(sysMemSize/3):(sysMemSize*2/3);
            if (pls->total_url_size >= max_url_size) {
                if (!pls->finished) {
                    gxlogi_raw_l("Cur n_segments:%d. segments url size (%.2fMB) > %.2fMB limit.\n", pls->n_segments, (float)pls->total_url_size/1024/1024, (float)max_url_size/1024/1024);
                    pls->finished = 1;
                }
                continue;
            }
        }
        if (av_strstart(line, "#EXT-X-STREAM-INF:", &ptr)) {
            is_variant = 1;
            memset(&variant_info, 0, sizeof(variant_info));
            ff_parse_key_value(ptr, (ff_parse_key_val_cb) handle_variant_args, &variant_info);
        } else if (av_strstart(line, "#EXT-X-KEY:", &ptr)) {
            struct key_info info = {0};
            if (!(info.uri = av_mallocz(def_max_url_size))) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
            ff_parse_key_value(ptr, (ff_parse_key_val_cb) handle_key_args, &info);
            key_type = KEY_NONE;
            has_iv = 0;
            if (!strcmp(info.method, "AES-128"))
                key_type = KEY_AES_128;
            if (!strcmp(info.method, "SAMPLE-AES"))
                key_type = KEY_SAMPLE_AES;
            if (!av_strncasecmp(info.iv, "0x", 2)) {
                ff_hex_to_data(iv, info.iv + 2);
                has_iv = 1;
            }
            if (key) {
                av_free(key);
                key = NULL;
            }
            key = av_strdup(info.uri);
            if (info.uri) {
                av_free(info.uri);
                info.uri = NULL;
            }
            if (!key) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
        } else if (av_strstart(line, "#EXT-X-MEDIA:", &ptr)) {
            struct rendition_info info = {0};
            if (!(info.uri = av_mallocz(def_max_url_size))) {
                ret= AVERROR(ENOMEM);
                goto fail;
            }
            ff_parse_key_value(ptr, (ff_parse_key_val_cb) handle_rendition_args, &info);
            new_rendition(c, &info, url);
            if (info.uri) {
                av_free(info.uri);
                info.uri = NULL;
            }
        } else if (av_strstart(line, "#EXT-X-TARGETDURATION:", &ptr)) {
            int64_t t;
            ret = ensure_playlist(c, &pls, url);
            if (ret < 0)
                goto fail;
            t = strtoll(ptr, NULL, 10);
            if (t < 0 || t >= INT64_MAX / AV_TIME_BASE) {
                ret = AVERROR_INVALIDDATA;
                goto fail;
            }
            pls->target_duration = t * AV_TIME_BASE;
        } else if (av_strstart(line, "#EXT-X-MEDIA-SEQUENCE:", &ptr)) {
            uint64_t seq_no;
            ret = ensure_playlist(c, &pls, url);
            if (ret < 0)
                goto fail;
            seq_no = strtoull(ptr, NULL, 10);
            if (seq_no > INT64_MAX/2) {
                gxlogi ("MEDIA-SEQUENCE higher than INT64_MAX/2, mask out the highest bit\n");
                seq_no &= INT64_MAX/2;
            }
            pls->start_seq_no = seq_no;
        } else if (av_strstart(line, "#EXT-X-PLAYLIST-TYPE:", &ptr)) {
            ret = ensure_playlist(c, &pls, url);
            if (ret < 0)
                goto fail;
            if (!strcmp(ptr, "EVENT"))
                pls->type = PLS_TYPE_EVENT;
            else if (!strcmp(ptr, "VOD"))
                pls->type = PLS_TYPE_VOD;
        } else if (av_strstart(line, "#EXT-X-MAP:", &ptr)) {
            struct init_section_info info = {0};
            if (!(info.uri = av_mallocz(def_max_url_size))) {
                ret= AVERROR(ENOMEM);
                goto fail;
            }
            ret = ensure_playlist(c, &pls, url);
            if (ret < 0) {
                if (info.uri) {
                    av_free(info.uri);
                    info.uri = NULL;
                }
                goto fail;
            }
            ff_parse_key_value(ptr, (ff_parse_key_val_cb) handle_init_section_args, &info);
            cur_init_section = new_init_section(pls, &info, url);
            if (info.uri) {
                av_free(info.uri);
                info.uri = NULL;
            }
            if (!cur_init_section) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
            cur_init_section->key_type = key_type;
            if (has_iv) {
                memcpy(cur_init_section->iv, iv, sizeof(iv));
            } else {
                int seq = pls->start_seq_no + pls->n_segments;
                memset(cur_init_section->iv, 0, sizeof(cur_init_section->iv));
                AV_WB32(cur_init_section->iv + 12, seq);
            }

            if (key_type != KEY_NONE) {
                ff_make_absolute_url(tmp_str, def_max_url_size-1, url, key);
                if (!tmp_str[0]) {
                    av_free(cur_init_section);
                    ret = AVERROR(EINVAL);
                    goto fail;
                }
                cur_init_section->key = av_strdup(tmp_str);
                if (!cur_init_section->key) {
                    av_free(cur_init_section);
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
            } else {
                cur_init_section->key = NULL;
            }
        } else if (av_strstart(line, "#EXT-X-ENDLIST", &ptr)) {
            if (pls)
                pls->finished = 1;
        } else if (av_strstart(line, "#EXTINF:", &ptr)) {
            is_segment = 1;
            duration   = atof(ptr) * AV_TIME_BASE;
        } else if (av_strstart(line, "#EXT-X-BYTERANGE:", &ptr)) {
            seg_size = strtoll(ptr, NULL, 10);
            ptr = strchr(ptr, '@');
            if (ptr)
                seg_offset = strtoll(ptr+1, NULL, 10);
        } else if (av_strstart(line, "#", NULL)) {
            gxlogi_raw_l ("Skip ('%s')\n", line);
            continue;
        } else if (line[0]) {
            if (is_variant) {
                if (auto_filter_decoder_unsupporrt_type(&variant_info, NULL)) {
                    continue;
                }
                if (!new_variant(c, &variant_info, line, url)) {
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
                is_variant = 0;
            }
            if (is_segment) {
                struct segment *seg = NULL;
                if (!pls) {
                    if (!new_variant(c, 0, url, NULL)) {
                        ret = AVERROR(ENOMEM);
                        goto fail;
                    }
                    pls = c->playlists[c->n_playlists - 1];
                }
                seg = av_malloc(sizeof(struct segment));
                if (!seg) {
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }

                if (has_iv) {
                    memcpy(seg->iv, iv, sizeof(iv));
                } else {
                    int seq = pls->start_seq_no + pls->n_segments;
                    memset(seg->iv, 0, sizeof(seg->iv));
                    AV_WB32(seg->iv + 12, seq);
                }

                if (key_type != KEY_NONE) {
                    ff_make_absolute_url(tmp_str, def_max_url_size-1, url, key);
                    if (!tmp_str[0]) {
                        ret = AVERROR(ENOMEM);
                        av_free(seg);
                        goto fail;
                    }
                    if ((same_size = find_segment_url_common_prefix_length(tmp_seg_key_url, tmp_str)) > 0) {
                        if (is_seg_key_base_flg) {
                            is_seg_key_base_flg = 0;
                            char *seg_key_base_url = "/seg_key_base.key";
                            int seg_key_base_url_size = same_size + 1 + strlen(seg_key_base_url);
                            if (!(pls->seg_key_base_url = av_malloc(seg_key_base_url_size))) {
                                ret = AVERROR(ENOMEM);
                                av_free(seg);
                                goto fail;
                            }
                            snprintf (pls->seg_key_base_url, same_size, "%s", tmp_seg_key_url);
                            av_strlcat(pls->seg_key_base_url, seg_key_base_url, seg_key_base_url_size);
                        }
                        if (!(seg->key = av_strdup(tmp_str+same_size))) {
                            ret = AVERROR(ENOMEM);
                            av_free(seg);
                            goto fail;
                        }
                    } else {
                        if (!tmp_seg_key_url) {
                            if (!(tmp_seg_key_url = av_strdup(tmp_str))) {
                                av_free(seg);
                                ret = AVERROR(ENOMEM);
                                goto fail;
                            }
                        }
                        if (!(seg->key = av_strdup(tmp_str))) {
                            av_free(seg);
                            ret = AVERROR(ENOMEM);
                            goto fail;
                        }
                    }
                    if (pls)
                        pls->total_url_size += strlen(tmp_str);
                } else {
                    seg->key = NULL;
                }

                ff_make_absolute_url(tmp_str, def_max_url_size-1, url, line);
                if (!tmp_str[0]) {
                    ret = AVERROR(EINVAL);
                    if (seg->key)
                        av_free(seg->key);
                    av_free(seg);
                    goto fail;
                }
                if ((same_size = find_segment_url_common_prefix_length(tmp_seg_url, tmp_str)) > 0) {
                    if (is_seg_base_flg) {
                        is_seg_base_flg = 0;
                        char *seg_base_url = "/seg_base.ts";
                        int seg_base_url_size = same_size + 1 + strlen(seg_base_url);
                        if (!(pls->seg_base_url = av_malloc(seg_base_url_size))) {
                            ret = AVERROR(ENOMEM);
                            if (seg->key)
                                av_free(seg->key);
                            av_free(seg);
                            goto fail;
                        }
                        snprintf (pls->seg_base_url, same_size, "%s", tmp_seg_url);
                        av_strlcat(pls->seg_base_url, seg_base_url, seg_base_url_size);
                    }
                    if (!(seg->url = av_strdup(tmp_str+same_size))) {
                        ret = AVERROR(ENOMEM);
                        if (seg->key)
                            av_free(seg->key);
                        av_free(seg);
                        goto fail;
                    }
                } else {
                    if (!tmp_seg_url) {
                        tmp_seg_url = av_strdup(tmp_str);
                    }
                    if (!(seg->url = av_strdup(tmp_str))) {
                        av_free(seg->key);
                        av_free(seg);
                        ret = AVERROR(ENOMEM);
                        goto fail;
                    }
                }
                if (pls) {
                    pls->total_url_size += strlen(seg->url);
                }
                if (duration < 0.001 * AV_TIME_BASE) {
                    duration = 0.001 * AV_TIME_BASE;
                }
                seg->duration = duration;
                seg->key_type = key_type;

                dynarray_add(&pls->segments, &pls->n_segments, seg);
                is_segment = 0;

                seg->size = seg_size;
                if (seg_size >= 0) {
                    seg->url_offset = seg_offset;
                    seg_offset += seg_size;
                    seg_size = -1;
                } else {
                    seg->url_offset = 0;
                    seg_offset = 0;
                }

                seg->init_section = cur_init_section;
            }
        }
    }

    if (pls && PLS_TYPE_VOD == pls->type && !pls->finished) {
        pls->finished = 1;
    }

    if (pls && !pls->finished) {/*live mode.*/
        if (pls->n_segments <= 8)/*if segment_nb <8, start_index:0.other:-3*/
            c->live_start_index = 0;
        AVFormatContext *s = c->ctx;
        AVDictionaryEntry *e = NULL;
        if (s && s->url_options && (e = av_dict_get(s->url_options, "live_start_index", NULL, 0))) {
            c->live_start_index = atoi(e->value);
        }
    }
    if (pls)
        pls->last_load_time = gx_av_gettime_relative();
    if (pls && pls->n_segments <= 0) {
        gxlogi ("There is no ts segment in hls m3u8 playlist.\n");
        ret = AVERROR_INVALIDDATA;
    }
fail:
    if (key) {
        av_freep(&key);
    }
    if (tmp_seg_url)
        av_free(tmp_seg_url);
    if (tmp_seg_key_url)
        av_free(tmp_seg_key_url);
    if (tmp_buf)
        av_freep(&tmp_buf);
    if (close_in)
        ff_format_io_close(c->ctx, &in);
    prog_now = (prog_now >= c->n_variants)?(c->n_variants-1):prog_now;
    prog_now = (prog_now >= 0)?prog_now:0;
    c->ctx->ctx_flags = c->ctx->ctx_flags & ~(unsigned)AVFMTCTX_UNSEEKABLE;
    if (!c->n_variants || !c->variants[prog_now]->n_playlists ||
            !(c->variants[prog_now]->playlists[0]->finished ||
                c->variants[prog_now]->playlists[0]->type == PLS_TYPE_EVENT)) {
        c->ctx->ctx_flags |= AVFMTCTX_UNSEEKABLE;
    }
    return ret;
}

static struct segment *current_segment(struct playlist *pls)
{
    int64_t n = pls->cur_seq_no - pls->start_seq_no;
    if ((n < 0) || (n >= pls->n_segments))
        return NULL;
    return pls->segments[n];
}

static struct segment *next_segment(struct playlist *pls)
{
    int64_t n = pls->cur_seq_no - pls->start_seq_no + 1;
    if ((n < 0) || (n >= pls->n_segments))
        return NULL;
    return pls->segments[n];
}

enum ReadFromURLMode {
    READ_NORMAL,
    READ_COMPLETE,
};

static int read_from_url(struct playlist *pls, struct segment *seg,
        uint8_t *buf, int buf_size,
        enum ReadFromURLMode mode)
{
    int ret;

    /* limit read if the segment was only a part of a file */
    if (seg->size >= 0)
        buf_size = FFMIN(buf_size, seg->size - pls->cur_seg_offset);

    ret = avio_read(pls->input, buf, buf_size);

    if (ret > 0)
        pls->cur_seg_offset += ret;

    return ret;
}

/* Parse the raw ID3 data and pass contents to caller */
static void parse_id3(AVFormatContext *s, AVIOContext *pb,
        AVDictionary **metadata, int64_t *dts, HLSAudioSetupInfo *audio_setup_info,
        ID3v2ExtraMetaAPIC **apic, ID3v2ExtraMeta **extra_meta)
{
    char id3_priv_owner_ts[] = "com.apple.streaming.transportStreamTimestamp";
    char id3_priv_owner_audio_setup[] = "com.apple.streaming.audioDescription";
    ID3v2ExtraMeta *meta;

    ff_id3v2_read_dict(pb, metadata, ID3v2_DEFAULT_MAGIC, extra_meta);
    for (meta = *extra_meta; meta; meta = meta->next) {
        if (!strcmp(meta->tag, "PRIV")) {
            ID3v2ExtraMetaPRIV *priv = meta->data;
            if (priv->datasize == 8 && !av_strncasecmp((char *)priv->owner, id3_priv_owner_ts, 44)) {
                /* 33-bit MPEG timestamp */
                int64_t ts = AV_RB64(priv->data);
                gxlogd("HLS ID3 audio timestamp:%lld\n", ts);
                if ((ts & ~((1ULL << 33) - 1)) == 0)
                    *dts = ts;
                else
                    gxlogd("Invalid HLS ID3 audio timestamp:%lld\n", ts);
            } else if (priv->datasize >= 8 && !av_strncasecmp((char *)priv->owner, id3_priv_owner_audio_setup, 36)) {
                ff_hls_senc_read_audio_setup_info(audio_setup_info, priv->data, priv->datasize);
            }
        } else if (!strcmp(meta->tag, "APIC") && apic)
            *apic = meta->data;
    }
}

/* Check if the ID3 metadata contents have changed */
static int id3_has_changed_values(struct playlist *pls, AVDictionary *metadata,
        ID3v2ExtraMetaAPIC *apic)
{
    AVDictionaryEntry *entry = NULL;
    AVDictionaryEntry *oldentry;
    /* check that no keys have changed values */
    while ((entry = av_dict_get(metadata, "", entry, AV_DICT_IGNORE_SUFFIX))) {
        oldentry = av_dict_get(pls->id3_initial, entry->key, NULL, AV_DICT_MATCH_CASE);
        if (!oldentry || strcmp(oldentry->value, entry->value) != 0)
            return 1;
    }

    /* check if apic appeared */
    if (apic && (pls->ctx->nb_streams != 2 || !pls->ctx->streams[1]->attached_pic.data))
        return 1;
    return 0;
}

/* Parse ID3 data and handle the found data */
static void handle_id3(AVIOContext *pb, struct playlist *pls)
{
    AVDictionary *metadata = NULL;
    ID3v2ExtraMetaAPIC *apic = NULL;
    ID3v2ExtraMeta *extra_meta = NULL;
    int64_t timestamp = AV_NOPTS_VALUE;

    parse_id3(pls->ctx, pb, &metadata, &timestamp, &pls->audio_setup_info, &apic, &extra_meta);

    if (timestamp != AV_NOPTS_VALUE) {
        pls->id3_mpegts_timestamp = timestamp;
        pls->id3_offset = 0;
    }

    if (!pls->id3_found) {
        /* initial ID3 tags */
        av_assert0(!pls->id3_deferred_extra);
        pls->id3_found = 1;

        /* get picture attachment and set text metadata */
        if (pls->ctx->nb_streams)
            ff_id3v2_parse_apic(pls->ctx, &extra_meta);
        else
            /* demuxer not yet opened, defer picture attachment */
            pls->id3_deferred_extra = extra_meta;

        av_dict_copy(&pls->ctx->metadata, metadata, 0);
        pls->id3_initial = metadata;

    } else {
        if (!pls->id3_changed && id3_has_changed_values(pls, metadata, apic)) {
            pls->id3_changed = 1;
        }
        av_dict_free(&metadata);
    }

    if (!pls->id3_deferred_extra)
        ff_id3v2_free_extra_meta(&extra_meta);
}

static void intercept_id3(struct playlist *pls, uint8_t *buf,
        int buf_size, int *len)
{
    /* intercept id3 tags, we do not want to pass them to the raw
     * demuxer on all segment switches */
    int bytes;
    int id3_buf_pos = 0;
    int fill_buf = 0;
    struct segment *seg = current_segment(pls);

    /* gather all the id3 tags */
    while (1) {
        /* see if we can retrieve enough data for ID3 header */
        if (*len < ID3v2_HEADER_SIZE && buf_size >= ID3v2_HEADER_SIZE) {
            bytes = read_from_url(pls, seg, buf + *len, ID3v2_HEADER_SIZE - *len, READ_COMPLETE);
            if (bytes > 0) {

                if (bytes == ID3v2_HEADER_SIZE - *len)
                    /* no EOF yet, so fill the caller buffer again after
                     * we have stripped the ID3 tags */
                    fill_buf = 1;

                *len += bytes;

            } else if (*len <= 0) {
                /* error/EOF */
                *len = bytes;
                fill_buf = 0;
            }
        }

        if (*len < ID3v2_HEADER_SIZE)
            break;

        if (ff_id3v2_match(buf, ID3v2_DEFAULT_MAGIC)) {
            int64_t maxsize = seg->size >= 0 ? seg->size : 1024*1024;
            int taglen = ff_id3v2_tag_len(buf);
            int tag_got_bytes = FFMIN(taglen, *len);
            int remaining = taglen - tag_got_bytes;

            if (taglen > maxsize) {
                gxlogd("Too large HLS ID3 tag (%d > %lld bytes)\n", taglen, maxsize);
                break;
            }

            /*
             * Copy the id3 tag to our temporary id3 buffer.
             * We could read a small id3 tag directly without memcpy, but
             * we would still need to copy the large tags, and handling
             * both of those cases together with the possibility for multiple
             * tags would make the handling a bit complex.
             */
            pls->id3_buf = av_fast_realloc(pls->id3_buf, &pls->id3_buf_size, id3_buf_pos + taglen);
            if (!pls->id3_buf)
                break;
            memcpy(pls->id3_buf + id3_buf_pos, buf, tag_got_bytes);
            id3_buf_pos += tag_got_bytes;

            /* strip the intercepted bytes */
            *len -= tag_got_bytes;
            memmove(buf, buf + tag_got_bytes, *len);

            if (remaining > 0) {
                /* read the rest of the tag in */
                if (read_from_url(pls, seg, pls->id3_buf + id3_buf_pos, remaining, READ_COMPLETE) != remaining)
                    break;
                id3_buf_pos += remaining;
            }

        } else {
            /* no more ID3 tags */
            break;
        }
    }

    /* re-fill buffer for the caller unless EOF */
    if (*len >= 0 && (fill_buf || *len == 0)) {
        bytes = read_from_url(pls, seg, buf + *len, buf_size - *len, READ_NORMAL);

        /* ignore error if we already had some data */
        if (bytes >= 0)
            *len += bytes;
        else if (*len == 0)
            *len = bytes;
    }

    if (pls->id3_buf) {
        /* Now parse all the ID3 tags */
        AVIOContext id3ioctx;
        ffio_init_context(&id3ioctx, pls->id3_buf, id3_buf_pos, 0, NULL, NULL, NULL, NULL);
        handle_id3(&id3ioctx, pls);
    }

    if (pls->is_id3_timestamped == -1)
        pls->is_id3_timestamped = (pls->id3_mpegts_timestamp != AV_NOPTS_VALUE);
}

static int open_input(HLSContext *c, struct playlist *pls, struct segment *seg, AVIOContext **in, int is_high_storage_mode)
{
    AVDictionary *opts = NULL;
    AVDictionaryEntry *e = NULL;
    AVFormatContext *ctx = NULL;
    int ret = -1, is_http = 0, new_url_size = 0, new_key_url_size = 0;;
    char *new_url = NULL, *new_key_url = NULL;

    // broker prior HTTP options that should be consistent across requests
    av_dict_metadata(&opts, "user_agent", c->user_agent, 0);
    av_dict_metadata(&opts, "Authorization", c->Authorization, 0);
    av_dict_metadata(&opts, "CustomHeaders", c->CustomHeaders, 0);
    av_dict_metadata(&opts, "socks5_proxy", c->socks5_proxy, 0);
    av_dict_metadata(&opts, "referer", c->referer, 0);
    av_dict_metadata(&opts, "ffcookies", c->cookies, 0);
    av_dict_metadata(&opts, "headers", c->headers, 0);
    av_dict_metadata(&opts, "http_proxy", c->http_proxy, 0);
    av_dict_metadata(&opts, "avioflags", c->avio_flags, 0);
    av_dict_metadata(&opts, "multiple_requests", "1", 0);
    av_dict_set_int(&opts, "rw_timeout", 8, 0);//set timeout:8S.
    av_dict_set_int(&opts, "is_reconnect", 0, 0);
    av_dict_set_int(&opts, "seekable", c->http_seekable, 0);
    av_dict_set_int(&opts, "is_live", (pls->finished)?0:1, 0);
    ctx = c->ctx;
    if (ctx && (e = av_dict_get(ctx->url_options, "ssl_cipher_list", NULL, 0)) && e->value) {
        av_dict_set(&opts, "ssl_cipher_list", e->value, 0);
    }

    if (seg->size >= 0) {
        /* try to restrict the HTTP request to the part we want
         * (if this is in fact a HTTP request) */
        av_dict_set_int(&opts, "offset", seg->url_offset, 0);
        av_dict_set_int(&opts, "end_offset", seg->url_offset + seg->size, 0);
    }

    if (seg->key_type == KEY_AES_128 || seg->key_type == KEY_SAMPLE_AES) {
        pls->http_req_start_time = av_gettime();
        pls->http_req_end_time = 0;
        pls->http_req_last_time = 0;
        pls->internal_http_read_size = 0;
        if (!pls->key_url || (pls->key_url && seg->key && strcmp(seg->key, pls->key_url))) {
            AVIOContext *pb = NULL;
            if (is_high_storage_mode && pls->seg_key_base_url) {
                new_key_url_size = strlen(pls->seg_key_base_url) + strlen(seg->key) + 1;
                if (!(new_key_url = av_malloc(new_key_url_size))) {
                    ret = AVERROR(ENOMEM);
                    goto cleanup;
                }
                ff_make_absolute_url(new_key_url, new_key_url_size-1, pls->seg_key_base_url, seg->key);
            } else {
                new_key_url_size = strlen(seg->key) + 1;
                if (!(new_key_url = av_malloc(new_key_url_size))) {
                    ret = AVERROR(ENOMEM);
                    goto cleanup;
                }
                snprintf (new_key_url, new_key_url_size-1, "%s", seg->key);
            }
            if (open_url(pls->parent, &pb, new_key_url, c->avio_opts, opts, NULL) == 0) {
                ret = avio_read(pb, pls->key, sizeof(pls->key));
                if (ret != sizeof(pls->key)) {
                    gxlogi_raw_l ("Unable to read key file:%s\n", seg->key);
                }
                ff_format_io_close(pls->parent, &pb);
            } else {
                gxlogd ("Unable to open key file %s\n", seg->key);
            }
            if (pls->key_url) {
                av_free(pls->key_url);
            }
            if (!(pls->key_url = av_strdup(seg->key))) {
                ret = AVERROR(ENOMEM);
                goto cleanup;
            }
        }
    }

    if (seg->key_type == KEY_AES_128) {
        AVDictionary *opts2 = NULL;
        int url_size = 0;
        char iv[33], key[33], *url;
        ff_data_to_hex(iv, seg->iv, sizeof(seg->iv), 0);
        ff_data_to_hex(key, pls->key, sizeof(pls->key), 0);
        iv[32] = key[32] = '\0';
        if (is_high_storage_mode && pls->seg_base_url) {
            new_url_size = strlen(pls->seg_base_url) + strlen(seg->url) + 4;
            if (!(new_url = av_malloc(new_url_size))) {
                ret = AVERROR(ENOMEM);
                goto cleanup;
            }
            ff_make_absolute_url(new_url, new_url_size-1, pls->seg_base_url, seg->url);
        } else {
            new_url_size = strlen(seg->url) + 3;
            if (!(new_url = av_malloc(new_url_size))) {
                ret = AVERROR(ENOMEM);
                goto cleanup;
            }
            snprintf (new_url, new_url_size-1, "%s", seg->url);
        }
		url_size = strlen(new_url) + strlen("crypto+") + 4;
        if (!(url = av_malloc(url_size))) {
            ret = AVERROR(ENOMEM);
            goto cleanup;
        }
        if (strstr(new_url, "://"))
            snprintf(url, url_size-1, "crypto+%s", new_url);
        else
            snprintf(url, url_size-1, "crypto:%s", new_url);

        av_dict_copy(&opts2, c->avio_opts, 0);
        av_dict_set(&opts2, "key", key, 0);
        av_dict_set(&opts2, "iv", iv, 0);
        ret = open_url(pls->parent, in, url, opts2, opts, &is_http);
        av_dict_free(&opts2);
        if (url)
            av_freep(&url);
        if (ret < 0) {
            goto cleanup;
        }
        ret = 0;
    } else {
        pls->http_req_start_time = av_gettime();
        pls->http_req_end_time = 0;
        pls->http_req_last_time = 0;
        pls->internal_http_read_size = 0;
        if (is_high_storage_mode && pls->seg_base_url) {
			new_url_size = strlen(pls->seg_base_url) + strlen(seg->url) + 4;
            if (!(new_url = av_malloc(new_url_size))) {
                ret = AVERROR(ENOMEM);
                goto cleanup;
            }
            ff_make_absolute_url(new_url, new_url_size-1, pls->seg_base_url, seg->url);
        } else {
			new_url_size = strlen(seg->url) + 3;
            if (!(new_url = av_malloc(new_url_size))) {
                ret = AVERROR(ENOMEM);
                goto cleanup;
            }
            snprintf (new_url, new_url_size-1, "%s", seg->url);
        }
        gxlogi_raw_l ("HLS request url '%s'\n", new_url);
        ret = open_url(pls->parent, in, new_url, c->avio_opts, opts, &is_http);
    }
    /* Seek to the requested position. If this was a HTTP request, the offset
     * should already be where want it to, but this allows e.g. local testing
     * without a HTTP server.
     *
     * This is not done for HTTP at all as avio_seek() does internal bookkeeping
     * of file offset which is out-of-sync with the actual offset when "offset"
     * AVOption is used with http protocol, causing the seek to not be a no-op
     * as would be expected. Wrong offset received from the server will not be
     * noticed without the call, though.
     */
    if (ret == 0 && !is_http && seg->key_type == KEY_NONE && seg->url_offset) {
        int64_t seekret = avio_seek(*in, seg->url_offset, SEEK_SET);
        if (seekret < 0) {
            gxlogd ("Unable to seek to offset:%lld of HLS segment:[%s]\n", seg->url_offset, seg->url);
            ret = seekret;
            ff_format_io_close(pls->parent, in);
        }
    }
cleanup:
    av_dict_free(&opts);
    if (new_url)
        av_free(new_url);
    if (new_key_url)
        av_free(new_key_url);
    pls->cur_seg_offset = 0;
    return ret;
}

static int update_init_section(struct playlist *pls, struct segment *seg)
{
    static const int max_init_section_size = 512*1024;
    HLSContext *c = pls->parent->priv_data;
    int64_t sec_size, urlsize;
    int ret = -1;

    if (seg->init_section == pls->cur_init_section) {
        return 0;
    }

    pls->cur_init_section = NULL;

    if (!seg->init_section) {
        return 0;
    }

    ret = open_input(c, pls, seg->init_section, &pls->input, 0);
    if (ret < 0) {
        gxlogd("Failed to open an initialization section in playlist %d\n", pls->index);
        return ret;
    }

    if (seg->init_section->size >= 0)
        sec_size = seg->init_section->size;
    else if ((urlsize = avio_size(pls->input)) >= 0)
        sec_size = urlsize;
    else
        sec_size = max_init_section_size;

    gxlogd("Downloading an initialization section of size %lld\n", sec_size);

    sec_size = FFMIN(sec_size, max_init_section_size);

    av_fast_malloc(&pls->init_sec_buf, (int *)(&pls->init_sec_buf_size), sec_size, 0);

    ret = read_from_url(pls, seg->init_section, pls->init_sec_buf,
            pls->init_sec_buf_size, READ_COMPLETE);
    ff_format_io_close(pls->parent, &pls->input);

    if (ret < 0)
        return ret;

    pls->cur_init_section = seg->init_section;
    pls->init_sec_data_len = ret;
    pls->init_sec_buf_read_offset = 0;
    /* spec says audio elementary streams do not have media initialization
     * sections, so there should be no ID3 timestamps */
    pls->is_id3_timestamped = 0;
    return 0;
}

static int playlist_needed(struct playlist *pls)
{
    AVFormatContext *s = pls->parent;
    int i, j;
    int stream_needed = 0;
    int first_st;

    /* If there is no context or streams yet, the playlist is needed */
    if (!pls->ctx || !pls->n_main_streams)
        return 1;

    /* check if any of the streams in the playlist are needed */
    for (i = 0; i < pls->n_main_streams; i++) {
        if (pls->main_streams[i]->discard < AVDISCARD_ALL) {
            stream_needed = 1;
            break;
        }
    }

    /* If all streams in the playlist were discarded, the playlist is not
     * needed (regardless of whether whole programs are discarded or not). */
    if (!stream_needed) {
        return 0;
    }

    /* Otherwise, check if all the programs (variants) this playlist is in are
     * discarded. Since all streams in the playlist are part of the same programs
     * we can just check the programs of the first stream. */
    first_st = pls->main_streams[0]->index;

    for (i = 0; i < s->nb_programs; i++) {
        AVProgram *program = s->programs[i];
        if (program->discard < AVDISCARD_ALL) {
            for (j = 0; j < program->nb_stream_indexes; j++) {
                if (program->stream_index[j] == first_st) {
                    /* playlist is in an undiscarded program */
                    return 1;
                }
            }
        }
    }

    /* some streams were not discarded but all the programs were */
    return 0;
}

static int http_response_status(int err)
{
    int ret = 0;
    switch (err) {
        case AVERROR_HTTP_BAD_REQUEST:
        case AVERROR_HTTP_UNAUTHORIZED:
        case AVERROR_HTTP_FORBIDDEN:
        case AVERROR_HTTP_NOT_FOUND:
        case AVERROR_HTTP_OTHER_4XX:
            ret = AVERROR_HTTP_OTHER_4XX;
            break;
        case AVERROR_HTTP_SERVER_ERROR:
            ret = AVERROR_HTTP_SERVER_ERROR;
            break;
        case AVERROR_NOENT:
            ret = AVERROR_NOENT;
            break;
        case AVERROR_INVALIDDATA:
            ret = AVERROR_INVALIDDATA;
            break;
        case AVERROR_EXIT:
        case AVERROR_EOF:
            ret = AVERROR_EOF;
            break;
        default:
            break;
    }
    return ret;
}

static int read_data(void *opaque, uint8_t *buf, int buf_size)
{
    struct playlist *v = opaque;
    HLSContext *c = v->parent->priv_data;
    int ret = 0, just_opened = 0, reload_count = 0, http_err_count = 0;
    struct segment *seg;

restart:
    if (!v->needed) {
        gxlogd ("needed is 0, return eof eof eof,%d,[%s]\n",v->needed,v->url);
        return AVERROR_EOF;
    }

    if (c->is_switch_track && v->is_restart_needed) {
        return AVERROR_EOF;
    }
    if (!v->input || (c->http_persistent && v->input_read_done)) {
        int64_t reload_interval = 0;
        /* Check that the playlist is still needed before opening a new segment. */
        v->needed = playlist_needed(v);

        if (!v->needed) {
            gxlogd ("No longer receiving playlist %d.[%s]\n", v->index, v->url);
            return AVERROR_EOF;
        }

reload:
        reload_count++;/*ff max_reload default:50*/
        if (reload_count > HLS_MAX_RELOAD) {
            return AVERROR_EOF;
        }

        if ((!v->finished) &&
                (v->cur_seq_no >= v->start_seq_no + v->n_segments)) {
            if ((ret = parse_playlist(c, v->url, v, NULL, c->prog_now, 0)) < 0) {
                if ((ret = http_response_status(ret))) {
                    if (ret == AVERROR_EOF)
                        return AVERROR_EOF;
                    http_err_count+= 1;
                    if (http_err_count > 2) {
                        gxlogi ("Invalid request.server response:%s\n", av_err2str(ret));
                        return ret;
                    }
                    goto reload;
                } else {
                    gxlogi ("Failed to reload playlist.idx:%d.ret:%d[%s]..\n",v->index, ret, av_err2str(ret));
                }
            }
            http_err_count = 0;
            /* If we need to reload the playlist again below (if there's still no more segments), switch to a reload interval of half the target duration. */
            if (v->target_duration > (20* AV_TIME_BASE)) {
                reload_interval = 10*AV_TIME_BASE;//upload 10S.
            } else {
                reload_interval = (v->target_duration >= (8* AV_TIME_BASE))?(v->target_duration/2):(v->target_duration*4/5);
            }
        }

        if (v->cur_seq_no < v->start_seq_no) {
            if (v->finished) {
                v->cur_seq_no = v->start_seq_no;
            } else {
                if (c->live_start_index < 0)
                    v->cur_seq_no = v->start_seq_no + FFMAX(v->n_segments + c->live_start_index, 0);
                else
                    v->cur_seq_no = v->start_seq_no + FFMIN(c->live_start_index, v->n_segments - 1);
                v->cur_seq_no = FFMAX(0, v->cur_seq_no);
            }
        }
        if (v->cur_seq_no > v->last_seq_no) {
            v->last_seq_no = v->cur_seq_no;
            v->m3u8_hold_counters = 0;
        } else if (v->last_seq_no == v->cur_seq_no) {
            if ((!v->finished) && (v->cur_seq_no >= (v->start_seq_no + v->n_segments))) {
                v->m3u8_hold_counters++;
                if (v->m3u8_hold_counters > 2) {
                    if (c->live_start_index < 0)
                        v->cur_seq_no = v->start_seq_no + FFMAX(v->n_segments + c->live_start_index, 0);
                    else
                        v->cur_seq_no = v->start_seq_no + FFMIN(c->live_start_index, v->n_segments - 1);
                    v->cur_seq_no = FFMAX(0, v->cur_seq_no);
                }
            }
        } else {
            gxlogd ("maybe the m3u8 list sequence have been wraped.\n");
        }

        if (v->cur_seq_no >= (v->start_seq_no + v->n_segments)) {
            if (v->finished)
                return AVERROR_EOF;

            int64_t cur_time = gx_av_gettime_relative();
            if (cur_time >= v->last_load_time) {
                while (cur_time - v->last_load_time < reload_interval) {
                    if (url_check_interrupt_cb())
                        return AVERROR_EXIT;
                    GxCore_ThreadDelay(100);
                    cur_time = gx_av_gettime_relative();
                }
                goto reload;
            }
        }

        v->input_read_done = 0;
        seg = current_segment(v);
        if (!seg) {
            goto reload;
        }
        /* load/update Media Initialization Section, if any */
        ret = update_init_section(v, seg);
        if (ret) {
            return ret;
        }

        if (c->http_multiple == 1 && v->input_next_requested && c->is_switch_track) {
            FFSWAP(AVIOContext *, v->input, v->input_next);
            v->input_next_requested = 0;
            v->cur_seg_offset = 0;
            ret = 0;
        } else {
            ret = open_input(c, v, seg, &v->input, 1);
        }
        if (ret < 0) {
            if (url_check_interrupt_cb())
                return AVERROR_EXIT;
            if ((ret = http_response_status(ret))) {
                http_err_count+= 1;
                if (ret == AVERROR_EOF)
                    return AVERROR_EOF;
                if (http_err_count > 3) {
                    gxlogi ("Invalid request.server response:%s\n", av_err2str(ret));
                    return ret;
                }
            }
            v->cur_seq_no += 1;
            goto reload;
        } else {
            http_err_count = 0;
        }
        just_opened = 1;
    }

    seg = next_segment(v);
    if (c->http_multiple == 1
            && !v->input_next_requested
            && seg && seg->key_type == KEY_NONE
            && av_strstart(seg->url, "http", NULL)
            && !c->is_switch_track) {
        ret = open_input(c, v, seg, &v->input_next, 1);
        if (ret < 0) {
            if (url_check_interrupt_cb())
                return AVERROR_EXIT;
            gxlogd ("failed to open segment %lld of playlist %d\n", v->cur_seq_no + 1, v->index);
        } else {
            v->input_next_requested = 1;
        }
    }

    if (v->init_sec_buf_read_offset < v->init_sec_data_len) {
        /* Push init section out first before first actual segment */
        int copy_size = FFMIN(v->init_sec_data_len - v->init_sec_buf_read_offset, buf_size);
        memcpy(buf, v->init_sec_buf, copy_size);
        v->init_sec_buf_read_offset += copy_size;
        return copy_size;
    }

    seg = current_segment(v);
    if (!seg) {
        goto restart;
    }
    ret = read_from_url(v, seg, buf, buf_size, READ_NORMAL);
    if (ret > 0) {
        v->internal_http_read_size += ret;
        v->http_req_last_time = av_gettime();
        if (just_opened && v->is_id3_timestamped != 0) {
            /* Intercept ID3 tags here, elementary audio streams are required
             * to convey timestamps using them in the beginning of each segment. */
            intercept_id3(v, buf, buf_size, &ret);
        }
        return ret;
    } else {
        /* avio_read fail.retry 200*100ms,return AVERROR(ETIMEDOUT).restart http request*/
        v->http_req_end_time = av_gettime();
        if (AVERROR(ETIMEDOUT) == ret) {
            ff_format_io_close(v->parent, &v->input);
            c->cur_seq_no = v->cur_seq_no;
            v->input_read_done = 1;
            goto restart;
        }
    }
    if (!c->is_switch_track && c->http_persistent && (seg->key_type == KEY_NONE) && av_strstart(seg->url, "http", NULL)) {
        v->input_read_done = 1;
    } else {
        ff_format_io_close(v->parent, &v->input);
        v->is_restart_needed = 1;
    }
    v->cur_seq_no++;
    c->cur_seq_no = v->cur_seq_no;
    if (c->is_switch_track) {
        return ret;
    }
    goto restart;
}

static void add_renditions_to_variant(HLSContext *c, struct variant *var,
        enum AVMediaType type, const char *group_id)
{
    int i;

    for (i = 0; i < c->n_renditions; i++) {
        struct rendition *rend = c->renditions[i];

        if (rend->type == type && !strcmp(rend->group_id, group_id)) {

            if (rend->playlist)
                /* rendition is an external playlist
                 * => add the playlist to the variant */
                dynarray_add(&var->playlists, &var->n_playlists, rend->playlist);
            else
                /* rendition is part of the variant main Media Playlist
                 * => add the rendition to the main Media Playlist */
                dynarray_add(&var->playlists[0]->renditions,
                        &var->playlists[0]->n_renditions,
                        rend);
        }
    }
}

static void add_metadata_from_renditions(AVFormatContext *s, struct playlist *pls,
        enum AVMediaType type)
{
    int rend_idx = 0;
    int i;

    for (i = 0; i < pls->n_main_streams; i++) {
        AVStream *st = pls->main_streams[i];

        if (st->codec->codec_type != type)
            continue;

        for (; rend_idx < pls->n_renditions; rend_idx++) {
            struct rendition *rend = pls->renditions[rend_idx];

            if (rend->type != type)
                continue;

            if (rend->language[0]) {
                av_dict_set(&st->metadata, "language", rend->language, 0);
            }

            if (rend->name[0]) {
                av_dict_set(&st->metadata, "comment", rend->name, 0);
            }
            st->disposition |= rend->disposition;

            if (0 != st->disposition)
                av_dict_set(&st->metadata, "default_value", "yes", 0);
        }

        if (rend_idx >=pls->n_renditions)
            break;
    }
}

/* if timestamp was in valid range: returns 1 and sets seq_no
 * if not: returns 0 and sets seq_no to closest segment */
static int find_timestamp_in_playlist(HLSContext *c, struct playlist *pls,
        int64_t timestamp, int64_t *seq_no)
{
    int i;
    int64_t pos = c->first_timestamp == AV_NOPTS_VALUE ?
        0 : c->first_timestamp;

    if (timestamp < pos) {
        *seq_no = pls->start_seq_no;
        return 1;
    }

    for (i = 0; i < pls->n_segments; i++) {
        int64_t diff = pos + pls->segments[i]->duration - timestamp;
        if (diff > 0) {
            *seq_no = pls->start_seq_no + i;
            return 1;
        }
        pos += pls->segments[i]->duration;
    }

    *seq_no = pls->start_seq_no + pls->n_segments - 1;

    return 0;
}

static int64_t select_cur_seq_no(HLSContext *c, struct playlist *pls)
{
    int64_t seq_no = -1;

    if (!pls->finished && !c->first_packet &&
            (pls->cur_seq_no >= (pls->start_seq_no + pls->n_segments))) {
        /* reload the playlist since it was suspended */
        parse_playlist(c, pls->url, pls, NULL, c->prog_now, 0);
    }

    /* If playback is already in progress (we are just selecting a new
     * playlist) and this is a complete file, find the matching segment
     * by counting durations. */
    if (pls->finished && c->cur_timestamp != AV_NOPTS_VALUE) {
        find_timestamp_in_playlist(c, pls, c->cur_timestamp, &seq_no);
        return seq_no;
    }

    if (!pls->finished) {
        if (!c->first_packet && /* we are doing a segment selection during playback */
                c->cur_seq_no >= pls->start_seq_no &&
                c->cur_seq_no < pls->start_seq_no + pls->n_segments) {
            /* While spec 3.4.3 says that we cannot assume anything about the
             * content at the same sequence number on different playlists,
             * in practice this seems to work and doing it otherwise would
             * require us to download a segment to inspect its timestamps. */
            return c->cur_seq_no;
        }

        /* If this is a live stream, start live_start_index segments from the
         * start or end */
        if (c->live_start_index < 0)
            return pls->start_seq_no + FFMAX(pls->n_segments + c->live_start_index, 0);
        else
            return pls->start_seq_no + FFMIN(c->live_start_index, pls->n_segments - 1);
    }

    /* Otherwise just start on the first segment. */
    return pls->start_seq_no;
}

static void add_stream_to_programs(AVFormatContext *s, struct playlist *pls, AVStream *stream)
{
    HLSContext *c = s->priv_data;
    int i, j;
    int bandwidth = -1;

    for (i = 0; i < c->n_variants; i++) {
        struct variant *v = c->variants[i];

        for (j = 0; j < v->n_playlists; j++) {
            if (v->playlists[j] != pls)
                continue;

            av_program_add_stream_index(s, i, stream->index);

            if (bandwidth < 0)
                bandwidth = v->bandwidth;
            else if (bandwidth != v->bandwidth)
                bandwidth = -1; /* stream in multiple variants with different bandwidths */
        }
    }

    if (bandwidth >= 0)
        av_dict_set_int(&stream->metadata, "variant_bitrate", bandwidth, 0);
}

static int set_stream_info_from_input_stream(AVStream *st, struct playlist *pls, AVStream *ist)
{
    int err = 0;
    if ((err = avcodec_hls_context_copy(st->codec, ist->codec)) < 0) {
        return err;
    }
    if (ist->metadata) {
        av_dict_copy(&st->metadata, ist->metadata, 0);
        av_dict_free(&ist->metadata);
    }
    st->codec->is_reset_extradata = 1;
    if (pls->is_id3_timestamped) /* custom timestamps via id3 */
        avpriv_set_pts_info(st, 33, 1, MPEG_TIME_BASE);
    else
        avpriv_set_pts_info(st, ist->pts_wrap_bits, ist->time_base.num, ist->time_base.den);
    return err;
}

/* add new subdemuxer streams to our context, if any */
static int update_streams_from_subdemuxer(AVFormatContext *s, struct playlist *pls, int is_switch_bandwidth)
{
    int err = -1;
    if (!pls || !pls->ctx || (!pls->n_segments)) {
        return AVERROR_INVALIDDATA;
    }
    if (is_switch_bandwidth) {
        if (pls->ctx && pls->ctx->nb_streams > 0 && s->nb_streams > 0 && pls->n_main_streams < pls->ctx->nb_streams) {
            int i = 0, j = 0;
            AVStream *st, *ist;
            for (i = 0; i < s->nb_streams; i++) {
                for (j = 0; j < pls->ctx->nb_streams; j++) {
                    st = s->streams[i];
                    ist = pls->ctx->streams[j];
                    if (st->codec->codec_type == ist->codec->codec_type) {
                        st->id = pls->index;
                        dynarray_add(&pls->main_streams, &pls->n_main_streams, st);
                        add_stream_to_programs(s, pls, st);
                        err = set_stream_info_from_input_stream(st, pls, ist);
                        if (err < 0)
                            return err;
                        break;
                    }
                }
            }
        }
    } else {
        while (pls->ctx && pls->n_main_streams < pls->ctx->nb_streams) {
            int ist_idx = 0;
            AVStream *st, *ist;
            ist_idx = pls->n_main_streams;
            st = avformat_new_stream(s, NULL);
            if (!st) {
                return AVERROR(ENOMEM);
            }
            ist = pls->ctx->streams[ist_idx];
            st->id = pls->index;
            dynarray_add(&pls->main_streams, &pls->n_main_streams, st);
            add_stream_to_programs(s, pls, st);
            err = set_stream_info_from_input_stream(st, pls, ist);
            if (err < 0)
                return err;
        }
    }
    return 0;
}

static void update_noheader_flag(AVFormatContext *s)
{
    HLSContext *c = s->priv_data;
    int flag_needed = 0;
    int i;

    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (!pls)
            continue;
        if (pls->has_noheader_flag) {
            flag_needed = 1;
            break;
        }
    }

    if (flag_needed)
        s->ctx_flags |= AVFMTCTX_NOHEADER;
    else
        s->ctx_flags &= ~AVFMTCTX_NOHEADER;
}

static int hls_close(AVFormatContext *s)
{
    HLSContext *c = s->priv_data;
    int is_on_abr = 0;

    GxPlayer_SystemGet(PSYS_NETWORK_HTTP_AUTO_ADAPABILITY, &is_on_abr);
    if (!c) {
        return 0;
    }
    if (is_on_abr && c->n_playlists > 1 && c->n_variants > 1) {
        GxCore_ThreadJoin(c->abr_thread);
        close_hls_abr(c->video_abr);
    }
    free_playlist_list(c);
    free_variant_list(c);
    free_rendition_list(c);
    if (c->crypto_ctx.aes_ctx)
        av_free(c->crypto_ctx.aes_ctx);
    av_dict_free(&c->avio_opts);
    ff_format_io_close(c->ctx, &c->playlist_pb);
    return 0;
}

static int open_input_streams(AVFormatContext *s, int i, int64_t highest_cur_seq_no, int *ret, int cur_seq_no, int is_switch_bandwidth)
{
    HLSContext *c = s->priv_data;
    struct playlist *pls = c->playlists[i];
    AVInputFormat *in_fmt = NULL;
    AVDictionaryEntry *opts = NULL;
    struct segment *seg = NULL;
    char *url = NULL;
    int initial_buffer_size = is_system_large_memory()?(64*1024):(32*1024);

    if (!(pls->ctx = avformat_alloc_context())) {
        *ret = AVERROR_NOMEM;
        return -1;
    }

    if (pls->n_segments == 0) {
        return 0;
    }

    pls->index	= i;
    pls->needed = 1;
    pls->parent = s;

    /*
     * If this is a live stream and this playlist looks like it is one segment
     * behind, try to sync it up so that every substream starts at the same
     * time position (so e.g. avformat_find_stream_info() will see packets from
     * all active streams within the first few seconds). This is not very generic,
     * though, as the sequence numbers are technically independent.
     */
    if (!pls->finished && pls->cur_seq_no == highest_cur_seq_no - 1 &&
            highest_cur_seq_no < pls->start_seq_no + pls->n_segments) {
        pls->cur_seq_no = highest_cur_seq_no;
    }

    pls->read_buffer = av_malloc(initial_buffer_size);
    if (!pls->read_buffer){
        *ret = AVERROR_NOMEM;
        avformat_free_context(pls->ctx);
        pls->ctx = NULL;
        return -1;
    }
    if ((opts = av_dict_get(s->url_options, "avioflags", NULL, 0))) {
        pls->ctx->avio_flags = atoi(opts->value);
    }
    pls->ctx->file_format = s->file_format;

    ffio_init_context(&pls->pb, pls->read_buffer, initial_buffer_size, pls->ctx->avio_flags & AVIO_FLAG_WRITE, pls, read_data, NULL, NULL);
    /* If encryption scheme is SAMPLE-AES, try to read	ID3 tags of  external audio track that contains audio setup information */
    seg = current_segment(pls);
    if (seg && seg->key_type == KEY_SAMPLE_AES && pls->n_renditions > 0 &&
            pls->renditions[0]->type == AVMEDIA_TYPE_AUDIO) {
        uint8_t buf[HLS_MAX_ID3_TAGS_DATA_LEN];
        if ((*ret = avio_read(&pls->pb, buf, HLS_MAX_ID3_TAGS_DATA_LEN)) < 0) {
            /* Fail if error was not end of file */
            if (*ret != AVERROR_EOF) {
                avformat_free_context(pls->ctx);
                pls->ctx = NULL;
                return *ret;
            }
        }
        *ret = 0;
        /* Reset reading */
        ff_format_io_close(pls->parent, &pls->input);
        pls->input = NULL;
        pls->input_read_done = 0;
        ff_format_io_close(pls->parent, &pls->input_next);
        pls->input_next = NULL;
        pls->input_next_requested = 0;
        pls->cur_seg_offset = 0;
        pls->cur_init_section = NULL;
        /* Reset EOF flag */
        pls->pb.eof_reached = 0;
        /* Clear any buffered data */
        pls->pb.buf_end = pls->pb.buf_ptr = pls->pb.buffer;
        /* Reset the position */
        pls->pb.pos = 0;
    }

    /*
     * If encryption scheme is SAMPLE-AES and audio setup information is present in external audio track,
     * use that information to find the media format, otherwise probe input data
     */
    seg = current_segment(pls);
    if (seg && seg->key_type == KEY_SAMPLE_AES && pls && pls->is_id3_timestamped &&
            pls->audio_setup_info.codec_id != AV_CODEC_ID_NONE) {
        // Keep this list in sync with ff_hls_senc_read_audio_setup_info()
        in_fmt = av_find_input_format(pls->audio_setup_info.codec_id == AV_CODEC_ID_AAC ? "aac" :
                pls->audio_setup_info.codec_id == AV_CODEC_ID_AC3 ? "ac3" : "eac3");
    } else {
        pls->pb.seekable = 0;
        pls->ctx->flags = AVFMT_FLAG_CUSTOM_IO;
        pls->ctx->probesize = 1024*32;
        pls->ctx->max_analyze_duration = 4*AV_TIME_BASE;
        if ((opts = av_dict_get(s->url_options, "ffprobesize", NULL, 0))) {
            pls->ctx->probesize = FFMAX(atoi(opts->value), 32*1024);
        }
        if ((opts = av_dict_get(s->url_options, "analyzeduration", NULL, 0))) {
            pls->ctx->max_analyze_duration = FFMAX((atoi(opts->value)*AV_TIME_BASE), (4*AV_TIME_BASE));
        }
        pls->ctx->index_limit = s->index_limit;
        url = av_strdup(pls->segments[cur_seq_no]->url);
        if (!url) {
            *ret = AVERROR_NOMEM;
            avformat_free_context(pls->ctx);
            pls->ctx = NULL;
            return -1;
        }
        *ret = av_probe_input_buffer(&pls->pb, &in_fmt, url, NULL, 0, 0);
        if (*ret < 0) {
            /* Free the ctx - it isn't initialized properly at this point,
             * so avformat_close_input shouldn't be called. If
             * avformat_open_input fails below, it frees and zeros the
             * context, so it doesn't need any special treatment like this. */
            gxloge("Error when loading first segment..ret:%d.err:[%s]..\n", *ret, av_err2str(*ret));
            avformat_free_context(pls->ctx);
            pls->ctx = NULL;
            av_free(url);
            return -1;
        }
    }

    seg = current_segment(pls);
    if (seg && seg->key_type == KEY_SAMPLE_AES) {
        if (strstr(in_fmt->name, "mov")) {
            char key[33];
            ff_data_to_hex(key, pls->key, sizeof(pls->key), 0);
            key[32] = '\0';
            av_dict_set(&s->url_options, "decryption_key", key, 0);
        } else if (!c->crypto_ctx.aes_ctx) {
            c->crypto_ctx.aes_ctx = av_aes_alloc();
            if (!c->crypto_ctx.aes_ctx) {
                avformat_free_context(pls->ctx);
                pls->ctx = NULL;
                return AVERROR(ENOMEM);
            }
        }
    }

    pls->ctx->pb      = &pls->pb;
    pls->ctx->flags   |= s->flags & ~AVFMT_FLAG_CUSTOM_IO;
    pls->ctx->file_format = s->file_format;
    *ret = avformat_open_input(&pls->ctx, url, in_fmt, &s->url_options);
    if (*ret < 0) {
        gxloge ("hls.c open input fail.ret:%d.err:(%s).\n", *ret, av_err2str(*ret));
        av_free(url);
        return -1;
    }
    av_free(url);

    if (pls->id3_deferred_extra && pls->ctx->nb_streams == 1) {
        ff_id3v2_parse_apic(pls->ctx, &pls->id3_deferred_extra);
        ff_id3v2_free_extra_meta(&pls->id3_deferred_extra);
    }

    if (pls->is_id3_timestamped == -1)
        gxlogd("No expected HTTP requests have been made\n");

    /*
     * For ID3 timestamped raw audio streams we need to detect the packet
     * durations to calculate timestamps in fill_timing_for_id3_timestamped_stream(),
     * but for other streams we can rely on our user calling avformat_find_stream_info()
     * on us if they want to.
     */
    if (pls->is_id3_timestamped || (pls->n_renditions > 0 && pls->renditions[0]->type == AVMEDIA_TYPE_AUDIO)) {
        seg = current_segment(pls);
        if (seg && seg->key_type == KEY_SAMPLE_AES && pls->audio_setup_info.setup_data_length > 0 &&
                pls->ctx->nb_streams == 1) {
            *ret = ff_hls_senc_parse_audio_setup_info(pls->ctx->streams[0], &pls->audio_setup_info);
        } else {
            *ret = avformat_find_stream_info(pls->ctx, NULL);
        }
        if (*ret < 0) {
            gxloge ("hls.c find stream info fail.%d(%s).\n", *ret, av_err2str(*ret));
            return *ret;
        }
    }
    pls->has_noheader_flag = !!(pls->ctx->ctx_flags & AVFMTCTX_NOHEADER);

    /* Create new AVStreams for each stream in this playlist */
    *ret = update_streams_from_subdemuxer(s, pls, is_switch_bandwidth);
    if (*ret < 0) {
        return -1;
    }

    /* Copy any metadata from playlist to main streams, but do not set event flags. */
    if (pls->n_main_streams)
        av_dict_copy(&pls->main_streams[cur_seq_no]->metadata, pls->ctx->metadata, 0);

    add_metadata_from_renditions(s, pls, AVMEDIA_TYPE_AUDIO);
    add_metadata_from_renditions(s, pls, AVMEDIA_TYPE_VIDEO);
    add_metadata_from_renditions(s, pls, AVMEDIA_TYPE_SUBTITLE);

    return 1;
}

static void close_cur_playlists_list_info(AVFormatContext *s, int cur_pls_idx)
{
    int i = 0, j = 0;
    HLSContext *c = s->priv_data;

    struct playlist *pls = c->playlists[cur_pls_idx];
    if (!pls || !pls->ctx || (!pls->n_segments) || (cur_pls_idx >= c->n_playlists)) {
        return;
    }
    free_segment_list(pls);
    pls->n_segments = 0;
    pls->finished = 0;
    pls->type = PLS_TYPE_UNSPECIFIED;
    free_init_section_list(pls);
    pls->n_init_sections = 0;
    for (j = 0; j < pls->n_main_streams; j++) {
        AVStream *st = pls->main_streams[j];
        if (st)
            av_dict_free(&st->metadata);
    }
    pls->n_main_streams = 0;
    if (pls->renditions) {
        av_freep(&pls->renditions);
    }
    if (pls->id3_buf) {
        av_free(pls->id3_buf);
        pls->id3_buf = NULL;
    }
    if (pls->id3_initial) {
        av_dict_free(&pls->id3_initial);
    }
    if (pls->id3_deferred_extra)
        ff_id3v2_free_extra_meta(&pls->id3_deferred_extra);
    if (pls->init_sec_buf) {
        av_free(pls->init_sec_buf);
        pls->init_sec_buf = NULL;
    }
    av_free_packet(&pls->pkt);
    if (pls->pb.buffer)
        av_freep(&pls->pb.buffer);
    if (pls->input)
        ff_format_io_close(c->ctx, &pls->input);
    pls->input_read_done = 0;
    if (pls->input_next)
        ff_format_io_close(c->ctx, &pls->input_next);
    pls->input_next_requested = 0;
    if (pls->ctx) {
        pls->ctx->pb = NULL;
        avformat_close_input(&pls->ctx);
    }
    ff_format_io_close(c->ctx, &c->playlist_pb);
    pls->is_restart_needed = 0;
}

static int recheck_hls_is_mulit_audio_track(AVFormatContext *s)
{
    int i = 0, audio_totol = 0;
    HLSContext *c = s->priv_data;
    if (!c) {
        return 0;
    }
    if (c->n_variants > 1) {
        return 0;
    }
    if (c->n_renditions <= 1 && c->renditions && c->renditions[0]->disposition <= 0)
        return 0;

    for (i = 0; i < c->n_renditions; i++) {
        if (c->renditions && AVMEDIA_TYPE_AUDIO == c->renditions[i]->type)
            audio_totol += 1;
    }
    return ((audio_totol > 0)?1:0);
}

int get_hls_hight_resolution_bandwidth_value(HLSContext *c)
{
    int i  =0, max_bandwidth = -1, second_max_bandwidth = -1, third_max_bandwidth = -1, select_bandwidth = 0, set_stream_clarity = 0;

    if (c->n_variants <= 1) {
        gxlogi_raw_l ("not a multi-bandwidth stream.\n");
        return AVERROR_NOTSUPP;
    }

    for (i = 0; i < c->n_variants; i++) {
        struct variant *v = c->variants[i];
        if (!v)
            continue;
        int cur_bandwidth = v->bandwidth;
        if (auto_filter_decoder_unsupporrt_type(NULL, v)) {
            continue;
        }
        if (cur_bandwidth > max_bandwidth) {
            third_max_bandwidth = second_max_bandwidth;
            second_max_bandwidth = max_bandwidth;
            max_bandwidth = cur_bandwidth;
        } else if ((cur_bandwidth > second_max_bandwidth) && (cur_bandwidth != max_bandwidth)) {
            third_max_bandwidth = second_max_bandwidth;
            second_max_bandwidth = cur_bandwidth;
        } else if ((cur_bandwidth > third_max_bandwidth) && (cur_bandwidth != second_max_bandwidth) && (cur_bandwidth != max_bandwidth)) {
            third_max_bandwidth = cur_bandwidth;
        }
    }

    GxPlayer_SystemGet(PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX, &set_stream_clarity);
    if (set_stream_clarity == 1) {/*Maximum resolution*/
        select_bandwidth = (max_bandwidth != -1)?max_bandwidth:0;
    } else if (set_stream_clarity == 2) {/*Second highest resolution*/
        select_bandwidth = (second_max_bandwidth != -1)?second_max_bandwidth:((max_bandwidth != -1)?max_bandwidth:0);
    } else {
        if (c->n_variants <= 3)/*When you only have three bandwidth, you take the second_max_bandwidth.*/
            select_bandwidth = (second_max_bandwidth != -1)?second_max_bandwidth:((max_bandwidth != -1)?max_bandwidth:0);
        else
            select_bandwidth = (third_max_bandwidth != -1)?third_max_bandwidth:((second_max_bandwidth != -1)?second_max_bandwidth:((max_bandwidth != -1)?max_bandwidth:0));
    }
    return select_bandwidth;
}

int get_hls_low_resolution_bandwidth_value(HLSContext *c)
{
    int i  =0, first_min_bandwidth = INT_MAX, second_min_bandwidth = INT_MAX, third_min_bandwidth = INT_MAX, select_bandwidth = 0, set_stream_clarity = 0;

    if (c->n_variants <= 1) {
        gxlogi ("not a multi-bandwidth stream.\n");
        return AVERROR_NOTSUPP;
    }

    for (i = 0; i < c->n_variants; i++) {
        struct variant *v = c->variants[i];
        if (!v)
            continue;
        int cur_bandwidth = v->bandwidth;
        if (auto_filter_decoder_unsupporrt_type(NULL, v)) {
            continue;
        }
        if (cur_bandwidth < first_min_bandwidth) {
            third_min_bandwidth = second_min_bandwidth;
            second_min_bandwidth = first_min_bandwidth;
            first_min_bandwidth = cur_bandwidth;
        } else if (cur_bandwidth < second_min_bandwidth && cur_bandwidth != first_min_bandwidth) {
            third_min_bandwidth = second_min_bandwidth;
            second_min_bandwidth = cur_bandwidth;
        } else if (cur_bandwidth < third_min_bandwidth && cur_bandwidth != second_min_bandwidth && cur_bandwidth != first_min_bandwidth) {
            third_min_bandwidth = cur_bandwidth;
        }
    }

    GxPlayer_SystemGet(PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX, &set_stream_clarity);
    if (set_stream_clarity == -1) {/*first mini bandwidth.*/
        select_bandwidth = (first_min_bandwidth != INT_MAX)?first_min_bandwidth:0;
    } else if (set_stream_clarity == -2) {/*second mini bandwidth.*/
        select_bandwidth = (second_min_bandwidth != INT_MAX)?second_min_bandwidth:((first_min_bandwidth != INT_MAX)?first_min_bandwidth:0);
    } else {
        if (c->n_variants <= 3)/*When you only have three bandwidth, you take the second_max_bandwidth.*/
            select_bandwidth = (second_min_bandwidth != INT_MAX)?second_min_bandwidth:((first_min_bandwidth != INT_MAX)?first_min_bandwidth:0);
        else
            select_bandwidth = (third_min_bandwidth != INT_MAX)?third_min_bandwidth:((second_min_bandwidth != INT_MAX)?second_min_bandwidth:((first_min_bandwidth != INT_MAX)?first_min_bandwidth:0));
    }
    return select_bandwidth;
}

int get_set_player_hls_multi_bandwidth_index(HLSContext *c)
{
    int i  =0, index = -1, max_bandwidth = -1, second_max_bandwidth = -1, third_max_bandwidth = -1, select_bandwidth = 0, set_stream_clarity = 0;

    if (c->n_variants <= 1) {
        gxlogi ("not a multi-bandwidth stream.\n");
        return AVERROR_NOTSUPP;
    }

    GxPlayer_SystemGet(PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX, &set_stream_clarity);
    if (set_stream_clarity < 0)/*get low resolution value*/
        select_bandwidth = get_hls_low_resolution_bandwidth_value(c);
    else if (set_stream_clarity > 0)/*get hight resolution value*/
        select_bandwidth = get_hls_hight_resolution_bandwidth_value(c);
    /*get bandwidth index.*/
    for (i = 0; i < c->n_variants; i++) {
        if (c->variants[i]->bandwidth > 0 && c->variants[i]->bandwidth == select_bandwidth) {
            index = i;
            break;
        }
    }
    return index;
}

static int get_hls_audio_prog_total(AVFormatContext *s, HLSContext *c, int flg)
{
    int i = 0, total = 0;
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        int j = 0;
        for (j = 0; j < pls->n_main_streams; j++) {
            AVStream *st = pls->main_streams[j];
            int rend_idx = 0;
            if (st->codec->codec_type != AVMEDIA_TYPE_AUDIO)
                continue;
            for (rend_idx = 0; rend_idx < pls->n_renditions; rend_idx++) {
                struct rendition *rend = pls->renditions[rend_idx];
                if (rend->type != AVMEDIA_TYPE_AUDIO)
                    continue;
                if (flg) {
                    if (rend->group_id[0]) {
                        av_dict_set(&(s->url_options), "aud_id", rend->group_id, AV_DICT_MULTIKEY);
                        av_dict_set(&(s->url_options), "aud_codecs", rend->group_id, AV_DICT_MULTIKEY);
                    }
                    if (rend->language[0]) {
                        av_dict_set(&(s->url_options), "aud_language", rend->name, AV_DICT_MULTIKEY);
                    }
                }
                total+= 1;
            }
            if (rend_idx >= pls->n_renditions)
                break;
        }
    }
    return total;
}

static void send_hls_info_to_demux_lavf_open(AVFormatContext *s, HLSContext *c)
{
    int i = 0, total = 0;
    if (c->n_variants > 1) {
        av_dict_set_int(&(s->url_options), "vid_stream_index", c->prog_now, 0);//hls mulit bandwidth current program index.
        av_dict_set_int(&(s->url_options), "vid_variant_bitrate_max", c->n_variants, 0);//hls mulit bandwidth total number..
        av_dict_set_int(&(s->url_options), "dash_cur_periods", 0, 0);
        for (i = 0; i < c->n_variants; i++) {
            struct variant *v = c->variants[i];
            char *vid_width = NULL, *vid_height = NULL;
            if (!v)
                continue;
            av_dict_set(&(s->url_options), "vid_id", "ffhls_bandwidth", AV_DICT_MULTIKEY);//hls bandwitdh.
            av_dict_set_int(&(s->url_options), "vid_variant_bitrate", v->bandwidth, AV_DICT_MULTIKEY);//hls bandwitdh.
            av_dict_set(&(s->url_options), "vid_codecs", v->codecs, AV_DICT_MULTIKEY);//mulit bandwidth hls link is AV codec.
            vid_width = av_strtok(v->resolution, "x", &vid_height);
            if (vid_height) {
                av_dict_set(&(s->url_options), "vid_width", vid_width, AV_DICT_MULTIKEY);//mulit bandwidth hls link is width.
                av_dict_set(&(s->url_options), "vid_height", vid_height, AV_DICT_MULTIKEY);//mulit bandwidth hls link is AV height.
            }
        }
    }
    total = get_hls_audio_prog_total(s, c, 0);
    if (total > 0)
        av_dict_set_int(&(s->url_options), "audio_prog_max", total, AV_DICT_MULTIKEY);
    get_hls_audio_prog_total(s, c, 1);
}

static int get_cur_stream_media_type(AVFormatContext *ctx)
{
    int i = 0, media_type = 0;
    for (i = 0; i < ctx->nb_streams; i++) {
        AVStream *streams=ctx->streams[i];
        if (!streams)
            continue;
        switch (streams->codec->codec_type) {
            case CODEC_TYPE_VIDEO:
                media_type |= MEDIA_TYPE_VIDEO;
                break;
            case CODEC_TYPE_AUDIO:
                media_type |= MEDIA_TYPE_AUDIO;
                break;
            default:
                break;
        }
    }
    return media_type;
}

static void check_streams_av_track_is_separate(AVFormatContext *s)
{
    int i = 0, av_separate = 0, media_type = 0;
    HLSContext *c = s->priv_data;

    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (!pls || !pls->ctx) {
            av_separate = 0;
            continue;
        }
        if (get_cur_stream_media_type(pls->ctx) == (MEDIA_TYPE_VIDEO|MEDIA_TYPE_AUDIO)) { // video + audio
            av_separate = 0;
            break;
        }
        av_separate = 1;
    }
    s->is_hls_av_separate = av_separate?1:0;
}

static int hls_copy_url_options(AVFormatContext *s, HLSContext *c, AVDictionaryEntry *e)
{
    int ret = 0, http_multiple = 0;
    HLS_AV_DICT_GET(s, e, c->user_agent, "user_agent");
    HLS_AV_DICT_GET(s, e, c->Authorization, "Authorization");
    HLS_AV_DICT_GET(s, e, c->CustomHeaders, "CustomHeaders");
    HLS_AV_DICT_GET(s, e, c->socks5_proxy, "socks5_proxy");
    HLS_AV_DICT_GET(s, e, c->referer, "referer");
    HLS_AV_DICT_GET(s, e, c->cookies, "ffcookies");
    HLS_AV_DICT_GET(s, e, c->headers, "headers");
    HLS_AV_DICT_GET(s, e, c->http_proxy, "http_proxy");
    HLS_AV_DICT_GET(s, e, c->avio_flags, "avioflags");
    if ((e = av_dict_get(s->url_options, "live_start_index", NULL, 0))) {
        c->live_start_index = atoi(e->value);
    }
    if ((e = av_dict_get(s->url_options, "mulit_bandwidth_prog_now", NULL, 0))) {
        c->prog_now = atoi(e->value);
    }
    GxPlayer_SystemGet(PSYS_NETWORK_HLS_MULTIPLE_LIST, &http_multiple);
    if (http_multiple && (e = av_dict_get(s->url_options, "http_version", NULL, 0))) {
        c->http_multiple = (!strncmp((const char *)e->value, "1.1", 3) || !strncmp((const char *)e->value, "2.0", 3));
    }
    if ((e = av_dict_get(s->url_options, "location", NULL, 0))) {
        if (s->url) {
            av_free(s->url);
            s->url = NULL;
        }
        if (!(s->url = av_strdup(e->value))) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
    }
    if ((e = av_dict_get(s->url_options, "ssl_cipher_list", NULL, 0)) && e->value) {
        av_dict_set(&c->avio_opts, "ssl_cipher_list", e->value, 0);
    }
    /* Some HLS servers don't like being sent the range header */
    if ((e = av_dict_get(s->url_options, "http_seekable", NULL, 0))) {
        c->http_seekable = atoi(e->value);
    }
    av_dict_set_int(&c->avio_opts, "seekable", c->http_seekable, 0);
fail:
    return ret;
}

static void get_hls_duration(AVFormatContext *s, HLSContext *c, int prog_now)
{
    int i = 0;
    c->prog_now = (c->prog_now < 0)?prog_now:c->prog_now;
    if (c->n_variants > 0) {
        for (i = 0; i < c->n_variants; i++) {
            if (c->variants[i]->playlists[0]->n_segments == 0) {
                c->variants[i]->playlists[0]->broken = 1;
            }
            if (c->variants[i]->playlists[0]->finished) {
                int64_t duration = 0;
                int j = 0;
                for (j = 0; j < c->variants[i]->playlists[0]->n_segments; j++)
                    duration += c->variants[i]->playlists[0]->segments[j]->duration;
                s->duration = duration;
            }
        }
    }
}

static int hls_context_initialization(AVFormatContext *s)
{
    int use_http_persistent_connections = 0, ret = 0;
    HLSContext *c = s->priv_data;
    AVDictionaryEntry *e = NULL;

    GxPlayer_SystemGet(PSYS_NETWORK_HTTP_PERSISTENT_CONNECTIONS, &use_http_persistent_connections);

    c->ctx                = s;
    c->first_packet = 1;
    c->first_timestamp = AV_NOPTS_VALUE;
    c->cur_timestamp = AV_NOPTS_VALUE;
    c->http_multiple = 0;
    c->http_persistent = use_http_persistent_connections?1:0;
    c->cur_m3u8_idx = -1;
    c->prog_now = -1;
    c->live_start_index = -3;
    c->http_seekable = 1;
    c->is_in_reopen = 0;

    if ((ret = hls_copy_url_options(s, c, e)) < 0) {
        return ret;
    }
    return 0;
}

static int check_support_customer_specified_resolution(HLSContext *c)
{
    int i, restup_w = 0, restup_h = 0, select_bandwidth = -1, cur_bandwidth_idx = -1;

    GxPlayer_SystemGet(PSYS_NETWORK_RESOLUTION_WIDTH, &restup_w);
    GxPlayer_SystemGet(PSYS_NETWORK_RESOLUTION_HEIGHT, &restup_h);

    if (!c || (c->n_variants <= 1) || (restup_w <= 0) || (restup_h <= 0 )||(restup_w>1920) || (restup_h>1080)) {
        return select_bandwidth;
    }

    for (i = 0; i < c->n_variants; i++) {
        char resolution[MAX_FIELD_LEN+1] = {0};
        char *vid_width = NULL, *vid_height = NULL;
        struct variant *v = c->variants[i];

        if (v) {
            av_strlcpy(resolution, v->resolution, sizeof(resolution));
            vid_width = av_strtok(resolution, "x", &vid_height);
            if ((vid_width && (atoi(vid_width) == restup_w)) && (vid_height && (atoi(vid_height) == restup_h))) {
                select_bandwidth = v->bandwidth;
                break;
            }
        }
    }

    if (i >= c->n_variants)
        return select_bandwidth;

    /*get bandwidth index.*/
    for (i = 0; i < c->n_variants; i++) {
        if (c->variants[i]->bandwidth > 0 && c->variants[i]->bandwidth == select_bandwidth) {
            cur_bandwidth_idx = i;
            break;
        }
    }
    return cur_bandwidth_idx;
}

static int check_hls_is_av_split_streams(HLSContext *c)
{
    int i = 0, has_audio = 0, has_video = 0;
    if (!c || c->n_playlists <= 1)
        return 0;
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (c->n_playlists > 1 && pls->n_renditions && pls->renditions[0] && !pls->n_variants) {
            has_audio = 1;
        } else if (c->n_playlists > 1 && pls->n_variants && !pls->n_renditions) {
            has_video = 1;
        }
    }
    c->is_av_split_streams = (has_audio & has_video)?1:0;
    return 1;
}
static int get_mulit_bandwidth_hls_optimal_route(AVFormatContext *s, int *prog_now)
{
    int ret = 0, i = 0, sup_mx_playlist =0, cur_bandwidth_idx = -1;
    HLSContext *c = s->priv_data;

    check_hls_is_av_split_streams(c);
    cur_bandwidth_idx = check_support_customer_specified_resolution(c);
    if (cur_bandwidth_idx < 0)
        cur_bandwidth_idx = get_set_player_hls_multi_bandwidth_index(c);
    GxPlayer_SystemGet(PSYS_NETWORK_FFHLS_MAX_PLAYLIST, &sup_mx_playlist);
    /* If the playlist only contained playlists (Master Playlist), parse each individual playlist. */
    if (c->n_playlists > 1 || c->playlists[0]->n_segments == 0) {
        int variant_idx = -1, cur_renditions_playlist_num = 0, cur_variant_playlist_num = 0, disposable = 1;
        sup_mx_playlist = FFMIN(sup_mx_playlist, c->n_playlists);
        for (i = 0; i < c->n_playlists; i++) {
            struct playlist *pls = c->playlists[i];
            if (c->n_playlists > 1 && pls->n_renditions && pls->renditions[0] && !pls->n_variants) {
                if (cur_renditions_playlist_num >= sup_mx_playlist/2) {
                    continue;
                }
                cur_renditions_playlist_num += 1;
            } else if (c->n_playlists > 1 && pls->n_variants && !pls->n_renditions) {
                variant_idx += 1;
                if (cur_variant_playlist_num >= sup_mx_playlist/2) {
                    continue;
                }
                /*compose bandwidth.is switch banwidth index.*/
                if (c->prog_now >= 0 && c->prog_now != variant_idx) {
                    continue;
                }
                /*compose bandwidth.is second max bandwidth index.*/
                if (c->prog_now < 0 && cur_bandwidth_idx >= 0 && cur_bandwidth_idx != variant_idx) {
                    continue;
                }
                /*curent bandwidth variants is support.*/
                if (c->n_variants > 1 && c->prog_now < 0 && cur_bandwidth_idx < 0) {
                    struct variant *v = c->variants[variant_idx];
                    if (auto_filter_decoder_unsupporrt_type(NULL, v)) {
                        continue;
                    }
                }
                cur_variant_playlist_num += 1;
                *prog_now = variant_idx;
            } else {
                if (pls->n_variants && !pls->n_renditions)
                    *prog_now = i;
            }
            if ((ret = parse_playlist(c, pls->url, pls, NULL, variant_idx, 1)) < 0) {
                if (AVERROR(ENOMEM)==ret)
                    goto fail;
                pls->broken = 1;
                if (pls->n_renditions && pls->renditions[0] && !pls->n_variants) {
                    cur_renditions_playlist_num -= 1;
                } else if (pls->n_variants && !pls->n_renditions) {
                    cur_variant_playlist_num -= 1;
                    cur_bandwidth_idx = variant_idx + 1;
                }
                if (c->n_playlists > 1 &&
                (AVERROR_HTTP_OTHER_4XX == http_response_status(ret) || AVERROR_HTTP_SERVER_ERROR == http_response_status(ret))) {
                    if (disposable) {
                        i = -1;
                        variant_idx = -1;
                        disposable = 0;
                        cur_variant_playlist_num = 0;
                        cur_renditions_playlist_num = 0;
                        cur_bandwidth_idx = variant_idx + 1;
                    }
                    continue;
                }
                goto fail;
            }
        }
    }
fail:
    return ret;
}

static int open_streams(AVFormatContext *s)
{
    int ret = 0, i = 0, highest_cur_seq_no = 0;
    HLSContext *c = s->priv_data;

    /* Select the starting segments */
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (!pls->n_segments)
            continue;
        pls->cur_seq_no = select_cur_seq_no(c, pls);
        highest_cur_seq_no = FFMAX(highest_cur_seq_no, pls->cur_seq_no);
    }

    /* Open the demuxer for each playlist */
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        int error = 0;
        if (!pls || !pls->n_segments) {
            continue;
        }
        ret = open_input_streams(s, i, highest_cur_seq_no, &error, 0, 0);
        if (-1 == ret) {
            ret = error;
            break;
        } else if (0 == ret) {
            continue;
        }
    }
    return ret;
}

static int hls_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    int ret = 0, i = 0, prog_now = 0, is_on_abr = 0;
    int64_t highest_cur_seq_no = 0;
    HLSContext *c = s->priv_data;

    if ((ret = hls_context_initialization(s)) < 0) {
        goto fail;
    }

    if ((ret = parse_playlist(c, s->url, NULL, s->pb, prog_now, 0)) < 0) {
        goto fail;
    }

    if (c && (c->n_variants <= 0)) {
        gxloge_raw_l ("Empty playlist.c->n_variants is %d.\n", c->n_variants);
        ret = AVERROR_EOF;
        goto fail;
    }

    if ((ret = get_mulit_bandwidth_hls_optimal_route(s, &prog_now)) < 0) {
        goto fail;
    }

    get_hls_duration(s, c, prog_now);

    /* Associate renditions with variants */
    for (i = 0; i < c->n_variants; i++) {
        struct variant *var = c->variants[i];
        AVProgram *program;

        if (!var)
            continue;
        program = av_new_program(s, i);
        if (!program)
            goto fail;

        if (var->audio_group[0])
            add_renditions_to_variant(c, var, AVMEDIA_TYPE_AUDIO, var->audio_group);
        if (var->video_group[0])
            add_renditions_to_variant(c, var, AVMEDIA_TYPE_VIDEO, var->video_group);
        if (var->subtitles_group[0])
            add_renditions_to_variant(c, var, AVMEDIA_TYPE_SUBTITLE, var->subtitles_group);
        av_dict_set_int(&program->metadata, "variant_bitrate", var->bandwidth, 0);
    }

    if ((ret = open_streams(s)) < 0) {
        goto fail;
    }

    GxPlayer_SystemGet(PSYS_NETWORK_HTTP_AUTO_ADAPABILITY, &is_on_abr);
    if (is_on_abr && c->n_playlists > 1 && c->n_variants > 1) {
        if ((ret = init_hls_abr(&(c->video_abr))) < 0)
            return ret;
        ret = GxCore_ThreadCreate("hls abr", &c->abr_thread, hls_abr_thread, s, 32*1024, GXOS_DEFAULT_PRIORITY+1);
        if (ret != 0) {
            gxloge ("GxCore_ThreadCreate failed : %s\n", strerror(ret));
            return AVERROR(ret);
        }
    }

    if (recheck_hls_is_mulit_audio_track(s))
        check_streams_av_track_is_separate(s);
    update_noheader_flag(s);
    send_hls_info_to_demux_lavf_open(s, c);
    return 0;
fail:
    hls_close(s);
    return ret;
}

static int switch_hls_mulit_bandwidth_adapt(AVFormatContext *s, int switch_idx)
{
    int i = 0, j = 0, ret = 0, variant_idx = -1;
    HLSContext *c = s->priv_data;

    if (!c || !(c->n_variants > 1) || switch_idx >= c->n_variants) {
        return ret;
    }
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (!pls)
            continue;
        if (c->n_playlists > 1 && pls->n_variants && !pls->n_renditions) {
            variant_idx += 1;
            if (variant_idx == switch_idx) {
                if ((ret = parse_playlist(c, pls->url, pls, NULL, 0, 0)) < 0) {
                    pls->broken = 1;
                    return ret;
                }
                /* If this isn't a live stream, calculate the total duration of the stream. */
                if (c->variants[variant_idx]->playlists[0]->finished) {
                    int64_t duration = 0;
                    for (j = 0; j < c->variants[variant_idx]->playlists[0]->n_segments; j++)
                        duration += c->variants[variant_idx]->playlists[0]->segments[j]->duration;
                    s->duration = duration;
                }
                pls->cur_seq_no = FFMAX(0, pls->finished?(select_cur_seq_no(c, pls)+1):select_cur_seq_no(c, pls));
                pls->cur_seq_no = (pls->cur_seq_no>= (pls->start_seq_no + pls->n_segments))?(pls->start_seq_no+pls->n_segments-3):pls->cur_seq_no;
                int cur_seq_no = FFMAX((pls->cur_seq_no - pls->start_seq_no), 0);
                ret = open_input_streams(s, i, pls->cur_seq_no, &ret, cur_seq_no, 1);
                if (-1 == ret) {
                    return ret;
                } else if (0 == ret) {
                    continue;
                } else {
                    c->cur_m3u8_idx = switch_idx;
                    for (j = 0; j < pls->n_main_streams; j++) {
                        c->playlists[i]->main_streams[j]->discard = AVDISCARD_DEFAULT;
                    }
                    break;
                }
            }
        }
    }
    return 0;
}

static int get_hls_mulit_audio_track_stream_index(AVFormatContext *s, int index)
{
    int i = 0, j = 0, iRetFlg = -1, stream_total = 0, first_audio_flg = 0;
    HLSContext *c = s->priv_data;
    if (!c)
        return iRetFlg;

    if (!(index < c->n_playlists))
        return c->n_playlists;

    if (c->n_variants <= 1) {
        for (i = 0; i <= index && i < c->n_playlists; i++) {
            struct playlist *pls = c->playlists[i];
            if (pls && pls->ctx) {
                if (i != index) {
                    stream_total += pls->ctx->nb_streams;
                } else {
                    first_audio_flg = 0;
                    for (j = 0; j < pls->ctx->nb_streams; j++) {
                        if (CODEC_TYPE_AUDIO == pls->ctx->streams[j]->codec->codec_type) {
                            stream_total += 1;
                            first_audio_flg = 1;
                        } else {
                            if (!first_audio_flg && pls->ctx->nb_streams > 1)
                                stream_total += 1;
                        }
                    }
                }
            }
        }
        iRetFlg = stream_total;
    }
    return iRetFlg;
}


static int recheck_discard_flags(AVFormatContext *s, int first)
{
    HLSContext *c = s->priv_data;
    int i = 0, changed = 0, cur_needed = 0;

    /* Check if any new streams are needed */
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (!pls || !pls->ctx || !pls->n_segments || pls->broken) {
            continue;
        }

        if (pls) {
            cur_needed = playlist_needed(c->playlists[i]);
            if (cur_needed && !pls->needed) {
                pls->needed = 1;
                changed = 1;
                pls->cur_seq_no = FFMAX(0, select_cur_seq_no(c, pls));//15;//
                pls->pb.eof_reached = 0;
                if (c->cur_timestamp != AV_NOPTS_VALUE) {
                    /* catch up */
                    pls->seek_timestamp = c->cur_timestamp;
                    pls->seek_flags = AVSEEK_FLAG_ANY;
                    pls->seek_stream_index = -1;
                }
            } else if (first && !cur_needed && pls->needed) {
                if (pls->input)
                    ff_format_io_close(pls->parent, &pls->input);
                pls->input_read_done = 0;
                if (pls->input_next)
                    ff_format_io_close(pls->parent, &pls->input_next);
                pls->input_next_requested = 0;
                pls->needed = 0;
                changed = 1;
                gxlogd("No longer receiving playlist %d\n", i);
            }
        }
    }
    return changed;
}

static void fill_timing_for_id3_timestamped_stream(struct playlist *pls)
{
    if (pls->id3_offset >= 0) {
        pls->pkt.dts = pls->id3_mpegts_timestamp +
            av_rescale_q(pls->id3_offset,
                    pls->ctx->streams[pls->pkt.stream_index]->time_base,
                    MPEG_TIME_BASE_Q);
        if (pls->pkt.duration)
            pls->id3_offset += pls->pkt.duration;
        else
            pls->id3_offset = -1;
    } else {
        /* there have been packets with unknown duration
         * since the last id3 tag, should not normally happen */
        pls->pkt.dts = AV_NOPTS_VALUE;
    }

    if (pls->pkt.duration)
        pls->pkt.duration = av_rescale_q(pls->pkt.duration,
                pls->ctx->streams[pls->pkt.stream_index]->time_base,
                MPEG_TIME_BASE_Q);

    pls->pkt.pts = AV_NOPTS_VALUE;
}

static AVRational get_timebase(struct playlist *pls)
{
    if (pls->is_id3_timestamped)
        return MPEG_TIME_BASE_Q;
    return pls->ctx->streams[pls->pkt.stream_index]->time_base;
}

static int compare_ts_with_wrapdetect(int64_t ts_a, struct playlist *pls_a,
        int64_t ts_b, struct playlist *pls_b)
{
    int64_t scaled_ts_a = av_rescale_q(ts_a, get_timebase(pls_a), MPEG_TIME_BASE_Q);
    int64_t scaled_ts_b = av_rescale_q(ts_b, get_timebase(pls_b), MPEG_TIME_BASE_Q);

    return av_compare_mod(scaled_ts_a, scaled_ts_b, 1LL << 33);
}

static int hls_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    HLSContext *c = s->priv_data;
    int ret, i, minplaylist = -1;

restart:
    ret = 0; i = 0; minplaylist = -1;
    recheck_discard_flags(s, c->first_packet);
    c->first_packet = 0;

    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
        if (!pls || !pls->ctx || (!pls->n_segments)) {
            continue;
        }
        /* Make sure we've got one buffered packet from each open playlist stream */
        if (pls->needed && !pls->pkt.data) {
            while (1) {
                int64_t ts_diff;
                AVRational tb;
                struct segment *seg = NULL;
                ret = av_read_frame(pls->ctx, &pls->pkt);
                if (ret < 0) {
                    if (c->is_switch_track)
                        break;
                    else if (!avio_feof(&pls->pb) && ret != AVERROR_EOF) {
                        return ret;
                    }
                    break;
                } else {
                    /* stream_index check prevents matching picture attachments etc. */
                    if (pls->is_id3_timestamped && pls->pkt.stream_index == 0) {
                        /* audio elementary streams are id3 timestamped */
                        fill_timing_for_id3_timestamped_stream(pls);
                    }
                    if (c->first_timestamp == AV_NOPTS_VALUE && pls->pkt.dts != AV_NOPTS_VALUE)
                        c->first_timestamp = av_rescale_q(pls->pkt.dts, get_timebase(pls), AV_TIME_BASE_Q);
                }

                seg = current_segment(pls);
                if (seg && seg->key_type == KEY_SAMPLE_AES && !strstr(pls->ctx->iformat->name, "mov")) {
                    enum AVCodecID codec_id = pls->ctx->streams[pls->pkt.stream_index]->codec->codec_id;
                    memcpy(c->crypto_ctx.iv, seg->iv, sizeof(seg->iv));
                    memcpy(c->crypto_ctx.key, pls->key, sizeof(pls->key));
                    ff_hls_senc_decrypt_frame(codec_id, &c->crypto_ctx, &(pls->pkt));
                }

                if (pls->seek_timestamp == AV_NOPTS_VALUE)
                    break;

                if (pls->seek_stream_index < 0 || pls->seek_stream_index == pls->pkt.stream_index) {
                    if (pls->pkt.dts == AV_NOPTS_VALUE) {
                        pls->seek_timestamp = AV_NOPTS_VALUE;
                        break;
                    }

                    tb = get_timebase(pls);
                    ts_diff = av_rescale_rnd(pls->pkt.dts, AV_TIME_BASE, tb.den, AV_ROUND_DOWN) - pls->seek_timestamp;
                    if (ts_diff >= 0 && (pls->seek_flags  & AVSEEK_FLAG_ANY || pls->pkt.flags & AV_PKT_FLAG_KEY)) {
                        pls->seek_timestamp = AV_NOPTS_VALUE;
                        break;
                    }
                }
                av_free_packet(&pls->pkt);
            }
        }

        if (ret < 0 && c->is_switch_track) {
            break;
        }

        /* Check if this stream has the packet with the lowest dts */
        if (pls->pkt.data) {
            struct playlist *minpls = (minplaylist < 0) ? NULL : c->playlists[minplaylist];
            if (minplaylist < 0) {
                minplaylist = i;
            } else {
                int64_t dts     =    pls->pkt.dts;
                int64_t mindts  = minpls->pkt.dts;
                if (dts == AV_NOPTS_VALUE ||
                        (mindts != AV_NOPTS_VALUE && compare_ts_with_wrapdetect(dts, pls, mindts, minpls) < 0))
                    minplaylist = i;
            }
        }
    }

    if (ret < 0 && c->is_switch_track && c->n_variants > 1) {
        close_cur_playlists_list_info(s, c->prog_now);
        ret = switch_hls_mulit_bandwidth_adapt(s, c->switch_idx);
        c->prog_now = c->switch_idx;
#if 0
        av_dict_set_int(&(s->url_options), "vid_stream_index", c->prog_now, 0);//hls mulit bandwidth current program index.
#endif
        c->is_switch_track = 0;
        c->is_in_reopen = 0;
        goto restart;
    }

    /* If we got a packet, return it */
    if (minplaylist >= 0) {
        struct playlist *pls = c->playlists[minplaylist];
        AVStream *ist;
        AVStream *st;
        if (!pls || !pls->ctx || (!pls->n_segments)) {
            return AVERROR_INVALIDDATA;
        }
        ret = update_streams_from_subdemuxer(s, pls, 0);
        if (ret < 0) {
            av_free_packet(&pls->pkt);
            return ret;
        }

        // If sub-demuxer reports updated metadata, copy it to the first stream
        // and set its AVSTREAM_EVENT_FLAG_METADATA_UPDATED flag.
        if (pls->ctx->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED) {
            if (pls->n_main_streams) {
                st = pls->main_streams[0];
                av_dict_copy(&st->metadata, pls->ctx->metadata, 0);
                st->event_flags |= AVSTREAM_EVENT_FLAG_METADATA_UPDATED;
            }
            pls->ctx->event_flags &= ~AVFMT_EVENT_FLAG_METADATA_UPDATED;
        }

        /* check if noheader flag has been cleared by the subdemuxer */
        if (pls->has_noheader_flag && !(pls->ctx->ctx_flags & AVFMTCTX_NOHEADER)) {
            pls->has_noheader_flag = 0;
            update_noheader_flag(s);
        }

        if (pls->pkt.stream_index >= pls->n_main_streams) {
            gxlogd("stream index inconsistency: index %d, %d main streams, %d subdemuxer streams\n",
                    pls->pkt.stream_index, pls->n_main_streams, pls->ctx->nb_streams);
            av_free_packet(&pls->pkt);
            return -1;
        }

        ist = pls->ctx->streams[pls->pkt.stream_index];
        st = pls->main_streams[pls->pkt.stream_index];

        av_packet_move_ref(pkt, &pls->pkt);
        pkt->stream_index = st->index;

        if (pkt->dts != AV_NOPTS_VALUE)
            c->cur_timestamp = av_rescale_q(pkt->dts, ist->time_base, AV_TIME_BASE_Q);

        /* There may be more situations where this would be useful, but this at least
         * handles newly probed codecs properly (i.e. request_probe by mpegts). */
        if (ist->codec->codec_id != st->codec->codec_id) {
            ret = set_stream_info_from_input_stream(st, pls, ist);
            if (ret < 0) {
                av_free_packet(pkt);
                return ret;
            }
        }
        return 0;
    }
    return AVERROR_EOF;
}

static int hls_read_seek(AVFormatContext *s, int stream_index,
        int64_t timestamp, int flags)
{
    HLSContext *c = s->priv_data;
    struct playlist *seek_pls = NULL;
    int i = 0, j = 0, stream_subdemuxer_index = -1;
    int64_t first_timestamp, seek_timestamp, duration, seq_no;

    if ((flags & AVSEEK_FLAG_BYTE) || (c->ctx->ctx_flags & AVFMTCTX_UNSEEKABLE))
        return AVERROR(ENOSYS);

    first_timestamp = c->first_timestamp == AV_NOPTS_VALUE ?
        0 : c->first_timestamp;

    seek_timestamp = av_rescale_rnd(timestamp, AV_TIME_BASE,
            s->streams[stream_index]->time_base.den,
            flags & AVSEEK_FLAG_BACKWARD ?
            AV_ROUND_DOWN : AV_ROUND_UP)+1;

    duration = s->duration == AV_NOPTS_VALUE ? 0 : s->duration;

    if (0 < duration && duration < seek_timestamp - first_timestamp)
        return AVERROR(EIO);

    /* find the playlist with the specified stream */
    for (i = 0; i < c->n_playlists; i++) {
        struct playlist *pls = c->playlists[i];
        if (!pls || !pls->ctx || (!pls->n_segments)) {
            continue;
        }
        for (j = 0; j < pls->n_main_streams; j++) {
            if (pls->main_streams[j] == s->streams[stream_index]) {
                seek_pls = pls;
                stream_subdemuxer_index = j;
                break;
            }
        }
    }
    /* check if the timestamp is valid for the playlist with the
     * specified stream index */
    if (!seek_pls || !find_timestamp_in_playlist(c, seek_pls, seek_timestamp, &seq_no))
        return AVERROR(EIO);

    /* set segment now so we do not need to search again below */
    seek_pls->cur_seq_no = seq_no;
    seek_pls->seek_stream_index = stream_subdemuxer_index;

    for (i = 0; i < c->n_playlists; i++) {
        /* Reset reading */
        struct playlist *pls = c->playlists[i];
        if (!pls || !pls->ctx || (!pls->n_segments)) {
            continue;
        }
        if (pls->input)
            ff_format_io_close(pls->parent, &pls->input);
        pls->input_read_done = 0;
        if (pls->input_next)
            ff_format_io_close(pls->parent, &pls->input_next);
        pls->input_next_requested = 0;
        av_free_packet(&pls->pkt);
        pls->pb.eof_reached = 0;
        /* Clear any buffered data */
        pls->pb.buf_end = pls->pb.buf_ptr = pls->pb.buffer;
        /* Reset the pos, to let the mpegts demuxer know we've seeked. */
        pls->pb.pos = 0;
        /* Flush the packet queue of the subdemuxer. */
        ff_read_frame_flush(pls->ctx);

        /* Reset the init segment so it's re-fetched and served appropiately */
        pls->cur_init_section = NULL;

        //pls->seek_timestamp = seek_timestamp;
        pls->seek_flags = flags;

        if (pls != seek_pls) {
            /* set closest segment seq_no for playlists not handled above */
            find_timestamp_in_playlist(c, pls, seek_timestamp, &pls->cur_seq_no);
            /* seek the playlist to the given position without taking
             * keyframes into account since this playlist does not have the
             * specified stream where we should look for the keyframes */
            pls->seek_stream_index = -1;
            pls->seek_flags |= AVSEEK_FLAG_ANY;
        }
    }

    c->cur_timestamp = seek_timestamp;

    return 0;
}

static int hls_read_control(AVFormatContext *s, int cmd, void *arg)
{
    int ret = -1;
    HLSContext *c;
    AvControl *av_control = (AvControl *)arg;

    if (!s || !av_control || av_control->discard_idx < 0 || av_control->retain_idx < 0)
        return ret;
    c = s->priv_data;

    switch (cmd) {
        case HLS_SEAMLESS_BANDWIDTH_SWITCH://hls seamless bandwidth switch
            if (c->n_variants > 1) {
                struct variant *v = c->variants[av_control->retain_idx];
                if (auto_filter_decoder_unsupporrt_type(NULL, v)) {
                    return ret;
                }
                if (av_control->retain_idx < c->n_playlists) {
                    c->switch_idx = av_control->retain_idx;
                    c->is_switch_track = 1;
                    c->is_in_reopen = 1;
                }
            }
            break;
        case HLS_MULIT_AUDIO_TRACK_STREAM_INDEX://hls mulit audio stream index.
            ret = get_hls_mulit_audio_track_stream_index(s, av_control->retain_idx);
            break;
        case HLS_IS_MULIT_AUDIO_TRACK://is mulit audio
            ret = recheck_hls_is_mulit_audio_track(s);
            break;
        default:
            break;
    }
    return ret;
}

static int hls_probe(AVProbeData *p)
{
    /* Require #EXTM3U at the start, and either one of the ones below
     * somewhere for a proper match. */
    if (strncmp((const char *)p->buf, "#EXTM3U", strlen("#EXTM3U")))
        return 0;

    if (av_strnstr((const char *)p->buf, "#EXT-X-STREAM-INF:", p->buf_size)     ||
            av_strnstr((const char *)p->buf, "#EXT-X-TARGETDURATION:", p->buf_size) ||
            (av_strnstr((const char *)p->buf, "#EXTM3U", p->buf_size) &&  av_strnstr((const char *)p->buf, "#EXTINF:", p->buf_size))   ||
            av_strnstr((const char *)p->buf, "#EXT-X-MEDIA-SEQUENCE:", p->buf_size))
        return AVPROBE_SCORE_MAX;
    return 0;
}

AVInputFormat hls_demuxer = {
    .name = "hls",
    .priv_data_size = sizeof(HLSContext),
    .read_probe = hls_probe,
    .read_header = hls_read_header,
    .read_packet = hls_read_packet,
    .read_close = hls_close,
    .read_seek = hls_read_seek,
    .read_control = hls_read_control,
};

