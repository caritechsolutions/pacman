#include "common/hash.h"
#include "config.h"
#include "rsa.h"
#ifdef CONFIG_PKA
#include "pka.h"
#endif

extern unsigned char otp_read_simple(unsigned int address);
extern unsigned char get_pkcs_header(int index);

#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
#define RSA_PUBLIC_KEY_SRAM_ADDR	(0x3CB0)	// rsa public key modulus addr in sram
#define RSA_PUBLIC_KEY_OTP_ADDR		(0x1F0)	        // rsa public key modulus addr in flash
#endif

int rsa_decrypt_sig(unsigned char *sig_addr, unsigned char *key_modulus, unsigned char *result)
{
	int i = 0, j = 0;
	uint8_t rsa_sig[COMMON_LENGTH];
	uint8_t rsa_result[COMMON_LENGTH];
	uint8_t rsa_modulus[COMMON_LENGTH];
	uint8_t *tmp_ptr = NULL;
#ifdef CONFIG_PKA
	for ( i = 0; i < COMMON_LENGTH; i++) {
		rsa_modulus[i] = key_modulus[COMMON_LENGTH - 1 - i];
		rsa_sig[i] = sig_addr[COMMON_LENGTH - 1 - i];
	}

	PKA_Montgomery_Pre(PKA_2048BIT_OPERAND_SIZE, (const uint32_t *)rsa_modulus);
	PKA_MODEXP(PKA_2048BIT_OPERAND_SIZE, (const uint32_t *)rsa_sig, (const uint32_t *)rsa_modulus, (uint32_t *)rsa_result);
	tmp_ptr = rsa_result;
#else
	rsa_dec(key_modulus, (uint8_t *)sig_addr, &tmp_ptr);
#endif
	//pkcs header compare
	for (i = 0; i < (COMMON_LENGTH - SHA256_HASH_SIZE); i++) {
		if(*(volatile unsigned char *)(tmp_ptr + COMMON_LENGTH - 1 - i) != get_pkcs_header(i))
			return -1;
	}

	for (; i < COMMON_LENGTH; i++) {
		*(volatile unsigned char *)(result + j) = *(volatile unsigned char *)(tmp_ptr + COMMON_LENGTH - 1 - i);
		j++;
	}

	return 0;
}

int rsa_pubkey_verif_fun(unsigned char *src_addr, int length, unsigned char *key_modulus, unsigned char *sig_addr)
{
	int err = 0;
	uint8_t hash[SHA256_HASH_SIZE];
	uint8_t rsa_hash[SHA256_HASH_SIZE];
	int i = 0;

	err = rsa_decrypt_sig(sig_addr, key_modulus, rsa_hash);
	if (err)
		return -1;

	gx_sha256(src_addr, length, hash);

	for (i = 0; i < SHA256_HASH_SIZE; i++) {
		if (hash[i] != rsa_hash[i])
			return -1;
	}

	return 0;
}

#if defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
int rsa_verif_fun(unsigned int src_addr, int length, unsigned int sig_addr)
{
	int i;
	uint8_t rsa_n[COMMON_LENGTH];

	if (REG_ENABLE_NDS_BOOT(44) == 0) { // level 1
		for ( i = 0; i < COMMON_LENGTH; i++)
			rsa_n[i] = otp_read_simple(RSA_PUBLIC_KEY_OTP_ADDR + i);
	} else { // level 2
		// get rsa public key modulus from flash
		for ( i = 0; i < COMMON_LENGTH; i++)
			rsa_n[i] = *(volatile unsigned char *)(SRAMBASE + RSA_PUBLIC_KEY_SRAM_ADDR + i);
	}

	return rsa_pubkey_verif_fun((uint8_t *)src_addr, length, rsa_n, (uint8_t *)sig_addr);
}
#endif
