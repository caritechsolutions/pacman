#ifndef _GX_DUMP_FILTER_H_
#define _GX_DUMP_FILTER_H_

#include "gx_mediafilter.h"
#include "gx_stream.h"
#include "gx_config.h"
#include "gx_demux.h"

#undef DEBUG_DUMPFILTER_WRITE_SPEED

typedef enum {
	GX_DUMPER_SET_EXTRA,
	GX_DUMPER_GET_DURATION ,
	GX_DUMPER_GET_VOLINFO ,
	GX_DUMPER_GET_FILESIZE ,
	GX_DUMPER_SET_PVR_CONTROL,
	GX_DUMPER_GET_PVR_CONTROL,
	GX_DUMPER_SET_SOURCE,
}GxDumpFilterCtrl;

typedef struct {
	GxDemuxerRecordHeaderInfo header_info;
	GxDemuxerRecordUserInfo   user_info;
	GxDemuxerRecordCtrlInfo   ctrl_info;
}GxDumpFilterExtra;

typedef enum {
	DUMPFILTER_DVB,
	DUMPFILTER_RAW,
	DUMPFILTER_FM,
}GxDumpFilterType;

typedef struct {
	GxMediaFilter filter;

	GxStream*     s;
	GxStream*     r;

	int64_t       cap;
	int           volume_size;
	int           volume_max;

	char*         url;
	int           eof;

	GxTime        start_ticktime;
	GxTime        pause_ticktime;
	unsigned int  pause_timems;
	unsigned int  empty_timems;
	unsigned int  timems[2];

	int32_t       thread;
	int32_t       write_full;
	GxPin*        inpin;

	void*         priv;
#ifdef DEBUG_DUMPFILTER_WRITE_SPEED
	unsigned int  idx;
	unsigned int  cnt;
	unsigned int  write_bksize[10];
	unsigned int  write_costms[10];
#endif
}GxDumpFilter;

typedef struct {
	int security_flag;
	int block_size;
}GxDumpFilterParam;

typedef struct {
	GxMediaFilterClass  _inherit;
	int type;
	int (*open)  (GxDumpFilter* df);
	int (*close) (GxDumpFilter* df);
	int (*read)  (GxDumpFilter* df);
	int (*write) (GxDumpFilter* df);
	int (*control)(GxDumpFilter* df, int cmd, void* args);

}GxDumpFilterClass;

#define GX_DUMPFILTER_PIN_NAME_IN       "dumpfilter_in"

#define GetDumpFilterClassFromObject(s) ((GxDumpFilterClass*)(GetGxClassFromObject(s)))
#define GXDUMPFILTER(s)                 ((GxDumpFilter*)(s))
#define GXDUMPFILTERCLASS(s)            ((GxDumpFilterClass *)(s))

extern GxDumpFilterClass gx_dumpfilter_base;
extern GxDumpFilter *GxDumpFilter_Open(const char* dsturl, GxDumpFilterType type);
extern void GxDumpFilter_Close(GxDumpFilter* df);
extern int GxDumpFilter_Read(GxDumpFilter *df, uint8_t *buffer, int len);
extern int GxDumpFilter_Control(GxDumpFilter* df, int cmd, void* args);
extern int GxDumpfilter_Heartbeats(GxDumpFilter* df);

#endif
