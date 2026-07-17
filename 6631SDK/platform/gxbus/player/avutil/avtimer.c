#include "gx_avtick.h"
#include "gx_avtimer.h"
#include "../gxplayer_internal.h"

typedef enum {
	AV_TIMER_READY = 0,
	AV_TIMER_RUN   = 1,
	AV_TIMER_PAUSE = 2,
} AV_TIMER_STATUS;

typedef struct {
	int            mutex;
	AV_TIMER_STATUS status;
	unsigned int   start_tickms;
	unsigned int   pause_tickms;
	unsigned int   blank_tickms;
} AV_TIMER;

void *avtimer_create(void)
{
	AV_TIMER *timer = av_mallocz(sizeof(AV_TIMER));

	GxCore_MutexCreate(&timer->mutex);

	GxCore_MutexLock(timer->mutex);
	timer->status = AV_TIMER_READY;
	timer->start_tickms = 0;
	timer->pause_tickms = 0;
	timer->blank_tickms = 0;
	GxCore_MutexUnlock(timer->mutex);

	return (void *)timer;
}

void avtimer_destroy(void *handle)
{
	AV_TIMER *timer = (AV_TIMER *)handle;

	timer->status = AV_TIMER_READY;
	GxCore_MutexDelete(timer->mutex);
	av_free(timer);
}

void avtimer_update(void *handle)
{
	AV_TIMER *timer = (AV_TIMER *)handle;

	GxCore_MutexLock(timer->mutex);
	timer->status = AV_TIMER_RUN;
	timer->pause_tickms = 0;
	timer->blank_tickms = 0;
	timer->start_tickms = av_get_tick(NULL, 0);
	GxCore_MutexUnlock(timer->mutex);
}

void avtimer_pause(void *handle)
{
	AV_TIMER *timer = (AV_TIMER *)handle;

	GxCore_MutexLock(timer->mutex);
	if (timer->status != AV_TIMER_PAUSE) {
		timer->status = AV_TIMER_PAUSE;
		timer->pause_tickms = av_get_tick(NULL, 0);
	}
	GxCore_MutexUnlock(timer->mutex);
}

void avtimer_resume(void *handle)
{
	AV_TIMER *timer = (AV_TIMER *)handle;

	GxCore_MutexLock(timer->mutex);
	if (timer->status != AV_TIMER_RUN) {
		timer->status = AV_TIMER_RUN;
		timer->blank_tickms += (av_get_tick(NULL, 0) - timer->pause_tickms);
		timer->pause_tickms  = 0;
	}
	GxCore_MutexUnlock(timer->mutex);
}

unsigned int avtimer_getms(void *handle)
{
	AV_TIMER *timer = (AV_TIMER *)handle;
	unsigned int tick_ms = 0;

	GxCore_MutexLock(timer->mutex);
	if (timer->status == AV_TIMER_PAUSE) {
		tick_ms = (timer->pause_tickms - timer->start_tickms - timer->blank_tickms);
	} else if (timer->status == AV_TIMER_RUN) {
		tick_ms = (av_get_tick(NULL, 0) - timer->start_tickms - timer->blank_tickms);
	}
	GxCore_MutexUnlock(timer->mutex);

	return tick_ms;
}
