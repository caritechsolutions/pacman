#include <gxhwlib_registers.h>
#include "cpu_config.h"
#include "io.h"

struct pll_mini_param {
	unsigned int dto_clk;
	unsigned int ddr_fre;
	unsigned int cpu_clk;
	unsigned int dvb_clk;
	unsigned int extern_clock_xtal;
	struct div_param *div_table_mini;
	unsigned int div_table_mini_size;
	struct dto_param *dto_table_mini;
	unsigned int dto_table_mini_size;
};

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

/* freq = freq_xtal / refdiv * fbdiv / posdiv1 / posdiv2
 * freq_xtal = 24 MHz or 27 Mhz
 * postdiv1 must >= postdiv2
 * */
#if defined(CONFIG_24M_XTAL)
static struct pll_param pll_param_mini_24mhz[] = {
	/* freq | refdiv | posdiv2 | posdiv1 | fbdiv */
	{1224000000 ,1  ,1  ,1, 51 },  //1224MHz
	{1188000000 ,2  ,1  ,1, 99 },  //1188MHz
	{1080000000 ,1  ,1  ,1, 45 },  //1080MHz
	{900000000  ,1  ,1  ,1, 37 },  //888MHz
	{850000000  ,1  ,1  ,1, 35 },  //840MHz
	{783000000  ,1  ,1  ,1, 33 },  //792MHz
	{750000000  ,1  ,1  ,1, 31 },  //744MHz
	{720000000  ,1  ,1  ,1, 30 },  //720MHz
	{667000000  ,1  ,1  ,2, 56 },  //672MHz
	{660000000  ,1  ,1  ,2, 55 },  //660MHz
	{533000000  ,1  ,1  ,2, 44 },  //528MHz
	{400000000  ,4  ,1  ,3, 200},  //400MHz
};
#endif

#if !defined(CONFIG_24M_XTAL)
static struct pll_param pll_param_mini_27mhz[] = {
	/* freq | refdiv | posdiv2 | posdiv1 | fbdiv */
	{1188000000 ,1  ,1  ,1, 44 },  //1188MHz
	{1080000000 ,1  ,1  ,1, 40 },  //1080MHz
	{900000000  ,1  ,1  ,1, 33 },  //891MHz
	{864000000  ,1  ,1  ,1, 32 },  //864MHz
	{850000000  ,1  ,1  ,1, 31 },  //837MHz
	{783000000  ,1  ,1  ,1, 29 },  //783MHz
	{750000000  ,1  ,1  ,1, 28 },  //756MHz
	{720000000  ,1  ,1  ,1, 27 },  //729MHz
	{667000000  ,1  ,1  ,1, 25 },  //675MHz
	{533000000  ,2  ,1  ,2, 79 },  //533MHz
	{400000000  ,4  ,1  ,2, 119},  //400MHz
};
#endif

static struct dto_param cygnus_dto_table_mini[] = {
	{10 , 2, 0x01745D17 , 1 << 17}    ,  // APB2_0 27MHz
	{11 , 2, 0x01999999 , 1 << 18}    ,  // APB2_2 29.7MHz
	{15 , 1, 0x08000000 , 1 << 2 }    ,  // secure 148.5MHz Speed up random number generation for ddr scrambling
	{19 , 2, 0x10000000 , 1 << 15}    ,  // DWSPI                    297MHz
};

static struct div_param cygnus_div_table_mini[] = {
	{1, 1, 0xf<<8   , 0     , 0     , 2<<8  , 0    }, // ck_core_clk_div        PLL_CPU_CLK/3=224M
	{2, 1, 0x3f     , 0     , 0     , 3     , 0    }, // clock_AHB              PLL_CPU_CLK/4=168M
};

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

// fo = fi*div/2^30
// div = fo*2^30/fi
// dto select the bus, fi is dto's pll fre, fo is bus fre (output fre), here fi is 1188000000
void DTO_Config(struct dto_param *param, unsigned char mmu_flag)
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

	if (mmu_flag)
		config_base = CONFIG_BASE_MMU;
	else
		config_base = CONFIG_BASE;

	if (mmu_flag)
		dto_cfg_addr = 0xa0601000 + 4 * (dto - 1);
	else
		dto_cfg_addr = 0x00601000 + 4 * (dto - 1);

	__raw_writel((0<<31 | 1<<30 | div), dto_cfg_addr);
	__raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);

	if (param->source_sel == 1)
		value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
	else if (param->source_sel == 2)
		value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;

	data = __raw_readl(value_cfg_addr);
	__raw_writel((data | source_sel_bit), value_cfg_addr);
}

void DIV_Config(struct div_param *param, unsigned char mmu_flag)
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

	if (div_cfg_id == 1)
		div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG;
	else if(div_cfg_id == 2)
		div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG2;
	else if (div_cfg_id == 3)
		div_cfg_addr = config_base + CONFIG_CLOCK_DIV_CONFIG3;

	if (value_cfg_id == 1)
		value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
	else if (value_cfg_id == 2)
		value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;

	data = __raw_readl(div_cfg_addr);
	__raw_writel((data & ~(bit_width)) | load | ratio, div_cfg_addr);
	data = __raw_readl(div_cfg_addr);
	__raw_writel((data | rstn), div_cfg_addr);

	data = __raw_readl(value_cfg_addr);
	__raw_writel((data | value), value_cfg_addr);
}

void source_config(unsigned int source_id, unsigned int value, unsigned int mmu_flag)
{
	int data ;
	unsigned int value_cfg_addr;
	unsigned int config_base;

	if (mmu_flag)
		config_base = CONFIG_BASE_MMU;
	else
		config_base = CONFIG_BASE;

	if (source_id == 1)
		value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
	else if (source_id == 2)
		value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;

	data = __raw_readl(value_cfg_addr);
	__raw_writel((data | value), value_cfg_addr);
}

#ifdef CHIP_BOARD_Cygnus_X5
static void stage1_data_reverse(unsigned char *val, unsigned int size)
{
	int i = 0;
	unsigned char tmp_val;
	for(i = 0; i < size / 2; i++) {
		tmp_val = val[i];
		val[i] = val[size - i - 1];
		val[size - i - 1] = tmp_val;
	}
}

static int stage1_strncmp(const char *cs, const char *ct, unsigned int count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
		count--;
	}
	return 0;
}

unsigned char is_6705(void)
{
#define GX_CHIP_NAME_LEN  20
	unsigned char name[GX_CHIP_NAME_LEN];

	unsigned int name_reg = CONFIG_BASE + CONFIG_CHIP_NAME0;
	unsigned int name_len = 12;
	int i = 0;

	for (i = 0; i < GX_CHIP_NAME_LEN; i++)
		name[i] = 0;
	for (i = 0; i < name_len; i++)
		name[i] = *(unsigned char *)(name_reg + i);
	stage1_data_reverse(name, name_len);

	if (stage1_strncmp(name, "6705", 4) == 0)
		return 1;
	return 0;
}

static unsigned int get_ddr_frequency(void)
{
	if (is_6705())
		return 533000000;
	return DDR_FREQUENCY_CONFIG;
}
#elif defined(CHIP_BOARD_6706HX_BBT)
unsigned char is_6706H1(void)
{
	unsigned char ddr_info = otp_read_simple(0x10D);
	if ((ddr_info & 0xF) == 0x3)
		return 1;
	return 0;
}

static unsigned int get_ddr_frequency(void)
{
	if (is_6706H1())
		return 720000000;
	return DDR_FREQUENCY_CONFIG;
}
#else
static unsigned int get_ddr_frequency(void)
{
	return DDR_FREQUENCY_CONFIG;
}
#endif

static void gx_pll_mini_param_init(struct pll_mini_param *pll)
{
	pll->dto_clk = PLL_DTO_CLK;
	pll->ddr_fre = get_ddr_frequency();
	pll->cpu_clk = PLL_CPU_CLK;
	pll->extern_clock_xtal = GX_EXT_CLOCK;
	pll->div_table_mini = cygnus_div_table_mini;
	pll->div_table_mini_size = sizeof(cygnus_div_table_mini) / sizeof(struct div_param);
	pll->dto_table_mini = cygnus_dto_table_mini;
	pll->dto_table_mini_size = sizeof(cygnus_dto_table_mini) / sizeof(struct dto_param);
}

void gx_setup_pll_mini_controller(void)
{
	int i = 0;
	struct pll_mini_param pll;

	gx_pll_mini_param_init(&pll);

	__raw_writel(0, CONFIG_BASE + CONFIG_SOURCE_SEL);
	__raw_writel(0, CONFIG_BASE + CONFIG_SOURCE_SEL2);

	/*print*/
	serial_init(1, pll.extern_clock_xtal);

	serial_put_no_mmu('R');

	// PLL config
	PLL_Config(CONFIG_BASE + CONFIG_PLL1_CONFIG, pll.dto_clk);    // DTO
	PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, pll.ddr_fre);    // DDR
	PLL_Config(CONFIG_BASE + CONFIG_PLL3_CONFIG, pll.cpu_clk);    // CPU

	while (*(volatile unsigned int*)(CONFIG_BASE+CONFIG_PLL_CONFIG_IN) & 0x7 != 0x7); // wait pll lock

	// DIV config
	for (i = 0; i < pll.div_table_mini_size; i++)
		DIV_Config(&pll.div_table_mini[i], 0);

	for (i = 0; i < pll.dto_table_mini_size; i++)
		DTO_Config(&pll.dto_table_mini[i], 0);

	// config cpu source sel to dto cpu
	source_config(2, 1<<20, 0);
	// ddr source sel config
	source_config(2, 1<<21, 0);
	/* reset sdc, jpeg, pp, apu, dvb */
	*(unsigned int*)(CONFIG_BASE + CONFIG_MPEG_CLD_RST_NORM) |= (1 << 0 | 1 << 2 | 1 << 3 | 1 << 20 | 1 << 21);
	SDelay(1);
	*(unsigned int*)(CONFIG_BASE + CONFIG_MPEG_CLD_RST_NORM) &= ~(1 << 0 | 1 << 2 | 1 << 3 | 1 << 20 | 1 << 21);

#ifdef ENABLE_SECURE_ALIGN
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CPU_SDC_AXI_8_62) |= 3; //config cpu 32 bit align
#endif

	serial_init(1, GX_UART_CLOCK);

	serial_put_no_mmu('U');
}
