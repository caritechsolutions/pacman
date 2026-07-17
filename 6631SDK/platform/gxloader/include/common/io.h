#ifndef __BOOTLOADER_IO_H
#define __BOOTLOADER_IO_H

#include "sys/types.h"

#define mb()            __asm__ __volatile__ ("" : : : "memory")

//
// RAW I/O
//
static __inline__ unsigned short __swap16(unsigned short x)
{
	return ((unsigned short) ((((unsigned short)(x) & 0x00ff) << 8) |
				(((unsigned short)(x) & 0xff00) >> 8)));
}

static __inline__ unsigned int __swap32(unsigned int x)
{
	return ((unsigned int)((((unsigned int)(x) & 0x000000ffU) << 24) |
				(((unsigned int)(x) & 0x0000ff00U) <<  8) |
				(((unsigned int)(x) & 0x00ff0000U) >>  8) |
				(((unsigned int)(x) & 0xff000000U) >> 24)));
}

#ifdef CONFIG_ENABLE_UBI
#include <linux/byteorder/generic.h>
#else
#define cpu_to_le32(x)          (x)
#define le32_to_cpu(x)          (x)
#define cpu_to_le(x)            (x)
#define cpu_to_be(x)            (__swap32(x))

#define le16_to_cpu(x)         (x)
#define be16_to_cpu(x)         (__swap16(x))
#endif

#define __raw_writew(val, addr) \
		(void)((*(volatile unsigned short *)(unsigned int)(addr)) = (unsigned short)(val))
#define __raw_writel(val, addr) \
		(void)((*(volatile unsigned int *)(unsigned int)(addr)) = (unsigned int)(val))

#define __raw_readb(addr) \
	  ({ unsigned char __v 	= (*(volatile unsigned char *) (addr)); __v; })
#define __raw_readw(addr) \
	  ({ unsigned short __v = (*(volatile unsigned short *) (addr)); __v; })
#define __raw_readl(addr) \
	  ({ unsigned int __v 	= (*(volatile unsigned int *) (addr)); __v; })

#define __raw_writeb(val, addr) \
	  (void)((*(volatile unsigned char *) (addr)) = (unsigned char)(val))

/*
 * Given a physical address and a length, return a virtual address
 * that can be used to access the memory range with the caching
 * properties specified by "flags".
 */
#define MAP_NOCACHE		(0)
#define MAP_WRCOMBINE	(0)
#define MAP_WRBACK		(0)
#define MAP_WRTHROUGH	(0)

static inline void *map_physmem(unsigned long paddr, unsigned long len, unsigned long flags)
{
	return (void *)paddr;
}

/*
 * Take down a mapping set up by map_physmem().
 */
static inline void unmap_physmem(void *vaddr, unsigned long flags)
{

}

#if !defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#define ioremap(physaddr, size)		(physaddr)
#define iounmap(physaddr)			((void)0)
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

#define readb(addr)				__raw_readb((unsigned long)addr)
#define readw(addr)				__raw_readw((unsigned long)addr)
#define readlong(addr)				__raw_readl((unsigned long)addr)
#define writeb(data, addr)			__raw_writeb((unsigned int)data,(unsigned long)addr)
#define writew(data, addr)			__raw_writew((unsigned int)data, (unsigned long)addr)
#define writelong(data, addr)		__raw_writel((unsigned int)data, (unsigned long)addr)

#define REG_SET_CLR_BIT(reg, bit, bit_val) do {		\
	unsigned int tmp = __raw_readl(reg);		\
	tmp &= ~(1UL << (bit));			\
	tmp |= ((bit_val) << (bit));			\
	__raw_writel(tmp, reg);				\
}while(0)

#define REG_GET_BIT(reg, bit) ((unsigned int)((__raw_readl(reg) >> (bit)) & 1))

#define REG_GET_FIELD(reg,mask,offset)\
	((__raw_readl(reg) & (mask)) >> (offset))

#define REG_SET_FIELD(reg,mask,val,offset) do {		\
	unsigned int tmp = __raw_readl(reg);		\
	tmp  &=  ~(mask);				\
	tmp  |=  ((val) << (offset)) & (mask);		\
	__raw_writel(tmp, reg);				\
}while(0)

#define REG_SET_BIT(reg,bit) do {			\
	unsigned int tmp = __raw_readl(reg);		\
	tmp |= (1UL << (bit));				\
	__raw_writel(tmp, reg);				\
}while(0)

#define REG_CLR_BIT(reg,bit) do {			\
	unsigned int tmp = __raw_readl(reg);		\
	tmp &= ~(1UL << (bit));				\
	__raw_writel(tmp, reg);				\
}while(0)

#define REG_GET_VAL(reg)	    __raw_readl(reg)
#define REG_SET_VAL(reg, val)	__raw_writel(val, reg)

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

static inline int generic_ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

#define ffs(x) generic_ffs(x)

#endif
