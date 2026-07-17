/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   gx3201.c
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   The functions of GX3201 in lowpower
*****************************************************************************/

#include "gxcomm.h"
#if GXCHIP_TYPE == GXCHIP_TYPE_GX3201

#define CONFIG_BASE                      0x0030a000
#define CONFIG_CLOCK_DIV_CONFIG          0x24
#define CONFIG_SOURCE_SEL                0x170
#define CONFIG_SOURCE_SEL2               0x174
#define CONFIG_MPEG_CLK_INHIBIT_NORM     0x18
#define CONFIG_DTO_BASE                  0x28

// fo = fi*div/2^30
// div = fo*2^30/fi
/*static void DTO_Config(unsigned int dto, unsigned int div)
{
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31);
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = 0;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31);
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|div;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|(1<<30)|div;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|(1<<30)|div;
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_DTO_BASE+4*(dto-1)) = (1<<31)|div;
}*/

/*****************************************************************************
 * Function    : GX3201_Delay
 * Description : Delay some time
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
/*static void GX3201_Delay(void)
{
	U32 Index;
	for(Index = 0;Index < 10000;Index++);
}*/

/*****************************************************************************
 * Function    : GX3201_ClosePll
 * Description : Close all the clock except the APB clock and cpu clock
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3201_ClosePll(void)
{
#if 0
	// config multi pin
	*(volatile unsigned int*)0x0030a140 = 0;
	*(volatile unsigned int*)0x0030a144 = 0;
	*(volatile unsigned int*)0x0030a148 = 0;
#endif

	*(volatile unsigned int*)0x0030a000 |= (1);

	// SVB S2
	*(volatile unsigned int *)0x0030a000 |= 1<<20;
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL2) |= (1<<16);
	*(volatile unsigned int *)(CONFIG_BASE+CONFIG_CLOCK_DIV_CONFIG)  &=~(1<<23);

	*(volatile unsigned int *)0x04900084 = 0;		// Video DAC
	*(volatile unsigned int *)0x0030a104 |= 1<<10;	// USB PHY1
	*(volatile unsigned int *)0x0030a104 |= 1<<26;	// USB PHY2
	*(volatile unsigned int *)0x0030a108 |= 1<<26;	// USB PHY3
	*(volatile unsigned int *)0x0030a200 = 0;		// DVB ADC
	*(volatile unsigned int *)0x0030a000 |= 1<<7;	// HDMI
	*(volatile unsigned int *)0x00c0040c |= 1<<24;	// DLL 0
	*(volatile unsigned int *)0x00c00424 |= 1<<24;	// DLL 1
	
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL) &= ~((0xff<<0)|(0x1ff<<12)|(0xf<<24));
	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_SOURCE_SEL2) &= ~((0x3f<<0)|(0xf<<12));

	*(volatile unsigned int*)(CONFIG_BASE+CONFIG_MPEG_CLK_INHIBIT_NORM) |= 0xffffffff&(~(1<<15))&(~(1<<16));

	/* must select cpu from xtal before close dto */
	*(volatile unsigned int *)0x0030a0cc |= 2;	// close DTO PLL

	*(volatile unsigned int *)0x0030a0e4 |= 2;	// SDR PLL
	*(volatile unsigned int *)0x0030a0c4 |= 2;	// VID PLL
	*(volatile unsigned int *)0x0030a0d4 |= 2;	// AUD PLL
	*(volatile unsigned int *)0x0030a0dc |= 2;	// DVB PLL
}

#if DEBUG_EN > 0

/*****************************************************************************
 * Function    : GX3201_ConfigPll
 * Description : Set the frequency of AHB to 108MHz
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3201_ConfigPll(void)
{
}

#endif

/*****************************************************************************
 * Function    : GX3201_CloseModule
 * Description : Close the modules such as WDT, INTC and TV
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3201_CloseModule(void)
{
#if 0
	//cold reset modle
	*(volatile unsigned int *)0x0050a000 = 0xfffffffe;

	//*(volatile unsigned int *)0x0050a004 = ((1 << 28) | (1 << 29));
	//*(volatile unsigned int *)0x0050a010 = ((1 << 28) | (1 << 29));
	GX3201_Delay();
	//*(volatile unsigned int *)0x0050a014 = ((1 << 28) | (1 << 29));
	//*(volatile unsigned int *)0x0050a008 = ((1 << 28) | (1 << 29));

	//Close WDT
	rWDT_CTRL = 0;

	//Close INTC
	rINTC_NENCLR  = 0xFFFFFFFF;
	rINTC_NENCLR2 = 0xFFFFFFFF;
	rINTC_FENCLR  = 0xFFFFFFFF;
	rINTC_FENCLR2 = 0xFFFFFFFF;

	//Close Audio DAC cold reset
	rVIDEOOUT_CTL |= (1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19);

	//set apb clock
	SOURCE_SEL &= ~(1<<11);

	//set cpu clock
	SOURCE_SEL &= ~(1<<10);

	//set cpu clock
	SOURCE_SEL &= ~(1<<13);

	unsigned int i;
	for(i=0;i<0xfff;i++);

	//PLL power down
	rPLL1_CONFIG &= ~(1<<15);	//PLL1 power down
	rPLL2_CONFIG &= ~(1<<15);	//PLL2 power down
	rPLL3_CONFIG &= ~(1<<15);	//PLL3 power down
	rPLL4_CONFIG &= ~(1<<15);	//PLL4 power down
	rPLL5_CONFIG &= ~(1<<15);	//PLL5 power down

	//dll power down
	rDLL_CONFIG &= ~(1<<5);

	//close USB clock input
	rDIV_CONFIG  |=  (3<<20);
	rDTO5_CONFIG &= ~(1<<31);
	rDTO6_CONFIG &= ~(1<<31);

	//DA power down
	rCLK_CONFIG   &= ~(1<<22);
	rVIDEOOUT_CTL |=  (1<<19);
	rCLK_CONFIG   |=  (1<<22);
#endif
}

/*****************************************************************************
 * Function    : GX3201_SetGpio
 * Description : Set the gpio
 * Arguments   : void
 * Returns     : void
 * Other       : void
 * **************************************************************************/
void GX3201_SetGpio(U64 GpioMask,U64 GpioData)
{
#if 0
	*(volatile unsigned int *)0xa030a134 &= ~(0x03 << 10);
	*(volatile unsigned int *)0xa030a134 |= (0x03 << 10);
	*(volatile unsigned int *)0xa0306000 |= (1 << 5);
	*(volatile unsigned int *)0xa030600c |= (1 << 5);
#endif
}

gxlowpower_func_t gxlowpower_func =
{
	.ClosePll     = GX3201_ClosePll,
	.CloseModule  = GX3201_CloseModule,
	.SetGpio      = GX3201_SetGpio,

#if DEBUG_EN > 0
	.ConfigPll    = GX3201_ConfigPll,
#else
	.ConfigPll    = NULL,
#endif
};

#endif

