/*
 *  Various utilities for ffmpeg system
 *  Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 *
 *  This file is part of FFmpeg.
 *
 *  FFmpeg is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  FFmpeg is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with FFmpeg; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "gx_common.h"

#include "riff.h"
#include "../avutil/dict.h"
#include "avformat.h"
#include "raw.h"
#include "bytestream.h"
#include "isom_utils.h"
#include "parser.h"

#include "../demuxer/bluray_pcm.h"

#ifndef LINUX_OS
#include <netdb.h>
#endif
#include "id3v2.h"

//#define ASSERT(exp) assert(exp)
#define ASSERT(exp) do {    \
	if (!(exp))             \
		gxloge("assert\n"); \
} while(0)


#define RELATIVE_TS_BASE (INT64_MAX - (1LL<<48))
#define AV_TS_MAX_STRING_SIZE 32

static inline char *av_ts_make_string(char *buf, int64_t ts)
{
	if (ts == AV_NOPTS_VALUE) snprintf(buf, AV_TS_MAX_STRING_SIZE, "NOPTS");
	else                      snprintf(buf, AV_TS_MAX_STRING_SIZE, "%lld", ts);
	return buf;
}

#define av_ts2str(ts) av_ts_make_string((char[AV_TS_MAX_STRING_SIZE]){0}, ts)

static int is_relative(int64_t ts) {
	return ts > (RELATIVE_TS_BASE - (1LL<<48));
}

/* fraction handling */

/**
 * f = val + (num / den) + 0.5.
 *
 * 'num' is normalized so that it is such as 0 <= num < den.
 *
 * @param f fractional number
 * @param val integer value
 * @param num must be >= 0
 * @param den must be >= 1
 */
#if 0
static void frac_init(AVFrac *f, int64_t val, int64_t num, int64_t den)
{
	num += (den >> 1);
	if (num >= den) {
		val += num / den;
		num = num % den;
	}
	f->val = val;
	f->num = num;
	f->den = den;
}
#endif

/**
 * Fractional addition to f: f = f + (incr / f->den).
 *
 * @param f fractional number
 * @param incr increment, can be positive or negative
 */
static void frac_add(AVFrac *f, int64_t incr)
{
	int64_t num, den;

	num = f->num + incr;
	den = f->den;
	if (num < 0) {
		f->val += num / den;
		num = num % den;
		if (num < 0) {
			num += den;
			f->val--;
		}
	} else if (num >= den) {
		f->val += num / den;
		num = num % den;
	}
	f->num = num;
}

/** head of registered input format linked list.*/
AVInputFormat* first_iformat = NULL;
/** head of registered output format linked list.*/
AVOutputFormat* first_oformat = NULL;

AVInputFormat  *av_iformat_next(AVInputFormat  *f)
{
	if(f) return f->next;
	else  return first_iformat;
}

AVOutputFormat *av_oformat_next(AVOutputFormat *f)
{
	if(f) return f->next;
	else  return first_oformat;
}


void av_register_input_format(AVInputFormat*  format)
{
	AVInputFormat* *p;
	p = &first_iformat;
	while (*p != NULL)
		p = &(*p)->next;
	*p = format;
	format->next = NULL;
}

void av_register_output_format(AVOutputFormat*  format)
{
	AVOutputFormat* *p;
	p = &first_oformat;
	while (*p != NULL)
		p = &(*p)->next;
	*p = format;
	format->next = NULL;
}

int av_match_ext(const char* filename, const char *extensions)
{
	const char* ext, *p;
	char ext1[32],* q;

	if (!filename)
		return 0;

	ext = strrchr(filename, '.');
	if (ext) {
		ext++;
		p = extensions;
		for (;;) {
			q = ext1;
			while (*p != '\0' &&* p != ',' && q - ext1 < sizeof(ext1) - 1)
				*q++ =* p++;
			*q = '\0';
			if (!strcasecmp(ext1, ext))
				return 1;
			if (*p == '\0')
				break;
			p++;
		}
	}
	return 0;
}

static int match_format(const char *name, const char *names)
{
	const char *p;
	int len, namelen;

	if (!name || !names)
		return 0;

	namelen = strlen(name);
	while ((p = strchr(names, ','))) {
		len = FFMAX(p - names, namelen);
		if (!av_strncasecmp(name, names, len))
			return 1;
		names = p+1;
	}
	return !av_strcasecmp(name, names);
}

AVOutputFormat *av_guess_format(const char *short_name, const char *filename,
		const char *mime_type)
{
	AVOutputFormat *fmt = NULL, *fmt_found;
	int score_max, score;

	/* specific test for image sequences */
#if 0 //CONFIG_IMAGE2_MUXER
	if (!short_name && filename &&
			av_filename_number_test(filename) &&
			ff_guess_image2_codec(filename) != CODEC_ID_NONE) {
		return av_guess_format("image2", NULL, NULL);
	}
#endif
	/* Find the proper file type. */
	fmt_found = NULL;
	score_max = 0;
	while ((fmt = av_oformat_next(fmt))) {
		score = 0;
		if (fmt->name && short_name && !av_strcasecmp(fmt->name, short_name))
			score += 100;
		if (fmt->mime_type && mime_type && !strcmp(fmt->mime_type, mime_type))
			score += 10;
		if (filename && fmt->extensions &&
				av_match_ext(filename, fmt->extensions)) {
			score += 5;
		}
		if (score > score_max) {
			score_max = score;
			fmt_found = fmt;
		}
	}
	return fmt_found;
}

enum CodecID av_guess_codec(AVOutputFormat *fmt, const char *short_name,
		const char *filename, const char *mime_type, enum CodecType type){
	if(type == CODEC_TYPE_VIDEO){
		enum CodecID codec_id= CODEC_ID_NONE;

#if 0 //CONFIG_IMAGE2_MUXER
		if(!strcmp(fmt->name, "image2") || !strcmp(fmt->name, "image2pipe")){
			codec_id= ff_guess_image2_codec(filename);
		}
#endif
		if(codec_id == CODEC_ID_NONE)
			codec_id= fmt->video_codec;
		return codec_id;
	}else if(type == CODEC_TYPE_AUDIO)
		return fmt->audio_codec;
	else if (type == CODEC_TYPE_SUBTITLE)
		return fmt->subtitle_codec;
	else
		return CODEC_ID_NONE;
}

AVInputFormat* av_find_input_format(const char *short_name)
{
	AVInputFormat *fmt = NULL;
	while ((fmt = av_iformat_next(fmt))) {
		if (match_format(short_name, fmt->name))
			return fmt;
	}
	return NULL;
}

int ffio_limit(ByteIOContext *s, int size)
{
#if 0
	if(s->maxsize>=0){
		int64_t remaining= s->maxsize - avio_tell(s);
		if(remaining < size){
			int64_t newsize= avio_size(s);
			if(!s->maxsize || s->maxsize<newsize)
				s->maxsize= newsize - !newsize;
			remaining= s->maxsize - avio_tell(s);
			remaining= FFMAX(remaining, 0);
		}

		if(s->maxsize>=0 && remaining+1 < size){
			av_log(0, AV_LOG_ERROR, "Truncating packet of size %d to %lld\n", size, remaining+1);
			size= remaining+1;
		}
	}
#endif
	return size;
}

int av_get_packet(ByteIOContext*  s, AVPacket * pkt, int size)
{
	int ret;
	int orig_size = size;
	size= ffio_limit(s, size);

	ret= av_new_packet(pkt, size);

	if(ret<0)
		return ret;

	pkt->pos= avio_tell(s);

	ret= avio_read(s, pkt->data, size);
	if(ret<=0) {
		av_free_packet(pkt);
		return ret;
	}
	else {
		av_shrink_packet(pkt, ret);
	}
	if (pkt->size < orig_size)
		pkt->flags |= AV_PKT_FLAG_CORRUPT;

	return ret;
}

int av_append_packet(ByteIOContext *s, AVPacket *pkt, int size)
{
	int ret;
	int old_size;
	if (!pkt->size)
		return av_get_packet(s, pkt, size);
	old_size = pkt->size;
	ret = av_grow_packet(pkt, size);
	if (ret < 0)
		return ret;
	ret = avio_read(s, pkt->data + old_size, size);
	av_shrink_packet(pkt, old_size + FFMAX(ret, 0));
	return ret;
}

int av_filename_number_test(const char* filename)
{
	char buf[1024];
	return filename && (av_get_frame_filename(buf, sizeof(buf), filename, 1)>=0);
}

AVInputFormat *av_probe_input_format3(AVProbeData *pd, int is_opened, int *score_ret)
{
	AVProbeData lpd = *pd;
	AVInputFormat *fmt1 = NULL, *fmt = NULL;
	int score = 0, nodat = 0, score_max=1;

	while ((fmt1 = av_iformat_next(fmt1))) {
		if (!is_opened == !(fmt1->flags & AVFMT_NOFILE)){
			continue;
		}
		score = 0;
		if (fmt1->read_probe) {
			score = fmt1->read_probe(&lpd);
			if(fmt1->extensions && av_match_ext(lpd.filename, fmt1->extensions))
				score = FFMAX(score, nodat ? AVPROBE_SCORE_MAX/4-1 : 1);
		} else if (fmt1->extensions) {
			if (av_match_ext(lpd.filename, fmt1->extensions)) {
				score = 50;
			}
		}
		if (score > score_max) {
			score_max = score;
			fmt = fmt1;
			if (score_max == AVPROBE_SCORE_MAX)
				break;
		}else if (score == score_max)
			fmt = NULL;
	}
	*score_ret= score_max;

	return fmt;
}

AVInputFormat *av_probe_input_format2(AVProbeData *pd, int is_opened, int *score_max)
{
	int score_ret;
	AVInputFormat *fmt= av_probe_input_format3(pd, is_opened, &score_ret);
	if(score_ret > *score_max){
		*score_max= score_ret;
		return fmt;
	}else
		return NULL;
}

AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened){
	int score=0;
	AVInputFormat *fmt;
	fmt = av_probe_input_format2(pd, is_opened, &score);
	return fmt;
}

static int set_codec_from_probe_data(AVFormatContext *s, AVStream *st, AVProbeData *pd)
{
	extern int mpegvideo_probe(AVProbeData *p);
	static const struct {
		const char *name; enum CodecID id; enum CodecType type;
	} fmt_id_type[] = {
		{ "aac",	   CODEC_ID_AAC, 	   CODEC_TYPE_AUDIO },
		{ "ac3",	   CODEC_ID_AC3, 	   CODEC_TYPE_AUDIO },
		{ "dts",	   CODEC_ID_DTS, 	   CODEC_TYPE_AUDIO },
		{ "dvbsub",    CODEC_ID_DVB_SUBTITLE,CODEC_TYPE_SUBTITLE },
		{ "dvbtxt",    CODEC_ID_DVB_TELETEXT,CODEC_TYPE_SUBTITLE },
		{ "eac3",	   CODEC_ID_EAC3,	   CODEC_TYPE_AUDIO },
		{ "h264",	   CODEC_ID_H264,	   CODEC_TYPE_VIDEO },
		{ "hevc",	   CODEC_ID_HEVC,	   CODEC_TYPE_VIDEO },
		{ "loas",	   CODEC_ID_AAC_LATM,  CODEC_TYPE_AUDIO },
		{ "m4v",	   CODEC_ID_MPEG4,	   CODEC_TYPE_VIDEO },
		{ "mjpeg_2000",CODEC_ID_JPEG2000,  CODEC_TYPE_VIDEO },
		{ "mp3",	   CODEC_ID_MP3, 	   CODEC_TYPE_AUDIO },
		{ "mpegvideo", CODEC_ID_MPEG2VIDEO,CODEC_TYPE_VIDEO },
		{ 0 }
	};
	int score=0;
	int i;
	AVInputFormat *fmt = av_probe_input_format3(pd, 1, &score);

	if (fmt && st->request_probe<=score) {
		gxlogd ("Probe with size=%d, packets=%d detected %s with score=%d\n",
				pd->buf_size, MAX_PROBE_PACKETS - st->probe_packets, fmt->name, score);
		for (i = 0; fmt_id_type[i].name; i++) {
			if (!strcmp(fmt->name, fmt_id_type[i].name)) {
				st->codec->codec_id   = fmt_id_type[i].id;
				st->codec->codec_type = fmt_id_type[i].type;
				break;
			}
		}
	}else{
		score=mpegvideo_probe(pd);
		if(score>0){
			for (i = 0; fmt_id_type[i].name; i++) {
				if (!strcmp("mpegvideo", fmt_id_type[i].name)) {
					st->codec->codec_id   = fmt_id_type[i].id;
					st->codec->codec_type = fmt_id_type[i].type;
					break;
				}
			}
		}
	}
	return score;
}

/************************************************************/
/* input media file */

int av_demuxer_open(AVFormatContext *ic){
	int err;

	if (ic->iformat->read_header) {
		err = ic->iformat->read_header(ic, NULL);
		if (err < 0)
			return err;
	}

	if (ic->pb && !ic->data_offset)
		ic->data_offset = avio_tell(ic->pb);

	return 0;
}

/** size of probe buffer, for guessing file type from file contents */

int av_probe_input_buffer(ByteIOContext *pb, AVInputFormat **fmt,
		const char *filename, void *logctx,
		unsigned int offset, unsigned int max_probe_size)
{
	AVProbeData pd = { filename ? filename : "", NULL, -offset };
	unsigned char *buf = NULL;
	int ret = 0, probe_size = PROBE_BUF_MIN, buf_offset = 0, score = 0, ret2 = 0;

	if (!max_probe_size) {
		max_probe_size = PROBE_BUF_MAX;
	} else if (max_probe_size > PROBE_BUF_MAX) {
		max_probe_size = PROBE_BUF_MAX;
	} else if (max_probe_size < PROBE_BUF_MIN) {
		return AVERROR(EINVAL);
	}

	if (offset >= max_probe_size) {
		return AVERROR(EINVAL);
	}

	for(probe_size= PROBE_BUF_MIN; probe_size<=max_probe_size && !*fmt;
			probe_size = FFMIN(probe_size<<1, FFMAX(max_probe_size, probe_size+1))) {
		score = probe_size < max_probe_size ? AVPROBE_SCORE_MAX/4:0;
		/* read probe data */
		if ((ret = av_reallocp(&buf, probe_size + AVPROBE_PADDING_SIZE)) < 0) {
			av_free(buf);
			return AVERROR(ENOMEM);
		}

		if ((ret = avio_read(pb, buf + buf_offset, (probe_size - buf_offset))) < 0) {
			/* fail if error was not end of file, otherwise, lower score */
			if (ret != AVERROR_EOF) {
				if (ret != AVERROR(EAGAIN)) {
					av_free(buf);
					return ret;
				}
			}
			score = 0;
			ret = 0;            /* error was end of file, nothing read */
		}


		buf_offset += ret;
		if (buf_offset < offset)
			continue;

		pd.buf_size = buf_offset - offset;
		pd.buf = &buf[offset];

		memset(pd.buf + pd.buf_size, 0, AVPROBE_PADDING_SIZE);

		/* guess file format */
		*fmt = av_probe_input_format2(&pd, 1, &score);
	}

	if (!*fmt) {
		gxloge ("invalid date. probe fail..\n");
		av_free(buf);
		return AVERROR(EINVAL);
	}

	/* rewind. reuse probe buffer to avoid seeking */
	if ((ret = url_rewind_with_probe_data(pb, buf, pd.buf_size)) < 0)
		av_free(buf);

	return ret;
}

static int init_input(AVFormatContext *s, const char *filename, AVDictionary **options)
{
	int ret = 0;
	AVProbeData pd = {filename, NULL, 0};

	if (s->pb) {
		s->flags |= AVFMT_FLAG_CUSTOM_IO;
		if (!s->iformat)
			return av_probe_input_buffer(s->pb, &s->iformat, filename, s, 0, s->probesize);
		else if (s->iformat->flags & AVFMT_NOFILE)
			gxlogd ("Custom ByteIOContext makes no sense and will be ignored with AVFMT_NOFILE format.\n");
		return ret;
	}

	if ((s->iformat && (s->iformat->flags&AVFMT_NOFILE)) || (!s->iformat && (s->iformat = av_probe_input_format(&pd, 0)))) {
		if (*options)
			av_dict_copy(&(s->url_options), *options, 0);
		return ret;
	}

	if ((ret = url_fopen(&(s->pb), filename, ((s->flags==AVIO_FLAG_WRITE)?AVIO_FLAG_READ:s->flags) | s->avio_flags, options)) < 0) {
		return ret;
	}

	if (s->iformat)
		return ret;

	return av_probe_input_buffer(s->pb, &s->iformat, filename, s, 0, s->probesize);
}

static AVPacket *add_to_pktbuf(AVPacketList **packet_buffer, AVPacket *pkt,
		AVPacketList **plast_pktl){
	AVPacketList *pktl = av_mallocz(sizeof(AVPacketList));
	if (!pktl)
		return NULL;

	if (*plast_pktl)
		pktl->seq = (*plast_pktl)->seq + 1;
	else
		pktl->seq = 0;

	if (*packet_buffer)
		(*plast_pktl)->next = pktl;
	else
		*packet_buffer = pktl;

	/* add the packet in the buffered packet list */
	*plast_pktl = pktl;
	pktl->pkt= *pkt;
	return &pktl->pkt;
}

static void limit_to_pktbuf(AVPacketList **phead_pktl, AVPacketList **plast_pktl)
{
	if (abs((*plast_pktl)->seq - (*phead_pktl)->seq) > MAX_LIMIT_PACKETS) {
		AVPacketList *tmp_pktl = (*phead_pktl);
		AVPacketList *nxt_pktl = (*phead_pktl)->next;

		av_free_packet(&tmp_pktl->pkt);
		av_free(tmp_pktl);
		*phead_pktl = nxt_pktl;
	}

	return;
}

static void queue_attached_pictures(AVFormatContext *s)
{
	int i;
	for (i = 0; i < s->nb_streams; i++)
		if (s->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC &&
				s->streams[i]->discard < AVDISCARD_ALL) {
			AVPacket copy = s->streams[i]->attached_pic;
			copy.destruct = NULL;
			add_to_pktbuf(&s->raw_packet_buffer, &copy, &s->raw_packet_buffer_end);
		}
}

int avformat_open_input(AVFormatContext **ps,
		const char *filename,AVInputFormat *fmt, AVDictionary** options)
{
	AVFormatContext *s = *ps;
	int ret = 0;
	AVDictionary *tmp = NULL;
	AVDictionaryEntry *e = NULL;
	ID3v2ExtraMeta *id3v2_extra_meta = NULL;

	if (!s && !(s = avformat_alloc_context())) {
		return AVERROR(ENOMEM);
	}

	if (fmt)
		s->iformat = fmt;

	if (options)
		av_dict_copy(&tmp, *options, 0);

	s->avio_flags = 0;
	if (tmp && (e = av_dict_get(tmp, "avioflags", NULL, 0))) {
		s->avio_flags = atoi(e->value);
	}
	if (!(s->url = av_strdup(filename ? filename : ""))) {
		ret = AVERROR(ENOMEM);
		goto fail;
	}
	if ((ret = init_input(s, filename, &tmp)) < 0) {
		goto fail;
	}

	/* check filename in case an image number is expected */
	if (s->iformat->flags & AVFMT_NEEDNUMBER) {
		if (!av_filename_number_test(filename)) {
			ret = AVERROR(EINVAL);
			goto fail;
		}
	}

	s->duration = s->start_time = AV_NOPTS_VALUE;

	/* allocate private data */
	if (s->iformat->priv_data_size > 0) {
		if (!(s->priv_data = av_mallocz(s->iformat->priv_data_size))) {
			ret = AVERROR(ENOMEM);
			goto fail;
		}
	}

	if ((!s->url_options) && ((s->pb && s->pb->options) || tmp)) {
		av_dict_copy(&(s->url_options), (s->pb && s->pb->options)?s->pb->options:tmp, 0);
	}
#ifdef CONFIG_DEMUX_MP3
	/* e.g. AVFMT_NOFILE formats will not have a ByteIOContext */
    if (s->pb)
        ff_id3v2_read_dict(s->pb, &s->id3v2_meta, ID3v2_DEFAULT_MAGIC, &id3v2_extra_meta);
#endif
	if (!(s->flags&AVFMT_FLAG_PRIV_OPT) && s->iformat->read_header) {
		if ((ret = s->iformat->read_header(s, NULL)) < 0)
			goto fail;
    }
#ifdef CONFIG_DEMUX_MP3
	ff_id3v2_free_extra_meta(&id3v2_extra_meta);
#endif
	queue_attached_pictures(s);

	if (!(s->flags&AVFMT_FLAG_PRIV_OPT) && s->pb && !s->data_offset)
		s->data_offset = avio_tell(s->pb);

	s->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;

	if (s->url_options || (options && *options)) {
		if (*options)
			av_dict_free(options);
		av_dict_copy(options, s->url_options?s->url_options:tmp, AV_DICT_MULTIKEY);
		if (tmp)
			av_dict_free(&tmp);
	}
	*ps = s;
	return 0;

fail:
#ifdef CONFIG_DEMUX_MP3
	ff_id3v2_free_extra_meta(&id3v2_extra_meta);
#endif
	if (tmp)
		av_dict_free(&tmp);
	if (s->iformat && (s->iformat->read_close))
		s->iformat->read_close(s);
	if (s->pb && !(s->flags & AVFMT_FLAG_CUSTOM_IO))
		url_fclose(s->pb);
	else if (s->pb && (s->flags & AVFMT_FLAG_CUSTOM_IO) && (s->file_format != GX_STREAMTYPE_STREAM)) {
		avio_context_free(&s->pb);
	}
	avformat_free_context(s);
	*ps = NULL;
	return ret;
}

/*******************************************************/

int ff_read_packet(AVFormatContext *s, AVPacket *pkt)
{
	int ret, i;
	AVStream *st = NULL;

	for(;;){
		AVPacketList *pktl = s->raw_packet_buffer;

		if (pktl) {
			*pkt = pktl->pkt;
			if(s->streams[pkt->stream_index]->request_probe <= 0){
				s->raw_packet_buffer = pktl->next;
				s->raw_packet_buffer_remaining_size += pkt->size;
				av_free(pktl);
				return 0;
			}
		}

		pkt->size = 0;
		av_init_packet(pkt);
		ret= s->iformat->read_packet(s, pkt);
		if (ret < 0) {
			if (!pktl || ret == AVERROR(EAGAIN) || ret==AVERROR(ENOMEM))
				return ret;
			for (i = 0; i < s->nb_streams; i++)
				if(s->streams[i]->request_probe > 0)
					s->streams[i]->request_probe = -1;
			continue;
		}

		if ((s->flags & AVFMT_FLAG_DISCARD_CORRUPT) &&
				(pkt->flags & AV_PKT_FLAG_CORRUPT)) {
			gxlogd ("Dropped corrupted packet (stream = %d)\n", pkt->stream_index);
			av_free_packet(pkt);
			continue;
		}

		if(!(s->flags & AVFMT_FLAG_KEEP_SIDE_DATA))
			av_packet_merge_side_data(pkt);

		if(pkt->stream_index >= (unsigned)s->nb_streams){
			gxlogd ("Invalid stream index %d\n", pkt->stream_index);
			continue;
		}

		st= s->streams[pkt->stream_index];

		switch(st->codec->codec_type){
			case CODEC_TYPE_VIDEO:
				if(s->video_codec_id)   st->codec->codec_id= s->video_codec_id;
				break;
			case CODEC_TYPE_AUDIO:
				if(s->audio_codec_id)   st->codec->codec_id= s->audio_codec_id;
				if (st->codec->codec_id == CODEC_ID_PCM_BLURAY) {
					BlurayPcmHeader hdr;
					if(pcm_bluray_parse_header(&hdr, pkt->data) == 0) {
						st->codec->bluray_pcm.channels    = hdr.channels;
						st->codec->bluray_pcm.big_endian  = hdr.big_endian;
						st->codec->bluray_pcm.sample_rate  = hdr.sample_rate;
						st->codec->bluray_pcm.sample_size = hdr.sample_size;
					}
				}
				break;
			case CODEC_TYPE_SUBTITLE:
				if(s->subtitle_codec_id)st->codec->codec_id= s->subtitle_codec_id;
				break;
			default:
				break;
		}

		if(!pktl && st->request_probe <= 0)
			return ret;

		if (!add_to_pktbuf(&s->raw_packet_buffer, pkt, &s->raw_packet_buffer_end)) {
			return AVERROR(ENOMEM);
		}
		s->raw_packet_buffer_remaining_size -= pkt->size;

		if(st->request_probe>0){
			AVProbeData *pd = &st->probe_data;
			int end;
//			av_log(s, AV_LOG_DEBUG, "probing stream %d pp:%d\n", st->index, st->probe_packets);
			--st->probe_packets;
			uint8_t *new_buf=NULL;

			if(pd->buf_size+pkt->size<s->probesize &&
					(NULL!=(new_buf = av_realloc(pd->buf, pd->buf_size+pkt->size+AVPROBE_PADDING_SIZE)))
			  ){//it will read too much

				pd->buf=new_buf;
				memcpy(pd->buf+pd->buf_size, pkt->data, pkt->size);
				pd->buf_size += pkt->size;
				memset(pd->buf+pd->buf_size, 0, AVPROBE_PADDING_SIZE);
				end=    s->raw_packet_buffer_remaining_size <= 0
					|| st->probe_packets<=0;

				if(end || av_log2(pd->buf_size) != av_log2(pd->buf_size - pkt->size)){
					int score= set_codec_from_probe_data(s, st, pd);
					if(    (st->codec->codec_id != CODEC_ID_NONE && score > AVPROBE_SCORE_MAX/4)
							|| end){
						pd->buf_size=0;
						av_freep(&pd->buf);
						st->request_probe= -1;
						if(st->codec->codec_id != CODEC_ID_NONE){
							gxlogd ("probed stream %d\n", st->index);
						}else
							gxlogd ("probed stream %d failed\n", st->index);
					}
				}
			}else{
				st->probe_packets=0;
				pd->buf_size=0;
				av_freep(&pd->buf);
				st->request_probe= -1;
			}
		}
		limit_to_pktbuf(&s->raw_packet_buffer, &s->raw_packet_buffer_end);
	}
}

int av_read_packet(AVFormatContext *s, AVPacket *pkt)
{
	return ff_read_packet(s, pkt);
}

#if 0
/**********************************************************/
static int determinable_frame_size(AVCodecContext *avctx)
{
	if (/*avctx->codec_id == CODEC_ID_AAC ||*/
			avctx->codec_id == CODEC_ID_MP1 ||
			avctx->codec_id == CODEC_ID_MP2 ||
			avctx->codec_id == CODEC_ID_MP3/* ||
											  avctx->codec_id == CODEC_ID_CELT*/)
		return 1;
	return 0;
}
#endif
/**
 * Get the number of samples of an audio frame. Return -1 on error.
 */
static int get_audio_frame_size(AVCodecContext *enc, int size, int mux)
{
	int frame_size;

	/* give frame_size priority if demuxing */
	if (!mux && enc->frame_size > 1)
		return enc->frame_size;

	if ((frame_size = av_get_audio_frame_duration(enc, size)) > 0)
		return frame_size;

	/* fallback to using frame_size if muxing */
	if (enc->frame_size > 1)
		return enc->frame_size;

	return -1;
}

/**
 * Return the frame duration in seconds. Return 0 if not available.
 */
static void compute_frame_duration(int *pnum, int *pden, AVStream *st,
		AVCodecParserContext *pc, AVPacket *pkt)
{
	int frame_size;

	*pnum = 0;
	*pden = 0;
	switch(st->codec->codec_type) {
		case CODEC_TYPE_VIDEO:
			if (!pc && st->r_frame_rate.num > 0 && st->r_frame_rate.num >0) {
				*pnum = st->r_frame_rate.den;
				*pden = st->r_frame_rate.num;
			} else if(st->time_base.num*1000LL > st->time_base.den) {
				*pnum = st->time_base.num;
				*pden = st->time_base.den;
			}else if(st->codec->time_base.num*1000LL > st->codec->time_base.den){
				*pnum = st->codec->time_base.num;
				*pden = st->codec->time_base.den;
				if (pc && pc->repeat_pict) {
					*pnum = (*pnum) * (1 + pc->repeat_pict);
				}
				//If this codec can be interlaced or progressive then we need a parser to compute duration of a packet
				//Thus if we have no parser in such case leave duration undefined.
				if(st->codec->ticks_per_frame>1 && !pc){
					*pnum = *pden = 0;
				}
			}
			break;
		case CODEC_TYPE_AUDIO:
			frame_size = get_audio_frame_size(st->codec, pkt->size, 0);
			if (frame_size <= 0 || st->codec->sample_rate <= 0)
				break;
			*pnum = frame_size;
			*pden = st->codec->sample_rate;
			break;
		default:
			break;
	}
}

static int is_intra_only(AVCodecContext *enc){
	if(enc->codec_type == CODEC_TYPE_AUDIO){
		return 1;
	}else if(enc->codec_type == CODEC_TYPE_VIDEO){
		switch(enc->codec_id){
			case CODEC_ID_MPEG1VIDEO:
			case CODEC_ID_MPEG2VIDEO:
			case CODEC_ID_DVD_NAV:
			case CODEC_ID_MJPEG:
			case CODEC_ID_MJPEGB:
			case CODEC_ID_LJPEG:
//			case CODEC_ID_PRORES:
			case CODEC_ID_RAWVIDEO:
//			case CODEC_ID_V210:
			case CODEC_ID_DVVIDEO:
			case CODEC_ID_HUFFYUV:
			case CODEC_ID_FFVHUFF:
			case CODEC_ID_ASV1:
			case CODEC_ID_ASV2:
			case CODEC_ID_VCR1:
			case CODEC_ID_DNXHD:
			case CODEC_ID_JPEG2000:
			case CODEC_ID_MDEC:
//			case CODEC_ID_UTVIDEO:
				return 1;
			default: break;
		}
	}
	return 0;
}

static AVPacketList *get_next_pkt(AVFormatContext *s, AVStream *st, AVPacketList *pktl)
{
	if (pktl->next)
		return pktl->next;
	if (pktl == s->parse_queue_end)
		return s->packet_buffer;
	return NULL;
}

static void update_initial_timestamps(AVFormatContext *s, int stream_index,
		int64_t dts, int64_t pts)
{
	AVStream *st= s->streams[stream_index];
	AVPacketList *pktl= s->parse_queue ? s->parse_queue : s->packet_buffer;

	if(st->first_dts != AV_NOPTS_VALUE || dts == AV_NOPTS_VALUE || st->cur_dts == AV_NOPTS_VALUE || is_relative(dts))
		return;

	st->first_dts= dts - (st->cur_dts - RELATIVE_TS_BASE);
	st->cur_dts= dts;

	if (is_relative(pts))
		pts += st->first_dts - RELATIVE_TS_BASE;

	for(; pktl; pktl= get_next_pkt(s, st, pktl)){
		if(pktl->pkt.stream_index != stream_index)
			continue;
		if(is_relative(pktl->pkt.pts))
			pktl->pkt.pts += st->first_dts - RELATIVE_TS_BASE;

		if(is_relative(pktl->pkt.dts))
			pktl->pkt.dts += st->first_dts - RELATIVE_TS_BASE;

		if(st->start_time == AV_NOPTS_VALUE && pktl->pkt.pts != AV_NOPTS_VALUE)
			st->start_time= pktl->pkt.pts;
	}
	if (st->start_time == AV_NOPTS_VALUE)
		st->start_time = pts;
}

static void update_initial_durations(AVFormatContext *s, AVStream *st,
		int stream_index, int duration)
{
	AVPacketList *pktl= s->parse_queue ? s->parse_queue : s->packet_buffer;
	int64_t cur_dts= RELATIVE_TS_BASE;

	if(st->first_dts != AV_NOPTS_VALUE){
		cur_dts= st->first_dts;
		for(; pktl; pktl= get_next_pkt(s, st, pktl)){
			if(pktl->pkt.stream_index == stream_index){
				if(pktl->pkt.pts != pktl->pkt.dts || pktl->pkt.dts != AV_NOPTS_VALUE || pktl->pkt.duration)
					break;
				cur_dts -= duration;
			}
		}
		if(pktl && pktl->pkt.dts != st->first_dts) {
			//av_log(s, AV_LOG_DEBUG, "first_dts %s not matching first dts %s in que\n", av_ts2str(st->first_dts), av_ts2str(pktl->pkt.dts));
			return;
		}
		if(!pktl) {
			//av_log(s, AV_LOG_DEBUG, "first_dts %s but no packet with dts in ques\n", av_ts2str(st->first_dts));
			return;
		}
		pktl= s->parse_queue ? s->parse_queue : s->packet_buffer;
		st->first_dts = cur_dts;
	}else if(st->cur_dts != RELATIVE_TS_BASE)
		return;

	for(; pktl; pktl= get_next_pkt(s, st, pktl)){
		if(pktl->pkt.stream_index != stream_index)
			continue;
		if(pktl->pkt.pts == pktl->pkt.dts && (pktl->pkt.dts == AV_NOPTS_VALUE || pktl->pkt.dts == st->first_dts)
				&& !pktl->pkt.duration){
			pktl->pkt.dts= cur_dts;
			if(!st->codec->has_b_frames)
				pktl->pkt.pts= cur_dts;
			//            if (st->codec->codec_type != CODEC_TYPE_AUDIO)
			pktl->pkt.duration = duration;
		}else
			break;
		cur_dts = pktl->pkt.dts + pktl->pkt.duration;
	}
	if(!pktl)
		st->cur_dts= cur_dts;
}
int64_t av_add_stable(AVRational ts_tb, int64_t ts, AVRational inc_tb, int64_t inc)
{
	AVRational step = av_mul_q(inc_tb, (AVRational) {inc, 1});

	if (av_cmp_q(step, ts_tb) < 0) {
		//increase step is too small for even 1 step to be representable
		return ts;
	} else {
		int64_t old = av_rescale_q(ts, ts_tb, step);
		int64_t old_ts = av_rescale_q(old, step, ts_tb);
		return av_rescale_q(old + 1, step, ts_tb) + (ts - old_ts);
	}
}
static int has_decode_delay_been_guessed(AVStream *st)
{
	if (st->codec->codec_id != CODEC_ID_H264) return 1;
	if (!st->info) // if we have left find_stream_info then nb_decoded_frames won't increase anymore for stream copy
		return 1;
#if 0
#if CONFIG_H264_DECODER
	if (st->codec->has_b_frames &&
			avpriv_h264_has_num_reorder_frames(st->codec) == st->codec->has_b_frames)
		return 1;
#endif
	if (st->codec->has_b_frames<3)
		return st->nb_decoded_frames >= 7;
	else if (st->codec->has_b_frames<4)
		return st->nb_decoded_frames >= 18;
	else
		return st->nb_decoded_frames >= 20;
#else
	return 0;
#endif
}
static int64_t select_from_pts_buffer(AVStream *st, int64_t *pts_buffer, int64_t dts) {
	int onein_oneout = st->codec->codec_id != CODEC_ID_H264 &&
		st->codec->codec_id != CODEC_ID_HEVC;

	if(!onein_oneout) {
#if 0
		int delay = st->codec->has_b_frames;
		int i;

		if (dts == AV_NOPTS_VALUE) {
			int64_t best_score = INT64_MAX;
			for (i = 0; i<delay; i++) {
				if (st->pts_reorder_error_count[i]) {
					int64_t score = st->pts_reorder_error[i] / st->pts_reorder_error_count[i];
					if (score < best_score) {
						best_score = score;
						dts = pts_buffer[i];
					}
				}
			}
		} else {
			for (i = 0; i<delay; i++) {
				if (pts_buffer[i] != AV_NOPTS_VALUE) {
					int64_t diff =  FFABS(pts_buffer[i] - dts)
						+ (uint64_t)st->pts_reorder_error[i];
					diff = FFMAX(diff, st->pts_reorder_error[i]);
					st->pts_reorder_error[i] = diff;
					st->pts_reorder_error_count[i]++;
					if (st->pts_reorder_error_count[i] > 250) {
						st->pts_reorder_error[i] >>= 1;
						st->pts_reorder_error_count[i] >>= 1;
					}
				}
			}
		}
#endif
	}

	if (dts == AV_NOPTS_VALUE)
		dts = pts_buffer[0];

	return dts;
}

static void compute_pkt_fields(AVFormatContext *s, AVStream *st,
		AVCodecParserContext *pc, AVPacket *pkt)
{
	int num, den, presentation_delayed, delay, i;
	int64_t offset;
	AVRational duration;
	int onein_oneout = 1; //st->codec->codec_id != CODEC_ID_H264 && st->codec->codec_id != CODEC_ID_HEVC;

	if (s->flags & AVFMT_FLAG_NOFILLIN)
		return;

	if (st->codec->codec_type == CODEC_TYPE_VIDEO	&& pkt->dts != AV_NOPTS_VALUE) {
		if (pkt->dts == pkt->pts && st->last_dts_for_order_check != AV_NOPTS_VALUE) {
			if (st->last_dts_for_order_check <= pkt->dts) {
				st->dts_ordered++;
			} else {
				st->dts_misordered++;
			}
			if (st->dts_ordered + st->dts_misordered > 250) {
				st->dts_ordered    >>= 1;
				st->dts_misordered >>= 1;
			}
		}

		st->last_dts_for_order_check = pkt->dts;
		if (st->dts_ordered < 8*st->dts_misordered && pkt->dts == pkt->pts)
			pkt->dts = AV_NOPTS_VALUE;
	}

	if ((s->flags & AVFMT_FLAG_IGNDTS) && pkt->pts != AV_NOPTS_VALUE)
		pkt->dts = AV_NOPTS_VALUE;

	if (pc && pc->pict_type == AV_PICTURE_TYPE_B
			&& !st->codec->has_b_frames)
		//FIXME Set low_delay = 0 when has_b_frames = 1
		st->codec->has_b_frames = 1;

	/* do we have a video B-frame ? */
	delay = st->codec->has_b_frames;
	presentation_delayed = 0;

	/* XXX: need has_b_frame, but cannot get it if the codec is
	 *  not initialized */
	if (delay &&
			pc && pc->pict_type != AV_PICTURE_TYPE_B)
		presentation_delayed = 1;

	if (pkt->pts != AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE &&
			st->pts_wrap_bits < 63 &&
			pkt->dts - (1LL << (st->pts_wrap_bits - 1)) > pkt->pts) {
		if (is_relative(st->cur_dts) || pkt->dts - (1LL<<(st->pts_wrap_bits - 1)) > st->cur_dts) {
			pkt->dts -= 1LL << st->pts_wrap_bits;
		} else
			pkt->pts += 1LL << st->pts_wrap_bits;
	}

	/* Some MPEG-2 in MPEG-PS lack dts (issue #171 / input_file.mpg).
	 * We take the conservative approach and discard both.
	 * Note: If this is misbehaving for an H.264 file, then possibly
	 * presentation_delayed is not set correctly. */
	if (delay == 1 && pkt->dts == pkt->pts &&
			pkt->dts != AV_NOPTS_VALUE && presentation_delayed) {
		if (    strcmp(s->iformat->name, "mov,mp4,m4a,3gp,3g2,mj2")
				&& strcmp(s->iformat->name, "flv")) // otherwise we discard correct timestamps for vc1-wmapro.ism
			pkt->dts = AV_NOPTS_VALUE;
	}

	duration = av_mul_q((AVRational) {pkt->duration, 1}, st->time_base);
	if (pkt->duration == 0) {
		compute_frame_duration(&num, &den, st, pc, pkt);
		if (den && num) {
			duration = (AVRational) {num, den};
			pkt->duration = av_rescale_rnd(1,
					num * (int64_t) st->time_base.den,
					den * (int64_t) st->time_base.num,
					AV_ROUND_DOWN);
		}
	}

	if (pkt->duration != 0 && (s->packet_buffer || s->parse_queue))
		update_initial_durations(s, st, pkt->stream_index, pkt->duration);

	/* Correct timestamps with byte offset if demuxers only have timestamps
	 * on packet boundaries */
	if (pc && st->need_parsing == AVSTREAM_PARSE_TIMESTAMPS && pkt->size) {
		/* this will estimate bitrate based on this frame's duration and size */
		offset = av_rescale(pc->offset, pkt->duration, pkt->size);
		if (pkt->pts != AV_NOPTS_VALUE)
			pkt->pts += offset;
		if (pkt->dts != AV_NOPTS_VALUE)
			pkt->dts += offset;
	}

	/* This may be redundant, but it should not hurt. */
	if (pkt->dts != AV_NOPTS_VALUE &&
			pkt->pts != AV_NOPTS_VALUE &&
			pkt->pts > pkt->dts)
		presentation_delayed = 1;

//	av_dlog(NULL,
//			"IN delayed:%d pts:%s, dts:%s cur_dts:%s st:%d pc:%p duration:%d\n",
//			presentation_delayed, av_ts2str(pkt->pts), av_ts2str(pkt->dts), av_ts2str(st->cur_dts),
//			pkt->stream_index, pc, pkt->duration);
	/* Interpolate PTS and DTS if they are not present. We skip H264
	 * currently because delay and has_b_frames are not reliably set. */
	if ((delay == 0 || (delay == 1 && pc)) &&
			onein_oneout) {
		if (presentation_delayed) {
			/* DTS = decompression timestamp */
			/* PTS = presentation timestamp */
			if (pkt->dts == AV_NOPTS_VALUE)
				pkt->dts = st->last_IP_pts;
			//update_initial_timestamps(s, pkt->stream_index, pkt->dts, pkt->pts, pkt);
			update_initial_timestamps(s, pkt->stream_index, pkt->dts, pkt->pts);
			if (pkt->dts == AV_NOPTS_VALUE)
				pkt->dts = st->cur_dts;

			/* This is tricky: the dts must be incremented by the duration
			 * of the frame we are displaying, i.e. the last I- or P-frame. */
			if (st->last_IP_duration == 0)
				st->last_IP_duration = pkt->duration;
			if (pkt->dts != AV_NOPTS_VALUE)
				st->cur_dts = pkt->dts + st->last_IP_duration;
			st->last_IP_duration = pkt->duration;
			st->last_IP_pts      = pkt->pts;
			/* Cannot compute PTS if not present (we can compute it only
			 * by knowing the future. */
		} else if (pkt->pts != AV_NOPTS_VALUE ||
				pkt->dts != AV_NOPTS_VALUE ||
				pkt->duration                ) {

			/* presentation is not delayed : PTS and DTS are the same */
			if (pkt->pts == AV_NOPTS_VALUE)
				pkt->pts = pkt->dts;
			//update_initial_timestamps(s, pkt->stream_index, pkt->pts,pkt->pts, pkt);
			update_initial_timestamps(s, pkt->stream_index, pkt->pts,pkt->pts);
			if (pkt->pts == AV_NOPTS_VALUE)
				pkt->pts = st->cur_dts;
			pkt->dts = pkt->pts;
			if (pkt->pts != AV_NOPTS_VALUE)
				st->cur_dts = av_add_stable(st->time_base, pkt->pts, duration, 1);
		}
	}
	if (pkt->pts != AV_NOPTS_VALUE && delay <= MAX_REORDER_DELAY && has_decode_delay_been_guessed(st)) {
		st->pts_buffer[0] = pkt->pts;
		for (i = 0; i<delay && st->pts_buffer[i] > st->pts_buffer[i + 1]; i++)
			FFSWAP(int64_t, st->pts_buffer[i], st->pts_buffer[i + 1]);

		pkt->dts = select_from_pts_buffer(st, st->pts_buffer, pkt->dts);
	}
	// We skipped it above so we try here.
	if (!onein_oneout)
		// This should happen on the first packet
		//update_initial_timestamps(s, pkt->stream_index, pkt->dts, pkt->pts, pkt);
		update_initial_timestamps(s, pkt->stream_index, pkt->dts, pkt->pts);
	if (pkt->dts > st->cur_dts)
		st->cur_dts = pkt->dts;

//	av_dlog(NULL, "OUTdelayed:%d/%d pts:%s, dts:%s cur_dts:%s\n",
//			presentation_delayed, delay, av_ts2str(pkt->pts), av_ts2str(pkt->dts), av_ts2str(st->cur_dts));

	/* update flags */
	if (is_intra_only(st->codec))
		pkt->flags |= AV_PKT_FLAG_KEY;
	if (pc)
		pkt->convergence_duration = pc->convergence_duration;
}

static void free_packet_buffer(AVPacketList **pkt_buf, AVPacketList **pkt_buf_end)
{

	while (*pkt_buf) {
		AVPacketList *pktl = *pkt_buf;
		if (!pktl)
			return;
		*pkt_buf = pktl->next;
		av_free_packet(&pktl->pkt);
		av_freep(&pktl);
	}
	*pkt_buf_end = NULL;
}

/**
 * Parse a packet, add all split parts to parse_queue
 *
 * @param pkt packet to parse, NULL when flushing the parser at end of stream
 */
static int parse_packet(AVFormatContext *s, AVPacket *pkt, int stream_index, int flush)
{
	AVPacket out_pkt = { 0 }, flush_pkt = { 0 };
	AVStream *st = s->streams[stream_index];
	uint8_t *data = pkt ? pkt->data : NULL;
	int size      = pkt ? pkt->size : 0;
	int ret = 0, got_output = flush;

	if (!pkt) {
		av_init_packet(&flush_pkt);
		pkt        = &flush_pkt;
		got_output = 1;
	} else if (!size && st->parser->flags & PARSER_FLAG_COMPLETE_FRAMES) {
		// preserve 0-size sync packets
		compute_pkt_fields(s, st, st->parser, pkt);
	}

	while (size > 0 || (pkt == &flush_pkt && got_output) || (flush && got_output)) {
		int len;

		av_init_packet(&out_pkt);
		len = av_parser_parse2(st->parser, st->codec,
				&out_pkt.data, &out_pkt.size, data, size,
				pkt->pts, pkt->dts, pkt->pos);

		pkt->pts = pkt->dts = AV_NOPTS_VALUE;
		pkt->pos = -1;
		/* increment read pointer */
		data += len;
		size -= len;

		got_output = !!out_pkt.size;

		if (!out_pkt.size)
			continue;

		if (pkt->side_data) {
			out_pkt.side_data       = pkt->side_data;
			out_pkt.side_data_elems = pkt->side_data_elems;
			pkt->side_data          = NULL;
			pkt->side_data_elems    = 0;
		}

		/* set the duration */
		out_pkt.duration = 0;
		if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
			if (st->codec->sample_rate > 0) {
				out_pkt.duration =
					av_rescale_q_rnd(st->parser->duration,
							(AVRational) { 1, st->codec->sample_rate },
							st->time_base,
							AV_ROUND_DOWN);
			}
		} else if (st->codec->time_base.num != 0 &&
				st->codec->time_base.den != 0) {
			out_pkt.duration = av_rescale_q_rnd(st->parser->duration,
					st->codec->time_base,
					st->time_base,
					AV_ROUND_DOWN);
		}

		out_pkt.stream_index = st->index;
		out_pkt.pts          = st->parser->pts;
		out_pkt.dts          = st->parser->dts;
		out_pkt.pos          = st->parser->pos;

		if (st->need_parsing == AVSTREAM_PARSE_FULL_RAW)
			out_pkt.pos = st->parser->frame_offset;

		if (st->parser->key_frame == 1 ||
				(st->parser->key_frame == -1 &&
				 st->parser->pict_type == AV_PICTURE_TYPE_I))
			out_pkt.flags |= AV_PKT_FLAG_KEY;

		if (st->parser->key_frame == -1 && st->parser->pict_type ==AV_PICTURE_TYPE_NONE && (pkt->flags&AV_PKT_FLAG_KEY))
			out_pkt.flags |= AV_PKT_FLAG_KEY;

		compute_pkt_fields(s, st, st->parser, &out_pkt);

		if (out_pkt.data == pkt->data && out_pkt.size == pkt->size) {
			out_pkt.destruct = pkt->destruct;
			pkt->destruct = NULL;
		}
		if ((ret = av_dup_packet(&out_pkt)) < 0)
			goto fail;

		if (!add_to_pktbuf(&s->parse_queue, &out_pkt, &s->parse_queue_end)) {
			av_free_packet(&out_pkt);
			ret = AVERROR(ENOMEM);
			goto fail;
		}
	}

	/* end of the stream => close and free the parser */
	if (pkt == &flush_pkt) {
		av_parser_close(st->parser);
		st->parser = NULL;
	}

fail:
	av_free_packet(pkt);
	return ret;
}

static int read_from_packet_buffer(AVPacketList **pkt_buffer,
		AVPacketList **pkt_buffer_end,
		AVPacket      *pkt)
{
	AVPacketList *pktl;
	av_assert0(*pkt_buffer);
	pktl = *pkt_buffer;
	*pkt = pktl->pkt;
	*pkt_buffer = pktl->next;
	if (!pktl->next)
		*pkt_buffer_end = NULL;
	av_freep(&pktl);
	return 0;
}

static int read_frame_internal(AVFormatContext *s, AVPacket *pkt)
{
	int ret = 0, i, got_packet = 0;
	int lost_packet = 0;

	av_init_packet(pkt);

	while (!got_packet && !s->parse_queue) {
		AVStream *st;
		AVPacket cur_pkt;

		/* read next packet */
		ret = ff_read_packet(s, &cur_pkt);
		if (cur_pkt.lost_packet != 0)
			lost_packet = cur_pkt.lost_packet;
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN) || ret==AVERROR(ENOMEM))
				return ret;
			/* flush the parsers */
			for(i = 0; i < s->nb_streams; i++) {
				st = s->streams[i];
				if (st->parser && st->need_parsing)
					parse_packet(s, &cur_pkt, st->index, 1);
			}
			/* all remaining packets are now in parse_queue =>
			 * really terminate parsing */
			break;
		}
		ret = 0;
		st  = s->streams[cur_pkt.stream_index];

		if (cur_pkt.pts != AV_NOPTS_VALUE &&
				cur_pkt.dts != AV_NOPTS_VALUE &&
				cur_pkt.pts < cur_pkt.dts) {
			gxlogd ("Invalid timestamps stream=%d, pts=%s, dts=%s, size=%d\n",
					cur_pkt.stream_index,
					av_ts2str(cur_pkt.pts),
					av_ts2str(cur_pkt.dts),
					cur_pkt.size);
		}
//		if (s->debug & FF_FDEBUG_TS)
//			av_log(s, AV_LOG_DEBUG, "ff_read_packet stream=%d, pts=%s, dts=%s, size=%d, duration=%d, flags=%d\n",
//					cur_pkt.stream_index,
//					av_ts2str(cur_pkt.pts),
//					av_ts2str(cur_pkt.dts),
//					cur_pkt.size,
//					cur_pkt.duration,
//					cur_pkt.flags);

		if (st->need_parsing && !st->parser && !(s->flags & AVFMT_FLAG_NOPARSE)) {
			st->parser = av_parser_init(st->codec->codec_id);
			if (!st->parser) {
//				av_log(s, AV_LOG_VERBOSE, "parser not found for codec "
//						"%s, packets or times may be invalid.\n",
//						avcodec_get_name(st->codec->codec_id));
				/* no parser available: just output the raw packets */
				if(st->codec->codec_id == CODEC_ID_HEVC) {
					st->need_parsing = AVSTREAM_PARSE_HEVC;
				} else {
					st->need_parsing = AVSTREAM_PARSE_NONE;
				}
			} else if(st->need_parsing == AVSTREAM_PARSE_HEADERS) {
				st->parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
			} else if(st->need_parsing == AVSTREAM_PARSE_FULL_ONCE) {
				st->parser->flags |= PARSER_FLAG_ONCE;
			} else if(st->need_parsing == AVSTREAM_PARSE_FULL_RAW) {
				st->parser->flags |= PARSER_FLAG_USE_CODEC_TS;
			}
		}

		if (!st->need_parsing || !st->parser) {
			/* no parsing needed: we just output the packet as is */
			*pkt = cur_pkt;
			compute_pkt_fields(s, st, NULL, pkt);
			if ((s->iformat->flags & AVFMT_GENERIC_INDEX) &&
					(pkt->flags & AV_PKT_FLAG_KEY) && pkt->dts != AV_NOPTS_VALUE) {
				ff_reduce_index(s, st->index);
				av_add_index_entry(st, pkt->pos, pkt->dts, 0, 0, AVINDEX_KEYFRAME);
			}
			got_packet = 1;
		} else if (st->discard < AVDISCARD_ALL) {
			if ((ret = parse_packet(s, &cur_pkt, cur_pkt.stream_index, 0)) < 0)
				return ret;
		} else {
			/* free packet */
			av_free_packet(&cur_pkt);
		}
		if (pkt->flags & AV_PKT_FLAG_KEY)
			st->skip_to_keyframe = 0;
		if (st->skip_to_keyframe) {
			av_free_packet(&cur_pkt);
			got_packet = 0;
		}
	}

	if (!got_packet && s->parse_queue)
		ret = read_from_packet_buffer(&s->parse_queue, &s->parse_queue_end, pkt);

//	if(s->debug & FF_FDEBUG_TS)
//		av_log(s, AV_LOG_DEBUG, "read_frame_internal stream=%d, pts=%s, dts=%s, size=%d, duration=%d, flags=%d\n",
//				pkt->stream_index,
//				av_ts2str(pkt->pts),
//				av_ts2str(pkt->dts),
//				pkt->size,
//				pkt->duration,
//				pkt->flags);
	if (lost_packet != 0)
		pkt->lost_packet = lost_packet;
	return ret;
}

int av_read_frame(AVFormatContext *s, AVPacket *pkt)
{
	const int genpts = s->flags & AVFMT_FLAG_GENPTS;
	int          eof = 0;
	int ret;

	if (!genpts) {
		ret = s->packet_buffer ? read_from_packet_buffer(&s->packet_buffer,
				&s->packet_buffer_end,
				pkt) :
			read_frame_internal(s, pkt);
		goto return_packet;
	}

	for (;;) {
		AVPacketList *pktl = s->packet_buffer;

		if (pktl) {
			AVPacket *next_pkt = &pktl->pkt;

			if (next_pkt->dts != AV_NOPTS_VALUE) {
				int wrap_bits = s->streams[next_pkt->stream_index]->pts_wrap_bits;
				// last dts seen for this stream. if any of packets following
				// current one had no dts, we will set this to AV_NOPTS_VALUE.
				int64_t last_dts = next_pkt->dts;
				int packets = 0;

				while (pktl && next_pkt->pts == AV_NOPTS_VALUE) {
					if (pktl->pkt.stream_index == next_pkt->stream_index &&
							(av_compare_mod(next_pkt->dts, pktl->pkt.dts, 2LL << (wrap_bits - 1)) < 0)) {
						if (av_compare_mod(pktl->pkt.pts, pktl->pkt.dts, 2LL << (wrap_bits - 1))) { //not b frame
							next_pkt->pts = pktl->pkt.dts;
						}
						if (last_dts != AV_NOPTS_VALUE) {
							// Once last dts was set to AV_NOPTS_VALUE, we don't change it.
							last_dts = pktl->pkt.dts;
						}
					}
					pktl = pktl->next;
					packets++;
				}

				//linxsh fix:
				//       more than 50 packets not find pts, we will fix dts
				//       it will solve read many packet, avoid memory not enough
#define MAX_PACKETS (50)
				if ((packets >= MAX_PACKETS) &&
						(next_pkt->pts == AV_NOPTS_VALUE) &&
						(next_pkt->dts != AV_NOPTS_VALUE)) {
					gxlogi(">>> more (%d) packets\n", packets);
					next_pkt->dts = next_pkt->pts = AV_NOPTS_VALUE;
				}

				if (eof && next_pkt->pts == AV_NOPTS_VALUE && last_dts != AV_NOPTS_VALUE) {
					// Fixing the last reference frame had none pts issue (For MXF etc).
					// We only do this when
					// 1. eof.
					// 2. we are not able to resolve a pts value for current packet.
					// 3. the packets for this stream at the end of the files had valid dts.
					next_pkt->pts = last_dts + next_pkt->duration;
				}
				pktl = s->packet_buffer;
			}

			/* read packet from packet buffer, if there is data */
			if (!(next_pkt->pts == AV_NOPTS_VALUE &&
						next_pkt->dts != AV_NOPTS_VALUE && !eof)) {
				ret = read_from_packet_buffer(&s->packet_buffer,
						&s->packet_buffer_end, pkt);
				goto return_packet;
			}
		}

		ret = read_frame_internal(s, pkt);
		if (ret < 0) {
			if (pktl && ret != AVERROR(EAGAIN)) {
				eof = 1;
				continue;
			} else
				return ret;
		}

		if (av_dup_packet(add_to_pktbuf(&s->packet_buffer, pkt,	&s->packet_buffer_end)) < 0) {
			return AVERROR(ENOMEM);
		}
	}

return_packet:
	if (is_relative(pkt->dts))
		pkt->dts -= RELATIVE_TS_BASE;
	if (is_relative(pkt->pts))
		pkt->pts -= RELATIVE_TS_BASE;
	return ret;
}

/* XXX: suppress the packet queue */
static void flush_packet_queue(AVFormatContext *s)
{
	if (NULL == s)
		return ;
	free_packet_buffer(&s->parse_queue,       &s->parse_queue_end);
	free_packet_buffer(&s->packet_buffer,     &s->packet_buffer_end);
	free_packet_buffer(&s->raw_packet_buffer, &s->raw_packet_buffer_end);

	s->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;
}

/*******************************************************/
/* seek support */
int av_find_default_stream_index(AVFormatContext *s)
{
	int first_audio_index = -1;
	int i;
	AVStream *st;

	if (s->nb_streams <= 0)
		return -1;
	for(i = 0; i < s->nb_streams; i++) {
		st = s->streams[i];
		if (st->codec->codec_type == CODEC_TYPE_VIDEO &&
				!(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
			return i;
		}
		if (first_audio_index < 0 && st->codec->codec_type == CODEC_TYPE_AUDIO)
			first_audio_index = i;
	}
	return first_audio_index >= 0 ? first_audio_index : 0;
}

/**
 * Flush the frame reader.
 */
void av_read_frame_flush(AVFormatContext *s)
{
	AVStream *st;
	int i, j;

	flush_packet_queue(s);

	/* for each stream, reset read state */
	for(i = 0; i < s->nb_streams; i++) {
		st = s->streams[i];

		if (st->parser) {
			av_parser_close(st->parser);
			st->parser = NULL;
		}
		st->last_IP_pts = AV_NOPTS_VALUE;
		if(st->first_dts == AV_NOPTS_VALUE) st->cur_dts = RELATIVE_TS_BASE;
		else                                st->cur_dts = AV_NOPTS_VALUE; /* we set the current DTS to an unspecified origin */
		st->reference_dts = AV_NOPTS_VALUE;

		st->probe_packets = MAX_PROBE_PACKETS;

		for(j=0; j<MAX_REORDER_DELAY+1; j++)
			st->pts_buffer[j]= AV_NOPTS_VALUE;
	}
}

void av_update_cur_dts(AVFormatContext *s, AVStream *ref_st, int64_t timestamp)
{
	int i;

	for(i = 0; i < s->nb_streams; i++) {
		AVStream *st = s->streams[i];

		st->cur_dts = av_rescale(timestamp,
				st->time_base.den * (int64_t)ref_st->time_base.num,
				st->time_base.num * (int64_t)ref_st->time_base.den);
	}
}

void ff_reduce_index(AVFormatContext *s, int stream_index)
{
	AVStream *st= s->streams[stream_index];
	unsigned int max_entries= s->max_index_size / sizeof(AVIndexEntry);

	if((unsigned)st->nb_index_entries >= max_entries){
		int i;
		for(i=0; 2*i<st->nb_index_entries; i++)
			st->index_entries[i]= st->index_entries[2*i];
		st->nb_index_entries= i;
	}
}

int ff_add_index_entry_mov(AVIndexEntry **index_entries,
		int *nb_index_entries,
		unsigned int *index_entries_allocated_size,
		int *index_entries_start,
		int *index_entries_end,
		int64_t pos, int64_t timestamp, int size, int distance, int flags, int fragment, int nPlusOnly,
		int index_max_count)
{

	AVIndexEntry *entries, *ie;
	int index;
	unsigned int nsize;
	int index_max_count_move = index_max_count / 5;

	if((unsigned)*nb_index_entries + 1 >= UINT_MAX / sizeof(AVIndexEntry))
		return -1;
	if(nPlusOnly){
		(*nb_index_entries)++;//only plus ,this is not an error,caller should take care
		return 0;
	}

	if (is_relative(timestamp)) //FIXME this maintains previous behavior but we should shift by the correct offset once known
		timestamp -= RELATIVE_TS_BASE;

	if (*index_entries_end - *index_entries_start == 0) {
		nsize = index_max_count * sizeof(AVIndexEntry);
		entries = (AVIndexEntry*)av_malloc(nsize);
		if (!entries) {
			gxlogd("#########mkv opt oom#######\n");
			return -1;
		} else {
			*index_entries_allocated_size = nsize;
			*index_entries = entries;
		}
	} else {
		if (*index_entries_end - *index_entries_start >= index_max_count) {
			/*
			 * this return value is as a flag
			 */
			if (fragment) {
				memmove(*index_entries, (*index_entries) + index_max_count_move, (index_max_count - index_max_count_move)*sizeof(AVIndexEntry));
				*index_entries_start += index_max_count_move;
			} else
				return -2;

		}
	}

	(*index_entries_end)++;
	(*nb_index_entries)++;
	index = *index_entries_end - *index_entries_start - 1;
	ie= &(*index_entries)[index];

	ie->pos = pos;
	ie->timestamp = timestamp;
	ie->min_distance= distance;
	ie->size= size;
	ie->flags = flags;

	return index;
}

#define IDX_PER_SECOND			(2)
#define MAX_TIMESTAMP_OFFSET	(0x7fffffff)
static int av_add_index_check_conditions(AVStream* st, int64_t timestamp, int flags, int idx_to_insert, int frame_rate)
{
	int min_offset;
	int timestamp_pre_offset, timestamp_back_offset;

	min_offset = frame_rate/IDX_PER_SECOND;
	timestamp_pre_offset  = 0;
	timestamp_back_offset = 0;

	if(idx_to_insert <= 0) {
		timestamp_back_offset = MAX_TIMESTAMP_OFFSET;
		if(st->nb_index_entries <= 0)
			timestamp_pre_offset = MAX_TIMESTAMP_OFFSET;
		else
			timestamp_pre_offset = timestamp - st->index_entries[st->nb_index_entries-1].timestamp;
	}
	else {
		timestamp_pre_offset  = timestamp - st->index_entries[idx_to_insert-1].timestamp;
		timestamp_back_offset = st->index_entries[idx_to_insert].timestamp - timestamp;
	}

	if(st->codec->codec_type == CODEC_TYPE_AUDIO) {
		if( timestamp_pre_offset >= min_offset &&
			timestamp_back_offset>= min_offset)
			return 1;
	}
	else if(st->codec->codec_type == CODEC_TYPE_VIDEO) {
		if( flags&AVINDEX_KEYFRAME ||
			(timestamp_pre_offset >= min_offset &&
			timestamp_back_offset>= min_offset))
			return 1;
	}

	return 0;
}

int av_add_index_entry(AVStream*  st, int64_t pos, int64_t timestamp, int size, int distance, int flags)
{
	int index;
	int frame_rate;
	AVIndexEntry *entries, *ie;

	if((unsigned)st->nb_index_entries + 1 >= UINT_MAX / sizeof(AVIndexEntry))
		return -1;

	if(!st->time_base.num || !st->time_base.den) {
		gxlogd("\n\n$$$$$$$$$$$$$$ invalid time base!\n\n");
		return -1;
	}
	frame_rate = st->time_base.den/st->time_base.num;

	if (is_relative(timestamp)) //FIXME this maintains previous behavior but we should shift by the correct offset once known
		timestamp -= RELATIVE_TS_BASE;

	entries = av_fast_realloc(st->index_entries,
			&st->index_entries_allocated_size,
			(st->nb_index_entries + 1) *
			sizeof(AVIndexEntry));
	if(!entries){
		gxlogd("av_fast_realloc (%d) = NULL\n", (st->nb_index_entries + 1) * sizeof(AVIndexEntry));
		return -1;
	}
	st->index_entries= entries;

	index = ff_index_search_timestamp(st, st->index_entries, st->nb_index_entries, timestamp, AVSEEK_FLAG_ANY, -1, 0);

	if(av_add_index_check_conditions(st, timestamp, flags, index, frame_rate)) {
		if(index<0) {
				index=st->nb_index_entries++;
				ie= &entries[index];
				ASSERT(index==0 || ie[-1].timestamp < timestamp);
		}
		else {
			ie= &entries[index];
			if(ie->timestamp != timestamp) {
				if(ie->timestamp <= timestamp)
					return -1;
				memmove(entries + index + 1, entries + index, sizeof(AVIndexEntry)*(st->nb_index_entries - index));
				st->nb_index_entries++;
			}
			else if(ie->pos == pos && distance < ie->min_distance) //do not reduce the distance
				distance= ie->min_distance;
		}

		ie->pos = pos;
		ie->timestamp = timestamp;
		ie->min_distance= distance;
		ie->size= size;
		ie->flags = flags;

		return index;
	}
	else
		return 0;
}

int ff_index_search_timestamp(AVStream* st, const AVIndexEntry *entries, int nb_entries,
		int64_t wanted_timestamp, int flags, int64_t tolerance, int ass_is_forward)
{
	int a, b, m;
	int64_t timestamp;

	if (entries == NULL || nb_entries <= 0)
		return -1;

retry:
	a = - 1;
	b = nb_entries;

	//optimize appending index entries at the end
	if(b && entries[b-1].timestamp < wanted_timestamp)
		a = b-1;

	while (b - a > 1) {
		m = (a + b) >> 1;
		timestamp = entries[m].timestamp;
		if(timestamp >= wanted_timestamp)
			b = m;
		if(timestamp <= wanted_timestamp)
			a = m;
	}
	m = (flags & AVSEEK_FLAG_BACKWARD) ? a : b;
	if (nb_entries>1 && wanted_timestamp<=0)
		m = 0;

	if(!(flags & AVSEEK_FLAG_ANY)){
		int mbak = m;
		int64_t found_timestamp = entries[m].timestamp;
		while(m>=0 && m<nb_entries && !(entries[m].flags & AVINDEX_KEYFRAME)){
			m += (flags & AVSEEK_FLAG_BACKWARD) ? -1 : 1;
			if(flags & AVSEEK_FLAG_TOLERANCE && tolerance>=0) {
				if(m>=0 && abs(entries[m].timestamp-found_timestamp)>=tolerance) {
					m = mbak;
					break;
				}
			}
		}
		if ((m < 0) && (flags & AVSEEK_FLAG_BACKWARD)) {
			m = mbak;
		}
		if (ass_is_forward)
			m = st->nb_index_entries;
	}
	if (!ass_is_forward && (m == nb_entries)) {
		if ( !(flags & AVSEEK_FLAG_ANY)) {
			flags |= AVSEEK_FLAG_ANY;
			goto retry;
		}
		return -1;
	}

	return  m;
}

int av_index_search_timestamp(AVStream* st, int64_t wanted_timestamp, int flags)
{
	return ff_index_search_timestamp(st, st->index_entries, st->nb_index_entries,
			wanted_timestamp, flags, st->seek_tolerance, 0);
}

int av_index_search_pos(AVStream*  st, int64_t wanted_pos, int flags)
{
	AVIndexEntry* entries = st->index_entries;
	int nb_entries = st->nb_index_entries;
	int a, b, m;
	int64_t pos;

	a = -1;
	b = nb_entries;

	while (b - a > 1) {
		m = (a + b) >> 1;
		pos = entries[m].pos;
		if (pos >= wanted_pos)
			b = m;
		if (pos <= wanted_pos)
			a = m;
	}
	m = (flags & AVSEEK_FLAG_BACKWARD) ? a : b;

	if (!(flags & AVSEEK_FLAG_ANY)) {
		while (m >= 0 && m < nb_entries && !(entries[m].flags & AVINDEX_KEYFRAME)) {
			m += (flags & AVSEEK_FLAG_BACKWARD) ? -1 : 1;
		}
	}

	if (m == nb_entries)
		return -1;
	return m;
}

int av_seek_frame_binary(AVFormatContext*  s, int stream_index, int64_t target_ts, int flags)
{
	AVInputFormat *avif= s->iformat;
	int64_t av_uninit(pos_min), av_uninit(pos_max), pos, pos_limit;
	int64_t ts_min, ts_max, ts;
	int index;
	int64_t ret;
	AVStream *st;

	if (stream_index < 0)
		return -1;

	av_dlog(s, "read_seek: %d %s\n", stream_index, av_ts2str(target_ts));

	ts_max=
		ts_min= AV_NOPTS_VALUE;
	pos_limit= -1; //gcc falsely says it may be uninitialized

	st= s->streams[stream_index];
	if(st->index_entries){
		AVIndexEntry *e;

		index= av_index_search_timestamp(st, target_ts, flags | AVSEEK_FLAG_BACKWARD); //FIXME whole func must be checked for non-keyframe entries in index case, especially read_timestamp()
		index= FFMAX(index, 0);
		e= &st->index_entries[index];

		if(e->timestamp <= target_ts || e->pos == e->min_distance){
			pos_min= e->pos;
			ts_min= e->timestamp;
			av_dlog(s, "using cached pos_min=0x%lld dts_min=%s\n",
					pos_min, av_ts2str(ts_min));
		}else{
			ASSERT(index==0);
		}

		index= av_index_search_timestamp(st, target_ts, flags & ~AVSEEK_FLAG_BACKWARD);
		ASSERT(index < st->nb_index_entries);
		if(index >= 0){
			e= &st->index_entries[index];
			ASSERT(e->timestamp >= target_ts);
			pos_max= e->pos;
			ts_max= e->timestamp;
			pos_limit= pos_max - e->min_distance;
			av_dlog(s, "using cached pos_max=0x%lld pos_limit=0x%lld dts_max=%s\n",
					pos_max, pos_limit, av_ts2str(ts_max));
		}
	}

	pos= av_gen_search(s, stream_index, target_ts, pos_min, pos_max, pos_limit, ts_min, ts_max, flags, &ts, avif->read_timestamp);
	if(pos<0)
		return -1;

	/* do the seek */
	if ((ret = avio_seek(s->pb, pos, SEEK_SET)) < 0)
		return ret;

	av_read_frame_flush(s);
	av_update_cur_dts(s, st, ts);

	return 0;
}

int64_t av_gen_search(AVFormatContext*  s, int stream_index, int64_t target_ts, int64_t pos_min, int64_t pos_max,
		int64_t pos_limit, int64_t ts_min, int64_t ts_max, int flags, int64_t*  ts_ret,
		int64_t(*read_timestamp) (struct AVFormatContext* , int, int64_t *, int64_t))
{
	int64_t pos, ts;
	int64_t start_pos, filesize;
	int no_change;

	av_dlog(s, "gen_seek: %d %s\n", stream_index, av_ts2str(target_ts));

	if(ts_min == AV_NOPTS_VALUE){
		pos_min = s->data_offset;
		ts_min = read_timestamp(s, stream_index, &pos_min, INT64_MAX);
		if (ts_min == AV_NOPTS_VALUE)
			return -1;
	}

	if(ts_min >= target_ts){
		*ts_ret= ts_min;
		return pos_min;
	}

	if(ts_max == AV_NOPTS_VALUE){
		int step= 1024;
		filesize = avio_size(s->pb);
		pos_max = filesize - 1;
		do{
			pos_max -= step;
			ts_max = read_timestamp(s, stream_index, &pos_max, pos_max + step);
			step += step;
		}while(ts_max == AV_NOPTS_VALUE && pos_max >= step);
		if (ts_max == AV_NOPTS_VALUE)
			return -1;

		for(;;){
			int64_t tmp_pos= pos_max + 1;
			int64_t tmp_ts= read_timestamp(s, stream_index, &tmp_pos, INT64_MAX);
			if(tmp_ts == AV_NOPTS_VALUE)
				break;
			ts_max= tmp_ts;
			pos_max= tmp_pos;
			if(tmp_pos >= filesize)
				break;
		}
		pos_limit= pos_max;
	}

	if(ts_max <= target_ts){
		*ts_ret= ts_max;
		return pos_max;
	}

	if(ts_min > ts_max){
		return -1;
	}else if(ts_min == ts_max){
		pos_limit= pos_min;
	}

	no_change=0;
	while (pos_min < pos_limit) {
		av_dlog(s, "pos_min=0x%lld pos_max=0x%lld dts_min=%s dts_max=%s\n",
				pos_min, pos_max, av_ts2str(ts_min), av_ts2str(ts_max));
		ASSERT(pos_limit <= pos_max);

		if(no_change==0){
			int64_t approximate_keyframe_distance= pos_max - pos_limit;
			// interpolate position (better than dichotomy)
			pos = av_rescale(target_ts - ts_min, pos_max - pos_min, ts_max - ts_min)
				+ pos_min - approximate_keyframe_distance;
		}else if(no_change==1){
			// bisection, if interpolation failed to change min or max pos last time
			pos = (pos_min + pos_limit)>>1;
		}else{
			/* linear search if bisection failed, can only happen if there
			   are very few or no keyframes between min/max */
			pos=pos_min;
		}
		if(pos <= pos_min)
			pos= pos_min + 1;
		else if(pos > pos_limit)
			pos= pos_limit;
		start_pos= pos;

		ts = read_timestamp(s, stream_index, &pos, INT64_MAX); //may pass pos_limit instead of -1
		if(pos == pos_max)
			no_change++;
		else
			no_change=0;
		av_dlog(s, "%lld %lld %lld / %s %s %s target:%s limit:%lld start:%lld noc:%d\n",
				pos_min, pos, pos_max,
				av_ts2str(ts_min), av_ts2str(ts), av_ts2str(ts_max), av_ts2str(target_ts),
				pos_limit, start_pos, no_change);
		if(ts == AV_NOPTS_VALUE){
			av_log(s, AV_LOG_ERROR, "read_timestamp() failed in the middle\n");
			return -1;
		}
		ASSERT(ts != AV_NOPTS_VALUE);
		if (target_ts <= ts) {
			pos_limit = start_pos - 1;
			pos_max = pos;
			ts_max = ts;
		}
		if (target_ts >= ts) {
			pos_min = pos;
			ts_min = ts;
		}
	}

	pos = (flags & AVSEEK_FLAG_BACKWARD) ? pos_min : pos_max;
	ts  = (flags & AVSEEK_FLAG_BACKWARD) ?  ts_min :  ts_max;
#if 0
	pos_min = pos;
	ts_min = read_timestamp(s, stream_index, &pos_min, INT64_MAX);
	pos_min++;
	ts_max = read_timestamp(s, stream_index, &pos_min, INT64_MAX);
	av_dlog(s, "pos=0x%lld %s<=%s<=%s\n",
			pos, av_ts2str(ts_min), av_ts2str(target_ts), av_ts2str(ts_max));
#endif
	*ts_ret= ts;
	return pos;
}

static int seek_frame_byte(AVFormatContext*  s, int stream_index, int64_t pos, int flags)
{
	int64_t pos_min, pos_max;

	pos_min = s->data_offset;
	pos_max = avio_size(s->pb) - 1;

	if     (pos < pos_min) pos= pos_min;
	else if(pos > pos_max) pos= pos_max;

	avio_seek(s->pb, pos, SEEK_SET);

	s->io_repositioned = 1;

	return 0;
}

static int seek_frame_generic(AVFormatContext*  s, int stream_index, int64_t timestamp, int flags)
{
	int index;
	int64_t ret;
	AVStream *st;
	AVIndexEntry *ie;

	st = s->streams[stream_index];

	index = av_index_search_timestamp(st, timestamp, flags);

	if(index < 0 && st->nb_index_entries && timestamp < st->index_entries[0].timestamp)
		return -1;

	if(index < 0 || index==st->nb_index_entries-1){
		AVPacket pkt;
		int nonkey=0;

		if(st->nb_index_entries > 0){
			ASSERT(st->index_entries);
			ie= &st->index_entries[st->nb_index_entries-1];
			if ((ret = avio_seek(s->pb, ie->pos, SEEK_SET)) < 0)
				return ret;
			av_update_cur_dts(s, st, ie->timestamp);
		}else{
			if ((ret = avio_seek(s->pb, s->data_offset, SEEK_SET)) < 0)
				return ret;
		}
		for (;;) {
			int read_status;
			do{
				read_status = av_read_frame(s, &pkt);
			} while (read_status == AVERROR(EAGAIN));
			if (read_status < 0) {
				break;
			}
			if(stream_index == pkt.stream_index && pkt.dts > timestamp){
				if(pkt.flags & AV_PKT_FLAG_KEY) {
					av_free_packet(&pkt);
					break;
				}
				if(nonkey++ > 1000 && st->codec->codec_id != CODEC_ID_CDGRAPHICS){
				//if(nonkey++ > 1000){
					av_free_packet(&pkt);
					gxlogd("seek_frame_generic failed as this stream seems to contain no keyframes after the target timestamp, %d non keyframes found\n", nonkey);
					break;
				}
			}
			av_free_packet(&pkt);
		}
		index = av_index_search_timestamp(st, timestamp, flags);
	}
	if (index < 0)
		return -1;

	av_read_frame_flush(s);
	if (s->iformat->read_seek){
		if(s->iformat->read_seek(s, stream_index, timestamp, flags) >= 0)
			return 0;
	}
	ie = &st->index_entries[index];
	if ((ret = avio_seek(s->pb, ie->pos, SEEK_SET)) < 0)
		return ret;
	av_update_cur_dts(s, st, ie->timestamp);

	return 0;
}

static int seek_frame_internal(AVFormatContext *s, int stream_index,
		int64_t timestamp, int flags)
{
	int ret;
	AVStream *st;

	if (flags & AVSEEK_FLAG_BYTE) {
		if (s->iformat->flags & AVFMT_NO_BYTE_SEEK)
			return -1;
		av_read_frame_flush(s);
		return seek_frame_byte(s, stream_index, timestamp, flags);
	}

	if(stream_index < 0) {
		stream_index= av_find_default_stream_index(s);
		if(stream_index < 0)
			return -1;
		st = s->streams[stream_index];
		if(flags&AVSEEK_FLAG_TOLERANCE && s->seek_tolerance_ms>=0) {
			st->seek_tolerance = s->seek_tolerance_ms * AV_TIME_BASE/1000;
			st->seek_tolerance = av_rescale(st->seek_tolerance, st->time_base.den, AV_TIME_BASE * (int64_t)st->time_base.num);
		}
		/* timestamp for default must be expressed in AV_TIME_BASE units */
		timestamp = av_rescale(timestamp, st->time_base.den, AV_TIME_BASE * (int64_t)st->time_base.num);
	}

	/* first, we try the format specific seek */
	if (s->iformat->read_seek) {
		av_read_frame_flush(s);
		ret = s->iformat->read_seek(s, stream_index, timestamp, flags);
	} else
		ret = -1;

	if (ret >= 0) {
		return 0;
	}

	return ret;

	if (s->iformat->read_timestamp && !(s->iformat->flags & AVFMT_NOBINSEARCH)) {
		av_read_frame_flush(s);
		return av_seek_frame_binary(s, stream_index, timestamp, flags);
	} else if (!(s->iformat->flags & AVFMT_NOGENSEARCH)) {
		av_read_frame_flush(s);
		return seek_frame_generic(s, stream_index, timestamp, flags);
	}
	else
		return -1;
}

int av_seek_frame(AVFormatContext*  s, int stream_index, int64_t timestamp, int flags)
{
	s->target_time = timestamp/AV_TIME_BASE;
	int ret = seek_frame_internal(s, stream_index, timestamp, flags);

	if (ret >= 0)
		queue_attached_pictures(s);

	return ret;
}

int avformat_seek_file(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
{
	if(min_ts > ts || max_ts < ts)
		return -1;

	if (s->iformat->read_seek2) {
		int ret;
		av_read_frame_flush(s);
		ret = s->iformat->read_seek2(s, stream_index, min_ts, ts, max_ts, flags);

		if (ret >= 0)
			queue_attached_pictures(s);
		return ret;
	}

	if(s->iformat->read_timestamp){
		//try to seek via read_timestamp()
	}

	//Fallback to old API if new is not implemented but old is
	//Note the old has somewat different sematics
	if (s->iformat->read_seek || 1) {
		int dir = (ts - min_ts > (uint64_t)(max_ts - ts) ? AVSEEK_FLAG_BACKWARD : 0);
		int ret = av_seek_frame(s, stream_index, ts, flags | dir);
		if (ret<0 && ts != min_ts && max_ts != ts) {
			ret = av_seek_frame(s, stream_index, dir ? max_ts : min_ts, flags | dir);
			if (ret >= 0)
				ret = av_seek_frame(s, stream_index, ts, flags | (dir^AVSEEK_FLAG_BACKWARD));
		}
		return ret;
	}
}
/*******************************************************/

/**
 *  Returns TRUE if the stream has accurate duration in any stream.
 *
 *  @return TRUE if the stream has accurate duration for at least one component.
 */
static int av_has_duration(AVFormatContext*  ic)
{
	int i;
	AVStream *st;

	for(i = 0;i < ic->nb_streams; i++) {
		st = ic->streams[i];
		if (st->duration != AV_NOPTS_VALUE)
			return 1;
	}
	if (ic->duration != AV_NOPTS_VALUE)
		return 1;
	return 0;
}

/**
 *  Estimate the stream timings from the one of each components.
 *
 *  Also computes the global bitrate if possible.
 */
static void update_stream_timings(AVFormatContext*  ic)
{
	int64_t start_time, start_time1, start_time_text, end_time, end_time1;
	int64_t duration, duration1, filesize;
	int i;
	AVStream *st;

	start_time = INT64_MAX;
	start_time_text = INT64_MAX;
	end_time = INT64_MIN;
	duration = INT64_MIN;
	for(i = 0;i < ic->nb_streams; i++) {
		st = ic->streams[i];
		if (st->start_time != AV_NOPTS_VALUE && st->time_base.den) {
			start_time1= av_rescale_q(st->start_time, st->time_base, AV_TIME_BASE_Q);
			if (st->codec->codec_type == CODEC_TYPE_SUBTITLE) {
				if (start_time1 < start_time_text)
					start_time_text = start_time1;
			} else
				start_time = FFMIN(start_time, start_time1);
			if (st->duration != AV_NOPTS_VALUE) {
				end_time1 = start_time1
					+ av_rescale_q(st->duration, st->time_base, AV_TIME_BASE_Q);
				end_time = FFMAX(end_time, end_time1);
			}
		}
		if (st->duration != AV_NOPTS_VALUE) {
			duration1 = av_rescale_q(st->duration, st->time_base, AV_TIME_BASE_Q);
			duration = FFMAX(duration, duration1);
		}
	}
	if (start_time == INT64_MAX || (start_time > start_time_text && start_time - start_time_text < AV_TIME_BASE))
		start_time = start_time_text;
	if (start_time != INT64_MAX) {
		ic->start_time = start_time;
		if (end_time != INT64_MIN)
			duration = FFMAX(duration, end_time - start_time);
	}
	if (duration != INT64_MIN && duration > 0 && ic->duration == AV_NOPTS_VALUE) {
		ic->duration = duration;
	}
	if (ic->pb && (filesize = avio_size(ic->pb)) > 0 && ic->duration != AV_NOPTS_VALUE) {
		/* compute the bitrate */
		ic->bit_rate = (double)filesize * 8.0 * AV_TIME_BASE /
			(double)ic->duration;
	}
}

static void fill_all_stream_timings(AVFormatContext*  ic)
{
	int i;
	AVStream *st;

	update_stream_timings(ic);
	for(i = 0;i < ic->nb_streams; i++) {
		st = ic->streams[i];
		if (st->start_time == AV_NOPTS_VALUE) {
			if(ic->start_time != AV_NOPTS_VALUE)
				st->start_time = av_rescale_q(ic->start_time, AV_TIME_BASE_Q, st->time_base);
			if(ic->duration != AV_NOPTS_VALUE)
				st->duration = av_rescale_q(ic->duration, AV_TIME_BASE_Q, st->time_base);
		}
	}
}

static void estimate_timings_from_bit_rate(AVFormatContext*  ic)
{
	int64_t filesize, duration;
	int bit_rate, i;
	AVStream *st;

	/* if bit_rate is already set, we believe it */
	if (ic->bit_rate <= 0) {
		bit_rate = 0;
		for(i=0;i<ic->nb_streams;i++) {
			st = ic->streams[i];
			if (st->codec->bit_rate > 0)
				bit_rate += st->codec->bit_rate;
		}
		ic->bit_rate = bit_rate;
	}

	/* if duration is already set, we believe it */
	if (ic->duration == AV_NOPTS_VALUE && ic->bit_rate != 0) {
		filesize = ic->pb ? avio_size(ic->pb) : 0;
		if (filesize > 0) {
			for(i = 0; i < ic->nb_streams; i++) {
				st = ic->streams[i];
				duration= av_rescale(8*filesize, st->time_base.den, ic->bit_rate*(int64_t)st->time_base.num);
				if (st->duration == AV_NOPTS_VALUE)
					st->duration = duration;
			}
		}
	}
}

#define DURATION_MAX_READ_SIZE 250000
#define DURATION_MAX_RETRY 4

/* only usable for MPEG-PS streams */
static void estimate_timings_from_pts(AVFormatContext *ic, int64_t old_offset)
{
	AVPacket pkt1, *pkt = &pkt1;
	AVStream *st;
	int read_size, i, ret;
	int64_t end_time;
	int64_t filesize, offset, duration;
	int retry=0;

	/* flush packet queue */
	flush_packet_queue(ic);

	for (i=0; i<ic->nb_streams; i++) {
		st = ic->streams[i];
		if (st->start_time == AV_NOPTS_VALUE && st->first_dts == AV_NOPTS_VALUE)
			gxlogd("start time is not set in estimate_timings_from_pts\n");

		if (st->parser) {
			av_parser_close(st->parser);
			st->parser= NULL;
		}
	}

	/* estimate the end time (duration) */
	/* XXX: may need to support wrapping */
	filesize = ic->pb ? avio_size(ic->pb) : 0;
	end_time = AV_NOPTS_VALUE;
	do{
		offset = filesize - (DURATION_MAX_READ_SIZE<<retry);
		if (offset < 0)
			offset = 0;

		avio_seek(ic->pb, offset, SEEK_SET);
		read_size = 0;
		for(;;) {
			if (read_size >= DURATION_MAX_READ_SIZE<<(FFMAX(retry-1,0)))
				break;

			do {
				ret = ff_read_packet(ic, pkt);
			} while(ret == AVERROR(EAGAIN));
			if (ret != 0)
				break;
			read_size += pkt->size;
			st = ic->streams[pkt->stream_index];
			if (pkt->pts != AV_NOPTS_VALUE &&
					(st->start_time != AV_NOPTS_VALUE ||
					 st->first_dts  != AV_NOPTS_VALUE)) {
				duration = end_time = pkt->pts;
				if (st->start_time != AV_NOPTS_VALUE)
					duration -= st->start_time;
				else
					duration -= st->first_dts;
#if 0
					if (duration < 0)
						duration += 1LL<<st->pts_wrap_bits;
#else
					if (duration > 0) {
						if (st->duration == AV_NOPTS_VALUE || st->duration < duration)
							st->duration = duration;
					}
#endif
			}
			av_free_packet(pkt);
		}
	}while(   end_time==AV_NOPTS_VALUE
			&& filesize > (DURATION_MAX_READ_SIZE<<retry)
			&& ++retry <= DURATION_MAX_RETRY);

	fill_all_stream_timings(ic);

	avio_seek(ic->pb, old_offset, SEEK_SET);
	for (i=0; i<ic->nb_streams; i++) {
		st= ic->streams[i];
		st->cur_dts= st->first_dts;
		st->last_IP_pts = AV_NOPTS_VALUE;
		st->reference_dts = AV_NOPTS_VALUE;
	}
}

static void estimate_timings(AVFormatContext *ic, int64_t old_offset)
{
	int64_t file_size;

	/* get the file size, if possible */
	if (ic->iformat->flags & AVFMT_NOFILE) {
		file_size = 0;
	} else {
		file_size = avio_size(ic->pb);
		file_size = FFMAX(0, file_size);
	}

	if ((!strcmp(ic->iformat->name, "mpeg") || !strcmp(ic->iformat->name, "mpegts")) &&
			file_size && ic->pb->seekable) {
		/* get accurate estimate from the PTSes */
		estimate_timings_from_pts(ic, old_offset);
	} else if (av_has_duration(ic)) {
		/* at least one component has timings - we use them for all
		   the components */
		fill_all_stream_timings(ic);
	} else {
		gxlogd ("Estimating duration from bitrate, this may be inaccurate\n");
		/* less precise: use bitrate info */
		estimate_timings_from_bit_rate(ic);
	}
	update_stream_timings(ic);

	{
		int i;
		AVStream av_unused *st;
		for(i = 0;i < ic->nb_streams; i++) {
			st = ic->streams[i];
			gxlogd ("%d: start_time: %0.3f duration: %0.3f\n", i,
					(double) st->start_time / AV_TIME_BASE,
					(double) st->duration   / AV_TIME_BASE);
		}
		gxlogd ("stream: start_time: %0.3f duration: %0.3f bitrate=%d kb/s\n",
				(double) ic->start_time / AV_TIME_BASE,
				(double) ic->duration   / AV_TIME_BASE,
				ic->bit_rate / 1000);
	}
}

static int has_codec_parameters(AVStream *st)
{
	AVCodecContext *avctx = st->codec;
	int val;
	switch (avctx->codec_type) {
		case CODEC_TYPE_AUDIO:
			//val = avctx->sample_rate && avctx->channels;
			//if (!avctx->frame_size && determinable_frame_size(avctx))
			//	return 0;
			//if (st->info->found_decoder >= 0 && avctx->sample_fmt == SAMPLE_FMT_NONE)
			//	return 0;
			break;
		case CODEC_TYPE_VIDEO:
			//if (!avctx->width)
			//	return 0;
			//if (st->info->found_decoder >= 0 && avctx->pix_fmt == PIX_FMT_NONE)
			//	return 0;
			break;
		case CODEC_TYPE_DATA:
			if(avctx->codec_id == CODEC_ID_NONE) return 1;
		default:
			val = 1;
			break;
	}
		//return avctx->codec_id != CODEC_ID_NONE && val != 0;
		return avctx->codec_id != CODEC_ID_NONE ;
}

#if 0
static int has_decode_delay_been_guessed(AVStream *st)
{
	return (st->codec->codec_id != CODEC_ID_H264 ||
		st->info->nb_decoded_frames >= 6);
}
#endif

/* returns 1 or 0 if or if not decoded data was returned, or a negative error */
static int try_decode_frame(AVStream *st, AVPacket *avpkt, AVDictionary **options)
{
	st->info->found_decoder = -1;
	return 0;
#if 0
//	AVCodec *codec;
	int got_picture = 1, ret = 0;
	AVFrame picture;
	AVPacket pkt = *avpkt;

	if (!avcodec_is_open(st->codec) && !st->info->found_decoder) {
		AVDictionary *thread_opt = NULL;

		codec = st->codec->codec ? st->codec->codec :
			avcodec_find_decoder(st->codec->codec_id);

		if (!codec) {
			st->info->found_decoder = -1;
			return -1;
		}

		/* force thread count to 1 since the h264 decoder will not extract SPS
		 *  and PPS to extradata during multi-threaded decoding */
		av_dict_set(options ? options : &thread_opt, "threads", "1", 0);
		ret = avcodec_open2(st->codec, codec, options ? options : &thread_opt);
		if (!options)
			av_dict_free(&thread_opt);
		if (ret < 0) {
			st->info->found_decoder = -1;
			return ret;
		}
		st->info->found_decoder = 1;
	} else if (!st->info->found_decoder)
		st->info->found_decoder = 1;

	if (st->info->found_decoder < 0)
		return -1;

	while ((pkt.size > 0 || (!pkt.data && got_picture)) &&
			ret >= 0 &&
			(!has_codec_parameters(st)         ||
			 !has_decode_delay_been_guessed(st) ||
			 (!st->codec_info_nb_frames && st->codec->codec->capabilities & CODEC_CAP_CHANNEL_CONF))) {
		got_picture = 0;
		avcodec_get_frame_defaults(&picture);
		switch(st->codec->codec_type) {
			case CODEC_TYPE_VIDEO:
				ret = avcodec_decode_video2(st->codec, &picture,
						&got_picture, &pkt);
				break;
			case CODEC_TYPE_AUDIO:
				ret = avcodec_decode_audio4(st->codec, &picture, &got_picture, &pkt);
				break;
			default:
				break;
		}
		if (ret >= 0) {
			if (got_picture)
				st->info->nb_decoded_frames++;
			pkt.data += ret;
			pkt.size -= ret;
			ret       = got_picture;
		}
	}
	if(!pkt.data && !got_picture)
		return -1;
	return ret;
#endif
}

unsigned int ff_codec_get_tag(const AVCodecTag*  tags, enum CodecID id)
{
	while (tags->id != CODEC_ID_NONE) {
		if (tags->id == id)
			return tags->tag;
		tags++;
	}
	return 0;
}

enum CodecID ff_codec_get_id(const AVCodecTag*  tags, unsigned int tag)
{
	int i;
	for (i = 0; tags[i].id != CODEC_ID_NONE; i++) {
		if (tag == tags[i].tag)
			return tags[i].id;
	}
	for (i = 0; tags[i].id != CODEC_ID_NONE; i++) {
		if (av_toupper((tag >> 0) & 0xFF) == av_toupper((tags[i].tag >> 0) & 0xFF)
				&& av_toupper((tag >> 8) & 0xFF) == av_toupper((tags[i].tag >> 8) & 0xFF)
				&& av_toupper((tag >> 16) & 0xFF) == av_toupper((tags[i].tag >> 16) & 0xFF)
				&& av_toupper((tag >> 24) & 0xFF) == av_toupper((tags[i].tag >> 24) & 0xFF))
			return tags[i].id;
	}
	return CODEC_ID_NONE;
}

unsigned int av_codec_get_tag(const AVCodecTag* const* tags, enum CodecID id)
{
	int i;
	for (i = 0; tags && tags[i]; i++) {
		int tag = ff_codec_get_tag(tags[i], id);
		if (tag)
			return tag;
	}
	return 0;
}

enum CodecID av_codec_get_id(const AVCodecTag* const* tags, unsigned int tag)
{
	int i;
	for(i = 0; tags && tags[i]; i++){
		enum CodecID id = ff_codec_get_id(tags[i], tag);
		if(id != CODEC_ID_NONE)
			return id;
	}
	return 0;
}

static void compute_chapters_end(AVFormatContext *s)
{
	unsigned int i, j;
	int64_t max_time = s->duration + ((s->start_time == AV_NOPTS_VALUE) ? 0 : s->start_time);

	for (i = 0; i < s->nb_chapters; i++)
		if (s->chapters[i]->end == AV_NOPTS_VALUE) {
			AVChapter *ch = s->chapters[i];
			int64_t   end = max_time ? av_rescale_q(max_time, AV_TIME_BASE_Q, ch->time_base)
				: INT64_MAX;

			for (j = 0; j < s->nb_chapters; j++) {
				AVChapter *ch1 = s->chapters[j];
				int64_t next_start = av_rescale_q(ch1->start, ch1->time_base, ch->time_base);
				if (j != i && next_start > ch->start && next_start < end)
					end = next_start;
			}
			ch->end = (end == INT64_MAX) ? ch->start : end;
		}
}

static int get_std_framerate(int i)
{
	if (i < 60*  12)
		return i*  1001;
	else
		return ((int[]) {24, 30, 60, 12, 15})[i - 60*  12] * 1000 * 12;
}

/*
 * Is the time base unreliable.
 * This is a heuristic to balance between quick acceptance of the values in
 * the headers vs. some extra checks.
 * Old DivX and Xvid often have nonsense timebases like 1fps or 2fps.
 * MPEG-2 commonly misuses field repeat flags to store different framerates.
 * And there are "variable" fps files this needs to detect as well.
 */
static int tb_unreliable(AVCodecContext *c){
	if(   c->time_base.den >= 101L*c->time_base.num
			|| c->time_base.den <    5L*c->time_base.num
			/*       || c->codec_tag == AV_RL32("DIVX")
					 || c->codec_tag == AV_RL32("XVID")*/
			|| c->codec_id == CODEC_ID_MPEG2VIDEO
			|| c->codec_id == CODEC_ID_H264
	  )
		return 1;
	return 0;
}

static void set_probesize_and_analyze_duration(AVFormatContext *ic, AVDictionary **options, int probesize, int max_analyze_duration)
{
	ic->probesize = probesize;
	ic->max_analyze_duration=max_analyze_duration;
	if (options && *options) {
		AVDictionaryEntry *e = NULL;
		if ((e = av_dict_get(*options, "ffprobesize", NULL, 0))) {
			ic->probesize = (atoi(e->value)*1024 > 32*1024)?(atoi(e->value)*1024):ic->probesize;
		}
		if ((e = av_dict_get(*options, "analyzeduration", NULL, 0))) {
			ic->max_analyze_duration = ((atoi(e->value)*AV_TIME_BASE)> AV_TIME_BASE)?(atoi(e->value)*AV_TIME_BASE):(AV_TIME_BASE);
		}
	}
}

extern AVRational av_inv_q(AVRational q);
int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options)
{
	int i, count = 0, ret, read_size = 0, j;
	AVStream *st = NULL;
	AVPacket pkt1, *pkt;
	int64_t old_offset = avio_tell(ic->pb);
	int orig_nb_streams = ic->nb_streams;        // new streams might appear, no options for those

	for(i=0;i<ic->nb_streams;i++) {
		st = ic->streams[i];

		if (st->codec->codec_type == CODEC_TYPE_VIDEO ||
				st->codec->codec_type == CODEC_TYPE_SUBTITLE) {
			if(!st->codec->time_base.num)
				st->codec->time_base= st->time_base;
		}
		//only for the split stuff
		if (!st->parser && !(ic->flags & AVFMT_FLAG_NOPARSE)) {
			st->parser = av_parser_init(st->codec->codec_id);
			if(st->parser){
				if(st->need_parsing == AVSTREAM_PARSE_HEADERS){
					st->parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
				} else if(st->need_parsing == AVSTREAM_PARSE_FULL_RAW) {
					st->parser->flags |= PARSER_FLAG_USE_CODEC_TS;
				}
			}
		}
#if 0
		codec = st->codec->codec ? st->codec->codec :
			avcodec_find_decoder(st->codec->codec_id);

		/* force thread count to 1 since the h264 decoder will not extract SPS
		 *  and PPS to extradata during multi-threaded decoding */
		av_dict_set(options ? &options[i] : &thread_opt, "threads", "1", 0);

		/* Ensure that subtitle_header is properly set. */
		if (st->codec->codec_type == CODEC_TYPE_SUBTITLE
				&& codec && !st->codec->codec)
			avcodec_open2(st->codec, codec, options ? &options[i]
					: &thread_opt);

		//try to just open decoders, in case this is enough to get parameters
		if (!has_codec_parameters(st)) {
			if (codec && !st->codec->codec)
				avcodec_open2(st->codec, codec, options ? &options[i]
						: &thread_opt);
		}
		if (!options)
			av_dict_free(&thread_opt);
#endif
	}

	for (i=0; i<ic->nb_streams; i++) {
		ic->streams[i]->info->last_dts = AV_NOPTS_VALUE;
		ic->streams[i]->info->fps_first_dts = AV_NOPTS_VALUE;
		ic->streams[i]->info->fps_last_dts  = AV_NOPTS_VALUE;
	}

	for(;;) {
		if (url_check_interrupt_cb()){
			ret= AVERROR_EXIT;
			gxlogd ("interrupted\n");
			break;
		}

		/* check if one codec still needs to be handled */
		for(i=0;i<ic->nb_streams;i++) {
				//int fps_analyze_framecount = 20;

			st = ic->streams[i];
			if (!has_codec_parameters(st))
				break;
#if 0
			/* if the timebase is coarse (like the usual millisecond precision
			   of mkv), we need to analyze more frames to reliably arrive at
			   the correct fps */
			if (av_q2d(st->time_base) > 0.0005)
				fps_analyze_framecount *= 2;
			if (ic->fps_probe_size >= 0)
				fps_analyze_framecount = ic->fps_probe_size;
			/* variable fps and no guess at the real fps */
			if(   tb_unreliable(st->codec) && !(st->r_frame_rate.num && st->avg_frame_rate.num)
					&& st->info->duration_count < fps_analyze_framecount
					&& st->codec->codec_type == CODEC_TYPE_VIDEO)
				break;
			if(st->parser && st->parser->parser->split && !st->codec->extradata)
				break;
			if (st->first_dts == AV_NOPTS_VALUE &&
					(st->codec->codec_type == CODEC_TYPE_VIDEO ||
					 st->codec->codec_type == CODEC_TYPE_AUDIO))
				break;
#endif
		}

		if (i == ic->nb_streams) {
			/*is mpegts type,and duration==-1 and get av codec_type,retry login avformat_find_stream_info function,get duration.*/
			/* NOTE: if the format has no header, then we need to read
			   some packets to get most of the streams, so we cannot
			   stop here */
			if (!(ic->ctx_flags & AVFMTCTX_NOHEADER)) {
				if ((ic->duration == -1)
					&& (i ==ic->nb_streams)
					&& ic->iformat
					&& !strcasecmp(ic->iformat->name, "mpegts")) {
					set_probesize_and_analyze_duration(ic, options, PROBE_BUF_MAX/8, AV_TIME_BASE/3);
				} else if ((ic->nb_streams > 0)
						&& (st && st->codec && st->codec->codec_id != CODEC_ID_NONE)
						&& (ic->iformat && !strcasecmp(ic->iformat->name, "hls"))) {
						set_probesize_and_analyze_duration(ic, options, 32*1024, AV_TIME_BASE/3);
				} else if ((1 == ic->nb_streams)
						&& (st && st->codec->codec_type == CODEC_TYPE_AUDIO)
						&& (ic->iformat && !strcasecmp(ic->iformat->name, "mp3"))) {
					gxlogd ("probe mp3 format.\n");
				} else if ((1 == ic->nb_streams)
						&& (st && st->codec->codec_type == CODEC_TYPE_AUDIO)
						&& (ic->iformat && !strcasecmp(ic->iformat->name, "aac"))) {
					set_probesize_and_analyze_duration(ic, options, 12*1024, AV_TIME_BASE/3);
				} else {
					if (ic->nobuffer & AVFMT_FLAG_QUICK_START) {
						/* if we found the info for all the codecs, we can stop */
						ret = count;
						gxlogd ("All info found\n");
						break;
					} else {
						set_probesize_and_analyze_duration(ic, options, PROBE_BUF_MAX/3, 2*AV_TIME_BASE);
					}
				}
			} else if ((ic->nb_streams > 0)
						&& (st && st->codec && st->codec->codec_id != CODEC_ID_NONE)
						&& (ic->iformat && !strcasecmp(ic->iformat->name, "hls"))) {
						set_probesize_and_analyze_duration(ic, options, 32*1024, AV_TIME_BASE/3);
				}
		}

		/* we did not get all the codec info, but we read too much data */
		if (read_size >= ic->probesize) {
			ret = count;
			gxlogd("Probe buffer size limit %d(%d) reached\n", read_size, ic->probesize);
			for (i = 0; i < ic->nb_streams; i++)
				if (!ic->streams[i]->r_frame_rate.num &&
						ic->streams[i]->info->duration_count <= 1)
					gxlogd("Stream #%d: not enough frames to estimate rate;consider increasing probesize\n", i);
			break;
		}

		/* NOTE: a new stream can be added there if no header in file
		   (AVFMTCTX_NOHEADER) */
		ret = read_frame_internal(ic, &pkt1);
		if (ret < 0) {
			/* EOF or error*/
			for (i = 0; i < ic->nb_streams; i++) {
				st = ic->streams[i];
				if (!has_codec_parameters(st)) {
					gxlogf("Can't Find stream information\n");
				} else {
					ret = 0;
				}
			}
			break;
		}

		if (ic->nobuffer & AVFMT_FLAG_NOBUFFER) {
			pkt = &pkt1;
		} else {
			pkt= add_to_pktbuf(&ic->packet_buffer, &pkt1, &ic->packet_buffer_end);
			if ((ret = av_dup_packet(pkt)) < 0)
				goto find_stream_info_err;
#if 0
			if(flag++>=ic->nb_streams){
				for (i = 0; i < ic->nb_streams; i++) {
					st = ic->streams[i];
					if (st->codec->codec_id == CODEC_ID_NONE)
						break;
				}
				if(i >= ic->nb_streams)
					break;
			}
#endif
		}
			read_size += pkt->size;
			st = ic->streams[pkt->stream_index];
			if (pkt->dts != AV_NOPTS_VALUE && st->codec_info_nb_frames > 1) {
				/* check for non-increasing dts */
				if (st->info->fps_last_dts != AV_NOPTS_VALUE &&
						st->info->fps_last_dts >= pkt->dts) {
					st->info->fps_first_dts =
						st->info->fps_last_dts  = AV_NOPTS_VALUE;
				}
				/* Check for a discontinuity in dts. If the difference in dts
				 * is more than 1000 times the average packet duration in the
				 * sequence, we treat it as a discontinuity. */
				if (st->info->fps_last_dts != AV_NOPTS_VALUE &&
						st->info->fps_last_dts_idx > st->info->fps_first_dts_idx &&
						(pkt->dts - st->info->fps_last_dts) / 1000 >
						(st->info->fps_last_dts     - st->info->fps_first_dts) /
						(st->info->fps_last_dts_idx - st->info->fps_first_dts_idx)) {
					st->info->fps_first_dts =
						st->info->fps_last_dts  = AV_NOPTS_VALUE;
				}

				/* update stored dts values */
				if (st->info->fps_first_dts == AV_NOPTS_VALUE) {
					st->info->fps_first_dts     = pkt->dts;
					st->info->fps_first_dts_idx = st->codec_info_nb_frames;
				}
				st->info->fps_last_dts     = pkt->dts;
				st->info->fps_last_dts_idx = st->codec_info_nb_frames;
			}
			if (st->codec_info_nb_frames>1) {
				if ((st->codec && st->codec->codec_id != CODEC_ID_NONE) && (ic->nobuffer & AVFMT_FLAG_QUICK_START)) {
					break;
				}
				int64_t t = 0;
				if (st->time_base.den > 0)
					t = av_rescale_q(st->info->codec_info_duration, st->time_base, AV_TIME_BASE_Q);
				if (st->avg_frame_rate.num > 0)
					t = FFMAX(t, av_rescale_q(st->codec_info_nb_frames, av_inv_q(st->avg_frame_rate), AV_TIME_BASE_Q));

				if (   t == 0
						&& st->codec_info_nb_frames>30
						&& st->info->fps_first_dts != AV_NOPTS_VALUE
						&& st->info->fps_last_dts  != AV_NOPTS_VALUE) {
					t = FFMAX(t, av_rescale_q(st->info->fps_last_dts - st->info->fps_first_dts, st->time_base, AV_TIME_BASE_Q));
				}
				if (t >= ic->max_analyze_duration) {
					if (ic->nobuffer & AVFMT_FLAG_NOBUFFER) {
						av_free_packet(&pkt1);
					}
					gxlogd("Probe limit read_size:%d(%d)..%lld(%lld) max_analyze_duration\n", read_size, ic->probesize, t, ic->max_analyze_duration);
					break;
				}
				if (pkt->duration) {
					st->info->codec_info_duration        += pkt->duration;
					//st->info->codec_info_duration_fields += st->parser && st->need_parsing && st->codec->ticks_per_frame ==2 ? st->parser->repeat_pict + 1 : 2;
				}
			}

#if 0
			if (st->codec_info_nb_frames>1) {
				int64_t t=0;
				if (st->time_base.den > 0)
					t = av_rescale_q(st->info->codec_info_duration, st->time_base, AV_TIME_BASE_Q);
				if (st->avg_frame_rate.num > 0)
					t = FFMAX(t, av_rescale_q(st->codec_info_nb_frames, (AVRational){st->avg_frame_rate.den, st->avg_frame_rate.num}, AV_TIME_BASE_Q));

				if (t > ic->max_analyze_duration) {
					av_log(ic, AV_LOG_WARNING, "max_analyze_duration %d reached at %lld\n", ic->max_analyze_duration, t);
					break;
				}
				st->info->codec_info_duration += pkt->duration;
			}
			if(pkt->duration)
			{
				int64_t last = st->info->last_dts;

			if(pkt->dts != AV_NOPTS_VALUE && last != AV_NOPTS_VALUE && pkt->dts > last){
				double dts= (is_relative(pkt->dts) ?  pkt->dts - RELATIVE_TS_BASE : pkt->dts) * av_q2d(st->time_base);
				int64_t duration= pkt->dts - last;

				//                 if(st->codec->codec_type == CODEC_TYPE_VIDEO)
				//                     av_log(NULL, AV_LOG_ERROR, "%f\n", dts);
				for (i=1; i<FF_ARRAY_ELEMS(st->info->duration_error[0][0]); i++) {
					int framerate= get_std_framerate(i);
					double sdts= dts*framerate/(1001*12);
					for(j=0; j<2; j++){
						int ticks= lrintf(sdts+j*0.5);
						double error= sdts - ticks + j*0.5;
						st->info->duration_error[j][0][i] += error;
						st->info->duration_error[j][1][i] += error*error;
					}
				}
				st->info->duration_count++;
				// ignore the first 4 values, they might have some random jitter
				if (st->info->duration_count > 3)
					st->info->duration_gcd = ff_gcd(st->info->duration_gcd, duration);
			}
			if (last == AV_NOPTS_VALUE || st->info->duration_count <= 1)
				st->info->last_dts = pkt->dts;
		}
#endif
		if ((st->need_parsing || (st->parser && st->parser->parser->split)) && !st->codec->extradata) {
			int i = 0;
			if (st->need_parsing == AVSTREAM_PARSE_HEVC) {
				i = hevc_split(st->codec, pkt->data, pkt->size);
			} else if(st->parser && st->parser->parser && st->parser->parser->split) {
				i = st->parser->parser->split(st->codec, pkt->data, pkt->size);
			}
			if (i > 0 && i < FF_MAX_EXTRADATA_SIZE) {
				st->codec->extradata_size= i;
				st->codec->extradata= av_malloc(st->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
				if (!st->codec->extradata)
					return AVERROR(ENOMEM);
				memcpy(st->codec->extradata, pkt->data, st->codec->extradata_size);
				memset(st->codec->extradata + i, 0, FF_INPUT_BUFFER_PADDING_SIZE);
			}
		}

		/* if still no information, we try to open the codec and to
		   decompress the frame. We try to avoid that in most cases as
		   it takes longer and uses more memory. For MPEG-4, we need to
		   decompress for QuickTime.

		   If CODEC_CAP_CHANNEL_CONF is set this will force decoding of at
		   least one frame of codec data, this makes sure the codec initializes
		   the channel configuration and does not only trust the values from the container.
		   */
		try_decode_frame(st, pkt, (options && i < orig_nb_streams ) ? &options[i] : NULL);
		if (ic->nobuffer & AVFMT_FLAG_NOBUFFER) {
			av_free_packet(&pkt1);
		}
		st->codec_info_nb_frames++;
		count++;
	}

//	if (flush_codecs) {
//		AVPacket empty_pkt = { 0 };
//		int err = 0;
//		av_init_packet(&empty_pkt);
//
//		ret = -1; /* we could not have all the codec parameters before EOF */
//		for(i=0;i<ic->nb_streams;i++) {
//			st = ic->streams[i];
//
//			/* flush the decoders */
//			if (st->info->found_decoder == 1) {
//				do {
//					err = try_decode_frame(st, &empty_pkt,
//							(options && i < orig_nb_streams) ?
//							&options[i] : NULL);
//				} while (err > 0 && !has_codec_parameters(st));
//
//				if (err < 0) {
//					av_log(ic, AV_LOG_INFO,
//							"decoding for stream %d failed\n", st->index);
//				}
//			}
//
//			if (!has_codec_parameters(st)){
//				char buf[256];
//				avcodec_string(buf, sizeof(buf), st->codec, 0);
//				av_log(ic, AV_LOG_WARNING,
//						"Could not find codec parameters (%s)\n", buf);
//			} else {
//				ret = 0;
//			}
//		}
//	}

	// close codecs which were opened in try_decode_frame()
//	for(i=0;i<ic->nb_streams;i++) {
//		st = ic->streams[i];
//		avcodec_close(st->codec);
//	}
	for(i=0;i<ic->nb_streams;i++) {
		st = ic->streams[i];
		if (st->codec->codec_type == CODEC_TYPE_VIDEO) {
			if(st->codec->codec_id == CODEC_ID_RAWVIDEO && !st->codec->codec_tag && !st->codec->bits_per_coded_sample){
				uint32_t tag= avcodec_pix_fmt_to_codec_tag(st->codec->pix_fmt);
//				if(ff_find_pix_fmt(ff_raw_pixelFormatTags, tag) == st->codec->pix_fmt)
					st->codec->codec_tag= tag;
			}

			if (st->codec_info_nb_frames>2 && !st->avg_frame_rate.num && st->info->codec_info_duration)
				av_reduce(&st->avg_frame_rate.num, &st->avg_frame_rate.den,
						(st->codec_info_nb_frames-2)*(int64_t)st->time_base.den,
						st->info->codec_info_duration*(int64_t)st->time_base.num, 60000);
			// the check for tb_unreliable() is not completely correct, since this is not about handling
			// a unreliable/inexact time base, but a time base that is finer than necessary, as e.g.
			// ipmovie.c produces.
			if (tb_unreliable(st->codec) && st->info->duration_count > 15 && st->info->duration_gcd > FFMAX(1, st->time_base.den/(500LL*st->time_base.num)) && !st->r_frame_rate.num)
				av_reduce(&st->r_frame_rate.num, &st->r_frame_rate.den, st->time_base.den, st->time_base.num * st->info->duration_gcd, INT_MAX);
			if (st->info->duration_count && !st->r_frame_rate.num
					&& tb_unreliable(st->codec) /*&&
					//FIXME we should not special-case MPEG-2, but this needs testing with non-MPEG-2 ...
					st->time_base.num*duration_sum[i]/st->info->duration_count*101LL > st->time_base.den*/){
				int num = 0;
				double best_error= 0.01;

				for (j=1; j<FF_ARRAY_ELEMS(st->info->duration_error[0][0]); j++) {
					int k;

					if(st->info->codec_info_duration && st->info->codec_info_duration*av_q2d(st->time_base) < (1001*12.0)/get_std_framerate(j))
						continue;
					if(!st->info->codec_info_duration && 1.0 < (1001*12.0)/get_std_framerate(j))
						continue;
					for(k=0; k<2; k++){
						int n= st->info->duration_count;
						double a= st->info->duration_error[k][0][j] / n;
						double error= st->info->duration_error[k][1][j]/n - a*a;

						if(error < best_error && best_error> 0.000000001){
							best_error= error;
							num = get_std_framerate(j);
						}
//						if(error < 0.02)
//							av_log(NULL, AV_LOG_DEBUG, "rfps: %f %f\n", get_std_framerate(j) / 12.0/1001, error);
					}
				}
				// do not increase frame rate by more than 1 % in order to match a standard rate.
				if (num && (!st->r_frame_rate.num || (double)num/(12*1001) < 1.01 * av_q2d(st->r_frame_rate)))
					av_reduce(&st->r_frame_rate.num, &st->r_frame_rate.den, num, 12*1001, INT_MAX);
			}

			if (!st->r_frame_rate.num){
				if(    st->codec->time_base.den * (int64_t)st->time_base.num
						<= st->codec->time_base.num * st->codec->ticks_per_frame * (int64_t)st->time_base.den){
					st->r_frame_rate.num = st->codec->time_base.den;
					st->r_frame_rate.den = st->codec->time_base.num * st->codec->ticks_per_frame;
				}else{
					st->r_frame_rate.num = st->time_base.den;
					st->r_frame_rate.den = st->time_base.num;
				}
			}
		}else if(st->codec->codec_type == CODEC_TYPE_AUDIO) {
			if(!st->codec->bits_per_coded_sample)
				st->codec->bits_per_coded_sample= av_get_bits_per_sample(st->codec->codec_id);
			// set stream disposition based on audio service type
			switch (st->codec->audio_service_type) {
				case AV_AUDIO_SERVICE_TYPE_EFFECTS:
					st->disposition = AV_DISPOSITION_CLEAN_EFFECTS;    break;
				case AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED:
					st->disposition = AV_DISPOSITION_VISUAL_IMPAIRED;  break;
				case AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED:
					st->disposition = AV_DISPOSITION_HEARING_IMPAIRED; break;
				case AV_AUDIO_SERVICE_TYPE_COMMENTARY:
					st->disposition = AV_DISPOSITION_COMMENT;          break;
				case AV_AUDIO_SERVICE_TYPE_KARAOKE:
					st->disposition = AV_DISPOSITION_KARAOKE;          break;
				default:
					break;
			}
		}
	}

	estimate_timings(ic, old_offset);

	compute_chapters_end(ic);

find_stream_info_err:
	for (i=0; i < ic->nb_streams; i++) {
		av_freep(&ic->streams[i]->info);
	}
	return ret;
}

AVProgram *av_find_program_from_stream(AVFormatContext *ic, AVProgram *last, int s)
{
	int i, j;

	for (i = 0; i < ic->nb_programs; i++) {
		if (ic->programs[i] == last) {
			last = NULL;
		} else {
			if (!last)
				for (j = 0; j < ic->programs[i]->nb_stream_indexes; j++)
					if (ic->programs[i]->stream_index[j] == s)
						return ic->programs[i];
		}
	}
	return NULL;
}

int av_find_best_stream(AVFormatContext *ic,
		enum CodecType type,
		int wanted_stream_nb,
		int related_stream,
		void **decoder_ret,
		int flags)
{
	int i, nb_streams = ic->nb_streams;
	int ret = AVERROR_STREAM_NOT_FOUND, best_count = -1;
	unsigned *program = NULL;
//	AVCodec *decoder = NULL, *best_decoder = NULL;

	if (related_stream >= 0 && wanted_stream_nb < 0) {
		AVProgram *p = av_find_program_from_stream(ic, NULL, related_stream);
		if (p) {
			program = p->stream_index;
			nb_streams = p->nb_stream_indexes;
		}
	}
	for (i = 0; i < nb_streams; i++) {
		int real_stream_index = program ? program[i] : i;
		AVStream *st = ic->streams[real_stream_index];
		AVCodecContext *avctx = st->codec;
		if (avctx->codec_type != type)
			continue;
		if (wanted_stream_nb >= 0 && real_stream_index != wanted_stream_nb)
			continue;
		if (st->disposition & (AV_DISPOSITION_HEARING_IMPAIRED|AV_DISPOSITION_VISUAL_IMPAIRED))
			continue;
//		if (decoder_ret) {
//			decoder = avcodec_find_decoder(st->codec->codec_id);
//			if (!decoder) {
//				if (ret < 0)
//					ret = AVERROR_DECODER_NOT_FOUND;
//				continue;
//			}
//		}
		if (best_count >= st->codec_info_nb_frames)
			continue;
		best_count = st->codec_info_nb_frames;
		ret = real_stream_index;
//		best_decoder = decoder;
		if (program && i == nb_streams - 1 && ret < 0) {
			program = NULL;
			nb_streams = ic->nb_streams;
			i = 0; /* no related stream found, try again with everything */
		}
	}
//	if (decoder_ret)
//		*decoder_ret = best_decoder;
	return ret;
}

/*******************************************************/

int av_read_play(AVFormatContext*  s)
{
	if (s->iformat->read_play)
		return s->iformat->read_play(s);
	if (s->pb)
		return url_pause(s->pb, 0);
	return AVERROR(ENOSYS);
}

int av_read_pause(AVFormatContext*  s)
{
	if (s->iformat->read_pause)
		return s->iformat->read_pause(s);
	if (s->pb)
		return url_pause(s->pb, 1);
	return AVERROR(ENOSYS);
}

void free_stream(AVFormatContext *s, AVStream **pst)
{
	AVStream *st = *pst;
	int i = 0;

	if (!st)
		return;

	for (i = 0; i < st->nb_side_data; i++) {
		if (st->side_data && st->side_data[i].data) {
			av_freep(&st->side_data[i].data);
		}
	}
	if (st->side_data)
		av_freep(&st->side_data);
	if (st->probe_data.buf)
		av_freep(&st->probe_data.buf);

	if (st->parser) {
		av_parser_close(st->parser);
	}
	if (st->attached_pic.data)
		av_free_packet(&st->attached_pic);
	av_dict_free(&st->metadata);
	if (st->index_entries)
		av_freep(&st->index_entries);
	if (st->codec->extradata)
		av_freep(&st->codec->extradata);
	if (st->codec)
		av_freep(&st->codec);
	if (s->iformat) {
		if(strcmp(s->iformat->name,"mov")){
			if (st->priv_data)
				av_freep(&st->priv_data);
		}else{
			MOVStreamContext* sc=(MOVStreamContext*)(st->priv_data);
			if(NULL!=sc){
				if (sc->chunk_offsets)
					av_freep(&sc->chunk_offsets);
				if (sc->stsc_data)
					av_freep(&sc->stsc_data);
				if (sc->sample_sizes)
					av_freep(&sc->sample_sizes);
				if (sc->keyframes)
					av_freep(&sc->keyframes);
				if (sc->stts_data)
					av_freep(&sc->stts_data);
				if (sc->stps_data)
					av_freep(&sc->stps_data);
				if (sc->elst_data)
					av_freep(&sc->elst_data);
			}
			if (st->priv_data)
				av_freep(&st->priv_data);
		}
	}
	if (st->info)
		av_freep(&st->info);
	if (st)
		av_freep(&st);
}

void avformat_free_context(AVFormatContext *s)
{
	int i;
	AVStream *st;

	av_opt_free(s);
	for (i = 0; i < s->nb_streams; i++) {
		free_stream(s, &s->streams[i]);
	}

	for(i=s->nb_programs-1; i>=0; i--) {
		if (s->programs && *(s->programs)) {
			av_dict_free(&s->programs[i]->metadata);
			av_freep(&s->programs[i]->stream_index);
			av_freep(&s->programs[i]->provider_name);
			av_freep(&s->programs[i]->name);
			av_freep(&s->programs[i]);
		}
	}
	av_freep(&s->programs);
	av_freep(&s->priv_data);
	while(s->nb_chapters--) {
		av_dict_free(&s->chapters[s->nb_chapters]->metadata);
		av_freep(&s->chapters[s->nb_chapters]);
	}
	if (s->chapters)
		av_freep(&s->chapters);
	if (s->streams)
		av_freep(&s->streams);
	if (s->url)
		av_freep(&s->url);
	if (s->metadata)
		av_dict_free(&s->metadata);
	if (s->url_options) {
		av_dict_free(&s->url_options);
	}
	av_free(s);
}

void avformat_close_input(AVFormatContext **ps)
{
	AVFormatContext *s = *ps;
	ByteIOContext *pb = (s->iformat && (s->iformat->flags & AVFMT_NOFILE)) || (s->flags & AVFMT_FLAG_CUSTOM_IO) ?
		NULL : s->pb;

	if (s->cur_st && s->cur_st->parser)
		av_free_packet(&s->cur_pkt);

	flush_packet_queue(s);
	if (s->iformat && (s->iformat->read_close))
		s->iformat->read_close(s);
	if (s->metadata) {
		av_dict_free(&s->metadata);
		s->metadata = NULL;
	}
	if (s->url_options) {
		av_dict_free(&(s->url_options));
	}
	if (s->id3v2_meta) {
		av_dict_free(&(s->id3v2_meta));
	}
	avformat_free_context(s);
	*ps = NULL;
	if (pb) {
		avio_flush(pb);
		url_fclose(pb);
	}
}

AVStream *av_new_stream(AVFormatContext *s, int id)
{
	AVStream *st = avformat_new_stream(s, NULL);
	if (st)
		st->id = id;
	return st;
}

AVStream *avformat_new_stream(AVFormatContext *s, void *c)
{
	AVStream *st;
	AVStream **streams;
	int i, media_max_streams = 0;
	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);

	if (s->nb_streams >= media_max_streams) {
		gxloge ("exceeds default values,fail fail.(default:%d)\n", media_max_streams);
		return NULL;
	}

	streams = av_realloc_array(s->streams, s->nb_streams + 1, sizeof(*streams));
	if (!streams)
		return NULL;
	s->streams = streams;

	st = av_mallocz(sizeof(AVStream));
	if (!st) {
		if (s->streams) {
			av_free(s->streams);
			s->streams = NULL;
		}
		return NULL;
	}

	if (!(st->info = av_mallocz(sizeof(*st->info)))) {
		if (st) {
			av_free(st);
			st = NULL;
		}
		if (s->streams) {
			av_free(s->streams);
			s->streams = NULL;
		}
		return NULL;
	}
	st->info->last_dts = AV_NOPTS_VALUE;

	st->codec = avcodec_alloc_context();
	if (!st->codec) {
		if (st->info) {
			av_free(st->info);
			st->info = NULL;
		}
		if (st) {
			av_free(st);
			st = NULL;
		}
		if (s->streams) {
			av_free(s->streams);
			s->streams = NULL;
		}
		return NULL;
	}
	if (s->iformat) {
		/* no default bitrate if decoding */
		st->codec->bit_rate = 0;
	}
	st->index = s->nb_streams;
	st->start_time = AV_NOPTS_VALUE;
	st->duration = AV_NOPTS_VALUE;
	/* we set the current DTS to 0 so that formats without any timestamps
	   but durations get some timestamps, formats with some unknown
	   timestamps have their first few packets buffered and the
	   timestamps corrected before they are returned to the user */
	st->cur_dts = s->iformat ? RELATIVE_TS_BASE : 0;
	st->first_dts = AV_NOPTS_VALUE;
	st->probe_packets = MAX_PROBE_PACKETS;
	st->seek_tolerance = -1;

	/* default pts setting is MPEG-like */
	avpriv_set_pts_info(st, 33, 1, 90000);
	st->last_IP_pts = AV_NOPTS_VALUE;
	for(i=0; i<MAX_REORDER_DELAY+1; i++)
		st->pts_buffer[i]= AV_NOPTS_VALUE;
	st->reference_dts = AV_NOPTS_VALUE;

	st->sample_aspect_ratio = (AVRational){0,1};

	s->streams[s->nb_streams++] = st;
	return st;
}

AVProgram *av_new_program(AVFormatContext *ac, int id)
{
	AVProgram *program=NULL;
	int i;

	for(i=0; i<ac->nb_programs; i++) {
		if(ac->programs[i]->id == id)
			program = ac->programs[i];
	}

	if(!program){
		program = av_mallocz(sizeof(AVProgram));
		if (!program)
			return NULL;
		dynarray_add(&ac->programs, (int*)&ac->nb_programs, program);
		program->discard = AVDISCARD_NONE;
	}
	program->id = id;

	return program;
}

AVChapter *new_chapter(AVFormatContext *s, int id,
		AVRational time_base,int64_t start, int64_t end, const char *title)
{
	AVChapter *chapter = NULL;
	int i;

	for(i=0; i<s->nb_chapters; i++)
		if(s->chapters[i]->id == id)
			chapter = s->chapters[i];

	if(!chapter){
		chapter= av_mallocz(sizeof(AVChapter));
		if(!chapter)
			return NULL;
		dynarray_add(&s->chapters, (int*)&s->nb_chapters, chapter);
	}
	av_dict_set((AVDictionary**)&chapter->metadata, "title", title, 0);
	chapter->id    = id;
	chapter->time_base= time_base;
	chapter->start = start;
	chapter->end   = end;

	return chapter;
}

/************************************************************/
/* output media file */

int avformat_alloc_output_context2(AVFormatContext **avctx, AVOutputFormat *oformat,
		const char *format, const char *filename)
{
	AVFormatContext *s = avformat_alloc_context();
	int ret = 0;

	*avctx = NULL;
	if (!s)
		goto nomem;

	if (!oformat) {
		if (format) {
			oformat = av_guess_format(format, NULL, NULL);
			if (!oformat) {
				ret = AVERROR(EINVAL);
				goto error;
			}
		} else {
			oformat = av_guess_format(NULL, filename, NULL);
			if (!oformat) {
				ret = AVERROR(EINVAL);
				goto error;
			}
		}
	}

	s->oformat = oformat;
	if (s->oformat->priv_data_size > 0) {
		s->priv_data = av_mallocz(s->oformat->priv_data_size);
		if (!s->priv_data)
			goto nomem;
		if (s->oformat->priv_class) {
			*(const AVClass**)s->priv_data= s->oformat->priv_class;
			av_opt_set_defaults(s->priv_data);
		}
	} else
		s->priv_data = NULL;

	if (filename) {
		if (!(s->url = av_strdup(filename)))
			goto nomem;
	}

	*avctx = s;
	return 0;
nomem:
	gxloge ("Out of memory\n");
	ret = AVERROR(ENOMEM);
error:
	avformat_free_context(s);
	return ret;
}

AVFormatContext *avformat_alloc_output_context(const char *format,
		AVOutputFormat *oformat, const char *filename)
{
	AVFormatContext *avctx;
	int ret = avformat_alloc_output_context2(&avctx, oformat, format, filename);
	return ret < 0 ? NULL : avctx;
}

#if 0
static int validate_codec_tag(AVFormatContext *s, AVStream *st)
{
	const AVCodecTag *avctag;
	int n;
	enum CodecID id = CODEC_ID_NONE;
	unsigned int tag = 0;

	/**
	 * Check that tag + id is in the table
	 * If neither is in the table -> OK
	 * If tag is in the table with another id -> FAIL
	 * If id is in the table with another tag -> FAIL unless strict < normal
	 */
	for (n = 0; s->oformat->codec_tag[n]; n++) {
		avctag = s->oformat->codec_tag[n];
		while (avctag->id != CODEC_ID_NONE) {
			if (av_toupper4(avctag->tag) == av_toupper4(st->codec->codec_tag)) {
				id = avctag->id;
				if (id == st->codec->codec_id)
					return 1;
			}
			if (avctag->id == st->codec->codec_id)
				tag = avctag->tag;
			avctag++;
		}
	}
	if (id != CODEC_ID_NONE)
		return 0;
//	if (tag && (st->codec->strict_std_compliance >= FF_COMPLIANCE_NORMAL))
////		return 0;
	return 1;
}
#endif

int avformat_write_header(AVFormatContext *s, AVDictionary **options)
{
	int ret = 0; //i;
	//AVStream *st;
	AVDictionary *tmp = NULL;

	if (options)
		av_dict_copy(&tmp, *options, 0);
	if ((ret = av_opt_set_dict(s, &tmp)) < 0)
		goto fail;
	if (s->priv_data && s->oformat->priv_class && *(const AVClass**)s->priv_data==s->oformat->priv_class &&
			(ret = av_opt_set_dict(s->priv_data, &tmp)) < 0)
		goto fail;

	// some sanity checks
	if (s->nb_streams == 0 && !(s->oformat->flags & AVFMT_NOSTREAMS)) {
		av_log(s, AV_LOG_ERROR, "no streams\n");
		ret = AVERROR(EINVAL);
		goto fail;
	}

#if 0
	for(i=0;i<s->nb_streams;i++) {
		st = s->streams[i];

		switch (st->codec->codec_type) {
			case CODEC_TYPE_AUDIO:
				if(st->codec->sample_rate<=0){
					av_log(s, AV_LOG_ERROR, "sample rate not set\n");
					ret = AVERROR(EINVAL);
					goto fail;
				}
				if(!st->codec->block_align)
					st->codec->block_align = st->codec->channels *
						av_get_bits_per_sample(st->codec->codec_id) >> 3;
				break;
			case CODEC_TYPE_VIDEO:
				if(st->codec->time_base.num<=0 || st->codec->time_base.den<=0){ //FIXME audio too?
					av_log(s, AV_LOG_ERROR, "time base not set\n");
					ret = AVERROR(EINVAL);
					goto fail;
				}
				if((st->codec->width<=0 || st->codec->height<=0) && !(s->oformat->flags & AVFMT_NODIMENSIONS)){
					av_log(s, AV_LOG_ERROR, "dimensions not set\n");
					ret = AVERROR(EINVAL);
					goto fail;
				}
				if(av_cmp_q(st->sample_aspect_ratio, st->codec->sample_aspect_ratio)
						&& FFABS(av_q2d(st->sample_aspect_ratio) - av_q2d(st->codec->sample_aspect_ratio)) > 0.004*av_q2d(st->sample_aspect_ratio)
				  ){
					av_log(s, AV_LOG_ERROR, "Aspect ratio mismatch between muxer "
							"(%d/%d) and encoder layer (%d/%d)\n",
							st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
							st->codec->sample_aspect_ratio.num,
							st->codec->sample_aspect_ratio.den);
					ret = AVERROR(EINVAL);
					goto fail;
				}
				break;
			default:
				break;
		}

		if(s->oformat->codec_tag){
			if(   st->codec->codec_tag
					&& st->codec->codec_id == CODEC_ID_RAWVIDEO
					&& (av_codec_get_tag(s->oformat->codec_tag, st->codec->codec_id) == 0 || av_codec_get_tag(s->oformat->codec_tag, st->codec->codec_id) ==MKTAG('r', 'a', 'w', ' '))
					&& !validate_codec_tag(s, st)){
				//the current rawvideo encoding system ends up setting the wrong codec_tag for avi/mov, we override it here
				st->codec->codec_tag= 0;
			}
			if(st->codec->codec_tag){
				if (!validate_codec_tag(s, st)) {
//					char tagbuf[32], cortag[32];
//					av_get_codec_tag_string(tagbuf, sizeof(tagbuf), st->codec->codec_tag);
//					av_get_codec_tag_string(cortag, sizeof(cortag), av_codec_get_tag(s->oformat->codec_tag, st->codec->codec_id));
//					av_log(s, AV_LOG_ERROR,
//							"Tag %s/0x%08x incompatible with output codec id '%d' (%s)\n",
//							tagbuf, st->codec->codec_tag, st->codec->codec_id, cortag);
					ret = AVERROR_INVALIDDATA;
					goto fail;
				}
			}else
				st->codec->codec_tag= av_codec_get_tag(s->oformat->codec_tag, st->codec->codec_id);
		}

		if(s->oformat->flags & AVFMT_GLOBALHEADER &&
				!(st->codec->flags & CODEC_FLAG_GLOBAL_HEADER))
			av_log(s, AV_LOG_WARNING, "Codec for stream %d does not use global headers but container format requires global headers\n", i);
	}
#endif

	if (!s->priv_data && s->oformat->priv_data_size > 0) {
		s->priv_data = av_mallocz(s->oformat->priv_data_size);
		if (!s->priv_data) {
			ret = AVERROR(ENOMEM);
			goto fail;
		}
		if (s->oformat->priv_class) {
			*(const AVClass**)s->priv_data= s->oformat->priv_class;
			av_opt_set_defaults(s->priv_data);
			if ((ret = av_opt_set_dict(s->priv_data, &tmp)) < 0)
				goto fail;
		}
	}

	/* set muxer identification string */
	if (s->nb_streams && !(s->streams[0]->codec->flags & CODEC_FLAG_BITEXACT)) {
		av_dict_set((AVDictionary**)&s->metadata, "encoder", LIBAVFORMAT_IDENT, 0);
	}

	if(s->oformat->write_header){
		ret = s->oformat->write_header(s);
		if (ret < 0)
			goto fail;
	}

#if 0
	/* init PTS generation */
	for(i=0;i<s->nb_streams;i++) {
		int64_t den = AV_NOPTS_VALUE;
		st = s->streams[i];

		switch (st->codec->codec_type) {
			case CODEC_TYPE_AUDIO:
				den = (int64_t)st->time_base.num * st->codec->sample_rate;
				break;
			case CODEC_TYPE_VIDEO:
				den = (int64_t)st->time_base.num * st->codec->time_base.den;
				break;
			default:
				break;
		}
		if (den != AV_NOPTS_VALUE) {
			if (den <= 0) {
				ret = AVERROR_INVALIDDATA;
				goto fail;
			}
			frac_init(&st->pts, 0, 0, den);
		}
	}
#endif

	if (options) {
		av_dict_free(options);
		*options = tmp;
	}
	return 0;
fail:
	av_dict_free(&tmp);
	return ret;
}

//FIXME merge with compute_pkt_fields
static int compute_pkt_fields2(AVFormatContext *s, AVStream *st, AVPacket *pkt)
{
	int delay = FFMAX(st->codec->has_b_frames, !!st->codec->max_b_frames);
	int num, den, frame_size, i;

	/* duration field */
	if (pkt->duration == 0) {
		compute_frame_duration(&num, &den, st, NULL, pkt);
		if (den && num) {
			pkt->duration = av_rescale(1, num * (int64_t)st->time_base.den * st->codec->ticks_per_frame, den * (int64_t)st->time_base.num);
		}
	}

	if(pkt->pts == AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE && delay==0)
		pkt->pts= pkt->dts;

	//XXX/FIXME this is a temporary hack until all encoders output pts
	if((pkt->pts == 0 || pkt->pts == AV_NOPTS_VALUE) && pkt->dts == AV_NOPTS_VALUE && !delay){
		static int warned;
		if (!warned) {
			warned = 1;
		}
		pkt->dts=
			//        pkt->pts= st->cur_dts;
			pkt->pts= st->pts.val;
	}

	//calculate dts from pts
	if(pkt->pts != AV_NOPTS_VALUE && pkt->dts == AV_NOPTS_VALUE && delay <= MAX_REORDER_DELAY){
		st->pts_buffer[0]= pkt->pts;
		for(i=1; i<delay+1 && st->pts_buffer[i] == AV_NOPTS_VALUE; i++)
			st->pts_buffer[i]= pkt->pts + (i-delay-1) * pkt->duration;
		for(i=0; i<delay && st->pts_buffer[i] > st->pts_buffer[i+1]; i++)
			FFSWAP(int64_t, st->pts_buffer[i], st->pts_buffer[i+1]);

		pkt->dts= st->pts_buffer[0];
	}

	if (st->cur_dts && st->cur_dts != AV_NOPTS_VALUE &&
			((!(s->oformat->flags & AVFMT_TS_NONSTRICT) &&
			  st->cur_dts >= pkt->dts) || st->cur_dts > pkt->dts)) {
		return AVERROR(EINVAL);
	}
	if(pkt->dts != AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE && pkt->pts < pkt->dts){
		return AVERROR(EINVAL);
	}

	//    av_log(s, AV_LOG_DEBUG, "av_write_frame: pts2:%s dts2:%s\n", av_ts2str(pkt->pts), av_ts2str(pkt->dts));
	st->cur_dts= pkt->dts;
	st->pts.val= pkt->dts;

	/* update pts */
	switch (st->codec->codec_type) {
		case CODEC_TYPE_AUDIO:
			frame_size = get_audio_frame_size(st->codec, pkt->size, 1);

			/* HACK/FIXME, we skip the initial 0 size packets as they are most
			   likely equal to the encoder delay, but it would be better if we
			   had the real timestamps from the encoder */
			if (frame_size >= 0 && (pkt->size || st->pts.num!=st->pts.den>>1 || st->pts.val)) {
				frac_add(&st->pts, (int64_t)st->time_base.den * frame_size);
			}
			break;
		case CODEC_TYPE_VIDEO:
			frac_add(&st->pts, (int64_t)st->time_base.den * st->codec->time_base.num);
			break;
		default:
			break;
	}
	return 0;
}

int av_write_frame(AVFormatContext *s, AVPacket *pkt)
{
	int ret;

	if (!pkt) {
		if (s->oformat->flags & AVFMT_ALLOW_FLUSH)
			return s->oformat->write_packet(s, pkt);
		return 1;
	}

//	ret = compute_pkt_fields2(s, s->streams[pkt->stream_index], pkt);
//
//	if(ret<0 && !(s->oformat->flags & AVFMT_NOTIMESTAMPS))
//		return ret;

	ret= s->oformat->write_packet(s, pkt);

	if (ret >= 0)
		s->streams[pkt->stream_index]->nb_frames++;
	return ret;
}

#define CHUNK_START 0x1000

int ff_interleave_add_packet(AVFormatContext *s, AVPacket *pkt,
		int (*compare)(AVFormatContext *, AVPacket *, AVPacket *))
{
	AVPacketList **next_point, *this_pktl;
	AVStream *st= s->streams[pkt->stream_index];
	int chunked= s->max_chunk_size || s->max_chunk_duration;

	this_pktl = av_mallocz(sizeof(AVPacketList));
	if (!this_pktl)
		return AVERROR(ENOMEM);
	this_pktl->pkt= *pkt;
	pkt->destruct= NULL;             // do not free original but only the copy
	av_dup_packet(&this_pktl->pkt);  // duplicate the packet if it uses non-alloced memory

	if(s->streams[pkt->stream_index]->last_in_packet_buffer){
		next_point = &(st->last_in_packet_buffer->next);
	}else{
		next_point = &s->packet_buffer;
	}

	if(*next_point){
		if(chunked){
			uint64_t max= av_rescale_q(s->max_chunk_duration, AV_TIME_BASE_Q, st->time_base);
			if(   st->interleaver_chunk_size     + pkt->size     <= s->max_chunk_size-1U
					&& st->interleaver_chunk_duration + pkt->duration <= max-1U){
				st->interleaver_chunk_size     += pkt->size;
				st->interleaver_chunk_duration += pkt->duration;
				goto next_non_null;
			}else{
				st->interleaver_chunk_size     =
					st->interleaver_chunk_duration = 0;
				this_pktl->pkt.flags |= CHUNK_START;
			}
		}

		if(compare(s, &s->packet_buffer_end->pkt, pkt)){
			while(   *next_point
					&& ((chunked && !((*next_point)->pkt.flags&CHUNK_START))
						|| !compare(s, &(*next_point)->pkt, pkt))){
				next_point= &(*next_point)->next;
			}
			if(*next_point)
				goto next_non_null;
		}else{
			next_point = &(s->packet_buffer_end->next);
		}
	}
	ASSERT(!*next_point);

	s->packet_buffer_end= this_pktl;
next_non_null:

	this_pktl->next= *next_point;

	s->streams[pkt->stream_index]->last_in_packet_buffer=
		*next_point= this_pktl;
	return 0;
}

static int ff_interleave_compare_dts(AVFormatContext *s, AVPacket *next, AVPacket *pkt)
{
	AVStream *st = s->streams[ pkt ->stream_index];
	AVStream *st2= s->streams[ next->stream_index];
	int comp = av_compare_ts(next->dts, st2->time_base, pkt->dts,
			st->time_base);
	if(s->audio_preload && ((st->codec->codec_type == CODEC_TYPE_AUDIO) != (st2->codec->codec_type == CODEC_TYPE_AUDIO))){
		int64_t ts = av_rescale_q(pkt ->dts, st ->time_base, AV_TIME_BASE_Q) - s->audio_preload*(st ->codec->codec_type == CODEC_TYPE_AUDIO);
		int64_t ts2= av_rescale_q(next->dts, st2->time_base, AV_TIME_BASE_Q) - s->audio_preload*(st2->codec->codec_type == CODEC_TYPE_AUDIO);
		if(ts == ts2){
			ts= ( pkt ->dts* st->time_base.num*AV_TIME_BASE - s->audio_preload*(int64_t)(st ->codec->codec_type == CODEC_TYPE_AUDIO)* st->time_base.den)*st2->time_base.den
				-( next->dts*st2->time_base.num*AV_TIME_BASE - s->audio_preload*(int64_t)(st2->codec->codec_type == CODEC_TYPE_AUDIO)*st2->time_base.den)* st->time_base.den;
			ts2=0;
		}
		comp= (ts>ts2) - (ts<ts2);
	}

	if (comp == 0)
		return pkt->stream_index < next->stream_index;
	return comp > 0;
}

int ff_interleave_packet_per_dts(AVFormatContext *s, AVPacket *out,
		AVPacket *pkt, int flush)
{
	AVPacketList *pktl;
	int stream_count=0, noninterleaved_count=0;
	int64_t delta_dts_max = 0;
	int i, ret;

	if(pkt){
		ret = ff_interleave_add_packet(s, pkt, ff_interleave_compare_dts);
		if (ret < 0)
			return ret;
	}

	for(i=0; i < s->nb_streams; i++) {
		if (s->streams[i]->last_in_packet_buffer) {
			++stream_count;
		} else if(s->streams[i]->codec->codec_type == CODEC_TYPE_SUBTITLE) {
			++noninterleaved_count;
		}
	}

	if (s->nb_streams == stream_count) {
		flush = 1;
	} else if (!flush){
		for(i=0; i < s->nb_streams; i++) {
			if (s->streams[i]->last_in_packet_buffer) {
				int64_t delta_dts =
					av_rescale_q(s->streams[i]->last_in_packet_buffer->pkt.dts,
							s->streams[i]->time_base,
							AV_TIME_BASE_Q) -
					av_rescale_q(s->packet_buffer->pkt.dts,
							s->streams[s->packet_buffer->pkt.stream_index]->time_base,
							AV_TIME_BASE_Q);
				delta_dts_max= FFMAX(delta_dts_max, delta_dts);
			}
		}
		if(s->nb_streams == stream_count+noninterleaved_count &&
				delta_dts_max > 20*AV_TIME_BASE) {
			av_log(s, AV_LOG_DEBUG, "flushing with %d noninterleaved\n", noninterleaved_count);
			flush = 1;
		}
	}
	if(stream_count && flush){
		pktl= s->packet_buffer;
		*out= pktl->pkt;

		s->packet_buffer= pktl->next;
		if(!s->packet_buffer)
			s->packet_buffer_end= NULL;

		if(s->streams[out->stream_index]->last_in_packet_buffer == pktl)
			s->streams[out->stream_index]->last_in_packet_buffer= NULL;
		av_freep(&pktl);
		return 1;
	}else{
		av_init_packet(out);
		return 0;
	}
}

int av_interleave_packet_per_dts(AVFormatContext *s, AVPacket *out,
		AVPacket *pkt, int flush)
{
	return ff_interleave_packet_per_dts(s, out, pkt, flush);
}

static int interleave_packet(AVFormatContext *s, AVPacket *out, AVPacket *in, int flush){
	if (s->oformat->interleave_packet) {
		int ret = s->oformat->interleave_packet(s, out, in, flush);
		if (in)
			av_free_packet(in);
		return ret;
	} else
		return ff_interleave_packet_per_dts(s, out, in, flush);
}

int av_interleaved_write_frame(AVFormatContext *s, AVPacket *pkt){
	int ret, flush = 0;

	if (pkt) {
		AVStream *st= s->streams[ pkt->stream_index];

		//FIXME/XXX/HACK drop zero sized packets
		if(st->codec->codec_type == CODEC_TYPE_AUDIO && pkt->size==0)
			return 0;

		if((ret = compute_pkt_fields2(s, st, pkt)) < 0 && !(s->oformat->flags & AVFMT_NOTIMESTAMPS))
			return ret;

		if(pkt->dts == AV_NOPTS_VALUE && !(s->oformat->flags & AVFMT_NOTIMESTAMPS))
			return AVERROR(EINVAL);
	} else {
		av_dlog(s, "av_interleaved_write_frame FLUSH\n");
		flush = 1;
	}

	for(;;){
		AVPacket opkt;
		int ret= interleave_packet(s, &opkt, pkt, flush);
		if(ret<=0) //FIXME cleanup needed for ret<0 ?
			return ret;

		ret= s->oformat->write_packet(s, &opkt);
		if (ret >= 0)
			s->streams[opkt.stream_index]->nb_frames++;

		av_free_packet(&opkt);
		pkt= NULL;

		if(ret<0)
			return ret;
		if(s->pb && s->pb->error)
			return s->pb->error;
	}
}

int av_write_trailer(AVFormatContext *s)
{
	int ret, i;

	for(;;){
		AVPacket pkt;
		ret= interleave_packet(s, &pkt, NULL, 1);
		if(ret<0) //FIXME cleanup needed for ret<0 ?
			goto fail;
		if(!ret)
			break;

		ret= s->oformat->write_packet(s, &pkt);
		if (ret >= 0)
			s->streams[pkt.stream_index]->nb_frames++;

		av_free_packet(&pkt);

		if(ret<0)
			goto fail;
		if(s->pb && s->pb->error)
			goto fail;
	}

	if(s->oformat->write_trailer)
		ret = s->oformat->write_trailer(s);
fail:
	if (s->pb)
		put_flush(s->pb);
	if(ret == 0)
		ret = s->pb ? s->pb->error : 0;
	for(i=0;i<s->nb_streams;i++) {
		av_freep(&s->streams[i]->priv_data);
		av_freep(&s->streams[i]->index_entries);
	}
	if (s->oformat->priv_class)
		av_opt_free(s->priv_data);
	av_freep(&s->priv_data);
	return ret;
}

int av_get_output_timestamp(struct AVFormatContext *s, int stream,
		int64_t *dts, int64_t *wall)
{
//	if (!s->oformat || !s->oformat->get_output_timestamp)
//		return AVERROR(ENOSYS);
//	s->oformat->get_output_timestamp(s, stream, dts, wall);
	return 0;
}

void av_program_add_stream_index(AVFormatContext *ac, int progid, unsigned int idx)
{
	int i, j;
	AVProgram *program=NULL;
	void *tmp;

	if (idx >= ac->nb_streams) {
		gxlogd ("stream index %d is not valid.\n", idx);
		return;
	}

	for(i=0; i<ac->nb_programs; i++){
		if(ac->programs[i]->id != progid)
			continue;
		program = ac->programs[i];
		for(j=0; j<program->nb_stream_indexes; j++)
			if(program->stream_index[j] == idx)
				return;

		tmp = av_realloc(program->stream_index, sizeof(unsigned int)*(program->nb_stream_indexes+1));
		if(!tmp)
			return;
		program->stream_index = tmp;
		program->stream_index[program->nb_stream_indexes++] = idx;
		return;
	}
}

#if 0
static void print_fps(double d, const char *postfix){
	uint64_t v= lrintf(d*100);
	if     (v% 100      ) av_log(NULL, AV_LOG_INFO, ", %3.2f %s", d, postfix);
	else if(v%(100*1000)) av_log(NULL, AV_LOG_INFO, ", %1.0f %s", d, postfix);
	else                  av_log(NULL, AV_LOG_INFO, ", %1.0fk %s", d/1000, postfix);
}

static void dump_metadata(void *ctx, AVDictionary *m, const char *indent)
{
	if(m && !(m->count == 1 &&  av_dict_get(m, "language", NULL, 0))){
		AVDictionaryEntry *tag=NULL;

		av_log(ctx, AV_LOG_INFO, "%sMetadata:\n", indent);
		while((tag=av_dict_get(m, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			if(strcmp("language", tag->key)){
				const char *p = tag->value;
				av_log(ctx, AV_LOG_INFO, "%s  %-16s: ", indent, tag->key);
				while(*p) {
					char tmp[256];
					size_t len = strcspn(p, "\xd\xa");
					av_strlcpy(tmp, p, FFMIN(sizeof(tmp), len+1));
					av_log(ctx, AV_LOG_INFO, "%s", tmp);
					p += len;
					if (*p == 0xd) av_log(ctx, AV_LOG_INFO, " ");
					if (*p == 0xa) av_log(ctx, AV_LOG_INFO, "\n%s  %-16s: ", indent, "");
					if (*p) p++;
				}
				av_log(ctx, AV_LOG_INFO, "\n");
			}
		}
	}
}

/* "user interface" functions */
static void dump_stream_format(AVFormatContext *ic, int i, int index, int is_output)
{
//	char buf[256];
	int flags = (is_output ? ic->oformat->flags : ic->iformat->flags);
	AVStream *st = ic->streams[i];
//	int g = av_gcd(st->time_base.num, st->time_base.den);
	AVDictionaryEntry *lang = av_dict_get((AVDictionary*)st->metadata, "language", NULL, 0);
//	avcodec_string(buf, sizeof(buf), st->codec, is_output);
	av_log(NULL, AV_LOG_INFO, "    Stream #%d:%d", index, i);
	/* the pid is an important information, so we display it */
	/* XXX: add a generic system */
	if (flags & AVFMT_SHOW_IDS)
		av_log(NULL, AV_LOG_INFO, "[0x%x]", st->id);
	if (lang)
		av_log(NULL, AV_LOG_INFO, "(%s)", lang->value);
//	av_log(NULL, AV_LOG_DEBUG, ", %d, %d/%d", st->codec_info_nb_frames, st->time_base.num/g, st->time_base.den/g);
//	av_log(NULL, AV_LOG_INFO, ": %s", buf);
	if (st->sample_aspect_ratio.num && // default
			av_cmp_q(st->sample_aspect_ratio, st->codec->sample_aspect_ratio)) {
		AVRational display_aspect_ratio;
		av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
				st->codec->width*st->sample_aspect_ratio.num,
				st->codec->height*st->sample_aspect_ratio.den,
				1024*1024);
		av_log(NULL, AV_LOG_INFO, ", SAR %d:%d DAR %d:%d",
				st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
				display_aspect_ratio.num, display_aspect_ratio.den);
	}
	if(st->codec->codec_type == CODEC_TYPE_VIDEO){
		if(st->avg_frame_rate.den && st->avg_frame_rate.num)
			print_fps(av_q2d(st->avg_frame_rate), "fps");
		if(st->r_frame_rate.den && st->r_frame_rate.num)
			print_fps(av_q2d(st->r_frame_rate), "tbr");
		if(st->time_base.den && st->time_base.num)
			print_fps(1/av_q2d(st->time_base), "tbn");
		if(st->codec->time_base.den && st->codec->time_base.num)
			print_fps(1/av_q2d(st->codec->time_base), "tbc");
	}
	if (st->disposition & AV_DISPOSITION_DEFAULT)
		av_log(NULL, AV_LOG_INFO, " (default)");
	if (st->disposition & AV_DISPOSITION_DUB)
		av_log(NULL, AV_LOG_INFO, " (dub)");
	if (st->disposition & AV_DISPOSITION_ORIGINAL)
		av_log(NULL, AV_LOG_INFO, " (original)");
	if (st->disposition & AV_DISPOSITION_COMMENT)
		av_log(NULL, AV_LOG_INFO, " (comment)");
	if (st->disposition & AV_DISPOSITION_LYRICS)
		av_log(NULL, AV_LOG_INFO, " (lyrics)");
	if (st->disposition & AV_DISPOSITION_KARAOKE)
		av_log(NULL, AV_LOG_INFO, " (karaoke)");
	if (st->disposition & AV_DISPOSITION_FORCED)
		av_log(NULL, AV_LOG_INFO, " (forced)");
	if (st->disposition & AV_DISPOSITION_HEARING_IMPAIRED)
		av_log(NULL, AV_LOG_INFO, " (hearing impaired)");
	if (st->disposition & AV_DISPOSITION_VISUAL_IMPAIRED)
		av_log(NULL, AV_LOG_INFO, " (visual impaired)");
	if (st->disposition & AV_DISPOSITION_CLEAN_EFFECTS)
		av_log(NULL, AV_LOG_INFO, " (clean effects)");
	av_log(NULL, AV_LOG_INFO, "\n");
	dump_metadata(NULL, (AVDictionary*)st->metadata, "    ");
}

void av_dump_format(AVFormatContext *ic,
		int index,
		const char *url,
		int is_output)
{
	int i;
	uint8_t *printed = ic->nb_streams ? av_mallocz(ic->nb_streams) : NULL;
	if (ic->nb_streams && !printed)
		return;

	dump_metadata(NULL, (AVDictionary*)ic->metadata, "  ");
	if (!is_output) {
		av_log(NULL, AV_LOG_INFO, "  Duration: ");
		if (ic->duration != AV_NOPTS_VALUE) {
			int hours, mins, secs, us;
			secs = ic->duration / AV_TIME_BASE;
			us = ic->duration % AV_TIME_BASE;
			mins = secs / 60;
			secs %= 60;
			hours = mins / 60;
			mins %= 60;
			av_log(NULL, AV_LOG_INFO, "%02d:%02d:%02d.%02d", hours, mins, secs,
					(100 * us) / AV_TIME_BASE);
		} else {
			av_log(NULL, AV_LOG_INFO, "N/A");
		}
		if (ic->start_time != AV_NOPTS_VALUE) {
			int secs, us;
			av_log(NULL, AV_LOG_INFO, ", start: ");
			secs = ic->start_time / AV_TIME_BASE;
			us = abs(ic->start_time % AV_TIME_BASE);
			av_log(NULL, AV_LOG_INFO, "%d.%06d",
					secs, (int)av_rescale(us, 1000000, AV_TIME_BASE));
		}
		av_log(NULL, AV_LOG_INFO, ", bitrate: ");
		if (ic->bit_rate) {
			av_log(NULL, AV_LOG_INFO,"%d kb/s", ic->bit_rate / 1000);
		} else {
			av_log(NULL, AV_LOG_INFO, "N/A");
		}
		av_log(NULL, AV_LOG_INFO, "\n");
	}
	for (i = 0; i < ic->nb_chapters; i++) {
		AVChapter *ch = ic->chapters[i];
		av_log(NULL, AV_LOG_INFO, "    Chapter #%d.%d: ", index, i);
		av_log(NULL, AV_LOG_INFO, "start %f, ", ch->start * av_q2d(ch->time_base));
		av_log(NULL, AV_LOG_INFO, "end %f\n",   ch->end   * av_q2d(ch->time_base));

		dump_metadata(NULL, (AVDictionary*)ch->metadata, "    ");
	}
	if(ic->nb_programs) {
		int j, k, total = 0;
		for(j=0; j<ic->nb_programs; j++) {
//			AVDictionaryEntry *name = av_dict_get(ic->programs[j]->metadata,
//					"name", NULL, 0);
//			av_log(NULL, AV_LOG_INFO, "  Program %d %s\n", ic->programs[j]->id,
//					name ? name->value : "");
			dump_metadata(NULL, (AVDictionary*)ic->programs[j]->metadata, "    ");
			for(k=0; k<ic->programs[j]->nb_stream_indexes; k++) {
				dump_stream_format(ic, ic->programs[j]->stream_index[k], index, is_output);
				printed[ic->programs[j]->stream_index[k]] = 1;
			}
			total += ic->programs[j]->nb_stream_indexes;
		}
		if (total < ic->nb_streams)
			av_log(NULL, AV_LOG_INFO, "  No Program\n");
	}
	for(i=0;i<ic->nb_streams;i++)
		if (!printed[i])
			dump_stream_format(ic, i, index, is_output);

	av_free(printed);
}
#endif

uint64_t GetExternalNetTimeCallback(void)
{
    extern GxExternalNetTimeCallback external_net_time_callback;
    if (external_net_time_callback) {
        return external_net_time_callback();
    }
    return 0;
}

uint64_t av_gettime(void)
{
    uint64_t time_us = 0;
    if ((time_us = GetExternalNetTimeCallback()) > 0) {
        return (((uint64_t)time_us)*1000000);
    } else {
        time_t t1 = time(NULL);
        time_us = (((uint64_t)t1)*1000000);
    }
    return time_us;
}

uint64_t av_ntp_time(void)
{
	return (uint64_t)(av_gettime() / 1000) * 1000 + NTP_OFFSET_US;
}

#define UINT64_C_HLS(c) (c ## ULL)
int64_t gx_av_gettime_relative(void)
{
    return av_gettime() + 42 * 60 * 60 * UINT64_C_HLS(1000000);
}

int av_get_frame_filename(char *buf, int buf_size,
		const char *path, int number)
{
	const char *p;
	char *q, buf1[20], c;
	int nd, len, percentd_found;

	q = buf;
	p = path;
	percentd_found = 0;
	for(;;) {
		c = *p++;
		if (c == '\0')
			break;
		if (c == '%') {
			do {
				nd = 0;
				while (isdigit(*p)) {
					nd = nd * 10 + *p++ - '0';
				}
				c = *p++;
			} while (isdigit(c));

			switch(c) {
				case '%':
					goto addchar;
				case 'd':
					if (percentd_found)
						goto fail;
					percentd_found = 1;
					snprintf(buf1, sizeof(buf1), "%0*d", nd, number);
					len = strlen(buf1);
					if ((q - buf + len) > buf_size - 1)
						goto fail;
					memcpy(q, buf1, len);
					q += len;
					break;
				default:
					goto fail;
			}
		} else {
addchar:
			if ((q - buf) < buf_size - 1)
				*q++ = c;
		}
	}
	if (!percentd_found)
		goto fail;
	*q = '\0';
	return 0;
fail:
	*q = '\0';
	return -1;
}

static void hex_dump_internal(void *avcl, FILE *f, int level, uint8_t *buf, int size)
{
	int len, i, j, c;

	for(i=0;i<size;i+=16) {
		len = size - i;
		if (len > 16)
			len = 16;
		gxlogd("%08x ", i);
		for(j=0;j<16;j++) {
			if (j < len)
				gxlogd(" %02x", buf[i+j]);
			else
				gxlogd("   ");
		}
		gxlogd(" ");
		for(j=0;j<len;j++) {
			c = buf[i+j];
			if (c < ' ' || c > '~')
				c = '.';
			gxlogd("%c", c);
		}
		gxlogd("\n");
	}
}

void av_hex_dump(FILE *f, uint8_t *buf, int size)
{
	hex_dump_internal(NULL, f, 0, buf, size);
}

void av_hex_dump_log(void *avcl, int level, uint8_t *buf, int size)
{
	hex_dump_internal(avcl, NULL, level, buf, size);
}

static void pkt_dump_internal(void *avcl, FILE *f, int level, AVPacket *pkt, int dump_payload, AVRational time_base)
{
	gxlogd("stream #%d:\n", pkt->stream_index);
	gxlogd("  keyframe=%d\n", ((pkt->flags & AV_PKT_FLAG_KEY) != 0));
	gxlogd("  duration=%0.3f\n", pkt->duration * av_q2d(time_base));
	/* DTS is _always_ valid after av_read_frame() */
	gxlogd("  dts=");
	if (pkt->dts == AV_NOPTS_VALUE)
		gxlogd("N/A");
	else
		gxlogd("%0.3f", pkt->dts * av_q2d(time_base));
	/* PTS may not be known if B-frames are present. */
	gxlogd("  pts=");
	if (pkt->pts == AV_NOPTS_VALUE)
		gxlogd("N/A");
	else
		gxlogd("%0.3f", pkt->pts * av_q2d(time_base));
	gxlogd("\n");
	gxlogd("  size=%d\n", pkt->size);
	if (dump_payload)
		av_hex_dump(f, pkt->data, pkt->size);
}

void av_pkt_dump(FILE *f, AVPacket *pkt, int dump_payload)
{
	AVRational tb = { 1, AV_TIME_BASE };
	pkt_dump_internal(NULL, f, 0, pkt, dump_payload, tb);
}

void av_pkt_dump2(FILE *f, AVPacket *pkt, int dump_payload, AVStream *st)
{
	pkt_dump_internal(NULL, f, 0, pkt, dump_payload, st->time_base);
}

void av_pkt_dump_log(void *avcl, int level, AVPacket *pkt, int dump_payload)
{
	AVRational tb = { 1, AV_TIME_BASE };
	pkt_dump_internal(avcl, NULL, level, pkt, dump_payload, tb);
}

void av_pkt_dump_log2(void *avcl, int level, AVPacket *pkt, int dump_payload,
		AVStream *st)
{
	pkt_dump_internal(avcl, NULL, level, pkt, dump_payload, st->time_base);
}

void av_url_split(char* proto, int proto_size, char* authorization, int authorization_size,
     char* hostname, int hostname_size, int *port_ptr, char *path, int path_size, const char *url)
{
	const char *p, *ls, *at, *at2, *col, *brk;

	if (port_ptr)
		*port_ptr = -1;
	if (proto_size > 0)
		proto[0] = 0;
	if (authorization_size > 0)
		authorization[0] = 0;
	if (hostname_size > 0)
		hostname[0] = 0;
	if (path_size > 0)
		path[0] = 0;

	/* parse protocol */
	if (url && (p = strchr(url, ':'))) {
		av_strlcpy(proto, url, FFMIN(proto_size, p + 1 - url));
		p++; /* skip ':' */
		if (*p == '/')
			p++;
		if (*p == '/')
			p++;
	} else {
		/* no protocol means plain filename */
		if (url) {
			av_strlcpy(path, url, path_size);
		}
		return;
	}

	/* separate path from hostname */
	ls = p + strcspn(p, "/?#");
	av_strlcpy(path, ls, path_size);

	/* the rest is hostname, use that to parse auth/port */
	if (ls != p) {
		/* authorization (user[:pass]@hostname) */
		at2 = p;
		while ((at = strchr(p, '@')) && at < ls) {
			av_strlcpy(authorization, at2, FFMIN(authorization_size, at + 1 - at2));
			p = at + 1; /* skip '@' */
		}

		if (*p == '[' && (brk = strchr(p, ']')) && brk < ls) {
			/* [host]:port */
			av_strlcpy(hostname, p + 1, FFMIN(hostname_size, brk - p));
			if (brk[1] == ':' && port_ptr)
				*port_ptr = atoi(brk + 2);
		} else if ((col = strchr(p, ':')) && col < ls) {
			av_strlcpy(hostname, p, FFMIN(col + 1 - p, hostname_size));
			if (port_ptr)
				*port_ptr = atoi(col + 1);
		} else {
			av_strlcpy(hostname, p, FFMIN(ls + 1 - p, hostname_size));
		}
	}
}

char *ff_data_to_hex(char *buff, const uint8_t *src, int s, int lowercase)
{
	int i;
	static const char hex_table_uc[16] = { '0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F' };
	static const char hex_table_lc[16] = { '0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f' };
	const char *hex_table = lowercase ? hex_table_lc : hex_table_uc;

	for(i = 0; i < s; i++) {
		buff[i * 2]     = hex_table[src[i] >> 4];
		buff[i * 2 + 1] = hex_table[src[i] & 0xF];
	}

	return buff;
}

int ff_hex_to_data(uint8_t *data, const char *p)
{
	int c, len, v;

	len = 0;
	v = 1;
	for (;;) {
		p += strspn(p, SPACE_CHARS);
		if (*p == '\0')
			break;
		c = av_toupper((unsigned char) *p++);
		if (c >= '0' && c <= '9')
			c = c - '0';
		else if (c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else
			break;
		v = (v << 4) | c;
		if (v & 0x100) {
			if (data)
				data[len] = v;
			len++;
			v = 1;
		}
	}
	return len;
}

void av_set_pts_info(AVStream*  s, int pts_wrap_bits, int pts_num, int pts_den)
{
	avpriv_set_pts_info(s, pts_wrap_bits, pts_num, pts_den);
}

void avpriv_set_pts_info(AVStream *s, int pts_wrap_bits,
		unsigned int pts_num, unsigned int pts_den)
{
	AVRational new_tb;
	if(av_reduce(&new_tb.num, &new_tb.den, pts_num, pts_den, INT_MAX)){
		if(new_tb.num != pts_num)
			gxlogd("st:%d removing common factor %d from timebase\n", s->index, pts_num/new_tb.num);
	}else {
		gxlogd("st:%d has too large timebase, reducing\n", s->index);
	}

	if(new_tb.num <= 0 || new_tb.den <= 0) {
		return;
	}
	s->time_base = new_tb;
	s->pts_wrap_bits = pts_wrap_bits;
}

int ff_url_join(char *str, int size, const char *proto,
		const char *authorization, const char *hostname,
		int port, const char *fmt, ...)
{
	struct addrinfo hints = { 0 }, *ai;

	str[0] = '\0';
	if (proto)
		av_strlcatf(str, size, "%s://", proto);
	if (authorization && authorization[0])
		av_strlcatf(str, size, "%s@", authorization);
	if (1 == GxCore_IfSupportIPV6()) {
		/* Determine if hostname is a numerical IPv6 address,
		 * properly escape it within [] in that case. */
		hints.ai_flags = AI_NUMERICHOST;
		if (!getaddrinfo(hostname, NULL, &hints, &ai)) {
			if (ai->ai_family == AF_INET6) {
				av_strlcat(str, "[", size);
				av_strlcat(str, hostname, size);
				av_strlcat(str, "]", size);
			} else {
				av_strlcat(str, hostname, size);
			}
			freeaddrinfo(ai);
		} else {
			/* Not an IPv6 address, just output the plain string. */
			av_strlcat(str, hostname, size);
		}
	} else {
		/* Not an IPv6 address, just output the plain string. */
		av_strlcat(str, hostname, size);
	}
	if (port >= 0)
		av_strlcatf(str, size, ":%d", port);
	if (fmt) {
		va_list vl;
		int len = strlen(str);

		va_start(vl, fmt);
		vsnprintf(str + len, size > len ? size - len : 0, fmt, vl);
		va_end(vl);
	}
	return strlen(str);
}

int ff_write_chained(AVFormatContext *dst, int dst_stream, AVPacket *pkt,
		AVFormatContext *src)
{
	AVPacket local_pkt;

	local_pkt = *pkt;
	local_pkt.stream_index = dst_stream;
	if (pkt->pts != AV_NOPTS_VALUE)
		local_pkt.pts = av_rescale_q(pkt->pts,
				src->streams[pkt->stream_index]->time_base,
				dst->streams[dst_stream]->time_base);
	if (pkt->dts != AV_NOPTS_VALUE)
		local_pkt.dts = av_rescale_q(pkt->dts,
				src->streams[pkt->stream_index]->time_base,
				dst->streams[dst_stream]->time_base);
	return av_write_frame(dst, &local_pkt);
}

void ff_parse_key_value(const char *str, ff_parse_key_val_cb callback_get_buf,
		void *context)
{
	const char *ptr = str;

	/* Parse key=value pairs. */
	for (;;) {
		const char *key;
		char *dest = NULL, *dest_end;
		int key_len, dest_len = 0;

		/* Skip whitespace and potential commas. */
		while (*ptr && (isspace(*ptr) || *ptr == ','))
			ptr++;
		if (!*ptr)
			break;

		key = ptr;

		if (!(ptr = strchr(key, '=')))
			break;
		ptr++;
		key_len = ptr - key;

		callback_get_buf(context, key, key_len, &dest, &dest_len);
		dest_end = dest + dest_len - 1;

		if (*ptr == '\"') {
			ptr++;
			while (*ptr && *ptr != '\"') {
				if (*ptr == '\\') {
					if (!ptr[1])
						break;
					if (dest && dest < dest_end)
						*dest++ = ptr[1];
					ptr += 2;
				} else {
					if (dest && dest < dest_end)
						*dest++ = *ptr;
					ptr++;
				}
			}
			if (*ptr == '\"')
				ptr++;
		} else {
			for (; *ptr && !(isspace(*ptr) || *ptr == ','); ptr++)
				if (dest && dest < dest_end)
					*dest++ = *ptr;
		}
		if (dest)
			*dest = 0;
	}
}

int ff_find_stream_index(AVFormatContext *s, int id)
{
	int i;
	for (i = 0; i < s->nb_streams; i++) {
		if (s->streams[i]->id == id)
			return i;
	}
	return -1;
}

void ff_make_absolute_url(char *buf, int size, const char *base,
		const char *rel)
{
	char *sep, *path_query;
	/* Absolute path, relative to the current server */
	if (base && strstr(base, "://") && rel[0] == '/') {
		if (base != buf)
			av_strlcpy(buf, base, size);
		sep = strstr(buf, "://");
		if (sep) {
			/* Take scheme from base url */
			if (rel[1] == '/') {
				sep[1] = '\0';
			} else {
				/* Take scheme and host from base url */
				sep += 3;
				sep = strchr(sep, '/');
				if (sep)
					*sep = '\0';
			}
		}
		av_strlcat(buf, rel, size);
		return;
	}
	/* If rel actually is an absolute url, just copy it */
	if (!base || strstr(rel, "://") || rel[0] == '/') {
		av_strlcpy(buf, rel, size);
		return;
	}
	if (base != buf)
		av_strlcpy(buf, base, size);
	/* Strip off any query string from base */
	path_query = strchr(buf, '?');
	if (path_query)
		*path_query = '\0';

	/* Is relative path just a new query part? */
	if (rel[0] == '?') {
		av_strlcat(buf, rel, size);
		return;
	}

	/* Remove the file name from the base url */
	sep = strrchr(buf, '/');
	if (sep)
		sep[1] = '\0';
	else
		buf[0] = '\0';
	while (av_strstart(rel, "../", NULL) && sep) {
		/* Remove the path delimiter at the end */
		sep[0] = '\0';
		sep = strrchr(buf, '/');
		/* If the next directory name to pop off is "..", break here */
		if (!strcmp(sep ? &sep[1] : buf, "..")) {
			/* Readd the slash we just removed */
			av_strlcat(buf, "/", size);
			break;
		}
		/* Cut off the directory name */
		if (sep)
			sep[1] = '\0';
		else
			buf[0] = '\0';
		rel += 3;
	}
	av_strlcat(buf, rel, size);
}

int64_t ff_iso8601_to_unix_time(const char *datestr)
{
#if 0
	struct tm time1 = {0}, time2 = {0};
	char *ret1, *ret2;
	ret1 = strptime(datestr, "%Y - %m - %d %T", &time1);
	ret2 = strptime(datestr, "%Y - %m - %dT%T", &time2);
	if (ret2 && !ret1)
		return av_timegm(&time2);
	else
		return av_timegm(&time1);
#else
	av_log(NULL, AV_LOG_WARNING, "strptime() unavailable on this system, cannot convert "
			"the date string.\n");
	return 0;
#endif
}

int avformat_query_codec(AVOutputFormat *ofmt, enum CodecID codec_id, int std_compliance)
{
	if (ofmt) {
//		if (ofmt->query_codec)
//			return ofmt->query_codec(codec_id, std_compliance);
//		else if (ofmt->codec_tag)
		if (ofmt->codec_tag)
			return !!av_codec_get_tag(ofmt->codec_tag, codec_id);
		else if (codec_id == ofmt->video_codec || codec_id == ofmt->audio_codec ||
				codec_id == ofmt->subtitle_codec)
			return 1;
	}
	return AVERROR_PATCHWELCOME;
}

int avformat_network_init(void)
{
	return 0;
}

int avformat_network_deinit(void)
{
	return 0;
}

#if 0
int ff_add_param_change(AVPacket *pkt, int32_t channels,
		uint64_t channel_layout, int32_t sample_rate,
		int32_t width, int32_t height)
{
	uint32_t flags = 0;
	int size = 4;
	uint8_t *data;
	if (!pkt)
		return AVERROR(EINVAL);
	if (channels) {
		size += 4;
		flags |= AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT;
	}
	if (channel_layout) {
		size += 8;
		flags |= AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT;
	}
	if (sample_rate) {
		size += 4;
		flags |= AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE;
	}
	if (width || height) {
		size += 8;
		flags |= AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS;
	}
	data = av_packet_new_side_data(pkt, AV_PKT_DATA_PARAM_CHANGE, size);
	if (!data)
		return AVERROR(ENOMEM);
	bytestream_put_le32(&data, flags);
	if (channels)
		bytestream_put_le32(&data, channels);
	if (channel_layout)
		bytestream_put_le64(&data, channel_layout);
	if (sample_rate)
		bytestream_put_le32(&data, sample_rate);
	if (width || height) {
		bytestream_put_le32(&data, width);
		bytestream_put_le32(&data, height);
	}
	return 0;
}
#endif

const struct AVCodecTag *avformat_get_riff_video_tags(void)
{
	return ff_codec_bmp_tags;
}
const struct AVCodecTag *avformat_get_riff_audio_tags(void)
{
	return ff_codec_wav_tags;
}

#if 0
AVRational av_guess_sample_aspect_ratio(AVFormatContext *format, AVStream *stream, AVFrame *frame)
{
	AVRational undef = {0, 1};
	AVRational stream_sample_aspect_ratio = stream ? stream->sample_aspect_ratio : undef;
	AVRational codec_sample_aspect_ratio  = stream && stream->codec ? stream->codec->sample_aspect_ratio : undef;
	AVRational frame_sample_aspect_ratio  = frame  ? frame->sample_aspect_ratio  : codec_sample_aspect_ratio;

	av_reduce(&stream_sample_aspect_ratio.num, &stream_sample_aspect_ratio.den,
			stream_sample_aspect_ratio.num,  stream_sample_aspect_ratio.den, INT_MAX);
	if (stream_sample_aspect_ratio.num <= 0 || stream_sample_aspect_ratio.den <= 0)
		stream_sample_aspect_ratio = undef;

	av_reduce(&frame_sample_aspect_ratio.num, &frame_sample_aspect_ratio.den,
			frame_sample_aspect_ratio.num,  frame_sample_aspect_ratio.den, INT_MAX);
	if (frame_sample_aspect_ratio.num <= 0 || frame_sample_aspect_ratio.den <= 0)
		frame_sample_aspect_ratio = undef;

	if (stream_sample_aspect_ratio.num)
		return stream_sample_aspect_ratio;
	else
		return frame_sample_aspect_ratio;
}
#endif

int aac_get_sample_rate_index(uint32_t sample_rate)
{
	if (92017 <= sample_rate)
		return 0;
	else if (75132 <= sample_rate)
		return 1;
	else if (55426 <= sample_rate)
		return 2;
	else if (46009 <= sample_rate)
		return 3;
	else if (37566 <= sample_rate)
		return 4;
	else if (27713 <= sample_rate)
		return 5;
	else if (23004 <= sample_rate)
		return 6;
	else if (18783 <= sample_rate)
		return 7;
	else if (13856 <= sample_rate)
		return 8;
	else if (11502 <= sample_rate)
		return 9;
	else if (9391 <= sample_rate)
		return 10;
	else
		return 11;
}


	int ff_alloc_extradata(AVCodecContext *avctx, int size)
	{
#define INT32_MAX            (2147483647)
		int ret;

		if (size < 0 || size >= INT32_MAX - FF_INPUT_BUFFER_PADDING_SIZE) {
			avctx->extradata_size = 0;
			return AVERROR(EINVAL);
		}
		avctx->extradata = av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);
		if (avctx->extradata) {
			memset(avctx->extradata + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
			avctx->extradata_size = size;
			ret = 0;
		} else {
			avctx->extradata_size = 0;
			ret = AVERROR(ENOMEM);
		}
		return ret;
	}


	const uint8_t *avpriv_find_start_code(const uint8_t *p,
			const uint8_t *end,
			uint32_t *state)
	{
		int i;
		av_assert0(p <= end);
		if (p >= end)
			return end;

		for (i = 0; i < 3; i++) {
			uint32_t tmp = *state << 8;
			*state = tmp + *(p++);
			if (tmp == 0x100 || p == end)
				return p;
		}

		while (p < end) {
			if      (p[-1] > 1      ) p += 3;
			else if (p[-2]          ) p += 2;
			else if (p[-3]|(p[-1]-1)) p++;
			else {
				p++;
				break;
			}
		}
		p = FFMIN(p, end) - 4;
		*state = AV_RB32(p);

		return p + 4;
	}

const char *avcodec_get_name(enum CodecID id)
{
    const AVCodecDescriptor *cd;
    //AVCodec *codec;

    if (id == CODEC_ID_NONE)
        return "none";
    cd = avcodec_descriptor_get(id);
    if (cd)
        return cd->name;
    return "unknown_codec";
}

const char *av_get_media_type_string(enum AVMediaType media_type)
{
    switch (media_type) {
        case CODEC_TYPE_VIDEO:      return "video";
        case CODEC_TYPE_AUDIO:      return "audio";
        case CODEC_TYPE_DATA:       return "data";
        case CODEC_TYPE_SUBTITLE:   return "subtitle";
        case CODEC_TYPE_ATTACHMENT: return "attachment";
        default:                      return NULL;
    }
}

void av_dump_format(AVFormatContext *ic, int index, const char *url, int is_output)
{
    int i, debug_network_stream = 0;
    if (ic->nb_streams <= 0)
        return;
    GxPlayer_SystemGet(PSYS_DEBUG_NETWORK_STREAM, &debug_network_stream);
    if (!debug_network_stream)
        return;

    gxlogi ("\n\n==================av_dump_format=====================\n");
    gxlogi_l  ("I/ %s #%d, %s, %s '%s':\n",
           is_output?"Output":"Input", index,
           is_output?ic->oformat->name:ic->iformat->name,
           is_output?"to":"from", url);

    if (!is_output) {
        if (ic->duration >= 0) {
            int64_t hours, mins, secs, us;
            int64_t duration = ic->duration + (ic->duration <= INT64_MAX - 5000 ? 5000 : 0);
            secs  = duration / AV_TIME_BASE;
            us    = duration % AV_TIME_BASE;
            mins  = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            if (ic->bit_rate)
                gxlogi ("    Duration: %02lld:%02lld:%02lld.%02lld. bitrate:%lld kb/s", hours, mins, secs, (100 * us) / AV_TIME_BASE, ic->bit_rate/1000);
            else
                gxlogi ("    Duration: %02lld:%02lld:%02lld.%02lld. bitrate:%N/A", hours, mins, secs, (100 * us) / AV_TIME_BASE);
        } else {
            gxlogi ("    Duration: N/A");
        }
    }

	if (ic->start_time != AV_NOPTS_VALUE) {
		int64_t secs, us;
		secs = av_llabs(ic->start_time / AV_TIME_BASE);
		us	 = av_llabs(ic->start_time % AV_TIME_BASE);
		gxlogi ("    start: %s%lld.%06lld", ic->start_time >= 0 ? "" : "-", secs,
			   (int) av_rescale(us, 1000000, AV_TIME_BASE));
	}

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream* st = ic->streams[i];
        gxlogi ("Stream #%d:%d(und): %s: %s(id:0x%x)\n", index, i,
            av_get_media_type_string(st->codec->codec_type), avcodec_get_name(st->codec->codec_id), st->codec->codec_id);
    }
    gxlogi ("\n\n");
}

uint8_t *av_stream_get_side_data(const AVStream *st,
                                 enum AVPacketSideDataType type, int *size)
{
    int i;

    for (i = 0; i < st->nb_side_data; i++) {
        if (st->side_data[i].type == type) {
            if (size)
                *size = st->side_data[i].size;
            return st->side_data[i].data;
        }
    }
    return NULL;
}

int av_stream_add_side_data(AVStream *st, enum AVPacketSideDataType type,
                            uint8_t *data, size_t size)
{
    AVPacketSideData *sd, *tmp;
    int i;

    for (i = 0; i < st->nb_side_data; i++) {
        sd = &st->side_data[i];

        if (sd->type == type) {
            av_freep(&sd->data);
            sd->data = data;
            sd->size = size;
            return 0;
        }
    }

    if ((unsigned)st->nb_side_data + 1 >= INT_MAX / sizeof(*st->side_data))
        return AVERROR(ERANGE);

    tmp = av_realloc(st->side_data, (st->nb_side_data + 1) * sizeof(*tmp));
    if (!tmp) {
        return AVERROR(ENOMEM);
    }

    st->side_data = tmp;
    st->nb_side_data++;

    sd = &st->side_data[st->nb_side_data - 1];
    sd->type = type;
    sd->data = data;
    sd->size = size;

    return 0;
}

uint8_t *av_stream_new_side_data(AVStream *st, enum AVPacketSideDataType type,
                                 int size)
{
    int ret;
    uint8_t *data = av_malloc(size);

    if (!data)
        return NULL;

    ret = av_stream_add_side_data(st, type, data, size);
    if (ret < 0) {
        av_freep(&data);
        return NULL;
    }

    return data;
}

unsigned int av_xiphlacing(unsigned char *s, unsigned int v)
{
	unsigned int n = 0;

	while (v >= 0xff) {
		*s++ = 0xff;
		v -= 0xff;
		n++;
	}
	*s = v;
	n++;
	return n;
}

extern int avio_close(AVIOContext *s);
void ff_format_io_close(AVFormatContext *s, AVIOContext **pb)
{
	if (*pb) {
		avio_close(*pb);
	}
	*pb = NULL;
}

int ff_copy_whiteblacklists(AVFormatContext *dst, const AVFormatContext *src)
{
	return 0;
}

/** Flush the frame reader. */
void ff_read_frame_flush(AVFormatContext *s)
{
    AVStream *st = NULL;
    int i = 0, j = 0;

    if (s == NULL) {
        return ;
    }
    flush_packet_queue(s);

    /* Reset read state for each stream. */
    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];

        if (st->parser) {
            av_parser_close(st->parser);
            st->parser = NULL;
        }
        st->last_IP_pts = AV_NOPTS_VALUE;
        st->last_dts_for_order_check = AV_NOPTS_VALUE;
        if (st->first_dts == AV_NOPTS_VALUE)
            st->cur_dts = RELATIVE_TS_BASE;
        else
            /* We set the current DTS to an unspecified origin. */
            st->cur_dts = AV_NOPTS_VALUE;

        st->probe_packets = MAX_PROBE_PACKETS;

        for (j = 0; j < MAX_REORDER_DELAY + 1; j++)
            st->pts_buffer[j] = AV_NOPTS_VALUE;

        //if (s->internal->inject_global_side_data)
        //    st->inject_global_side_data = 1;

        //st->skip_samples = 0;
    }
}

int avformat_control(AVFormatContext **ps, int cmd, void *arg)
{
    AVFormatContext *s;
    int ret = -1;
    if (!ps || !*ps) {
        return ret;
    }
    s  = *ps;
    if (s && s->iformat && s->iformat->read_control) {
        ret = s->iformat->read_control(s, cmd, arg);
    }
    return ret;
}

int gx_av_get_random_seed(void)
{
    return rand();
}

void codec_hls_context_reset(AVCodecContext *par)
{
    av_freep(&par->extradata);

    memset(par, 0, sizeof(*par));

    par->codec_type          = AVMEDIA_TYPE_UNKNOWN;
    par->sample_aspect_ratio = (AVRational){ 0, 1 };
}

#define AV_INPUT_BUFFER_PADDING_SIZE 64
int avcodec_hls_context_copy(AVCodecContext *dst, AVCodecContext *src)
{
    codec_hls_context_reset(dst);
    memcpy(dst, src, sizeof(*dst));
    dst->extradata      = NULL;
    dst->extradata_size = 0;
    if (src->extradata) {
        dst->extradata = av_mallocz(src->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!dst->extradata) {
            return AVERROR(ENOMEM);
        }
        memcpy(dst->extradata, src->extradata, src->extradata_size);
        dst->extradata_size = src->extradata_size;
    }
    return 0;
}

void ff_update_cur_dts(AVFormatContext *s, AVStream *ref_st, int64_t timestamp)
{
    int i;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];

        st->cur_dts =
            av_rescale(timestamp,
                       st->time_base.den * (int64_t) ref_st->time_base.num,
                       st->time_base.num * (int64_t) ref_st->time_base.den);
    }
}

int64_t ff_gen_search(AVFormatContext *s, int stream_index, int64_t target_ts,
                      int64_t pos_min, int64_t pos_max, int64_t pos_limit,
                      int64_t ts_min, int64_t ts_max,
                      int flags, int64_t *ts_ret,
                      int64_t (*read_timestamp)(struct AVFormatContext *, int,
                                                int64_t *, int64_t))
{
    int64_t pos, ts = AV_NOPTS_VALUE;
    int64_t start_pos;
    int no_change;
//    int ret;

    av_log(s, AV_LOG_TRACE, "gen_seek: %d %s\n", stream_index, av_ts2str(target_ts));

#if 0
    if (ts_min == AV_NOPTS_VALUE) {
        pos_min = s->internal->data_offset;
        ts_min  = ff_read_timestamp(s, stream_index, &pos_min, INT64_MAX, read_timestamp);
        if (ts_min == AV_NOPTS_VALUE)
            return -1;
    }
#endif

    if (ts_min >= target_ts) {
        *ts_ret = ts_min;
        return pos_min;
    }

#if 0
    if (ts_max == AV_NOPTS_VALUE) {
        if ((ret = ff_find_last_ts(s, stream_index, &ts_max, &pos_max, read_timestamp)) < 0)
            return ret;
        pos_limit = pos_max;
    }
#endif

    if (ts_max <= target_ts) {
        *ts_ret = ts_max;
        return pos_max;
    }

    av_assert0(ts_min < ts_max);

    no_change = 0;
    while (pos_min < pos_limit) {
        av_log(s, AV_LOG_TRACE,
                "pos_min=0x%"PRIx64" pos_max=0x%"PRIx64" dts_min=%s dts_max=%s\n",
                pos_min, pos_max, av_ts2str(ts_min), av_ts2str(ts_max));
        av_assert0(pos_limit <= pos_max);

        if (no_change == 0) {
            int64_t approximate_keyframe_distance = pos_max - pos_limit;
            // interpolate position (better than dichotomy)
            pos = av_rescale(target_ts - ts_min, pos_max - pos_min,
                             ts_max - ts_min) +
                  pos_min - approximate_keyframe_distance;
        } else if (no_change == 1) {
            // bisection if interpolation did not change min / max pos last time
            pos = (pos_min + pos_limit) >> 1;
        } else {
            /* linear search if bisection failed, can only happen if there
             * are very few or no keyframes between min/max */
            pos = pos_min;
        }
        if (pos <= pos_min)
            pos = pos_min + 1;
        else if (pos > pos_limit)
            pos = pos_limit;
        start_pos = pos;

        // May pass pos_limit instead of -1.
#if 0
        ts = ff_read_timestamp(s, stream_index, &pos, INT64_MAX, read_timestamp);
        if (pos == pos_max)
            no_change++;
        else
            no_change = 0;
#endif
        av_log(s, AV_LOG_TRACE, "%"PRId64" %"PRId64" %"PRId64" / %s %s %s"
                " target:%s limit:%"PRId64" start:%"PRId64" noc:%d\n",
                pos_min, pos, pos_max,
                av_ts2str(ts_min), av_ts2str(ts), av_ts2str(ts_max), av_ts2str(target_ts),
                pos_limit, start_pos, no_change);
        if (ts == AV_NOPTS_VALUE) {
            av_log(s, AV_LOG_ERROR, "read_timestamp() failed in the middle\n");
            return -1;
        }

        if (target_ts <= ts) {
            pos_limit = start_pos - 1;
            pos_max   = pos;
            ts_max    = ts;
        }
        if (target_ts >= ts) {
            pos_min = pos;
            ts_min  = ts;
        }
    }

    pos     = (flags & AVSEEK_FLAG_BACKWARD) ? pos_min : pos_max;
    ts      = (flags & AVSEEK_FLAG_BACKWARD) ? ts_min  : ts_max;
#if 0
    pos_min = pos;
    ts_min  = ff_read_timestamp(s, stream_index, &pos_min, INT64_MAX, read_timestamp);
    pos_min++;
    ts_max = ff_read_timestamp(s, stream_index, &pos_min, INT64_MAX, read_timestamp);
    av_log(s, AV_LOG_TRACE, "pos=0x%"PRIx64" %s<=%s<=%s\n",
            pos, av_ts2str(ts_min), av_ts2str(target_ts), av_ts2str(ts_max));
#endif
    *ts_ret = ts;
    return pos;
}


int ff_seek_frame_binary(AVFormatContext *s, int stream_index,
                         int64_t target_ts, int flags)
{
    const AVInputFormat *avif = s->iformat;
    int64_t av_uninit(pos_min), av_uninit(pos_max), pos, pos_limit;
    int64_t ts_min, ts_max, ts;
    int index;
    int64_t ret;
    AVStream *st;

    if (stream_index < 0)
        return -1;

    av_log(s, AV_LOG_TRACE, "read_seek: %d %s\n", stream_index, av_ts2str(target_ts));

    ts_max =
    ts_min = AV_NOPTS_VALUE;
    pos_limit = -1; // GCC falsely says it may be uninitialized.

    st = s->streams[stream_index];
    if (st->index_entries) {
        AVIndexEntry *e;

        /* FIXME: Whole function must be checked for non-keyframe entries in
         * index case, especially read_timestamp(). */
        index = av_index_search_timestamp(st, target_ts,
                                          flags | AVSEEK_FLAG_BACKWARD);
        index = FFMAX(index, 0);
        e     = &st->index_entries[index];

        if (e->timestamp <= target_ts || e->pos == e->min_distance) {
            pos_min = e->pos;
            ts_min  = e->timestamp;
            av_log(s, AV_LOG_TRACE, "using cached pos_min=0x%"PRIx64" dts_min=%s\n",
                    pos_min, av_ts2str(ts_min));
        } else {
            av_assert1(index == 0);
        }

        index = av_index_search_timestamp(st, target_ts,
                                          flags & ~AVSEEK_FLAG_BACKWARD);
        av_assert0(index < st->nb_index_entries);
        if (index >= 0) {
            e = &st->index_entries[index];
            av_assert1(e->timestamp >= target_ts);
            pos_max   = e->pos;
            ts_max    = e->timestamp;
            pos_limit = pos_max - e->min_distance;
            av_log(s, AV_LOG_TRACE, "using cached pos_max=0x%"PRIx64" pos_limit=0x%"PRIx64
                    " dts_max=%s\n", pos_max, pos_limit, av_ts2str(ts_max));
        }
    }

    pos = ff_gen_search(s, stream_index, target_ts, pos_min, pos_max, pos_limit,
                        ts_min, ts_max, flags, &ts, avif->read_timestamp);
    if (pos < 0)
        return -1;

    /* do the seek */
    if ((ret = avio_seek(s->pb, pos, SEEK_SET)) < 0)
        return ret;

    ff_read_frame_flush(s);
    ff_update_cur_dts(s, st, ts);

    return 0;
}

AVChapter *avpriv_new_chapter(AVFormatContext *s, int id, AVRational time_base,
                              int64_t start, int64_t end, const char *title)
{
    AVChapter *chapter = NULL;
    int i;

    if (end != AV_NOPTS_VALUE && start > end) {
        return NULL;
    }

    for (i = 0; i < s->nb_chapters; i++)
        if (s->chapters[i]->id == id)
            chapter = s->chapters[i];

    if (!chapter) {
        chapter = av_mallocz(sizeof(AVChapter));
        if (!chapter)
            return NULL;
        dynarray_add(&s->chapters, (int*)(&s->nb_chapters), chapter);
    }
    av_dict_set(&chapter->metadata, "title", title, 0);
    chapter->id        = id;
    chapter->time_base = time_base;
    chapter->start     = start;
    chapter->end       = end;

    return chapter;
}



