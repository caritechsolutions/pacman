#ifndef __VPU_FEATURE_H__
#define __VPU_FEATURE_H__
#include "gxav_common.h"
#include "gxav_vpu_propertytypes.h"

typedef struct {
	void (*setup)            (unsigned int base_addr);
	int  (*ioremap)          (void);
	void (*iounmap)          (void);

	int  (*sys_all_layer_en) (int en);
	int  (*sys_set_mixer)    (GxVpuMixerLayers *cfg);
	int  (*sys_set_dac)      (int dac_id, GxVpuMixerID src);
	int  (*sys_set_dac_en)   (int dac_id, unsigned int en);
	int  (*sys_set_dac_gain) (int dac_id, unsigned int val);

	int  (*ehc_set_gb_en)    (int g_en, int b_en);

	int  (*ce_init)          (void);
	int  (*ce_enable)        (int en);
	int  (*ce_set_format)    (GxColorFormat format);
	int  (*ce_set_mode)      (int mode);
	int  (*ce_set_layer)     (int sel);
	int  (*ce_set_buf_addr)  (unsigned int buf, unsigned int width, unsigned int height, GxColorFormat color_format);
	int  (*ce_set_bind_to_vpp2)  (int en);
	int  (*ce_set_cap_delay) (unsigned int val);

	int  (*vpp2_enable)      (int en);
	int  (*vpp2_zoom)        (GxAvRect *clip, GxAvRect *view);
	int  (*vpp2_set_zoom_para)(unsigned int vzoom, unsigned int hzoom);
	int  (*vpp2_set_zoom1_para)(unsigned int vzoom, unsigned int hzoom);
	int  (*vpp2_set_v_phase) (unsigned int vb, unsigned int vt);
	int  (*vpp2_set_down_scale)(unsigned int vdown_en, unsigned int hdown_en);
	int  (*vpp2_set_filter)  (unsigned int *data, unsigned int size);

	int  (*vpp_set_hfilter)  (unsigned int sign, unsigned int bypass, unsigned short *fir);

	int  (*pic_set_pos)      (unsigned int x, unsigned int y);
	int  (*pic_set_clip_size)(unsigned int w, unsigned int h);
	int  (*pic_set_view_size)(unsigned int w, unsigned int h);
	int  (*pic_set_zoom)     (unsigned int vzoom, unsigned int hzoom);
	int  (*pic_set_v_phase)  (unsigned int vb, unsigned int vt);
	int  (*pic_set_down_scale)(unsigned int vdown_en, unsigned int hdown_en);
	int  (*pic_set_filter_sign)(unsigned int val);
	int  (*pic_set_zoom_para)(unsigned int *data, unsigned int size);
	int  (*pic_set_hfilter_para)(unsigned int val);
	int  (*pic_set_addr)(unsigned int y_addr, unsigned int uv_addr, unsigned int bpp);
	int  (*pic_set_size)(unsigned int width, unsigned int height, unsigned int stride, GxColorFormat format);
	int  (*pic_set_enable)(int en);

	int  (*osd_set_pos)      (GxLayerID layer, unsigned int x, unsigned int y);
	int  (*osd_set_clip_size)(GxLayerID layer, unsigned int w, unsigned int h);
	int  (*osd_set_view_size)(GxLayerID layer, unsigned int w, unsigned int h);
	int  (*osd_set_zoom)     (GxLayerID layer, unsigned int vzoom, unsigned int hzoom);
	int  (*osd_set_vtop_phase)(GxLayerID layer, unsigned int val);
	int  (*osd_set_down_scale)(GxLayerID layer, unsigned int vdown_en, unsigned int hdown_en);
	int  (*osd_set_zoom_para)(GxLayerID layer, unsigned int *data, unsigned int size);
	int  (*osd_set_addr)(GxLayerID layer, unsigned int y_addr, unsigned int uv_addr, unsigned int bpp);
	int  (*osd_set_size)(GxLayerID layer, unsigned int width, unsigned int height, unsigned int stride, GxColorFormat format);
	int  (*osd_set_enable)(GxLayerID layer, int en);
} VpuFeatureOps;

int vpu_setup(void);
int vpu_install(unsigned int base_addr);

int vpu_uninstall(void);
int vpu_sys_all_layer_en(int en);
int vpu_sys_set_mixer(GxVpuMixerLayers *cfg);
int vpu_sys_set_dac(int dac_id, GxVpuMixerID src);
int vpu_sys_set_dac_en(int dac_id, unsigned int en);
int vpu_sys_set_dac_gain(int dac_id, unsigned int val);
int vpu_ehc_set_green_blue(int g_en, int b_en);
int vpu_ce_init(void);
int vpu_ce_enable(int en);
int vpu_ce_set_format(GxColorFormat format);
int vpu_ce_set_mode(int mode);
int vpu_ce_set_layer(int sel);
int vpu_ce_set_buf_addr(unsigned int buf, unsigned int width, unsigned int height, GxColorFormat color_format);
int vpu_ce_set_bind_to_vpp2(int en);
int vpu_ce_set_cap_delay(unsigned int val);    // TODO
int vpu_vpp2_enable(int en);
int vpu_vpp2_zoom(GxAvRect *clip, GxAvRect *view);
int vpu_vpp2_set_zoom_para(unsigned int vzoom, unsigned int hzoom);     // TODO
int vpu_vpp2_set_zoom1_para(unsigned int vzoom, unsigned int hzoom);    // TODO
int vpu_vpp2_set_v_phase(unsigned int vb, unsigned int vt);     // TODO
int vpu_vpp2_set_down_scale(unsigned int vdown_en, unsigned int hdown_en);   // TODO
int vpu_vpp2_set_filter(unsigned int *data, unsigned int size);   // TODO
int vpu_pic_set_pos(unsigned int x, unsigned int y);
int vpu_pic_set_clip_size(unsigned int w, unsigned int h);
int vpu_pic_set_view_size(unsigned int w, unsigned int h);
int vpu_pic_set_zoom(unsigned int vzoom, unsigned int hzoom);
int vpu_pic_set_v_phase(unsigned int vb, unsigned int vt);
int vpu_pic_set_down_scale(unsigned int vdown_en, unsigned int hdown_en);
int vpu_pic_set_filter_sign(unsigned int val);
int vpu_pic_set_zoom_para(unsigned int *data, unsigned int size);
int vpu_pic_set_hfilter_para(unsigned int val);
int vpu_pic_set_addr(unsigned int y_addr, unsigned int uv_addr, unsigned int bpp);
int vpu_pic_set_size(unsigned int width, unsigned int height, unsigned int stride, GxColorFormat format);
int vpu_pic_set_enable(int en);
int vpu_vpp_set_hfilter(unsigned int sign, unsigned int bypass, unsigned short *fir);

int vpu_osd_set_pos(GxLayerID layer, unsigned int x, unsigned int y);
int vpu_osd_set_clip_size(GxLayerID layer, unsigned int w, unsigned int h);
int vpu_osd_set_view_size(GxLayerID layer, unsigned int w, unsigned int h);
int vpu_osd_set_zoom(GxLayerID layer, unsigned int vzoom, unsigned int hzoom);
int vpu_osd_set_vtop_phase(GxLayerID layer, unsigned int val);
int vpu_osd_set_down_scale(GxLayerID layer, unsigned int vdown_en, unsigned int hdown_en);
int vpu_osd_set_zoom_para(GxLayerID layer, unsigned int *data, unsigned int size);
int vpu_osd_set_addr(GxLayerID layer, unsigned int y_addr, unsigned int uv_addr, unsigned int bpp);
int vpu_osd_set_size(GxLayerID layer, unsigned int width, unsigned int height, unsigned int stride, GxColorFormat format);
int vpu_osd_set_enable(GxLayerID layer, int en);

#endif /*__VPU_FEATURE_H__*/

