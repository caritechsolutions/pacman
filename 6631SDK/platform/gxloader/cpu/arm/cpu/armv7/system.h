#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define CR_M	(1 << 0)	/* MMU enable				*/
#define CR_A	(1 << 1)	/* Alignment abort enable		*/
#define CR_C	(1 << 2)	/* Dcache enable			*/
#define CR_W	(1 << 3)	/* Write buffer enable			*/
#define CR_P	(1 << 4)	/* 32-bit exception handler		*/
#define CR_D	(1 << 5)	/* 32-bit data address range		*/
#define CR_L	(1 << 6)	/* Implementation defined		*/
#define CR_B	(1 << 7)	/* Big endian				*/
#define CR_S	(1 << 8)	/* System MMU protection		*/
#define CR_R	(1 << 9)	/* ROM MMU protection			*/
#define CR_F	(1 << 10)	/* Implementation defined		*/
#define CR_Z	(1 << 11)	/* Implementation defined		*/
#define CR_I	(1 << 12)	/* Icache enable			*/
#define CR_V	(1 << 13)	/* Vectors relocated to 0xffff0000	*/
#define CR_RR	(1 << 14)	/* Round Robin cache replacement	*/
#define CR_L4	(1 << 15)	/* LDR pc can set T bit			*/
#define CR_DT	(1 << 16)
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	/* Fast interrupt (lower latency mode)	*/
#define CR_U	(1 << 22)	/* Unaligned access operation		*/
#define CR_XP	(1 << 23)	/* Extended page tables			*/
#define CR_VE	(1 << 24)	/* Vectored interrupts			*/
#define CR_EE	(1 << 25)	/* Exception (Big) Endian		*/
#define CR_TRE	(1 << 28)	/* TEX remap enable			*/
#define CR_AFE	(1 << 29)	/* Access flag enable			*/
#define CR_TE	(1 << 30)	/* Thumb exception enable		*/


/* Short-Descriptor Translation Table Level 1 Bits */
#define TTB_SECT_NS_MASK	(1 << 19)
#define TTB_SECT_NG_MASK	(1 << 17)
#define TTB_SECT_S_MASK		(1 << 16)
/* Note: TTB AP bits are set elsewhere */
#define TTB_SECT_AP		(3 << 10)
#define TTB_SECT_TEX(x)		((x & 0x7) << 12)
#define TTB_SECT_DOMAIN(x)	((x & 0xf) << 5)
#define TTB_SECT_XN_MASK	(1 << 4)
#define TTB_SECT_C_MASK		(1 << 3)
#define TTB_SECT_B_MASK		(1 << 2)
#define TTB_SECT		(2 << 0)

/* Size of an MMU section */
enum {
	MMU_SECTION_SHIFT	= 20, /* 1MB */
	MMU_SECTION_SIZE	= 1 << MMU_SECTION_SHIFT,
};
/*
 * Short-descriptor format memory region attributes, without TEX remap
 * (ARM Architecture Reference Manual section G5.7.2 [DDI0487E.a])
 *
 * TEX[0] C  B
 *   0    0  0   Device-nGnRnE (aka Strongly-Ordered)
 *   0    1  0   Outer/Inner Write-Through, Read-Allocate No Write-Allocate
 *   0    1  1   Outer/Inner Write-Back, Read-Allocate No Write-Allocate
 *   1    1  1   Outer/Inner Write-Back, Read-Allocate Write-Allocate
 */
enum dcache_option {
	DCACHE_OFF = TTB_SECT_DOMAIN(0) | TTB_SECT_XN_MASK | TTB_SECT,
	DCACHE_WRITETHROUGH = TTB_SECT_DOMAIN(0) | TTB_SECT | TTB_SECT_C_MASK,
	DCACHE_WRITEBACK = DCACHE_WRITETHROUGH | TTB_SECT_B_MASK,
	DCACHE_WRITEALLOC = DCACHE_WRITEBACK | TTB_SECT_TEX(1),
};

static inline unsigned int get_cr(void)
{
	unsigned int val;

	asm volatile("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val)
								:
								: "cc");
	return val;
}

#endif
