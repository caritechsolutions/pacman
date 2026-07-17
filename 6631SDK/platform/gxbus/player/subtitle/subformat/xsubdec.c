/*
 * XSUB subtitle decoder
 * Copyright (c) 2007 Reimar Döffinger
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

#include "../avutil/mathematics.h"
#include "../avformat/avformat.h"
#include "../avformat/avcodec.h"
#include "get_bits.h"
#include "bytestream.h"

#if defined(CONFIG_FFMPEG_ENABLE)
extern const uint8_t log2_tab[256];
#else
const uint8_t log2_tab[256] = {
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding rnd)
{
	int64_t r = 0;

	rnd = rnd&~AV_ROUND_PASS_MINMAX;

	assert(c > 0);
	assert(b >= 0);
	assert(rnd >= 0 && rnd <= 5 && rnd != 4);

	if (a < 0 && a != INT64_MIN)
		return -av_rescale_rnd(-a, b, c, rnd ^ ((rnd >> 1) & 1));

	if (rnd == AV_ROUND_NEAR_INF)
		r = c / 2;
	else if (rnd & 1)
		r = c - 1;

	if (b <= INT_MAX && c <= INT_MAX) {
		if (a <= INT_MAX)
			return (a*  b + r) / c;
		else
			return a / c*  b + (a % c * b + r) / c;
	} else {
		uint64_t a0 = a & 0xFFFFFFFF;
		uint64_t a1 = a >> 32;
		uint64_t b0 = b & 0xFFFFFFFF;
		uint64_t b1 = b >> 32;
		uint64_t t1 = a0*  b1 + a1 * b0;
		uint64_t t1a = t1 << 32;
		int i;

		a0 = a0*  b0 + t1a;
		a1 = a1*  b1 + (t1 >> 32) + (a0 < t1a);
		a0 += r;
		a1 += a0 < r;

		for (i = 63; i >= 0; i--) {
			a1 += a1 + ((a0 >> i) & 1);
			t1 += t1;
			if ( /*o ||*/ c <= a1) {
				a1 -= c;
				t1++;
			}
		}
		return t1;
	}
}

int64_t av_rescale_q(int64_t a, AVRational bq, AVRational cq)
{
	int64_t b = bq.num*  (int64_t) cq.den;
	int64_t c = cq.num*  (int64_t) bq.den;
	return av_rescale_rnd(a, b, c, AV_ROUND_NEAR_INF);
}

#endif

static const uint8_t tc_offsets[9] = { 0, 1, 3, 4, 6, 7, 9, 10, 11 };
static const uint8_t tc_muls[9] = { 10, 6, 10, 6, 10, 10, 10, 10, 1 };

static int64_t parse_timecode(const uint8_t *buf, int64_t packet_time) {
	int i;
	int64_t ms = 0;
	if (buf[2] != ':' || buf[5] != ':' || buf[8] != '.')
		return AV_NOPTS_VALUE;
	for (i = 0; i < sizeof(tc_offsets); i++) {
		uint8_t c = buf[tc_offsets[i]] - '0';
		if (c > 9) return AV_NOPTS_VALUE;
		ms = (ms + c) * tc_muls[i];
	}
	return ms - packet_time;
}

void xsub_decode_free(void *data)
{
	unsigned int i = 0;
	AVSubtitle *sub = data;

	if (sub->rects) {
		for (i = 0; i < sub->num_rects; i++) {
			if (sub->rects[i]) {
				if (sub->rects[i]->data[0])
					av_freep(&sub->rects[i]->data[0]);
				if (sub->rects[i]->data[1])
					av_freep(&sub->rects[i]->data[1]);
				av_freep(&sub->rects[i]);
			}
		}
		av_freep(&sub->rects);
	}
}

int xsub_decode_frame(int has_alpha, int64_t pts, void *in_data, int in_size, void *data, int *data_size)
{
	const uint8_t *buf = in_data;
	int buf_size = in_size;
	AVSubtitle *sub = data;
	const uint8_t *buf_end = buf + buf_size;
	uint8_t *bitmap;
	int w, h, x, y, i, ret;
	int64_t packet_time = 0;
	GetBitContext gb;
	//   int has_alpha = avctx->codec_tag == MKTAG('D','X','S','A');

	// check that at least header fits
	if (buf_size < 27 + 7 * 2 + 4 * (3 + has_alpha)) {
		return -1;
	}

	// read start and end time
	if (buf[0] != '[' || buf[13] != '-' || buf[26] != ']') {
		return -1;
	}
	if (pts != AV_NOPTS_VALUE)
		packet_time = av_rescale_q(pts, AV_TIME_BASE_Q, (AVRational){1, 1000});
	sub->start_display_time = parse_timecode(buf +  1, packet_time);
	sub->end_display_time   = parse_timecode(buf + 14, packet_time);
	buf += 27;

	// read header
	w = bytestream_get_le16(&buf);
	h = bytestream_get_le16(&buf);
	if ((w < 0 || h < 0) || (w > 1920 || h > 1080))
		return -1;
	x = bytestream_get_le16(&buf);
	y = bytestream_get_le16(&buf);
	// skip bottom right position, it gives no new information
	bytestream_get_le16(&buf);
	bytestream_get_le16(&buf);
	// The following value is supposed to indicate the start offset
	// (relative to the palette) of the data for the second field,
	// however there are files in which it has a bogus value and thus
	// we just ignore it
	bytestream_get_le16(&buf);

	if (buf_end - buf < h + 3*4)
		return AVERROR_INVALIDDATA;

	// allocate sub and set values
	sub->rects =  av_mallocz(sizeof(*sub->rects));
	if (!sub->rects)
		return AVERROR(ENOMEM);

	sub->rects[0] = av_mallocz(sizeof(*sub->rects[0]));
	if (!sub->rects[0]) {
		av_freep(&sub->rects);
		return AVERROR(ENOMEM);
	}
	sub->rects[0]->x = x; sub->rects[0]->y = y;
	sub->rects[0]->w = w; sub->rects[0]->h = h;
	sub->rects[0]->type = SUBTITLE_BITMAP;
	sub->rects[0]->linesize[0] = w;
	sub->rects[0]->data[0] = av_malloc(w * h);
	sub->rects[0]->nb_colors = 4;
	sub->rects[0]->data[1] = av_mallocz(AVPALETTE_SIZE);
	if (!sub->rects[0]->data[0] || !sub->rects[0]->data[1]) {
		av_freep(&sub->rects[0]->data[1]);
		av_freep(&sub->rects[0]->data[0]);
		av_freep(&sub->rects[0]);
		av_freep(&sub->rects);
		return AVERROR(ENOMEM);
	}
	sub->num_rects = 1;

	// read palette
	for (i = 0; i < sub->rects[0]->nb_colors; i++)
		((uint32_t*)sub->rects[0]->data[1])[i] = bytestream_get_be24(&buf);

	if (!has_alpha) {
		// make all except background (first entry) non-transparent
		for (i = 1; i < sub->rects[0]->nb_colors; i++)
			((uint32_t *)sub->rects[0]->data[1])[i] |= 0xff000000;
	} else {
		for (i = 0; i < sub->rects[0]->nb_colors; i++)
			((uint32_t *)sub->rects[0]->data[1])[i] |= (unsigned)*buf++ << 24;
	}

	// process RLE-compressed data
	if ((ret = init_get_bits8(&gb, buf, buf_end - buf)) < 0) {
		xsub_decode_free(data);
		return ret;
	}

	bitmap = sub->rects[0]->data[0];
	for (y = 0; y < h; y++) {
		// interlaced: do odd lines
		if (y == (h + 1) / 2) bitmap = sub->rects[0]->data[0] + w;
		for (x = 0; x < w; ) {
			int log2 = log2_tab[show_bits(&gb, 8)];
			int run = get_bits(&gb, 14 - 4 * (log2 >> 1));
			int color = get_bits(&gb, 2);
			run = FFMIN(run, w - x);
			// run length 0 means till end of row
			if (!run) run = w - x;
			memset(bitmap, color, run);
			bitmap += run;
			x += run;
		}
		// interlaced, skip every second line
		bitmap += w;
		align_get_bits(&gb);
	}
	*data_size = 1;
	return buf_size;
}
