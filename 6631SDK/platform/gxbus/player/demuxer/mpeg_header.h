#ifndef _MPEG_HEADER_H_
#define _MPEG_HEADER_H_

#include <stdio.h>

#define getbits 	mp_getbits
#define getbits16 	mp_getbits16

typedef struct {
	// video info:
	int mpeg1;		// 0=mpeg2  1=mpeg1
	int display_picture_width;
	int display_picture_height;
	int aspect_ratio_information;
	int frame_rate_code;
	float fps;
	int bitrate;		// 0x3FFFF==VBR
	// timing:
	int picture_structure;
	int progressive_sequence;
	int repeat_first_field;
	int progressive_frame;
	int top_field_first;
	int display_time;	// secs*100
	//the following are for mpeg4
	unsigned int timeinc_resolution, timeinc_bits, timeinc_unit;
	int picture_type;
} MpegHeader;

int mp_header_process_sequence_header(MpegHeader * picture, unsigned char *buffer);
int mp_header_process_extension(MpegHeader * picture, unsigned char *buffer);
float mpeg12_aspect_info(MpegHeader * picture);
int mp4_header_process_vol(MpegHeader * picture, unsigned char *buffer);
void mp4_header_process_vop(MpegHeader * picture, unsigned char *buffer);
int h264_parse_sps(MpegHeader * picture, unsigned char *buf, int len);
int mp_vc1_decode_sequence_header(MpegHeader * picture, unsigned char *buf, int len);
unsigned char mp_getbits(unsigned char *buffer, unsigned int from, unsigned char len);

#endif
