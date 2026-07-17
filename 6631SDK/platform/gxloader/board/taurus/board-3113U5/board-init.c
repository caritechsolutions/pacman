
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
	/* PORT00 ~ PORT05 默认为普通GPIO功能,无需管脚复用 */
        {_BOTTOM_,   47,     4,           MP_INV_V,    MP_INV_V,  0},           //NC          |SDA2/PORT06
        {_BOTTOM_,   48,     5,           MP_INV_V,    MP_INV_V,  0},           //NC          |SCL2/PORT07
        {_BOTTOM_,   49,     6,           MP_INV_V,    MP_INV_V,  0},           //NC          |UART2TX/PORT08
        {_BOTTOM_,   50,     7,           MP_INV_V,    MP_INV_V,  0},           //NC          |UART2RX/PORT09
	/* PORT10 ~ PORT14 默认为普通GPIO功能,无需管脚复用 */
        {_BOTTOM_,   52,     8,           64,          MP_INV_V,  0},           //NC          |IR/PORT15(PMUPORT00)/TSOUT7
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {_BOTTOM_,   53,     9,           65,          MP_INV_V,  1},           //NC          |DBGTDI/PORT16(PMUPORT01)/TSOUT8/SDBGTDI
        {_BOTTOM_,   54,     10,          66,          MP_INV_V,  1},           //NC          |DBGTDO/PORT17(PMUPORT02)/TSOUT9/SDBGTDO
        {_BOTTOM_,   55,     11,          67,          MP_INV_V,  1},           //NC          |DBGTMS/PORT18(PMUPORT03)/TSOUT10/SDBGTMS
        {_BOTTOM_,   56,     12,          68,          MP_INV_V,  0},           //NC          |DBGTCK/PORT19(PMUPORT04)/SDBGTCK
        {_BOTTOM_,   57,     13,          69,          MP_INV_V,  3},           //NC          |DBGTRST/PORT20(PMUPORT05)/SDBGTRST/CEC
#endif
        {_RIGHT_,    5,      14,          70,          128,       0},           //NC          |SC1CLK/PORT21/TSIDATA7/SDBGTDI/DEVADDR0
        {_RIGHT_,    6,      15,          71,          129,       0},           //NC          |SC1RST/PORT22/TSIDATA6/SDBGTDO
        {_RIGHT_,    7,      16,          72,          130,       0},           //NC          |SC1PWR/PORT23/TSIDATA5/SDBGTMS
        {_RIGHT_,    8,      17,          73,          131,       0},           //NC          |SC1CD/PORT24/TSIDATA4/SDBGTCK
        {_RIGHT_,    9,      18,          74,          132,       0},           //NC          |SC1DATA/PORT25/TSIDATA3/SDBGTRST
        {_RIGHT_,    13,     19,          75,          MP_INV_V,  1},           //NC          |NULL/PORT26/TSIDATA2/DEVADDR1
        {_RIGHT_,    14,     20,          76,          MP_INV_V,  0},           //NC          |HVSEL/PORT27/TSIDATA1
        {_RIGHT_,    15,     21,          77,          MP_INV_V,  0},           //NC          |DiSEqCo/PORT28/TSIDATA0
        {_RIGHT_,    16,     22,          78,          MP_INV_V,  0},           //NC          |AGC/PORT29/TSICLK
        {_RIGHT_,    17,     23,          MP_INV_V,    MP_INV_V,  0},           //NC          |PORT30/TSISYNC
        {_RIGHT_,    18,     24,          MP_INV_V,    MP_INV_V,  0},           //NC          |PORT31/TSIVALID
        {_RIGHT_,    25,     25,          81,          MP_INV_V,  0},           //NC          |TSODATA7/PORT32/TSI2DATA7
        {_RIGHT_,    26,     26,          82,          MP_INV_V,  0},           //NC          |TSODATA6/PORT33/TSI2DATA6
        {_RIGHT_,    27,     27,          83,          MP_INV_V,  0},           //NC          |TSODATA5/PORT34/TSI2DATA5
        {_RIGHT_,    28,     28,          84,          MP_INV_V,  0},           //NC          |TSODATA4/PORT35/TSI2DATA4
        {_RIGHT_,    29,     29,          85,          MP_INV_V,  0},           //NC          |TSODATA3/PORT36/TSI2DATA3
        {_RIGHT_,    36,     30,          86,          MP_INV_V,  0},           //NC          |TSODATA2/PORT37/TSI2DATA2
        {_RIGHT_,    37,     31,          87,          MP_INV_V,  0},           //NC          |TSODATA1/PORT38/TSI2DATA1
        {_RIGHT_,    38,     32,          88,          MP_INV_V,  0},           //NC          |TSODATA0/PORT39/TSI2DATA0
        {_RIGHT_,    39,     33,          89,          MP_INV_V,  0},           //NC          |TSOCLK/PORT40/TSI2CLK
        {_RIGHT_,    40,     34,          90,          MP_INV_V,  0},           //NC          |TSOSYNC/PORT41/TSI2SYNC
        {_RIGHT_,    41,     35,          91,          MP_INV_V,  0},           //NC          |TSOVALID/PORT42/TSI2VALID
        {_RIGHT_,    42,     36,          92,          MP_INV_V,  1},           //NC          |HVSEL/PORT43
        {_RIGHT_,    43,     37,          MP_INV_V,    MP_INV_V,  1},           //NC          |DiSEqCo/PORT44
        {_RIGHT_,    44,     38,          MP_INV_V,    MP_INV_V,  1},           //NC          |AGC/PORT45
        {_RIGHT_,    19,     39,          93,          MP_INV_V,  0},           //NC          |SDA1/PORT46/SDA_T
        {_RIGHT_,    20,     40,          94,          MP_INV_V,  0},           //NC          |SCL1/PORT47/SCL_T
        {_LEFT_,     11,     41,          95,          MP_INV_V,  0},           //NC          |DDCSDA/PORT48/SDAS
        {_LEFT_,     12,     42,          96,          MP_INV_V,  0},           //NC          |DDCSCL/PORT49/SCLS
        {_LEFT_,     13,     43,          97,          MP_INV_V,  0},           //NC          |SPDIF/PORT50/TSOUT0
        {_LEFT_,     127,    44,          MP_INV_V,    MP_INV_V,  0},           //NC          |RMCRSDV/PORT51
        {_LEFT_,     128,    45,          MP_INV_V,    MP_INV_V,  0},           //NC          |MD/PORT52
        {_LEFT_,     129,    46,          MP_INV_V,    MP_INV_V,  0},           //NC          |MDC/PORT53
        {_LEFT_,     130,    47,          MP_INV_V,    MP_INV_V,  0},           //NC          |RMTXEN/PORT54
        {_LEFT_,     131,    48,          MP_INV_V,    MP_INV_V,  0},           //NC          |RMTXD1/PORT55
        {_LEFT_,     132,    49,          MP_INV_V,    MP_INV_V,  0},           //NC          |RMTXD0/PORT56
        {_LEFT_,     133,    50,          MP_INV_V,    MP_INV_V,  0},           //NC          |RMCLK/PORT57
        {_LEFT_,     134,    51,          MP_INV_V,    MP_INV_V,  0},           //NC          |RMRXD1/PORT58
        {_LEFT_,     135,    52,          MP_INV_V,    MP_INV_V,  0},           //NC          |RMRXD0/PORT59
	/* PORT60 ~ PORT64 默认为普通GPIO功能,无需管脚复用 */
        {_BOTTOM_,   5,      53,          98,          135,       0},           //NC          |UART1TX/PORT65/AUARTTX/SURARTTX/TSOUT1
        {_BOTTOM_,   6,      54,          99,          MP_INV_V,  0},           //NC          |UART1RX/PORT66/TSOUT2
        {_BOTTOM_,   7,      0,           MP_INV_V,    MP_INV_V,  0},           //NC          |SPISCK/TSOUT3
        {_BOTTOM_,   8,      1,           MP_INV_V,    MP_INV_V,  0},           //NC          |SPIMOSI/TSOUT4
        {_BOTTOM_,   9,      2,           MP_INV_V,    MP_INV_V,  0},           //NC          |SPICSn/TSOUT5
        {_BOTTOM_,   10,     3,           MP_INV_V,    MP_INV_V,  0},           //NC          |SPIMISO/TSOUT6
	/* PORT67 ~ PORT80 默认为普通GPIO功能,无需管脚复用 */
        {ARRAY_END_FLAG, ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, NOT_CONFIG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
    {3,   26, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//Mute
	{11,  16, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//POWER_CONTRL  0:off  1:on
	{0,   17, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW},//PANEL_SDA
	{1,   18, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW},//PANEL_CLK
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
	loader_panel_init(0, 1);
	loader_panel_display();
	return 0;
}

int panel_update_call(void)
{
	return -1;
}

