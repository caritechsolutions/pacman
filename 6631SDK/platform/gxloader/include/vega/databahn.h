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

struct otp_dram_param {
	unsigned int  reg_addr;
	unsigned char reg_offset;
	unsigned char reg_bit_width;
	unsigned char otp_data_byte_index;
	unsigned char otp_data_bit_offset;
};

void Databahn_Init(void);
void Databahn_SetParameters(struct reg_param *regs, int count);
void Databahn_SetParameter(unsigned int reg, unsigned int offset, unsigned int width, unsigned int value);
unsigned int Databahn_GetParameter(unsigned int reg, unsigned int offset, unsigned int width);

unsigned int Databahn_GetIntStatus(void);
unsigned int Databahn_isReady(void);
unsigned int Databahn_GetAltMemPhyParamter(void);
unsigned int Databahn_isAltMemPhyReady(void);
void Databahn_FPGA(void);
//void Databahn_BIST(void);
unsigned int Databahn_isBISTComplete(void);
void Databahn_ClearBISTInt(void);
void Databahn_Config(void);

#endif
