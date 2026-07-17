#include "gxcore.h"
#include "gxaribcc.h"
#include "aribcc.h"

#if ARIBCC_SUPPORT
GX_LIST_HEAD(aribcc_timer_list);

static int	timer_running = 0;
static handle_t timer_lock = -1;
static handle_t timer_sem = -1;

#define TIMER_LOCK()                    GxCore_MutexLock(timer_lock)
#define TIMER_UNLOCK()                  GxCore_MutexUnlock(timer_lock)

struct timer_ext {
	struct gxlist_head    timer_list;
	unsigned int          timer_handle;
	unsigned int          timer_arg;
	int                   timer_ms;
	int                   timer_end;
	aribcc_timer_event    timer_func;
};

static unsigned int get_tick_time(void)
{
	GxTime time_now = { 0 };
	unsigned int time_msec = 0;

	GxCore_GetTickTime(&time_now);
	time_msec = time_now.seconds * 1000 + time_now.microsecs / 1000;
	return (time_msec);
}

static int mutex_timer(void)
{
	int ret = 0;

	if(timer_lock == -1) {
		ret = GxCore_MutexCreate(&timer_lock);
		if (ret != GXCORE_SUCCESS) {
			return GXCORE_ERROR;
		}
	}

	if(timer_sem == -1) {
		ret = GxCore_SemCreate(&timer_sem,0);
		if (ret != GXCORE_SUCCESS) {
			GxCore_MutexDelete(timer_lock);
			return GXCORE_ERROR;
		}
	}

	return GXCORE_SUCCESS;
}

static int aribcc_start(void)
{
	timer_running = 1;
	return 0;
}

static int aribcc_stop(void)
{
	timer_running = 0;
	GxCore_SemPost(timer_sem);
	return 0;
}

static handle_t aribcc_create_timer(aribcc_timer_event func,void *arg,int ms)
{
	struct timer_ext* timer_ext;

	mutex_timer();

	timer_ext = GxCore_Malloc(sizeof(struct timer_ext));
	if (timer_ext == NULL) {
		return GXCORE_ERROR;
	}
	memset(timer_ext, 0, sizeof(struct timer_ext));

	timer_ext->timer_handle = (unsigned int)timer_ext;
	timer_ext->timer_arg = (unsigned int)arg;
	timer_ext->timer_ms = ms;
	timer_ext->timer_end = get_tick_time() + ms;
	timer_ext->timer_func = func;

	if(gxlist_empty(&aribcc_timer_list))
		GxCore_SemPost(timer_sem);

	TIMER_LOCK();
	gxlist_add(&timer_ext->timer_list, &aribcc_timer_list);
	TIMER_UNLOCK();

	return (handle_t)timer_ext;
}

static int aribcc_remove_timer(handle_t handle)
{
	struct timer_ext* timer_ext = (struct timer_ext*)handle;

	if(timer_ext == NULL)
		return GXCORE_ERROR;

	TIMER_LOCK();
	gxlist_del(&timer_ext->timer_list);
	TIMER_UNLOCK();

	GxCore_Free(timer_ext);

	return GXCORE_SUCCESS;
}

static void aribcc_delay(unsigned int ms)
{
	GxCore_ThreadDelay(ms);
}

static int aribcc_timer_exec(void)
{
	struct gxlist_head* pos,*n;
	struct timer_ext* timer_ext;
	unsigned int now;

	mutex_timer();

	while (timer_running) {
		TIMER_LOCK();
		if (gxlist_empty(&aribcc_timer_list)) {
			TIMER_UNLOCK();
			GxCore_SemWait(timer_sem);
			continue;
		}
		now = get_tick_time();
		gxlist_for_each_safe(pos, n,&aribcc_timer_list) {
			timer_ext = gxlist_entry(pos, struct timer_ext, timer_list);
			if (timer_ext == NULL || timer_ext->timer_ms <= 0)
				continue;
			if(now < timer_ext->timer_end)
				continue;
			if (timer_ext->timer_func != NULL) {
				timer_ext->timer_func(timer_ext->timer_handle,timer_ext->timer_arg);
				timer_ext->timer_end = get_tick_time() + timer_ext->timer_ms;
			}
		}
		TIMER_UNLOCK();
		GxCore_ThreadDelay(50);
	}

	return 0;
}

GxAribCC_TimerOps aribcc_timer = {
	.start   = aribcc_start,
	.stop    = aribcc_stop,
	.create  = aribcc_create_timer,
	.remove  = aribcc_remove_timer,
	.delayms = aribcc_delay,
	.exec    = aribcc_timer_exec,
};
#endif

