
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
#define DDR_OUTPUT_DRIVER 0x4
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0) |= (DDR_OUTPUT_DRIVER<<3) | (DDR_OUTPUT_DRIVER<<6) | (DDR_OUTPUT_DRIVER<<24);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1) |= (DDR_OUTPUT_DRIVER<<0) | (DDR_OUTPUT_DRIVER<<11);
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2) |= (2<<0) | (2<<11);
}

/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
                                            /* chip_package: generic */
 /*      size | chip_core_num | 0_bit | 1_bit | 2_bit | func_sel */ /* package_num | func_list */
        {_BOTTOM_,   105,    32,    MP_INV_V,    MP_INV_V,  1},           //NC          |PORT00(PMUPORT00)/IR
#ifndef CONFIG_DBG_PIN_NO_MULTI
        {_RIGHT_,    1,      33,    MP_INV_V,    MP_INV_V,  1},           //NC          |DBGTDI/PORT01(PMUPORT01)
        {_RIGHT_,    2,      34,    123,         MP_INV_V,  0},           //NC          |DBGDTO/PORT02(PMUPORT02)/CEC
        {_RIGHT_,    3,      35,    96,          MP_INV_V,  1},           //NC          |DBGTMS/PORT03(PMUPORT03)/NFRDY0
        {_RIGHT_,    4,      36,    96,          MP_INV_V,  0},           //NC          |DBGTCK/PORT04(PMUPORT04)/NFOE
        {_RIGHT_,    5,      37,    96,          MP_INV_V,  1},           //NC          |DBGTRST/PORT05(PMUPORT05)/NFCLE
#endif
        {_RIGHT_,    7,      38,    97,          MP_INV_V,  0},           //NC          |PORT06(PMUPORT06)/SPISCK/NFALE/SPISCK_DW
        {_RIGHT_,    8,      39,    98,          MP_INV_V,  0},           //NC          |PORT07(PMUPORT07)/SPIMOSI/NFWE/SPIMOSI_DW
        {_RIGHT_,    9,      87,    99,          MP_INV_V,  0},           //NC          |SPICSn/NFDATA7/SPICSn_DW
        {_RIGHT_,    10,     40,    100,         MP_INV_V,  0},           //NC          |PORT08(PMUPORTO8)/SPIMISO/NFDATA6/SPIMISO_DW
        {_RIGHT_,    11,     41,    122,         MP_INV_V,  0},           //NC          |PORT09/TSI1VALID/TSOVALID
        {_RIGHT_,    13,     42,    101,         MP_INV_V,  0},           //NC          |PORT10(PMUPORT09)/SC1CLK/TSI1DATA0/TSODATA0
        {_RIGHT_,    14,     43,    102,         MP_INV_V,  0},           //NC          |PORT11(PMUPORT10)/SC1RST/TSI1DATA1/TSODATA1
        {_RIGHT_,    15,     44,    103,         MP_INV_V,  0},           //NC          |PORT12(PMUPORT11)/SC1PWR/TSI1DATA2/TSODATA2
        {_RIGHT_,    16,     45,    104,         MP_INV_V,  0},           //NC          |PORT13(PMUPORT12)/SC1CD/TSI1DATA3/TSODATA3
        {_RIGHT_,    17,     46,    105,         MP_INV_V,  0},           //NC          |PORT14(PMUPORT13)/SC1DATA/TSI1DATA4/TSODATA4
        {_RIGHT_,    19,     47,    106,         133,       0},           //NC          |PORT15(PMUPORT14)/UART2TX/TSI1DATA5/TSODATA5/NFDATA5
        {_RIGHT_,    20,     48,    107,         134,       0},           //NC          |PORT16(PMUPORT15)/UART2RX/TSI1DATA6/TSODATA6/NFDAT4
        {_RIGHT_,    21,     49,    108,         135,       0},           //NC          |PORT17(PMUPORT16)/SDA2/TSI1DATA7/TSODATA7/NFDAT3
        {_RIGHT_,    22,     50,    109,         136,       0},           //NC          |PORT18(PMUPORT17)/SCL2/TSI1CLK/TSOCLK/NFDAT2
        {_RIGHT_,    23,     51,    110,         MP_INV_V,  0},           //NC          |PORT19/TSI1SYNC/TSOSYNC
        {_RIGHT_,    38,     52,    120,         MP_INV_V,  0},           //NC          |PORT20/TSI2VALID/TSOVALID
        {_RIGHT_,    39,     53,    120,         MP_INV_V,  0},           //NC          |PORT21/TSI2DATA0/TSODATA0
        {_RIGHT_,    40,     54,    120,         MP_INV_V,  0},           //NC          |PORT22/TSI2DATA1/TSODATA1
        {_RIGHT_,    41,     55,    120,         MP_INV_V,  0},           //NC          |PORT23/TSI2DATA2/TSODATA2
        {_RIGHT_,    42,     56,    120,         MP_INV_V,  0},           //NC          |PORT24/TSI2DATA3/TSODATA3
        {_RIGHT_,    44,     57,    120,         MP_INV_V,  0},           //NC          |PORT25/TSI2DATA4/TSODATA4
        {_RIGHT_,    45,     58,    120,         MP_INV_V,  0},           //NC          |PORT26/TSI2DATA5/TSODATA5
        {_RIGHT_,    46,     59,    120,         MP_INV_V,  0},           //NC          |PORT27/TSI2DATA6/TSODATA6
        {_RIGHT_,    47,     60,    120,         MP_INV_V,  0},           //NC          |PORT28/TSI2DATA7/TSODATA7
        {_RIGHT_,    48,     61,    120,         MP_INV_V,  0},           //NC          |PORT29/TSI2SCLK/TSOCLK
        {_RIGHT_,    50,     62,    120,         MP_INV_V,  0},           //NC          |PORT30/TSI2SYNC/TSOSYNC
        {_RIGHT_,    51,     63,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT31(PMUPORT18)/SDA2
        {_RIGHT_,    52,     64,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT32(PMUPORT19)/SCL2
        {_RIGHT_,    53,     65,    111,         MP_INV_V,  0},           //NC          |PORT33(PMUPORT20)/UART2TX/AUARTTX
        {_RIGHT_,    54,     66,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT34(PMUPORT21)/UART2RX
        {_RIGHT_,    56,     67,    113,         132,       1},           //NC          |PORT35(PMUPORT22)/HVSEL/SDA3/NFDAT1/DiSEqCi
        {_RIGHT_,    57,     68,    114,         MP_INV_V,  1},           //NC          |PORT36(PMUPORT23)/DiSEqCo/SCL3/NFDAT0
        {_RIGHT_,    58,     69,    115,         MP_INV_V,  1},           //NC          |PORT37(PMUPORT24)/AGC1/AGC2
        {_RIGHT_,    59,     70,    116,         MP_INV_V,  0},           //NC          |UART1TX/PORT38(PMUPORT25)/AUARTTX
        {_RIGHT_,    60,     71,    MP_INV_V,    MP_INV_V,  0},           //NC          |UART1RX/PORT39(PMUPORT26)
        {_RIGHT_,    62,     72,    119,         MP_INV_V,  1},           //NC          |PORT40(PMUPORT27)/SDA1/SDA_T
        {_RIGHT_,    63,     73,    119,         MP_INV_V,  1},           //NC          |PORT41(PMUPORT28)/SCL1/SCL_T
        {_RIGHT_,    64,     80,    MP_INV_V,    MP_INV_V,  1},           //NC          |PORT48(PMUPORT29)/CEC
        {_TOP_,      82,     74,    117,         MP_INV_V,  1},           //NC          |PORT42/DDCSDA/SDA2
        {_TOP_,      83,     75,    118,         MP_INV_V,  1},           //NC          |PORT43/DDCSCL/SCL2
        {_TOP_,      84,     76,    MP_INV_V,    MP_INV_V,  1},           //NC          |PORT44/SPDIF
        {_TOP_,      85,     77,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT45/SPD_LED
        {_TOP_,      86,     78,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT46/LINK_LED
        {_TOP_,      87,     79,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT47/ACT_LED
        {_LEFT_,     28,     85,    MP_INV_V,    MP_INV_V,  1},           //NC          |PORT64(GBPORT00)/SC1CLK
         // 1:此处为空是由于PORT64 ~ PORT68为同一组，同时开启             //NC          |PORT65(GBPORT01)/SC1RST
         //   同时关闭，即要么全部是PORT64 ~ PORT68，要么是               //NC          |PORT66(GBPORT02)/SC1PWR
         //   SC1CLK ~ SC1DATA,如下同理                                   //NC          |PORT67(GBPORT03)/SC1CD
         // 2:PORT69(GBPORT05)默认就是这个功能，无需进行管脚复用          //NC          |PORT68(GBPORT04)/SC1DATA
        {_LEFT_,     35,     86,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT70(GBPORT06)/NFRDY0
                                                                          //NC          |PORT71(GBPORT07)/NFOE
                                                                          //NC          |PORT72(GBPORT08)/NFCLE
                                                                          //NC          |PORT73(GBPORT09)/NFALE
                                                                          //NC          |PORT74(GBPORT10)/NFWE
        {_LEFT_,     41,     88,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT75(GBPORT11)/NFDATA7
                                                                          //NC          |PORT76(GBPORT12)/NFDATA6
                                                                          //NC          |PORT77(GBPORT13)/NFDATA5
                                                                          //NC          |PORT78(GBPORT14)/NFDATA4
                                                                          //NC          |PORT79(GBPORT15)/NFDATA3
                                                                          //NC          |PORT80(GBPORT16)/NFDATA2
                                                                          //NC          |PORT81(GBPORT17)/NFDATA1
                                                                          //NC          |PORT82(GBPORT18)/NFDATA0
        {_LEFT_,     58,     81,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT83(GBPORT19)/TSI3VALID
        {_LEFT_,     59,     82,    121,         MP_INV_V,  0},           //NC          |PORT84(GBPORT20)/TSI3DATA0/TSISVALID
                                                                          //NC          |PORT85(GBPORT21)/TSI3DATA1/TSISDATA
                                                                          //NC          |PORT86(GBPORT22)/TSI3DATA2/TSISCLK
                                                                          //NC          |PORT87(GBPORT23)/TSI3DATA3/TSISSYNC
        {_LEFT_,     64,     83,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT88(GBPORT24)/TSI3DATA4
                                                                          //NC          |PORT89(GBPORT25)/TSI3DATA5
        {_LEFT_,     66,     84,    MP_INV_V,    MP_INV_V,  0},           //NC          |PORT90(GBPORT26)/TSI3DATA6
                                                                          //NC          |PORT91(GBPORT27)/TSI3DATA7
                                                                          //NC          |PORT92(GBPORT28)/TSI3CLK
                                                                          //NC          |PORT93(GBPORT29)/TSI3SYNC
        {ARRAY_END_FLAG, ARRAY_END_FLAG_U16, ARRAY_END_FLAG, ARRAY_END_FLAG, ARRAY_END_FLAG, NOT_CONFIG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
/*	{9 ,  1, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{10,  0, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{11,  3, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },
	{12, 12, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_LOW },
	{DWSPI_CS0_VIRTUAL_GPIO, 64, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},*/
	{3,  1, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH}, //CPU_TDI/MUTE_CTRL 静音输出
	{5,  5, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH}, //CPU_TRST/LNB_ON_OFF
	{0, 31, GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT, GX_GPIO_LOW}, //PANEL_DATA
	{1, 32, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH}, //PANEL_CLK
	{11,  3, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//LED for power and standby, 1: power led display, 0:standby led display
	{16,  34, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//remote led (flicker with key), 0:light up, 1:light off
	{13,  33, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//power cont

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
extern void loader_panel_display(void);
extern void loader_panel_init(unsigned int sda, unsigned int clk);
    loader_panel_init(0,1);
    loader_panel_display();
	return -1;
}

int panel_update_call(void)
{
	return -1;
}

