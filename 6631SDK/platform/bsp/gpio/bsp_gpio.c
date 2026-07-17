#include "bsp_gpio_ops.h"

/***********************仅用于GX6605S和TAURUS*************************/
#define GX_REG_VIRTUAL_BASE1         0xa0000000
#define REG_PINMUX_PORTA         (0x0030a13c + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTB         (0x0030a140 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTC         (0x0030a144 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTD         (0x0030a148 + GX_REG_VIRTUAL_BASE1)

#define REG_SET_CLR_BIT(reg, bit, bit_val) do {		\
	unsigned int tmp = *(volatile unsigned int*)(reg);		\
	tmp &= ~(1UL << (bit));			\
	tmp |= ((bit_val) << (bit));			\
	*(volatile unsigned int*)(reg) = tmp;				\
}while(0)
/**********************************************************************/

static int _set_input(unsigned int port)
{
	gx_gpio_setio(port, GX_GPIO_INPUT);
    return 0;
}

static int _set_output(unsigned int port)
{
	gx_gpio_setio(port, GX_GPIO_OUTPUT);
    return 0;
}

static int _set_high(unsigned int port)
{
	gx_gpio_setlevel(port, GX_GPIO_HIGH);
    return 0;
}

static int _set_low(unsigned int port)
{
	gx_gpio_setlevel(port, GX_GPIO_LOW);
    return 0;
}

static int _read(unsigned int port)
{
    int ret = gx_gpio_getlevel(port);
	return ret;
}

/*
 *此函数仅适用于gx6605s/taurus
 * */
static int _switch(MulPin pin)
{
    if(pin.sel0 < 32)
        REG_SET_CLR_BIT(REG_PINMUX_PORTA, pin.sel0, pin.fun & 0x1);
    else if (pin.sel0 < 64)
        REG_SET_CLR_BIT(REG_PINMUX_PORTB, pin.sel0 - 32, pin.fun & 0x1);
    else if (pin.sel0 < 96)
        REG_SET_CLR_BIT(REG_PINMUX_PORTC, pin.sel0 - 64, pin.fun & 0x1);
    else if (pin.sel0 < 128)
        REG_SET_CLR_BIT(REG_PINMUX_PORTD, pin.sel0 - 96, pin.fun & 0x1);

    if(pin.sel1 < 32)
        REG_SET_CLR_BIT(REG_PINMUX_PORTA, pin.sel1, (pin.fun & 0x2) >> 1);
    else if (pin.sel1 < 64)
        REG_SET_CLR_BIT(REG_PINMUX_PORTB, pin.sel1 - 32, (pin.fun & 0x2) >> 1);
    else if (pin.sel1 < 96)
        REG_SET_CLR_BIT(REG_PINMUX_PORTC, pin.sel1 - 64, (pin.fun & 0x2) >> 1);
    else if (pin.sel1 < 128)
        REG_SET_CLR_BIT(REG_PINMUX_PORTD, pin.sel1 - 96, (pin.fun & 0x2) >> 1);

    if(pin.sel2 < 32)
        REG_SET_CLR_BIT(REG_PINMUX_PORTA, pin.sel2, (pin.fun & 0x4) >> 2);
    else if (pin.sel2 < 64)
        REG_SET_CLR_BIT(REG_PINMUX_PORTB, pin.sel2 - 32, (pin.fun & 0x4) >> 2);
    else if (pin.sel2 < 96)
        REG_SET_CLR_BIT(REG_PINMUX_PORTC, pin.sel2 - 64, (pin.fun & 0x4) >> 2);
    else if (pin.sel2 < 128)
        REG_SET_CLR_BIT(REG_PINMUX_PORTD, pin.sel2 - 96, (pin.fun & 0x4) >> 2);

    return 0;
}

static BspGpioIoctl gpio_ioctl =
{
    .ioctl_input = _set_input,
    .ioctl_output= _set_output,
    .ioctl_high = _set_high,
    .ioctl_low = _set_low,
    .ioctl_read = _read,
    .ioctl_switch = _switch,
};

BspGpioOps g_BspGpio =
{
    .gpio_init = 0,
    .gpio_open = 0,
    .gpio_close = 0,
    .gpio_read = 0,
    .gpio_ioctl = &gpio_ioctl,
};

