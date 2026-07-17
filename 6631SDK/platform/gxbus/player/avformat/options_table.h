/*
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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

#ifndef AVFORMAT_OPTIONS_TABLE
#define AVFORMAT_OPTIONS_TABLE

#include <limits.h>

#include "../avutil/opt.h"
#include "avformat.h"

#define OFFSET(x) offsetof(AVFormatContext,x)
#define DEFAULT 0 //should be NAN but it does not work as it is not a constant in glibc as required by ANSI/ISO C
//these names are too long to be readable
#define E AV_OPT_FLAG_ENCODING_PARAM
#define D AV_OPT_FLAG_DECODING_PARAM

static const AVOption format_options[]={
{"avioflags", NULL, OFFSET(flags), AV_OPT_TYPE_FLAGS, {.i64 = DEFAULT}, INT_MIN, INT_MAX, D|E, "avioflags"},
{"direct", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVIO_FLAG_DIRECT}, INT_MIN, INT_MAX, D|E, "avioflags"},
{"probesize", NULL, OFFSET(probesize), AV_OPT_TYPE_INT, {.i64 = PROBE_BUF_MAX}, 32, INT_MAX, D, NULL},
{"packetsize", NULL, OFFSET(packet_size), AV_OPT_TYPE_INT, {.i64 = DEFAULT}, 0, INT_MAX, E, NULL},
{"fflags", NULL, OFFSET(flags), AV_OPT_TYPE_FLAGS, {.i64 = DEFAULT}, INT_MIN, INT_MAX, D|E, "fflags"},
{"ignidx", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_IGNIDX}, INT_MIN, INT_MAX, D, "fflags"},
{"genpts", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_GENPTS}, INT_MIN, INT_MAX, D, "fflags"},
{"nofillin", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_NOFILLIN}, INT_MIN, INT_MAX, D, "fflags"},
{"noparse", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_NOPARSE}, INT_MIN, INT_MAX, D, "fflags"},
{"igndts", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_IGNDTS}, INT_MIN, INT_MAX, D, "fflags"},
{"discardcorrupt", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_DISCARD_CORRUPT}, INT_MIN, INT_MAX, D, "fflags"},
{"sortdts", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_SORT_DTS}, INT_MIN, INT_MAX, D, "fflags"},
{"keepside", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_KEEP_SIDE_DATA}, INT_MIN, INT_MAX, D, "fflags"},
{"latm", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AVFMT_FLAG_MP4A_LATM}, INT_MIN, INT_MAX, E, "fflags"},
{"analyzeduration", NULL, OFFSET(max_analyze_duration), AV_OPT_TYPE_INT, {.i64 = 0}, 0, INT_MAX, D, NULL},
{"cryptokey", NULL, OFFSET(key), AV_OPT_TYPE_BINARY, {.dbl = 0}, 0, 0, D, NULL},
{"indexmem", NULL, OFFSET(max_index_size), AV_OPT_TYPE_INT, {.i64 = 1<<20}, 0, INT_MAX, D, NULL},
//{"rtbufsize", NULL, OFFSET(max_picture_buffer), AV_OPT_TYPE_INT, {.dbl = 3041280 }, 0, INT_MAX, D}, /* defaults to 1s of 15fps 352x288 YUYV422 video */
//{"fdebug", NULL, OFFSET(debug), AV_OPT_TYPE_FLAGS, {.dbl = DEFAULT }, 0, INT_MAX, E|D, "fdebug"},
//{"ts", NULL, 0, AV_OPT_TYPE_CONST, {.dbl = FF_FDEBUG_TS }, INT_MIN, INT_MAX, E|D, "fdebug"},
{"max_delay", NULL, OFFSET(max_delay), AV_OPT_TYPE_INT, {.i64 = -1}, -1, INT_MAX, E|D, NULL},
{"fpsprobesize", NULL, OFFSET(fps_probe_size), AV_OPT_TYPE_INT, {.i64 = -1}, -1, INT_MAX-1, D, NULL},
{"audio_preload", NULL, OFFSET(audio_preload), AV_OPT_TYPE_INT, {.i64 = 0}, 0, INT_MAX-1, E, NULL},
{"chunk_duration", NULL, OFFSET(max_chunk_duration), AV_OPT_TYPE_INT, {.i64 = 0}, 0, INT_MAX-1, E, NULL},
{"chunk_size", NULL, OFFSET(max_chunk_size), AV_OPT_TYPE_INT, {.i64 = 0}, 0, INT_MAX-1, E, NULL},
/* this is a crutch for avconv, since it cannot deal with identically named options in different contexts.
 * to be removed when avconv is fixed */
{"f_err_detect", NULL, OFFSET(error_recognition), AV_OPT_TYPE_FLAGS, {.i64 = AV_EF_CRCCHECK}, INT_MIN, INT_MAX, D, "err_detect"},
{"err_detect", NULL, OFFSET(error_recognition), AV_OPT_TYPE_FLAGS, {.i64 = AV_EF_CRCCHECK}, INT_MIN, INT_MAX, D, "err_detect"},
{"crccheck", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AV_EF_CRCCHECK}, INT_MIN, INT_MAX, D, "err_detect"},
{"bitstream", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AV_EF_BITSTREAM}, INT_MIN, INT_MAX, D, "err_detect"},
{"buffer", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AV_EF_BUFFER}, INT_MIN, INT_MAX, D, "err_detect"},
{"explode", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AV_EF_EXPLODE}, INT_MIN, INT_MAX, D, "err_detect"},
{"careful",    NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AV_EF_CAREFUL}, INT_MIN, INT_MAX, D, "err_detect"},
{"compliant",  NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AV_EF_COMPLIANT}, INT_MIN, INT_MAX, D, "err_detect"},
{"aggressive", NULL, 0, AV_OPT_TYPE_CONST, {.i64 = AV_EF_AGGRESSIVE}, INT_MIN, INT_MAX, D, "err_detect"},
{NULL},
};

#undef E
#undef D
#undef DEFAULT
#undef OFFSET

#endif // AVFORMAT_OPTIONS_TABLE
