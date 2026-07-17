#ifndef _DEMUX_OGG_H_
#define _DEMUX_OGG_H_

#include "gx_demux.h"
#include "stheader.h"

#include "../avutil/intreadwrite.h"
#include "../avutil/common.h"

#include "tremor/ivorbiscodec.h"

#ifdef HAVE_OGGTHEORA
#include "theora.h"
#endif

#ifdef HAVE_SUBTITLE
#include "sub.h"
#endif

#define BLOCK_SIZE                 4096
#define OGG_SUB_MAX_LINE           128

#define FOURCC_VORBIS              GX_FOURCC('v', 'r', 'b', 's')
#define FOURCC_SPEEX               GX_FOURCC('s', 'p', 'x', ' ')
#define FOURCC_THEORA              GX_FOURCC('t', 'h', 'e', 'o')

#define get_uint16(b)              AV_RL16(b)
#define get_uint32(b)              AV_RL32(b)
#define get_uint64(b)              AV_RL64(b)

#define NUM_VORBIS_HDR_PACKETS     3

/// Some defines from OggDS
#define PACKET_TYPE_HEADER         0x01
#define PACKET_TYPE_BITS           0x07
#define PACKET_LEN_BITS01          0xc0
#define PACKET_LEN_BITS2           0x02
#define PACKET_IS_SYNCPOINT        0x08

#ifdef HAVE_OGGTHEORA
typedef struct theora_struct {
	theora_state st;
	theora_comment cc;
	theora_info inf;
} OggTheora;
#endif

typedef struct stream_header_video {
	int32_t width;
	int32_t height;
} stream_header_video;

typedef struct stream_header_audio {
	int16_t channels;
	int16_t blockalign;
	int32_t avgbytespersec;
} stream_header_audio;

typedef struct __attribute__ ((__packed__)) stream_header {
	char streamtype[8];
	char subtype[4];

	int32_t size;		// size of the structure

	int64_t time_unit;	// in reference time
	int64_t samples_per_unit;
	int32_t default_len;	// in media time

	int32_t buffersize;
	int16_t bits_per_sample;

	int16_t padding;

	union {
		// Video specific
		stream_header_video video;
		// Audio specific
		stream_header_audio audio;
	} sh;
} stream_header;

/// Our private datas

typedef struct ogg_syncpoint {
	int64_t granulepos;
	off_t page_pos;
} ogg_syncpoint_t;

/// A logical stream
typedef struct ogg_stream {
	/// Timestamping stuff
	float samplerate;	/// granulpos 2 time
	int64_t lastpos;
	int32_t lastsize;

	// Logical stream state
	ogg_stream_state stream;
	int hdr_packets;
	int vorbis;
	int speex;
	int theora;
	int flac;
	int text;
	int id;

	vorbis_info vi;
	int vi_inited;

	void *ogg_d;
} ogg_stream_t;

typedef struct ogg_demuxer {
	/// Physical stream state
	ogg_sync_state sync;
	/// Current page
	ogg_page page;
	/// Logical streams
	ogg_stream_t *subs;
	int num_sub;
	ogg_syncpoint_t *syncpoints;
	int num_syncpoint;
	off_t pos, last_size;
	int64_t final_granulepos;

	/* Used for subtitle switching. */
	int n_text;
	int *text_ids;
	char **text_langs;
} DemuxOggPriv;

#endif
