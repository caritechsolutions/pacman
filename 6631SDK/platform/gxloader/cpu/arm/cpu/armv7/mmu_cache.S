#include "linkage.h"

/*
 *	turn on icache
 *
 */
ENTRY(enable_icache)
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #0x1000               // Set I bit
	mcr p15, 0, r0, c1, c0, 0
	bx lr
ENDPROC(enable_icache)

/*
 *	turn on dcache
 *
 */
ENTRY(enable_dcache)
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #0x0004               // Set C bit
	mcr p15, 0, r0, c1, c0, 0
	bx lr
ENDPROC(enable_dcache)

/*
 *	turn off icache
 *
 */
ENTRY(disable_icache)
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x1000               // Clear I bit
	mcr p15, 0, r0, c1, c0, 0
	bx lr
ENDPROC(disable_icache)

/*
 *	turn off dcache
 *
 */
ENTRY(disable_dcache)
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x0004               // Clear C bit
	mcr p15, 0, r0, c1, c0, 0
	bx lr
ENDPROC(disable_dcache)

/*
 *	turn off mmu
 *
 */
ENTRY(disable_mmu)
	mov r0, #0
	mcr p15, 0, r0, c8, c7, 0          //    @ invalidate TLBs
	mcr p15, 0, r0, c8, c6, 0          //    @ invalidate data TLBs
	mcr p15, 0, r0, c8, c5, 0          //    @ invalidate instruction TLBs
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x1                   //    @ close mmu
	mcr p15, 0, r0, c1, c0, 0
	bx lr
ENDPROC(disable_mmu)
