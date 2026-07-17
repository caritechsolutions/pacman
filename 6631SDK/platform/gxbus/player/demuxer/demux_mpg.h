#ifndef _DEMUX_MPG_H_
#define _DEMUX_MPG_H_

#include "demux_audio.h"

#include "stheader.h"
#include "parse_es.h"

#define MAX_PS_PACKETSIZE   (224*1024)

#define UNKNOWN             0
#define VIDEO_MPEG1         0x10000001
#define VIDEO_MPEG2         0x10000002
#define VIDEO_MPEG4         0x10000004
#define VIDEO_H264          0x10000005
#define AUDIO_MP2           0x50
#define AUDIO_A52           0x2000
#define AUDIO_LPCM_BE       0x10001
#define AUDIO_AAC           GX_FOURCC('M', 'P', '4', 'A')

// 200000 is a wild guess
#define TIMESTAMP_PROBE_LEN 100000

//MAX_PTS_DIFF_FOR_CONSECUTIVE denotes the maximum difference
//between two pts to consider them consecutive
// 1.0 is a wild guess
#define MAX_PTS_DIFF_FOR_CONSECUTIVE 1.0

typedef struct mpg_demuxer {
	float last_pts;
	float first_pts;	// first pts found in stream
	float first_to_final_pts_len;	// difference between final pts and first pts
	int has_valid_timestamps;	// !=0 iff time stamps look linear
	// (not necessarily starting with 0)
	unsigned int es_map[0x40];	//es map of stream types (associated to the pes id) from 0xb0 to 0xef
	int num_a_streams;
	int a_stream_ids[MAX_A_STREAMS];
	GxParseES es_parser;
    float   bps;
    int64_t lastseekpts;
    int64_t lastseekpos;
} DemuxMpgPriv;

#endif
