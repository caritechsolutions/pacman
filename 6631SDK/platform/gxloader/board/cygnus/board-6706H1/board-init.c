
/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology, Inc. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/*
 * hardware version: generic_DEMO_V1.0  2013.10.30
 * */

#include "../board-common.h"
#include "cpu_config.h"

char logo_partition_name[20] = PARTITION_LOGO;
char logo_file_name[20] = "gxlogo.jpg";
enum cvbs_mode_enum g_cvbs_mode = G_PAL;
enum ypbpr_hdmi_mode_enum g_ypbpr_hdmi_mode = G_YPBPR_HDMI_1080I_50HZ;
enum vout_dac_case default_dac_case_svpu = CASE_BGRX;
enum vout_dac_case default_dac_case_vpu = CASE_BGRX;

/*
 * 8M SPI-FLASH
 */
struct partition_info default_partition [] = {
#if 0
	{
		.name = "BOOT",
		.total_size = 64 * K,
		.start_addr = AUTO,
	},
	{
		.name = "TABLE",
		.total_size = 64 * K,
		.start_addr = 0x10000,
	},
	{
		.name = "LOGO",
		.total_size = 192 * K,
		.start_addr = 0x20000,
	},
	{
		.name = "V_OEM",
		.total_size = 64 * K,
		.start_addr = 0x50000,
	},
	{
		.name = "OTA",
		.total_size = 640 * K,
		.start_addr = 0x60000,
	},
	{
		.name = "KERNEL",
		.total_size = 3 * M,
		.start_addr = 0x100000,
	},
	{
		.name = "ROOT",
		.total_size = 2 * M,
		.start_addr = 0x400000,
	},
	{
		.name = "DATA",
		.total_size = AUTO,
		.start_addr = 0x600000,
	},

#endif
	{
		.name = "",
	}
};

void __attribute__((section(".reset_patch"))) ddr_config_patch(void)
{
#define DDR_OUTPUT_DRIVER 0x6
#define DDR_ADDR_OUTPUT_DRIVER 0x4
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<16) | (DDR_OUTPUT_DRIVER<<19) | (DDR_OUTPUT_DRIVER<<22) | (DDR_OUTPUT_DRIVER<<25) | (DDR_OUTPUT_DRIVER<<28); //feed, odt, dm, cmd
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11); //dqs and clk
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (DDR_ADDR_OUTPUT_DRIVER<<11); //cmmd
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG3) |= (DDR_OUTPUT_DRIVER<<16) | (DDR_OUTPUT_DRIVER<<20) | (DDR_OUTPUT_DRIVER<<24) |(DDR_OUTPUT_DRIVER<<28); //dq
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG5) |= (DDR_ADDR_OUTPUT_DRIVER<<0) |(DDR_ADDR_OUTPUT_DRIVER<<3) |(DDR_ADDR_OUTPUT_DRIVER<<6) |(DDR_ADDR_OUTPUT_DRIVER<<9) |(DDR_ADDR_OUTPUT_DRIVER<<12) |(DDR_ADDR_OUTPUT_DRIVER<<15) |(DDR_ADDR_OUTPUT_DRIVER<<18) |(DDR_ADDR_OUTPUT_DRIVER<<21) |(DDR_ADDR_OUTPUT_DRIVER<<24) |(DDR_ADDR_OUTPUT_DRIVER<<27); //addr
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG6) |= (DDR_ADDR_OUTPUT_DRIVER<<0) |(DDR_ADDR_OUTPUT_DRIVER<<3) |(DDR_ADDR_OUTPUT_DRIVER<<6) |(DDR_ADDR_OUTPUT_DRIVER<<9) |(DDR_ADDR_OUTPUT_DRIVER<<12) |(DDR_ADDR_OUTPUT_DRIVER<<16) |(DDR_ADDR_OUTPUT_DRIVER<<19) |(DDR_ADDR_OUTPUT_DRIVER<<22); //addr, ba
}


/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
/*       pin_id  func_sel                     func_list */
        {0,       1},          //NC          |PORT00/SPISCK
        {1,       1},          //NC          |PORT01/SPIMOSI
        {2,       1},          //NC          |PORT02/SPICSn
        {3,       1},          //NC          |PORT03/SPIMISO
        {4,       0},          //NC          |PORT04/WP
        {5,       0},          //NC          |PORT05/HOLD
        {6,       2},          //NC          |PORT06/WP/UART1TX/SUARTTX/AUARTTX
        {7,       2},          //NC          |PORT07/HOLD/UART1RX/SUARTRX/AUARTRX
        {8,       0},          //NC          |PORT08/SC1CLK
        {9,       0},          //NC          |PORT09/SC1RST
        {10,      0},          //NC          |PORT10/SC1PWR
        {11,      0},          //NC          |PORT11/SC1CD
        {12,      0},          //NC          |PORT12/SC1DATA
        {13,      0},          //NC          |PORT13/I2SDATA/UART2TX
        {14,      0},          //NC          |PORT14/I2SBCK/URART2RX
        {15,      0},          //NC          |PORT15/I2SLRCK/SDA2
        {16,      0},          //NC          |PORT16/I2SMCK/SCL2
        {17,      0},          //NC          |PORT17(PMUPORT00)/SDA2/UART2TX/WP/SUARTTX/AUARTTX
        {18,      0},          //NC          |PORT18(PMUPORT01)/SCL2/UART2RX/CEC/HOLD/SUARTRX/AURATRX
        {19,      1},          //NC          |PORT19(PMUPORT02)/IR
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {20,      1},          //NC          |PORT20(PMUPORT03)/DBGTDI/SDBGTDI/ADBGTDI/ADCCLK/TSOUT10
        {21,      1},          //NC          |PORT21(PMUPORT04)/DBGTDO/SDBGTDO/ADBGTDO/ADCBIT9/TSOUT9
        {22,      1},          //NC          |PORT22(PMUPORT05)/DBGTMS/SDBGTMS/ADBGTMS/ADCBIT8/TSOUT8
        {23,      1},          //NC          |PORT23(PMUPORT06)/DBGTCK/SDBGTCK/ADBGTCK/ADCBIT7/TSOUT7
        {24,      6},          //NC          |PORT24(PMUPORT07)/DBGTRST/SDBGTRST/ADBGTRST/ADCBIT6/TSOUT6/CEC
#endif
        {25,      0},          //NC          |PORT25(PMUPORT08)/CEC/TSIDATA7/TSODATA7/ADCBIT5/TSOUT5/PLL_OUT
        {26,      1},          //NC          |PORT26/SC1CLK/TSIDATA6/TSODATA6/ADCBIT4/TSOUT4/SDBGTDI/ADBGTDI
        {27,      1},          //NC          |PORT27/SC1RST/TSIDATA5/TSODATA5/ADCBIT3/TSOUT3/SDBGTDO/ADBGTDO
        {28,      1},          //NC          |PORT28/SC1PWR/TSIDATA4/TSODATA4/ADCBIT2/TSOUT2/SDBGTMS/ADBGTMS
        {29,      1},          //NC          |PORT29/SC1CD/TSIDATA3/TSODATA3/ADCBIT1/TSOUT1/SDBGTCK/ADBGTCK
        {30,      1},          //NC          |PORT30/SC1DATA/TSIDATA2/TSODATA2/ADCBIT0/TSOUT0/SDBGTRST/ADBGTRST
        {31,      0},          //NC          |PORT31/SDA2/TSIDATA1/TSODATA1/RMCRSDV
        {32,      0},          //NC          |PORT32/SCL2/TSIDATA0/TSODATA0/MD
        {33,      0},          //NC          |PORT33
        {34,      0},          //NC          |PORT34/AGC/TSICLK/TSOCLK/FM_AGC/MDC
        {35,      0},          //NC          |PORT35/UART2TX/TSISYNC/TSOSYNC/RMTXEN/SUARTTX/AUARTTX
        {36,      0},          //NC          |PORT36/UART2RX/TSIVALID/TSOVALID/RMTXD1/SUARTRX/AUARTRX
        {37,      0},          //NC          |PORT37/SDA1/SDAT/RMTXD0
        {38,      1},          //NC          |PORT38/SCL1/SCLT/RMCLK
        {39,      0},          //NC          |PORT39/TSISDATA3
        {40,      0},          //NC          |PORT40/TSISDATA2
        {41,      0},          //NC          |PORT41/TSISDATA1
        {42,      0},          //NC          |PORT42/TSISDATA0
        {43,      0},          //NC          |PORT43/TSISCLK
        {44,      0},          //NC          |PORT44/TSISSYNC
        {45,      0},          //NC          |PORT45/TSISVALID
        {46,      0},          //NC          |PORT46/SDA1
        {47,      0},          //NC          |PORT47/SCL1
        {48,      1},          //NC          |PORT48/SDA2/SDAT/RMRXD1
        {49,      1},          //NC          |PORT49/SCL2/SCLT/RMRXD0
        {50,      1},          //NC          |PORT50/AGC/FM_AGC
        {51,      1},          //NC          |PORT51/DDCSDA/SDAS
        {52,      1},          //NC          |PORT52/DDCSCL/SCLS
        {53,      1},          //NC          |PORT53/SPDIF
        {54,      0},          //NC          |PORT54/UART1TX/SUARTTX/AUARTTX
        {55,      0},          //NC          |PORT55/UART1RX/SUARTRX/AUARTRX
        {56,      0},          //NC          |PORT56/I2SDATA
        {57,      0},          //NC          |PORT57/I2SBCK
        {58,      0},          //NC          |PORT58/I2SLRCK
        {59,      0},          //NC          |PORT59/I2SMCK
        {60,      0},          //NC          |PORT60/RMCRSDV
        {61,      0},          //NC          |PORT61/MD
        {62,      0},          //NC          |PORT62/MDC
        {63,      0},          //NC          |PORT63/RMTXEN
        {64,      0},          //NC          |PORT64/RMTXD1
        {65,      0},          //NC          |PORT65/RMTXD0
        {66,      0},          //NC          |PORT66/RMCLK
        {67,      0},          //NC          |PORT67/RMRXD1
        {68,      0},          //NC          |PORT68/RMRXD0
        {69,      0},          //NC          |PORT69/PHYCLK
        {70,      0},          //NC          |PORT70/SDAPHY
        {71,      0},          //NC          |PORT71/SCLPHY
        {72,      0},          //NC          |PORT72
        {73,      0},          //NC          |PORT73(PMUPORT09)/xtal_out
        {ARRAY_END_FLAG_U16, NOT_CONFIG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
	{0,     18,    GX_GPIO_CONFIG_VALID,    GX_GPIO_INPUT,  GX_GPIO_LOW},//PANEL DATA
	{1,     17,    GX_GPIO_CONFIG_VALID,    GX_GPIO_OUTPUT, GX_GPIO_HIGH},//PANEL CLK
	{3,     31,    GX_GPIO_CONFIG_VALID,    GX_GPIO_OUTPUT, GX_GPIO_HIGH},//Mute
	{15,    37,    GX_GPIO_CONFIG_VALID,    GX_GPIO_OUTPUT,  GX_GPIO_LOW}, //ANT POWER AND LNB SHORT
	{16,    25,    GX_GPIO_CONFIG_VALID,    GX_GPIO_OUTPUT, GX_GPIO_HIGH},//Power CNTL
	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* Board specific initialization routine */
int board_init(void)
{
	//enable_dac(ENABLE_CVBS);
	gpio_init();
	mulpin_init();

	//cmdline_add("av_extend:0xa0305000=0x12345678,0xa0306000=0x87654321");
	//cmdline_add("ex_extend:ex...");

#ifdef ENABLE_USB_RECOVER
extern int usb_recover_proc(void);
	g_board_extend.func_usb_update = usb_recover_proc;	//register function pointer
#endif

	//ts_mode_config(1, 1, 2, 0, 1, 0, -1);	//set ts1 to serial ts, and mode is 2, no valid, sync, little endian
	ts_mode_config(0, 0, 2, 1, 1, 0, -1);	//set ts0 to serial ts, and mode is 2, valid, sync, little endian

	return 0;
}

//panel function description:
//you can init panel && show BOOT by panel_show_call()
//you can scan panel && return 0 by panel_update_call() to enter ota forcibly
int panel_show_call(void)
{
	return -1;
}

int panel_update_call(void)
{
	return -1;
}

