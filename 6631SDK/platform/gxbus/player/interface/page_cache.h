#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "gx_common.h"

#define MAX_HOLEPAGES (128)

struct page_hole {
	unsigned char *buffer;
	unsigned int size;
	unsigned int maxcnt;
	unsigned int nowcnt;
	struct {
		unsigned char *buffer;
		unsigned int used;
	} pages[MAX_HOLEPAGES];
};

struct cache_page {
	struct gxlist_head  head;
	unsigned char*      buffer;
	size_t              size;
	unsigned int        pts;
	int                 security;
};

struct page_cache {
	struct gxlist_head  used_list;
	struct gxlist_head  free_list;

	handle_t            used_sem;
	handle_t            free_sem;

	int                 used_list_mutex;
	int                 free_list_mutex;

	int                 free_page_num;
	int                 alloc_page_num;

	int                 max_used_num;
	int                 max_cache_num;

	size_t              page_size;
	size_t              totle_size;

	struct page_hole    memhole;
};

int page_cache_init(struct page_cache *cache, size_t page_size, int max_use_page, int max_cache_page, size_t hole_size);

void page_cache_uninit(struct page_cache *cache);

struct cache_page *page_cache_alloc_page(struct page_cache *cache);

void page_cache_free_page(struct page_cache *cache, struct cache_page *page);

struct cache_page* page_cache_get_page(struct page_cache *cache);

int page_cache_put_page(struct page_cache *cache, struct cache_page* page);

void page_cache_exit(struct page_cache *cache);

int page_cache_is_full(struct page_cache *cache);

#endif
