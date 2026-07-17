#include "bsp_panel_ops.h"
#include "bsp_panel.h"
#include <linux/string.h>
#ifdef PANEL_FD650
//******************************************************************************
//********FD650*FD650*FD650*FD650*FD650*FD650*FD650*FD650*FD650*FD650***********
//******************************************************************************
//FD650资料：对于诸如 DVB 面板的“长线”传输应用,越高的通信频率,越不利于信号的完
//整性,因此建议通信频率在 100KHz 以下,修改我司软件驱动包中的 DELAY 宏,根据
//上位机主控制端时钟调整函数。
//DELAY 延时时序宏，控制 SCL 以及 SDA 的速率，建议在5uS 以上，
//#define DELAY               PANEL_DELAY(6)

// 定时器轮循状态时间间隔，单位ms
#define QUERY_TIME_MS       50
// 按键扫描去抖动时间保护，单位ms
#define PROTECT_TIMES       200

unsigned int s_panel_key = 0;
static unsigned char panel_lock_flag = 0;
static unsigned char panel_sec_flag = 0;
static unsigned char panel_standby_flag = 0;
static unsigned int panel_brigthness = FD650_SYSON_1;

static unsigned char s_panel_led_str[PANEL_LED_TOTAL] = {0};
//static unsigned char panel_old_string[PANEL_LED_TOTAL] = {0};
static unsigned char s_led_num[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static panel_parameter s_panel_parameter;
static PanelGpioMap s_panel_gpio={0};
#define PANEL_MULTI_KEY
#ifdef PANEL_MULTI_KEY
#define LONG_KEY_TIMEOUT   500
static unsigned int  CH1_key_counter=0;//CH-
static unsigned int  CH2_key_counter=0;//CH+
static unsigned int  MN_key_counter=0;//CH+

static unsigned int  long_key=0;//0 short_key  1 long_key
static unsigned int  start_flag=0;
static unsigned int  CH1_short_key=0;
static unsigned int  CH2_short_key=0;
static unsigned int  MN_short_key=0;

extern void create_key_timer(timer_func func, unsigned long ms);
extern void delete_key_timer(void);
#endif
/* ---------------------------------------------- */

static unsigned char BIT_A=(1<<0);    /*          A           */
static unsigned char BIT_B= (1<<1);    /*       -------        */
static unsigned char BIT_C= (1<<2);    /*      |       |       */
static unsigned char BIT_D= (1<<3);    /*    F |       |  B    */
static unsigned char BIT_E =(1<<4);    /*       ---G---        */
static unsigned char BIT_F =(1<<5);    /*      |       |  C    */
static unsigned char BIT_G= (1<<6);    /*    E |       |	    */
static unsigned char BIT_P= (1<<7);    /*       ---D---   P    */

// 加载字数据命令
unsigned short FD650_DIG0 = 0x1400;			// 数码管位0显示,需另加8位数据
unsigned short FD650_DIG1 = 0x1500;			// 数码管位1显示,需另加8位数据
unsigned short FD650_DIG2 = 0x1600;			// 数码管位2显示,需另加8位数据
unsigned short FD650_DIG3 = 0x1700;			// 数码管位3显示,需另加8位数据

#define FD650_DOT			BIT_P			// 数码管小数点显示


/***********************************************************************************************************************
*                                     数码管显示代码定义
***********************************************************************************************************************/
#define BCD_TABLE_MAX 100
typedef struct _led_bitmap
{
	unsigned char character;
	unsigned char bitmap;
} led_bitmap;
static led_bitmap BCD_decode_tab[BCD_TABLE_MAX];
static void ioctl_set_led_bit_value(void)
{
	BIT_A = (unsigned char)s_panel_parameter.led_bit[0];
	BIT_B = (unsigned char)s_panel_parameter.led_bit[1];
	BIT_C = (unsigned char)s_panel_parameter.led_bit[2];
	BIT_D = (unsigned char)s_panel_parameter.led_bit[3];
	BIT_E = (unsigned char)s_panel_parameter.led_bit[4];
	BIT_F = (unsigned char)s_panel_parameter.led_bit[5];
	BIT_G = (unsigned char)s_panel_parameter.led_bit[6];
	BIT_P = (unsigned char)s_panel_parameter.led_bit[7];

}
static void ioctl_set_digtal_value(void)
{
	FD650_DIG0 = (unsigned short)s_panel_parameter.digtal_value[0];
	FD650_DIG1 = (unsigned short)s_panel_parameter.digtal_value[1];
	FD650_DIG2 = (unsigned short)s_panel_parameter.digtal_value[2];
	FD650_DIG3 = (unsigned short)s_panel_parameter.digtal_value[3];
}

static void  led_set_decode_tab( void)
{
int num = 0;
#define DATA_0      (BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F)
#define DATA_1      ((BIT_B|BIT_C))
#define DATA_2      ((BIT_A|BIT_B|BIT_D|BIT_E|BIT_G))
#define DATA_3      ((BIT_A|BIT_B|BIT_C|BIT_D|BIT_G))
#define DATA_4      ((BIT_B|BIT_C|BIT_F|BIT_G))
#define DATA_5      ((BIT_A|BIT_C|BIT_D|BIT_F|BIT_G))
#define DATA_6      ((BIT_A|BIT_C|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_7      ((BIT_A|BIT_B|BIT_C))
#define DATA_8      ((BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_9      ((BIT_A|BIT_B|BIT_C|BIT_D|BIT_F|BIT_G))
#define DATA_A      ((BIT_A|BIT_B|BIT_C|BIT_E|BIT_F|BIT_G))
#define DATA_a      ((BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_G))
#define DATA_b      ((BIT_C|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_C      ((BIT_A|BIT_D|BIT_E|BIT_F))
#define DATA_c      ((BIT_D|BIT_E|BIT_G))
#define DATA_d      ((BIT_B|BIT_C|BIT_D|BIT_E|BIT_G))
#define DATA_E      ((BIT_A|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_e      ((BIT_A|BIT_B|BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_F      ((BIT_A|BIT_E|BIT_F|BIT_G))
#define DATA_H      ((BIT_B|BIT_C|BIT_E|BIT_F|BIT_G))
#define DATA_h      ((BIT_C|BIT_E|BIT_F|BIT_G))
#define DATA_I      ((BIT_E|BIT_F))
#define DATA_i      ((BIT_E))
#define DATA_J      ((BIT_B|BIT_C|BIT_D))
#define DATA_L      ((BIT_D|BIT_E|BIT_F))
#define DATA_N      ((BIT_A|BIT_B|BIT_C|BIT_E|BIT_F))
#define DATA_n      ((BIT_C|BIT_E|BIT_G))
#define DATA_O      ((BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F))
#define DATA_o      ((BIT_C|BIT_D|BIT_E|BIT_G))
#define DATA_P      ((BIT_A|BIT_B|BIT_E|BIT_F|BIT_G))
#define DATA_t      ((BIT_D|BIT_E|BIT_F|BIT_G))
#define DATA_U      ((BIT_B|BIT_C|BIT_D|BIT_E|BIT_F))
#define DATA_u      ((BIT_C|BIT_D|BIT_E))
#define DATA_L      ((BIT_D|BIT_E|BIT_F))
#define DATA_r      ((BIT_E|BIT_G))
#define DATA_DARK   ((0x00))
#define CONVERSE  0xff^




	 led_bitmap BCD_decode_tab_1[] =
	{
		{'0', DATA_0}, {'1', DATA_1}, {'2', DATA_2}, {'3', DATA_3},
		{'4', DATA_4}, {'5', DATA_5}, {'6', DATA_6}, {'7', DATA_7},
		{'8', DATA_8}, {'9', DATA_9}, {'a', DATA_a}, {'A', DATA_A},
		{'b', DATA_b}, {'B', DATA_b}, {'c', DATA_c}, {'C', DATA_C},
		{'d', DATA_d}, {'D', DATA_d}, {'e', DATA_e}, {'E', DATA_E},
		{'f', DATA_F}, {'F', DATA_F}, {'n', DATA_n}, {'N', DATA_N},
		{'o', DATA_o}, {'O', DATA_O}, {'t', DATA_t}, {'P', DATA_P},
		{'U',DATA_U},{'r',DATA_r},
	};//BCD码字映射
	led_bitmap BCD_decode_tab_0[] =
	{
		{'0',(CONVERSE DATA_0)}, {'1',(CONVERSE DATA_1)}, {'2',(CONVERSE DATA_2)}, {'3',(CONVERSE DATA_3)},
		{'4',(CONVERSE DATA_4)}, {'5',(CONVERSE DATA_5)}, {'6',(CONVERSE DATA_6)}, {'7',(CONVERSE DATA_7)},
		{'8',(CONVERSE DATA_8)}, {'9',(CONVERSE DATA_9)}, {'a',(CONVERSE DATA_a)}, {'A',(CONVERSE DATA_A)},
		{'b',(CONVERSE DATA_b)}, {'B',(CONVERSE DATA_b)}, {'c',(CONVERSE DATA_c)}, {'C',(CONVERSE DATA_C)},
		{'d',(CONVERSE DATA_d)}, {'D',(CONVERSE DATA_d)}, {'e',(CONVERSE DATA_e)}, {'E',(CONVERSE DATA_E)},
		{'f',(CONVERSE DATA_F)}, {'F',(CONVERSE DATA_F)}, {'n',(CONVERSE DATA_n)}, {'N',(CONVERSE DATA_N)},
		{'o',(CONVERSE DATA_o)}, {'O',(CONVERSE DATA_O)}, {'t',(CONVERSE DATA_t)}, {'P',(CONVERSE DATA_P)},
		{'U',(CONVERSE DATA_U)},{'r',(CONVERSE DATA_r)},
	};//BCD码字映射
	memset((void *)BCD_decode_tab,0,BCD_TABLE_MAX*sizeof(led_bitmap));

	if(0 == s_panel_parameter.led_converse)
	{
		num = sizeof(BCD_decode_tab_1) / sizeof(led_bitmap);
		num = (num > BCD_TABLE_MAX) ? BCD_TABLE_MAX : num;

		memcpy((void *)BCD_decode_tab,BCD_decode_tab_1,num*sizeof(led_bitmap));
	}
	else
	{
		num = sizeof(BCD_decode_tab_0) / sizeof(led_bitmap);
		num = (num > BCD_TABLE_MAX) ? BCD_TABLE_MAX : num;
		memcpy((void *)BCD_decode_tab,BCD_decode_tab_0,num*sizeof(led_bitmap));
	}

}


void FD650_SCL_SET(void)//将SCL设置为高电平
{
        gx_gpio_setlevel(s_panel_gpio.panel_gpio_scl, GX_GPIO_HIGH);
}
void FD650_SCL_CLR(void)//将SCL设置为低电平
{
        gx_gpio_setlevel(s_panel_gpio.panel_gpio_scl, GX_GPIO_LOW);
}
void FD650_SCL_D_OUT(void)// 设置SCL为输出方向,对于双向I/O需切换为输出
{
        gx_gpio_setio(s_panel_gpio.panel_gpio_scl, GX_GPIO_OUTPUT);
}
void FD650_SDA_SET(void)//将SDA设置为高电平
{
        gx_gpio_setio(s_panel_gpio.panel_gpio_sda, GX_GPIO_INPUT);
}
void FD650_SDA_CLR(void)//将SDA设置为低电平
{
        gx_gpio_setlevel(s_panel_gpio.panel_gpio_sda, GX_GPIO_LOW);
        gx_gpio_setio(s_panel_gpio.panel_gpio_sda, GX_GPIO_OUTPUT);
}
int FD650_SDA_IN(void)//当SDA设为输入方向时，读取的电平值
{
        return gx_gpio_getlevel(s_panel_gpio.panel_gpio_sda);
}
void FD650_SDA_D_OUT(void)// 设置SDA为输出方向,对于双向I/O需切换为输出
{
        gx_gpio_setio(s_panel_gpio.panel_gpio_sda, GX_GPIO_INPUT);
        gx_gpio_setlevel(s_panel_gpio.panel_gpio_sda, GX_GPIO_LOW);
}
void FD650_SDA_D_IN(void)// 设置SDA为输入方向,对于双向I/O需切换为输入
{
        gx_gpio_setio(s_panel_gpio.panel_gpio_sda, GX_GPIO_INPUT);
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
    // 根据厂家建议修改。modify 20170531
    FD650_SDA_SET();//释放总线
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
    for( i = 0; i != 8; i++ )
    {
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
    return;
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

void Led_Show_650(char *led_str, unsigned char sec_flag, unsigned char lock_flag, unsigned char standby_flag)
{
	int i;
	unsigned char data[PANEL_LED_TOTAL]={0x00, 0x00, 0x00, 0x00};
	unsigned short cmd[PANEL_LED_TOTAL]={FD650_DIG0, FD650_DIG1, FD650_DIG2, FD650_DIG3};

	if(led_str == 0)
	{
		return;
	}

	FD650_Write_Cmd(panel_brigthness);// 开启显示和键盘，8段显示方式

	//发显示数据
	for(i=0; i < PANEL_LED_TOTAL; i++)
	{
		data[i] = Led_Get_Code(led_str[i]);
		if(((i == FD650_SEC_LED) && (sec_flag != 0))
		    || ((i == s_panel_parameter.lock_led) && (lock_flag != 0))
		    || ((i == s_panel_parameter.standby_led) && (standby_flag != 0))
            )
		{
		    data[i] |= FD650_DOT;
		}

		FD650_Write_Cmd(cmd[i] | data[i]);
	}
}

/******************************************************************************/
static int show_number_count =0;
static int ioctl_show_num(unsigned int num)
{
    unsigned char  i;
    unsigned char value;
    unsigned int int_value = 10000;
	panel_sec_flag = 0;
	show_number_count = 4;

    if(num >= 10000)
        return -1;

    for (i = 0; i < PANEL_LED_TOTAL; i++)
    {
        value = (num - (num/int_value)*int_value)/(int_value/10);
        int_value /= 10;
        s_panel_led_str[i] = s_led_num[value];
        //panel_old_string[i] = s_panel_led_str[i];
    }

    //Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
    return 0;
}
static int ioctl_show_time(unsigned int num)
{
    unsigned char  i;
    unsigned char value;
    unsigned int int_value = 10000;
    if(num >= 10000)
        return -1;

	if(show_number_count > 0)
	{
		show_number_count -- ;
	}
	else
	{
		for (i = 0; i < PANEL_LED_TOTAL; i++)
		{
			value = (num - (num/int_value)*int_value)/(int_value/10);
			int_value /= 10;
			s_panel_led_str[i] = s_led_num[value];
		}
		panel_sec_flag = !panel_sec_flag;

		//Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
	}
	return 0;
}

static int ioctl_show_str(PanelString str)
{
	panel_sec_flag = 0;

    switch (str)
    {
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
	    case PANEL_LED_UPGRADE:
            s_panel_led_str[PANEL_LED0] = 'U';
            s_panel_led_str[PANEL_LED1] = 'P';
            s_panel_led_str[PANEL_LED2] = '9';
            s_panel_led_str[PANEL_LED3] = 'r';
            break;
        default:
            break;
    }

	//Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);

    return 0;
}

static int ioctl_show_quality(unsigned int quality)
{
    unsigned char i;
    unsigned char value;
    unsigned int int_value = 100;

    if(quality > 100)
        return -1;
	panel_sec_flag = 0;

    if(quality == 100)
    {
        s_panel_led_str[PANEL_LED0] = 'P';
        s_panel_led_str[PANEL_LED1] = s_led_num[1];
        s_panel_led_str[PANEL_LED2] = s_led_num[0];
        s_panel_led_str[PANEL_LED3] = s_led_num[0];
        //Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
        return 0;
    }

    s_panel_led_str[PANEL_LED0] = ' ';
    s_panel_led_str[PANEL_LED1] = 'P';
    //panel_old_string[PANEL_LED0] = s_panel_led_str[PANEL_LED0];
    //panel_old_string[PANEL_LED1] = s_panel_led_str[PANEL_LED1];

    for (i = PANEL_LED2; i < PANEL_LED_TOTAL; i++)
    {
        value = (quality - (quality/int_value)*int_value)/(int_value/10);
        int_value /= 10;
        s_panel_led_str[i] = s_led_num[value];
        //panel_old_string[i] = s_panel_led_str[i];
    }

	//Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
    return 0;
}

static int ioctl_show_lock(unsigned int lock)
{
    panel_lock_flag = lock;
    //Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
    return 0;
}

static int ioctl_show_standby(unsigned int standby)
{
    panel_standby_flag = standby;
    //Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
    return 0;
}

static int ioctl_show_brigthness(unsigned int brigthness)
{
	switch(brigthness)
	{
		case PANEL_LED_BRIGTHNESS_LOW:
			panel_brigthness = FD650_SYSON_1;
		break;
		case PANEL_LED_BRIGTHNESS_MID:
			panel_brigthness = FD650_SYSON_4;
		break;
		case PANEL_LED_BRIGTHNESS_HIG:
			panel_brigthness = FD650_SYSON_8;
		break;
		default:
			panel_brigthness = FD650_SYSON_8;
		break;
	}
	//printf("ioctl_show_brigthness brigthness = %d panel_brigthness =%d ",brigthness,panel_brigthness);
	return 0;
}

static int ioctl_read_key(unsigned int *key)
{
    *key = s_panel_key;
    return 0;
}

static int ioctl_set_gpiomap(void *param)
{
    memset(&s_panel_gpio,0,sizeof(PanelGpioMap));
    memcpy(&s_panel_gpio,(PanelGpioMap*)param,sizeof(PanelGpioMap));
    return 0;
}
#ifdef PANEL_MULTI_KEY

unsigned int ck_running = 0;
static void  check_long_key(unsigned long data)
{
    if (((CH1_key_counter != 1)&&(CH2_key_counter == 0)&&(MN_key_counter == 0))
        ||((CH2_key_counter != 1)&& (CH1_key_counter == 0)&&(MN_key_counter == 0))
        ||((MN_key_counter != 1)&& (CH1_key_counter == 0)&& (CH2_key_counter == 0))
        )
    {
        start_flag = 1;
    }
    if ( CH1_key_counter > 2 )
    {
        long_key = 1;
    }
    //////////////////////////////////////////////////////
    if( CH2_key_counter > 2)
    {
        long_key = 1;
    }
    if ( MN_key_counter > 2 )
    {
        long_key = 1;
    }
}
static unsigned char long_key_press(unsigned char value)
{
    unsigned char ret_value = 0;
    static unsigned char pre_keyvalue = 0;
    if((value == s_panel_parameter.scan_code[PANEL_KEYMAP_UP])&&(s_panel_parameter.scan_code[PANEL_KEYMAP_UP]!=0))
    {
         CH1_key_counter++;
         CH1_short_key=1;
         ret_value=PANEL_LEY_NULL;
         if(CH1_key_counter==1)
         {
             create_key_timer(check_long_key, LONG_KEY_TIMEOUT);
         }
         if(start_flag==1)
         {
             delete_key_timer();
             if(long_key==1)//long_key
             {
                 ret_value=s_panel_parameter.scan_code[PANEL_KEYMAP_RIGHT];
               // printf("send==========bbbbbbbbb:%d\n",ret_value);
             }
         }
    }
    else if((value == s_panel_parameter.scan_code[PANEL_KEYMAP_DOWN])&&(s_panel_parameter.scan_code[PANEL_KEYMAP_DOWN]!=0))
    {
         CH2_key_counter++;
         CH2_short_key=1;
         ret_value=PANEL_LEY_NULL;
         if(CH2_key_counter==1)
         {
            create_key_timer(check_long_key, LONG_KEY_TIMEOUT);
         }
         if(start_flag==1)
         {
             delete_key_timer();
             if(long_key==1)//long_key
             {
                 ret_value = s_panel_parameter.scan_code[PANEL_KEYMAP_LEFT];
                // printf("send==========aaaaaaaaaaaa:%d\n",ret_value);
             }
         }
    }
    else if((value == s_panel_parameter.scan_code[PANEL_KEYMAP_MENU])&&(s_panel_parameter.scan_code[PANEL_KEYMAP_MENU]!=0))
    {
         MN_key_counter++;
         MN_short_key=1;
         ret_value=PANEL_LEY_NULL;
         if(MN_key_counter==1)
         {
            create_key_timer(check_long_key, LONG_KEY_TIMEOUT);
         }
         if(start_flag==1)
         {
             delete_key_timer();
             if(long_key==1)//long_key
             {
                 ret_value = s_panel_parameter.scan_code[PANEL_KEYMAP_OK];
                // printf("send==========cccccccccccccc:%d\n",ret_value);
             }
         }
    }
    else
    {
        //printf("no key no key \n");
        if(CH1_short_key==1)
        {
            if(pre_keyvalue == 0)
            {
                ret_value = s_panel_parameter.scan_code[PANEL_KEYMAP_UP];
            }
            else
            {
                ret_value = 0;
            }
            CH1_short_key=0;
            start_flag=0;
            delete_key_timer();
        }
        else if(CH2_short_key==1)
        {
            if(pre_keyvalue == 0)
            {
                ret_value = s_panel_parameter.scan_code[PANEL_KEYMAP_DOWN];
            }
            else
            {
                ret_value = 0;
            }
            CH2_short_key=0;
            start_flag=0;
            delete_key_timer();
        }
        else if(MN_short_key==1)
        {
            if(pre_keyvalue == 0)
            {
                ret_value = s_panel_parameter.scan_code[PANEL_KEYMAP_MENU];
            }
            else
            {
                ret_value = 0;
            }
            MN_short_key=0;
            start_flag=0;
            delete_key_timer();
        }
        else
        {
            start_flag=0;
            long_key=0;
            delete_key_timer();

            ret_value=value;

        }
        CH1_key_counter=0;
        CH2_key_counter=0;
        MN_key_counter=0;
    }
    pre_keyvalue = ret_value;
    return ret_value;
}
#endif
// 修改了之前panel的实现方式，去抖动部分时间可调整，详见宏定义“PROTECT_TIMES”。
// 同时调整了去抖动的代码实现流程，之前的版本放到read部分实现，实现的方式比较暴
// 力，直接while死循环，有两个弊端：1）时间会根据不同的芯片主频而发生变化；2）会
// 卡住调用该接口的线程。目前的将该部分处理，转移到面板自己的定时器里面，不用死
// 循环。
// 特别说明：之前的版本，如果长按，会多次调用wakeup接口，目前的做法是一次wakeup
// 一次select，对应操作。
#define PROTECT_COUNT   (PROTECT_TIMES/QUERY_TIME_MS)
static void  _panel_scan_callback(unsigned long data)
{
    unsigned char value = 0;
    static unsigned char key_pre = 0;
    static int  protect_count = 0;

    value = FD650_Read_Key();

    //问题：经常出现在产品老化实验中,例如开始显示正常,长时间后显示乱码或黑屏
    //分析: 关键是设计电路及 PCB 时要考虑抗干扰,要考虑电流走向,FD650 可以
    //直接驱动显示,所以在电源和地线 中流过的电流较大,如果 GND 走线不佳,会导
    //致整个系统的 GND 电压不统一,从而产生较大的干扰。如果处理不当,普通的单片
    //机受到干扰可能会导致程序死机,而 FD650 是纯硬件电路,是由相当于很多 个
    //74TTL 逻辑芯片组成的电路,所以即使受到干扰也很容易恢复:只要对 FD650 芯片
    //重新发送命令就可以 随时重新工作,命令包括设置系统参数、设置显示参数、设
    //置闪烁控制、加载数据等。
    //对于显示时间要求较长的全天候应用,建议在应用程序里每隔一段时间,对
    //FD650 的系统参数,显示参数和闪烁控制进行刷新,及时恢复外界干扰导致的参数
    //错误。刷新参数不影响当前的显示。
	Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
#ifdef PANEL_MULTI_KEY
    if(s_panel_parameter.longkey_enable == 1)
    {
        value = long_key_press(value);
    }
#endif

    if(PANEL_LEY_NULL != value)
    {
        if(key_pre == value)
        {
            protect_count++;
        }
        else
        {
            key_pre = value;
            protect_count = 0;
        }

        if((PROTECT_COUNT == protect_count) || (0 == protect_count))
        {
            //printf("\n\033[34mc=%d, key_pre =%d v=%d\033[0m\n",protect_count, key_pre ,value);
            s_panel_key = value;

            poll_wakeup();
            protect_count = 0;
        }
    }
    else
    {
        key_pre = 0;
        protect_count = 0;
        s_panel_key = 0;
    }
}

static int ioctl_set_keymap(void *param)
{
    memset(&s_panel_parameter,0,sizeof(panel_parameter));
    memcpy(&s_panel_parameter,(panel_parameter*)param,sizeof(panel_parameter));

    //app_panel_init(&s_panel_parameter);
	ioctl_set_led_bit_value();
	ioctl_set_digtal_value();
	led_set_decode_tab();
	ioctl_show_str(PANEL_LED_ON);
    return 0;
}
static int fd650_init(void)
{
	memset(&s_panel_parameter,0,sizeof(panel_parameter));
    return 0;
}

// FD650面板定时器的轮循周期调整，之前5ms的时间主要是沿用之前1642面板的实现，用
// 于4段数码管显示保持人眼不敏感添加的最大时间。但实际FD650有保持余辉的功能，所
// 以只是为了键值检测，不需要那么高的频率，目前调整为50ms执行一次，具体可调，见
// 宏定义"QUERY_TIME_MS"。
static int fd650_open(void)
{
    create_bsp_timer(_panel_scan_callback, QUERY_TIME_MS/*5*/);
    return 0;
}

static int fd650_close(void)
{
    delete_bsp_timer();
#ifdef PANEL_MULTI_KEY
     delete_key_timer();
#endif
    s_panel_key = 0;
    ioctl_show_str(PANEL_LED_OFF);
	Led_Show_650((char*)s_panel_led_str, panel_sec_flag, panel_lock_flag, panel_standby_flag);
    return 0;
}

// 配合“_panel_scan_callback（）”修改，转移去抖动的代码实现。同时怎加0键值状态的
// 保护。
static int fd650_read(unsigned int *key_val)
{
    if(0 == s_panel_key)
    {
        *key_val = 0;
        return -1;
    }

	printk("=============%d===========\n",s_panel_key);
    //printk("s_panel_key = %d\n", s_panel_key);
    if(s_panel_key ==  s_panel_parameter.scan_code[PANEL_KEYMAP_RIGHT])
        *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_RIGHT];
    else if(s_panel_key ==  s_panel_parameter.scan_code[PANEL_KEYMAP_LEFT])
        *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_LEFT];
    else if(s_panel_key ==  s_panel_parameter.scan_code[PANEL_KEYMAP_MENU])
        *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_MENU];
	else if(s_panel_key ==  s_panel_parameter.scan_code[PANEL_KEYMAP_OK])
        *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_OK];
    else if(s_panel_key ==  s_panel_parameter.scan_code[PANEL_KEYMAP_DOWN])
        *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_DOWN];
     else if(s_panel_key ==  s_panel_parameter.scan_code[PANEL_KEYMAP_UP])
        *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_UP];
    else if(s_panel_key ==   s_panel_parameter.scan_code[PANEL_KEYMAP_PW])
        *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_PW];
    else if(s_panel_key ==   s_panel_parameter.scan_code[PANEL_KEYMAP_EXIT])
	   *key_val = s_panel_parameter.virtual_key[PANEL_KEYMAP_EXIT];
    else
        *key_val = 0;
    return 0;
}

static BspPanelIoctl fd650_ioctl =
{
    .io_show_str = ioctl_show_str,
    .io_show_num = ioctl_show_num,
    .io_show_quality = ioctl_show_quality,
    .io_show_lock = ioctl_show_lock,
    .io_show_standby = ioctl_show_standby,
    .io_show_time = ioctl_show_time,
    .io_read_key = ioctl_read_key,
    .io_set_gpiomap = ioctl_set_gpiomap,
    .io_set_keymap = ioctl_set_keymap,
    .io_mcu_lowpower = 0,
    .io_show_brigthness = ioctl_show_brigthness,
};

BspPanelOps g_BspPanel  =
{
    .panel_init = fd650_init,
    .panel_open = fd650_open,
    .panel_close = fd650_close,
    .panel_read = fd650_read,
    .panel_ioctl = &fd650_ioctl,
};

#endif

