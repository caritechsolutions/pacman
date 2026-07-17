/*
*  Monkey's Audio APE demuxer
*  Copyright (c) 2007 Benjamin Zores <ben@geexbox.org>
*   based upon libdemac from Dave Chapman.
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

#include "avformat.h"
#include "../avutil/intreadwrite.h"

#ifndef UINT_MAX
#define UINT_MAX (0xfffffff)
#endif

/* The earliest and latest file formats supported by this library*/
#define APE_MIN_VERSION 3950
#define APE_MAX_VERSION 3990

#define MAC_FORMAT_FLAG_8_BIT                 1	// is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2	// uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4	// uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8	// is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16	// has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32	// create the wave header on decompression (not stored)

#define MAC_SUBFRAME_SIZE 4608

#define APE_EXTRADATA_SIZE 6

/* APE tags*/
#define APE_TAG_VERSION               2000
#define APE_TAG_FOOTER_BYTES          32
#define APE_TAG_FLAG_CONTAINS_HEADER  (1 << 31)
#define APE_TAG_FLAG_IS_HEADER        (1 << 29)

#if 0
#define TAG(name, field)  {name, offsetof(AVFormatContext, field), sizeof(((AVFormatContext* )0)->field)}

static const struct {
	char* name;
	int offset;
	int size;
} tags[] = {
	TAG("Title", title),
	    TAG("Artist", author),
	    TAG("Copyright", copyright),
	    TAG("Comment", comment), TAG("Album", album), TAG("Year", year), TAG("Track", track), TAG("Genre", genre), {
	NULL}
};
#endif

typedef struct {
	int64_t pos;
	int nblocks;
	int size;
	int skip;
	int64_t pts;
} APEFrame;

typedef struct {
	/* Derived fields*/
	uint32_t junklength;
	uint32_t firstframe;
	uint32_t totalsamples;
	int currentframe;
	APEFrame* frames;

	/* Info from Descriptor Block*/
	char magic[4];
	int16_t fileversion;
	int16_t padding1;
	uint32_t descriptorlength;
	uint32_t headerlength;
	uint32_t seektablelength;
	uint32_t wavheaderlength;
	uint32_t audiodatalength;
	uint32_t audiodatalength_high;
	uint32_t wavtaillength;
	uint8_t md5[16];

	/* Info from Header Block*/
	uint16_t compressiontype;
	uint16_t formatflags;
	uint32_t blocksperframe;
	uint32_t finalframeblocks;
	uint32_t totalframes;
	uint16_t bps;
	uint16_t channels;
	uint32_t samplerate;

	/* Seektable*/
	uint32_t* seektable;
} APEContext;

static void ape_tag_read_field(AVFormatContext*  s)
{
	ByteIOContext* pb = s->pb;
	uint8_t buf[1024];
	uint32_t size;
	int i;

	memset(buf, 0, 1024);
	size = get_le32(pb);	/* field size*/
	url_fskip(pb, 4);	/* skip field flags*/

	for (i = 0; pb->buf_ptr[i] != '0' && pb->buf_ptr[i] >= 0x20 && pb->buf_ptr[i] <= 0x7E; i++) ;

	get_buffer(pb, buf, GXMIN(i, 1024));
	url_fskip(pb, 1);
#if 0
	for (i = 0; tags[i].name; i++)
		if (!strcmp((const char*)buf, tags[i].name)) {
			if (tags[i].size == sizeof(int)) {
				char tmp[16];
				get_buffer(pb, (unsigned char*)tmp, GXMIN(sizeof(tmp), size));
				*(int* )(((char *)s) + tags[i].offset) = atoi(tmp);
			} else {
				get_buffer(pb, ((unsigned char* )s) + tags[i].offset, GXMIN(tags[i].size, size));
			}
			break;
		}

	if (!tags[i].name)
		url_fskip(pb, size);
#endif
}

static void ape_parse_tag(AVFormatContext*  s)
{
	ByteIOContext* pb = s->pb;
	int file_size = url_fsize(pb);
	uint32_t val, fields, tag_bytes;
	uint8_t buf[8];
	int i;

	if (file_size < APE_TAG_FOOTER_BYTES)
		return;

	url_fseek(pb, file_size - APE_TAG_FOOTER_BYTES, SEEK_SET);

	get_buffer(pb, buf, 8);	/* APETAGEX*/
	if (strncmp((const char*)buf, "APETAGEX", 8)) {
		return;
	}

	val = get_le32(pb);	/* APE tag version*/
	if (val > APE_TAG_VERSION) {
		return;
	}

	tag_bytes = get_le32(pb);	/* tag size*/
	if (tag_bytes - APE_TAG_FOOTER_BYTES > (1024*  1024 * 16)) {
		return;
	}

	fields = get_le32(pb);	/* number of fields*/
	if (fields > 65536) {
		return;
	}

	val = get_le32(pb);	/* flags*/
	if (val & APE_TAG_FLAG_IS_HEADER) {
		return;
	}

	if (val & APE_TAG_FLAG_CONTAINS_HEADER)
		tag_bytes += 2*  APE_TAG_FOOTER_BYTES;

	url_fseek(pb, file_size - tag_bytes, SEEK_SET);

	for (i = 0; i < fields; i++)
		ape_tag_read_field(s);

}

static int ape_probe(AVProbeData*  p)
{
	if (p->buf[0] == 'M' && p->buf[1] == 'A' && p->buf[2] == 'C' && p->buf[3] == ' ')
		return AVPROBE_SCORE_MAX;

	return 0;
}

static int ape_read_header(AVFormatContext*  s, AVFormatParameters * ap)
{
	ByteIOContext* pb = s->pb;
	APEContext* ape = s->priv_data;
	AVStream* st;
	uint32_t tag;
	int i;
	int total_blocks;
	int64_t pts;

	/* TODO: Skip any leading junk such as id3v2 tags*/
	ape->junklength = 0;

	tag = get_le32(pb);
	if (tag != MKTAG('M', 'A', 'C', ' '))
		return -1;

	ape->fileversion = get_le16(pb);

	if (ape->fileversion < APE_MIN_VERSION || ape->fileversion > APE_MAX_VERSION) {
		return -1;
	}

	if (ape->fileversion >= 3980) {
		ape->padding1 = get_le16(pb);
		ape->descriptorlength = get_le32(pb);
		ape->headerlength = get_le32(pb);
		ape->seektablelength = get_le32(pb);
		ape->wavheaderlength = get_le32(pb);
		ape->audiodatalength = get_le32(pb);
		ape->audiodatalength_high = get_le32(pb);
		ape->wavtaillength = get_le32(pb);
		get_buffer(pb, ape->md5, 16);

		/* Skip any unknown bytes at the end of the descriptor.
		   This is for future compatibility*/
		if (ape->descriptorlength > 52)
			url_fseek(pb, ape->descriptorlength - 52, SEEK_CUR);

		/* Read header data*/
		ape->compressiontype = get_le16(pb);
		ape->formatflags = get_le16(pb);
		ape->blocksperframe = get_le32(pb);
		ape->finalframeblocks = get_le32(pb);
		ape->totalframes = get_le32(pb);
		ape->bps = get_le16(pb);
		ape->channels = get_le16(pb);
		ape->samplerate = get_le32(pb);
	} else {
		ape->descriptorlength = 0;
		ape->headerlength = 32;

		ape->compressiontype = get_le16(pb);
		ape->formatflags = get_le16(pb);
		ape->channels = get_le16(pb);
		ape->samplerate = get_le32(pb);
		ape->wavheaderlength = get_le32(pb);
		ape->wavtaillength = get_le32(pb);
		ape->totalframes = get_le32(pb);
		ape->finalframeblocks = get_le32(pb);

		if (ape->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL) {
			url_fseek(pb, 4, SEEK_CUR);	/* Skip the peak level*/
			ape->headerlength += 4;
		}

		if (ape->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS) {
			ape->seektablelength = get_le32(pb);
			ape->headerlength += 4;
			ape->seektablelength*= sizeof(int32_t);
		} else
			ape->seektablelength = ape->totalframes*  sizeof(int32_t);

		if (ape->formatflags & MAC_FORMAT_FLAG_8_BIT)
			ape->bps = 8;
		else if (ape->formatflags & MAC_FORMAT_FLAG_24_BIT)
			ape->bps = 24;
		else
			ape->bps = 16;

		if (ape->fileversion >= 3950)
			ape->blocksperframe = 73728*  4;
		else if (ape->fileversion >= 3900 || (ape->fileversion >= 3800 && ape->compressiontype >= 4000))
			ape->blocksperframe = 73728;
		else
			ape->blocksperframe = 9216;

		/* Skip any stored wav header*/
		if (!(ape->formatflags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
			url_fskip(pb, ape->wavheaderlength);
	}

	if (ape->totalframes > UINT_MAX / sizeof(APEFrame)) {
		return -1;
	}
	ape->frames = av_malloc(ape->totalframes*  sizeof(APEFrame));
	if (!ape->frames)
		return AVERROR_NOMEM;
	ape->firstframe =
	    ape->junklength + ape->descriptorlength + ape->headerlength + ape->seektablelength + ape->wavheaderlength;
	ape->currentframe = 0;

	ape->totalsamples = ape->finalframeblocks;
	if (ape->totalframes > 1)
		ape->totalsamples += ape->blocksperframe*  (ape->totalframes - 1);

	if (ape->seektablelength > 0) {
		ape->seektable = av_malloc(ape->seektablelength);
		for (i = 0; i < ape->seektablelength / sizeof(uint32_t); i++)
			ape->seektable[i] = get_le32(pb);
	}

	ape->frames[0].pos = ape->firstframe;
	ape->frames[0].nblocks = ape->blocksperframe;
	ape->frames[0].skip = 0;
	for (i = 1; i < ape->totalframes; i++) {
		ape->frames[i].pos = ape->seektable[i];	//ape->frames[i-1].pos + ape->blocksperframe;
		ape->frames[i].nblocks = ape->blocksperframe;
		ape->frames[i - 1].size = ape->frames[i].pos - ape->frames[i - 1].pos;
		ape->frames[i].skip = (ape->frames[i].pos - ape->frames[0].pos) & 3;
	}
	ape->frames[ape->totalframes - 1].size = ape->finalframeblocks*  4;
	ape->frames[ape->totalframes - 1].nblocks = ape->finalframeblocks;

	for (i = 0; i < ape->totalframes; i++) {
		if (ape->frames[i].skip) {
			ape->frames[i].pos -= ape->frames[i].skip;
			ape->frames[i].size += ape->frames[i].skip;
		}
		ape->frames[i].size = (ape->frames[i].size + 3) & ~3;
	}

	/* try to read APE tags*/
	if (!url_is_streamed(pb)) {
		ape_parse_tag(s);
		url_fseek(pb, 0, SEEK_SET);
	}

	/* now we are ready: build format streams*/
	st = av_new_stream(s, 0);
	if (!st)
		return -1;

	total_blocks =
	    (ape->totalframes == 0) ? 0 : ((ape->totalframes - 1)*  ape->blocksperframe) + ape->finalframeblocks;

	st->codec->codec_type = CODEC_TYPE_AUDIO;
	st->codec->codec_id = CODEC_ID_APE;
	st->codec->codec_tag = MKTAG('A', 'P', 'E', ' ');
	st->codec->channels = ape->channels;
	st->codec->sample_rate = ape->samplerate;
	st->codec->bits_per_sample = ape->bps;
	st->codec->frame_size = MAC_SUBFRAME_SIZE;

	st->nb_frames = ape->totalframes;
	s->start_time = 0;
	s->duration = (int64_t) total_blocks* AV_TIME_BASE / ape->samplerate;
	av_set_pts_info(st, 64, MAC_SUBFRAME_SIZE, ape->samplerate);

	st->codec->extradata = av_malloc(APE_EXTRADATA_SIZE);
	st->codec->extradata_size = APE_EXTRADATA_SIZE;
	AV_WL16(st->codec->extradata + 0, ape->fileversion);
	AV_WL16(st->codec->extradata + 2, ape->compressiontype);
	AV_WL16(st->codec->extradata + 4, ape->formatflags);

	pts = 0;
	for (i = 0; i < ape->totalframes; i++) {
		ape->frames[i].pts = pts;
		av_add_index_entry(st, ape->frames[i].pos, ape->frames[i].pts, 0, 0, AVINDEX_KEYFRAME);
		pts += ape->blocksperframe / MAC_SUBFRAME_SIZE;
	}

	return 0;
}

static int ape_read_packet(AVFormatContext*  s, AVPacket * pkt)
{
	int ret;
	int nblocks;
	APEContext* ape = s->priv_data;
	uint32_t extra_size = 8;

	if (url_feof(s->pb))
		return AVERROR_IO;
	if (ape->currentframe > ape->totalframes)
		return AVERROR_IO;

	url_fseek(s->pb, ape->frames[ape->currentframe].pos, SEEK_SET);

	/* Calculate how many blocks there are in this frame*/
	if (ape->currentframe == (ape->totalframes - 1))
		nblocks = ape->finalframeblocks;
	else
		nblocks = ape->blocksperframe;

	if (av_new_packet(pkt, ape->frames[ape->currentframe].size + extra_size) < 0)
		return AVERROR_NOMEM;

	AV_WL32(pkt->data, nblocks);
	AV_WL32(pkt->data + 4, ape->frames[ape->currentframe].skip);
	ret = get_buffer(s->pb, pkt->data + extra_size, ape->frames[ape->currentframe].size);

	pkt->pts = ape->frames[ape->currentframe].pts;
	pkt->stream_index = 0;

	/* note: we need to modify the packet size here to handle the last
	   packet*/
	pkt->size = ret + extra_size;

	ape->currentframe++;

	return 0;
}

static int ape_read_close(AVFormatContext*  s)
{
	APEContext* ape = s->priv_data;

	av_freep(&ape->frames);
	av_freep(&ape->seektable);
	return 0;
}

static int ape_read_seek(AVFormatContext*  s, int stream_index, int64_t timestamp, int flags)
{
	AVStream* st = s->streams[stream_index];
	APEContext* ape = s->priv_data;
	int index = av_index_search_timestamp(st, timestamp, flags);

	if (index < 0)
		return -1;

	ape->currentframe = index;
	url_fseek(s->pb, ape->frames[ape->currentframe].pos, SEEK_SET);
	return 0;
}

AVInputFormat ape_demuxer = {
	.name           = "ape",
	.priv_data_size = sizeof(APEContext),
	.read_probe     = ape_probe,
	.read_header    = ape_read_header,
	.read_packet    = ape_read_packet,
	.read_close     = ape_read_close,
	.read_seek      = ape_read_seek,
	.extensions = "ape,apl,mac"
};
