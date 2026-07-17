#include "sub_timer.h"
#include "gx_mem.h"

#define SUBT_FREE(p) if(NULL != p) {av_free(p); p = NULL; }

subt_event_list *subt_timer_list = NULL;
subt_event_list *subt_last_timer = NULL;



static unsigned int _hd_get_tick(void)
{
    GxTime time_now = { 0 };
    unsigned int time_msec = 0;

    GxCore_GetTickTime(&time_now);
    time_msec = time_now.seconds * 1000 + time_now.microsecs / 1000;
    return (time_msec);
}
 
static void _hd_delay(unsigned int ms)
{
    GxCore_ThreadDelay(ms);
}


static unsigned int timer_get_tick(void)
{
	return (_hd_get_tick());
}

static subt_event_list *_check_empty_timer(subt_event_list* pTimerlist)
{
	subt_event_list *pTimerCur = NULL;

	if(NULL == pTimerlist)
	{
		return (pTimerCur);
	}

	pTimerCur = pTimerlist;
	while(pTimerCur)
	{
		if(-1 == pTimerCur->active)
		{
			break;
		}

		pTimerCur = pTimerCur->next;
	}

	return (pTimerCur);
}

static handle_t timer_mutex = -1;

static int timer_lock(void)
{
	int ret = 0;

	if(-1 == timer_mutex)
	{
		ret = GxCore_MutexCreate(&timer_mutex);
	}

	ret = GxCore_MutexLock(timer_mutex);

	return (ret);
}

static int timer_unlock(void)
{
	int ret = 0;

	ret = GxCore_MutexUnlock(timer_mutex);

	return (ret);
}

static subt_event_list* subt_create_timer(subt_timer_event event, int time, void *data,  SUBT_TIMER_FLAG flag)
{
	subt_event_list *pTimer = NULL;

	if(NULL == event)
	{
		return (NULL);
	}

	timer_lock();

	pTimer = subt_timer_list;

	if(NULL == pTimer)
	{
		pTimer = (subt_event_list *)av_malloc(sizeof(subt_event_list));
		if(NULL == pTimer)
		{
			timer_unlock();
			return (NULL);
		}

		memset(pTimer, 0, sizeof(subt_event_list));

		pTimer->delta_time = time;
		pTimer->trigger_time = timer_get_tick() + time;
		pTimer->event = event;
		pTimer->flag = flag;
		pTimer->data = data;
		pTimer->active = 1;
		pTimer->next = NULL;
		subt_timer_list = subt_last_timer = pTimer;
	}
	else
	{   pTimer = _check_empty_timer(subt_timer_list);
		if(NULL == pTimer)
		{
			pTimer = (subt_event_list *)av_malloc(sizeof(subt_event_list));
			if(NULL == pTimer)
			{
				timer_unlock();
				return (NULL);
			}

			pTimer->next = subt_timer_list;
			subt_timer_list = pTimer;
		}

		pTimer->delta_time = time;
		pTimer->trigger_time = timer_get_tick() + time;
		pTimer->event = event;
		pTimer->active = 1;
		pTimer->flag = flag;
		pTimer->data = data;
	}

	timer_unlock();

	return (pTimer);
}

static int subt_reset_timer(subt_event_list *pTimer)
{
	subt_event_list *timer_cur = NULL;

	if(NULL == pTimer)
	{
		return (1);
	}
	timer_lock();
	timer_cur = subt_timer_list;

	while(timer_cur)
	{
		if(pTimer == timer_cur)
		{
			break;
		}
		timer_cur = timer_cur->next;
	}

	if(NULL == timer_cur)
	{
		timer_unlock();
		return (1);
	}

	pTimer->active = 1;
	pTimer->trigger_time = timer_get_tick() + pTimer->delta_time;
	timer_unlock();
	return (0);
}

static int _check_timer(subt_event_list *pTimer)
{
	subt_event_list *pCur = NULL;

	if(NULL == pTimer)
	{
		return (1);
	}

	if(NULL == subt_timer_list)
	{
		return (1);
	}

	pCur = subt_timer_list;

	while(pCur)
	{
		if(pCur == pTimer)
		{
			break;
		}

		pCur = pCur->next;
	}

	if(pCur)
	{
		return (0);
	}
	else
	{
		return (1);
	}
}

static int subt_remove_timer(subt_event_list *pTimer)
{
	timer_lock();
	if(NULL == pTimer)
	{
		timer_unlock();
		return (1);
	}

	if(_check_timer(pTimer))
	{
		timer_unlock();
		return (1);
	}

	pTimer->active = -1;
	timer_unlock();

	return (0);
}

static int subt_remove_timer_list(void)
{
	subt_event_list *pCur = NULL;
	subt_event_list *pNext = NULL;

	pCur = subt_timer_list;

	while(pCur)
	{
		pNext = pCur->next;

		SUBT_FREE(pCur);

		pCur = pNext;
	}
	subt_timer_list = NULL;
	return (0);
}

static int subt_timer_stop(subt_event_list *pTimer)
{
	timer_lock();
	if(NULL == pTimer)
	{
		timer_unlock();
		return (1);
	}

	if(_check_timer(pTimer))
	{
		timer_unlock();
		return (1);
	}

	pTimer->active = 0;
	timer_unlock();
	return (0);
}

static int subt_timer_exec(void)
{
	int ret = 0x7FFFFFFF, offset = 0;
	unsigned int    now;
	subt_event_list *pTimer = NULL;
	subt_event_list *pNext = NULL;

	timer_lock();
	pTimer = subt_timer_list;
	now = timer_get_tick();
	while(pTimer)
	{
		pNext = pTimer->next;

		if(now >= pTimer->trigger_time)
		{
			// TODO:
			/*Get some window data or widget data*/
			if(pTimer->event && (1 == pTimer->active))
			{
				timer_unlock();
				pTimer->event(pTimer->data);
				timer_lock();

				if((SUBT_TIMER_ONCE == pTimer->flag) && (pTimer->trigger_time <= timer_get_tick()))
				{

					pTimer->active = -1;
				}
				else if((SUBT_TIMER_REPEAT == pTimer->flag) && (1 == pTimer->active))
				{
					pTimer->trigger_time = pTimer->delta_time + timer_get_tick();
				}
				else
				{
					;
				}
			}
		}
		else if(1 == pTimer->active)
		{
			offset = pTimer->trigger_time - timer_get_tick();
			if((offset > 0) && (ret > offset))
			{
				ret = offset;
			}
		}

		pTimer = pNext;
	}
	timer_unlock();
	return (ret);
}

static void subt_delay(unsigned int ms)
{
	_hd_delay(ms);
}



GxSubtitle_TimerOps gxsub_timer = {
                                      subt_create_timer,
                                      subt_reset_timer,
                                      subt_timer_stop,
                                      subt_remove_timer,
                                      subt_remove_timer_list,
                                      subt_delay,
                                      subt_timer_exec,
                                  };

