#include "cygnus_vpu_vout.h"
#include "gxhwlib_registers.h"
#include "common/io.h"

#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

#define VPP2_BASE_ADDR                    (VPU_BASE_ADDR + 0x6000)
#define CE_BASE_ADDR                      (VPU_BASE_ADDR + 0x7000)

int vpu_init(void)
{
	REG_SET_FIELD(VPU_BASE_ADDR + 0x10, 0x1 << 0, 0, 0);
	REG_SET_BIT(VPU_BASE_ADDR + 0x48, 3);
	REG_SET_VAL(VPU_BASE_ADDR + 0x50, 0x412);
}

static unsigned ce_zoom_param_h[] = {
	/*[0x07100]*/ 0x750afb01,
	/*[0x07104]*/ 0x0000fb0a,
	/*[0x07108]*/ 0x7611f801,
	/*[0x0710c]*/ 0x0000fc04,
	/*[0x07110]*/ 0x7119f703,
	/*[0x07114]*/ 0x0000fffd,
	/*[0x07118]*/ 0x6f21f403,
	/*[0x0711c]*/ 0x000000f9,
	/*[0x07120]*/ 0x692af303,
	/*[0x07124]*/ 0x000001f6,
	/*[0x07128]*/ 0x6233f203,
	/*[0x0712c]*/ 0x000003f3,
	/*[0x07130]*/ 0x5e3bf004,
	/*[0x07134]*/ 0x000003f0,
	/*[0x07138]*/ 0x5645ef04,
	/*[0x0713c]*/ 0x000003ef,
	/*[0x07140]*/ 0x4d4def04,
	/*[0x07144]*/ 0x000004ef,
};

static unsigned ce_zoom_param_v[] = {
	/*[0x07200]*/ 0x6813f802,
	/*[0x07204]*/ 0x0000f813,
	/*[0x07208]*/ 0x671af702,
	/*[0x0720c]*/ 0x0000f90d,
	/*[0x07210]*/ 0x6620f602,
	/*[0x07214]*/ 0x0000fa08,
	/*[0x07218]*/ 0x6227f503,
	/*[0x0721c]*/ 0x0000fc03,
	/*[0x07220]*/ 0x5f2ef403,
	/*[0x07224]*/ 0x0000fdff,
	/*[0x07228]*/ 0x5b35f402,
	/*[0x0722c]*/ 0x0000fefc,
	/*[0x07230]*/ 0x553df402,
	/*[0x07234]*/ 0x0000fff9,
	/*[0x07238]*/ 0x5044f401,
	/*[0x0723c]*/ 0x000000f7,
	/*[0x07240]*/ 0x4a4af501,
	/*[0x07244]*/ 0x000001f5,
};

int ce_init(void)
{
	int i = 0;
	int val = 0;

	REG_SET_FIELD(CE_BASE_ADDR + 0x88, 0x7 << 12, 0, 12);
	REG_SET_FIELD(CE_BASE_ADDR + 0x70, 0x1 << 24, 0x1, 24);

	for (i = 0; i < sizeof(ce_zoom_param_h)/sizeof(ce_zoom_param_h[0]); i++) {
		REG_SET_FIELD(CE_BASE_ADDR + 0x100 + i * 4, 0xffffffff, ce_zoom_param_h[i], 0);
	}

	for (i = 0; i < sizeof(ce_zoom_param_v)/sizeof(ce_zoom_param_v[0]); i++) {
		REG_SET_FIELD(CE_BASE_ADDR + 0x200 + i * 4, 0xffffffff, ce_zoom_param_v[i], 0);
	}
}

int vpp2_enable(int en)
{
	REG_SET_FIELD(VPP2_BASE_ADDR + 0x30, 0x1 << 31, (en&0x1), 31);

	return 0;
}

int vpp2_zoom(int x, int y, int width, int height)
{
	REG_SET_FIELD(VPP2_BASE_ADDR + 0x38, 0x7ff << 16, y, 16);
	REG_SET_FIELD(VPP2_BASE_ADDR + 0x38, 0x7ff << 0, y + height - 1, 0);

	REG_SET_FIELD(VPP2_BASE_ADDR + 0x3c, 0x7FF << 16, x, 16);
	REG_SET_FIELD(VPP2_BASE_ADDR + 0x3c, 0x7ff << 0, x + width - 1, 0);

	return 0;
}

int vpp2_update(void)
{
	REG_SET_BIT(VPP2_BASE_ADDR + 0x28, 0);
	REG_CLR_BIT(VPP2_BASE_ADDR + 0x28, 0);

	return 0;
}

int ce_enable(int en)
{
	REG_SET_FIELD(CE_BASE_ADDR + 0x68, 0x1 << 0, en, 0);

	return 0;
}

int ce_set_format(GxColorFormat format)
{
	int val = 0;

	if (format == GX_COLOR_FMT_YCBCR422)
		val = 0;
	if (format == GX_COLOR_FMT_YCBCR422_Y_UV)
		val = 1;
	if (format == GX_COLOR_FMT_YCBCR420_Y_UV)
		val = 2;
	if (format == GX_COLOR_FMT_YCBCR444_Y_U_V)
		val = 3;

	REG_SET_FIELD(CE_BASE_ADDR + 0x70, 0x3 << 0, val, 0);
	return 0;
}

int ce_update(void)
{
	REG_SET_BIT(CE_BASE_ADDR + 0x90, 0);
	REG_CLR_BIT(CE_BASE_ADDR + 0x90, 0);
}

int ce_set_auto(void)
{
	REG_SET_FIELD(CE_BASE_ADDR + 0x70, 0x1 << 4, 0, 4);
	REG_SET_FIELD(CE_BASE_ADDR + 0x84, 0x1 << 31, 0, 31);
	REG_SET_BIT(CE_BASE_ADDR + 0x70, 24);
	REG_SET_FIELD(CE_BASE_ADDR + 0x8c, 0x3 << 30, 0, 30);
	return 0;
}

int ce_set_layer(int layer_sel)
{
	int val = 0;

	val = REG_GET_FIELD(CE_BASE_ADDR + 0x6c, 0x1f << 3, 3);
	val |= (1 << layer_sel);

	REG_SET_FIELD(CE_BASE_ADDR + 0x6c, 0x1f << 3, val, 3);
	REG_SET_FIELD(CE_BASE_ADDR + 0x6c, 0x3 << 16, 1, 16);

	return 0;
}

int ce_set_buf(unsigned buf, int width, int height, int color_format)
{
	int i;

	for(i = 0; i < 4; i++) {
		REG_SET_FIELD(CE_BASE_ADDR + i * 0x18, 0xffffffff, buf, 0);
		REG_SET_FIELD(CE_BASE_ADDR + i * 0x18 + 0x4, 0xffffffff, buf + width * 2, 0);
		REG_SET_FIELD(CE_BASE_ADDR + i * 0x18 + 0x8, 0xffffffff,  0, 0);
		REG_SET_FIELD(CE_BASE_ADDR + i * 0x18 + 0xc, 0xffffffff,  0, 0);
		REG_SET_FIELD(CE_BASE_ADDR + i * 0x18 + 0x10, 0xffffffff,  0, 0);
		REG_SET_FIELD(CE_BASE_ADDR + i * 0x18 + 0x14, 0xffffffff,  0, 0);
	}
	REG_SET_FIELD(CE_BASE_ADDR + 0x74, 0x7ff << 0, width, 0);
#if defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
	REG_SET_VAL(VPU_BASE_ADDR + 0x278, buf);
	REG_SET_VAL(VPU_BASE_ADDR + 0x27c, width * height * 2);
#endif /*CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA*/

	return 0;
}

int ce_bind_to_vpp2(int en)
{
	REG_SET_FIELD(VPU_BASE_ADDR + 0x10, 0x1 << 4, en, 4);
	return 0;
}

#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

