#include "gx_rcc.h"
#include "sys/types.h"
#include "config.h"

/*
 * tips: these function will be invoked by stage1, so you can't use malloc in this file
 */

static struct gx_rcc_data_s rcc_data __attribute__((section(".sram"))) = {0};

#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
#define RCC_CLOCK_FRE          148000000
#else
#define RCC_CLOCK_FRE          173000000
#endif

#define RCC_INTERVAL_VALUE     (1000)
#define RCC_BUS_ACC_CONTROL_VALUE     (1)

int __attribute__((section(".sram_text"))) rcc_init(void)
{
	volatile struct rcc_reg_s *p_reg_base;

	rcc_data.reg_base = RCC_BASE_ADDR;
	p_reg_base = (volatile struct rcc_reg_s *)rcc_data.reg_base;
	RCC_SET_INTERVAL(p_reg_base, RCC_INTERVAL_VALUE);
	RCC_SET_BUS_ACC_CONTROL(p_reg_base, RCC_BUS_ACC_CONTROL_VALUE);

	return 0;
}

int __attribute__((section(".sram_text"))) rcc_mem_protect(unsigned int start_addr, unsigned int size)
{
	int i;
	volatile struct rcc_reg_s *p_reg_base;

	p_reg_base = (volatile struct rcc_reg_s *)rcc_data.reg_base;

	for (i = 0; i < GX_RCC_SECTION_MAX; i++) {

		if (rcc_data.gx_rcc_section[i].used_flag == 0) {

			rcc_data.gx_rcc_section[i].start_addr = start_addr;
			rcc_data.gx_rcc_section[i].size = size;
			rcc_data.gx_rcc_section[i].used_flag = 1;

			switch (i) {
			case 0:
				RCC_SET_RCC0_START_ADDR(p_reg_base, start_addr);
				RCC_SET_RCC0_SIZE(p_reg_base, size);
				RCC_SECTION0_START(p_reg_base);
				while(!RCC_SECTION0_STATUS(p_reg_base));
				break;
			case 1:
				RCC_SET_RCC1_START_ADDR(p_reg_base, start_addr);
				RCC_SET_RCC1_SIZE(p_reg_base, size);
				RCC_SECTION1_START(p_reg_base);
				while(!RCC_SECTION1_STATUS(p_reg_base));
				break;
			case 2:
				RCC_SET_RCC2_START_ADDR(p_reg_base, start_addr);
				RCC_SET_RCC2_SIZE(p_reg_base, size);
				RCC_SECTION2_START(p_reg_base);
				while(!RCC_SECTION2_STATUS(p_reg_base));
				break;
			case 3:
				RCC_SET_RCC3_START_ADDR(p_reg_base, start_addr);
				RCC_SET_RCC3_SIZE(p_reg_base, size);
				RCC_SECTION3_START(p_reg_base);
				while(!RCC_SECTION3_STATUS(p_reg_base));
				break;
			default:
				break;
			}

			break;
		}

	}
	if (i == GX_RCC_SECTION_MAX) {
		//printf("error: no free rcc section.\n");
		return -1;
	}

	return 0;
}
