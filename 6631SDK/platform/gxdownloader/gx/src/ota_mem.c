#include "common/gxmalloc.h"
#include "gxcore.h"
#include "ota_mem.h"

unsigned char *download_memhole_malloc(unsigned long size)
{
#ifdef NO_OS
	return GxCore_Malloc(size);
#else
	unsigned char *node;
	unsigned char *tmp, *ret;
	unsigned int page_align = 4;
	GxHwMallocObj *vqobj;
	unsigned int *p;

	unsigned int ssize = sizeof(GxHwMallocObj) + size + page_align;

	vqobj = GxCore_Mallocz(sizeof(GxHwMallocObj));
	if (vqobj == NULL) {
		gxloge("%s,malloc (vqobj) NULL\n",__func__);
		return NULL;
	}

	vqobj->size = ssize;
	GxCore_HwMalloc(vqobj, MALLOC_NO_CACHE);

	if (vqobj->kernel_p == NULL) {
		GxCore_Free(vqobj);
		gxloge("%s,hw malloc (%d) NULL\n",__func__, ssize);
		return NULL;
	}

	node = vqobj->usr_p;
	tmp = node + sizeof(void *);
	ret = (unsigned char *)((unsigned int)(tmp + page_align - 1) & (~(unsigned int)( page_align - 1)));
	p   = (unsigned int *) (ret - 4);
	*p  = (unsigned int)vqobj;

	return ret;
#endif
}

unsigned char *download_memhole_realloc(unsigned char *mem, unsigned long old_size, unsigned long new_size)
{
#ifdef NO_OS
	return GxCore_Realloc(mem, new_size);
#else
	unsigned char *ptr = download_memhole_malloc(new_size);
	if (ptr == NULL)
		return ptr;
	if (old_size < new_size)
		memcpy(ptr, mem, old_size);
	else
		memcpy(ptr, mem, new_size);
	download_memhole_free(mem);
	return ptr;
#endif
}

void download_memhole_free(unsigned char *p)
{
#ifdef NO_OS
	GxCore_Free(p);
#else
	if (p) {
		GxHwMallocObj *tmp = (GxHwMallocObj *)(*(unsigned int *)(p - 4));
		if (tmp) {
			//GxCore_HwCheckDebug();
			GxCore_HwFree(tmp);
			GxCore_Free(tmp);
		}
	}
#endif
}
