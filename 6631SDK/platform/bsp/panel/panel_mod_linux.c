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
#include "bsp_panel_ops.h"

#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>

#define PANEL_MAJOR 22
#define TICK_HZ (100)

static struct timer_list bsp_timer;
static struct timer_list key_timer;
static timer_func bsp_panel_callback = NULL;
static timer_func key_panel_callback = NULL;
static unsigned long bsp_timer_delay = 0;
static unsigned long key_timer_delay = 0;

wait_queue_head_t g_PanelPollQueue;


static void bsp_timer_cb(unsigned long data)
{
    if(bsp_panel_callback != NULL)
    {
       bsp_panel_callback(data);
    }
    mod_timer(&bsp_timer, jiffies + bsp_timer_delay);
}

void create_bsp_timer(timer_func func, unsigned long ms)
{
    bsp_panel_callback = func;
    // linux下使用内核定时器，使用"jiffies"计时，目前系统的时间单位是10ms，所以
    // 需要将传入的ms转换成"jiffies"的计数。保证面板驱动部分时间配置ecos和linux
    // 统一。
    bsp_timer_delay = (ms>10)?((ms%10)>0?(ms/10+1):(ms/10)):1;
    bsp_timer.function = bsp_timer_cb;
    bsp_timer.expires = jiffies + bsp_timer_delay;
    if(!timer_pending(&bsp_timer))
    {
        init_timer(&bsp_timer);
        add_timer(&bsp_timer);
        //printk(KERN_INFO"create timer!\n");
    }
}

void delete_bsp_timer(void)
{
    del_timer(&bsp_timer);
    //printk(KERN_INFO"delete timer!\n");
}

static void key_timer_cb(unsigned long data)
{
    if(key_panel_callback != NULL)
    {
        key_panel_callback(data);
    }
    mod_timer(&key_timer, jiffies + key_timer_delay);
}

void create_key_timer(timer_func func, unsigned long ms)
{
    key_panel_callback = func;
    // linux下使用内核定时器，使用"jiffies"计时，目前系统的时间单位是10ms，所以
    // 需要将传入的ms转换成"jiffies"的计数。保证面板驱动部分时间配置ecos和linux
    // 统一。
    key_timer_delay = (ms>10)?((ms%10)>0?(ms/10+1):(ms/10)):1;
    key_timer.function = key_timer_cb;
    key_timer.expires = jiffies + key_timer_delay;
    if(!timer_pending(&key_timer))
    {
        init_timer(&key_timer);
        add_timer(&key_timer);
        //printk(KERN_INFO"create timer!\n");
    }
}

void delete_key_timer(void)
{
    del_timer(&key_timer);
    //printk(KERN_INFO"delete timer!\n");
}

// for select
// 增加“thiz_select_active”，用于配合poll和select的使用。同步ECOS的实现。
static char thiz_select_active = 0;
void poll_wakeup(void)
{
    if(0 == thiz_select_active)
    {
        thiz_select_active = 1;
        wake_up_interruptible(&g_PanelPollQueue);
    }
}

void bsp_panel_get_ticktime(BspPanelTickTime *tick_time)
{
    if(tick_time == NULL)
        return;

    tick_time->seconds = jiffies / TICK_HZ;
    tick_time->microsecs = (jiffies % TICK_HZ) * (1000 / TICK_HZ) * 1000;
}

unsigned int _panel_poll(struct file *filp, struct poll_table_struct *table)
{
    unsigned int mask = 0;
    if (thiz_select_active == 0)
    {
        poll_wait(filp, &g_PanelPollQueue, table);//add to monitor queue
    }
    else
    {
        thiz_select_active = 0;
        mask |= POLLIN;     //dev readable
    }
    return mask;

}

static int _panel_open(struct inode *inode, struct file *file)
{
    if(g_BspPanel.panel_open != NULL)
    {
        g_BspPanel.panel_open();
        //printk(KERN_INFO"open panel!\n");
    }
    return 0;
}

static int _panel_release(struct inode *inode, struct file *file)
{
    if(g_BspPanel.panel_close != NULL)
    {
        g_BspPanel.panel_close();
        //printk(KERN_INFO"close panel!\n");
    }
    return 0;
}

static ssize_t _panel_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int ret = -EFAULT;
    if(g_BspPanel.panel_read != NULL)
    {
        unsigned int key_val = 0;
        if(g_BspPanel.panel_read(&key_val) == 0)
        {
             put_user(key_val, (unsigned int __user *)buf);
             ret = 0;
             //printk(KERN_INFO"read panel!\n");
        }
    }
    return ret;
}

static long _panel_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = -EFAULT;

    switch(cmd)
    {
        case PANEL_SHOW_STR:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_str != NULL))
            {
                PanelString str = PANEL_LED_DARK;
                //printk(KERN_INFO"panel ioctl show str!\n");
                get_user(str, (PanelString __user*)arg);
                ret = g_BspPanel.panel_ioctl->io_show_str(str);
            }
            break;

        case PANEL_SHOW_NUM:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_num != NULL))
            {
                unsigned int num = 0;
                //printk(KERN_INFO"panel ioctl show num!\n");
                get_user(num, (unsigned int __user*)arg);
                ret = g_BspPanel.panel_ioctl->io_show_num(num);
            }
            break;

        case PANEL_READ_KEY:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_read_key != NULL))
            {
                unsigned int key = 0;
                //printk(KERN_INFO"panel ioctl read key!\n");
                ret = g_BspPanel.panel_ioctl->io_read_key(&key);
                put_user(key, (unsigned int __user*)arg);
            }
            break;

        case PANEL_SHOW_LOCK:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_lock != NULL))
            {
                unsigned int lock = 0;
                //printk(KERN_INFO"panel ioctl show lock!\n");
                get_user(lock, (unsigned int __user*)arg);
                ret = g_BspPanel.panel_ioctl->io_show_lock(lock);
            }
            break;

	case PANEL_SHOW_STANDBY:
		//printk("\n___________ %s::%d ___________  PANEL_SHOW_STANDBY \n", __FUNCTION__, __LINE__);
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_standby != NULL))
            {
		//printk("\n___________ %s::%d ___________  PANEL_SHOW_STANDBY \n", __FUNCTION__, __LINE__);
                unsigned int standby = 0;
                get_user(standby, (unsigned int __user*)arg);
                ret = g_BspPanel.panel_ioctl->io_show_standby(standby);
            }
            break;

        case PANEL_SHOW_QUALITY:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_quality != NULL))
            {
                unsigned int quality = 0;
                //printk(KERN_INFO"panel ioctl show quality!\n");
                get_user(quality, (unsigned int __user*)arg);
                ret = g_BspPanel.panel_ioctl->io_show_quality(quality);
            }
            break;

         case PANEL_SHOW_TIME:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_time!= NULL))
            {
                unsigned int time = 0;
                //printk(KERN_INFO"panel ioctl show time!\n");
                get_user(time, (unsigned int __user*)arg);
                ret = g_BspPanel.panel_ioctl->io_show_time(time);
            }
            break;

        case PANEL_SET_GPIOMAP:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_set_gpiomap != NULL))
            {
                PanelGpioMap panel_gpio = {0};
                if(copy_from_user(&panel_gpio, (PanelGpioMap __user*)arg, sizeof(panel_gpio)) == 0)
                {
                    ret = g_BspPanel.panel_ioctl->io_set_gpiomap(&panel_gpio);
                }
            }
            break;
        case PANEL_SET_KEYMAP:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_set_keymap != NULL))
            {
                panel_parameter panel;
                memset(&panel,0,sizeof(panel_parameter));
                if(copy_from_user(&panel, (panel_parameter __user*)arg, sizeof(panel)) == 0)
                {
		    /*int i;
                    for(i = 0; i < PANEL_KEYMAP_TOTAL; i++)
                    {
                        printk(KERN_INFO"key %d = 0x%x \n", i, keymap[i]);
                    }*/
                    ret = g_BspPanel.panel_ioctl->io_set_keymap(&panel);
                }
            }
            break;

        case PANEL_MCU_LOWPOWER:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_mcu_lowpower!= NULL))
            {
                //printk(KERN_INFO"panel ioctl show time!\n");
                //get_user(time, (unsigned int __user*)arg);
                ret = g_BspPanel.panel_ioctl->io_mcu_lowpower();
            }
            break;
     case PANEL_SHOW_BRIGHTNESS:
            if((g_BspPanel.panel_ioctl != NULL) && (g_BspPanel.panel_ioctl->io_show_brigthness != NULL))
            {

                unsigned int brigthness ;
                get_user(brigthness, (unsigned int __user*)arg);

                g_BspPanel.panel_ioctl->io_show_brigthness(brigthness);
            }
            break;

        default:
            printk(KERN_INFO"panel ioctl none!\n");
            break;
    }

    return ret;
}

static struct file_operations panel_fops =
{
    .read = _panel_read,
    .unlocked_ioctl = _panel_ioctl,
    .open = _panel_open,
    .release = _panel_release,
    .poll = _panel_poll,
};

static int __init panel_dev_init(void)
{
    printk(KERN_INFO"hello, panel!\n");

    register_chrdev(PANEL_MAJOR, "gxpanel0", &panel_fops);
    if(g_BspPanel.panel_init != NULL)
    {
        g_BspPanel.panel_init();
        // for select
        thiz_select_active = 0;
        init_waitqueue_head(&g_PanelPollQueue);
    }

    return 0;
}

static void __exit panel_dev_exit(void)
{
    unregister_chrdev(PANEL_MAJOR, "gxpanel0");
    printk(KERN_INFO"bye, panel!\n");
}

module_init(panel_dev_init);
module_exit(panel_dev_exit);

MODULE_DESCRIPTION("driver for gx stb panel");
MODULE_AUTHOR("APP");
MODULE_LICENSE("GPL");

#endif
