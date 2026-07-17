/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   gx3211.c
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   The functions of GX3211 in lowpower
*****************************************************************************/

#include "gxcomm.h"
#if GXCHIP_TYPE == GXCHIP_TYPE_GX3211

#define CONFIG_BASE                    0x0030a000
#define CONFIG_AUDIO_CODEC_CONTROL     0x1A4
#define CONFIG_ADC                     0x200
#define CONFIG_DAC                     0x1F0
#define CONFIG_USB_CONFIG              0x100
#define CONFIG_USB1_CONFIG             0x104
#define CONFIG_USB2_CONFIG             0x108
#define CONFIG_USB3_CONFIG             0x10c
#define CONFIG_EPHY_CONFIG             0x114
#define CONFIG_SOURCE_SEL              0x170
#define CONFIG_CLOCK_DIV_CONFIG2       0x178
#define CONFIG_DRAM_CONFIG0            0x120
#define CONFIG_MPEG_CLK_INHIBIT_1SET   0x1c
#define CONFIG_MPEG_CLK_INHIBIT2_1SET  0x6c
#define PLL_CPU_CONFIG_BASE            0xc0
#define PLL_DTO_CONFIG_BASE            0xc8
#define PLL_DVB_CONFIG_BASE            0xd0
#define PLL_DDR_CONFIG_BASE            0xe0
#define CONFIG_DTO_BASE                0x28

//pin multiplex configure
#define REG_PINMUX_PORTA         0x0030a13c
#define REG_PINMUX_PORTB         0x0030a140
#define REG_PINMUX_PORTC         0x0030a144
#define REG_PINMUX_PORTD         0x0030a148
#define REG_PINMUX_PORTE         0x0030a14c

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

struct mulpin_config_s{
	unsigned short chip_core_num;
	unsigned char sel0;
	unsigned char sel1;
	unsigned char sel2;
	unsigned char fun;
};

struct mulpin_config_s mulpin_table[] = {
                                            /* chip_package: generic */
 /* chip_core_num | 0_bit | 1_bit |  2_bit  |func_sel*/ /* package_num | func_list */
        {82,        70,     102,    MP_INV_V,  1},           // NC          | SC2CMDVCC_PORT38
        {83,        71,     103,    MP_INV_V,  1},           // NC          | SC23V5V_PORT39
        {84,        72,     104,    MP_INV_V,  1},           // NC          | SC2DATC4_PORT40
        {85,        73,     105,    MP_INV_V,  1},           // NC          | SC2CLK_PORT41
        {86,        74,     106,    MP_INV_V,  1},           // NC          | SC2DET_PORT42
        {88,        75,     107,    MP_INV_V,  1},           // NC          | SC2RST_PORT43
        {90,        76,     108,    MP_INV_V,  1},           // NC          | SC2DATC8_PORT44
        {92,        77,     109,    MP_INV_V,  1},           // NC          | SC2DATC7_PORT45
        {104,       0,      32,     MP_INV_V,  1},           // NC          | DBGTDI_PORT0
        {105,       1,      33,     MP_INV_V,  1},           // NC          | DBGTDO_PORT1
        {106,       2,      34,     MP_INV_V,  1},           // NC          | DBGTMS_PORT2
        {107,       3,      35,     MP_INV_V,  1},           // NC          | DBGTCK_PORT3
        {108,       4,      36,     MP_INV_V,  1},           // NC          | DBGTRST_PORT4
        {110,       128,    132,    MP_INV_V,  0},           // NC          | SPISCK_NFRDY0_NFRDY0_SPISCKDW
        {111,       129,    133,    MP_INV_V,  0},           // NC          | SPIMOSI_NFOE_NFWE_SPIMOSIDW
        {112,       130,    134,    MP_INV_V,  0},           // NC          | SPICSn_NFCLE_NFALE_SPICSnDW
        {113,       131,    135,    MP_INV_V,  0},           // NC          | SPIMISO_NFALE_NFCLE_SPIMISODW
        {116,       78,     110,    MP_INV_V,  1},           // NC          | SC1CLK_PORT46_NFOE
        {118,       79,     111,    MP_INV_V,  1},           // NC          | SC1RST_PORT47_NFDAT7
        {120,       80,     112,    MP_INV_V,  1},           // NC          | SC1PWR_PORT48_NFDAT6
        {122,       81,     113,    MP_INV_V,  1},           // NC          | SC1CD_PORT49_NFDAT5
        {124,       82,     114,    MP_INV_V,  1},           // NC          | SC1DATA_PORT50_NFDAT4
        {126,       83,     115,    MP_INV_V,  1},           // NC          | TSI3DAT5_PORT51_NFDAT3
        {128,       84,     116,    MP_INV_V,  1},           // NC          | TSI3DAT6_PORT52_NFDAT2
        {130,       85,     117,    MP_INV_V,  1},           // NC          | TSI3DAT7_PORT53_NFDAT1
        {132,       86,     118,    MP_INV_V,  1},           // NC          | TSI3CLK_PORT54_NFDAT0
        {136,       5,      37,     MP_INV_V,  1},           // NC          | TSI1DATA0_PORT5_NFWE_BTDATA0
        {137,       6,      38,          136,  1},           // NC          | TSI1DATA1_PORT6_SC1CLK_BTDATA1_SCTDI
        {138,       7,      39,          136,  1},           // NC          | TSI1DATA2_PORT7_SC1RST_BTDATA2_SCTDO
        {139,       8,      40,          136,  1},           // NC          | TSI1DATA3_PORT8_SC1PWR_BTDATA3_SCTMS
        {140,       9,      41,          136,  1},           // NC          | TSI1DATA4_PORT9_SC1CD_BTDATA4_SCTCK
        {141,       10,     42,          136,  1},           // NC          | TSI1DATA5_PORT10_SC1DATA_BTDATA5_SCTRST
        {142,       11,     43,     MP_INV_V,  1},           // NC          | TSI1DATA6_PORT11_NFDAT7_BTDATA6
        {143,       12,     44,     MP_INV_V,  1},           // NC          | TSI1DATA7_PORT12_NFDAT6_BTDATA7
        {144,       13,     45,     MP_INV_V,  1},           // NC          | TSI1CLK_PORT13_NFDAT5_BTCLK
        {149,       87,     119,    MP_INV_V,  2},           // NC          | SCANPWDCLk_TSI2DATA0_PORT55
        {150,       88,     120,    MP_INV_V,  2},           // NC          | SCANPWDDAT_TSI2DATA1_PORT56
        {151,       89,     121,    MP_INV_V,  2},           // NC          | SCANPWDSET_TSI2DATA2_PORT57
        {152,       90,     122,    MP_INV_V,  2},           // NC          | UARTTXICAM1_TSI2DATA3_PORT58
        {153,       91,     123,    MP_INV_V,  2},           // NC          | UARTRXICAM1_TSI2DATA4_PORT59
        {154,       92,     124,    MP_INV_V,  2},           // NC          | UARTTXICAM2_TSI2DATA5_PORT60
        {155,       93,     125,    MP_INV_V,  2},           // NC          | UARTRXICAM2_TSI2DATA6_PORT61
        {156,       94,     126,    MP_INV_V,  2},           // NC          | UARTTXICAM3_TSI2DATA7_PORT62
        {158,       95,     127,    MP_INV_V,  2},           // NC          | UARTRXICAM3_TSI2SCLK_PORT63
        {161,       14,     46,     MP_INV_V,  1},           // NC          | SDA2_PORT14_AUARTTX_PCMBCK
        {162,       15,     47,     MP_INV_V,  1},           // NC          | SCL2_PORT15_NFDAT4_PCMDIN
        {163,       16,     48,          137,  0},           // NC          | UARTTXICAM_UART2TX_NFDAT3_PCMSCKO_SCUARTTX
        {164,       17,     49,          137,  0},           // NC          | UARTRXICAM_UART2RX_NFDAT2_PCMLRCK_SCUARTRX
        {165,       18,     50,     MP_INV_V,  1},           // NC          | HVSEL_PORT18_NFDAT1_DiSEqCi
        {166,       19,     51,     MP_INV_V,  1},           // NC          | DiSEqCo_PORT19_NFDAT0
        {168,       20,     52,     MP_INV_V,  1},           // NC          | AGC_PORT20
        {175,       21,     53,     MP_INV_V,  0},           // NC          | UART1TX_PORT21_AUARTTX
        {176,       22,     54,     MP_INV_V,  0},           // NC          | UART1RX_PORT22
        {177,       23,     55,     MP_INV_V,  1},           // NC          | SDA1_PORT23
        {178,       24,     56,     MP_INV_V,  1},           // NC          | SCL1_PORT24
        {272,       26,     58,     MP_INV_V,  1},           // NC          | DDCSDA_PORT26_SDA2_HDMIDTB0
        {273,       27,     59,     MP_INV_V,  1},           // NC          | DDCSCL_PORT27_SCL2_HDMIDTB1
        {274,       28,     60,     MP_INV_V,  1},           // NC          | SPDIF_PORT28
        {276,       29,     61,     MP_INV_V,  1},           // NC          | RMCRSDV_PORT29_SPDLED
        {277,       30,     62,     MP_INV_V,  1},           // NC          | MD_PORT30_LINKLED
        {278,       31,     63,     MP_INV_V,  1},           // NC          | MDC_PORT31_ACTLED
        {279,       64,     96,     MP_INV_V,  1},           // NC          | RMTXEN_PORT32_AJTDI_SC1CLK
        {281,       65,     97,     MP_INV_V,  1},           // NC          | RMTXD1_PORT33_AJTDO_SC1RST
        {282,       66,     98,     MP_INV_V,  1},           // NC          | RMTXD0_PORT34_AJTMS_SC1PWR
        {283,       67,     99,     MP_INV_V,  1},           // NC          | RMCLK_PORT35_AJTCK_SC1CD
        {285,       68,     100,    MP_INV_V,  1},           // NC          | RMRXD1_PORT36_AJRST_SC1DATA
        {287,       69,     101,    MP_INV_V,  1},           // NC          | RMRXD0_PORT37_AOAMCLK

	{ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG}
};

struct gpio_entry_lowpower g_gpio_table[] = {
	//phy_gpio   | 	io_mode		|	output_value
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
	{18,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{19,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{20,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{21,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{22,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{23,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
	{24,		GX_GPIO_OUTPUT,		GX_GPIO_LOW},
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

	{ARRAY_END_FLAG, 0, 0}
};

/*****************************************************************************
 * Function    : GX3211_ClosePll
 * Description : Close all the clock except the APB clock and cpu clock
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3211_ClosePll(void)
{
	// set all IP except the PLL to powerdown mode.
	// set the video dac to powerdown
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DAC) &= ~(0x3<<0); // set the evbg and enextref to zero
	*(volatile unsigned int*)(0x04800134) &= ~(0x1f<<0); // set the evbg and enextref to zero

	// set the audio dac to powerdown
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_AUDIO_CODEC_CONTROL) &= ~(0x1ff<<1);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_AUDIO_CODEC_CONTROL) |= (0x1<<4); // 11:4 data set  1
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_AUDIO_CODEC_CONTROL) |= (0x1<<2); // set the load
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_AUDIO_CODEC_CONTROL) |= (0x1<<1); // set the sfr clock
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_AUDIO_CODEC_CONTROL) &= ~(0x1<<1); // clear the sfr clock

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_AUDIO_CODEC_CONTROL) &= ~0x1; // set the audio dac to rstn

	// HDMI to iddq
	*(volatile unsigned int*)(0x04f0c000) = 0x10; // set the HDMI PHY in iddq mode

	// USB PHY
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_USB1_CONFIG) |= (0x1<<10)|(0x1<<26); // set to the suspend mode
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_USB1_CONFIG) &= ~(0x1<<13); // USB PHY reset

	// ETH PHY
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_EPHY_CONFIG) |= (0x1<<1); // ETH shut down
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_EPHY_CONFIG) &= ~(0x1<<2); // ETH reset


	// ADC powerdwon
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_ADC) &= ~(0x3<<18); // adc power down

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (0xf<<11); // POWER DOWN

	// gate the module clock
	// demux clock to osc, because the sram clock is demux
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) &= ~(1<<17);
	// apb clock to osc
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) &= ~(0x3<<19);
	// cpu clock to osc
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) &= ~(0x1<<24);

	*(volatile unsigned int*)(0x0030a004) |= ((1<<15)|(1<<21)); //reset otp
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG2) &= ~((0x1<<12)|(0x1<<21)|(0x1<<30));

	// set 1 gate the clock
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_CLK_INHIBIT_1SET) = 0xfffe7fff; // apb0 and apb2 not gate 16 15 bit
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_CLK_INHIBIT2_1SET) = 0xffffffff;

	// powedown the PLL
	*(volatile unsigned int*)(CONFIG_BASE+PLL_CPU_CONFIG_BASE) |= (0x1<<14);
	*(volatile unsigned int*)(CONFIG_BASE+PLL_DTO_CONFIG_BASE) |= (0x1<<14);
	*(volatile unsigned int*)(CONFIG_BASE+PLL_DVB_CONFIG_BASE) |= (0x1<<14);
	*(volatile unsigned int*)(CONFIG_BASE+PLL_DDR_CONFIG_BASE) |= (0x1<<14);

}



#if DEBUG_EN > 0

/*****************************************************************************
 * Function    : GX3211_ConfigPll
 * Description : Set the frequency of AHB to 108MHz
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3211_ConfigPll(void)
{
}

#endif

/*****************************************************************************
 * Function    : GX3211_CloseModule
 * Description : Close the modules such as WDT, INTC and TV
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3211_CloseModule(void)
{
}

/*****************************************************************************
 * Function    : GX3211_SetGpio
 * Description : Set the gpio
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3211_SetGpio(U64 GpioMask,U64 GpioData)
{
}

static void set_mulpin_fun(int sel, int fun)
{
	if(sel < 32){
		REG_SET_CLR_BIT(REG_PINMUX_PORTA, sel, fun);
	}else if (sel < 64){
		REG_SET_CLR_BIT(REG_PINMUX_PORTB, sel - 32, fun);
	}else if (sel < 96){
		REG_SET_CLR_BIT(REG_PINMUX_PORTC, sel - 64, fun);
	}else if (sel < 128){
		REG_SET_CLR_BIT(REG_PINMUX_PORTD, sel - 96, fun);
	}else if (sel < 160){
		REG_SET_CLR_BIT(REG_PINMUX_PORTE, sel - 128, fun);
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
			set_mulpin_fun(mulpin_table[i].sel2, (mulpin_table[i].fun & 0x4) >> 2);
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

void GX3211_Config_Pin_Gpio(void)
{
	Config_Gpio();
	Config_Pin();
	*(volatile unsigned int*)(SPI_BASE_ADDR) |= (0x1<<10)&(~(0x1<<17));
}

gxlowpower_func_t gxlowpower_func =
{
	.ClosePll     = GX3211_ClosePll,
	.CloseModule  = GX3211_CloseModule,
	.SetGpio      = GX3211_SetGpio,
	.ConfigPin    = GX3211_Config_Pin_Gpio,

#if DEBUG_EN > 0
	.ConfigPll    = GX3211_ConfigPll,
#else
	.ConfigPll    = NULL,
#endif
};

#endif

