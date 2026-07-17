/******************************************************************************

  Copyright (C), 2008-2018, Nationalchip Tech. Co., Ltd.

* *****************************************************************************
	File Name     : gx_es2frame.c
	Version       : Initial Draft
	Author        : Nationalchip multimedia software group
	Created       : 2009/01/14
	Last Modified :
	Description   : The decoder source files
	Function List :
	History       :
	1.Date        : 2009/01/14
	Author      : shencz
	Modification: Created file
******************************************************************************/
#include "vd_es2frame.h"
#include <stdarg.h>
#include "vd.h"

extern VdESBuffer vd_es_buffer;

unsigned int  dvideobuf_len = 0;
int dnext_nal = 0;
static int telecine = 0;
static float telecine_cnt = -2.5;
static ParseEsHeader dpicture;

uint8_t*  dvideobuffer = NULL;
///! legacy variable, 4 if stream is synced, 0 if not
int dvideobuf_code_len = 0;

static float frameratecode2framerate[16] = {
	0,
	// Official mpeg1/2 framerates: (1-8)
	24000.0 / 1001, 24, 25,
	30000.0 / 1001, 30, 50,
	60000.0 / 1001, 60,
	// Xing's 15fps: (9)
	15,
	// libmpeg3's "Unofficial economy rates": (10-13)
	5, 10, 12, 15,
	// some invalid ones: (14-15)
	0, 0
};

#define MAX_SYNCLEN 				(10*  1024 * 1024)

#define MAX_VIDEO_PACKET_SIZE 		(224*1024+4)
#define VIDEOBUFFER_SIZE 			0x100000
#define FF_INPUT_BUFFER_PADDING_SIZE 8

void* memalign(size_t align, size_t size);

int GxDecoderParse_ReadData(GxFifo*  ds, unsigned char *mem, int len)
{
	int x;
	int bytes = 0;
	while (len > 0) {
		x = GxFifo_GetLength(ds);
		if (x == 0) {
			gxlogf("@@@@@@@@@---No Data---@@@@@@@@@\n");	//if (!GxDemuxStream_FillBuffer(ds))
			return (bytes);
		} else {
			if (x > len)
				x = len;
			if (mem)
				//memcpy(mem + bytes, &ds->buffer[ds->out], x);
				GxFifo_Read(ds, mem + bytes, x);
			bytes += x;
			len -= x;
			//ds->out += x;
		}
	}
	return bytes;
}

int GxDecoderParse_Pattern3(unsigned char*  data, unsigned char *mem, int maxlen, int *read, uint32_t pattern)
{
	register uint32_t head = 0xffffff00;
	register uint32_t pat = pattern & 0xffffff00;
	int total_len = 0;
	do {
		register unsigned char* ds_buf = &vd_es_buffer.buffer[vd_es_buffer.size];
		int len = vd_es_buffer.size - vd_es_buffer.pos;
		register long pos = -len;
		if (pos >= 0) {
			return 0;
		}
	    do {
	      head |= ds_buf[pos];
	      head <<= 8;
	    } while (++pos && head != pat);
	    len += pos;
	    if (total_len + len > maxlen)
			len = maxlen - total_len;
		if(mem)
			memcpy(&mem[total_len],&vd_es_buffer.buffer[vd_es_buffer.pos],len);
		vd_es_buffer.pos+=len;
	    total_len += len;
	  } while ((head != pat || total_len < 3) && total_len < maxlen );
	if (read)
		*read = total_len;
	return total_len >= 3 && head == pat;
}

int decoder_es_sync_video_packet(unsigned char* data)
{
	if (!dvideobuf_code_len) {
		int skipped = 0;
		if (!GxDecoderParse_Pattern3(data, NULL, MAX_SYNCLEN, &skipped, 0x100)) {
			goto eof_out;
		}

		dnext_nal = vd_es_buffer.buffer[vd_es_buffer.pos++];
		if (dnext_nal < 0)
			goto eof_out;
		dvideobuf_code_len = 4;
	}
	return 0x100 | dnext_nal;

      eof_out:
	dnext_nal = -1;
	dvideobuf_code_len = 0;
	return 0;
}

// return: packet length
int decoder_es_read_video_packet(unsigned char*  data)
{
	int packet_start;
	int res, read;

	if (VIDEOBUFFER_SIZE - dvideobuf_len < 5)
		return 0;
	// COPY STARTCODE:
	packet_start = dvideobuf_len;
	dvideobuffer[dvideobuf_len + 0] = 0;
	dvideobuffer[dvideobuf_len + 1] = 0;
	dvideobuffer[dvideobuf_len + 2] = 1;
	dvideobuffer[dvideobuf_len + 3] = dnext_nal;
	dvideobuf_len += 4;

	// READ PACKET:
//	res = GxDecoderParse_Pattern3(data, &dvideobuffer[dvideobuf_len], VIDEOBUFFER_SIZE - dvideobuf_len, &read, 0x100);
	res = GxDecoderParse_Pattern3(data, NULL, VIDEOBUFFER_SIZE - dvideobuf_len, &read, 0x100);
	dvideobuf_len += read;
	if (!res)
		return (0);	//goto eof_out;

	dvideobuf_len -= 3;

	dnext_nal = vd_es_buffer.buffer[vd_es_buffer.pos++];
	if (dnext_nal < 0)
		goto eof_out;
	dvideobuf_code_len = 4;

	return dvideobuf_len - packet_start;

      eof_out:
	dnext_nal = -1;
	dvideobuf_code_len = 0;
	return dvideobuf_len - packet_start;
}

int dmp_header_process_sequence_header(ParseEsHeader*  picture, unsigned char *buffer)
{
	int width, height;

	if ((buffer[6] & 0x20) != 0x20) {
		gxlogf("missing marker bit!\n");
		return 1;	/* missing marker_bit*/
	}

	height = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];

	picture->display_picture_width = (height >> 12);
	picture->display_picture_height = (height & 0xfff);

	width = ((height >> 12) + 15) & ~15;
	height = ((height & 0xfff) + 15) & ~15;

	picture->aspect_ratio_information = buffer[3] >> 4;
	picture->frame_rate_code = buffer[3] & 15;
	picture->fps = frameratecode2framerate[picture->frame_rate_code];
	picture->bitrate = (buffer[4] << 10) | (buffer[5] << 2) | (buffer[6] >> 6);
	picture->mpeg1 = 1;
	picture->picture_structure = 3;	//FRAME_PICTURE;
	picture->display_time = 100;
	return 0;
}

static int header_process_sequence_extension(ParseEsHeader*  picture, unsigned char *buffer)
{
	/* check chroma format, size extensions, marker bit*/

	if (((buffer[1] & 0x06) == 0x00) ||
	    ((buffer[1] & 0x01) != 0x00) || (buffer[2] & 0xe0) || ((buffer[3] & 0x01) != 0x01))
		return 1;

	picture->progressive_sequence = (buffer[1] >> 3) & 1;
	picture->mpeg1 = 0;
	return 0;
}

static int header_process_picture_coding_extension(ParseEsHeader*  picture, unsigned char *buffer)
{
	picture->picture_structure = buffer[2] & 3;
	picture->top_field_first = buffer[3] >> 7;
	picture->repeat_first_field = (buffer[3] >> 1) & 1;
	picture->progressive_frame = buffer[4] >> 7;

	// repeat_first implementation by A'rpi/ESP-team, based on libmpeg3:
	picture->display_time = 100;
	if (picture->repeat_first_field) {
		if (picture->progressive_sequence) {
			if (picture->top_field_first)
				picture->display_time += 200;
			else
				picture->display_time += 100;
		} else if (picture->progressive_frame) {
			picture->display_time += 50;
		}
	}
	//temopral hack. We calc time on every field, so if we have 2 fields
	// interlaced we'll end with double time for 1 frame
	if (picture->picture_structure != 3)
		picture->display_time /= 2;
	return 0;
}

int dmp_header_process_extension(ParseEsHeader*  picture, unsigned char *buffer)
{
	switch (buffer[0] & 0xf0) {
	case 0x10:		/* sequence extension*/
		return header_process_sequence_extension(picture, buffer);
	case 0x80:		/* picture coding extension*/
		return header_process_picture_coding_extension(picture, buffer);
	}
	return 0;
}

int decoder_read_init(void)
{
	dvideobuf_len = 0;
	dvideobuf_code_len = 0;
	telecine = 0;
	telecine_cnt = -2.5;

	vd_es_buffer.buffer = av_malloc(MAX_PACK_BYTES);
	vd_es_buffer.pos    = 0;
	vd_es_buffer.size   = 0;

	if (!dvideobuffer) {
		dvideobuffer = (uint8_t* ) memalign(8, VIDEOBUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
		if (dvideobuffer)
			memset(dvideobuffer + VIDEOBUFFER_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);
		else
			return 0;
	}

	return (1);
}

static void process_userdata(unsigned char* buf, int len)
{
	/* if the user data starts with "CC", assume it is a CC info packet*/
	if (len > 2 && buf[0] == 'C' && buf[1] == 'C') {
		// TODO:subcc_process_data(buf+2,len-2);
	}
	if (len > 2 && buf[0] == 'T' && buf[1] == 'Y') {
		// TODO:ty_processuserdata( buf + 2, len - 2 );
		return;
	}
}

int vd_header_process_sequence_header(ParseEsHeader*  picture, unsigned char *buffer)
{
	int width, height;

	if ((buffer[6] & 0x20) != 0x20) {
		gxlogf("missing marker bit!\n");
		return 1;	/* missing marker_bit*/
	}

	height = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];

	picture->display_picture_width = (height >> 12);
	picture->display_picture_height = (height & 0xfff);

	width = ((height >> 12) + 15) & ~15;
	height = ((height & 0xfff) + 15) & ~15;

	picture->aspect_ratio_information = buffer[3] >> 4;
	picture->frame_rate_code = buffer[3] & 15;
	picture->fps = frameratecode2framerate[picture->frame_rate_code];
	picture->bitrate = (buffer[4] << 10) | (buffer[5] << 2) | (buffer[6] >> 6);
	picture->mpeg1 = 1;
	picture->picture_structure = 3;	//FRAME_PICTURE;
	picture->display_time = 100;
	return 0;
}

int vd_header_process_extension(ParseEsHeader*  picture, unsigned char *buffer)
{
	switch (buffer[0] & 0xf0) {
	case 0x10:		/* sequence extension*/
		return header_process_sequence_extension(picture, buffer);
	case 0x80:		/* picture coding extension*/
		return header_process_picture_coding_extension(picture, buffer);
	}
	return 0;
}

int decoder_read_frame(unsigned char*  data, float *frame_time_ptr, unsigned char **start, int force_fps)
{
	float frame_time = 1;
	int picture_coding_type = 0;
	int in_size = 0;
	//*start = NULL;
	int in_frame = 0;

	//float newfps;
	//videobuf_len=0;
	while (dvideobuf_len < VIDEOBUFFER_SIZE - MAX_VIDEO_PACKET_SIZE) {
		int i = decoder_es_sync_video_packet(data);
		//void* buffer=&videobuffer[videobuf_len+4];
		int pos = dvideobuf_len + 4;
		if (in_frame) {
			if (i < 0x101 || i >= 0x1B0) {	// not slice code -> end of frame
				if (!i) {
					return -1;	// EOF
				}
				break;
			}
		} else {
			if (i == 0x100) {
			}
			if (i >= 0x101 && i < 0x1B0)
				in_frame = 1;	// picture startcode
			else if (!i) {
				return -1;	// EOF
			}
		}

		if (!decoder_es_read_video_packet(data)) {
			return -1;	// EOF
		}

		// process headers:
		switch (i) {
		case 0x1B3:
			vd_header_process_sequence_header(&dpicture, &dvideobuffer[pos]);
			break;
		case 0x1B5:
			vd_header_process_extension(&dpicture, &dvideobuffer[pos]);
			break;
		case 0x1B2:
			process_userdata(&dvideobuffer[pos], dvideobuf_len - pos);
			break;
		case 0x100:
			picture_coding_type = (dvideobuffer[pos + 1] >> 3) & 7;
			break;
		}
	}

	*start = dvideobuffer;
	in_size = dvideobuf_len;

	// get mpeg fps:
	// fix mpeg2 frametime:
	frame_time = (dpicture.display_time)*  0.01f;
	dpicture.display_time = 100;
	dvideobuf_len = 0;

	telecine_cnt*= 0.9;	// drift out error
	telecine_cnt += frame_time - 5.0 / 4.0;

	if (telecine) {
		frame_time = 1;
		if (telecine_cnt < -1.5 || telecine_cnt > 1.5)
			telecine = 0;
	} else {
		if (telecine_cnt > -0.5 && telecine_cnt < 0.5 && !force_fps) {
			telecine = 1;
		}
	}

	//------------------------ frame decoded. --------------------

	// Increase video timers:

	// override frame_time for variable/unknown FPS formats:

	if (frame_time_ptr)
		*frame_time_ptr = frame_time;
	return in_size;
}

