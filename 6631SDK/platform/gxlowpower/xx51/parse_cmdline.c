#include "parse_cmdline.h"
#include <stdio.h>
#include <stdlib.h>

#define DPRAM_BASE_ADDR 0
#define TAG_MAGIC    0x54410003

__xdata struct gx_parse_cmdline *g_cmdline = (__xdata struct gx_parse_cmdline*)0x0;
__xdata void *g_extra_param = (__xdata void *)0x100;
unsigned long local_time_in_sec = 0;

char get_localtime_hour(void)
{
	char hour = 0;

	hour = (local_time_in_sec / 3600) % 24;

	return hour;
}

char get_localtime_min(void)
{
	char min = 0;

	min = (local_time_in_sec / 60) % 60;

	return min;
}

unsigned long get_localtime_in_sec(void)
{
	return local_time_in_sec;
}

static void local_time_init(void)
{
	if (g_cmdline->tag_header != TAG_MAGIC)
	{
		local_time_in_sec = 0;
	}
	else
	{
		if (g_cmdline->curtime == 0xFFFFFFFF)
			local_time_in_sec = g_cmdline->utctime + g_cmdline->timezone + g_cmdline->timesummer * 3600;
		else
			local_time_in_sec = g_cmdline->curtime;
	}
}

char get_clock_speed(void)
{
	if (g_cmdline->clock_speed == 24000000)
		return LOWPOWER_24MHZ_CLOCK;
	else
		return LOWPOWER_27MHZ_CLOCK;
}

unsigned long get_clock_frequency(void)
{
	return g_cmdline->clock_speed;
}

void local_time_add(void)
{
	local_time_in_sec++;
}

void time_write_back(void)
{
	if (g_cmdline->curtime == 0xFFFFFFFF)
		g_cmdline->utctime = local_time_in_sec - (g_cmdline->timezone + g_cmdline->timesummer * 3600);
	else
		g_cmdline->curtime = local_time_in_sec;
}

void parse_cmdline_init(void)
{
	g_cmdline->waketime = g_cmdline->waketime * 100; //外面传入为s为单位,由于定时器计时为10ms一次,乘以100转为10ms单位
	local_time_init();
}
