#ifndef __APP_I2C_H__
#define __APP_I2C_H__

typedef struct
{
    unsigned int chip_address;
    unsigned int address_width;
    int send_noack;
    int send_stop;
}I2cSetParam;

typedef struct
{
    unsigned int count;
    unsigned char *rtx_data;
}I2cUserData;

int app_gxi2c_tx(unsigned int id, unsigned int reg_address, I2cSetParam *set_param, I2cUserData *user_data);
int app_gxi2c_rx(unsigned int id, unsigned int reg_address, I2cSetParam *set_param, I2cUserData *user_data);
#endif
