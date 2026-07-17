#include "common/gx_mem_info.h"

#include "io.h"
#include <gxhwlib_registers.h>

int demux_SMP_config(int mem_type, int id, unsigned int addr, int size)
{
#ifdef CONFIG_ARCH_CKMMU
	int ret = -1;
	static uint8_t es_mask = 0;
	static uint8_t ts_mask = 0;

	if (mem_type == GXMEM_FLAG_DEMUX_ES) {
		switch(id) {
		case 0:
			__raw_writel(size,  DEMUX_ES0_BUF_SIZE);
			__raw_writel(addr, DEMUX_ES0_BUF_ADDR);
			es_mask |= 1 << 0;
			break;
		case 1:
			__raw_writel(size,  DEMUX_ES1_BUF_SIZE);
			__raw_writel(addr, DEMUX_ES1_BUF_ADDR);
			es_mask |= 1 << 1;
			break;
		case 2:
			__raw_writel(size,  DEMUX_ES2_BUF_SIZE);
			__raw_writel(addr, DEMUX_ES2_BUF_ADDR);
			es_mask |= 1 << 2;
			break;
		case 3:
			__raw_writel(size,  DEMUX_ES3_BUF_SIZE);
			__raw_writel(addr, DEMUX_ES3_BUF_ADDR);
			es_mask |= 1 << 3;
			break;
		}
	} else if (mem_type == GXMEM_FLAG_DEMUX_TSW) {
		__raw_writel(size,  DEMUX_TS0_BUF_SIZE);
		__raw_writel(addr, DEMUX_TS0_BUF_ADDR);
	} else if (mem_type == GXMEM_FLAG_DEMUX_TSR) {
		__raw_writel(size,  DEMUX_TS1_BUF_SIZE);
		__raw_writel(addr, DEMUX_TS1_BUF_ADDR);
	} else {
		printf("demux smp config: mem_type %d error\n", mem_type);
		return -1;
	}

	if (es_mask == 0xf) {
#if defined CONFIG_ARCH_CKMMU_TAURUS
		REG_SET_BIT(DEMUX_BUFFER_READY, 1);
#endif
		printf("demux es smp config ready\n");
		uint32_t saddr = *(volatile uint32_t *)DEMUX_ES0_BUF_ADDR + 0xa0000000;
		printf("register: %x, addr: %x\n", DEMUX_ES0_BUF_ADDR, saddr);
		printf("read  %x, value: %x\n", saddr, *(uint32_t *)saddr);
		*(uint32_t *)saddr = 0x55aa55aa;
		printf("write 0x55aa55aa to %x, read value: %x\n", saddr, *(uint32_t *)saddr);
	}
	if (ts_mask == 0x3) {
#if defined CONFIG_ARCH_CKMMU_TAURUS
		REG_SET_BIT(DEMUX_BUFFER_READY, 0);
#endif
		printf("demux ts smp config ready\n");
	}
	return 0;
#endif
}
