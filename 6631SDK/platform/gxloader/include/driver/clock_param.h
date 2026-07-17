#ifndef __CLOCK_PARAM_H__
#define __CLOCK_PARAM_H__

typedef enum {
	SOURCE_XTAL_CLK  = 0,
	SOURCE_DTO_CLK   = 1,
	SOURCE_PLL_CLK   = 2,
	SOURCE_SPDIF_CLK = 3,
	SOURCE_VPU_CLK   = 4,
	SOURCE_SVPU_CLK  = 5,
} ClockSource;

typedef struct {
	unsigned int clk_addr;
	unsigned int dto_addr;
} ClockRegRes;

typedef struct {
	unsigned int vpu_up;
	unsigned int vpu;
	unsigned int hdmi;
	unsigned int dcs;
} ClockRegVout;

typedef struct {
	unsigned int svpu_up;
	unsigned int svpu;
} ClockRegSvpu;

typedef struct {
	void (*setup)         (ClockRegRes *res);
	int  (*ioremap)       (void);
	void (*iounmap)       (void);

	void (* apu_cold_rst)(unsigned int enable);
	void (* vpu_cold_rst)(unsigned int enable);
	void (*vdec_cold_rst)(unsigned int enable);
	void (* vpp_cold_rst)(unsigned int enable);
	void (*jpeg_cold_rst)(unsigned int enable);
	void (*hdmi_cold_rst)(unsigned int enable);

	void (* apu_hot_rst) (unsigned int enable);
	void (* vpu_hot_rst) (unsigned int enable);
	void (*jpeg_hot_rst) (unsigned int enable);
	void (*hdmi_hot_rst) (unsigned int enable);

	void (*  vpp_clk_config)(unsigned int  clk);
	void (* adec_clk_config)(unsigned int  clk);
	void (* adac_clk_config)(unsigned int  clk);
	void (*  i2s_clk_config)(unsigned int  clk, unsigned int id);
	void (*spdif_clk_config)(unsigned int  clk, unsigned int id);
	void (* vout_clk_config)(ClockRegVout *clk);
	void (* svpu_clk_config)(ClockRegSvpu *clk);

	void (* adec_clk_source)(ClockSource src);
	void (*  i2s_clk_source)(ClockSource src);
	void (*spdif_clk_source)(ClockSource src);
	void (*demux_clk_source)(ClockSource src);
	void (*  stc_clk_source)(ClockSource src);
	void (* vdac_clk_source)(ClockSource src, int id);
	void (* vdec_clk_source)(ClockSource src);

	void (* adec_clk_enable)(unsigned int enable);
	void (* adac_clk_enable)(unsigned int enable);
	void (*  i2s_clk_enable)(unsigned int enable);
	void (*spdif_clk_enable)(unsigned int enable);
	void (*  vga_clk_enable)(unsigned int enable);
	void (*  vpp_clk_enable)(unsigned int enable);
	void (* jpeg_clk_enable)(unsigned int enable);
	void (* vdec_clk_enable)(unsigned int enable);
	void (* vdac_clk_enable)(unsigned int enable);
	void (* hdmi_clk_enable)(unsigned int enable);
	void (*  vpu_clk_enable)(unsigned int enable);
	void (* svpu_clk_enable)(unsigned int enable);
	void (*demux_clk_enable)(unsigned int enable);
	void (*pidfr_clk_enable)(unsigned int enable);
	void (*  stc_clk_enable)(unsigned int enable);

	void (* adac_mute_set)    (unsigned int enable);
	void (* adac_mclk_set)    (unsigned int mclock);
	void (* adac_power_config)(unsigned int stepn, unsigned int skipn);
	void (* adac_power_enable)(unsigned int power, unsigned int enable);
	void (* adac_volume_set)  (int voldb);
} ClockRegOps;

#endif /*__CLOCK_PARAM_H__*/

