
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
enum vout_dac_case default_dac_case_svpu = CASE_GBRX;
enum vout_dac_case default_dac_case_vpu = CASE_GBRX;

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
#define DDR_OUTPUT_DRIVER 0x4

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<24);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
	                                       /* chip_package: generic */
 /* chip_core_num | bit0 | bit1    | bit2       | func_select */   /* package_num | func_list */
#ifndef CONFIG_DBG_PIN_NO_MULTI
		{4,         0,     44,       MP_INV_V,    0},              // NC          | DBGTDI_PORT00_ADCODATA0
		{5,         1,     45,       MP_INV_V,    1},              // NC          | DBGTDO_PORT01_ADCODATA1
		{6,         2,     46,       MP_INV_V,    1},              // NC          | DBGTMS_PORT02_ADCODATA2
		{7,         3,     47,       MP_INV_V,    1},              // NC          | DBGTCK_PORT03_ADCODATA3
		{8,         4,     48,       MP_INV_V,    1},              // NC          | DBGTRST_PORT04_ADCODATA4
#endif
		{26,        5,     32,       64,          1},              // NC          | SC1CLK_PORT05_TSI1DATA0_AJTDI_DVBFSYNC_ADCODATA5
		{27,        6,     33,       65,          0},              // NC          | SC1RST_PORT06_TSI1DATA1_AJTDO_ADCODATA6
		{28,        7,     34,       66,          1},              // NC          | SC1PWR_PORT07_TSI1DATA2_AJTMS_ADCODATA7
		{29,        8,     35,       67,          1},              // NC          | SC1CD_PORT08_TSI1DATA3_AJTCK_ADCODATA8
		{31,        9,     36,       68,          1},              // NC          | SC1DATA_PORT09_TSI1DATA4_AJRST_ADCODATA9
		{32,        10,    37,       69,          2},              // NC          | DiSEqCi_PORT10_TSI1DATA5_ADCOCLK
		{33,        11,    38,       MP_INV_V,    2},              // NC          | HVSEL_PORT11_TSI1DATA6
		{34,        12,    39,       MP_INV_V,    0},              // NC          | DiSEqCo_PORT12_TSI1DATA7
		{35,        13,    40,       MP_INV_V,    0},              // NC          | AGC_PORT13_TSI1CLK
		{54,        14,    51,       MP_INV_V,    0},              // NC          | SDA1_PORT14_SDAT
		{55,        15,    52,       MP_INV_V,    0},              // NC          | SCL1_PORT15_SCLT
		{56,        16,    41,       MP_INV_V,    0},              // NC          | UART1TX_PORT16_AUARTTX_HDMIDBT0
		{57,        17,    42,       MP_INV_V,    0},              // NC          | UART1RX_PORT17_HDMIDBT1
		{126,       18,    49,       MP_INV_V,    0},              // NC          | DDCSDA_PORT18_SDAS
		{127,       19,    50,       MP_INV_V,    0},              // NC          | DDCSCL_PORT19_SCLS
		{128,       20,    43,       MP_INV_V,    0},              // NC          | SPDIF_PORT20_OTPAVDDEN

		{ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
	{0, 7,  GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT,  GX_GPIO_LOW},//PANEL DATA
	{1, 8,  GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//PANEL CLK
	{3, 5, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW},
	{5, 1, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},			//lnb power
	{14, 4, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW},			//demod reset
	{13, 2, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW},			//13/18 select
	{15, 3, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},		//Ant Power short
	{20, 9, GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT, GX_GPIO_LOW},		//LNB short 0: normal 1: short
/*	{9,   5,  GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{10,  10, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{11,  8,  GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT,  GX_GPIO_HIGH},
	{DWSPI_CS0_VIRTUAL_GPIO, 64, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},*/

	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* Board specific initialization routine */
int board_init(void)
{
	//enable_dac(ENABLE_CVBS);
	gpio_init();
	mulpin_init();
	gx_gpio_init();
	udelay(100*1000);
	gx_gpio_setlevel(14, GX_GPIO_HIGH);

	//cmdline_add("av_extend:0xa0305000=0x12345678,0xa0306000=0x87654321");
	//cmdline_add("ex_extend:ex...");

#ifdef ENABLE_USB_RECOVER
extern int usb_recover_proc(void);
	g_board_extend.func_usb_update = usb_recover_proc;	//register function pointer
#endif

	// 6605s+dvbt2
	ts_mode_config(1, 1, 4, 0, 1, 1, 1);	//set ts1

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

