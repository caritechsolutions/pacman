
#ifndef _PARSE_ES_H_
#define _PARSE_ES_H_

#include "gx_demux.h"

#define MAX_SYNCLEN 				(10 * 1024 * 1024)

#define MAX_VIDEO_PACKET_SIZE 		(224*1024+4)
#define VIDEOBUFFER_SIZE 			0x100000

typedef struct parse_es {
	unsigned char *videobuffer;
	int videobuf_len;
	unsigned char *videobuf_code;
	int videobuf_code_len;
	int next_nal;
	GxDemuxStream *ds;
} GxParseES;

//extern GxParseES  es_parser;

int GxParseES_Reset(GxParseES*  es);
int GxParseES_Init(GxParseES * es, GxDemuxStream * ds);
int GxParseES_SyncVideoPacket(GxParseES * es);
int GxParseES_ReadVideoPacket(GxParseES * es);
int GxParseES_SkipVideoPacket(GxParseES * es);

#endif
