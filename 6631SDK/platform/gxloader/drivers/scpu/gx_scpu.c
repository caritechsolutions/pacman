#include "gx_scpu.h"
#include <gxhwlib_registers.h>
#include "cpu_config.h"
#include "common/gx_mem_info.h"
#include "io.h"
#include "chip_info.h"

#ifdef CONFIG_ENABLE_SCPU
#define CYCLE_HZ  100     //单位HZ，具体值由 CORET_CALIB （系统计时模块）寄存器决定
void scpu_init(void)
{
	//config ddr protect length
	*(volatile unsigned int *)GXSCPU_PROT_SIZE = CMDLINE_SCPUMEM_SIZE;

	// config clock
	*(volatile unsigned int *)GXSCPU_CTIM_CALIB = GX_EXT_CLOCK / CYCLE_HZ - 1;

	//config code base the ddr phy addr
	*(volatile unsigned int *)GXSCPU_COSE_BASE = CMDLINE_SCPUMEM_START;

	//enable scpu
	*(volatile unsigned int *)GXSCPU_COSE_BASE |= 1;
}
#endif

int scpu_SMP_config(int mem_type, int id, unsigned int addr, int size)
{
#ifdef CONFIG_ARCH_CKMMU
	int ret = -1;
	unsigned int value;

	if (mem_type == GXMEM_FLAG_SCPU) {
	} else {
		printf("scpu smp config: mem_type %d error\n", mem_type);
		return -1;
	}

#if defined(CONFIG_ARCH_CKMMU_SCORPIO) || defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	if (gxcore_chip_probe() == 0x3113) {
		*(volatile unsigned int *)(CONFIG_BASE_MMU + CLOCK_GATE_CONFIG2_REG_1CLR) = 1 << 12;
		value = *(volatile unsigned int *)(CONFIG_BASE_MMU + CLOCK_GATE_REG3);
		value &= ~(1<<19);
		*(volatile unsigned int *)(CONFIG_BASE_MMU + CLOCK_GATE_REG3) = value;
	}
#endif

	__raw_writel(size, SCPU_CODE_DDR_SIZE);
	__raw_writel(addr, SCPU_CODE_DDR_CTR);
	REG_SET_BIT(SCPU_CODE_DDR_CTR, 1);
	REG_SET_BIT(SCPU_CODE_DDR_CTR, 0);
	printf("scpu smp config ready\n");
	return 0;
#endif
}
