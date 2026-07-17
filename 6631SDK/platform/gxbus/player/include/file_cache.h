#ifndef __FILE_CACHE_H__
#define __FILE_CACHE_H__

#include "gx_common.h"

struct fcblk {
	size_t size;
	uint32_t pts;
	int rollback;
};

struct file_cache {
	char* dir;

	size_t head, tail;
	size_t ridx, widx;
	size_t rpos, wpos;
	size_t nowsize, maxsize;
	struct record_file *rf_r, *rf_w;

	struct fcblk* fblk;
	handle_t mutex;
	unsigned char *wbuf;
	int            wptr;
};


extern int file_cache_init(struct file_cache* cache, const char* dir, int cachesize);

extern size_t file_cache_read(struct file_cache* cache, unsigned char* buffer, size_t size, uint32_t* pts);

extern size_t file_cache_write(struct file_cache* cache, unsigned char* buffer, size_t size, uint32_t pts);

extern int file_cache_uninit(struct file_cache* cache);

#endif
