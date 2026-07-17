#include "permit.h"

static uint32_t reg_array[] = {
	GX_BASE_AHB0_SEC,
	GX_BASE_APB0_SEC, GX_BASE_APB1_SEC, GX_BASE_APB2_SEC, GX_BASE_APB3_SEC,
	GX_BASE_AHB1_SEC, GX_BASE_AHB2_SEC, GX_BASE_AHB3_SEC
};

void sirius_permit_init(void)
{
	int i = 0;
	for (i = 0; i <= GX_PERMIT_BUS_AHB3; i ++) {
		REG_SET_VAL(reg_array[i] + 0x0, 0xffffffff);
		REG_SET_VAL(reg_array[i] + 0x4, 0xffffffff);
		REG_SET_VAL(reg_array[i] + 0x8, 0xffffffff);
		REG_SET_VAL(reg_array[i] + 0xc, 0xffffffff);
	}
}

void sirius_permit_get_reginfo(
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
	CASE_PERMIT_MOD(GP          , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(SRAM_BOTTOM , OFFSET_SRAM_AUTH, OFFSET_SRAM_AUTH, 1, 0);
	CASE_PERMIT_MOD(SRAM_TOP    , OFFSET_SRAM_AUTH, OFFSET_SRAM_AUTH, 3, 2);
	CASE_PERMIT_MOD(DEMUX_TS    , OFFSET_EXT, OFFSET_EXT, 0, 3);
	CASE_PERMIT_MOD(DEMUX_ES    , OFFSET_EXT, OFFSET_EXT, 1, 4);
	default:
		*id = -1;
		return;
	}
	*tx_reg += reg_array[*id];
	*rx_reg += reg_array[*id];
}
