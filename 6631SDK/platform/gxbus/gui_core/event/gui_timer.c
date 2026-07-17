#include "gui_private.h"
#include "hd_system.h"

static event_list* _get_timer_from_handle(int handle)
{
	GuiCore *gui_core = GUI_GetCurrent();
	event_list *pTimerCur = NULL;

	if((NULL == gui_core->timer_list) ||(0 == handle))
	{
		return (NULL);
	}

	pTimerCur = gui_core->timer_list;
	while(pTimerCur)
	{
		if((pTimerCur) && (pTimerCur->handle == handle))
			break;

		pTimerCur = pTimerCur->next;
	}

	return (pTimerCur);
}

static unsigned int timer_get_tick(void)
{
	return (hd_get_tick());
}

static event_list *_check_empty_timer(event_list* pTimerlist)
{
	event_list *pTimerCur = NULL;

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

event_list* create_timer(timer_event event, int time, void *data,  TIMER_FLAG flag)
{
	GuiCore *gui_core = GUI_GetCurrent();
	event_list *pTimer = NULL;

	if(NULL == event)
	{
		return (NULL);
	}

	timer_lock();

	pTimer = gui_core->timer_list;

	if(NULL == pTimer)
	{
		pTimer = (event_list *)GUI_MALLOCZ(sizeof(event_list));
		if(NULL == pTimer)
		{
			timer_unlock();
			return (NULL);
		}

		memset(pTimer, 0, sizeof(event_list));

		pTimer->delta_time = time;
		if(TIMER_REPEAT_NOW == flag)
			pTimer->trigger_time = timer_get_tick();
		else
			pTimer->trigger_time = timer_get_tick() + time;
		pTimer->event = event;
		pTimer->flag = flag;
		pTimer->data = data;
		pTimer->active = 1;
		pTimer->next = NULL;
		gui_core->timer_list = gui_core->last_timer = pTimer;
	}
	else
	{   pTimer = _check_empty_timer(gui_core->timer_list);
		if(NULL == pTimer)
		{
			pTimer = (event_list *)GUI_MALLOCZ(sizeof(event_list));
			if(NULL == pTimer)
			{
				timer_unlock();
				return (NULL);
			}

			pTimer->next = gui_core->timer_list;
			gui_core->timer_list = pTimer;
		}

		pTimer->delta_time = time;
		if(TIMER_REPEAT_NOW == flag)
			pTimer->trigger_time = timer_get_tick();
		else
			pTimer->trigger_time = timer_get_tick() + time;
		pTimer->event = event;
		pTimer->active = 1;
		pTimer->flag = flag;
		pTimer->data = data;
	}

	pTimer->handle = rand();
	timer_unlock();

	return ((event_list*)(pTimer->handle));
}

int reset_timer(event_list *pTimer)
{
	int timer_handle = 0;
	GuiCore *gui_core = GUI_GetCurrent();
	event_list *timer_cur = NULL;

	if(NULL == pTimer)
	{
		return (1);
	}

	timer_lock();
	timer_handle = (int)pTimer;
	pTimer = _get_timer_from_handle(timer_handle);
	if(NULL == pTimer)
	{
		timer_unlock();
		return (1);
	}
	timer_cur = gui_core->timer_list;

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
        pTimer->active = -1;
		timer_unlock();
		return (1);
	}

	pTimer->active = 1;
	if(TIMER_REPEAT_NOW == pTimer->flag)
		pTimer->trigger_time = timer_get_tick();
	else
		pTimer->trigger_time = timer_get_tick() + pTimer->delta_time;
	timer_unlock();
	return (0);
}

static int _check_timer(event_list *pTimer)
{
	GuiCore *gui_core = GUI_GetCurrent();
	event_list *pCur = NULL;

	if(NULL == pTimer)
	{
		return (1);
	}

	if(NULL == gui_core->timer_list)
	{
		return (1);
	}

	pCur = gui_core->timer_list;

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

int remove_timer(event_list *pTimer)
{
	int timer_handle = 0;

	timer_lock();
	if(NULL == pTimer)
	{
		timer_unlock();
		return (1);
	}
	timer_handle = (int)pTimer;
	pTimer = _get_timer_from_handle(timer_handle);
	if(NULL == pTimer)
	{
		timer_unlock();
		return (1);
	}

	if(_check_timer(pTimer))
	{
	    GUI_Debug_Print("Timer ID %p is not existed!\n", pTimer);
	    timer_unlock();
	    return (1);
	}

	pTimer->active = -1;
	pTimer->handle = 0;
	pTimer->event = pTimer->data = NULL;
	timer_unlock();

	return (0);
}

int remove_timer_list(void)
{
	GuiCore *gui_core = GUI_GetCurrent();
	event_list *pCur = NULL;
	event_list *pNext = NULL;

	pCur = gui_core->timer_list;

	while(pCur)
	{
		pNext = pCur->next;

		GUI_FREE(pCur);

		pCur = pNext;
	}
	gui_core->timer_list = NULL;
	return (0);
}

int timer_stop(event_list *pTimer)
{
	int timer_handle = 0;

	timer_lock();
	if(NULL == pTimer)
	{
		timer_unlock();
		return (1);
	}
	timer_handle = (int)pTimer;
	pTimer = _get_timer_from_handle(timer_handle);
	if(NULL == pTimer)
	{
		timer_unlock();
		return (1);
	}

	if(_check_timer(pTimer))
	{
	    GUI_Debug_Print("Timer ID %p is not existed!\n", pTimer);
	    timer_unlock();
	    return (1);
	}

	pTimer->active = 0;
	timer_unlock();
	return (0);
}

int timer_exec(void)
{
	int ret = 0x7FFFFFFF, offset = 0;
	unsigned int    now, start = 0, end = 0;
	GuiCore *gui_core = GUI_GetCurrent();
	event_list *pTimer = NULL;
	event_list *pNext = NULL;
	timer_event cb;

	timer_lock();
	pTimer = gui_core->timer_list;
	now = timer_get_tick();
	while(pTimer)
	{
		pNext = pTimer->next;

		if(now >= pTimer->trigger_time)
		{
			// TODO:
			/*Get some window data or widget data*/
			cb = pTimer->event;
			if(pTimer->event && (1 == pTimer->active))
			{
				if((TIMER_ONCE == pTimer->flag) &&
				   (pTimer->handle) &&
				   (pTimer->trigger_time <= timer_get_tick()))
				{
					pTimer->event = NULL;
					pTimer->active = -1;
					pTimer->handle = 0;
				}
				else if(((TIMER_REPEAT == pTimer->flag) || (TIMER_REPEAT_NOW == pTimer->flag)) &&
						(1 == pTimer->active))
				{
					pTimer->trigger_time = pTimer->delta_time + timer_get_tick();
				}
				else
				{
					;
				}

				timer_unlock();
				start = hd_get_tick();
				cb(pTimer->data);
				end = hd_get_tick();
				if((end - start) > 1000)
				{
					gxlogd("[GUI] ######## Timer %p excute too long ########\n", pTimer);
				}
				timer_lock();
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

void gui_delay(unsigned int ms)
{
	hd_delay(ms);
}

unsigned int gui_get_current_time(void)
{
	GxTime current_time = {0};

	GxCore_GetLocalTime(&current_time);

	return ((current_time.seconds * 1000000 + current_time.microsecs) / 1000);
}

