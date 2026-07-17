#include "power_manage.h"
#include "reg_dw8051.h"
#include "parse_cmdline.h"
#include "gpio.h"

void system_suspend(void)
{
	char power_gpio = g_cmdline->powerio[0];
	if (g_cmdline->powerio[1])
		mcu_control_signal |= (0x20);//断电参数
	else
		mcu_control_signal &= ~(0x20);//断电参数

	mcu_control_signal |= 1 << 1;

	gx_gpio_funsel_config(power_gpio, 2);
}

void system_resume(void)
{
	mcu_control_signal |= 1 << 2;
}

