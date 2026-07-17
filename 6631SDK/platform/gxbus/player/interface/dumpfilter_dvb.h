#ifndef _GX_DUMP_FILTER_DVB_H_
#define _GX_DUMP_FILTER_DVB_H_

#include "page_cache.h"
#include "gx_dumpfilter.h"
#include "gx_demux.h"

#define DUMP_DVB_TIMEOUTUS   (20*1000*1000)

typedef struct {
	int user_pid[RECODER_USER_MAX_COUNT];
	int user_cc[RECODER_USER_MAX_COUNT];
	GxDemuxerRecordHeaderInfo header_info;
	GxDemuxerRecordUserInfo   user_info;
	GxDemuxerRecordCtrlInfo   ctrl_info;

	struct cache_page page;
	int    tsw_ptr_en;
	int    block_size;
	int    buffer_size;
	int    buffer_rptr;
	int    timeout_us;
	int    is_config;
	int    ts_encrypt;
	struct vol_hdr ts_hdr;
	unsigned int fill_hdr_timems;
}GxDumpFilterDVBPriv;


#endif
