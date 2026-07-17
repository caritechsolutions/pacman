/*
 *  utils for libavcodec
 *  Copyright (c) 2001 Fabrice Bellard.
 *  Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
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

/**
 *  @file utils.c
 *  utils.
 */

#include "avcodec.h"

void avcodec_get_context_defaults2(AVCodecContext*  s, enum CodecType codec_type)
{
	memset(s, 0, sizeof(AVCodecContext));

	s->codec_type = codec_type;
	s->rc_eq = "tex^qComp";
	s->time_base = (AVRational) {
		0, 1};
	s->sample_aspect_ratio = (AVRational) {
		0, 1};
	s->pix_fmt = PIX_FMT_NONE;
	s->sample_fmt = SAMPLE_FMT_S16;	// FIXME: set to NONE

	s->palctrl = NULL;
}

AVCodecContext* avcodec_alloc_context2(enum CodecType codec_type)
{
	AVCodecContext* avctx = av_mallocz(sizeof(AVCodecContext));

	if (avctx == NULL)
		return NULL;

	avcodec_get_context_defaults2(avctx, codec_type);

	return avctx;
}

AVCodecContext* avcodec_alloc_context(void)
{
	return avcodec_alloc_context2(CODEC_TYPE_UNKNOWN);
}

int av_get_bits_per_sample(enum CodecID codec_id)
{
	switch (codec_id) {
		case CODEC_ID_ADPCM_SBPRO_2:
			return 2;
		case CODEC_ID_ADPCM_SBPRO_3:
			return 3;
		case CODEC_ID_ADPCM_SBPRO_4:
		case CODEC_ID_ADPCM_CT:
			return 4;
		case CODEC_ID_PCM_ALAW:
		case CODEC_ID_PCM_MULAW:
		case CODEC_ID_PCM_S8:
		case CODEC_ID_PCM_U8:
			return 8;
		case CODEC_ID_PCM_S16BE:
		case CODEC_ID_PCM_S16LE:
		case CODEC_ID_PCM_U16BE:
		case CODEC_ID_PCM_U16LE:
			return 16;
		case CODEC_ID_PCM_S24DAUD:
		case CODEC_ID_PCM_S24BE:
		case CODEC_ID_PCM_S24LE:
		case CODEC_ID_PCM_U24BE:
		case CODEC_ID_PCM_U24LE:
			return 24;
		case CODEC_ID_PCM_S32BE:
		case CODEC_ID_PCM_S32LE:
		case CODEC_ID_PCM_U32BE:
		case CODEC_ID_PCM_U32LE:
			return 32;
		default:
			return 0;
	}
}

int av_get_audio_frame_duration(AVCodecContext *avctx, int frame_bytes)
{
	int id, sr, ch, ba, tag, bps;

	id  = avctx->codec_id;
	sr  = avctx->sample_rate;
	ch  = avctx->channels;
	ba  = avctx->block_align;
	tag = avctx->codec_tag;
	bps = av_get_bits_per_sample(avctx->codec_id);

	/* codecs with an exact constant bits per sample */
	if (bps > 0 && ch > 0 && frame_bytes > 0 && ch < 32768 && bps < 32768)
		return (frame_bytes * 8LL) / (bps * ch);
	bps = avctx->bits_per_coded_sample;

	/* codecs with a fixed packet duration */
	switch (id) {
		case CODEC_ID_ADPCM_ADX:    return   32;
		case CODEC_ID_ADPCM_IMA_QT: return   64;
//		case CODEC_ID_ADPCM_EA_XAS: return  128;
		case CODEC_ID_AMR_NB:
		case CODEC_ID_GSM:
		case CODEC_ID_QCELP:
		case CODEC_ID_RA_144:
		case CODEC_ID_RA_288:       return  160;
		case CODEC_ID_IMC:          return  256;
		case CODEC_ID_AMR_WB:
		case CODEC_ID_GSM_MS:       return  320;
		case CODEC_ID_MP1:          return  384;
//		case CODEC_ID_ATRAC1:       return  512;
		case CODEC_ID_ATRAC3:       return 1024;
		case CODEC_ID_MP2:
		case CODEC_ID_MUSEPACK7:    return 1152;
		case CODEC_ID_AC3:          return 1536;
		default:                    break;
	}

	if (sr > 0) {
		/* calc from sample rate */
		if (id == CODEC_ID_TTA)
			return 256 * sr / 245;

//		if (ch > 0) {
//			/* calc from sample rate and channels */
//			if (id == CODEC_ID_BINKAUDIO_DCT)
//				return (480 << (sr / 22050)) / ch;
//		}
	}

	if (ba > 0) {
		/* calc from block_align */
		if (id == CODEC_ID_SIPR) {
			switch (ba) {
				case 20: return 160;
				case 19: return 144;
				case 29: return 288;
				case 37: return 480;
				default: break;
			}
		}
	}

	if (frame_bytes > 0) {
		/* calc from frame_bytes only */
		if (id == CODEC_ID_TRUESPEECH)
			return 240 * (frame_bytes / 32);
		if (id == CODEC_ID_NELLYMOSER)
			return 256 * (frame_bytes / 64);

		if (bps > 0) {
			/* calc from frame_bytes and bits_per_coded_sample */
			if (id == CODEC_ID_ADPCM_G726)
				return frame_bytes * 8 / bps;
		}

		if (ch > 0) {
			/* calc from frame_bytes and channels */
			switch (id) {
				case CODEC_ID_ADPCM_4XM:
//				case CODEC_ID_ADPCM_IMA_ISS:
					return (frame_bytes - 4 * ch) * 2 / ch;
				case CODEC_ID_ADPCM_IMA_SMJPEG:
					return (frame_bytes - 4) * 2 / ch;
				case CODEC_ID_ADPCM_IMA_AMV:
					return (frame_bytes - 8) * 2 / ch;
				case CODEC_ID_ADPCM_XA:
					return (frame_bytes / 128) * 224 / ch;
				case CODEC_ID_INTERPLAY_DPCM:
					return (frame_bytes - 6 - ch) / ch;
				case CODEC_ID_ROQ_DPCM:
					return (frame_bytes - 8) / ch;
				case CODEC_ID_XAN_DPCM:
					return (frame_bytes - 2 * ch) / ch;
				case CODEC_ID_MACE3:
					return 3 * frame_bytes / ch;
				case CODEC_ID_MACE6:
					return 6 * frame_bytes / ch;
//				case CODEC_ID_PCM_LXF:
//					return 2 * (frame_bytes / (5 * ch));
				default:
					break;
			}

			if (tag) {
				/* calc from frame_bytes, channels, and codec_tag */
				if (id == CODEC_ID_SOL_DPCM) {
					if (tag == 3)
						return frame_bytes / ch;
					else
						return frame_bytes * 2 / ch;
				}
			}

			if (ba > 0) {
				/* calc from frame_bytes, channels, and block_align */
				int blocks = frame_bytes / ba;
				switch (avctx->codec_id) {
					case CODEC_ID_ADPCM_IMA_WAV:
						return blocks * (1 + (ba - 4 * ch) / (4 * ch) * 8);
					case CODEC_ID_ADPCM_IMA_DK3:
						return blocks * (((ba - 16) * 2 / 3 * 4) / ch);
					case CODEC_ID_ADPCM_IMA_DK4:
						return blocks * (1 + (ba - 4 * ch) * 2 / ch);
					case CODEC_ID_ADPCM_MS:
						return blocks * (2 + (ba - 7 * ch) * 2 / ch);
					default:
						break;
				}
			}

			if (bps > 0) {
				/* calc from frame_bytes, channels, and bits_per_coded_sample */
				switch (avctx->codec_id) {
					case CODEC_ID_PCM_DVD:
						return 2 * (frame_bytes / ((bps * 2 / 8) * ch));
					case CODEC_ID_PCM_BLURAY:
						
						return frame_bytes / (((((ch)+(2)-1)&~((2)-1)) * bps) / 8);
//					case CODEC_ID_S302M:
//						return 2 * (frame_bytes / ((bps + 4) / 4)) / ch;
					default:
						break;
				}
			}
		}
	}

	return 0;
}
