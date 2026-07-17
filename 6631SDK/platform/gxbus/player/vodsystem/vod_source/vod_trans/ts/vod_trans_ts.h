#ifndef __VOD_TRANS_TS_H__
#define __VOD_TRANS_TS_H__

int32_t vod_trans_ts_open(void);
void	vod_trans_ts_close(void);
uint8_t* vod_trans_ts_malloc_buffer(int32_t len, uint32_t* token);
void    vod_trans_ts_insert_buffer(uint32_t token, int32_t state);
int32_t vod_trans_ts_get_frame(int32_t aud_or_vid, uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe);
void vod_trans_ts_set_bufsize(int32_t size);
int32_t vod_trans_ts_get_bufsize(void);
uint32_t vod_trans_ts_get_v_packnum(void);
uint32_t vod_trans_ts_get_a_packnum(void);
void vod_trans_ts_clean(void);
int32_t vod_trans_ts_sync(char* data, int len);

#endif
