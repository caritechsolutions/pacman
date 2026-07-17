#ifndef _DEMUX_LAVF_H_
#define _DEMUX_LAVF_H_

#include "gx_demux.h"
#include "stheader.h"

#include "../avformat/avformat.h"
#include "../avformat/riff.h"

#define PROBE_BUF_SIZE (1<<20)
#define BIO_BUFFER_SIZE 32768

typedef struct {
	GxFifo*  fifo;
	int32_t  pid;
	int32_t  right;
	int32_t  total_len;
	int32_t  len;
	uint8_t* data;
}LavfDvbSubFilter;

typedef struct  {
	AVInputFormat *iformat;
	AVFormatContext *avfc;
	AVDictionary *opts;
	uint8_t buffer[BIO_BUFFER_SIZE];
	int audio_streams;
	int video_streams;
	int sub_streams;
	int audio_index;
	int video_index;
	int sub_index;
	int sub_fill;
	int64_t last_pts;
	int astreams[MAX_A_STREAMS];
	int vstreams[MAX_V_STREAMS];
	int sstreams[MAX_S_STREAMS];
	int nal_size_size;
	int abort;
	int check_first_apkt;
	int extradata_size;
	int page_seguence;
	int protocol_type;
	LavfDvbSubFilter  sub_filter;
	int switch_state;
	int new_video_index;
	int new_audio_index;
	int new_id;
	void *new_sh;
} DemuxLavfPriv;

typedef enum
{
	GX_PROTOLCOL_UNKNOWN=0,
	GX_PROTOLCOL_FILE,
	GX_PROTOLCOL_HTTP,
	GX_PROTOLCOL_HTTPS,
	GX_PROTOLCOL_HLS,
	GX_PROTOLCOL_UDP,
	GX_PROTOLCOL_RTP,
	GX_PROTOLCOL_RTMP,
	GX_PROTOLCOL_RTSP,
	GX_PROTOLCOL_MMS,
	GX_PROTOLCOL_CONCAT,
}GX_SUPPORT_PROTOCOL;

typedef struct
{
	int iProtocol;
	char acProtocol[12];
}GX_LAVF_SUPPORT_PROTOCOL;

//add begin.
typedef struct LavfVideoTrackInfo {
	int vid_variant_bitrate;
	int vid_width;
	int vid_height;
	int vid_frameRate;
	char *vid_id;
	char *vid_codecs;
	char *vid_sar;
} LavfVideoTrackInfo;

typedef struct LavfAudioTrackInfo {
	int bandwidth;
	int aud_samplerate;
	char *aud_variant_bitrate;
	char *aud_id;
	char *aud_language;
	char *aud_codecs;
} LavfAudioTrackInfo;

typedef struct LavfSubtitlesTrackInfo {
	char *sub_id;
	char *sub_language;
	char *sub_codecs;
} LavfSubtitlesTrackInfo;

typedef struct LavfMulitStreamInformation {
	int cur_vid_idx;
	int vid_max;
	LavfVideoTrackInfo *video_track;
	int cur_aud_idx;
	int audio_max;
	LavfAudioTrackInfo *audio_track;
	int cur_sub_idx;
	int subtiles_max;
	LavfSubtitlesTrackInfo *subtitles_track;
} LavfMulitStreamInformation;

/*continuous network video clips*/
typedef struct lavf_cont_stream_clips {
	int cur_periods;
	//int cur_periods_idx;
	int n_periods;
	LavfMulitStreamInformation **periods_stream_info;
} lavf_cont_stream_clips;
//add end.

#endif
