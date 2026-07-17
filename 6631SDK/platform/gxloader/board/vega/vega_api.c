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
#include "../../drivers/audio/spdif.h"


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

static unsigned int __get_inno_pll_frequency(unsigned int pll)
{
	unsigned int refdiv ;
	unsigned int postdiv2 ;
	unsigned int postdiv1 ;
	unsigned int FBdiv ;
	unsigned int pll_val;

	pll_val = __raw_readl(pll);
	refdiv =   MAX((pll_val >> 18) & 0x1F, 1);
	postdiv2 = ((pll_val >> 23) & 0x7) + 1;
	postdiv1 = ((pll_val >> 12) & 0x7) + 1;
	FBdiv =    MAX((pll_val) & 0x3FF, 1);

	return (DEFAULT_EXT_CLOCK_XTAL / 1000000 * FBdiv / postdiv1 / postdiv2 / refdiv);
}

void gx_print_allregisters(void)
{
	unsigned int pll_cpu_addr = (CONFIG_CLOCK_BASE + CONFIG_CPU_PLL_CONFIG);
#ifdef DDR_PLL_USE_INNO
	unsigned int pll_ddr_addr = (CONFIG_CLOCK_BASE + CONFIG_INNO_PLL_CONFIG0);
#else
	unsigned int pll_ddr_addr = (CONFIG_CLOCK_BASE + CONFIG_DDR_PLL_CONFIG);
#endif

	printf("cpu freq\t: %d MHz\r\n", __get_pll_frequency(pll_cpu_addr));
#ifdef DDR_PLL_USE_INNO
	printf("memory freq\t: %d MHz\r\n", __get_inno_pll_frequency(pll_ddr_addr));
#else
	printf("memory freq\t: %d MHz\r\n", __get_pll_frequency(pll_ddr_addr));
#endif
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
#if !defined(CONFIG_ENABLE_GDB_DEBUG) && !defined(CONFIG_FPGA_BOARD)
	gx_setup_pll_full_controller();
#endif

#ifdef CONFIG_USB_EYE_DIAGRAM_TEST
	usb_eye_test();
#endif

	/* default: ts1&ts2 parallel mode, no valid, no sync, little endian */
//	ts_mode_config(1, 0, 0, 0, 0, 0, 1);
//	ts_mode_config(2, 0, 0, 0, 0, 0, 1);

	board_init();

#ifdef CONFIG_ENABLE_NET
	gx_setup_net();
#endif

	vout_init();

#ifdef CONFIG_FPGA_BOARD
#define readl(addr) ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })
#define writel(b,addr) (void)((*(volatile unsigned int *) (addr)) = (b))
#ifdef CONFIG_ENABLE_NANDFLASH
	writel(0x09090909, 0x89d00058);
	writel(0x09090909, 0x89d0005c);
	writel(0x09090909, 0x89d00060);
	writel(0x09090909, 0x89d00064);
#else
	//writel(0x0a0a0a0a, 0x89d00058);
	//writel(0x0a0a0a0a, 0x89d0005c);
	writel(0x0a0a0000, 0x89d00060);
	writel(0x0a0a0a0a, 0x89d00064);
#endif
#endif

	aout_spdif_powercfg();
}


