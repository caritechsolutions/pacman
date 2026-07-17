#include "firewall.h"
#include "canopus_firewall_reg.h"

#define CANOPUS_FILTER_MAX_NUM   (20)

static int filter_mask = 0;

void canopus_firewall_init(void)
{
	int i = 0;

	__raw_writel(CMDLINE_DRAM_SIZE, FW_DDR_SPACE);

	// clean all filter config
	for ( i = FW_FILTER_BASE(0); i <= FW_FILTER_CFG(19); i += 4)
		__raw_writel(0x00000000, i);

	// enable all instance & not hide default filter
	__raw_writel(0x1f, FW_GLOBLE_CFG);

	// enable default filter all read & write
	__raw_writel(0xffffffff, FW_DEFAULT_RD_MASK);
	__raw_writel(0xffffffff, FW_DEFAULT_WR_MASK);

	// Upload reg config
	__raw_writel(0x01, FW_CONFIG_UPLOAD);

	for (i = 0; i < CANOPUS_FILTER_MAX_NUM; i++) {
		if (__raw_readl(FW_FILTER_CFG(i)) != 0) {
			if (i == 10) {// GP_BUF_0
				filter_mask |= 0xf<<i;
				i+=3;
				continue;
			}
			filter_mask |= 1<<i;
		}
	}
}

int canopus_firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission, int flag)
{
	int secure_flag = 0;
	uint8_t cfg_value = 0;
	uint32_t _addr = 0, _size = 0, pos = 0, find = 0, i;

	for (i = 0; i < CANOPUS_FILTER_MAX_NUM; i++) {
		_addr = __raw_readl(FW_FILTER_BASE(i));
		_size = __raw_readl(FW_FILTER_SIZE(i));

		// find idle filter
		if ((find == 0) && (_addr == 0)) {
			if (filter_mask & (0x1<<i))
				continue;
			find = 1;
			pos = i;
		}

		// find same buffer
		if ((_addr == addr) && (_size == size)) {
			if ((filter_mask & (0x1<<i))) {
				printf("%s %d error : trying to modify addr[0x%08x] size[0x%08x]\n", __func__, __LINE__, _addr, _size);
				return -1;
			}
			pos = i;
			break;
		}
	}

	if (i == CANOPUS_FILTER_MAX_NUM && !find) {
		printf("%s %d error : no enough filter to use\n", __func__, __LINE__);
		return -1;
	}

	//printf("%s %d: find filter id = %d, mask = 0x%x\n", __func__, __LINE__, pos, filter_mask);

	secure_flag = (flag & FILTER_FLAG_SECURE_CHECK) ? 1 : 0;

	// normal filter
	// filter base address/size
	__raw_writel(addr, FW_FILTER_BASE(pos));
	__raw_writel(size, FW_FILTER_SIZE(pos));

	// filter read/write permit
	__raw_writel(master_rd_permission, FW_FILTER_RD_MASK(pos));
	__raw_writel(master_wr_permission, FW_FILTER_WR_MASK(pos));

	// filter config
	cfg_value = ((1 << 2) | (secure_flag << 1) | (1 << 0)) & 0xf;
	__raw_writel(cfg_value, FW_FILTER_CFG(pos));

	// Upload reg config
	__raw_writel(0x01, FW_CONFIG_UPLOAD);

	return 0;
}
