/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the GX3201 boot loader */

#include "stdio.h"
#include "string.h"
#include "crc.h"

#include "io.h"
#include "config.h"
#include "gxhwlib_registers.h"

#include "util.h"
#include "gx_api.h"
#include "cpu_config.h"
#include "chip_info.h"

#include "partition.h"
#include "board-common.h"

extern int board_init(void);
extern void gx_rtc_delay_us(volatile unsigned int delay);
extern int display_demo_warning(void);

#ifdef CONFIG_ENABLE_RELEASE_CHECK
#define RELEASE_INFO_MAGIC	0x12345678
#define UNRELEASE_INFO_MAGIC	0x00000000
#define RELEASE_INFO_STR_SIZE	(256 - 4 - 4)
struct release_info_s{
	unsigned int magic;
	unsigned char str[RELEASE_INFO_STR_SIZE];
	unsigned int crc32;
};
int gx_release_check(void)
{
	unsigned int i;
	struct partition_info *partition_boot;
	struct release_info_s *p_release_info;
	unsigned int calc_crc;
	unsigned char *p_printf_str;
	unsigned int printf_size = 0;
	unsigned char *p_tmp;


	partition_boot = all_partition_get(PARTITION_BOOT);
	p_release_info = malloc(sizeof(struct release_info_s));
	partition_read(partition_boot, partition_boot->total_size - 512, p_release_info, sizeof(struct release_info_s));
	if(p_release_info->magic == RELEASE_INFO_MAGIC){
		calc_crc = crc32(0, p_release_info->str, RELEASE_INFO_STR_SIZE);
		if(p_release_info->crc32 != calc_crc){
			printf("warning: the release info string crc is error, save_crc(0x%x) != calc_crc(0x%x).\n", p_release_info->crc32, calc_crc);
			display_demo_warning();
			return -1;
		}

		p_tmp = p_release_info->str;
		for(i = 0; i < RELEASE_INFO_STR_SIZE; i++){
			if(*p_tmp++ == '#')
				break;
		}
		printf_size = (unsigned int)p_tmp - (unsigned int)p_release_info->str - 1;
		p_printf_str = malloc(RELEASE_INFO_STR_SIZE);
		memset(p_printf_str, 0, RELEASE_INFO_STR_SIZE);
		memcpy(p_printf_str, p_release_info->str, printf_size);

		printf("\n");
		printf("this is release version, the string is:\n");
		printf("%s\n", p_printf_str);
		printf("\n");
		free(p_printf_str);

		return 0;
	}if(p_release_info->magic == UNRELEASE_INFO_MAGIC){
		printf("\nwarning: the flash.bin is a demo version, please confirm.\n\n");
		display_demo_warning();
		return -1;
	}
	return -1;
}
#endif

int check_hw_version(void)
{
	return 0;
}

void HandleUndef(void)
{
	while (1);
}

void HandleSWI(void)
{
	while (1);
}

void HandlePabort(void)
{
	while (1);
}

void HandleDabort(void)
{
	while (1);
}

#ifdef CONFIG_ENABLE_NET
static void gx_setup_net(void)
{
}
#endif

static unsigned int __get_pll_frequency(unsigned int pll)
{
	unsigned int refdiv ;
	unsigned int postdiv2 ;
	unsigned int postdiv1 ;
	unsigned int FBdiv ;
	unsigned int pll_val;

	pll_val = __raw_readl(pll);
	refdiv =   MAX((pll_val >> 20) & 0x3F, 1);
	postdiv2 = MAX((pll_val >> 16) & 0x7, 1);
	postdiv1 = MAX((pll_val >> 12) & 0x7, 1);
	FBdiv =    MAX((pll_val) & 0xFFF, 1);

	return (DEFAULT_EXT_CLOCK_XTAL / 1000000 * FBdiv / postdiv1 / postdiv2 / refdiv);

}

static void gx_print_taurus_allregisters(void)
{
	unsigned int div, cpu_fre,cpu_base;
	unsigned int pll_ddr;
	unsigned int refdiv ;
	unsigned int postdiv2 ;
	unsigned int postdiv1 ;
	unsigned int FBdiv ;

	cpu_base = *(volatile unsigned int *)(CONFIG_BASE_MMU + CONFIG_DTO12_CONFIG);
	div = cpu_base & 0x3FFFFFFF;
	pll_ddr = *(volatile unsigned int *)(CONFIG_BASE_MMU + CONFIG_PLL2_CONFIG);
	cpu_fre = ((unsigned long long)PLL_DTO_CLK * div) >> 30;

	refdiv =   MAX((pll_ddr >> 20) & 0x3F, 1);
	postdiv2 = MAX((pll_ddr >> 16) & 0x7, 1);
	postdiv1 = MAX((pll_ddr >> 12) & 0x7, 1);
	FBdiv =    MAX((pll_ddr) & 0xFFF, 1);

	printf("cpu freq\t: %d MHz\r\n", cpu_fre/1000000);
	printf("memory freq\t: %d MHz\r\n", DEFAULT_EXT_CLOCK_XTAL / 1000000 * FBdiv / postdiv1 / postdiv2 / refdiv);
}


static void gx_print_scorpio_allregisters(void)
{
	unsigned int pll_cpu_addr = (CONFIG_BASE_MMU + CONFIG_PLL3_CONFIG);
	unsigned int pll_ddr_addr = (CONFIG_BASE_MMU + CONFIG_PLL2_CONFIG);

	// scorpio 的cpu频率是DTO_PLL/2 = 1188/2 = 594
	printf("cpu freq\t: %d MHz\r\n", 594);
	printf("memory freq\t: %d MHz\r\n", __get_pll_frequency(pll_ddr_addr));
}

void gx_print_allregisters(void)
{
	if (gxcore_chip_probe() == 0x3113)
		gx_print_scorpio_allregisters();
	else
		gx_print_taurus_allregisters();
}

static unsigned char* get_board_type(void)
{
	static unsigned char board_name[10];
	unsigned char *name;

	name = gx_get_chip_name();
	memcpy(board_name, name, 6);
	board_name[6]='\0';

	return board_name;
}

static unsigned char* get_chip_mode(void)
{
	if (gxcore_chip_probe() == 0x3113)
		return "scorpio";
	else
		return "taurus";
}

static unsigned int get_ddr_size(void)
{
	unsigned char value;
	gx_otp_read(0x10D, 1, &value);
	value = (value >> 4) & 0xf;
	if (value == 8)
		return 128*1024*1024;
	else
		return 64*1024*1024;
}

void gx_print_board_info(void)
{
	printf("cpu family\t: %s\n"     , CONFIG_ARCH);
	printf("chip model\t: %s\n"     , get_chip_mode());
	printf("board type\t: %s\n"     , get_board_type());
	printf("memory size\t: %lu MB\n", get_ddr_size() / 1024 / 1024);

}

void cmdline_add_chip_type(void)
{
}

#ifdef CONFIG_USB_EYE_DIAGRAM_TEST
static void usb_phy_parameter_print(void)
{
#define PRINT_LEN 0x50
	int i = 0;
	int total_num = 0;

	total_num = PRINT_LEN;
	for (i = 0; i < total_num; i+=4) {
		if ((i % 16) == 0)
			printf("\n0x%x: ", USB_PHY_PORT1 + i);
		printf("0x%08x ", *(volatile unsigned int *)(USB_PHY_PORT1 + i));
	}
	printf("\n");

	total_num = PRINT_LEN;
	for (i = 0; i < total_num; i+=4) {
		if ((i % 16) == 0)
			printf("\n0x%x: ", USB_PHY_PORT2 + i);
		printf("0x%08x ", *(volatile unsigned int *)(USB_PHY_PORT2 + i));
	}
	printf("\n");
}

static void usb_ehci_test(void)
{
#define EHCI_OPBASE	(REG_BASE_USB_EHCI+0x10)

#define EHCI_OPREG_USBCMD		(EHCI_OPBASE+0x0)
#define EHCI_OPREG_USBSTS		(EHCI_OPBASE+0x4)
#define EHCI_OPREG_USBINTR		(EHCI_OPBASE+0x8)
#define EHCI_OPREG_FRINDEX		(EHCI_OPBASE+0xc)
#define EHCI_OPREG_CTRLDSSEGMENT	(EHCI_OPBASE+0x10)
#define EHCI_OPREG_PERIODICLISTBASE	(EHCI_OPBASE+0x14)
#define EHCI_OPREG_ASYNCLISTADDR	(EHCI_OPBASE+0x18)
#define EHCI_OPREG_CONFIGFLAG		(EHCI_OPBASE+0x40)
#define EHCI_OPREG_PORTSC1		(EHCI_OPBASE+0x44)
#define EHCI_OPREG_PORTSC2		(EHCI_OPBASE+0x48)
#define EHCI_OPREG_PORTSC3		(EHCI_OPBASE+0x4c)
	printf("enter echi test, hardware engineer can test after this printf.\n");
	*(volatile unsigned int *)(EHCI_OPREG_USBCMD) &= ~((1<<5)|(1<<4));
	*(volatile unsigned int *)(EHCI_OPREG_PORTSC1) |= (1<<7);
	*(volatile unsigned int *)(EHCI_OPREG_USBCMD) &= ~0x1;	//ehci stop
	while(((*(volatile unsigned int *)(EHCI_OPREG_USBSTS)) & (1<<12)) == 0);
	*(volatile unsigned int *)(EHCI_OPREG_PORTSC1) |= (4<<16);
	*(volatile unsigned int *)(EHCI_OPREG_PORTSC2) |= (4<<16);
	*(volatile unsigned int *)(EHCI_OPREG_PORTSC3) |= (4<<16);
	while(1);
}

void usb_eye_test(void)
{
	extern int usb_module_clock_enable(void);

	usb_module_clock_enable();
	usb_phy_parameter_print();
	usb_ehci_test();
	return ;
}
#endif

/****************************************************
 *
 *Intialization for board-gx3201
 *
 * *************************************************/

void arch_init(void)
{
#ifndef CONFIG_ENABLE_GDB_DEBUG
	gx_setup_pll_full_controller();
#endif

#ifdef CONFIG_USB_EYE_DIAGRAM_TEST
	usb_eye_test();
#endif

	/* default: ts1&ts2 parallel mode, no valid, no sync, little endian */
//	ts_mode_config(1, 0, 0, 0, 0, 0, 1);
//	ts_mode_config(2, 0, 0, 0, 0, 0, 1);

	board_init();

	cmdline_add_chip_type();

#ifdef CONFIG_ENABLE_NET
//	gx_setup_net();
#endif

	//asm volatile ("jsri .\n");
	if (gxcore_chip_probe() == 0x3113)
		clock_init();

	vout_init();

	aout_spdif_powercfg();
}

