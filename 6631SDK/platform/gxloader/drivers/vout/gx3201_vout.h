#ifndef __GX3201_VOUT_H_
#define __GX3201_VOUT_H_

void vpu_svpu_hdmi_init(void);
int gx_layer_zoom(int layer, int source_width, int source_height, int dest_width, int dest_height, int start_x, int start_y);
int gx3211_svpu_do_run(void);

#endif

