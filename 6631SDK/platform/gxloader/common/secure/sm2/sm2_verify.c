#include "sm2.h"
#include "common/hash.h"
#include "config.h"
#include "stdio.h"
#ifdef CONFIG_PKA
#include "pka.h"
#endif

extern unsigned char otp_read_simple(unsigned int address);

#define COMMON_LENGTH		(32)
#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
#define SM2_INFO_OTP_ADDR	(0x1F0)		// sm2 info in otp addr

#define SM2_P_X_OTP_ADDR	(0x02B0)	// x coordinate of public key addr in otp
#define SM2_P_Y_OTP_ADDR	(0x02D0)	// y coordinate of public key addr in otp
#define SM2_P_X_SRAM_ADDR	(0x3D70)	// x coordinate of public key addr in sram
#define SM2_P_Y_SRAM_ADDR	(0x3D90)	// y coordinate of public key addr in sram

#define SM2_ENTLA_SRAM_ADDR	(0x3CB0)	// ENTLA addr in sram
#define SM2_IDA_BOOT_ADDR	(0x3CB2)	// IDA addr in sram
#define SM2_ENTLA_LEN	(2)		// ENTLA len in flash
#define SM2_IDA_MAX_LEN	(158)		// IDA max len in flash

#define COPRO_BASE		(0x88600000)
#endif

#define TMP_BUFFER_LEGTH	(SM2_ENTLA_LEN + SM2_IDA_MAX_LEN + COMMON_LENGTH * 6 + COMMON_LENGTH)

struct sm2_otp {
	unsigned int n[8];
	unsigned int p[8];
	unsigned int a[8];
	unsigned int b[8];
	unsigned int xG[8];
	unsigned int yG[8];
	unsigned int xP[8];
	unsigned int yP[8];
	unsigned int r[8];
	unsigned int s[8];
	unsigned int hash[8];
};

static void sm2_load_from_otp(char * dst, unsigned int otp_addr)
{
	int i;
	dst += 32;
	for(i = 0; i < 32; i++)
		*(--dst) = otp_read_simple(otp_addr++);
}

unsigned long get_copro_base_address(void)
{
	return COPRO_BASE;
}

int sm2_verif_fun(unsigned int src_addr, int length, unsigned int sig_addr)
{
	int err = 0;
	int i = 0;
	int j = 0;
	unsigned int result = 0;
	unsigned int len = 0;
	unsigned int tmp_len = TMP_BUFFER_LEGTH;
	unsigned int addr = 0;
	struct sm2_otp sm2_info;
	unsigned char *dest_addr = (unsigned char *)&sm2_info;
	unsigned char *tmp_addr = 0;
	unsigned char tmp = 0;

	// read sm2 info from otp
	addr = SM2_INFO_OTP_ADDR;
	for (i = 0; i < 6; i++) {
		sm2_load_from_otp(dest_addr, addr);
		dest_addr += COMMON_LENGTH;
		addr += COMMON_LENGTH;
	}

	if (REG_ENABLE_NDS_BOOT(44)) { // level 2
		dest_addr = (unsigned char *)&sm2_info.xP;
		addr = SM2_P_X_SRAM_ADDR;
		for (i = 0; i < COMMON_LENGTH; i++)
			*(dest_addr + i) = *(volatile unsigned char *)(SRAMBASE + addr + COMMON_LENGTH - 1 - i);

		dest_addr = (unsigned char *)&sm2_info.yP;
		addr = SM2_P_Y_SRAM_ADDR;
		for (i = 0; i < COMMON_LENGTH; i++)
			*(dest_addr + i) = *(volatile unsigned char *)(SRAMBASE + addr + COMMON_LENGTH - 1 - i);
	} else { // level 1
		addr = SM2_P_X_OTP_ADDR;
		sm2_load_from_otp((char *)&sm2_info.xP, addr);
		addr = SM2_P_Y_OTP_ADDR;
		sm2_load_from_otp((char *)&sm2_info.yP, addr);
	}

	// stage2 sign info
	dest_addr = (unsigned char *)&sm2_info.r;
	for (i = 0; i < COMMON_LENGTH; i++)
		*(dest_addr + i) = *((unsigned char *)sig_addr + COMMON_LENGTH - 1 - i);	// stage2 sign of R

	dest_addr = (unsigned char *)&sm2_info.s;
	for (i = 0; i < COMMON_LENGTH; i++)
		*(dest_addr + i) = *((unsigned char *)sig_addr + COMMON_LENGTH + COMMON_LENGTH - 1 - i);	// stage2 sign of S

	// sm2 fixed value
	// ENTLA and IDA
	dest_addr = (unsigned char *)(src_addr - tmp_len);
	len = 2;
	addr = SM2_ENTLA_SRAM_ADDR;
	for (i = 0; i < len; i++)
		dest_addr[i] = *(volatile unsigned char *)(SRAMBASE + addr + i);

	result = (dest_addr[0] << 8) | (dest_addr[1]);
	result = result / 8;
	if (result > SM2_IDA_MAX_LEN) {
		err = -1;
		goto exit;
	}

	tmp_len = TMP_BUFFER_LEGTH - SM2_IDA_MAX_LEN + result;
	dest_addr = (unsigned char *)(src_addr - tmp_len);
	len = SM2_ENTLA_LEN + result;
	for (i = 0; i < len; i++)
		dest_addr[i] = *(volatile unsigned char *)(SRAMBASE + addr + i);

	// a/b/Gx/Gy/Px/Py
	tmp_addr = (unsigned char *)(&sm2_info.a);
	for (i = 0; i < 6; i++) {
		for (j = 0; j < COMMON_LENGTH; j++) {
			*(dest_addr + SM2_ENTLA_LEN + result + i * COMMON_LENGTH + j) = *(tmp_addr + 31 - j);
		}
		tmp_addr += COMMON_LENGTH;
	}

	// fixed value hash
	gx_sm3((int *)dest_addr, tmp_len - COMMON_LENGTH, (int *)(dest_addr + tmp_len - COMMON_LENGTH));

	// stage2 hash info
	gx_sm3((int *)(dest_addr + tmp_len - COMMON_LENGTH), length  + COMMON_LENGTH, (int *)&sm2_info.hash);

	dest_addr = (unsigned char *)&sm2_info.hash;
	tmp_addr = (unsigned char *)&sm2_info.hash + 31;
	for (i = 0; i < COMMON_LENGTH/2; i++) {
		tmp = *tmp_addr;
		*tmp_addr = *dest_addr;
		*dest_addr = tmp;
		tmp_addr--;
		dest_addr++;
	}

	err = sm2_verif((const unsigned int *)&sm2_info.p,
			(const unsigned int *)&sm2_info.a,
			(const unsigned int *)&sm2_info.b,
			COMMON_LENGTH / 4,
			(const unsigned int *)&sm2_info.n,
			COMMON_LENGTH / 4,
			2,
			(const unsigned int *)&sm2_info.xG,
			(const unsigned int *)&sm2_info.yG,
			(const unsigned int *)&sm2_info.xP,
			(const unsigned int *)&sm2_info.yP,
			(const unsigned int *)&sm2_info.r,
			(const unsigned int *)&sm2_info.s,
			(const unsigned int *)&sm2_info.hash,
			COMMON_LENGTH / 4,
			&result
		       );

	if ((result != 1) || (err != 0))
		err = -1;
exit:
	return err;
}

/**
  Initialize the PRNG
 * \return 0 on success, nonzero otherwise
 */
// For compiling
int prng_init(void)
{
	return 0;
}

/**
  Get random bytes from the PRNG. Puts len random bytes into buff
 * \param buff the buffer to put the random bytes
 * \param len the number of random bytes to get
 * \return 0 on success, nonzero otherwise
 */
// For compiling
int prng_get_random_bytes(unsigned char *buff, unsigned int len)
{
	return 0;
}

#ifdef CONFIG_PKA
int sm2_verif(const unsigned int * const p,
                          const unsigned int * const a,
                          const unsigned int * const b,
                          unsigned int size_p_in_words,
                          const unsigned int * const n,
                          unsigned int size_n_in_words,
                          unsigned int h,
                          const unsigned int * const x_G,
                          const unsigned int * const y_G,
                          const unsigned int * const x_P,
                          const unsigned int * const y_P,
                          const unsigned int * const r,
                          const unsigned int * const s,
                          const unsigned int * const m,
                          unsigned int size_m_in_words,
                          unsigned int * p_result)
{
	int i = 0;
	struct pka_ecc_pmult_param ecc_pmult_param;
	struct pka_ecc_padd_param ecc_padd_param;
	unsigned int ecc_temp0[8] = {-1};
	unsigned int ecc_temp1[8] = {-1};
	unsigned int ecc_temp2[8] = {-1};
	unsigned int ecc_temp3[8] = {-1};
	unsigned int ecc_temp4[8] = {-1};

	if (p == NULL || a == NULL || b == NULL || n == NULL ||
	    x_G == NULL || y_G == NULL || x_P == NULL ||
	    y_P == NULL || r == NULL || s == NULL || m == NULL ||
	    p_result == NULL)
		return -1;

	*p_result = 0;

	/* B1 Check whether r is in the interval [1, n-1] */
	for (i = 0; i < size_n_in_words; i++) {
		if (r[i] >= 1)
			break;
	}
	if (i == size_n_in_words)
		return -1;
	for (i = size_n_in_words - 1; i >= 0; i--) {
		if (r[i] == (n[i] - 1))
			continue;
		else if (r[i] < (n[i] - 1))
			break;
		else /* > */
			return -1;
	}

	/* B2 Check whether s is in the interval [1, n-1] */
	for (i = 0; i < size_n_in_words; i++) {
		if (s[i] >= 1)
			break;
	}
	if (i == size_n_in_words)
		return -1;
	for (i = size_n_in_words - 1; i >= 0; i--) {
		if (s[i] == (n[i] - 1))
			continue;
		else if (s[i] < (n[i] - 1))
			break;
		else /* > */
			return -1;
	}


	/* B3 and B4 have be done before call this function */
	PKA_Montgomery_Pre(PKA_256BIT_OPERAND_SIZE, p);
	/* B5 t = (r+s) mod n
	 * t == ecc_temp0
	 * */
	PKA_MODADD(PKA_256BIT_OPERAND_SIZE, r, s, n, ecc_temp0);
	for (i = 0; i < size_n_in_words; i++) {
		if (ecc_temp0[i] != 0)
			break;
	}
	if (i == size_n_in_words) /* indicate t == 0 */
		return -1;

	/* sG */
	ecc_pmult_param.Px = x_G;
	ecc_pmult_param.Py = y_G;
	ecc_pmult_param.a = a;
	ecc_pmult_param.k = s;
	ecc_pmult_param.p = p;
	PKA_ECC_PMULT(PKA_256BIT_OPERAND_SIZE, &ecc_pmult_param, ecc_temp1, ecc_temp2);
	/* tP */
	ecc_pmult_param.Px = x_P;
	ecc_pmult_param.Py = y_P;
	ecc_pmult_param.a = a;
	ecc_pmult_param.k = ecc_temp0;
	ecc_pmult_param.p = p;
	PKA_ECC_PMULT(PKA_256BIT_OPERAND_SIZE, &ecc_pmult_param, ecc_temp3, ecc_temp4);
	/* B6 sG + tP
	 * x == ecc_temp1  y == ecc_temp2 */
	ecc_padd_param.Px = ecc_temp1;
	ecc_padd_param.Py = ecc_temp2;
	ecc_padd_param.Qx = ecc_temp3;
	ecc_padd_param.Qy = ecc_temp4;
	ecc_padd_param.a = a;
	ecc_padd_param.p = p;
	PKA_ECC_PADD(PKA_256BIT_OPERAND_SIZE, &ecc_padd_param, ecc_temp1, ecc_temp2);
	/* B7 R = (e'+x') mod n
	 * R == ecc_temp0
	 * */
	PKA_MODADD(PKA_256BIT_OPERAND_SIZE, m, ecc_temp1, n, ecc_temp0);

	for (i = 0; i < size_n_in_words; i++) {
		if (r[i] != ecc_temp0[i])
			return -1;
	}
	if (i != size_n_in_words)
		return -1;
	else
		*p_result = 1;

	return 0;
}
#endif
