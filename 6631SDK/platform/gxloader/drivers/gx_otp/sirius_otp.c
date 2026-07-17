#include "stdio.h"
#include "gxhwlib_registers.h"
#include "otp.h"

#define GX_OTP_CTRL      *(volatile unsigned int *)(REG_BASE_OTP + 0x00)
#define GX_OTP_STATUS    *(volatile unsigned int *)(REG_BASE_OTP + 0x04)

#define GX_OTP_CTRL_ADDRESS     (0)
#define GX_OTP_CTRL_WR_DATA     (16)
#define GX_OTP_CTRL_RD          (30)
#define GX_OTP_CTRL_WR          (31)

#define GX_OTP_STATUS_RD_DATA   (0)
#define GX_OTP_STATUS_BUSY      (8)
#define GX_OTP_STATUS_RD_VALID  (9)
#define GX_OTP_STATUS_RW_FAIL   (12)

static int otp_read_byte(unsigned int addr)
{
	int val = 0;

	while ((GX_OTP_STATUS >> GX_OTP_STATUS_BUSY) & 0x1);
	GX_OTP_CTRL &= ~(0xFFFF);
	GX_OTP_CTRL |= ((addr << 3) & 0xFFFF);
	GX_OTP_CTRL |= (1 << GX_OTP_CTRL_RD);

	while ((GX_OTP_STATUS >> GX_OTP_STATUS_BUSY) & 0x1);
	if ((GX_OTP_STATUS >> GX_OTP_STATUS_RD_VALID) & 0x1)
		val = (GX_OTP_STATUS >> GX_OTP_STATUS_RD_DATA) & 0xFF;

	GX_OTP_CTRL &= ~(1 << GX_OTP_CTRL_RD);

	if ((!((GX_OTP_STATUS >> GX_OTP_STATUS_RD_VALID) & 0x1)) || ((GX_OTP_STATUS >> GX_OTP_STATUS_RW_FAIL) & 0x1))
		return -1;

	return val;
}

int gx_otp_read(unsigned int start_addr, int rd_num, unsigned char *data)
{
	int i = 0;
	int val = 0;

	for (i = 0; i < rd_num; i++) {
		val = otp_read_byte(start_addr + i);
		if (val < 0)
			return val;
		else
			data[i] = val;
	}

	return rd_num;
}

#ifdef CONFIG_ENABLE_OTP_FULLFUNCTION
static int otp_write_byte(unsigned int addr, unsigned char val)
{
	while ((GX_OTP_STATUS >> GX_OTP_STATUS_BUSY) & 0x1);
	GX_OTP_CTRL &= ~(0xFFFF);
	GX_OTP_CTRL |= ((addr << 3) & 0xFFFF);

	GX_OTP_CTRL &= ~(0xFF << GX_OTP_CTRL_WR_DATA);
	GX_OTP_CTRL |= (val << GX_OTP_CTRL_WR_DATA);
	GX_OTP_CTRL |= (1 << GX_OTP_CTRL_WR);

	while ((GX_OTP_STATUS >> GX_OTP_STATUS_BUSY) & 0x1);

	GX_OTP_CTRL &= ~(1 << GX_OTP_CTRL_WR);

	if (((GX_OTP_STATUS >> GX_OTP_STATUS_RW_FAIL) & 0x1))
		return -1;

	return 0;
}

int gx_otp_write(unsigned int start_addr, int wr_num, unsigned char *data)
{
	int i = 0;
	int val = 0;

	for (i = 0; i < wr_num; i++) {
		val = otp_write_byte(start_addr + i, data[i]);
		if (val < 0)
			return val;
	}

	return wr_num;
}
#endif

bool gx_otp_query_flag(int flag)
{
#ifdef IS_SIRIUS_SERIES
	switch(flag) {
	case OTP_FLAG_DDR_CONTENT_LIMIT:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_DDR_CONTENT_LIMIT))
			return true;
		break;

	case OTP_FLAG_DDR_SCRAMBLE:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_DDR_SCRAMBLE))
			return true;
		break;

	case OTP_FLAG_TS_BUF_PROTECT:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_TS_BUF_PROTECT))
			return true;
		break;

	case OTP_FLAG_ES_BUF_PROTECT:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_ES_BUF_PROTECT))
			return true;
		break;

	case OTP_FLAG_FW_BUF_PROTECT:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_FW_BUF_PROTECT))
			return true;
		break;

#if defined (CONFIG_ARCH_ARM_SIRIUS) || defined (CONFIG_ARCH_ARM_CANOPUS)
	case OTP_FLAG_GP_BUF_PROTECT:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_GP_BUF_PROTECT))
			return true;
		break;
#endif

	case OTP_FLAG_AOUT_BUF_PROTECT:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_AOUT_BUF_PROTECT))
			return true;
		break;

	case OTP_FLAG_SVPU_BUF_PROTECT:
	case OTP_FLAG_VOUT_BUF_PROTECT:
		if (OTP_FLAG_IS_TRUE(BIT_OTP_FLAG_VOUT_BUF_PROTECT))
			return true;
		break;

	default:
		break;
	}
#endif
	return false;
}
