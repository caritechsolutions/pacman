#include "panel_fd650.h"
#include "board-common.h"
#include "gpio.h"

#ifndef NULL
#define NULL 0
#endif

/* led */
enum {
	PANEL_LED0 = 0,
	PANEL_LED1,
	PANEL_LED2,
	PANEL_LED3,
	PANEL_LED_TOTAL,
};

/* led string */
typedef enum {
	PANEL_LED_ON = 0,
	PANEL_LED_OFF,
	PANEL_LED_BOOT,
	PANEL_LED_DARK,
}PanelString;



#define DELAY {\
	volatile unsigned int temp = 150; \
	do{\
		temp--;\
	}while(temp);\
}

typedef struct _led_bitmap {
	unsigned char character;
	unsigned char bitmap;
} led_bitmap;

// The port for panel, physical GPIO number
static unsigned int PANLE_FD650_SDA	= 12;
static unsigned int PANLE_FD650_SCL	= 13;

//******************************************************************************
//********FD650*FD650*FD650*FD650*FD650*FD650*FD650*FD650*FD650*FD650***********
//******************************************************************************
//static unsigned char panel_lock_flag = 0, panel_standby_flag = 0;
//static unsigned char panel_sec_flag = 0;
static unsigned char s_panel_led_str[PANEL_LED_TOTAL] = {0};
//static unsigned char s_panel_bak_str[PANEL_LED_TOTAL] = {0};
static unsigned char s_led_num[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

const led_bitmap BCD_decode_tab[] = {
	{'0', DATA_0}, {'1', DATA_1}, {'2', DATA_2}, {'3', DATA_3},
	{'4', DATA_4}, {'5', DATA_5}, {'6', DATA_6}, {'7', DATA_7},
	{'8', DATA_8}, {'9', DATA_9}, {'a', DATA_a}, {'A', DATA_A},
	{'b', DATA_b}, {'B', DATA_b}, {'c', DATA_c}, {'C', DATA_C},
	{'d', DATA_d}, {'D', DATA_d}, {'e', DATA_e}, {'E', DATA_E},
	{'f', DATA_F}, {'F', DATA_F}, {'n', DATA_n}, {'N', DATA_N},
	{'o', DATA_o}, {'O', DATA_O}, {'t', DATA_t}, {'P', DATA_P},
};//BCD码字映射

void FD650_SCL_SET(void)//将SCL设置为高电平
{
	gx_gpio_setlevel(PANLE_FD650_SCL,GX_GPIO_HIGH);
}

void FD650_SCL_CLR(void)//将SCL设置为低电平
{
	gx_gpio_setlevel(PANLE_FD650_SCL,GX_GPIO_LOW);
}

void FD650_SCL_D_OUT(void)// 设置SCL为输出方向,对于双向I/O需切换为输出
{
	gx_gpio_setio(PANLE_FD650_SCL,GX_GPIO_OUTPUT);
}

void FD650_SDA_SET(void)//将SDA设置为高电平
{
    gx_gpio_setio(PANLE_FD650_SDA, GX_GPIO_INPUT);
	DELAY;
}

void FD650_SDA_CLR(void)//将SDA设置为低电平
{
    gx_gpio_setio(PANLE_FD650_SDA,GX_GPIO_OUTPUT);
}

int FD650_SDA_IN(void)//当SDA设为输入方向时，读取的电平值
{
	return gx_gpio_getlevel(PANLE_FD650_SDA);
}

void FD650_SDA_D_OUT(void)// 设置SDA为输出方向,对于双向I/O需切换为输出
{
    gx_gpio_setio(PANLE_FD650_SDA, GX_GPIO_INPUT);
    gx_gpio_setlevel(PANLE_FD650_SDA, GX_GPIO_LOW);
}

void FD650_SDA_D_IN(void)// 设置SDA为输入方向,对于双向I/O需切换为输入
{
	gx_gpio_setio(PANLE_FD650_SDA,GX_GPIO_INPUT);
}

void FD650_Start( void )
{
	FD650_SDA_SET();

	FD650_SCL_D_OUT();
	FD650_SCL_SET();
	DELAY;
	FD650_SDA_CLR();
	DELAY;
	FD650_SCL_CLR();
}

void FD650_Stop( void )
{
	FD650_SDA_CLR();
	DELAY;
	FD650_SCL_SET();
	DELAY;
	FD650_SDA_SET();
	DELAY;
}

void FD650_WrByte(unsigned char dat)
{
	unsigned char i;

	for( i = 0; i != 8; i++ ) {
		if( dat & 0x80 )
			FD650_SDA_SET();
		else
			FD650_SDA_CLR();
		DELAY;
		FD650_SCL_SET();
		dat <<= 1;
		DELAY;  // 可选延时
		FD650_SCL_CLR();
	}
	FD650_SDA_SET();
	DELAY;
	FD650_SCL_SET();
	DELAY;
    if(FD650_SDA_IN())
        FD650_SDA_SET();
    else
        FD650_SDA_CLR();    // surpress glitch after falling edge of SCL
	FD650_SCL_CLR();
}

unsigned char  FD650_RdByte(void)
{
	unsigned char dat,i;

	FD650_SDA_SET();
	dat = 0;
	for( i = 0; i != 8; i++ ) {
		DELAY;  // 可选延时
		FD650_SCL_SET();
		DELAY;  // 可选延时
		dat <<= 1;
		if( FD650_SDA_IN() )
			dat++;
		FD650_SCL_CLR();
	}
	FD650_SDA_SET();
	DELAY;
	FD650_SCL_SET();
	DELAY;
	FD650_SCL_CLR();
	return dat;
}

/****************************************FD650操作函数*********************************************/

void FD650_Write_Cmd(unsigned short cmd)	//写命令
{
	FD650_Start();
	FD650_WrByte(((unsigned char)(cmd>>7)&0x3E)|0x40);
	FD650_WrByte((unsigned char)cmd);
	FD650_Stop();
}

unsigned char FD650_Read_Key(void)		//读取按键
{
	unsigned char keycode = 0;

	FD650_Start();
	FD650_WrByte((((((unsigned char)(FD650_GET_KEY>>7))&0x3E)|0x01)|0x40));
    DELAY;
	keycode=FD650_RdByte();
	FD650_Stop();
	if((keycode&0x00000040) ==0)
		keycode = 0;
	if(keycode == 0xFF)
		keycode = 0;

	return keycode;
}

/******************************************************************************/
static unsigned char Led_Get_Code(char cTemp)
{
	unsigned char i, bitmap=0x00;
	unsigned char num = sizeof(BCD_decode_tab) / sizeof(led_bitmap);
	for(i=0; i<num; i++) {
		if(BCD_decode_tab[i].character == cTemp) {
			bitmap = BCD_decode_tab[i].bitmap;
			break;
		}
	}

	return bitmap;
}

void Led_Show_650(char *led_str, unsigned char sec_flag, unsigned char lock_flag, unsigned char standby_flag)
{
	int i;
	unsigned char data[PANEL_LED_TOTAL]={0x00, 0x00, 0x00, 0x00};
	unsigned short cmd[PANEL_LED_TOTAL]={FD650_DIG0, FD650_DIG1, FD650_DIG2, FD650_DIG3};

	if(led_str == 0)
		return;

	FD650_Write_Cmd(FD650_SYSON_8);// 开启显示和键盘，8段显示方式
	//发显示数据
	for(i=0; i < PANEL_LED_TOTAL; i++) {
		data[i] = Led_Get_Code(led_str[i]);
		if(((i == FD650_SEC_LED) && (sec_flag != 0))
				|| ((i == FD650_LOCK_LED) && (lock_flag != 0))
				|| ((i == FD650_STANDBY_LED) && (standby_flag != 0)))
			data[i] |= FD650_DOT;

		FD650_Write_Cmd(cmd[i] | data[i]);
	}
}

/******************************************************************************/
static int ioctl_show_num(unsigned int num)
{
	unsigned char  i;
	unsigned char value;
	unsigned int int_value = 10000;

	if(num >= 10000)
		return -1;

	for (i = 0; i < PANEL_LED_TOTAL; i++) {
		value = (num - (num/int_value)*int_value)/(int_value/10);
		int_value /= 10;
		s_panel_led_str[i] = s_led_num[value];
		//panel_old_string[i] = s_panel_led_str[i];
	}

	Led_Show_650((char*)s_panel_led_str, 0, 0, 0);
	return 0;
}

static int ioctl_show_str(PanelString str)
{
	switch (str) {
		case PANEL_LED_ON:
			s_panel_led_str[PANEL_LED0] = 0;
			s_panel_led_str[PANEL_LED1] = 'O';
			s_panel_led_str[PANEL_LED2] = 'N';
			s_panel_led_str[PANEL_LED3] = 0;
			break;

		case PANEL_LED_OFF:
			s_panel_led_str[PANEL_LED0] = 0;
			s_panel_led_str[PANEL_LED1] = 'O';
			s_panel_led_str[PANEL_LED2] = 'F';
			s_panel_led_str[PANEL_LED3] = 'F';
			break;

		case PANEL_LED_BOOT:
			s_panel_led_str[PANEL_LED0] = 'b';
			s_panel_led_str[PANEL_LED1] = 'o';
			s_panel_led_str[PANEL_LED2] = 'o';
			s_panel_led_str[PANEL_LED3] = 't';
			break;

		case PANEL_LED_DARK:
			s_panel_led_str[PANEL_LED0] = 0;
			s_panel_led_str[PANEL_LED1] = 0;
			s_panel_led_str[PANEL_LED2] = 0;
			s_panel_led_str[PANEL_LED3] = 0;
			break;

		default:
			break;
	}

	Led_Show_650((char*)s_panel_led_str, 0, 0, 0);

	return 0;
}

static int ioctl_show_quality(unsigned int quality)
{
	unsigned char i;
	unsigned char value;
	unsigned int int_value = 100;

	if(quality >= 100)
		return -1;

	s_panel_led_str[PANEL_LED0] = ' ';
	s_panel_led_str[PANEL_LED1] = 'P';

	for (i = PANEL_LED2; i < PANEL_LED_TOTAL; i++) {
		value = (quality - (quality/int_value)*int_value)/(int_value/10);
		int_value /= 10;
		s_panel_led_str[i] = s_led_num[value];
		//panel_old_string[i] = s_panel_led_str[i];
	}
	Led_Show_650((char*)s_panel_led_str, 0, 0, 0);
	return 0;
}

void loader_panel_display(void)
{
	ioctl_show_str(PANEL_LED_BOOT);
}

void loader_panel_init(unsigned int sda, unsigned int clk)
{
	PANLE_FD650_SDA = sda;
	PANLE_FD650_SCL = clk;
    FD650_SDA_D_OUT();
}


