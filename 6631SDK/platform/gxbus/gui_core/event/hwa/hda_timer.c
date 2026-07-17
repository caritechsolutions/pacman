#include "gui_private.h"

unsigned int hd_get_tick(void)
{
	GxTime time_now = { 0 };
	unsigned int time_msec = 0;

	GxCore_GetTickTime(&time_now);
	time_msec = time_now.seconds * 1000 + time_now.microsecs / 1000;
	return (time_msec);
}

void hd_delay(unsigned int ms)
{
	if (ms != 0)
		GxCore_ThreadDelay(ms);
}

