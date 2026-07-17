#include "avformat.h"
#include "gx_stream.h"
#include "avio.h"
#include "string.h"

#define ASSOCIATIONLIBRE_SEPARATOR "|"

typedef struct StreamsContext {
    int stream_index;
    int64_t cur_timestamp;
    char *url;
    AVFormatContext *ctx;
} StreamsContext;

typedef struct AssociationLibreContext {
    int n_playlists;
    StreamsContext **playlists;
} AssociationLibreContext;

static int associationlibre_probe(AVProbeData *p)
{
    if (av_strstart(p->filename, "associationlibre:", NULL)) {
        return AVPROBE_SCORE_MAX;
    }
    return 0;
}

static int parse_url_playlist(AVFormatContext *s, AssociationLibreContext *c)
{
    int i = 0, len = 0, ret = 0;
    char *next_url = NULL, *cur_url = NULL;
    if (!s || !s->url) {
        gxloge ("invalid url!\n");
        return AVERROR(EINVAL);
    }
    next_url = s->url;
    /*delete avcompose: header.*/
    if (!av_strstart(next_url, "associationlibre:", (const char **)&next_url)) {
        gxloge ("invalid url!\n");
        ret = AVERROR(EINVAL);
        goto fail;
    }
    next_url = s->url;
    while ((cur_url = av_strtok(next_url, "|", &next_url)))  {
        char *av_dur = NULL;
        cur_url += strspn(cur_url, "associationlibre:");
        if ((av_dur = av_stristr(cur_url, "av_duration="))) {
            int data1 = 0, data2 = 0, data3 = 0;
            av_dur += strlen("av_duration=");
            ret = sscanf(av_dur, "%02d:%02d:%2d", &data1, &data2, &data3);
        } else {
            StreamsContext *tmp_program = NULL;
            if (!(tmp_program = av_mallocz(sizeof(struct StreamsContext)))) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
            if (!(tmp_program->url = av_strdup(cur_url))) {
                ret = AVERROR(ENOMEM);
                if (tmp_program)
                    av_freep(&tmp_program);
                goto fail;
            }
            dynarray_add(&c->playlists, &c->n_playlists, tmp_program);
            if (c->n_playlists > 2) {
                break;
            }
        }
    }
fail:
    return ret;
}

static int open_demux_for_component(AVFormatContext *s, StreamsContext *c)
{
    int ret = 0, index_limit =0;
    if (!(c->ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    GxPlayer_SystemGet(PSYS_INDEX_CACHE, &index_limit);
    c->ctx->flags = AVIO_FLAG_READ;
    c->ctx->nobuffer = 0;
    c->ctx->file_format  = GX_STREAMTYPE_STREAM;
    c->ctx->index_limit  = index_limit;
    c->ctx->seek_by_time = s->seek_by_time;
    if ((ret = avformat_open_input(&(c->ctx), c->url, NULL, &s->url_options)) < 0) {
        gxloge ("open input fail. ret=%d(%s).\n", ret, av_fourcc2str(abs(ret)));
        goto fail;
    }
    if ((ret = avformat_find_stream_info(c->ctx, &s->url_options)) < 0) {
        gxloge ("find stream info fail. ret=%d(%s).\n", ret, av_fourcc2str(abs(ret)));
        goto fail;
    }
fail:
    return ret;
}

static int associationlibre_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
#define AV_INPUT_BUFFER_PADDING_SIZE 64
    AssociationLibreContext *c = s->priv_data;
    int ret = 0, i = 0, j = 0;
    AVDictionaryEntry *e = NULL;

    if ((ret = parse_url_playlist(s, c)) < 0) {
        return ret;
    }
    for (i = 0; i < c->n_playlists; i++) {
        StreamsContext *rep = c->playlists[i];
        if (!rep)
            continue;
        if ((ret = open_demux_for_component(s, rep))<0) {
            continue;
        }
        if (!(rep->ctx))
            continue;
        rep->stream_index = i;
        s->start_time = rep->ctx->start_time/1000;
        for (j = 0; j < rep->ctx->nb_streams; j++) {
            AVStream *ist = rep->ctx->streams[j];
            if ((e = av_dict_get(s->url_options, "ff_an_streams", NULL, 0))) {
                if ((atoi(e->value)-1) == i) {
                    if (CODEC_TYPE_AUDIO == ist->codec->codec_type) {
                        ist->discard = AVDISCARD_ALL;
                        continue;
                    }
                }
            }
            if ((e = av_dict_get(s->url_options, "ff_vn_streams", NULL, 0))) {
                if ((atoi(e->value)-1) == i) {
                    if (CODEC_TYPE_VIDEO == ist->codec->codec_type) {
                        ist->discard = AVDISCARD_ALL;
                        continue;
                    }
                }
            }
            AVStream *st = avformat_new_stream(s, NULL);
            if (!st) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
            st->id = j;
            st->codec->codec_type = ist->codec->codec_type;
            st->codec->codec_id = ist->codec->codec_id;
            st->codec->bit_rate = ist->codec->bit_rate;
            st->codec->sample_rate = ist->codec->sample_rate;
            st->duration = ist->duration;
            if (ist->codec->extradata) {
                if (!st->codec->extradata) {
                    st->codec->extradata = av_mallocz(ist->codec->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
                    if (!st->codec->extradata) {
                        gxloge ("malloc fail..size=%d.\n", ist->codec->extradata_size);
                        return AVERROR(ENOMEM);
                    }
                }
                memcpy(st->codec->extradata, ist->codec->extradata, ist->codec->extradata_size);
                st->codec->extradata_size = ist->codec->extradata_size;
            }
            avpriv_set_pts_info(st, ist->pts_wrap_bits, ist->time_base.num, ist->time_base.den);
        }
    }
fail:
    return ret;
}

static int associationlibre_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    AssociationLibreContext *c = s->priv_data;
    StreamsContext *cur = NULL;
    StreamsContext *rep = NULL;
    int i = 0, j = 0, ret = 0, get_packet_bypts = 0;
    int64_t mints= 0;

    get_packet_bypts = s->get_packet_bypts;
    if (!c)
        return AVERROR(EINVAL);
    for (i = 0; i < c->n_playlists; i++) {
        rep = c->playlists[i];
        if (!rep || !rep->ctx)
            continue;
        if (!cur || rep->cur_timestamp < mints) {
            cur = rep;
            mints = rep->cur_timestamp;
        }
    }
    if (!cur) {
        return AVERROR(EINVAL);
    }
    cur->ctx->get_packet_bypts = get_packet_bypts;
    while (!ret) {
       if (url_check_interrupt_cb())
           return AVERROR_EXIT;
        ret = av_read_frame(cur->ctx, pkt);
        if (ret >= 0) {
            /* If we got a packet, return it */
            AVStream *ist = cur->ctx->streams[pkt->stream_index];
            if (AVDISCARD_ALL == ist->discard) {
                av_free_packet(pkt);
                continue;
            }
            cur->cur_timestamp = av_rescale(pkt->pts, (int64_t)cur->ctx->streams[0]->time_base.num * 90000, cur->ctx->streams[0]->time_base.den);
            pkt->stream_index = cur->stream_index;
            return ret;
        }
        return ret;
    }
    return ret;
}

static int associationlibre_seek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)
{
    int ret = AVERROR(ENOSYS), i = 0, seek_idx = -1;
    AVStream *st = NULL;
    AssociationLibreContext *c = s->priv_data;
    if (!c)
        return ret;
    seek_idx = av_find_default_stream_index(s);
    if (seek_idx < 0 || seek_idx >= s->nb_streams)
        return ret;
    st = s->streams[seek_idx];
    if (!st)
        return ret;
    if (st->time_base.den > 0) {
        timestamp = timestamp*AV_TIME_BASE * st->time_base.num/((int64_t)st->time_base.den);
    }else {
        return ret;
    }
    for (i = 0; i < c->n_playlists; i++) {
        StreamsContext *cur = c->playlists[i];
        if (!cur)
            continue;
        cur->ctx->seek_tolerance_ms = 10000;
        cur->ctx->ass_is_forward = 1;
        if((ret = av_seek_frame(cur->ctx, -1, timestamp, (AVSEEK_FLAG_TOLERANCE| AVSEEK_FLAG_BACKWARD))) < 0) {
            gxlogi ("aassociationlibre.c seek frame fail.\n");
            ret = -1;
            break;
        }
        cur->cur_timestamp = 0;
        cur->ctx->ass_is_forward = 0;
    }
    return ret;
}

static int associationlibre_close(AVFormatContext *s)
{
    AssociationLibreContext *c = s->priv_data;
    int i = 0;
    if (c) {
        for (i = 0; i < c->n_playlists; i++) {
            StreamsContext *pls = c->playlists[i];
            if (!pls)
                continue;
            if (pls && pls->ctx) {
                avformat_close_input(&pls->ctx);
            }
            if (pls->url) {
                av_free(pls->url);
                pls->url = NULL;
            }
            if (pls) {
                av_free(pls);
                pls = NULL;
            }
        }
    }
    if (c->playlists) {
        av_freep(&c->playlists);
    }
    return 0;
}

AVInputFormat associationlibre_demuxer = {
    .name           = "associationlibre",
    .priv_data_size = sizeof(AssociationLibreContext),
    .read_probe     = associationlibre_probe,
    .read_header    = associationlibre_read_header,
    .read_packet    = associationlibre_read_packet,
    .read_close     = associationlibre_close,
    .read_seek      = associationlibre_seek,
    .flags          = AVFMT_NOFILE,
};

