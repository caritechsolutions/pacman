#ifndef __LOG_H__
#define __LOG_H__

typedef enum {
	LOG_INFO    = (1 << 0),
	LOG_WARNING = (1 << 1),
	LOG_ERROR   = (1 << 2),
	LOG_DEBUG   = (1 << 3),
} LOG_LEVEL_E;

typedef enum {
	LOG_GXPLAYER = (1 <<  1),
	LOG_MEDIAAPI = (1 <<  2),
	LOG_COMMON   = (1 << 31),
} LOG_MODULE_E;

void Log_Printf   (LOG_MODULE_E m, LOG_LEVEL_E l, const char* fmt, ...);
void Log_SetModule(LOG_MODULE_E m);
void Log_SetLevel (LOG_LEVEL_E  l);

LOG_MODULE_E Log_GetModule(void);
LOG_LEVEL_E  Log_GetLevel (void);

#endif
