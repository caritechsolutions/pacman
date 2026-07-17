#include "gx_demux.h"

extern int avpacket_probe_has_iframe(GxDemuxer* demuxer, unsigned char *buf, int size);

extern int avpacket_parse_header(GxDemuxer* demuxer, unsigned char *buf, int size);
