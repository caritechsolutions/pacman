#ifndef __XTR_MPEG4_
#define __XTR_MPEG4_

#include "demux_lavf.h"
#include "put_bits.h"
#include "bytestream.h"

#define MPEG4_EXTRA_LEN       64

extern int demux_lavf_fill_buffer(GxDemuxer* demuxer, GxDemuxStream* dsds);
void xtr_mpeg4_check(GxDemuxer* demuxer);
int  xtr_mpeg4_packet(GxStreamVideoHeader* sh, uint8_t* buffer, int src_len, GxDemuxPacket *dp_dst);

#endif
