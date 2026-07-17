#include "decompress.h"
#include "sys/types.h"

static void cache_flush_all(void)
{
#if defined(CONFIG_ARCH_CKMMU)
	//icache & dcache clr & inv
	asm volatile(	"movi r7, 0x33;"
			"sync;"
			"mtcr r7, cr17;"
			:
			:
			:"r7","memory"
		    );
#elif defined(CONFIG_ARCH_ARM)
	void flush_dcache_all(void);
	void invalidate_icache_all(void);

	// flush dcache & inv icache
	flush_dcache_all();
	invalidate_icache_all();
#endif
}

int decompress_stage2(void)
{
	extern unsigned long input_data;
	extern unsigned long input_data_end;
	unsigned long compressed_stage2_size = (unsigned long)&input_data_end - (unsigned long)&input_data;
	extern int __bss_start__, __bss_end__;

	/* clears BSS section */
	memset(&__bss_start__, 0, (uint32_t)&__bss_end__ - (uint32_t)&__bss_start__);

	unsigned long mem_start = 0;
	unsigned long mem_end   = 0;

	/* initialize heap */
	mem_start = HEAP_START_ADDR;
	mem_end   = HEAP_END_ADDR;
	heap_init((void *)mem_start, mem_end - mem_start);

	printf("decompress stage2...");
	decompress((unsigned char *)&input_data, compressed_stage2_size, (unsigned char *)LOADER_START_ADDR, LOADER_END_ADDR - LOADER_START_ADDR);
	printf("ok\n");

	cache_flush_all();

	return 0;
}
