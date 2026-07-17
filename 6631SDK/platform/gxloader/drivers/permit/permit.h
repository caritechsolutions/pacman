#ifndef __PERMIT_H__
#define __PERMIT_H__

#include "io.h"
#include "gxhwlib_registers.h"
#include "gx_permit.h"

enum {
	GX_PERMIT_BUS_AHB0,
	GX_PERMIT_BUS_APB0,
	GX_PERMIT_BUS_APB1,
	GX_PERMIT_BUS_APB2,
	GX_PERMIT_BUS_APB3,
	GX_PERMIT_BUS_AHB1,
	GX_PERMIT_BUS_AHB2,
	GX_PERMIT_BUS_AHB3,
	GX_PERMIT_BUS_SENSOR,
};

#define GX_BASE_AHB0_SEC (0x83C00000)
#define GX_BASE_APB0_SEC (0x8200F000)
#define GX_BASE_APB1_SEC (0x8240F000)
#define GX_BASE_APB2_SEC (0x8280F000)
#define GX_BASE_APB3_SEC (0x82C0F000)
#define GX_BASE_AHB1_SEC (0x88F00000)
#define GX_BASE_AHB2_SEC (0x89F00000)
#define GX_BASE_AHB3_SEC (0x8AF00000)
#define GX_BASE_SENSOR_BUS (0x88CF0000)

#define OFFSET_RX               (0x0)
#define OFFSET_TX               (0x4)
#define OFFSET_EXT              (0x8)
#define OFFSET_SRAM_BOTTOM_SIZE (0x8)
#define OFFSET_SRAM_AUTH        (0xc)

#define CASE_PERMIT_MOD(mod, _tx_reg, _rx_reg, _tx_bit, _rx_bit) \
	case GXSE_PERMIT_MOD_##mod: \
	*tx_reg = _tx_reg; \
	*rx_reg = _rx_reg; \
	*tx_bit = _tx_bit; \
	*rx_bit = _rx_bit; \
	break;

void sirius_permit_init(void);
void vega_permit_init(void);

void sirius_permit_get_reginfo(
	uint32_t module, uint32_t *id, uint32_t *tx_reg, uint32_t *rx_reg, uint32_t *tx_bit, uint32_t *rx_bit);

void canopus_permit_get_reginfo(
	uint32_t module, uint32_t *id, uint32_t *tx_reg, uint32_t *rx_reg, uint32_t *tx_bit, uint32_t *rx_bit);

void vega_permit_get_reginfo(
	uint32_t module, uint32_t *id, uint32_t *tx_reg, uint32_t *rx_reg, uint32_t *tx_bit, uint32_t *rx_bit);

#endif
