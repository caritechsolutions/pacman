#include "app_module.h"

#define MAX_I2C_DEV (3)

#ifdef LINUX_OS
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <fcntl.h>

static int g_bLittleEndian = 1;
static char *s_i2c_dev[MAX_I2C_DEV] =
{
    "/dev/i2c-0",
    "/dev/i2c-1",
    "/dev/i2c-2",
};

int app_gxi2c_tx(unsigned int id, unsigned int reg_address, I2cSetParam *set_param, I2cUserData *user_data)
{
	int ret = 0;
	unsigned char buf[4] = {0};
    struct i2c_rdwr_ioctl_data rdwrdata;
    struct i2c_msg data_msg[2];
    int i2c_fd = 0;

	if ((id >= MAX_I2C_DEV)
            || (set_param == NULL)
            || (user_data == NULL)
            || (user_data->rtx_data == NULL))
    {
        return -1;
    }

    if((i2c_fd = open(s_i2c_dev[id], O_RDWR)) <= 0)
    {
        app_log_error("~~~~~~~~~~~[%s]%d, open i2c err!~~~~~~~~~~\n", __func__, __LINE__);
        return -1;
    }

	/* change endianness for ARM */
	if(g_bLittleEndian)
	{
		switch (set_param->address_width)
		{
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)(reg_address>>8);
				buf[1] = (unsigned char)reg_address;
				break;

			case 4:
				buf[0] = (unsigned char)(reg_address>>24);
				buf[1] = (unsigned char)(reg_address>>16);
				buf[2] = (unsigned char)(reg_address>>8);
				buf[3] = (unsigned char)reg_address;
				break;

				/* invalid address width */
			default:
				return -1;
		}
	}
	else /* CK */
	{
		switch (set_param->address_width)
		{
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				break;

			case 4:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				buf[2] = (unsigned char)(reg_address>>16);
				buf[3] = (unsigned char)(reg_address>>24);
				break;

				/* invalid address width */
			default:
				return -1;
		}
	}
    rdwrdata.nmsgs = 2;
    rdwrdata.msgs = data_msg;//(struct i2c_msg *)GxCore_Malloc(2 * sizeof(struct i2c_msg));

    //Send chip address.
    rdwrdata.msgs[0].addr = set_param->chip_address;
    rdwrdata.msgs[0].len = set_param->address_width;
    rdwrdata.msgs[0].buf = buf;
    if (set_param->send_stop && set_param->address_width)
        rdwrdata.msgs[0].flags = 0;
    else
        rdwrdata.msgs[0].flags = I2C_M_REV_DIR_ADDR;

    //Write data.
    rdwrdata.msgs[1].addr = set_param->chip_address;
    rdwrdata.msgs[1].len = user_data->count;
    rdwrdata.msgs[1].buf = user_data->rtx_data;
    if(set_param->send_noack && set_param->address_width)
        rdwrdata.msgs[1].flags = I2C_M_NOSTART;
    else
        rdwrdata.msgs[1].flags = 0;

    //don't send chip address for a8293, send stop at last
	if (!set_param->address_width)
		rdwrdata.msgs[1].flags |= I2C_M_NOSTART;

    ret = ioctl(i2c_fd, I2C_RDWR, &rdwrdata);

    close(i2c_fd);

    if (ret < 0)
		app_log_error("APP TX: Error during I2C_RDWR ioctl with error code: %d\n", ret);

	if (ret == 2)	// 2 msg success
		return user_data->count;
	else
		return -1;
}

int app_gxi2c_rx(unsigned int id, unsigned int reg_address, I2cSetParam *set_param, I2cUserData *user_data)
{
	int ret = 0;
	unsigned char buf[4] = {0};
    struct i2c_rdwr_ioctl_data rdwrdata;
    struct i2c_msg data_msg[2];
    int i2c_fd = 0;

    if ((id >= MAX_I2C_DEV)
            || (set_param == NULL)
            || (user_data == NULL)
            || (user_data->rtx_data == NULL))
    {
        return -1;
    }

    if((i2c_fd = open(s_i2c_dev[id], O_RDWR)) <= 0)
    {
        app_log_error("~~~~~~~~~~~[%s]%d, open i2c err!~~~~~~~~~~\n", __func__, __LINE__);
        return -1;
    }

	/* change endianness for ARM */
	if (g_bLittleEndian)
	{
		switch(set_param->address_width)
		{
			/* some chips needn't register address */
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)(reg_address>>8);
				buf[1] = (unsigned char)reg_address;
				break;

			case 4:
				buf[0] = (unsigned char)(reg_address>>24);
				buf[1] = (unsigned char)(reg_address>>16);
				buf[2] = (unsigned char)(reg_address>>8);
				buf[3] = (unsigned char)reg_address;
				break;

				/* invalid address width */
			default:
				return -1;
		}
	}
	else /* CK */
	{
		switch (set_param->address_width)
		{
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				break;

			case 4:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				buf[2] = (unsigned char)(reg_address>>16);
				buf[3] = (unsigned char)(reg_address>>24);
				break;

				/* invalid address width */
			default:
				return -1;
		}
	}

    rdwrdata.nmsgs = 2;
    rdwrdata.msgs = data_msg;//(struct i2c_msg *)GxCore_Malloc(2 * sizeof(struct i2c_msg));

    //Send chip address.
    rdwrdata.msgs[0].addr = set_param->chip_address;
    rdwrdata.msgs[0].len = set_param->address_width;
    rdwrdata.msgs[0].buf = buf;
    if(set_param->send_stop && set_param->address_width)
        rdwrdata.msgs[0].flags = 0;
    else
        rdwrdata.msgs[0].flags = I2C_M_REV_DIR_ADDR;

    if (!set_param->address_width)
        rdwrdata.msgs[0].flags |= I2C_M_RD;

    //Read data.
    rdwrdata.msgs[1].addr = set_param->chip_address;
    rdwrdata.msgs[1].len = user_data->count;
    rdwrdata.msgs[1].buf = user_data->rtx_data;
    if (set_param->send_noack)
        rdwrdata.msgs[1].flags = I2C_M_RD|I2C_M_NO_RD_ACK;
    else
        rdwrdata.msgs[1].flags = I2C_M_RD;

    if (!set_param->address_width)
		rdwrdata.msgs[1].flags |= I2C_M_NOSTART;

    ret = ioctl(i2c_fd, I2C_RDWR, &rdwrdata);

    //GxCore_Free(rdwrdata.msgs);
    close(i2c_fd);

    if (ret < 0)
		app_log_error("APP RX: Error during I2C_RDWR ioctl with error code: %d\n", ret);

	if (ret == 2)	// 2 msg success
		return user_data->count;
	else
		return -1;
}
#endif

#ifdef ECOS_OS
extern void *gx_i2c_open(unsigned int id);
extern int gx_i2c_set(void *dev, unsigned int busid, unsigned int chip_addr, unsigned int address_width, int send_noack, int send_stop);
extern int gx_i2c_tx(void *dev, unsigned int reg_address, const unsigned char *tx_data, unsigned int count);
extern int gx_i2c_rx(void *dev, unsigned int reg_address, unsigned char *rx_data, unsigned int count);
int app_gxi2c_tx(unsigned int id, unsigned int reg_address, I2cSetParam *set_param, I2cUserData *user_data)
{
    void *d_i2c = NULL;

    if ((id >= MAX_I2C_DEV)
            || (set_param == NULL)
            || (user_data == NULL)
            || (user_data->rtx_data == NULL))
    {
        return -1;
    }

    d_i2c = gx_i2c_open(id);
    if(d_i2c == NULL)
        return -1;

    gx_i2c_set(d_i2c, 0, set_param->chip_address, set_param->address_width, set_param->send_noack, set_param->send_stop);
    if(gx_i2c_tx(d_i2c, reg_address, user_data->rtx_data, user_data->count) == user_data->count)
        return user_data->count;
    else
        return -1;
}

int app_gxi2c_rx(unsigned int id, unsigned int reg_address, I2cSetParam *set_param, I2cUserData *user_data)
{
    void *d_i2c = NULL;

    if ((id >= MAX_I2C_DEV)
            || (set_param == NULL)
            || (user_data == NULL)
            || (user_data->rtx_data == NULL))
    {
        return -1;
    }

    d_i2c = gx_i2c_open(id);
    if(d_i2c == NULL)
        return -1;

    gx_i2c_set(d_i2c, 0, set_param->chip_address, set_param->address_width, set_param->send_noack, set_param->send_stop);
    if(gx_i2c_rx(d_i2c, reg_address, user_data->rtx_data, user_data->count) == user_data->count)
        return user_data->count;
    else
        return -1;
}
#endif

void app_i2c_test_demo(void)
{
#define DEMOD_I2C_ID    2
#define DEMOD_I2C_ADDR    0x80
#define DEMOD_REG_CHIPID_H    0x00
#define DEMOD_REG_CHIPID_L    0x01
#define DEMOD_REG_RST    0x30
#define DEMOD_RST_CMD    1

    uint8_t val = 0;
    uint16_t chip_id = 0;
    I2cSetParam i2c_set = {0};
    I2cUserData data = {0};

    //get demod chip id
    i2c_set.chip_address = DEMOD_I2C_ADDR;
    i2c_set.address_width = 1;
    i2c_set.send_noack = 1;
    i2c_set.send_stop = 1;

    data.count = 1;
    data.rtx_data = &val;

    app_gxi2c_rx(DEMOD_I2C_ID, DEMOD_REG_CHIPID_H, &i2c_set, &data);
    chip_id = (val << 8);
    app_gxi2c_rx(DEMOD_I2C_ID, DEMOD_REG_CHIPID_L, &i2c_set, &data);
    chip_id |= (val & 0xff);
   // app_log_debug("~~~~~~~~~~~~~~~~~[%s] demod id = 0x%x~~~~~~~~~~~~~~~~~\n", __func__, chip_id);

    //init demod
    i2c_set.chip_address = DEMOD_I2C_ADDR;
    i2c_set.address_width = 1;
    i2c_set.send_noack = 1;
    i2c_set.send_stop = 0;

    val = DEMOD_RST_CMD;
    data.count = 1;
    data.rtx_data = &val;

    app_gxi2c_tx(DEMOD_I2C_ID, DEMOD_REG_RST, &i2c_set, &data);
}
