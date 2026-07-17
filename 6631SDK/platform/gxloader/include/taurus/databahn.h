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
