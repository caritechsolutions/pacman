#ifndef __APP_DES_CACHE_H__
#define __APP_DES_CACHE_H__

#include "common/list.h"
#include "gxtype.h"
#include "gxos/gxcore_os_core.h"
#include "gxos/gxcore_os.h"
#include "common/gxmalloc.h"
#include "app_config.h"


struct cachepage {
	struct gxlist_head  head;
	unsigned char*      buffer;
	size_t              size;
	unsigned int        pts;
};

struct des_cache {
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
};

int app_des_cache_init(struct des_cache *cache, size_t page_size, int max_use_page, int max_cache_page);

void app_des_cache_uninit(struct des_cache *cache);

struct cachepage *app_des_cache_alloc_page(struct des_cache *cache);

void app_des_cache_free_page(struct des_cache *cache, struct cachepage *page);

struct cachepage* app_des_cache_get_page(struct des_cache *cache);

int app_des_cache_put_page(struct des_cache *cache, struct cachepage* page);

void app_des_cache_exit(struct des_cache *cache);

#endif
