/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   gx6605s.c
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   The functions of GX6605S in lowpower
*****************************************************************************/

#include "gxcomm.h"
#if GXCHIP_TYPE == GXCHIP_TYPE_GX6605S

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
#define DATABAHN_LP_CMD_MASK_ENTRY                  0x02
#define DATABAHN_LP_CMD_STATE_SELF_REFRESH          (2<<2)
#define DATABAHN_LP_Entry_Self_Refresh              (DATABAHN_LP_CMD_MASK_ENTRY | DATABAHN_LP_CMD_STATE_SELF_REFRESH)
#define Databahn_Base                  0x00c00000
#define Databahn_Reg(x)                (Databahn_Base+(x<<2))

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
 /* chip_core_num | bit0 | bit1    | bit2       | func_select */   /* package_num | func_list */
        {4,         0,     44,       MP_INV_V,    1},              // NC          | DBGTDI_PORT00_ADCODATA0
        {5,         1,     45,       MP_INV_V,    1},              // NC          | DBGTDO_PORT01_ADCODATA1
        {6,         2,     46,       MP_INV_V,    1},              // NC          | DBGTMS_PORT02_ADCODATA2
        {7,         3,     47,       MP_INV_V,    1},              // NC          | DBGTCK_PORT03_ADCODATA3
        {8,         4,     48,       MP_INV_V,    1},              // NC          | DBGTRST_PORT04_ADCODATA4
        {26,        5,     32,       64,          1},              // NC          | SC1CLK_PORT05_TSI1DATA0_AJTDI_DVBFSYNC_ADCODATA5
        {27,        6,     33,       65,          1},              // NC          | SC1RST_PORT06_TSI1DATA1_AJTDO_ADCODATA6
        {28,        7,     34,       66,          1},              // NC          | SC1PWR_PORT07_TSI1DATA2_AJTMS_ADCODATA7
        {29,        8,     35,       67,          1},              // NC          | SC1CD_PORT08_TSI1DATA3_AJTCK_ADCODATA8
        {31,        9,     36,       68,          1},              // NC          | SC1DATA_PORT09_TSI1DATA4_AJRST_ADCODATA9
        {32,        10,    37,       69,          1},              // NC          | DiSEqCi_PORT10_TSI1DATA5_ADCOCLK
        {33,        11,    38,       MP_INV_V,    1},              // NC          | HVSEL_PORT11_TSI1DATA6
        {34,        12,    39,       MP_INV_V,    1},              // NC          | DiSEqCo_PORT12_TSI1DATA7
        {35,        13,    40,       MP_INV_V,    1},              // NC          | AGC_PORT13_TSI1CLK
        {54,        14,    MP_INV_V, MP_INV_V,    1},              // NC          | SDA1_PORT14
        {55,        15,    MP_INV_V, MP_INV_V,    1},              // NC          | SCL1_PORT15
        {56,        16,    41,       MP_INV_V,    0},              // NC          | UART1TX_PORT16_AUARTTX_HDMIDBT0
        {57,        17,    42,       MP_INV_V,    0},              // NC          | UART1RX_PORT17_HDMIDBT1
        {126,       18,    MP_INV_V, MP_INV_V,    1},              // NC          | DDCSDA_PORT18
        {127,       19,    MP_INV_V, MP_INV_V,    1},              // NC          | DDCSCL_PORT19
        {128,       20,    43,       MP_INV_V,    1},              // NC          | SPDIF_PORT20_OTPAVDDEN

    {ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG}
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
	{ARRAY_END_FLAG, 0, 0}
};

void SetRegisterField(unsigned int reg, unsigned int offset, unsigned int width, unsigned int value)
{
	unsigned int result;
	unsigned int original;
	unsigned int mask;
	unsigned int value_masked;

	original = *(volatile unsigned int *)reg;

	if(width==32)
	{
		mask = 0xffffffff;
	}
	else
	{
		mask = (1<<width)-1;
	}
	value_masked = value&mask;
	result = (original & (~(mask<<offset))) | (value_masked<<offset);

	*(volatile unsigned int *)reg = result;
}

unsigned int GetRegisterField(unsigned int reg, unsigned int offset, unsigned int width)
{
	unsigned int result;
	unsigned int original;
	unsigned int mask;
	unsigned int mask_shifted;

	original = *(volatile unsigned int *)reg;
	if(width==32)
	{
		mask = 0xffffffff;
	}
	else
	{
		mask = (1<<width)-1;
	}
	mask_shifted = mask<<offset;
	result = (original&mask_shifted)>>offset;

	return result;
}

void Databahn_SetParameter(unsigned int reg, unsigned int offset, unsigned int width, unsigned int value)
{
	SetRegisterField(Databahn_Reg(reg), offset, width, value);
}

unsigned int Databahn_GetParameter(unsigned int reg, unsigned int offset, unsigned int width)
{
	return GetRegisterField(Databahn_Reg(reg), offset, width);
}

#define DATABAHN_REG_CONTROLLER_BUSY_ADDR           45
#define DATABAHN_REG_CONTROLLER_BUSY_OFFSET         8
#define DATABAHN_REG_CONTROLLER_BUSY_WIDTH          1
#define DATABAHN_REG_LP_ARB_STATE_ADDR              21
#define DATABAHN_REG_LP_ARB_STATE_OFFSET            8
#define DATABAHN_REG_LP_ARB_STATE_WIDTH             2
#define FATABAHN_LP_ARB_STATE_SOFTWARE              0x1
#define FATABAHN_LP_ARB_STATE_IDLE                  0x0
#define DATABAHN_REG_LP_CMD_ADDR                    20
#define DATABAHN_REG_LP_CMD_OFFSET                  24
#define DATABAHN_REG_LP_CMD_WIDTH                   8
#define DATABAHN_LP_CMD_MASK_LOCK                   0x80
#define DATABAHN_REG_LP_STATE_ADDR                  21
#define DATABAHN_REG_LP_STATE_OFFSET                0
#define DATABAHN_REG_LP_STATE_WIDTH                 7
#define DATABAHN_LP_STATE_MASK_VALID                (1<<6)
void Databahn_LPCmd(unsigned int cmd)
{
	unsigned int controller_busy;
	unsigned int lp_arb_state;
	unsigned int lp_state;

	do {
		controller_busy = Databahn_GetParameter(DATABAHN_REG_CONTROLLER_BUSY_ADDR, DATABAHN_REG_CONTROLLER_BUSY_OFFSET, DATABAHN_REG_CONTROLLER_BUSY_WIDTH);
	} while(controller_busy);

	do {
		lp_arb_state = Databahn_GetParameter(DATABAHN_REG_LP_ARB_STATE_ADDR, DATABAHN_REG_LP_ARB_STATE_OFFSET, DATABAHN_REG_LP_ARB_STATE_WIDTH);
		if(lp_arb_state==FATABAHN_LP_ARB_STATE_SOFTWARE)
			break;
	} while(lp_arb_state!=FATABAHN_LP_ARB_STATE_IDLE);

	Databahn_SetParameter(DATABAHN_REG_LP_CMD_ADDR, DATABAHN_REG_LP_CMD_OFFSET, DATABAHN_REG_LP_CMD_WIDTH, DATABAHN_LP_CMD_MASK_LOCK|cmd);

	do {
		lp_state = Databahn_GetParameter(DATABAHN_REG_LP_STATE_ADDR, DATABAHN_REG_LP_STATE_OFFSET, DATABAHN_REG_LP_STATE_WIDTH);
	} while(!(lp_state&DATABAHN_LP_STATE_MASK_VALID));
}
/*****************************************************************************
 * Function    : GX6605S_ClosePll
 * Description : Close all the clock except the APB clock and cpu clock
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX6605S_ClosePll(void)
{

	// solve the problem when reboot from soft , it may boot failed. ddr is not right
	Databahn_LPCmd(DATABAHN_LP_Entry_Self_Refresh);

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
 * Function    : GX6605S_ConfigPll
 * Description : Set the frequency of AHB to 108MHz
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX6605S_ConfigPll(void)
{
}

#endif

/*****************************************************************************
 * Function    : GX6605S_CloseModule
 * Description : Close the modules such as WDT, INTC and TV
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX6605S_CloseModule(void)
{
}

/*****************************************************************************
 * Function    : GX6605S_SetGpio
 * Description : Set the gpio
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX6605S_SetGpio(U64 GpioMask,U64 GpioData)
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

void GX6605S_Config_Pin_Gpio(void)
{
	Config_Gpio();
	Config_Pin();
	*(volatile unsigned int*)(SPI_BASE_ADDR) |= (0x1<<10)&(~(0x1<<17));
}

gxlowpower_func_t gxlowpower_func =
{
	.ClosePll     = GX6605S_ClosePll,
	.CloseModule  = GX6605S_CloseModule,
	.SetGpio      = GX6605S_SetGpio,
	.ConfigPin    = GX6605S_Config_Pin_Gpio,

#if DEBUG_EN > 0
	.ConfigPll    = GX6605S_ConfigPll,
#else
	.ConfigPll    = NULL,
#endif
};

#endif

