#ifndef __DEMUX_MPEGDASH_H__
#define __DEMUX_MPEGDASH_H__

#include "gx_demux.h"
#include "gx_stream.h"
#include "stheader.h"

#include "../avformat/avformat.h"
#include "../avformat/riff.h"
#include "mpegdash/MultimediaStream.h"

#define PROBE_BUF_SIZE (1<<20)
#define BIO_BUFFER_SIZE 32768
typedef enum {
	DEMUX_MPEGDASH_VIDEO    = (1 << 0),
	DEMUX_MPEGDASH_AUDIO    = (1 << 1),
	DEMUX_MPEGDASH_SUBTITLE = (1 << 2),
	DEMUX_MPEGDASH_DATA     = (1 << 3),
	DEMUX_MPEGDASH_STREAM   = (1 << 4),

	DEMUX_MPEGDASH_UNKNOWN  = 0xffffffff,
} MpegDashStreamType;

typedef struct {
#define RECV_BUFFER_SIZE  256*1024
    MpegDashStreamType type;
    AVInputFormat      *avif;
    AVFormatContext    *avfc;
    ByteIOContext      *pb;
    uint8_t            buffer[BIO_BUFFER_SIZE];

	uint8_t            recv_buffer[RECV_BUFFER_SIZE];
	unsigned int       buf_pos, buf_len;
	unsigned int       pos, start_pos, end_pos;

	int64_t            last_pts;
	int                abort;
	int                check_first_apkt;
	int                eof;
} MpegDashStream;

typedef struct  {
#define MAX_MPEGDASH_STREAM_NUM (3)
	int   pthread_depack;

	unsigned int   audio_streams;
	unsigned int   video_streams;
	unsigned int   sub_streams;
	unsigned int   astreams[MAX_A_STREAMS];
	unsigned int   vstreams[MAX_V_STREAMS];
	unsigned int   sstreams[MAX_S_STREAMS];

	unsigned int   stream_index;
	unsigned int   stream_num;
	MpegDashStream *stream[MAX_MPEGDASH_STREAM_NUM];

	unsigned int   periodNum;
	MpegDashPeriod *period;

	unsigned int   nal_size_size;
	unsigned int   extradata_size;

	void           *priv;
} DemuxMpegDashPriv;


#endif
