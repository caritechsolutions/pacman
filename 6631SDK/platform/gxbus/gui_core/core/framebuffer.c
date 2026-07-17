#include "framebuffer.h"
#include "gui_private.h"
#include "q_malloc.h"
#include "common/memhole.h"
#include "IMG.h"
#include "gui_hw_pool.h"
#include "module/config/gxconfig.h"
#ifndef NO_OS
#include "image_cache.h"
#endif /*NO_OS*/
#include "gdi.h"

//#define FREE_NOCACHE

#define Q_MALLOC_INIT      vqm_malloc_init
#    define Q_MALLOC(p, flag) 		GxCore_HwMalloc(p, flag)
#    define Q_FREE(p)       		GxCore_HwFree(p)
static inline void fb_lock(struct framebuffer *fb) {
	GxCore_MutexLock(fb->lock);
}

static inline void fb_unlock(struct framebuffer *fb) {
	GxCore_MutexUnlock(fb->lock);
}

static inline void *_framebuffer_malloc(struct framebuffer *fb, GxHwMallocObj *p)
{
	fb_lock(fb);
	if((NULL == p->usr_p) && (NULL == p->kernel_p))
	{
		/*For GDI, there is no memory management system*/
		if(gui.config.only_gdi) {
			Q_MALLOC(p, MALLOC_NO_CACHE);
			if(p->usr_p && p->kernel_p) {
				fb->used_size += p->size;
			}
		} else {
			Q_MALLOC(p, MALLOC_WITH_CACHE);
			if(p->usr_p && p->kernel_p) {
				fb->used_size += p->size;
			}
		}
	}

	if(fb->max_used_size < fb->used_size) {
		fb->max_used_size = fb->used_size;
	}

	fb_unlock(fb);

	return p;
}

static inline void *_framebuffer_malloc_nocache(struct framebuffer *fb, GxHwMallocObj *p)
{
	fb_lock(fb);
	if((NULL == p->usr_p) && (NULL == p->kernel_p))
	{
		Q_MALLOC(p, MALLOC_NO_CACHE);
		if(p->usr_p && p->kernel_p) {
			fb->used_size += p->size;
		}
	}
	if(fb->max_used_size < fb->used_size) {
		fb->max_used_size = fb->used_size;
	}

	fb_unlock(fb);

	return p;
}

static inline void *_framebuffer_malloc_main(struct framebuffer *fb, GxHwMallocObj *p, int flag)
{
	fb_lock(fb);
	if((NULL == p->usr_p) && (NULL == p->kernel_p))
	{
		Q_MALLOC(p, flag);
		if(p->usr_p && p->kernel_p) {
			fb->used_size += p->size;
		}
	}
	if(fb->max_used_size < fb->used_size) {
		fb->max_used_size = fb->used_size;
	}

	fb_unlock(fb);

	return p;
}

static inline void _framebuffer_free(struct framebuffer *fb, GxHwMallocObj *p)
{
	fb_lock(fb);
	if((p->usr_p) && (p->kernel_p))
	{
		fb->used_size -= p->size;
		Q_FREE(p);
		p->usr_p = p->kernel_p = NULL;
	}
	fb_unlock(fb);
}

static void hw_pool_init(struct framebuffer *fb)
{
#ifndef NO_OS
	int mem_size = 0;
	GxHwMallocObj p = {0};

	GxBus_ConfigGetInt(GUI_HW_POOL_SIZE, &mem_size, GUIHW_POOL_SIZE);
	GxBus_ConfigGetInt(GUI_HW_POOL_THRESHOLD_KEY, (int32_t *)(&(fb->hw_pool_threashold)), GUI_HW_POOL_THRESHOLD);
	p.size = mem_size;
	Q_MALLOC((&p), MALLOC_NO_CACHE);
	gui_hw_pool_init(p.usr_p, (int32_t)p.kernel_p, mem_size);
	if(p.usr_p) {
		fb->hw_pool = p.usr_p;
		fb->hw_pool_size = mem_size;
	}
#endif /*NO_OS*/
}

void fb_init(struct framebuffer *fb)
{
	GxHwInfo vq_info = {0};
	struct gxav_memhole_info info;

	memset(&info, 0, sizeof(info));
	strcpy(info.name, "videomem");
	GxAVMemHoleGetInfo(g_device_handle, &info);
	GxCore_HwGetInfo(&vq_info);

	memset(fb, 0, sizeof(struct framebuffer));
	fb->kernel_address = (uint32_t)vq_info.kernel_address;
	fb->size = vq_info.size;
	fb->mmap_address = vq_info.mmp_address;
	GxCore_MutexCreate(&fb->lock);

	fb->mem_mgr = (void *)fb->mmap_address;
	gxlogd("framebuffer phys = %x, mmap = %p, size = %d, mgr = %p\n", fb->kernel_address, fb->mmap_address, fb->size, fb->mem_mgr);
#ifndef NO_OS
	gui.config.image_cache = GxCore_Malloc(sizeof(ImageCache));
	image_cache_init(gui.config.image_cache, fb->size);
#endif /*NO_OS*/
	hw_pool_init(fb);
}

static bool _is_gui_hw_mem(GxHwMallocObj *p)
{
	if((p->usr_p) &&
	   (p->usr_p >= gui.fb.hw_pool) && (p->usr_p <= (gui.fb.hw_pool + gui.fb.hw_pool_size)))
		return (GUI_TRUE);
	else
		return (GUI_FALSE);
}

int s_total_fb_times;
int s_pool_fb_times;

void *fb_malloc(struct framebuffer *fb, GxHwMallocObj *p)
{
	if(p->size <= fb->hw_pool_threashold) {
		s_total_fb_times++;
		if((p->usr_p) && (!_is_gui_hw_mem(p))) {
			goto NORMAL;
		}
		gui_hw_pool_malloc(p, MALLOC_WITH_CACHE);
		if(((NULL != p->usr_p) && (NULL != p->kernel_p)) &&
		   ((p->usr_p >= fb->hw_pool) && (p->usr_p <= (fb->hw_pool + fb->hw_pool_size)))) {
			s_pool_fb_times++;
			return (p);
		} else {
			if(p->id) {
				s_pool_fb_times++;
				p->id = 0;
				goto NORMAL;
//				return p;
			} else {
				p->id = 0;
				p->usr_p = p->kernel_p = NULL;
			}
		}
	}
NORMAL:
	p = _framebuffer_malloc(fb, p);
	if ((NULL == p) ||
		((p->usr_p == NULL) && (p->kernel_p == NULL))) {
		gdi_commit();
#ifndef NO_OS
		image_cache_clean(gui.config.image_cache);
#endif /*NO_OS*/
		p = _framebuffer_malloc(fb, p);
	}

	return p;
}


void *fb_malloc_nocache(struct framebuffer *fb, GxHwMallocObj *p)
{
#ifndef NO_OS
	if(p->size <= fb->hw_pool_threashold) {
		s_total_fb_times++;
		gui_hw_pool_malloc(p, MALLOC_NO_CACHE);
		if(((NULL != p->usr_p) && (NULL != p->kernel_p)) &&
		   ((p->usr_p >= fb->hw_pool) && (p->usr_p <= (fb->hw_pool + fb->hw_pool_size)))) {
			s_pool_fb_times++;
			return (p);
		} else {
			if(p->id) {
				s_pool_fb_times++;
				return p;
			} else {
				p->id = 0;
				p->usr_p = p->kernel_p = NULL;
			}
		}
	}
#endif /*NO_OS*/
	p = _framebuffer_malloc_nocache(fb, p);
	if ((NULL == p) ||
		((p->usr_p == NULL) && (p->kernel_p == NULL))) {
		gdi_commit();
#ifndef NO_OS
		image_cache_clean(gui.config.image_cache);
#endif /*NO_OS*/
		p = _framebuffer_malloc_nocache(fb, p);
	}

	return p;
}

void fb_print_hwpool_hitrate(void)
{
	int rate = 0;

	rate = s_pool_fb_times * 100 / s_total_fb_times;

	gxlogi("#########FB POOL HIT Rate = %d %%############\n", rate);
}

void *fb_malloc_main(struct framebuffer *fb, GxHwMallocObj *p, int flag)
{
	p = _framebuffer_malloc_main(fb, p, flag);
	return p;
}

void fb_free(struct framebuffer *fb, GxHwMallocObj *p)
{
	if (p) {
#ifndef NO_OS
		if(_is_gui_hw_mem(p)) {
			gui_hw_pool_free(p);
			return;
		}
#endif /*NO_OS*/
		if((p->usr_p) && (p->kernel_p)) {
			fb->used_size -= p->size;
		}

		if(fb->used_size < 0) {
			gxlogf ("-----------overflow-------\n");
		}

		_framebuffer_free(fb, p); //, gui.config.block_join);
		if (fb->used_size == 0)
			fb->mem_mgr = (void*)fb->mmap_address;
	}
}

int fb_check(struct framebuffer *fb, GxHwMallocObj *p)
{
	int ret = 0;

	if (p) {
		if((p->usr_p >= fb->hw_pool) && (p->usr_p <= (fb->hw_pool + fb->hw_pool_size))) {
			ret = gui_hw_pool_check(p);
			if(ret) {
				while(1);
			}
			return (ret);
		}
		ret = GxCore_HwCheck(p);
	}

	if(ret)
		while(1);

	return 0;
}

void fb_info_print(void)
{
#ifndef NO_OS
	fb_lock(&(gui.fb));
	gxlogf("-------------------------------------------\n");
	gxlogf("\tFB used size = %d\t\n", gui.fb.used_size);
	gxlogf("\tFB max used size = %d\t\n", gui.fb.max_used_size);
	gxlogf("\tFB overflow size = %d\t\n", gui.fb.overflow_size);
	gxlogf("-------------------------------------------\n");
	fb_unlock(&(gui.fb));
#endif
}

void GxGUI_GetMemHoleUsdInfo(GxMemholeInfo *info)
{
#ifndef NO_OS
	const char *window_name = NULL;

	if(NULL == info) {
		return;
	}

	memset(info->desc, 0, 128 * sizeof(char));
	window_name = GUI_GetFocusWindow();
	if(NULL == window_name) {
		strncpy(info->desc, "null", 4);
	} else {
		strncpy(info->desc, window_name, strlen(window_name));
	}

	info->now_used = gui.fb.used_size;
	info->max_used = gui.fb.max_used_size;
#endif

	return;
}

void GxGUI_GetMemHoleOverFlowInfo(GxMemholeInfo *info)
{
#ifndef NO_OS
	const char *window_name = NULL;

	if(NULL == info) {
		return;
	}

	memset(info->desc, 0, 128 * sizeof(char));
	window_name = GUI_GetFocusWindow();
	if(NULL == window_name) {
		strncpy(info->desc, "null", 4);
	} else {
		strncpy(info->desc, window_name, strlen(window_name));
	}

	info->now_used = gui.fb.overflow_size;
	info->max_used = gui.fb.max_overflow_size;
#endif

	return;
}

