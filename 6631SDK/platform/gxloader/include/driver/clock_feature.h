#ifndef __CLOCK_FEATURE_H__
#define __CLOCK_FEATURE_H__
#include "clock_hal.h"
#include "gxav_vout_propertytypes.h"

int clock_setup(void);
int clock_set_vpu_enable(unsigned int en);
int clock_set_svpu_enable(unsigned int en);
int clock_set_dac_src(unsigned int channel, enum video_dac_clk_source src);
int clock_set_hdmi_enable(unsigned int en);
int clock_set_vdac_enable(unsigned int en);
int clock_set_jpeg_enable(unsigned int en);
int vout_set_main_clock_freq(GxVideoOutProperty_Mode mode, unsigned int vpu_up, unsigned vpu, unsigned hdmi, unsigned dcs);
int vout_set_second_clock_freq(GxVideoOutProperty_Mode mode, unsigned int svpu_up, unsigned int svpu);
int vout_reset_clock(void);

#endif /*__CLOCK_FEATURE_H__*/

