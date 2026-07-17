#include <stdio.h>
#include "guestbus.h"
//#include "gpio.h"
//#include "board-common.h"

#include "app.h"



#ifdef HAVE_LIB_CIM
#include "module/app_gpio.h"
#ifdef ECOS_OS
#include "gxcore_hw_bsp.h"
#else
//#include <linux/gx_gpio.h>
#endif
//#define GUESTBUS_DEBUG

#define CI_CD1       100
#define CI_CD2       101
#define CI_RESET     102
#define CI_REGSELECT 103

#define SGB_PORT_IOWRn	111//33//  0
#define SGB_PORT_IORDn	110//32 // 1
#define SGB_PORT_OEn	112//34// 2
#define SGB_PORT_WEn	113//36// 3

#define SGB_PORT_CSn0	115//28//5


#define SGB_PORT_D0	117//15//6
#define SGB_PORT_D1	116//14//7
#define SGB_PORT_D2	118//31//8
#define SGB_PORT_D3	123//54//9
#define SGB_PORT_D4	122//53//10
#define SGB_PORT_D5	121//52//11
#define SGB_PORT_D6	119//30//12
#define SGB_PORT_D7	120//50//13


#define SGB_PORT_A0	133//49  //SCCD  //21
#define SGB_PORT_A1	132//48 //SCPWR  //20
#define SGB_PORT_A2	131//47 //SCRST  //19
#define SGB_PORT_A3	130//46 //SCCLK  //18
#define SGB_PORT_A4	128//44//  17
#define SGB_PORT_A5	129//45//29
#define SGB_PORT_A6	126//42//28
#define SGB_PORT_A7	127//43//27
#define SGB_PORT_A8	125//41//26
#define SGB_PORT_A9	124//40//25
#define SGB_PORT_A10	134//38//24
#define SGB_PORT_A11	135//39//23
#define SGB_PORT_A12	136//29//22
#define SGB_PORT_A13	137//30//16
#define SGB_PORT_A14	138//31//15
//#define SGB_PORT_A15	14

#define GX_GPIO_INPUT	0
#define GX_GPIO_OUTPUT	1


#define SGB_DAT_MASK	(0xff<<6)
#define SGB_DAT_OFS	(6)
#define SGB_OUT_MASK	((0x3f)|(0x1f<<17)|(0x7f<<23))


int sgb_data_pins[] = {
 SGB_PORT_D0,
 SGB_PORT_D1,
 SGB_PORT_D2,
 SGB_PORT_D3,
 SGB_PORT_D4,
 SGB_PORT_D5,
 SGB_PORT_D6,
 SGB_PORT_D7
};


int sgb_addr_pins[] = {
SGB_PORT_A0,
SGB_PORT_A1,
SGB_PORT_A2,
SGB_PORT_A3,
SGB_PORT_A4,
SGB_PORT_A5,
SGB_PORT_A6,
SGB_PORT_A7,
SGB_PORT_A8,
SGB_PORT_A9,
SGB_PORT_A10,
SGB_PORT_A11

};



void gb_delay(unsigned int ms)
{
	GxCore_ThreadDelay(ms);
}

void gb_delay_us(unsigned int us)
{
    volatile unsigned int i = 40*us;
	while(i--);
}

void gb_delay_ns(unsigned int ns200)
{
    volatile unsigned int i = ns200;
	while(i--);
#if 0
	int i = ns200;
	while(i--){
		__asm__ __volatile__ ("nop");
		}
#endif
}

static void sgb_set_addr(unsigned int addr)
{
	int i;


	for(i=0;i<12;i++)
	{
		if(addr&(1<<i))

			app_ci_gpio_setlevel(sgb_addr_pins[i], 1);
		else
			app_ci_gpio_setlevel(sgb_addr_pins[i], 0);
	}

}

static void sgb_dat_in(void)
{

	int i=0;

	for(i=0;i<8;i++)
	{
		app_ci_gpio_setio(sgb_data_pins[i], GX_GPIO_INPUT);

	}

}

static void sgb_dat_out(void)
{
	int i=0;

	for(i=0;i<8;i++)
	{
		app_ci_gpio_setio(sgb_data_pins[i], GX_GPIO_OUTPUT);
	}

}

static void sgb_set_iowrn(char value)
{

	if(value)
		app_ci_gpio_setlevel(SGB_PORT_IOWRn, 1);
	else
		app_ci_gpio_setlevel(SGB_PORT_IOWRn, 0);

}

static void sgb_set_iordn(char value)
{

	if(value)
		app_ci_gpio_setlevel(SGB_PORT_IORDn, 1);
	else
		app_ci_gpio_setlevel(SGB_PORT_IORDn, 0);

}

static void sgb_set_oen(char value)
{
	if(value)
		app_ci_gpio_setlevel(SGB_PORT_OEn, 1);
	else
		app_ci_gpio_setlevel(SGB_PORT_OEn, 0);

}

static void sgb_set_wen(char value)
{

	if(value)
		app_ci_gpio_setlevel(SGB_PORT_WEn, 1);
	else
		app_ci_gpio_setlevel(SGB_PORT_WEn, 0);

}

static void sgb_set_cs0n(char value)
{

	if(value)
		app_ci_gpio_setlevel(SGB_PORT_CSn0,1);

	else
		app_ci_gpio_setlevel(SGB_PORT_CSn0,0);

}

static void sgb_set_dat(char dat)
{
	int i=0;
	for(i=0;i<8;i++)
	{
		if((dat<<(7-i))&0x80)
		{
			app_ci_gpio_setlevel(sgb_data_pins[i], 1);
		}
		else
		{
			app_ci_gpio_setlevel(sgb_data_pins[i], 0);
		}

	}

}

static unsigned char sgb_get_dat(void)
{
	int i=0;
	unsigned char tmp =0;
	for(i=0;i<8;i++)
	{
		if(app_ci_gpio_getlevel(sgb_data_pins[i]))
		{
			tmp=(tmp>>1)|0x80;

		}
		else
		{
			tmp=(tmp>>1)&0x7F;
		}

	}
	return tmp;

}





void gb_init(void)
{
	app_ci_gpio_setlevel(CI_RESET, 0);
//	gx_gpio_setlevel(CI_REGSELECT, 0); //it has been connected to GND
}
int gb_detect_card(void)
{
	int ret;
	int cd1;
	//int cd2;
	static int flag = -1;
	cd1 = app_ci_gpio_getlevel(CI_CD1);
//	app_log_debug(" cd1 =%d [%s] [%d] \n",cd1,__FUNCTION__,__LINE__);
	//cd2 = app_ci_gpio_getlevel(CI_CD2);
	//ret = (cd1|cd2)?0:1;
    ret = cd1?0:1;
	if(ret && (flag != 1))
	{
		app_log_debug("\nCI, detect the ci module\n");
		flag = 1;
	}
	else if((ret == 0)&& (flag == 1))
	{
		app_log_debug("\nCI, the ci module is removed\n");
		flag = 0;
	}

	return ret;
}



void gb_reset_card(void)
{

	sgb_set_cs0n(1);
	sgb_set_oen(1);
	sgb_set_wen(1);
	sgb_set_iordn(1);
	sgb_set_iowrn(1);
	app_ci_gpio_setlevel(CI_RESET,1);
	gb_delay(100);
	app_ci_gpio_setlevel(CI_RESET, 0);

}

void gb_readmem(unsigned int addr,unsigned char *rbuf,unsigned int len)
{
#ifdef GUESTBUS_DEBUG
    unsigned char *p = rbuf;
	unsigned int start_addr = addr;
#endif
	unsigned i;
	sgb_dat_in();
	sgb_set_cs0n(0);

	for(i=0; i<len; i++) {


		sgb_set_addr(addr);
		gb_delay_ns(100);
		sgb_set_oen(0);
		gb_delay_ns(100);
		*(rbuf++) = sgb_get_dat();
		sgb_set_oen(1);
		gb_delay_ns(10);
		addr += 2;
			}
	sgb_set_cs0n(1);

#ifdef GUESTBUS_DEBUG
	app_log_debug("\nCI, %s,%d, debug:addr = 0x%x, len = %d\n",__FUNCTION__,__LINE__,start_addr,len);
	for(i = 0; i < len; i++)
	{
		if(i%16==0)
			app_log_debug("\n");

		app_log_debug(" 0x%02x", p[i]);
	}
	app_log_debug("\nCI, debug end!\n");
#endif
}

void gb_writemem(unsigned int addr, unsigned char *wbuf, unsigned int len)
{
	unsigned i;
#ifdef GUESTBUS_DEBUG
	unsigned int addr_temp = addr;
	app_log_debug("\nCI, %s,%d, debug:addr = 0x%x, len = %d\n",__FUNCTION__,__LINE__,addr,len);
	for(i = 0; i < len; i++)
	{
		app_log_debug(" 0x%02x", wbuf[i]);
	}

#endif
	sgb_dat_out();
	sgb_set_cs0n(0);

	for(i=0; i<len; i++) {

		sgb_set_addr(addr);
		gb_delay_ns(100);
		sgb_set_dat(*(wbuf++));
		gb_delay_ns(10);
		sgb_set_wen(0);
		gb_delay_ns(100);
		sgb_set_wen(1);
		gb_delay_ns(10);
		addr += 2;

	}
	sgb_set_cs0n(1);
#if 0//def GUESTBUS_DEBUG
	app_log_debug("\nCI, debug end!\n");
{
	unsigned char data = 0;
    gb_readmem(addr_temp, &data, 1);
	app_log_debug("\nCI, read data after write, data = 0x%x\n", data);
}
#endif

}
void gb_readio(unsigned int addr,unsigned char *rbuf,unsigned int len)
{
#ifdef GUESTBUS_DEBUG
	unsigned char *p = rbuf;
	unsigned int start_addr = addr;
	app_log_debug("\nCI, %s,%d, debug:addr = 0x%x, len = %d\n",__FUNCTION__,__LINE__,start_addr,len);
#endif
	unsigned i;
	sgb_dat_in();
	sgb_set_cs0n(0);

	for(i=0; i<len; i++) {
		sgb_set_addr(addr);
		gb_delay_ns(10);
		sgb_set_iordn(0);
		gb_delay_ns(100);
		*(rbuf++) = sgb_get_dat();
		sgb_set_iordn(1);
		gb_delay_ns(10);

	}
	sgb_set_cs0n(1);
#ifdef GUESTBUS_DEBUG

	for(i = 0; i < len; i++)
	{
		app_log_debug(" 0x%02x", p[i]);
	}
	app_log_debug("\nCI, debug end!\n");

#endif
}


void gb_writeio(unsigned int addr, unsigned char *wbuf, unsigned int len)
{
	unsigned i;
#ifdef GUESTBUS_DEBUG
	app_log_debug("\nCI, %s,%d, debug:addr = 0x%x, len = %d\n",__FUNCTION__,__LINE__,addr,len);
	for(i = 0; i < len; i++)
	{
		app_log_debug(" 0x%02x", wbuf[i]);
	}

#endif
	sgb_dat_out();
	sgb_set_cs0n(0);

	for(i=0; i<len; i++) {
		sgb_set_addr(addr);
		sgb_set_dat(*(wbuf++));
		gb_delay_ns(10);
		sgb_set_iowrn(0);
		gb_delay_ns(100);
		sgb_set_iowrn(1);
		gb_delay_ns(10);

	}
	sgb_set_cs0n(1);
#ifdef GUESTBUS_DEBUG
	app_log_debug("\nCI, debug end!\n");
#endif

}


#endif
