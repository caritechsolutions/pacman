#include "reg_dw8051.h"
#include "irr.h"
#include "power_manage.h"
#include "gpio.h"
#include "gx_cec.h"
#include "gx_irr.h"
#include "parse_cmdline.h"
#include "gx_timer.h"
#include "timer.h"
#include "drvFP.h"
#include "config.h"

//#define TEST
//#define TEST_GPIO
unsigned char  flush_flag = 1;
static void enable_interrupt(void)
{
	IE |= 0x80;	//打开全局中断
}

static void disable_interrupt(void)
{
	IE &= ~0x80; //关闭全局中断
}

static void delay_ms_27mhz(unsigned int ms)
{
	unsigned int j;
	for (; ms > 0; ms--)
		for (j = 0; j< 395; j++);
}

static void delay_ms_24mhz(unsigned int ms)
{
	unsigned int j;
	for (; ms > 0; ms--)
		for (j = 0; j< 351; j++);
}

void delay_ms(unsigned int ms)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
	{
		delay_ms_24mhz(ms);
	}
	else
	{
		delay_ms_27mhz(ms);
	}
}

void serial0_initial(void)
{
	gx_gpio_funsel_config(UART_TX_GPIO, 0);
	mcu_config_reg |= 0x01;

	SCON0 = 0x40;//串口0模式1

	//baud_rate = 9600 = CLK / (32 * (65536 - RCAP2H,RCAP2L))
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
	{
		TH2 = 0xff;
		TL2 = 0xb1;
		RCAP2H = 0xff;
		RCAP2L = 0xb1;
	}
	else
	{
		TH2 = 0xff;
		TL2 = 0xa8;
		RCAP2H = 0xff;
		RCAP2L = 0xa8;
	}
	//TCLK  =  1;//这里单bit操作上板时会出错
	//TR2  =  1;
	T2CON = 0x14;//设置timer2为baud模式并为串口0发送串口数据
}

void serial0_send(unsigned char dat)//串口数据只能是八位
{
	SBUF0 = dat;
	while(!TI_0);
	TI_0 = 0;
}
#ifdef TEST
#if 0
void virtual_serial_delay(void)
{
	unsigned char delay_time = 99;
	while(delay_time--);
}

void serialv_send(unsigned char data)
{
	unsigned char i = 0;
	GPIO_P03 = 0;
	virtual_serial_delay();

	for (i = 0; i < 8; i++)
	{
		if (data & 0x01)
		{
			GPIO_P03 = 1;
		}
		else
		{
			GPIO_P03 = 0;
		}
		virtual_serial_delay();
		data >>= 1;
	}
	GPIO_P03 = 1;
	virtual_serial_delay();
}
#endif

void print_long(unsigned long data)
{
	serial0_send(data);
	serial0_send(data >> 8);
	serial0_send(data >> 16);
	serial0_send(data >> 24);
}

void dpram_test(void)
{
#define XBYTE ((unsigned char volatile __xdata *) 0)
	unsigned int i = 0;

	for (i = 0; i < 256; i++)
	{
		XBYTE[i] = i;
	}

	for (i = 256; i < 512; i++)
	{
		XBYTE[i] = 511 - i;
	}

	for (i = 0; i < 512; i++)
	{
		serial0_send(XBYTE[i]);
	}
}
#ifdef TEST_GPIO
void gpio_fixed_test(void)
{
	int i = 0;
	for (i = 0; i < MAX_GPIO_NUM; i++) {
		gx_gpio_funsel_config(i, 1);
	}
}
void gpio_output_test(void)
{
	int i = 0;
	while (1) {
		for (i = 0; i < MAX_GPIO_NUM; i++) {
			gx_gpio_setlevel(i, 1);
		}
		delay_ms(1000);
		for (i = 0; i < MAX_GPIO_NUM; i++) {
			gx_gpio_setlevel(i, 0);
		}
		delay_ms(1000);
	}
}

void gpio_input_test(void)
{
	int i = 0;
	char value;
	for (i = 0; i < MAX_GPIO_NUM; i++) {
		if (i == UART_TX_GPIO)
			continue;
		gx_gpio_setio(i, 0);
	}
	while (1) {
		for (i = 0; i < MAX_GPIO_NUM; i++) {
			if (i == UART_TX_GPIO)
				continue;
			value = gx_gpio_getlevel(i);
			if (i / 10 != 0) {
				serial0_send('0' + (i / 10));
			}
			serial0_send('0' + (i % 10));
			serial0_send(': ');
			serial0_send('0' + value);
			serial0_send('\r');
			serial0_send('\n');
		}
		delay_ms(2000);
	}
}
#endif

void mcu_int2_init(void)
{
	EIE |= 1; //打开int2外部中断
	MCU_INT2_EN = 1;	/* 使能MCU接收CPU中断 */
}

void int2_n_ISR(void) __interrupt 8
{
	/*TODO*/
	EXIF &= ~(1 << 4);
	MCU_INT2 = 1;
}

static void test_app(void)
{
	gpio_output_test();
}
#else

static void wakeupio_init(void)
{
	unsigned long wake_gpio = g_cmdline->wakeupio[0];
	char wake_level = g_cmdline->wakeupio[1];

	if (wake_gpio == 0xFFFFFFFF)
		return;

	gx_gpio_setio(wake_gpio, 0);

	if (wake_level)
		gx_gpio_setlevel(wake_gpio, 0);
	else
		gx_gpio_setlevel(wake_gpio, 1);
}

static int get_wakeupio_resume_flag(void)
{
	unsigned long wake_gpio = g_cmdline->wakeupio[0];
	char gpio_level;
	static unsigned char wakeup_flag = 0;
	unsigned int system_time;

	if (wake_gpio == 0xFFFFFFFF)
		return 0;

	gpio_level = gx_gpio_getlevel(wake_gpio);
	if (gpio_level != g_cmdline->wakeupio[1])
		wakeup_flag = 0;

	if((gpio_level == g_cmdline->wakeupio[1]) && (wakeup_flag == 0)) {
		wakeup_flag = 1;
		system_time = get_system_run_time();
	}

	if ((wakeup_flag) && (get_system_run_time() != system_time))
		return 1;
	else
		return 0;
}

static void power_control(void)
{
	unsigned long key_value = 0;
	unsigned long irr_key = 0;
	unsigned char key_protocol = IRR_NOT_SURE;
	unsigned char i = 0;
	__bit reset_key = 0;

	if (!irr_get_key(&key_value, &key_protocol))
	{
		for (i = 0; i < g_cmdline->keynum; i++)
		{
			if ((key_protocol == IRR_NEC) && (g_cmdline->keybuf[i] <= 0xFFFF))
			{
				irr_key = ((key_value & 0xFF000000) >> 16) | ((key_value & 0xFF00) >> 8);
			}
			else
			{
				irr_key = key_value;
			}

			if (g_cmdline->keybuf[i] == irr_key)
			{
				reset_key = 1;
				break;
			}
		}
	}
	if(!reset_key)
	{
		reset_key = MDrv_FD650_Powerkey_Statu(g_cmdline->keybuf[g_cmdline->keynum-1]);
	}
#ifdef ENABLE_CEC
	if (reset_key == 1 || cec_get_tv_resume_flag() == 1
		|| get_waketimeup() == 1 || get_wakeupio_resume_flag() == 1)
#else
	if (reset_key == 1 || get_waketimeup() == 1
		|| get_wakeupio_resume_flag() == 1)
#endif
	{
		if (get_waketimeup() == 1)
		{
			if (g_cmdline->reboot_flag)
				g_cmdline->wakeflag = 3;
			else
				g_cmdline->wakeflag = 2;

		}
		else
		{
			g_cmdline->wakeflag = 1;
		}
		time_write_back();
#ifdef ENABLE_CEC
		if (g_cmdline->cecmode == 1) {
			if (cec_get_tv_resume_flag() == 1)
			{
			    cec_set_tv_resume_flag(0);
			    delay_ms(10);
			}
			else
			{
			    cec_resume();
			    delay_ms(1000);	/* wait cec signal had send to TV */
			}
		}
#endif
		disable_interrupt();
		system_resume();
		while(1);
	}
}

static void lowpower_app(void)
{
	system_suspend();	/* 进入低功耗,关闭CPU并断掉CPU供电 */
	gx_gpio_init();
	irr_init();
#ifdef ENABLE_CEC
	if (g_cmdline->cecmode == 1
		|| g_cmdline->cecmode == 2) {
		cec_init();
		cec_suspend();		/* 关闭电视机(如果电视机有CEC功能) */
	}
#endif
	MDrv_FP_Init();
	wakeupio_init();

	if (g_cmdline->timeshowflag == 0)
		MDrv_FP_ShowOFF();

	while(1)
	{
		power_control();
		/*Display time */
		if(g_cmdline->timeshowflag == 1 && flush_flag == 1)
		{
			MDrv_FP_ShowTime();
			flush_flag = 0;
		}
	}
}
#endif

int main(void)
{
	mcu_control_signal = 0x01;//启动标志
	enable_interrupt();
	parse_cmdline_init();
	gx_timer_init();

#ifdef TEST
	test_app();
#else
	lowpower_app();
#endif

	return 0;
}

