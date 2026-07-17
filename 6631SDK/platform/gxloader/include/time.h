#ifndef __TIME_H__
#define __TIME_H__
#include "sys/types.h"

extern unsigned long g_start_time;
/*
 * gx_time_init()
 *
 *  DESCRIPTION
 *      Initialize time module.
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *      On sucess, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
u32 gx_time_init(void);

/*
 * gx_time_get_us()
 *
 *  DESCRIPTION
 *      Get the current time, the unit is us.
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *      The current time.
 *
 *  NOTES
 *
 * */
u64 gx_time_get_us(void);

/*
 * gx_time_get_ms()
 *
 *  DESCRIPTION
 *      Get the current time, the unit is ms.
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *      The current time.
 *
 *  NOTES
 *
 * */
u32 gx_time_get_ms(void);

/*
 * gx_time_get_s()
 *
 *  DESCRIPTION
 *      Get the current time, the unit is second.
 *
 *  PARAMETER
 *
 *  RETURN VALUE
 *      The current time.
 *
 *  NOTES
 *
 * */
u32 gx_time_get_s(void);

/*
 * gx_time_delay_us()
 *
 *  DESCRIPTION
 *      Delay for a certain time.
 *
 *  PARAMETER
 *      @delay
 *           The time value to delay.
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void gx_time_delay_us(u32 delay);
void udelay(unsigned int usec);
void mdelay(unsigned int msec);
int timer_timeout(unsigned int startticks, int timeout);
unsigned long get_timer(unsigned long base);
#endif
