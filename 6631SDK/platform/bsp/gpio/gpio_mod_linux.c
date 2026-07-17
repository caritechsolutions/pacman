#ifdef LINUX_OS

#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include "bsp_gpio_ops.h"
#define GPIO_MAJOR 23

static int _gpio_open(struct inode *inode, struct file *file)
{
    if(g_BspGpio.gpio_open != NULL)
    {
        g_BspGpio.gpio_open();
    }
    return 0;
}

static int _gpio_release(struct inode *inode, struct file *file)
{
    if(g_BspGpio.gpio_close != NULL)
    {
        g_BspGpio.gpio_close();
    }
    return 0;
}

static ssize_t _gpio_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int ret = -EFAULT;
    if(g_BspGpio.gpio_read != NULL)
    {
        g_BspGpio.gpio_read();
    }
    return ret;
}

static long _gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = -EFAULT;

    switch(cmd)
    {
        case GPIO_INPUT:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_input != NULL))
            {
                unsigned int port = 0xff;
                get_user(port, (unsigned int __user*)arg);
                ret = g_BspGpio.gpio_ioctl->ioctl_input(port);
            }
            break;

        case GPIO_OUTPUT:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_output != NULL))
            {
                unsigned int port = 0xff;
                get_user(port, (unsigned int __user*)arg);
                ret = g_BspGpio.gpio_ioctl->ioctl_output(port);
            }
            break;

        case GPIO_HIGH:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_high != NULL))
            {
                unsigned int port = 0xff;
                get_user(port, (unsigned int __user*)arg);
                ret = g_BspGpio.gpio_ioctl->ioctl_high(port);
            }
            break;

        case GPIO_LOW:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_low != NULL))
            {
                unsigned int port = 0xff;
                get_user(port, (unsigned int __user*)arg);
                ret = g_BspGpio.gpio_ioctl->ioctl_low(port);
            }
            break;

        case GPIO_READ:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_read != NULL))
            {
                GpioData data = {0};
                if(copy_from_user(&data, (GpioData __user*)arg, sizeof(GpioData)) == 0)
                {
                    data.value = g_BspGpio.gpio_ioctl->ioctl_read(data.port);
                    ret = copy_to_user((GpioData __user*)arg, &data, sizeof(GpioData));
                }
            }
            break;

        case GPIO_PIN_FUN_SWITCH:
            if((g_BspGpio.gpio_ioctl != NULL) && (g_BspGpio.gpio_ioctl->ioctl_switch != NULL))
            {
                MulPin pin = {0};
                if(0 == copy_from_user(&pin, (MulPin __user*)arg, sizeof(MulPin)))
                    ret = g_BspGpio.gpio_ioctl->ioctl_switch(pin);
            }
            break;

        default:
            printk(KERN_INFO"gpio set none!\n");
            break;
    }

    return ret;
}

static struct file_operations gpio_fops =
{
    .read = _gpio_read,
    .unlocked_ioctl = _gpio_ioctl,
    .open = _gpio_open,
    .release = _gpio_release,
};

static int __init gpio_dev_init(void)
{
    printk(KERN_INFO"hello, GPIO!\n");

    register_chrdev(GPIO_MAJOR, "gxgpio", &gpio_fops);
    if(g_BspGpio.gpio_init != NULL)
    {
        g_BspGpio.gpio_init();
    }

    return 0;
}

static void __exit gpio_dev_exit(void)
{
    unregister_chrdev(GPIO_MAJOR, "gxgpio");
    printk(KERN_INFO"bye, GPIO!\n");
}

module_init(gpio_dev_init);
module_exit(gpio_dev_exit);

MODULE_DESCRIPTION("driver for gx gpio");
MODULE_AUTHOR("APP");
MODULE_LICENSE("GPL");

#endif
