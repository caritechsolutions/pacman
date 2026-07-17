#include "clock.h"
#include "v31/clk_reg.h"

static XClkV31Reg v31_reg = {
#if defined(CONFIG_ARCH_CKMMU_SCORPIO) || defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	.phy = {
		.sys = (unsigned int)0x0030A000,
	},
	.reg = {
		.sys = (struct sys_xv31regs *)0xA030A000,
	}
#endif
};

void clock_init(void)
{
	xclkv31reg_setup(&v31_reg);
}

void clock_vout_clk_config(ClockVout *vout)
{
	XClkV31RegVoutClk v31_vout;

	v31_vout.vpu_up = vout->vpu_up;
	v31_vout.vpu    = vout->vpu;
	v31_vout.hdmi   = vout->hdmi;
	v31_vout.dcs    = vout->dcs;
	xclkv31reg_vout_clk_config(&v31_vout);
}

void clock_svpu_clk_config(ClockSvpu *svpu)
{
	XClkV31RegSvpuClk v31_svpu;

	v31_svpu.svpu_up = svpu->svpu_up;
	v31_svpu.svpu    = svpu->svpu;
	xclkv31reg_svpu_clk_config(&v31_svpu);
}

void clock_vout_clk_enable(unsigned int enable)
{
	xclkv31reg_vpu_clk_enable (enable);
	xclkv31reg_svpu_clk_enable(enable);
	xclkv31reg_hdmi_clk_enable(enable);
	xclkv31reg_vdac_clk_enable(enable);
	xclkv31reg_jpeg_clk_enable(enable);
}
