#ifndef __CPU_H__
#define __CPU_H__

void icache_enable(void);
void dcache_enable(void);
void icache_disable(void);
void dcache_disable(void);
void icache_invalidate(void);
void icache_flush(void);
void dcache_invalidate(void);
void dcache_flush(void);
void mmu_disable(void);
void irq_enable(void);
void fiq_enable(void);
void irq_disable(void);
void fiq_disable(void);
void switch_stack(unsigned int new_stack, unsigned int old_stack);
#endif

