#include <chip_info.h>

#define SCORPIO_CHIP_ID 0x3113
#define writel(v,addr) (void)((*(volatile unsigned int *) (addr)) = (v))

extern unsigned int chip_probe(unsigned int mmu_enable);

int usb_module_clock_enable(void)
{
	unsigned int v;
	unsigned int chip_id;

	chip_id = gxcore_chip_probe();
	if (chip_id != SCORPIO_CHIP_ID)
		return 0;

	// usb1_mac_clk/usb_mac_clk/usb_48m_clk/usb_12m_clk/usb_ahb_clk/usb_ahb_mac_clk
	v = ( (1 << 17) | (1 << 18) | (1 << 19) | (1 << 20) | (1 << 25) | (1 << 26) );
	writel(v, 0xa030a020);

	// usb_sys_pre_clk
	v = (1 << 17);
	writel(v, 0xa030a080);
	return 0;
}

int usb_module_clock_disable(void)
{
	unsigned int v;
	unsigned int chip_id;

	chip_id = gxcore_chip_probe();
	if (chip_id != SCORPIO_CHIP_ID)
		return 0;

	// usb_sys_pre_clk
	v = (1 << 17);
	writel(v, 0xa030a07c);

	// usb1_mac_clk/usb_mac_clk/usb_48m_clk/usb_12m_clk/usb_ahb_clk/usb_ahb_mac_clk
	v = ( (1 << 17) | (1 << 18) | (1 << 19) | (1 << 20) | (1 << 25) | (1 << 26) );
	writel(v, 0xa030a01c);
	return 0;
}
