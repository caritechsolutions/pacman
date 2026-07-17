
#include "pack_cache.h"

#if 1
#define PACK_CACHE_LOCK(c) GxCore_MutexLock(c->mutex)
#define PACK_CACHE_UNLOCK(c)GxCore_MutexUnlock(c->mutex)
#define PACK_CACHE_LOCK_INIT(c) GxCore_MutexCreate(&c->mutex)
#define PACK_CACHE_LOCK_UNINIT(c) GxCore_MutexDelete(c->mutex)
#else
#define PACK_CACHE_LOCK(c)         (void)0
#define PACK_CACHE_UNLOCK(c)       (void)0
#define PACK_CACHE_LOCK_INIT(c)    (void)0
#define PACK_CACHE_LOCK_UNINIT(c)  (void)0
#endif

int pack_cache_init(struct pack_cache *cache, size_t pack_size, int max_packs, int max_tick)
{
	int i;
	memset(cache, 0, sizeof(struct pack_cache));

	if (max_packs > MAX_PACKS) {
		gxloge("max_packs error!");
		goto errout;
	}

	for (i=0; i<max_packs; i++) {
		cache->packs[i].buffer = av_malloc(pack_size);
		if (cache->packs[i].buffer == NULL) {
			gxloge("oom error!");
			goto errout;
		}
		cache->packs[i].size = pack_size;
	}

	cache->pack_size = pack_size;
	cache->max_packs = max_packs;
	cache->max_tick  = max_tick;

	PACK_CACHE_LOCK_INIT(cache);
	return 0;

errout:
	pack_cache_uninit(cache);
	return -1;
}

void pack_cache_uninit(struct pack_cache *cache)
{
	int i;

	PACK_CACHE_LOCK(cache);
	for (i=0; i<cache->max_packs; i++) {
		if (cache->packs[i].buffer != NULL) {
			av_free(cache->packs[i].buffer);
			cache->packs[i].buffer = NULL;
		}
	}
	PACK_CACHE_UNLOCK(cache);

	PACK_CACHE_LOCK_UNINIT(cache);
}

static void pack_cache_update(struct pack_cache *cache)
{
	if (cache->pos_v != cache->pos_r) {
		int curw = cache->pos_w == 0 ? cache->max_packs - 1 :  cache->pos_w - 1;

		if (cache->packs[cache->pos_v].state == 2 && cache->packs[curw].pts - cache->packs[cache->pos_v].pts > cache->max_tick) {
			cache->packs[cache->pos_v].state = 3;
			cache->pos_v = (cache->pos_v + 1) % cache->max_packs;
		}
	}
}

struct cache_pack *pack_cache_alloc_pack(struct pack_cache *cache)
{
	struct cache_pack* pack = NULL;

	PACK_CACHE_LOCK(cache);
	pack_cache_update(cache);
	if (cache->packs[cache->pos_w].state == 0 || cache->packs[cache->pos_w].state == 3){
		pack = &cache->packs[cache->pos_w];
		cache->pos_w = (cache->pos_w + 1) % cache->max_packs;
		pack->state = 1;
		pack->size = cache->pack_size;
		gxlogd("%s, pos_r w v = %d %d %d\n", __func__, cache->pos_r, cache->pos_w, cache->pos_v);
	}
	PACK_CACHE_UNLOCK(cache);

	return pack;
}

void pack_cache_free_pack(struct pack_cache *cache, struct cache_pack *pack)
{
	PACK_CACHE_LOCK(cache);
	pack_cache_update(cache);
	PACK_CACHE_UNLOCK(cache);
}

static int pack_cache_pack_ready(struct pack_cache *cache)
{
	if (cache->rst == 0) {
		int curw = cache->pos_w == 0 ? cache->max_packs - 1 :  cache->pos_w - 1;

		if (cache->packs[cache->pos_r].state != 2)
			return 0;
		if (cache->packs[curw].pts - cache->packs[cache->pos_r].pts < cache->max_tick)
			return 0;
	}

	return 1;
}

struct cache_pack* pack_cache_get_pack_cached(struct pack_cache *cache)
{
	if (pack_cache_pack_ready(cache))
		return pack_cache_get_pack(cache);
	return NULL;
}

struct cache_pack* pack_cache_get_pack(struct pack_cache *cache)
{
	struct cache_pack* pack = NULL;

	if (cache->packs[cache->pos_r].state == 2) {
		pack = &cache->packs[cache->pos_r];
		cache->pos_r = (cache->pos_r + 1) % cache->max_packs;
		cache->now_packs --;
		gxlogd("%s, pos_r w v = %d %d %d\n", __func__, cache->pos_r, cache->pos_w, cache->pos_v);
	}

	return pack;
}

int pack_cache_put_pack(struct pack_cache *cache, struct cache_pack* pack)
{
	PACK_CACHE_LOCK(cache);
	pack->state = 2;
	cache->now_packs ++;
	pack_cache_update(cache);
	PACK_CACHE_UNLOCK(cache);

	return 0;
}

int pack_cache_reset(struct pack_cache *cache)
{
	int i;

	PACK_CACHE_LOCK(cache);
	gxlogi("##########################################################\n");
	gxlogi("%s, w:%d, r:%d, n:%d, v=%d", __func__, cache->pos_w, cache->pos_r, cache->now_packs, cache->pos_v);
	cache->pos_r = cache->pos_v;
	cache->rst_size = 0;
	if (cache->pos_w >= cache->pos_r) {
		for (i=cache->pos_r; i<cache->pos_w; i++) {
			gxlogi("state[%d]=%d, s=%d", i, cache->packs[i].state, cache->packs[i].size);
			if (cache->packs[i].state == 3)
				cache->packs[i].state = 2;
			if (cache->packs[i].state == 2)
				cache->rst_size += cache->packs[i].size;
		}
	}
	else {
		for (i=0; i<cache->pos_w; i++) {
			gxlogi("state[%d]=%d, s=%d", i, cache->packs[i].state, cache->packs[i].size);
			if (cache->packs[i].state == 3)
				cache->packs[i].state = 2;
			if (cache->packs[i].state == 2)
				cache->rst_size += cache->packs[i].size;
		}
		for (i=cache->pos_r; i<cache->max_packs; i++) {
			gxlogi("state[%d]=%d, s=%d", i, cache->packs[i].state, cache->packs[i].size);
			if (cache->packs[i].state == 3)
				cache->packs[i].state = 2;
			if (cache->packs[i].state == 2)
				cache->rst_size += cache->packs[i].size;
		}
	}

	cache->now_packs = cache->pos_w > cache->pos_r ? cache->pos_w - cache->pos_r :
		cache->pos_w - cache->pos_r + cache->max_packs;

	gxlogi("##########################################################\n");
	gxlogi("%s, pksize:%d, pknb:%d, tick:%d0\n", __func__, cache->pack_size, cache->max_packs, cache->max_tick);
	gxlogi("%s, w:%d, r:%d, n:%d, s=%d\n", __func__, cache->pos_w, cache->pos_r, cache->now_packs, cache->rst_size);
	gxlogi("##########################################################\n");
	PACK_CACHE_UNLOCK(cache);

	return 0;
}

void pack_cache_exit(struct pack_cache *cache)
{

}

int pack_cache_reset_set(struct pack_cache *cache)
{
	cache->rst = 1;
	return 0;
}

int pack_cache_reset_clr(struct pack_cache *cache)
{
	cache->rst = 0;
	return 0;
}

