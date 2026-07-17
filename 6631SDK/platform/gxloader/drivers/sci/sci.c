#include <gxhwlib_registers.h>
#include "chip_info.h"
#include "sci.h"

int gxsci_init_vcc_polarity(SCI_POL vcc)
{
	unsigned int reg_base;

#ifdef CONFIG_ARCH_CKMMU_GX6605S
	if (gxcore_chip_sub_probe() == 1)
		reg_base = REG_BASE_SMARTCARD0;
	else
		reg_base = REG_BASE_SMARTCARD1;
#else
	reg_base = REG_BASE_SMARTCARD;
#endif

	if (vcc == SCI_POL_LOW)
		*(volatile unsigned int *)(reg_base + 0x4) |= 1 << 13;
	else
		*(volatile unsigned int *)(reg_base + 0x4) &= ~(1 << 13);
	return 0;
}
