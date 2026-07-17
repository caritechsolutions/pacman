#ifndef __APP_PSI_PARSE_H__
#define __APP_PSI_PARSE_H__
#include "module/si/si_public.h"
#include "module/si/si_parser.h"
#include "module/si/si_public_parser.h"

#define SMALL_BUF_SIZE  (5 * 1024)  // pat,pmt...all small buf
#define SI_FREE(x)  if(x){GxCore_Free(x);x=NULL;}

enum DebugLevel
{
    SI_LOG_DBG = 0,
    SI_LOG_INFO,
    SI_LOG_ERR,
};

extern int32_t s_SiPrintLevel;
#define SI_ERR(fmt, args...)  do {                                 \
    printf("\033[1;31m");                                          \
    printf("[SI_ERR][%s:%d]: ", __func__, __LINE__);               \
    printf(fmt, ##args);                                           \
    printf("\033[0m\n");                                           \
} while(0)

#define SI_DBG(fmt, args...)  do {                                 \
    if (s_SiPrintLevel <= SI_LOG_DBG) {                            \
        printf("\033[33m");                                        \
        printf("[SI_DBG][%s:%d]: ", __func__, __LINE__);           \
        printf(fmt, ##args);                                       \
        printf("\033[0m\n");                                       \
    }                                                              \
} while(0)

#define SI_INFO(fmt, args...)  do {                                \
    if (s_SiPrintLevel <= SI_LOG_INFO) {                           \
        printf("\033[32m");                                        \
        printf("[SI_INFO][%s:%d]: ", __func__, __LINE__);          \
        printf(fmt, ##args);                                       \
        printf("\033[0m\n");                                       \
    }                                                              \
} while(0)

extern void *app_si_parse(private_table_cfg *table_cfg, uint8_t *section_data, uint32_t len);

#endif

