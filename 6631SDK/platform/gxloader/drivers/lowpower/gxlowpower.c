#include <stdio.h>
#include "gxhwlib_registers.h"
#include "config.h"
#include "gxlowpower.h"

int lowpower_get_wake_flag(unsigned int *wakeflag)
{
	if (wakeflag == NULL)
		return -1;
#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI) \
	|| defined(CONFIG_ARCH_CKMMU_CYGNUS) \
	|| defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO) \
	|| defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	struct gx_tag_pmu *dpram_pmu;
	dpram_pmu = (struct gx_tag_pmu *)(REG_BASE_PMU + DPRAM_BASE);
	if (dpram_pmu->tag_header == GXLOWPOWER_MAGIC_NUMBER) { /* Magic */
		if (dpram_pmu->wakeflag != MANUAL_WAKE_FLAG &&
		    dpram_pmu->wakeflag != AUTO_WAKE_FLAG &&
		    dpram_pmu->wakeflag != REBOOT_WAKE_FLAG)
			dpram_pmu->wakeflag = NOT_WAKE_FLAG;

		*wakeflag = dpram_pmu->wakeflag;
	} else
		*wakeflag = NOT_WAKE_FLAG;
#elif defined(CONFIG_ARCH_CKMMU_GX6605S)
	struct gx_tag_mem *ram_mem;
	ram_mem = (struct gx_tag_mem *)(REG_BASE_PMU + PMU_DPRAM_SIZE - PMU_FW_DATA_SIZE);
	if (ram_mem->tag_header == GXLOWPOWER_MAGIC_NUMBER) { /* Magic */
		if (ram_mem->wakeflag != MANUAL_WAKE_FLAG &&
		    ram_mem->wakeflag != AUTO_WAKE_FLAG &&
		    ram_mem->wakeflag != REBOOT_WAKE_FLAG)
			ram_mem->wakeflag = NOT_WAKE_FLAG;

		*wakeflag = ram_mem->wakeflag;
	} else
		*wakeflag = NOT_WAKE_FLAG;

#else
	/*TODO*/
	*wakeflag = NOT_WAKE_FLAG;
#endif
	return 0;
}

