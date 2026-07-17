/*
 * CPU specific code
 */

void icache_disable(void)
{
	/* turn off I-cache */
	asm volatile (
			"mfcr r7 , cr18;"
			"bclri r7 , 2;"
			"mtcr r7 , cr18;"
			"nop;"
			"nop;"
			:
			:
			:"r7","memory");
}

void dcache_disable(void)
{
	/* turn off D-cache */
	asm volatile (
			"mfcr r7 , cr18;"
			"bclri r7 , 3;"
			"mtcr r7 , cr18;"
			"nop;"
			"nop;"
			:
			:
			:"r7","memory");
}

void icache_flush(void)
{
	/* flush I-cache, icache clr & inv*/
	asm volatile(
			"movi r7, 0x31;"
			"sync;"
			"mtcr r7, cr17;"
			:
			:
			:"r7","memory");
}

void dcache_flush(void)
{
	/* flush D-cache, dcache clr & inv*/
	asm volatile(
			"movi r7, 0x32;"
			"sync;"
			"mtcr r7, cr17;"
			:
			:
			:"r7","memory");
}

void irq_enable(void)
{
	/* enable irq */
	asm volatile (
			"mfcr r7, cr0\n"
			"bseti r7, 6\n"
			"mtcr r7, cr0\n"
			:
			:
			:
			);
}

void fiq_enable(void)
{
	/* enable fiq */
	asm volatile (
			"mfcr r7, cr0\n"
			"bseti r7, 4\n"
			"mtcr r7, cr0\n"
			:
			:
			:
			);
}

void irq_disable(void)
{
	/* disable irq */
	asm volatile (
			"mfcr r7, cr0\n"
			"bclri r7, 6\n"
			"mtcr r7, cr0\n"
			:
			:
			:
			);
}

void fiq_disable(void)
{
	/* disable fiq */
	asm volatile (
			"mfcr r7, cr0\n"
			"bclri r7, 4\n"
			"mtcr r7, cr0\n"
			:
			:
			:
			);
}
