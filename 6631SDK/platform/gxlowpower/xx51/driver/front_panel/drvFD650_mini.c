//#include "../../../../../sboot/include/autoconf.h"
#include "drvFP.h"
#include "gpio.h"
#include "parse_cmdline.h"
// 读取按键代码命令
#define FD650_GET_KEY 0x0700          // 获取按键,返回按键代码

#define FD650_BIT_ENABLE  0x01    // 开启
#define FD650_BIT_SLEEP   0x04    // 睡眠控制位
#define FD650_BIT_7SEG    0x08    // 7段控制位
#define FD650_BIT_INTENS1 0x10    // 1级亮度
#define FD650_BIT_INTENS2 0x20    // 2级亮度
#define FD650_BIT_INTENS3 0x30    // 3级亮度
#define FD650_BIT_INTENS4 0x40    // 4级亮度
#define FD650_BIT_INTENS5 0x50    // 5级亮度
#define FD650_BIT_INTENS6 0x60    // 6级亮度
#define FD650_BIT_INTENS7 0x70    // 7级亮度
#define FD650_BIT_INTENS8 0x00    // 8级亮度

#define FD650_SYSOFF  0x0400      // 关闭显示、关闭键盘
#define FD650_SYSON ( FD650_SYSOFF | FD650_BIT_ENABLE ) // 开启显示、键盘
#define FD650_SLEEPOFF  FD650_SYSOFF  // 关闭睡眠
#define FD650_SLEEPON ( FD650_SYSOFF | FD650_BIT_SLEEP )  // 开启睡眠
#define FD650_7SEG_ON ( FD650_SYSON | FD650_BIT_7SEG )  // 开启七段模式
#define FD650_8SEG_ON ( FD650_SYSON | 0x00 )  // 开启八段模式
#define FD650_SYSON_1 ( FD650_SYSON | FD650_BIT_INTENS1 ) // 开启显示、键盘、1级亮度
//以此类推
#define FD650_SYSON_4 ( FD650_SYSON | FD650_BIT_INTENS4 ) // 开启显示、键盘、4级亮度
//以此类推
#define FD650_SYSON_8 ( FD650_SYSON | FD650_BIT_INTENS8 ) // 开启显示、键盘、8级亮度

//phy gpio                              mcu //cpu
#if defined (SIRIUS_LOWPOWER) || defined (VEGA_LOWPOWER)
#define GPIO_FP_DATA                    3   //3
#define GPIO_FP_CLK                     4   //4
#elif defined(TAURUS_LOWPOWER)
#define GPIO_FP_DATA                    2   //17
#define GPIO_FP_CLK                     3   //18
#elif defined(GEMINI_LOWPOWER)
#define GPIO_FP_DATA                    14   //14
#define GPIO_FP_CLK                     13   //13
#endif

#define PAN_KEY_POWER   0x47    //key

#define CLK_DELAY_TIME       100

#define GX_GPIO_LEVEL_HIGH 1
#define GX_GPIO_LEVEL_LOW 0
#define GX_GPIO_IO_OUTPUT 1
#define GX_GPIO_IO_INPUT 0

char fp_clk_gpio = GPIO_FP_CLK;
char fp_data_gpio = GPIO_FP_DATA;

#define  FD650_SCL_SET   \
	  gx_gpio_setlevel(fp_clk_gpio, GX_GPIO_LEVEL_HIGH);
#define  FD650_SCL_CLR  \
	  gx_gpio_setlevel(fp_clk_gpio, GX_GPIO_LEVEL_LOW);
#define FD650_SCL_D_OUT \
	  gx_gpio_setio(fp_clk_gpio, GX_GPIO_IO_OUTPUT);

#define  FD650_SDA_SET   \
      gx_gpio_setlevel(fp_data_gpio, GX_GPIO_LEVEL_HIGH);
#define  FD650_SDA_CLR    \
	  gx_gpio_setlevel(fp_data_gpio, GX_GPIO_LEVEL_LOW);
#define  FD650_SDA_IN    \
	  gx_gpio_getlevel(fp_data_gpio)
#define  FD650_SDA_D_OUT  \
	  gx_gpio_setio(fp_data_gpio, GX_GPIO_IO_OUTPUT);
#define  FD650_SDA_D_IN   \
	  gx_gpio_setio(fp_data_gpio, GX_GPIO_IO_INPUT);
#define  FD650_GPIO_INIT


/*************************************************************************************/
static int fd650_power_key_enable = 0;

static void _delaySomeNop(UINT16 cnt)
{
    while ((cnt--) > 0)
    {
        //FD650_CLK_DELAY;
        //asm __volatile__("nop");
       // asm __volatile__("nop");
        //asm __volatile__("nop");
        //asm __volatile__("nop");

    }
}

/****************************************************************
 *
 *   Function Name:FD650_Start
 *
 *   Description:
 *
 *   Parameter:
 *
 *   return：
****************************************************************/
void MDrv_FD650_Start(void)
{
    FD650_SDA_SET;
    FD650_SDA_D_OUT;
    FD650_SCL_SET;
    FD650_SCL_D_OUT;
    _delaySomeNop(15);
    FD650_SDA_CLR;
    _delaySomeNop(15);
    FD650_SCL_CLR;
}

/****************************************************************
 *
 *    Function Name:FD650_Stop
 *
 *    Description:
 *
 *    Parameter:
 *
 *    return：
****************************************************************/
void MDrv_FD650_Stop(void)
{
    FD650_SDA_CLR;
    FD650_SDA_D_OUT;
    _delaySomeNop(15);
    FD650_SCL_SET;
    _delaySomeNop(15);
    FD650_SDA_SET;
    _delaySomeNop(15);
    FD650_SDA_D_IN;
}

/****************************************************************
 *
 *    Function Name:FD650_WrByte
 *
 *    Description: Write One Byte data
 *
 *    Parameter: data
 *
 *    return：
****************************************************************/
void MDrv_FD650_WrByte(UINT8 dat)
{
    UINT8 i;
    FD650_SDA_D_OUT;
    for (i = 0; i != 8; i++)
    {
        if (dat & 0x80)
        {
            FD650_SDA_SET;
        }
        else
        {
            FD650_SDA_CLR;
        }
        _delaySomeNop(15);
        FD650_SCL_SET;
        dat <<= 1;
        _delaySomeNop(15);  // choose delay
        FD650_SCL_CLR;
    }
    FD650_SDA_D_IN;
    FD650_SDA_SET;
    _delaySomeNop(15);
    FD650_SCL_SET;
    _delaySomeNop(15);
    FD650_SCL_CLR;
}

/****************************************************************
 *
 *    Function Name:FD650_WrByte
 *
 *    Description: Read One Byte data
 *
 *    Parameter:
 *
 *    return：data
****************************************************************/
UINT8 MDrv_FD650_RdByte(void)
{
    UINT8 dat, i;
    FD650_SDA_SET;
    FD650_SDA_D_IN;
    dat = 0;
    for (i = 0; i != 8; i++)
    {
        _delaySomeNop(15);  //choose delay
        FD650_SCL_SET;
        _delaySomeNop(15);  // choose delay
        dat <<= 1;
        if (FD650_SDA_IN) dat++;
        _delaySomeNop(15); //
        FD650_SCL_CLR;
    }
    FD650_SDA_SET;
    _delaySomeNop(15);
    FD650_SCL_SET;
    _delaySomeNop(15);
    FD650_SCL_CLR;
    return dat;
}

/****************************************FD650 function**********************/

/****************************************************************
 *
 *    Function Name:FD650_Write
 *
 *    Description:by cmd
 *
 *    Parameter: cmd FD650.H
 *
 *    return：
****************************************************************/
void MDrv_FD650_Write(UINT16 cmd)   //write cmd
{
    MDrv_FD650_Start();
    MDrv_FD650_WrByte(((UINT8)(cmd >> 7) & 0x3E) | 0x40);
    MDrv_FD650_WrByte((UINT8)cmd);
    //_delaySomeNop(1000);
    MDrv_FD650_Stop();
    return;
}

/****************************************************************
 *
 *    Function Name:FD650_Read
 *
 *    Description:read key value
 *
 *    Parameter:
 *
 *    return：key value
****************************************************************/
UINT8 MDrv_FD650_Read(void)
{
    UINT8 keycode = 0;
    UINT32 value = 0;
    MDrv_FD650_Start();
    value = ((FD650_GET_KEY >> 7) & 0x3E) | 0x01 | 0x40 ;
    MDrv_FD650_WrByte((UINT8)value);
    keycode = MDrv_FD650_RdByte();
    //printf("keycode = 0X%X \n",keycode);
    _delaySomeNop(1000);
    MDrv_FD650_Stop();

    return keycode;
}

void MDrv_FD650_Init(void)
{
	char panel_clk = g_cmdline->panelio[0];
	char panel_dat = g_cmdline->panelio[1];

	if (panel_clk != panel_dat
		&& panel_clk != 0
		&& panel_dat != 0) {
		fp_clk_gpio = panel_clk;
		fp_data_gpio = panel_dat;
	}
	MDrv_FD650_Write(FD650_SYSON_1);
}

//-------------------------------------------------------------------------------------------------
/// 650Get Key
/// @return None
//-------------------------------------------------------------------------------------------------
UINT8 MDrv_FP_GetKey(void)
{
    UINT8 KeyValue = 0;
	KeyValue = MDrv_FD650_Read();
    return KeyValue;
}

//避免刚进入lowpower就重启
UINT8 MDrv_FD650_Powerkey_Statu(UINT8 powerkey)
{
    UINT8 ret = 0;
    if(fd650_power_key_enable)
    {
        if(MDrv_FP_GetKey() == powerkey)
            ret = 1;
    }
    else
    {
        if(MDrv_FP_GetKey() != powerkey)
            fd650_power_key_enable = 1;
    }
    return ret;
}
#if 0
   // 用于lowpower调试简易显示
#define FD650_BIT_ENABLE  0x01    // 开启/关闭位
#define FD650_BIT_SLEEP   0x04    // 睡眠控制位
#define FD650_BIT_7SEG    0x08    // 7段控制位
#define FD650_BIT_INTENS1 0x10    // 1级亮度
#define FD650_BIT_INTENS4 0x40    // 4级亮度
#define FD650_BIT_INTENS8 0x00    // 8级亮度

#define FD650_SYSOFF  0x0400      // 关闭显示、关闭键盘
#define FD650_SYSON ( FD650_SYSOFF | FD650_BIT_ENABLE ) // 开启显示、键盘
#define FD650_SLEEPOFF  FD650_SYSOFF  // 关闭睡眠
#define FD650_SLEEPON ( FD650_SYSOFF | FD650_BIT_SLEEP )  // 开启睡眠
#define FD650_7SEG_ON ( FD650_SYSON | FD650_BIT_7SEG )  // 开启七段模式
#define FD650_8SEG_ON ( FD650_SYSON | 0x00 )  // 开启八段模式
#define FD650_SYSON_1 ( FD650_SYSON | FD650_BIT_INTENS1 ) // 开启显示、键盘、1级亮度
#define FD650_SYSON_4 ( FD650_SYSON | FD650_BIT_INTENS4 ) // 开启显示、键盘、4级亮度
#define FD650_SYSON_8 ( FD650_SYSON | FD650_BIT_INTENS8 ) // 开启显示、键盘、8级亮度

// 加载字数据命令
#define FD650_DIG0    0x1400      // 数码管位0显示,需另加8位数据
#define FD650_DIG1    0x1500      // 数码管位1显示,需另加8位数据
#define FD650_DIG2    0x1600      // 数码管位2显示,需另加8位数据
#define FD650_DIG3    0x1700      // 数码管位3显示,需另加8位数据
#define FD650_DOT     0x0080      // 数码管小数点显示


typedef struct _led_bitmap
{
	unsigned char character;
	unsigned char bitmap;
} led_bitmap;

#define MAX_CHAR_NUM 10
static led_bitmap BCD_decode_tab[MAX_CHAR_NUM]=
{
    {'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F},
    {'4', 0x66}, {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07},
    {'8', 0x7F}, {'9', 0x6F} //BCD码字映射
};
/*{
    {'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F},
    {'4', 0x66}, {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07},
    {'8', 0x7F}, {'9', 0x6F}, {'a', 0x77}, {'A', 0x77},
    {'b', 0x7C}, {'B', 0x7C}, {'c', 0x58}, {'C', 0x39},
    {'d', 0x5E}, {'D', 0x5E}, {'e', 0x79}, {'E', 0x79},
    {'f', 0x71}, {'F', 0x71}, {'o', 0x5C}, {'t', 0x78},
    {'l', 0x30}, {'L', 0x38}, {'n', 0x37}, {'p', 0x73},
    {'P', 0x73}, {'O', 0x3F}, {'u', 0x1C}, {'U', 0x3E},
    {'S', 0x6D}, {'s', 0x6D}, {'-', 0x40}, {' ', 0x00},
    {'N', 0x37}
};//BCD decode*/


static unsigned char Led_Get_Code(unsigned char cTemp)
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

//Led_Show_650("1234")
void Led_Show_650(char *led_str)
{
	int i;
	unsigned char data[4]={0x00, 0x00, 0x00, 0x00};
	unsigned short cmd[4]={FD650_DIG0, FD650_DIG1, FD650_DIG2, FD650_DIG3};

    MDrv_FD650_Write(FD650_SYSON_8);// 开启显示和键盘，8段显示方式

	//发显示数据
	for(i=0; i < 4; i++)
	{
		data[i] = Led_Get_Code(led_str[i]);
        MDrv_FD650_Write(cmd[i] | data[i]);
	}
}
#endif
