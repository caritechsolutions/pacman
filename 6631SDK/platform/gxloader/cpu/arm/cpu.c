/*
 * CPU specific code
 */

void flush_dcache_all(void);
void invalidate_dcache_all(void);
void flush_icache_all(void);
void invalidate_icache_all(void);
void enable_dcache(void);
void enable_icache(void);
void disable_icache(void);
void disable_dcache(void);
void disable_mmu(void);
void enable_irq(void);
void enable_fiq(void);
void disable_irq(void);
void disable_fiq(void);

void icache_enable(void)
{
	/* turn on I-cache */
	enable_icache();
}

void dcache_enable(void)
{
	/* turn on D-cache */
	enable_dcache();
}

void icache_disable(void)
{
	/* turn off I-cache */
	disable_icache();
}

void dcache_disable(void)
{
	/* turn off D-cache */
	disable_dcache();
}

void icache_invalidate(void)
{
	/* inv I-cache */
	invalidate_icache_all();
}

void icache_flush(void)
{
	/* flush I-cache, icache clr & inv*/
	flush_icache_all();
}

void dcache_invalidate(void)
{
	/* inv D-cache */
	invalidate_dcache_all();
}

void dcache_flush(void)
{
	/* flush D-cache, dcache clr & inv*/
	flush_dcache_all();
}

void mmu_disable(void)
{
	/* turn off mmu */
	disable_mmu();
}

void irq_enable(void)
{
	/* turn on irq */
	enable_irq();
}

void fiq_enable(void)
{
	/* turn on fiq */
	enable_fiq();
}

void irq_disable(void)
{
	/* turn off irq */
	disable_irq();
}

void fiq_disable(void)
{
	/* turn off fiq */
	disable_fiq();
}

void switch_stack(unsigned int new_stack, unsigned int old_stack)
{
	unsigned int stack_pointer = 0;
	unsigned int offset = 0;

	if (new_stack == old_stack)
		return;
	asm volatile("mov %0, sp" : "=r" (stack_pointer));
	offset = old_stack - stack_pointer; //get offset
	stack_pointer = new_stack - offset;
	memcpy((void *)(new_stack - offset), (void *)(old_stack - offset), offset);
	asm volatile (
		"mov sp, %0\n"
		:
		: "r" (stack_pointer)
	);
}
