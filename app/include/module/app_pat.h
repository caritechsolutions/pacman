#ifndef __APP_PAT_H__
#define __APP_PAT_H__

#include "app_config.h"

#if PAT_SUPPORT
void app_pat_start(uint16_t demux_id,uint16_t ts_id,uint8_t version);
void app_pat_stop(void);
#endif
#endif

