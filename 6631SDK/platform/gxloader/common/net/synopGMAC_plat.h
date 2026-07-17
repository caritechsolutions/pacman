#ifndef SYNOP_GMAC_PLAT_H
#define SYNOP_GMAC_PLAT_H


#include "synopGMAC_plat.h"
#include <linux/dma-mapping.h>
#include "stdio.h"
#include "time.h"
#include "sys/types.h"
#include "io.h"
#include "stdio.h"

#ifndef IRQ_NONE
#define IRQ_NONE	(0)
#endif
#ifndef IRQ_HANDLED
#define IRQ_HANDLED	(1)
#endif

#if 0
#define TR0(fmt, args...) printf(fmt, ##args)
#else
# define TR0(fmt, args...) /* not debugging: nothing */
#endif

#ifdef DEBUG
#undef TR
#  define TR(fmt, args...) printf(fmt, ##args)
#else
# define TR(fmt, args...) /* not debugging: nothing */
#endif

#define DEFAULT_DELAY_VARIABLE  10
#define DEFAULT_LOOP_VARIABLE   10000

/* There are platform related endian conversions
 *
 */

#define LE32_TO_CPU	__le32_to_cpu
#define BE32_TO_CPU	__be32_to_cpu
#define CPU_TO_LE32	__cpu_to_le32

/* Error Codes */
#define ESYNOPGMACNOERR   0
#define ESYNOPGMACNOMEM   1
#define ESYNOPGMACPHYERR  2
#define ESYNOPGMACBUSY    3

static void __inline__ *plat_alloc_memory(u32 bytes)
{
	return kmalloc((size_t)bytes, 0);
}

static __inline__ void *plat_alloc_consistent_dmaable_memory(u32 size, u32 *addr)
{
	return dma_alloc_coherent(size, addr);
}

static __inline__ void plat_free_consistent_dmaable_memory(u32 size, void *addr,u32 dma_addr)
{
	if(addr)
		free(addr);
}

static void __inline__ plat_free_memory(void *buffer)
{
	kfree(buffer);
}

static void __inline__ plat_delay(u32 delay)
{
	udelay(delay);
}

static u32 __inline__ synopGMACReadReg(u32 *RegBase, u32 RegOffset)
{
	u32 addr = (u32)RegBase + RegOffset;
	return readlong((void *)addr);
}

static void  __inline__ synopGMACWriteReg(u32 *RegBase, u32 RegOffset, u32 RegData)
{
	u32 addr = (u32)RegBase + RegOffset;
	writelong(RegData,(void *)addr);
}

static void __inline__ synopGMACSetBits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readlong((void *)addr);
	data |= BitPos;
	writelong(data,(void *)addr);
}

static void __inline__ synopGMACClearBits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readlong((void *)addr);
	data &= (~BitPos);
	writelong(data,(void *)addr);
}

static bool __inline__ synopGMACCheckBits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
	u32 addr = (u32)RegBase + RegOffset;
	u32 data = readlong((void *)addr);
	data &= BitPos;

	return data ? true : false;
}

#endif
