#ifndef __DVBSUB_DEC_H__
#define __DVBSUB_DEC_H__

extern handle_t dvbsub_init_decoder();
extern int dvbsub_decode        (handle_t handle, const uint8_t *buf, int buf_size);
extern int dvbsub_config_decoder(handle_t handle, int comp_page_id, int anci_page_id);
extern int dvbsub_close_decoder (handle_t handle);
extern int dvbsub_clean         (handle_t handle);
extern int dvbsub_display_init  (handle_t handle);
extern int dvbsub_show_timer    (handle_t handle);

#endif

