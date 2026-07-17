#include "reg_dw8051.h"
#include "gpio.h"
#include "gx_irr.h"
#include "parse_cmdline.h"
#include "config.h"

static irr_parse_t irr_parse;

void irr_init(void)
{
	/* 红外接收引脚配置在对应中断int0_n 中断*/
	gx_gpio_setio(IRR_GPIO, 0);	/* 设置为输入模式 */

	TCON |= 1 << 0;	/* int0_n下降沿触发中断 */

	/* 用timer 0计数脉冲时间 */
	CKCON &= ~(1 << 3); /* CLK / 12*/
	TMOD &= ~(3 << 2);
	TMOD |= 1 << 0; /* 16bit counter */
	TH0 = 0;
	TL0 = 0;
	TCON |= 1 << 4; /* enable timer 0*/

	IE |= 1 << 0; /* 打开int0_n中断 */
	
	/* 目前支持的几个红外协议都有一个共性即两个按键必定有一个延时,且延时必定大于30ms,这里用定时器0的溢出中断做红外遥控两个按键的区分 */
	IE |= 1 << 1; /* 打开定时器超时溢出中断,66536/(CLK*12) = 0.029s CLK = 24000000 or 27000000*/

	irr_parse.key_code = 0;
	irr_parse.key_code_valid = 0;
	irr_parse.protocol = IRR_NOT_SURE;
	irr_parse.pulse_num = 0;
	irr_parse.pulse_width = 0;
}

char irr_get_key(unsigned long *key_value, unsigned char *protocol)
{
	IE &= ~(1 << 7); //disable global interrupt
	if (irr_parse.key_code_valid)
	{
		*key_value = irr_parse.key_code;
		*protocol = irr_parse.key_protocol;
		irr_parse.key_code_valid = 0;
		irr_parse.key_protocol = IRR_NOT_SURE;
		irr_parse.key_code = 0;
		IE |= 1 << 7;  //enable gloable interrupt
		return 0;
	}
	IE |= 1 << 7;  //enable gloable interrupt
	return -1;
}

/* 以下所有计数为时间乘以定时器时钟频率所得,定时器时钟由系统时钟12分频得到, 系统时钟为27MHZ或者24MHZ */
/* NEC引导码两个下降沿间隔 引导13.5ms  0.0135*(CLK/12) */
static unsigned int irr_nec_boot_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 30000; //(0.0135 + 0.0015)*(24000000/12) = 30000
	else
		return 33750; //(0.0135 + 0.0015)*(27000000/12) = 33750
}

static unsigned int irr_nec_boot_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 24000; //(0.0135 - 0.0015)*(24000000/12) = 24000
	else
		return 27000; //(0.0135 - 0.0015)*(27000000/12) = 27000
}

/* NEC简码(连续按键)两个下降沿间隔 引导11.5ms  0.01125*(CLK/12) */
static unsigned int irr_nec_simple_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 25500; //(0.01125 + 0.0015)*(24000000/12) = 25500;
	else
		return 28688; //(0.01125 + 0.0015)*(27000000/12) = 28687.5;
}

static unsigned int irr_nec_simple_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 19500; //(0.01125 - 0.0015)*(24000000/12) = 19500;
	else
		return 21938; //(0.01125 - 0.0015)*(27000000/12) = 21937.5;
}

/* NEC逻辑1 两个下降沿间隔 2.25ms 0.00225*(CLK/12) */
static unsigned int irr_nec_1_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 5300; //0.00225*(24000000/12) = 4500
	else
		return 5862; //0.00225*(27000000/12) = 5062.5
}

static unsigned int irr_nec_1_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 4100; //0.00225*(24000000/12) = 4500
	else
		return 4662; //0.00225*(27000000/12) = 5062.5
}

/* NEC逻辑0 两个下降沿间隔 1.125ms 0.001125*(CLK/12) */
static unsigned int irr_nec_0_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 3050; //0.00125*(24000000/12) = 2250
	else
		return 3331; //0.00125*(27000000/12) = 2531.25
}

static unsigned int irr_nec_0_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 1850; //0.00125*(24000000/12) = 2250
	else
		return 2131; //0.00125*(27000000/12) = 2531.25
}

/* 松下引导码和40bit引导码一样两个下降沿间隔 引导5.440ms  0.00544*(CLK/12) */
static unsigned int irr_panasonic_40bit_boot_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 11040; //0.00544*(24000000/12) = 10880;
	else
		return 12400; //0.00544*(27000000/12) = 12240;
}

static unsigned int irr_panasonic_40bit_boot_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 10740; //0.00544*(24000000/12) = 10880;
	else
		return 12100; //0.00544*(27000000/12) = 12240;
}

/* 松下逻辑1 两个下降沿间隔 1.73ms 0.00173*(CLK/12) 40bit逻辑1 两个下降沿间隔 1.68ms 0.00168*(CLK/12) = 3780
   松下和40bit协议的逻辑1下降沿间隔比较接近，由于采集误差和每个遥控器的发射的红外有细微区别，不好区分，所以统一定义
*/
static unsigned int irr_panasonic_40bit_1_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 3860; //0.00173*(24000000/12) = 3460;
	else
		return 4292; //0.00173*(27000000/12) = 3892.5;
}

static unsigned int irr_panasonic_40bit_1_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 2948; //0.00173*(24000000/12) = 3460;
	else
		return 3380; //0.00173*(27000000/12) = 3892.5;
}

/* 松下逻辑0 两个下降沿间隔 0.760ms 0.00076*(CLK/12) */
static unsigned int irr_panasonic_0_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 2020; //0.00076*(24000000/12) = 1520;
	else
		return 2210; //0.00076*(27000000/12) = 1710;
}

static unsigned int irr_panasonic_0_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 1120; //0.00076*(24000000/12) = 1520;
	else
		return 1310; //0.00076*(27000000/12) = 1710;
}

/* 40bit简码(连续按键)两个下降沿间隔 引导7.84ms  0.00784*(CLK/12) */
static unsigned int irr_40bit_simple_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 25400; //0.00784*(24000000/12) = 15680;
	else
		return 25400; //0.00784*(27000000/12) = 17640;
}

static unsigned int irr_40bit_simple_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 25200; //0.00784*(24000000/12) = 15680;
	else
		return 25200; //0.00784*(27000000/12) = 17640;
}

/* 40bit逻辑0 两个下降沿间隔 1.12ms 0.00112*(CLK/12)*/
static unsigned int irr_40bit_0_code_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 2670; //0.00112*(24000000/12) = 2240;
	else
		return 2950; //0.00112*(27000000/12) = 2520;
}

static unsigned int irr_40bit_0_code_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 2040; //0.00112*(24000000/12) = 2240;
	else
		return 2320; //0.00112*(27000000/12) = 2520;
}

/* Philips RC5 两个下降沿间隔 1倍脉冲时间 1.7778ms 0.0017778*(CLK/12) */
static unsigned int irr_rc5_1_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 3655; //0.0017778*(24000000/12) = 3555.6;
	else
		return 4100; //0.0017778*(27000000/12) = 4000.05;
}

static unsigned int irr_rc5_1_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 3455; //0.0017778*(24000000/12) = 3555.6;
	else
		return 3900; //0.0017778*(27000000/12) = 4000.05;
}

/* Philips RC5 两个下降沿间隔 1.5倍脉冲时间2.6667ms 0.0026667*(CLK/12) */
static unsigned int irr_rc5_1_5_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 5433; //0.0026667*(24000000/12) = 5333.4;
	else
		return 6100; //0.0026667*(27000000/12) = 6000.075;
}

static unsigned int irr_rc5_1_5_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 5233; //0.0026667*(24000000/12) = 5333.4;
	else
		return 5900; //0.0026667*(27000000/12) = 6000.075;
}

/* Philips RC5 两个下降沿间隔 2倍脉冲时间 3.5556ms 0.0035556*(CLK/12) */
static unsigned int irr_rc5_2_pulse_max_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 7211; //0.0035556*(24000000/12) = 7111.2;
	else
		return 8100; //0.0035556*(27000000/12) = 8000.1;
}

static unsigned int irr_rc5_2_pulse_min_time(void)
{
	if (get_clock_speed() == LOWPOWER_24MHZ_CLOCK)
		return 7011; //0.0035556*(24000000/12) = 7111.2;
	else
		return 7900; //0.0035556*(27000000/12) = 8000.1;
}

static void irr_parse_nec(void)
{
	if(irr_parse.pulse_num >= 2 && irr_parse.pulse_num <= 33){
		if(irr_parse.pulse_width >= irr_nec_1_code_pulse_min_time() && irr_parse.pulse_width <= irr_nec_1_code_pulse_max_time()){
			irr_parse.irr_32bit_data <<= 1;
			irr_parse.pulse_num++;
		}else if (irr_parse.pulse_width >= irr_nec_0_code_pulse_min_time() && irr_parse.pulse_width <= irr_nec_0_code_pulse_max_time()){
			irr_parse.irr_32bit_data <<= 1;
			irr_parse.irr_32bit_data |= 1;
			irr_parse.pulse_num++;
		}
		else
		{
			irr_parse.pulse_num = 0;
			return;
		}

		if(irr_parse.pulse_num == 34){
			irr_parse.key_code = irr_parse.irr_32bit_data;
			irr_parse.key_protocol = IRR_NEC;
			irr_parse.key_code_valid = 1;
		}
	}
}

static void irr_parse_philips_rc5(void)
{
	static __bit last_rc5_bit = 1;

	if (irr_parse.pulse_num == 1)
	{
		last_rc5_bit = 1;
	}

	irr_parse.pulse_num++;

	if (irr_parse.pulse_width >= irr_rc5_1_pulse_min_time() && irr_parse.pulse_width <= irr_rc5_1_pulse_max_time())
	{
		irr_parse.irr_32bit_data <<= 1;
		if (last_rc5_bit == 1)
		{
			irr_parse.irr_32bit_data |= 1;
		}
	}else if (irr_parse.pulse_width >= irr_rc5_1_5_pulse_min_time() && irr_parse.pulse_width <= irr_rc5_1_5_pulse_max_time()){
		if (last_rc5_bit == 1)
		{
			irr_parse.irr_32bit_data <<= 1;
			last_rc5_bit = 0;
		}
		else
		{
			irr_parse.irr_32bit_data <<= 1;
			irr_parse.irr_32bit_data <<= 1;
			irr_parse.irr_32bit_data |= 1;
			last_rc5_bit = 1;
		}
	}else if (irr_parse.pulse_width >= irr_rc5_2_pulse_min_time() && irr_parse.pulse_width <= irr_rc5_2_pulse_max_time())
	{
		irr_parse.irr_32bit_data <<= 1;
		irr_parse.irr_32bit_data <<= 1;
		irr_parse.irr_32bit_data |= 1;
		last_rc5_bit = 1;
	}else{
		irr_parse.pulse_num = 0;
	}
}

/* unsigned char 高低位转置 0xea -> 0x57*/
static unsigned char reverseBits(unsigned char value) {
    value = (value & 0xF0) >> 4 | (value & 0x0F) << 4;
    value = (value & 0xCC) >> 2 | (value & 0x33) << 2;
    value = (value & 0xAA) >> 1 | (value & 0x55) << 1;
    return value;
}

static void irr_parse_panasonic(void)
{
	static unsigned int panasonic_data = 0;
	if(irr_parse.pulse_num >= 34 && irr_parse.pulse_num <= 49){
		if(irr_parse.pulse_width >= irr_panasonic_40bit_1_code_pulse_min_time() && irr_parse.pulse_width <= irr_panasonic_40bit_1_code_pulse_max_time()){
			panasonic_data <<= 1;
			panasonic_data |= 1;
			irr_parse.pulse_num++;
		}else if (irr_parse.pulse_width >= irr_panasonic_0_code_pulse_min_time() && irr_parse.pulse_width <= irr_panasonic_0_code_pulse_max_time()){
			panasonic_data <<= 1;
			irr_parse.pulse_num++;
		}
		else
		{
			irr_parse.pulse_num = 0;
			return;
		}

		if(irr_parse.pulse_num == 50){
			panasonic_data = (panasonic_data&0xff00) | reverseBits(panasonic_data & 0xFF);
			panasonic_data = (panasonic_data&0x00ff) | (reverseBits((panasonic_data >> 8) & 0xFF) << 8);

			irr_parse.key_code = ((unsigned long)(panasonic_data & 0xFF00) << 16) | ((panasonic_data & 0xFF) << 8);
			irr_parse.key_protocol = IRR_PANASONIC;
			irr_parse.key_code_valid = 1;
		}
	}
	else
	{
		/* 不处理设备码，默认对的 */
		if (irr_parse.pulse_num == 33)
		{
			panasonic_data = 0;
		}
		irr_parse.pulse_num++;
	}
}

static void irr_parse_40bit(void)
{
	static unsigned long key_code = 0;

	if((irr_parse.pulse_num >= 10 && irr_parse.pulse_num <= 25)
			|| (irr_parse.pulse_num >= 27 && irr_parse.pulse_num <= 42)){
		if(irr_parse.pulse_width >= irr_panasonic_40bit_1_code_pulse_min_time() && irr_parse.pulse_width <= irr_panasonic_40bit_1_code_pulse_max_time()){
			key_code <<= 1;
			key_code |= 1;
			irr_parse.pulse_num++;
		}else if (irr_parse.pulse_width >= irr_40bit_0_code_pulse_min_time() && irr_parse.pulse_width <= irr_40bit_0_code_pulse_max_time()){
			key_code <<= 1;
			irr_parse.pulse_num++;
		}
		else
		{
			irr_parse.pulse_num = 0;
			return;
		}

		if(irr_parse.pulse_num == 43){
			irr_parse.key_code = key_code;
			irr_parse.key_protocol = IRR_40BIT;
			irr_parse.key_code_valid = 1;
		}
	}
	else
	{
		/* 不处理前8位引导码，默认对的 */
		if (irr_parse.pulse_num == 9)
		{
			key_code = 0;
		}
		irr_parse.pulse_num++;
	}
}

void timer0_ISR(void) __interrupt 1
{
	TL0 = 0x00;
	TH0 = 0x00;

	if (irr_parse.protocol == IRR_PHILIPS_RC5)
	{
		irr_parse.key_code = irr_parse.irr_32bit_data & 0x7FF;
		irr_parse.key_protocol = IRR_PHILIPS_RC5;
		irr_parse.key_code_valid = 1;
	}

	irr_parse.irr_32bit_data = 0;
	irr_parse.pulse_num = 0;
	irr_parse.protocol = IRR_NOT_SURE;
}

void int0_n_ISR(void) __interrupt 0
{
	irr_parse.pulse_width = (TH0 << 8) | TL0;
	TL0 = 0x00;
	TH0 = 0x00;

	switch (irr_parse.protocol){
		case IRR_NOT_SURE:
		{
			if (irr_parse.pulse_num == 0)
				irr_parse.pulse_num++;
			else if (irr_parse.pulse_num == 1){
				if(irr_parse.pulse_width >= irr_nec_boot_code_pulse_min_time() && irr_parse.pulse_width <= irr_nec_boot_code_pulse_max_time()){
					irr_parse.protocol = IRR_NEC;
					irr_parse.pulse_num++;
				}else if ((irr_parse.pulse_width >= irr_nec_simple_code_pulse_min_time() && irr_parse.pulse_width <= irr_nec_simple_code_pulse_max_time())
						|| (irr_parse.pulse_width >= irr_40bit_simple_code_pulse_min_time() && irr_parse.pulse_width <= irr_40bit_simple_code_pulse_max_time())){
					/* 重复按键不做操作 */
					irr_parse.pulse_num = 0;
				}else if (irr_parse.pulse_width >= irr_panasonic_40bit_boot_code_pulse_min_time() && irr_parse.pulse_width <= irr_panasonic_40bit_boot_code_pulse_max_time()){
					irr_parse.pulse_num++;
				}
				else if ((irr_parse.pulse_width >= irr_rc5_1_pulse_min_time() && irr_parse.pulse_width <= irr_rc5_1_pulse_max_time())
						|| (irr_parse.pulse_width >= irr_rc5_1_5_pulse_min_time() && irr_parse.pulse_width <= irr_rc5_1_5_pulse_max_time())
						|| (irr_parse.pulse_width >= irr_rc5_2_pulse_min_time() && irr_parse.pulse_width <= irr_rc5_2_pulse_max_time()))
				{
					irr_parse.protocol = IRR_PHILIPS_RC5;
					irr_parse.irr_32bit_data = 1; /* philips rc5起始位通常是逻辑1*/
					irr_parse_philips_rc5();
				}
				else{
					irr_parse.pulse_num = 0;
				}
			}
			else if (irr_parse.pulse_num >= 2){
				if (irr_parse.pulse_width >= irr_panasonic_40bit_1_code_pulse_min_time() && irr_parse.pulse_width <= irr_panasonic_40bit_1_code_pulse_max_time()){
					irr_parse_panasonic();
					irr_parse.pulse_num--;
					irr_parse_40bit();
				}else if ((irr_parse.pulse_width >= irr_panasonic_0_code_pulse_min_time() && irr_parse.pulse_width <= irr_panasonic_0_code_pulse_max_time())){
					irr_parse_panasonic();
					irr_parse.protocol = IRR_PANASONIC;
				}else if((irr_parse.pulse_width >= irr_40bit_0_code_pulse_min_time() && irr_parse.pulse_width <= irr_40bit_0_code_pulse_max_time())){
					irr_parse_40bit();
					irr_parse.protocol = IRR_40BIT;
				}else{
					irr_parse.pulse_num = 0;
				}
			}

			break;
		}
		case IRR_NEC:
		{
			irr_parse_nec();
			break;
		}
		case IRR_PHILIPS_RC5:
		{
			irr_parse_philips_rc5();
			break;
		}
		case IRR_PANASONIC:
		{
			irr_parse_panasonic();
			break;
		}
		case IRR_40BIT:
		{
			irr_parse_40bit();
			break;
		}
		case IRR_BESCON:
		{
			break;
		}
		default:
			break;
	}
}
