#include <gxhwlib_registers.h>
#include "cpu_config.h"
#include "io.h"

extern void serial_put_no_mmu(char);
extern void serial_init(int, int);

static struct dto_param dto_sec_table_mini[] = {
	{6,         1,       0x01745D17,   1<<13}, // APB2_0 27MHz
	{7,         1,       0x01999999,   1<<15}, // APB2_2 29.7MHz
};

static struct div_param div_table_mini[] = {
	{2, 2, 0xff<<24 , 1<<31 , 1<<30 , 3<<24 , 1<<20}, // DW SPI 1188/4=297MHz
};

static struct div_param div_sec_table_mini[] = {
	{0, 1, 0xff<<0  , 1<<7  , 1<<6  , 7<<0  , 1<<6 }, // Secure ctrl 148.5MHz
	{1, 1, 0xf<<4   , 0     , 0     , 8<<4  , 0    }, // ARM debug_pclk, PLL_CPU_CLK/div6≈150MHz
	{1, 1, 0xf<<0   , 0     , 0     , 0<<0  , 0    }, // ARM AXI clock div PLL_CPU_CLK/div2≈364.5MHz
	{2, 1, 0xff<<24 , 1<<31 , 1<<30 , 5<<24 , 1<<12}, // AHB3 198MHz
	{2, 1, 0xff<<16 , 1<<23 , 1<<22 , 7<<16 , 1<<11}, // AHB2 148.5MHz
	{2, 1, 0xff<<8  , 1<<15 , 1<<14 , 9<<8  , 1<<10}, // AHB1 118.8MHz
	{2, 1, 0xff<<0  , 1<<7  , 1<<6  , 7<<0  , 1<<9 }, // AHB0 148.5MHz
};

/* freq = freq_xtal / refdiv * fbdiv / posdiv1 / posdiv2
 * freq_xtal = 24 MHz or 27 Mhz
 * postdiv1 must >= postdiv2
 * */
#if defined(CONFIG_24M_XTAL)
static struct pll_param pll_param_mini_24mhz[] = {
	/* freq | refdiv | posdiv2 | posdiv1 | fbdiv */
	{1536000000 ,1  ,1  ,1, 64 },  //1536MHz
	{1224000000 ,1  ,1  ,1, 51 },  //1224MHz
	{1188000000 ,2  ,1  ,1, 99 },  //1188MHz
	{1080000000 ,1  ,1  ,1, 45 },  //1080MHz
	{900000000  ,1  ,1  ,1, 37 },  //888MHz
	{850000000  ,1  ,1  ,1, 35 },  //840MHz
	{783000000  ,1  ,1  ,1, 33 },  //792MHz
	{750000000  ,1  ,1  ,1, 31 },  //744MHz
	{667000000  ,1  ,1  ,2, 56 },  //672MHz
	{660000000  ,1  ,1  ,2, 55 },  //660MHz
	{533000000  ,1  ,1  ,2, 44 },  //528MHz
	{400000000  ,4  ,1  ,3, 200},  //400MHz
};
#endif

#if !defined(CONFIG_24M_XTAL)
static struct pll_param pll_param_mini_27mhz[] = {
	/* freq | refdiv | posdiv2 | posdiv1 | fbdiv */
	{1539000000 ,1  ,1  ,1, 57 },  //1539MHz
	{1188000000 ,1  ,1  ,1, 44 },  //1188MHz
	{1161000000 ,1  ,1  ,1, 43 },  //1161MHz
	{1080000000 ,1  ,1  ,1, 40 },  //1080MHz
	{900000000  ,1  ,1  ,1, 33 },  //891MHz
	{864000000  ,1  ,1  ,1, 32 },  //864MHz
	{850000000  ,1  ,1  ,1, 31 },  //837MHz
	{783000000  ,1  ,1  ,1, 29 },  //783MHz
	{750000000  ,1  ,1  ,1, 28 },  //756MHz
	{729000000  ,1  ,1  ,1, 27 },  //729MHz
	{667000000  ,1  ,1  ,1, 25 },  //675MHz
	{533000000  ,2  ,1  ,2, 79 },  //533MHz
	{400000000  ,4  ,1  ,2, 119},  //400MHz
};
#endif
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

static void __dto_config(struct dto_param *param, unsigned char mmu_flag, unsigned char sec_flag)
{
	unsigned int data;
	unsigned int config_base;
	unsigned int dto_cfg_addr;

	unsigned int value_cfg_addr;
	unsigned int dto;
	unsigned int source_sel;
	unsigned int div;
	unsigned int source_sel_bit;

	if ((!param) || !(param->dto))
		return;

	dto		= param->dto;
	source_sel	= param->source_sel;
	div		= param->div;
	source_sel_bit	= param->source_sel_bit;

	config_base = CONFIG_BASE_MMU;

	if (sec_flag == 0) {
		dto_cfg_addr = config_base + CONFIG_DTO1_CONFIG + 4 * (dto - 1);

		if (param->source_sel == 1)
			value_cfg_addr = config_base + CONFIG_CLOCK_SOURCE1;
		else if (param->source_sel == 2)
			value_cfg_addr = config_base + CONFIG_CLOCK_SOURCE2;
	} else {
		dto_cfg_addr = config_base + CONFIG_DTO1_CONFIG_SEC + 4 * (dto - 1);

		value_cfg_addr = config_base + CONFIG_CLOCK_SOURCE_SEC;
	}

	__raw_writel((0<<31 | 1<<30 | div), dto_cfg_addr);
	__raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);

	data = __raw_readl(value_cfg_addr);
	__raw_writel((data | source_sel_bit), value_cfg_addr);
}

// fo = fi*div/2^30
// div = fo*2^30/fi
// dto select the bus, fi is dto's pll fre, fo is bus fre (output fre), here fi is 1188000000
void DTO_Config(struct dto_param *param, unsigned char mmu_flag)
{
	__dto_config(param, mmu_flag, 0);
}

// fo = fi*div/2^30
// div = fo*2^30/fi
// dto select the bus, fi is dto's pll fre, fo is bus fre (output fre), here fi is 1188000000
void DTO_Sec_Config(struct dto_param *param, unsigned char mmu_flag)
{
	__dto_config(param, mmu_flag, 1);
}

void __div_config(struct div_param *param, unsigned char mmu_flag, unsigned char sec_flag)
{
	int i ;
	int data ;

	unsigned int div_cfg_addr;
	unsigned int value_cfg_addr;
	unsigned int config_base;

	unsigned int div_cfg_id;
	unsigned int value_cfg_id;
	unsigned int bit_width;
	unsigned int rstn;
	unsigned int load;
	unsigned int ratio;
	unsigned int value;

	if (!param)
		return;

	if (mmu_flag)
		config_base = CONFIG_BASE_MMU;
	else
		config_base = CONFIG_BASE;

	div_cfg_id   = param->div_cfg_id;
	value_cfg_id = param->value_cfg_id;
	bit_width    = param->bit_width;
	rstn         = param->rstn;
	load         = param->load;
	ratio        = param->ratio;
	value        = param->value;

	if (sec_flag == 0) {
		if (div_cfg_id == 1)
			div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG1;
		else if(div_cfg_id == 2)
			div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG2;
		else if (div_cfg_id == 3)
			div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG3;
		else if (div_cfg_id == 4)
			div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG4;

		if (value_cfg_id == 1)
			value_cfg_addr = config_base + CONFIG_CLOCK_SOURCE1;
		else if (value_cfg_id == 2)
			value_cfg_addr = config_base + CONFIG_CLOCK_SOURCE2;
	} else {
		if (div_cfg_id == 0)
			div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG0_SEC;
		else if(div_cfg_id == 1)
			div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG1_SEC;
		else if (div_cfg_id == 2)
			div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG2_SEC;

		value_cfg_addr = config_base + CONFIG_CLOCK_SOURCE_SEC;
	}

	data = __raw_readl(div_cfg_addr);
	__raw_writel((data & ~(bit_width)) | load | ratio, div_cfg_addr);
	data = __raw_readl(div_cfg_addr);
	__raw_writel((data | rstn), div_cfg_addr);

	data = __raw_readl(value_cfg_addr);
	__raw_writel((data | value), value_cfg_addr);
}

void DIV_Config(struct div_param *param, unsigned char mmu_flag)
{
	__div_config(param, mmu_flag, 0);
}

void DIV_Sec_Config(struct div_param *param, unsigned char mmu_flag)
{
	__div_config(param, mmu_flag, 1);
}

void PLL_Config(unsigned int pll,unsigned int freq)
{
	int i ;
	volatile unsigned int j = 100;
	struct pll_param *p_pll_param_mini;
	unsigned int pll_param_mini_size = 0;

#ifdef CONFIG_24M_XTAL
	p_pll_param_mini = pll_param_mini_24mhz;
	pll_param_mini_size = sizeof(pll_param_mini_24mhz)/sizeof(struct pll_param);
#else
	p_pll_param_mini = pll_param_mini_27mhz;
	pll_param_mini_size = sizeof(pll_param_mini_27mhz)/sizeof(struct pll_param);
#endif

	for (i = 0; i < pll_param_mini_size; i++) {
		if (freq == p_pll_param_mini[i].freq) {
			unsigned int bp = 0 ;
			unsigned int refdiv = p_pll_param_mini[i].refdiv ;
			unsigned int postdiv2 = p_pll_param_mini[i].postdiv2 ;
			unsigned int postdiv1 = p_pll_param_mini[i].postdiv1 ;
			unsigned int FBdiv = p_pll_param_mini[i].FBdiv ;

			__raw_writel((0xf<<27)|(bp<<26)|(refdiv<<20)|(postdiv2<<16)|(postdiv1<<12)|FBdiv, pll);
			while(j--);
			__raw_writel((0x0<<27)|(bp<<26)|(refdiv<<20)|(postdiv2<<16)|(postdiv1<<12)|FBdiv, pll) ;
		}
	}
}

void gx_fpga_board_init(void)
{
	serial_init(1, GX_EXT_CLOCK);
	serial_put_no_mmu('R');
	serial_put_no_mmu('U');
	serial_put_no_mmu('N');
	serial_put_no_mmu('F');
	serial_put_no_mmu('P');
	serial_put_no_mmu('G');
	serial_put_no_mmu('A');
}

unsigned int __get_ddr_fre(void)
{
#ifdef CHIP_BOARD_6631SHXD
	unsigned char ddr_info = otp_read_simple(0x1B8);

	if ((ddr_info & 0xF) == 0x3)
		return 783000000;
	else
		return DDR_FREQUENCY_CONFIG; //667000000
#else
	return DDR_FREQUENCY_CONFIG;
#endif
}

void gx_setup_pll_mini_controller(void)
{
	int i = 0;
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CLOCK_SOURCE1) = 0;
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CLOCK_SOURCE2) = 0;
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CLOCK_SOURCE_SEC) = 0;

	/*print*/
	serial_init(1, GX_EXT_CLOCK);
	serial_put_no_mmu('R');

	// PLL config
	PLL_Config(CONFIG_BASE + CONFIG_DTO_PLL_CONFIG, PLL_DTO_CLK);			//set the DTO is 1188M, from pll
	PLL_Config(CONFIG_BASE + CONFIG_DDR_PLL_CONFIG, __get_ddr_fre());		//set the DDR fre, from pll
	PLL_Config(CONFIG_BASE + CONFIG_CPU_PLL_CONFIG, PLL_CPU_CLK);			//set the CPU fre, from pll

	while ((*(volatile unsigned int*)(CONFIG_BASE+CONFIG_PLL_CONFIG_IN) & 0x7) != 0x7); // wait pll lock

	// DIV config
	for (i = 0; i < sizeof(div_table_mini) / sizeof(struct div_param); i++)
		DIV_Config(&div_table_mini[i], 0);
	for (i = 0; i < sizeof(div_sec_table_mini) / sizeof(struct div_param); i++)
		DIV_Sec_Config(&div_sec_table_mini[i], 0);
	// DTO config
	for (i = 0; i < sizeof(dto_sec_table_mini) / sizeof(struct dto_param); i++)
		DTO_Sec_Config(&dto_sec_table_mini[i], 0);

	//set source
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_SOURCE_SEC) |= (1<<8);//selcect the CPU clock to from xtal to PLL

	/************** DDR clock ****************/
	/*************this is for ddr. ddr have controller and phy and port clock ****************/
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_SOURCE_SEC) |= (1<<17);
#ifdef ENABLE_SECURE_ALIGN
	// spi
	*(volatile unsigned int*)(REG_BASE_SPI + 0x0408) |= (1<<16); //REG_BASE_SPI是新增的地址
#endif

	serial_init(1, GX_UART_CLOCK);
	serial_put_no_mmu('U');
}
