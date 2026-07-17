#ifndef __VOUT_FEATURE_H__
#define __VOUT_FEATURE_H__

#include "gxav_vout_propertytypes.h"

void vout_install(unsigned int addr[2]);
int vout_hd_set_mode(unsigned int channel, GxVideoOutProperty_Mode mode);
int vout_rca_set_mode(unsigned int channel, GxVideoOutProperty_Mode mode);
int vout_scart_set_mode(unsigned int channel, GxVideoOutProperty_Mode mode);
int vout_set_dac_fullcase_change(unsigned int channel, unsigned int val);
int vout_set_dac_mode(unsigned int channel, unsigned int mode);
int vout_set_ext_coef_en(unsigned int channel, unsigned int en);
int vout_set_bt656_hpos(unsigned int channel, unsigned int pos);
int vout_set_bt656_vpos(unsigned int channel, unsigned int pos);
int vout_set_bt656_sync_low(unsigned int channel, unsigned int val);
int vout_set_bt656_topline_start(unsigned int channel, unsigned int line);
int vout_set_bt656_topline_end(unsigned int channel, unsigned int line);
int vout_get_tv_active(unsigned int channel, unsigned int *on, unsigned int *off);
int vout_set_tv_active(unsigned int channel, unsigned int on, unsigned int off);
int vout_set_mix_fir_para(unsigned int channel, unsigned int y_fir1, unsigned int c_fir1);
int vout_set_gamma_ctrl(unsigned int channel, unsigned int gamma0, unsigned int gamma1, unsigned int gamma2);
int vout_set_init_val(unsigned int channel, unsigned int val);

#endif /*__VOUT_FEATURE_H__*/

