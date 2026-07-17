//#include "../../../../../sboot/include/autoconf.h"
#include "drvFP.h"
#include "gpio.h"
#include "parse_cmdline.h"
#include "config.h"

//led mapping
typedef struct _led_bitmap
{
    UINT8 character;
    UINT8 bitmap;
} led_bitmap;

enum {
	PANEL_LED0 = 0,
	PANEL_LED1,
	PANEL_LED2,
	PANEL_LED3,
	PANEL_LED_TOTAL
};

#define BIT_A (1<<0)    /*          A           */
#define BIT_B (1<<1)    /*       -------        */
#define BIT_C (1<<2)    /*      |       |       */
#define BIT_D (1<<3)    /*    F |       |  B    */
#define BIT_E (1<<4)    /*       ---G---        */
#define BIT_F (1<<5)    /*      |       |  C    */
#define BIT_G (1<<6)    /*    E |       |	    */
#define BIT_P (1<<7)    /*       ---D---   P    */

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
#define DATA_L      (CONVERSE(BIT_D|BIT_E|BIT_F))
#define DATA_DARK   (CONVERSE(0x00))
#define DATA_CON     (CONVERSE(BIT_G))

#define FD650_BIT_ENABLE  0x01    // 开启/关闭位
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


// 加载字数据命令
#define FD650_DIG0    0x1400      // 数码管位0显示,需另加8位数据
#define FD650_DIG1    0x1500      // 数码管位1显示,需另加8位数据
#define FD650_DIG2    0x1600      // 数码管位2显示,需另加8位数据
#define FD650_DIG3    0x1700      // 数码管位3显示,需另加8位数据

#define FD650_DOT     BIT_P      // 数码管小数点显示

// 读取按键代码命令
#define FD650_GET_KEY 0x0700          // 获取按键,返回按键代码

#define LEDMAPNUM 37

#define PAN_KEY_POWER   0x77    //key

#define CLK_DELAY_TIME       100

#define GX_GPIO_LEVEL_HIGH 1
#define GX_GPIO_LEVEL_LOW 0
#define GX_GPIO_IO_OUTPUT 1
#define GX_GPIO_IO_INPUT 0

char fp_clk_gpio = GPIO_FP_CLK;
char fp_data_gpio = GPIO_FP_DATA;
char panel_brightness = 0; //0 就是 8 级亮度

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


//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------
static const  led_bitmap bcd_decode_tab[LEDMAPNUM] =
{
    {'0', DATA_0}, {'1', DATA_1}, {'2', DATA_2}, {'3', DATA_3},
    {'4', DATA_4}, {'5', DATA_5}, {'6', DATA_6}, {'7', DATA_7},
    {'8', DATA_8}, {'9', DATA_9}, {'a', DATA_a}, {'A', DATA_A},
    {'b', DATA_b}, {'B', DATA_b}, {'c', DATA_c}, {'C', DATA_C},
    {'d', DATA_d}, {'D', DATA_d}, {'e', DATA_E}, {'E', DATA_E},
    {'f', DATA_F}, {'F', DATA_F}, {'o', DATA_o}, {'t', DATA_t},
    {'l', DATA_L}, {'L', DATA_L}, {'n', DATA_n}, {'p', DATA_P},
    {'P', DATA_P}, {'O', DATA_O}, {'u', DATA_u}, {'U', DATA_U},
    {'S', DATA_5}, {'s', DATA_5}, {'-', DATA_CON}, {' ', DATA_DARK},
    {'N', DATA_N}
};//BCD decode

static UINT8 gDispBuf[5] = {0};
static BOOL sec_flag;
unsigned char panel_dot_ctrl_enable = 0;
unsigned char panel_dot_ctrl_data = 0;

/*************************************************************************************/

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

BOOL MDrv_FrontPnl_EnableLED(BOOL  eEnableLED)
{
    if (eEnableLED)
    {
        gx_gpio_setlevel(fp_clk_gpio, GX_GPIO_LEVEL_HIGH);
    }
    else
    {
        gx_gpio_setlevel(fp_clk_gpio, GX_GPIO_LEVEL_LOW);
    }
    return TRUE;
}

static UINT8 MDrv_Led_Get_Code(UINT8 cTemp)
{
    UINT8 i, bitmap = 0x00;

    for (i = 0; i < LEDMAPNUM; i++)
    {
        if (bcd_decode_tab[i].character == cTemp)
        {
            bitmap = bcd_decode_tab[i].bitmap;
			//printf("MDrv_Led_Get_Code cTemp:%c,bitmap:0x%x\n", cTemp,bitmap);
            break;
        }
    }
    return bitmap;
}

void MDrv_FP_SetDigital(char ch, UINT8 index)
{
    if (index > PANEL_LED3)
        return;
    gDispBuf[index] = ch;
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

void MDrv_FP_DispUpdate(void)
{
    unsigned char i;
    unsigned char data[4] = {0x00, 0x00, 0x00, 0x00};
    unsigned short cmd[PANEL_LED_TOTAL]={FD650_DIG0, FD650_DIG1, FD650_DIG2, FD650_DIG3};

    MDrv_FD650_Write(FD650_SYSON | panel_brightness);
    for (i = 0; i < PANEL_LED_TOTAL; i++)
    {
        data[i] = MDrv_Led_Get_Code(gDispBuf[i]);
        if ((panel_dot_ctrl_enable >> i) & 1)
            data[i] |= FD650_DOT * ((panel_dot_ctrl_data >> i) & 1);
        else {
            if (i == PANEL_LED1 && sec_flag)
                data[i] |= FD650_DOT;
        }
        MDrv_FD650_Write(cmd[i] | (UINT8)data[i]);
    }
}



//-------------------------------------------------------------------------------------------------
/// Initialize 650
/// @return TRUE  - Success
///         FALSE - Failure
//-------------------------------------------------------------------------------------------------
void MDrv_FP_Init(void)
{

    char panel_clk = g_cmdline->panelio[0];
    char panel_dat = g_cmdline->panelio[1];

    panel_brightness = (((char )g_cmdline->panel_brightness) & 0x7) << 4;
    panel_dot_ctrl_enable = (g_cmdline->panel_dot_ctrl >> 4) & 0xF;
    panel_dot_ctrl_data = g_cmdline->panel_dot_ctrl & 0xF;

    if (panel_clk != panel_dat)
    {
        fp_clk_gpio = panel_clk;
        fp_data_gpio = panel_dat;
    }

    MDrv_FrontPnl_EnableLED(TRUE);
    MDrv_FD650_Write(FD650_SYSON | panel_brightness);
}

//static  UINT8 bColonEnble = 1;
//static UINT32      systimeCount = 0;
//static UINT32 u32RtcGetCounter = 0;


void MDrv_FP_EnableColon(UINT8 bEnble)
{
	if(bEnble)
	{
		sec_flag =TRUE;
	}
	else
	{
		sec_flag =FALSE;
	}
}

int MDrv_FP_ShowTime(void)
{
    UINT8       HourCounter;
    UINT8       MinuCounter;
    static char bColon_Count = 0;

    HourCounter = get_localtime_hour();
    MinuCounter = get_localtime_min();
    bColon_Count++;
    if(bColon_Count > 4)
    {
        if(bColon_Count == 8)
        {
            bColon_Count = 0;
        }
        MDrv_FP_EnableColon(0);
    }
    else
    {
        MDrv_FP_EnableColon(1);
    }

    MDrv_FP_SetDigital(0x30 + (HourCounter / 10), 0);
    MDrv_FP_SetDigital(0x30 + (HourCounter % 10), 1);
    MDrv_FP_SetDigital(0x30 + (MinuCounter / 10), 2);
    MDrv_FP_SetDigital(0x30 + (MinuCounter % 10), 3);
    MDrv_FP_DispUpdate();

    return 0;
}

int MDrv_FP_ShowOFF(void)
{
    MDrv_FP_EnableColon(0);

    MDrv_FP_SetDigital(0, 0);
    MDrv_FP_SetDigital('O', 1);
    MDrv_FP_SetDigital('F', 2);
    MDrv_FP_SetDigital('F', 3);
    MDrv_FP_DispUpdate();

    return 0;
}

UINT8 MDrv_FD650_Powerkey_Statu(UINT8 powerkey)
{
    UINT8 keyCode;
    static UINT8 u8GotoStandby = 0;
    static UINT8 systemRuntimeCount = 0;

    if (systemRuntimeCount <= 1)
    {
        systemRuntimeCount ++;
        return 0;
    }
    if (u8GotoStandby == 1)
        return 0;
    keyCode = MDrv_FP_GetKey();

    if (keyCode == powerkey)
    {
        u8GotoStandby = 1;
        return 1;
    }

    return 0;
}
