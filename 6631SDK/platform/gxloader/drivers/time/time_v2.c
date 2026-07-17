#include <stdio.h>
#include <config.h>
#include <io.h>
#include <types.h>
#include <interrupt.h>
#include <div64.h>
#include "time_interface.h"

#define    COUNTER_2_STATUS      (REG_BASE_COUNTER + 0x40)
#define    COUNTER_2_VALUE       (REG_BASE_COUNTER + 0x44)
#define    COUNTER_2_ACCSNAP     (REG_BASE_COUNTER + 0x48)
#define    COUNTER_2_CONTROL     (REG_BASE_COUNTER + 0x50)
#define    COUNTER_2_CONFIG      (REG_BASE_COUNTER + 0x60)
#define    COUNTER_2_PRE         (REG_BASE_COUNTER + 0x64)
#define    COUNTER_2_INI         (REG_BASE_COUNTER + 0x68)

static void counter_time_init(void)
{
	static int counter_inited = 0;

	if (counter_inited)
		return;

	counter_inited = 1;

	__raw_writel(0x1, COUNTER_2_CONTROL);
	__raw_writel(0x0, COUNTER_2_CONTROL);
	__raw_writel(0x1, COUNTER_2_CONFIG);
	__raw_writel(26, COUNTER_2_PRE); //1us 1/pre_clk
	__raw_writel(0, COUNTER_2_INI);
	__raw_writel(0x2, COUNTER_2_CONTROL); //begin count
}

u64 gx_time_v2_get_us(void)
{
	u32 counter_us_l = __raw_readl(COUNTER_2_VALUE);
	u64 counter_us_h = __raw_readl(COUNTER_2_ACCSNAP);

	return counter_us_l|(counter_us_h << 32);
}

u32 gx_time_v2_init(void)
{
	counter_time_init();
	return 0;
}

struct gx_time_ops time_v2_ops = {
	.init = gx_time_v2_init,
	.get_us = gx_time_v2_get_us,
};
