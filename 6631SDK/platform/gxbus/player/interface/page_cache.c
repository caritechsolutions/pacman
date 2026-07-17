
#include "page_cache.h"

#define FREE_LIST_LOCK(c) GxCore_MutexLock(c->free_list_mutex)
#define FREE_LIST_UNLOCK(c)GxCore_MutexUnlock(c->free_list_mutex)
#define FREE_LIST_LOCK_INIT(c) GxCore_MutexCreate(&c->free_list_mutex)
#define FREE_LIST_LOCK_UNINIT(c) GxCore_MutexDelete(c->free_list_mutex)

#define USED_LIST_LOCK(c) GxCore_MutexLock(c->used_list_mutex)
#define USED_LIST_UNLOCK(c)GxCore_MutexUnlock(c->used_list_mutex)
#define USED_LIST_LOCK_INIT(c) GxCore_MutexCreate(&c->used_list_mutex)
#define USED_LIST_LOCK_UNINIT(c) GxCore_MutexDelete(c->used_list_mutex)

struct cache_page *page_cache_alloc_page(struct page_cache *cache)
{
	struct cache_page* page = NULL;

	GxCore_SemWait(cache->free_sem);

	FREE_LIST_LOCK(cache);
	page = (struct cache_page*)gxlist_get(&cache->free_list);
	FREE_LIST_UNLOCK(cache);

	if(page){
		cache->free_page_num--;
		page->size = cache->page_size;
	}
	else if(cache->alloc_page_num < cache->max_used_num){
		page = (struct cache_page*)av_mallocz(sizeof(struct cache_page));
		if(page){
			page->buffer = av_malloc(cache->page_size);
			if(page->buffer == NULL){
				av_free(page);
				return NULL;
			}
			page->size = cache->page_size;
			cache->alloc_page_num++;
			gxlogi("%s.. max_page = %d, used_page = %d, memsize = %d\n", __func__, cache->max_used_num, cache->alloc_page_num, cache->alloc_page_num * page->size);
			gxlogd("%s.. max_free = %d, now_free  = %d\n", __func__, cache->max_cache_num, cache->free_page_num);
		}
	}
	else if (cache->memhole.buffer != NULL) {
		gxlogi("%s.. cache full ! will use memhole! cache->memhole_size = %d, used_page = %d\n", __func__, cache->memhole.size, cache->alloc_page_num);
		if (cache->memhole.pages[cache->memhole.nowcnt].used == 0) {
			page = (struct cache_page*)av_mallocz(sizeof(struct cache_page));
			if(page){
				page->buffer = cache->memhole.pages[cache->memhole.nowcnt].buffer;
				page->size = cache->page_size;
				cache->alloc_page_num++;
				cache->memhole.nowcnt++;
				cache->memhole.nowcnt %= cache->memhole.maxcnt;
			}
		}
		else {
			gxlogi("%s.. hw cache full! cache->memhole_size = %d\n", __func__, cache->memhole.size);
		}
	}

	return page;
}

static void cache_free_page(struct page_cache *cache, struct cache_page *page)
{
	if (cache->memhole.buffer) {
		unsigned int start = (unsigned int)cache->memhole.buffer;
		if ((unsigned int)page->buffer >= start && (unsigned int)page->buffer < start + cache->memhole.size) {
			unsigned int pos = ((unsigned int)page->buffer - start )/cache->page_size;
			cache->memhole.pages[pos].used = 0;
		}
		else
			av_free(page->buffer);
	}
	else
		av_free(page->buffer);
	av_free(page);
}

void page_cache_free_page(struct page_cache *cache, struct cache_page *page)
{
	if (cache->free_page_num > cache->max_cache_num){
		if (page){
			cache_free_page(cache, page);
			cache->alloc_page_num--;
			gxlogd("%s.. max_page = %d, used_page = %d\n", __func__, cache->max_used_num, cache->alloc_page_num);
			gxlogd("%s.. max_free = %d, now_page  = %d\n", __func__, cache->max_cache_num, cache->free_page_num);
		}
	}
	else {
		FREE_LIST_LOCK(cache);
		page->size = 0;
		gxlist_add(&page->head, &cache->free_list);
		cache->free_page_num++;
		FREE_LIST_UNLOCK(cache);
	}

	GxCore_SemPost(cache->free_sem);
}

struct cache_page* page_cache_get_page(struct page_cache *cache)
{
	struct cache_page* page;

	int ret = GxCore_SemTryWait(cache->used_sem);
	if (ret != 0) {
		return NULL;
	}

	USED_LIST_LOCK(cache);
	page = (struct cache_page*)gxlist_get(&cache->used_list);
	cache->totle_size -= page->size;
	USED_LIST_UNLOCK(cache);

	return page;
}

int page_cache_put_page(struct page_cache *cache, struct cache_page* page)
{
	USED_LIST_LOCK(cache);
	gxlist_add_tail((struct gxlist_head *)page, &cache->used_list);
	cache->totle_size += page->size;
	USED_LIST_UNLOCK(cache);

	GxCore_SemPost(cache->used_sem);

	return 0;
}

int page_cache_is_full(struct page_cache *cache)
{
	int ret;
	FREE_LIST_LOCK(cache);
	ret = gxlist_empty(&cache->free_list) && (cache->alloc_page_num >= cache->max_used_num + cache->memhole.maxcnt);
	FREE_LIST_UNLOCK(cache);

	return ret;
}

int page_cache_init(struct page_cache *cache, size_t page_size, int max_use_page, int max_cache_page, size_t hole_size)
{
	GX_INIT_LIST_HEAD(&cache->used_list);
	GX_INIT_LIST_HEAD(&cache->free_list);

	USED_LIST_LOCK_INIT(cache);
	FREE_LIST_LOCK_INIT(cache);

	if (GxCore_SemCreate(&(cache->free_sem), max_use_page) != GXCORE_SUCCESS)
		goto errout;
	if (GxCore_SemCreate(&(cache->used_sem), 0) != GXCORE_SUCCESS)
		goto errout;

	cache->free_page_num = 0;
	cache->alloc_page_num = 0;

	cache->page_size = page_size;
	cache->max_used_num = max_use_page;
	cache->max_cache_num = max_cache_page;

	if (hole_size) {
		cache->memhole.buffer = av_memhole_malloc_user(hole_size);
		if (cache->memhole.buffer != NULL) {
			int i;
			memset(cache->memhole.pages, 0, sizeof(cache->memhole.pages));
			cache->memhole.size = hole_size;
			cache->memhole.nowcnt = 0;
			cache->memhole.maxcnt = GXMIN(MAX_HOLEPAGES, cache->memhole.size/cache->page_size);
			for (i=0; i<cache->memhole.maxcnt; i++) {
				cache->memhole.pages[i].buffer = cache->memhole.buffer + i * cache->page_size;
				GxCore_SemPost(cache->free_sem);
			}
		}
	}

	return 0;

errout:
	page_cache_uninit(cache);
	return -1;
}

void page_cache_uninit(struct page_cache *cache)
{
	struct cache_page *page = NULL, *n;

	gxlist_for_each_entry_safe(page, n, &cache->used_list, head){
		cache_free_page(cache, page);
	}

	gxlist_for_each_entry_safe(page, n, &cache->free_list, head){
		cache_free_page(cache, page);
	}

	USED_LIST_LOCK_UNINIT(cache);
	FREE_LIST_LOCK_UNINIT(cache);

	GxCore_SemDelete(cache->free_sem);
	GxCore_SemDelete(cache->used_sem);

	if (cache->memhole.buffer) {
		av_memhole_free_user(cache->memhole.buffer);
		memset(&cache->memhole, 0, sizeof(cache->memhole));
	}
}

void page_cache_exit(struct page_cache *cache)
{
	GxCore_SemPost(cache->used_sem);
	GxCore_SemPost(cache->free_sem);
}

