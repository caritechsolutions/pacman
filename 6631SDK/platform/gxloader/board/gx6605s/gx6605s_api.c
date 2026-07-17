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

#include "partition.h"
#include "board-common.h"

extern int board_init(void);
extern void udelay(volatile unsigned int delay);
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

void gx_print_allregisters(void)
{
#if 0
	unsigned char clkn, clkm, clkod;
	unsigned int fvco, cpu_fre,cpu_base;

	cpu_base = *(volatile unsigned int *)(GX_REG_VIRTUAL_BASE1 + PLL_CPU_CONFIG_BASE);
	clkm = cpu_base & 0xff;
	clkn = (cpu_base >> 8) & 0xf;
	clkod =(cpu_base >> 12) & 0x3;

	fvco = GX_EXT_CLOCK * clkm / clkn;
	cpu_fre = fvco / (1 << clkod);

	printf("cpu freq\t: %d MHz\r\n", cpu_fre/1000000);
	printf("memory freq\t: %d MHz\r\n", DDR_FREQUENCY_CONFIG/1000000);
#endif
}

void cmdline_add_chip_type(void)
{
#ifdef CONFIG_ARCH_CKMMU_GX3211
	cmdline_add("chipid=0x3211");
#endif
#ifdef CONFIG_ARCH_CKMMU_GX6605S
	cmdline_add("chipid=0x6605");
#endif
}

#ifdef CONFIG_USB_EYE_DIAGRAM_TEST
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
	ts_mode_config(1, 0, 0, 0, 0, 0, 1);
	ts_mode_config(2, 0, 0, 0, 0, 0, 1);

	board_init();

	cmdline_add_chip_type();

#ifdef CONFIG_ENABLE_NET
	gx_setup_net();
#endif

	vout_init();
}

