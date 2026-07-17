#include "config.h"

#define SHA256_HASH_WORDS 8

#define HASH_MSG_DI					(REG_BASE_HASH + 0x0000)
#define HASH_CONTROL1					(REG_BASE_HASH + 0x0100)
#define HASH_CONTROL2					(REG_BASE_HASH + 0x0104)
#define HASH_STATUS					(REG_BASE_HASH + 0x0108)
#define HASH_INTR					(REG_BASE_HASH + 0x0154)
#define HASH_VAL_H0					(REG_BASE_HASH + 0x1020)
#define HASH_SM2VAL_H0					(REG_BASE_HASH + 0x1040)

#define DATA_ENDIAN
#define HASH_ENDIAN

static int gx_sha256_set_DI(int *data, unsigned int len, int final_block)
{
	int i;
	int ctrl2;
	int err = 0;

	ctrl2 = 0x0;

	if (len > 64) {
		err = -1;
		goto exit;
	}

	if (final_block) {
		if (len == 64) {
			for (i = 0; i < 16; i++)
				(*(volatile unsigned int *)HASH_MSG_DI) = data[i];
			ctrl2 |= (0x1 << 1); // message finish
			(*(volatile unsigned int *)HASH_CONTROL2) = ctrl2;
		} else {
			for (i = 0; i < len/4; i++)
				(*(volatile unsigned int *)HASH_MSG_DI) = data[i];

			if (len % 4 == 1)
				ctrl2  = ((ctrl2 & 0xfffffff3) | (0x1 << 2)); // 1 byte  vld
			else if (len % 4 == 2)
				ctrl2  = ((ctrl2 & 0xfffffff3) | (0x2 << 2)); // 2 bytes vld
			else if (len % 4 == 3)
				ctrl2  = ((ctrl2 & 0xfffffff3) | (0x3 << 2)); // 3 bytes vld

			ctrl2 |= (0x1 << 1); // message finish
			(*(volatile unsigned int *)HASH_CONTROL2) = ctrl2;
			if (len % 4 != 0)
				(*(volatile unsigned int *)HASH_MSG_DI) = data[i]; // last data
		}
	} else {
		for (i = 0; i < 16; i++)
			(*(volatile unsigned int *)HASH_MSG_DI) = data[i];
	}

	err = 0;
exit:
	return err;
}

static void gx_sha256_get_DO(int *hash, int sha_sm_n)
{
	int i;
	int addr = sha_sm_n ? HASH_VAL_H0 : HASH_SM2VAL_H0;

	for (i = 0; i < 8; i++)
		hash[i] = (*(volatile unsigned int *)(addr + i * 4));
}

int gx_hw_sha256(int *data, unsigned int len, int *hash, int sha_sm_n)
{
	int ctrl1;
	int ctrl2;
	unsigned int cur_len;
	int final_block;
	int hash_done;
	int err = -1;

	ctrl1 = 0x0;
	ctrl2 = 0x0;

	ctrl1  = (ctrl1 & 0xffff00ff) | ((sha_sm_n ? 0x2 : 0x4) << 8); // sha-256 alg
#ifdef DATA_ENDIAN
	ctrl1 |= 0x1 << 1; // change data endian
#endif

#ifdef HASH_ENDIAN
	ctrl1 |= 0x1; // change hash endian
#endif
	(*(volatile unsigned int *)HASH_CONTROL1) = ctrl1;
	ctrl2 |= 0x1; // message start
	(*(volatile unsigned int *)HASH_CONTROL2) = ctrl2;
	ctrl2 &= ~(0x1); // clear message start

	while (len) {
		if(len > 64) {
			cur_len = 64;
			final_block = 0;
		} else {
			cur_len = len;
			final_block = 1;
		}

		err = gx_sha256_set_DI(data, cur_len, final_block);
		if (err < 0)
			goto exit;

		while ((*(volatile unsigned int *)HASH_STATUS) & 0x1);

		len -= cur_len;
		data += cur_len/4;
	}

	do {
		if (sha_sm_n)
			hash_done = ((*(volatile unsigned int *)HASH_INTR) >> 1);
		else
			hash_done = ((*(volatile unsigned int *)HASH_INTR) >> 2);
	} while ((hash_done & 0x1) != 0x1); // wait for hash done

	(*(volatile unsigned int *)HASH_CONTROL2) = 0;
	gx_sha256_get_DO(hash,sha_sm_n);

	if (sha_sm_n)
		(*(volatile unsigned int *)HASH_INTR) |= (0x1 << 1); // clear intr flag
	else
		(*(volatile unsigned int *)HASH_INTR) |= (0x1 << 2);


	err = 0;
exit:
	return err;
}

