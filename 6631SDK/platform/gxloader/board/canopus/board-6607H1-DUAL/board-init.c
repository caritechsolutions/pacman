
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
#define DDR_OUTPUT_DRIVER 0x7
#define DDR_ADDR_OUTPUT_DRIVER 0x4
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<16) | (DDR_OUTPUT_DRIVER<<19) | (DDR_OUTPUT_DRIVER<<22) | (DDR_OUTPUT_DRIVER<<25) | (DDR_OUTPUT_DRIVER<<28); //feed, odt, dm, cmd
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11); //dqs and clk
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (DDR_ADDR_OUTPUT_DRIVER<<11); //cmd
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG3) |= (DDR_OUTPUT_DRIVER<<16) | (DDR_OUTPUT_DRIVER<<20) | (DDR_OUTPUT_DRIVER<<24) |(DDR_OUTPUT_DRIVER<<28); //dq
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG5) |= (DDR_ADDR_OUTPUT_DRIVER<<0) |(DDR_ADDR_OUTPUT_DRIVER<<3) |(DDR_ADDR_OUTPUT_DRIVER<<6) |(DDR_ADDR_OUTPUT_DRIVER<<9) |(DDR_ADDR_OUTPUT_DRIVER<<12) |(DDR_ADDR_OUTPUT_DRIVER<<15) |(DDR_ADDR_OUTPUT_DRIVER<<18) |(DDR_ADDR_OUTPUT_DRIVER<<21) |(DDR_ADDR_OUTPUT_DRIVER<<24) |(DDR_ADDR_OUTPUT_DRIVER<<27); //addr
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG6) |= (DDR_ADDR_OUTPUT_DRIVER<<0) |(DDR_ADDR_OUTPUT_DRIVER<<3) |(DDR_ADDR_OUTPUT_DRIVER<<6) |(DDR_ADDR_OUTPUT_DRIVER<<9) |(DDR_ADDR_OUTPUT_DRIVER<<12) |(DDR_ADDR_OUTPUT_DRIVER<<16) |(DDR_ADDR_OUTPUT_DRIVER<<19) |(DDR_ADDR_OUTPUT_DRIVER<<22); //addr, ba
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
/*       pin_id  func_sel                          fun0               fun1         fun2         fun3         fun4       fun5         fun6         fun7         */
        {0,        0},          //NC          |GPIO00(PMUPORT00) |MD           |SDIO_CLK     |            |           |            |            |HDMI_CH2_R[2]   |
        {1,        0},          //NC          |GPIO01(PMUPORT01) |MDC          |SDIO_CMD     |ADCQDATA[6] |           |            |            |HDMI_CH2_R[1]   |
        {2,        0},          //NC          |GPIO02(PMUPORT02) |RMCRSDV      |SDIO_DATA[0] |ADCQDATA[5] |           |            |            |HDMI_CH2_R[0]   |
        {3,        0},          //NC          |GPIO03(PMUPORT03) |RMTXEN       |SDIO_DATA[1] |ADCQDATA[4] |           |            |            |HDMI_CH1_G[9]   |
        {4,        0},          //NC          |GPIO04(PMUPORT04) |RMTXD[1]     |SDIO_DATA[2] |ADCQDATA[3] |           |            |            |HDMI_CH1_G[8]   |
        {5,        0},          //NC          |GPIO05(PMUPORT05) |RMTXD[0]     |SDIO_DATA[3] |ADCQDATA[2] |           |            |            |HDMI_CH1_G[7]   |
        {6,        0},          //NC          |GPIO06(PMUPORT06) |RMCLK        |SDIO_PWR     |ADCQDATA[1] |           |            |            |HDMI_CH1_G[6]   |
        {7,        5},          //NC          |GPIO07(PMUPORT07) |RMRXD[1]     |SDIO_CD      |ADCQDATA[0] |UART1_CTS  |MD          |            |                |
        {8,        5},          //NC          |GPIO08(PMUPORT08) |RMRXD[0]     |SDIO_WP      |ADCCLKOUT   |UART1_RTS  |MDC         |            |                |
        {9,        1},          //NC          |GPIO09(PMUPORT09) |UART1TX      |SUARTTX      |AUARTTX     |           |            |            |                |
        {10,       1},          //NC          |GPIO10(PMUPORT10) |UART1RX      |SUARTRX      |            |           |            |            |                |
        {11,       1},          //NC          |GPIO11(PMUPORT11) |IR           |             |            |           |            |            |                |
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {12,       0},          //NC          |GPIO12(PMUPORT12) |DBGTDI       |SDBGTDI      |            |           |            |            |                |
        {13,       0},          //NC          |GPIO13(PMUPORT13) |DBGTDO       |SDBGTDO      |            |           |            |            |                |
        {14,       0},          //NC          |GPIO14(PMUPORT14) |DBGTMS       |SDBGTMS      |            |           |            |            |                |
        {15,       0},          //NC          |GPIO15(PMUPORT15) |DBGTCK       |SDBGTCK      |            |           |            |            |                |
        {16,       0},          //NC          |GPIO16(PMUPORT16) |DBGTRST      |SDBGTRST     |CEC         |           |            |            |                |
#endif
        {17,       0},          //NC          |GPIO17(PMUPORT17) |TSI1DATA[7]  |TSIS1DATA[1] |NFRDY       |SPIHOLD    |NF_SPIHOLD  |ADCIDATA[9] |                |
        {18,       2},          //NC          |GPIO18            |TSI1DATA[6]  |TSIS1DATA[0] |NFOE        |SPISCK     |NF_SPISCK   |UART2TX     |ADCIDATA[8]     |
        {19,       2},          //NC          |GPIO19            |TSI1DATA[5]  |TSIS1CLK     |NFCLE       |SPIMOSI    |NF_SPIMOSI  |UART2RX     |ADCIDATA[7]     |
        {20,       2},          //NC          |GPIO20            |TSI1DATA[4]  |TSIS1SYNC    |NFALE       |SPICSN     |NF_SPICSN   |SDA2        |ADCIDATA[6]     |
        {21,       0},          //NC          |GPIO21            |TSI1DATA[3]  |TSIS1VALID   |NFWE        |SPIMISO    |NF_SPIMISO  |SCL2        |ADCIDATA[5]     |
        {22,       2},          //NC          |GPIO22            |TSI1DATA[2]  |SC1CLK       |NFDATA[7]   |SPIWP      |NF_SPIWP    |DBGTDI      |ADCIDATA[4]     |
        {23,       2},          //NC          |GPIO23            |TSI1DATA[1]  |SC1RST       |NFDATA[6]   |DBGTDO     |ADCIDATA[3] |            |HDMI_CH1_G[5]   |
        {24,       2},          //NC          |GPIO24            |TSI1DATA[0]  |SC1PWR       |NFDATA[5]   |DBGTMS     |ADCIDATA[2] |            |HDMI_CH1_G[4]   |
        {25,       2},          //NC          |GPIO25            |TSI1CLK      |SC1CD        |NFDATA[4]   |DBGTCK     |ADCIDATA[1] |            |HDMI_CH1_G[3]   |
        {26,       2},          //NC          |GPIO26            |TSI1SYNC     |SC1DATA      |NFDATA[3]   |DBGTRST    |ADCIDATA[0] |            |HDMI_CH1_G[2]   |
        {27,       0},          //NC          |GPIO27            |TSI1VALID    |             |NFDATA[2]   |           |            |            |                |
        {28,       0},          //NC          |GPIO28            |             |             |NFDATA[1]   |           |            |            |                |
        {29,       0},          //NC          |GPIO29            |             |             |NFDATA[0]   |           |            |            |                |
        {30,       0},          //NC          |GPIO30            |TSOVALID     |TSI3VALID    |TSI2VALID   |           |            |            |HDMI_CH1_G[1]   |
        {31,       0},          //NC          |GPIO31            |TSODATA[7]   |TSIS3DATA[1] |TSI2DATA[7] |           |            |            |                |
        {32,       0},          //NC          |GPIO32            |TSODATA[6]   |TSIS3DATA[0] |TSI2DATA[6] |           |            |            |                |
        {33,       0},          //NC          |GPIO33            |TSODATA[5]   |TSIS3CLK     |TSI2DATA[5] |           |            |            |                |
        {34,       0},          //NC          |GPIO34            |TSODATA[4]   |TSIS3SYNC    |TSI2DATA[4] |           |            |            |                |
        {35,       0},          //NC          |GPIO35            |TSODATA[3]   |TSIS3VALID   |TSI2DATA[3] |           |            |            |                |
        {36,       0},          //NC          |GPIO36            |TSODATA[2]   |SC1CLK       |TSI2DATA[2] |           |            |            |                |
        {37,       0},          //NC          |GPIO37            |TSODATA[1]   |SC1RST       |TSI2DATA[1] |           |            |            |                |
        {38,       0},          //NC          |GPIO38            |TSODATA[0]   |SC1PWR       |TSI2DATA[0] |           |            |            |                |
        {39,       0},          //NC          |GPIO39            |TSOCLK       |SC1CD        |TSI2CLK     |           |            |            |                |
        {40,       0},          //NC          |GPIO40            |TSOSYNC      |SC1DATA      |TSI2SYNC    |           |            |            |                |
        {41,       0},          //NC          |GPIO41            |TSODATA[7]   |TSI3DATA[7]  |NFDATA[2]   |TSIS2VALID |SC1CLK      |            |HDMI_CH1_G[0]   |
        {42,       0},          //NC          |GPIO42            |TSODATA[6]   |TSI3DATA[6]  |NFDATA[1]   |TSIS2DATA0 |SC1RST      |            |HDMI_CH0_B[9]   |
        {43,       0},          //NC          |GPIO43            |TSODATA[5]   |TSI3DATA[5]  |NFDATA[0]   |TSIS2CLK   |SC1PWR      |            |HDMI_CH0_B[8]   |
        {44,       0},          //NC          |GPIO44            |TSODATA[4]   |TSI3DATA[4]  |            |TSIS2SYNC  |SC1CD       |            |HDMI_CH0_B[7]   |
        {45,       3},          //NC          |GPIO45            |TSODATA[3]   |TSI3DATA[3]  |HVSEL       |SDA3       |SC1DATA     |SDA3        |HDMI_CH0_B[6]   |
        {46,       3},          //NC          |GPIO46            |TSODATA[2]   |TSI3DATA[2]  |DiSEqCi     |SCL3       |            |SCL3        |HDMI_CH0_B[5]   |
        {47,       3},          //NC          |GPIO47            |TSODATA[1]   |TSI3DATA[1]  |DiSEqCo     |AGC2       |UART3TX     |            |                |
        {48,       3},          //NC          |GPIO48            |TSODATA[0]   |TSI3DATA[0]  |AGC         |           |UART3RX     |            |HDMI_CH0_B[4]   |
        {49,       3},          //NC          |GPIO49            |TSOCLK       |TSI3CLK      |SDA1        |SDA_T      |UART3CTS    |            |HDMI_CH0_B[3]   |
        {50,       3},          //NC          |GPIO50            |TSOSYNC      |TSI3SYNC     |SCL1        |SCL_T      |UART3RTS    |            |HDMI_CH0_B[2]   |
        {51,       0},          //NC          |GPIO51            |HVSEL        |SDA4         |            |           |            |            |HDMI_CH0_B[1]   |
        {52,       0},          //NC          |GPIO52            |DiSEqCi      |SCL4         |            |           |            |            |HDMI_CH0_B[0]   |
        {53,       0},          //NC          |GPIO53            |DiSEqCo      |AGC2         |            |           |            |            |PHY_HDMI_TDCLK  |
        {54,       0},          //NC          |GPIO54            |AGC          |             |            |           |            |            |PHY_HDMI_HPDDET |
        {55,       0},          //NC          |GPIO55            |SDA2         |SDA_T        |            |           |            |            |PHY_HDMI_LOCKED |
        {56,       0},          //NC          |GPIO56            |SCL2         |SCL_T        |            |           |            |            |PHY_HDMI_RXDET  |
        {57,       0},          //NC          |GPIO57            |             |             |            |           |            |            |                |
        {58,       0},          //NC          |GPIO58            |             |             |            |           |            |            |                |
        {59,       0},          //NC          |GPIO59            |             |             |            |           |            |            |                |
        {60,       0},          //NC          |GPIO60            |             |             |            |           |            |            |                |
        {61,       0},          //NC          |GPIO61            |             |             |            |           |            |            |                |
        {62,       0},          //NC          |GPIO62            |             |             |            |           |            |            |                |
        {63,       0},          //NC          |GPIO63            |SDIO_PWR     |             |            |           |            |            |                |
        {64,       0},          //NC          |GPIO64            |SDIO_CLK     |             |            |           |            |            |PHY_HDMI_INT    |
        {65,       0},          //NC          |GPIO65            |SDIO_CMD     |UART2TX      |SDA1        |           |            |            |PHY_HDMI_REFCLK |
        {66,       0},          //NC          |GPIO66            |SDIO_DATA[0] |UART2RX      |SCL1        |           |            |            |PHY_HDMI_PRECLK |
        {67,       0},          //NC          |GPIO67            |SDIO_DATA[1] |UART2_CTS    |            |           |            |            |PHY_HDMI_PCLK   |
        {68,       0},          //NC          |GPIO68            |SDIO_DATA[2] |UART2_RTS    |            |           |            |            |                |
        {69,       0},          //NC          |GPIO69            |SDIO_DATA[3] |SPDIF        |            |           |            |            |                |
        {70,       0},          //NC          |GPIO70            |SDIO_CD      |             |            |           |            |            |                |
        {71,       0},          //NC          |GPIO71            |SDIO_WP      |             |            |           |            |            |                |
        {72,       1},          //NC          |GPIO72            |DDCSDA       |SDAS         |            |           |            |            |                |
        {73,       1},          //NC          |GPIO73            |DDCSCL       |SCLS         |            |           |            |            |                |
        {74,       1},          //NC          |GPIO74            |HPD          |             |            |           |            |            |                |
        {75,       1},          //NC          |GPIO75            |SPDIF        |             |            |           |            |            |                |
        {76,       0},          //NC          |GPIO76            |PHY_CLK      |             |            |           |            |            |                |
        {78,       1},          //NC          |GPIO78            |RMCRSDV      |             |            |           |            |            |                |
        {81,       1},          //NC          |GPIO81            |RMTXEN       |             |            |           |            |            |                |
        {82,       1},          //NC          |GPIO82            |RMTXD[1]     |             |            |           |            |            |                |
        {83,       1},          //NC          |GPIO83            |RMTXD[0]     |             |            |           |            |            |                |
        {84,       1},          //NC          |GPIO84            |RMCLK        |             |            |           |            |            |                |
        {85,       1},          //NC          |GPIO85            |RMRXD[1]     |             |            |           |            |            |                |
        {86,       1},          //NC          |GPIO86            |RMRXD[0]     |             |            |           |            |            |                |
        {87,       0},          //NC          |GPIO87            |MD           |PHY_CLK      |            |           |            |            |                |
        {90,       0},          //NC          |GPIO90            |MDC          |             |            |           |            |            |                |
        {91,       0},          //NC          |GPIO91            |NFRDY        |SPIHOLD      |NF_SPIHOLD  |PHY_CLK    |            |            |                |
        {92,       2},          //NC          |GPIO92            |NFOE         |SPISCK       |NF_SPISCK   |           |            |            |                |
        {93,       2},          //NC          |GPIO93            |NFCLE        |SPIMOSI      |NF_SPIMOSI  |           |            |            |                |
        {94,       2},          //NC          |GPIO94            |NFALE        |SPICSN       |NF_SPICSN   |           |            |            |                |
        {95,       2},          //NC          |GPIO95            |NFWE         |SPIMISO      |NF_SPIMISO  |           |            |            |                |
        {96,       0},          //NC          |GPIO96            |NFDATA[7]    |SPIWP        |NF_SPIWP    |           |            |            |                |
        {97,       0},          //NC          |GPIO97            |NFDATA[6]    |             |            |           |            |            |HDMI_CH2_R[9]   |
        {98,       0},          //NC          |GPIO98            |NFDATA[5]    |UART3TX      |            |           |            |            |HDMI_CH2_R[8]   |
        {99,       0},          //NC          |GPIO99            |NFDATA[4]    |UART3RX      |            |           |            |            |HDMI_CH2_R[7]   |
        {100,      0},          //NC          |GPIO100           |NFDATA[3]    |UART3CTS     |            |           |            |            |HDMI_CH2_R[6]   |
        {101,      0},          //NC          |GPIO101           |NFDATA[2]    |UART3RTS     |ADCQDATA[9] |           |            |            |HDMI_CH2_R[5]   |
        {102,      0},          //NC          |GPIO102           |NFDATA[1]    |SDA3         |ADCQDATA[8] |           |            |            |HDMI_CH2_R[4]   |
        {103,      0},          //NC          |GPIO103           |NFDATA[0]    |SCL3         |ADCQDATA[7] |           |            |            |HDMI_CH2_R[3]   |
        {ARRAY_END_FLAG_U16, NOT_CONFIG}
};
struct gpio_entry_bootloader g_gpio_table[] = {
	{0,  14, GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT, GX_GPIO_LOW},// fd650, sda
	{1,  15, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},// fd650, clk
	{4,  12, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//LNB2 power, 1:enable, 0:disable
	{5,  13, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//LNB1 power, 1:enable, 0:disable
	{16, 17, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//power cont, 1:normal, 0:standby
	{20, 16, GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT, GX_GPIO_LOW},// LNB1 short, 0: normal, 1: short
	{7,  91, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW}, //PHY reset, 0: reset,  1: normal
	{21, 96, GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT, GX_GPIO_LOW}, //LNB2 Short, 0: normal, 1: short
	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* Board specific initialization routine */
int board_init(void)
{
	enable_dac(ENABLE_CVBS);
	gpio_init();
	mulpin_init();
	//cmdline_add("av_extend:0xa0305000=0x12345678,0xa0306000=0x87654321");
	//cmdline_add("ex_extend:ex...");

#ifdef ENABLE_USB_RECOVER
extern int usb_recover_proc(void);
	g_board_extend.func_usb_update = usb_recover_proc;	//register function pointer
#endif
	ts_mode_config(1, 1, 0x88, 0, 1, 1, 1);
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

