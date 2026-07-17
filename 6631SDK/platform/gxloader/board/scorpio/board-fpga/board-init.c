
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
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG0)=0x868801b1;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG1)=0x00223446;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG2)=0x00003446;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_DRAM_CONFIG3)=0x00000001;
}


/* NRE PIN MULTIPLEX TABLE*/
struct mulpin_config_s mulpin_table[] = {
/*       pin_id  func_sel                     func_list */
        {0,       0},          //NC          |
        {1,       0},          //NC          |
        {2,       0},          //NC          |
        {3,       0},          //NC          |
        {4,       0},          //NC          |
        {5,       0},          //NC          |
        {6,       0},          //NC          |
        {7,       0},          //NC          |
        {8,       0},          //NC          |
        {9,       0},          //NC          |
        {10,      0},          //NC          |
        {11,      0},          //NC          |
        {12,      0},          //NC          |
        {13,      0},          //NC          |
        {14,      0},          //NC          |
        {15,      0},          //NC          |
        {16,      0},          //NC          |
        {17,      0},          //NC          |
        {18,      0},          //NC          |
        {19,      0},          //NC          |
        {20,      0},          //NC          |
        {21,      0},          //NC          |
        {22,      0},          //NC          |
        {23,      0},          //NC          |
        {24,      0},          //NC          |
        {25,      0},          //NC          |
        {26,      0},          //NC          |
        {ARRAY_END_FLAG_U16, NOT_CONFIG}
};

struct gpio_entry_bootloader g_gpio_table[] = {
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

	*(volatile unsigned int*)(CONFIG_BASE_MMU + 0x40c) = 0x43434343;
	*(volatile unsigned int*)(CONFIG_BASE_MMU + 0x410) = 0x43434343;
	*(volatile unsigned int*)(CONFIG_BASE_MMU + 0x414) = 0x40434343;
	value = *(volatile unsigned int*)(CONFIG_BASE_MMU + 0x704);
	value &= ~(0x3<<4);
	value |= (0x1<<4);
	*(volatile unsigned int*)(CONFIG_BASE_MMU + 0x704) = value;

	//ts_mode_config(1, 1, 2, 0, 1, 0, -1);	//set ts1 to serial ts, and mode is 2, no valid, sync, little endian
	ts_mode_config(0, 0, 2, 1, 1, 0, -1);	//set ts0 to serial ts, and mode is 2, valid, sync, little endian

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

