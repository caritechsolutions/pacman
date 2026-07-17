// B-License-Identifier: B-2-Clause
/*
 * Copyright (c) 2020, STMicroelectronics International N.V.
 */

#include <stdarg.h>
#include <string.h>
#include "module/app_log.h"

#if (APP_LOG_LEVEL < APP_LOG_MIN) || (APP_LOG_LEVEL > APP_LOG_MAX)
#error "Invalid value of APP_LOG_LEVEL"
#endif

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define BIT(nr)     (1 << (nr))

#define LOG_LONG_PREFIX_MASK (0x1a)

#if (APP_LOG_LEVEL >= APP_LOG_ERROR)

static int thiz_log_level = APP_LOG_FLOW;

static void app_log_ext_puts(const char *str)
{
	printf(str);
}

void app_log_set_level(int level)
{
	if (((int)level >= GXLOG_MIN) && (level <= GXLOG_MAX))
		thiz_log_level = level;
	else
		thiz_log_level = APP_LOG_MAX;
}

int app_log_get_level(void)
{
	return thiz_log_level;
}

static char app_log_level_to_string(int level, bool level_ok)
{
	/*
	 * U = Unused
	 * E = Error
	 * I = Information
	 * D = Debug
	 * F = Flow
	 */
	static const char lvl_strs[] = { 'U', 'E', 'I', 'D', 'F' };
	int l = 0;

	if (!level_ok)
		return 'M';

	if ((level >= APP_LOG_MIN) && (level <= APP_LOG_MAX))
		l = level;

	return lvl_strs[l];
}

void app_log_vprintf(const char *function, int line, int level, bool level_ok,
		   const char *fmt, va_list ap)
{
	char buf[MAX_PRINT_SIZE];
	size_t boffs = 0;
	int res;

	if (level_ok && level > thiz_log_level)
		return;

	/* Print the type of message */
	res = snprintf(buf, sizeof(buf), "%c/",
		       app_log_level_to_string(level, level_ok));
	if (res < 0)
		return;

	if((level_ok) && (level > 0))
	boffs += res;

	if (level_ok && (BIT(level) & LOG_LONG_PREFIX_MASK)) {
		if (function) {
			res = snprintf(buf + boffs, sizeof(buf) - boffs, "%s:%d ",
				       function, line);
			if (res < 0)
				return;
			boffs += res;
		}
	} else {
		/* Add space after location info */
		if (boffs >= sizeof(buf) - 1)
		    return;
		buf[boffs++] = ' ';
		buf[boffs] = 0;
	}

	res = vsnprintf(buf + boffs, sizeof(buf) - boffs, fmt, ap);
	if (res > 0)
		boffs += res;

	if (boffs >= (sizeof(buf) - 1))
		boffs = sizeof(buf) - 2;

	buf[boffs] = '\n';
	while (boffs && buf[boffs] == '\n')
		boffs--;
	boffs++;
	buf[boffs + 1] = '\0';

	app_log_ext_puts(buf);
}

/* Format gxlog of user ta. Inline with kernel ta */
void app_log_printf(const char *function, int line, int level, bool level_ok,
		  const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	app_log_vprintf(function, line, level, level_ok, fmt, ap);
	va_end(ap);
}


#else

/*
 * In case we have a zero or negative gxlog level when compiling optee_os, we
 * have to add stubs to gxlog functions in case they are used with TA having a
 * non-zero gxlog level
 */

void app_log_set_level(int level)
{
}

int app_log_get_level(void)
{
	return 0;
}

void app_log_printf(const char *function, int line, int level, bool level_ok, const char *fmt, ...)
{
}

#endif

