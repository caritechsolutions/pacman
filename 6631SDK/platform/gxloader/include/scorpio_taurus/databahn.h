#ifndef _DATABAHN_H_
#define _DATABAHN_H_

#include <gxhwlib_registers.h>
#include "danali_register.h"

#define Databahn_Reg(x) (REG_BASE_DENALI - GX_REG_VIRTUAL_BASE1 + ((x)<<2))

struct reg_param {
	unsigned char reg;
	unsigned char offset;
	unsigned char width;
	unsigned short value;	/* in order to reduce the space of stage1 */
};

typedef enum {
	DDR_KGD_ESMT_LIJING = 0,
	DDR_KGD_ESMT_JINHUA = 1,
	DDR_KGD_WINBOND     = 2,
	DDR_KGD_ETRON       = 3,
	DDR_KGD_NANYA       = 4,
	DDR_KGD_GD          = 5
} DDR_KGD_TYPE;

struct ddr_patch {
	unsigned char kgd_type;
	unsigned int addr;
	unsigned int value;
};

#define OTP_DRAM_PARAM_LEN 7 /* bytes */

struct otp_dram_param {
	unsigned int  reg_addr;
	unsigned char reg_offset;
	unsigned char reg_bit_width;
	unsigned char otp_data_byte_index;
	unsigned char otp_data_bit_offset;
};

enum {
	DDR2 = 2,
	DDR3 = 3,
	DDR4 = 4
};

#endif
