/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	gxota_panel.h
* Author    : 	DAD
* Project   :	GXOTA
* Type      :	Template
******************************************************************************
* Purpose   :	模块头文件
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2017.12.01	      DAD	         creation
            hello, please write revision information here.
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __GXOTA_PANEL_H__
#define  __GXOTA_PANEL_H__
#include "types.h"

//#define __PANEL_DEBUG__
#ifdef __PANEL_DEBUG__
	#define panel_printf(...)  gxloge(__VA_ARGS__)
#else
	#define panel_printf(...)  {;}
#endif
#ifndef FALSE
#define FALSE      0
#define TRUE       1
#endif
/**************客户面板控制宏*****************/

/***************************************************/
#define STB_PANEL_INTERVAL_MS	3

#define MAX_PANEL_NAME_LENGTH	8

#define STB_KEY_NUMBER			4

#define GPIO_TOTAL_NUM				31

#define GX3113_PINMUX_PANEL		0xd050a130

#define EPORT_ADDR                            (0xd0505000)


#define PANEL_KEY_POWER          (0xaf)
#define PANEL_KEY_MENU           (0xcf)
#define PANEL_KEY_UP             (0x77)
#define PANEL_KEY_DOWN           (0xd7)
#define PANEL_KEY_LEFT           (0xb7)
#define PANEL_KEY_RIGHT          (0x37)
#define PANEL_KEY_OK             (0x57)
#define PANEL_KEY_EXIT           (0x4f)

#define BIT_A (0)//            a
#define BIT_B (1)//         -------
#define BIT_C (2)//        |       |
#define BIT_D (3)//    //f |       | b
#define BIT_E (4)//         ---g---
#define BIT_F (5)//        |       |	c
#define BIT_G (6)//    //e |       |
#define BIT_P (7)//         ---d---   p
#define BIT_O_1635 (0<<0)//            0
#define BIT_A_1635 (1<<0)//            a
#define BIT_B_1635 (1<<1)//         -------
#define BIT_C_1635 (1<<2)//        |       |
#define BIT_D_1635 (1<<3)//    //f |       | b
#define BIT_E_1635 (1<<4)//         ---g---
#define BIT_F_1635 (1<<5)//        |       | c
#define BIT_G_1635 (1<<6)//    //e |       |
#define BIT_P_1635 (1<<7)//         ---d---   p

#define PANEL_TYPE_NONE 0
typedef enum GpioOutput_e
{
	GPIO_INPUT = 0,
	GPIO_OUTPUT
}GpioOutput_t;

enum
{
	PROT_BASE_LOW_ADDRESS,
	PROT_BASE_HIGH_ADDRESS,
	PORT_BASE_ADDRESS_END,
};

enum
{
	PANEL_READ_KEY = 0,
	PANEL_STRING,
	PANEL_DATA,
	PANEL_LOCK,
	PANEL_UNLOCK,
	PANEL_POWER_OFF,
	PANEL_SET_BLOCKMODE,
	PANEL_STANDBY,
	PANEL_WAKEUP,
	PANEL_KEY,
};

enum
{
	LED_DATA_0,
	LED_DATA_1,
	LED_DATA_2,
	LED_DATA_3,
	LED_DATA_4,
	LED_DATA_5,
	LED_DATA_6,
	LED_DATA_7,
	LED_DATA_8,
	LED_DATA_9,
	LED_DATA_E,
	LED_DATA_F,
	LED_DATA_N,
	LED_DATA_P,
	LED_DATA_t,
	LED_DATA_L,
	LED_DATA_b,
	LED_DATA_o,
	LED_DATA_HIDE,

	LED_DATA_C,
	LED_DATA_U,
	LED_DATA_A,
	LED_DATA_DARK,
};
typedef uint8_t (*gx_stb_panel_public_cfg_multiplex)(uint64_t);
typedef uint8_t (*gx_stb_panel_public_set_gpio_level)(uint8_t,uint8_t);/*gpio, level*/
typedef uint32_t (*gx_stb_panel_public_get_gpio_level)(uint8_t, uint64_t*);
typedef uint32_t (*gx_stb_panel_public_set_gpio_output)(uint8_t);
typedef uint32_t (*gx_stb_panel_public_set_gpio_input)(uint8_t);
typedef uint32_t (*gx_stb_panel_public_init_gpio)(uint64_t);
typedef uint32_t (*gx_stb_panel_public_open_gpio)(uint8_t, GpioOutput_t);
typedef uint32_t (*gx_stb_panel_public_deal_key)(uint32_t);

typedef struct GXPANEL_PUBLIC_Config_s
{
	uint32_t m_nGpioHandle;
	gx_stb_panel_public_cfg_multiplex m_PanelCfgMultiplexFun;
	gx_stb_panel_public_set_gpio_level m_PanelSetGpioLevelFun;
	gx_stb_panel_public_get_gpio_level m_PanelGetGpioLevelFun;
	gx_stb_panel_public_set_gpio_output m_PanelSetGpioOutPutFun;
	gx_stb_panel_public_set_gpio_input m_PanelSetGpioInPutFun;
	gx_stb_panel_public_init_gpio m_PanelInitGpioFun;
	gx_stb_panel_public_open_gpio m_PanelOpenGpioFun;
	gx_stb_panel_public_deal_key m_PanelDealKey;
}GXPANEL_PUBLIC_Config_t;
extern GXPANEL_PUBLIC_Config_t g_PanelPublicConfig;
void gxota_panel_public_config(void);


#define _NOP_\
    {\
	    volatile uint32_t  temp = 10; \
	    do{\
	        temp--;\
	    }while(temp);\
    }

#endif /* __GXOTA_PANEL_H__ */

