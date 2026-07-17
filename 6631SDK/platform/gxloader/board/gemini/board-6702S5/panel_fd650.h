/*
 * =====================================================================================
 *
 *       Filename:  stb_panel.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2010年10月10日 20时07分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __STB_PANEL_H__
#define __STB_PANEL_H__

#define PANEL_FD650

/* key */
#define PANEL_KEY_MENU   95
#define PANEL_KEY_DOWN   71
#define PANEL_KEY_UP     79
#define PANEL_KEY_LEFT   103
#define PANEL_KEY_RIGHT  111
#define PANEL_KEY_OK     87
#define PANEL_KEY_EXIT   118
#define PANEL_KEY_PW     119
#define PANEL_LEY_NULL   0

//#define PANEL_KEY_TOTAL   6

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
#define DATA_L      (CONVERSE(BIT_D|BIT_E|BIT_F))
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

// 读取按键代码命令
#define FD650_GET_KEY	0x0700					// 获取按键,返回按键代码

#define FD650_SEC_LED   1
#define FD650_LOCK_LED    3
#define FD650_STANDBY_LED    2

void loader_panel_display(void);
void loader_panel_init(unsigned int sda, unsigned int clk);

#endif

