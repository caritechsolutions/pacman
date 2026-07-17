#ifndef __BSP_PANEL_OPS_H__
#define __BSP_PANEL_OPS_H__

#include "bsp_dev_panel.h"

#ifdef ECOS_OS
#include "stdio.h"
#include "gxcore_hw_bsp.h"
#endif

#ifdef LINUX_OS
#include <linux/gx_gpio.h>
#include <linux/kernel.h>
/*#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/poll.h>
*/
//Minilinux
//
#endif

//之前的延时方式受整机芯片的频率的变化影响，目前更换成API接口，不受系统和芯片频率的影响，
//方便在各平台移植。
//
#if 1
// move to bsp_panel.c
#define DELAY	\
    {\
	    volatile unsigned int temp = 150; \
	    do{\
	        temp--;\
	    }while(temp);\
    }
#endif
//#define PANEL_DELAY(us) gx_udelay(us)//gx_udelay会影响串口读写见问题93071

/* led */
enum
{
    PANEL_LED0 = 0,
    PANEL_LED1,
    PANEL_LED2,
    PANEL_LED3,
    PANEL_LED_TOTAL,
};

typedef struct
{
    int (*io_show_str)(PanelString);
    int (*io_show_num)(unsigned int);
    int (*io_show_quality)(unsigned int);
    int (*io_show_lock)(unsigned int);
    int (*io_show_time)(unsigned int);
    int (*io_read_key)(unsigned int*);
    int (*io_set_gpiomap)(void *);
    int (*io_set_keymap)(void *);
    int (*io_mcu_lowpower)(void);
    int (*io_show_standby)(unsigned int);
    int (*io_show_brigthness)(unsigned int );
}BspPanelIoctl;


typedef struct
{
    int (*panel_init)(void);
    int (*panel_open)(void);
    int (*panel_close)(void);
    int (*panel_read)(unsigned int*);
    BspPanelIoctl *panel_ioctl;
}BspPanelOps;

typedef struct
{
    unsigned int seconds;
    unsigned int microsecs;
}BspPanelTickTime;

typedef void (*timer_func)(unsigned long);

extern void create_bsp_timer(timer_func func, unsigned long ms);
extern void delete_bsp_timer(void);
extern void poll_wakeup(void);

extern BspPanelOps g_BspPanel;

extern unsigned int s_panel_key;
#endif
