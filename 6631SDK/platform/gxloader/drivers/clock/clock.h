#ifndef __DRIVERS_CLOCK_H__
#define __DRIVERS_CLOCK_H__

typedef struct {
	unsigned int vpu_up;
	unsigned int vpu;
	unsigned int hdmi;
	unsigned int dcs;
} ClockVout;

typedef struct {
	unsigned int svpu_up;
	unsigned int svpu;
} ClockSvpu;

void clock_init(void);
void clock_vout_clk_config(ClockVout *vout);
void clock_svpu_clk_config(ClockSvpu *svpu);
void clock_vout_clk_enable(unsigned int enable);

#endif
