#include "time_interface.h"
#include "chip_info.h"
#include <div64.h>

extern struct gx_time_ops time_v1_ops;
extern struct gx_time_ops time_v2_ops;

unsigned long g_start_time = 0;
unsigned long g_end_time = 0;
struct gx_time_ops *time_ops;

u32 gx_time_init(void)
{
#ifdef CONFIG_ARCH_CKMMU_SCORPIO_TAURUS
	if (gxcore_chip_probe() == 0x3113)
		time_ops = &time_v2_ops;
	else
		time_ops = &time_v1_ops;
#else
#ifdef CONFIG_TIME_GX_V1
    time_ops = &time_v1_ops;
#else
    time_ops = &time_v2_ops;
#endif
#endif

    time_ops->init();

    return 0;
}

u64 gx_time_get_us(void)
{
	return time_ops->get_us();
}

u32 gx_time_get_ms(void)
{
	u64 time;

	time = time_ops->get_us();
	do_div(time, 1000);

	return (u32)time;
}

u32 gx_time_get_s(void)
{
	u64 time;
	time = gx_time_get_ms();
	do_div(time, 1000);

	return (u32)time;
}

void gx_time_delay_us(u32 delay)
{
	u64 crt;
	u64 init;

	init = time_ops->get_us();
	while (1) {
		crt = time_ops->get_us();
		if (crt > (init + delay))
			break;
	}
}

void udelay(unsigned int usec)
{
	gx_time_delay_us(usec);
}

void mdelay(unsigned int msec)
{
	while (msec-- > 0)
		gx_time_delay_us(998);
}

int timer_timeout(unsigned int startticks, int timeout)
{
	unsigned int ticks = gx_time_get_ms();

	if (timeout == 0)
		return 1;
	else if ((ticks - startticks) > timeout)
		return 1;

	return 0;
}

unsigned long get_timer(unsigned long base)
{
        return (gx_time_get_ms() - base);
}
