#ifndef __GX_ES2FRAME_H__
#define __GX_ES2FRAME_H__

#include "gx_mediafilter.h"

typedef struct {
	unsigned char          *buffer;
	int                    pos;
	int                    size;
}VdESBuffer;


typedef struct {
	// video info:
	int mpeg1;	// 0=mpeg2  1=mpeg1
	int display_picture_width;
	int display_picture_height;
	int aspect_ratio_information;
	int frame_rate_code;
	float fps;
	int bitrate;	// 0x3FFFF==VBR
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
} ParseEsHeader;

int decoder_read_init(void);

int decoder_read_frame(unsigned char*, float *frame_time_ptr, unsigned char **start, int force_fps);


#endif
