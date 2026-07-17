
/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology, Inc. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/*
 * hardware version: dvbc-3201_DEMO_V1.0  2013.10.30
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

/* denali phase adjust */
unsigned char DENALI_PHASE_ADJUST[6] __attribute__((section(".sram"))) = {
	0x6d,
	0x13,
	0x6d,
	0x13,
	0x61,
	0x61,
};

void __attribute__((section(".reset_patch"))) ddr_config_patch(void)
{
#define CLK_DATA_DRIVER 14
#define ADDR_CTL_DRIVER 14

#ifdef CONFIG_DDR3
	//DDR3
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_DRAM_CONFIG0) = 2|(1<<3)|(1<<12)|(1<<13)|(1<<14)|(CLK_DATA_DRIVER<<4)|(CLK_DATA_DRIVER<<8)|(ADDR_CTL_DRIVER<<16)|(ADDR_CTL_DRIVER<<20)|(3<<24)|(3<<28);
	*(volatile unsigned int*)(CONFIG_BASE + CONFIG_DRAM_CONFIG1) = 1|(4<<4)|(4<<8)|(4<<12)|(4<<16)|(1<<22);
#elif defined(CONFIG_DDR2)
	//DDR2
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0)   = 7|(1<<3)|(1<<12)|(CLK_DATA_DRIVER<<4)|(CLK_DATA_DRIVER<<8)|(ADDR_CTL_DRIVER<<16)|(ADDR_CTL_DRIVER<<20)|(1<<24)|(1<<28);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1)   = 3|(6<<4)|(6<<8)|(6<<12)|(6<<16)|(1<<20)|(0<<22);
#else
#error "you must define CONFIG_DDR3 or CONFIG_DDR2"
#endif

}

void __attribute__((section(".reset_patch"))) sdram_reset_patch_s(void)
{
#if 1
        //GPIO 45 to control ddr reset
     	*(volatile unsigned int*)0x0030a144 |= (1 << (45-32));
	*(volatile unsigned int*)0x0030a148 &= ~(1 << 17);
	*(volatile unsigned int*)0x00306000 |= (1 << (45-32));
	*(volatile unsigned int*)0x0030600c |= (1 << (45-32));
#endif
	return;
}

void __attribute__((section(".reset_patch"))) sdram_reset_patch_e(void)
{
#if 1
	//GPIO 45
	*(volatile unsigned int*)0x00306008 |= (1 << (45-32));
#endif
	return;
}

/* NRE PING MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
        {_BOTTOM_,  74,  3, MULPIN_INVALID_VALUE, 1},           //SC1CLK_PORT03_SCICD
        {_BOTTOM_,  75,  4, MULPIN_INVALID_VALUE, 1},           //SC1RST_PORT04_SC2CLK
        {_BOTTOM_,  76,  5, MULPIN_INVALID_VALUE, 1},           //SC1PWR_PORT05_SCIC4
        {_BOTTOM_,  77,  6, MULPIN_INVALID_VALUE, 1},           //SC1CD_PORT06_SCI3V5V
        {_BOTTOM_,  78,  7, MULPIN_INVALID_VALUE, 1},           //SC1DATA_PORT07_SCICMDVCC
        {_BOTTOM_, 101,  8, 77, 1},                             //SPIHOLD_PORT08_NFRDY1
        {_BOTTOM_, 102,  MULPIN_INVALID_VALUE, 77, 0},          //SPISCK_NULL_NULL_NFRE
        {_BOTTOM_, 104,  MULPIN_INVALID_VALUE, 77, 0},          //SPIMOSI_NULL_NULL_NFCLE

        { _RIGHT_,   1,  MULPIN_INVALID_VALUE, 77, 0},          //SPICSn_NULL_NULL_NFALE
        { _RIGHT_,   3,  MULPIN_INVALID_VALUE, 77, 0},          //SPIMISO_NULL_NULL_NFWE
        { _RIGHT_,   5,  9, 77, 1},                             //SPIWP_PORT09_NFCS0n
#ifndef CONFIG_DBG_PIN_NO_MULTI
        { _RIGHT_,   7, 10, MULPIN_INVALID_VALUE, 0},           //DBGTDI_PORT10
        { _RIGHT_,   8, 11, MULPIN_INVALID_VALUE, 0},           //DBGTDO_PORT11
        { _RIGHT_,  10, 12, MULPIN_INVALID_VALUE, 0},           //DBGTMS_PORT12
        { _RIGHT_,  11, 13, MULPIN_INVALID_VALUE, 0},           //DBGTCK_PORT13
        { _RIGHT_,  12, 14, MULPIN_INVALID_VALUE, 0},           //DBGTRST_PORT14
#endif
        { _RIGHT_,  21, 63, MULPIN_INVALID_VALUE, 1},           //GBCSn1_PORT63
        { _RIGHT_,  22, 19, 64, 1},                             //RMCRSDV_PORT19_AJTDI_GBCSn0
        { _RIGHT_,  23, 20, 64, 1},                             //MD_PORT20_AJTDO_GBDATA2
        { _RIGHT_,  24, 21, 64, 1},                             //MDC_PORT21_AJTMS_GBDATA1
        { _RIGHT_,  25, 22, 64, 1},                             //RMTXEN_PORT22_AJTCK_GBDATA0
        { _RIGHT_,  26, 23, 64, 1},                             //RMTXD1_PORT23_AJTRST_GBDATA7
        { _RIGHT_,  27, 24, 65, 1},                             //RMTXD0_PORT24_AOAMCLK_GBDATA6
        { _RIGHT_,  29, 25, 66, 1},                             //RMCLK_PORT25_AUARTTX_GBDATA5
        { _RIGHT_,  30, 26, 75, 1},                             //RMRXD1_PORT26_GBDATA3
        { _RIGHT_,  31, 27, 75, 1},                             //RMRXD0_PORT27_GBDATA4
        { _RIGHT_,  39, 28, 76, 1},                             //TSI2DATA5_PORT28_TSODATA5
        { _RIGHT_,  40, 29, 76, 1},                             //TSI2DATA6_PORT29_TSODATA6
        { _RIGHT_,  41, 30, 76, 1},                             //TSI2DATA7_PORT30_TSODATA7
        { _RIGHT_,  42, 31, 76, 1},                             //TSI2CLK_PORT31_TSOCLK
        { _RIGHT_,  44, 32, 76, 1},                             //TSI2SYNC_PORT32_TSOSYNC
        { _RIGHT_,  45, 33, MULPIN_INVALID_VALUE, 1},           //TSI1VALID_PORT33
        { _RIGHT_,  46, 34, 73, 1},                             //TSI1DATA0_PORT34_TSI2DATA5
        { _RIGHT_,  47, 35, 73, 1},                             //TSI1DATA1_PORT35_TSI2DATA6
        { _RIGHT_,  48, 36, 73, 1},                             //TSI1DATA2_PORT36_TSI2DATA7
        { _RIGHT_,  49, 37, 73, 1},                             //TSI1DATA3_PORT37_TSI2CLK
        { _RIGHT_,  62, 38, 68, 2},                             //TSI1DATA4_PORT38_SC1CLK_NFDAT7
        { _RIGHT_,  63, 39, 68, 2},                             //TSI1DATA5_PORT39_SC1RST_NFDAT6
        { _RIGHT_,  64, 40, 68, 2},                             //TSI1DATA6_PORT40_SC1PWR_NFDAT5
        { _RIGHT_,  65, 41, 68, 2},                             //TSI1DATA7_PORT41_SC1CD_NFDAT4
        { _RIGHT_,  66, 42, 68, 2},                             //TSI1CLK_PORT42_SC1DATA_NFDATA3
        { _RIGHT_,  67, 43, MULPIN_INVALID_VALUE, 1},           //TSI1SYNC_PORT43
        { _RIGHT_,  68, 15, 69, 1},                             //UART2TX_PORT15_SDA2
        { _RIGHT_,  69, 16, 69, 1},                             //UART2RX_PORT16_SCL2
        { _RIGHT_,  70, 17, 70, 0},                             //UART1TX_PORT17_AUARTTX
        { _RIGHT_,  71, 18, 70, 0},                             //UART1RX_PORT18
        { _RIGHT_,  72, 44, 80, 1},                             //HVSEL_PORT44_NFRDY0
        { _RIGHT_,  73, 45, 81, 1},                             //DiSEqCo_PORT45_NFCS1n
        { _RIGHT_,  74, 78, 71, 0},                             //SDA1_NULL_SDAD_NFDATA2
        { _RIGHT_,  75, 78, 71, 0},                             //SCL1_NULL_SCLD_NFDATA1
        { _RIGHT_,  76, 46, 79, 0},                             //AGC_PORT46_NFDATA0

        {   _TOP_,  80, 47, MULPIN_INVALID_VALUE, 0},           //HDMISEL_PORT47
        {   _TOP_,  81, 48, MULPIN_INVALID_VALUE, 0},           //DDCSDA_PORT48
        {   _TOP_,  82, 49, MULPIN_INVALID_VALUE, 0},           //DDCSCL_PORT49
        {   _TOP_,  83, 50, MULPIN_INVALID_VALUE, 1},           //SPDIF_PORT50
        {   _TOP_,  84, 51, 72, 2},                             //NFCS3n_PORT51_RMCRSDV_HDMICEC
        {   _TOP_,  85, 52, 72, 2},                             //NFCS2n_PORT52_MD_NFRDY1
        {   _TOP_,  86, 53, 72, 2},                             //NFCS0n_PORT53_MDC

        {  _LEFT_,   1, 54, 72, 2},                             //NFCS1n_PORT54_RMTXEN
        {  _LEFT_,   2, 55, 72, 2},                             //NFWP_SC1CLK_RMTXD1
        {  _LEFT_,   3, 75, 72, 2},                             //NFDAT0_SC1RST_RMTXD0_GBADDR10
        {  _LEFT_,   4, 75, 72, 2},                             //NFDAT1_SC1PWR_RMCLK_GBADDR11
        {  _LEFT_,   5, 75, 72, 2},                             //NFDAT2_SC1CD_RMRXD1_GBADDR09
        {  _LEFT_,   6, 75, 72, 2},                             //NFDAT3_SC1DATA_RMRXD0_GBADDR08
        {  _LEFT_,  12, 75, MULPIN_INVALID_VALUE, 0},           //NFDAT4_GBADDR12
        {  _LEFT_,  13, 75, MULPIN_INVALID_VALUE, 0},           //NFDAT5_GBADDR07
        {  _LEFT_,  14, 75, 74, 0},                             //NFDAT6_AOPCMSD3_GBADDR06
        {  _LEFT_,  15, 75, 74, 0},                             //NFDAT7_AOPCMSD2_GBADDR05
        {  _LEFT_,  16, 75, 74, 0},                             //NFWE_AOPCMSD1_GBADDR04
        {  _LEFT_,  18, 75, 74, 0},                             //NFALE_AOPCMSD0_GBADDR03
        {  _LEFT_,  19, 75, 74, 0},                             //NFCLE_AOAMCLK_GBADDR02
        {  _LEFT_,  20, 75, 74, 0},                             //NFOE_AOPCMCLK_GBADDR01
        {  _LEFT_,  21, 75, 74, 0},                             //NFRDY0_AOPCMWS_GBADDR00

	{ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
	{34,34,GX_GPIO_CONFIG_VALID,GX_GPIO_OUTPUT,GX_GPIO_LOW},
	{50,50,GX_GPIO_CONFIG_VALID,GX_GPIO_OUTPUT,GX_GPIO_LOW},
/*	{9 ,  1, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{10,  0, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{11,  3, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },
	{12, 12, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },*/
	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* Board specific initialization routine */
int board_init(void)
{
	enable_dac(ENABLE_CVBS);
	gpio_init();
	mulpin_init();
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
	return -1;
}

int panel_update_call(void)
{
	return -1;
}

