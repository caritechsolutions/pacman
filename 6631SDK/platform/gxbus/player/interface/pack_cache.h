#ifndef __PACK_CACHE_H__
#define __PACK_CACHE_H__

#include "gx_common.h"

#define MAX_PACKS (4096)

struct cache_pack {
	unsigned char* buffer;
	size_t         size;
	long long      pts;
	unsigned int   state;
};

struct pack_cache {
	handle_t          mutex;

	int               pos_v;
	int               pos_r;
	int               pos_w;

	int               max_tick;
	int               max_packs;

	size_t            pack_size;
	size_t            now_packs;

	unsigned int rst_time;
	int rst_size;
	int rst;

	struct cache_pack packs[MAX_PACKS];
};

int pack_cache_init(struct pack_cache *cache, size_t pack_size, int max_packs, int max_tick);

void pack_cache_uninit(struct pack_cache *cache);

struct cache_pack *pack_cache_alloc_pack(struct pack_cache *cache);

void pack_cache_free_pack(struct pack_cache *cache, struct cache_pack *pack);

struct cache_pack* pack_cache_get_pack(struct pack_cache *cache);

struct cache_pack* pack_cache_get_pack_cached(struct pack_cache *cache);

int pack_cache_put_pack(struct pack_cache *cache, struct cache_pack* pack);

void pack_cache_exit(struct pack_cache *cache);

int pack_cache_reset(struct pack_cache *cache);

int pack_cache_reset_set(struct pack_cache *cache);

int pack_cache_reset_clr(struct pack_cache *cache);

#endif
