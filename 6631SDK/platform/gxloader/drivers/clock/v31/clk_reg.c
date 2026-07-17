#include "../common/reg_bits.h"
#include "clk_reg.h"

#define gxlog_e(...) ((void)0)

static XClkV31Reg *_v31reg = 0;
static unsigned short adac_clock_map[8][2] = {
	{128,  0xb8}, {192,  0xb0}, {256,  0xa8}, {384,  0xa0},
	{512,  0x98}, {768,  0x90}, {1024, 0x88}, {1536, 0x80},
};
static unsigned int adac_vol_index = 0x7f;
#define XCLKV31REG_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define XCLKV31REG_PLL (1188000000)

static unsigned int _get_dto_config(unsigned int clk)
{
	unsigned long long val = clk;

	val = val * (1 << 30);
	do_div(val, XCLKV31REG_PLL);

	return (unsigned int)val;
}

static unsigned int _get_div_config(unsigned int clk)
{
	return ((XCLKV31REG_PLL / clk) - 1);
}

void xclkv31reg_setup(XClkV31Reg *reg)
{
	_v31reg = reg;
}

void xclkv31reg_apu_cold_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1SET_REG), (0x1 << 20));
	else
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1CLR_REG), (0x1 << 20));
}

void xclkv31reg_vpu_cold_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1SET_REG), (0x1 << 8));
	else
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1CLR_REG), (0x1 << 8));
}

void xclkv31reg_vdec_cold_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1SET_REG), (0x1 << 1));
	else
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1CLR_REG), (0x1 << 1));
}

void xclkv31reg_vpp_cold_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1SET_REG), (0x1 << 3));
	else
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1CLR_REG), (0x1 << 3));
}

void xclkv31reg_jpeg_cold_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1SET_REG), (0x1 << 2));
	else
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1CLR_REG), (0x1 << 2));
}

void xclkv31reg_hdmi_cold_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1SET_REG), (0x1 << 7));
	else
		gxreg_set_val(&(_v31reg->reg.sys->CLD_RST_1CLR_REG), (0x1 << 7));
}

void xclkv31reg_apu_hot_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1SET_REG), (0x1 << 20));
	else
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1CLR_REG), (0x1 << 20));
}

void xclkv31reg_vpu_hot_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1SET_REG), (0x1 << 8));
	else
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1CLR_REG), (0x1 << 8));
}

void xclkv31reg_jpeg_hot_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1SET_REG), (0x1 << 2));
	else
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1CLR_REG), (0x1 << 2));
}

void xclkv31reg_hdmi_hot_rst(unsigned int enable)
{
	if (enable)
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1SET_REG), (0x1 << 7));
	else
		gxreg_set_val(&(_v31reg->reg.sys->HOT_RST_1CLR_REG), (0x1 << 7));
}

void xclkv31reg_vpp_clk_config(unsigned int clk)
{
	unsigned int val = 0;

	if (clk == 0)//复位时钟，保持配置
		val = gxreg_get_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG), 6, 16);
	else
		val = _get_div_config(clk);
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       0,  1, 14);//时钟源切换到晶振
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  0,  1, 23);//rst_en  复位有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  0,  1, 22);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  1,  1, 23);//rst_en  复位无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),val,  6, 16);//配置分频系数
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  1,  1, 22);//load_en 更新有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  1,  1, 22);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       1,  1, 14);//时钟源切换到DTO
}

void xclkv31reg_adec_clk_config(unsigned int clk)
{
	unsigned int val = _get_div_config(clk);

	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       0,  1, 15);//时钟源切换到晶振
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  0,  1, 31);//rst_en  复位有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  0,  1, 30);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  1,  1, 31);//rst_en  复位无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),val,  6, 24);//配置分频系数
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  1,  1, 30);//load_en 更新有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG4_REG),  0,  1, 30);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       1,  1, 15);//时钟源切换到DTO
}

void xclkv31reg_adac_clk_config(unsigned int clk)
{
	//audio lodac 时钟源
	unsigned int val = _get_div_config(clk);

	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       0,  1,  0);//时钟源切换到晶振
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1, 31);//rst_en  复位有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1, 30);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   1,  1, 31);//rst_en  复位无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG), val,  6, 24);//配置分频系数
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   1,  1, 30);//load_en 更新有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1, 30);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       1,  1,  0);//时钟源切换到DIV
}

void xclkv31reg_i2s_clk_config(unsigned int clk, unsigned int id)
{
	//audio srl 时钟源
	unsigned int val = _get_dto_config(clk);

	if (id == 0) {
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),  0,  1,  1);//时钟源切换到X
		gxreg_set_field(&(_v31reg->reg.sys->DTO1_CONFIG_REG),   0,  1, 31);//rst_en  复位有效
		gxreg_set_field(&(_v31reg->reg.sys->DTO1_CONFIG_REG),   0,  1, 30);//load_en 更新无效
		gxreg_set_field(&(_v31reg->reg.sys->DTO1_CONFIG_REG),   1,  1, 31);//rst_en  复位无效
		gxreg_set_field(&(_v31reg->reg.sys->DTO1_CONFIG_REG), val, 30,  0);//配置分频系数
		gxreg_set_field(&(_v31reg->reg.sys->DTO1_CONFIG_REG),   1,  1, 30);//load_en 更新有效
		gxreg_set_field(&(_v31reg->reg.sys->DTO1_CONFIG_REG),   0,  1, 30);//load_en 更新无效
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),  1,  1,  1);//时钟源切换到DTO
	} else if (id == 1) {
		gxlog_e(LOG_CLK, "%s %d\n", __func__, __LINE__);
	}
}

void xclkv31reg_spdif_clk_config(unsigned int clk, unsigned int id)
{
	//audio spd 时钟源
	unsigned int val = _get_dto_config(clk);

	if (id == 0) {
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),  0,  1,  2);//时钟源切换到晶振
		gxreg_set_field(&(_v31reg->reg.sys->DTO2_CONFIG_REG),   0,  1, 31);//rst_en  复位有效
		gxreg_set_field(&(_v31reg->reg.sys->DTO2_CONFIG_REG),   0,  1, 30);//load_en 更新无效
		gxreg_set_field(&(_v31reg->reg.sys->DTO2_CONFIG_REG),   1,  1, 31);//rst_en  复位无效
		gxreg_set_field(&(_v31reg->reg.sys->DTO2_CONFIG_REG), val, 30,  0);//配置分频系数
		gxreg_set_field(&(_v31reg->reg.sys->DTO2_CONFIG_REG),   1,  1, 30);//load_en 更新有效
		gxreg_set_field(&(_v31reg->reg.sys->DTO2_CONFIG_REG),   0,  1, 30);//load_en 更新无效
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),  1,  1,  2);//时钟源切换到DTO
	} else if (id == 1) {
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE3_REG),  0,  1,  0);//时钟源切换到晶振
		gxreg_set_field(&(_v31reg->reg.sys->DTO15_CONFIG_REG),   0,  1, 31);//rst_en  复位有效
		gxreg_set_field(&(_v31reg->reg.sys->DTO15_CONFIG_REG),   0,  1, 30);//load_en 更新无效
		gxreg_set_field(&(_v31reg->reg.sys->DTO15_CONFIG_REG),   1,  1, 31);//rst_en  复位无效
		gxreg_set_field(&(_v31reg->reg.sys->DTO15_CONFIG_REG), val, 30,  0);//配置分频系数
		gxreg_set_field(&(_v31reg->reg.sys->DTO15_CONFIG_REG),   1,  1, 30);//load_en 更新有效
		gxreg_set_field(&(_v31reg->reg.sys->DTO15_CONFIG_REG),   0,  1, 30);//load_en 更新无效
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE3_REG),  1,  1,  0);//时钟源切换到DTO
	}
}

void xclkv31reg_vout_clk_config(XClkV31RegVoutClk *clk)
{
	unsigned int div_clk = 0;
	unsigned int val = 0, gate_div = 0;

	div_clk = XCLKV31REG_MAX(div_clk, clk->vpu_up);
	div_clk = XCLKV31REG_MAX(div_clk, clk->vpu);
	div_clk = XCLKV31REG_MAX(div_clk, clk->hdmi);
	div_clk = XCLKV31REG_MAX(div_clk, clk->dcs);
	val = _get_div_config(div_clk);
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       0,  1,  5);//时钟源切换到晶振
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1,  6);//rst_en  复位有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1,  7);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   1,  1,  6);//rst_en  复位无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG), val,  6,  0);//配置分频系数
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   1,  1,  7);//load_en 更新有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1,  7);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       1,  1,  5);//时钟源切换到晶振

	gate_div = div_clk / clk->vpu;
	gate_div = (gate_div == 2) ? 1 : 0;
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG), gate_div,  1, 10);
	gate_div = div_clk / clk->dcs;
	gate_div = (gate_div == 2) ? 1 : 0;
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG), gate_div,  1,  9);
	gate_div = div_clk / clk->hdmi;
	gate_div = (gate_div == 2) ? 1 : 0;
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG), gate_div,  1,  8);

	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),        0,  1, 11);//rst_en gate复位有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),        1,  1, 11);//rst_en gate复位无效
}

void xclkv31reg_svpu_clk_config(XClkV31RegSvpuClk *clk)
{
	unsigned int div_clk = 0;
	unsigned int val = 0, gate_div = 0;

	div_clk = XCLKV31REG_MAX(div_clk, clk->svpu_up);
	val = _get_div_config(div_clk);
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       0,  1,  6);//时钟源切换到晶振
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1, 23);//rst_en  复位有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1, 22);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   1,  1, 23);//rst_en  复位无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG), val,  6, 16);//配置分频系数
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   1,  1, 22);//load_en 更新有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG_REG),   0,  1, 22);//load_en 更新无效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG),       1,  1,  6);//时钟源切换到PLL

	gate_div = ((div_clk / clk->svpu) == 2) ? 1 : 0;
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG3_REG), gate_div,  1,  0);
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG3_REG), 0,  1, 3);//rst_en gate复位有效
	gxreg_set_field(&(_v31reg->reg.sys->CLOCK_DIV_CONFIG3_REG), 1,  1, 3);//rst_en gate复位无效
}

void xclkv31reg_adec_clk_source(XClkV31RegSource src)
{
	switch (src) {
	case XCLKV31REG_FROM_XTAL:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 0, 1, 15);
		break;
	case XCLKV31REG_FROM_DTO:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 1, 1, 15);
		break;
	default:
		break;
	}
}

void xclkv31reg_i2s_clk_source(XClkV31RegSource src)
{
	switch (src) {
	case XCLKV31REG_FROM_XTAL:
	case XCLKV31REG_FROM_SPDIF:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 0, 1, 1);
		break;
	case XCLKV31REG_FROM_DTO:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 1, 1, 1);
		break;
	default:
		break;
	}
}

void xclkv31reg_spdif_clk_source(XClkV31RegSource src)
{
	switch (src) {
	case XCLKV31REG_FROM_XTAL:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 0, 1, 2);
		break;
	case XCLKV31REG_FROM_DTO:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 1, 1, 2);
		break;
	default:
		break;
	}
}

void xclkv31reg_demux_clk_source(XClkV31RegSource src)
{
	//Secure寄存器配置
	gxlog_e(LOG_CLK, "%s %d\n", __func__, __LINE__);
}

void xclkv31reg_stc_clk_source(XClkV31RegSource src)
{
	//Secure寄存器配置
	gxlog_e(LOG_CLK, "%s %d\n", __func__, __LINE__);
}

void xclkv31reg_vdac_clk_source(XClkV31RegSource src, int dac_id)
{
	switch (src) {
	case XCLKV31REG_FROM_VPU:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE2_REG), 0, 1, 6);
		break;
	case XCLKV31REG_FROM_SVPU:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE2_REG), 1, 1, 6);
		break;
	default:
		break;
	}
}

void xclkv31reg_vdec_clk_source(XClkV31RegSource src)
{
	switch (src) {
	case XCLKV31REG_FROM_XTAL:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 0, 1, 12);
		break;
	case XCLKV31REG_FROM_DTO:
		gxreg_set_field(&(_v31reg->reg.sys->CLOCK_SOURCE_REG), 1, 1, 12);
		break;
	default:
		break;
	}
}

void xclkv31reg_adec_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 11)); //module
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 11)); //pre
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 11));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 11));
	}
}

void xclkv31reg_adac_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 3));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 4));
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 3));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 4));
	}
}

void xclkv31reg_i2s_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 0)); //module
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 3)); //pre
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF4_CLR), (0x1 << 3)); //ahb
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_CLR), (0x1 << 3)); //ahb10
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_SET), (0x1 << 3));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 0));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 3));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF4_SET), (0x1 << 3));
	}
}

void xclkv31reg_spdif_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 1)); //spd0 module
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 1)); //spd0 pre
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 0)); //spd1 module
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 2)); //spd1 pre
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 1));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 1));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 0));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 2));
	}
}

void xclkv31reg_vga_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),   (0x1 << 12));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR),  (0x1 << 22));
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),   (0x1 << 12));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET),  (0x1 << 22));
	}
}

void xclkv31reg_vpp_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 10)); //pp module
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 10)); //pp pre
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_CLR), (0x1 <<  0)); //pp ahb
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 10));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 10));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_SET), (0x1 <<  0));
	}
}

void xclkv31reg_jpeg_clk_enable(unsigned int enable)
{
	int id = 0;

	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 9));//jpeg module
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 9));//jpeg pre
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF4_CLR), (0x1 << 0));//jpeg ahb
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 9));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 9));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF4_SET), (0x1 << 0));
	}
}

void xclkv31reg_vdec_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 8));//video decoder module
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR), (0x1 << 8));//video decoder pre
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_CLR), (0x1 << 5));//video decoder ahb
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 8));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET), (0x1 << 8));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_SET), (0x1 << 5));
	}
}

void xclkv31reg_vdac_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR), (0x1 << 30));
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET), (0x1 << 30));
	}
}

void xclkv31reg_hdmi_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 <<  5));//hdmi pixel
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 22));//hdmi cec
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),  (0x1 << 23));//hdmi sfr
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF4_CLR), (0x1 <<  4));//hdmi hdcp
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 <<  5));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 22));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),  (0x1 << 23));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF4_SET), (0x1 <<  4));
	}
}

void xclkv31reg_vpu_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),   (0x1 << 4));//vpu pixel
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_CLR),  (0x1 << 7));//vpu pixel double
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR),  (0x1 << 5));//vpu pre
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_CLR),  (0x1 << 2));//vpu ahb
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),   (0x1 << 4));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_SET),  (0x1 << 7));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET),  (0x1 << 5));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_SET),  (0x1 << 2));
	}
}

void xclkv31reg_svpu_clk_enable(unsigned int enable)
{
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),   (0x1 << 7));//vpu pixel
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),   (0x1 << 6));//vpu pixel double
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR),  (0x1 << 6));//vpu pre
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),   (0x1 << 7));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),   (0x1 << 6));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET),  (0x1 << 6));
	}
}

void xclkv31reg_demux_clk_enable(unsigned int enable)
{
	//demux sys
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),   (0x1 << 13));//demux sys
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR),  (0x1 << 13));//demux pre
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),   (0x1 << 13));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET),  (0x1 << 13));
	}
}

void xclkv31reg_pidfr_clk_enable(unsigned int enable)
{
	//pidfilter sys
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_CLR),  (0x1 << 9));//pidfilter sys
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF2_SET),  (0x1 << 9));
	}
}

void xclkv31reg_stc_clk_enable(unsigned int enable)
{
	//demux sys
	if (enable) {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_CLR),   (0x1 << 14));//stc sys
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_CLR),  (0x1 << 14));//stc pre
	} else {
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF_SET),   (0x1 << 14));
		gxreg_set_val(&(_v31reg->reg.sys->CLOCK_GATE_CONF3_SET),  (0x1 << 14));
	}
}

void xclkv31reg_adac_volume_set(int voldb)
{
	int begin_index = 0x7f;
	int begin_voldb = 6;
	int end_voldb = -36;

	if (voldb > begin_voldb)
		voldb = begin_voldb;

	if (voldb < end_voldb)
		voldb = end_voldb;

	adac_vol_index = begin_index - (begin_voldb - voldb);
}

void xclkv31reg_adac_mute_set(unsigned int enable)
{
	unsigned int value0 = 0;
	unsigned int value1 = enable ? 0xff : adac_vol_index;

	gxreg_set_val(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), ((0<<12)|(1<<3)|(1<<0)));
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);

	value0 = gxreg_get_val(&(_v31reg->reg.sys->AUDIO_CODEC_DATA_REG)) & 0xfffffffe;
	gxreg_set_val(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), ((0<<12)|((0x00|value0)<<4)|(1<<2)|(1<<0)));
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);

	gxreg_set_val(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), ((3<<12)|(value1<<4)|(1<<2)|(1<<0)));
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);

	gxreg_set_val(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), ((0<<12)|((0x01|value0)<<4)|(1<<2)|(1<<0)));
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);

	gxreg_set_val(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), ((3<<12)|(value1<<4)|(1<<2)|(1<<0)));
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
}

void xclkv31reg_adac_mclk_set(unsigned int mclock)
{
	unsigned int value = 0, i = 0;

	for (i = 0; i < 8; i++) {
		if (mclock == adac_clock_map[i][0]) {
			value = adac_clock_map[i][1];
			break;
		}
	}

	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 0);
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);

	gxreg_set_val(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), ((0<<12)|(value<<4)|(1<<2)|(1<<0)));
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);

	xclkv31reg_adac_mute_set(0);

	gxreg_set_val(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), ((2<<12)|(0x38<<4)|(1<<2)|(1<<0)));
	gxreg_set_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
	gxreg_clr_bit(&(_v31reg->reg.sys->AUDIO_CODEC_CTRL_REG), 1);
}

void xclkv31reg_adac_power_config(unsigned int stepn, unsigned int skipn)
{
	gxreg_set_field(&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), stepn, 4, 20);
	gxreg_set_val  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF2_REG), skipn);
}

void xclkv31reg_adac_power_enable(unsigned int power, unsigned int enable)
{
	if (power) {
		if (enable) {
			gxreg_set_field(&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 2, 2, 24);
			gxreg_set_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 19);
			gxreg_clr_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 27);
		} else {
			gxreg_set_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 31);
			gxreg_set_field(&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 0, 2, 24);
			gxreg_set_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 27);
		}
	} else {
		if (enable) {
			gxreg_set_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 29);
			gxreg_set_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 28);
			gxreg_set_field(&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 3, 2, 24);
			gxreg_clr_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 26);
			gxreg_set_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 31);
		} else {
			gxreg_clr_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 31);
			gxreg_set_field(&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 0, 2, 24);
			gxreg_set_bit  (&(_v31reg->reg.sys->AUDIO_CODEC_CONF1_REG), 26);
		}
	}
}
