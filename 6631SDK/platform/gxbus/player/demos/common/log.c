#include <stdio.h>
#include <stdarg.h>
#include "log.h"
#include "common/gxlog.h"

static LOG_MODULE_E sModule = 0x0;
static LOG_LEVEL_E  sLevel  = 0x0;

void Log_Printf(LOG_MODULE_E m, LOG_LEVEL_E l, const char* fmt, ...)
{
	va_list args;
	const char *m_string = NULL;
	const char *l_string  = NULL;

	switch (l & sLevel) {
	case LOG_INFO:
		l_string = "info";
		break;
	case LOG_WARNING:
		l_string = "warn";
		break;
	case LOG_ERROR:
		l_string = "erro";
		break;
	case LOG_DEBUG:
		l_string = "debg";
		break;
	default:
		return;
	}

	switch (m & sModule) {
	case LOG_GXPLAYER:
		m_string = "gxplayer";
		break;
	case LOG_MEDIAAPI:
		m_string = "mediaapi";
		break;
	case LOG_COMMON:
		m_string = "common";
		break;
	default:
		return;
	}

	gxlogd ("[%s]-[%s]: ", l_string, m_string);
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	return;
}

void Log_SetModule(LOG_MODULE_E m)
{
	sModule = m;

	return;
}

void Log_SetLevel(LOG_LEVEL_E  l)
{
	sLevel = l;

	return;
}

LOG_MODULE_E Log_GetModule(void)
{
	return sModule;
}

LOG_LEVEL_E Log_GetLevel(void)
{
	return sLevel;
}
