#ifndef __STREAM_M3U8__
#define __STREAM_M3U8__

#include "gx_stream.h"

typedef struct{
	char* buffer;
	int size;
}GxStream_RollBack;

int GxStream_UpdateM3U8(GxStream* stream);

int GxStream_StreamList_Get_Count(GxStreamList* s);
int GxStream_StreamList_Add(GxStreamList* s, char* url);
int GxStream_StreamList_Read(GxStreamList* s, uint8_t* buffer, int len);
GxStreamList* GxStream_StreamList_Create(GxStream* s, char* hls_url);
void GxStream_StreamList_Destory(GxStreamList* s);

#endif
