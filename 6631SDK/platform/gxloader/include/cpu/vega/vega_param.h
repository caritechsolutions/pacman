#ifndef __VEGA_PARAM_H__
#define __VEGA_PARAM_H__

/* DO NOT INCLUDE ANY HEADER FILES AS THIS FILE IS USED BY STAGE1 AND PC TOOLS */

#define    SIZE_OF_TYPE_NOT_LARGER_THAN(type, size) \
	static inline char size_of_##type##_not_larger_than_##size(void) \
{ \
	    char __dummy1[size - sizeof(type)]; \
	    return __dummy1[-1]; \
}

#define DENALI_CONFIG_CTL_NUM 159
#define DENALI_CONFIG_PHY_NUM 31

#define OTP_DRAM_PARAM_LEN 12 /* bytes */

#define REE_SECURITY_CFG_SIZE 384  /* bytes */
#define TEE_SECURITY_CFG_SIZE 1020 /* bytes */

typedef struct __attribute__((packed)) {
	unsigned int stage2_flash_size;
	unsigned int stage2_dram_addr;
} spl_boot_info;
SIZE_OF_TYPE_NOT_LARGER_THAN(spl_boot_info, 8)

typedef struct __attribute__((packed)) {
	unsigned int extern_clock_xtal;
	unsigned int dto_pll_config;
	unsigned int ddr_pll_config;
	unsigned int ddr_pll_config_backup;
	unsigned int cpu_pll_config;
	unsigned char arm_axi_clk_div;
	unsigned char arm_debug_clk_div;
	unsigned char ahb0_clk_div;
	unsigned char ahb1_clk_div;
	unsigned char ahb2_clk_div;
	unsigned char ahb3_clk_div;
	unsigned char secure_clk_div;
	unsigned char secure_clk_src;
	unsigned int apb0_clk_div;
	unsigned int apb2_clk_div;
	unsigned char spi_clk_div;
	unsigned char ddr_pll_use_inno;
} spl_clock_info;
SIZE_OF_TYPE_NOT_LARGER_THAN(spl_clock_info, 128)

typedef struct __attribute__((packed)) {
	unsigned int dram_type;
	unsigned int dram_size;
	unsigned int dram_config[8];
	unsigned int dram_config_backup[8];
	unsigned int DENALI_CONFIG_CTL[DENALI_CONFIG_CTL_NUM];
	unsigned int DENALI_CONFIG_PHY[DENALI_CONFIG_PHY_NUM];
	unsigned int DENALI_CONFIG_CTL_BACKUP[DENALI_CONFIG_CTL_NUM];
	unsigned int DENALI_CONFIG_PHY_BACKUP[DENALI_CONFIG_PHY_NUM];
	unsigned char read_dram_priority[5];
	unsigned char write_dram_priority[5];
	unsigned char vref_value;
	unsigned char ddr_patch_param_1[OTP_DRAM_PARAM_LEN];
	unsigned char ddr_patch_param_2[OTP_DRAM_PARAM_LEN];
} spl_dram_info;
SIZE_OF_TYPE_NOT_LARGER_THAN(spl_dram_info, 2048)

typedef struct {
	unsigned int clk_div;        /* spinor 分频值 */
	unsigned int sample_delay;   /* spinor 读采样延时 */
	unsigned int addr_cyc;       /* spinor 地址长度 */
}nor_flash_info_t;

typedef struct {
	unsigned int clk_div;                /* spinand 分频值 */
	unsigned int smp_dly_neg_spi;        /* spinand 采样延时 */
	unsigned int page_size;              /* spinand 页大小 */
	unsigned int page_num_per_block;     /* spinand block含的页数 */
	unsigned int plane_sel;              /* spinand plane奇偶选择 */
	unsigned int tsc_tcshs;              /* spinand cs高电平最小宽度 */
	unsigned int dummy_before_addr;      /* spinand addr前是否有dummy字节 */
	unsigned int dummy_after_addr;       /* spinand addr后面是否有dummy字节 */
	unsigned int addr_cyc;               /* spinand 地址长度 */
}spinand_flash_info_t;

typedef struct {
	unsigned int page_size;              /* nand 页大小 */
	unsigned int page_num_per_block;     /* nand block含的页数 */
	unsigned int twp_twh;                /* nand we#高低电平宽度 */
	unsigned int trp_treh;               /* nand re#高低电平宽度 */
	unsigned int trp_treh_fast;          /* nand 批量读re#高低电平宽度 */
	unsigned int smp_dly_neg_fast;       /* nand 读采样延时 */
	unsigned int twb_trr;                /* nand rb#采样延时 */
	unsigned int trwad;                  /* nand 读写切换的时间 */
	unsigned int busy_timeout;           /* nand busy 超时时间 */
	unsigned int addr_cyc;               /* nand 地址长度 */
}paranand_flash_info_t;

typedef struct __attribute__((packed)) {
	nor_flash_info_t nor_flash;
	spinand_flash_info_t snand_flash;
	paranand_flash_info_t nand_flash;
} spl_flash_info;
SIZE_OF_TYPE_NOT_LARGER_THAN(spl_flash_info, 128)

typedef struct  __attribute__((packed)) {
	unsigned int tee_stage2_flash_size;
	unsigned int tee_stage2_dram_addr;
} tee_spl_boot_info ;
SIZE_OF_TYPE_NOT_LARGER_THAN(tee_spl_boot_info, 8)

typedef struct {
	unsigned int stage2_flash_size;
	unsigned int stage2_flash_size_checksum;
	unsigned int stage2_dram_addr;
	unsigned int stage2_dram_addr_checksum;
	unsigned int tee_stage2_flash_size;
	unsigned int tee_stage2_flash_size_checksum;
	unsigned int tee_stage2_dram_addr;
	unsigned int tee_stage2_dram_addr_checksum;
} loader_boot_info;
SIZE_OF_TYPE_NOT_LARGER_THAN(loader_boot_info, 32)

typedef struct {
	unsigned int addr;
	unsigned int value;
	unsigned int mask;
} security_cfg_t;

extern spl_boot_info *g_spl_boot_info;
extern spl_clock_info *g_spl_clock_info;
extern spl_dram_info *g_spl_dram_info;
extern spl_flash_info  *g_spl_flash_info;
extern tee_spl_boot_info *g_tee_spl_boot_info;
extern loader_boot_info boot_info;

#endif
