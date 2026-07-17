#include "stdio.h"
#include "linux/errno.h"
#include "gx_i2c.h"
#include "i2c.h"
#include "config.h"
#include "gxhwlib_registers.h"

#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_CKMMU_SCORPIO) || defined(CONFIG_ARCH_CKMMU_GX3211) || defined(CONFIG_ARCH_CKMMU_GX6605S) || defined(CONFIG_ARCH_CKMMU_GX3201) || defined(CONFIG_ARCH_CKMMU_GX3113C)
#ifdef CONFIG_I2C_TYPE_GX
#define MAX_I2C_DEVICES (3)
#else
#define MAX_I2C_DEVICES (2)
#endif
#elif defined(CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#define MAX_I2C_DEVICES (2)
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
#define MAX_I2C_DEVICES (4)
#else
#error "you must defined the macro"
#endif

#ifdef CONFIG_I2C_BUS_FREQ
#define I2C_BUS_FREQ CONFIG_I2C_BUS_FREQ
#else
#define I2C_BUS_FREQ 100000UL
#endif

struct gx_i2c_device	g_gx_i2c_device[MAX_I2C_DEVICES];

#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_CKMMU_SCORPIO) || defined(CONFIG_ARCH_CKMMU_GX3211) || defined(CONFIG_ARCH_CKMMU_GX6605S) || defined(CONFIG_ARCH_CKMMU_GX3201) || defined(CONFIG_ARCH_CKMMU_GX3113C)
#ifdef CONFIG_I2C_TYPE_GX
struct gx31x0_i2c g_gx31x0_i2c[] = {
	{(REG_BASE_I2C1), 27000000UL, I2C_BUS_FREQ, 0, 0},
	{(REG_BASE_I2C2), 27000000UL, I2C_BUS_FREQ, 0, 0},
	{(REG_BASE_I2C3), 27000000UL, I2C_BUS_FREQ, 0, 0},
};
#else
struct gx31x0_i2c g_gx31x0_i2c[] = {
	{(REG_BASE_I2C4), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
	{(REG_BASE_I2C5), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
};
#endif
#elif defined(CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#ifdef CONFIG_I2C_TYPE_GX
struct gx31x0_i2c g_gx31x0_i2c[] = {
	{(REG_BASE_I2C1), 27000000UL, I2C_BUS_FREQ, 0, 0},
	{(REG_BASE_I2C2), 27000000UL, I2C_BUS_FREQ, 0, 0},
};
#else
struct gx31x0_i2c g_gx31x0_i2c[] = {
	{(REG_BASE_I2C3), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
	{(REG_BASE_I2C4), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
};
#endif
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
#ifdef CONFIG_I2C_TYPE_GX
struct gx31x0_i2c g_gx31x0_i2c[] = {
	{(REG_BASE_I2C1), 27000000UL, I2C_BUS_FREQ, 0, 0},
	{(REG_BASE_I2C2), 27000000UL, I2C_BUS_FREQ, 0, 0},
	{(REG_BASE_I2C3), 27000000UL, I2C_BUS_FREQ, 0, 0},
	{(REG_BASE_I2C4), 27000000UL, I2C_BUS_FREQ, 0, 0},
};
#else
struct gx31x0_i2c g_gx31x0_i2c[] = {
	{(REG_BASE_I2C5), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
	{(REG_BASE_I2C6), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
	{(REG_BASE_I2C7), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
	{(REG_BASE_I2C8), 27000000UL, I2C_BUS_FREQ, 5000, 5000},
};
#endif
#else
#error "you must defined the macro"
#endif

static int g_bLittleEndian = 1; /* default: ARM */
static struct gx_i2c_device *g_i2c_list = NULL;

static struct gx_i2c_device* gx_i2c_dev_register(uint32_t id)
{
	struct gx_i2c_device* i2c = NULL;
	struct gx_i2c_device* last_node = g_i2c_list;

	for (i2c = &g_gx_i2c_device[0]; i2c != &g_gx_i2c_device[MAX_I2C_DEVICES]; i2c++) {
		if (i2c->used == 0)
			break;
	}

	if (i2c == &g_gx_i2c_device[MAX_I2C_DEVICES]) {
		printf("%s(): no free g_gx_i2c_device found\n", __func__);
		return NULL;
	}

	/* init the device node */
	i2c->i2c_devid          = id;
	i2c->used               = 1;
	i2c->next               = NULL;
	i2c->dev                = &g_gx31x0_i2c[id];

	/* add this device node at list tail */
	if (!g_i2c_list)
		g_i2c_list = i2c;
	else {
		while (last_node->next)
			last_node = last_node->next;
		last_node->next = i2c;
	}

	return i2c;
}

static int gx_i2c_dev_unregister(struct gx_i2c_device* device)
{
	struct gx_i2c_device *previous_i2c  = g_i2c_list;
	struct gx_i2c_device *current_i2c   = g_i2c_list;

	if (!device)
		return -1;

	while (current_i2c) {
		/* delete the device node if found */
		if (device->i2c_devid == current_i2c->i2c_devid) {

			if (previous_i2c==current_i2c)
				/* first node */
				g_i2c_list = current_i2c->next;
			else
				previous_i2c->next = current_i2c->next;

			device->i2c_devid     = 0;
			device->used          = 0;
			device->chip_address  = 0;
			device->address_width = 0;
			device->retries       = 0;
			device->dev           = NULL;

			gx_i2c_do_exit(device);
			device->i2c_type      = NULL;

			return 0;
		}

		previous_i2c    = current_i2c;
		current_i2c     = current_i2c->next;
	}

	/* no the specified device found */
	return -ENOMEM;
}

static int gx_i2c_transaction(void *dev, uint32_t reg_address, const uint8_t *data, uint32_t count, int read)
{
	int ret = 0;
	int num = 2;
	uint8_t buf[4] = {0};

	struct i2c_msg msgs[2] = {0};
	struct gx_i2c_device *i2c = (struct gx_i2c_device *)dev;

	if (!i2c || !data)
		return -EINVAL;

	/* change endianness */
	if (g_bLittleEndian){
		switch (i2c->address_width)
		{
			case 0:
				break;
			case 1:
				buf[0] = (uint8_t)(reg_address);
				break;

			case 2:
				buf[0] = (uint8_t)(reg_address >> 8);
				buf[1] = (uint8_t)(reg_address);
				break;

			case 3:
				buf[0] = (uint8_t)(reg_address >> 16);
				buf[1] = (uint8_t)(reg_address >> 8);
				buf[2] = (uint8_t)(reg_address);
				break;

			case 4:
				buf[0] = (uint8_t)(reg_address >> 24);
				buf[1] = (uint8_t)(reg_address >> 16);
				buf[2] = (uint8_t)(reg_address >> 8);
				buf[3] = (uint8_t)(reg_address);
				break;

				/* invalid address width */
			default:
				return -EINVAL;
		}
	} else {
		switch (i2c->address_width)
		{
			case 0:
				break;
			case 1:
				buf[0] = (uint8_t)(reg_address);
				break;

			case 2:
				buf[0] = (uint8_t)(reg_address);
				buf[1] = (uint8_t)(reg_address >> 8);
				break;

			case 3:
				buf[0] = (uint8_t)(reg_address);
				buf[1] = (uint8_t)(reg_address >> 8);
				buf[2] = (uint8_t)(reg_address >> 16);
				break;

			case 4:
				buf[0] = (uint8_t)(reg_address);
				buf[1] = (uint8_t)(reg_address >> 8);
				buf[2] = (uint8_t)(reg_address >> 16);
				buf[3] = (uint8_t)(reg_address >> 24);
				break;

				/* invalid address width */
			default:
				return -EINVAL;
		}
	}

	/* Send chip address. */
	msgs[0].len   = i2c->address_width;
	msgs[0].addr  = i2c->chip_address;
	msgs[0].buf   = &buf[0];
	if (i2c->send_stop && i2c->address_width)
		msgs[0].flags = 0;
	else
		msgs[0].flags = I2C_M_REV_DIR_ADDR;

	if (read && !i2c->address_width)
		msgs[0].flags |= I2C_M_RD;

	/* data. */
	msgs[1].len   = count;
	msgs[1].addr  = i2c->chip_address;
	msgs[1].buf   = (uint8_t *)data;
	if (!read) {
		if (i2c->send_noack && i2c->address_width)
			msgs[1].flags = I2C_M_NOSTART;
		else
			msgs[1].flags = 0;
	} else {
		if (i2c->send_noack)
			msgs[1].flags = I2C_M_RD|I2C_M_NO_RD_ACK;
		else
			msgs[1].flags = I2C_M_RD;
	}

	/* don't send chip address for a8293, send stop at last */
	if (!i2c->address_width)
		msgs[1].flags |= I2C_M_NOSTART;

	ret = gx_i2c_do_transfer(i2c, &msgs[0], num);

	if (ret < 0)
		printf("Error during I2C_RDWR ioctl with error code: %d\n", ret);

	return (ret == 0) ? count:ret;
}

static struct gx_i2c_device *gx_i2c_open_dev(uint32_t id)
{
	struct gx_i2c_device *i2c = g_i2c_list;

	/* find the i2c device node */
	while (i2c) {
		if (i2c->i2c_devid == id){
			printf("%s(),find active i2c device,i2c_devid = %d\n", __func__, i2c->i2c_devid);
			return i2c;
		}
		i2c = i2c->next;
	}

	/* if list is NULL or device not found, create one node */
	return gx_i2c_dev_register(id);
}

static int gx_i2c_closedev(void *dev)
{
	return gx_i2c_dev_unregister((struct gx_i2c_device *)dev);
}

static int gx_i2c_setdev(void *dev, uint32_t busid, uint32_t chip_addr,
		uint32_t address_width, int send_noack, int send_stop)
{
	struct gx_i2c_device *i2c = (struct gx_i2c_device *)dev;

	if (!i2c)
		return -EINVAL;

	i2c->chip_address         = chip_addr;
	i2c->address_width        = address_width;
	i2c->send_noack           = send_noack;
	i2c->send_stop            = send_stop;
	i2c->retries              = 2;

	return gx_i2c_do_init(i2c);
}

//-----------Export functions-----------------
void *gx_i2c_open(uint32_t id)
{
	return (void *)gx_i2c_open_dev(id);
}

int gx_i2c_close(void *dev)
{
	return gx_i2c_closedev(dev);
}

int gx_i2c_set(void *dev, uint32_t busid, uint32_t chip_addr, uint32_t address_width,
		int send_noack, int send_stop)
{
	return gx_i2c_setdev(dev, busid, chip_addr, address_width, send_noack, send_stop);
}

int gx_i2c_tx(void *dev, uint32_t reg_address, const uint8_t *tx_data, uint32_t count)
{
	return gx_i2c_transaction(dev, reg_address, tx_data, count, 0);
}

int gx_i2c_rx(void *dev, uint32_t reg_address, uint8_t *rx_data, uint32_t count)
{
	return gx_i2c_transaction(dev, reg_address, rx_data, count, 1);
}

int gx_i2c_clk_conf(void *dev, uint32_t clk_value)
{
	if (!dev)
		return -EINVAL;

	return gx_i2c_do_clk_conf((struct gx_i2c_device *)dev, clk_value);
}
