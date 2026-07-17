#ifndef _GX_DUMP_FILTER_RAW_H_
#define _GX_DUMP_FILTER_RAW_H_

#include "page_cache.h"
#include "gx_dumpfilter.h"
#include "gxplayer_demuxer.h"

#define DUMP_RAW_PAGE_SIZE   (512 * 188)
#define DUMP_RAW_BLOCK_SIZE  (DUMP_RAW_PAGE_SIZE - 188)

#define DUMP_RAW_TIMEOUTUS   (20000)

typedef struct {
	struct cache_page page;
}GxDumpFilterRAWPriv;


#endif
