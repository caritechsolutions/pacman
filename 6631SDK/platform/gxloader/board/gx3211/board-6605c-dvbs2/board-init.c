
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
#define DDR_OUTPUT_DRIVER 0x6

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<24);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
                                            /* chip_package: generic */
 /* chip_core_num | 0_bit | 1_bit |   2_bit  |func_sel*/ /* package_num | func_list */
        {82,        70,     102,    MP_INV_V,  0},         // NC          | SC2CMDVCC_PORT38
        {83,        71,     103,    MP_INV_V,  0},         // NC          | SC23V5V_PORT39
        {84,        72,     104,    MP_INV_V,  0},         // NC          | SC2DATC4_PORT40
        {85,        73,     105,    MP_INV_V,  0},         // NC          | SC2CLK_PORT41
        {86,        74,     106,    MP_INV_V,  0},         // NC          | SC2DET_PORT42
        {88,        75,     107,    MP_INV_V,  0},         // NC          | SC2RST_PORT43
        {90,        76,     108,    MP_INV_V,  0},         // NC          | SC2DATC8_PORT44
        {92,        77,     109,    MP_INV_V,  0},         // NC          | SC2DATC7_PORT45
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {104,       0,      32,     MP_INV_V,  0},         // 29          | DBGTDI_PORT0
        {105,       1,      33,     MP_INV_V,  0},         // 30          | DBGTDO_PORT1
        {106,       2,      34,     MP_INV_V,  0},         // 31          | DBGTMS_PORT2
        {107,       3,      35,     MP_INV_V,  0},         // 32          | DBGTCK_PORT3
        {108,       4,      36,     MP_INV_V,  0},         // 33          | DBGTRST_PORT4
#endif
        {110,       128,    132,    MP_INV_V,  0},         // 34          | SPISCK_NFRDY0_NFRDY0_SPISCKDW
        {111,       129,    133,    MP_INV_V,  0},         // 35          | SPIMOSI_NFOE_NFWE_SPIMOSIDW
        {112,       130,    134,    MP_INV_V,  0},         // 36          | SPICSn_NFCLE_NFALE_SPICSnDW
        {113,       131,    135,    MP_INV_V,  0},         // 37          | SPIMISO_NFALE_NFCLE_SPIMISODW
        {116,       78,     110,    MP_INV_V,  0},         // NC          | SC1CLK_PORT46_NFOE
        {118,       79,     111,    MP_INV_V,  0},         // NC          | SC1RST_PORT47_NFDAT7
        {120,       80,     112,    MP_INV_V,  0},         // NC          | SC1PWR_PORT48_NFDAT6
        {122,       81,     113,    MP_INV_V,  0},         // NC          | SC1CD_PORT49_NFDAT5
        {124,       82,     114,    MP_INV_V,  0},         // NC          | SC1DATA_PORT50_NFDAT4
        {126,       83,     115,    MP_INV_V,  0},         // NC          | TSI3DAT5_PORT51_NFDAT3
        {128,       84,     116,    MP_INV_V,  0},         // NC          | TSI3DAT6_PORT52_NFDAT2
        {130,       85,     117,    MP_INV_V,  0},         // NC          | TSI3DAT7_PORT53_NFDAT1
        {132,       86,     118,    MP_INV_V,  0},         // NC          | TSI3CLK_PORT54_NFDAT0
        {136,       5,      37,     MP_INV_V,  0},         // 39          | TSI1DATA0_PORT5_NFWE_BTDATA0
        {137,       6,      38,          136,  0},         // 40          | TSI1DATA1_PORT6_SC1CLK_BTDATA1_SCTDI
        {138,       7,      39,          136,  0},         // 41          | TSI1DATA2_PORT7_SC1RST_BTDATA2_SCTDO
        {139,       8,      40,          136,  0},         // 42          | TSI1DATA3_PORT8_SC1PWR_BTDATA3_SCTMS
        {140,       9,      41,          136,  0},         // 43          | TSI1DATA4_PORT9_SC1CD_BTDATA4_SCTCK
        {141,       10,     42,          136,  0},         // 44          | TSI1DATA5_PORT10_SC1DATA_BTDATA5_SCTRST
        {142,       11,     43,     MP_INV_V,  0},         // 45          | TSI1DATA6_PORT11_NFDAT7_BTDATA6
        {143,       12,     44,     MP_INV_V,  0},         // 46          | TSI1DATA7_PORT12_NFDAT6_BTDATA7
        {144,       13,     45,     MP_INV_V,  0},         // 47          | TSI1CLK_PORT13_NFDAT5_BTCLK
        {149,       87,     119,    MP_INV_V,  0},         // NC          | SCANPWDCLk_TSI2DATA0_PORT55
        {150,       88,     120,    MP_INV_V,  0},         // NC          | SCANPWDDAT_TSI2DATA1_PORT56
        {151,       89,     121,    MP_INV_V,  0},         // NC          | SCANPWDSET_TSI2DATA2_PORT57
        {152,       90,     122,    MP_INV_V,  0},         // NC          | UARTTXICAM1_TSI2DATA3_PORT58
        {153,       91,     123,    MP_INV_V,  0},         // NC          | UARTRXICAM1_TSI2DATA4_PORT59
        {154,       92,     124,    MP_INV_V,  0},         // NC          | UARTTXICAM2_TSI2DATA5_PORT60
        {155,       93,     125,    MP_INV_V,  0},         // NC          | UARTRXICAM2_TSI2DATA6_PORT61
        {156,       94,     126,    MP_INV_V,  0},         // NC          | UARTTXICAM3_TSI2DATA7_PORT62
        {158,       95,     127,    MP_INV_V,  0},         // NC          | UARTRXICAM3_TSI2SCLK_PORT63
        {161,       14,     46,     MP_INV_V,  0},         // 48          | SDA2_PORT14_AUARTTX_PCMBCK
        {162,       15,     47,     MP_INV_V,  0},         // 49          | SCL2_PORT15_NFDAT4_PCMDIN
        {163,       16,     48,          137,  0},         // 50          | UARTTXICAM_UART2TX_NFDAT3_PCMSCKO_SCUARTTX
        {164,       17,     49,          137,  0},         // 51          | UARTRXICAM_UART2RX_NFDAT2_PCMLRCK_SCUARTRX
        {165,       18,     50,     MP_INV_V,  0},         // 52          | HVSEL_PORT18_NFDAT1_DiSEqCi
        {166,       19,     51,     MP_INV_V,  0},         // 53          | DiSEqCo_PORT19_NFDAT0
        {168,       20,     52,     MP_INV_V,  0},         // 54          | AGC_PORT20
        {175,       21,     53,     MP_INV_V,  0},         // 58          | UART1TX_PORT21_AUARTTX
        {176,       22,     54,     MP_INV_V,  0},         // 59          | UART1RX_PORT22
        {177,       23,     55,     MP_INV_V,  0},         // 60          | SDA1_PORT23
        {178,       24,     56,     MP_INV_V,  0},         // 61          | SCL1_PORT24
        {272,       26,     58,     MP_INV_V,  0},         // 90          | DDCSDA_PORT26_SDA2_HDMIDTB0
        {273,       27,     59,     MP_INV_V,  0},         // 91          | DDCSCL_PORT27_SCL2_HDMIDTB1
        {274,       28,     60,     MP_INV_V,  0},         // 92          | SPDIF_PORT28
        {276,       29,     61,     MP_INV_V,  0},         // NC          | RMCRSDV_PORT29_SPDLED
        {277,       30,     62,     MP_INV_V,  2},         // 94          | MD_PORT30_LINKLED
        {278,       31,     63,     MP_INV_V,  2},         // 95          | MDC_PORT31_ACTLED
        {279,       64,     96,     MP_INV_V,  0},         // 96          | RMTXEN_PORT32_AJTDI_SC1CLK
        {281,       65,     97,     MP_INV_V,  0},         // 97          | RMTXD1_PORT33_AJTDO_SC1RST
        {282,       66,     98,     MP_INV_V,  0},         // 98          | RMTXD0_PORT34_AJTMS_SC1PWR
        {283,       67,     99,     MP_INV_V,  0},         // 99          | RMCLK_PORT35_AJTCK_SC1CD
        {285,       68,     100,    MP_INV_V,  0},         // 100         | RMRXD1_PORT36_AJRST_SC1DATA
        {287,       69,     101,    MP_INV_V,  0},         // 101         | RMRXD0_PORT37_AOAMCLK

	{ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
/*	{9 ,  1, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{10,  0, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{11,  3, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },
	{12, 12, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },
	{DWSPI_CS0_VIRTUAL_GPIO, 64, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},*/

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

