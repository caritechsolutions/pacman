#ifndef __GX_GPIO_H__
#define __GX_GPIO_H__

#include "gpio.h"

/* final GPIO entry in SDRAM */
struct gpio_entry
{
	unsigned char valid;
	unsigned char vir_gpio; /* gpio logical number */
	unsigned char phy_gpio; /* gpio physical number */
	unsigned char io_mode:1;  /* 0: input, 1: output */
	unsigned char output_value:1; /* 0: low, 1: high (only for output mode) */
	unsigned char reserved:6;
};

//#define DEBUG
#ifdef DEBUG
#define GPIO_PRINTF      printf
#else
#define GPIO_PRINTF(...) do{}while(0);
#endif

#define ONE_GROUP_GPIO_NUM   32
#define GPIO_GROUP_NUM       CONFIG_GPIO_SET_NUM
#define GX_GPIO_PHY_NUM      (ONE_GROUP_GPIO_NUM * GPIO_GROUP_NUM)
#define GX_GPIO_TOTAL_NUM    (255)
#define PWM_CHANNEL_NUM 8
#define PWM_SEL_WIDTH 4

struct gx_gpio_pri_info_s{
	struct gpio_entry table[GX_GPIO_TOTAL_NUM];
	unsigned int reg_base[GPIO_GROUP_NUM];
	int output_read_support;  /* 0: not support. 1: support read output status from reg */
	unsigned int gpio_clk;
};


#endif
