#include <gxhwlib_registers.h>
#include "cpu_config.h"

extern void serial_put_no_mmu(char);
extern void serial_init(int, int);

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

// div = 2^30 / dto_step
// fo = (fi)/(div)
// dto select the bus, fi is dto's pll fre, fo is bus fre (output fre), here fi is 1188000000
void DTO_Config(unsigned int dto, unsigned int dto_step, unsigned char mmu_flag)
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
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | dto_step;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | (1 << 30) | dto_step;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | (1 << 30) | dto_step;
	*(volatile unsigned int *)(dto_config_base + CONFIG_DTO_BASE + 4 * (dto - 1)) = (1 << 31) | dto_step;
}

/* Sirius pll.c */
/* freq = freq_xtal / refdiv * fbdiv / posdiv1 / posdiv2
 * freq_xtal = 24 MHz or 27 Mhz
 * postdiv1 must >= postdiv2
 * */
static struct param{
	unsigned int   freq;
	unsigned char  pll_posdiv2;
	unsigned char  pll_posdiv1;
	unsigned char  pll_refdiv;
	unsigned int   pll_fbdiv;
} param_table[] = {
	/* freq | posdiv2 | posdiv1 | refdiv | fbdiv */
#ifdef CONFIG_24M_XTAL
	{400000000	, 1  , 3  , 4  , 200} , // 400Hz
	{432000000	, 1  , 3  , 2  , 108} , // 432Hz
	{500000000	, 1  , 2  , 3  , 125} , // 500MHz 用于 cpu
	{533000000	, 1  , 3  , 3  , 200} , // 533MHz
	{594000000	, 1  , 2  , 4  , 198} , // 594MHz
	{600000000	, 1  , 2  , 3  , 150} , // 600MHz 用于 cpu
	{666000000	, 1  , 2  , 4  , 222} , // 666MHz
	{702000000	, 1  , 1  , 4  , 117} , // 702MHz 用于 cpu 或者ddr 降频
	{800000000	, 1  , 2  , 3  , 200} , // 800MHz 用于 cpu 超频以及DDR
	{900000000	, 1  , 1  , 4  , 150} , // 900MHz 用于 cpu 超频
	{999000000	, 1  , 1  , 3  , 125} , // 999MHz 用于 cpu 超频
	{1100000000	, 1  , 1  , 4  , 183} , // 1.10025GHz 用于 cpu 超频
	{1188000000	, 1  , 1  , 2  , 99 } , // 1.188GHz 用于DTO
#else
	{400000000  , 1  , 3  , 2  ,  89} , // 400Hz
	{432000000  , 1  , 3  , 3  , 144} , // 432Hz
	{500000000  , 1  , 2  , 5  , 185} , // 500MHz 用于 cpu
	{533000000  , 1  , 1  , 4  ,  79} , // 533MHz
	{594000000  , 1  , 1  , 2  ,  44} , // 594MHz
	{600000000  , 1  , 2  , 5  , 222} , // 600MHz 用于 cpu
	{666000000  , 1  , 1  , 3  ,  74} , // 666MHz
	{675000000  , 1  , 1  , 1  ,  25} , // 675MHz
	{702000000  , 1  , 1  , 3  ,  78} , // 702MHz 用于 cpu 或者ddr 降频
	{756000000  , 1  , 1  , 1  ,  28} , // 756MHz 用于外置ddr方案 降频
	{783000000  , 1  , 1  , 1  ,  29} , // 783MHz
	{800000000  , 1  , 1  , 3  ,  89} , // 800MHz 用于 cpu 超频以及DDR
	{900000000  , 1  , 1  , 3  , 100} , // 900MHz 用于 cpu 超频
	{999000000  , 1  , 1  , 1  , 37 } , // 999MHz 用于 cpu 超频
	{1100000000 , 1  , 1  , 4  , 163} , // 1.10025GHz 用于 cpu 超频
	{1188000000 , 1  , 1  , 1  , 44 } , // 1.188GHz 用于DTO
#endif
};

// 其中freq_xtal是晶振频率27000000/24000000等
void PLL_Config(unsigned int pll, unsigned int freq)
{
	int i;
	volatile unsigned int j = 100;
	for (i=0; i < sizeof(param_table) / sizeof(struct param); i++) {
		if (freq == param_table[i].freq) {
			unsigned int pll_posdiv1 = param_table[i].pll_posdiv1;
			unsigned int pll_posdiv2 = param_table[i].pll_posdiv2;
			unsigned int pll_refdiv = param_table[i].pll_refdiv;
			unsigned int pll_fbdiv = param_table[i].pll_fbdiv;
			*(volatile unsigned int*)(pll) = (0xf<<27)|(pll_refdiv<<20)|(pll_posdiv2<<16)|(pll_posdiv1<<12)|pll_fbdiv;
			while(j--);
			// [14] is pdrst, when change the Pll value, need pd go high at least 10ns. now cpu is xtal 37M. so set is ok
			*(volatile unsigned int*)(pll) &= ~(0xd<<27);
		}
	}
}

void gx_setup_pll_mini_controller(void)
{
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) = 0;
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL2) = 0;

	// PLL config
	PLL_Config(CONFIG_BASE + CONFIG_PLL1_CONFIG, 1188000000);			//set the DTO is 1188M, from pll
	PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, DDR_FREQUENCY_CONFIG);		//set the DDR fre, from pll
	PLL_Config(CONFIG_BASE + CONFIG_PLL3_CONFIG, CPU_FREQUENCY_CONFIG);		//set the CPU fre, from pll

	while (*(volatile unsigned int*)(CONFIG_BASE+CONFIG_PLL_CONFIG_IN) != 0x7); // wait pll lock

	/*print*/
	serial_init(1, GX_EXT_CLOCK);
	serial_put_no_mmu('R');

	/************** CPU clock ****************/
	// AXI bus from CPU clock
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CLOCK_DIV_CONFIG2) &= ~(0xf<<0);
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CLOCK_DIV_CONFIG2) |= (0x3<<0);// cpu 700Mhz to core axi clock div3 to 233

	// debug clk from CPU clock
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CLOCK_DIV_CONFIG4) &= ~(0xf<<8);
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CLOCK_DIV_CONFIG4) |= (0x7<<8);// cpu 700Mhz to arm_debug_clk clock div4 to 175

	// AHB bus from CPU
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG4) &= ~(1<<20);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG4) |= (1<<20); //rst
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG4) &= ~(0x3f<<12);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG4) |= (0x5<<12); // 675 div 6 to 112
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG4) &= ~(1<<19);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG4) |= (1<<19); //load en

	//set source
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<24);//selcect the CPU clock to from xtal to PLL
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL2) |= (1<<11);//selcect the AHB clock to from xtal to CPU clock

	//gate the nfc
	//*(volatile unsigned int*)(CONFIG_BASE + CONFIG_MPEG_HOT_RST_1SET) |= (1<<24);//gate
	//*(volatile unsigned int*)(CONFIG_BASE + CONFIG_MPEG_HOT_RST_1CLR) |= (1<<24);//open
	//gate the usb
	//*(volatile unsigned int*)(CONFIG_BASE + CONFIG_MPEG_HOT_RST_1SET) |= (1<<25);//gate
	//*(volatile unsigned int*)(CONFIG_BASE + CONFIG_MPEG_HOT_RST_1CLR) |= (1<<25);//open
	//gate the mac
	//*(volatile unsigned int*)(CONFIG_BASE + CONFIG_MPEG_HOT_RST_1SET) |= (1<<26);//gate
	//*(volatile unsigned int*)(CONFIG_BASE + CONFIG_MPEG_HOT_RST_1CLR) |= (1<<26);//open
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_MPEG_CLK_INHIBIT2_NORM) |= (1<<16);//gate the gse module

	/************** SCPU clock ****************/
	// scpu clock from DTO div
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) &= ~(1<<7);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) |= (1<<7); //rst
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) &= ~(0x3f<<0);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) |= (0x7<<0); //1188 div8  to 148.5
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) &= ~(1<<6);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_CLOCK_DIV_CONFIG3) |= (1<<6); //load en
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) |= (1<<23);//selcect the SCPU clock to from xtal to DTO div


	/************** DDR clock ****************/
	/*************this is for ddr. ddr have controller and phy and port clock ****************/
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) |= (1<<25); //select the DDR clock from xtal to PLL

	/************** APB clock ****************/
	// APB2_3 clock(for smartcard) //180M
	DTO_Config(12, 0x9b26c9b, 0);
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) |= (1<<22); //select the APB_2_3 clock from xtal to DTO

	// APB2_2 clock(for UART) //29.4912M
	DTO_Config(15, 0x0196B86B, 0);
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) |= (1<<21); //select the APB_2_2 clock from xtal to DTO

	// APB2_1/APB2 clock(for GPIO/SPI and so on) //167.714M
	DTO_Config(22, 0x09249249, 0);
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) |= (1<<20); //select the APB_2_1 clock from xtal to DTO

#if 0
	// APB2_0 clock(for ir/i2c/rtc/timer)
	DTO_Config(3, 0x2E8BA2F, 0);//54M
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) |= (1<<19); //select the APB_2_0/APB2 clock from xtal to DTO
#else
	// APB2_0 clock(for ir/i2c/rtc/timer)
	DTO_Config(3, 0x1745D17, 0);//27M
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_SOURCE_SEL) |= (1<<19); //select the APB_2_0/APB2 clock from xtal to DTO
#endif

	serial_init(1, GX_UART_CLOCK);
	serial_put_no_mmu('U');
}
