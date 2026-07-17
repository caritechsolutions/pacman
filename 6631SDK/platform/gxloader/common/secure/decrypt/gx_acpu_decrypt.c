// porting from sirius rom code, modfiy later
#include "gx_acpu_decrypt.h"

static int byteswap( int x ){
    return (\
    ( ( x & 0x000000FF ) << 24 ) \
  | ( ( x & 0x0000FF00 ) << 8 ) \
  | ( ( x & 0x00FF0000 ) >> 8 ) \
  | ( ( x & 0xFF000000 ) >> 24 ) \
  );
}

static int gx_acpu_decrypt_set_DI(int *src, unsigned int data_length)
{
	int i,j;
	int ret = -1;

	if (data_length > 128)
		return ret;

	for (i = 0; i < data_length / 16; i++) {
		for (j = 0; j < 4; j++)
			(*(volatile unsigned int *)(CRYPTO_DI0 + j * 4)) = byteswap(src[3 - j]);

		(*(volatile unsigned int *)CRYPTO_CONTROL) |= 0x1<<4; // Di update
		src += 4;
	}

	ret = 0;
	return ret;
}

static int gx_acpu_decrypt_get_DO(int *dst, unsigned int data_length)
{
	int i,j;
	int ret = -1;

	if (data_length > 128)
		return ret;

	for (i = 0; i < data_length / 16; i++) {
		(*(volatile unsigned int *)CRYPTO_CONTROL) |= 0x1<<5; // Do update

		for ( j = 0; j < 4; j++)
			dst[3-j] = byteswap((*(volatile unsigned int *)(CRYPTO_DO0 + j * 4)));
		dst += 4;
	}

	ret = 0;
	return ret;
}

static int gx_acpu_decrypt_set_IV(void)
{
	int i;

	for (i = 0; i < 4; i++)
		(*(volatile unsigned int *)(CRYPTO_IV0 + i * 4)) = 0;

	return 0;
}

// dst is NULL when decrypt key
int gx_acpu_decrypt( int key_sel, unsigned int data_length, int *src, int *dst)
{
	unsigned int current_len = 0;
	int flash_key_alg_sel = 0;

	flash_key_alg_sel = OTP_GET_CFG_DAT(FLASH_ENCRYPT_ALG_SELECT);
	if (!flash_key_alg_sel)
		GX_CRYPTO_SET_ALGO_SM4();
	else
		GX_CRYPTO_SET_ALGO_AES128();

	if ( data_length == 0)
		return -1;
	(*(volatile unsigned int *)CRYPTO_DATA_LENGTH) = data_length;
	/*
	 * set decryption mode
	 * The encryption and decryption mode is determined according to the macro definition,
	 * and the hardware module has no clear control signal.
	 * Derived key for decryption when dst == NULL, only supports ECB mode.
	 * when dst != NULL decrypt data
	 */
	if (dst == 0) {
		GX_CRYPTO_SET_OPT_ECB();
		//rslt_dst -> private register1，Set the register to store the decrypted result
		GX_CRYPTO_SET_RSLT_DES(GX_CRYPTO_PRI_REG1);
	} else {
#ifdef CONFIG_DATA_ENCRYPT_TYPE_ECB
		GX_CRYPTO_SET_OPT_ECB();
#else
		GX_CRYPTO_SET_OPT_CBC();
		// conf IV0~IV3
		gx_acpu_decrypt_set_IV();
#endif// rslt_dst -> public register
		GX_CRYPTO_SET_RSLT_DES(GX_CRYPTO_PUB_REG);
	}

	GX_CRYPTO_SET_KEY(key_sel);
	if (key_sel == GX_CRYPTO_KEY_OTP)
		GX_CRYPTO_OTP_KEY_EN();
	else if (key_sel == GX_CRYPTO_KEY_BCMK || key_sel == GX_CRYPTO_KEY_BCK)
		GX_CRYPTO_BC_KEY_EN();
	GX_CRYPTO_SET_FIRST_BLOCK();
	GX_CRYPTO_SET_DECRYPT();

	//decrypto
	while (data_length) {
		if( data_length >= 128)
			current_len = 128;
		else
			current_len = data_length;

		gx_acpu_decrypt_set_DI(src,current_len); // Di[0]~Di[3]
		GX_CRYPTO_SET_START();
		// wait for decrypt done
		while (((*(volatile unsigned int *)CRYPTO_INTR) & 0x1) != 0x1);
		// clear intr flag
		(*(volatile unsigned int *)CRYPTO_INTR) |= 0x1;

		if (dst == 0)
			break;

		gx_acpu_decrypt_get_DO(dst,current_len); // Do[0]~Do[3]

		data_length -= current_len;
		src += current_len / 4;
		dst += current_len / 4;
	}
	return 0;
	// to clear internal keys even when not used(BCMK, BCK, expanded OTP Key)
	// because later decryption also need this key, we should NOT clear it here!!!
	//(*(volatile unsigned int *)CRYPTO_CONTROL) |= (0x1 << 23); // clear BCMK and BCK
	//(*(volatile unsigned int *)CRYPTO_CONTROL) |= (0x1 << 20); // clear expanded OTP Key

}

