#ifndef __TDCXO_H__
#define __TDCXO_H__

#include <stdio.h>
#include "config.h"

/**
 * 测量校准值
 *
 * \param xtal 选择晶振类型，0 TKD0 7pf; 1 TKD1 7pf; 2 TXC 7pf; 3 Hosonic 9pf; 4 TDK 9pf;
 * \param fine 补偿精度，传0默认值35
 * \param *adjust 测量的 adjust 值，可以是负数
 * \param timeout 测试超时时间,传0为默认148ms
 *
 * \return 返回函数是否执行成功
 * \retval 0 功能正常
 * \retval -1 超时
 */
extern int gxtdcxo_measure(u32 xtal,u32 fine,s32 *adjust, s32 *ppm, u32 timeout);
/**
 * 热敏补偿
 *
 * \param xtal 选择晶振类型，0 TKD0 7pf; 1 TKD1 7pf; 2 TXC 7pf; 3 Hosonic 9pf; 4 TDK 9pf;
 * \param fine 补偿精度，传0默认值35
 * \param adjust 补偿校准值,可能是负数
 * \param gate 补偿门限，只有定时补偿的时候需要传递，loader 不需要
 * \param times 补偿间隔时间,传0默认值148ms
 *
 * \return 返回函数是否执行成功
 * \retval 0 执行成功
 * \retval -1 功能出错
 */
extern int gxtdcxo_compensate(u32 xtal,u32 fine, s32 adjust, u32 gate,u32 times);

#endif


