#include "bluray_pcm.h"
#include "demux_ts.h"
#include "../avutil/samplefmt.h"
#include "../avcodec/bytestream.h"
#include "../avutil/channel_layout.h"

#define FFALIGN(x, a) (((x)+(a)-1)&~((a)-1))

int pcm_bluray_parse_header(BlurayPcmHeader *hdr, const uint8_t *buf)
{
	int bits_per_coded_sample;
	static const uint8_t bits_per_samples[4] = { 0, 16, 20, 24 };
	static const uint32_t channel_layouts[16] = {
		0, AV_CH_LAYOUT_MONO, 0, AV_CH_LAYOUT_STEREO, AV_CH_LAYOUT_SURROUND,
		AV_CH_LAYOUT_2_1, AV_CH_LAYOUT_4POINT0, AV_CH_LAYOUT_2_2,
		AV_CH_LAYOUT_5POINT0, AV_CH_LAYOUT_5POINT1, AV_CH_LAYOUT_7POINT0,
		AV_CH_LAYOUT_7POINT1, 0, 0, 0, 0};
	static const uint8_t channels[16] = {0, 1, 0, 2, 3, 3, 4, 4, 5, 6, 7, 8, 0, 0, 0, 0};
	uint8_t channel_layout = buf[2] >> 4;

	bits_per_coded_sample = bits_per_samples[buf[3] >> 6];
	if (!(bits_per_coded_sample == 16 || bits_per_coded_sample == 24)) {
		return -1;
	}
	hdr->sample_format = (bits_per_coded_sample == 16) ? AV_SAMPLE_FMT_S16
		: AV_SAMPLE_FMT_S32;
	if (hdr->sample_format == AV_SAMPLE_FMT_S16)
		hdr->sample_size = 16;
	else if (hdr->sample_format == AV_SAMPLE_FMT_S32)
		hdr->sample_size = 32;

	switch (buf[2] & 0x0f) {
	case 1:
		hdr->sample_rate = 48000;
		break;
	case 4:
		hdr->sample_rate = 96000;
		break;
	case 5:
		hdr->sample_rate = 192000;
		break;
	default:
		hdr->sample_rate = 0;
		return -1;
	}
	hdr->big_endian = 1;
	hdr->channels =  channels[channel_layout];
	hdr->channel_layout = channel_layouts[channel_layout];
	if (!hdr->channels) {
		return -1;
	}
	//hdr->o_bps = FFALIGN(hdr->channels, 2) * hdr->sample_rate * bits_per_coded_sample;

	return 0;
}

GxDemuxPacket* lpcm_swith_to_pcm(GxDemuxer* demuxer, BlurayPcmHeader *hdr, uint8_t* buf, int buf_size)
{
	int num_source_channels, channel;
	int sample_size, samples;
	GxDemuxPacket* dp = NULL;
	int i = 0, j = 0;
	uint8_t* tmp_buf = av_mallocz(2*buf_size);
	GetByteContext gb;
	int32_t *dst32 = (int32_t *)tmp_buf;

	if (tmp_buf == NULL) return NULL;

	num_source_channels = FFALIGN(hdr->channels, 2);
	sample_size = (num_source_channels *
			(hdr->sample_format == AV_SAMPLE_FMT_S16 ? 16 : 24)) >> 3;
	samples = buf_size / sample_size;
	bytestream2_init(&gb, buf, buf_size);

	if (samples) {
		switch (hdr->channel_layout) {
			/* cases with same number of source and coded channels */
		case AV_CH_LAYOUT_STEREO:
		case AV_CH_LAYOUT_4POINT0:
		case AV_CH_LAYOUT_2_2:
			samples *= num_source_channels;
			if (AV_SAMPLE_FMT_S16 == hdr->sample_format) {
				memcpy(tmp_buf, buf, buf_size);
				j = buf_size;
			} else {
				do {
					*dst32++ = bytestream2_get_le24(&gb);
					j += 4;
					i += 3;
				} while (--samples);
			}
			break;
			/* cases where number of source channels = coded channels + 1 */
		case AV_CH_LAYOUT_MONO:
		case AV_CH_LAYOUT_SURROUND:
		case AV_CH_LAYOUT_2_1:
		case AV_CH_LAYOUT_5POINT0:
			if (AV_SAMPLE_FMT_S16 == hdr->sample_format) {
				do {
					channel = hdr->channels;
					do {
						memcpy(tmp_buf+j, buf+i, 2);
						j += 2;
						i += 2;
					} while (--channel);
					i += 2;
				} while (--samples);
			} else {
				do {
					channel = hdr->channels;
					do {
						*dst32++ = bytestream2_get_le24(&gb);
						j += 4;
						i += 3;
					} while (--channel);
					bytestream2_skip(&gb, 3);
					i += 3;
				} while (--samples);
			}
			break;
			/* remapping: L, R, C, LBack, RBack, LF */
		case AV_CH_LAYOUT_5POINT1:
			if (AV_SAMPLE_FMT_S16 == hdr->sample_format) {
				do {
					memcpy(tmp_buf+j, buf+i, 2);
					memcpy(tmp_buf+j+2, buf+i+2, 2);
					memcpy(tmp_buf+j+4, buf+i+4, 2);
					memcpy(tmp_buf+j+8, buf+i+6, 2);
					memcpy(tmp_buf+j+10, buf+i+8, 2);
					memcpy(tmp_buf+j+6, buf+i+10, 2);
					j += 12;
					i += 12;
				} while (--samples);
			} else {
				do {
					dst32[0] = bytestream2_get_le24(&gb);
					dst32[1] = bytestream2_get_le24(&gb);
					dst32[2] = bytestream2_get_le24(&gb);
					dst32[4] = bytestream2_get_le24(&gb);
					dst32[5] = bytestream2_get_le24(&gb);
					dst32[3] = bytestream2_get_le24(&gb);
					dst32 += 6;
					j += 24;
					i += 18;
				} while (--samples);
			}
			break;
			/* remapping: L, R, C, LSide, LBack, RBack, RSide, <unused> */
		case AV_CH_LAYOUT_7POINT0:
			if (AV_SAMPLE_FMT_S16 == hdr->sample_format) {
				do {
					memcpy(tmp_buf+j, buf+i, 2);
					memcpy(tmp_buf+j+2, buf+i+2, 2);
					memcpy(tmp_buf+j+4, buf+i+4, 2);
					memcpy(tmp_buf+j+8, buf+i+6, 2);
					memcpy(tmp_buf+j+10, buf+i+8, 2);
					memcpy(tmp_buf+j+6, buf+i+10, 2);
					memcpy(tmp_buf+j+12, buf+i+12, 2);
					j += 14;
					i += 16;
				} while (--samples);
			} else {
				do {
					dst32[0] = bytestream2_get_le24(&gb);
					dst32[1] = bytestream2_get_le24(&gb);
					dst32[2] = bytestream2_get_le24(&gb);
					dst32[5] = bytestream2_get_le24(&gb);
					dst32[3] = bytestream2_get_le24(&gb);
					dst32[4] = bytestream2_get_le24(&gb);
					dst32[6] = bytestream2_get_le24(&gb);
					dst32 += 7;
					j += 28;
					i += 24;
				} while (--samples);
			}
			break;
			/* remapping: L, R, C, LSide, LBack, RBack, RSide, LF */
		case AV_CH_LAYOUT_7POINT1:
			if (AV_SAMPLE_FMT_S16 == hdr->sample_format) {
				do {
					memcpy(tmp_buf+j, buf+i, 2);
					memcpy(tmp_buf+j+2, buf+i+2, 2);
					memcpy(tmp_buf+j+4, buf+i+4, 2);
					memcpy(tmp_buf+j+12, buf+i+6, 2);
					memcpy(tmp_buf+j+8, buf+i+8, 2);
					memcpy(tmp_buf+j+10, buf+i+10, 2);
					memcpy(tmp_buf+j+14, buf+i+12, 2);
					memcpy(tmp_buf+j+6, buf+i+14, 2);
					i += 16;
					j += 16;
				} while (--samples);
			} else {
				do {
					dst32[0] = bytestream2_get_le24(&gb);
					dst32[1] = bytestream2_get_le24(&gb);
					dst32[2] = bytestream2_get_le24(&gb);
					dst32[6] = bytestream2_get_le24(&gb);
					dst32[4] = bytestream2_get_le24(&gb);
					dst32[5] = bytestream2_get_le24(&gb);
					dst32[7] = bytestream2_get_le24(&gb);
					dst32[3] = bytestream2_get_le24(&gb);
					dst32 += 8;
					j += 32;
					i += 24;
				} while (--samples);
			}
			break;
		}
		dp = GxDemuxPacket_Create(demuxer, NULL, j);
		if (dp)
			GxDemuxPacket_Write(dp, tmp_buf, j);
	}
	av_free(tmp_buf);

	return dp;
}

