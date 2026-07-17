#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include "gx_gpio.h"

/*
sirius lowpower与主cpu的gpio对应关系
主cpu    lowpower  目前使用用情况
PORT00   PORT00    红外检测
PORT01   PORT01    待机唤醒
PORT02   PORT02    cec模块使用
PORT03   PORT03
PORT04   PORT04
PORT05   PORT05
PORT06   PORT06
PORT07   PORT07
PORT08   PORT08
PORT10   PORT09
PORT11   PORT10
PORT12   PORT11
PORT13   PORT12
PORT14   PORT13
PORT15   PORT14
PORT16   PORT15
PORT17   PORT16
PORT18   PORT17
PORT31   PORT18
PORT32   PORT19
PORT33   PORT20
PORT34   PORT21
PORT35   PORT22
PORT36   PORT23
PORT37   PORT24
PORT38   PORT25
PORT39   PORT26
PORT40   PORT27
PORT41   PORT28
PORT48   PORT29
*/

/*
taurus lowpower与主cpu的gpio对应关系
主cpu    lowpower   目前使用情况
PORT15   PORT00     红外检测
PORT16   PORT01     待机唤醒
PORT17   PORT02
PORT18   PORT03
PORT19   PORT04
PORT20   PORT05     cec模块使用
*/



/* gx_gpio_funsel_config
 * funsel 0: mcu 控制gpio模式 1: 固定值 2: 待机电源控制状态机模式或者CEC控制, 除了PORT02和PORT29在2模式下为CEC，其他PORT在2模式都为待机电源控制 3: acpu 控制 gpio
 * */
extern char gx_gpio_funsel_config(uint8_t gpio, uint8_t funsel);
extern char gx_gpio_setio(uint8_t gpio, uint8_t io);
extern char gx_gpio_getlevel(uint8_t gpio);
extern char gx_gpio_setlevel(uint8_t gpio, uint8_t value);
extern void gx_gpio_init(void);
#endif
