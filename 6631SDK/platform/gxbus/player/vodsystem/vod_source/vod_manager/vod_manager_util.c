#include "../include/vod_manager_main.h"
#include "../include/vod_porting_all.h"
#include "../include/vod_trans_api.h"

typedef struct vod_common_time_
{
	unsigned int interval;
	unsigned int begin;
	unsigned int used;

	unsigned int flag ;
}vod_common_time_t;

static vod_common_time_t  s_timer[VOD_MANAGER_TIMER_KIND_END];

static uint32_t vod_manager_nexttimer_index(uint32_t *current_index)
{
	uint32_t index = *current_index + 1;

	if (index >= VOD_MANAGER_TIMER_KIND_END)
	{
		index = VOD_MANAGER_TIMER_NONE + 1;
	}
	*current_index = index;
	return index;
}

void vod_manager_timer_init(void)
{
	memset(&s_timer, 0, sizeof(vod_common_time_t) * VOD_MANAGER_TIMER_KIND_END);
}

/* 定时器设置 */
int32_t vod_manager_timer_set(int interval , int timer , int used , int flag )
{
	vod_common_time_t * pTimer;

	if(timer <= VOD_MANAGER_TIMER_NONE || timer >= VOD_MANAGER_TIMER_KIND_END)
	{
		return ERRNO_VOD_PARAM;
	}

	pTimer = &(s_timer[timer]);

	pTimer->begin = vod_porting_get_ms();
	pTimer->interval = interval ;
	pTimer->used = used ;
	pTimer->flag = flag ;

	return ERRNO_VOD_NO_ERROR;
}

int32_t vod_manager_timer_tigger(void)
{
	unsigned int now = vod_porting_get_ms() ;
	static unsigned int last_index = 0 ;
	unsigned int next_index = last_index;
	vod_common_time_t * pTimer ;
	unsigned int index = vod_manager_nexttimer_index(&next_index);

	/* 确保每个定时器都能被检测到 */
	do
	{
		pTimer = &(s_timer[index]);
		if ( pTimer->used == VOD_MANAGER_TIMER_UNUSED )
			continue ;

		if ( (now - pTimer->begin) >= (pTimer->interval))
		{
			if ( pTimer->flag == VOD_MANAGER_TIMER_MOD_LOOP )
			{
				pTimer->begin = now;
			}
			else
			{
				pTimer->used = VOD_MANAGER_TIMER_UNUSED ;
			}

			/* 这个表明TIMER标志 */
			last_index = index;
			return index ;
		}
	}
	while (vod_manager_nexttimer_index(&index) != next_index);

	return VOD_MANAGER_TIMER_NONE;
}
