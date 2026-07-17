#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#ifdef ECOS_OS
#ifdef __cplusplus

extern "C" void bsp_gpio_mod_init(void);
class cyg_gpio_init_class {
    public:
        cyg_gpio_init_class(void)
        {
            bsp_gpio_mod_init();
        }
};

#define MOD_GPIO  cyg_gpio_init_class   gpio0   CYGBLD_ATTRIB_INIT_PRI(60110);
#endif
#endif

#define GPIO_NAME "/dev/gxgpio"

typedef struct
{
    unsigned int port;
    unsigned int value;
}GpioData;

typedef struct
{
    unsigned char sel0;
    unsigned char sel1;
    unsigned char sel2;
    unsigned char fun;
}MulPin;

#define GPIO_INPUT		_IOW('x', 0x00, unsigned int)
#define GPIO_OUTPUT		_IOW('x', 0x01, unsigned int)
#define GPIO_HIGH		_IOW('x', 0x02, unsigned int)
#define GPIO_LOW		_IOW('x', 0x03, unsigned int)
#define GPIO_READ		_IOWR('x', 0x04, GpioData)
#define GPIO_PIN_FUN_SWITCH   _IOW('x', 0x05, MulPin)

#endif
