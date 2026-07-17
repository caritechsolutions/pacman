#include "gxcomm.h"

/* ---------------------------------------------- */

#define BIT_A (1<<0)    /*          A           */
#define BIT_B (1<<1)    /*       -------        */
#define BIT_C (1<<2)    /*      |       |       */
#define BIT_D (1<<3)    /*    F |       |  B    */
#define BIT_E (1<<4)    /*       ---G---        */
#define BIT_F (1<<5)    /*      |       |  C    */
#define BIT_G (1<<6)    /*    E |       |	    */
#define BIT_P (1<<7)    /*       ---D---   P    */

/***********************************************************************************************************************
 *                                     数码管显示代码定义
 ***********************************************************************************************************************/
#if 0
#define CONVERSE 0xff^
#else
#define CONVERSE 0xff&
#endif
#define DATA_0      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F))
#define DATA_1      (CONVERSE(BIT_B|BIT_C))
#define DATA_2      (CONVERSE(BIT_A|BIT_B|BIT_D|BIT_E|BIT_G))
#define DATA_3      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_D|BIT_G))
#define DATA_4      (CONVERSE(BIT_B|BIT_C|BIT_F|BIT_G))
#define DATA_5      (CONVERSE(BIT_A|BIT_C|BIT_D|BIT_F|BIT_G))
#define DATA_6      (CONVERSE(BIT_A|BIT_C|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_7      (CONVERSE(BIT_A|BIT_B|BIT_C))
#define DATA_8      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_9      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_D|BIT_F|BIT_G))
#define DATA_A      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_E|BIT_F|BIT_G))
#define DATA_a      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_G))
#define DATA_b      (CONVERSE(BIT_C|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_C      (CONVERSE(BIT_A|BIT_D|BIT_E|BIT_F))
#define DATA_c      (CONVERSE(BIT_D|BIT_E|BIT_G))
#define DATA_d      (CONVERSE(BIT_B|BIT_C|BIT_D|BIT_E|BIT_G))
#define DATA_E      (CONVERSE(BIT_A|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_e      (CONVERSE(BIT_A|BIT_B|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_F      (CONVERSE(BIT_A|BIT_E|BIT_F|BIT_G))
#define DATA_H      (CONVERSE(BIT_B|BIT_C|BIT_E|BIT_F|BIT_G))
#define DATA_h      (CONVERSE(BIT_C|BIT_E|BIT_F|BIT_G))
#define DATA_I      (CONVERSE(BIT_E|BIT_F))
#define DATA_i      (CONVERSE(BIT_E))
#define DATA_J      (CONVERSE(BIT_B|BIT_C|BIT_D))
#define DATA_L      (CONVERSE(BIT_D|BIT_E|BIT_F))
#define DATA_N      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_E|BIT_F))
#define DATA_n      (CONVERSE(BIT_C|BIT_E|BIT_G))
#define DATA_O      (CONVERSE(BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F))
#define DATA_o      (CONVERSE(BIT_C|BIT_D|BIT_E|BIT_G))
#define DATA_P      (CONVERSE(BIT_A|BIT_B|BIT_E|BIT_F|BIT_G))
#define DATA_t      (CONVERSE(BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_U      (CONVERSE(BIT_B|BIT_C|BIT_D|BIT_E|BIT_F))
#define DATA_u      (CONVERSE(BIT_C|BIT_D|BIT_E))
#define DATA_DARK   (CONVERSE(0x00))


/* **************************************硬件相关*********************************************** */

/* ********************************************************************************************* */
// 设置系统参数命令

#define FD650_BIT_ENABLE	0x01		// 开启/关闭位
#define FD650_BIT_SLEEP		0x04		// 睡眠控制位
#define FD650_BIT_7SEG		0x08		// 7段控制位
#define FD650_BIT_INTENS1	0x10		// 1级亮度
#define FD650_BIT_INTENS2	0x20		// 2级亮度
#define FD650_BIT_INTENS3	0x30		// 3级亮度
#define FD650_BIT_INTENS4	0x40		// 4级亮度
#define FD650_BIT_INTENS5	0x50		// 5级亮度
#define FD650_BIT_INTENS6	0x60		// 6级亮度
#define FD650_BIT_INTENS7	0x70		// 7级亮度
#define FD650_BIT_INTENS8	0x00		// 8级亮度

#define FD650_SYSOFF	0x0400			// 关闭显示、关闭键盘
#define FD650_SYSON		( FD650_SYSOFF | FD650_BIT_ENABLE )	// 开启显示、键盘
#define FD650_SLEEPOFF	FD650_SYSOFF	// 关闭睡眠
#define FD650_SLEEPON	( FD650_SYSOFF | FD650_BIT_SLEEP )	// 开启睡眠
#define FD650_7SEG_ON	( FD650_SYSON | FD650_BIT_7SEG )	// 开启七段模式
#define FD650_8SEG_ON	( FD650_SYSON | 0x00 )	// 开启八段模式
#define FD650_SYSON_1	( FD650_SYSON | FD650_BIT_INTENS1 )	// 开启显示、键盘、1级亮度
//以此类推
#define FD650_SYSON_4	( FD650_SYSON | FD650_BIT_INTENS4 )	// 开启显示、键盘、4级亮度
//以此类推
#define FD650_SYSON_8	( FD650_SYSON | FD650_BIT_INTENS8 )	// 开启显示、键盘、8级亮度


// 加载字数据命令
#define FD650_DIG0		0x1400			// 数码管位0显示,需另加8位数据
#define FD650_DIG1		0x1500			// 数码管位1显示,需另加8位数据
#define FD650_DIG2		0x1600			// 数码管位2显示,需另加8位数据
#define FD650_DIG3		0x1700			// 数码管位3显示,需另加8位数据

#define FD650_DOT			0x0080			// 数码管小数点显示
#define FD650_COLON_LED		1

// 读取按键代码命令
#define FD650_GET_KEY	0x0700					// 获取按键,返回按键代码

#define FD650_STANDBY_LED 2

//6605s 7,8  6622 2,3
#if GXCHIP_TYPE == GXCHIP_TYPE_GX6605S
#define GPIO_PANEL_SDA	7
#define GPIO_PANEL_SCL	8
#elif GXCHIP_TYPE == GXCHIP_TYPE_GX3211
#define GPIO_PANEL_SDA	2
#define GPIO_PANEL_SCL	3
#else //需要确认
#define GPIO_PANEL_SDA	7
#define GPIO_PANEL_SCL	8
#endif

#define DELAY 	\
{\
	volatile  unsigned int temp = 55; \
	do{\
		temp--;\
	}while(temp);\
}

enum
{
	PANEL_LED0 = 0,
	PANEL_LED1,
	PANEL_LED2,
	PANEL_LED3,
	PANEL_LED_TOTAL,
};

typedef struct _led_bitmap
{
	unsigned char character;
	unsigned char bitmap;
} led_bitmap;

static unsigned int gpio_panel_sda;
static unsigned int gpio_panel_scl;
static unsigned char s_panel_led_str[PANEL_LED_TOTAL] = {0};
static unsigned char s_led_num[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static unsigned char panel_dot_ctrl_enable = 0;
static unsigned char panel_dot_ctrl_data = 0;
static unsigned char panel_brightness = 0;

const led_bitmap BCD_decode_tab[] =
{
	{'0', DATA_0}, {'1', DATA_1}, {'2', DATA_2}, {'3', DATA_3},
	{'4', DATA_4}, {'5', DATA_5}, {'6', DATA_6}, {'7', DATA_7},
	{'8', DATA_8}, {'9', DATA_9}, {'a', DATA_a}, {'A', DATA_A},
	{'b', DATA_b}, {'B', DATA_b}, {'c', DATA_c}, {'C', DATA_C},
	{'d', DATA_d}, {'D', DATA_d}, {'e', DATA_e}, {'E', DATA_E},
	{'f', DATA_F}, {'F', DATA_F}, {'n', DATA_n}, {'N', DATA_N},
	{'o', DATA_o}, {'O', DATA_O}, {'t', DATA_t}, {'P', DATA_P},
};//BCD码字映射

static unsigned char Led_Get_Code(char cTemp)
{
	unsigned char i, bitmap=0x00;
	unsigned char num = sizeof(BCD_decode_tab) / sizeof(led_bitmap);
	for(i=0; i<num; i++)
	{
		if(BCD_decode_tab[i].character == cTemp)
		{
			bitmap = BCD_decode_tab[i].bitmap;
			break;
		}
	}

	return bitmap;
}

void FD650_SCL_SET(void)//将SCL设置为高电平
{
	*(volatile unsigned int*)0x00305008 |= (1 << gpio_panel_scl);
}

void FD650_SCL_CLR(void)//将SCL设置为低电平
{
	*(volatile unsigned int*)0x0030500c |= (1 << gpio_panel_scl);
}

void FD650_SCL_D_OUT(void)// 设置SCL为输出方向,对于双向I/O需切换为输出
{
	*(volatile unsigned int*)0x00305000 |= (1<<gpio_panel_scl);
}

void FD650_SDA_SET(void)//将SDA设置为高电平
{
	*(volatile unsigned int*)0x00305008 |= (1 << gpio_panel_sda);
}

void FD650_SDA_CLR(void)//将SDA设置为低电平
{
	*(volatile unsigned int*)0x0030500c |= (1 << gpio_panel_sda);
}

int FD650_SDA_IN(void)//当SDA设为输入方向时，读取的电平值
{
	unsigned int value;
	value = (*(volatile  unsigned int*)0x00305004 >> gpio_panel_sda) & 1;
	return value;
}

void FD650_SDA_D_OUT(void)// 设置SDA为输出方向,对于双向I/O需切换为输出
{
	*(volatile unsigned int*)0x00305000 |= (1<<gpio_panel_sda);
}

void FD650_SDA_D_IN(void)// 设置SDA为输入方向,对于双向I/O需切换为输入
{
	*(volatile unsigned int*)0x00305000 &= (~(1<<gpio_panel_sda));
}

void _fd650_start( void )
{
	FD650_SDA_D_OUT();
	FD650_SDA_SET();
	FD650_SCL_D_OUT();
	FD650_SCL_SET();
	DELAY;
	FD650_SDA_CLR();
	DELAY;
	FD650_SCL_CLR();
}

void _fd650_stop(void)
{
	FD650_SDA_CLR();
	FD650_SDA_D_OUT();
	DELAY;
	FD650_SCL_SET();
	DELAY;
	FD650_SDA_SET();
	DELAY;
	FD650_SDA_D_IN();
}

void _fd650_write_byte(unsigned char dat)
{
	unsigned char i;
	FD650_SDA_D_OUT();
	for( i = 0; i != 8; i++ )
	{
		if( dat & 0x80 )
		{
			FD650_SDA_SET();
		}
		else
		{
			FD650_SDA_CLR();
		}
		DELAY;
		FD650_SCL_SET();
		dat <<= 1;
		DELAY;  // 可选延时
		FD650_SCL_CLR();
	}
	FD650_SDA_D_IN();
	FD650_SDA_SET();
	DELAY;
	FD650_SCL_SET();
	DELAY;
	FD650_SCL_CLR();
}

unsigned char  _fd650_read_byte(void)
{
	unsigned char dat,i;
	FD650_SDA_SET();
	FD650_SDA_D_IN();
	dat = 0;
	for( i = 0; i != 8; i++ )
	{
		DELAY;  // 可选延时
		FD650_SCL_SET();
		DELAY;  // 可选延时
		dat <<= 1;
		if( FD650_SDA_IN() ) dat++;
		FD650_SCL_CLR();
	}
	FD650_SDA_SET();
	DELAY;
	FD650_SCL_SET();
	DELAY;
	FD650_SCL_CLR();
	return dat;
}

void _fd650_write_cmd(unsigned short cmd)	//写命令
{
	_fd650_start();
	_fd650_write_byte(((unsigned char)(cmd>>7)&0x3E)|0x40);
	_fd650_write_byte((unsigned char)cmd);
	_fd650_stop();
	return;
}

void _fd650_show_time(unsigned int time, unsigned int count)
{
	unsigned char  i;
	unsigned char value;
	unsigned char data[PANEL_LED_TOTAL]={0x00, 0x00, 0x00, 0x00};
	unsigned short cmd[PANEL_LED_TOTAL]={FD650_DIG0, FD650_DIG1, FD650_DIG2, FD650_DIG3};

	if(time> 9999)
		time = 9999;

	for (i = 0; i < PANEL_LED_TOTAL; i++)
	{
		value = time%10;
		time /= 10;
		s_panel_led_str[PANEL_LED_TOTAL -i -1] = s_led_num[value];
	}

	_fd650_write_cmd(FD650_SYSON | panel_brightness);// 开启显示和键盘，8段显示方式

	//发显示数据
	for(i=0; i < PANEL_LED_TOTAL; i++)
	{
		data[i] = Led_Get_Code(s_panel_led_str[i]);
		if ((panel_dot_ctrl_enable >> i) & 1)
			data[i] |= FD650_DOT * ((panel_dot_ctrl_data >> i) & 1);
		else {
			if(((i == FD650_COLON_LED) && (count%2 ==0)))
				data[i] |= FD650_DOT;
		}
		_fd650_write_cmd(cmd[i] | data[i]);
	}
}

void _fd650_show_on(void)
{
	unsigned char  i;
	unsigned char data[PANEL_LED_TOTAL]={0x00, 0x00, 0x00, 0x00};
	unsigned short cmd[PANEL_LED_TOTAL]={FD650_DIG0, FD650_DIG1, FD650_DIG2, FD650_DIG3};

	s_panel_led_str[PANEL_LED0] = 'b';
	s_panel_led_str[PANEL_LED1] = 'o';
	s_panel_led_str[PANEL_LED2] = 'o';
	s_panel_led_str[PANEL_LED3] = 't';

	_fd650_write_cmd(FD650_SYSON | panel_brightness);// 开启显示和键盘，8段显示方式

	//发显示数据
	for(i=0; i < PANEL_LED_TOTAL; i++)
	{
		data[i] = Led_Get_Code(s_panel_led_str[i]);
		_fd650_write_cmd(cmd[i] | data[i]);
	}

}

void _fd650_show_off(void)
{
	unsigned char  i;
	unsigned char data[PANEL_LED_TOTAL]={0x00, 0x00, 0x00, 0x00};
	unsigned short cmd[PANEL_LED_TOTAL]={FD650_DIG0, FD650_DIG1, FD650_DIG2, FD650_DIG3};

	s_panel_led_str[PANEL_LED0] = 0;
	s_panel_led_str[PANEL_LED1] = 'O';
	s_panel_led_str[PANEL_LED2] = 'F';
	s_panel_led_str[PANEL_LED3] = 'F';

	_fd650_write_cmd(FD650_SYSON | panel_brightness);// 开启显示和键盘，8段显示方式

	//发显示数据
	for(i=0; i < PANEL_LED_TOTAL; i++)
	{
		data[i] = Led_Get_Code(s_panel_led_str[i]);
		if ((panel_dot_ctrl_enable >> i) & 1)
			data[i] |= FD650_DOT * ((panel_dot_ctrl_data >> i) & 1);
		else {
			if(i == FD650_STANDBY_LED)
				data[i] |= FD650_DOT;
		}
		_fd650_write_cmd(cmd[i] | data[i]);
	}

}
unsigned char _fd650_read_key(void)		//读取按键
{
	unsigned char keycode = 0;

	_fd650_start();
	_fd650_write_byte((((((unsigned char)(FD650_GET_KEY>>7))&0x3E)|0x01)|0x40));
	keycode=_fd650_read_byte();
	_fd650_stop();
	if((keycode & 0x00000040) ==0)
		keycode = 0;

	return keycode;
}

void _fd650_mulpin_config(void)
{
	*(volatile unsigned int*)0x0030a13c|=  ((1 << gpio_panel_scl) |(1 << gpio_panel_sda));
}

void _fd650_mute_config(void)
{
#if 0
	*(volatile unsigned int*)0x0030a140 |= (1 << 5);
	*(volatile unsigned int*)0x00305000 |= (1 << 5);
	*(volatile unsigned int*)0x0030500c |= (1 << 5);
#endif
}

void _fd650_init(struct gx_parse_cmdline *pcmdline)
{
	if (pcmdline->panelio[0] == 0)
		gpio_panel_scl = GPIO_PANEL_SCL;
	else
		gpio_panel_scl = pcmdline->panelio[0];

	if (pcmdline->panelio[1] == 0)
		gpio_panel_sda = GPIO_PANEL_SDA;
	else
		gpio_panel_sda = pcmdline->panelio[1];

	panel_dot_ctrl_enable = (pcmdline->panel_dot_ctrl >> 4) & 0xF;
	panel_dot_ctrl_data   = pcmdline->panel_dot_ctrl & 0xF;
	panel_brightness      = (pcmdline->panel_brightness & 0x7) << 4;
}

