#ifndef MPLAYER_MTC_STREAMING_H
#define MPLAYER_MTC_STREAMING_H

#include "gx_stream.h"

#define MTC_PROTOCOL_LEN (32)
#define MTC_KEY_LEN      (32)
#define MTC_ADDR_ALIGN   (64)
#define MTC_SIZE_ALIGN   (8)

typedef struct mtc {
	int  algo;
	int  sub_mode;
	char protocol[MTC_PROTOCOL_LEN];
	unsigned char key_value[MTC_KEY_LEN];
	int  key_len;
	unsigned char iv_value[MTC_KEY_LEN];
	int  iv_len;
	int  pad_len;
	uint8_t* pad;
	uint8_t* w_inbuf;
	uint8_t* w_outbuf;
	off_t    w_fpos;
	int r_indata;
	int r_outdata;
	uint8_t* r_outptr;
	uint8_t* r_inbuf;
	uint8_t* r_outbuf;
	off_t    r_fpos;
} GxMtc;

typedef struct stream_mtc {
	GxStream  parent;
	GxMtc     mtc;
	GxStream* children;
} GxStreamMtc;

#endif
