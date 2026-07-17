#include "reg_dw8051.h"
#include "gpio.h"
#include "parse_cmdline.h"

static unsigned char waketimeup = 0;
static unsigned int timer1_initial_value = 0;
static unsigned int system_run_time = 0; /*!< 单位为10ms */

void gx_timer_init(void)
{
	CKCON &= ~(1 << 4); /* CLK / 12 CLK = 27000000 or 24000000 */
	TMOD &= ~(3 << 6);
	TMOD |= 1 << 4; /* 16bit counter */
	//timer start value = 65536 - (CLK / 12 /100) + 12, (CLK / 12 / 100)为10ms超时, 加上12为进入中断到重新计时耗费的定时周期(cpu周期数/12)
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		timer1_initial_value = 45548;
	else
		timer1_initial_value = 43048;
	TH1 = (unsigned char)(timer1_initial_value >> 8);
	TL1 = (unsigned char)timer1_initial_value;

	IE |= 1 << 3; /* 打开定时器超时溢出中断 */

	TCON |= 1 << 6; /* enable timer 1*/
}

static void gx_waketime_check(void)
{
	static unsigned long time_count = 0;

	if (g_cmdline->waketime > 0)
	{
		if (++time_count == g_cmdline->waketime)
		{
			waketimeup = 1;
		}
	}
}

unsigned int get_system_run_time(void)
{
	return system_run_time;
}

static void system_time_add(void)
{
	system_run_time++;
}

extern unsigned char flush_flag;
static void gx_utc_time_add_every_sec(void)
{
        static unsigned int time_count = 0;
	if (++time_count == 100) //10*100 = 1s
	{
		local_time_add();
		time_count = 0;
	}
        if ((time_count & 15) == 0)//time_count % 16
        {
                flush_flag = 1;
        }
}

unsigned char get_waketimeup(void)
{
	return waketimeup;
}

void timer1_ISR(void) __interrupt 3
{
	TH1 = (unsigned char)(timer1_initial_value >> 8);
	TL1 = (unsigned char)timer1_initial_value;
	gx_waketime_check();
	gx_utc_time_add_every_sec();
	system_time_add();
}

