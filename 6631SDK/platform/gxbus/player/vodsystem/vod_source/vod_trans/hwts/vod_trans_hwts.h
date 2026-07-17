#ifndef __VOD_TRANS_HWTS_H__
#define __VOD_TRANS_HWTS_H__
#include "../../include/vod_in_common_def.h"

int32_t vod_trans_hwts_open(void);
void vod_trans_hwts_close(void);
uint8_t* vod_trans_hwts_malloc_buffer(int32_t len, uint32_t* token);
int32_t vod_trans_hwts_insert_buffer(uint32_t token, int32_t state);
int32_t vod_trans_hwts_sync(char* data, int len);
int32_t vod_trans_hwts_get_info(struct ts_stream_info* ts_info);
int32_t vod_trans_hwts_demux(uint32_t* handle);
#endif
