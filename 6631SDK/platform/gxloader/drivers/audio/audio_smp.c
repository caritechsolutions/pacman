#include "common/gx_mem_info.h"
#include "io.h"
#include <gxhwlib_registers.h>

int audio_SMP_config(int mem_type, int id, unsigned int addr, int size)
{
#if defined(CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS)
#define ADSP_WORKBUF_ADDR_REG (0xA460011C)
#define ADSP_WORKBUF_SIZE_REG (0xA46001E8)
#define ADSP_RST_REG          (0xA460011C)
#elif defined(CONFIG_ARCH_ARM_SIRIUS)
#define ADSP_WORKBUF_ADDR_REG (0x8A50011C)
#define ADSP_WORKBUF_SIZE_REG (0x8A5001E8)
#define ADSP_RST_REG          (0x8A50011C)
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
#define ADSP_WORKBUF_ADDR_REG (0x8A50013C)
#define ADSP_WORKBUF_SIZE_REG (0x8A500144)
#define ADSP_RST_REG          (0x8A500154)
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO)
#define ADSP_WORKBUF_ADDR_REG (0xA460013C)
#define ADSP_WORKBUF_SIZE_REG (0xA4600144)
#define ADSP_RST_REG          (0xA4600154)
#else
	printf("audio SMP config error!!!");
	return -1;
#endif

#define ADSP_CODE_OFFSET (0x100)
	//code header size: ADSP_CODE_OFFSET. adsp jump self.
	unsigned char *code_buf = (unsigned char *)(addr + ADSP_CODE_OFFSET + GX_DRAM_VIRTUAL_OFFSET);
	code_buf[0] = 0x00;
	code_buf[1] = 0x00;
	code_buf[2] = 0x00;
	code_buf[3] = 0x00;
	__raw_writel(size, ADSP_WORKBUF_SIZE_REG);
	__raw_writel(addr, ADSP_WORKBUF_ADDR_REG);
	REG_SET_BIT(ADSP_RST_REG, 0);
	REG_CLR_BIT(ADSP_RST_REG, 0);

	printf("afw smp config ready\n");
	return 0;
}
