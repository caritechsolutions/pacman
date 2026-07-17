#include "gx_acpu_mtc.h"

void gx_acpu_mtc_decrypt(unsigned int *src_addr, unsigned int length, unsigned int *dst_addr)
{
#if !defined(CONFIG_ARCH_ARM_SIRIUS) && !defined(CONFIG_ARCH_ARM_CANOPUS)
	// length: bytes to decrypt in bytes
	volatile struct mtc *mtcp = (volatile struct mtc*)MTC_BASE_ADDR;
	unsigned int j;
	unsigned int word_per_buf;

	// 因硬件在调用AES加解密时存在出错的情况，所以每次使用前最好重置一下
	// // 3211中存在 AES 连续解密出错问题，需 MTC_RST_CONTRL(0X60) 寄存器的第5bit需重置(硬件bug)
	volatile int count = 5000;
	mtcp->rst_ctrl = 0xff;
	while(count--);
	mtcp->rst_ctrl = 0x0;
	// Clear Mode
	length &= 0xfffffff0;
	// from OTP flash KEY
	mtcp->keysel = 7;
	// Flash encrypt mode, CBC, AES-128
	mtcp->ctrl1 |= (1<<29); // Flash encrypt mode
	mtcp->ctrl1 |= (1<<7);  // CBC
	mtcp->ctrl1 |= (1<<5);  //AES
	mtcp->ctrl1 &= ~(3<<3); //128

	mtcp->len = length;

	mtcp->iv3 = 0;
	mtcp->iv2 = 0;
	mtcp->iv1 = 0;
	mtcp->iv0 = 0;

	// key ready
	mtcp->ctrl1 |= (1<<2);
	// AES core ready
	mtcp->ctrl2 = 0<<1;
	mtcp->ctrl2 = 1<<1;

	length >>= 2;

	while(length) {
		if( length < 16 )
			word_per_buf = length;
		else
			word_per_buf = 16;

		length -= word_per_buf;

		while( (mtcp->status & 0x1) );
		for(j=0; j<word_per_buf; j++) {
			mtcp->wdata = *src_addr;
			src_addr++;
		}

		while( (mtcp->status & 0x2) );
		for(j=0; j<word_per_buf; j++) {
			*dst_addr = mtcp->rdata;
			dst_addr++;
		}
	}
	mtcp->ctrl2 = 0;
	mtcp->ctrl1 = 0;
	mtcp->isr = 0xff;
#endif
}

#define M2M_SET_DESC_LIST(list, buf, len) \
	do { \
		((struct _m2m_desc *)(list))->start_addr = swab32((unsigned int)buf); \
		((struct _m2m_desc *)(list))->length     = swab32(len); \
		((struct _m2m_desc *)(list))->conf       = swab32(0x3); \
		((struct _m2m_desc *)(list))->next       = (void *)swab32((unsigned int)list); \
	} while(0);

static int _m2m_crypto(GxTfmCrypto *param, int encrypt)
{
#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
	int i = 0, iv_len = 0, key_len;
	unsigned int value = 0;
	static int init = 0;
	static unsigned char *list = NULL;
	volatile struct m2m *reg = (volatile struct m2m*)REG_BASE_M2M;


	if (init == 0) {
		reg->m2m_int_en = 0;
		reg->space_err    = 0xff;
		reg->almost_full  = 0xff;
		reg->almost_empty = 0xff;
		reg->key_err      = 0xff;
		reg->sync_err     = 0xff;
		reg->dma_done     = 0xff;
		reg->sync_byte_err_threshold = 3;
		reg->key_switch_threshold    = 3;
		reg->almost_empty_threshold  = 0x200;
		reg->almost_full_threshold   = 0x800;
		reg->ctrl = (0x5<<24) | (0x1<<8) | (0x1<<0); // small endian | channel 0 enable
		list = (unsigned char *)page_malloc(0x800);
		if(list == NULL)
		{
			printf("malloc list failed\n");
			return -1;
		}
		memset(list, 0, 0x800);
		dcache_flush();
		init = 1;
	}

	iv_len = param->iv_len / 4;
	if (param->opt == TFM_OPT_CBC) {
		for (i = 0; i < iv_len; i++) {
			memcpy(&value, &(param->iv[4*i]), 4);
			reg->iv[iv_len-i-1] = swab32(value);
		}
		reg->iv_update = 1;
		reg->iv_update = 0;
	}
	reg->channels[0].ctrl = (param->opt<<24) | (param->alg<<27); //config mode and alg

	key_len = param->soft_key_len / 4;
	if (param->key == TFM_KEY_ACPU_SOFT){
		reg->channels[0].ctrl |= (17<<16); // acpu_soft_key
		for (i = 0; i < key_len; i++) {
			memcpy(&value, &(param->soft_key[4*i]), 4);
			reg->soft_key[key_len-i-1] = swab32(value);
		}
		reg->soft_key_update = 1;
		reg->soft_key_update = 0;
	} else
		reg->channels[0].ctrl |= (param->key<<16); // otp_soft_key/otp_firm_key

	if (encrypt)
		reg->channels[0].ctrl |= 0x1 << 31; // encrypt
	else
		reg->channels[0].ctrl &= ~(0x1 << 31); // decrypt


	reg->channels[0].ctrl |= 0x1;  // update key
	reg->channels[0].ctrl &= ~(0x1);
	reg->channels[0].src_chain_ptr = (unsigned int)list;
	reg->channels[0].dst_chain_ptr = (unsigned int)list+0x400;
	reg->channels[0].weight = 8;
	reg->channels[0].data_len = param->data_len;
	dcache_flush();

	M2M_SET_DESC_LIST(list, param->src_addr, param->data_len);
	M2M_SET_DESC_LIST(list+0x400, param->dst_addr, param->data_len);

	dcache_flush();
	reg->channels[0].src_update_size = param->data_len;
	reg->channels[0].dst_update_size = param->data_len;
	reg->header_update = 0x1 | (0x1<<8);

	while ((reg->m2m_int & (~0x18)) == 0);
	if ((reg->m2m_int & (0x1<<0)) == 0) { // ISR is not dma_done
		reg->space_err    = 0xff;
		reg->almost_full  = 0xff;
		reg->almost_empty = 0xff;
		reg->key_err      = 0xff;
		reg->sync_err     = 0xff;
		printf("m2m isr error = %x\n", reg->m2m_int);
		return -1;
	}
	reg->dma_done = 0xff;
	dcache_flush();
#endif
	return 0;
}

int gx_acpu_m2m_encrypt(GxTfmCrypto *param)
{
	return _m2m_crypto(param, 1);
}

int gx_acpu_m2m_decrypt(GxTfmCrypto *param)
{
	return _m2m_crypto(param, 0);
}

