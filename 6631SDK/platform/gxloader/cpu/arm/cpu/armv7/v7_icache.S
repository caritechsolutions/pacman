#include "linkage.h"

.section ".cache_section"

/*
 *	flush_icache_all()
 *
 *	Flush(Clean and Invalidate) the whole I-cache.
 *
 */
ENTRY(flush_icache_all)
	bx lr
ENDPROC(flush_icache_all)

/*
 *	invalidate_icache_all()
 *
 *	Invalidate the whole I-cache.
 *
 */
ENTRY(invalidate_icache_all)
	mov     r0, #0
	// Invalidate all instruction caches to PoU. Also flushes branch target cache.
	mcr p15, 0, r0, c7, c5, 0
	// Invalidate entire branch predictor array
	mcr p15, 0, r0, c7, c5, 6
	bx lr
ENDPROC(invalidate_icache_all)
