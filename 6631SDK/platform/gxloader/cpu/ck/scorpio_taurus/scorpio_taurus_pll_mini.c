/** 
 ***********************************************************************
 *                          CONFIDENTIAL                               *
 *        Hangzhou Nationalchip Science and Technology Co. Ltd.        *
 *                       All Rights Reserved.                          *
 ***********************************************************************
 * @file pll_mini.c
 * @brief 
 * @author leo
 * @date 2023-11-13
 */
#include <gxhwlib_registers.h>
#include <cpu_config.h>
#include <io.h>
#ifdef CONFIG_EXTERN_PARAM
#include "board_param.h"
#endif

struct pll_mini_param {
	unsigned int dto_clk;
	unsigned int ddr_fre;
	unsigned int dvb_clk;
	unsigned int extern_clock_xtal;
	struct div_param *div_table_mini;
	unsigned int div_table_mini_size;
	struct dto_param *dto_table_mini;
	unsigned int dto_table_mini_size;
};

extern void serial_put_no_mmu(char);
extern void serial_init(int, int);

static unsigned int chip_id = 0xFFFF;
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
/* 24MHz pll mini used for gemini and taurus */
#if defined(CONFIG_24M_XTAL) || defined(CONFIG_EXTERN_PARAM)
static struct pll_param pll_param_mini_24mhz[] = {
	/* freq | refdiv | posdiv2 | posdiv1 | fbdiv */
	{1224000000 ,1  ,1  ,1, 51 },  //1224MHz
	{1200000000 ,1  ,1  ,1, 50 },  //1200MHz
	{1188000000 ,2  ,1  ,1, 99 },  //1188MHz
	{1152000000 ,1  ,1  ,1, 48 },  //1152MHz
	{1080000000 ,1  ,1  ,1, 45 },  //1080MHz
	{783000000  ,1  ,1  ,1, 33 },  //792MHz
	{667000000  ,1  ,1  ,1, 28 },  //672MHz
	{648000000  ,1  ,1  ,1, 27 },  //648MHz
	{540000000  ,1  ,1  ,2, 45 },  //540MHz
	{533000000  ,1  ,1  ,2, 44 },  //528MHz
	{528000000  ,1  ,1  ,1, 22 },  //528MHz
	{408000000  ,1  ,1  ,2, 34 },  //408MHz
	{400000000  ,4  ,1  ,2, 134},  //400MHz
};
#endif

/* 27MHz pll mini used for gemini and taurus */
#if !defined(CONFIG_24M_XTAL) || defined(CONFIG_EXTERN_PARAM)
static struct pll_param pll_param_mini_27mhz[] = {
	/* freq | refdiv | posdiv2 | posdiv1 | fbdiv */
	{1188000000 ,1  ,1  ,1, 44 },  //1188MHz
	{1152000000 ,3  ,1  ,1, 128},  //1152MHz
	{1080000000 ,1  ,1  ,1, 40 },  //1080MHz
	{783000000  ,1  ,1  ,1, 29 },  //783MHz
	{667000000  ,1  ,1  ,1, 25 },  //675MHz
	{540000000  ,1  ,1  ,2, 40 },  //540MHz
	{533000000  ,2  ,1  ,2, 79 },  //533MHz
	{405000000  ,1  ,1  ,2, 30 },  //405MHz
	{400000000  ,4  ,1  ,2, 119},  //400MHz
};
#endif

/* taurus dto table mini and div table mini*/
#if defined(CONFIG_EXTERN_PARAM) || defined(CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
static struct dto_param taurus_dto_table_mini[] = {
	{10 , 1, 0x01745D17 , 1 << 19},		// APB2_0         27 MHz
	{11 , 1, 0x01999999 , 1 << 20},		// APB2_2         29.7MHz
	{12 , 1, 0x20000000 , 1 << 24},		// DTO_CPU        594MHz
	{13 , 1, 0x05555555 , 1 << 21},		// DTO_APB2_4     99MHz
};

static struct div_param taurus_div_table_mini[] = {
	{2, 2, 0xf      , 0     , 0     , 2     ,    0 }, // arm_core_clk_div	198M
	{3, 2, 0x7<<3   , 0     , 0     , 4<<3  ,    0 }, // clock_AHB		118.8M
};
#endif

/* scorpio dto table mini and div table mini*/
#if defined(CONFIG_EXTERN_PARAM) || defined(CONFIG_ARCH_CKMMU_SCORPIO) || defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
static struct dto_param scorpio_dto_table_mini[] = {
  {10 , 1, 0x01745D17 , 1 << 19}    ,  // APB2_0         27MHz
	{11 , 1, 0x0199999a , 1 << 20}    ,  // APB2_2         29.7MHz
	{12 , 1, 0x20000000 , 1 << 24}    ,  // DTO_CPU        594MHz
	{13 , 1, 0x0aaaaaab , 1 << 21}    ,  // DTO_APB2_4     198MHz
};

static struct div_param scorpio_div_table_mini[] = {
	{2, 1, 0xf   , 0     , 0     , 0x03  , 1<<25    }, // cpu_clk_div_ratio_core_config       198M
	{2, 1, 0xf<<4     , 0     , 0     , 0x7<<4     , 1<<26}, // ck_clk_div_ratio_core              118.8M
	{5, 2, 0x3f<<24     , 0x3<<30     , 0     , 0x8<<24     , 1<<19 }, // scpu_clk_div_config            132MHZ
	{7, 3, 0x3f     , 0x3<<6     , 0     , 0x5     , 1<<6 }, // ssi_clk_div_ratio              198MHZ
};
#endif

void PLL_Config(unsigned int pll,unsigned int freq)
{
	int i ;
	volatile unsigned int j = 100;
	struct pll_param *p_pll_param_mini;
	unsigned int pll_param_mini_size = 0, data;

#ifdef CONFIG_EXTERN_PARAM
	if (board_param->extern_clock_xtal == 24000000) {
		p_pll_param_mini = pll_param_mini_24mhz;
		pll_param_mini_size = sizeof(pll_param_mini_24mhz)/sizeof(struct pll_param);
	}
	else {
		p_pll_param_mini = pll_param_mini_27mhz;
		pll_param_mini_size = sizeof(pll_param_mini_27mhz)/sizeof(struct pll_param);
	}
#else
#ifdef CONFIG_24M_XTAL
	p_pll_param_mini = pll_param_mini_24mhz;
	pll_param_mini_size = sizeof(pll_param_mini_24mhz)/sizeof(struct pll_param);
#else
	p_pll_param_mini = pll_param_mini_27mhz;
	pll_param_mini_size = sizeof(pll_param_mini_27mhz)/sizeof(struct pll_param);
#endif
#endif

	for (i = 0; i < pll_param_mini_size; i++) {
		if (freq == p_pll_param_mini[i].freq) {
			unsigned int bp = 1 ;
			unsigned int refdiv = p_pll_param_mini[i].refdiv ;
			unsigned int postdiv2 = p_pll_param_mini[i].postdiv2 ;
			unsigned int postdiv1 = p_pll_param_mini[i].postdiv1 ;
			unsigned int FBdiv = p_pll_param_mini[i].FBdiv ;
      __raw_writel((1<<26|1<<27|1<<28|1<<29|1<<30), pll);
      data = __raw_readl(pll);
			__raw_writel(((refdiv<<20)|(postdiv2<<16)|(postdiv1<<12)|FBdiv|data), pll);
      data = __raw_readl(pll);
			while(j--);
			__raw_writel(data&0x03ffffff, pll) ;
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

	/* Gemini dto base = 0x00601000
	 * Taurus dto base = CONFIG_BASE + 0x28 = 0x0030a028
	 */
#ifdef CONFIG_EXTERN_PARAM
	dto_cfg_addr = config_base + 0x28 + 4 * (dto - 1);

#else
	dto_cfg_addr = config_base + 0x28 + 4 * (dto - 1);
#endif

#ifdef CONFIG_EXTERN_PARAM
  if (board_param->chip == TAURUS_CHIP_ID) {
    __raw_writel((0<<31 | 1<<30 | div), dto_cfg_addr);
    __raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);

		if (param->source_sel == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
		else if (param->source_sel == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
	} else {
    __raw_writel((0<<31 | 0<<30 | div), dto_cfg_addr);
    __raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);

		if (param->source_sel == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE1_SEL;
		else if (param->source_sel == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
	}
#else
#ifdef CONFIG_ARCH_CKMMU_TAURUS
    __raw_writel((0<<31 | 1<<30 | div), dto_cfg_addr);
    __raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);
		if (param->source_sel == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
		else if (param->source_sel == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
  if (chip_id == 0x6616) {
    __raw_writel((0<<31 | 1<<30 | div), dto_cfg_addr);
    __raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);

		if (param->source_sel == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
		else if (param->source_sel == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
	} else {
    __raw_writel((0<<31 | 0<<30 | div), dto_cfg_addr);
    __raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);

		if (param->source_sel == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE1_SEL;
		else if (param->source_sel == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
	}
#else
    __raw_writel((0<<31 | 0<<30 | div), dto_cfg_addr);
    __raw_writel((1<<31 | 1<<30 | div), dto_cfg_addr);
		if (param->source_sel == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE1_SEL;
		else if (param->source_sel == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE2_SEL;
#endif
#endif

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
	else if (div_cfg_id == 5)
		div_cfg_addr = config_base + CONFIG_DIV5_CONFIG;
	else if (div_cfg_id == 7)
		div_cfg_addr = config_base + CONFIG_DIV7_CONFIG;

#ifdef CONFIG_EXTERN_PARAM
  if (board_param->chip == TAURUS_CHIP_ID) {
		if (value_cfg_id == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
		else if (value_cfg_id == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
		else if (value_cfg_id == 3)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
	} else {
		if (value_cfg_id == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE1_SEL;
		else if (value_cfg_id == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE2_SEL;
		else if (value_cfg_id == 3)
			value_cfg_addr = config_base + CONFIG_SOURCE3_SEL;
	}
#else
#ifdef CONFIG_ARCH_CKMMU_TAURUS
		if (value_cfg_id == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
		else if (value_cfg_id == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
		else if (value_cfg_id == 3)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
  if (chip_id == 0x6616) {
		if (value_cfg_id == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL;
		else if (value_cfg_id == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
		else if (value_cfg_id == 3)
			value_cfg_addr = config_base + CONFIG_SOURCE_SEL2;
	} else {
		if (value_cfg_id == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE1_SEL;
		else if (value_cfg_id == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE2_SEL;
		else if (value_cfg_id == 3)
			value_cfg_addr = config_base + CONFIG_SOURCE3_SEL;
	}
#else
		if (value_cfg_id == 1)
			value_cfg_addr = config_base + CONFIG_SOURCE1_SEL;
		else if (value_cfg_id == 2)
			value_cfg_addr = config_base + CONFIG_SOURCE2_SEL;
		else if (value_cfg_id == 3)
			value_cfg_addr = config_base + CONFIG_SOURCE3_SEL;
#endif

#endif

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


static void gx_pll_mini_param_init(struct pll_mini_param *pll)
{
#ifdef CONFIG_EXTERN_PARAM
	pll->dto_clk = PLL_DTO_CLK;
	pll->ddr_fre = board_param->ddr_frequency;
	pll->extern_clock_xtal = board_param->extern_clock_xtal;
	if (board_param->chip == TAURUS_CHIP_ID) {
	  if (board_param->extern_clock_xtal == 24000000
		    && (OTP_GET_CFG_DATA_NOMMU(15, 1))) //dvbc
		  pll->dvb_clk = 1200000000;
	  else
		  pll->dvb_clk = 1188000000;
	  pll->div_table_mini = taurus_div_table_mini;
	  pll->div_table_mini_size = sizeof(taurus_div_table_mini) / sizeof(struct div_param);
	  pll->dto_table_mini = taurus_dto_table_mini;
	  pll->dto_table_mini_size = sizeof(taurus_dto_table_mini) / sizeof(struct dto_param);
	} else {
		pll->dvb_clk = 540000000;
	  pll->div_table_mini = scorpio_div_table_mini;
	  pll->div_table_mini_size = sizeof(scorpio_div_table_mini) / sizeof(struct div_param);
	  pll->dto_table_mini = scorpio_dto_table_mini;
	  pll->dto_table_mini_size = sizeof(scorpio_dto_table_mini) / sizeof(struct dto_param);
	}
#else
#ifdef DDR_SEC_PAR_INCLUDE
#ifdef CONFIG_ARCH_CKMMU_TAURUS
	unsigned char ddr_info = DDR_INFO_GET_NO_MMU();
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	unsigned char ddr_info = otp_read_simple(0x10D);
#endif
#endif
	pll->dto_clk = PLL_DTO_CLK;
	pll->ddr_fre = DDR_FREQUENCY_CONFIG;
#ifdef DDR_SEC_PAR_INCLUDE
#ifdef CONFIG_ARCH_CKMMU_TAURUS
	/* only taurus DDR3 1GB should do follow work */
#ifdef CONFIG_24M_XTAL
	if ((ddr_info & 0xF) == 2) {
		if (pll->ddr_fre == 400000000)
			pll->ddr_fre = 408000000;
	}
#else
	if ((ddr_info & 0xF) == 2) {
		if (pll->ddr_fre == 533000000)
			pll->ddr_fre = 540000000;
		else if (pll->ddr_fre == 400000000)
			pll->ddr_fre = 405000000;
	}
#endif
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	/* only taurus DDR3 1GB should do follow work */
	if (chip_id == 0x6616) {
#ifdef CONFIG_24M_XTAL
		if ((ddr_info & 0xF) == 2) {
			if (pll->ddr_fre == 400000000)
				pll->ddr_fre = 408000000;
		}
#else
		if ((ddr_info & 0xF) == 2) {
			if (pll->ddr_fre == 533000000)
				pll->ddr_fre = 540000000;
			else if (pll->ddr_fre == 400000000)
				pll->ddr_fre = 405000000;
		}
#endif
	}
#endif
#endif
	pll->extern_clock_xtal = GX_EXT_CLOCK;
#ifdef CONFIG_ARCH_CKMMU_TAURUS
#if defined(DVB_CHANNEL_C) && defined(CONFIG_24M_XTAL)
	pll->dvb_clk = 1200000000;
#else
	pll->dvb_clk = 1188000000;
#endif
	pll->div_table_mini = taurus_div_table_mini;
	pll->div_table_mini_size = sizeof(taurus_div_table_mini) / sizeof(struct div_param);
	pll->dto_table_mini = taurus_dto_table_mini;
	pll->dto_table_mini_size = sizeof(taurus_dto_table_mini) / sizeof(struct dto_param);
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	if (chip_id == 0x6616) {
#if defined(DVB_CHANNEL_C) && defined(CONFIG_24M_XTAL)
		pll->dvb_clk = 1200000000;
#else
		pll->dvb_clk = 1188000000;
#endif
		pll->div_table_mini = taurus_div_table_mini;
		pll->div_table_mini_size = sizeof(taurus_div_table_mini) / sizeof(struct div_param);
		pll->dto_table_mini = taurus_dto_table_mini;
		pll->dto_table_mini_size = sizeof(taurus_dto_table_mini) / sizeof(struct dto_param);
	} else {
		pll->dvb_clk = 540000000;
		pll->div_table_mini = scorpio_div_table_mini;
		pll->div_table_mini_size = sizeof(scorpio_div_table_mini) / sizeof(struct div_param);
		pll->dto_table_mini = scorpio_dto_table_mini;
		pll->dto_table_mini_size = sizeof(scorpio_dto_table_mini) / sizeof(struct dto_param);
	}
#else
	pll->dvb_clk = 540000000;
	pll->div_table_mini = scorpio_div_table_mini;
	pll->div_table_mini_size = sizeof(scorpio_div_table_mini) / sizeof(struct div_param);
	pll->dto_table_mini = scorpio_dto_table_mini;
	pll->dto_table_mini_size = sizeof(scorpio_dto_table_mini) / sizeof(struct dto_param);
#endif
#endif
}

extern unsigned int chip_probe(unsigned int mmu_enable);

void gx_setup_pll_mini_controller(void)
{
	int i = 0;
	volatile unsigned int j = 1;
	struct pll_mini_param pll;

#ifdef CONFIG_ARCH_CKMMU_SCORPIO_TAURUS
	chip_id = chip_probe(0);
#endif

	gx_pll_mini_param_init(&pll);

	if (chip_id == 0x3113)
		__raw_writel(0, CONFIG_BASE + CONFIG_SOURCE1_SEL);

	__raw_writel(0, CONFIG_BASE + CONFIG_SOURCE_SEL);
	__raw_writel(0, CONFIG_BASE + CONFIG_SOURCE_SEL2);

#ifdef CONFIG_EXTERN_PARAM
	  // PLL config
  PLL_Config(CONFIG_BASE + CONFIG_PLL1_CONFIG, pll.dto_clk);		// DTO
	if (board_param->chip == TAURUS_CHIP_ID) {
	  PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, pll.ddr_fre);		// DDR
	} else { // scorpio
    if (pll.ddr_fre == 533000000) {
	    PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, 528000000);		// DDR
	  } else {
	    PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, 648000000);		// DDR
    }
	}
	PLL_Config(CONFIG_BASE + CONFIG_PLL3_CONFIG, pll.dvb_clk);		// DVB
#else
	  // PLL config
  PLL_Config(CONFIG_BASE + CONFIG_PLL1_CONFIG, pll.dto_clk);		// DTO
#ifdef CONFIG_ARCH_CKMMU_TAURUS
  PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, pll.ddr_fre);		// DDR
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
  if (chip_id == 0x6616) {
  	  PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, pll.ddr_fre);// DDR
  } else {
    if (pll.ddr_fre == 533000000) {
	    PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, 528000000);		// DDR
	} else {
	    PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, 648000000);		// DDR
    }
  }
#else
    if (pll.ddr_fre == 533000000) {
	    PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, 528000000);		// DDR
	  } else {
	    PLL_Config(CONFIG_BASE + CONFIG_PLL2_CONFIG, 648000000);		// DDR
    }
#endif
	PLL_Config(CONFIG_BASE + CONFIG_PLL3_CONFIG, pll.dvb_clk);		// DVB

#endif
	while (*(volatile unsigned int*)(CONFIG_BASE+CONFIG_PLL_CONFIG_IN) != 0x7); // wait pll lock

	/*print*/
	serial_init(1, pll.extern_clock_xtal);

	serial_put_no_mmu('R');

	// DIV config
	for (i = 0; i < pll.div_table_mini_size; i++)
		DIV_Config(&pll.div_table_mini[i], 0);

	for (i = 0; i < pll.dto_table_mini_size; i++)
		DTO_Config(&pll.dto_table_mini[i], 0);

	// ddr source sel config
#ifdef CONFIG_EXTERN_PARAM
	if (board_param->chip == TAURUS_CHIP_ID)
		source_config(1, 1<<25, 0);
	else { /* scorpio */
	  //clock_scpu_en clock_APB2_4_en
	  *(volatile unsigned int*)(CONFIG_BASE + CLOCK_GATE_CONFIG2_REG) |= (1<<12 | 1<<11);
	  //clock_scpu_pre_en clock_apb2_4_pre_en
	  *(volatile unsigned int*)(CONFIG_BASE + CLOCK_GATE_REG3) |= (1<<19 | 1<<16);
	}
#else
#ifdef CONFIG_ARCH_CKMMU_TAURUS
	source_config(1, 1<<25, 0);
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	if (chip_id == 0x6616)
		source_config(1, 1<<25, 0);
	else { /* scorpio */
	  //clock_scpu_en clock_APB2_4_en
	  *(volatile unsigned int*)(CONFIG_BASE + CLOCK_GATE_CONFIG2_REG) |= (1<<12 | 1<<11);
	  //clock_scpu_pre_en clock_apb2_4_pre_en
	  *(volatile unsigned int*)(CONFIG_BASE + CLOCK_GATE_REG3) |= (1<<19 | 1<<16);
	}
#else /* scorpio */
	  //clock_scpu_en clock_APB2_4_en
	  *(volatile unsigned int*)(CONFIG_BASE + CLOCK_GATE_CONFIG2_REG) |= (1<<12 | 1<<11);
	  //clock_scpu_pre_en clock_apb2_4_pre_en
	  *(volatile unsigned int*)(CONFIG_BASE + CLOCK_GATE_REG3) |= (1<<19 | 1<<16);
#endif
#endif

  SDelay(20);

#ifdef ENABLE_SECURE_ALIGN
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_CPU_SDC_AXI_8_62) |= 3; //config cpu 32 bit align
#endif

	serial_init(1, GX_UART_CLOCK);

	serial_put_no_mmu('U');
}

