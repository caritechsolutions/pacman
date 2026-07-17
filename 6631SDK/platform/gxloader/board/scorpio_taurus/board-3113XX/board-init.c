
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
#include "chip_info.h"
#include "panel_fd650.h"
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#include "gxav_vout_propertytypes.h"
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

char logo_partition_name[20] = PARTITION_LOGO;
char logo_file_name[20] = "gxlogo.jpg";
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
enum cvbs_mode_enum g_cvbs_mode = GXAV_VOUT_PAL;
enum ypbpr_hdmi_mode_enum g_ypbpr_hdmi_mode = GXAV_VOUT_1080I_50HZ;
#else /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
enum cvbs_mode_enum g_cvbs_mode = G_PAL;
enum ypbpr_hdmi_mode_enum g_ypbpr_hdmi_mode = G_YPBPR_HDMI_1080I_50HZ;
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
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
struct taurus_mulpin_config_s taurus_mulpin_table[] = {
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

struct gpio_entry_bootloader g_taurus_gpio_table[] = {
/*	{9,   5,  GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{10,  10, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{11,  8,  GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT,  GX_GPIO_HIGH},
	{DWSPI_CS0_VIRTUAL_GPIO, 64, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},*/

	{0 ,  17, GX_GPIO_CONFIG_VALID, GX_GPIO_INPUT, GX_GPIO_LOW},
	{1 ,  18, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{3 ,  26, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},
	{11,  16, GX_GPIO_CONFIG_VALID, GX_GPIO_OUTPUT, GX_GPIO_HIGH},//POWER_CONTRL  0:off  1:on
	{ARRAY_END_FLAG, ARRAY_END_FLAG, 0, 0, 0}
};

/* NRE PIN MULTIPLEX TABLE*/
struct scorpio_mulpin_config_s scorpio_mulpin_table[] = {
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

struct gpio_entry_bootloader g_scorpio_gpio_table[] = {
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

	cmdline_clear();
	printf("chip name : %s", gx_get_chip_name());
	if ((strncmp((const char *)gx_get_chip_name(), "3113X1", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3113x1", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3377X1", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3377x1", 6) == 0)) {
		cmdline_init(CONFIG_CMDLINE_VALUE1);
	} else if ((strncmp((const char *)gx_get_chip_name(), "3113X5", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3113x5", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3377X5", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3377x5", 6) == 0)) {
		cmdline_init(CONFIG_CMDLINE_VALUE2);
	} else if ((strncmp((const char *)gx_get_chip_name(), "3113E1", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3113e1", 6) == 0)) {
		cmdline_init(CONFIG_CMDLINE_VALUE3);
	} else if ((strncmp((const char *)gx_get_chip_name(), "3113E5", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3113e5", 6) == 0)) {
		cmdline_init(CONFIG_CMDLINE_VALUE4);
	} else if ((strncmp((const char *)gx_get_chip_name(), "3113U1", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3113u1", 6) == 0)) {
		cmdline_init(CONFIG_CMDLINE_VALUE5);
	} else if ((strncmp((const char *)gx_get_chip_name(), "3113U5", 6) == 0) ||
	(strncmp((const char *)gx_get_chip_name(), "3113u5", 6) == 0)) {
		cmdline_init(CONFIG_CMDLINE_VALUE6);
	}
#ifdef ENABLE_USB_RECOVER
extern int usb_recover_proc(void);
	g_board_extend.func_usb_update = usb_recover_proc;	//register function pointer
#endif

	if (gxcore_chip_probe() == 0x3113) {
		value = *(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_TS_OUT_SEL);
		value &= ~(0x3<<4);
		value |= (0x1<<4);
		*(volatile unsigned int*)(CONFIG_BASE_MMU + CONFIG_TS_OUT_SEL) = value;
		sci_config(1, 1); // vccen_pol is high; detect_pol is high
	}
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

