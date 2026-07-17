/*
 * ISO Media common code
 * copyright (c) 2001 Fabrice Bellard
 * copyright (c) 2002 Francois Revol <revol@free.fr>
 * copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@free.fr>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_ISOM_UTILS_H
#define AVFORMAT_ISOM_UTILS_H

#include "avio.h"
#include "riff.h"
#include "../avutil/encryption_info.h"

#define MAX_INDEX_MEMORY_MOV     (0x40000)
#define MAX_SUB_INDEX_MEMORY_MOV (0x8000)
#define MOV_INDEX_COUNT          ((int)(MAX_INDEX_MEMORY_MOV/sizeof(AVIndexEntry)))
#define BACKWARD_INDEX_COUNT     ((int)(MAX_INDEX_MEMORY_MOV/sizeof(AVIndexEntry)/2))
#define MOV_SUB_INDEX_COUNT      ((int)(MAX_SUB_INDEX_MEMORY_MOV/sizeof(AVIndexEntry)))
#define BACKWARD_SUB_INDEX_COUNT ((int)(MAX_SUB_INDEX_MEMORY_MOV/sizeof(AVIndexEntry)/2))

typedef struct {
	unsigned char count;
	unsigned char duration;
} MOVCtts_8;

typedef struct {
	unsigned short count;
	unsigned short duration;
} MOVCtts_16;

typedef struct {
	unsigned int count;
	int duration;
} MOVCtts_32;

typedef struct {
	unsigned char count;
	unsigned char duration;
} MOVStts_8;

typedef struct {
	unsigned short count;
	unsigned short duration;
} MOVStts_16;

typedef struct {
	unsigned int count;
	unsigned int duration;
} MOVStts_32;

typedef struct {
	int first;
	int count;
	int id;
} MOVStsc;

typedef struct {
	uint32_t type;
	char *path;
} MOVDref;

typedef struct {
	uint32_t type;
	int64_t offset;
	int64_t size; /* total size (excluding the size and type fields) */
} MOVAtom;

typedef struct MOVElst {
	int64_t duration;
	int64_t time;
	float rate;
} MOVElst;


typedef struct {
	unsigned track_id;
	uint64_t base_data_offset;
	uint64_t moof_offset;
	unsigned stsd_id;
	unsigned duration;
	unsigned size;
	unsigned flags;
} MOVFragment;

typedef struct {
	unsigned track_id;
	unsigned stsd_id;
	unsigned duration;
	unsigned size;
	unsigned flags;
} MOVTrackExt;

typedef struct MOVEncryptionIndex {
	unsigned int nb_encrypted_samples;
	AVEncryptionInfo **encrypted_samples;

	uint8_t* auxiliary_info_sizes;
	size_t auxiliary_info_sample_count;
	uint8_t auxiliary_info_default_size;
	uint64_t* auxiliary_offsets;
	size_t auxiliary_offsets_count;
} MOVEncryptionIndex;

typedef struct MOVFragmentStreamInfo {
	int id;
	int64_t sidx_pts;
	int64_t first_tfra_pts;
	int64_t tfdt_dts;
	int index_entry;
	MOVEncryptionIndex *encryption_index;
} MOVFragmentStreamInfo;

typedef struct MOVFragmentIndexItem {
	int64_t moof_offset;
	int headers_read;
	int current;
	int nb_stream_info;
	MOVFragmentStreamInfo * stream_info;
} MOVFragmentIndexItem;

typedef struct MOVFragmentIndex {
	int allocated_size;
	int complete;
	int current;
	int nb_items;
	MOVFragmentIndexItem * item;
} MOVFragmentIndex;

typedef struct MOVStreamContext {
	ByteIOContext *pb;
	int pb_is_copied;
	int ffindex;          ///< AVStream index
	int next_chunk;
	unsigned int chunk_count;
	unsigned int chunk_bytes;
	void *chunk_offsets;
	unsigned int stts_count;
	unsigned int stts_bytes;
	void        *stts_data;
	unsigned int ctts_count;
	unsigned int ctts_bytes;
	void        *ctts_data;
	unsigned int stsc_count;
	MOVStsc     *stsc_data;
	unsigned int stps_count;
	unsigned *stps_data;  ///< partial sync sample for mpeg-2 open gop
	MOVElst *elst_data;
	unsigned int elst_count;
	int ctts_index;
	int ctts_sample;
	unsigned int sample_size;
	unsigned int sample_count;
	unsigned int sample_bytes;
	void *sample_sizes;
	unsigned int keyframe_count;
	int *keyframes;
	int time_scale;
	int64_t time_offset;      ///< time offset of the first edit list entry
	int current_sample;
	unsigned int bytes_per_frame;
	unsigned int samples_per_frame;
	int dv_audio_container;
	int pseudo_stream_id; ///< -1 means demux all ids
	int16_t audio_cid;    ///< stsd audio compression id
	unsigned drefs_count;
	MOVDref *drefs;
	int dref_id;
	int wrong_dts;        ///< dts are wrong due to huge ctts offset (iMovie files)
	int width;            ///< tkhd width
	int height;           ///< tkhd height
	int dts_shift;        ///< dts shift when ctts is negative
	int has_sidx;  // If there is an sidx entry for this stream.
	struct {
		struct AVAESCTR* aes_ctr;
		struct AVAES *aes_ctx;
		unsigned int frag_index_entry_base;
		unsigned int per_sample_iv_size;
		AVEncryptionInfo *default_encrypted_sample;
		MOVEncryptionIndex *encryption_index;
	} cenc;
} MOVStreamContext;

typedef struct MOVContext {
	AVFormatContext *fc;
	int time_scale;
	int64_t duration;     ///< duration of the longest track
	int found_moov;       ///< 'moov' atom has been found
	int found_mdat;       ///< 'mdat' atom has been found
	AVFormatContext *dv_fctx;
	int isom;             ///< 1 if file is ISO Media (mp4/3gp)
	MOVFragment fragment; ///< current fragment in moof atom
	MOVTrackExt *trex_data;
	unsigned trex_count;
	int itunes_metadata;  ///< metadata are itunes style
	int64_t next_root_atom;
	int decryption_key_size;
	int is_cenc_decryption;
	uint8_t *decryption_key;
	MOVFragmentIndex frag_index;
} MOVContext;

enum {
	CENC_DECRYPT    = 0,
	CBC1_DECRYPT    = 1,
	CENS_DECRYPT    = 2,
	CBCS_DECRYPT    = 3,
};

#endif /* AVFORMAT_ISOM_H */

