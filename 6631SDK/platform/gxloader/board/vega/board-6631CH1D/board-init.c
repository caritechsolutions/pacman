
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
#include "panel_fd650.h"

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
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<24);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (3<<0) | (3<<11);
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
/*       pin_id  func_sel                            fun0               fun1         fun2           fun3         fun4         fun5         fun6         fun7         */
        {0,         0},          //NC          |GPIO00 (PMUPORT00) |MD           |SDIO_WP      |             |            |            |              |           |
        {1,         0},          //NC          |GPIO01 (PMUPORT01) |MDC          |SDIO_PWR     |             |            |            |              |           |
        {2,         0},          //NC          |GPIO02 (PMUPORT02) |RMCRSDV      |SDIO_CD      |             |            |            |              |           |
        {3,         0},          //NC          |GPIO03 (PMUPORT03) |RMTXEN       |SDIO_DATA[1] |             |            |            |              |           |
        {4,         0},          //NC          |GPIO04 (PMUPORT04) |RMTXD[1]     |SDIO_DATA[0] |             |            |            |              |           |
        {5,         0},          //NC          |GPIO05 (PMUPORT05) |RMTXD[0]     |SDIO_CLK     |PLL_OUT1     |            |            |              |           |
        {6,         0},          //NC          |GPIO06 (PMUPORT06) |RMCLK        |SDIO_CMD     |             |            |            |              |           |
        {7,         0},          //NC          |GPIO07 (PMUPORT07) |RMRXD[1]     |SDIO_DATA[3] |UART1_CTS    |MD          |            |              |           |
        {8,         0},          //NC          |GPIO08 (PMUPORT08) |RMRXD[0]     |SDIO_DATA[2] |ADCQDATA[9]  |UART1_RTS   |MDC         |CEC(开漏输出) |           |
        {9,         1},          //NC          |GPIO09 (PMUPORT09) |UART1TX      |AUARTTX      |             |            |            |              |           |
        {10,        1},          //NC          |GPIO10 (PMUPORT10) |UART1RX      |             |             |            |            |              |           |
        {11,        1},          //NC          |GPIO11 (PMUPORT11) |IR           |             |             |            |            |              |           |
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {12,        1},          //NC          |GPIO12 (PMUPORT12) |DBGTDI       |             |             |            |            |              |           |
        {13,        1},          //NC          |GPIO13 (PMUPORT13) |DBGTDO       |             |             |            |            |              |           |
        {14,        1},          //NC          |GPIO14 (PMUPORT14) |DBGTMS       |             |             |            |            |              |           |
        {15,        1},          //NC          |GPIO15 (PMUPORT15) |DBGTCK       |             |             |            |            |              |           |
        {16,        2},          //NC          |GPIO16 (PMUPORT16) |DBGTRST      |CEC          |             |            |            |              |           |
#endif
        {17,        0},          //NC          |GPIO17 (PMUPORT17) |TSI1DATA[7]  |TSODATA[7]   |TSIS0DATA[1] |CEC         |            |              |           |
        {18,        0},          //NC          |GPIO18             |TSI1DATA[6]  |TSODATA[6]   |TSIS0DATA[0] |ADCQDATA[9] |UART2TX     |DBGTDI        |SC1CLK     |
        {19,        0},          //NC          |GPIO19             |TSI1DATA[5]  |TSODATA[5]   |TSIS0CLK     |ADCQDATA[8] |UART2RX     |DBGTDO        |SC1RST     |
        {20,        0},          //NC          |GPIO20             |TSI1DATA[4]  |TSODATA[4]   |TSIS0SYNC    |ADCQDATA[7] |SDA2        |DBGTMS        |SC1DATA    |
        {21,        0},          //NC          |GPIO21             |TSI1DATA[3]  |TSODATA[3]   |TSIS0VALID   |ADCQDATA[6] |SCL2        |DBGTCK        |SC1PWR     |
        {22,        3},          //NC          |GPIO22             |TSI1DATA[2]  |TSODATA[2]   |SC1CLK       |ADCQDATA[5] |            |DBGTRST       |SC1CD      |
        {23,        3},          //NC          |GPIO23             |TSI1DATA[1]  |TSODATA[1]   |SC1RST       |ADCQDATA[4] |            |              |           |
        {24,        3},          //NC          |GPIO24             |TSI1DATA[0]  |TSODATA[0]   |SC1PWR       |ADCQDATA[3] |            |              |           |
        {25,        3},          //NC          |GPIO25             |TSI1CLK      |TSOCLK       |SC1CD        |ADCQDATA[2] |            |              |           |
        {26,        3},          //NC          |GPIO26             |TSI1SYNC     |TSOSYNC      |SC1DATA      |ADCQDATA[1] |            |              |           |
        {27,        0},          //NC          |GPIO27             |TSI1VALID    |TSOVALID     |             |ADCQDATA[0] |            |              |           |
        {38,        0},          //NC          |GPIO38             |SDA3         |UART2TX      |TSIS0VALID   |            |            |              |           |
        {39,        0},          //NC          |GPIO39             |SCL3         |UART2RX      |TSIS0DATA[3] |            |            |              |           |
        {40,        0},          //NC          |GPIO40             |TSOVALID     |TSI2VALID    |TSIS0DATA[2] |SC1DATA     |ADCCLK      |              |           |
        {41,        0},          //NC          |GPIO41             |TSODATA[7]   |TSI2DATA[7]  |TSIS0DATA[1] |SC1CLK      |ADCIDATA[9] |              |           |
        {42,        0},          //NC          |GPIO42             |TSODATA[6]   |TSI2DATA[6]  |TSIS0DATA[0] |SC1RST      |ADCIDATA[8] |              |           |
        {43,        0},          //NC          |GPIO43             |TSODATA[5]   |TSI2DATA[5]  |TSIS0CLK     |SC1PWR      |ADCIDATA[7] |              |           |
        {44,        0},          //NC          |GPIO44             |TSODATA[4]   |TSI2DATA[4]  |TSIS0SYNC    |SC1CD       |ADCIDATA[6] |              |           |
        {45,        3},          //NC          |GPIO45             |TSODATA[3]   |TSI2DATA[3]  |HVSEL        |SDA3        |SC1DATA     |ADCIDATA[5]   |           |
        {46,        3},          //NC          |GPIO46             |TSODATA[2]   |TSI2DATA[2]  |DiSEqCi      |SCL3        |HVSEL       |ADCIDATA[4]   |           |
        {47,        4},          //NC          |GPIO47             |TSODATA[1]   |TSI2DATA[1]  |DiSEqCo      |AGC2        |UART3TX     |ADCIDATA[3]   |HDMI_TX    |
        {48,        3},          //NC          |GPIO48             |TSODATA[0]   |TSI2DATA[0]  |AGC          |            |UART3RX     |ADCIDATA[2]   |HDMI_RX    |
        {49,        3},          //NC          |GPIO49             |TSOCLK       |TSI2CLK      |SDA1         |SDA_T       |UART3CTS    |ADCIDATA[1]   |           |
        {50,        3},          //NC          |GPIO50             |TSOSYNC      |TSI2SYNC     |SCL1         |SCL_T       |UART3RTS    |ADCIDATA[0]   |           |
        {51,        0},          //NC          |GPIO51             |HVSEL        |SDA4         |             |            |            |              |           |
        {52,        0},          //NC          |GPIO52             |DiSEqCi      |SCL4         |HVSEL        |            |            |              |           |
        {53,        0},          //NC          |GPIO53             |DiSEqCo      |AGC2         |             |            |            |              |           |
        {54,        0},          //NC          |GPIO54             |AGC          |             |             |            |            |              |           |
        {55,        0},          //NC          |GPIO55             |SDA2         |SDA_T        |             |            |            |              |           |
        {56,        0},          //NC          |GPIO56             |SCL2         |SCL_T        |             |            |            |              |           |
        {72,        1},          //NC          |GPIO72             |DDCSDA       |SDAS         |             |            |            |              |           |
        {73,        1},          //NC          |GPIO73             |DDCSCL       |SCLS         |             |            |            |              |           |
        {74,        1},          //NC          |GPIO74             |HPD          |             |             |            |            |              |           |
        {63,        0},          //NC          |GPIO63             |SDIO_WP      |             |             |            |            |              |           |
        {64,        0},          //NC          |GPIO64             |SDIO_CD      |             |             |            |            |              |           |
        {65,        0},          //NC          |GPIO65             |SDIO_DATA[1] |UART2TX      |SDA1         |            |            |              |           |
        {66,        0},          //NC          |GPIO66             |SDIO_DATA[0] |UART2RX      |SCL1         |            |            |              |           |
        {67,        0},          //NC          |GPIO67             |SDIO_CLK     |UART2_CTS    |             |            |            |              |           |
        {68,        0},          //NC          |GPIO68             |SDIO_CMD     |UART2_RTS    |             |            |            |              |           |
        {69,        0},          //NC          |GPIO69             |SDIO_DATA[3] |SPDIF        |PHY_CLK      |            |            |              |           |
        {70,        0},          //NC          |GPIO70             |SDIO_DATA[2] |PHY_CLK      |             |            |            |              |           |
        {75,        1},          //NC          |GPIO75             |SPDIF        |PLL_OUT      |             |            |            |              |           |
        {76,        0},          //NC          |GPIO76             |PHY_CLK      |             |             |            |            |              |           |
        {78,        0},          //NC          |GPIO78             |RMCRSDV      |             |             |            |            |              |           |
        {81,        0},          //NC          |GPIO81             |RMTXEN       |             |             |            |            |              |           |
        {82,        0},          //NC          |GPIO82             |RMTXD[1]     |             |             |            |            |              |           |
        {83,        0},          //NC          |GPIO83             |RMTXD[0]     |             |             |            |            |              |           |
        {84,        0},          //NC          |GPIO84             |RMCLK        |             |             |            |            |              |           |
        {85,        0},          //NC          |GPIO85             |RMRXD[1]     |             |             |            |            |              |           |
        {86,        0},          //NC          |GPIO86             |RMRXD[0]     |PHY_CLK      |             |            |            |              |           |
        {87,        0},          //NC          |GPIO87             |MD           |PHY_CLK      |             |            |            |              |           |
        {90,        2},          //NC          |GPIO90             |MDC          |SPDIF        |PLL_OUT      |            |            |              |           |
        {91,        0},          //NC          |GPIO91             |NFRDY        |RMCRSDV      |PHY_CLK      |            |            |              |           |
        {92,        2},          //NC          |GPIO92             |NFOE         |RMTXEN       |             |            |            |              |           |
        {93,        2},          //NC          |GPIO93             |NFCLE        |RMTXD[1]     |             |            |            |              |           |
        {94,        2},          //NC          |GPIO94             |NFALE        |RMTXD[0]     |             |            |            |              |           |
        {95,        2},          //NC          |GPIO95             |NFWE         |RMCLK        |             |            |            |              |           |
        {96,        2},          //NC          |GPIO96             |NFDATA[7]    |RMRXD[1]     |             |            |            |              |           |
        {97,        2},          //NC          |GPIO97             |NFDATA[6]    |RMRXD[0]     |             |            |            |              |           |
        {98,        2},          //NC          |GPIO98             |NFDATA[5]    |SPIHOLD      |NF_SPIHOLD   |            |            |              |           |
        {99,        2},          //NC          |GPIO99             |NFDATA[4]    |SPISCK       |NF_SPISCK    |            |            |              |           |
        {100,       2},          //NC          |GPIO100            |NFDATA[3]    |SPIMOSI      |NF_SPIMOSI   |            |            |              |           |
        {101,       2},          //NC          |GPIO101            |NFDATA[2]    |SPICSN       |NF_SPICSN    |            |            |              |           |
        {102,       2},          //NC          |GPIO102            |NFDATA[1]    |SPIMISO      |NF_SPIMISO   |            |            |              |           |
        {103,       2},          //NC          |GPIO103            |NFDATA[0]    |SPIWP        |NF_SPIWP     |            |            |              |           |
        {104,       0},          //NC          |GPIO104            |UART3TX      |             |             |            |            |              |           |
        {105,       0},          //NC          |GPIO105            |UART3RX      |             |             |            |            |              |           |
        {106,       0},          //NC          |GPIO106            |UART3CTS     |             |             |            |            |              |           |
        {107,       0},          //NC          |GPIO107            |UART3RTS     |             |             |            |            |              |           |
        {ARRAY_END_FLAG_U16, NOT_CONFIG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
	{0,  7, GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT, GX_GPIO_LOW},// fd650, sda
	{1,  8, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},// fd650, clk
	{3,  91, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//mute control for cvbs, 1:mute; 0:unmute

	{13, 17, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//power cont, 1:normal, 0:standby
	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* Board specific initialization routine */
int board_init(void)
{
	enable_dac(ENABLE_CVBS);
	gpio_init();
	mulpin_init();

	ts_mux_config(TS_MUX_DVB_TS_OUT1, TS_IN_TS2_P_S, -1);
	//cmdline_add("av_extend:0xa0305000=0x12345678,0xa0306000=0x87654321");
	//cmdline_add("ex_extend:ex...");

#ifdef ENABLE_USB_RECOVER
extern int usb_recover_proc(void);
	g_board_extend.func_usb_update = usb_recover_proc;	//register function pointer
#endif

	return 0;
}

//panel function description:
//you can init panel && show BOOT by panel_show_call()
//you can scan panel && return 0 by panel_update_call() to enter ota forcibly
int panel_show_call(void)
{
	loader_panel_init(0,1);
	loader_panel_display();
	return 0;
}

int panel_update_call(void)
{
	return -1;
}

