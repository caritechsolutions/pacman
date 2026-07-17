#ifndef __GX_ACPU_DECRYPT_H
#define __GX_ACPU_DECRYPT_H
#include "gxhwlib_registers.h"

#define OTP_GET_CFG_DAT(offset) REG_ENABLE_NDS_BOOT(offset)
#define CRYPTO_DI0                  (REG_BASE_CRYPTO + 0x0)
#define CRYPTO_DO0                  (REG_BASE_CRYPTO + 0x10)
#define CRYPTO_DATA_LENGTH          (REG_BASE_CRYPTO + 0x200)
#define CRYPTO_CONTROL              (REG_BASE_CRYPTO + 0x300)
#define CRYPTO_IV0                  (REG_BASE_CRYPTO + 0x450)
#define CRYPTO_INTR                 (REG_BASE_CRYPTO + 0x604)
//Select the type of key to use when encrypting and decrypting data
typedef enum {
    GX_CRYPTO_KEY_BCMK,
    GX_CRYPTO_KEY_BCK,
    GX_CRYPTO_KEY_IMK,
    GX_CRYPTO_KEY_IK,
    GX_CRYPTO_KEY_REG1,
    GX_CRYPTO_KEY_REG2,
    GX_CRYPTO_KEY_REG3,
    GX_CRYPTO_KEY_CCCK,
    GX_CRYPTO_KEY_OTP,
    GX_CRYPTO_KEY_SOFT,
} GxCryptoKey;
//it is used to select four registers for storing result, three private registers,and one public register.
typedef enum {
    GX_CRYPTO_PRI_REG1,
    GX_CRYPTO_PRI_REG2,
    GX_CRYPTO_PRI_REG3,
    GX_CRYPTO_PUB_REG,
} GxCryptoRslt;
//Obtain the value of the corresponding OTP flag by reading the data of the corresponding address of the register.
typedef enum {
	ACPU_FLASH_ENCRYPT_OTP_KEY_MODE_ENABLE = 46,
	BOOT_CODE_ENC_ENFORCEMENT_ENABLE = 62,
	ACPU_FLASH_ENCRYPT_EXT_KEY_ENABLE = 77,
	FLASH_ENCRYPT_ALG_SELECT = 80,
	FLASH_ENCRYPT_KEY_SELECT = 82,
}OTP_CFG_DATA_OFFSET;

#define GX_CRYPTO_SET_ENCRYPT()         (*(volatile unsigned int *)CRYPTO_CONTROL |= (1 << 31))
#define GX_CRYPTO_SET_DECRYPT()         (*(volatile unsigned int *)CRYPTO_CONTROL  &= ~(1 << 31))

#define GX_CRYPTO_SET_START()           (*(volatile unsigned int *)CRYPTO_CONTROL  |= (1 << 0))
#define GX_CRYPTO_OTP_KEY_EN()          (*(volatile unsigned int *)CRYPTO_CONTROL  |= (1 << 26))
#define GX_CRYPTO_BC_KEY_EN()           (*(volatile unsigned int *)CRYPTO_CONTROL  |= (1 << 24))
#define GX_CRYPTO_SET_FIRST_BLOCK()     (*(volatile unsigned int *)CRYPTO_CONTROL  |= (1 << 12))
#define GX_CRYPTO_SET_ALGO_AES128()     (*(volatile unsigned int *)CRYPTO_CONTROL  &= ~(1 << 29))
#define GX_CRYPTO_SET_ALGO_SM4()        (*(volatile unsigned int *)CRYPTO_CONTROL  |= (1 << 29))
#define GX_CRYPTO_SET_OPT_ECB()         (*(volatile unsigned int *)CRYPTO_CONTROL  &= ~(1 << 28))
#define GX_CRYPTO_SET_OPT_CBC()         (*(volatile unsigned int *)CRYPTO_CONTROL  |= (1 << 28))
#define GX_CRYPTO_SET_KEY(value)        {*(volatile unsigned int *)CRYPTO_CONTROL  &= ~(0xf << 16);\
                                        *(volatile unsigned int *)CRYPTO_CONTROL |= (value << 16);}

#define GX_CRYPTO_SET_RSLT_DES(value)   {*(volatile unsigned int *)CRYPTO_CONTROL  &= ~(3 << 8);\
                                        *(volatile unsigned int *)CRYPTO_CONTROL  |= (value << 8);}

int gx_acpu_decrypt( int key_sel, unsigned int data_length, int *src, int *dst);
#endif
