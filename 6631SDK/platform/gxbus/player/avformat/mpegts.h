/*
 * MPEG2 transport stream defines
 * Copyright (c) 2003 Fabrice Bellard
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

#ifndef AVFORMAT_MPEGTS_H
#define AVFORMAT_MPEGTS_H

#include "avformat.h"

#define TS_FEC_PACKET_SIZE 204
#define TS_DVHS_PACKET_SIZE 192
#define TS_PACKET_SIZE 188
#define TS_MAX_PACKET_SIZE 204

#define NB_PID_MAX 8192
#define MAX_SECTION_SIZE 4096

/* pids */
#define PAT_PID                 0x0000
#define SDT_PID                 0x0011

/* table ids */
#define PAT_TID   0x00
#define PMT_TID   0x02
#define SDT_TID   0x42

#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_AUDIO_AAC_LATM  0x11
#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_H265      0x24
#define STREAM_TYPE_VIDEO_CAVS      0x42
#define STREAM_TYPE_VIDEO_VC1       0xea
#define STREAM_TYPE_VIDEO_AVS       0x42
#define STREAM_TYPE_VIDEO_DIRAC     0xd1

#define STREAM_TYPE_AUDIO_AC3       0x81
#define STREAM_TYPE_AUDIO_DTS       0x8a

typedef enum {
	VIDEO_UNKNOWN = -1,
	VIDEO_MPEG1 = 0x10000001,
	VIDEO_MPEG2 = 0x10000002,
	VIDEO_MPEG4 = 0x10000004,
	VIDEO_H264 = 0x10000005,
	VIDEO_H265 = 0x10000006,
	VIDEO_AVC = GX_FOURCC('a', 'v', 'c', '1'),
	VIDEO_VC1 = GX_FOURCC('W', 'V', 'C', '1'),
	VIDEO_AVS = 0x42,
	AUDIO_MP2 = 0x50,
	AUDIO_A52 = 0x2000,
	AUDIO_DTS = 0x2001,
	AUDIO_LPCM_BE = 0x10001,
	AUDIO_AAC = GX_FOURCC('M', 'T', 'A', 'L'),
	AUDIO_ADTS = GX_FOURCC('A', 'D', 'T', 'S'),
	SPU_DVD = 0x3000000,
	SPU_DVB = 0x3000001,
	SPU_TXT = 0x3000002,
	PES_PRIVATE1 = 0xBD00000,
	SL_PES_STREAM = 0xD000000,
	SL_SECTION = 0xD100000,
	MP4_OD = 0xD200000,
} ESStreamType;

typedef struct MpegTSContext MpegTSContext;

void avpriv_mpegts_parse_close(MpegTSContext *ts);
MpegTSContext *avpriv_mpegts_parse_open(AVFormatContext *s);

int avpriv_mpegts_parse_packet(MpegTSContext *ts, AVPacket *pkt,
                               const uint8_t *buf, int len);
#endif /* AVFORMAT_MPEGTS_H */
