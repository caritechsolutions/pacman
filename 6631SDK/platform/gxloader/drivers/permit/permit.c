#include "permit.h"

static void permit_get_reginfo(uint32_t module, uint32_t *id, uint32_t *tx_reg, uint32_t *rx_reg, uint32_t *tx_bit, uint32_t *rx_bit)
{
	*id = -1;
#if defined(CONFIG_ARCH_ARM_SIRIUS)
	sirius_permit_get_reginfo(module, id, tx_reg, rx_reg, tx_bit, rx_bit);
#elif defined(CONFIG_ARCH_ARM_CANOPUS)
	canopus_permit_get_reginfo(module, id, tx_reg, rx_reg, tx_bit, rx_bit);
#elif defined(CONFIG_ARCH_ARM_VEGA)
	vega_permit_get_reginfo(module, id, tx_reg, rx_reg, tx_bit, rx_bit);
#endif
}

void permit_init(void)
{
#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
	sirius_permit_init();
#elif defined(CONFIG_ARCH_ARM_VEGA)
	vega_permit_init();
#endif
}

void permit_set_config(unsigned int module, unsigned int flags)
{
	uint32_t id = 0, tx_bit = 0, rx_bit = 0, tx_reg = 0, rx_reg = 0;

	permit_get_reginfo(module, &id, &tx_reg, &rx_reg, &tx_bit, &rx_bit);
	if (id == -1)
		return;

	if (rx_reg == tx_reg && rx_bit == tx_bit) {
		if (flags)
			REG_CLR_BIT(rx_reg, rx_bit);
		else
			REG_SET_BIT(rx_reg, rx_bit);

	} else {
		if (flags & PERMIT_FLAG_REE_UNREADABLE)
			REG_CLR_BIT(rx_reg, rx_bit);
		else
			REG_SET_BIT(rx_reg, rx_bit);

		if (flags & PERMIT_FLAG_REE_UNWRITABLE)
			REG_CLR_BIT(tx_reg, tx_bit);
		else
			REG_SET_BIT(tx_reg, tx_bit);
	}
}
