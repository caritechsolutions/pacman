#ifndef _DEMUX_AUDIO_H_
#define _DEMUX_AUDIO_H_

#include "gx_demux.h"
#include "stheader.h"

#include "../avutil/intreadwrite.h"

#define MP3                     1
#define WAV                     2
#define fLaC                    3

#define HDR_SIZE                4

//! rather arbitrary value for maximum length of wav-format headers
#define MAX_WAVHDR_LEN          (1 * 1024 * 1024)
//! how many valid frames in a row we need before accepting as valid MP3
#define MIN_MP3_HDRS            6

#define FLAC_SIGNATURE_SIZE     4
#define FLAC_STREAMINFO_SIZE    34
#define FLAC_SEEKPOINT_SIZE     18

#define DTS_PROBE_SIZE          16384

#define audio_decode_mp3_header(hbuf) \
	audio_get_mp3_header(hbuf,NULL,NULL,NULL,NULL,NULL)

typedef enum {
	FLAC_STREAMINFO = 0,
	FLAC_PADDING,
	FLAC_APPLICATION,
	FLAC_SEEKTABLE,
	FLAC_VORBIS_COMMENT,
	FLAC_CUESHEET
} flac_preamble_t;

typedef struct demux_audio_priv {
	int frmt;
	double next_pts;
	int hr_mp3_seek;
	int vbr;
	int frame_num;
	int mpa_spf;
	int esa_size;
} DemuxAudioPriv;

typedef struct audio_mp3_header {
	off_t frame_pos;	// start of first frame in this "chain" of headers
	off_t next_frame_pos;	// here we expect the next header with same parameters
	int mp3_chans;
	int mp3_freq;
	int mpa_spf;
	int mpa_layer;
	int mpa_br;
	int cons_headers;	// if this reaches MIN_MP3_HDRS we accept as MP3 file
	struct audio_mp3_header *next;
} AudioMp3Header;

int audio_check_mp3_header(unsigned int head);

#endif
