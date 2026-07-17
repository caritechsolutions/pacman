/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   gx3113c.c
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   The functions of GX3113c in lowpower
*****************************************************************************/

#include "gxcomm.h"
#if GXCHIP_TYPE == GXCHIP_TYPE_GX3113C

#define CONFIG_BASE                    0x0030a000
#define CONFIG_CLOCK_DIV_CONFIG        0x24
#define CONFIG_SOURCE_SEL              0x170
#define CONFIG_SOURCE_SEL2             0x174
#define CONFIG_MPEG_CLK_INHIBIT_NORM   0x18
#define CONFIG_DTO_BASE                0x28

//pin multiplex configure
#define REG_PINMUX_PORTA         0x0030a140
#define REG_PINMUX_PORTB         0x0030a144
#define REG_PINMUX_PORTC         0x0030a148
#define REG_PINMUX_PORTD         0x0030a14c

#define ARRAY_END_FLAG                  0xff
#define ARRAY_END_FLAG_U16              0xffff
#define MULPIN_INVALID_VALUE            0xff
#define NOT_CONFIG                      0xff
#define MP_INV_V                        MULPIN_INVALID_VALUE
#define ARRAY_MAX_COUNT	1000

enum {
	GX_GPIO_INPUT = 0,
	GX_GPIO_OUTPUT
};

enum {
	GX_GPIO_LOW = 0,
	GX_GPIO_HIGH
};

struct mulpin_config_s{
	unsigned short chip_core_num;
	unsigned char sel0;
	unsigned char sel1;
	unsigned char fun;
};

struct gpio_entry_lowpower{
	unsigned char phy_gpio;     /* gpio physical number */
	unsigned char io_mode:1;    /* 0: input, 1: output */
	unsigned char output_value:1;   /* 0: low, 1: high (only for output mode) */
};

struct gpio_register {
	unsigned long rGPIO_EPDDR;
	unsigned long rGPIO_EPDR;
	unsigned long rGPIO_EPBSET;
	unsigned long rGPIO_EPBCLR;
};

struct mulpin_config_s mulpin_table[] = {
 /* chip_core_num | l_bit | h_bit | func_select */    /* package_num | func_list */
	{73 ,  0, MULPIN_INVALID_VALUE, 1},           // NC          | GBIOWRn_PORT00
	{74 ,  1, MULPIN_INVALID_VALUE, 1},           // NC          | GBIORDn_PORT01
	{75 ,  2, MULPIN_INVALID_VALUE, 1},           // NC          | GBOEn_PORT02
	{76 ,  3, MULPIN_INVALID_VALUE, 1},           // NC          | GBWEn_PORT03
	{77 ,  4, MULPIN_INVALID_VALUE, 1},           // NC          | GBCSn1_PORT04
	{78 ,  5, MULPIN_INVALID_VALUE, 1},           // NC          | GBCSn0_PORT05
	{79 ,  6, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA0_PORT06
	{80 ,  7, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA1_PORT07
	{81 ,  8, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA2_PORT08
	{82 ,  9, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA3_PORT09
	{83 , 10, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA4_PORT10
	{84 , 11, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA5_PORT11
	{85 , 12, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA6_PORT12
	{86 , 13, MULPIN_INVALID_VALUE, 1},           // NC          | GBDATA7_PORT13
	{101, 21, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR00_PORT21
	{102, 20, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR01_PORT20
	{103, 19, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR02_PORT19
	{104, 18, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR03_PORT18
	{105, 17, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR04_PORT17
	{106, 29, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR05_PORT29
	{107, 28, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR06_PORT28
	{108, 27, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR07_PORT27
	{109, 26, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR08_PORT26
	{110, 25, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR09_PORT25
	{111, 24, MULPIN_INVALID_VALUE, 1},           // NC          | GBADDR10_PORT24
	{112, 23, 80, 3},                             // NC          | AJTDI_SC1DET_GBADDR11_PORT23
	{113, 22, 81, 3},                             // NC          | AJTDO_SC1PWR_GBADDR12_PORT22
	{115, 16, 82, 3},                             // NC          | AJTMS_SC1DAT_GBADDR13_PORT16
	{116, 15, 83, 3},                             // NC          | AJTCK_SC1CLK_GBADDR14_PORT15
	{117, 14, 84, 3},                             // NC          | AJTRST_SC1RST_GBADDR15_PORT14
	{118, 32, MULPIN_INVALID_VALUE, 1},           // NC          | DBGTDI_PORT32
	{119, 33, MULPIN_INVALID_VALUE, 1},           // NC          | DBGTDO_PORT33
	{120, 34, MULPIN_INVALID_VALUE, 1},           // NC          | DBGTMS_PORT34
	{121, 35, MULPIN_INVALID_VALUE, 1},           // NC          | DBGTCK_PORT35
	{122, 36, MULPIN_INVALID_VALUE, 1},           // NC          | DBGTRST_PORT36
	{124, 37, MULPIN_INVALID_VALUE, 1},           // NC          | TS1VALID_PORT37
	{125, 38, MULPIN_INVALID_VALUE, 1},           // NC          | TS1DAT0_PORT38
	{126, 39, 87, 1},                             // NC          | ScanPwdClk_PORT39_TS1DTA1
	{127, 40, 87, 1},                             // NC          | ScanPwdDat_PORT40_TS1DAT2
	{128, 41, 87, 1},                             // NC          | ScanPwdSel_PORT41_TS1DAT3
	{129, 42, 89, 1},                             // NC          | UARTTXICAM1_PORT42_TS1DAT4
	{130, 43, 89, 1},                             // NC          | UARTRXICAM1_PORT43_TS1DAT5
	{131, 44, 89, 1},                             // NC          | UARTTXICAM2_PORT44_TS1DAT6
	{132, 45, 89, 1},                             // NC          | UARTRXICAM2_PORT45_TS1DAT7
	{133, 46, 89, 1},                             // NC          | UARTTXICAM3_PORT46_TS1CLK
	{134, 47, 89, 1},                             // NC          | UARTRXICAM3_PORT47_TS1SYNC
	{141, 48, MULPIN_INVALID_VALUE, 1},           // NC          | TSIVALID_PORT48
	{142, 49, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT0_PORT49
	{143, 50, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT1_PORT50
	{144, 51, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT2_PORT51
	{145, 52, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT3_PORT52
	{146, 53, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT4_PORT53
	{147, 54, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT5_PORT54
	{148, 55, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT6_PORT55
	{149, 56, MULPIN_INVALID_VALUE, 1},           // NC          | TSIDAT7_PORT56
	{150, 57, MULPIN_INVALID_VALUE, 1},           // NC          | TSICLK_PORT57
	{151, 58, MULPIN_INVALID_VALUE, 1},           // NC          | TSISYNC_PORT58
	{152, 59, MULPIN_INVALID_VALUE, 1},           // NC          | IIC1SCLT_PORT59
	{153, 60, MULPIN_INVALID_VALUE, 1},           // NC          | IIC1SDAT_PORT60
	{156, 61, MULPIN_INVALID_VALUE, 1},           // NC          | AGC_PORT61
	{221, 62, 79, 1},                             // NC          | SPDIF_PORT62_OtpClockOsc
	{222, 63, 85, 0},                             // NC          | UARTTX0_PORT63_AUARTTX
	{223, 64, MULPIN_INVALID_VALUE, 0},           // NC          | UARTRX0_PORT64
	{224, 30, MULPIN_INVALID_VALUE, 1},           // NC          | SPIHOLD_PORT30
	{225, 31, MULPIN_INVALID_VALUE, 1},           // NC          | SPIWP_PORT31
	{230, 65, 86, 1},                             // NC          | UARTTXICAM_PORT65_UARTTX1_AUARTTX
	{231, 66, 88, 1},                             // NC          | UARTRXICAM_PORT66_UARTRX1_AGC1
	{232, 70, MULPIN_INVALID_VALUE, 1},           // NC          | SCI1CMDVCC_PORT70
	{233, 74, MULPIN_INVALID_VALUE, 1},           // NC          | SCI13V5V_PORT74
	{234, 68, MULPIN_INVALID_VALUE, 1},           // NC          | SCI1DATC4_PORT68
	{235, 72, MULPIN_INVALID_VALUE, 1},           // NC          | SCI1CLK_PORT72
	{236, 69, MULPIN_INVALID_VALUE, 1},           // NC          | SCI1DET_PORT69
	{237, 73, MULPIN_INVALID_VALUE, 1},           // NC          | SCI1RST_PORT73
	{238, 67, MULPIN_INVALID_VALUE, 1},           // NC          | SCI1DATC8_PORT67
	{239, 71, MULPIN_INVALID_VALUE, 1},           // NC          | SCI1DATC7_PORT71

	{ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG}
};

struct gpio_entry_lowpower g_gpio_table[] = {
	{0, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{1, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{2, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{3, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{4, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{5, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{6, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{7, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{8, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{9, 		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{10,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{11,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{12,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{13,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{14,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{15,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{16,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{17,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{18,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{19,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{20,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{21,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{22,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{23,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{24,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{25,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{26,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{27,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{28,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{29,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{30,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{31,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{32,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{33,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{34,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{35,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{36,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{37,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{38,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{39,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{40,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{41,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{42,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{43,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{44,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{45,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{46,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{47,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{48,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{49,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{50,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{51,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{52,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{53,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{54,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{55,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{56,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{57,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{58,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{59,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{60,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{61,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{62,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{63,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{64,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{65,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{66,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{67,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{68,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{69,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{70,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{71,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{72,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{73,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{74,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{75,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},

	{ARRAY_END_FLAG, 0, 0}
};
// fo = fi*div/2^30
// div = fo*2^30/fi
/*static void DTO_Config(unsigned int dto, unsigned int div)
{
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31);
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = 0;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31);
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|div;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|(1<<30)|div;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|(1<<30)|div;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|div;
}*/

/*****************************************************************************
 * Function    : GX3201_Delay
 * Description : Delay some time
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
/*static void GX3201_Delay(void)
{
	U32 Index;
	for(Index = 0;Index < 10000;Index++);
}*/

/*****************************************************************************
 * Function    : GX3201_ClosePll
 * Description : Close all the clock except the APB clock and cpu clock
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
 
void GX3113c_ClosePll(void)
{
//	return ;
	
#if 0
	// config multi pin
	*(volatile unsigned int*)0x0030a140 = 0;
	*(volatile unsigned int*)0x0030a144 = 0;
	*(volatile unsigned int*)0x0030a148 = 0;
#endif


//	*(volatile unsigned int *)0x04900084 = 0;		// Video DAC
	*(volatile unsigned int *)0x04800134 = 0xffffffff;		// Video DAC
	*(volatile unsigned int *)0x0030a104 |= 1<<10;	// USB PHY1
	*(volatile unsigned int *)0x0030a200 = 0;		// DVB ADC
	
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) &= ~((0xff<<0)|(0x1ff<<12)|(0xf<<24));
	//*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL2) &= ~((0x3f<<0)|(0xf<<12));
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) = 0x00;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL2) = 0x00;

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_CLK_INHIBIT_NORM) |= 0xffffffff&(~(1<<15))&(~(1<<16)); 

	/* must select cpu from xtal before close dto */
	*(volatile unsigned int *)0x0030a0c0 &=~ (1<<15); // close DTO PLL
	*(volatile unsigned int *)0x0030a0c4 &=~ (1<<15);  // VID PLL
	*(volatile unsigned int *)(CONFIG_BASE) |= 1;      // reset ddr
}

#if DEBUG_EN > 0

/*****************************************************************************
 * Function    : GX3201_ConfigPll
 * Description : Set the frequency of AHB to 108MHz
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3113c_ConfigPll(void)
{
}

#endif

/*****************************************************************************
 * Function    : GX3201_CloseModule
 * Description : Close the modules such as WDT, INTC and TV
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3113c_CloseModule(void)
{
#if 0
	//cold reset modle
	*(volatile unsigned int *)0x0050a000 = 0xfffffffe;

	//*(volatile unsigned int *)0x0050a004 = ((1 << 28) | (1 << 29));
	//*(volatile unsigned int *)0x0050a010 = ((1 << 28) | (1 << 29));
	GX3201_Delay();
	//*(volatile unsigned int *)0x0050a014 = ((1 << 28) | (1 << 29));
	//*(volatile unsigned int *)0x0050a008 = ((1 << 28) | (1 << 29));

	//Close WDT
	rWDT_CTRL = 0;

	//Close INTC
	rINTC_NENCLR  = 0xFFFFFFFF;
	rINTC_NENCLR2 = 0xFFFFFFFF;
	rINTC_FENCLR  = 0xFFFFFFFF;
	rINTC_FENCLR2 = 0xFFFFFFFF;

	//Close Audio DAC cold reset
	rVIDEOOUT_CTL |= (1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19);

	//set apb clock
	SOURCE_SEL &= ~(1<<11);

	//set cpu clock
	SOURCE_SEL &= ~(1<<10);

	//set cpu clock
	SOURCE_SEL &= ~(1<<13);

	unsigned int i;
	for(i=0;i<0xfff;i++);

	//PLL power down
	rPLL1_CONFIG &= ~(1<<15);	//PLL1 power down
	rPLL2_CONFIG &= ~(1<<15);	//PLL2 power down
	rPLL3_CONFIG &= ~(1<<15);	//PLL3 power down
	rPLL4_CONFIG &= ~(1<<15);	//PLL4 power down
	rPLL5_CONFIG &= ~(1<<15);	//PLL5 power down

	//dll power down
	rDLL_CONFIG &= ~(1<<5);

	//close USB clock input
	rDIV_CONFIG  |=  (3<<20);
	rDTO5_CONFIG &= ~(1<<31);
	rDTO6_CONFIG &= ~(1<<31);

	//DA power down
	rCLK_CONFIG   &= ~(1<<22);
	rVIDEOOUT_CTL |=  (1<<19);
	rCLK_CONFIG   |=  (1<<22);
#endif
}

/*****************************************************************************
 * Function    : GX3201_SetGpio
 * Description : Set the gpio
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3113c_SetGpio(U64 GpioMask,U64 GpioData)
{
#if 0
	*(volatile unsigned int *)0xa030a134 &= ~(0x03 << 10);
	*(volatile unsigned int *)0xa030a134 |= (0x03 << 10);
	*(volatile unsigned int *)0xa0306000 |= (1 << 5);
	*(volatile unsigned int *)0xa030600c |= (1 << 5);
#endif
}

static void set_mulpin_fun(int sel, int fun)
{
	if(sel < 32){
		REG_SET_CLR_BIT(REG_PINMUX_PORTA, sel, fun);
	}else if (sel < 64){
		REG_SET_CLR_BIT(REG_PINMUX_PORTB, sel - 32, fun);
	}else if (sel < 96){
		REG_SET_CLR_BIT(REG_PINMUX_PORTC, sel - 64, fun);
	}
}

void Config_Pin(void)
{
	volatile int i = 0 ,array_count = 0;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if(mulpin_table[i].chip_core_num == ARRAY_END_FLAG_U16) {
			array_count = i;
			break;
		}
	}

	for(i = 0; i < array_count; i++){
		if(mulpin_table[i].fun != NOT_CONFIG){
			set_mulpin_fun(mulpin_table[i].sel0, mulpin_table[i].fun & 0x1);
			set_mulpin_fun(mulpin_table[i].sel1, (mulpin_table[i].fun & 0x2) >> 1);
		}
	}
}

void Config_Gpio(void)
{
	volatile unsigned int i 		= 0;
	volatile unsigned int entry_num			= 0;
	volatile unsigned int offs 			    = 0;		/* 0~31 */
	volatile struct gpio_register *gx_gpio_register = NULL;

	for(i = 0; i < ARRAY_MAX_COUNT; i++){
		if(g_gpio_table[i].phy_gpio == ARRAY_END_FLAG){
			entry_num = i;
			break;
		}
	}

	for (i = 0; i < entry_num; i++)
	{
		/* initialize gpio */

		if (g_gpio_table[i].phy_gpio < 32) {
			gx_gpio_register = (volatile struct gpio_register*)(EPORT_BASE_ADDR);
			offs = g_gpio_table[i].phy_gpio;
		} else if(g_gpio_table[i].phy_gpio < 64) {
			gx_gpio_register = (volatile struct gpio_register*)(EPORT2_BASE_ADDR);
			offs = g_gpio_table[i].phy_gpio - 32;
		} else if(g_gpio_table[i].phy_gpio < 96) {
			gx_gpio_register = (volatile struct gpio_register*)(EPORT3_BASE_ADDR);
			offs = g_gpio_table[i].phy_gpio - 64;
		} else {
			GXLOWPOWER_TinyPutStr("\ngpio parameters error\n");
		}

		if (g_gpio_table[i].io_mode == 0)
		{
			gx_gpio_register->rGPIO_EPDDR &= ~(1 << offs);
		} else {
			gx_gpio_register->rGPIO_EPDDR |= 1 << offs;
			if (g_gpio_table[i].output_value)
				gx_gpio_register->rGPIO_EPBSET = 1 << offs;
			else
				gx_gpio_register->rGPIO_EPBCLR = 1 << offs;
		}
	}
}

void GX3113c_Config_Pin_Gpio(void)
{
	Config_Gpio();
	Config_Pin();
	*(volatile unsigned int*)(SPI_BASE_ADDR) |= (0x1<<10)&(~(0x1<<17));
}

gxlowpower_func_t gxlowpower_func =
{
	.ClosePll     = GX3113c_ClosePll,
	.CloseModule  = GX3113c_CloseModule,
	.SetGpio      = GX3113c_SetGpio,
	.ConfigPin    = GX3113c_Config_Pin_Gpio,
#if DEBUG_EN > 0
	.ConfigPll    = GX3113c_ConfigPll,
#else
	.ConfigPll    = NULL,
#endif
};

#endif

