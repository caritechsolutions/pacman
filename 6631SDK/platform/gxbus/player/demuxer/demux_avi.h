#ifndef _DEMUX_AVI_H_
#define _DEMUX_AVI_H_

#include "gx_demux.h"
#include "stheader.h"
#include "avi_header.h"
#include "../avutil/common.h"

typedef struct {
	// index stuff:
	void *idx;
	int idx_size;
	off_t idx_pos;
	off_t idx_pos_a;
	off_t idx_pos_v;
	off_t idx_offset;	// ennyit kell hozzaadni az index offset ertekekhez
	// bps-based PTS stuff:
	int video_pack_no;
	int audio_block_size;
	off_t audio_block_no;
	// interleaved PTS stuff:
	int skip_video_frames;
	int audio_streams;
	float avi_audio_pts;
	float avi_video_pts;
	float pts_correction;
	unsigned int pts_corr_bytes;
	unsigned char pts_corrected;
	unsigned char pts_has_video;
	unsigned int numberofframes;
	AVISuperIndexChunk *suidx;
	int suidx_size;
	int isodml;
} DemuxAviPriv;

#endif
