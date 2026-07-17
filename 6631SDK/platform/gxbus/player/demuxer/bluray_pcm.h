#ifndef __BLURAY_PCM_H__
#define __BLURAY_PCM_H__

#include "gx_demux.h"

typedef struct {
	int channels;
	int channel_layout;
	int big_endian;
	int sample_rate;
	int sample_size;
	int sample_format;
}BlurayPcmHeader;

int pcm_bluray_parse_header(BlurayPcmHeader *hdr, const uint8_t *buf);
GxDemuxPacket* lpcm_swith_to_pcm(GxDemuxer* demuxer, BlurayPcmHeader *hdr, uint8_t* buf, int buf_size);
#endif
