#ifdef ECOS_OS

#include "gxcore.h"
#include "bsp_gpio_ops.h"

static Cyg_ErrNo _gpio_open(struct cyg_devtab_entry **tab,
                       struct cyg_devtab_entry *sub_tab,
                       const char *name)
{
    if(g_BspGpio.gpio_open != NULL)
    {
        g_BspGpio.gpio_open();
    }
    return ENOERR;
}

static Cyg_ErrNo _gpio_close(cyg_io_handle_t handle)
{
    if(g_BspGpio.gpio_close!= NULL)
    {
        g_BspGpio.gpio_close();
    }
    return ENOERR;
}

static Cyg_ErrNo _gpio_read(cyg_io_handle_t handle,
                         void *buf, cyg_uint32 *len)
{
    if(g_BspGpio.gpio_read!= NULL)
    {
        g_BspGpio.gpio_read();
    }
    return ENOERR;
}

static Cyg_ErrNo _gpio_ioctl(cyg_io_handle_t handle, cyg_uint32 key, void *buf)
{
    switch(key)
    {
        case GPIO_INPUT:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_input != NULL))
            {
                unsigned int port = *(unsigned int*)buf;
                g_BspGpio.gpio_ioctl->ioctl_input(port);
            }
            break;

        case GPIO_OUTPUT:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_output != NULL))
            {
                unsigned int port = *(unsigned int*)buf;
                g_BspGpio.gpio_ioctl->ioctl_output(port);
            }
            break;

        case GPIO_HIGH:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_high != NULL))
            {
                unsigned int port = *(unsigned int*)buf;
                g_BspGpio.gpio_ioctl->ioctl_high(port);
            }
            break;

        case GPIO_LOW:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_low != NULL))
            {
                unsigned int port = *(unsigned int*)buf;
                g_BspGpio.gpio_ioctl->ioctl_low(port);
            }
            break;

        case GPIO_READ:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_read != NULL))
            {
                GpioData *data = (GpioData*)buf;
                data->value = g_BspGpio.gpio_ioctl->ioctl_read(data->port);
            }
            break;

        case GPIO_PIN_FUN_SWITCH:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_switch != NULL))
            {
                MulPin pin = *(MulPin *)buf;
                g_BspGpio.gpio_ioctl->ioctl_switch(pin);
            }
            break;
    }

    return ENOERR;
}

static Cyg_ErrNo _gpio_set_config(cyg_io_handle_t handle, cyg_uint32 key,
                                          const void* buf, cyg_uint32* len)
{
    return ENOERR;
}

static cyg_devio_table_t gpio_handlers =
{
    .read = _gpio_read,
    .ioctl = _gpio_ioctl,
    .close = _gpio_close,
    .set_config = _gpio_set_config,
};

static cyg_devtab_entry_t gpio_dev =
{
	GPIO_NAME,
	NULL,
	&gpio_handlers,
	NULL,
	_gpio_open,
	NULL
};

void bsp_gpio_mod_init(void)
{
	Cyg_ErrNo ret = char_dev_register(&gpio_dev, GPIO_NAME, NULL);

	if(g_BspGpio.gpio_init != NULL)
	{
	    g_BspGpio.gpio_init();
	}
}

#endif
