#include "firewall.h"
#include "sram_firewall_reg.h"

void firewall_init(void)
{
#if defined(CONFIG_ARCH_ARM_SIRIUS)
	sirius_firewall_init();
#elif defined(CONFIG_ARCH_ARM_CANOPUS)
	canopus_firewall_init();
#elif defined(CONFIG_ARCH_ARM_VEGA)
	vega_firewall_init();
#endif
}

int firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission, int flag)
{
	int ret = 0;
	if (addr & 0x3ff) {
		printf("%s %d error : addr must be aligned in 1KB\n", __func__, __LINE__);
		return -2;
	}

	if (flag & FILTER_FLAG_FORCE_ALIGNMENT)
		size = (size + 0x3ff) & 0xfffffc00;
	if (size & 0x3ff) {
		printf("%s %d error : size must be aligned in 1KB\n", __func__, __LINE__);
		return -2;
	}

#if defined(CONFIG_ARCH_ARM_SIRIUS)
	ret = sirius_firewall_config_filter(addr, size, master_rd_permission, master_wr_permission, flag);
#elif defined(CONFIG_ARCH_ARM_CANOPUS)
	ret = canopus_firewall_config_filter(addr, size, master_rd_permission, master_wr_permission, flag);
#elif defined(CONFIG_ARCH_ARM_VEGA)
	ret = vega_firewall_config_filter(addr, size, master_rd_permission, master_wr_permission, flag);
#endif
	return ret;
}

int sram_firewall_config_filter(unsigned int addr, int size, int readable, int writable, int flag)
{
	int id = 1;

	if (addr & 0x3ff) {
		printf("%s %d error : addr must be aligned in 1KB\n", __func__, __LINE__);
		return -2;
	}

	if (flag & FILTER_FLAG_FORCE_ALIGNMENT)
		size = (size + 0x3ff) & 0xfffffc00;
	if (size & 0x3ff) {
		printf("%s %d error : size must be aligned in 1KB\n", __func__, __LINE__);
		return -2;
	}

	if (addr < SRAMBASE ||
		addr > (SRAMBASE + SRAM_SIZE) ||
		(addr + size) > (SRAMBASE + SRAM_SIZE)) {
		printf("%s %d error : addr = 0x%x, size = 0x%x\n", __func__, __LINE__, addr, size);
		return -2;
	}

#if defined (CONFIG_ARCH_ARM_SIRIUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
	if (REG_GET_VAL(REG_SFW_BOTTOM_SIZE) == 0 && addr != SRAMBASE) {
		printf("%s %d error : The first area shoud begin with SRAMBASE\n", __func__, __LINE__);
		return -1;
	}

	if (addr == SRAMBASE) {
		if (size == SRAM_SIZE)
			REG_SET_VAL(REG_SFW_BOTTOM_SIZE, 0);
		else {
			id = 0;
			REG_SET_VAL(REG_SFW_BOTTOM_SIZE, size);
		}
	} else {
		if ((addr != (SRAMBASE + REG_GET_VAL(REG_SFW_BOTTOM_SIZE))) || ((addr + size) != (SRAMBASE + SRAM_SIZE))) {
			printf("%s %d error : addr = 0x%x, size = 0x%x, bottom_end = 0x%x\n",
				   __func__, __LINE__, addr, size, REG_GET_VAL(REG_SFW_BOTTOM_SIZE));
			return -1;
		}
	}

	if (id == 0) { // Bottom area
		if (readable)
			REG_SET_BIT(REG_SFW_AUTH, BIT_SRAM_BOTTOM_RX);
		else
			REG_CLR_BIT(REG_SFW_AUTH, BIT_SRAM_BOTTOM_RX);
		if (writable)
			REG_SET_BIT(REG_SFW_AUTH, BIT_SRAM_BOTTOM_TX);
		else
			REG_CLR_BIT(REG_SFW_AUTH, BIT_SRAM_BOTTOM_TX);

	} else { // TOP area
		if (readable)
			REG_SET_BIT(REG_SFW_AUTH, BIT_SRAM_TOP_RX);
		else
			REG_CLR_BIT(REG_SFW_AUTH, BIT_SRAM_TOP_RX);
		if (writable)
			REG_SET_BIT(REG_SFW_AUTH, BIT_SRAM_TOP_TX);
		else
			REG_CLR_BIT(REG_SFW_AUTH, BIT_SRAM_TOP_TX);
	}
#endif
	return 0;
}
