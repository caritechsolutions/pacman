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
 *         *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __STB_PANEL_H__
#define __STB_PANEL_H__

#define PANEL_FD650

/* key */

#define PANEL_LEY_NULL    0

//#define PANEL_KEY_TOTAL   6

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

#if 0
// 加载字数据命令
#define FD650_DIG0		0x1400			// 数码管位0显示,需另加8位数据
#define FD650_DIG1		0x1500			// 数码管位1显示,需另加8位数据
#define FD650_DIG2		0x1600			// 数码管位2显示,需另加8位数据
#define FD650_DIG3		0x1700			// 数码管位3显示,需另加8位数据

#define FD650_DOT			0x0080			// 数码管小数点显示
#endif

// 读取按键代码命令
#define FD650_GET_KEY	0x0700					// 获取按键,返回按键代码

#define FD650_SEC_LED   1
//#define FD650_LOCK_LED    3
//#define FD650_STANDBY_LED    2

#endif

