#include "gx_avtick.h"
#include "../gxplayer_internal.h"

unsigned int av_get_tick(const char* prompt, unsigned int line)
{
	unsigned int ms;
	GxTime now_tick;
	GxCore_GetTickTime(&now_tick);
	ms = now_tick.seconds * 1000 + now_tick.microsecs / 1000;
	if (prompt) {
		gxlogd("\n\033[031m%s:%d\033[036m %u \033[0mms\n\n", prompt, line, ms);
	}

	return ms;
}

unsigned int av_get_tick_ms(const char* prompt, unsigned int line)
{
	unsigned int ms;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	ms = (unsigned int)(tv.tv_usec/1000 + tv.tv_sec * 1000);

	if (prompt) {
		gxlogd("\n\033[031m%s:%d\033[036m %u \033[0mms\n\n", prompt, line, ms);
	}

	return ms;
}
