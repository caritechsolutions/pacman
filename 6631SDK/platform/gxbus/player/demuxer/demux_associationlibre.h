#ifndef _DEMUX_LAVF_H_
#define _DEMUX_LAVF_H_

#include "gx_demux.h"
#include "stheader.h"

#include "../avformat/avformat.h"
#include "../avformat/riff.h"

#define PROBE_BUF_SIZE (1<<20)
#define BIO_BUFFER_SIZE 32768

struct representation {
	AVInputFormat *avfc;
	AVFormatContext *ctx;
	ByteIOContext* pb;
	uint8_t buffer[BIO_BUFFER_SIZE];
	char *url;
	GxStream  *stream;
	int stream_index;
	int avstream_index;
	enum AVMediaType type;
	int64_t cur_timestamp;
	int64_t last_pts;
	int nal_size_size;
	int check_first_apkt;
	int extradata_size;
	int page_seguence;
	void *priv;
};


typedef struct  {
	int n_videos;
	struct representation **videos;
	int n_audios;
	struct representation **audios;
	int audio_streams;
	int video_streams;
	int audio_index;
	int video_index;
	int64_t last_pts;
	int astreams[MAX_A_STREAMS];
	int vstreams[MAX_V_STREAMS];
	int nal_size_size;
	int abort;
	int extradata_size;
	int page_seguence;
	int protocol_type;
	int check_first_apkt;
	void *priv;
	AVDictionary *opts;
} DemuxAssociationLibrePriv;

#endif
