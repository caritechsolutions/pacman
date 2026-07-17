#ifndef __GXLOADER_LOG_H__
#define __GXLOADER_LOG_H__

#include <stdio.h>

#define GXLOG_MIN   0
#define GXLOG_ERROR 1
#define GXLOG_INFO  2
#define GXLOG_DEBUG 3
#define GXLOG_FLOW  4
#define GXLOG_MAX   GXLOG_FLOW

#ifndef GXLOG_LEVEL
#define GXLOG_LEVEL GXLOG_INFO
#endif

#define gxlog_printf_helper(level, level_ok, ...) printf(__VA_ARGS__)
#define gxlog_lprintf_helper(level, level_ok, ...) printf(__VA_ARGS__)

/* Formatted gxlog tagged with GXLOG_ERROR level */
#if (GXLOG_LEVEL < GXLOG_ERROR)
#define gxloge(...)   (void)0
#define gxloge_l(...) (void)0
#else
#define gxloge(...)   gxlog_printf_helper(GXLOG_ERROR, true, __VA_ARGS__)
#define gxloge_l(...) gxlog_lprintf_helper(GXLOG_ERROR, true, __VA_ARGS__)
#endif

/* Formatted gxlog tagged with GXLOG_INFO level */
#if (GXLOG_LEVEL < GXLOG_INFO)
#define gxlogi(...)   (void)0
#define gxlogi_l(...) (void)0
#else
#define gxlogi(...)   gxlog_printf_helper(GXLOG_INFO, true, __VA_ARGS__)
#define gxlogi_l(...) gxlog_lprintf_helper(GXLOG_INFO, true, __VA_ARGS__)
#endif

/* Formatted gxlog tagged with GXLOG_DEBUG level */
#if (GXLOG_LEVEL < GXLOG_DEBUG)
#define gxlogd(...)   (void)0
#define gxlogd_l(...) (void)0
#else
#define gxlogd(...)   gxlog_printf_helper(GXLOG_DEBUG, true, __VA_ARGS__)
#define gxlogd_l(...) gxlog_lprintf_helper(GXLOG_DEBUG, true, __VA_ARGS__)
#endif

/* Formatted gxlog tagged with GXLOG_FLOW level */
#if (GXLOG_LEVEL < GXLOG_FLOW)
#define gxlogf(...)   (void)0
#define gxlogf_l(...) (void)0
#else
#define gxlogf(...)   gxlog_printf_helper(GXLOG_FLOW, true, __VA_ARGS__)
#define gxlogf_l(...) gxlog_lprintf_helper(GXLOG_FLOW, true, __VA_ARGS__)
#endif

#endif
