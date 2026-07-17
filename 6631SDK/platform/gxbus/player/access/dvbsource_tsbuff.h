
#ifndef __DVB_TSBUFF_H__
#define __DVB_TSBUFF_H__

#include "gx_common.h"
#include "gx_mediafilter.h"
#include "page_cache.h"
#include "file_cache.h"

#define TSBUFF_FIFO_R_SIZE (188*1024*10)
#define TSBUFF_FIFO_W_SIZE (188*1024)

#define TSBUFF_BLOCK_SIZE  (188*100*3)

#define TSBUFF_PTHREAD_PRIORITY (GXOS_DEFAULT_PRIORITY-1)

#define TSBUFF_SLOT_MAX 64

typedef struct {
	int running;
	int cachesize, cache2mem;
	char* cachedir;
	unsigned char* fcache_r, *fcache_w;

	int blksize;
	unsigned int delayms;
	struct cache_page *cur_page_r, *cur_page_w;

	handle_t sem;
	struct page_cache cache;
	struct file_cache fcache;
}TSBuffStreamPort;

typedef struct {
	int opencnt;
	unsigned int delayms;

	handle_t dev;
	handle_t dmx_r;
	handle_t dmx_w;

	int vpid;
	int apid;
	int pcrpid;
	int tsid;
	int dmxid;
	int delaydmx;

	int running;
	int pthread_r;
	int pthread_w;
	int kernel_mode;

	GxFifo* fifo_r;
	GxFifo* fifo_w;

	int ts_out_pin;
	int slot_nb;
	GxDemuxProperty_Slot slot[TSBUFF_SLOT_MAX];
	int slotusecnt[TSBUFF_SLOT_MAX];

	handle_t priv;
	PlayerStreamPortCallback stream;
}TSBuff;

#endif
