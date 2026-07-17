
#include "module/app_des_cache.h"

#define FREE_LIST_LOCK(c) GxCore_MutexLock(c->free_list_mutex)
#define FREE_LIST_UNLOCK(c)GxCore_MutexUnlock(c->free_list_mutex)
#define FREE_LIST_LOCK_INIT(c) GxCore_MutexCreate(&c->free_list_mutex)
#define FREE_LIST_LOCK_UNINIT(c) GxCore_MutexDelete(c->free_list_mutex)

#define USED_LIST_LOCK(c) GxCore_MutexLock(c->used_list_mutex)
#define USED_LIST_UNLOCK(c)GxCore_MutexUnlock(c->used_list_mutex)
#define USED_LIST_LOCK_INIT(c) GxCore_MutexCreate(&c->used_list_mutex)
#define USED_LIST_LOCK_UNINIT(c) GxCore_MutexDelete(c->used_list_mutex)

struct cachepage *app_des_cache_alloc_page(struct des_cache *cache)
{
	struct cachepage* page = NULL;

	GxCore_SemWait(cache->free_sem);

	FREE_LIST_LOCK(cache);
	page = (struct cachepage*)gxlist_get(&cache->free_list);
	FREE_LIST_UNLOCK(cache);

	if(page){
		cache->free_page_num--;
	}
	else if(cache->alloc_page_num < cache->max_used_num){
		page = (struct cachepage*)GxCore_Mallocz(sizeof(struct cachepage));
		if(page){
			page->buffer = GxCore_Malloc(cache->page_size);
			if(page->buffer == NULL){
				GxCore_Free(page);
				return NULL;
			}
			page->size = cache->page_size;
			cache->alloc_page_num++;
		}
	}

    //app_log_debug("%s page total %d, free %d\n",__func__, cache->alloc_page_num, cache->free_page_num);

	return page;
}

void app_des_cache_free_page(struct des_cache *cache, struct cachepage *page)
{
	if (cache->free_page_num > cache->max_cache_num){
		if (page){
			GxCore_Free(page->buffer);
			GxCore_Free(page);
			cache->alloc_page_num--;
		}
	}
	else {
		FREE_LIST_LOCK(cache);
		gxlist_add(&page->head, &cache->free_list);
		cache->free_page_num++;
		FREE_LIST_UNLOCK(cache);
	}

    //app_log_debug("%s page total %d, free %d\n",__func__, cache->alloc_page_num, cache->free_page_num);

	GxCore_SemPost(cache->free_sem);
}

int app_des_cache_init(struct des_cache *cache, size_t page_size, int max_use_page, int max_cache_page)
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

	return 0;

errout:
	app_des_cache_uninit(cache);
	return -1;
}

void app_des_cache_uninit(struct des_cache *cache)
{
	struct cachepage *page = NULL, *n;

	gxlist_for_each_entry_safe(page, n, &cache->used_list, head){
		GxCore_Free(page->buffer);
		GxCore_Free(page);
	}

	gxlist_for_each_entry_safe(page, n, &cache->free_list, head){
		GxCore_Free(page->buffer);
		GxCore_Free(page);
	}

	USED_LIST_LOCK_UNINIT(cache);
	FREE_LIST_LOCK_UNINIT(cache);

	GxCore_SemDelete(cache->free_sem);
	GxCore_SemDelete(cache->used_sem);
}

void app_des_cache_exit(struct des_cache *cache)
{
	GxCore_SemPost(cache->used_sem);
	GxCore_SemPost(cache->free_sem);
}

struct cachepage* app_des_cache_get_page(struct des_cache *cache)
{
	struct cachepage* page;

	GxCore_SemWait(cache->used_sem);

	USED_LIST_LOCK(cache);
	page = (struct cachepage*)gxlist_get(&cache->used_list);
	USED_LIST_UNLOCK(cache);

	return page;
}

int app_des_cache_put_page(struct des_cache *cache, struct cachepage* page)
{
	USED_LIST_LOCK(cache);
	gxlist_add_tail((struct gxlist_head *)page, &cache->used_list);
	cache->totle_size += page->size;
	USED_LIST_UNLOCK(cache);

	GxCore_SemPost(cache->used_sem);

	return 0;
}

