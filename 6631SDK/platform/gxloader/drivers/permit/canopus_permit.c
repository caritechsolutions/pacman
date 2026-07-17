#include "permit.h"

static uint32_t reg_array[] = {
	GX_BASE_AHB0_SEC,
	GX_BASE_APB0_SEC, GX_BASE_APB1_SEC, GX_BASE_APB2_SEC, GX_BASE_APB3_SEC,
	GX_BASE_AHB1_SEC, GX_BASE_AHB2_SEC, GX_BASE_AHB3_SEC
};

void canopus_permit_get_reginfo(
	uint32_t module, uint32_t *id, uint32_t *tx_reg, uint32_t *rx_reg, uint32_t *tx_bit, uint32_t *rx_bit)
{
	uint32_t mod_bit = module & 0xf;
	*id  = (module >> 8) & 0xf;

	switch (module) {
	CASE_PERMIT_MOD(SDC         , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
	CASE_PERMIT_MOD(PKA         , OFFSET_TX, OFFSET_RX, mod_bit, mod_bit);
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
	CASE_PERMIT_MOD(JTAG        , OFFSET_EXT, OFFSET_EXT, 0, 0);
	CASE_PERMIT_MOD(RNG         , OFFSET_EXT, OFFSET_EXT, 2, 2);
	CASE_PERMIT_MOD(SYSCLK      , OFFSET_EXT, OFFSET_EXT, 3, 4);
	CASE_PERMIT_MOD(DEMUX_TS    , OFFSET_EXT, OFFSET_EXT, 0, 3);
	CASE_PERMIT_MOD(DEMUX_ES    , OFFSET_EXT, OFFSET_EXT, 1, 4);
	CASE_PERMIT_MOD(DEMUX_KEY   , OFFSET_EXT, OFFSET_EXT, 2, 5);
	CASE_PERMIT_MOD(ADSP_SEC    , OFFSET_EXT, OFFSET_EXT, 7, 8);
	CASE_PERMIT_MOD(APLAY_SEC   , OFFSET_EXT, OFFSET_EXT, 9, 10);
	CASE_PERMIT_MOD(GP_SEC      , OFFSET_EXT, OFFSET_EXT, 12, 12);
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
