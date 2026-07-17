#include "permit.h"

static uint32_t reg_array[] = {
	GX_BASE_AHB0_SEC,
	GX_BASE_APB0_SEC, GX_BASE_APB1_SEC, GX_BASE_APB2_SEC, GX_BASE_APB3_SEC,
	GX_BASE_AHB1_SEC, GX_BASE_AHB2_SEC, GX_BASE_AHB3_SEC,
	GX_BASE_SENSOR_BUS,
};

void vega_permit_init(void)
{
	int i = 0;
	for (i = 0; i <= GX_PERMIT_BUS_SENSOR; i ++) {
		printf("config reg %x\n", reg_array[i]);
		REG_SET_VAL(reg_array[i] + 0x0, 0xffffffff);
		REG_SET_VAL(reg_array[i] + 0x4, 0xffffffff);
		if (reg_array[i] == GX_BASE_AHB3_SEC)
			REG_SET_VAL(reg_array[i] + 0x8, 0xffffdfff);
		else
			REG_SET_VAL(reg_array[i] + 0x8, 0xffffffff);
		REG_SET_VAL(reg_array[i] + 0xc, 0xffffffff);
	}
}

void vega_permit_get_reginfo(
	uint32_t module, uint32_t *id, uint32_t *tx_reg, uint32_t *rx_reg, uint32_t *tx_bit, uint32_t *rx_bit)
{
	uint32_t mod_bit = module & 0xf;
	*id  = (module >> 8) & 0xf;

	switch (module) {
	CASE_PERMIT_MOD(SDC         , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(FIREWALL    , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(CRYPTO      , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(MSR2_KLM    , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(M2M         , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(SYS_CONFIG  , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(TRNG        , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(HASH        , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(RCC         , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(OTPC        , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(A7_DAP      , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(AHB_FIREWALL, OFFSET_TX, OFFSET_RX, 0, 0);
	CASE_PERMIT_MOD(RSA         , OFFSET_TX, OFFSET_RX, 9, 9);
	CASE_PERMIT_MOD(TSIO        , OFFSET_TX, OFFSET_RX, 12, 12);
	CASE_PERMIT_MOD(AKL         , OFFSET_TX, OFFSET_RX, 11, 11);
	CASE_PERMIT_MOD(CLOCK_CONFIG, OFFSET_TX, OFFSET_RX, 12, 12);
	CASE_PERMIT_MOD(PIN_CONFIG  , OFFSET_TX, OFFSET_RX, 13, 13);
	CASE_PERMIT_MOD(RNG_CERT    , OFFSET_TX, OFFSET_RX, 14, 14);
	CASE_PERMIT_MOD(SENSOR      , OFFSET_TX, OFFSET_RX, 12, 12);
	CASE_PERMIT_MOD(SENSOR0_27M , OFFSET_TX, OFFSET_RX, 0, 0);
	CASE_PERMIT_MOD(SENSOR1_27M , OFFSET_TX, OFFSET_RX, 1, 1);
	CASE_PERMIT_MOD(SENSOR0_198M, OFFSET_TX, OFFSET_RX, 2, 2);
	CASE_PERMIT_MOD(SENSOR1_198M, OFFSET_TX, OFFSET_RX, 3, 3);
	CASE_PERMIT_MOD(SRAM_BOTTOM , OFFSET_SRAM_AUTH, OFFSET_SRAM_AUTH, 1, 0);
	CASE_PERMIT_MOD(SRAM_TOP    , OFFSET_SRAM_AUTH, OFFSET_SRAM_AUTH, 3, 2);
	CASE_PERMIT_MOD(JTAG        , OFFSET_EXT, OFFSET_EXT, 0, 0);
	CASE_PERMIT_MOD(RNG         , OFFSET_EXT, OFFSET_EXT, 2, 2);
	CASE_PERMIT_MOD(SYSCLK      , OFFSET_EXT, OFFSET_EXT, 3, 4);
	CASE_PERMIT_MOD(DEMUX_TS    , OFFSET_EXT, OFFSET_EXT, 0, 3);
	CASE_PERMIT_MOD(DEMUX_ES    , OFFSET_EXT, OFFSET_EXT, 1, 4);
	CASE_PERMIT_MOD(DEMUX_KEY   , OFFSET_EXT, OFFSET_EXT, 2, 5);
	CASE_PERMIT_MOD(ADSP_SEC    , OFFSET_EXT, OFFSET_EXT, 7, 8);
	CASE_PERMIT_MOD(APLAY_SEC   , OFFSET_EXT, OFFSET_EXT, 9, 10);
	CASE_PERMIT_MOD(HDMI_SEC    , OFFSET_EXT, OFFSET_EXT, 15, 16);
	CASE_PERMIT_MOD(VPU_SEC     , OFFSET_EXT, OFFSET_EXT, 17, 18);
	CASE_PERMIT_MOD(VDEC_SEC    , OFFSET_EXT, OFFSET_EXT, 21, 20);
	default:
		*id = -1;
		return;
	}
	*tx_reg += reg_array[*id];
	*rx_reg += reg_array[*id];
}
