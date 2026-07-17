/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the GX3201 boot loader */

#include "stdio.h"
#include "string.h"

#include "io.h"
#include "config.h"
#include "gxhwlib_registers.h"

#include "util.h"
#include "gx_api.h"
#include "cpu_config.h"

#include "partition.h"

extern int board_init(void);
extern void udelay(volatile unsigned int delay);

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
	unsigned int div, cpu_fre;
	div = *(volatile unsigned int *)(CONFIG_BASE_MMU + CONFIG_DTO_BASE + 4 * (13 - 1)) & 0x3fffffff;
	cpu_fre = (unsigned long long)PLL_DTO_CLK * div / (1024 * 1024 * 1024);
	printf("cpu freq\t: %d MHz\r\n", cpu_fre/1000000);
	printf("memory freq\t: %d MHz\r\n", DDR_FREQUENCY_CONFIG/1000000);
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

	board_init();

#ifdef CONFIG_ENABLE_NET
	gx_setup_net();
#endif
}


