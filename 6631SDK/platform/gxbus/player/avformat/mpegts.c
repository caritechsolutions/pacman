/*
 * MPEG2 transport stream (aka DVB) demuxer
 * Copyright (c) 2002-2003 Fabrice Bellard
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

//#define DEBUG
//#define DEBUG_SEEK
#define USE_SYNCPOINT_SEARCH

#include "../avutil/crc.h"
#include "../avutil/intreadwrite.h"
#include "avformat.h"
#include "bytestream.h"
#include "mpegts.h"
#include "seek.h"
#include "gx_stream.h"

/* 1.0 second at 24Mbit/s */
#define MAX_SCAN_PACKETS 32000

/* maximum size in which we look for synchronisation if
   synchronisation is lost */
#define MAX_RESYNC_SIZE 65536

#define MAX_PES_PAYLOAD 90*1024//ff defalut max:200KB

#define AV_INPUT_BUFFER_PADDING_SIZE 64
static const uint8_t opus_default_extradata[30] = {
	'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t opus_coupled_stream_cnt[9] = {
    1, 0, 1, 1, 2, 2, 2, 3, 3
};

static const uint8_t opus_stream_cnt[9] = {
    1, 1, 1, 2, 2, 3, 4, 4, 5,
};

static const uint8_t opus_channel_map[8][8] = {
    { 0 },
    { 0,1 },
    { 0,2,1 },
    { 0,1,2,3 },
    { 0,4,1,2,3 },
    { 0,4,1,2,3,5 },
    { 0,4,1,2,3,5,6 },
    { 0,6,1,2,3,4,5,7 },
};

enum MpegTSFilterType {
    MPEGTS_PES,
    MPEGTS_SECTION,
};

typedef struct MpegTSFilter MpegTSFilter;

typedef int PESCallback(MpegTSFilter *f, const uint8_t *buf, int len, int is_start, int64_t pos);

typedef struct MpegTSPESFilter {
    PESCallback *pes_cb;
    void *opaque;
} MpegTSPESFilter;

typedef void SectionCallback(MpegTSFilter *f, const uint8_t *buf, int len);

typedef void SetServiceCallback(void *opaque, int ret);

typedef struct MpegTSSectionFilter {
    int section_index;
    int section_h_size;
    uint8_t *section_buf;
    unsigned int check_crc:1;
    unsigned int end_of_section_reached:1;
    SectionCallback *section_cb;
    void *opaque;
} MpegTSSectionFilter;

struct MpegTSFilter {
    int pid;
    int last_cc; /* last cc code (-1 if first packet) */
    enum MpegTSFilterType type;
    union {
        MpegTSPESFilter pes_filter;
        MpegTSSectionFilter section_filter;
    } u;
};

#define MAX_PIDS_PER_PROGRAM 64
struct Program {
    unsigned int id; //program id/service id
    unsigned int nb_pids;
    unsigned int pids[MAX_PIDS_PER_PROGRAM];
    /** have we found pmt for this program */
    int pmt_found;
};

struct MpegTSContext {
    /* user data */
    AVFormatContext *stream;
    /** raw packet size, including FEC if present            */
    int raw_packet_size;

    int pos47;

    /** if true, all pids are analyzed to find streams       */
    int auto_guess;

    /** compute exact PCR for each transport stream packet   */
    int mpeg2ts_compute_pcr;

    int64_t cur_pcr;    /**< used to estimate the exact PCR  */
    int pcr_incr;       /**< used to estimate the exact PCR  */

    /* data needed to handle file based ts */
    /** stop parsing loop                                    */
    int stop_parse;
    /** packet containing Audio/Video data                   */
    AVPacket *pkt;
    /** to detect seek                                       */
    int64_t last_pos;

    /******************************************/
    /* private mpegts data */
    /* scan context */
    /** structure to keep track of Program->pids mapping     */
    unsigned int nb_prg;
    int current_pid;
    struct Program *prg;


    /** filters for various streams specified by PMT + for the PAT and PMT */
    MpegTSFilter *pids[NB_PID_MAX];
};

/* TS stream handling */

enum MpegTSState {
    MPEGTS_HEADER = 0,
    MPEGTS_PESHEADER,
    MPEGTS_PESHEADER_FILL,
    MPEGTS_PAYLOAD,
    MPEGTS_SKIP,
};

/* enough for PES header + length */
#define PES_START_SIZE  6
#define PES_HEADER_SIZE 9
#define MAX_PES_HEADER_SIZE (9 + 255)

typedef struct PESContext {
    int pid;
    int pcr_pid; /**< if -1 then all packets containing PCR are considered */
    int stream_type;
    MpegTSContext *ts;
    AVFormatContext *stream;
    AVStream *st;
    AVStream *sub_st; /**< stream for the embedded AC3 stream in HDMV TrueHD */
    enum MpegTSState state;
    /* used to get the format */
    int data_index;
    int total_size;
    int pes_header_size;
    int extended_stream_id;
    int64_t pts, dts;
    int64_t ts_packet_pos; /**< position of first TS packet of this PES packet */
    uint8_t header[MAX_PES_HEADER_SIZE];
    uint8_t *buffer;
} PESContext;

extern AVInputFormat mpegts_demuxer;

static void clear_program(MpegTSContext *ts, unsigned int programid)
{
    int i;

    for(i=0; i<ts->nb_prg; i++)
        if(ts->prg[i].id == programid)
            ts->prg[i].nb_pids = 0;
}

static void clear_programs(MpegTSContext *ts)
{
    av_freep(&ts->prg);
    ts->nb_prg=0;
}

static void add_pat_entry(MpegTSContext *ts, unsigned int programid)
{
    struct Program *p;
    void *tmp = av_realloc(ts->prg, (ts->nb_prg+1)*sizeof(struct Program));
    if(!tmp)
        return;
    ts->prg = tmp;
    p = &ts->prg[ts->nb_prg];
    p->id = programid;
    p->nb_pids = 0;
    p->pmt_found = 0;
    ts->nb_prg++;
}

static void add_pid_to_pmt(MpegTSContext *ts, unsigned int programid, unsigned int pid)
{
    int i;
    struct Program *p = NULL;
    for(i=0; i<ts->nb_prg; i++) {
        if(ts->prg[i].id == programid) {
            p = &ts->prg[i];
            break;
        }
    }
    if(!p)
        return;

    if(p->nb_pids >= MAX_PIDS_PER_PROGRAM)
        return;
    p->pids[p->nb_pids++] = pid;
}

static struct Program * get_program(MpegTSContext *ts, unsigned int programid)
{
    int i;
    for (i = 0; i < ts->nb_prg; i++) {
        if (ts->prg[i].id == programid) {
            return &ts->prg[i];
        }
    }
    return NULL;
}

static void set_pmt_found(MpegTSContext *ts, unsigned int programid)
{
    struct Program *p = get_program(ts, programid);
    if (!p)
        return;

    p->pmt_found = 1;
}

/**
 * \brief discard_pid() decides if the pid is to be discarded according
 *                      to caller's programs selection
 * \param ts    : - TS context
 * \param pid   : - pid
 * \return 1 if the pid is only comprised in programs that have .discard=AVDISCARD_ALL
 *         0 otherwise
 */
static int discard_pid(MpegTSContext *ts, unsigned int pid)
{
    int i, j, k;
    int used = 0, discarded = 0;
    struct Program *p;
    for(i=0; i<ts->nb_prg; i++) {
        p = &ts->prg[i];
        for(j=0; j<p->nb_pids; j++) {
            if(p->pids[j] != pid)
                continue;
            //is program with id p->id set to be discarded?
            for(k=0; k<ts->stream->nb_programs; k++) {
                if(ts->stream->programs[k]->id == p->id) {
                    if(ts->stream->programs[k]->discard == AVDISCARD_ALL)
                        discarded++;
                    else
                        used++;
                }
            }
        }
    }

    return !used && discarded;
}

/**
 *  Assembles PES packets out of TS packets, and then calls the "section_cb"
 *  function when they are complete.
 */
static void write_section_data(AVFormatContext *s, MpegTSFilter *tss1,
                               const uint8_t *buf, int buf_size, int is_start)
{
    MpegTSSectionFilter *tss = &tss1->u.section_filter;
    int len;

    if (is_start) {
        memcpy(tss->section_buf, buf, buf_size);
        tss->section_index = buf_size;
        tss->section_h_size = -1;
        tss->end_of_section_reached = 0;
    } else {
        if (tss->end_of_section_reached)
            return;
        len = 4096 - tss->section_index;
        if (buf_size < len)
            len = buf_size;
        memcpy(tss->section_buf + tss->section_index, buf, len);
        tss->section_index += len;
    }

    /* compute section length if possible */
    if (tss->section_h_size == -1 && tss->section_index >= 3) {
        len = (AV_RB16(tss->section_buf + 1) & 0xfff) + 3;
        if (len > 4096)
            return;
        tss->section_h_size = len;
    }

    if (tss->section_h_size != -1 && tss->section_index >= tss->section_h_size) {
        tss->end_of_section_reached = 1;
        if (!tss->check_crc ||
            av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1,
                   tss->section_buf, tss->section_h_size) == 0)
            tss->section_cb(tss1, tss->section_buf, tss->section_h_size);
    }
}

static MpegTSFilter *mpegts_open_section_filter(MpegTSContext *ts, unsigned int pid,
                                         SectionCallback *section_cb, void *opaque,
                                         int check_crc)

{
    MpegTSFilter *filter;
    MpegTSSectionFilter *sec;

    //gxlogf("Filter: pid=0x%x\n", pid);

    if (pid >= NB_PID_MAX || ts->pids[pid])
        return NULL;
    filter = av_mallocz(sizeof(MpegTSFilter));
    if (!filter)
        return NULL;
    ts->pids[pid] = filter;
    filter->type = MPEGTS_SECTION;
    filter->pid = pid;
    filter->last_cc = -1;
    sec = &filter->u.section_filter;
    sec->section_cb = section_cb;
    sec->opaque = opaque;
    sec->section_buf = av_malloc(MAX_SECTION_SIZE);
    sec->check_crc = check_crc;
    if (!sec->section_buf) {
        av_free(filter);
        return NULL;
    }
    return filter;
}

static MpegTSFilter *mpegts_open_pes_filter(MpegTSContext *ts, unsigned int pid,
                                     PESCallback *pes_cb,
                                     void *opaque)
{
    MpegTSFilter *filter;
    MpegTSPESFilter *pes;

    if (pid >= NB_PID_MAX || ts->pids[pid])
        return NULL;
    filter = av_mallocz(sizeof(MpegTSFilter));
    if (!filter)
        return NULL;
    ts->pids[pid] = filter;
    filter->type = MPEGTS_PES;
    filter->pid = pid;
    filter->last_cc = -1;
    pes = &filter->u.pes_filter;
    pes->pes_cb = pes_cb;
    pes->opaque = opaque;
    return filter;
}

static void mpegts_close_filter(MpegTSContext *ts, MpegTSFilter *filter)
{
    int pid = 0;
    if (!filter)
        return ;

    pid = filter->pid;
    if (filter->type == MPEGTS_SECTION) {
        av_freep(&filter->u.section_filter.section_buf);
    } else if (filter->type == MPEGTS_PES) {
        PESContext *pes = filter->u.pes_filter.opaque;
        if (pes->buffer) {
            av_free(pes->buffer);
            pes->buffer = NULL;
        }
        /* referenced private data will be freed later in
         * av_close_input_stream */
        if (!((PESContext *)filter->u.pes_filter.opaque)->st) {
            av_freep(&filter->u.pes_filter.opaque);
        }
    }

    if (filter) {
        av_free(filter);
        filter = NULL;
    }
    if (pid < NB_PID_MAX) {
        ts->pids[pid] = NULL;
    }
}

static int analyze(const uint8_t *buf, int size, int packet_size, int *index){
    int stat[TS_MAX_PACKET_SIZE];
    int i;
    int x=0;
    int best_score=0;

    memset(stat, 0, packet_size*sizeof(int));

    for(x=i=0; i<size-3; i++){
        if(buf[i] == 0x47 && !(buf[i+1] & 0x80) && (buf[i+3] & 0x30)){
            stat[x]++;
            if(stat[x] > best_score){
                best_score= stat[x];
                if(index) *index= x;
            }
        }

        x++;
        if(x == packet_size) x= 0;
    }

    return best_score;
}

/* autodetect fec presence. Must have at least 1024 bytes  */
static int get_packet_size(const uint8_t *buf, int size)
{
    int score, fec_score, dvhs_score;

    if (size < (TS_FEC_PACKET_SIZE * 5 + 1))
        return -1;

    score    = analyze(buf, size, TS_PACKET_SIZE, NULL);
    dvhs_score    = analyze(buf, size, TS_DVHS_PACKET_SIZE, NULL);
    fec_score= analyze(buf, size, TS_FEC_PACKET_SIZE, NULL);
//    av_log(NULL, AV_LOG_DEBUG, "score: %d, dvhs_score: %d, fec_score: %d \n", score, dvhs_score, fec_score);

    if     (score > fec_score && score > dvhs_score) return TS_PACKET_SIZE;
    else if(dvhs_score > score && dvhs_score > fec_score) return TS_DVHS_PACKET_SIZE;
    else if(score < fec_score && dvhs_score < fec_score) return TS_FEC_PACKET_SIZE;
    else                       return -1;
}

typedef struct SectionHeader {
    uint8_t tid;
    uint16_t id;
    uint8_t version;
    uint8_t sec_num;
    uint8_t last_sec_num;
} SectionHeader;

static inline int get8(const uint8_t **pp, const uint8_t *p_end)
{
    const uint8_t *p;
    int c;

    p = *pp;
    if (p >= p_end)
        return -1;
    c = *p++;
    *pp = p;
    return c;
}

static inline int get16(const uint8_t **pp, const uint8_t *p_end)
{
    const uint8_t *p;
    int c;

    p = *pp;
    if ((p + 1) >= p_end)
        return -1;
    c = AV_RB16(p);
    p += 2;
    *pp = p;
    return c;
}

/* read and allocate a DVB string preceeded by its length */
static char *getstr8(const uint8_t **pp, const uint8_t *p_end)
{
    int len;
    const uint8_t *p;
    char *str;

    p = *pp;
    len = get8(&p, p_end);
    if (len < 0)
        return NULL;
    if ((p + len) > p_end)
        return NULL;
    str = av_malloc(len + 1);
    if (!str)
        return NULL;
    memcpy(str, p, len);
    str[len] = '\0';
    p += len;
    *pp = p;
    return str;
}

static int parse_section_header(SectionHeader *h,
                                const uint8_t **pp, const uint8_t *p_end)
{
    int val;

    val = get8(pp, p_end);
    if (val < 0)
        return -1;
    h->tid = val;
    *pp += 2;
    val = get16(pp, p_end);
    if (val < 0)
        return -1;
    h->id = val;
    val = get8(pp, p_end);
    if (val < 0)
        return -1;
    h->version = (val >> 1) & 0x1f;
    val = get8(pp, p_end);
    if (val < 0)
        return -1;
    h->sec_num = val;
    val = get8(pp, p_end);
    if (val < 0)
        return -1;
    h->last_sec_num = val;
    return 0;
}

typedef struct {
    uint32_t stream_type;
    enum AVMediaType codec_type;
    enum CodecID codec_id;
	ESStreamType codec_tag;
} StreamType;

static const StreamType ISO_types[] = {
    { 0x01, AVMEDIA_TYPE_VIDEO, CODEC_ID_MPEG2VIDEO, VIDEO_MPEG1},
    { 0x02, AVMEDIA_TYPE_VIDEO, CODEC_ID_MPEG2VIDEO, VIDEO_MPEG2},
    { 0x03, AVMEDIA_TYPE_AUDIO,        CODEC_ID_MP3, AUDIO_MP2},
    { 0x04, AVMEDIA_TYPE_AUDIO,        CODEC_ID_MP3, AUDIO_MP2},
    { 0x0f, AVMEDIA_TYPE_AUDIO,   CODEC_ID_AAC_LATM, AUDIO_AAC},
    { 0x10, AVMEDIA_TYPE_VIDEO,      CODEC_ID_MPEG4, VIDEO_MPEG4},
    { 0x11, AVMEDIA_TYPE_AUDIO,   CODEC_ID_AAC_LATM, AUDIO_AAC}, /* LATM syntax */
    { 0x1b, AVMEDIA_TYPE_VIDEO,       CODEC_ID_H264, VIDEO_H264},
    { 0x20, AVMEDIA_TYPE_VIDEO,       CODEC_ID_H264, VIDEO_H264},
    { 0x24, AVMEDIA_TYPE_VIDEO,       CODEC_ID_HEVC, VIDEO_H265},
    { 0x42, AVMEDIA_TYPE_VIDEO,      CODEC_ID_CAVS, VIDEO_AVS},
    { 0xd1, AVMEDIA_TYPE_VIDEO,      CODEC_ID_DIRAC, 0xA4},
    { 0xea, AVMEDIA_TYPE_VIDEO,        CODEC_ID_VC1, VIDEO_VC1},
    { 0 },
};

static const StreamType HDMV_types[] = {
	{ 0x80, AVMEDIA_TYPE_AUDIO, CODEC_ID_PCM_BLURAY, AUDIO_LPCM_BE},
	{ 0x81, AVMEDIA_TYPE_AUDIO, CODEC_ID_AC3,        AUDIO_A52},
	{ 0x82, AVMEDIA_TYPE_AUDIO, CODEC_ID_DTS,        AUDIO_DTS},
	{ 0x83, AVMEDIA_TYPE_AUDIO, CODEC_ID_TRUEHD,     VIDEO_UNKNOWN},
	{ 0x84, AVMEDIA_TYPE_AUDIO, CODEC_ID_EAC3,       VIDEO_UNKNOWN},
	{ 0x85, AVMEDIA_TYPE_AUDIO, CODEC_ID_DTS,        AUDIO_DTS}, /* DTS HD */
	{ 0x86, AVMEDIA_TYPE_AUDIO, CODEC_ID_DTS,        AUDIO_DTS}, /* DTS HD MASTER*/
	{ 0xa1, AVMEDIA_TYPE_AUDIO, CODEC_ID_EAC3,       AUDIO_A52}, /* E-AC3 Secondary Audio */
	{ 0xa2, AVMEDIA_TYPE_AUDIO, CODEC_ID_DTS,        AUDIO_DTS}, /* DTS Express Secondary Audio */
	{ 0x90, AVMEDIA_TYPE_SUBTITLE, CODEC_ID_HDMV_PGS_SUBTITLE,  SPU_DVD},
	{ 0x92, AVMEDIA_TYPE_SUBTITLE, CODEC_ID_TEXT,               SPU_TXT},
	{ 0 },
};

/* ATSC ? */
static const StreamType MISC_types[] = {
    { 0x81, AVMEDIA_TYPE_AUDIO,   CODEC_ID_AC3, AUDIO_A52},
    { 0x8a, AVMEDIA_TYPE_AUDIO,   CODEC_ID_DTS, AUDIO_DTS},
    { 0 },
};

/* HLS Sample Encryption Types  */
static const StreamType HLS_SAMPLE_ENC_types[] = {
    { 0xdb, AVMEDIA_TYPE_VIDEO, CODEC_ID_H264},
    { 0xcf, AVMEDIA_TYPE_AUDIO, CODEC_ID_AAC },
    { 0xc1, AVMEDIA_TYPE_AUDIO, CODEC_ID_AC3 },
    { 0xc2, AVMEDIA_TYPE_AUDIO, CODEC_ID_EAC3},
    { 0 },
};

static const StreamType REGD_types[] = {
    { MKTAG('d', 'r', 'a', 'c'), AVMEDIA_TYPE_VIDEO, CODEC_ID_DIRAC },
    { MKTAG('A', 'C', '-', '3'), AVMEDIA_TYPE_AUDIO, CODEC_ID_AC3   },
    { MKTAG('B', 'S', 'S', 'D'), AVMEDIA_TYPE_AUDIO, CODEC_ID_S302M },
    { MKTAG('D', 'T', 'S', '1'), AVMEDIA_TYPE_AUDIO, CODEC_ID_DTS   },
    { MKTAG('D', 'T', 'S', '2'), AVMEDIA_TYPE_AUDIO, CODEC_ID_DTS   },
    { MKTAG('D', 'T', 'S', '3'), AVMEDIA_TYPE_AUDIO, CODEC_ID_DTS   },
    { MKTAG('H', 'E', 'V', 'C'), AVMEDIA_TYPE_VIDEO, CODEC_ID_HEVC  },
    { MKTAG('K', 'L', 'V', 'A'), AVMEDIA_TYPE_DATA,  CODEC_ID_SMPTE_KLV },
    { MKTAG('V', 'C', '-', '1'), AVMEDIA_TYPE_VIDEO, CODEC_ID_VC1   },
    { MKTAG('O', 'p', 'u', 's'), AVMEDIA_TYPE_AUDIO, CODEC_ID_OPUS  },
    { MKTAG('I', 'D', '3', ' '), AVMEDIA_TYPE_DATA,  CODEC_ID_TIMED_ID3 },
    { 0 },
};

static const StreamType METADATA_types[] = {
    { MKTAG('K','L','V','A'), AVMEDIA_TYPE_DATA, CODEC_ID_SMPTE_KLV },
    { MKTAG('I','D','3',' '), AVMEDIA_TYPE_DATA, CODEC_ID_TIMED_ID3 },
    { 0 },
};

/* descriptor present */
static const StreamType DESC_types[] = {
    { 0x6a, AVMEDIA_TYPE_AUDIO,             CODEC_ID_AC3, 0xA4}, /* AC-3 descriptor */
    { 0x7a, AVMEDIA_TYPE_AUDIO,            CODEC_ID_EAC3, VIDEO_UNKNOWN}, /* E-AC-3 descriptor */
    { 0x7b, AVMEDIA_TYPE_AUDIO,             CODEC_ID_DTS, AUDIO_DTS},
    { 0x56, AVMEDIA_TYPE_SUBTITLE, CODEC_ID_DVB_TELETEXT, VIDEO_UNKNOWN},
    { 0x59, AVMEDIA_TYPE_SUBTITLE, CODEC_ID_DVB_SUBTITLE, 0xE0}, /* subtitling descriptor */
    { 0 },
};

static void mpegts_find_stream_type(AVStream *st,
                                    uint32_t stream_type, const StreamType *types)
{
    for (; types->stream_type; types++) {
        if (stream_type == types->stream_type) {
            st->codec->codec_type = types->codec_type;
            st->codec->codec_id   = types->codec_id;
            st->codec->codec_tag  = types->codec_tag;
            st->request_probe = 0;
            return;
        }
    }
}

static int mpegts_set_stream_info(AVStream *st, PESContext *pes,
                                  uint32_t stream_type, uint32_t prog_reg_desc)
{
    av_set_pts_info(st, 33, 1, 90000);
    st->priv_data = pes;
    st->codec->codec_type = CODEC_TYPE_UNKNOWN;
    st->codec->codec_id   = CODEC_ID_NONE;
    st->need_parsing = AVSTREAM_PARSE_FULL;
    pes->st = st;
    pes->stream_type = stream_type;

    gxlogf("stream=%d stream_type=%x pid=%x prog_reg_desc=%.4s\n",
           st->index, pes->stream_type, pes->pid, (char*)&prog_reg_desc);

    st->codec->codec_tag = pes->stream_type;

    mpegts_find_stream_type(st, pes->stream_type, ISO_types);

    if (pes->stream_type == 4 || pes->stream_type == 0x0f)
        st->request_probe = 50;

    if (prog_reg_desc == AV_RL32("HDMV") &&
        st->codec->codec_id == CODEC_ID_NONE) {
        mpegts_find_stream_type(st, pes->stream_type, HDMV_types);
        if (pes->stream_type == 0x83) {
            // HDMV TrueHD streams also contain an AC3 coded version of the
            // audio track - add a second stream for this
            AVStream *sub_st;
            // priv_data cannot be shared between streams
            PESContext *sub_pes = av_malloc(sizeof(*sub_pes));
            if (!sub_pes)
                return AVERROR(ENOMEM);
            memcpy(sub_pes, pes, sizeof(*sub_pes));

            sub_st = av_new_stream(pes->stream, pes->pid);
            if (!sub_st) {
                av_free(sub_pes);
                return AVERROR(ENOMEM);
            }

            av_set_pts_info(sub_st, 33, 1, 90000);
            sub_st->priv_data = sub_pes;
            sub_st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
            sub_st->codec->codec_id   = CODEC_ID_AC3;
            sub_st->need_parsing = AVSTREAM_PARSE_FULL;
            sub_pes->sub_st = pes->sub_st = sub_st;
        }
    }
    if (pes->stream_type == 0x11)
        gxlogd ("AAC LATM not currently supported, patch welcome\n");
    if (st->codec->codec_id == CODEC_ID_NONE)
        mpegts_find_stream_type(st, pes->stream_type, MISC_types);
    if (st->codec->codec_id == AV_CODEC_ID_NONE)
        mpegts_find_stream_type(st, pes->stream_type, HLS_SAMPLE_ENC_types);
    if ((st->codec->codec_id == AV_CODEC_ID_NONE || (st->request_probe > 0 && st->request_probe < AVPROBE_SCORE_STREAM_RETRY / 5)) &&
        (st->probe_packets > 0) &&
        stream_type == STREAM_TYPE_PRIVATE_DATA) {
        st->codec->codec_type = CODEC_TYPE_DATA;
        st->codec->codec_id   = CODEC_ID_BIN_DATA;
        st->request_probe = AVPROBE_SCORE_STREAM_RETRY / 5;
    }

    return 0;
}

static int64_t get_pts(const uint8_t *p)
{
   int64_t pts = (int64_t) (p[0] & 0x0E) << 29;
   pts |= p[1] << 22;
   pts |= (p[2] & 0xFE) << 14;
   pts |= p[3] << 7;
   pts |= (p[4] & 0xFE) >> 1;
   return pts;
}

static void reset_pes_packet_state(PESContext *pes)
{
    pes->pts        = AV_NOPTS_VALUE;
    pes->dts        = AV_NOPTS_VALUE;
    pes->data_index = 0;
    //pes->flags      = 0;
    if (pes->buffer)
        av_freep(&pes->buffer);
}

static void new_pes_packet(PESContext *pes, AVPacket *pkt)
{
    av_init_packet(pkt);

    pkt->destruct = av_destruct_packet;
    pkt->data = pes->buffer;
    pkt->size = pes->data_index;
    memset(pkt->data+pkt->size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    // Separate out the AC3 substream from an HDMV combined TrueHD/AC3 PID
    if (pes->sub_st && pes->stream_type == 0x83 && pes->extended_stream_id == 0x76)
        pkt->stream_index = pes->sub_st->index;
    else
        pkt->stream_index = pes->st->index;
    pkt->pts = pes->pts;
    pkt->dts = pes->dts;
    /* store position of first TS packet of this PES packet */
    pkt->pos = pes->ts_packet_pos;

    /* reset pts values */
    pes->pts = AV_NOPTS_VALUE;
    pes->dts = AV_NOPTS_VALUE;
    pes->buffer = NULL;
    pes->data_index = 0;
}

/* return non zero if a packet could be constructed */
static int mpegts_push_data(MpegTSFilter *filter,
                            const uint8_t *buf, int buf_size, int is_start,
                            int64_t pos)
{
    PESContext *pes = filter->u.pes_filter.opaque;
    MpegTSContext *ts = pes->ts;
    const uint8_t *p;
    int len, code, media_max_streams = 0;
    GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);

    if(!ts->pkt)
        return 0;

    if (is_start) {
        if (pes->state == MPEGTS_PAYLOAD && pes->data_index > 0) {
            new_pes_packet(pes, ts->pkt);
            ts->stop_parse = 1;
        } else {
            reset_pes_packet_state(pes);
        }
        pes->state = MPEGTS_HEADER;
        pes->data_index = 0;
        pes->ts_packet_pos = pos;
    }
    p = buf;
    while (buf_size > 0) {
        switch(pes->state) {
        case MPEGTS_HEADER:
            len = PES_START_SIZE - pes->data_index;
            if (len > buf_size)
                len = buf_size;
            memcpy(pes->header + pes->data_index, p, len);
            pes->data_index += len;
            p += len;
            buf_size -= len;
            if (pes->data_index == PES_START_SIZE) {
                /* we got all the PES or section header. We can now
                   decide */
#if 0
                av_hex_dump_log(pes->stream, AV_LOG_DEBUG, pes->header, pes->data_index);
#endif
                if (pes->header[0] == 0x00 && pes->header[1] == 0x00 &&
                    pes->header[2] == 0x01) {
                    /* it must be an mpeg2 PES stream */
                    code = pes->header[3] | 0x100;
                    //gxlogf("pid=%x pes_code=%#x\n", pes->pid, code);

                    if ((!pes->st && pes->stream->nb_streams == media_max_streams) ||
                        (pes->st && pes->st->discard == AVDISCARD_ALL) ||
                        code == 0x1be) /* padding_stream */
                        goto skip;

                    /* stream not present in PMT */
                    if (!pes->st) {
                        pes->st = av_new_stream(ts->stream, pes->pid);
                        if (!pes->st)
                            return AVERROR(ENOMEM);
                        mpegts_set_stream_info(pes->st, pes, 0, 0);
                    }

                    pes->total_size = AV_RB16(pes->header + 4);
                    /* NOTE: a zero total size means the PES size is
                       unbounded */
                    if (!pes->total_size)
                        pes->total_size = MAX_PES_PAYLOAD;

                    /* allocate pes buffer */
                    if (!pes->buffer) {
                        pes->buffer = av_mallocz(pes->total_size+FF_INPUT_BUFFER_PADDING_SIZE);
                        if (!pes->buffer)
                            return AVERROR(ENOMEM);
                    }

                    if (code != 0x1bc && code != 0x1bf && /* program_stream_map, private_stream_2 */
                        code != 0x1f0 && code != 0x1f1 && /* ECM, EMM */
                        code != 0x1ff && code != 0x1f2 && /* program_stream_directory, DSMCC_stream */
                        code != 0x1f8) {                  /* ITU-T Rec. H.222.1 type E stream */
                        pes->state = MPEGTS_PESHEADER;
                        if (pes->st->codec && pes->st->codec->codec_id == CODEC_ID_NONE && !pes->st->request_probe) {
                            gxlogf("pid=%x stream_type=%x probing\n",
                                    pes->pid, pes->stream_type);
                            pes->st->request_probe = 1;
                        }
                    } else {
                        pes->state = MPEGTS_PAYLOAD;
                        pes->data_index = 0;
                    }
                } else {
                    /* otherwise, it should be a table */
                    /* skip packet */
                skip:
                    pes->state = MPEGTS_SKIP;
                    continue;
                }
            }
            break;
            /**********************************************/
            /* PES packing parsing */
        case MPEGTS_PESHEADER:
            len = PES_HEADER_SIZE - pes->data_index;
            if (len < 0)
                return AVERROR_INVALIDDATA;
            if (len > buf_size)
                len = buf_size;
            memcpy(pes->header + pes->data_index, p, len);
            pes->data_index += len;
            p += len;
            buf_size -= len;
            if (pes->data_index == PES_HEADER_SIZE) {
                pes->pes_header_size = pes->header[8] + 9;
                pes->state = MPEGTS_PESHEADER_FILL;
            }
            break;
        case MPEGTS_PESHEADER_FILL:
            len = pes->pes_header_size - pes->data_index;
            if (len < 0)
                return AVERROR_INVALIDDATA;
            if (len > buf_size)
                len = buf_size;
            memcpy(pes->header + pes->data_index, p, len);
            pes->data_index += len;
            p += len;
            buf_size -= len;
            if (pes->data_index == pes->pes_header_size) {
                const uint8_t *r;
                unsigned int flags, pes_ext, skip;

                flags = pes->header[7];
                r = pes->header + 9;
                pes->pts = AV_NOPTS_VALUE;
                pes->dts = AV_NOPTS_VALUE;
                if ((flags & 0xc0) == 0x80) {
                    pes->dts = pes->pts = get_pts(r);
                    r += 5;
                } else if ((flags & 0xc0) == 0xc0) {
                    pes->pts = get_pts(r);
                    r += 5;
                    pes->dts = get_pts(r);
                    r += 5;
                }
                pes->extended_stream_id = -1;
                if (flags & 0x01) { /* PES extension */
                    pes_ext = *r++;
                    /* Skip PES private data, program packet sequence counter and P-STD buffer */
                    skip = (pes_ext >> 4) & 0xb;
                    skip += skip & 0x9;
                    r += skip;
                    if ((pes_ext & 0x41) == 0x01 &&
                        (r + 2) <= (pes->header + pes->pes_header_size)) {
                        /* PES extension 2 */
                        if ((r[0] & 0x7f) > 0 && (r[1] & 0x80) == 0)
                            pes->extended_stream_id = r[1];
                    }
                }

                /* we got the full header. We parse it and get the payload */
                pes->state = MPEGTS_PAYLOAD;
                pes->data_index = 0;
            }
            break;
        case MPEGTS_PAYLOAD:
            if (buf_size > 0 && pes->buffer) {
                if ((pes->data_index > 0) && ((pes->data_index+buf_size) > pes->total_size)) {
                    new_pes_packet(pes, ts->pkt);
                    pes->total_size = MAX_PES_PAYLOAD;
                    if (!pes->buffer) {
                        pes->buffer = av_mallocz(pes->total_size+FF_INPUT_BUFFER_PADDING_SIZE);
                        if (!pes->buffer)
                            return AVERROR(ENOMEM);
                    }
                    ts->stop_parse = 1;
                } else if (pes->data_index == 0 && buf_size > pes->total_size) {
                    // pes packet size is < ts size packet and pes data is padded with 0xff
                    // not sure if this is legal in ts but see issue #2392
                    buf_size = pes->total_size;
                }
                memcpy(pes->buffer+pes->data_index, p, buf_size);
                pes->data_index += buf_size;
            }
            buf_size = 0;
            /* emit complete packets with known packet size
             * decreases demuxer delay for infrequent packets like subtitles from
             * a couple of seconds to milliseconds for properly muxed files.
             * total_size is the number of bytes following pes_packet_length
             * in the pes header, i.e. not counting the first 6 bytes */
            if (!ts->stop_parse && (pes->total_size < MAX_PES_PAYLOAD) &&
                ((pes->pes_header_size + pes->data_index) == (pes->total_size + PES_START_SIZE))) {
                ts->stop_parse = 1;
                new_pes_packet(pes, ts->pkt);
            }
            break;
        case MPEGTS_SKIP:
            buf_size = 0;
            break;
        }
    }

    return 0;
}

static PESContext *add_pes_stream(MpegTSContext *ts, int pid, int pcr_pid)
{
    MpegTSFilter *tss;
    PESContext *pes;

    /* if no pid found, then add a pid context */
    pes = av_mallocz(sizeof(PESContext));
    if (!pes)
        return 0;
    pes->ts = ts;
    pes->stream = ts->stream;
    pes->pid = pid;
    pes->pcr_pid = pcr_pid;
    pes->state = MPEGTS_SKIP;
    pes->pts = AV_NOPTS_VALUE;
    pes->dts = AV_NOPTS_VALUE;
    tss = mpegts_open_pes_filter(ts, pid, mpegts_push_data, pes);
    if (!tss) {
        av_free(pes);
        return 0;
    }
    return pes;
}

static void pmt_cb(MpegTSFilter *filter, const uint8_t *section, int section_len)
{
    MpegTSContext *ts = filter->u.section_filter.opaque;
    SectionHeader h1, *h = &h1;
    PESContext *pes;
    AVStream *st;
    const uint8_t *p, *p_end, *desc_list_end, *desc_end;
    int program_info_length, pcr_pid, pid, stream_type;
    int desc_list_len, desc_len, desc_tag, ext_desc_tag;
    int comp_page, anc_page;
    char language[4];
    uint32_t prog_reg_desc = 0; /* registration descriptor */

#ifdef DEBUG
    gxlogf("PMT: len %i\n", section_len);
    av_hex_dump_log(ts->stream, AV_LOG_DEBUG, (uint8_t *)section, section_len);
#endif

    p_end = section + section_len - 4;
    p = section;
    if (parse_section_header(h, &p, p_end) < 0)
        return;

    //gxlogf("sid=0x%x sec_num=%d/%d\n", h->id, h->sec_num, h->last_sec_num);

    if (h->tid != PMT_TID)
        return;

    clear_program(ts, h->id);
    pcr_pid = get16(&p, p_end) & 0x1fff;
    if (pcr_pid < 0)
        return;
    add_pid_to_pmt(ts, h->id, pcr_pid);

    //gxlogf("pcr_pid=0x%x\n", pcr_pid);

    program_info_length = get16(&p, p_end) & 0xfff;
    if (program_info_length < 0)
        return;
    while(program_info_length >= 2) {
        uint8_t tag, len;
        tag = get8(&p, p_end);
        len = get8(&p, p_end);
        if(len > program_info_length - 2)
            //something else is broken, exit the program_descriptors_loop
            break;
        program_info_length -= len + 2;
        if(tag == 0x05 && len >= 4) { // registration descriptor
            prog_reg_desc = bytestream_get_le32(&p);
            len -= 4;
        }
        p += len;
    }
    p += program_info_length;
    if (p >= p_end)
        return;

    // stop parsing after pmt, we found header
    if (!ts->stream->nb_streams)
        ts->stop_parse = 1;

    set_pmt_found(ts, h->id);

    for(;;) {
        st = 0;
        stream_type = get8(&p, p_end);
        if (stream_type < 0)
            break;
        pid = get16(&p, p_end);
        if (pid < 0) {
            break;
        }
        pid = pid & 0x1fff;
        if (pid == ts->current_pid) {
             break;
        }

        /* now create ffmpeg stream */
        if (ts->pids[pid] && ts->pids[pid]->type == MPEGTS_PES) {
            pes = ts->pids[pid]->u.pes_filter.opaque;
            st = pes->st;
        } else {
            if (ts->pids[pid])
                mpegts_close_filter(ts, ts->pids[pid]); //wrongly added sdt filter probably
            pes = add_pes_stream(ts, pid, pcr_pid);
            if (pes) {
                st = av_new_stream(pes->stream, pes->pid);
                if (!st) {
                    return;
                }
                st->id = pes->pid;
            }
        }

        if (!st)
            return;

        if (pes && !pes->stream_type)
            mpegts_set_stream_info(st, pes, stream_type, prog_reg_desc);

        add_pid_to_pmt(ts, h->id, pid);

        av_program_add_stream_index(ts->stream, h->id, st->index);

        desc_list_len = get16(&p, p_end);
        if (desc_list_len < 0)
            break;
        desc_list_len = desc_list_len & 0xfff;
        desc_list_end = p + desc_list_len;
        if (desc_list_end > p_end)
            break;
        for(;;) {
            desc_tag = get8(&p, desc_list_end);
            if (desc_tag < 0)
                break;
            desc_len = get8(&p, desc_list_end);
            if (desc_len < 0)
                break;
            desc_end = p + desc_len;
            if (desc_end > desc_list_end)
                break;

            //gxlogf("tag: 0x%02x len=%d\n", desc_tag, desc_len);

            if ((st->codec->codec_id == CODEC_ID_NONE || st->request_probe > 0) &&
                stream_type == STREAM_TYPE_PRIVATE_DATA)
                mpegts_find_stream_type(st, desc_tag, DESC_types);

            switch(desc_tag) {
            case 0x56: /* DVB teletext descriptor */
                language[0] = get8(&p, desc_end);
                language[1] = get8(&p, desc_end);
                language[2] = get8(&p, desc_end);
                language[3] = 0;
                comp_page = (get8(&p, desc_end) & 0x7);
                anc_page = get8(&p, desc_end);
                st->codec->codec_tag = (anc_page << 8) | comp_page;
                av_dict_set(&st->metadata, "language", language, 0);
                p -= 2;
                break;
            case 0x59: /* subtitling descriptor */
                language[0] = get8(&p, desc_end);
                language[1] = get8(&p, desc_end);
                language[2] = get8(&p, desc_end);
                language[3] = 0;
                get8(&p, desc_end);
                comp_page = get16(&p, desc_end);
                anc_page = get16(&p, desc_end);
                st->codec->codec_tag = (anc_page << 16) | comp_page;
                av_dict_set(&st->metadata, "language", language, 0);
                break;
            case 0x0a: /* ISO 639 language descriptor */
                language[0] = get8(&p, desc_end);
                language[1] = get8(&p, desc_end);
                language[2] = get8(&p, desc_end);
                language[3] = 0;
                av_dict_set(&st->metadata, "language", language, 0);
                break;
            case 0x05: /* registration descriptor */
            {
				st->codec->codec_tag = bytestream_get_le32(&p);
                //gxlogf("reg_desc=%d codec_id %d\n", st->codec->codec_tag, st->codec->codec_id);
                if ((st->codec->codec_id == CODEC_ID_NONE) || (st->request_probe > 0))
                    mpegts_find_stream_type(st, st->codec->codec_tag, REGD_types);
                break;
            }
            case 0x26: /* metadata descriptor */
                if (get16(&p, desc_end) == 0xFFFF)
                    p += 4;
                if (get8(&p, desc_end) == 0xFF) {
                    st->codec->codec_tag = bytestream_get_le32(&p);
                    if (st->codec->codec_id == CODEC_ID_NONE)
                        mpegts_find_stream_type(st, st->codec->codec_tag, METADATA_types);
                }
                break;
			case 0x7f: /* DVB extension descriptor */
				ext_desc_tag = get8(&p, desc_end);
				if (ext_desc_tag < 0)
					continue;
				if (st->codec->codec_id == CODEC_ID_OPUS &&
						ext_desc_tag == 0x80) { /* User defined (provisional Opus) */
					if (!st->codec->extradata) {
						int channel_config_code, channels;

						st->codec->extradata = av_mallocz(sizeof(opus_default_extradata) +
								AV_INPUT_BUFFER_PADDING_SIZE);
						if (!st->codec->extradata)
							continue;

						st->codec->extradata_size = sizeof(opus_default_extradata);
						memcpy(st->codec->extradata, opus_default_extradata, sizeof(opus_default_extradata));

						channel_config_code = get8(&p, desc_end);
						if (channel_config_code < 0)
							continue;
						if (channel_config_code <= 0x8) {
							st->codec->extradata[9]  = channels = channel_config_code ? channel_config_code : 2;
							st->codec->extradata[18] = channel_config_code ? (channels > 2) : /* Dual Mono */ 255;
							st->codec->extradata[19] = opus_stream_cnt[channel_config_code];
							st->codec->extradata[20] = opus_coupled_stream_cnt[channel_config_code];
							memcpy(&st->codec->extradata[21], opus_channel_map[channels - 1], channels);
						} else {
							//av_log(st, AV_LOG_ERROR, "Opus in MPEG-TS - channel_config_code > 0x8");
						}
						st->need_parsing = AVSTREAM_PARSE_FULL;
						//st->internal->need_context_update = 1;
					}
				}
				if (ext_desc_tag == 0x06) { /* supplementary audio descriptor */
					int flags;

					if (desc_len < 1)
						continue;
					flags = get8(&p, desc_end);

					if ((flags & 0x80) == 0) /* mix_type */
						st->disposition |= AV_DISPOSITION_DEPENDENT;

					switch ((flags >> 2) & 0x1F) { /* editorial_classification */
					case 0x01:
						st->disposition |= AV_DISPOSITION_VISUAL_IMPAIRED;
						break;
					case 0x02:
						st->disposition |= AV_DISPOSITION_HEARING_IMPAIRED;
						break;
					case 0x03:
						st->disposition |= AV_DISPOSITION_VISUAL_IMPAIRED;
						break;
					}

					if (flags & 0x01) { /* language_code_present */
						if (desc_len < 4)
							continue;
						language[0] = get8(&p, desc_end);
						language[1] = get8(&p, desc_end);
						language[2] = get8(&p, desc_end);
						language[3] = 0;

						/* This language always has to override a possible
						 * ISO 639 language descriptor language */
						if (language[0])
							av_dict_set(&st->metadata, "language", language, 0);
					}
				}
				break;
			default:
				break;
			}
			p = desc_end;

            if (prog_reg_desc == AV_RL32("HDMV") && stream_type == 0x83 && pes->sub_st) {
                av_program_add_stream_index(ts->stream, h->id, pes->sub_st->index);
                pes->sub_st->codec->codec_tag = st->codec->codec_tag;
            }
        }
        p = desc_list_end;
    }
#if 0
    /* all parameters are there */
    mpegts_close_filter(ts, filter);
#endif
}

static void pat_cb(MpegTSFilter *filter, const uint8_t *section, int section_len)
{
    MpegTSContext *ts = filter->u.section_filter.opaque;
    SectionHeader h1, *h = &h1;
    const uint8_t *p, *p_end;
    int sid, pmt_pid;

#ifdef DEBUG
    gxlogf("PAT:\n");
    av_hex_dump_log(ts->stream, AV_LOG_DEBUG, (uint8_t *)section, section_len);
#endif
    p_end = section + section_len - 4;
    p = section;
    if (parse_section_header(h, &p, p_end) < 0)
        return;
    if (h->tid != PAT_TID)
        return;

    clear_programs(ts);
    for(;;) {
        sid = get16(&p, p_end);
        if (sid < 0)
            break;
        pmt_pid = get16(&p, p_end);
        if (pmt_pid < 0)
            break;
        pmt_pid = pmt_pid & 0x1fff;

        //gxlogf("sid=0x%x pid=0x%x\n", sid, pmt_pid);

        if (sid == 0x0000) {
            /* NIT info */
        } else {
            MpegTSFilter *fil = ts->pids[pmt_pid];
            av_new_program(ts->stream, sid);
            if (fil) {
                if (fil->type != MPEGTS_SECTION || fil->pid != pmt_pid || fil->u.section_filter.section_cb != pmt_cb)
                    mpegts_close_filter(ts, ts->pids[pmt_pid]);
            }
            mpegts_open_section_filter(ts, pmt_pid, pmt_cb, ts, 1);
            add_pat_entry(ts, sid);
            add_pid_to_pmt(ts, sid, 0); //add pat pid to program
            add_pid_to_pmt(ts, sid, pmt_pid);
        }
    }
}

static void sdt_cb(MpegTSFilter *filter, const uint8_t *section, int section_len)
{
    MpegTSContext *ts = filter->u.section_filter.opaque;
    SectionHeader h1, *h = &h1;
    const uint8_t *p, *p_end, *desc_list_end, *desc_end;
    int onid, val, sid, desc_list_len, desc_tag, desc_len, service_type;
    char *name, *provider_name;

#ifdef DEBUG
    gxlogf("SDT:\n");
    av_hex_dump_log(ts->stream, AV_LOG_DEBUG, (uint8_t *)section, section_len);
#endif

    p_end = section + section_len - 4;
    p = section;
    if (parse_section_header(h, &p, p_end) < 0)
        return;
    if (h->tid != SDT_TID)
        return;
    onid = get16(&p, p_end);
    if (onid < 0)
        return;
    val = get8(&p, p_end);
    if (val < 0)
        return;
    for(;;) {
        sid = get16(&p, p_end);
        if (sid < 0)
            break;
        val = get8(&p, p_end);
        if (val < 0)
            break;
        desc_list_len = get16(&p, p_end) & 0xfff;
        if (desc_list_len < 0)
            break;
        desc_list_end = p + desc_list_len;
        if (desc_list_end > p_end)
            break;
        for(;;) {
            desc_tag = get8(&p, desc_list_end);
            if (desc_tag < 0)
                break;
            desc_len = get8(&p, desc_list_end);
            desc_end = p + desc_len;
            if (desc_end > desc_list_end)
                break;

            //gxlogf("tag: 0x%02x len=%d\n", desc_tag, desc_len);

            switch(desc_tag) {
            case 0x48:
                service_type = get8(&p, p_end);
                if (service_type < 0)
                    break;
                provider_name = getstr8(&p, p_end);
                if (!provider_name)
                    break;
                name = getstr8(&p, p_end);
                if (name) {
                    AVProgram *program = av_new_program(ts->stream, sid);
                    if(program) {
                        av_dict_set(&program->metadata, "name", name, 0);
                        av_dict_set(&program->metadata, "provider_name", provider_name, 0);
                    }
                }
                av_free(name);
                av_free(provider_name);
                break;
            default:
                break;
            }
            p = desc_end;
        }
        p = desc_list_end;
    }
}

/* handle one TS packet */
static int handle_packet(MpegTSContext *ts, const uint8_t *packet)
{
    AVFormatContext *s = ts->stream;
    MpegTSFilter *tss;
    int len, pid, cc, expected_cc, cc_ok, afc, is_start;
    const uint8_t *p, *p_end;
    int64_t pos;

    pid = AV_RB16(packet + 1) & 0x1fff;
    if(pid && discard_pid(ts, pid))
        return 0;
    is_start = packet[1] & 0x40;
    tss = ts->pids[pid];
    if (ts->auto_guess && !tss && is_start) {
        add_pes_stream(ts, pid, -1);
        tss = ts->pids[pid];
    }
    if (!tss)
        return 0;

    /* continuity check (currently not used) */
    cc = (packet[3] & 0xf);
    expected_cc = (packet[3] & 0x10) ? (tss->last_cc + 1) & 0x0f : tss->last_cc;
    cc_ok = (tss->last_cc < 0) || (expected_cc == cc);
    tss->last_cc = cc;

    ts->current_pid = pid;
    /* skip adaptation field */
    afc = (packet[3] >> 4) & 3;
    p = packet + 4;
    if (afc == 0) /* reserved value */
        return 0;
    if (afc == 2) /* adaptation field only */
        return 0;
    if (afc == 3) {
        /* skip adapation field */
        p += p[0] + 1;
    }
    /* if past the end of packet, ignore */
    p_end = packet + TS_PACKET_SIZE;
    if (p >= p_end)
        return 0;

    pos = url_ftell(ts->stream->pb);
    ts->pos47= pos % ts->raw_packet_size;

    if (tss->type == MPEGTS_SECTION) {
        if (is_start) {
            /* pointer field present */
            len = *p++;
            if (p + len > p_end)
                return 0;
            if (len && cc_ok) {
                /* write remaining section bytes */
                write_section_data(s, tss,
                                   p, len, 0);
                /* check whether filter has been closed */
                if (!ts->pids[pid])
                    return 0;
            }
            p += len;
            if (p < p_end) {
                write_section_data(s, tss,
                                   p, p_end - p, 1);
            }
        } else {
            if (cc_ok) {
                write_section_data(s, tss,
                                   p, p_end - p, 0);
            }
        }

        // stop find_stream_info from waiting for more streams
        // when all programs have received a PMT
        if (ts->stream->ctx_flags & AVFMTCTX_NOHEADER) {
            int i;
            for (i = 0; i < ts->nb_prg; i++) {
                if (!ts->prg[i].pmt_found)
                    break;
            }
            if (i == ts->nb_prg && ts->nb_prg > 0) {
                int types = 0;
                for (i = 0; i < ts->stream->nb_streams; i++) {
                    AVStream *st = ts->stream->streams[i];
                    if (st->codec->codec_type >= 0)
                        types |= 1<<st->codec->codec_type;
                }
                if ((types & (1<<AVMEDIA_TYPE_AUDIO) && types & (1<<AVMEDIA_TYPE_VIDEO)) || pos > 100000) {
                    ts->stream->ctx_flags &= ~AVFMTCTX_NOHEADER;
                }
            }
        }
    } else {
        int ret;
        // Note: The position here points actually behind the current packet.
        if ((ret = tss->u.pes_filter.pes_cb(tss, p, p_end - p, is_start,
                                            pos - ts->raw_packet_size)) < 0)
            return ret;
    }

    return 0;
}

/* return the consumed length if a packet was output, or -1 if no
 * packet is output */
int avpriv_mpegts_parse_packet(MpegTSContext *ts, AVPacket *pkt,
                               const uint8_t *buf, int len)
{
    int len1;

    len1 = len;
    ts->pkt = pkt;
    for (;;) {
        ts->stop_parse = 0;
        if (len < TS_PACKET_SIZE) {
            return AVERROR_INVALIDDATA;
        }
        if (buf[0] != 0x47) {
            buf++;
            len--;
        } else {
            handle_packet(ts, buf);
            buf += TS_PACKET_SIZE;
            len -= TS_PACKET_SIZE;
            if (ts->stop_parse == 1)
                break;
        }
    }
    return len1 - len;
}

/**************************************************************/
/* parsing functions - called from other demuxers such as RTP */

MpegTSContext *avpriv_mpegts_parse_open(AVFormatContext *s)
{
    MpegTSContext *ts = NULL;

    ts = av_mallocz(sizeof(MpegTSContext));
    if (!ts)
        return NULL;
    /* no stream case, currently used by RTP */
    ts->raw_packet_size = TS_PACKET_SIZE;
    ts->stream = s;
    ts->auto_guess = 1;
    mpegts_open_section_filter(ts, SDT_PID, sdt_cb, ts, 1);
    mpegts_open_section_filter(ts, PAT_PID, pat_cb, ts, 1);

    return ts;
}

/* XXX: try to find a better synchro over several packets (use
   get_packet_size() ?) */
static int mpegts_resync(AVFormatContext *s, int seekback, const uint8_t *current_packet)
{
    MpegTSContext *ts = s->priv_data;
    AVIOContext *pb = s->pb;
    int c, i;
    uint64_t pos = avio_tell(pb);
    int64_t back = FFMIN(seekback, pos);

    //Special case for files like 01c56b0dc1.ts
    if (current_packet[0] == 0x80 && current_packet[12] == 0x47 && pos >= TS_PACKET_SIZE) {
        avio_seek(pb, 12 - TS_PACKET_SIZE, SEEK_CUR);
        return 0;
    }

    avio_seek(pb, -back, SEEK_CUR);

    for (i = 0; i < MAX_RESYNC_SIZE; i++) {
        c = avio_r8(pb);
        if (c < 0)
            return -1;
        if (avio_feof(pb))
            return AVERROR_EOF;
        if (c == 0x47) {
            int64_t cur_pos = url_ftell(pb)-1;
            if(cur_pos < 0)
                cur_pos=0;
            url_fseek(pb, cur_pos, SEEK_SET);
            return 0;
        }
    }
    gxloge ("max resync size reached, could not find sync byte\n");
    /* no sync found */
    return AVERROR_INVALIDDATA;
}

static int external_mpegts_descramble_callback(uint8_t **data_in, unsigned int dataInLen, uint8_t *dataOut, unsigned int dataOutLen)
{
    extern GxStreamMpegtsCallback public_stream_mpegts_callback;
    if (public_stream_mpegts_callback) {
        GxStreamMpegtsCallbackParam param;
        param.dataIn     = *data_in;
        param.dataInLen  = dataInLen;
        param.dataOut     = dataOut;
        param.dataOutLen = dataOutLen;
        if (public_stream_mpegts_callback(&param) != 0) {
            gxloge("mpegts descraamble error.\n");
            return -1;
        }
        return 0;
    }
    memcpy(dataOut, *data_in, dataOutLen);
    return 0;
}

/* return -1 if error or EOF. Return 0 if OK. */
static int read_packet(AVFormatContext *s, uint8_t *buf, int raw_packet_size, uint8_t *data, int data_size)
{
    ByteIOContext *pb = s->pb;
    uint8_t *tmp_data = NULL;
    int skip, len;

    for(;;) {
        len = ffio_read_indirect(pb, buf, TS_PACKET_SIZE, (unsigned char **)(&tmp_data));
        if (len != TS_PACKET_SIZE) {
            if (len == AVERROR_EOF)
                return AVERROR_EOF;
            return len < 0 ? len : AVERROR(EIO);
        }
        /* check paquet sync byte */
        if (tmp_data[0] != 0x47) {
            /* find a new packet start */
            if (mpegts_resync(s, raw_packet_size, tmp_data) < 0)
                return AVERROR(EAGAIN);
            else
                continue;
        } else {
            break;
        }
    }

    if (external_mpegts_descramble_callback(&tmp_data, TS_PACKET_SIZE, data, data_size) < 0) {
        return AVERROR(EIO);
    }
    return 0;
}

static void finished_reading_packet(AVFormatContext *s, int raw_packet_size)
{
    AVIOContext *pb = s->pb;
    int skip = raw_packet_size - TS_PACKET_SIZE;
    if (skip > 0)
        avio_skip(pb, skip);
}

static int handle_packets(MpegTSContext *ts, int nb_packets)
{
    AVFormatContext *s = ts->stream;
    uint8_t packet[TS_PACKET_SIZE];
    int packet_num, ret;
    uint8_t data[TS_PACKET_SIZE];

    ts->stop_parse = 0;
    packet_num = 0;
    for(;;) {
        if(url_check_interrupt_cb())
            return -AVERROR(EOF);

        if (ts->stop_parse>0)
            break;
        packet_num++;
        if (nb_packets != 0 && packet_num >= nb_packets)
            break;
        ret = read_packet(s, packet, ts->raw_packet_size, data, TS_PACKET_SIZE);
        if (ret != 0)
            return ret;
        ret = handle_packet(ts, data);
        finished_reading_packet(s, ts->raw_packet_size);
        if (ret != 0)
            return ret;
    }
    return 0;
}

static int mpegts_probe(AVProbeData *p)
{
#if 1
    const int size= p->buf_size;
    int score, fec_score, dvhs_score;
    int check_count= size / TS_FEC_PACKET_SIZE;
#define CHECK_COUNT 10

    if (check_count < CHECK_COUNT)
        return -1;

    score     = analyze(p->buf, TS_PACKET_SIZE     *check_count, TS_PACKET_SIZE     , NULL)*CHECK_COUNT/check_count;
    dvhs_score= analyze(p->buf, TS_DVHS_PACKET_SIZE*check_count, TS_DVHS_PACKET_SIZE, NULL)*CHECK_COUNT/check_count;
    fec_score = analyze(p->buf, TS_FEC_PACKET_SIZE *check_count, TS_FEC_PACKET_SIZE , NULL)*CHECK_COUNT/check_count;
//    av_log(NULL, AV_LOG_DEBUG, "score: %d, dvhs_score: %d, fec_score: %d \n", score, dvhs_score, fec_score);

// we need a clear definition for the returned score otherwise things will become messy sooner or later
    if     (score > fec_score && score > dvhs_score && score > 6) return AVPROBE_SCORE_MAX + score     - CHECK_COUNT;
    else if(dvhs_score > score && dvhs_score > fec_score && dvhs_score > 6) return AVPROBE_SCORE_MAX + dvhs_score  - CHECK_COUNT;
    else if(                 fec_score > 6) return AVPROBE_SCORE_MAX + fec_score - CHECK_COUNT;
    else                                    return -1;
#else
    /* only use the extension for safer guess */
    if (av_match_ext(p->filename, "ts"))
        return AVPROBE_SCORE_MAX;
    else
        return 0;
#endif
}

/* return the 90kHz PCR and the extension for the 27MHz PCR. return
   (-1) if not available */
static int parse_pcr(int64_t *ppcr_high, int *ppcr_low,
                     const uint8_t *packet)
{
    int afc, len, flags;
    const uint8_t *p;
    unsigned int v;

    afc = (packet[3] >> 4) & 3;
    if (afc <= 1)
        return -1;
    p = packet + 4;
    len = p[0];
    p++;
    if (len == 0)
        return -1;
    flags = *p++;
    len--;
    if (!(flags & 0x10))
        return -1;
    if (len < 6)
        return -1;
    v = AV_RB32(p);
    *ppcr_high = ((int64_t)v << 1) | (p[4] >> 7);
    *ppcr_low = ((p[4] & 1) << 8) | p[5];
    return 0;
}

static int mpegts_read_header(AVFormatContext *s,
                              AVFormatParameters *ap)
{
#define MPEGTS_PROBE_PACKET_MAX_BUF (5*1024)
    MpegTSContext *ts = s->priv_data;
    ByteIOContext *pb = s->pb;
    uint8_t *buf = NULL;
    int len;
    int64_t pos;
    int ret = 0;
    AVPacket pkt = {0};

    if (!(buf = av_mallocz(MPEGTS_PROBE_PACKET_MAX_BUF+1)))
        goto fail;

    if (ap) {
        ts->mpeg2ts_compute_pcr = ap->mpeg2ts_compute_pcr;
        if(ap->mpeg2ts_raw){
            av_free(buf);
            gxlogd ("use mpegtsraw_demuxer!\n");
            goto fail;
        }
    }

    /* read the first 1024 bytes to get packet size */
    pos = url_ftell(pb);
    len = get_buffer(pb, buf, MPEGTS_PROBE_PACKET_MAX_BUF);
    if (len != MPEGTS_PROBE_PACKET_MAX_BUF) {
        av_free(buf);
        goto fail;
    }
    ts->raw_packet_size = get_packet_size(buf, MPEGTS_PROBE_PACKET_MAX_BUF);
    if (ts->raw_packet_size <= 0) {
        av_free(buf);
        goto fail;
    }
    av_free(buf);
    ts->stream = s;
    ts->auto_guess = 0;

    if (s->iformat == &mpegts_demuxer) {
        /* normal demux */

        /* first do a scaning to get all the services */
        url_fseek(pb, pos, SEEK_SET);

        mpegts_open_section_filter(ts, SDT_PID, sdt_cb, ts, 1);

        mpegts_open_section_filter(ts, PAT_PID, pat_cb, ts, 1);

        ret = handle_packets(ts, s->probesize / ts->raw_packet_size);
        if (ret < 0)
            goto fail;
        /* if could not find service, enable auto_guess */

        ts->auto_guess = 1;

        gxlogf("tuning done\n");

        s->ctx_flags |= AVFMTCTX_NOHEADER;
    } else {
        AVStream *st;
        int pcr_pid, pid, nb_packets, nb_pcrs, ret, pcr_l;
        int64_t pcrs[2], pcr_h;
        int packet_count[2];
        uint8_t packet[TS_PACKET_SIZE];
        uint8_t data[TS_PACKET_SIZE];

        /* only read packets */

        st = av_new_stream(s, 0);
        if (!st)
            goto fail;
        av_set_pts_info(st, 60, 1, 27000000);
        st->codec->codec_type = AVMEDIA_TYPE_DATA;
        st->codec->codec_id = CODEC_ID_MPEG2TS;

        /* we iterate until we find two PCRs to estimate the bitrate */
        pcr_pid = -1;
        nb_pcrs = 0;
        nb_packets = 0;
        for(;;) {
            ret = read_packet(s, packet, ts->raw_packet_size, data, TS_PACKET_SIZE);
            if (ret < 0)
                return ret;
            pid = AV_RB16(data + 1) & 0x1fff;
            if ((pcr_pid == -1 || pcr_pid == pid) &&
                parse_pcr(&pcr_h, &pcr_l, data) == 0) {
                pcr_pid = pid;
                packet_count[nb_pcrs] = nb_packets;
                pcrs[nb_pcrs] = pcr_h * 300 + pcr_l;
                nb_pcrs++;
                if (nb_pcrs >= 2)
                    break;
            }
            nb_packets++;
        }

        /* NOTE1: the bitrate is computed without the FEC */
        /* NOTE2: it is only the bitrate of the start of the stream */
        ts->pcr_incr = (pcrs[1] - pcrs[0]) / (packet_count[1] - packet_count[0]);
        ts->cur_pcr = pcrs[0] - ts->pcr_incr * packet_count[0];
        s->bit_rate = (TS_PACKET_SIZE * 8) * 27e6 / ts->pcr_incr;
        st->codec->bit_rate = s->bit_rate;
        st->start_time = ts->cur_pcr;
#if 0
        av_log(ts->stream, AV_LOG_DEBUG, "start=%0.3f pcr=%0.3f incr=%d\n",
               st->start_time / 1000000.0, pcrs[0] / 27e6, ts->pcr_incr);
#endif
    }

    if(s->file_format != GX_STREAMTYPE_STREAM){
        ts->pkt = &pkt;

        ret = handle_packets(ts, s->probesize / ts->raw_packet_size);
        url_fseek(pb, pos, SEEK_SET);

        if (ret < 0 || ts->pkt->size == 0) {
            av_free_packet(&pkt);
            ts->pkt = NULL;
            goto fail;
        }

        av_free_packet(&pkt);
        ts->pkt = NULL;
    } else {
        url_fseek(pb, pos, SEEK_SET);
    }
    return 0;
 fail:
    return -1;
}

#define MAX_PACKET_READAHEAD ((128 * 1024) / 188)

static int mpegts_raw_read_packet(AVFormatContext *s,
                                  AVPacket *pkt)
{
    MpegTSContext *ts = s->priv_data;
    int ret, i;
    int64_t pcr_h, next_pcr_h, pos;
    int pcr_l, next_pcr_l;
    uint8_t pcr_buf[12];
    uint8_t data[TS_PACKET_SIZE];

    if (av_new_packet(pkt, TS_PACKET_SIZE) < 0)
        return AVERROR(ENOMEM);
    pkt->pos= url_ftell(s->pb);
    ret = read_packet(s, pkt->data, ts->raw_packet_size, data, TS_PACKET_SIZE);
    if (ret < 0) {
        av_free_packet(pkt);
        return ret;
    }
    if (ts->mpeg2ts_compute_pcr) {
        /* compute exact PCR for each packet */
        if (parse_pcr(&pcr_h, &pcr_l, pkt->data) == 0) {
            /* we read the next PCR (XXX: optimize it by using a bigger buffer */
            pos = url_ftell(s->pb);
            for(i = 0; i < MAX_PACKET_READAHEAD; i++) {
                url_fseek(s->pb, pos + i * ts->raw_packet_size, SEEK_SET);
                get_buffer(s->pb, pcr_buf, 12);
                if (parse_pcr(&next_pcr_h, &next_pcr_l, pcr_buf) == 0) {
                    /* XXX: not precise enough */
                    ts->pcr_incr = ((next_pcr_h - pcr_h) * 300 + (next_pcr_l - pcr_l)) /
                        (i + 1);
                    break;
                }
            }
            url_fseek(s->pb, pos, SEEK_SET);
            /* no next PCR found: we use previous increment */
            ts->cur_pcr = pcr_h * 300 + pcr_l;
        }
        pkt->pts = ts->cur_pcr;
        pkt->duration = ts->pcr_incr;
        ts->cur_pcr += ts->pcr_incr;
    }
    pkt->stream_index = 0;
    return 0;
}

static int mpegts_read_packet(AVFormatContext *s,
                              AVPacket *pkt)
{
    MpegTSContext *ts = s->priv_data;
    int ret, i;

    if (url_ftell(s->pb) != ts->last_pos) {
        /* seek detected, flush pes buffer */
        for (i = 0; i < NB_PID_MAX; i++) {
            if (ts->pids[i] && ts->pids[i]->type == MPEGTS_PES) {
                PESContext *pes = ts->pids[i]->u.pes_filter.opaque;
                av_freep(&pes->buffer);
                pes->data_index = 0;
                pes->state = MPEGTS_SKIP; /* skip until pes header */
            }
        }
    }

    ts->pkt = pkt;
    ret = handle_packets(ts, 0);
    if (ret < 0) {
        av_free_packet(ts->pkt);
        /* flush pes data left */
        for (i = 0; i < NB_PID_MAX; i++) {
            if (ts->pids[i] && ts->pids[i]->type == MPEGTS_PES) {
                PESContext *pes = ts->pids[i]->u.pes_filter.opaque;
                if (pes->state == MPEGTS_PAYLOAD && pes->data_index > 0) {
                    new_pes_packet(pes, pkt);
                    pes->state = MPEGTS_SKIP;
                    ret = 0;
                    break;
                }
            }
        }
    }

	pkt->flags |= AVINDEX_KEYFRAME;
    ts->last_pos = url_ftell(s->pb);

    return ret;
}
static void mpegts_free(MpegTSContext *ts)
{
    int i;

    clear_programs(ts);

    for (i = 0; i < NB_PID_MAX; i++)
        if (ts->pids[i])
            mpegts_close_filter(ts, ts->pids[i]);
}

static int mpegts_read_close(AVFormatContext *s)
{
    MpegTSContext *ts = s->priv_data;
    if (ts)
        mpegts_free(ts);
    return 0;
}

static int64_t mpegts_get_pcr(AVFormatContext *s, int stream_index,
                              int64_t *ppos, int64_t pos_limit)
{
    MpegTSContext *ts = s->priv_data;
    int64_t pos, timestamp;
    uint8_t buf[TS_PACKET_SIZE];
    int pcr_l, pcr_pid = ((PESContext*)s->streams[stream_index]->priv_data)->pcr_pid;
    const int find_next= 1;
    pos = ((*ppos  + ts->raw_packet_size - 1 - ts->pos47) / ts->raw_packet_size) * ts->raw_packet_size + ts->pos47;
    while(pos < pos_limit) {
        if (avio_seek(s->pb, pos, SEEK_SET) < 0)
            return AV_NOPTS_VALUE;
        if (avio_read(s->pb, buf, TS_PACKET_SIZE) != TS_PACKET_SIZE)
            return AV_NOPTS_VALUE;
        if (buf[0] != 0x47) {
            if (mpegts_resync(s, TS_PACKET_SIZE, buf) < 0)
            return AV_NOPTS_VALUE;
                pos = avio_tell(s->pb);
                continue;
        }
        if ((pcr_pid < 0 || (AV_RB16(buf + 1) & 0x1fff) == pcr_pid) && parse_pcr(&timestamp, &pcr_l, buf) == 0) {
            *ppos = pos;
            return timestamp;
        }
        pos += ts->raw_packet_size;
    }
    return timestamp;
}

#ifdef USE_SYNCPOINT_SEARCH

static int read_seek2(AVFormatContext *s,
                      int stream_index,
                      int64_t min_ts,
                      int64_t target_ts,
                      int64_t max_ts,
                      int flags)
{
    int64_t pos;

    int64_t ts_ret, ts_adj;
    int stream_index_gen_search;
    AVStream *st;
    AVParserState *backup;

    backup = ff_store_parser_state(s);

    // detect direction of seeking for search purposes
    flags |= (target_ts - min_ts > (uint64_t)(max_ts - target_ts)) ?
             AVSEEK_FLAG_BACKWARD : 0;

    if (flags & AVSEEK_FLAG_BYTE) {
        // use position directly, we will search starting from it
        pos = target_ts;
    } else {
        // search for some position with good timestamp match
        if (stream_index < 0) {
            stream_index_gen_search = av_find_default_stream_index(s);
            if (stream_index_gen_search < 0) {
                ff_restore_parser_state(s, backup);
                return -1;
            }

            st = s->streams[stream_index_gen_search];
            // timestamp for default must be expressed in AV_TIME_BASE units
            ts_adj = av_rescale(target_ts,
                                st->time_base.den,
                                AV_TIME_BASE * (int64_t)st->time_base.num);
        } else {
            ts_adj = target_ts;
            stream_index_gen_search = stream_index;
        }
        pos = av_gen_search(s, stream_index_gen_search, ts_adj,
                            0, INT64_MAX, -1,
                            AV_NOPTS_VALUE,
                            AV_NOPTS_VALUE,
                            flags, &ts_ret, mpegts_get_pcr);
        if (pos < 0) {
            ff_restore_parser_state(s, backup);
            return -1;
        }
    }

    // search for actual matching keyframe/starting position for all streams
    if (ff_gen_syncpoint_search(s, stream_index, pos,
                                min_ts, target_ts, max_ts,
                                flags) < 0) {
        ff_restore_parser_state(s, backup);
        return -1;
    }

    ff_free_parser_state(s, backup);
    return 0;
}

static int read_seek(AVFormatContext *s, int stream_index, int64_t target_ts, int flags)
{
    int ret;

	gxlogd("start_time %lld, duration %lld, lastseekpos %lld, lastseekpts %lld\n",
			s->start_time,s->duration,s->lastseekpos,s->lastseekpts);

    ret = read_seek2(s, stream_index, INT64_MIN, target_ts, INT64_MAX, flags);
	gxlogd("mpegts seek complete\n");
    return ret;
#if 0
    if (flags & AVSEEK_FLAG_BACKWARD) {
        flags &= ~AVSEEK_FLAG_BACKWARD;
        ret = read_seek2(s, stream_index, INT64_MIN, target_ts, target_ts, flags);
        if (ret < 0)
            // for compatibility reasons, seek to the best-fitting timestamp
            ret = read_seek2(s, stream_index, INT64_MIN, target_ts, INT64_MAX, flags);
    } else {
        ret = read_seek2(s, stream_index, target_ts, target_ts, INT64_MAX, flags);
        if (ret < 0)
            // for compatibility reasons, seek to the best-fitting timestamp
            ret = read_seek2(s, stream_index, INT64_MIN, target_ts, INT64_MAX, flags);
    }
    return ret;
#endif
}

#else

static int read_seek(AVFormatContext *s, int stream_index, int64_t target_ts, int flags){
    MpegTSContext *ts = s->priv_data;
    uint8_t buf[TS_PACKET_SIZE];
    int64_t pos;

    if(av_seek_frame_binary(s, stream_index, target_ts, flags) < 0)
        return -1;

    pos= url_ftell(s->pb);

    for(;;) {
        url_fseek(s->pb, pos, SEEK_SET);
        if (get_buffer(s->pb, buf, TS_PACKET_SIZE) != TS_PACKET_SIZE)
            return -1;
//        pid = AV_RB16(buf + 1) & 0x1fff;
        if(buf[1] & 0x40) break;
        pos += ts->raw_packet_size;
    }
    url_fseek(s->pb, pos, SEEK_SET);

    return 0;
}

#endif

static int mpegts_read_seek(AVFormatContext* s, int stream_index, int64_t timestamp, int flags)
{
#define PS_READ_SEEK_TIMEOUT_SEC (3)
	int64_t npos, pos, pts=-1, start_time = s->start_time/AV_TIME_BASE;
	AVPacket pkt;
	int i = 0, count=0, ret = 0;
	AVStream *st = NULL;
	GxTime time = {0};
	int is_audio_data = 1, media_max_streams = 0;
	enum AVDiscard *discard_bk;

	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);
	if (!(discard_bk = av_mallocz(sizeof(enum AVDiscard)*media_max_streams))) {
		return AVERROR(ENOMEM);
	}

	/* if dst timestamp equl start_time, seek SEEK_SET directly */
	if (stream_index >= 0 && stream_index < s->nb_streams) {
		st = s->streams[stream_index];
		if (timestamp == av_rescale(s->start_time, st->time_base.den, AV_TIME_BASE * (int64_t)st->time_base.num)) {
			avio_seek(s->pb, 0, SEEK_SET);
			av_free(discard_bk);
			return 0;
		}
	}

	/* now find stream */
	for(i=0;i<s->nb_streams;i++) {
		st = s->streams[i];
		discard_bk[i] = st->discard;
		st->discard	  = AVDISCARD_DEFAULT;
		if(st->discard != AVDISCARD_ALL && st->codec && st->codec->codec_type == CODEC_TYPE_AUDIO)
			break;
	}
	if(i == s->nb_streams)
		i = 0;

	/*get probe pos*/
	npos = avio_tell(s->pb);
	pos = (s->target_time - start_time)*s->bit_rate/8;

	{/*probe current audio stream_index.check is audio data*/
		st = s->streams[i];
		if (st->codec->codec_type == CODEC_TYPE_AUDIO && st->first_dts == AV_NOPTS_VALUE) {
			is_audio_data = 0;
		}
		if (!is_audio_data) {/*3s check audio data.*/
			GxCore_GetTickTime(&time);
			for(;;) {
				GxTime time_tmp = {0};
				GxCore_GetTickTime(&time_tmp);
				if(time_tmp.seconds-time.seconds>PS_READ_SEEK_TIMEOUT_SEC){
					gxloge ("mpegts.c probe audio data 3S timeout.\n");
					break;
				}
				if(av_read_frame(s, &pkt) < 0) {
					avio_seek(s->pb, pos, SEEK_SET);
					goto RET;
				}
				if (pkt.stream_index==i) {
					is_audio_data = 1;
					av_free_packet(&pkt);
					break;
				}
				av_free_packet(&pkt);
			}
		}
	}
	GxCore_GetTickTime(&time);
	/*get nearest pts*/
	pkt.pts	= 0;
	do {
		GxTime time_tmp = {0};
		GxCore_GetTickTime(&time_tmp);
		if(time_tmp.seconds-time.seconds>PS_READ_SEEK_TIMEOUT_SEC){
			gxlogf("av_read_frame time out !\n");
			avio_seek(s->pb, pos, SEEK_SET);
			goto RET;
		}
		pkt.pts = -1;
		url_fseek(s->pb, pos, SEEK_SET);
		while(pkt.pts == -1 || pkt.pts == pts || pkt.pos == -1 || ((pkt.stream_index != i) && is_audio_data)) {
			if((ret = av_read_frame(s, &pkt)) < 0){
				gxlogf("av_read_frame failed!.ret=%d.\n", ret);
				avio_seek(s->pb, pos, SEEK_SET);
				goto RET;
			}
			av_free_packet(&pkt);
		}
		pts = pkt.pts;
		if ((pkt.pts/90000.0 -start_time) > 2)
			pos = (s->target_time -start_time) * pkt.pos /(pkt.pts/90000.0 -start_time);
		else
			pos = pkt.pos;
		gxlogf("timestamp = %ld, pkt->pts = %ld,pkt->pts0=%lld\n", (long)s->target_time, (long)(pkt.pts/90000.0),pkt.pts);

		if(pts/90000.0 > s->target_time)
			count |= 1;
		else
			count |= 2;
		if(count == 3)
			break;
	} while(abs(pts/90000.0 - s->target_time)>2);

RET:
	for(i=0;i<s->nb_streams;i++) {
		st = s->streams[i];
		st->discard = discard_bk[i];
	}
	if (discard_bk)
		av_free(discard_bk);
	return 0;
}

void avpriv_mpegts_parse_close(MpegTSContext *ts)
{
    if (ts) {
        mpegts_free(ts);
        av_free(ts);
    }
}


AVInputFormat mpegts_demuxer = {
	.name = "mpegts",
	.priv_data_size = sizeof(MpegTSContext),
	.read_probe = mpegts_probe,
	.read_header = mpegts_read_header,
	.read_packet = mpegts_read_packet,
	.read_close = mpegts_read_close,
	.read_seek = mpegts_read_seek,
	.read_control = NULL,
	.read_timestamp = mpegts_get_pcr,
	.flags = AVFMT_SHOW_IDS|AVFMT_TS_DISCONT,
#ifdef USE_SYNCPOINT_SEARCH
	.read_seek2 = read_seek2,
#endif
};

AVInputFormat mpegtsraw_demuxer = {
    .name = "mpegtsraw",
    .priv_data_size = sizeof(MpegTSContext),
    .read_probe = NULL,
    .read_header = mpegts_read_header,
    .read_packet = mpegts_raw_read_packet,
    .read_close = mpegts_read_close,
    .read_seek = read_seek,
    .read_control = NULL,
    .read_timestamp = mpegts_get_pcr,
    .flags = AVFMT_SHOW_IDS|AVFMT_TS_DISCONT,
#ifdef USE_SYNCPOINT_SEARCH
    .read_seek2 = read_seek2,
#endif
};
