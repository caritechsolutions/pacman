#ifndef __GX_I2C__
#define __GX_I2C__
#include "i2c.h"


struct gx31x0_i2c {
	unsigned int      regs;
	unsigned int      i2c_pclk;
	unsigned int      i2c_bus_freq;
	unsigned int      hight_time;
	unsigned int      low_time;
};

struct gx_i2c_device {
	int               i2c_devid; /* the id is i2c device id */
	int               used;
	unsigned int      chip_address;
	unsigned int      address_width; /* 1 or 2 bytes, often */
	int               send_noack;
	int               send_stop;
	int               retries;
	void              *i2c_type;
	struct gx31x0_i2c *dev;
	struct gx_i2c_device *next;
};

#ifdef I2C_DEBUG
#define I2C_DBG(fmt, args...)   printf(fmt, ##args)
#else
#define I2C_DBG(fmt, args...)   ((void)0)
#endif

int gx_i2c_do_init(struct gx_i2c_device *adap);
int gx_i2c_do_exit(struct gx_i2c_device *adap);
int gx_i2c_do_clk_conf(struct gx_i2c_device *adap, uint32_t clk_value);
int gx_i2c_do_transfer(struct gx_i2c_device *adap, struct i2c_msg *msgs, int num);

#endif
