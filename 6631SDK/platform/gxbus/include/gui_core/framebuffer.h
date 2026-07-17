#ifndef __FRAMEBUFFER_H_
#define __FRAMEBUFFER_H_

#include "gxcore.h"
#include "gxtype.h"
#ifdef NO_OS
#include "common/gx_hw_malloc.h"
#endif /*NO_OS*/
#include "common/memhole.h"

__BEGIN_DECLS

#define                GUI_HW_POOL_THRESHOLD              (32 * 1024)

struct framebuffer {
	uint32_t  kernel_address;
	uint8_t  *mmap_address;
	uint32_t size;
	void *mem_mgr;
	uint32_t used_size;
	uint32_t max_used_size;
	uint32_t overflow_size;
	uint32_t max_overflow_size;
	handle_t lock;
	void *hw_pool;
	size_t hw_pool_size;
	size_t hw_pool_threashold;
};

void fb_init(struct framebuffer *fb);
void *fb_malloc(struct framebuffer *fb, GxHwMallocObj *p);
void *fb_malloc_nocache(struct framebuffer *fb, GxHwMallocObj *p);
void *fb_malloc_main(struct framebuffer *fb, GxHwMallocObj *p, int flag);
void fb_free(struct framebuffer *fb, GxHwMallocObj *p);
int fb_check(struct framebuffer *fb, GxHwMallocObj *p);
void fb_info_print(void);
void fb_print_hwpool_hitrate(void);

/**
 *
 * \brief   获取GUI对洞内存的使用情况
 * \details    使用场景： 调试,查看GUI当前洞内存的使用情况，包括峰值和当前值
 *
 * \param   info    [out]  Memhole 使用情况.
 * \retval  void   无
 **/

void GxGUI_GetMemHoleUsdInfo(GxMemholeInfo *info);

static inline int is_fb_memory(struct framebuffer *fb, uint8_t *p) {
	return p >= fb->mmap_address && p < fb->mmap_address + fb->size ? 1 : 0;
}

static inline void *fb_getmmap(struct framebuffer *fb, uint32_t p)
{
	if (fb && p >= fb->kernel_address && p < fb->kernel_address + fb->size)
		return fb->mmap_address + (p - fb->kernel_address);

	return NULL;
}

static inline uint32_t fb_getphys(struct framebuffer *fb, uint8_t *p)
{
	if (fb && p >= fb->mmap_address && p < fb->mmap_address + fb->size)
		return fb->kernel_address + (p - fb->mmap_address);

	return 0;
}

__END_DECLS

#endif

