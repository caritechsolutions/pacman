/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :     bsp_panel.c
* Author    : 	hongg
* Project   :
* Type      :	
******************************************************************************
* Purpose   :	设备初始化入
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2008.8.28	      hongg	         creation
*****************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include "stdio.h"
#include "string.h"
#include "gxota_panel.h"
#include "panel.h"
#define GXPIO_BIT_HIGHT_LEVEL 1
#define GXPIO_BIT_LOW_LEVEL    0
#define MANUFACTURE_JINYA
#define GXOTA_PANEL_CLK_GPIO      23
#define GXOTA_PANEL_DATA_GPIO     22
#define GXOTA_PANEL_STANDBY_GPIO  21
#define GXOTA_PANEL_LOCK_GPIO     24
extern uint32_t s_panel_key;
//ct1642
typedef struct GXPANEL_CT1642_Config_s
{
	uint8_t m_chPanelName[MAX_PANEL_NAME_LENGTH];

	uint8_t m_chLedBit[8];

	uint8_t m_chLedCnt;	//the number of panel led
	uint8_t m_chCurLenCnt;//cur scan led

	uint8_t m_chShowMode;//panelmode_t

	uint8_t m_chDataLight;	//段选点亮电平状态

	uint8_t m_chChipSelectLight;//位选点亮电平状态

	uint8_t m_chLockLight;
	uint8_t m_chGpioLockEn;//如果为GPIO口控制 lock 灯，该成员设为1

	uint8_t m_chClkPin;	//the clk pin of hc164
	uint8_t m_chDataPin;	//the data pin of hc164

	uint8_t m_chKeyPin[2];	//the kd pin
	uint32_t m_nKeyValue[8];

	uint8_t m_chLockPin;	//the lock pin
	uint8_t m_chLedPin[4];//the led pin
	uint32_t m_nLedAddr[5];//the led pin

	uint32_t m_nPortBaseAddr[PORT_BASE_ADDRESS_END];//the base address of port

	GXPANEL_PUBLIC_Config_t m_tPanelPublicConfig;
}GXPANEL_CT1642_Config_t;

GXPANEL_CT1642_Config_t g_PanelCT1642Config;


static uint8_t gs_chLedData[LED_DATA_DARK+1]={
		(BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F),		//0
		(BIT_B|BIT_C),								//1
		(BIT_A|BIT_B|BIT_D|BIT_E|BIT_G),			//2
		(BIT_A|BIT_B|BIT_C|BIT_D|BIT_G),			//3
		(BIT_B|BIT_C|BIT_F|BIT_G),					//4
		(BIT_A|BIT_C|BIT_D|BIT_F|BIT_G),			//5
		(BIT_A|BIT_C|BIT_D|BIT_E|BIT_F|BIT_G),		//6
		(BIT_A|BIT_B|BIT_C),						//7
		(BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F|BIT_G),//8
		(BIT_A|BIT_B|BIT_C|BIT_D|BIT_F|BIT_G),		//9
		(BIT_A|BIT_D|BIT_E|BIT_F|BIT_G),			//E
		(BIT_A|BIT_E|BIT_F|BIT_G),					//F
		(BIT_A|BIT_B|BIT_C|BIT_E|BIT_F),			//N
		(BIT_A|BIT_B|BIT_E|BIT_F|BIT_G),			//P
	(BIT_C|BIT_D|BIT_E|BIT_F|BIT_G),            //t
	(BIT_D|BIT_E|BIT_F),                        //L
	(BIT_C|BIT_D|BIT_E|BIT_F|BIT_G),            //b
	(BIT_C|BIT_D|BIT_E|BIT_G),                  //o
	0x00,                                       //HIDE
        (BIT_A|BIT_D|BIT_E|BIT_F),                  //C
	(BIT_B|BIT_C|BIT_D|BIT_E|BIT_F),            //U
	0x00					    //dark
	};/*0,1,2,3,4,5,6,7,8,9,E,F,N,P,T,dark*/


static uint8_t gs_chLedValue[4]= {0,};
static uint8_t s_Lockenable;


static void gx_stb_panel_ct1642_send_keymsg(uint32_t nKeyIndex);
static void gx_stb_panel_ct1642_set_led_string(unsigned char* str);
static void gx_stb_panel_ct1642_lock(void);
static void gx_stb_panel_ct1642_set_led_signal_value(uint32_t nLedValue);
static void gx_stb_panel_ct1642_set_led_value(uint32_t nLedValue);
static void gx_stb_panel_ct1642_unlock(void);

static void gx_stb_panel_ct1642_set_led_dark(void)
{
	gs_chLedValue[0] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[1] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[2] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[3] = gs_chLedData[LED_DATA_DARK];
}
static void gx_stb_panel_ct1642_init_led_data(void)
{
	gs_chLedData[LED_DATA_0] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]);
	gs_chLedData[LED_DATA_1] = (g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]);
	gs_chLedData[LED_DATA_2] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_3] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_4] = (g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_5] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_6] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_7] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]);
	gs_chLedData[LED_DATA_8] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_9] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_E] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_F] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_N] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]);
	gs_chLedData[LED_DATA_P] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_B]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
    gs_chLedData[LED_DATA_C] = (g_PanelCT1642Config.m_chLedBit[BIT_A]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]);
	gs_chLedData[LED_DATA_t] = (g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
    gs_chLedData[LED_DATA_U] = (g_PanelCT1642Config.m_chLedBit[BIT_B]
                    | g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]);
	gs_chLedData[LED_DATA_L] = (g_PanelCT1642Config.m_chLedBit[BIT_G]);
	gs_chLedData[LED_DATA_b] = (g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_F]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);

	/*wangjian add on 20141215*/
	gs_chLedData[LED_DATA_o] = (g_PanelCT1642Config.m_chLedBit[BIT_C]
					| g_PanelCT1642Config.m_chLedBit[BIT_D]
					| g_PanelCT1642Config.m_chLedBit[BIT_E]
					| g_PanelCT1642Config.m_chLedBit[BIT_G]);
}

static void gx_stb_panel_ct1642_show_mode(panelmode_t PanelMode)
{
	g_PanelCT1642Config.m_chShowMode = PanelMode;
}

static void gx_stb_panel_ct1642_enable(void)
{
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
									, GXPIO_BIT_LOW_LEVEL);
    g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
									, GXPIO_BIT_HIGHT_LEVEL);
    g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
									, GXPIO_BIT_HIGHT_LEVEL);
}

static void gx_stb_panel_ct1642_disable(void)
{
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
									, GXPIO_BIT_LOW_LEVEL);
    g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
									, GXPIO_BIT_LOW_LEVEL);
    g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
									, GXPIO_BIT_HIGHT_LEVEL);
}


static void gx_stb_panel_ct1642_send_data_to_led(uint8_t chData, uint8_t chPosition)//串并转换
{	
	uint8_t i;
	uint16_t data =0;

	switch(chPosition)
	{	case 0:
			if(s_Lockenable)
			{
				data = g_PanelCT1642Config.m_nLedAddr[0] + (chData |(1<<5));
			}
			else
			{
				data = g_PanelCT1642Config.m_nLedAddr[0] + chData;
			}
			break;
		case 1:
			data = g_PanelCT1642Config.m_nLedAddr[1] + chData;
			break;
		case 2:
			data = g_PanelCT1642Config.m_nLedAddr[2] + chData;
			break;
		case 3:
			data = g_PanelCT1642Config.m_nLedAddr[3] + chData;
			break;		
		case 4:
			data = g_PanelCT1642Config.m_nLedAddr[4] + chData;
			break;
	}
	
	for( i = 0; i < 8; i++ )		//8位移位寄存器
	{
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
										, GXPIO_BIT_LOW_LEVEL);
 		
		//从74HC164最低位每次移入一个值（0/1）,数字的高位在前
		if(data & 0x8000)//最低位 移入一个1
		{
			g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
										, GXPIO_BIT_HIGHT_LEVEL);
		}
		else//最低位 移入一个0
		{
			g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
										, GXPIO_BIT_LOW_LEVEL);
		}
		
		data <<= 1;
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
										, GXPIO_BIT_HIGHT_LEVEL);
	}

	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
									, GXPIO_BIT_LOW_LEVEL);
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
									, GXPIO_BIT_HIGHT_LEVEL);
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
									, GXPIO_BIT_HIGHT_LEVEL);

	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
									, GXPIO_BIT_LOW_LEVEL);
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
									, GXPIO_BIT_HIGHT_LEVEL);
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
									, GXPIO_BIT_HIGHT_LEVEL);
#ifdef MANUFACTURE_JINYA
	for( i = 0; i < 8; i++ )		//8位移位寄存器
	{
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
										, GXPIO_BIT_LOW_LEVEL);
 		
		//从74HC164最低位每次移入一个值（0/1）,数字的高位在前
		if(data & 0x8000)//最低位 移入一个1
		{
			g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
										, GXPIO_BIT_HIGHT_LEVEL);
		}
		else//最低位 移入一个0
		{
			g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
										, GXPIO_BIT_LOW_LEVEL);
		}
		
		data <<= 1;
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chClkPin
										, GXPIO_BIT_HIGHT_LEVEL);
	}
#endif 
	gx_stb_panel_ct1642_enable();//?
	gx_stb_panel_ct1642_disable();//?
}

 
static void gx_stb_panel_ct1642_led_scan(void)
{   
	gx_stb_panel_ct1642_send_data_to_led(gs_chLedValue[g_PanelCT1642Config.m_chCurLenCnt]
										, g_PanelCT1642Config.m_chCurLenCnt);
}


static void gx_stb_panel_ct1642_key_scan(void)
{	
	uint64_t nKeyLevel;
	static uint8_t KeyCount = 0;
	static uint8_t s_chPress = 0;
	uint32_t i,chKeyCnt;

	if(g_PanelCT1642Config.m_chKeyPin[0] == g_PanelCT1642Config.m_chKeyPin[1])
		chKeyCnt = 2;
	else 
		chKeyCnt = 1;

	//gx_stb_panel_ct1642_send_data_to_led(0xff, 4);		
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelGetGpioLevelFun(g_PanelCT1642Config.m_chKeyPin[0], &nKeyLevel);
	g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelGetGpioLevelFun(g_PanelCT1642Config.m_chKeyPin[1], &nKeyLevel);

	if(nKeyLevel && (1 == s_chPress))
	{	
		KeyCount++;
		if(KeyCount == 0x10)
		{	KeyCount = 0x90;
		}
		else if(KeyCount == 0xaF)
		{	KeyCount = 0x90;
			s_chPress = 0;	
		}
		return;
	}
	else if(nKeyLevel && (0 == s_chPress))
	{
	//	panel_printf("key pressed\n");//?
	}
	else if((!nKeyLevel)&& (1 == s_chPress))
	{	
		KeyCount = 0;
		s_chPress = 0;	
		return;
	}
	else if((!nKeyLevel)&& (0 == s_chPress))
	{	
		return;
	}	

	//gx_stb_panel_ct1642_send_data_to_led(0x00, 4);	//yubing add 100707	

	for(i = 0; i < 4*chKeyCnt; i++)
	{
		//gx_stb_panel_ct1642_send_data_to_led(1<<i, 4);
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelGetGpioLevelFun(g_PanelCT1642Config.m_chKeyPin[0], &nKeyLevel);
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelGetGpioLevelFun(g_PanelCT1642Config.m_chKeyPin[1], &nKeyLevel);
		
		if ((nKeyLevel ) && (s_chPress==0))//如果有按键
		{
			//panel_printf("\n##############[ct1642=0=] i:%d chKeyCnt:%d\n", i, chKeyCnt);
			gx_stb_panel_ct1642_send_keymsg(i);
			s_chPress = 1;
		}

		//nKeyLevel &= ~(((uint64_t)1) << g_PanelCT1642Config.m_chKeyPin[0]);
		/*if ((nKeyLevel  ) && (s_chPress==0))//如果有按键
		{
			//panel_printf("\n##############[ct1642=1=] i:%d chKeyCnt:%d\n", i, chKeyCnt);
			gx_stb_panel_ct1642_send_keymsg(1*chKeyCnt + i);
			s_chPress = 1;
		}*/
		//g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chDataPin
		//								, GXPIO_BIT_HIGHT_LEVEL);
	}	
}

static void gx_stb_panel_ct1642_scan_manager(void)
{
	g_PanelCT1642Config.m_chCurLenCnt ++;

	if (g_PanelCT1642Config.m_chCurLenCnt >= g_PanelCT1642Config.m_chLedCnt)
		g_PanelCT1642Config.m_chCurLenCnt = 0; 

	gx_stb_panel_ct1642_key_scan();	
	gx_stb_panel_ct1642_led_scan(); 
}



static void gx_stb_panel_ct1642_set_led_value(uint32_t nLedValue)
{
	uint32_t i;
	
	gs_chLedValue[0] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[1] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[2] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[3] = gs_chLedData[LED_DATA_DARK];
	
	if(nLedValue > 9999)
		nLedValue = 9999;

	for(i = 0; i < g_PanelCT1642Config.m_chLedCnt; i++)
	{
		gs_chLedValue[i] = (nLedValue)%10;
		nLedValue = (nLedValue - gs_chLedValue[i])/10;
	}
		
	for(i = 0; i < g_PanelCT1642Config.m_chLedCnt; i++)
	{
		gs_chLedValue[i] = gs_chLedData[gs_chLedValue[i]];
	}

}

static void gx_stb_panel_ct1642_set_led_signal_value(uint32_t nLedValue)
{
	uint32_t i;

	gs_chLedValue[0] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[1] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[2] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[3] = gs_chLedData[LED_DATA_DARK];
	
	if(nLedValue > 9999)
		nLedValue = 9999;

	for(i = 0; i < g_PanelCT1642Config.m_chLedCnt; i++)
	{
		gs_chLedValue[i] = (nLedValue)%10;
		nLedValue = (nLedValue - gs_chLedValue[i])/10;
	}
		
	for(i = 0; i < g_PanelCT1642Config.m_chLedCnt - 1; i++)
	{
		gs_chLedValue[i] = gs_chLedData[gs_chLedValue[i]];
	}

	gs_chLedValue[g_PanelCT1642Config.m_chLedCnt-1] = gs_chLedData[LED_DATA_P];
}


static void gx_stb_panel_ct1642_set_led_string(unsigned char* str)
{
	unsigned char LedStr = *str;
	gs_chLedValue[0] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[1] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[2] = gs_chLedData[LED_DATA_DARK];
	gs_chLedValue[3] = gs_chLedData[LED_DATA_DARK];

	LedStr = 7; 
	switch(LedStr)
	{
		case GXLED_BOOT:
			if(3 == g_PanelCT1642Config.m_chLedCnt)
			{
				gs_chLedValue[0] = gs_chLedData[LED_DATA_t];
				gs_chLedValue[1] = gs_chLedData[LED_DATA_o];
				gs_chLedValue[2] = gs_chLedData[LED_DATA_b];
			}
			else if(4 == g_PanelCT1642Config.m_chLedCnt)
			{
				gs_chLedValue[0] = gs_chLedData[LED_DATA_t];
				gs_chLedValue[1] = gs_chLedData[LED_DATA_o];
				gs_chLedValue[2] = gs_chLedData[LED_DATA_o];
				gs_chLedValue[3] = gs_chLedData[LED_DATA_b];
			}				
			break;
		case GXLED_INIT:
			if(3 == g_PanelCT1642Config.m_chLedCnt)
			{
				gs_chLedValue[0] = gs_chLedData[LED_DATA_t];
				gs_chLedValue[1] = gs_chLedData[LED_DATA_N];
				gs_chLedValue[2] = gs_chLedData[LED_DATA_1];
			}
			else if(4 == g_PanelCT1642Config.m_chLedCnt)
			{
				gs_chLedValue[0] = gs_chLedData[LED_DATA_t];
				gs_chLedValue[1] = gs_chLedData[LED_DATA_1];
				gs_chLedValue[2] = gs_chLedData[LED_DATA_N];
				gs_chLedValue[3] = gs_chLedData[LED_DATA_1];
			}
			break;
		case GXLED_ON:
			gs_chLedValue[0] = gs_chLedData[LED_DATA_N];
			gs_chLedValue[1] = gs_chLedData[LED_DATA_0];
			break;
		case GXLED_OFF:
			gs_chLedValue[0] = gs_chLedData[LED_DATA_F];
			gs_chLedValue[1] = gs_chLedData[LED_DATA_F];
			gs_chLedValue[2] = gs_chLedData[LED_DATA_0];
			break;
		case GXLED_E:
			gs_chLedValue[0] = gs_chLedData[LED_DATA_E];
			break;
		case GXLED_P:
			gs_chLedValue[0] = gs_chLedData[LED_DATA_P];
			break;
		case GXLED_USB:
			gs_chLedValue[0] = gs_chLedData[LED_DATA_b];
			gs_chLedValue[1] = gs_chLedData[LED_DATA_5];
			gs_chLedValue[2] = gs_chLedData[LED_DATA_U];
            		break;
		case GXLED_SEAR:
			if(3 == g_PanelCT1642Config.m_chLedCnt)
			{
				gs_chLedValue[0] = gs_chLedData[LED_DATA_L];
				gs_chLedValue[1] = gs_chLedData[LED_DATA_L];
				gs_chLedValue[2] = gs_chLedData[LED_DATA_L];
			}
			else if(4 == g_PanelCT1642Config.m_chLedCnt)
			{
				gs_chLedValue[0] = gs_chLedData[LED_DATA_L];
				gs_chLedValue[1] = gs_chLedData[LED_DATA_L];
				gs_chLedValue[2] = gs_chLedData[LED_DATA_L];
				gs_chLedValue[3] = gs_chLedData[LED_DATA_L];
			}
			break;
		default:
			break;
	}
}


static void gx_stb_panel_ct1642_lock(void)
{
	if(g_PanelCT1642Config.m_chGpioLockEn)
	{
#ifdef MANUFACTURE_JINYA
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chLockPin
										, GXPIO_BIT_LOW_LEVEL);
#else
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chLockPin
										, GXPIO_BIT_HIGHT_LEVEL);
#endif
	}
	else
	{
		
		s_Lockenable = 1;
	}
}

static void gx_stb_panel_ct1642_unlock(void)
{
	if(g_PanelCT1642Config.m_chGpioLockEn)
	{
#ifdef MANUFACTURE_JINYA
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chLockPin
										, GXPIO_BIT_HIGHT_LEVEL);
#else
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelSetGpioLevelFun(g_PanelCT1642Config.m_chLockPin
										, GXPIO_BIT_LOW_LEVEL);
#endif

	}
	else
	{
		s_Lockenable = 0;
	}
}

static void gx_stb_panel_ct1642_power_off(void)
{
	gx_stb_panel_ct1642_set_led_dark();
	gx_stb_panel_ct1642_unlock();
	gx_stb_panel_ct1642_scan_manager();	
}

static void gx_stb_panel_ct1642_send_keymsg(uint32_t nKeyIndex)
{
//	panel_printf("+++++++++ key index %d\n", nKeyIndex);
	if((nKeyIndex!=0xff)&&(nKeyIndex<8))
		{
			s_panel_key = g_PanelCT1642Config.m_nKeyValue[nKeyIndex];
			//poll_wakeup();

	}
	//		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelDealKey(nKeyIndex);

	
//	panel_printf("+++++++++++key index %d\n",nKeyIndex);
}

static void gx_stb_panel_ct1642_init_pin(void)
{
	//GXPIO_InitParams_t  init_params;
	//GXPIO_OpenParams_t 	open_params;
	
	uint64_t nCfgPin = (((uint64_t)1) << g_PanelCT1642Config.m_chClkPin)
					| (((uint64_t)1) << g_PanelCT1642Config.m_chDataPin)
					| (((uint64_t)1) << g_PanelCT1642Config.m_chKeyPin[0])
					| (((uint64_t)1) << g_PanelCT1642Config.m_chKeyPin[1])
					| (((uint64_t)1) << g_PanelCT1642Config.m_chLockPin);


	gx_stb_panel_ct1642_init_led_data();
	
	(g_PanelPublicConfig.m_PanelCfgMultiplexFun)(nCfgPin);

	if (g_PanelCT1642Config.m_nPortBaseAddr[0] != 0
		&& g_PanelCT1642Config.m_nPortBaseAddr[1] == 0)
	{
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelInitGpioFun(nCfgPin);
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelOpenGpioFun(g_PanelCT1642Config.m_chClkPin, GPIO_OUTPUT);
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelOpenGpioFun(g_PanelCT1642Config.m_chDataPin, GPIO_OUTPUT);
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelOpenGpioFun(g_PanelCT1642Config.m_chKeyPin[0], GPIO_INPUT);
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelOpenGpioFun(g_PanelCT1642Config.m_chKeyPin[1], GPIO_INPUT);
		#if 0
		init_params.BaseAddress = g_PanelCT1642Config.m_nPortBaseAddr[0]; 
		init_params.IntNumber = 0;
		init_params.IntSource = 0;
		init_params.IntEn = 0;

	
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelInitGpioFun("FrontPanel", &init_params);
		
		open_params.ReservedBits = (1 << g_PanelCT1642Config.m_chClkPin) 
								 | (1 << g_PanelCT1642Config.m_chDataPin) 
								 | (1 << g_PanelCT1642Config.m_chKeyPin[0]) 
								 | (1 << g_PanelCT1642Config.m_chKeyPin[1]); 
		
		open_params.BitCfg[g_PanelCT1642Config.m_chClkPin].Direction = GXPIO_BIT_OUTPUT;
		open_params.BitCfg[g_PanelCT1642Config.m_chClkPin].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chClkPin].Level = GXPIO_BIT_HIGHT_LEVEL;

		open_params.BitCfg[g_PanelCT1642Config.m_chDataPin].Direction = GXPIO_BIT_OUTPUT;
		open_params.BitCfg[g_PanelCT1642Config.m_chDataPin].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chDataPin].Level = GXPIO_BIT_HIGHT_LEVEL;

		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[0]].Direction = GXPIO_BIT_INPUT;//Note:INput here
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[0]].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[0]].Level = GXPIO_BIT_HIGHT_LEVEL;

		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[1]].Direction = GXPIO_BIT_INPUT;//Note:INput here
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[1]].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[1]].Level = GXPIO_BIT_HIGHT_LEVEL;
		
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelOpenGpioFun("FrontPanel", &open_params, &g_PanelPublicConfig.m_nGpioHandle);
		#endif
	}
	else if (g_PanelCT1642Config.m_nPortBaseAddr[0] == 0
		&& g_PanelCT1642Config.m_nPortBaseAddr[1] != 0)
	{
		#if 0
		init_params.BaseAddress = g_PanelCT1642Config.m_nPortBaseAddr[1]; 
		init_params.IntNumber = 0;
		init_params.IntSource = 0;
		init_params.IntEn = 0;

	
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelInitGpioFun("FrontPanel", &init_params);
		
		open_params.ReservedBits = (1 << (g_PanelCT1642Config.m_chClkPin - 32)) 
								 | (1 << (g_PanelCT1642Config.m_chDataPin - 32)) 
								 | (1 << (g_PanelCT1642Config.m_chKeyPin[0] - 32)) 
								 | (1 << (g_PanelCT1642Config.m_chKeyPin[1] - 32)); 
		
		open_params.BitCfg[g_PanelCT1642Config.m_chClkPin - 32].Direction = GXPIO_BIT_OUTPUT;
		open_params.BitCfg[g_PanelCT1642Config.m_chClkPin - 32].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chClkPin - 32].Level = GXPIO_BIT_HIGHT_LEVEL;

		open_params.BitCfg[g_PanelCT1642Config.m_chDataPin - 32].Direction = GXPIO_BIT_OUTPUT;
		open_params.BitCfg[g_PanelCT1642Config.m_chDataPin - 32].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chDataPin - 32].Level = GXPIO_BIT_HIGHT_LEVEL;

		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[0] - 32].Direction = GXPIO_BIT_INPUT;//Note:INput here
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[0] - 32].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[0] - 32].Level = GXPIO_BIT_HIGHT_LEVEL;

		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[1] - 32].Direction = GXPIO_BIT_INPUT;//Note:INput here
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[1] - 32].Pole = GXPIO_BIT_POSITIVE;
		open_params.BitCfg[g_PanelCT1642Config.m_chKeyPin[1] - 32].Level = GXPIO_BIT_HIGHT_LEVEL;
		
		g_PanelCT1642Config.m_tPanelPublicConfig.m_PanelOpenGpioFun("FrontPanel", &open_params, &g_PanelPublicConfig.m_nGpioHandle);
		#endif
	}
	else
	{
	//	panel_printf("[gx_stb_panel_ct1642_init_pin] panel gpio cfg error\n");
	}

	//gx_stb_panel_ct1642_set_led_value(0000);
}

static void gx_stb_panel_ct1642_isr(void)
{

		gx_stb_panel_ct1642_scan_manager();
		//panel_printf("gx_stb_panel_fd650_isr \n");
#if defined(LINUX_OS)
		create_bsp_timer(gx_stb_panel_fd650_scan_manager,1);
#endif

}

static uint32_t gx_stb_panel_ct1642_config(void)
{
	/*wangjian mdify on 20141211*/
	#ifdef MANUFACTURE_JINYA
	uint32_t nKeyValue[8]={PANEL_KEY_MENU
				, PANEL_KEY_OK
				, PANEL_KEY_UP
				, PANEL_KEY_DOWN
				, PANEL_KEY_RIGHT
				, PANEL_KEY_LEFT
				, 0
				, 0
				};
	#endif
	#ifdef MANUFACTURE_JINYA
	uint8_t chLedBit[8]={1<<0
					, 1<<1
					, 1<<2
					, 1<<3
					, 1<<4
					, 1<<5
					, 1<<6
					, 1<<7
					};
	#endif

	memset(g_PanelCT1642Config.m_chPanelName, 0, sizeof(g_PanelCT1642Config.m_chPanelName));
	sprintf((void*)g_PanelCT1642Config.m_chPanelName, "%s", "CT-1642");
	memcpy(g_PanelCT1642Config.m_nKeyValue,nKeyValue,sizeof(nKeyValue));
	memcpy(g_PanelCT1642Config.m_chLedBit,chLedBit,sizeof(chLedBit));
	
	g_PanelCT1642Config.m_chChipSelectLight = GXPIO_BIT_LOW_LEVEL;
	
	g_PanelCT1642Config.m_chDataLight = GXPIO_BIT_HIGHT_LEVEL;
	g_PanelCT1642Config.m_chLockLight = GXPIO_BIT_HIGHT_LEVEL;

	g_PanelCT1642Config.m_chClkPin    = GXOTA_PANEL_CLK_GPIO;//23;
	g_PanelCT1642Config.m_chDataPin   = GXOTA_PANEL_DATA_GPIO;//22;
	g_PanelCT1642Config.m_chKeyPin[0] = GXOTA_PANEL_STANDBY_GPIO;
	g_PanelCT1642Config.m_chKeyPin[1] = GXOTA_PANEL_STANDBY_GPIO;
	g_PanelCT1642Config.m_chLockPin   = GXOTA_PANEL_LOCK_GPIO;//24;
	#ifdef MANUFACTURE_BOSHANG
	g_PanelCT1642Config.m_chGpioLockEn =0;
	#endif
	#ifdef MANUFACTURE_HUILONG
	g_PanelCT1642Config.m_chGpioLockEn =1;
	#endif
	#ifdef MANUFACTURE_JINYA
	g_PanelCT1642Config.m_chGpioLockEn =0;
	#endif
	g_PanelCT1642Config.m_nLedAddr[0] = 0xef00;
	g_PanelCT1642Config.m_nLedAddr[1] = 0xdf00;
	g_PanelCT1642Config.m_nLedAddr[2] = 0xbf00;
	g_PanelCT1642Config.m_nLedAddr[3] = 0x7f00;	
	g_PanelCT1642Config.m_nLedAddr[4] = 0xff00;
	
	g_PanelCT1642Config.m_chLedCnt = 4;
	g_PanelCT1642Config.m_chCurLenCnt = 0;

	g_PanelCT1642Config.m_nPortBaseAddr[PROT_BASE_LOW_ADDRESS] = EPORT_ADDR;
	g_PanelCT1642Config.m_nPortBaseAddr[PROT_BASE_HIGH_ADDRESS] = 0;

	gxota_panel_public_config();
	memcpy(&g_PanelCT1642Config.m_tPanelPublicConfig
		, &g_PanelPublicConfig
		, sizeof(GXPANEL_PUBLIC_Config_t));

	return 0;
}

panel_t ct1642_jinya ={
	.config = gx_stb_panel_ct1642_config,
	.isr = gx_stb_panel_ct1642_isr,
	.init_pin = gx_stb_panel_ct1642_init_pin,
	.lock = gx_stb_panel_ct1642_lock,
	.set_led_signal_value = gx_stb_panel_ct1642_set_led_signal_value,
	.set_led_string = gx_stb_panel_ct1642_set_led_string,
	.set_led_value = gx_stb_panel_ct1642_set_led_value,
	.unlock = gx_stb_panel_ct1642_unlock,
	.show_mode = gx_stb_panel_ct1642_show_mode,
	.scan_manager = gx_stb_panel_ct1642_scan_manager,
	.power_off = gx_stb_panel_ct1642_power_off,
};
