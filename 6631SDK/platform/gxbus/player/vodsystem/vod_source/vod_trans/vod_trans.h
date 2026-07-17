#ifndef __VOD_TRANS_H__
#define __VOD_TRANS_H__

int32_t vod_trans_open(int32_t type);
void vod_trans_close(void);
int vod_trans_sync(char* buffer, int len);
uint8_t* vod_trans_malloc_buffer(int32_t len, uint32_t* token);
int32_t vod_trans_insert_buffer(uint32_t token, int32_t state);
int32_t vod_trans_get_frame(int32_t aud_or_vid, uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe);
void vod_trans_set_bufsize(int32_t size);
int32_t vod_trans_get_bufsize(void);
uint32_t vod_trans_get_v_packnum(void);
uint32_t vod_trans_get_a_packnum(void);
void vod_trans_clean(void);
int vod_trans_demux(uint32_t* handle);

#endif
