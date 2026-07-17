#include "image_cache.h"
#include "gui_private.h"
#include "gui.h"

extern void images_release_data(void *data);
extern void logo_Free(void *data);

static int image_cache_sort_insert(ImageCache *cache, image_desc *new_desc);

static void _free_list_add(ImageCache *cache, image_desc *desc, struct gxlist_head *head)
{
	if (desc->hw_accel) {
		gxlist_add_tail(&desc->head, head);
	}
}

static void _free_list_del(ImageCache *cache, image_desc *desc)
{
	if(desc->head.prev && desc->head.next)
	    gxlist_del_init(&desc->head);
}

status_t image_cache_init(ImageCache *cache, size_t memory_size)
{
	if (cache == NULL)
		return GXCORE_ERROR;

	memset(cache, 0, sizeof(ImageCache));
	cache->hash_imgs = hash_new(40, images_release_data);
	cache->max_memory = memory_size;
	cache->fb = &gui.fb;
	cache->freed_size = 0;
	GxCore_MutexCreate(&cache->lock);

	GX_INIT_LIST_HEAD(&cache->free_sort_list);

	cache->max_size = 128;
	cache->array = GxCore_Malloc(sizeof(void*) * cache->max_size);
	memset(cache->array, 0, sizeof(void*) * cache->max_size);

	return GXCORE_SUCCESS;
}

status_t image_cache_free(ImageCache *cache)
{
	if (cache == NULL)
		return GXCORE_ERROR;
	hash_release(cache->hash_imgs);
	GxCore_Free(cache->array);
	GxCore_MutexDelete(cache->lock);
	GUI_FREE(cache);

	return GXCORE_SUCCESS;
}

static void image_cache_lock(ImageCache *cache)
{
	GxCore_MutexLock(cache->lock);
}

static void image_cache_unlock(ImageCache *cache)
{
	GxCore_MutexUnlock(cache->lock);
}

status_t path_image_cache_put(ImageCache *cache, const char *key, const char *filename, enum load_mode load_mode)
{
	image_desc *desc = hash_get(cache->hash_imgs, key);
	if(NULL == desc)
	{
	    desc = gal_img_new();
	    GX_INIT_LIST_HEAD(&desc->head);
	    hash_add(cache->hash_imgs, key, desc);
	}
	if (desc == NULL)
		return GXCORE_ERROR;

	gal_img_cleanup(desc);

	if(desc->filename)
		GUI_FREE(desc->filename);
	if(desc->vq != NULL)
	{
		hd_free(desc->vq);
		GUI_FREE(desc->vq);
	}
	desc->filename  = GxCore_Strdup(filename);
	desc->load_mode = load_mode;
	desc->data      = NULL;
	desc->size      = 0;

	return GXCORE_SUCCESS;
}

status_t image_cache_put(GuiConfig *config, const char *key, const char *filename, enum load_mode load_mode)
{
	ImageCache *cache = NULL;

	cache = (void *)(config->image_cache);
	image_desc *desc = hash_get(cache->hash_imgs, key);
	if(NULL == desc)
	{
	    desc = gal_img_new();
	    GX_INIT_LIST_HEAD(&desc->head);
	    hash_add(cache->hash_imgs, key, desc);
	}
	if (desc == NULL)
		return GXCORE_ERROR;

	GUI_FREE(desc->filename);
	GUI_FREE(desc->data);
	hd_free(desc->vq);
	GUI_FREE(desc->vq);
	char *full_name = gui_mallocpath_swapfile(config->files.file_xml_image, filename);

	desc->theme     = config->name;
	desc->filename  = full_name;
	desc->load_mode = load_mode;
	desc->data      = NULL;
	desc->size      = 0;

	return GXCORE_SUCCESS;
}

int _gxlogd_list(FILE *fp, struct gxlist_head *list, const char *name, size_t *size)
{
	int count = 0;
	image_desc *p;

	fprintf(fp, "%s\n", name);
	gxlist_for_each_entry(p, list, head) {
		if (size)
			*size += p->size;
		count++;
		fprintf(fp, "\t%s, hot=%d, ref=%d, size=%d\n",
				p->filename, p->hot, p->ref, p->size);
	}
	fprintf(fp, "========================================================================\n");

	return count;
}

void image_cache_list_image(ImageCache *cache, const char *msg)
{
	gxlogd("Cache Info end: %dkb/%dkb\n", cache->freed_size / 1024, cache->max_memory / 1024);
	gxlogd("======================= %s ====================\n", msg);
	_gxlogd_list(stdout, &cache->free_sort_list, "free_sort_list", NULL);
	gxlogd("======================= end ====================\n");
}

/**
 * 清除cache 中所有图片
 *
 */
int image_cache_clean(ImageCache *cache)
{
	image_desc *desc, *n;

	gxlist_for_each_entry_safe(desc, n, &cache->free_sort_list, head) {
		_free_list_del(cache, desc);
		gal_img_cleanup(desc);
	}

	return 0;
}

/**
 * 清除cache 中所有图片,并强制释放洞内存及管理块
 *
 */
int image_cache_clean_all(ImageCache *cache)
{
	image_desc *desc, *n;

	gxlist_for_each_entry_safe(desc, n, &cache->free_sort_list, head) {
		_free_list_del(cache, desc);
		gal_img_cleanup(desc);
		GUI_FREE(desc->vq);
	}

	return 0;
}

/*
 * 找到最适合大小的desc 清理
 */
int image_cache_schedule(ImageCache *cache, int need_size)
{
	image_desc *desc, *n, *prior = NULL;
	int size = 0;

	if (need_size > cache->freed_size) {// 全部释放都不够用，就干脆不要释放了
//		gxlogd("No schedule: %d > %d\n", need_size, cache->freed_size);
		return -1;
	}
	image_cache_lock(cache);
	gxlist_for_each_entry_safe(desc, n, &cache->free_sort_list, head) {
		if (desc->hw_accel == 0)
		    continue;

		if (need_size - size < desc->size) { // 需求的空间比当前 desc 小
			prior = desc;
			continue;
		}

		if (prior != NULL) { // 前面有一个足够大的desc，删掉它就足了
			break;
		}

		size += desc->size;
		_free_list_del(cache, desc);
		gal_img_cleanup(desc);
//		gxlogd("Image Cleanup %s, hot=%d\n", desc->filename, desc->hot);
		if (size >= need_size)
			break;
	}

	if (prior != NULL) {
		size += prior->size;
		_free_list_del(cache, prior);
		gal_img_cleanup(prior);
		prior = NULL;
	}
	image_cache_unlock(cache);

	return 0;
}

/*
 * 1. 按热度从低到高排序
 * 2. 当热度相等时，按大小从大到小排序
 */
static int image_cache_sort_insert(ImageCache *cache, image_desc *new_desc)
{
	image_desc *desc, *n;

	gxlist_for_each_entry_safe(desc, n, &cache->free_sort_list, head) {
		if ((desc == new_desc) ||
			((desc->filename) && (new_desc->filename) && (strcmp(desc->filename, new_desc->filename))) == 0) {
			return 0;
		}

	}

	_free_list_add(cache, new_desc, &cache->free_sort_list);

	return 0;
}

static int image_cache_get_need_img(ImageCache *cache, image_desc **new_desc)
{
	image_desc *desc, *n;

	gxlist_for_each_entry_safe(desc, n, &cache->free_sort_list, head) {
	    if(desc == (*new_desc))
	    {
		*new_desc = desc;
		_free_list_del(cache, desc);
		return 0;
	    }
	}

	*new_desc = NULL;
	return 0;
}

image_desc *image_cache_check(ImageCache *cache, const char *key)
{
	image_desc *desc = NULL;

	if((NULL == cache) || (NULL == key))
		return (desc);

	if(cache->hash_imgs)
		desc = (image_desc *)hash_get(cache->hash_imgs, key);

	return (desc);
}


int image_cache_get(ImageCache *cache, const char *key, image_desc **ret_desc)
{
	image_desc *desc = NULL, *new_desc = NULL;

	if (cache == NULL || key == NULL || ret_desc == NULL)
		return -1;

	image_cache_lock(cache);
	desc = (image_desc*)hash_get(cache->hash_imgs, key);
	image_cache_unlock(cache);
	if (desc == NULL) {
		/*No error if key is 'null'*/
		if(0 != strcasecmp("null", key))
		{
			gxlogd("[WARING]: Image %s No Found NULL\n", key);
		}
		return -1;
	}

	new_desc = desc;
	image_cache_lock(cache);
	image_cache_get_need_img(cache, &new_desc);
	image_cache_unlock(cache);
	if(new_desc)
	{
	    *ret_desc = desc;
	    return 0;
	}

	if(desc->vq) {
	    desc->vq = hd_malloc(&(desc->vq), desc->vq->size);
            hd_check(desc->vq);
	}

	if ((NULL == desc->data) && (NULL == desc->vq)) { // 数据尚未加载
		image_cache_lock(cache);
		desc->type = UNKOWN_TYPE;
		desc = gal_img_load(desc, desc->filename);
		if (NULL == desc) {
			gxlogd("[GUI] %s image load failed.\n", key);
			image_cache_unlock(cache);
			return -1;
		}
		if (desc->hw_accel)
			hd_check(desc->vq);
		if (desc->data == NULL) {
			gxlogd("error : func=%s, key=%s, filename=%s, disc=%p\n", __func__,
					key, desc->filename, desc);
#ifdef GXDBG
			while(1);
#else
			*ret_desc = NULL;
			image_cache_unlock(cache);
			return (-1);
#endif /*GXDBG*/
		}

		image_cache_sort_insert(cache, desc);
		image_cache_unlock(cache);
	}
	else if((desc->vq) &&
		((NO_CACHE == desc->vq->flag) || (NULL == desc->vq->usr_p)) &&
		(desc->load_mode == NO_LOAD)) { // 数据尚未加载
		if(NULL == desc->vq->usr_p) {
			desc->data = NULL;
			desc->width = desc->height = desc->size = 0;
			GUI_FREE(desc->vq);
		}

		image_cache_lock(cache);
		desc->type = UNKOWN_TYPE;
		desc = gal_img_load(desc, desc->filename);
		image_cache_unlock(cache);
		if (NULL == desc) {
			gxlogd("[GUI] %s image load failed\n", key);
			return -1;
		}
		if (desc->hw_accel)
			hd_check(desc->vq);
		if (desc->data == NULL) {
			gxlogd("error : func=%s, key=%s, filename=%s, disc=%p\n", __func__,
					key, desc->filename, desc);
#ifdef GXDBG
			while(1);
#else
			*ret_desc = NULL;
			return (-1);
#endif /*GXDBG*/
		}
	}
	else if(((desc->vq) && (CACHE == desc->vq->flag)) || (desc->load_mode == DELAY_LOAD)) { // 数据加载
	    desc->data = desc->vq->usr_p;
	    desc->hw_accel = GUI_TRUE;
	    desc->size = desc->vq->size;
	}
	if (desc) {
		desc->hot++;
		desc->ref++;
		if (desc->ref == 1) { // 从空闲列表中删除
			image_cache_lock(cache);
			_free_list_del(cache, desc);
			image_cache_unlock(cache);
		}
	}

	*ret_desc = desc;

	return 0;
}

int image_cache_clear(ImageCache *cache, image_desc **desc)
{
	if (desc) {
		if(NO_LOAD != (*desc)->load_mode) {
			return (0);
		}

		image_desc *this_desc = *desc;
		image_cache_lock(cache);
		gxlist_del_init(&this_desc->head); // 从占用队列中移去
		image_cache_sort_insert(cache, this_desc);
		image_cache_unlock(cache);
		*desc = NULL;
	}

	return 0;
}

static void image_auto_load_thread(void *data)
{
	ImageCache *cache = (ImageCache*)data;
	image_desc *desc;
	hash_iterator_t *iter;

	GxCore_ThreadDetach();

	iter = hash_iter_new(cache->hash_imgs);

	while (1) {
		image_cache_lock(cache);
		if (hash_iter_next(iter) == NULL) {
			image_cache_unlock(cache);
			break;
		}
		desc = (image_desc*)hash_iter_get_data(iter);
		if (desc->load_mode == DELAY_LOAD && desc->data == NULL) { // 延迟加载图片
			desc = gal_img_load(desc, desc->filename);
			desc->hot = 10;
			desc->ref = 0;
			image_cache_sort_insert(cache, desc);
		}
		image_cache_unlock(cache);
	}
}

void image_cache_thread(ImageCache *cache)
{
	GxCore_ThreadCreate("image_auto_load_thread",
			&cache->thread_handle,
			image_auto_load_thread,
			cache,
			1024 * 50,
			GXOS_DEFAULT_PRIORITY + 1);
}

void image_cache_log(ImageCache *cache, const char *filename)
{
#ifdef LINUX_OS
	int i;
	size_t all_size = 0;
	GuiSurface *surface;
	FILE *fp = fopen(filename, "w");
	int file_count = 0;

	if (fp == NULL)
		return;

	file_count += _gxlogd_list(fp, &cache->free_sort_list, "\nFree image list:", &all_size);

	fprintf(fp, "All image memory size: %d, File number: %d\n", all_size, file_count);

	fprintf(fp, "\n\nSurface list:\n");
	for (i = 0; i < cache->max_size; i++) {
		surface = (GuiSurface*)cache->array[i];

		if (surface) {
			fprintf(fp, "\tSurface: %d x %d x %d, size = %d\n", surface->sf.width, surface->sf.height, surface->bpp,
					surface->sf.width * surface->sf.height * surface->bpp / 8);
		}
	}

	fprintf(fp, "\n\nFrame buffer total size: %d KB, used size: %d KB(%2.2f%%), max_used_size: %d KB(%2.2f%%), overflow_size: %d KB\n",
			gui.fb.size / 1024, gui.fb.used_size / 1024, gui.fb.used_size / (gui.fb.size / 100.0),
			gui.fb.max_used_size / 1024, gui.fb.max_used_size / (gui.fb.size / 100.0),
			gui.fb.overflow_size / 1024
			);

	fclose(fp);
#endif
}

void image_cache_add_data(ImageCache *cache, void *data)
{
	int i;

	if(NULL == cache) {
		return;
	}

	for (i = 0; i < cache->max_size; i++) {
		if (cache->array[i] == NULL) {
			cache->array[i] = data;
			return;
		}
	}

	cache->array = GxCore_Realloc(cache->array, sizeof(void*) * (cache->max_size + 128));
	memset(&cache->array[cache->max_size], 0, sizeof(void*) * 128);
	cache->array[cache->max_size] = data;
	cache->max_size += 128;
}

void image_cache_clr_data(ImageCache *cache, void *data)
{
	int i;

	if(NULL == cache) {
		return;
	}

	for (i = 0; i < cache->max_size; i++) {
		if (cache->array[i] == data) {
			cache->array[i] = NULL;
			break;
		}
	}
}
