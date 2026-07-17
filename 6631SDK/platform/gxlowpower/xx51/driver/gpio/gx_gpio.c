#include "gx_gpio.h"
#include "gpio.h"
#include "reg_dw8051.h"
#include "parse_cmdline.h"
#include "config.h"

char gx_gpio_funsel_config(uint8_t gpio, uint8_t funsel)
{
	uint8_t funsel_h;
	uint8_t funsel_l;

	if (gpio >= MAX_GPIO_NUM)
		return -1;

	funsel_h = ((funsel >> 1) & 1);
	funsel_l = (funsel & 1);

	if (gpio <= 7){
#ifdef CYGNUS_LOWPOWER
		/* The funsel of gpio07 and gpio05 need to be swapped in cygnus */
		if (gpio == 7)
			gpio = 5;
		else if (gpio == 5)
			gpio = 7;
#endif
		mcu_gpio_funsel00_h &= ~(1 << gpio);
		mcu_gpio_funsel00_l &= ~(1 << gpio);
		mcu_gpio_funsel00_h |= (funsel_h << gpio);
		mcu_gpio_funsel00_l |= (funsel_l << gpio);
	} else if (gpio >= 8 && gpio <= 15){
		gpio -= 8;
		mcu_gpio_funsel01_h &= ~(1 << gpio);
		mcu_gpio_funsel01_l &= ~(1 << gpio);
		mcu_gpio_funsel01_h |= (funsel_h << gpio);
		mcu_gpio_funsel01_l |= (funsel_l << gpio);
	} else if (gpio >= 16 && gpio <= 23){
		gpio -= 16;
		mcu_gpio_funsel10_h &= ~(1 << gpio);
		mcu_gpio_funsel10_l &= ~(1 << gpio);
		mcu_gpio_funsel10_h |= (funsel_h << gpio);
		mcu_gpio_funsel10_l |= (funsel_l << gpio);
	} else if (gpio >= 24 && gpio <= 31){
		gpio -= 24;
		mcu_gpio_funsel11_h &= ~(1 << gpio);
		mcu_gpio_funsel11_l &= ~(1 << gpio);
		mcu_gpio_funsel11_h |= (funsel_h << gpio);
		mcu_gpio_funsel11_l |= (funsel_l << gpio);
	}

	return 0;
}

/* setting gpio input/output mode, 0: input, 1: output */
char gx_gpio_setio(uint8_t gpio, uint8_t io)
{
	if (gpio >= MAX_GPIO_NUM)
		return -1;

	if (io == 1) {
		gx_gpio_funsel_config(gpio, 0);
		return 0;
	}

	return gx_gpio_setlevel(gpio, 1);
}

/* set the gpio high/low level for output pins, 0: low, 1: high */
char gx_gpio_setlevel(uint8_t gpio, uint8_t value)
{
	if (gpio >= MAX_GPIO_NUM)
		return -1;

	gx_gpio_funsel_config(gpio, 0);

	if (gpio <= 7){
		if (value == 1)
			GPIO_P0 |= 1 << gpio;
		else
			GPIO_P0 &= ~(1 << gpio);
	} else if (gpio >= 8 && gpio <= 15){
		gpio -= 8;
		if (value == 1)
			GPIO_P1 |= 1 << gpio;
		else
			GPIO_P1 &= ~(1 << gpio);
	} else if (gpio >= 16 && gpio <= 23){
		gpio -= 16;
		if (value == 1)
			GPIO_P2 |= 1 << gpio;
		else
			GPIO_P2 &= ~(1 << gpio);
	} else if (gpio >= 24 && gpio <= 31){
		gpio -= 24;
		if (value == 1)
			GPIO_P3 |= 1 << gpio;
		else
			GPIO_P3 &= ~(1 << gpio);
	}

	return 0;
}

/* get the gpio level for input pins, returns 0: low, 1: high*/
char gx_gpio_getlevel(uint8_t gpio)
{
	if (gpio >= MAX_GPIO_NUM)
		return -1;
	
	if (gpio <= 7){
		return (GPIO_P0 >> gpio) & 1;
	} else if (gpio >= 8 && gpio <= 15){
		gpio -= 8;
		return (GPIO_P1 >> gpio) & 1;
	} else if (gpio >= 16 && gpio <= 23){
		gpio -= 16;
		return (GPIO_P2 >> gpio) & 1;
	} else if (gpio >= 24 && gpio <= 31){
		gpio -= 24;
		return (GPIO_P3 >> gpio) & 1;
	}

	return 0;
}

void gx_gpio_init(void)
{
	char gpio = 0;
	unsigned int gpiomask_low = g_cmdline->gpiomask & 0xFFFF;
	unsigned int gpiomask_high = (g_cmdline->gpiomask >> 16) & 0xFFFF;
	unsigned int gpiolevel_low = g_cmdline->gpiolevel & 0xFFFF;
	unsigned int gpiolevel_high = (g_cmdline->gpiolevel >> 16) & 0xFFFF;

	for (gpio = 0; gpio < MAX_GPIO_NUM; gpio++) {
		if (gpio == g_cmdline->powerio[0])
			continue;
		#ifdef ENABLE_CEC
		if ((gpio == CEC_GPIO) && (g_cmdline->cecmode > 0))
			continue;
		#endif
		if (gpio < 16) {
			if ((gpiomask_low & (1 << gpio)) == 0) {
				if ((gpiolevel_low & (1 << gpio)) != 0) {
					gx_gpio_setlevel(gpio, 1);
				}
				else {
					gx_gpio_setlevel(gpio, 0);
				}
			} else
				gx_gpio_setlevel(gpio, 0);
		} else {
			if ((gpiomask_high & (1 << (gpio - 16))) == 0) {
				if ((gpiolevel_high & (1 << (gpio - 16))) != 0) {
					gx_gpio_setlevel(gpio, 1);
				}
				else {
					gx_gpio_setlevel(gpio, 0);
				}
			} else
				gx_gpio_setlevel(gpio, 0);

		}
	}
}
