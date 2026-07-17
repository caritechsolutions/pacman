#include "gx_ddr.h"

int gx_ddr_get_phase_test(void)
{
#if defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
#define PMU_CPU_STATE_REG0 0x89700310
#define DDR_PHASE_TEST_RESULT_ADDR PMU_CPU_STATE_REG0
	return *(volatile unsigned int *) DDR_PHASE_TEST_RESULT_ADDR;
#else
	return 0;
#endif
}
