#ifndef _GX_DUMP_FILTER_FM_H_
#define _GX_DUMP_FILTER_FM_H_

#include "page_cache.h"
#include "gx_dumpfilter.h"


typedef struct {
	GxStream         *source;
	GxStreamDataInfo  data_info;
	struct cache_page page;
	unsigned int      samples_size;
	unsigned char     wav_header[44];
	int               wav_header_fill;
}GxDumpFilterFMPriv;


#endif
