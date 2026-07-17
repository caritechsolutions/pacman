/* S-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2020, STMicroelectronics International N.V.
 */
#ifndef __APP_LOG_H__
#define __APP_LOG_H__

#include "gxtype.h"
#include "gxcore.h"
#include "app_config.h"

__BEGIN_DECLS

#define APP_LOG_MIN   0
#define APP_LOG_ERROR 1
#define APP_LOG_INFO  2
#define APP_LOG_DEBUG 3
#define APP_LOG_FLOW  4
#define APP_LOG_MAX   APP_LOG_FLOW

#define APP_MAX_PRINT_SIZE      256
#define APP_MAX_FUNC_PRINT_SIZE 32


extern void app_log_set_level(int level);
extern int app_log_get_level(void);


extern void app_log_printf(const char *function, int line, int level, bool level_ok, const char *fmt, ...);


#define app_log_printf_helper(level, level_ok, ...) \
	gxlog_printf(__func__, __LINE__, (level), (level_ok),  __VA_ARGS__)

#define app_log_printf_nohelp(level, level_ok, ...) \
	app_log_printf(__func__, __LINE__, (level), (level_ok),  __VA_ARGS__)

/* Formatted app_log tagged with level independent */
#if (APP_LOG_LEVEL <= APP_LOG_MIN)
#define app_log_min(...)   (void)0
#else
#define app_log_min(...)   printf(__VA_ARGS__)
#endif

/* Formatted app_log tagged with APP_LOG_ERROR level */
#if (APP_LOG_LEVEL < APP_LOG_ERROR)
#define app_log_error(...)   (void)0
#else
#define app_log_error(...)   app_log_printf_helper(APP_LOG_ERROR, true, __VA_ARGS__)
#endif

/* Formatted app_log tagged with APP_LOG_INFO level */
#if (APP_LOG_LEVEL < APP_LOG_INFO)
#define app_log_info(...)   (void)0
#else
#define app_log_info(...)   app_log_printf_helper(APP_LOG_INFO, true, __VA_ARGS__)
#endif

/* Formatted app_log tagged with APP_LOG_DEBUG level */
#if (APP_LOG_LEVEL < APP_LOG_DEBUG)
#define app_log_debug(...)   (void)0
#else
#define app_log_debug(...)   app_log_printf_helper(APP_LOG_DEBUG, true, __VA_ARGS__)
#endif

/* Formatted app_log tagged with APP_LOG_FLOW level */
#if (APP_LOG_LEVEL < APP_LOG_FLOW)
#define app_log_flow(...)   (void)0
#else
#define app_log_flow(...)   app_log_printf_helper(APP_LOG_FLOW, true, __VA_ARGS__)
#endif


__END_DECLS

#endif /* GXLOG_H */
