#include "firewall.h"
#include "sirius_firewall_reg.h"

#define SIRIUS_FILTER_MAX_NUM   (24)

static int filter_mask = 0;

void sirius_firewall_init(void)
{
	int i = 0;

	// disable all filter
	__raw_writel(0x00000000, FW_FILTER_CFG0);
	__raw_writel(0x00000000, FW_FILTER_CFG1);
	__raw_writel(0x00000000, FW_FILTER_CFG2);

	// clean all filter config
	for ( i = FW_FILTER_BASE(0); i <= FW_FILTER_WR_MASK(23); i += 4)
		__raw_writel(0x00000000, i);

	// enable all instance & not hide default filter
	__raw_writel(0x1f, FW_GLOBLE_CFG);

	// enable default filter all read & write
	__raw_writel(0xffffffff, FW_DEFAULT_RD_MASK);
	__raw_writel(0xffffffff, FW_DEFAULT_WR_MASK);

	// Upload reg config
	__raw_writel(0x01, FW_CONFIG_UPLOAD);

	for (i = 0; i < SIRIUS_FILTER_MAX_NUM; i++) {
		if (__raw_readl(FW_FILTER_BASE(i)) != 0)
			filter_mask |= 1<<i;
	}
}

int sirius_firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission, int flag)
{
	uint8_t cfg_value = 0;
	int32_t secure_flag = 0, fw_filter_cfg = 0;
	uint32_t _addr = 0, _end = 0, pos = 0, find = 0, i;

	for (i = 0; i < SIRIUS_FILTER_MAX_NUM; i++) {
		_addr = __raw_readl(FW_FILTER_BASE(i));
		_end  = __raw_readl(FW_FILTER_TOP(i));

		// find idle filter
		if ((find == 0) && (_addr == 0)) {
			if (filter_mask & (0x1<<i))
				continue;
			find = 1;
			pos = i;
		}

		// find same buffer
		if ((_addr == addr) && ((_end - _addr) == size)) {
			if ((filter_mask & (0x1<<i))) {
				printf("%s %d error : trying to modify addr[0x%08x] size[0x%08x]\n", __func__, __LINE__, addr, size);
				return -1;
			}
			pos = i;
			break;
		}
	}

	if (i == SIRIUS_FILTER_MAX_NUM && !find) {
		printf("%s %d error : no enough filter to use\n", __func__, __LINE__);
		return -1;
	}

	// normal filter
	// filter base address/size
	__raw_writel(addr, FW_FILTER_BASE(pos));
	__raw_writel(size, FW_FILTER_TOP(pos));

	// filter read/write permit
	__raw_writel(master_rd_permission, FW_FILTER_RD_MASK(pos));
	__raw_writel(master_wr_permission, FW_FILTER_WR_MASK(pos));

	// filter config
	cfg_value = ((1 << 2) | (secure_flag << 1) | (1 << 0)) & 0xf;
	fw_filter_cfg = __raw_readl(FW_FILTER_CFG0 + (pos / FILTER_MASK) * 4);
	fw_filter_cfg &= ~((0xF) << ((pos % FILTER_MASK) * 4));
	fw_filter_cfg |= ((cfg_value) << ((pos % FILTER_MASK) * 4));
	__raw_writel(fw_filter_cfg, FW_FILTER_CFG0 + (pos / FILTER_MASK) * 4);

	// Upload reg config
	__raw_writel(0x01, FW_CONFIG_UPLOAD);

	return 0;
}
