#ifndef GX_SUBREADER_H
#define GX_SUBREADER_H

#include "gx_stream.h"
#include "../subtitle/subformat/vobsub.h"
#include "../subtitle/subformat/spudec.h"

#define SUB_INVALID       -1

#define SUB_MAX_STREAM    8
#define SUB_MAX_TEXT      12

#define GX_SUBTITLE_POSTFIX_MAX  (16)

typedef struct subreader_class GxSubReaderClass;

enum {
	SUB_MICRODVD    ,
	SUB_SUBRIP      ,
	SUB_SUBVIEWER   ,
	SUB_VPLAYER     ,
	SUB_RT          ,
	SUB_SSA         ,
	SUB_PJS         ,
	SUB_SUBVIEWER2  ,
	SUB_MPL2        ,
	SUB_LCR         ,
	SUB_SAMI        ,
	SUB_TXT         ,
	SUB_VTT         ,
};

enum {
	SUB_ENC_UTF8,
	SUB_ENC_ANSI,
	SUB_ENC_UCS,
	SUB_ENC_SPU,
	SUB_ENC_ERR,
};

typedef struct subpage{
	int32_t lines;
	int32_t encode ;
	uint32_t start;
	uint32_t end;

	char*   text[SUB_MAX_TEXT];
	struct  subpage* next;
}GxSubPage;

typedef struct {
	GxSubReaderClass*  cls;
	GxSubPage*  sub_hdr;
	char*       filename;
	int         sub_uses_time;
	int         sub_track;
	char*       sub_lang[SUB_MAX_STREAM];
	int         sub_errs;
	int32_t     encode ;
	vobsub_t*   vob ;
	GxSpuDec*   spu ;
	int32_t     sub_num;
	int32_t     fd;
	int32_t     sub_format;
	uint32_t    sub_duration;
} GxSubData;

typedef struct {
	int   sub_id;
	uint32_t   startpts;
	uint32_t   endpts;
}GxSubQueue;

typedef struct{
	int  cur_id;
	int  encode;
	int  num;
	int  id[MAX_LANG_NUM];
	char lang[MAX_LANG_NUM][16];
	char track_name[MAX_LANG_NUM][16];
	char codec_id[MAX_LANG_NUM][16];
}GxSubInfo;

struct subreader_class{
	char*      name;
	char*      extensions[32];

	GxSubData* (*open)(const char* lang,const char* filename);
	void       (*close)(GxSubData* subd );
	int        (*get_packet)(GxSubData* sub,void** pkt,int* len,int64_t* startpts ,int64_t* endpts);
	void       (*switch_stream)(GxSubData* sub,int id);
	int        (*alloc_chr)(GxSubData* sub, uint32_t sublines);
	void       (*free_chr)(GxSubData* subd);
	int        (*get_info)(GxSubData* subd, GxSubInfo* subinfo);
	int        (*reset_chr)(GxSubData* sub);
};

GxSubData* GxSubReader_Open(const char* lang,const char* filename);
void GxSubReader_Close( GxSubData * subd );
int  GxSubReader_GetPacket(GxSubData *sub,void** pkt,int* len,int64_t* startpts ,int64_t* endpts);
void GxSubReader_SwitchStream(GxSubData* sub,int id);
int  GxSubReader_Alloc_Chr(GxSubData* sub, int sublines);
void GxSubReader_Free_Chr(GxSubData* sub);
int  GxSubReader_Reset_Chr(GxSubData* sub);
void GxSubReader_GetInfo(GxSubData* subd, void* sub_info);
int  GxSubReader_Get_ByTime(GxSubData* sub, uint32_t cur, int delay, uint32_t* start, uint32_t* end);

#endif 


