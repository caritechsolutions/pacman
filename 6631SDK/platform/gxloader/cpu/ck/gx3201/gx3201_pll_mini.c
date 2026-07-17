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
void DTO_Config(unsigned int dto, unsigned int div, unsigned char mmu_flag)
{
	unsigned int dto_config_base;
	if(mmu_flag){
		dto_config_base = CONFIG_BASE_MMU;
	}else{
		dto_config_base = CONFIG_BASE;
	}
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
	unsigned int   freq;
	unsigned short clkf;
	unsigned char  clkr;
	unsigned char  clkod;
} param_table[] = {
#if 0	/* in order to save sram memroy, only reserve cases that had be used. */
	{18432000   , 255  , 24 , 15}, // 18.432MHz
	{24576000   , 127  , 14 , 9} , // 24.576MHz
	{32768000   , 511  , 44 , 9} , // 32.768MHz
	{33868800   , 293  , 24 , 9} , // 33.8688MHz
	{36864000   , 255  , 24 , 7} , // 36.864MHz
	{40960000   , 895  , 62 , 9} , // 40.96MHz
	{45158400   , 391  , 24 , 9} , // 45.1584MHz
	{49152000   , 255  , 14 , 9} , // 49.152MHz
	{57600000   , 11   , 0  , 5} , // 57.6MHz
	{65536000   , 1023 , 44 , 9} , // 65.536MHz
	{67737600   , 588  , 24 , 9} , // 67.7376MHz
	{73728000   , 255  , 24 , 3} , // 73.728MHz
	{81920000   , 1791 , 62 , 9} , // 81.92MHz
	{90316800   , 783  , 24 , 9} , // 90.3168MHz
	{98304000   , 1023 , 24 , 11}, // 98.304MHz
	{125000000  , 624  , 35 , 3} , // 125MHz
	{200000000  , 124  , 8  , 1} , // 200MHz
	{297000000  , 164  , 3  , 3} , // 297MHz
	{333000000  , 184  , 7  , 1} , // 333MHz
	{400000000  , 249  , 8  , 1} , // 400MHz
	{445500000  , 494  , 31 , 0} , // 445.5MHZ
	{500000000  , 190  , 10 , 0} , // 500MHZ
	{533000000  , 36   , 0  , 1} , // 533MHz
	{728000000  , 1364 , 53 , 0} , // 728MHz
	{1066000000 , 36   , 0  , 0} , // 1066MHz
	{1186800000 , 1010 , 22 , 0} , // 1.1868GHz
	{1188000000 , 164  , 3  , 0} , // 1.188GHz
	{1189200000 , 990  , 23 , 0} , // 1.1892GHz
#else
	{57600000   , 255  , 14 , 7} , // 57.6MHz
	{400000000  , 799  , 26 , 1} , // 400MHz
	{500000000  , 36   , 0  , 1} , // 500MHz
	{533000000  , 38   , 0  , 1} , // 533MHz
	{728000000  , 26   , 0  , 0} , // actually 729MHz
	{98304000   , 1834 , 62 , 7} , // 98.304MHz
	{1186800000 , 3999 , 12 , 0} , // 1.1868GHz
	{1188000000 , 43   , 0  , 0}   // 1.188GHz
#endif
};
// fvco = frclk * (nf + 1) / (nr + 1)
// fclkout = fvco / (od + 1)
void PLL_Config(unsigned int pll, unsigned int freq)
{
	int i;
	for (i=0; i < sizeof(param_table) / sizeof(struct param); i++) {
		if (freq == param_table[i].freq) {
			unsigned int clkf = param_table[i].clkf;
			unsigned int clkr = param_table[i].clkr;
			unsigned int clkod = param_table[i].clkod;
			unsigned int bwadj = clkf;
			*(volatile unsigned int*)(pll + 0) = clkf | (clkr << 16) | (clkod << 24);
			*(volatile unsigned int*)(pll + 4) = 1;
			*(volatile unsigned int*)(pll + 4) = (1 << 7)|(bwadj << 16);
		}
	}
}

/* cpu_config.c */
/* gx3201_pll.c */
void gx_setup_pll_mini_controller(void)
{
	/*cpu/bus div:must be config before pll_config*/
	//*(volatile unsigned int*)0x0030a178 |= 0x3;	//cpu_clk_div_ratio if 600M,ahb div 3 200M
	*(volatile unsigned int*)0x0030a174 |= (0x7 << 28);	//if 600M, ahb div 4 150M.  AHB
	*(volatile unsigned int*)0x0030a178 |= 0x3;	//if 600M, ahb div 3 200M.  CPU AXI BUS
	*(volatile unsigned int*)0x0030a178 |= (1<<4)|(1<<5);//ahb update


	PLL_Config(PLL_DTO_CONFIG_BASE, PLL_DTO_CLK);
	PLL_Config(PLL_SDR_CONFIG_BASE, DDR_FREQUENCY_CONFIG);

	PLL_Config(PLL_VID_CONFIG_BASE, 1186800000);

	// CPU clock from dto
	DTO_Config(13, 0x20000000, 0);	//600M
	/*print*/
	serial_put_no_mmu('R');
	//DTO_Config(13, 0x10000000, 0);//300M  must modify with 0x0030a178
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<24)|(1<<28);	// DTO

	// APB2_2 clock(for UART)
	DTO_Config(11, 0x0196b86b, 0);	//fo = fi*div/2^30 = 1188000000*0x0196b86b/(2^30) = 29491199
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<20);

	serial_init(1, GX_UART_CLOCK);
	serial_put_no_mmu('U');
}
