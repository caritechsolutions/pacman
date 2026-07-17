/*
 * Copyright (C) 2019 xukai <xukai@nationalchip.com>
 *
 * SPDX-License-Identifier:    GPL-2.0+
 */

#include "common/io.h"
#include "cpu_config.h"
#include "driver/gx_padmux.h"

#define CHIP_PIN_MAX       74  // 管脚复用个数
#define CHIP_PIN_PER_REG   4   // 每个寄存器有多少个管脚复用配置
#define CHIP_PIN_BITS      8   // 每个管脚复用占用寄存器bit数
#define CHIP_PIN_MASK      0x7 // 管脚复用配置掩码(每4个bit位配置一个管脚复用)
#define CHIP_PIN_SEL_BASE  (CONFIG_BASE_MMU + CONFIG_GPIO_FUNCTION_SEL_BASE) // 管脚复用配置基地址

/* GPIO00 ~ GPIO07 的管脚复用是错乱的 */
struct cygnus_gpio_func_info {
	unsigned char pad_id;
	unsigned char base;
	unsigned char offset;
} gpio_func_table[8] = {
	{0, 0, 2},
	{1, 0, 3},
	{2, 1, 0},
	{3, 1, 1},
	{4, 1, 2},
	{5, 1, 3},
	{6, 0, 0},
	{7, 0, 1},
};

int padmux_get(int pad_id)
{
	int base, offset, func;
	unsigned int pinconfig, mask;

	if (pad_id < 0 || pad_id >= CHIP_PIN_MAX)
		return -1;

	if (pad_id > 7) {
		base = pad_id / CHIP_PIN_PER_REG;
		offset = pad_id % CHIP_PIN_PER_REG;
	} else {
		base = gpio_func_table[pad_id].base;
		offset = gpio_func_table[pad_id].offset;
	}
	mask = CHIP_PIN_MASK << (offset * CHIP_PIN_BITS);

	pinconfig = __raw_readl(CHIP_PIN_SEL_BASE + base * 4);
	func = (pinconfig & mask) >> (offset * CHIP_PIN_BITS);

	return func;
}

int padmux_check(int pad_id, int function)
{
	unsigned char act_fun = 0;

	if (pad_id < 0 || pad_id >= CHIP_PIN_MAX)
		return -1;
	if (function < 0 || function > CHIP_PIN_MASK)
		return -1;

	act_fun = padmux_get(pad_id);
	if (act_fun == function)
		return 0;
	else
		return -1;
}

int padmux_set(int pad_id, int function)
{
	int base, offset, func;
	unsigned int pinconfig;

	if (pad_id < 0 || pad_id >= CHIP_PIN_MAX)
		return -1;
	if (function < 0 || function > CHIP_PIN_MASK)
		return -1;

	if (pad_id > 7) {
		base = pad_id / CHIP_PIN_PER_REG;
		offset = pad_id % CHIP_PIN_PER_REG;
	} else {
		base = gpio_func_table[pad_id].base;
		offset = gpio_func_table[pad_id].offset;
	}

	pinconfig = __raw_readl(CHIP_PIN_SEL_BASE + base * 4);
	pinconfig &= ~((CHIP_PIN_MASK) << (offset * CHIP_PIN_BITS));
	func = (function & CHIP_PIN_MASK) << (offset * CHIP_PIN_BITS);
	pinconfig |= func;
	__raw_writel(pinconfig, CHIP_PIN_SEL_BASE + base * 4);

	if (padmux_check(pad_id, function) == 0)
		return 0;
	else
		return -1;
}
