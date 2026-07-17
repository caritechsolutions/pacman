#ifndef __I2C__
#define __I2C__

struct i2c_msg {
	unsigned short addr;
	unsigned short flags;

#define I2C_M_RD           0x0001
#define I2C_M_TEN          0x0010
#define I2C_M_RECV_LEN     0x0400
#define I2C_M_NO_RD_ACK    0x0800
#define I2C_M_IGNORE_NAK   0x1000
#define I2C_M_REV_DIR_ADDR 0x2000
#define I2C_M_NOSTART      0x4000

	unsigned short len;
	uint8_t        *buf;
};

void *gx_i2c_open(uint32_t id);
int gx_i2c_set(void *dev, uint32_t busid, uint32_t chip_addr, uint32_t address_width,
		int send_noack, int send_stop);
int gx_i2c_tx(void *dev, uint32_t reg_address, const uint8_t *tx_data, uint32_t count);
int gx_i2c_transfer(void *dev, struct i2c_msg *msgs, int num);
int gx_i2c_rx(void *dev, uint32_t reg_address, uint8_t *rx_data, uint32_t count);
int gx_i2c_clk_conf(void *dev, uint32_t clk_value);
int gx_i2c_close(void *dev);

#endif

