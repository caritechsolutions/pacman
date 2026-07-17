
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

/* Note: In order to achieve the stage1 code of gemini and taurus the same, the configuration of ddr_config_patch in each board-init.c of gemini and taurus must also be consistent  */
void __attribute__((section(".reset_patch"))) ddr_config_patch(void)
{
#define DDR_OUTPUT_DRIVER 0x6

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<24);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (3<<0) | (3<<11);
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
                                            /* chip_package: generic */
 /*      size | chip_core_num | 0_bit | 1_bit | 2_bit | func_sel */ /* package_num | func_list */
        {_BOTTOM_,   41,     14,          MP_INV_V,    MP_INV_V,  0},           //NC          |IR/PORT00(PMUPORT00)
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {_BOTTOM_,   42,     15,          69,          MP_INV_V,  0},           //NC          |DBGTDI/PORT01(PMUPORT01)/SDBGTDI
        {_BOTTOM_,   43,     16,          70,          MP_INV_V,  0},           //NC          |DBGTDO/PORT02(PMUPORT02)/SDBGTDO
        {_BOTTOM_,   44,     17,          71,          MP_INV_V,  0},           //NC          |DBGTMS/PORT03(PMUPORT03)/SDBGTMS
        {_BOTTOM_,   45,     18,          72,          MP_INV_V,  0},           //NC          |DBGTCK/PORT04(PMUPORT04)/SDBGTCK
        {_BOTTOM_,   46,     19,          73,          MP_INV_V,  0},           //NC          |DBGTRST/PORT05(PMUPORT05)/SDBGTRST/CEC
#endif
        {_BOTTOM_,   47,     20,          74,          128,       0},           //NC          |TSIDATA7/PORT06(PMUPORT06)/TSODATA7/SC1CLK/ADCBIT9/SDBGTDI
        {_BOTTOM_,   48,     21,          75,          129,       0},           //NC          |TSIDATA6/PORT07(PMUPORT07)/TSODATA6/SC1RST/ADCBIT8/SDBGTDO/NC/I2SDATA
        {_BOTTOM_,   49,     22,          76,          130,       0},           //NC          |TSIDATA5/PORT08(PMUPORT08)/TSODATA5/SC1PWR/ADCBIT7/SDBGTMS/NC/I2SBCK
        {_BOTTOM_,   50,     23,          77,          131,       0},           //NC          |TSIDATA4/PORT09(PMUPORT09)/TSODATA4/SC1CD/ADCBIT6/SDBGTCK/NC/I2SLRCK
        {_RIGHT_,    1,      24,          78,          132,       0},           //NC          |TSIDATA3/PORT10(PMUPORT10)/TSODATA3/SC1DATA/ADCBIT5/SDBGTRST/NC/I2SMCK
        {_RIGHT_,    13,     25,          79,          135,       0},           //NC          |TSIDATA2/PORT11(PMUPORT11)/TSODATA2/I2SDATA/ADCBIT4
        {_RIGHT_,    14,     26,          80,          136,       0},           //NC          |TSIDATA1/PORT12(PMUPORT12)/TSODATA1/I2SBCK/ADCBIT3
        {_RIGHT_,    15,     27,          81,          137,       0},           //NC          |TSIDATA0/PORT13(PMUPORT13)/TSODATA0/I2SLRCK/ADCBIT2
        {_RIGHT_,    16,     28,          82,          138,       0},           //NC          |TSICLK/PORT14(PMUPORT14)/TSOCLK/I2SMCK/ADCBIT1
        {_RIGHT_,    17,     29,          83,          133,       0},           //NC          |TSISYNC/PORT15/TSOSYNC/UART2TX/ADCBIT0/SDA2
        {_RIGHT_,    18,     30,          84,          134,       0},           //NC          |TSIVALID/PORT16/TSOVALID/UART2RX/ADCCLK/SCL2
        {_RIGHT_,    30,     31,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA3/PORT17
        {_RIGHT_,    31,     32,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA2/PORT18
        {_RIGHT_,    32,     33,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA1/PORT19
        {_RIGHT_,    33,     34,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISDATA0/PORT20
        {_RIGHT_,    34,     35,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISCLK/PORT21
        {_RIGHT_,    35,     36,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISSYNC/PORT22
        {_RIGHT_,    36,     37,          MP_INV_V,    MP_INV_V,  0},           //NC          |TSISVALID/PORT23
        {_RIGHT_,    37,     38,          90,          MP_INV_V,  0},           //NC          |SDA2/PORT24/ADCRDY
        {_RIGHT_,    38,     39,          91,          MP_INV_V,  0},           //NC          |SCL2/PORT25/ADCSYNC
        {_RIGHT_,    39,     40,          MP_INV_V,    MP_INV_V,  1},           //NC          |AGC2/PORT26
        {_RIGHT_,    44,     41,          85,          MP_INV_V,  0},           //NC          |SDA1/PORT27/SDAT
        {_RIGHT_,    45,     42,          86,          MP_INV_V,  0},           //NC          |SCL1/PORT28/SCLT
        {_RIGHT_,    46,     43,          MP_INV_V,    MP_INV_V,  0},           //NC          |AGC/PORT29
	/* PORT30 ~ PORT35 默认为普通GPIO功能,无需管脚复用 */
        {_TOP_,      22,     49,          MP_INV_V,    MP_INV_V,  0},           //NC          |PORT36/DDCSDA
        {_TOP_,      23,     50,          MP_INV_V,    MP_INV_V,  0},           //NC          |PORT37/DDCSCL
        {_LEFT_,     7,      44,          87,          MP_INV_V,  0},           //NC          |DDCSDA/PORT38/SDAS
        {_LEFT_,     8,      45,          88,          MP_INV_V,  0},           //NC          |DDCSCL/PORT39/SCLS
        {_LEFT_,     9,      46,          89,          MP_INV_V,  0},           //NC          |SPDIF/PORT40
        {_LEFT_,     16,     47,          MP_INV_V,    MP_INV_V,  0},           //NC          |UART2TX/PORT41
        {_LEFT_,     17,     48,          MP_INV_V,    MP_INV_V,  0},           //NC          |UART2RX/PORT42
	/* PORT43 ~ PORT45 默认为普通GPIO功能,无需管脚复用 */
        {_BOTTOM_,   3,      0,           MP_INV_V,    MP_INV_V,  0},           //NC          |SPD_LED/PORT47
        {_BOTTOM_,   4,      1,           64,          MP_INV_V,  0},           //NC          |LINK_LED/PORT48/UART2TX/SDA2
        {_BOTTOM_,   5,      2,           65,          MP_INV_V,  0},           //NC          |ACT_LED/PORT49/UART2RX/SCL2
        {_BOTTOM_,   6,      3,           66,          MP_INV_V,  0},           //NC          |UART1TX/PORT50/AUARTTX/SUARTTX
        {_BOTTOM_,   7,      4,           MP_INV_V,    MP_INV_V,  0},           //NC          |UART1RX/PORT51
        {_BOTTOM_,   22,     5,           MP_INV_V,    MP_INV_V,  0},           //NC          |I2SDATA/PORT52
        {_BOTTOM_,   23,     6,           MP_INV_V,    MP_INV_V,  0},           //NC          |I2SBCK/PORT53
        {_BOTTOM_,   24,     7,           MP_INV_V,    MP_INV_V,  0},           //NC          |I2SLRCK/PORT54
        {_BOTTOM_,   25,     8,           MP_INV_V,    MP_INV_V,  0},           //NC          |I2SMCK/PORT55
        {_BOTTOM_,   26,     9,           67,          MP_INV_V,  0},           //NC          |SC1CLK/PORT56/SDA2
        {_BOTTOM_,   27,     10,          68,          MP_INV_V,  0},           //NC          |SC1RST/PORT57/SCL2
        {_BOTTOM_,   29,     11,          MP_INV_V,    MP_INV_V,  0},           //NC          |SC1PWR/PORT58
        {_BOTTOM_,   20,     12,          MP_INV_V,    MP_INV_V,  0},           //NC          |SC1CD/PORT59
        {_BOTTOM_,   30,     13,          MP_INV_V,    MP_INV_V,  0},           //NC          |SC1DATA/PORT60
        {ARRAY_END_FLAG, ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, NOT_CONFIG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
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

	//cmdline_add("av_extend:0xa0305000=0x12345678,0xa0306000=0x87654321");
	//cmdline_add("ex_extend:ex...");

#ifdef ENABLE_USB_RECOVER
extern int usb_recover_proc(void);
	g_board_extend.func_usb_update = usb_recover_proc;	//register function pointer
#endif

	//ts_mode_config(1, 1, 2, 0, 1, 0, -1);	//set ts1 to serial ts, and mode is 2, no valid, sync, little endian

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

