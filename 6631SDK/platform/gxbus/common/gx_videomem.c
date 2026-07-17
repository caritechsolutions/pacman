#include "gxcore.h"
#include "common/gx_hw_malloc.h"

unsigned char* Gx_VideoMemoryMapToUser(unsigned char *p)
{
	return GxCore_MemholeMap(p);
}

unsigned char* Gx_VideoMemoryMapToKernel(unsigned char *p)
{
	if (p) {
		GxHwInfo vq_info = {0};
		unsigned int addr_k, addr_u;
		GxCore_HwGetInfo(&vq_info);

		addr_k = (unsigned int)vq_info.kernel_address;
		addr_u = (unsigned int)vq_info.mmp_address;
		return (unsigned char*)((unsigned int)p - (addr_u - addr_k));
	}

	return NULL;
}

unsigned char *Gx_VideoMemoryMalloc(unsigned long size, unsigned int align_shift)
{
	unsigned char *uaddr = NULL;
	GxCore_MemholeMallocExt(size, &uaddr, NULL, MALLOC_NO_CACHE, align_shift);
	return uaddr;
}

void Gx_VideoMemoryFree(unsigned char *p)
{
	GxCore_MemholeFree(Gx_VideoMemoryMapToUser(p));
}
