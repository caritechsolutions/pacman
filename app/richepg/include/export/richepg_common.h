#ifndef __RICHEPG_COMMON_H__
#define __RICHEPG_COMMON_H__
#include "gxtype.h"
#include "common/gxmalloc.h"
#include <time.h>

#ifdef MEMORY_DEBUG
#define _richepg_malloc     GxCore_Malloc
#define _richepg_calloc     GxCore_Calloc
#define _richepg_realloc    GxCore_Realloc
#define _richepg_strdup     GxCore_Strdup
#define _richepg_free       GxCore_Free
#else
#define _richepg_malloc     malloc
#define _richepg_calloc     calloc
#define _richepg_realloc    realloc
#define _richepg_strdup     strdup
#define _richepg_free       free
#endif

#define RICHEPG_FREE(x)     do { if(x) _richepg_free(x); x=NULL; } while(0)

#define RICHEPG_DBG_PRINT   0
#define RICHEPG_CAROUSEL_TIME_DEBUG 1
#define RICHEPG_ERR(fmt, args...)  do {                     \
    printf("[eERR][%s:%d]: ", __func__, __LINE__);          \
    printf(fmt, ##args);                                    \
} while(0)

#define RICHEPG_INFO(fmt, args...)  do {                    \
    printf("[eINFO][%s:%d]: ", __func__, __LINE__);         \
    printf(fmt, ##args);                                    \
} while(0)

#define RICHEPG_SINFO       printf

#if RICHEPG_DBG_PRINT
#define RICHEPG_DBG(fmt, args...)  do {                     \
    printf("[eDBG][%s:%d]: ", __func__, __LINE__);          \
    printf(fmt, ##args);                                    \
} while(0)
#else
#define RICHEPG_DBG(fmt, args...)  do { } while (0)
#endif

int richepg_copy_file_to_file(const char *src, const char *dst, int unit_size);
int richepg_copy_data_to_file(uint8_t *data, int size, const char *dst);
int richepg_read_file_to_data(const char *src, uint8_t **data, int *size);
int richepg_free_read_file_data(uint8_t **data, int *size);
time_t richepg_system_utc_time_get(void);

#endif
