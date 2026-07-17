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
	unsigned char  clkbp;
	unsigned char  clkod;
	unsigned char  clkn;
	unsigned char  clkm;
} param_table[] = {
#ifdef CONFIG_24M_XTAL
	{400000000  , 0  , 0  , 3  , 50} ,  // 400Hz
	{432000000  , 0  , 0  , 1  , 18} ,  // 432Hz
	{533000000  , 0  , 0  , 5 , 111} , // 533MHz
	{594000000  , 0  , 0  , 4  , 99 } , // 816MHz
	{999000000  , 0  , 0  , 5  , 208 } , // 999MHz
	{1188000000 , 0  , 0  , 2  , 99 } , // 1.188GHz
#else
	{400000000  , 0  , 0  , 1  , 15} ,  // 432Hz
	{432000000  , 0  , 0  , 1  , 16} ,  // 432Hz
	{533000000  , 0  , 0  , 11 , 217} , // 533MHz
	{594000000  , 0  , 0  , 1  , 22 } , // 816MHz
	{999000000  , 0  , 0  , 1  , 37 } , // 999MHz
	{1188000000 , 0  , 0  , 1  , 44 } , // 1.188GHz
#endif
};

// fvco = freq_xtal * clkm / clkn,其中freq_xtal是晶振频率27000000
// fclkout = fvco / (2 ^ clkod)
void PLL_Config(unsigned int pll, unsigned int freq)
{
	int i;
	volatile unsigned int j = 100;
	for (i=0; i < sizeof(param_table) / sizeof(struct param); i++) {
		if (freq == param_table[i].freq) {
			unsigned int clkbp = param_table[i].clkbp;
			unsigned int clkod = param_table[i].clkod;
			unsigned int clkn = param_table[i].clkn;
			unsigned int clkm = param_table[i].clkm;
			*(volatile unsigned int*)(pll) = (clkbp<<15)|(1<<14)|(clkod<<12)|(clkn<<8)|clkm;
			while(j--);
			// [14] is pdrst, when change the Pll value, need pd go high at least 10ns. now cpu is xtal 37M. so set is ok
			*(volatile unsigned int*)(pll) &= ~(1<<14);
		}
	}
}

/* cpu_config.c */
/* gx3211_pll.c */
void gx_setup_pll_mini_controller(void)
{
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) = 0;
	// PLL config
	PLL_Config(PLL_CPU_CONFIG_BASE, 594000000); //set the CPU is 816M
	PLL_Config(PLL_DTO_CONFIG_BASE, 1188000000);//set the DTO is 1188M
	PLL_Config(PLL_DVB_CONFIG_BASE, 999000000); //set the DVB is 999M
	PLL_Config(PLL_DDR_CONFIG_BASE, DDR_FREQUENCY_CONFIG);//set the DDR fre

	while (*(volatile unsigned int*)(CONFIG_BASE+CONFIG_PLL_CONFIG_IN) != 0xF); // wait pll lock

	/*print*/
	serial_init(1, GX_EXT_CLOCK);
	serial_put_no_mmu('R');
/**************	// DTO CPU clock****************/
#if 1   //CPU_FREQ
	//set CPU AXI bus
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG2) &= ~(0xf<<0);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG2) |= (0x3<<0);// cpu axi clock div3 to 198

	//SET AHB clock
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL2) &= ~(0xf<<28);//AHB clock div 5 to 120
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL2) |= (0x7<<28); //0b:111 is five div

	//set source
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<24);//selcect the CPU clock to from xtal to PLL

	//gate the nfc
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_HOT_RST_1SET) |= (1<<24);//gate
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_HOT_RST_1CLR) |= (1<<24);//open
	//gate the usb
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_HOT_RST_1SET) |= (1<<25);//gate
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_HOT_RST_1CLR) |= (1<<25);//open
	//gate the mac
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_HOT_RST_1SET) |= (1<<26);//gate
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_HOT_RST_1CLR) |= (1<<26);//open

#endif

/*************this is for ddr. ddr have controller and phy and port clock ****************/
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) &= ~(1<<25);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<25);
///	end DDR clock

#if 1   //UART_CLOCK
	// APB2_2 clock(for UART)//29.4912M
	DTO_Config(11, 0x0196B86B, 0); //just for uart ,can change
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<20);
#endif

#if 0
	// APB2_0
	DTO_Config(10, 0x2E8BA2F, 0);//54M  ir and other interface
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<19);
#else
	// APB2_0
	DTO_Config(10, 0x1745D17, 0);//27M  ir and other interface
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) |= (1<<19);
#endif

#ifdef ENABLE_SECURE_ALIGN
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CPU_32BIT_ALIGN) |= 3; //config cpu 32 bit align
#endif

	serial_init(1, GX_UART_CLOCK);
	serial_put_no_mmu('U');
}
