
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
enum vout_dac_case default_dac_case_svpu = CASE_GBRX;
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
#ifdef CONFIG_ACCELERATOR
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0)=0x868801b1;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1)=0x00223446;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2)=0x00003446;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG3)=0x00000001;
#else
#define DDR_OUTPUT_DRIVER 0x6

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<24);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (3<<0) | (3<<11);
#endif
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
/*       pin_id  func_sel                     func0         func1                 func2           func3          func4          func5        func6         func7  */
        {0,       0},          //NC          | UART1_TX   | GPIO00            |  SCPU_UART_TX | SDA2        |   AUART_TX   |             |             |         |
        {1,       0},          //NC          | UART1_RX   | GPIO01            |  SCPU_UART_RX | SCL2        |   AUART_RX   |             |             |         |
        {2,       0},          //NC          | SPI_SCK    | GPIO02            |               |             |              |             |             |         |
        {3,       0},          //NC          | SPI_MOSI   | GPIO03            |               |             |              |             |             |         |
        {4,       0},          //NC          | SPI_CSN    | GPIO04            |               |             |              |             |             |         |
        {5,       0},          //NC          | SPI_MISO   | GPIO05            |               |             |              |             |             |         |
        {6,       0},          //NC          | IR         | GPIO06(PMUPORT00) |               |             |              |             |             |         |
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {7,       1},          //NC          | DBGTDI     | GPIO07(PMUPORT01) |  SDBGTDI      | TSI_DATA[7] |  TSO_DATA[7] |             |             |         |
        {8,       1},          //NC          | DBGTDO     | GPIO08(PMUPORT02) |  SDBGTDO      | TSI_DATA[6] |  TSO_DATA[6] |             |             |         |
        {9,       1},          //NC          | DBGTMS     | GPIO09(PMUPORT03) |  SDBGTMS      | TSI_DATA[5] |  TSO_DATA[5] |             |             |         |
        {10,      0},          //NC          | DBGTCK     | GPIO10(PMUPORT04) |  SDBGTCK      | TSI_DATA[4] |  TSO_DATA[4] |             |             |         |
        {11,      5},          //NC          | DBGTRST    | GPIO11(PMUPORT05) |  SDBGTRST     | TSI_DATA[3] |  TSO_DATA[3] |     CEC     |             |         |
#endif
        {12,      0},          //NC          | SC1CLK     | GPIO12            |  SDBGTDI      | TSI_DATA[7] |  TSO_DATA[2] |             |             |         |
        {13,      0},          //NC          | SC1RST     | GPIO13            |  SDBGTDO      | TSI_DATA[6] |  TSO_DATA[1] |             |             |         |
        {14,      0},          //NC          | SC1PWR     | GPIO14            |  SDBGTMS      | TSI_DATA[5] |  TSO_DATA[0] |             |             |         |
        {15,      0},          //NC          | SC1CD      | GPIO15            |  SDBGTCK      | TSI_DATA[4] |  TSO_CLK     |             |             |         |
        {16,      0},          //NC          | SC1DATA    | GPIO16            |  SDBGTRST     | TSI_DATA[3] |  TSO_SYNC    |             |             |         |
        {17,      1},          //NC          | UART2_RX   | GPIO17            |               | TSI_DATA[2] |  TSO_VALID   |  SDA2       | HDCP_UARTRX |         |
        {18,      3},          //NC          | UART2_TX   | GPIO18            |               | TSI_DATA[1] |  TSO_SYNC    |  SCL2       | HDCP_UARTTX |         |
        {19,      3},          //NC          | AGC2       | GPIO19            |  SCPU_UART_TX | TSI_DATA[0] |  TSO_CLK     |  SPDIF      | AUART_TX    | CEC     |
        {20,      0},          //NC          | AGC1       | GPIO20            |               | TSI_CLK     |  TSO_DATA[0] |             |             |         |
        {21,      0},          //NC          | SDA1       | GPIO21            |  DVB_SDAT     | TSI_SYNC    |  TSO_DATA[1] |  CEC        |             |         |
        {22,      0},          //NC          | SCL1       | GPIO22            |  DVB_SCLT     | TSI_VALID   |  TSO_DATA[2] |  AGC1       |             |         |
        {23,      0},          //NC          | DDCSDA     | GPIO23            |  SDAS         | SDA3        |  SDA2        |             |             |         |
        {24,      0},          //NC          | DDCSCL     | GPIO24            |  SCLS         | SCL3        |  SCL2        |             |             |         |
        {25,      0},          //NC          | SPDIF      | GPIO25            | OTP_AVDD_EN   |             |              |             |             |         |
        {ARRAY_END_FLAG_U16, NOT_CONFIG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
    {3,   17, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//Mute
	{11,  7, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//POWER_CONTRL  0:off  1:on
	{0,   8, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW},//PANEL_SDA
	{1,   9, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW},//PANEL_CLK
	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* Board specific initialization routine */
int board_init(void)
{
	unsigned int value;

	//enable_dac(ENABLE_CVBS);
	gpio_init();
	mulpin_init();

	//cmdline_add("av_extend:0xa0305000=0x12345678,0xa0306000=0x87654321");
	//cmdline_add("ex_extend:ex...");

#ifdef ENABLE_USB_RECOVER
extern int usb_recover_proc(void);
	g_board_extend.func_usb_update = usb_recover_proc;	//register function pointer
#endif

	value = *(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_TS_OUT_SEL);
	value &= ~(0x3<<4);
	value |= (0x1<<4);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_TS_OUT_SEL) = value;

	//ts_mode_config(1, 1, 2, 0, 1, 0, -1);	//set ts1 to serial ts, and mode is 2, no valid, sync, little endian
	//ts_mode_config(0, 0, 2, 1, 1, 0, -1);	//set ts0 to serial ts, and mode is 2, valid, sync, little endian

	return 0;
}

//panel function description:
//you can init panel && show BOOT by panel_show_call()
//you can scan panel && return 0 by panel_update_call() to enter ota forcibly
int panel_show_call(void)
{
	loader_panel_init(0, 1);
	loader_panel_display();
	return 0;
}

int panel_update_call(void)
{
	return -1;
}

