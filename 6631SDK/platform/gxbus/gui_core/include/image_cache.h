#ifndef IMAGE_CACHE_H
#define IMAGE_CACHE_H

#include "gui_hash.h"
#include "IMG.h"
#include "gui_core.h"

typedef struct {
	hash_t*      hash_imgs;
	size_t       max_memory; // 最大内存限制
	size_t       freed_size;
	handle_t     thread_handle;
	handle_t     lock;
	int          image_count;
	struct gxlist_head free_sort_list;
//	struct gxlist_head used_list;
	// Surface List
	int max_size;
	void **array;
	struct framebuffer *fb;
}ImageCache;

int gal_img_uncache(int flag);
status_t image_cache_init    (ImageCache *cache, size_t memory_size);
status_t image_cache_free    (ImageCache *cache);
void cache_increase(ImageCache *cache, size_t size);
void cache_decrease(ImageCache *cache, size_t size);
int      image_cache_clean   (ImageCache *cache);
int      image_cache_clean_all(ImageCache *cache);
int      image_cache_schedule(ImageCache *cache, int need_size);
void     image_cache_thread  (ImageCache *cache);
status_t image_cache_put(GuiConfig *config, const char *key, const char *filename, enum load_mode load_mode);
status_t path_image_cache_put     (ImageCache *cache, const char *key, const char *filename, enum load_mode load_mode);
int      image_cache_get     (ImageCache *cache, const char *key, image_desc **desc);
int      image_cache_clear   (ImageCache *cache, image_desc **desc);
void     image_cache_log     (ImageCache *cache, const char *filename);
void     image_cache_add_data(ImageCache *cache, void *data);
void     image_cache_clr_data(ImageCache *cache, void *data);
image_desc *image_cache_check(ImageCache *cache, const char *key);

static inline size_t image_cache_empty(ImageCache *cache) {
	int ret = gxlist_empty(&cache->free_sort_list) ? 1 : 0;

	if (ret == 1 && cache->freed_size != 0) {
		gxlogd("[BUG]: error size = %d\n", cache->freed_size);
		while(1);
	}

	return ret;
}
#endif
