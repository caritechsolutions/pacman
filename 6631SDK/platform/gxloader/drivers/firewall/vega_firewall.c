#include "firewall.h"
#include "vega_firewall_reg.h"

#define VEGA_FILTER_MAX_NUM   (20)

void vega_firewall_init(void)
{
	int i = 0, id = 0;

	__raw_writel(CMDLINE_DRAM_SIZE, FW_DDR_SPACE);

	if (__raw_readl(FW_OTP_INFO) & 0xf)
		id = FILTER_FIRST_SOFT_BUFFER_ID;
	// clean all filter config
	for ( i = FW_FILTER_BASE(id); i <= FW_FILTER_CFG(VEGA_FILTER_MAX_NUM - 1); i += 4)
		__raw_writel(0x00000000, i);

	// enable default filter all read & write
	__raw_writel(0x97fdfdf8, FW_DEFAULT_RD_MASK);
	__raw_writel(0x97d39df0, FW_DEFAULT_WR_MASK);

	// enable all instance & hide default filter
	__raw_writel(0x3f, FW_GLOBLE_CFG);

	// Upload reg config
	__raw_writel(0x01, FW_CONFIG_UPLOAD);
}

int vega_firewall_config_filter(unsigned int addr, int size, int master_rd_permission, int master_wr_permission, int flag)
{
	int i = 0;
	static int filter_num = 0;
	int cur_filter_num = 0, secure_flag = 0;
	unsigned char cfg_value = 0;

	if (addr == 0 && size == 0) {
		__raw_writel(master_rd_permission, FW_DEFAULT_RD_MASK);
		__raw_writel(master_wr_permission, FW_DEFAULT_WR_MASK);
		__raw_writel(master_wr_permission, FW_DEFAULT_WR_MASK);

		// Upload reg config
		__raw_writel(0x01, FW_CONFIG_UPLOAD);
	}

	for (i = 0; i < VEGA_FILTER_MAX_NUM; i++) {
		int _addr = __raw_readl(FW_FILTER_BASE(i));
		int _size = __raw_readl(FW_FILTER_SIZE(i));
		if ((_addr == addr) && (_size == size)) {
			cur_filter_num = i;
			break;
		}
	}

	if (i == VEGA_FILTER_MAX_NUM) {
		if (filter_num >= VEGA_FILTER_MAX_NUM) {
			printf("%s %d error : no enough filter to use\n", __func__, __LINE__);
			return -1;
		}
		cur_filter_num = filter_num;
		filter_num++;
	}

	secure_flag = (flag & FILTER_FLAG_SECURE_CHECK) ? 1 : 0;

	// normal filter
	// filter base address/size
	__raw_writel(addr, FW_FILTER_BASE(cur_filter_num));
	__raw_writel(size, FW_FILTER_SIZE(cur_filter_num));

	// filter read/write permit
	__raw_writel(master_rd_permission, FW_FILTER_RD_MASK(cur_filter_num));
	__raw_writel(master_wr_permission, FW_FILTER_WR_MASK(cur_filter_num));

	// filter config
	cfg_value = ((1 << 2) | (secure_flag << 1) | (1 << 0)) & 0xf;
	__raw_writel(cfg_value, FW_FILTER_CFG(cur_filter_num));

	// Upload reg config
	__raw_writel(0x01, FW_CONFIG_UPLOAD);

	return 0;
}
