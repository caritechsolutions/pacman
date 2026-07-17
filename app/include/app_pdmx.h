#ifndef __APP_PDMX_H__
#define __APP_PDMX_H__

void app_pdmx_register(void);
void app_pdmx_unregister(void);
void app_pdmx_play_stop(int tuner);
void app_pdmx_play_start(int tuner, int ts_source, int demux_id, int ts_size,  void *prog_params);
void app_pdmx_prepare(int tuner, void* prog_params);
void app_pdmx_start_ts_deal(int ts_src, int dmx_id, int ts_size);
void app_pdmx_stop(void);
int app_pdmx_get_active_flag(void);

#endif
