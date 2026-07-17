#include "gx_cec.h"
#include "reg_dw8051.h"
#include "power_manage.h"
#include "string.h"
#include "parse_cmdline.h"
#include "config.h"
#include "gpio.h"

#define XBYTE ((unsigned char volatile __xdata *) 0)
#define CEC_CTRL	32808
#define CEC_MASK	32769
#define CEC_ADDRL	32770
#define CEC_ADDRH	32771
#define CEC_TX_CNT	32772
#define CEC_RX_CNT	32773
#define CEC_TX_DATA00	32774
#define CEC_TX_DATA01	32775
#define CEC_TX_DATA02	32776
#define CEC_TX_DATA03	32777
#define CEC_TX_DATA04	32778
#define CEC_TX_DATA05	32779
#define CEC_TX_DATA06	32780
#define CEC_TX_DATA07	32781
#define CEC_TX_DATA08	32782
#define CEC_TX_DATA09	32783
#define CEC_TX_DATA10	32784
#define CEC_TX_DATA11	32785
#define CEC_TX_DATA12	32786
#define CEC_TX_DATA13	32787
#define CEC_TX_DATA14	32788
#define CEC_TX_DATA15	32789
#define CEC_RX_DATA00	32790
#define CEC_RX_DATA01	32791
#define CEC_RX_DATA02	32792
#define CEC_RX_DATA03	32793
#define CEC_RX_DATA04	32794
#define CEC_RX_DATA05	32795
#define CEC_RX_DATA06	32796
#define CEC_RX_DATA07	32797
#define CEC_RX_DATA08	32798
#define CEC_RX_DATA09	32799
#define CEC_RX_DATA10	32800
#define CEC_RX_DATA11	32801
#define CEC_RX_DATA12	32802
#define CEC_RX_DATA13	32803
#define CEC_RX_DATA14	32804
#define CEC_RX_DATA15	32805
#define CEC_LOCK	32806
#define CEC_WAKEUPCTRL	32807

#define MAX_CEC_RX_DATA_NUM	10

static volatile char tv_resume_flag = 0;

void cec_init(void)
{
	mcu_config_reg |= 0x18; //CEC模块使能[4]和复位[3]
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		mcu_config_reg |= 0x01 << 1;
	IE |= 1 << 2;
	MCU_INT1_EN = 0x7f;	/* 使能CEC中断 */
	XBYTE[CEC_MASK] = 0;
	TCON |= 1 << 2;	/* int1_n下降沿触发中断 必须配置为下降沿中断,否则会出现程序死机的问题 */
	gx_gpio_funsel_config(CEC_GPIO, 2);
	XBYTE[CEC_WAKEUPCTRL] = 0;
}

void cec_suspend(void)
{
	XBYTE[CEC_TX_CNT] = 0x02;
	XBYTE[CEC_TX_DATA00] = 0x10;
	XBYTE[CEC_TX_DATA01] = 0x36;
	XBYTE[CEC_CTRL] = 0x03;
}
void cec_resume(void)
{
	XBYTE[CEC_TX_CNT] = 0x02;
	XBYTE[CEC_TX_DATA00] = 0x10;
	XBYTE[CEC_TX_DATA01] = 0x04;
	XBYTE[CEC_CTRL] = 0x03;
}

unsigned char cec_get_tv_resume_flag(void)
{
	return tv_resume_flag;
}

void cec_set_tv_resume_flag(unsigned char flag)
{
	tv_resume_flag = flag;
}

void cec_ISR(void) __interrupt 2 //int1_n ISR
{
	if (MCU_INT1 & 0x01)
	{

	}
	if (MCU_INT1 & 0x02)
	{
		unsigned char data = XBYTE[CEC_WAKEUPCTRL];
		if (data != 0) {
			tv_resume_flag = 1;
			XBYTE[CEC_WAKEUPCTRL] = 0;
		}
		XBYTE[CEC_LOCK] = 0;
	}
	if (MCU_INT1 & 0x04)
	{

	}
	if (MCU_INT1 & 0x08)
	{

	}
	if (MCU_INT1 & 0X10)
	{

	}
	if (MCU_INT1 & 0x20)
	{

	}
	if (MCU_INT1 & 0x40)
	{

	}
	MCU_INT1 = 0x7F;
}
