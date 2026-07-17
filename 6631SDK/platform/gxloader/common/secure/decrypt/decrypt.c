#include "config.h"
#include "decrypt.h"
#include "sys/types.h"

#if defined (CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
#include "gx_acpu_decrypt.h"
#define KEY_LEN                         (16)
#define STAGE2_KEY_OFFSET               (6)
#define STAGE2_ENCRYPT_FLAG_OFFSET      (2)
#elif defined (CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI)
#include "gx_acpu_mtc.h"
#endif

int decrypt(unsigned char *data_addr, int data_len, unsigned char *addition_buf)
{
#if defined (CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_ARM_CANOPUS)
	int flash_key_sel = 0;
	int boot_code_enc = 0;
	int acpu_flash_enc_otp_key = 0;
	int acpu_flash_enc_otp_key_exp_en = 0;
	int key_sel = -1;
	int derive_key_sel = -1;
	int ret = -1;
	int stage2_encrypt_flag = 0;

	flash_key_sel = OTP_GET_CFG_DAT(FLASH_ENCRYPT_KEY_SELECT);
	/*
	 * boot_code_enc used for irdeto mode,BCMK from KDS
	 * boot_code_enc = 1 enable the bootloader encrypt function,root key is BCMK
	 * boot_code_enc = 0 not open the bootloader encrypt function
	 */
	boot_code_enc = OTP_GET_CFG_DAT(BOOT_CODE_ENC_ENFORCEMENT_ENABLE);
	/*
	 * otp encryption function
	 * acpu_flash_enc_otp_key = 1 enable the ACPU flash encryption function,the key come from the otp
	 * acpu_flash_enc_otp_key = 0 disable the ACPU flash
	 */
	acpu_flash_enc_otp_key = OTP_GET_CFG_DAT(ACPU_FLASH_ENCRYPT_OTP_KEY_MODE_ENABLE);
	/*
	 * otp derive enable
	 * acpu_flash_enc_otp_key_exp_en = 1 ACPU flash encryption key is in flash,the key must used the OTP key decrypt
	 * acpu_flash_enc_otp_key_exp_en = 0 ACPU flash encryption key driect used the otp key
	 */
	acpu_flash_enc_otp_key_exp_en = OTP_GET_CFG_DAT(ACPU_FLASH_ENCRYPT_EXT_KEY_ENABLE);
	stage2_encrypt_flag = *(unsigned int*)(addition_buf + STAGE2_ENCRYPT_FLAG_OFFSET);

	if (boot_code_enc == 1)
		flash_key_sel = 1;

	if (stage2_encrypt_flag) {
		if (acpu_flash_enc_otp_key == 0 && boot_code_enc == 0) {
#if !defined (ENABLE_STAGE1_ENCRYPT_DERIVE) && defined (ENABLE_STAGE2_ENCRYPT_DERIVE)
			boot_code_enc = 1;
			flash_key_sel = 1;
#else
			acpu_flash_enc_otp_key = 1;
			flash_key_sel = 0;
#endif
		}
	}

	if (!flash_key_sel && acpu_flash_enc_otp_key) {
		if (acpu_flash_enc_otp_key_exp_en) {
			derive_key_sel = GX_CRYPTO_KEY_OTP;
			key_sel = GX_CRYPTO_KEY_REG1;
		} else {
			ret = gx_acpu_decrypt(GX_CRYPTO_KEY_OTP,data_len,(int *)data_addr,(int *)data_addr);
			if (ret < 0)
				return -1;
			else
				return 0;
		}
	} else if (flash_key_sel && boot_code_enc) {
		derive_key_sel = GX_CRYPTO_KEY_BCMK;
		key_sel = GX_CRYPTO_KEY_BCK;
	} else
		return -1;

	if (derive_key_sel == -1 || key_sel == -1)
		return -1;

	ret = gx_acpu_decrypt(derive_key_sel, KEY_LEN, (int*)(addition_buf + STAGE2_KEY_OFFSET),NULL);
	if (ret < 0)
		return -1;

	ret = gx_acpu_decrypt(key_sel,data_len,(int *)data_addr,(int *)data_addr);
	if (ret < 0)
		return -1;
#elif defined (CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI) || defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_CKMMU_SCORPIO) \
	|| defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	if (!OTP_GET_CFG_DATA(FLASH_ENCRYPT_ENABLE, 1))
		return -1;
	gx_acpu_mtc_decrypt((unsigned int *)data_addr, data_len, (unsigned int *)data_addr);
#endif
	return 0;

}
