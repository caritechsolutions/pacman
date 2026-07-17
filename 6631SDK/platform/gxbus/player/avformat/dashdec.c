/*
 * Dynamic Adaptive Streaming over HTTP demux
 * Copyright (c) 2017 samsamsam@o2.pl based on HLS demux
 * Copyright (c) 2017 Steven Liu
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
#include "riff.h"

#if defined(CONFIG_OPENSOURCE_LIBXML2)
    #include "libxml/parser.h"
    #include "libxml/tree.h"
    #include "libxml/xmlstring.h"
#else
    #include "tree.h"
    #include "parser.h"
#endif

#include "../avutil/intreadwrite.h"
#include "../avutil/avstring.h"
#include "../avutil/dict.h"
#include "../avutil/bprintf.h"
#include "avformat.h"
#include "riff.h"
#include "gx_stream.h"
#include "dash.h"
#include "avio.h"
#include "string.h"
#include "tree.h"
#include "parser.h"
#include "http.h"
#include "memmgr.h"
#include "abr.h"

#if defined(CONFIG_OPENSOURCE_LIBXML2)
    #define DashXmlNodePtr xmlNodePtr
    #define DashxmlFree(ptr)           xmlFree(ptr)
    #define DashxmlNodeGetContent xmlNodeGetContent
    #define DashxmlGetProp xmlGetProp
    #define DashxmlFirstElementChild xmlFirstElementChild
    #define DashxmlNextElementSibling xmlNextElementSibling
    #define DashxmlAttrPtr xmlAttrPtr
    #define DashxmlCopyNode xmlCopyNode
    #define DashxmlNewNode xmlNewNode
    #define DashxmlFreeDoc xmlFreeDoc
    #define DashxmlFreeNode xmlFreeNode
#else
    #include "tree.h"
    #include "parser.h"
    #define DashXmlNodePtr GXxmlNodePtr
    #define DashxmlFree(ptr)           GXxmlFree(ptr)
    #define DashxmlNodeGetContent GXxmlNodeGetContent
    #define DashxmlGetProp GXxmlGetProp
    #define DashxmlFirstElementChild GXxmlFirstElementChild
    #define DashxmlNextElementSibling GXxmlNextElementSibling
    #define DashxmlAttrPtr GXxmlAttrPtr
    #define DashxmlCopyNode GXxmlCopyNode
    #define DashxmlNewNode GXxmlNewNode
    #define DashxmlFreeDoc GXxmlFreeDoc
    #define DashxmlFreeNode GXxmlFreeNode
#endif

#define MAX_BPRINT_READ_SIZE (UINT_MAX - 1)
#define INITIAL_VIDEO_BUFFER_SIZE (131072)
#define INITIAL_OTHER_BUFFER_SIZE (65536)
#define DEFAULT_MANIFEST_SIZE (8 * 1024)
#define MAX_CONTENT_PROTECTION_NUM 3

extern GxStreamOptionCallback public_stream_callback;

struct fragment {
    int64_t url_offset;
    int64_t size;
    char *url;
};

/*
 * reference to : ISO_IEC_23009-1-DASH-2012
 * Section: 5.3.9.6.2
 * Table: Table 17 — Semantics of SegmentTimeline element
 * */
struct timeline {
    /* starttime: Element or Attribute Name
     * specifies the MPD start time, in @timescale units,
     * the first Segment in the series starts relative to the beginning of the Period.
     * The value of this attribute must be equal to or greater than the sum of the previous S
     * element earliest presentation time and the sum of the contiguous Segment durations.
     * If the value of the attribute is greater than what is expressed by the previous S element,
     * it expresses discontinuities in the timeline.
     * If not present then the value shall be assumed to be zero for the first S element
     * and for the subsequent S elements, the value shall be assumed to be the sum of
     * the previous S element's earliest presentation time and contiguous duration
     * (i.e. previous S@starttime + @duration * (@repeat + 1)).
     * */
    int64_t starttime;
    /* repeat: Element or Attribute Name
     * specifies the repeat count of the number of following contiguous Segments with
     * the same duration expressed by the value of @duration. This value is zero-based
     * (e.g. a value of three means four Segments in the contiguous series).
     * */
    int64_t repeat;
    /* duration: Element or Attribute Name
     * specifies the Segment duration, in units of the value of the @timescale.
     * */
    int64_t duration;
};

/*
 * Each playlist has its own demuxer. If it is currently active,
 * it has an opened AVIOContext too, and potentially an AVPacket
 * containing the next packet from this stream.
 */
struct representation {
    char *url_template;
    AVIOContext pb;
    AVIOContext *input;
    AVFormatContext *parent;
    AVFormatContext *ctx;
    AVPacket pkt;
    int rep_idx;
    int rep_count;
    int stream_index;
    int input_read_done;
    char *id;
    char *lang;
    char *codecs;
    char *width;
    char *height;
    char *sar;
    char *samplerate;

    enum AVMediaType type;
    int bandwidth;
    AVRational framerate;
    //begin. TODO:not use.
    //AVStream *assoc_stream; /* demuxer stream associated with this representation */
    //end
    int n_fragments;
    struct fragment **fragments; /* VOD list of fragment for profile */

    int n_timelines;
    struct timeline **timelines;

    int64_t first_seq_no;
    int64_t last_seq_no;
    int64_t start_number; /* used in case when we have dynamic list of segment to know which segments are new one*/

    int64_t fragment_duration;
    int64_t fragment_timescale;

    int64_t presentation_timeoffset;

    int64_t cur_seq_no;
    int64_t previous_seq_no;
    int64_t cur_seg_offset;
    int64_t cur_seg_size;
    struct fragment *cur_seg;
    AVStream *assoc_stream; /* demuxer stream associated with this representation */

    /* Currently active Media Initialization Section */
    struct fragment *init_section;
    uint8_t *init_sec_buf;
    int init_sec_buf_size;
    uint32_t init_sec_data_len;
    uint32_t init_sec_buf_read_offset;
    int64_t cur_timestamp;
    int is_restart_needed;
    int discard;
    int needed;
    int is_not_request_flag;/*0:default: request data, 1: not request data*/
    int is_refresh_manifest; //false live flags.
    int read_segment_num;
    uint64_t http_req_start_time;
    uint64_t http_req_end_time;
    uint64_t http_req_last_time;
    uint64_t internal_http_read_size;
    int is_switch_track;
};

struct ContentProtection {
    char *contentprotection_value;
    char *contentprotection_default_kid;
    char *contentprotection_schemeIduri;
    char *contentprotection_pssh;
};

typedef struct Period {
    int cur_vtrack_idx; // current video track index.
    int n_videos;
    struct representation **videos;
    int cur_atrack_idx;  // current audio track index.
    int n_audios;
    struct representation **audios;
    int cur_strack_idx;  // current subtile track index.
    int n_subtitles;
    struct representation **subtitles;

    /* Period Attribute */
    int64_t period_duration;
    int64_t period_start;
    /* AdaptationSet Attribute */
    char *adaptionset_lang;

    /* Flags for init section*/
    int is_init_section_common_video;
    int is_init_section_common_audio;
    int is_init_section_common_subtitle;

    int n_contentprotection;
    struct ContentProtection **contentprotection;

    int is_in_advance;
    enum AVMediaType is_in_advance_type;
    int switch_idx;
    int is_switch_track;
    enum AVMediaType switch_media_type;
    AVFormatContext *ctx;
    ABRContext* audio_abr;
    ABRContext* video_abr;
    int is_in_reopen;
    int is_open_fail_flag;
    int64_t current_max_seg;
} Period;

typedef struct DASHContext {
    char *base_url;
    /* MediaPresentationDescription Attribute */
    uint64_t media_presentation_duration;
    uint64_t suggested_presentation_delay;
    uint64_t availability_start_time;
    uint64_t previous_availability_start_time;
    uint64_t availability_end_time;
    uint64_t publish_time;
    uint64_t minimum_update_period;
    uint64_t time_shift_buffer_depth;
    uint64_t maxSegmentDuration;
    uint64_t min_buffer_time;

    int http_persistent;
    int is_false_live;

    int is_live;
    AVDictionary *avio_opts;
    int max_url_size;

    int n_periods;
    int current_period;
    Period **periods;
    AVIOContext *keepalive_manifest;

    unsigned int last_time_update_period;
    handle_t abr_thread;
    AvControl av_control;
} DASHContext;

typedef struct Drm_Identifier {
    char vender_name[64];
    char identifier[37];
} Drm_Identifier;

#define AV_BPRINT_SIZE_UNLIMITED  ((unsigned)-1)

static int switch_dash_mulitprogram_track(AVFormatContext *s, int switch_idx, int type);
static int dash_close(AVFormatContext *s);
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

typedef struct VideoRateAbbr {
    const char *abbr;
    AVRational rate;
} VideoRateAbbr;

static int get_max_http_persistent_num(void)
{
#define PLAYER_DEFAULT_SYSMEM_SIZE (8*1024*1024)
    int sysMemSize = 0;
	GxPlayer_SystemGet(PSYS_PACKET_CACHE, &sysMemSize);
    if (sysMemSize >= PLAYER_DEFAULT_SYSMEM_SIZE) {
        return 100;
    }
    return 50;
}

static int auto_filter_decoder_unsupporrt_type(struct representation *rep)
{
    if (!rep) {
        return 1;
    }
    if ((rep->width &&  (atoi(rep->width) > 1920)) || (rep->height && (atoi(rep->height) > 1080))) {
        gxlogi_raw_l ("Current bandwidth variants:%sx%s.Codec not support.\n", rep->width, rep->height);
        return 1;
    }
    if (rep->framerate.num > 60) {
        gxlogi_raw_l ("Current framerate:%d.Codec not support.\n", rep->framerate.num);
        return 1;
    }
    if (rep->codecs) {
        if ((av_stristr(rep->codecs, "avc1.")) /*H.264 (AVC)*/
        || (av_stristr(rep->codecs, "mp4a.")) /*AAC-LC/HE-AAC*/
        || (av_stristr(rep->codecs, "hvc1.")) /*H.265 (HEVC)*/
        || (av_stristr(rep->codecs, "hev1.")) /*H.265 (HEVC)*/
        || (av_stristr(rep->codecs, "ac-3")) /*AC-3 (Dolby Digital)*/
        || (av_stristr(rep->codecs, "ec-3")) /*EC-3 (Dolby Digital Plus)*/
        || (av_stristr(rep->codecs, "opus"))) /*Opus*/ {
            if (av_stristr(rep->codecs, "vp09.") || av_stristr(rep->codecs, "av01.")) {
                gxlogi_raw_l("Current codecs:%s. The decoder does not support it.\n", rep->codecs);
                return 1;
            }
            return 0;
        } else if (av_stristr(rep->codecs, "vp09.") || av_stristr(rep->codecs, "av01.")) {
            gxlogi_raw_l("Current codecs:%s. The decoder does not support it.\n", rep->codecs);
            return 1;
        } else {
            gxlogi_raw_l("Current codecs:%s. The decoder does not support it.\n", rep->codecs);
            return 1;
        }
    }
    return 0;
}

static void dashdec_add_metric(DASHContext* c, enum AVMediaType type, float tpt,
        int buffer_max, int buffer_r, int buffer_c)
{
    ABRContext* abr = NULL;
    if ((type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) || (c->n_periods <= 0 || c->current_period < 0)) {
        gxloge ("mediatype not found when adding metric.\n");
    }
    abr = (type == AVMEDIA_TYPE_VIDEO)?c->periods[c->current_period]->video_abr:c->periods[c->current_period]->audio_abr;
    abr_add_metric(abr, tpt, buffer_max, buffer_r, buffer_c);
}

static int dashdec_get_stream(DASHContext* c, enum AVMediaType type)
{
    int *bandwidth, ai, i = 0;
    Period *cur_periods = NULL;

    if ((type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) || (c->n_periods <= 0 || c->current_period < 0 || c->current_period >= c->n_periods)) {
        gxloge ("mediatype not found when getting stream.\n");
        return AVERROR(EINVAL);
    }

    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return AVERROR(EINVAL);

    if (type == AVMEDIA_TYPE_VIDEO) {
        if (cur_periods->n_videos <= 0)
            return AVERROR(EINVAL);
        bandwidth = (int*)av_mallocz(sizeof(int) * cur_periods->n_videos);
        if (!bandwidth)
            return AVERROR(ENOMEM);
        for (i = 0; i < cur_periods->n_videos; ++i) {
            bandwidth[i] = cur_periods->videos[i]->bandwidth;
        }
        ai = abr_get_stream(cur_periods->video_abr, bandwidth, cur_periods->n_videos);
        av_free(bandwidth);
        return (ai == -1)?-1:cur_periods->videos[ai]->stream_index;
    } else if (type == AVMEDIA_TYPE_AUDIO) {
        if (cur_periods->n_audios <= 0)
            return AVERROR(EINVAL);
        bandwidth = (int*)av_mallocz(sizeof(int) * cur_periods->n_audios);
        if (!bandwidth)
            return AVERROR(ENOMEM);
        for (i = 0; i < cur_periods->n_audios; ++i) {
            bandwidth[i] = cur_periods->audios[i]->bandwidth;
        }
        ai = abr_get_stream(cur_periods->audio_abr, bandwidth, cur_periods->n_audios);
        av_free(bandwidth);
        return (ai == -1)?-1:cur_periods->audios[ai]->stream_index;
    }
    return AVERROR(EINVAL);
}

static int init_dash_abr(ABRContext** http_abr)
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

static void close_dash_abr(ABRContext* abr)
{
    if (abr) {
        if (abr->throughput_history)
            av_free(abr->throughput_history);
        av_free(abr);
    }
}

/*
   BUPT
   this function is for abr schedule
   tpt(Mbps), buffer(KB)
 */
static int dash_abr_update(AVFormatContext *s, enum AVMediaType type)
{
    struct representation *cur_rep = NULL;
    DASHContext *c = s->priv_data;
    float tpt = -1;
    Period *cur_periods = NULL;
    uint64_t t1 = av_gettime();
    int swit_idx = -1, timeout = 0, cur_idx = -1, buffer_max = 240, buffer_r = 90, buffer_c = 126;

    if (!c || (c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods)) {
        return AVERROR(EINVAL);
    }

    cur_periods = c->periods[c->current_period];
    if (!cur_periods) {
        return AVERROR(EINVAL);
    }

    if (!((cur_periods->cur_vtrack_idx >= 0) || (cur_periods->cur_atrack_idx >= 0))) {
        return AVERROR(EINVAL);
    }

    if ((type != AVMEDIA_TYPE_VIDEO) && (type != AVMEDIA_TYPE_AUDIO)) {
        return AVERROR(EINVAL);
    }

    if (type == AVMEDIA_TYPE_VIDEO) {
        cur_rep = cur_periods->videos[cur_periods->cur_vtrack_idx];
        cur_idx = cur_periods->cur_vtrack_idx;
    } else if (type == AVMEDIA_TYPE_AUDIO) {
        cur_rep = cur_periods->audios[cur_periods->cur_atrack_idx];
        cur_idx = cur_periods->cur_atrack_idx;
    } else {
        return AVERROR(EINVAL);
    }
    while (cur_rep && cur_rep->http_req_end_time == 0) {
        if (url_check_interrupt_cb())
            return AVERROR_EXIT;
        GxCore_ThreadDelay(100);
        if (((av_gettime() - t1)/1000) > 5000) {
            timeout = 1;
            break;
        }
    }
    if (cur_rep && cur_rep->http_req_end_time > 0) {
        tpt = 8 * (float)cur_rep->internal_http_read_size / (cur_rep->http_req_end_time - cur_rep->http_req_start_time);
    } else if (cur_rep && cur_rep->internal_http_read_size > 0) {
        tpt = 8 * (float)cur_rep->internal_http_read_size / (cur_rep->http_req_last_time - cur_rep->http_req_start_time);
    } else {
        return 0;
    }

    dashdec_add_metric(c, type, tpt, buffer_max, buffer_r, buffer_c);
    swit_idx = dashdec_get_stream(c, type);
    if (swit_idx >= 0 && swit_idx != cur_idx && !cur_periods->is_in_reopen) {
        if ((swit_idx >= cur_periods->n_videos) || (swit_idx == cur_idx)) {
            return 0;
        }
        cur_rep = cur_periods->videos[swit_idx];
        if (auto_filter_decoder_unsupporrt_type(cur_rep)) {
            gxloge ("decoder unsupport.\n");
            return 0;
        }
        cur_periods->switch_idx = swit_idx;
        cur_periods->is_switch_track = 1;
        cur_periods->switch_media_type = CODEC_TYPE_VIDEO;
        GxCore_ThreadDelay(3000);
    }
    gxlogd ("[%lld]DASH Metric(%s): tpt=%f, swit_idx=%d, timeout=%d, cur_bandwith=%d\n", av_gettime(),
            type==AVMEDIA_TYPE_VIDEO?"video":"audio", tpt, swit_idx, timeout, cur_rep->bandwidth);
    return 0;
}

static void dash_abr_thread(void* arg)
{
    int ret = 0;
    AVFormatContext *s = arg;
    if (!s)
        return ;
    while (!url_check_interrupt_cb()) {
        if ((ret = dash_abr_update(s, AVMEDIA_TYPE_VIDEO)) < 0)
            break;
        GxCore_ThreadDelay(500);
    }
    return ;
}

static const VideoRateAbbr video_rate_abbrs[]= {
    { "ntsc",      { 30000, 1001 } },
    { "pal",       {    25,    1 } },
    { "qntsc",     { 30000, 1001 } }, /* VCD compliant NTSC */
    { "qpal",      {    25,    1 } }, /* VCD compliant PAL */
    { "sntsc",     { 30000, 1001 } }, /* square pixel NTSC */
    { "spal",      {    25,    1 } }, /* square pixel PAL */
    { "film",      {    24,    1 } },
    { "ntsc-film", { 24000, 1001 } },
};

int av_parse_ratio(AVRational *q, const char *str, int max,
        int log_offset, void *log_ctx)
{
    char c;
    int ret;

    if (sscanf(str, "%d:%d%c", &q->num, &q->den, &c) != 2) {
        double d;
        ret = av_expr_parse_and_eval(&d, str, NULL, NULL,
                NULL, NULL, NULL, NULL,
                NULL, log_offset, log_ctx);
        if (ret < 0)
            return ret;
        *q = av_d2q(d, max);
    } else {
        av_reduce(&q->num, &q->den, q->num, q->den, max);
    }

    return 0;
}

static int av_parse_ratio_quiet(AVRational *rate, const char *str, int max)
{
    return av_parse_ratio(rate, str, max, AV_LOG_MAX_OFFSET, NULL);
}

int av_parse_video_rate(AVRational *rate, const char *arg)
{
    int i, ret;
    int n = FF_ARRAY_ELEMS(video_rate_abbrs);

    /* First, we check our abbreviation table */
    for (i = 0; i < n; ++i)
        if (!strcmp(video_rate_abbrs[i].abbr, arg)) {
            *rate = video_rate_abbrs[i].rate;
            return 0;
        }

    /* Then, we try to parse it as fraction */
    if ((ret = av_parse_ratio_quiet(rate, arg, 1001000)) < 0)
        return ret;
    if (rate->num <= 0 || rate->den <= 0)
        return AVERROR(EINVAL);
    return 0;
}

/**
 * Create an AVRational.
 *
 * Useful for compilers that do not support compound literals.
 *
 * @note The return value is not reduced.
 * @see av_reduce()
 */
static inline AVRational av_make_q(int num, int den)
{
    AVRational r = { num, den };
    return r;
}

static char *av_strireplace(const char *str, const char *from, const char *to)
{
    char *ret = NULL;
    const char *pstr2, *pstr = str;
    size_t tolen = strlen(to), fromlen = strlen(from);
    AVBPrint pbuf;

    av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);
    while ((pstr2 = av_stristr(pstr, from))) {
        av_bprint_append_data(&pbuf, pstr, pstr2 - pstr);
        pstr = pstr2 + fromlen;
        av_bprint_append_data(&pbuf, to, tolen);
    }
    av_bprint_append_data(&pbuf, pstr, strlen(pstr));
    if (!av_bprint_is_complete(&pbuf)) {
        av_bprint_finalize(&pbuf, NULL);
    } else {
        av_bprint_finalize(&pbuf, &ret);
    }

    return ret;
}

static int ishttp(char *url)
{
    return av_strstart(url, "http", NULL);
}

static int aligned(int val)
{
    return ((val + 0x3F) >> 6) << 6;
}

static uint64_t get_current_time_in_sec(void)
{
    return  (av_gettime() / 1000000);
}

static uint64_t get_utc_date_time_insec(AVFormatContext *s, const char *datetime)
{
    struct tm timeinfo;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, ret = 0;
    float second = 0.0;

    /* ISO-8601 date parser */
    if (!datetime)
        return 0;

    ret = sscanf(datetime, "%d-%d-%dT%d:%d:%fZ", &year, &month, &day, &hour, &minute, &second);
    /* year, month, day, hour, minute, second  6 arguments */
    if (ret != 6) {
        gxlogd ("get_utc_date_time_insec get a wrong time format\n");
    }
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon  = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min  = minute;
    timeinfo.tm_sec  = (int)second;

    return av_timegm(&timeinfo);
}

static uint32_t get_duration_insec(AVFormatContext *s, const char *duration)
{
    /* ISO-8601 duration parser */
    uint32_t days = 0, hours = 0, mins = 0, secs = 0;
    int size = 0;
    float value = 0;
    char type = '\0';
    const char *ptr = duration;

    while (*ptr) {
        if (*ptr == 'P' || *ptr == 'T') {
            ptr++;
            continue;
        }

        if (sscanf(ptr, "%f%c%n", &value, &type, &size) != 2) {
            gxlogd ("get_duration_insec get a wrong time format\n");
            return 0; /* parser error */
        }
        switch (type) {
            case 'D':
                days = (uint32_t)value;
                break;
            case 'H':
                hours = (uint32_t)value;
                break;
            case 'M':
                mins = (uint32_t)value;
                break;
            case 'S':
                secs = (uint32_t)value;
                break;
            default:
                // handle invalid type
                break;
        }
        ptr += size;
    }
    return  ((days * 24 + hours) * 60 + mins) * 60 + secs;
}

static int64_t get_segment_start_time_based_on_timeline(struct representation *pls, int64_t cur_seq_no)
{
    int i = 0;
    int64_t start_time = 0, j = 0, num = 0;

    if (pls->n_timelines) {
        num = pls->first_seq_no;
        for (i = 0; i < pls->n_timelines; i++) {
            if (pls->timelines[i]->starttime > 0) {
                start_time = pls->timelines[i]->starttime;
            }
            if (num == cur_seq_no)
                goto finish;

            start_time += pls->timelines[i]->duration;

            if (pls->timelines[i]->repeat == -1) {
                start_time = pls->timelines[i]->duration * cur_seq_no;
                goto finish;
            }

            for (j = 0; j < pls->timelines[i]->repeat; j++) {
                num += 1;
                if (num == cur_seq_no)
                    goto finish;
                start_time += pls->timelines[i]->duration;
            }
            num += 1;
        }
    }
finish:
    return start_time;
}

static int64_t calc_next_seg_no_from_timelines(struct representation *pls, int64_t cur_time)
{
    int i = 0;
    int64_t j = 0, num = 0, start_time = 0;

    for (i = 0; i < pls->n_timelines; i++) {
        if (pls->timelines[i]->starttime > 0) {
            start_time = pls->timelines[i]->starttime;
        }
        if (start_time > cur_time)
            goto finish;

        start_time = start_time + pls->timelines[i]->duration;
        for (j = 0; j < pls->timelines[i]->repeat; j++) {
            num++;
            if (start_time > cur_time)
                goto finish;
            start_time += pls->timelines[i]->duration;
        }
        num++;
    }

    return -1;

finish:
    return num;
}

static void free_fragment(struct fragment **seg)
{
    if (!(*seg)) {
        return;
    }
    if ((*seg)->url)
        av_freep(&(*seg)->url);
    av_freep(seg);
}

static void free_fragment_list(struct representation *pls)
{
    int i;
    if (!pls)
        return ;

    for (i = 0; i < pls->n_fragments; i++) {
        free_fragment(&pls->fragments[i]);
    }
    if (pls->fragments)
        av_freep(&pls->fragments);
    pls->n_fragments = 0;
}

static void free_timelines_list(struct representation *pls)
{
    int i;
    if (!pls)
        return ;

    for (i = 0; i < pls->n_timelines; i++) {
        av_freep(&pls->timelines[i]);
    }
    if (pls->timelines)
        av_freep(&pls->timelines);
    pls->n_timelines = 0;
}

static void free_representation(struct representation *pls)
{
    if (!pls)
        return ;
    free_fragment_list(pls);
    free_timelines_list(pls);
    free_fragment(&pls->cur_seg);
    free_fragment(&pls->init_section);
    if (pls->init_sec_buf) {
        av_free(pls->init_sec_buf);
        pls->init_sec_buf = NULL;
    }
    if (pls->pb.buffer) {
        av_free(pls->pb.buffer);
        pls->pb.buffer = NULL;
    }
    if (pls->input) {
        ff_format_io_close(pls->parent, &pls->input);
    }
    pls->input_read_done = 0;
    if (pls && pls->ctx) {
        avformat_close_input(&pls->ctx);
    }
    if (pls->url_template) {
        av_free(pls->url_template);
        pls->url_template = NULL;
    }
    if (pls->lang) {
        av_free(pls->lang);
        pls->lang = NULL;
    }
    if (pls->codecs) {
        av_free(pls->codecs);
        pls->codecs = NULL;
    }
    if (pls->width) {
        av_free(pls->width);
        pls->width = NULL;
    }
    if (pls->height) {
        av_free(pls->height);
        pls->height = NULL;
    }
    if (pls->sar) {
        av_free(pls->sar);
        pls->sar = NULL;
    }
    if (pls->samplerate) {
        av_free(pls->samplerate);
        pls->samplerate = NULL;
    }
    if (pls->id) {
        av_free(pls->id);
        pls->id = NULL;
    }
    if (pls) {
        av_free(pls);
        pls = NULL;
    }
}

static void free_video_list(struct Period *cur_periods)
{
    int i;
    for (i = 0; i < cur_periods->n_videos; i++) {
        struct representation *pls = cur_periods->videos[i];
        free_representation(pls);
    }
    if (cur_periods->videos) {
        av_free(cur_periods->videos);
        cur_periods->videos = NULL;
    }
    cur_periods->n_videos = 0;
}

static void free_audio_list(struct Period *cur_periods)
{
    int i = 0;
    for (i = 0; i < cur_periods->n_audios; i++) {
        struct representation *pls = cur_periods->audios[i];
        free_representation(pls);
    }
    if (cur_periods->audios) {
        av_free(cur_periods->audios);
        cur_periods->audios = NULL;
    }
    cur_periods->n_audios = 0;
}

static void free_subtitle_list(struct Period *cur_periods)
{
    int i = 0;
    for (i = 0; i < cur_periods->n_subtitles; i++) {
        struct representation *pls = cur_periods->subtitles[i];
        free_representation(pls);
    }
    if (cur_periods->subtitles) {
        av_free(cur_periods->subtitles);
        cur_periods->subtitles = NULL;
    }
    cur_periods->n_subtitles = 0;
}

static void free_period_list(DASHContext* c) {
    int i;
    if ((c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods))
        return ;
    for (i = 0; i < c->n_periods; i++) {
        Period *cur_periods = c->periods[i];
        if (!cur_periods) {
            continue;
        }
        free_video_list(cur_periods);
        free_audio_list(cur_periods);
        free_subtitle_list(cur_periods);
        if (cur_periods) {
            av_free(cur_periods);
            cur_periods = NULL;
        }
    }
}

static void free_contentprotection_list(DASHContext *c, int cur_period)
{
    int i;
    if ((cur_period < 0) || (c->n_periods <= 0) || (cur_period >= c->n_periods))
        return ;
    for (i = 0; i < c->periods[cur_period]->n_contentprotection; i++) {
        struct ContentProtection *pls = c->periods[cur_period]->contentprotection[i];
        if (pls) {
            if (pls->contentprotection_value)
                av_freep(&pls->contentprotection_value);
            if (pls->contentprotection_default_kid)
                av_freep(&pls->contentprotection_default_kid);
            if (pls->contentprotection_schemeIduri)
                av_freep(&pls->contentprotection_schemeIduri);
            if (pls->contentprotection_pssh)
                av_freep(&pls->contentprotection_pssh);
            if (pls)
                av_freep(&pls);
        }
    }
    if (c->periods[cur_period]->contentprotection)
        av_freep(&c->periods[cur_period]->contentprotection);
    c->periods[cur_period]->n_contentprotection = 0;
}

extern int ff_http_do_new_request(URLContext *h, const char *uri, AVDictionary **opts);
static int open_url_keepalive(AVIOContext **pb, const char *url, AVDictionary **options)
{
    int ret = 0;
    URLContext *uc = ffio_geturlcontext(*pb);
    if (!uc)
        return ret;
    (*pb)->eof_reached = 0;
    ret = ff_http_do_new_request(uc, url, options);
    if (ret < 0) {
        ff_format_io_close(NULL, pb);
    }
    return ret;
}

static int open_url(AVFormatContext *s, AVIOContext **pb, char *url,
        AVDictionary *opts, AVDictionary *opts2, int *is_http_out)
{
    DASHContext *c = s->priv_data;
    AVDictionary *tmp = NULL;
    char *proto_name = NULL;
    int ret = -1, flags = 0;
    AVDictionaryEntry *e = NULL;
    int is_http = av_strstart(url, "http", NULL);

    av_dict_copy(&tmp, opts, 0);
    av_dict_copy(&tmp, opts2, 0);

    if (av_strstart(url, "crypto", NULL)) {
        if (url[6] == '+' || url[6] == ':')
            proto_name = avio_find_protocol_name(url + 7);
    }

    if (!proto_name)
        proto_name = avio_find_protocol_name(url);

    if (!proto_name)
        return AVERROR_INVALIDDATA;

    // only http(s) & file are allowed
    if (av_strstart(proto_name, "file", NULL)) {
        char *allowed_extensions = "aac,m4a,m4s,m4v,mov,mp4,webm,ts";
        if (strcmp(allowed_extensions, "ALL") && !av_match_ext(url, allowed_extensions)) {
            gxloge ("dashdec.c. not support protocol.\n");
            return AVERROR_INVALIDDATA;
        }
    } else if (av_strstart(proto_name, "http", NULL)) {
        ;
    } else {
        if (proto_name) {
            av_free(proto_name);
            proto_name = NULL;
        }
        return AVERROR_INVALIDDATA;
    }

    if (!strncmp(proto_name, url, strlen(proto_name)) && url[strlen(proto_name)] == ':')
        ;
    else if (av_strstart(url, "crypto", NULL) && !strncmp(proto_name, url + 7, strlen(proto_name)) && url[7 + strlen(proto_name)] == ':')
        ;
    else if (strcmp(proto_name, "file") || !strncmp(url, "file,", 5)) {
        if (proto_name) {
            av_free(proto_name);
            proto_name = NULL;
        }
        return AVERROR_INVALIDDATA;
    }

    if ((e = av_dict_get(c->avio_opts, "avioflags", NULL, 0))) {
        flags = atoi(e->value);
    }


    if (is_http && c->http_persistent && *pb) {
        av_dict_set(&tmp, "multiple_requests", "1", 0);
        ret = open_url_keepalive(pb, url, &tmp);
        if (ret == AVERROR_EXIT) {
            av_dict_free(&tmp);
            if (proto_name) {
                av_free(proto_name);
                proto_name = NULL;
            }
            return ret;
        } else if (ret < 0) {
            if (ret != AVERROR_EOF)
                gxlogi_raw_l ("keepalive request failed for '%s' with error:%d(%s) when opening url\n", url, ret, av_err2str(ret));
            ret = avio_open2(pb, url, AVIO_FLAG_READ | flags, &tmp);
        }
    } else {
        av_freep(pb);
        ret = avio_open2(pb, url, AVIO_FLAG_READ | flags, &tmp);
    }
    if (ret >= 0) {
        // update cookies on http response with setcookies.
#if 0
        char *new_cookies = NULL;

        if (!(s->flags & AVFMT_FLAG_CUSTOM_IO))
            av_opt_get(*pb, "cookies", AV_OPT_SEARCH_CHILDREN, (uint8_t**)&new_cookies);

        if (new_cookies) {
            av_dict_set(&opts, "cookies", new_cookies, AV_DICT_DONT_STRDUP_VAL);
        }
#endif
    }

    av_dict_free(&tmp);
    if (is_http_out)
        *is_http_out = is_http;
    if (proto_name) {
        av_free(proto_name);
        proto_name = NULL;
    }

    return ret;
}

static char *get_content_url(DashXmlNodePtr *baseurl_nodes,
        int n_baseurl_nodes,
        int max_url_size,
        char *rep_id_val,
        char *rep_bandwidth_val,
        char *val)
{
    int i;
    unsigned char *text;
    char *url = NULL;
    char *tmp_str = av_mallocz(max_url_size);
    if (!tmp_str) {
        return NULL;
    }
    char *tmp_str_2 = av_mallocz(max_url_size);
    if (!tmp_str_2) {
        if (tmp_str) {
            av_free(tmp_str);
            tmp_str = NULL;
        }
        return NULL;
    }

    for (i = 0; i < n_baseurl_nodes; ++i) {
#if defined(CONFIG_OPENSOURCE_LIBXML2)
        if (baseurl_nodes[i] &&
                baseurl_nodes[i]->children &&
                baseurl_nodes[i]->children->type == XML_TEXT_NODE) {
            text = xmlNodeGetContent(baseurl_nodes[i]->children);
#else
        if (baseurl_nodes[i] &&
                baseurl_nodes[i]->childs &&
                baseurl_nodes[i]->childs->type == XML_TEXT_NODE) {
            text = DashxmlNodeGetContent(baseurl_nodes[i]->childs);
#endif
            if (text) {
                memset(tmp_str, 0, max_url_size);
                memset(tmp_str_2, 0, max_url_size);
                ff_make_absolute_url(tmp_str_2, max_url_size, tmp_str, (char *)text);
                av_strlcpy(tmp_str, tmp_str_2, max_url_size);
                DashxmlFree(text);
            }
        }
    }

    if (val) {
        ff_make_absolute_url(tmp_str, max_url_size, tmp_str, val);
    }

    if (rep_id_val) {
        url = av_strireplace(tmp_str, "$RepresentationID$", (const char*)rep_id_val);
        if (!url) {
            goto end;
        }
        av_strlcpy(tmp_str, url, max_url_size);
    }
    if (rep_bandwidth_val && tmp_str[0] != '\0') {
        // free any previously assigned url before reassigning
        if (url)
            av_freep(&url);
        url = av_strireplace(tmp_str, "$Bandwidth$", (const char*)rep_bandwidth_val);
        if (!url) {
            goto end;
        }
    }
end:
    if (tmp_str) {
        av_free(tmp_str);
        tmp_str = NULL;
    }
    if (tmp_str_2) {
        av_free(tmp_str_2);
        tmp_str_2 = NULL;
    }
    return url;
}

static char *get_val_from_nodes_tab(DashXmlNodePtr *nodes, const int n_nodes, char *attrname)
{
    int i = 0;
    char *val;

    for (i = 0; i < n_nodes; ++i) {
        if (nodes[i]) {
            val = (char *)DashxmlGetProp(nodes[i], (unsigned char *)attrname);
            if (val)
                return val;
        }
    }

    return NULL;
}

static DashXmlNodePtr find_child_node_by_name(DashXmlNodePtr rootnode, const char *nodename)
{
    DashXmlNodePtr node = rootnode;
    if (!node) {
        return NULL;
    }

    node = DashxmlFirstElementChild(node);
    while (node) {
        if (!av_strcasecmp((const char *)node->name, nodename)) {
            return node;
        }
        node = DashxmlNextElementSibling(node);
    }
    return NULL;
}

static enum AVMediaType get_content_type(DashXmlNodePtr node)
{
    enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    int i = 0;
    const unsigned char *attr;
    unsigned char *val = NULL;

    if (node) {
        for (i = 0; i < 2; i++) {
            attr = (const unsigned char *)(i ? "mimeType" : "contentType");
            val = DashxmlGetProp(node, attr);
            if (val) {
                if (av_stristr((const char *)val, "video")) {
                    type = AVMEDIA_TYPE_VIDEO;
                } else if (av_stristr((const char *)val, "audio")) {
                    type = AVMEDIA_TYPE_AUDIO;
                } else if (av_stristr((const char *)val, "text")) {
                    type = AVMEDIA_TYPE_SUBTITLE;
                }
                DashxmlFree(val);
            }
        }
    }
    return type;
}

static struct fragment * get_Fragment(unsigned char *range)
{
    struct fragment * seg =  av_mallocz(sizeof(struct fragment));

    if (!seg)
        return NULL;

    seg->size = -1;
    if (range) {
        char *str_end_offset;
        char *str_offset = av_strtok((char *)range, "-", &str_end_offset);
        seg->url_offset = strtoll(str_offset, NULL, 10);
        seg->size = strtoll(str_end_offset, NULL, 10) - seg->url_offset+1;
    }

    return seg;
}

static int parse_manifest_contentprotection(AVFormatContext *s, DashXmlNodePtr node)
{
    int ret = AVERROR(EINVAL);
    char *val = NULL;
    DashxmlAttrPtr attr = NULL;
    DashXmlNodePtr contentprotection_node = NULL;
    struct ContentProtection *conpro = NULL;
    DASHContext *c = s->priv_data;
    if (!node)
        goto end;

    if (c->periods[c->current_period]->n_contentprotection >= MAX_CONTENT_PROTECTION_NUM) {
        goto end;
    }
    attr = node->properties;
    if (!(conpro = av_mallocz(sizeof(struct ContentProtection)))) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    while (attr) {
        val = (char *)DashxmlGetProp(node, (unsigned char *)attr->name);
        if (!av_strcasecmp((char *)attr->name, "value")) {
            if (!conpro->contentprotection_value) {
                conpro->contentprotection_value = av_strdup(val);
            }
        } else if (!av_strcasecmp((char *)attr->name, "default_KID")) {
            if (!conpro->contentprotection_default_kid)
                conpro->contentprotection_default_kid = av_strdup(val);
        } else if (!av_strcasecmp((char *)attr->name, "schemeIdUri")) {
            if (!conpro->contentprotection_schemeIduri)
                conpro->contentprotection_schemeIduri = av_strdup(val);
        }
        attr = attr->next;
        DashxmlFree(val);
    }
    contentprotection_node = DashxmlFirstElementChild(node);
    while (contentprotection_node) {
        if (!av_strcasecmp((char *)contentprotection_node->name, "pssh")) {
            char *val = NULL;
            val = (char *)DashxmlNodeGetContent(contentprotection_node);
            if (!conpro->contentprotection_pssh)
                conpro->contentprotection_pssh = av_strdup(val);
            DashxmlFree(val);
        }
        contentprotection_node = DashxmlNextElementSibling(contentprotection_node);
    }
    ret = av_dynarray_add_nofree(&c->periods[c->current_period]->contentprotection, &c->periods[c->current_period]->n_contentprotection, conpro);
    if (ret < 0) {
        if (conpro) {
            av_freep(&conpro);
        }
    }
end:
    return ret;
}

static int parse_manifest_segmenturlnode(AVFormatContext *s, struct representation *rep,
        DashXmlNodePtr fragmenturl_node,
        DashXmlNodePtr *baseurl_nodes,
        char *rep_id_val,
        char *rep_bandwidth_val)
{
    DASHContext *c = s->priv_data;
    unsigned char *initialization_val = NULL;
    unsigned char *media_val = NULL;
    unsigned char *range_val = NULL;
    int err = 0, def_max_url_size = 0, max_url_size = 0;

    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    max_url_size = c ? c->max_url_size: def_max_url_size;

    if (!av_strcasecmp((const char *)fragmenturl_node->name, (const char *)"Initialization")) {
        initialization_val = DashxmlGetProp(fragmenturl_node, (unsigned char *)"sourceURL");
        range_val = DashxmlGetProp(fragmenturl_node, (unsigned char *)"range");
        if (initialization_val || range_val) {
            free_fragment(&rep->init_section);
            rep->init_section = get_Fragment(range_val);
            DashxmlFree(range_val);
            if (!rep->init_section) {
                DashxmlFree(initialization_val);
                return AVERROR(ENOMEM);
            }
            rep->init_section->url = get_content_url(baseurl_nodes, 4, max_url_size,
                    rep_id_val, rep_bandwidth_val, (char *)initialization_val);
            DashxmlFree(initialization_val);
            if (!rep->init_section->url) {
                av_free(rep->init_section);
                return AVERROR(ENOMEM);
            }
        }
    } else if (!av_strcasecmp((const char *)fragmenturl_node->name, (const char *)"SegmentURL")) {
        media_val = DashxmlGetProp(fragmenturl_node, (unsigned char *)"media");
        range_val = DashxmlGetProp(fragmenturl_node, (unsigned char *)"mediaRange");
        if (media_val || range_val) {
            struct fragment *seg = get_Fragment(range_val);
            DashxmlFree(range_val);
            if (!seg) {
                DashxmlFree(media_val);
                return AVERROR(ENOMEM);
            }
            seg->url = get_content_url(baseurl_nodes, 4, max_url_size,
                    rep_id_val, rep_bandwidth_val, (char *)media_val);
            DashxmlFree(media_val);
            if (!seg->url) {
                if (seg) {
                    av_free(seg);
                    seg = NULL;
                }
                return AVERROR(ENOMEM);
            }
            err = av_dynarray_add_nofree(&rep->fragments, &rep->n_fragments, seg);
            if (err < 0) {
                free_fragment(&seg);
                return err;
            }
        }
    }

    return 0;
}

static int parse_manifest_segmenttimeline(AVFormatContext *s, struct representation *rep,
        DashXmlNodePtr fragment_timeline_node)
{
    DashxmlAttrPtr attr = NULL;
    unsigned char *val  = NULL;
    int err = 0;

    if (!av_strcasecmp((const char *)fragment_timeline_node->name, (const char *)"S")) {
        struct timeline *tml = av_mallocz(sizeof(struct timeline));
        if (!tml) {
            return AVERROR(ENOMEM);
        }
        attr = fragment_timeline_node->properties;
        while (attr) {
            val = DashxmlGetProp(fragment_timeline_node, attr->name);

            if (!val) {
                continue;
            }

            if (!av_strcasecmp((const char *)attr->name, (const char *)"t")) {
                tml->starttime = (int64_t)strtoll((char *)val, NULL, 10);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"r")) {
                tml->repeat =(int64_t) strtoll((char *)val, NULL, 10);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"d")) {
                tml->duration = (int64_t)strtoll((char *)val, NULL, 10);
            }
            attr = attr->next;
            DashxmlFree(val);
        }
        err = av_dynarray_add_nofree(&rep->timelines, &rep->n_timelines, tml);
        if (err < 0) {
            av_free(tml);
            return err;
        }

    }

    return 0;
}

static int resolve_content_path(AVFormatContext *s, char *url, int *max_url_size, DashXmlNodePtr *baseurl_nodes, int n_baseurl_nodes) {

    char *tmp_str = NULL;
    char *path = NULL;
    char *mpdName = NULL;
    DashXmlNodePtr node = NULL;
    unsigned char *baseurl = NULL;
    unsigned char *root_url = NULL;
    char *text = NULL;
    char *tmp = NULL;

    int isRootHttp = 0;
    char token ='/';
    int start =  0;
    int rootId = 0;
    int updated = 0;
    int size = 0;
    int i, def_max_url_size = 0;
    int tmp_max_url_size = strlen(url);

    GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
    tmp_max_url_size = (tmp_max_url_size > def_max_url_size)?def_max_url_size:tmp_max_url_size;

    for (i = n_baseurl_nodes-1; i >= 0 ; i--) {
        text = (char *)DashxmlNodeGetContent(baseurl_nodes[i]);
        if (!text)
            continue;
        tmp_max_url_size += strlen(text);
        if (ishttp(text)) {
            DashxmlFree(text);
            break;
        }
        DashxmlFree(text);
    }

    tmp_max_url_size = aligned(tmp_max_url_size);
    text = av_mallocz(tmp_max_url_size+2);
    if (!text) {
        updated = AVERROR(ENOMEM);
        goto end;
    }
    av_strlcpy(text, url, FFMIN(tmp_max_url_size+2, strlen(url)+1));
    tmp = text;
    while ((mpdName = av_strtok(tmp, "/", &tmp)))  {
        size = strlen(mpdName);
    }
    if (text) {
        av_free(text);
        text = NULL;
    }

    path = av_mallocz(tmp_max_url_size);
    tmp_str = av_mallocz(tmp_max_url_size);
    if (!tmp_str || !path) {
        updated = AVERROR(ENOMEM);
        goto end;
    }

    av_strlcpy (path, url, FFMIN(tmp_max_url_size, strlen(url) - size + 1));
    for (rootId = n_baseurl_nodes - 1; rootId > 0; rootId --) {
        if (!(node = baseurl_nodes[rootId])) {
            continue;
        }
        text = (char *)DashxmlNodeGetContent(node);
        if (ishttp(text)) {
            DashxmlFree(text);
            break;
        }
        DashxmlFree(text);
    }

    node = baseurl_nodes[rootId];
    baseurl = DashxmlNodeGetContent(node);
    if (!baseurl) {
        root_url = (unsigned char *)path;
    } else {
        root_url = (unsigned char*)((av_strcasecmp((char *)baseurl, "")) ? baseurl : ((unsigned char*)path));
    }

    if (node) {
#if defined(CONFIG_OPENSOURCE_LIBXML2)
        xmlNodeSetContent(node, root_url);
#else
        GXxmlNodeSetContent(node, root_url);
#endif
        updated = 1;
    }

    size = strlen((char*)root_url);
    isRootHttp = ishttp((char*)root_url);

    if (size > 0 && root_url[size - 1] != token) {
        av_strlcat((char*)root_url, "/", size + 2);
        size += 2;
    }

    for (i = 0; i < n_baseurl_nodes; ++i) {
        if (i == rootId) {
            continue;
        }
        text = (char *)DashxmlNodeGetContent(baseurl_nodes[i]);
        if (text) {
            memset(tmp_str, 0, strlen(tmp_str));
            if (!ishttp(text) && isRootHttp) {
                av_strlcpy(tmp_str, (char*)root_url, FFMIN(tmp_max_url_size, size + 1));
            }
            start = (text[0] == token);
            av_strlcat(tmp_str, text + start, tmp_max_url_size);
#if defined(CONFIG_OPENSOURCE_LIBXML2)
            xmlNodeSetContent(baseurl_nodes[i], (unsigned char*)tmp_str);
#else
            GXxmlNodeSetContent(baseurl_nodes[i], (unsigned char*)tmp_str);
#endif
            updated = 1;
            DashxmlFree(text);
        }
    }

end:
    if (tmp_max_url_size > *max_url_size) {
        *max_url_size = tmp_max_url_size;
    }
    if (path)
        av_free(path);
    if (tmp_str)
        av_free(tmp_str);
    DashxmlFree(baseurl);
    return updated;

}

static int parse_manifest_representation(AVFormatContext *s, char *url,
        DashXmlNodePtr node,
        DashXmlNodePtr adaptionset_node,
        DashXmlNodePtr mpd_baseurl_node,
        DashXmlNodePtr period_baseurl_node,
        DashXmlNodePtr period_segmenttemplate_node,
        DashXmlNodePtr period_segmentlist_node,
        DashXmlNodePtr fragment_template_node,
        DashXmlNodePtr content_component_node,
        DashXmlNodePtr adaptionset_baseurl_node,
        DashXmlNodePtr adaptionset_segmentlist_node,
        DashXmlNodePtr adaptionset_supplementalproperty_node)
{
    int32_t ret = 0;
    DASHContext *c = s->priv_data;
    struct representation *rep = NULL;
    struct fragment *seg = NULL;
    DashXmlNodePtr representation_segmenttemplate_node = NULL;
    DashXmlNodePtr representation_baseurl_node = NULL;
    DashXmlNodePtr representation_segmentlist_node = NULL;
    DashXmlNodePtr representation_contentprotection_node = NULL;
    DashXmlNodePtr one_contentprotection_node = NULL;
    DashXmlNodePtr segmentlists_tab[3];
    DashXmlNodePtr fragment_timeline_node = NULL;
    DashXmlNodePtr fragment_templates_tab[5];
    DashXmlNodePtr baseurl_nodes[4];
    DashXmlNodePtr representation_node = node;
    char *val = NULL, *val1 = NULL;
    char *rep_bandwidth_val;
    enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;

    // try get information from representation
    if (type == AVMEDIA_TYPE_UNKNOWN)
        type = get_content_type(representation_node);
    // try get information from contentComponen
    if (type == AVMEDIA_TYPE_UNKNOWN)
        type = get_content_type(content_component_node);
    // try get information from adaption set
    if (type == AVMEDIA_TYPE_UNKNOWN)
        type = get_content_type(adaptionset_node);
    if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO &&
            type != AVMEDIA_TYPE_SUBTITLE) {
        gxloge_raw_l ("Parsing '%s' - skipp not supported representation type\n", url);
        return 0;
    }

    // convert selected representation to our internal struct
    rep = av_mallocz(sizeof(struct representation));
    if (!rep) {
        return AVERROR(ENOMEM);
    }
    if ((c->current_period >= 0) && c->periods[c->current_period]->adaptionset_lang) {
        rep->lang = av_strdup(c->periods[c->current_period]->adaptionset_lang);
        if (!rep->lang) {
            gxloge ("alloc language memory failure\n");
            av_freep(&rep);
            return AVERROR(ENOMEM);
        }
    }
    rep->parent = s;
    representation_segmenttemplate_node = find_child_node_by_name(representation_node, "SegmentTemplate");
    representation_baseurl_node = find_child_node_by_name(representation_node, "BaseURL");
    representation_segmentlist_node = find_child_node_by_name(representation_node, "SegmentList");
    rep_bandwidth_val = (char *)DashxmlGetProp(representation_node, (unsigned char *)"bandwidth");
    representation_contentprotection_node = find_child_node_by_name(representation_node, "ContentProtection");
    val = (char *)DashxmlGetProp(representation_node, (unsigned char *)"id");
    if (val) {
        rep->id = av_strdup(val);
        DashxmlFree(val);
        if (!rep->id) {
            goto enomem;
        }
    }
    val1 = (char *)DashxmlGetProp(representation_node, (unsigned char *)"codecs");
    if (val1) {
        rep->codecs = av_strdup(val1);
        DashxmlFree(val1);
        if (!rep->codecs) {
            goto enomem;
        }
    }
    val1 = (char *)DashxmlGetProp(representation_node, (unsigned char *)"width");
    if (val1) {
        rep->width = av_strdup(val1);
        DashxmlFree(val1);
        if (!rep->width) {
            goto enomem;
        }
    }
    val1 = (char *)DashxmlGetProp(representation_node, (unsigned char *)"height");
    if (val1) {
        rep->height = av_strdup(val1);
        DashxmlFree(val1);
        if (!rep->height) {
            goto enomem;
        }
    }
    val1 = (char *)DashxmlGetProp(representation_node, (unsigned char *)"sar");
    if (val1) {
        rep->sar = av_strdup(val1);
        DashxmlFree(val1);
        if (!rep->sar) {
            goto enomem;
        }
    }
    val1 = (char *)DashxmlGetProp(representation_node, (unsigned char *)"audioSamplingRate");
    if (val1) {
        rep->samplerate = av_strdup(val1);
        DashxmlFree(val1);
        if (!rep->samplerate) {
            goto enomem;
        }
    }

    baseurl_nodes[0] = mpd_baseurl_node;
    baseurl_nodes[1] = period_baseurl_node;
    baseurl_nodes[2] = adaptionset_baseurl_node;
    baseurl_nodes[3] = representation_baseurl_node;
    ret = resolve_content_path(s, url, &c->max_url_size, baseurl_nodes, 4);
    c->max_url_size = aligned(c->max_url_size
            + (rep->id ? strlen(rep->id) : 0)
            + (rep_bandwidth_val ? strlen(rep_bandwidth_val) : 0));
    if (ret == AVERROR(ENOMEM) || ret == 0)
        goto free;

    if (representation_contentprotection_node) {
        one_contentprotection_node = DashxmlFirstElementChild(node);
        while (one_contentprotection_node) {
            if (!av_strcasecmp((char *)one_contentprotection_node->name, "ContentProtection")) {
                parse_manifest_contentprotection(s, one_contentprotection_node);
            }
            one_contentprotection_node = DashxmlNextElementSibling(one_contentprotection_node);
        }
    }

    if (representation_segmenttemplate_node || fragment_template_node || period_segmenttemplate_node) {
        fragment_timeline_node = NULL;
        fragment_templates_tab[0] = representation_segmenttemplate_node;
        fragment_templates_tab[1] = adaptionset_segmentlist_node;
        fragment_templates_tab[2] = fragment_template_node;
        fragment_templates_tab[3] = period_segmenttemplate_node;
        fragment_templates_tab[4] = period_segmentlist_node;
        val = get_val_from_nodes_tab(fragment_templates_tab, 4, "initialization");
        if (val) {
            rep->init_section = av_mallocz(sizeof(struct fragment));
            if (!rep->init_section) {
                DashxmlFree(val);
                goto enomem;
            }
            c->max_url_size = aligned(c->max_url_size  + strlen(val));
            rep->init_section->url = get_content_url(baseurl_nodes, 4, c->max_url_size, rep->id, rep_bandwidth_val, val);
            DashxmlFree(val);
            if (!rep->init_section->url)
                goto enomem;
            rep->init_section->size = -1;
        }
        val = get_val_from_nodes_tab(fragment_templates_tab, 4, "media");
        if (val) {
            c->max_url_size = aligned(c->max_url_size  + strlen(val));
            rep->url_template = get_content_url(baseurl_nodes, 4, c->max_url_size, rep->id, rep_bandwidth_val, val);
            DashxmlFree(val);
            if (!rep->url_template) {
                goto enomem;
            }
        }
        val = get_val_from_nodes_tab(fragment_templates_tab, 4, "presentationTimeOffset");
        if (val) {
            rep->presentation_timeoffset = (int64_t) strtoll(val, NULL, 10);
            gxlogd ("rep->presentation_timeoffset =%lld\n", rep->presentation_timeoffset);
            DashxmlFree(val);
        }
        val = get_val_from_nodes_tab(fragment_templates_tab, 4, "duration");
        if (val) {
            rep->fragment_duration = (int64_t) strtoll(val, NULL, 10);
            gxlogd ("rep->fragment_duration =%lld\n", rep->fragment_duration);
            DashxmlFree(val);
        }
        val = get_val_from_nodes_tab(fragment_templates_tab, 4, "timescale");
        if (val) {
            rep->fragment_timescale = (int64_t) strtoll(val, NULL, 10);
            gxlogd ("rep->fragment_timescale =%lld\n", rep->fragment_timescale);
            DashxmlFree(val);
        }
        val = get_val_from_nodes_tab(fragment_templates_tab, 4, "startNumber");
        if (val) {
            rep->start_number = rep->first_seq_no = (int64_t) strtoll(val, NULL, 10);
            gxlogd ("rep->first_seq_no =%lld\n", rep->first_seq_no);
            DashxmlFree(val);
        }
        if (adaptionset_supplementalproperty_node) {
            char *scheme_id_uri = (char *)DashxmlGetProp(adaptionset_supplementalproperty_node, (unsigned char *)"schemeIdUri");
            if (scheme_id_uri) {
                int is_last_segment_number = !av_strcasecmp(scheme_id_uri, "http://dashif.org/guidelines/last-segment-number");
                DashxmlFree(scheme_id_uri);
                if (is_last_segment_number) {
                    val = (char *)DashxmlGetProp(adaptionset_supplementalproperty_node, (unsigned char *)"value");
                    if (!val) {
                        gxlogd ("Missing value attribute in adaptionset_supplementalproperty_node\n");
                    } else {
                        rep->last_seq_no = (int64_t)strtoll(val, NULL, 10) - 1;
                        DashxmlFree(val);
                    }
                }
            }
        }

        fragment_timeline_node = find_child_node_by_name(representation_segmenttemplate_node, "SegmentTimeline");
        if (!fragment_timeline_node)
            fragment_timeline_node = find_child_node_by_name(fragment_template_node, "SegmentTimeline");
        if (!fragment_timeline_node)
            fragment_timeline_node = find_child_node_by_name(adaptionset_segmentlist_node, "SegmentTimeline");
        if (!fragment_timeline_node)
            fragment_timeline_node = find_child_node_by_name(period_segmentlist_node, "SegmentTimeline");
        if (fragment_timeline_node) {
            fragment_timeline_node = DashxmlFirstElementChild(fragment_timeline_node);
            while (fragment_timeline_node) {
                ret = parse_manifest_segmenttimeline(s, rep, fragment_timeline_node);
                if (ret < 0)
                    goto free;
                fragment_timeline_node = DashxmlNextElementSibling(fragment_timeline_node);
            }
        }
    } else if (representation_baseurl_node && !representation_segmentlist_node) {
        seg = av_mallocz(sizeof(struct fragment));
        if (!seg)
            goto enomem;
        ret = av_dynarray_add_nofree(&rep->fragments, &rep->n_fragments, seg);
        if (ret < 0) {
            av_free(seg);
            goto free;
        }
        seg->url = get_content_url(baseurl_nodes, 4, c->max_url_size, rep->id, rep_bandwidth_val, NULL);
        if (!seg->url)
            goto enomem;
        seg->size = -1;
    } else if (representation_segmentlist_node) {
        // TODO: https://www.brendanlong.com/the-structure-of-an-mpeg-dash-mpd.html
        // http://www-itec.uni-klu.ac.at/dash/ddash/mpdGenerator.php?fragmentlength=15&type=full
        DashXmlNodePtr fragmenturl_node = NULL;
        segmentlists_tab[0] = representation_segmentlist_node;
        segmentlists_tab[1] = adaptionset_segmentlist_node;
        segmentlists_tab[2] = period_segmentlist_node;

        val = get_val_from_nodes_tab(segmentlists_tab, 3, "duration");
        if (val) {
            rep->fragment_duration = (int64_t) strtoll(val, NULL, 10);
            DashxmlFree(val);
        }
        val = get_val_from_nodes_tab(segmentlists_tab, 3, "timescale");
        if (val) {
            rep->fragment_timescale = (int64_t) strtoll(val, NULL, 10);
            DashxmlFree(val);
        }
        val = get_val_from_nodes_tab(segmentlists_tab, 3, "startNumber");
        if (val) {
            rep->start_number = rep->first_seq_no = (int64_t) strtoll(val, NULL, 10);
            DashxmlFree(val);
        }

        fragmenturl_node = DashxmlFirstElementChild(representation_segmentlist_node);
        while (fragmenturl_node) {
            ret = parse_manifest_segmenturlnode(s, rep, fragmenturl_node, baseurl_nodes, rep->id, rep_bandwidth_val);
            if (ret < 0)
                goto free;
            fragmenturl_node = DashxmlNextElementSibling(fragmenturl_node);
        }

        fragment_timeline_node = find_child_node_by_name(adaptionset_segmentlist_node, "SegmentTimeline");
        if (!fragment_timeline_node)
            fragment_timeline_node = find_child_node_by_name(period_segmentlist_node, "SegmentTimeline");
        if (fragment_timeline_node) {
            fragment_timeline_node = DashxmlFirstElementChild(fragment_timeline_node);
            while (fragment_timeline_node) {
                ret = parse_manifest_segmenttimeline(s, rep, fragment_timeline_node);
                if (ret < 0)
                    goto free;
                fragment_timeline_node = DashxmlNextElementSibling(fragment_timeline_node);
            }
        }
    } else {
        gxloge_raw_l ("Unknown format of Representation node id '%s' \n", rep->id ? rep->id : "");
        goto free;
    }

    if (rep->fragment_duration > 0 && !rep->fragment_timescale)
        rep->fragment_timescale = 1;
    rep->bandwidth = rep_bandwidth_val ? atoi(rep_bandwidth_val) : 0;
    rep->framerate = av_make_q(0, 0);
    if (type == AVMEDIA_TYPE_VIDEO) {
        char *rep_framerate_val = (char *)DashxmlGetProp(representation_node, (unsigned char *)"frameRate");
        if (rep_framerate_val) {
            ret = av_parse_video_rate(&rep->framerate, rep_framerate_val);
            if (ret < 0)
                gxlogi_raw_l ("Ignoring invalid frame rate '%s'\n", rep_framerate_val);
            DashxmlFree(rep_framerate_val);
        }
    }
    if (auto_filter_decoder_unsupporrt_type(rep)) {
        goto free;
    }
    switch (type) {
        case AVMEDIA_TYPE_VIDEO:
            ret = av_dynarray_add_nofree(&c->periods[c->current_period]->videos, &c->periods[c->current_period]->n_videos, rep);
            break;
        case AVMEDIA_TYPE_AUDIO:
            ret = av_dynarray_add_nofree(&c->periods[c->current_period]->audios, &c->periods[c->current_period]->n_audios, rep);
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            ret = av_dynarray_add_nofree(&c->periods[c->current_period]->subtitles, &c->periods[c->current_period]->n_subtitles, rep);
            break;
        default:
            break;
    }
    if (ret < 0)
        goto free;

end:
    if (rep_bandwidth_val)
        DashxmlFree(rep_bandwidth_val);
    return ret;
enomem:
    ret = AVERROR(ENOMEM);
free:
    free_representation(rep);
    goto end;
}

static int parse_manifest_adaptationset_attr(AVFormatContext *s, DashXmlNodePtr adaptionset_node)
{
    DASHContext *c = s->priv_data;

    if (!c || !adaptionset_node || (c->current_period < 0) || (c->n_periods < 0) || (c->current_period >= c->n_periods)) {
        gxlogd ("Cannot get AdaptionSet\n");
        return AVERROR(EINVAL);
    }
    c->periods[c->n_periods - 1]->adaptionset_lang = (char *)DashxmlGetProp(adaptionset_node, (unsigned char *)"lang");

    return 0;
}

static int parse_manifest_adaptationset(AVFormatContext *s, char *url,
        DashXmlNodePtr adaptionset_node,
        DashXmlNodePtr mpd_baseurl_node,
        DashXmlNodePtr period_baseurl_node,
        DashXmlNodePtr period_segmenttemplate_node,
        DashXmlNodePtr period_segmentlist_node)
{
    int ret = 0;
    DASHContext *c = s->priv_data;
    DashXmlNodePtr fragment_template_node = NULL;
    DashXmlNodePtr content_component_node = NULL;
    DashXmlNodePtr adaptionset_baseurl_node = NULL;
    DashXmlNodePtr adaptionset_segmentlist_node = NULL;
    DashXmlNodePtr adaptionset_supplementalproperty_node = NULL;
    DashXmlNodePtr node = NULL;

    if ((c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods)) {
        return AVERROR(EINVAL);
    }

    ret = parse_manifest_adaptationset_attr(s, adaptionset_node);
    if (ret < 0)
        return ret;

    node = DashxmlFirstElementChild(adaptionset_node);
    while (node) {
        if (!av_strcasecmp((const char *)node->name, (const char *)"SegmentTemplate")) {
            fragment_template_node = node;
        } else if (!av_strcasecmp((const char *)node->name, (const char *)"ContentComponent")) {
            content_component_node = node;
        } else if (!av_strcasecmp((const char *)node->name, (const char *)"BaseURL")) {
            adaptionset_baseurl_node = node;
        } else if (!av_strcasecmp((const char *)node->name, (const char *)"SegmentList")) {
            adaptionset_segmentlist_node = node;
        } else if (!av_strcasecmp((const char *)node->name, (const char *)"SupplementalProperty")) {
            adaptionset_supplementalproperty_node = node;
        } else if (!av_strcasecmp((const char *)node->name, "ContentProtection")) {
            parse_manifest_contentprotection(s, node);
        } else if (!av_strcasecmp((const char *)node->name, (const char *)"Representation")) {
            ret = parse_manifest_representation(s, url, node,
                    adaptionset_node,
                    mpd_baseurl_node,
                    period_baseurl_node,
                    period_segmenttemplate_node,
                    period_segmentlist_node,
                    fragment_template_node,
                    content_component_node,
                    adaptionset_baseurl_node,
                    adaptionset_segmentlist_node,
                    adaptionset_supplementalproperty_node);
            if (ret < 0) {
                goto err;
            }
        }
        node = DashxmlNextElementSibling(node);
    }
err:
    if (c->periods[c->current_period]->adaptionset_lang) {
        DashxmlFree(c->periods[c->current_period]->adaptionset_lang);
        c->periods[c->current_period]->adaptionset_lang = NULL;
    }
    return ret;
}

static int parse_programinformation(AVFormatContext *s, DashXmlNodePtr node)
{
    unsigned char *val = NULL;

    node = DashxmlFirstElementChild(node);
    while (node) {
        if (!av_strcasecmp((char *)node->name, "Title")) {
            val = DashxmlNodeGetContent(node);
            if (val) {
                av_dict_set(&s->metadata, "Title", (char *)val, 0);
            }
        } else if (!av_strcasecmp((char *)node->name, "Source")) {
            val = DashxmlNodeGetContent(node);
            if (val) {
                av_dict_set(&s->metadata, "Source", (char *)val, 0);
            }
        } else if (!av_strcasecmp((char *)node->name, "Copyright")) {
            val = DashxmlNodeGetContent(node);
            if (val) {
                av_dict_set(&s->metadata, "Copyright", (char *)val, 0);
            }
        }
        node = DashxmlNextElementSibling(node);
        DashxmlFree(val);
    }
    return 0;
}

int dash_avio_read_to_bprint(AVIOContext *h, AVBPrint *pb, size_t max_size)
{
    int ret = 0;
    char buf[1024];
    while (max_size) {
        ret = avio_read(h, buf, FFMIN(max_size, sizeof(buf)));
        if (ret == AVERROR_EOF)
            return 0;
        if (ret <= 0)
            return ret;
        av_bprint_append_data(pb, buf, ret);
        if (!av_bprint_is_complete(pb))
            return AVERROR(ENOMEM);
        max_size -= ret;
    }
    return 0;
}

static int parse_manifest_period(AVFormatContext* s, char *url, DashXmlNodePtr period_node, DashXmlNodePtr mpd_baseurl_node, int is_refresh)
{
    int ret = 0, max_periods = 0;
    DASHContext* c = s->priv_data;
    unsigned char* val = NULL;
    Period* pd = NULL;
    DashxmlAttrPtr attr = NULL;
    DashXmlNodePtr period_baseurl_node = NULL;
    DashXmlNodePtr adaptionset_node = NULL;
    DashXmlNodePtr period_segmenttemplate_node = NULL;
    DashXmlNodePtr period_segmentlist_node = NULL;

    GxPlayer_SystemGet(PSYS_MPEGDASH_MAX_PERIODS, &max_periods);
    if (c->n_periods >= max_periods) {
        gxlogi ("exceeds default values.(default:%d)\n", max_periods);
        return 0;
    }

    if (!is_refresh) {
        if (!(pd = av_mallocz(sizeof(Period)))) {
            return AVERROR(ENOMEM);
        }
    } else {
        pd = c->periods[((c->current_period<0)?0:c->current_period)];
    }
    pd->period_duration = -1;
    pd->period_start = -1;

    // parse period attrs
    attr = period_node->properties;
    while (attr) {
        val = DashxmlGetProp(period_node, attr->name);
        if (!av_strcasecmp((const char *)attr->name, (const char *)"duration")) {
            pd->period_duration = get_duration_insec(s, (const char *)val);
        } else if (!av_strcasecmp((const char *)attr->name, (const char *)"start")) {
            pd->period_start = get_duration_insec(s, (const char *)val);
        }
        attr = attr->next;
        DashxmlFree(val);
    }
    if (c->n_periods > 1 && pd->period_duration == -1) {
        gxloge ("missing duration attr in multi-period\n");
        if (pd)
            av_freep(&pd);
        return -1;
    }

    //gx add begin :
#if defined(CONFIG_OPENSOURCE_LIBXML2)
    char *mpd_Base_url = (char *)xmlNodeGetContent(mpd_baseurl_node->children);
#else
    char *mpd_Base_url = (char *)DashxmlNodeGetContent(mpd_baseurl_node->childs);
#endif
    /*This is a special case, deserving special treatment.*/
    if (mpd_Base_url && strstr(mpd_Base_url, "../../")) {
        int def_max_url_size = 0;
        char *sep_url= NULL;
        char *temp_base_url = NULL;
        GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);

        if (!(temp_base_url = av_mallocz(def_max_url_size))) {
            return AVERROR(ENOMEM);
        }
        if (strrchr(c->base_url, '/')) {
            sep_url = av_mallocz(strlen(c->base_url));
            if (!sep_url) {
                if (pd) {
                    av_freep(&pd);
                }
                if (temp_base_url) {
                    av_freep(&temp_base_url);
                }
                return AVERROR(ENOMEM);
            }
            memcpy (sep_url, c->base_url, strlen(c->base_url)-1);
            av_free(c->base_url);
            if (strrchr(sep_url, '/')) {
                c->base_url = av_mallocz(strlen(sep_url));
                if (!c->base_url) {
                    if (sep_url)
                        av_free(sep_url);
                    if (pd)
                        av_free(pd);
                    if (temp_base_url)
                        av_free(temp_base_url);
                    return AVERROR(ENOMEM);
                }
                memcpy (c->base_url, sep_url, strlen(sep_url)-1);
                av_freep(&sep_url);
                if (strrchr(c->base_url, '/')) {
                    snprintf (temp_base_url, sizeof(temp_base_url)-1,"%s%s", c->base_url, mpd_Base_url+strlen("../../"));
                    av_free(c->base_url);
                    c->base_url = av_strdup(temp_base_url);
                    if (!c->base_url) {
                        if (pd)
                            av_freep(&pd);
                        if (temp_base_url) {
                            av_freep(&temp_base_url);
                        }
                        return AVERROR(ENOMEM);
                    }
                }
            }
        }
        if (temp_base_url) {
            av_freep(&temp_base_url);
        }
    }
    //gx add end:

    if (!is_refresh) {
        ret = av_dynarray_add_nofree(&c->periods, &c->n_periods, pd);
        if (ret < 0) {
            if (pd)
                av_freep(&pd);
            return ret;
        }
        c->current_period = FFMAX(c->n_periods - 1, 0);
    } else {
        c->n_periods += 1;
        c->current_period = FFMAX(c->n_periods - 1, 0);
    }

    // parse adaptation recursively 
    adaptionset_node = DashxmlFirstElementChild(period_node);
    while (adaptionset_node) {
        if (!av_strcasecmp((const char *)adaptionset_node->name, (const char *)"BaseURL")) {
            period_baseurl_node = adaptionset_node;
        } else if (!av_strcasecmp((const char *)adaptionset_node->name, (const char *)"SegmentTemplate")) {
            period_segmenttemplate_node = adaptionset_node;
        } else if (!av_strcasecmp((const char *)adaptionset_node->name, (const char *)"SegmentList")) {
            period_segmentlist_node = adaptionset_node;
        } else if (!av_strcasecmp((const char *)adaptionset_node->name, (const char *)"AdaptationSet")) {
            ret = parse_manifest_adaptationset(s, url, adaptionset_node, mpd_baseurl_node, period_baseurl_node, period_segmenttemplate_node, period_segmentlist_node);
            if (AVERROR(ENOMEM) == ret) {
                break;
            }
        }
        adaptionset_node = DashxmlNextElementSibling(adaptionset_node);
    }

    return ret;
}

#if !defined(CONFIG_OPENSOURCE_LIBXML2)
extern GXxmlDocPtr GXxmlParseBuffer(const char *filename, char *buffer);
#endif
extern int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags);
static int parse_manifest(AVFormatContext *s, char *url, AVIOContext *in, int is_refresh)
{
    DASHContext *c = s->priv_data;
    int ret = 0, flags = 0, close_in = 0, recv_buf_size = 0, is_use_flase_live = 0, i =0;
    int64_t filesize = 0;
    AVBPrint buf;
    AVDictionary *opts = NULL;
#if defined(CONFIG_OPENSOURCE_LIBXML2)
    xmlDoc *doc = NULL;
#else
    GXxmlDocPtr doc = NULL;
#endif
    DashXmlNodePtr root_element = NULL;
    DashXmlNodePtr node = NULL;
    DashXmlNodePtr tmp_node = NULL;
    DashXmlNodePtr mpd_baseurl_node = NULL;
    DashxmlAttrPtr attr = NULL;
    unsigned char *val  = NULL;
    AVDictionaryEntry *e = NULL;

    if (!url) {
        return AVERROR(EINVAL);
    }
    int is_http = av_strstart(url, "http", NULL);
    GxPlayer_SystemGet(PSYS_NETWORK_IS_FALSE_LIVE, &is_use_flase_live);
    if (is_http && !in && c->http_persistent && c->keepalive_manifest && (!is_use_flase_live)) {
        in = c->keepalive_manifest;
        av_dict_set(&opts, "multiple_requests", "1", 0);
        ret = open_url_keepalive(&c->keepalive_manifest, url, &opts);
        av_dict_free(&opts);
        if (ret == AVERROR_EXIT) {
            return ret;
        } else if (ret < 0) {
            if (ret != AVERROR_EOF)
                gxlogi_raw_l("keepalive request failed for '%s' with error:'%s' when parsing playlist\n", url, av_err2str(ret));
            in = NULL;
        }
    }
    if (!in) {
        if (!is_use_flase_live && c->http_persistent)
            av_dict_set(&opts, "multiple_requests", "1", 0);
        av_dict_copy(&opts, c->avio_opts, 0);
        if ((e = av_dict_get(opts, "avioflags", NULL, 0))) {
            flags = atoi(e->value);
        }
        GxPlayer_SystemGet(PSYS_NETWORK_RECVBUF, &recv_buf_size);
        av_dict_set_int(&opts, "ff_recvbuf_size", recv_buf_size, 0);
        av_dict_set_int(&opts, "rw_timeout", 9, 0);//set timeout:9S.
        ret = avio_open2(&in, url, AVIO_FLAG_READ | flags, &opts);
        av_dict_free(&opts);
        if (is_http && c->http_persistent && (!is_use_flase_live)) {
            c->keepalive_manifest = in;
        } else {
            close_in = 1;
        }
        if (ret < 0)
            return ret;
    }
    c->base_url = av_strdup(url);
    if (!(c->base_url)) {
        if (close_in) {
            avio_close(in);
        }
        return AVERROR(ENOMEM);
    }
    /*********************************************************************************************************
      if player https://wptv.wpcdn.pl/2991716-chunked/manifest.mpd?token=t292bf562328b7a9b5fc438a196bb90409,
      get Content-Range is proble.
     **********************************************************************************************************/
    filesize = avio_size(in);
    if (filesize <= 0) {
        filesize = DEFAULT_MANIFEST_SIZE;//default manifest size:8KB
    }
    if (filesize > MAX_BPRINT_READ_SIZE) {
        gxloge ("Manifest too large: %lld.\n", filesize);
        return AVERROR_INVALIDDATA;
    }

    if (is_refresh) {
        c->n_periods = 0;
        c->current_period = 0;
    }

    av_bprint_init(&buf, filesize + 1, AV_BPRINT_SIZE_UNLIMITED);
    dash_avio_read_to_bprint(in, &buf, MAX_BPRINT_READ_SIZE);
    if (!avio_feof(in) || filesize <= 0 || buf.len <= 0) {
        gxloge_raw_l ("Unable to read to manifest '%s'\n", url);
        ret = AVERROR_INVALIDDATA;
    } else {
#if defined(CONFIG_OPENSOURCE_LIBXML2)
        doc = xmlReadMemory(buf.str, filesize, c->base_url, NULL, 0);
        root_element = xmlDocGetRootElement(doc);
#else
        doc = GXxmlParseBuffer(c->base_url, buf.str);
        root_element = doc->root;
#endif
        node = root_element;

        if (!node) {
            ret = AVERROR_INVALIDDATA;
            gxloge_raw_l ("Unable to parse '%s' - missing root node\n", url);
            goto cleanup;
        }

        if (node->type != XML_ELEMENT_NODE ||
                av_strcasecmp((const char *)node->name, (const char *)"MPD")) {
            ret = AVERROR_INVALIDDATA;
            gxloge_raw_l ("Unable to parse '%s' - wrong root node name[%s] type[%d]\n", url, node->name, (int)node->type);
            goto cleanup;
        }

        val = DashxmlGetProp(node, (unsigned char*)"type");
        if (!val) {
            gxloge_raw_l ("Unable to parse '%s' - missing type attrib\n", url);
            ret = AVERROR_INVALIDDATA;
            goto cleanup;
        }

        if (!av_strcasecmp((const char *)val, (const char *)"dynamic"))
            c->is_live = 1;
        else
            c->is_live = 0;

        DashxmlFree(val);
        attr = node->properties;
        while (attr) {
            if (url_check_interrupt_cb()) {
                ret = AVERROR_EXIT;
                goto cleanup;
            }
            val = DashxmlGetProp(node, attr->name);
            if (!av_strcasecmp((const char *)attr->name, (const char *)"availabilityStartTime")) {
                c->availability_start_time = get_utc_date_time_insec(s, (const char *)val);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"availabilityEndTime")) {
                c->availability_end_time = get_utc_date_time_insec(s, (const char *)val);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"publishTime")) {
                c->publish_time = get_utc_date_time_insec(s, (const char *)val);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"minimumUpdatePeriod")) {
                c->minimum_update_period = get_duration_insec(s, (const char *)val);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"timeShiftBufferDepth")) {
                c->time_shift_buffer_depth = get_duration_insec(s, (const char *)val);
            } else if(!av_strcasecmp((const char *)attr->name, (const char *)"maxSegmentDuration")){
                c->maxSegmentDuration = get_duration_insec(s, (const char *)val);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"minBufferTime")) {
                c->min_buffer_time = get_duration_insec(s, (const char *)val);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"suggestedPresentationDelay")) {
                c->suggested_presentation_delay = get_duration_insec(s, (const char *)val);
            } else if (!av_strcasecmp((const char *)attr->name, (const char *)"mediaPresentationDuration")) {
                c->media_presentation_duration = get_duration_insec(s, (const char *)val);
            }
            attr = attr->next;
            DashxmlFree(val);
        }

        tmp_node = find_child_node_by_name(node, "BaseURL");
        if (tmp_node) {
            mpd_baseurl_node = DashxmlCopyNode(tmp_node,1);
        } else {
            mpd_baseurl_node = DashxmlNewNode(NULL, (unsigned char*)"BaseURL");
        }

        // at now we can handle only one period, with the longest duration
        node = DashxmlFirstElementChild(node);
        while (node) {
            if (url_check_interrupt_cb()) {
                ret = AVERROR_EXIT;
                goto cleanup;
            }
            if (!av_strcasecmp((const char *)node->name, (const char *)"Period")) {
                ret = parse_manifest_period(s, url, node, mpd_baseurl_node, is_refresh);
                if (ret < 0) {
                    free_period_list(c);
                    goto cleanup;
                }
            } else if (!av_strcasecmp((const char *)node->name, (const char *)"ProgramInformation")) {
                parse_programinformation(s, node);
            }
            node = DashxmlNextElementSibling(node);
        }
cleanup:
        /*free the document */
        DashxmlFreeDoc(doc);
#if defined(CONFIG_OPENSOURCE_LIBXML2)
        xmlCleanupParser();
#endif
        DashxmlFreeNode(mpd_baseurl_node);
    }
    av_bprint_finalize(&buf, NULL);
    if (close_in) {
        avio_close(in);
    }
    c->current_period = 0;
    // fill in period parameters by auto-reference.
    // duration is present for each period
    for (i = 0; i < c->n_periods; ++i) {
        if (c->periods[i]->period_start == -1) {
            if (i == 0 && !c->is_live) {
                c->periods[i]->period_start = 0;
            }
            // for i==0 && c->is_live, it's early available periods.
            if (i != 0) {
                c->periods[i]->period_start = c->periods[i - 1]->period_start + c->periods[i - 1]->period_duration;
            }
        }
    }
    return ret;
}

static int64_t calc_lines_repeat(int is_live, struct representation *pls)
{
    int64_t num = 0;
    if (is_live && pls->n_timelines) {
        int i = 0;
        num = pls->n_timelines;
        for (i = 0; i < pls->n_timelines; i++) {
            num += pls->timelines[i]->repeat;
            gxlogd ("calc_lines_repeat num=%lld..idx=%d..repeat=%d.\n",num, i, pls->timelines[i]->repeat);
        }
    }
    return num;
}

static int64_t calc_cur_seg_no(AVFormatContext *s, struct representation *pls)
{
    DASHContext *c = s->priv_data;
    int is_full_detection = 0;
    int64_t num = 0, start_time_offset = 0, repeat = 0;

    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

    if (c->is_live) {
        if (pls->n_fragments) {
            num = pls->first_seq_no;
        } else if (pls->n_timelines) {
            start_time_offset = get_segment_start_time_based_on_timeline(pls, 0xFFFFFFFF) - 60 * pls->fragment_timescale; // 60 seconds before end
            num = calc_next_seg_no_from_timelines(pls, start_time_offset);
            if (num == -1)
                num = pls->first_seq_no;
            else
                num += pls->first_seq_no;
            if (is_full_detection) {
                repeat = calc_lines_repeat(c->is_live, pls);
                if(c->maxSegmentDuration > 5)
                    num = num + repeat;
                else
                    num = num + repeat/2;
            }
        } else if (pls->fragment_duration){
            if (pls->presentation_timeoffset) {/*get net time.*/
                num = pls->first_seq_no + (((get_current_time_in_sec() - c->availability_start_time) * pls->fragment_timescale)-pls->presentation_timeoffset) / pls->fragment_duration - c->min_buffer_time;
            } else if (c->publish_time > 0 && !c->availability_start_time) {
                if (c->min_buffer_time) {
                    num = pls->first_seq_no + (((c->publish_time + pls->fragment_duration) - c->suggested_presentation_delay) * pls->fragment_timescale) / pls->fragment_duration - c->min_buffer_time;
                } else {
                    num = pls->first_seq_no + (((c->publish_time - c->time_shift_buffer_depth + pls->fragment_duration) - c->suggested_presentation_delay) * pls->fragment_timescale) / pls->fragment_duration;
                }
            } else {
                num = pls->first_seq_no + (((get_current_time_in_sec() - c->availability_start_time) - c->suggested_presentation_delay) * pls->fragment_timescale) / pls->fragment_duration;
            }
        }
    } else {
        num = pls->first_seq_no;
    }
    return num;
}

static int64_t calc_min_seg_no(AVFormatContext *s, struct representation *pls)
{
    DASHContext *c = s->priv_data;
    int64_t num = 0;
    if (!c)
        return num;

    if (c->is_live && pls->fragment_duration) {
        num = pls->first_seq_no + (((get_current_time_in_sec() - c->availability_start_time) - c->time_shift_buffer_depth) * pls->fragment_timescale) / pls->fragment_duration;
    } else {
        num = pls->first_seq_no;
    }
    return num;
}

static int64_t calc_max_seg_no(struct representation *pls, DASHContext *c)
{
    int64_t num = 0;

    if (pls->n_fragments) {
        num = pls->first_seq_no + pls->n_fragments - 1;
    } else if (pls->n_timelines) {
        int i = 0;
        num = pls->first_seq_no + pls->n_timelines - 1;
        for (i = 0; i < pls->n_timelines; i++) {
            if (pls->timelines[i]->repeat == -1) {
                int length_of_each_segment = pls->timelines[i]->duration / pls->fragment_timescale;
                num =  c->periods[c->current_period]->period_duration / length_of_each_segment;
            } else {
                num += pls->timelines[i]->repeat;
            }
        }
    } else if (c->is_live && pls->fragment_duration) {
        num = pls->first_seq_no + (((get_current_time_in_sec() - c->availability_start_time)) * pls->fragment_timescale)  / pls->fragment_duration;
    } else if (pls->fragment_duration) {
        if (c->periods[c->current_period]->period_duration > 0) {
            num = pls->first_seq_no + av_rescale_rnd(1, c->periods[c->current_period]->period_duration * pls->fragment_timescale, pls->fragment_duration, AV_ROUND_UP);
        } else {
            num = pls->first_seq_no + av_rescale_rnd(1, c->media_presentation_duration * pls->fragment_timescale, pls->fragment_duration, AV_ROUND_UP);
        }
    }

    return num;
}

static void move_timelines(struct representation *rep_src, struct representation *rep_dest, DASHContext *c)
{
    int is_full_detection = 0;
    Period *cur_periods = NULL;
    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

    if (!c || c->current_period < 0 || c->n_periods <= 0 || c->current_period >= c->n_periods) {
        return ;
    }
    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return ;

    if (rep_dest && rep_src ) {
        free_timelines_list(rep_dest);
        rep_dest->timelines    = rep_src->timelines;
        rep_dest->n_timelines  = rep_src->n_timelines;
        rep_dest->first_seq_no = rep_src->first_seq_no;
        rep_dest->last_seq_no = calc_max_seg_no(rep_dest, c);
        rep_src->timelines = NULL;
        rep_src->n_timelines = 0;
        rep_dest->cur_seq_no = rep_src->cur_seq_no;
        if(is_full_detection && c->is_live){
            int64_t cur_seq_no = 0, num = 0;
            gxlogd ("current_max_seg=%lld cur_seq_no = %lld\n",cur_periods->current_max_seg,rep_dest->cur_seq_no);
            num = calc_lines_repeat(c->is_live, rep_dest);
            if (c->maxSegmentDuration > 5 &&  cur_periods->current_max_seg >= (rep_src->start_number + num -1)){
                cur_seq_no = rep_src->start_number + num;
            } else {
                cur_seq_no = rep_src->start_number + num/2;
                if (c->maxSegmentDuration > 5 && cur_seq_no <= cur_periods->current_max_seg)
                    cur_seq_no = rep_src->start_number + num;
                if (c->maxSegmentDuration < 5 && cur_seq_no <= cur_periods->current_max_seg)
                    cur_seq_no = rep_src->start_number + num;
            }
            rep_dest->cur_seq_no = cur_seq_no;
            gxlogd ("start_number=%lld cur_seq_no=%lld cur_seq_no=%lld\n", rep_src->start_number,cur_seq_no,rep_dest->cur_seq_no);
        }
    }
}

static void move_segments(struct representation *rep_src, struct representation *rep_dest, DASHContext *c)
{
    if (rep_dest && rep_src ) {
        free_fragment_list(rep_dest);
        if (rep_src->start_number > (rep_dest->start_number + rep_dest->n_fragments))
            rep_dest->cur_seq_no = 0;
        else
            rep_dest->cur_seq_no += rep_src->start_number - rep_dest->start_number;
        rep_dest->fragments    = rep_src->fragments;
        rep_dest->n_fragments  = rep_src->n_fragments;
        rep_dest->parent  = rep_src->parent;
        rep_dest->last_seq_no = calc_max_seg_no(rep_dest, c);
        rep_src->fragments = NULL;
        rep_src->n_fragments = 0;
    }
}

static int refresh_manifest(AVFormatContext *s)
{
    int ret = 0, i = 0, retry_cnt = 0, current_period = 0;
    DASHContext *c = s->priv_data;
    Period *cur_periods = NULL, *prev_periods = NULL;

    if (!c || (c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods)) {
        return AVERROR_INVALIDDATA;
    }
    // save current context
    int n_videos = c->periods[c->current_period]->n_videos;
    struct representation **videos = c->periods[c->current_period]->videos;
    int n_audios = c->periods[c->current_period]->n_audios;
    struct representation **audios = c->periods[c->current_period]->audios;
    int n_subtitles = c->periods[c->current_period]->n_subtitles;
    struct representation **subtitles = c->periods[c->current_period]->subtitles;
    char *base_url = c->base_url;
    int n_periods = c->n_periods;
    current_period = c->current_period;
    c->base_url = NULL;
    for (i = 0; i < c->n_periods; i++) {
        Period *tmp_periods = c->periods[i];
        if (i == current_period || !tmp_periods)
            continue;
        if (tmp_periods->subtitles)
            free_subtitle_list(tmp_periods);
        if (tmp_periods->audios)
            free_audio_list(tmp_periods);
        if (tmp_periods->videos)
            free_video_list(tmp_periods);
        if (tmp_periods->n_contentprotection)
            free_contentprotection_list(c, i);
    }
    c->periods[current_period]->n_videos = 0;
    c->periods[current_period]->videos = NULL;
    c->periods[current_period]->n_audios = 0;
    c->periods[current_period]->audios = NULL;
    c->periods[current_period]->n_subtitles = 0;
    c->periods[current_period]->subtitles = NULL;
    c->max_url_size = 0;
    c->n_periods = 0;
    c->current_period = 0;
    ret = parse_manifest(s, s->url, NULL, 1);
    if (ret < 0) {
        c->current_period = current_period;
        c->n_periods = n_periods;
        goto finish;
    }
    if (c->n_periods != n_periods) {
        gxlogi_raw_l("new manifest has mismatched no. of periods, %d -> %d\n", n_periods, c->n_periods);
        ret = AVERROR_INVALIDDATA;
        goto finish;
    }
    c->current_period = current_period;
    c->n_periods = n_periods;
    cur_periods = c->periods[c->current_period];
    if (cur_periods->n_videos != n_videos) {
        gxlogi_raw_l("new manifest has mismatched no. of video representations, %d -> %d\n", n_videos, cur_periods->n_videos);
        ret = AVERROR_INVALIDDATA;
        goto finish;
    }
    if (cur_periods->n_audios != n_audios) {
        gxlogi_raw_l("new manifest has mismatched no. of audio representations, %d -> %d\n", n_audios, cur_periods->n_audios);
        ret = AVERROR_INVALIDDATA;
        goto finish;
    }
    if (cur_periods->n_subtitles != n_subtitles) {
        gxlogi_raw_l("new manifest has mismatched no. of subtitles representations, %d -> %d\n", n_subtitles, cur_periods->n_subtitles);
        ret = AVERROR_INVALIDDATA;
        goto finish;
    }

    for (i = 0; i < n_videos; i++) {
        struct representation *cur_video = videos[i];
        struct representation *ccur_video = cur_periods->videos[i];
        if (cur_video->timelines) {
            // calc current time
            int64_t currentTime = get_segment_start_time_based_on_timeline(cur_video, cur_video->cur_seq_no) / cur_video->fragment_timescale;
            // update segments
            ccur_video->cur_seq_no = calc_next_seg_no_from_timelines(ccur_video, currentTime * cur_video->fragment_timescale - 1);
            if (ccur_video->cur_seq_no >= 0) {
                move_timelines(ccur_video, cur_video, c);
            }
        }
        if (cur_video->fragments) {
            move_segments(ccur_video, cur_video, c);
        }
    }

    for (i = 0; i < n_audios; i++) {
        struct representation *cur_audio = audios[i];
        struct representation *ccur_audio = cur_periods->audios[i];
        if (cur_audio->timelines) {
            // calc current time
            int64_t currentTime = get_segment_start_time_based_on_timeline(cur_audio, cur_audio->cur_seq_no) / cur_audio->fragment_timescale;
            // update segments
            ccur_audio->cur_seq_no = calc_next_seg_no_from_timelines(ccur_audio, currentTime * cur_audio->fragment_timescale - 1);
            if (ccur_audio->cur_seq_no >= 0) {
                move_timelines(ccur_audio, cur_audio, c);
            }
        }
        if (cur_audio->fragments) {
            move_segments(ccur_audio, cur_audio, c);
        }
    }

    for (i = 0; i < n_subtitles; i++) {
        struct representation *cur_subtiless = subtitles[i];
        struct representation *ccur_subtiless = cur_periods->subtitles[i];
        if (cur_subtiless->timelines) {
            // calc current time
            int64_t currentTime = get_segment_start_time_based_on_timeline(cur_subtiless, cur_subtiless->cur_seq_no) / cur_subtiless->fragment_timescale;
            // update segments
            ccur_subtiless->cur_seq_no = calc_next_seg_no_from_timelines(ccur_subtiless, currentTime * cur_subtiless->fragment_timescale - 1);
            if (ccur_subtiless->cur_seq_no >= 0) {
                move_timelines(ccur_subtiless, cur_subtiless, c);
            }
        }
        if (cur_subtiless->fragments) {
            move_segments(ccur_subtiless, cur_subtiless, c);
        }
    }

finish:
    // restore context
    if (c->base_url) {
        if (base_url) {
            av_free(base_url);
            base_url = NULL;
        }
    } else {
        c->base_url  = base_url;
    }
    if ((c->current_period) < 0 || (current_period < 0) || (c->n_periods <= 0)) {
        return AVERROR_INVALIDDATA;
    }
    for (i = 0; i < c->n_periods; i++) {
        Period *tmp_periods = c->periods[i];
        if (i != current_period || !tmp_periods)
            continue;
        if (tmp_periods->subtitles)
            free_subtitle_list(tmp_periods);
        if (tmp_periods->audios)
            free_audio_list(tmp_periods);
        if (tmp_periods->videos)
            free_video_list(tmp_periods);
        if (tmp_periods->n_contentprotection)
            free_contentprotection_list(c, i);
    }
    c->periods[c->current_period]->n_subtitles = n_subtitles;
    c->periods[c->current_period]->subtitles = subtitles;
    c->periods[c->current_period]->n_audios = n_audios;
    c->periods[c->current_period]->audios = audios;
    c->periods[c->current_period]->n_videos = n_videos;
    c->periods[c->current_period]->videos = videos;
    return ret;
}

static struct fragment *get_current_fragment(struct representation *pls, int *flags)
{
    int ret = 0, is_full_detection = 0;
    int64_t min_seq_no = 0, max_seq_no = 0, restart_cun = 0;
    struct fragment *seg = NULL;
    struct fragment *seg_ptr = NULL;
    DASHContext *c = pls->parent->priv_data;

    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
    if (!c)
        return NULL;
    while ((pls->n_fragments > 0)) {
        if (url_check_interrupt_cb()) {
            *flags = AVERROR_EXIT;
            return NULL;
        }
        if (pls->cur_seq_no < pls->n_fragments) {
            seg_ptr = pls->fragments[pls->cur_seq_no];
            seg = av_mallocz(sizeof(struct fragment));
            if (!seg) {
                return NULL;
            }
            seg->url = av_strdup(seg_ptr->url);
            if (!seg->url) {
                av_free(seg);
                *flags = AVERROR(ENOMEM);
                return NULL;
            }
            seg->size = seg_ptr->size;
            seg->url_offset = seg_ptr->url_offset;
            return seg;
        } else if (c->is_live) {
            if ((!c->is_false_live) || (c->is_false_live && pls->is_refresh_manifest)) {
                ret = refresh_manifest(pls->parent);
                if (ret < 0) {
                    *flags = ret;
                    return NULL;
                }
            }
        } else {
            break;
        }
    }

    if (c->is_live) {
        if (pls->parent) {
            min_seq_no = calc_min_seg_no(pls->parent, pls);
        }
        int64_t tmp_cur_seq_no = -1;
        max_seq_no = calc_max_seg_no(pls, c);
        unsigned int cur_time = av_get_tick(NULL, 0);
        uint64_t update_manifest_time = (c->time_shift_buffer_depth>c->minimum_update_period)?((c->time_shift_buffer_depth - c->minimum_update_period)*1000):0;
        if (pls->timelines || pls->fragments) {
            if ((c->is_false_live && pls->is_refresh_manifest)||((cur_time - c->last_time_update_period)>=update_manifest_time)) {
                tmp_cur_seq_no = pls->cur_seq_no;
                c->last_time_update_period = av_get_tick(NULL, 0);
                ret = refresh_manifest(pls->parent);
                if (ret < 0) {
                    *flags = ret;
                    return NULL;
                }
                pls->is_refresh_manifest = 0;
            }
        }
        if (pls->cur_seq_no <= min_seq_no) {
            pls->cur_seq_no = calc_cur_seg_no(pls->parent, pls);
            if ((pls->timelines || pls->fragments) && (tmp_cur_seq_no > 0) && (pls->cur_seq_no < tmp_cur_seq_no))
                pls->cur_seq_no = tmp_cur_seq_no;
        } else {
            gxlogd ("new fragment: cur:%lld min:%lld max:%lld.\n", pls->cur_seq_no, min_seq_no, max_seq_no);
        }
        seg = av_mallocz(sizeof(struct fragment));
        if (!seg) {
            *flags = AVERROR(ENOMEM);
            return NULL;
        }
    } else if (pls->cur_seq_no <= pls->last_seq_no) {
        seg = av_mallocz(sizeof(struct fragment));
        if (!seg) {
            *flags = AVERROR(ENOMEM);
            return NULL;
        }
    }

    if (seg) {
        if (!pls->url_template) {
            gxloge ("Cannot get fragment, missing template URL\n");
            av_free(seg);
            return NULL;
        }
        int def_max_url_size = 0;
        GxPlayer_SystemGet(PSYS_MAX_URL_SIZE, &def_max_url_size);
        c->max_url_size = (c->max_url_size <= 0)?def_max_url_size:c->max_url_size;
        char *tmpfilename= av_mallocz(c->max_url_size);
        if (!tmpfilename) {
            av_free(seg);
            *flags = AVERROR(ENOMEM);
            return NULL;
        }
        int64_t start_time = get_segment_start_time_based_on_timeline(pls, pls->cur_seq_no);
        pls->previous_seq_no = pls->cur_seq_no;
        ff_dash_fill_tmpl_params(tmpfilename, c->max_url_size, pls->url_template, 0, (int)pls->cur_seq_no, 0, start_time);
        seg->url = av_strireplace(pls->url_template, pls->url_template, tmpfilename);
        if (!seg->url) {
            seg->url = av_strdup(pls->url_template);
            if (!seg->url) {
                *flags = AVERROR(ENOMEM);
                if (tmpfilename)
                    av_free(tmpfilename);
                if (seg)
                    av_free(seg);
                return NULL;
            }
        }
        if (tmpfilename) {
            av_free(tmpfilename);
            tmpfilename = NULL;
        }
        seg->size = -1;
    }

    return seg;
}

static int read_from_url(struct representation *pls, struct fragment *seg, uint8_t *buf, int buf_size)
{
    int ret;

    /* limit read if the fragment was only a part of a file */
    if (seg->size >= 0)
        buf_size = FFMIN(buf_size, pls->cur_seg_size - pls->cur_seg_offset);

    ret = avio_read(pls->input, (unsigned char*)buf, buf_size);
    if (ret > 0) {
        pls->cur_seg_offset += ret;
        pls->internal_http_read_size += ret;
        pls->http_req_last_time = av_gettime();
    }
    return ret;
}

static int open_input(DASHContext *c, struct representation *pls, struct fragment *seg)
{
    AVDictionary *opts = NULL;
    char *url = NULL;
    int ret = 0;
    int is_http = 0;

    if (c->http_persistent)
        av_dict_set_int(&opts, "multiple_requests", 1, 0);

    url = av_mallocz(c->max_url_size);
    if (!url) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    if (seg->size >= 0) {
        /* try to restrict the HTTP request to the part we want
         * (if this is in fact a HTTP request) */
        av_dict_set_int(&opts, "offset", seg->url_offset, 0);
        av_dict_set_int(&opts, "end_offset", seg->url_offset + seg->size, 0);
    }
    av_dict_set_int(&opts, "is_live", (c->is_live)?1:0, 0);

    ff_make_absolute_url(url, c->max_url_size, c->base_url, seg->url);
    gxlogi_raw_l ("DASH request stream_index %d for url '%s', offset %lld cur_seq_no %lld last_seq_no %lld is_in_advance %d http_persistent %d\n",
            pls->stream_index,url, seg->url_offset,pls->cur_seq_no,pls->last_seq_no, c->periods[c->current_period]->is_in_advance, c->http_persistent);
    av_dict_set_int(&opts, "rw_timeout", 9, 0);//set timeout:9S.
    pls->http_req_start_time = av_gettime();
    pls->http_req_end_time = 0;
    pls->http_req_last_time = 0;
    pls->internal_http_read_size = 0;
    ret = open_url(pls->parent, &pls->input, url, c->avio_opts, opts, NULL);
    if (ret < 0) {
        goto cleanup;
    }

cleanup:
    av_free(url);
    av_dict_free(&opts);
    pls->cur_seg_offset = 0;
    pls->cur_seg_size = seg->size;
    return ret;
}

static int update_init_section(struct representation *pls)
{
    static const int max_init_section_size = 512 * 1024;
    DASHContext *c = pls->parent->priv_data;
    size_t sec_size;
    int64_t urlsize;
    int ret = 0, restart_cun = 0, is_full_detection = 0;

    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
restart:
    if (!pls->init_section || pls->init_sec_buf)
        return 0;

    ret = open_input(c, pls, pls->init_section);
    if (ret < 0) {
        gxlogd ("Failed to open an initialization section in playlist %d\n", pls->rep_idx);
        if (is_full_detection && (restart_cun < 3)) {
            restart_cun++;
            goto restart;
        }
        return ret;
    }

    if (pls->init_section->size >= 0)
        sec_size = pls->init_section->size;
    else if ((urlsize = avio_size(pls->input)) >= 0)
        sec_size = urlsize;
    else
        sec_size = max_init_section_size;

    sec_size = FFMIN(sec_size, max_init_section_size);

    av_fast_malloc(&pls->init_sec_buf, &pls->init_sec_buf_size, sec_size, 0);

    ret = read_from_url(pls, pls->init_section, pls->init_sec_buf, pls->init_sec_buf_size);
    ff_format_io_close(pls->parent, &pls->input);
    if (ret < 0)
        return ret;

    pls->init_sec_data_len = ret;
    pls->init_sec_buf_read_offset = 0;

    return 0;
}

static int64_t seek_data(void *opaque, int64_t offset, int whence)
{
    struct representation *v = opaque;
    if (v->n_fragments && !v->init_sec_data_len) {
        return avio_seek(v->input, offset, whence);
    }

    return AVERROR(ENOSYS);
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
        case AVERROR_EXIT:
            ret = AVERROR_EXIT;
            break;
        case AVERROR_EOF:
            ret = AVERROR_EOF;
            break;
        default:
            ret = err;
            break;
    }
    return ret;
}

static int read_data(void *opaque, uint8_t *buf, int buf_size)
{
#define DASH_MAX_RELOAD 20
    int ret = 0, flags = 0, reload_count = 0, response_reload_count = 0, is_use_flase_live = 0, is_full_detection = 0, res_fail_retry = 0;
    struct representation *v = opaque;
    if (!v->parent)
        return ret;
    DASHContext *c = v->parent->priv_data;
    Period *cur_periods = NULL;

    if (!c || c->current_period < 0 || c->n_periods <= 0 || c->current_period >= c->n_periods) {
        return AVERROR(EINVAL);
    }
    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return AVERROR(EINVAL);

    v->is_refresh_manifest = 0;
    GxPlayer_SystemGet(PSYS_NETWORK_IS_FALSE_LIVE, &is_use_flase_live);
    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

restart:
    reload_count++;
    if (reload_count > DASH_MAX_RELOAD) {
        gxlogi_raw_l ("Invalid request.reload count=%d.MAX_RELOAD=%d.\n", reload_count, DASH_MAX_RELOAD);
        return AVERROR_EOF;
    }
    if (!v->input || (c->http_persistent && v->input_read_done)) {
        free_fragment(&v->cur_seg);
        v->cur_seg = get_current_fragment(v, &flags);
        if (c->is_live && is_use_flase_live && (c->previous_availability_start_time != c->availability_start_time)) {
            return AVERROR_RSTRT;
        }
        if (!v->cur_seg) {
            ret = http_response_status(flags);
            if(c->is_live && is_use_flase_live && ((AVERROR_HTTP_OTHER_4XX == ret) || (AVERROR_HTTP_SERVER_ERROR == ret))) {/*update mpd fail.send restart info.*/
                return AVERROR_RSTRT;
            }
            ret = (ret >= 0)?AVERROR_EOF:ret; //ret >= 0,flags is 0.
            goto end;
        }

        /* load/update Media Initialization Section, if any */
        ret = update_init_section(v);
        if (ret) {
            goto end;
        }
        ret = open_input(c, v, v->cur_seg);
        if (ret < 0) {
            if (url_check_interrupt_cb()) {
                ret = AVERROR_EXIT;
                goto end;
            }
            if(c->is_live && is_use_flase_live && (response_reload_count > 3)) {
                return AVERROR_RSTRT;
            }
            if (!c->is_live)
                v->cur_seq_no++;
            if ((ret = http_response_status(ret))) {
                if (AVERROR_HTTP_OTHER_4XX == ret || AVERROR_HTTP_SERVER_ERROR == ret || (is_full_detection && (AVERROR(EIO) == ret))) {
                    if(is_full_detection && c->is_live && c->maxSegmentDuration > 5 && res_fail_retry < 10){
                        int delay = (c->maxSegmentDuration <= 0)?10:(c->maxSegmentDuration*1000/10);
                        res_fail_retry++;
                        GxCore_ThreadDelay(delay);
                        gxlogd ("res_fail_retry =%d\n",res_fail_retry);
                        goto restart;
                    }
                    res_fail_retry = 0;
                    v->is_refresh_manifest = 1;
                    cur_periods->is_in_advance = 1;
                    cur_periods->is_in_advance_type = v->type;
                    if (is_full_detection && c->is_live) {
                        cur_periods->is_open_fail_flag = 1;
                    }
                    gxlogi ("Invalid request.server response ret:%d(%s)\n", ret, av_err2str(ret));
                }
            }
            response_reload_count += 1;
            res_fail_retry = 0;
            goto restart;
        }
        response_reload_count = 0;
        v->input_read_done = 0;
        res_fail_retry = 0;
    }

    if (v->init_sec_buf_read_offset < v->init_sec_data_len) {
        /* Push init section out first before first actual fragment */
        int copy_size = FFMIN(v->init_sec_data_len - v->init_sec_buf_read_offset, buf_size);
        memcpy(buf, v->init_sec_buf, copy_size);
        v->init_sec_buf_read_offset += copy_size;
        ret = copy_size;
        goto end;
    }

    /* check the v->cur_seg, if it is null, get current and double check if the new v->cur_seg*/
    if (!v->cur_seg) {
        v->cur_seg = get_current_fragment(v, &flags);
    }
    if (!v->cur_seg) {
        ret = http_response_status(flags);
        ret = (ret >= 0)?AVERROR_EOF:ret; //ret >= 0,flags is 0.
        goto end;
    }
    ret = read_from_url(v, v->cur_seg, buf, buf_size);
    if (ret > 0) {
        goto end;
    } else if (AVERROR(EIO) == ret) {
        if (is_use_flase_live && c->is_live) {
            v->http_req_end_time = av_gettime();
            return AVERROR_RSTRT;
        }
    }

    if (is_full_detection) {
        if(cur_periods->is_open_fail_flag
                && c->maxSegmentDuration > 5
                && ((cur_periods->is_in_advance_type  != CODEC_TYPE_UNKNOWN)
                    && (cur_periods->is_in_advance_type != v->type))){
            cur_periods->is_in_advance = 0;
            v->input_read_done = 1;
            v->cur_seg_offset = 0;
            cur_periods->is_open_fail_flag = 0;
            cur_periods->is_in_advance_type = CODEC_TYPE_UNKNOWN;
            goto restart;
        }
    }

    if (c->is_live || v->cur_seq_no < v->last_seq_no) {
        v->http_req_end_time = av_gettime();
        v->read_segment_num += 1;
        if (!v->is_restart_needed) {
            v->cur_seq_no++;
            cur_periods->is_in_advance = 0;
        }
        if (c->http_persistent
                && (v->read_segment_num < get_max_http_persistent_num())
                && (!((cur_periods->is_switch_track || (is_full_detection && v->is_switch_track)) && (cur_periods->switch_media_type == v->type)))) {
            v->input_read_done = 1;
            v->cur_seg_offset = 0;
        } else {
            v->is_restart_needed = 1;
        }
    }

    if ((!is_full_detection) && c->http_persistent && (v->cur_seq_no >= v->last_seq_no)) {
        if (c->n_periods > 1 && (c->current_period < (c->n_periods-1))) {
            return AVERROR_NEXT;
        } else {
            return AVERROR_EOF;
        }
    }
    if (c->http_persistent
            && (v->read_segment_num < get_max_http_persistent_num())
            && (!((cur_periods->is_switch_track || (is_full_detection && v->is_switch_track)) && (cur_periods->switch_media_type == v->type)))) {
        goto restart;
    }
end:
    return ret;
}

static void close_demux_for_component(struct representation *pls)
{
    /* note: the internal buffer could have changed */
    if (pls->pb.buffer) {
        av_free(pls->pb.buffer);
        pls->pb.buffer = NULL;
    }
    memset(&pls->pb, 0x00, sizeof(AVIOContext));
    avformat_close_input(&pls->ctx);
    pls->ctx = NULL;
}

static int reopen_demux_for_component(AVFormatContext *s, struct representation *pls, enum AVMediaType media_type)
{
    DASHContext *c = s->priv_data;
    AVInputFormat *in_fmt = NULL;
    unsigned char *avio_ctx_buffer  = NULL;
    int ret = 0, i, flags = 0, buffer_size = 0;
    int iIndex = 0;
    AVDictionaryEntry *e = NULL;
    AVDictionary *opts = NULL;

    if (c->avio_opts)
        av_dict_copy(&opts, c->avio_opts, 0);

    if (pls && pls->ctx) {
        close_demux_for_component(pls);
    }

    if (url_check_interrupt_cb()) {
        ret = AVERROR_EXIT;
        goto fail;
    }

    pls->ctx = avformat_alloc_context();
    if (!(pls->ctx)) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    buffer_size = (media_type == AVMEDIA_TYPE_VIDEO)?INITIAL_VIDEO_BUFFER_SIZE:INITIAL_OTHER_BUFFER_SIZE;
    avio_ctx_buffer  = av_malloc(buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        avformat_free_context(pls->ctx);
        pls->ctx = NULL;
        goto fail;
    }

    if (opts && (e = av_dict_get(opts, "avioflags", NULL, 0))) {
        flags = atoi(e->value);
    }

    pls->type = media_type;
    ffio_init_context(&pls->pb, avio_ctx_buffer, buffer_size, flags & AVIO_FLAG_WRITE, pls, read_data, NULL, (c->is_live)?NULL:seek_data);
    GxPlayer_SystemGet(PSYS_INDEX_CACHE, &pls->ctx->index_limit);
    pls->pb.seekable = 0;
    pls->ctx->flags = AVFMT_FLAG_CUSTOM_IO;
    pls->ctx->probesize = 1024 * 8;
    pls->ctx->max_analyze_duration = 4 * AV_TIME_BASE;

    if ((e = av_dict_get(opts, "ffprobesize", NULL, 0))) {
        pls->ctx->probesize = ((atoi(e->value))> 8*1024)?(atoi(e->value)):(8*1024);
    }
    if ((e = av_dict_get(opts, "analyzeduration", NULL, 0))) {
        pls->ctx->max_analyze_duration = ((atoi(e->value)*AV_TIME_BASE)> 4*AV_TIME_BASE)?(atoi(e->value)*AV_TIME_BASE):(4*AV_TIME_BASE);
    }

    ret = av_probe_input_buffer(&pls->pb, &in_fmt, "", NULL, 0, 0);
    if (ret < 0) {
        if (pls->ctx) {
            avformat_free_context(pls->ctx);
            pls->ctx = NULL;
        }
        goto fail;
    }
    pls->ctx->pb = &pls->pb;
    pls->ctx->file_format = s->file_format;
    // provide additional information from mpd if available
    ret = avformat_open_input(&pls->ctx, "", in_fmt, &opts); //pls->init_section->url
    if (ret < 0) {
        gxloge_raw_l ("dashdec.c open input fail.ret=%d(%s).\n", ret, av_err2str(ret));
        goto fail;
    }

    if (pls->n_fragments) {
        if (pls->framerate.den) {
            for (i = 0; i < pls->ctx->nb_streams; i++)
                pls->ctx->streams[i]->r_frame_rate = pls->framerate;
        }
        if ((ret = avformat_find_stream_info(pls->ctx, &opts)) < 0) {
            gxloge_raw_l ("dashdec.c find stream info fail. ret=%d(%s).\n", ret, av_err2str(ret));
            goto fail;
        }
    }
    ret = 0;
fail:
    av_dict_free(&opts);
    return ret;
}

#define AV_INPUT_BUFFER_PADDING_SIZE 64
static int avcodec_parameters_copy(AVStream *st, AVStream *ist)
{
    st->codec->codec_type = ist->codec->codec_type;
    st->codec->codec_id = ist->codec->codec_id;
    st->codec->bit_rate = ist->codec->bit_rate;
    st->codec->sample_rate = ist->codec->sample_rate;
    st->codec->channels = ist->codec->channels;
    st->codec->bits_per_coded_sample = ist->codec->bits_per_coded_sample;
    if (ist->codec->extradata) {
        if (!st->codec->extradata) {
            if (st->codec->extradata)
                av_freep(&st->codec->extradata);
            if (!(st->codec->extradata = av_mallocz(ist->codec->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE))) {
                gxloge ("malloc fail.\n");
                return AVERROR(ENOMEM);
            }
        }
        memcpy(st->codec->extradata, ist->codec->extradata, ist->codec->extradata_size);
        st->codec->extradata_size = ist->codec->extradata_size;
        st->codec->is_reset_extradata = 1;
    }
    return 0;
}

static int open_demux_for_component(AVFormatContext *s, struct representation *pls, int front_strems_idx, enum AVMediaType media_type)
{
    int ret = -1, i = 0;
    AVStream *st = NULL;
    AVStream *ist = NULL;

    pls->parent = s;
    pls->cur_seq_no  = calc_cur_seg_no(s, pls);
    if (!pls->last_seq_no) {
        pls->last_seq_no = calc_max_seg_no(pls, s->priv_data);
    }
    ret = reopen_demux_for_component(s, pls, media_type);
    if (ret < 0) {
        return ret;
    }
    for (i = 0; i < pls->ctx->nb_streams; i++) {
        ist = pls->ctx->streams[i];
        if (front_strems_idx < 0) {
            st = avformat_new_stream(s, NULL);
            if (!st) {
                gxloge ("malloc fail!!, ret=%d\n", ret);
                return AVERROR(ENOMEM);
            }
            st->id = i;
        } else {
            st = s->streams[front_strems_idx];
            if (!st) {
                i -= 1;
                front_strems_idx = -1;
                continue;
            }
        }
        avcodec_parameters_copy(st, ist);
        avpriv_set_pts_info(st, ist->pts_wrap_bits, ist->time_base.num, ist->time_base.den);
        // copy disposition
        st->disposition = ist->disposition;
    }
    return 0;
}

static int is_common_init_section_exist(struct representation **pls, int n_pls)
{
    struct fragment *first_init_section = pls[0]->init_section;
    char *url =NULL;
    int64_t url_offset = -1;
    int64_t size = -1;
    int i = 0;

    if (first_init_section == NULL || n_pls == 0)
        return 0;

    url = first_init_section->url;
    url_offset = first_init_section->url_offset;
    size = pls[0]->init_section->size;
    for (i=0;i<n_pls;i++) {
        if (!pls[i]->init_section)
            continue;
        if (av_strcasecmp(pls[i]->init_section->url,url) ||
                pls[i]->init_section->url_offset != url_offset ||
                pls[i]->init_section->size != size) {
            return 0;
        }
    }
    return 1;
}

static int copy_init_section(struct representation *rep_dest, struct representation *rep_src)
{
    rep_dest->init_sec_buf = av_mallocz(rep_src->init_sec_buf_size);
    if (!rep_dest->init_sec_buf) {
        gxloge ("malloc fail!!, %d\n", rep_src->init_sec_buf_size);
        return AVERROR(ENOMEM);
    }
    memcpy(rep_dest->init_sec_buf, rep_src->init_sec_buf, rep_src->init_sec_data_len);
    rep_dest->init_sec_buf_size = rep_src->init_sec_buf_size;
    rep_dest->init_sec_data_len = rep_src->init_sec_data_len;
    rep_dest->cur_timestamp = rep_src->cur_timestamp;

    return 0;
}

static void send_dash_info_to_demux_lavf(AVFormatContext *s, DASHContext *c)
{
    int i = 0, idx = 0;
    struct representation *rep;
    Period *cur_periods = NULL;
    if (!c || (c->current_period < 0) || (c->current_period >= c->n_periods)) {
        return;
    }

    av_dict_set_int(&(s->url_options), "dash_n_periods", c->n_periods, 0); // periods.
    av_dict_set_int(&(s->url_options), "dash_cur_periods", c->current_period, 0); // periods.
    for (idx = 0; idx < c->n_periods; idx++) {
        cur_periods = c->periods[idx];
        if (!cur_periods)
            return;
        av_dict_set_int(&(s->url_options), "cur_periods_idx", i, AV_DICT_MULTIKEY); // video streams number.
        if (cur_periods->n_videos > 1 || cur_periods->n_audios > 1 || cur_periods->n_subtitles > 1) {
            if (cur_periods->n_videos > 0) {
                av_dict_set_int(&(s->url_options), "vid_variant_bitrate_max", cur_periods->n_videos, AV_DICT_MULTIKEY); // video streams number.
                av_dict_set_int(&(s->url_options), "vid_stream_index", cur_periods->cur_vtrack_idx, AV_DICT_MULTIKEY); // current video stream index.
                for (i = 0; i < cur_periods->n_videos; i++) {
                    rep = cur_periods->videos[i];
                    av_dict_set(&(s->url_options), "vid_id", rep->id, AV_DICT_MULTIKEY); //dash video id..
                    if (rep->bandwidth > 0)
                        av_dict_set_int(&(s->url_options), "vid_variant_bitrate", rep->bandwidth, AV_DICT_MULTIKEY); //dash video bandwitdh.
                    av_dict_set(&(s->url_options), "vid_codecs", rep->codecs, AV_DICT_MULTIKEY); // video codecs.
                    av_dict_set(&(s->url_options), "vid_width", rep->width, AV_DICT_MULTIKEY); // video width.
                    av_dict_set(&(s->url_options), "vid_height", rep->height, AV_DICT_MULTIKEY); // video height.
                    av_dict_set(&(s->url_options), "vid_sar", rep->sar, AV_DICT_MULTIKEY); // video sar.
                }
            }
            if (cur_periods->n_audios > 0) {
                av_dict_set_int(&(s->url_options), "audio_prog_max", cur_periods->n_audios, AV_DICT_MULTIKEY); // audio streams number.
                av_dict_set_int(&(s->url_options), "aud_stream_index", cur_periods->cur_atrack_idx, AV_DICT_MULTIKEY); // current audio stream index
                for (i = 0; i < cur_periods->n_audios; i++) {
                    rep = cur_periods->audios[i];
                    av_dict_set(&(s->url_options), "aud_id", rep->id, AV_DICT_MULTIKEY); //dash audio id.
                    if (rep->bandwidth > 0)
                        av_dict_set_int(&(s->url_options), "a_variant_bitrate", rep->bandwidth, AV_DICT_MULTIKEY); // audio bandwidth.
                    av_dict_set(&(s->url_options), "aud_language", rep->lang, AV_DICT_MULTIKEY); //dash audio language.
                    av_dict_set(&(s->url_options), "aud_samplerate", rep->samplerate, AV_DICT_MULTIKEY); //audio codecs.
                    av_dict_set(&(s->url_options), "aud_codecs", rep->codecs, AV_DICT_MULTIKEY); //audio codecs.
                    if (rep->discard != AVDISCARD_ALL)
                        av_dict_set_int(&(s->url_options), "aud_stream_index", i, AV_DICT_MULTIKEY); // current audio stream index
                }
            }
            if (cur_periods->n_subtitles > 0) {
                av_dict_set_int(&(s->url_options), "sub_prog_max", cur_periods->n_subtitles, AV_DICT_MULTIKEY); // subtiles streams number.
                av_dict_set_int(&(s->url_options), "sub_stream_index", cur_periods->cur_strack_idx, AV_DICT_MULTIKEY); // current subtiles stream index.
                for (i = 0; i < cur_periods->n_subtitles; i++) {
                    rep = cur_periods->subtitles[i];
                    av_dict_set(&(s->url_options), "sub_id", rep->id, AV_DICT_MULTIKEY);//dash subtile id..
                    av_dict_set(&(s->url_options), "sub_language", rep->lang, AV_DICT_MULTIKEY);//dash subtiles language.
                    av_dict_set(&(s->url_options), "sub_codecs", rep->codecs, AV_DICT_MULTIKEY); //current subtiles codecs.
                }
            }
        }
    }
}

static int dash_segment_is_reconnect(int err)
{
    int ret = 0;
    switch (err) {
        case AVERROR_HTTP_BAD_REQUEST:
        case AVERROR_HTTP_UNAUTHORIZED:
        case AVERROR_HTTP_FORBIDDEN:
        case AVERROR_HTTP_NOT_FOUND:
        case AVERROR_HTTP_OTHER_4XX:
        case AVERROR_HTTP_SERVER_ERROR:
            ret = 1;
            break;
        default:
            ret = 0;
            break;
    }
    return ret;
}

static int parse_schemeIduri_get_uuid(char *contentprotection_schemeIduri)
{
    int i = 0;
    char *param, *str, *old_str, *next_param;
    /* Reference: https://dashif.org/identifiers/content_protection/ */
    static const Drm_Identifier common_identifier[] = {
        {"ABV DRM (MoDRM)", "6dd8b3c3-45f4-4a68-bf3a-64168d01a4a6"},
        {"Adobe Primetime DRM version 4", "f239e769-efa3-4850-9c16-a903c6932efb"},
        {"Alticast", "616c7469-6361-7374-2d50-726f74656374"},
        {"Apple FairPlay", "94ce86fb-07ff-4f43-adb8-93d2fa968ca2" },
        {"Arris Titanium", "279fe473-512c-48fe-ade8-d176fee6b40f"},
        {"ChinaDRM", "3d5e6d35-9b9a-41e8-b843-dd3c6e72c42c" },
        {"Clear Key AES-128", "3ea8778f-7742-4bf9-b18b-e834b2acbd47"},
        {"Clear Key SAMPLE-AES", "be58615b-19c4-4684-88b3-c8c57e99e957"},
        {"Clear Key DASH-IF", "e2719d58-a985-b3c9-781a-b030af78d30e"},
        {"CMLA (OMA DRM)", "644fe7b5-260f-4fad-949a-0762ffb054B4"},
        {"Commscope Titanium V3", "37c33258-7b99-4c7e-b15d-19af74482154"},
        {"CoreCrypt", "45d481cb-8fe0-49c0-ada9-ab2d2455b2f2"},
        {"DigiCAP SmartXess", "dcf4e3e3-62f1-5818-7ba6-0a6fe33ff3dd"},
        {"DivX DRM Series 5", "35bf197b-530e-42d7-8b65-1b4bf415070f"},
        {"Irdeto Content Protection", "80a6be7e-1448-4c37-9e70-d5aebe04c8d2"},
        {"Marlin Adaptive Streaming Simple Profile V1.0", "5e629af5-38da-4063-8977-97ffbd9902d4"},
        {"Microsoft PlayReady", "9a04f079-9840-4286-ab92-e65be0885f95"},
        {"MobiTV DRM", "6a99532d-869f-5922-9a91-113ab7b1e2f3"},
        {"Nagra MediaAccess PRM 3.0", "adb41c24-2dbf-4a6d-958b-4457c0d27b95"},
        {"SecureMedia", "1f83e1e8-6ee9-4f0d-ba2f-5ec4e3ed1a66"},
        {"SecureMedia SteelKnot", "992c46e6-c437-4899-b6a0-50fa91ad0e39"},
        {"Synamedia/Cisco/NDS VideoGuard DRM", "a68129d3-575b-4f1a-9cba-3223846cf7c3"},
        {"Unitend DRM (UDRM)", "aa11967f-cc01-4a4a-8e99-c5d3dddfea2d"},
        {"Verimatrix VCAS", "9a27dd82-fde2-4725-8cbc-4234aa06ec09"},
        {"Viaccess-Orca DRM (VODRM)", "b4413586-c58c-ffb0-94a5-d4896c1af6c3"},
        {"VisionCrypt", "793b7956-9f94-4946-a942-23e7ef7e44b4"},
        {"W3C Common PSSH box", "1077efec-c0b2-4d02-ace3-3c1e52e2fb4b"},
        {"Widevine Content Protection", "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"},
    };

    if (contentprotection_schemeIduri) {
        str = old_str = av_strdup(contentprotection_schemeIduri);
        if (!str)
            return AVERROR(ENOMEM);
        while ((param = av_strtok(str, ":", &next_param))) {
            if (!av_strcasecmp(param, "uuid")) {
                for (i = 0; i < sizeof(common_identifier)/sizeof(common_identifier[0]); i++) {
                    if (!av_strcasecmp(next_param, common_identifier[i].identifier)) {
                        gxlogi_raw_l ("DRM content protection vender name:[%s] UUID:[%s]\n", common_identifier[i].vender_name, common_identifier[i].identifier);
                        av_freep(&old_str);
                        return 1;
                    }
                }
            }
            str = NULL;
        }
        av_freep(&old_str);
    }
    return 0;
}

static int dash_mpd_contentprotection_info(AVFormatContext *s)
{
    int i = 0, ret = AVERROR(EINVAL);
    DASHContext *c = s->priv_data;
    Period *cur_periods = NULL;

    if (!c || (c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods)) {
        return ret;
    }
    for (i = 0; i < c->n_periods; i++) {
        cur_periods = c->periods[i];
        if (!cur_periods)
            return ret;
        if (public_stream_callback) {
            GxStreamOptionCallbackParam param;
            for (i = 0; i < cur_periods->n_contentprotection; i++) {
                struct ContentProtection *tmp;
                tmp = cur_periods->contentprotection[i];
                if (!tmp)
                    continue;
#if 0
                if (tmp->contentprotection_default_kid) {
                    param.default_KID_Size = strlen(tmp->contentprotection_default_kid);
                    param.default_KID = (unsigned char *)tmp->contentprotection_default_kid;
                }
                if (tmp->contentprotection_schemeIduri) {
                    param.schemeIdUri_Size = strlen(tmp->contentprotection_schemeIduri);
                    param.schemeIdUri = (unsigned char *)tmp->contentprotection_schemeIduri;
                }
                if (tmp->contentprotection_pssh) {
                    param.pssh_Size = strlen(tmp->contentprotection_pssh);
                    param.pssh = (unsigned char *)tmp->contentprotection_pssh;
                }
                break;
#else
                if (!(ret = parse_schemeIduri_get_uuid(tmp->contentprotection_schemeIduri))) {
                    gxlogi_raw_l ("The manifest mpd for this url is ContentProtection!! idx:%d(%d) value:(%s) default_kid:(%s) schemeIduri:(%s) pssh:(%s)\n",
                            i, cur_periods->n_contentprotection, tmp->contentprotection_value,
                            tmp->contentprotection_default_kid, tmp->contentprotection_schemeIduri, tmp->contentprotection_pssh);
                }
#endif
            }
#if 0
            param.work_mode  = 1;
            if (public_stream_callback(&param) != 0) { // input encry key.
                gxloge("callback error \n");
                return ret;
            }
#endif
        }else if (cur_periods->n_contentprotection > 0) {
            for (i = 0; i < cur_periods->n_contentprotection; i++) {
                struct ContentProtection *tmp;
                tmp = cur_periods->contentprotection[i];
                if (!tmp)
                    continue;
                if (!(ret = parse_schemeIduri_get_uuid(tmp->contentprotection_schemeIduri))) {
                    gxlogi_raw_l ("The manifest mpd for this url is ContentProtection!! idx:%d(%d) value:(%s) default_kid:(%s) schemeIduri:(%s) pssh:(%s)\n",
                            i, cur_periods->n_contentprotection, tmp->contentprotection_value,
                            tmp->contentprotection_default_kid, tmp->contentprotection_schemeIduri, tmp->contentprotection_pssh);
                }
            }
        }
    }
    return ret;
}

static int check_support_customer_specified_resolution(DASHContext *c)
{
    int i, restup_w = 0, restup_h = 0, select_bandwidth = -1, cur_bandwidth_idx = -1;
    struct representation *rep;
    Period *cur_periods = NULL;

    GxPlayer_SystemGet(PSYS_NETWORK_RESOLUTION_WIDTH, &restup_w);
    GxPlayer_SystemGet(PSYS_NETWORK_RESOLUTION_HEIGHT, &restup_h);

    if (!c || (c->current_period < 0) || (c->n_periods <= 0) || (restup_w <= 0) || (restup_h <= 0 )||(restup_w>1920) || (restup_h>1080)) {
        return select_bandwidth;
    }

    cur_periods = c->periods[c->current_period];
    if ((cur_periods->n_videos <= 1) || (cur_periods->cur_vtrack_idx >= 0)) {
        return select_bandwidth;
    }

    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (rep && (rep->width && (atoi(rep->width) == restup_w))
			&& (rep->height && (atoi(rep->height) == restup_h))) {
            select_bandwidth = rep->bandwidth;
            break;
        }
    }

    if (i >= cur_periods->n_videos)
        return select_bandwidth;

    /*find select bandwidth idex.*/
    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (rep && rep->bandwidth > 0 && rep->bandwidth == select_bandwidth) {
            cur_bandwidth_idx = i;
            break;
        }
    }
    return cur_bandwidth_idx;
}


int get_dash_low_resolution_bandwidth_value(DASHContext *c)
{
    int i  =0, first_min_bandwidth = INT_MAX, second_min_bandwidth = INT_MAX, third_min_bandwidth = INT_MAX, select_bandwidth = 0, set_stream_clarity = 0;
    struct representation *rep;
    Period *cur_periods = NULL;

    if (!c || c->current_period < 0 || c->n_periods <= 0 || c->current_period >= c->n_periods) {
        return AVERROR_NOTSUPP;
    }

    cur_periods = c->periods[c->current_period];
    if ((cur_periods->n_videos <= 1) || (cur_periods->cur_vtrack_idx >= 0)) {
        return AVERROR_NOTSUPP;
    }

    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (!rep)
            continue;
        if (auto_filter_decoder_unsupporrt_type(rep)) {
            continue;
        }
        if (rep->bandwidth < first_min_bandwidth) {
            third_min_bandwidth = second_min_bandwidth;
            second_min_bandwidth = first_min_bandwidth;
            first_min_bandwidth = rep->bandwidth;
        } else if (rep->bandwidth < second_min_bandwidth && rep->bandwidth != first_min_bandwidth) {
            third_min_bandwidth = second_min_bandwidth;
            second_min_bandwidth = rep->bandwidth;
        } else if (rep->bandwidth < third_min_bandwidth && rep->bandwidth != second_min_bandwidth && rep->bandwidth != first_min_bandwidth) {
            third_min_bandwidth = rep->bandwidth;
        }
    }

    GxPlayer_SystemGet(PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX, &set_stream_clarity);
    if (set_stream_clarity == -1) {/*first mini bandwidth.*/
        select_bandwidth = (first_min_bandwidth != INT_MAX)?first_min_bandwidth:0;
    } else if (set_stream_clarity == -2) {/*second mini bandwidth.*/
        select_bandwidth = (second_min_bandwidth != INT_MAX)?second_min_bandwidth:((first_min_bandwidth != INT_MAX)?first_min_bandwidth:0);
    } else {
        if (cur_periods->n_videos <= 3)/*When you only have three bandwidth, you take the second_min_bandwidth.*/
            select_bandwidth = (second_min_bandwidth != INT_MAX)?second_min_bandwidth:((first_min_bandwidth != INT_MAX)?first_min_bandwidth:0);
        else
            select_bandwidth = (third_min_bandwidth != INT_MAX)?third_min_bandwidth:((second_min_bandwidth != INT_MAX)?second_min_bandwidth:((first_min_bandwidth != INT_MAX)?first_min_bandwidth:0));
    }
    return select_bandwidth;
}

int get_dash_hight_resolution_bandwidth_value(DASHContext *c)
{
    int i  =0, max_bandwidth = -1, second_max_bandwidth = -1, third_max_bandwidth = -1, select_bandwidth = 0, set_stream_clarity = 0;
    struct representation *rep;
    Period *cur_periods = NULL;

    if (!c || c->current_period < 0 || c->n_periods <= 0 || c->current_period >= c->n_periods) {
        return AVERROR_NOTSUPP;
    }

    cur_periods = c->periods[c->current_period];
    if ((cur_periods->n_videos <= 1) || (cur_periods->cur_vtrack_idx >= 0)) {
        return AVERROR_NOTSUPP;
    }

    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (!rep)
            continue;
        if (auto_filter_decoder_unsupporrt_type(rep)) {
            continue;
        }
        if (rep->bandwidth > max_bandwidth) {
            third_max_bandwidth = second_max_bandwidth;
            second_max_bandwidth = max_bandwidth;
            max_bandwidth = rep->bandwidth;
        } else if (rep->bandwidth > second_max_bandwidth && rep->bandwidth != max_bandwidth) {
            third_max_bandwidth = second_max_bandwidth;
            second_max_bandwidth = rep->bandwidth;
        } else if (rep->bandwidth > third_max_bandwidth && rep->bandwidth != second_max_bandwidth && rep->bandwidth != max_bandwidth) {
            third_max_bandwidth = rep->bandwidth;
        }
    }

    GxPlayer_SystemGet(PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX, &set_stream_clarity);
    if (set_stream_clarity == 1) {/*first max bandwidth.*/
        select_bandwidth = (max_bandwidth != -1)?max_bandwidth:0;
    } else if (set_stream_clarity == 2) {/*second max bandwidth.*/
        select_bandwidth = (second_max_bandwidth != -1)?second_max_bandwidth:((max_bandwidth != -1)?max_bandwidth:0);
    } else {
        if (cur_periods->n_videos <= 3)/*When you only have three bandwidth, you take the second_max_bandwidth.*/
            select_bandwidth = (second_max_bandwidth != -1)?second_max_bandwidth:((max_bandwidth != -1)?max_bandwidth:0);
        else
            select_bandwidth = (third_max_bandwidth != -1)?third_max_bandwidth:((second_max_bandwidth != -1)?second_max_bandwidth:((max_bandwidth != -1)?max_bandwidth:0));
    }
    return select_bandwidth;
}

int get_set_player_multi_bandwidth_index(DASHContext *c)
{
    int i  =0, max_bandwidth = -1, second_max_bandwidth = -1, third_max_bandwidth = -1, select_bandwidth = 0, index = 0, set_stream_clarity = 0;
    struct representation *rep;
    Period *cur_periods = NULL;

    if (!c || c->current_period < 0 || c->n_periods <= 0 || c->current_period >= c->n_periods) {
        return AVERROR_NOTSUPP;
    }

    cur_periods = c->periods[c->current_period];
    if ((cur_periods->n_videos <= 1) || (cur_periods->cur_vtrack_idx >= 0)) {
        return AVERROR_NOTSUPP;
    }

    GxPlayer_SystemGet(PSYS_NETWORK_DEFAULT_BANDWIDTH_INDEX, &set_stream_clarity);
    if (set_stream_clarity > 0) {
        select_bandwidth = get_dash_hight_resolution_bandwidth_value(c);
    } else if (set_stream_clarity < 0) {
        select_bandwidth = get_dash_low_resolution_bandwidth_value(c);
    }

    /*find select bandwidth idex.*/
    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (rep && rep->bandwidth > 0 && rep->bandwidth == select_bandwidth) {
            index = i;
            break;
        }
    }
    return index;
}

static int is_http_redirection(AVFormatContext *s)
{
    DASHContext *c = s->priv_data;
    if (!c) {
        return AVERROR_NOTSUPP;
    }

    if (s->url_options) {
        AVDictionaryEntry *e = NULL;
        av_dict_copy(&c->avio_opts, s->url_options, 0);
        if ((e = av_dict_get(s->url_options, "location", NULL, 0))) {
            if (s->url) {
                av_free(s->url);
                s->url = NULL;
            }
            if (!(s->url = av_strdup(e->value))) {
                return AVERROR(ENOMEM);
            }
        }
    }
    return 0;
}

static void dash_context_param_init(AVFormatContext *s)
{
    DASHContext *c = s->priv_data;
    Period *cur_periods = NULL;
    int use_http_persistent_connections = 0, is_use_flase_live = 0;

    GxPlayer_SystemGet(PSYS_NETWORK_HTTP_PERSISTENT_CONNECTIONS, &use_http_persistent_connections);
    c->http_persistent = use_http_persistent_connections?1:0;
    GxPlayer_SystemGet(PSYS_NETWORK_IS_FALSE_LIVE, &is_use_flase_live);
    c->is_false_live = is_use_flase_live?1:0;
    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return ;
    cur_periods->is_switch_track = 0;
    cur_periods->switch_idx = -1;
    cur_periods->is_in_advance_type = CODEC_TYPE_UNKNOWN;
    cur_periods->switch_media_type = CODEC_TYPE_UNKNOWN;
    c->previous_availability_start_time = (c->is_live)?c->availability_start_time:0;
    cur_periods->is_in_reopen = 0;

    /* If this isn't a live stream, fill the total duration of the stream. */
    if (!c->is_live) {
        s->duration = (int64_t) c->media_presentation_duration * AV_TIME_BASE;
    } else {
        av_dict_set(&c->avio_opts, "seekable", "0", 0);
    }
}

static int open_streams(AVFormatContext *s)
{
    DASHContext *c = s->priv_data;
    struct representation *rep;
    AVProgram *program;
    Period *cur_periods = NULL;
    int ret = 0, stream_index = 0, i = 0, j = 0, retry = 4, is_full_detection = 0;

    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return AVERROR(EINVAL);
    if(cur_periods->n_videos)
        cur_periods->is_init_section_common_video = is_common_init_section_exist(cur_periods->videos, cur_periods->n_videos);
    /* Open the demuxer for video and audio components if available */
    if (!is_full_detection && cur_periods->cur_vtrack_idx <= 0) {
        cur_periods->cur_vtrack_idx = check_support_customer_specified_resolution(c);
        if (cur_periods->cur_vtrack_idx < 0)
            cur_periods->cur_vtrack_idx = get_set_player_multi_bandwidth_index(c);
    }
    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (!is_full_detection && (cur_periods->cur_vtrack_idx >= 0) && (i != cur_periods->cur_vtrack_idx)) {
            continue;
        }
        if (i > 0 && cur_periods->is_init_section_common_video) {
            ret = copy_init_section(rep, cur_periods->videos[0]);
            if (ret < 0)
                return ret;
        }
        ret = open_demux_for_component(s, rep, -1, AVMEDIA_TYPE_VIDEO);
        if (ret)
            return ret;
        rep->stream_index = stream_index;
        ++stream_index;
        rep->needed = 1;
        if (!is_full_detection) {
            cur_periods->cur_vtrack_idx = i;
        }
        rep->read_segment_num = 0;
        if (!is_full_detection)
            break;
    }

    if(cur_periods->n_audios)
        cur_periods->is_init_section_common_audio = is_common_init_section_exist(cur_periods->audios, cur_periods->n_audios);
    for (i = 0; i < cur_periods->n_audios; i++) {
        rep = cur_periods->audios[i];
        rep->is_not_request_flag = 0;
        if (i > 0 && cur_periods->is_init_section_common_audio) {
            ret = copy_init_section(rep, cur_periods->audios[0]);
            if (ret < 0)
                return ret;
        }
        ret = open_demux_for_component(s, rep, -1, AVMEDIA_TYPE_AUDIO);
        if (ret) {
            if (dash_segment_is_reconnect(ret)) {
                if (--retry <= 0) {
                    rep->is_not_request_flag = 1;/*retry 4, http request fail(404 or 4**). is not audio data.*/
                } else {
                    i -= 1;
                }
                continue;
            }
            return ret;
        }
        rep->stream_index = stream_index;
        ++stream_index;
        rep->needed = 1;
        if (!is_full_detection) {
            cur_periods->cur_atrack_idx = i;
        }
        rep->read_segment_num = 0;
        if (!is_full_detection)
            break;
    }

    if (cur_periods->n_subtitles)
        cur_periods->is_init_section_common_subtitle = is_common_init_section_exist(cur_periods->subtitles, cur_periods->n_subtitles);
    for (i = 0; i < cur_periods->n_subtitles; i++) {
        rep = cur_periods->subtitles[i];
        if (i > 0 && cur_periods->is_init_section_common_subtitle) {
            ret = copy_init_section(rep, cur_periods->subtitles[0]);
            if (ret < 0)
                return ret;
        }
        ret = open_demux_for_component(s, rep, -1, AVMEDIA_TYPE_SUBTITLE);
        if (ret)
            return ret;
        rep->stream_index = stream_index;
        ++stream_index;
        rep->needed = 1;
        if (!is_full_detection) {
            cur_periods->cur_strack_idx = i;
        }
        rep->read_segment_num = 0;
        if (!is_full_detection)
            break;
    }
    if (!stream_index) {
        dash_close(s);
        return AVERROR(EINVAL);
    }

    if (is_full_detection) {
        /* Create a program */
        program = av_new_program(s, 0);
        if (!program) {
            dash_close(s);
            return AVERROR(ENOMEM);
        }
        for (i = 0; i < cur_periods->n_videos; i++) {
            rep = cur_periods->videos[i];
            av_program_add_stream_index(s, 0, rep->stream_index);
            rep->assoc_stream = s->streams[rep->stream_index];
        }
        for (i = 0; i < cur_periods->n_audios; i++) {
            rep = cur_periods->audios[i];
            av_program_add_stream_index(s, 0, rep->stream_index);
            rep->assoc_stream = s->streams[rep->stream_index];
        }
        for (i = 0; i < cur_periods->n_subtitles; i++) {
            rep = cur_periods->subtitles[i];
            av_program_add_stream_index(s, 0, rep->stream_index);
            rep->assoc_stream = s->streams[rep->stream_index];
        }
    }

    return ret;
}

static int dash_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    DASHContext *c = s->priv_data;
    Period *cur_periods = NULL;
    int ret = AVERROR(EINVAL), is_on_abr = 0, i = 0;

    GxPlayer_SystemGet(PSYS_NETWORK_HTTP_AUTO_ADAPABILITY, &is_on_abr);

    if (!c) {
        return ret;
    }

    if ((ret = is_http_redirection(s)) < 0) {
        return ret;
    }

    c->last_time_update_period = av_get_tick(NULL, 0);
    if ((ret = parse_manifest(s, s->url, s->pb, 0)) < 0) {
        return ret;
    }

    if ((c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods))
        return AVERROR(EINVAL);

    for (i = 0; i <c->n_periods; i++) {
        cur_periods = c->periods[c->current_period];
        if (!cur_periods)
            return AVERROR(EINVAL);
        cur_periods->cur_vtrack_idx = -1;
        if (s->url_options) {
            AVDictionaryEntry *e = NULL;
            av_dict_copy(&c->avio_opts, s->url_options, 0);
            if ((e = av_dict_get(s->url_options, "mulit_bandwidth_prog_now", NULL, 0))) {
                cur_periods->cur_vtrack_idx = atoi(e->value);
            }
        }
    }
    if (is_on_abr) {
        cur_periods = c->periods[c->current_period];
        if (cur_periods->n_videos > 1) {
            if ((ret = init_dash_abr(&(cur_periods->video_abr))) < 0)
                return ret;
            ret = GxCore_ThreadCreate("dash abr", &c->abr_thread, dash_abr_thread, s, 32*1024, GXOS_DEFAULT_PRIORITY+1);
            if (ret != 0) {
                gxloge ("GxCore_ThreadCreate failed : %s\n", strerror(ret));
                return AVERROR(ret);
            }
        }
    }

    dash_mpd_contentprotection_info(s);
    dash_context_param_init(s);
    if ((ret = open_streams(s)) < 0) {
        return ret;
    }
    send_dash_info_to_demux_lavf(s, c);
    return 0;
}

static void reset_avcodec_parameters(AVFormatContext *s, struct representation *pls, int type)
{
    int k = 0;
    AVStream *st = NULL;
    AVStream *ist = NULL;
    if (!pls || !pls->ctx) {
        return ;
    }
    for (k = 0; k < pls->ctx->nb_streams; k++) {
        ist = pls->ctx->streams[k];
        st = s->streams[k];
        if (!st || !ist)
            continue;
        if (type == st->codec->codec_type) {
            break;
        }
    }
    if (k < pls->ctx->nb_streams) {
        avcodec_parameters_copy(st, ist);
        avpriv_set_pts_info(st, ist->pts_wrap_bits, ist->time_base.num, ist->time_base.den);
    }
}

static int recheck_discard_flags(AVFormatContext *s, struct representation **p, int n, enum AVMediaType media_type)
{
    int i, j, ret = 0, targ = 0, is_full_detection = 0;
    DASHContext *c = s->priv_data;

    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

    for (i = 0; i < n; i++) {
        int needed = 0, is_reopen = 0, is_close = 0;
        struct representation *pls = p[i];
        if (AVMEDIA_TYPE_AUDIO == media_type && pls->is_not_request_flag) {
            continue;
        }

        if (is_full_detection) {
            needed = (!pls->assoc_stream) || (pls->assoc_stream->discard < AVDISCARD_ALL);
            is_reopen = needed && pls && !pls->ctx;
            is_close = !needed && pls && pls->ctx;
        } else {
            is_reopen = pls && pls->needed && !pls->ctx && pls->discard < AVDISCARD_ALL;
            is_close = pls && !pls->needed && pls->ctx;
        }

        if (is_reopen) {
            pls->cur_seg_offset = 0;
            pls->init_sec_buf_read_offset = 0;
            /* Catch up */
            for (j = 0; j < n; j++) {
                pls->cur_seq_no = FFMAX(pls->cur_seq_no, p[j]->cur_seq_no);
            }
            if ((ret = reopen_demux_for_component(s, pls, media_type)) < 0) {
                if (pls && pls->pb.buffer) {
                    av_freep(&pls->pb.buffer);
                }
                return ret;
            }
            reset_avcodec_parameters(s, pls, media_type);
            if (c)
                c->periods[c->current_period]->is_in_reopen = 0;
        } else if (is_close) {
            close_demux_for_component(pls);
            pls->input_read_done = 0;
            if (pls->input) {
                ff_format_io_close(pls->parent, &pls->input);
            }
        }
        if(is_full_detection && c->is_live && (targ == 0)){
            for (j = 0; j < n; j++) {
                c->periods[c->current_period]->current_max_seg = FFMAX(pls->cur_seq_no, p[j]->cur_seq_no);
            }
            targ = 1;
        }
    }
    return ret;
}

static int close_cur_dash_period(AVFormatContext *s, int cur_period)
{
    int is_on_abr = 0, i = 0;
    DASHContext *c = s->priv_data;
    Period *cur_periods = NULL;

    GxPlayer_SystemGet(PSYS_NETWORK_HTTP_AUTO_ADAPABILITY, &is_on_abr);

    if (c->current_period < 0 || c->n_periods <= 0 || c->current_period >= c->n_periods)
        return AVERROR(EINVAL);

    cur_periods = c->periods[c->current_period];
    if (is_on_abr) {
        if (cur_periods->n_videos > 1) {
            close_dash_abr(cur_periods->video_abr);
        }
    }

    if (c->n_periods > 0) {
        if (c->keepalive_manifest) {
            avio_close(c->keepalive_manifest);
            c->keepalive_manifest = NULL;
        }
    }

    for (i = 0; i < cur_periods->n_videos; i++) {
        struct representation **p = cur_periods->videos;
        struct representation *pls = p[i];
        if (pls && !pls->needed && pls->ctx) {
            close_demux_for_component(pls);
            pls->input_read_done = 0;
            if (pls->input) {
                ff_format_io_close(pls->parent, &pls->input);
            }
        }
    }

    for (i = 0; i < cur_periods->n_audios; i++) {
        struct representation **p = cur_periods->audios;
        struct representation *pls = p[i];
        if (pls && !pls->needed && pls->ctx) {
            close_demux_for_component(pls);
            pls->input_read_done = 0;
            if (pls->input) {
                ff_format_io_close(pls->parent, &pls->input);
            }
        }
    }

    for (i = 0; i < cur_periods->n_subtitles; i++) {
        struct representation **p = cur_periods->subtitles;
        struct representation *pls = p[i];
        if (pls && !pls->needed && pls->ctx) {
            close_demux_for_component(pls);
            pls->input_read_done = 0;
            if (pls->input) {
                ff_format_io_close(pls->parent, &pls->input);
            }
        }
    }

    return 0;
}

static int dash_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    DASHContext *c = s->priv_data;
    int ret = 0, i, is_replace = 0, is_use_flase_live = 0, is_full_detection = 0, is_next_periods = 0, is_on_abr = 0;
    int64_t mints = 0;
    struct representation *cur = NULL;
    struct representation *rep = NULL;
    Period *cur_periods = NULL;
    enum AVMediaType cur_media_type = CODEC_TYPE_UNKNOWN;

    GxPlayer_SystemGet(PSYS_NETWORK_IS_FALSE_LIVE, &is_use_flase_live);
    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

restart:
    ret = i = mints = 0;
    cur = rep = NULL;

    cur_periods = c->periods[c->current_period];
    if ((ret = recheck_discard_flags(s, cur_periods->videos, cur_periods->n_videos, AVMEDIA_TYPE_VIDEO)) < 0) {
        return ret;
    }
    if ((ret = recheck_discard_flags(s, cur_periods->audios, cur_periods->n_audios, AVMEDIA_TYPE_AUDIO)) < 0) {
        return ret;
    }
    if ((ret = recheck_discard_flags(s, cur_periods->subtitles, cur_periods->n_subtitles, AVMEDIA_TYPE_SUBTITLE)) < 0) {
        return ret;
    }

    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (!rep->ctx)
            continue;
        if (!cur || rep->cur_timestamp < mints) {
            cur = rep;
            mints = rep->cur_timestamp;
            cur_media_type = AVMEDIA_TYPE_VIDEO;
        }
    }

    for (i = 0; i < cur_periods->n_audios; i++) {
        rep = cur_periods->audios[i];
        if (rep->is_not_request_flag || !rep->ctx)
            continue;
        if (cur_periods->is_in_advance) {
            if (rep->cur_timestamp > mints && (cur_periods->is_in_advance_type == CODEC_TYPE_AUDIO)) {//auido 4xx 5xx.update mpd.
                is_replace = 0;
                mints = 0;
            } else if (rep->cur_timestamp > mints && (cur_periods->is_in_advance_type == CODEC_TYPE_VIDEO)) {
                is_replace = 1;
            } else if (rep->cur_timestamp < mints && (cur_periods->is_in_advance_type == CODEC_TYPE_AUDIO)) {
                is_replace = 1;
            } else if (rep->cur_timestamp < mints && (cur_periods->is_in_advance_type == CODEC_TYPE_VIDEO)) {
                is_replace = 0;
                mints = 0;
            }
        } else {
            is_replace = 0;
            cur_periods->is_in_advance = 0;
        }
        if (!cur || (rep->cur_timestamp < mints) || (is_replace && cur_periods->is_in_advance)) {
            cur = rep;
            mints = rep->cur_timestamp;
            cur_media_type = AVMEDIA_TYPE_AUDIO;
        }
    }

    for (i = 0; i < cur_periods->n_subtitles; i++) {
        rep = cur_periods->subtitles[i];
        if (!rep->ctx)
            continue;
        if (!cur || rep->cur_timestamp < mints) {
            cur = rep;
            mints = rep->cur_timestamp;
            cur_media_type = AVMEDIA_TYPE_SUBTITLE;
        }
    }

    if (!cur) {
        return AVERROR(EINVAL);
    }

    // multi-period support
    if ((c->n_periods > 1) && is_next_periods && (cur->cur_seq_no >= cur->last_seq_no)) {
        is_next_periods = 0;
        if (c->current_period < c->n_periods-1) {
            close_cur_dash_period(s, c->current_period);
            c->current_period++;
            GxPlayer_SystemGet(PSYS_NETWORK_HTTP_AUTO_ADAPABILITY, &is_on_abr);
            if (is_on_abr) {
                cur_periods = c->periods[c->current_period];
                if (cur_periods->n_videos > 1) {
                    if ((ret = init_dash_abr(&(cur_periods->video_abr))) < 0) {
                        return ret;
                    }
                }
            }
            dash_context_param_init(s);
            if ((ret = open_streams(s)) < 0) {
                return ret;
            }
            goto restart;
        }
    }

    ret = 0;
    while (!ret) {
        ret = av_read_frame(cur->ctx, pkt);
        if (ret >= 0) {
            /* If we got a packet, return it */
            cur->cur_timestamp = av_rescale(pkt->pts, (int64_t)cur->ctx->streams[0]->time_base.num * 90000, cur->ctx->streams[0]->time_base.den);
            pkt->stream_index = cur->stream_index;
            return 0;
        }

        if (url_check_interrupt_cb()) {
            return AVERROR_EXIT;
        }
        if (is_use_flase_live && c->is_live && ret == AVERROR_RSTRT) {
            return AVERROR_RSTRT;
        } else if (ret == AVERROR_NEXT) {
            is_next_periods = 1;
            goto restart;
        }
        if (is_full_detection) {
            if (cur->is_restart_needed) {
                cur->cur_seg_offset = 0;
                cur->init_sec_buf_read_offset = 0;
                cur->is_restart_needed = 0;
                if (cur->is_switch_track == 0) {
                    ret = reopen_demux_for_component(s, cur, cur_media_type);
                    if (ret < 0) {
                        if (cur && cur->pb.buffer){
                            av_freep(&cur->pb.buffer);
                        }
                    }
                    cur->read_segment_num = 0;
                } else {
                    if (cur_periods->switch_media_type == cur->type) {
                        s->is_switch_av_track = 1;
                        s->streams[c->av_control.discard_idx]->discard = AVDISCARD_ALL;
                        s->streams[c->av_control.discard_idx]->discard = AVDISCARD_DEFAULT;
                        cur->is_switch_track = 0;
                        cur->is_restart_needed = 0;
                        cur_periods->switch_media_type = CODEC_TYPE_UNKNOWN;
                        goto restart;
                    }
                }
            }
        } else {
            if (cur_periods->is_switch_track && cur_periods->switch_media_type == cur->type) {
                cur_periods->is_switch_track = 0;
                cur_periods->is_in_reopen = 1;
                ret = switch_dash_mulitprogram_track(s, cur_periods->switch_idx, cur_periods->switch_media_type);
                cur_periods->switch_media_type = CODEC_TYPE_UNKNOWN;
                if (cur->is_restart_needed)
                    cur->is_restart_needed = 0;
                goto restart;
            }

            if (cur->is_restart_needed) {
                cur->cur_seg_offset = 0;
                cur->init_sec_buf_read_offset = 0;
                cur->is_restart_needed = 0;
                if (cur->input) {
                    ff_format_io_close(cur->parent, &cur->input);
                }
                ret = reopen_demux_for_component(s, cur, cur_media_type);
                if (ret < 0) {
                    if (cur && cur->pb.buffer) {
                        av_freep(&cur->pb.buffer);
                    }
                }
                cur->read_segment_num = 0;
            }
        }
    }
    return AVERROR_EOF;
}

static int dash_close(AVFormatContext *s)
{
    int is_on_abr = 0, i = 0;
    DASHContext *c = s->priv_data;
    Period *cur_periods = NULL;

    GxPlayer_SystemGet(PSYS_NETWORK_HTTP_AUTO_ADAPABILITY, &is_on_abr);
    if (!c) {
        return 0;
    }

    for (i = 0; i < c->n_periods; i++) {
        cur_periods = c->periods[i];
        if (!cur_periods)
            return 0;

        if (is_on_abr) {
            if (cur_periods->n_videos > 1) {
                GxCore_ThreadJoin(c->abr_thread);
                close_dash_abr(cur_periods->video_abr);
            }
        }

        if (c->keepalive_manifest) {
            avio_close(c->keepalive_manifest);
            c->keepalive_manifest = NULL;
        }
        if (cur_periods->n_contentprotection >= 0 && cur_periods->contentprotection) {
            free_contentprotection_list(c, i);
        }
        //free_period_list(c);
        free_video_list(cur_periods);
        free_audio_list(cur_periods);
        free_subtitle_list(cur_periods);
        if (cur_periods) {
            av_free(cur_periods);
            cur_periods = NULL;
        }

    }
    if (c->periods)
        av_freep(&c->periods);
    if (c->avio_opts)
        av_dict_free(&c->avio_opts);
    if (c->base_url)
        av_freep(&c->base_url);
    return 0;
}

static int dash_seek(AVFormatContext *s, struct representation *pls, int64_t seek_pos_msec, int flags, int dry_run, enum AVMediaType media_type)
{
    int ret = 0, i = 0, j = 0;
    int64_t duration = 0;

    // single fragment mode
    if (pls->n_fragments == 1) {
        pls->cur_timestamp = 0;
        pls->cur_seg_offset = 0;
        if (dry_run)
            return 0;
        return av_seek_frame(pls->ctx, -1, seek_pos_msec * 1000, flags);
    }

    if (pls->input) {
        ff_format_io_close(pls->parent, &pls->input);
    }
    pls->input_read_done = 0;

    // find the nearest fragment
    if (pls->n_timelines > 0 && pls->fragment_timescale > 0) {
        int64_t num = pls->first_seq_no;
        for (i = 0; i < pls->n_timelines; i++) {
            if (pls->timelines[i]->starttime > 0) {
                duration = pls->timelines[i]->starttime;
            }
            duration += pls->timelines[i]->duration;
            if (seek_pos_msec < ((duration * 1000) /  pls->fragment_timescale)) {
                goto set_seq_num;
            }
            for (j = 0; j < pls->timelines[i]->repeat; j++) {
                duration += pls->timelines[i]->duration;
                num++;
                if (seek_pos_msec < ((duration * 1000) /  pls->fragment_timescale)) {
                    goto set_seq_num;
                }
            }
            num++;
        }

set_seq_num:
        pls->cur_seq_no = num > pls->last_seq_no ? pls->last_seq_no : num;
    } else if (pls->fragment_duration > 0) {
        pls->cur_seq_no = pls->first_seq_no + ((seek_pos_msec * pls->fragment_timescale) / pls->fragment_duration) / 1000;
    } else {
        pls->cur_seq_no = pls->first_seq_no;
    }
    pls->cur_timestamp = 0;
    pls->cur_seg_offset = 0;
    pls->init_sec_buf_read_offset = 0;
    ret = dry_run ? 0 : reopen_demux_for_component(s, pls, media_type);

    return ret;
}

static int dash_read_seek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)
{
    int ret = 0, i = 0, target_period = 0;
    struct representation *rep = NULL;
    DASHContext *c = s->priv_data;
    Period *cur_periods = NULL;

    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return ret;

    int64_t seek_pos_msec = av_rescale_rnd(timestamp, 1000,
            s->streams[stream_index]->time_base.den,
            flags & AVSEEK_FLAG_BACKWARD ?
            AV_ROUND_DOWN : AV_ROUND_UP);
    if ((flags & AVSEEK_FLAG_BYTE) || c->is_live) {
        return AVERROR(ENOSYS);
    }
    /* Find which period seek_pos exists for multi-period*/
    if (c->n_periods > 1) {
        target_period = -1;
        for (i = 0; i < c->n_periods; ++i) {
            if (seek_pos_msec <= c->periods[i]->period_duration * 1000) {
                target_period = i;
                break;
            } else {
                seek_pos_msec -= c->periods[i]->period_duration * 1000;
            }
        }
        if (target_period < 0) {
            gxloge ("dash seek failed to find correct period\n");
            return AVERROR(ENOSYS);
        }
        if (target_period != c->current_period) {
            gxloge ("switch period from %d to %d\n", c->current_period, target_period);
            close_cur_dash_period(s, c->current_period);
            gxloge ("period abc acb abc abc abc abc..%d(%d)..\n", c->current_period, c->n_periods);
            c->current_period = target_period;
            dash_context_param_init(s);
            if ((ret = open_streams(s)) < 0) {
                gxloge ("dashdec.c read packet open streams fail fail fail..ret=%d.\n", ret);
                return ret;
            }
        }
        cur_periods = c->periods[c->current_period];
        if (!cur_periods)
            return ret;
    }

    /* Seek in discarded streams with dry_run=1 to avoid reopening them */
    for (i = 0; i < cur_periods->n_videos; i++) {
        rep = cur_periods->videos[i];
        if (!rep->ctx)
            continue;
        if (!ret)
            ret = dash_seek(s, cur_periods->videos[i], seek_pos_msec, flags, !cur_periods->videos[i]->ctx, AVMEDIA_TYPE_VIDEO);
    }
    for (i = 0; i < cur_periods->n_audios; i++) {
        rep = cur_periods->audios[i];
        if (!rep->ctx)
            continue;
        if (!ret)
            ret = dash_seek(s, cur_periods->audios[i], seek_pos_msec, flags, !cur_periods->audios[i]->ctx, AVMEDIA_TYPE_AUDIO);
    }
    for (i = 0; i < cur_periods->n_subtitles; i++) {
        rep = cur_periods->subtitles[i];
        if (!rep->ctx)
            continue;
        if (!ret)
            ret = dash_seek(s, cur_periods->subtitles[i], seek_pos_msec, flags, !cur_periods->subtitles[i]->ctx, AVMEDIA_TYPE_SUBTITLE);
    }

    return ret;
}

static int dash_probe(AVProbeData *p)
{
    if (!av_stristr((char *)p->buf, "<MPD"))
        return 0;

    if (av_stristr((char *)p->buf, "dash:profile:isoff-on-demand:2011") ||
            av_stristr((char *)p->buf, "dash:profile:isoff-live:2011") ||
            av_stristr((char *)p->buf, "dash:profile:isoff-live:2012") ||
            av_stristr((char *)p->buf, "dash:profile:isoff-main:2011") ||
            av_stristr((char *)p->buf, "3GPP:PSS:profile:DASH1")) {
        return AVPROBE_SCORE_MAX;
    }
    if (av_stristr((char *)p->buf, "dash:profile")) {
        return AVPROBE_SCORE_MAX;
    }

    return 0;
}

static int switch_dash_mulitprogram_track(AVFormatContext *s, int switch_idx, int type)
{
    int i = 0, ret = -1, cur_seq_no = -1, stream_index = -1, front_strems_idx= -1, previous_seq_no= -1, first_seq_no = -1, last_seq_no = -1;
    DASHContext *c = s->priv_data;
    struct representation *rep;
    Period *cur_periods = NULL;

    if (!s || !c || (c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods))
        return ret;
    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return ret;
    switch (type) {
        case CODEC_TYPE_VIDEO: // video track.
            if ((switch_idx >= cur_periods->n_videos) || (switch_idx == cur_periods->cur_vtrack_idx)) {
                return ret;
            }
            rep = cur_periods->videos[cur_periods->cur_vtrack_idx];
            cur_seq_no = rep->cur_seq_no;
            last_seq_no = rep->last_seq_no;
            previous_seq_no = rep->previous_seq_no;
            stream_index = rep->stream_index;
            first_seq_no = rep->first_seq_no;
            for (i = 0; i < s->nb_streams; i++) {
                AVStream* st = s->streams[i];
                if (CODEC_TYPE_VIDEO == st->codec->codec_type) {
                    front_strems_idx = i;
                }
            }
            rep->discard = AVDISCARD_ALL;
            rep->needed = 0;
            rep = cur_periods->videos[switch_idx];
            rep->first_seq_no = first_seq_no;
            rep->cur_seq_no = cur_seq_no;
            rep->last_seq_no = last_seq_no;
            rep->previous_seq_no = previous_seq_no;
            rep->stream_index = stream_index;
            rep->discard = AVDISCARD_NONE;
            rep->needed = 1;
            cur_periods->cur_vtrack_idx = switch_idx;
            rep->discard = AVDISCARD_DEFAULT;
            av_dict_set_int(&(s->url_options), "vid_stream_index", switch_idx, 0); // current video stream index
            ret = 0;
            break;
        case CODEC_TYPE_AUDIO: // audio track.
            if ((switch_idx >= cur_periods->n_audios) || (switch_idx == cur_periods->cur_atrack_idx)) {
                return ret;
            }
            rep = cur_periods->audios[cur_periods->cur_atrack_idx];
            cur_seq_no = rep->cur_seq_no;
            last_seq_no = rep->last_seq_no;
            previous_seq_no = rep->previous_seq_no;
            stream_index = rep->stream_index;
            first_seq_no = rep->first_seq_no;
            for (i = 0; i < s->nb_streams; i++) {
                AVStream* st = s->streams[i];
                if (CODEC_TYPE_AUDIO == st->codec->codec_type) {
                    front_strems_idx = i;
                }
            }
            rep->discard = AVDISCARD_ALL;
            rep->needed = 0;
            rep = cur_periods->audios[switch_idx];
            rep->first_seq_no = first_seq_no;
            rep->cur_seq_no = cur_seq_no;
            rep->last_seq_no = last_seq_no;
            rep->previous_seq_no = previous_seq_no;
            rep->stream_index = stream_index;
            rep->discard = AVDISCARD_DEFAULT;
            rep->needed = 1;
            cur_periods->cur_atrack_idx = switch_idx;
            rep->discard = AVDISCARD_DEFAULT;
            av_dict_set_int(&(s->url_options), "aud_stream_index", switch_idx, 0); // current video stream index
            ret = 0;
            break;
        case CODEC_TYPE_SUBTITLE: // subtile track.
            if ((switch_idx >= cur_periods->n_subtitles) || (switch_idx == cur_periods->cur_strack_idx)) {
                return ret;
            }
            rep = cur_periods->subtitles[cur_periods->cur_strack_idx];
            cur_seq_no = rep->cur_seq_no;
            last_seq_no = rep->last_seq_no;
            previous_seq_no = rep->previous_seq_no;
            stream_index = rep->stream_index;
            first_seq_no = rep->first_seq_no;
            for (i = 0; i < s->nb_streams; i++) {
                AVStream* st = s->streams[i];
                if (CODEC_TYPE_SUBTITLE == st->codec->codec_type) {
                    front_strems_idx = i;
                }
            }
            rep->discard = AVDISCARD_ALL;
            rep->needed = 0;
            rep = cur_periods->subtitles[switch_idx];
            rep->first_seq_no = first_seq_no;
            rep->cur_seq_no = cur_seq_no;
            rep->last_seq_no = last_seq_no;
            rep->previous_seq_no = previous_seq_no;
            rep->stream_index = stream_index;
            rep->discard = AVDISCARD_DEFAULT;
            rep->needed = 1;
            cur_periods->cur_strack_idx = switch_idx;
            rep->discard = AVDISCARD_DEFAULT;
            av_dict_set_int(&(s->url_options), "sub_stream_index", switch_idx, 0); // current video stream index
            ret = 0;
            break;
    }
    return ret;
}

static int dash_read_control(AVFormatContext *s, int cmd, void *arg)
{
    int i = 0, ret = -1, is_full_detection = 0;
    DASHContext *c = s->priv_data;
    struct representation *rep;
    Period *cur_periods = NULL;
    AvControl *av_control = (AvControl *)arg;

    GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
    if (!s || !c || !av_control || av_control->discard_idx < 0 || av_control->retain_idx < 0 || (c->current_period < 0) || (c->n_periods <= 0) || (c->current_period >= c->n_periods))
        return ret;
    cur_periods = c->periods[c->current_period];
    if (!cur_periods)
        return ret;

    switch (cmd) {
        case DASH_SWITCH_VIDEO_MULITPROGRAM_TRACK:
            if (is_full_detection) {
                for (i = 0; i < cur_periods->n_videos; i++) {
                    rep = cur_periods->videos[i];
                    if (auto_filter_decoder_unsupporrt_type(rep)) {
                        return ret;
                    }
                    if (rep->ctx && rep->last_seq_no != 0) {
                        if (rep->stream_index == av_control->discard_idx) {
                            c->av_control = *av_control;
                            rep->is_switch_track = 1;
                            cur_periods->switch_media_type = CODEC_TYPE_VIDEO;
                            break;
                        } else {
                            rep->is_switch_track = 0;
                        }
                    }
                }
                if (i >= cur_periods->n_videos) { /*not find or last_seq_no == 0*/
                    cur_periods->switch_media_type = CODEC_TYPE_UNKNOWN;
                    s->streams[av_control->discard_idx]->discard = AVDISCARD_ALL;
                    s->streams[av_control->retain_idx]->discard = AVDISCARD_DEFAULT;
                }
            } else {
                if ((av_control->retain_idx >= cur_periods->n_videos) || (av_control->retain_idx == cur_periods->cur_vtrack_idx)) {
                    return ret;
                }
                rep = cur_periods->videos[av_control->retain_idx];
                if (auto_filter_decoder_unsupporrt_type(rep)) {
                    return ret;
                }
                cur_periods->switch_idx = av_control->retain_idx;
                cur_periods->is_switch_track = 1;
                cur_periods->switch_media_type = CODEC_TYPE_VIDEO;
            }
            break;
        case DASH_SWITCH_AUDIO_MULITPROGRAM_TRACK:
            if ((av_control->retain_idx >= cur_periods->n_audios) || (av_control->retain_idx == cur_periods->cur_atrack_idx)) {
                return ret;
            }
            cur_periods->switch_idx = av_control->retain_idx;
            cur_periods->is_switch_track = 1;
            cur_periods->switch_media_type = CODEC_TYPE_AUDIO;
            break;
        case DASH_SWITCH_SUBTILE_MULITPROGRAM_TRACK:
            if ((av_control->retain_idx >= cur_periods->n_subtitles) || (av_control->retain_idx == cur_periods->cur_strack_idx)) {
                return ret;
            }
            cur_periods->switch_idx = av_control->retain_idx;
            cur_periods->is_switch_track = 1;
            cur_periods->switch_media_type = CODEC_TYPE_SUBTITLE;
            break;
        default:
            gxlogi ("dashdec.c not support!!\n");
            break;
    }
    return ret;
}

AVInputFormat dash_demuxer = {
    .name               = "dash",
    .priv_data_size     = sizeof(DASHContext),
    .read_probe         = dash_probe,
    .read_header        = dash_read_header,
    .read_packet        = dash_read_packet,
    .read_close         = dash_close,
    .read_seek          = dash_read_seek,
    .read_control       = dash_read_control,
};


