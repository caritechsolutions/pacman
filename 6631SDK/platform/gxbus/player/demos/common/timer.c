#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "common/gxmalloc.h"
#include "gx_mem.h"
#include "timer.h"
#include "log.h"

typedef struct {
	int time_s;
	int time_ms;
}TIMER_HANDLE_S;

#define TIMER_CHECK_NULL(handle) do {                                                   \
	if (!handle) {                                                                 \
		Log_Printf(LOG_COMMON, LOG_ERROR, "%s %d\n", __func__, __LINE__);     \
	}                                                                              \
} while(0)

void* Timer_Create(void)
{
	TIMER_HANDLE_S *time_handle;
	struct timeval time;

	gettimeofday(&time, NULL);

	time_handle = (TIMER_HANDLE_S *)av_malloc(sizeof(TIMER_HANDLE_S));
	time_handle->time_s = 0;
	time_handle->time_ms = 0;

	return time_handle;
}

void* Timer_Clear(void* handle)
{
	TIMER_HANDLE_S *time_handle = (TIMER_HANDLE_S *)handle;
	struct timeval time;

	TIMER_CHECK_NULL(time_handle);
	gettimeofday(&time, NULL);
	time_handle->time_s = time.tv_sec;
	time_handle->time_ms = time.tv_usec / 1000;

	return time_handle;
}

int Timer_Get_Ms(void* handle)
{
	TIMER_HANDLE_S *time_handle = (TIMER_HANDLE_S *)handle;
	int seconds = 0, mseconds = 0;
	struct timeval time;

	TIMER_CHECK_NULL(time_handle);
	gettimeofday(&time, NULL);
	seconds = time.tv_sec - time_handle->time_s;
	mseconds = (time.tv_usec / 1000) - time_handle->time_ms;

	return (seconds * 1000) + mseconds;
}

void Timer_Destroy(void* handle)
{
	TIMER_HANDLE_S *time_handle = (TIMER_HANDLE_S *)handle;

	TIMER_CHECK_NULL(time_handle);
	time_handle->time_s = 0;
	time_handle->time_ms = 0;
	av_free(time_handle);

	return;
}
