// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <sys/types.h>
#include <asm/types.h>
#include "system.h"

#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)

static unsigned int mmu_table[4096 + 256] __attribute__ ((section (".mmu_tables")));

static void set_section_phys(int section, phys_addr_t phys,
			     enum dcache_option option)
{
	u32 *page_table = (u32 *)mmu_table;
	u32 value = TTB_SECT_AP;

	/* Add the page offset */
	value |= phys;

	/* Add caching bits */
	value |= option;

	/* Set PTE */
	page_table[section] = value;
}

void set_section_dcache(int section, enum dcache_option option)
{
	set_section_phys(section, (u32)section << MMU_SECTION_SHIFT, option);
}

void mmu_set_region_dcache_behaviour_phys(phys_addr_t start, phys_addr_t phys,
					size_t size, enum dcache_option option)
{
	u32 *page_table = (u32 *)mmu_table;
	unsigned long startpt, stoppt;
	unsigned long upto, end;

	/* div by 2 before start + size to avoid phys_addr_t overflow */
	end = ALIGN((start / 2) + (size / 2), MMU_SECTION_SIZE / 2)
	      >> (MMU_SECTION_SHIFT - 1);
	start = start >> MMU_SECTION_SHIFT;

	for (upto = start; upto < end; upto++, phys += MMU_SECTION_SIZE)
		set_section_phys(upto, phys, option);
}

/* to activate the MMU we need to set up virtual memory: use 1M areas */
void mmu_table_setup(void)
{
	int i;
	u32 reg;

	/* Set up an identity-mapping for all 4GB, rw for everyone */
	for (i = 0; i < ((4096ULL * 1024 * 1024) >> MMU_SECTION_SHIFT); i++)
		set_section_dcache(i, DCACHE_OFF);

	/* cacheable sdram */
	mmu_set_region_dcache_behaviour_phys(0x00000000, 0x00000000, 1024 * 1024 * 1024, DCACHE_WRITEBACK);
	/* uncacheable sdram */
	mmu_set_region_dcache_behaviour_phys(0x40000000, 0x00000000, 1024 * 1024 * 1024, DCACHE_OFF);
	/* cacheable ROM  */
	mmu_set_region_dcache_behaviour_phys(0x80000000, 0x80000000, 1 * 1024 * 1024, DCACHE_WRITEBACK);
	/* io */
	mmu_set_region_dcache_behaviour_phys(0x80100000, 0x80100000, 3 * 1024 * 1024, DCACHE_OFF);
	/* cacheable sram */
	mmu_set_region_dcache_behaviour_phys(0x80400000, 0x80400000, 1 * 1024 * 1024, DCACHE_WRITEBACK);
	/* io */
	mmu_set_region_dcache_behaviour_phys(0x80500000, 0x80500000, 2043 * 1024 * 1024, DCACHE_OFF);

	if ((get_cr() & CR_C) != 0) {
		flush_dcache_all();
		/* Invalidate entire unified TLB */
		asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));
		asm volatile("dsb");
		asm volatile("isb");
	}

	/* Set TTBR0 */
	reg = (u32)mmu_table;
	asm volatile("mcr p15, 0, %0, c2, c0, 0"
		     : : "r" (reg) : "memory");
}
