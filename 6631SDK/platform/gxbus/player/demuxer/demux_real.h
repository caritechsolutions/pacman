#ifndef _DEMUX_REAL_H_
#define _DEMUX_REAL_H_

#include "gx_demux.h"
#include "stheader.h"

#include "../avutil/common.h"

#define SKIP_BITS(n) buffer<<=n
#define SHOW_BITS(n) ((buffer)>>(32-(n)))

typedef struct real_index_table_t {
	unsigned int timestamp;
	int offset;
} RealIndexTable;

typedef struct {
	/* for seeking */
	int index_chunk_offset;
	RealIndexTable **index_table;

	int *index_table_size;
	int *index_av_malloc_size;
	int data_chunk_offset;
	int num_of_packets;
	int current_packet;

	// need for seek
	int audio_need_keyframe;
	int video_after_seek;

	int current_apacket;
	int current_vpacket;

	// timestamp correction:
	int64_t kf_base;	// timestamp of the prev. video keyframe
	unsigned int kf_pts;	// timestamp of next video keyframe
	unsigned int a_pts;	// previous audio timestamp
	double v_pts;		// previous video timestamp
	unsigned long duration;

	/**
        * Used to demux multirate files
        */
	int is_multirate;	///< != 0 for multirate files
	int *str_data_offset;	///< Data chunk offset for every audio/video stream
	int audio_curpos;	///< Current file position for audio demuxing
	int video_curpos;	///< Current file position for video demuxing
	int a_num_of_packets;	///< Number of audio packets
	int v_num_of_packets;	///< Number of video packets
	int a_idx_ptr;		///< Audio index position pointer
	int v_idx_ptr;		///< Video index position pointer
	int a_bitrate;		///< Audio bitrate
	int v_bitrate;		///< Video bitrate
	int stream_switch;	///< Flag used to switch audio/video demuxing

	/**
        * Used to reorder audio data
        */
	unsigned int *intl_id;	///< interleaver id, per stream
	int *sub_packet_size;	///< sub packet size, per stream
	int *sub_packet_h;	///< number of coded frames per block
	int *coded_framesize;	///< coded frame size, per stream
	int *audiopk_size;	///< audio packet size
	unsigned char *audio_buf;	///< place to store reordered audio data
	double *audio_timestamp;	///< timestamp for each audio packet
	int sub_packet_cnt;	///< number of subpacket already received
	int audio_filepos;	///< file position of first audio packet in block
} DemuxRealPriv;

#pragma pack(1)
typedef struct real_video_demux_info {
	uint32_t sync1;
	uint32_t sync2;
	uint32_t sync3;
	uint32_t sync4;
	uint32_t len;
	uint32_t timestamp;
	uint16_t seqnum;
	uint16_t flags;
	uint32_t lastpacket;
	uint32_t segnum;
} RealDemuxVideoHeader;
#pragma pack()

#pragma pack(1)
typedef struct real_audio_demux_info {
	uint32_t u32DataLen;
	uint16_t u16Reserved;
    uint32_t u32Timestamp;
    uint16_t u16DataFlags;
} RealDemuxAudioHeader;
#pragma pack()

#pragma pack(1)
typedef struct real_video_packet_info {
    uint32_t   u32Length;
    uint32_t   u32MOFTag;
    uint32_t   u32SubMOFTag;
    uint16_t   u16Width;
    uint16_t   u16Height;
    uint16_t   u16BitCount;
    uint16_t   u16PadWidth;
    uint16_t   u16PadHeight;
    uint32_t   u32FramesPerSecond;
} RealPacketVideoHeader;
#pragma pack()

//! use at most 200 MB of memory for index, corresponds to around 25 million entries
#define MAX_INDEX_ENTRIES (200*1024*1024 / sizeof(RealIndexTable))

#endif
