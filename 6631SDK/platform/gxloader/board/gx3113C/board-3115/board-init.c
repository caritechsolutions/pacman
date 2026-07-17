
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

extern unsigned int DENALI_CONFIG_PHY[26];
void __attribute__((section(".reset_patch"))) ddr_config_patch(void)
{
#define DDR_OUTPUT_DRIVER 0x3
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER << 21);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER << 24);
	DENALI_CONFIG_PHY[25] = 0x00010101 | (DDR_OUTPUT_DRIVER << 4) | (DDR_OUTPUT_DRIVER << 0xc) | (DDR_OUTPUT_DRIVER << 0x14);
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
                                                      /* chip_package: generic */
 /* chip_core_num | l_bit | h_bit | func_select */    /* package_num | func_list */
	{73 ,  0, MULPIN_INVALID_VALUE, 0},           // NC          | GBIOWRn_PORT00
	{74 ,  1, MULPIN_INVALID_VALUE, 0},           // NC          | GBIORDn_PORT01
	{75 ,  2, MULPIN_INVALID_VALUE, 0},           // NC          | GBOEn_PORT02
	{76 ,  3, MULPIN_INVALID_VALUE, 0},           // NC          | GBWEn_PORT03
	{77 ,  4, MULPIN_INVALID_VALUE, 0},           // NC          | GBCSn1_PORT04
	{78 ,  5, MULPIN_INVALID_VALUE, 0},           // NC          | GBCSn0_PORT05
	{79 ,  6, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA0_PORT06
	{80 ,  7, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA1_PORT07
	{81 ,  8, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA2_PORT08
	{82 ,  9, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA3_PORT09
	{83 , 10, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA4_PORT10
	{84 , 11, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA5_PORT11
	{85 , 12, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA6_PORT12
	{86 , 13, MULPIN_INVALID_VALUE, 0},           // NC          | GBDATA7_PORT13
	{101, 21, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR00_PORT21
	{102, 20, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR01_PORT20
	{103, 19, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR02_PORT19
	{104, 18, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR03_PORT18
	{105, 17, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR04_PORT17
	{106, 29, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR05_PORT29
	{107, 28, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR06_PORT28
	{108, 27, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR07_PORT27
	{109, 26, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR08_PORT26
	{110, 25, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR09_PORT25
	{111, 24, MULPIN_INVALID_VALUE, 0},           // NC          | GBADDR10_PORT24
	{112, 23, 80, 0},                             // 33          | AJTDI_SC1DET_GBADDR11_PORT23
	{113, 22, 81, 0},                             // 34          | AJTDO_SC1PWR_GBADDR12_PORT22
	{115, 16, 82, 0},                             // 35          | AJTMS_SC1DAT_GBADDR13_PORT16
	{116, 15, 83, 0},                             // 36          | AJTCK_SC1CLK_GBADDR14_PORT15
	{117, 14, 84, 0},                             // 37          | AJTRST_SC1RST_GBADDR15_PORT14
#ifndef CONFIG_DBG_PIN_NO_MULTI
	{118, 32, MULPIN_INVALID_VALUE, 0},           // 38          | DBGTDI_PORT32
	{119, 33, MULPIN_INVALID_VALUE, 0},           // 39          | DBGTDO_PORT33
	{120, 34, MULPIN_INVALID_VALUE, 0},           // 40          | DBGTMS_PORT34
	{121, 35, MULPIN_INVALID_VALUE, 0},           // 41          | DBGTCK_PORT35
	{122, 36, MULPIN_INVALID_VALUE, 0},           // 42          | DBGTRST_PORT36
#endif
	{124, 37, MULPIN_INVALID_VALUE, 0},           // NC          | TS1VALID_PORT37
	{125, 38, MULPIN_INVALID_VALUE, 0},           // 44          | TS1DAT0_PORT38
	{126, 39, 87, 2},                             // 45          | ScanPwdClk_PORT39_TS1DTA1
	{127, 40, 87, 2},                             // 46          | ScanPwdDat_PORT40_TS1DAT2
	{128, 41, 87, 2},                             // 47          | ScanPwdSel_PORT41_TS1DAT3
	{129, 42, 89, 2},                             // 48          | UARTTXICAM1_PORT42_TS1DAT4
	{130, 43, 89, 2},                             // 49          | UARTRXICAM1_PORT43_TS1DAT5
	{131, 44, 89, 2},                             // 50          | UARTTXICAM2_PORT44_TS1DAT6
	{132, 45, 89, 2},                             // 51          | UARTRXICAM2_PORT45_TS1DAT7
	{133, 46, 89, 2},                             // 52          | UARTTXICAM3_PORT46_TS1CLK
	{134, 47, 89, 2},                             // 53          | UARTRXICAM3_PORT47_TS1SYNC
	{141, 48, MULPIN_INVALID_VALUE, 0},           // NC          | TSIVALID_PORT48
	{142, 49, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT0_PORT49
	{143, 50, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT1_PORT50
	{144, 51, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT2_PORT51
	{145, 52, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT3_PORT52
	{146, 53, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT4_PORT53
	{147, 54, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT5_PORT54
	{148, 55, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT6_PORT55
	{149, 56, MULPIN_INVALID_VALUE, 0},           // NC          | TSIDAT7_PORT56
	{150, 57, MULPIN_INVALID_VALUE, 0},           // NC          | TSICLK_PORT57
	{151, 58, MULPIN_INVALID_VALUE, 0},           // NC          | TSISYNC_PORT58
	{152, 59, MULPIN_INVALID_VALUE, 0},           // NC          | IIC1SCLT_PORT59
	{153, 60, MULPIN_INVALID_VALUE, 0},           // NC          | IIC1SDAT_PORT60
	{156, 61, MULPIN_INVALID_VALUE, 0},           // 58          | AGC_PORT61
	{221, 62, 79, 0},                             // 89          | SPDIF_PORT62_OtpClockOsc
	{222, 63, 85, 0},                             // 90          | UARTTX1_PORT63_AUARTTX
	{223, 64, MULPIN_INVALID_VALUE, 0},           // 91          | UARTRX1_PORT64
	{224, 30, MULPIN_INVALID_VALUE, 0},           // NC          | SPIHOLD_PORT30
	{225, 31, MULPIN_INVALID_VALUE, 0},           // NC          | SPIWP_PORT31
	{230, 65, 86, 0},                             // NC          | UARTTXICAM_PORT65_UARTTX0_AUARTTX
	{231, 66, 88, 0},                             // NC          | UARTRXICAM_PORT66_UARTX0_AGC1
	{232, 70, MULPIN_INVALID_VALUE, 0},           // NC          | SCI1CMDVCC_PORT70
	{233, 74, MULPIN_INVALID_VALUE, 0},           // NC          | SCI13V5V_PORT74
	{234, 68, MULPIN_INVALID_VALUE, 0},           // NC          | SCI1DATC4_PORT68
	{235, 72, MULPIN_INVALID_VALUE, 0},           // NC          | SCI1CLK_PORT72
	{236, 69, MULPIN_INVALID_VALUE, 0},           // NC          | SCI1DET_PORT69
	{237, 73, MULPIN_INVALID_VALUE, 0},           // NC          | SCI1RST_PORT73
	{238, 67, MULPIN_INVALID_VALUE, 0},           // NC          | SCI1DATC8_PORT67
	{239, 71, MULPIN_INVALID_VALUE, 0},           // NC          | SCI1DATC7_PORT71

	{ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
/*	{9 ,  1, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{10,  0, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{11,  3, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },
	{12, 12, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },*/

	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* Board specific initialization routine */
int board_init(void)
{
	gpio_init();
	mulpin_init();

	//cmdline_add("av_extend:0xa0305000=0x12345678,0xa0306000=0x87654321");
	//cmdline_add("ex_extend:ex...");

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

