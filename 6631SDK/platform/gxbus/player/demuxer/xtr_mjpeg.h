#ifndef __XTR_MJPG__
#define __XTR_MJPG__

#include "demux_lavf.h"
#include "put_bits.h"
#include "bytestream.h"
#include "../decoder/tjpgd.h"

#define DTH_JPG_LEN (420)

extern int demux_lavf_fill_buffer(GxDemuxer* demuxer, GxDemuxStream* dsds);
int xtr_mjpeg_check(GxDemuxer* demuxer);
int xtr_mjpeg_packet(uint8_t* buffer, int src_len, GxDemuxPacket *dp_dst);

#endif
