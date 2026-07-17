#ifndef __CLK_REG_H__
#define __CLK_REG_H__

struct sys_xv31regs {
	unsigned int   CLD_RST_NORM_REG;      /* 0x000 */
	unsigned int   CLD_RST_1SET_REG;      /* 0x004 */
	unsigned int   CLD_RST_1CLR_REG;      /* 0x008 */
	unsigned int   HOT_RST_NORM_REG;      /* 0x00c */
	unsigned int   HOT_RST_1SET_REG;      /* 0x010 */
	unsigned int   HOT_RST_1CLR_REG;      /* 0x014 */
	unsigned int   CLOCK_GATE_CONF_REG;   /* 0x018 */
	unsigned int   CLOCK_GATE_CONF_SET;   /* 0x01c */
	unsigned int   CLOCK_GATE_CONF_CLR;   /* 0x020 */
	unsigned int   CLOCK_DIV_CONFIG_REG;  /* 0x024 */
	unsigned int   DTO1_CONFIG_REG;       /* 0x028 */
	unsigned int   DTO2_CONFIG_REG;       /* 0x02c */
	unsigned int   CLOCK_DIV_CONFIG4_REG; /* 0x030 */
	unsigned int   CLOCK_DIV_CONFIG5_REG; /* 0x034 */
	unsigned int   CLOCK_DIV_CONFIG6_REG; /* 0x038 */
	unsigned int   CLOCK_DIV_CONFIG7_REG; /* 0x03c */
	unsigned int   CLOCK_DIV_CONFIG8_REG; /* 0x040 */
	unsigned int   RESV1[2];
	unsigned int   DTO10_CONFIG_REG;      /* 0x04c */
	unsigned int   DTO11_CONFIG_REG;      /* 0x050 */
	unsigned int   DTO12_CONFIG_REG;      /* 0x054 */
	unsigned int   DTO13_CONFIG_REG;      /* 0x058 */
	unsigned int   RESV2[1];
	unsigned int   DTO15_CONFIG_REG;      /* 0x060 */
	unsigned int   RESV3[1];
	unsigned int   CLOCK_GATE_CONF2_REG;  /* 0x068 */
	unsigned int   CLOCK_GATE_CONF2_SET;  /* 0x06c */
	unsigned int   CLOCK_GATE_CONF2_CLR;  /* 0x070 */
	unsigned int   RESV4[1];
	unsigned int   CLOCK_GATE_CONF3_REG;  /* 0x078 */
	unsigned int   CLOCK_GATE_CONF3_SET;  /* 0x07c */
	unsigned int   CLOCK_GATE_CONF3_CLR;  /* 0x080 */
	unsigned int   RESV5[1];
	unsigned int   CLOCK_GATE_CONF4_REG;  /* 0x088 */
	unsigned int   CLOCK_GATE_CONF4_SET;  /* 0x08c */
	unsigned int   CLOCK_GATE_CONF4_CLR;  /* 0x090 */
	unsigned int   RESV6[54];
	unsigned int   CLOCK_SOURCE_REG;      /* 0x16c */
	unsigned int   CLOCK_SOURCE2_REG;     /* 0x170 */
	unsigned int   CLOCK_SOURCE3_REG;     /* 0x174 */
	unsigned int   CLOCK_DIV_CONFIG2_REG; /* 0x178 */
	unsigned int   CLOCK_DIV_CONFIG3_REG; /* 0x17c */
	unsigned int   RESV7[8];
	unsigned int   AUDIO_CODEC_DATA_REG;  /* 0x1a0 */
	unsigned int   AUDIO_CODEC_CTRL_REG;  /* 0x1a4 */
	unsigned int   AUDIO_CODEC_CONF1_REG; /* 0x1a8 */
	unsigned int   AUDIO_CODEC_CONF2_REG; /* 0x1ac */
};

typedef struct {
	unsigned int vpu_up;
	unsigned int vpu;
	unsigned int hdmi;
	unsigned int dcs;
} XClkV31RegVoutClk;

typedef struct {
	unsigned int svpu_up;
	unsigned int svpu;
} XClkV31RegSvpuClk;

typedef struct {
	struct {
		unsigned int        sys;
	} phy;
	struct {
		struct sys_xv31regs *sys;
	} reg;
} XClkV31Reg;

typedef enum {
	XCLKV31REG_FROM_XTAL  = 0,
	XCLKV31REG_FROM_DTO   = 1,
	XCLKV31REG_FROM_DIV   = 2,
	XCLKV31REG_FROM_SPDIF = 3, //audio i2s
	XCLKV31REG_FROM_VPU   = 4, //video dac
	XCLKV31REG_FROM_SVPU  = 5, //video dac
} XClkV31RegSource;

extern void xclkv31reg_setup(XClkV31Reg *reg);

extern void xclkv31reg_apu_cold_rst (unsigned int enable);
extern void xclkv31reg_vpu_cold_rst (unsigned int enable);
extern void xclkv31reg_vdec_cold_rst(unsigned int enable);
extern void xclkv31reg_vpp_cold_rst (unsigned int enable);
extern void xclkv31reg_jpeg_cold_rst(unsigned int enable);
extern void xclkv31reg_hdmi_cold_rst(unsigned int enable);

extern void xclkv31reg_apu_hot_rst  (unsigned int enable);
extern void xclkv31reg_vpu_hot_rst  (unsigned int enable);
extern void xclkv31reg_jpeg_hot_rst (unsigned int enable);
extern void xclkv31reg_hdmi_hot_rst (unsigned int enable);

extern void xclkv31reg_vpp_clk_config  (unsigned int clk);
extern void xclkv31reg_adec_clk_config (unsigned int clk);
extern void xclkv31reg_adac_clk_config (unsigned int clk);
extern void xclkv31reg_i2s_clk_config  (unsigned int clk, unsigned int id);
extern void xclkv31reg_spdif_clk_config(unsigned int clk, unsigned int id);
extern void xclkv31reg_vout_clk_config (XClkV31RegVoutClk *clk);
extern void xclkv31reg_svpu_clk_config (XClkV31RegSvpuClk *clk);

extern void xclkv31reg_adec_clk_source (XClkV31RegSource src);
extern void xclkv31reg_i2s_clk_source  (XClkV31RegSource src);
extern void xclkv31reg_spdif_clk_source(XClkV31RegSource src);
extern void xclkv31reg_demux_clk_source(XClkV31RegSource src);
extern void xclkv31reg_stc_clk_source  (XClkV31RegSource src);
extern void xclkv31reg_vdac_clk_source (XClkV31RegSource src, int dac_id);
extern void xclkv31reg_vdec_clk_source (XClkV31RegSource src);

extern void xclkv31reg_adec_clk_enable (unsigned int enable);
extern void xclkv31reg_adac_clk_enable (unsigned int enable);
extern void xclkv31reg_i2s_clk_enable  (unsigned int enable);
extern void xclkv31reg_spdif_clk_enable(unsigned int enable);
extern void xclkv31reg_vga_clk_enable  (unsigned int enable);
extern void xclkv31reg_vpp_clk_enable  (unsigned int enable);
extern void xclkv31reg_jpeg_clk_enable (unsigned int enable);
extern void xclkv31reg_vdec_clk_enable (unsigned int enable);
extern void xclkv31reg_vdac_clk_enable (unsigned int enable);
extern void xclkv31reg_hdmi_clk_enable (unsigned int enable);
extern void xclkv31reg_vpu_clk_enable  (unsigned int enable);
extern void xclkv31reg_svpu_clk_enable (unsigned int enable);
extern void xclkv31reg_demux_clk_enable(unsigned int enable);
extern void xclkv31reg_pidfr_clk_enable(unsigned int enable);
extern void xclkv31reg_stc_clk_enable  (unsigned int enable);

extern void xclkv31reg_adac_mute_set(unsigned int enable);
extern void xclkv31reg_adac_mclk_set(unsigned int mclock);
extern void xclkv31reg_adac_power_config(unsigned int stepn, unsigned int skipn);
extern void xclkv31reg_adac_power_enable(unsigned int power, unsigned int enable);
extern void xclkv31reg_adac_volume_set(int voldb);
#endif
