#include "stdio.h"
#include "string.h"
#include "io.h"
#include "sys/types.h"
#include "linux/errno.h"
#include "gxhwlib_registers.h"

#include "gx_i2c.h"
#include "i2c.h"


#define I2CON_W_ACK      (1 << 1) //write ack
#define I2CON_R_ACK      (1 << 2) //read ack
#define I2CON_SI         (1 << 3) //operatin finish
#define I2CON_STO        (1 << 4) //stop
#define I2CON_STA        (1 << 5) //start
#define I2CON_I2EN       (1 << 6) //module enable
#define I2CON_I2IE       (1 << 7) //interrupt enable

enum {
	I2CON = 0x0,
	I2DATA = 0x4,
	I2DIV = 0x8,
};

enum {
	I2C_READ = 0,
	I2C_WRITE
};

struct gx_i2c_dev {
	unsigned int base;
	unsigned int chip_address;
	unsigned int retries;
};

/* NOTE: In USER space, you can read()/write() via i2c-device interface in NONE-RESTART mode.
   If you want RESTART mode, you MUST use iotcl by I2C_RDWR.
   Define your own character device if you're NOT using i2c-device interface.
   I2C device node: /dev/i2c-0, i2c-1, i2c-2, etc. */

static uint32_t gxi2c_waitforack(struct gx_i2c_dev *dev)
{
	int delay  = 100000;
	uint32_t status = 0;

	while (delay-- > 0) {
		status = __raw_readl(dev->base + I2CON);

		/* wait for SI & ACK */
		if (status & (1 << 3) && !(status & (1 << 1)))
			return 0;
	}

	return -EAGAIN;
}

static uint32_t gx31x0_i2c_stop(struct gx_i2c_dev *dev)
{
	__raw_writel(I2CON_I2EN | I2CON_STO, dev->base + I2CON);

	if (gxi2c_waitforack(dev)) {
		printf("%s(): i2c send stop signal failed\n", __FUNCTION__);
		return -EAGAIN;
	}

	/* need disable i2c?? */
	__raw_writel(0, dev->base + I2CON);

	return 0;
}

static uint32_t gx31x0_i2c_start(struct gx_i2c_dev *dev, struct i2c_msg *msg, unsigned long direction)
{
	int trytimes = 10;
	uint16_t address  = msg->addr<<1;

	switch (direction) {
		case I2C_READ:
			address |= 1;
			break;
		case I2C_WRITE:
			address &= ~1;
			break;
		default:
			return -EAGAIN;
	}

	do {
		__raw_writel(address, dev->base + I2DATA);

		/* start */
		__raw_writel(I2CON_I2EN | I2CON_STA, dev->base + I2CON);

		if (!gxi2c_waitforack(dev))
			return 0;

	} while(trytimes-- > 0);

	printf("%s(): i2c send start signal failed\n", __func__);

	return -EAGAIN;
}

static uint32_t gx31x0_i2c_read(struct gx_i2c_dev *dev, struct i2c_msg *msg)
{
	int i = 0;
	int delay  = 100000;
	uint32_t status = 0;

	I2C_DBG("\n------------- ALL READ DATA START -------------\n");

	/*
	 * NOTE: here's one BUG for GX3001/GX3110. ACK/NOACK must be sent before reading data,
	 *
	 * or the 1st data is incorrect (device address). This is opposite to the datasheet.
	 */
	for (i = 0; i < msg->len; ++i) {

		/* send ACK/NOACK back to device */
		if ((i == msg->len - 1) && ((msg->flags & I2C_M_NO_RD_ACK) == I2C_M_NO_RD_ACK)) {
			/* LAST byte: HIGH ack indicating the last one */
			__raw_writel(I2CON_I2EN | I2CON_R_ACK, dev->base + I2CON);
		}
		else{
			__raw_writel(I2CON_I2EN, dev->base + I2CON);
		}

		/* wait for the finish of operation */
		while (delay-- > 0) {
			status = __raw_readl(dev->base + I2CON);

			if (status & (1 << 3)){
				break;
			}
		}

		if (delay <= 0){
			printf("%s(): i2c read msg->buf[%d]failed\n", __func__, i);
			return -EAGAIN;
		}

		delay = 100000;

		/* read data out */
		msg->buf[i] = __raw_readl(dev->base + I2DATA) & 0xff;

		if (i && !(i % 32))
			I2C_DBG("\n");

		I2C_DBG("%02x ", msg->buf[i]);
		I2C_DBG("\n------------- ALL READ DATA END   -------------\n");

	}

	return 0;
}

static uint32_t gx31x0_i2c_write(struct gx_i2c_dev *dev, struct i2c_msg *msg)
{
	int i = 0;
	int trytimes = 10;

	I2C_DBG("\n------------- ALL WRITTEN DATA START -------------\n");

	for (i = 0; i < msg->len; ++i) {

		trytimes = 10;

		if (i && !(i % 32))
			I2C_DBG("\n");

		I2C_DBG("%02x ", msg->buf[i]);
		while(trytimes-- > 0){
			__raw_writel(msg->buf[i], dev->base + I2DATA);
			__raw_writel(I2CON_I2EN, dev->base + I2CON);

			if (!gxi2c_waitforack(dev))
				break;
		}

		if (trytimes <= 0){
			printf("%s(): i2c write msg->buf[%d]failed\n", __FUNCTION__, i);
			return -EAGAIN;
		}
	}

	I2C_DBG("\n------------- ALL WRITTEN DATA END   -------------\n");

	return 0;
}

static int gx31x0_i2c_doxfer(struct gx_i2c_dev *dev, struct i2c_msg *msgs, int num)
{
	uint32_t i   = 0;
	uint32_t res = 0;

	I2C_DBG("\n================== I2C TRANSFERS ==================\n");

	for (i = 0; i < num; ++i) {
		if ((msgs[i].flags & I2C_M_NOSTART) != I2C_M_NOSTART) {

			/* send device self address simultaneously */
			I2C_DBG(" [%d] [R/W] start\n", i);

			res = gx31x0_i2c_start(dev, msgs + i, ((msgs[i].flags & I2C_M_RD) == I2C_M_RD) ? I2C_READ:I2C_WRITE);
			if (res)
				goto out;
		}

		/* 0 address width operations passed */
		if (!msgs[i].len)
			goto send_stop;

		if ((msgs[i].flags & I2C_M_RD) == I2C_M_RD) {
			/* read operations */
			I2C_DBG(" [%d] [R] read\n", i);
			res = gx31x0_i2c_read(dev, msgs + i);
			if (res)
				goto out;
		} else {
			/* write operations */
			I2C_DBG(" [%d] [W] write\n", i);
			res = gx31x0_i2c_write(dev, msgs + i);
			if (res)
				goto out;
		}

send_stop:
		if ((msgs[i].flags & I2C_M_REV_DIR_ADDR) != I2C_M_REV_DIR_ADDR){
			I2C_DBG(" [%d] [S] stop\n", i);
			res = gx31x0_i2c_stop(dev);
			if (res)
				goto out;
		}
	}

	return 0;
out:
	gx31x0_i2c_stop(dev);

	return res;
}

/* gx31x0_i2c_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
 */

static int gx31x0_i2c_xfer(struct gx_i2c_dev *dev, struct i2c_msg *msgs, int num)
{
	int ret   = 0;
	int retry = 0;

	for (retry = 0; retry < dev->retries; retry++) {
		ret = gx31x0_i2c_doxfer(dev, msgs, num);

		if (ret != -EAGAIN)
			return ret;

		printf("%s(): Retrying transmission (%d)\n", __func__, retry);
		//udelay(100);
	}

	return -EIO;
}

static int gx31x0_clk_conf(struct gx_i2c_device *adap, uint32_t clk_value)
{
	int div = 0;
	struct gx_i2c_dev *dev = (struct gx_i2c_dev *)adap->i2c_type;

	if (clk_value)
		adap->dev->i2c_bus_freq = clk_value;

	div = adap->dev->i2c_pclk / adap->dev->i2c_bus_freq / 4 - 1;

	div = (div << 16) + div;
	__raw_writel(div, dev->base + I2DIV);

	return 0;
}

static void gx_i2c_mulpin_config(int bus_num)
{
#ifdef CONFIG_ARCH_CKMMU_GX3211
	if (bus_num == 0)
		*(volatile unsigned int *)REG_PINMUX_PORTE &= ~(1 << 20);
	else if (bus_num == 1)
		*(volatile unsigned int *)REG_PINMUX_PORTE &= ~(1 << 21);
#elif defined CONFIG_ARCH_ARM_SIRIUS
	if (bus_num == 0)
		*(volatile unsigned int *)REG_PINMUX_PORTA &= ~(1 << 20);
	else if (bus_num == 1)
		*(volatile unsigned int *)REG_PINMUX_PORTA &= ~(1 << 21);
#elif defined CONFIG_ARCH_CKMMU_TAURUS
	if (bus_num == 0)
		*(volatile unsigned int *)REG_PINMUX_PORTF &= ~(1 << 20);
	else if (bus_num == 1)
		*(volatile unsigned int *)REG_PINMUX_PORTF &= ~(1 << 21);
#elif defined CONFIG_ARCH_CKMMU_SCORPIO
	if (bus_num == 0)
		*(volatile unsigned int *)REG_INTERNAL_FUNSEL &= ~(1 << 20);
	else if (bus_num == 1)
		*(volatile unsigned int *)REG_INTERNAL_FUNSEL &= ~(1 << 21);

#endif
}

static int gx31x0_i2c_init(struct gx_i2c_device *adap)
{
	uint32_t div = 0;

	/* i2c div set */
	// div = i2c->i2c_pclk/i2c->i2c_bus_freq/4 - 1;

	/*
	 *bug for hardware, if 4 division, it fail often
	 */
	struct gx_i2c_dev *dev = adap->i2c_type;

	/* I2C device address should be even */
	if (adap->chip_address & 1)
		return -EIO;

	adap->chip_address >>= 1;

	if (!dev) {
		dev = malloc(sizeof(struct gx_i2c_dev));
		if (!dev) {
			printf("[%s %d] malloc failed\n", __func__, __LINE__);
			return -ENOMEM;
		}

		adap->i2c_type = dev;
		dev->base = adap->dev->regs;
		dev->retries = adap->retries;
		dev->chip_address = adap->chip_address;
	}

	gx31x0_clk_conf(adap, 0);
	gx_i2c_mulpin_config(adap->i2c_devid);

	/* enable the i2c controller, disabling interrupt now */
	__raw_writel(I2CON_I2EN, dev->base + I2CON);

	return 0;
}

// -----------------------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------------
int gx_i2c_do_transfer(struct gx_i2c_device *adap, struct i2c_msg *msgs, int num)
{
	int ret;

	if (!adap || !msgs || !adap->i2c_type)
		return -EINVAL;

	ret = gx31x0_i2c_xfer((struct gx_i2c_dev *)adap->i2c_type, &msgs[0], num);

	if (ret < 0)
		printf("XFR: Error during I2C_RDWR ioctl with error code: %d\n", ret);

	return ret;
}

int gx_i2c_do_init(struct gx_i2c_device *adap)
{
	return gx31x0_i2c_init(adap);
}

int gx_i2c_do_exit(struct gx_i2c_device *adap)
{
	if (adap && adap->i2c_type)
		free(adap->i2c_type);

	return 0;
}

int gx_i2c_do_clk_conf(struct gx_i2c_device *adap, uint32_t clk_value)
{
	return gx31x0_clk_conf(adap, clk_value);
}
