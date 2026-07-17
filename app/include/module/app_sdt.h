
#ifndef __APP_SDT_H__
#define __APP_SDT_H__

#include "app_config.h"

#if SDT_SUPPORT
void app_sdt_start(uint16_t demux_id,uint16_t ts_id,uint8_t version);
void app_sdt_stop(void);
#endif
#endif
