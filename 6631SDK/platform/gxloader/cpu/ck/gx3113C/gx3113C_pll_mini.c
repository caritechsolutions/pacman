#include <gxhwlib_registers.h>
#include "cpu_config.h"

extern void serial_put_no_mmu(char);

/* routine.c */
void SDelay(unsigned int delay_by)
{
	volatile int i;
	volatile int j;

	for(i=0; i<delay_by; i++)
	{
		for(j=0; j<10; j++)
		{
			j=j;
		}
	}
}

// fo = fi*div/2^30
// div = fo*2^30/fi
// dto select the bus, fi is dto's pll fre, fo is bus fre (output fre), here fi is 1188000000
void DTO_Config(unsigned int dto, unsigned int div)
{
	unsigned int dto_config_base;
	dto_config_base = CONFIG_BASE;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31);
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = 0;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31);
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | div;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | (1 << 30) | div;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | (1 << 30) | div;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | div;
}

/* pll.c */
static struct param{
	unsigned int   	freq;
	unsigned char 	clkbp;
	unsigned char 	clkpd;
	unsigned char 	clkbs;
	unsigned char 	clkod;
	unsigned char 	clkr;
	unsigned char 	clkf;

} param_table[] = {
#ifdef CONFIG_24M_XTAL
	{162000000  , 0, 1, 0, 1, 1, 26} ,
	{200000000  , 0, 1, 0, 1, 1, 32} ,
	{300000000  , 0, 1, 0, 1, 0, 24} ,
	{864000000  , 0, 1, 1, 0, 0, 35} , //31(27M);29(28.8M)
#else
	{162000000  , 0, 1, 0, 1, 1, 23} ,
	{200000000  , 0, 1, 0, 1, 1, 29} ,
	{300000000  , 0, 1, 0, 1, 0, 21} ,
	{864000000  , 0, 1, 1, 0, 0, 31} , //31(27M);29(28.8M)
#endif
};
// fvco = frclk * nf / nr
// fclkout = fvco /od
void PLL_Config(unsigned int pll, unsigned int freq)
{
	int i;
	volatile unsigned int j = 100;
	for (i=0; i < sizeof(param_table) / sizeof(struct param); i++) {
		if (freq == param_table[i].freq) {
		       	unsigned int clkbp = param_table[i].clkbp;
			unsigned int clkpd = param_table[i].clkpd;
			unsigned int clkbs = param_table[i].clkbs;
			unsigned int clkod = param_table[i].clkod;
			unsigned int clkf = param_table[i].clkf;
			unsigned int clkr = param_table[i].clkr;
			*(volatile unsigned int*)(pll) = (clkbp<<16)|(0<<15)|(clkbs<<14)|(clkod<<12)|(clkr<<7)|clkf;
			while(j--);
			*(volatile unsigned int*)(pll) |= (clkpd<<15);
		}
	}
}

/* cpu_config.c */
/* gx3113C_pll.c */
void gx_setup_pll_mini_controller(void)
{
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) = 0;

	//close dac
	*(volatile unsigned int *)0xa4800134 = 0x1f;

	// PLL config
	PLL_Config(PLL_DTO_CONFIG_BASE, PLL_DTO_CLK ); //set the DTO is 864M

	PLL_Config(PLL_DDR_CONFIG_BASE, DDR_FREQUENCY_CONFIG);//set the ddr2 pll 300M

	//for AHB div
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG2) &= ~(0xf);//15:12 for clk ahb 108M
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG2) |= (0x4);

#if (defined CONFIG_ENABLE_GX_OTP) && (defined ENABLE_BOOT_TOOL)
	// do nothing
#else
	// cpu clk : 864/2=432M
	DTO_Config(13,0x20000000);
	//set cpu div
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG) &= ~(0xf<<12);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG) |= (0x1<<12);
	//set cpu source from dto
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<24);
#endif

	// clock dolphin for ddr io //108M
	DTO_Config(14, 0x8000000);//108
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<21);

	// APB2_2 clock(for UART)
	DTO_Config(11, 0x022F3D93); //just for uart ,can change
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<20);

	serial_init(1, GX_UART_CLOCK);
	serial_put_no_mmu('U');
}
