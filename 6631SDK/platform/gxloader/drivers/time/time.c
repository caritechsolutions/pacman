#include <stdio.h>
#include <config.h>
#include <io.h>
#include <types.h>
#include <interrupt.h>
#include <div64.h>
#include "time_interface.h"

#define    COUNTER_2_STATUS      (REG_BASE_COUNTER + 0x40)
#define    COUNTER_2_VALUE       (REG_BASE_COUNTER + 0x44)
#define    COUNTER_2_CONTROL     (REG_BASE_COUNTER + 0x50)
#define    COUNTER_2_CONFIG      (REG_BASE_COUNTER + 0x60)
#define    COUNTER_2_PRE         (REG_BASE_COUNTER + 0x64)
#define    COUNTER_2_INI         (REG_BASE_COUNTER + 0x68)

#define    COUNTER_3_STATUS      (REG_BASE_COUNTER + 0x80)
#define    COUNTER_3_VALUE       (REG_BASE_COUNTER + 0x84)
#define    COUNTER_3_CONTROL     (REG_BASE_COUNTER + 0x90)
#define    COUNTER_3_CONFIG      (REG_BASE_COUNTER + 0xa0)
#define    COUNTER_3_PRE         (REG_BASE_COUNTER + 0xa4)
#define    COUNTER_3_INI         (REG_BASE_COUNTER + 0xa8)

#ifdef CONFIG_ARCH_ARM_SIRIUS
#define COUNTER2_VECTOR 51
#define COUNTER3_VECTOR 52
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
#define COUNTER2_VECTOR 59
#define COUNTER3_VECTOR 58
#else
#define COUNTER2_VECTOR 11
#define COUNTER3_VECTOR 12
#endif

#define COUNTER2_OVERFLOW_TIME 0xd693a400  //1h (60*60*1000*1000)
#define COUNTER3_OVERFLOW_TIME 0xd64758c0  //59m55s (59*60*1000*1000 + 55*1000*1000)

static volatile u32 g_counter2_overflow;
static volatile u32 g_counter3_overflow;

#ifdef CONFIG_ENABLE_IRQ
static enum interrupt_return ctr2_interrupt_isr(unsigned int irq, void *pdata)
{
	u32 value;
	value = __raw_readl(COUNTER_2_STATUS);
	__raw_writel(value | (0x1<<1), COUNTER_2_STATUS);
	g_counter2_overflow++;

	return HANDLED;
}

static enum interrupt_return ctr3_interrupt_isr(unsigned int irq, void *pdata)
{
	u32 value;
	value = __raw_readl(COUNTER_3_STATUS);
	__raw_writel(value | (0x1<<1), COUNTER_3_STATUS);
	g_counter3_overflow++;

	return HANDLED;
}
#endif

static u32 get_counter2_offset(void)
{
	return (__raw_readl(COUNTER_2_VALUE) - (0 - COUNTER2_OVERFLOW_TIME));
}

static u32 get_counter3_offset(void)
{
	return (__raw_readl(COUNTER_3_VALUE) - (0 - COUNTER3_OVERFLOW_TIME));
}

static void counter_time_init(void)
{
	static int counter_inited = 0;

	if (counter_inited)
		return;

	counter_inited = 1;
	g_counter2_overflow = 0;
	g_counter3_overflow = 0;

	__raw_writel(0x1, COUNTER_2_CONTROL);
	__raw_writel(0x0, COUNTER_2_CONTROL);
	__raw_writel(26, COUNTER_2_PRE); //1us 1/pre_clk
	__raw_writel(0 - COUNTER2_OVERFLOW_TIME, COUNTER_2_INI);

	__raw_writel(0x1, COUNTER_3_CONTROL);
	__raw_writel(0x0, COUNTER_3_CONTROL);
	__raw_writel(26, COUNTER_3_PRE); //1us 1/pre_clk
	__raw_writel(0 - COUNTER3_OVERFLOW_TIME, COUNTER_3_INI);


#ifdef CONFIG_ENABLE_IRQ
	__raw_writel(0x3, COUNTER_2_CONFIG);
	__raw_writel(0x3, COUNTER_3_CONFIG);
	gx_request_interrupt(COUNTER2_VECTOR, IRQ, ctr2_interrupt_isr, NULL);
	gx_request_interrupt(COUNTER3_VECTOR, IRQ, ctr3_interrupt_isr, NULL);
#else
	__raw_writel(0x1, COUNTER_2_CONFIG);
	__raw_writel(0x1, COUNTER_3_CONFIG);
#endif
	__raw_writel(0x2, COUNTER_2_CONTROL); //begin count
	__raw_writel(0x2, COUNTER_3_CONTROL); //begin count
}

u64 gx_time_v1_get_us(void)
{
	u32 counter2_overflow;
	u32 counter2_offset;
	u32 counter3_overflow;
	u32 counter3_offset;
	u64 counter2_us;
	u64 counter3_us;
	u64 old_counter_us;
	u64 new_counter_us;

	counter2_overflow = g_counter2_overflow;
	counter2_offset = get_counter2_offset();
	counter3_overflow = g_counter3_overflow;
	counter3_offset = get_counter3_offset();
	counter2_us = (u64)counter2_overflow * COUNTER2_OVERFLOW_TIME + counter2_offset;
	counter3_us = (u64)counter3_overflow * COUNTER3_OVERFLOW_TIME + counter3_offset;
	old_counter_us = counter2_us > counter3_us ? counter2_us : counter3_us;

	counter2_overflow = g_counter2_overflow;
	counter2_offset = get_counter2_offset();
	counter3_overflow = g_counter3_overflow;
	counter3_offset = get_counter3_offset();
	counter2_us = (u64)counter2_overflow * COUNTER2_OVERFLOW_TIME + counter2_offset;
	counter3_us = (u64)counter3_overflow * COUNTER3_OVERFLOW_TIME + counter3_offset;
	new_counter_us = counter2_us > counter3_us ? counter2_us : counter3_us;

	return (new_counter_us > old_counter_us ? new_counter_us : old_counter_us);
}

u32 gx_time_v1_init(void)
{
	counter_time_init();
	return 0;
}

struct gx_time_ops time_v1_ops = {
	.init = gx_time_v1_init,
	.get_us = gx_time_v1_get_us,
};
