
#ifndef __GX_OTP_H__
#define __GX_OTP_H__

#define OTP_FLAG_DDR_CONTENT_LIMIT (0)
#define OTP_FLAG_DDR_SCRAMBLE      (1)
#define OTP_FLAG_TS_BUF_PROTECT    (2)
#define OTP_FLAG_ES_BUF_PROTECT    (3)
#define OTP_FLAG_FW_BUF_PROTECT    (4)
#define OTP_FLAG_GP_BUF_PROTECT    (5)
#define OTP_FLAG_SVPU_BUF_PROTECT  (6)
#define OTP_FLAG_AOUT_BUF_PROTECT  (7)
#define OTP_FLAG_VOUT_BUF_PROTECT  (8)
#define OTP_FLAG_SCPU_BUF_PROTECT  (9)

int gx_otp_read(unsigned int start_addr, int rd_num, unsigned char *data);
int gx_otp_write(unsigned int start_addr, int wr_num, unsigned char *data);
bool gx_otp_query_flag(int flag);

#endif
