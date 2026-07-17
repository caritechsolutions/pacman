#ifndef __BSP_GPIO_OPS_H__
#define __BSP_GPIO_OPS_H__

#ifdef ECOS_OS
#include "gxcore_hw_bsp.h"
#endif

#ifdef LINUX_OS
#include <linux/gx_gpio.h>
#endif

#include "bsp_dev_gpio.h"

typedef struct
{
    int (*ioctl_input)(unsigned int);
    int (*ioctl_output)(unsigned int);
    int (*ioctl_high)(unsigned int);
    int (*ioctl_low)(unsigned int);
    int (*ioctl_read)(unsigned int);
    int (*ioctl_switch)(MulPin pin);
}BspGpioIoctl;

typedef struct
{
    int (*gpio_init)(void);
    int (*gpio_open)(void);
    int (*gpio_close)(void);
    int (*gpio_read)(void);
    BspGpioIoctl *gpio_ioctl;
}BspGpioOps;

extern BspGpioOps g_BspGpio;
#endif
