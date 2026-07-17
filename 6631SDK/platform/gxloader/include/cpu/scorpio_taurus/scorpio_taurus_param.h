#ifndef __SCORPIO_TAURUS_PARAM_H__
#define __SCORPIO_TAURUS_PARAM_H__

#define DENALI_CONFIG_CTL_NUM 155
#define DENALI_CONFIG_PHY_NUM 26

#define TAURUS_CHIP_ID 0x6616
#define SCORPIO_CHIP_ID 0x3113

/* sizeof(struct board_param_info) must <= EXTERN_PARAM_FILE_SIZE(EXTERN_PARAM_FILE_SIZE is 1024) bytes */
struct board_param_info {
	unsigned int chip;
	unsigned int extern_clock_xtal;
	unsigned int cpu_frequency;
	unsigned int stage2_stack_top_addr;
	unsigned int stage2_start_addr;
	unsigned int ddr_type;
	unsigned int ddr_frequency;
	unsigned int DENALI_CONFIG_CTL[DENALI_CONFIG_CTL_NUM];
	unsigned int DENALI_CONFIG_PHY[DENALI_CONFIG_PHY_NUM];
}__attribute__ ((packed));

extern struct board_param_info* board_param;

#endif
