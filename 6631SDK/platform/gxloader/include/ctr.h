#ifndef __CTR_H__
#define __CTR_H__

/* init ctr: every 1ms generate a interrupt */
int  gx_ctr_init(void);
/* disable ctr & disable interrupt*/
int  gx_ctr_disable(void);
/* register callback fun*/
int gx_ctr_callback_register(int (*fun)(void*), int interval_time_ms, int repeat_times, void *private_data);

#endif

