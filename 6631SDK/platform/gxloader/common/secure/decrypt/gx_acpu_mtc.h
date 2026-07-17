#ifndef _GX_ACPU_MTC_H_
#define _GX_ACPU_MTC_H_
#include "gxhwlib_registers.h"
#include "sys/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TFM_ALG_AES128,
	TFM_ALG_TDES,
	TFM_ALG_DES,
	TFM_ALG_AES192,
	TFM_ALG_AES256,
	TFM_ALG_SM4,
} GxTfmAlg;

typedef enum {
	TFM_OPT_ECB, ///< 电码本模式
	TFM_OPT_CBC, ///< 密码分组链接模式，需要IV
} GxTfmOpt;

typedef enum {
	TFM_KEY_ACPU_SOFT = 17,
	TFM_KEY_OTP_SOFT  = 19,
	TFM_KEY_OTP_FIRM,
} GxTfmKey;

typedef struct {
	GxTfmAlg       alg;               ///<  选择加解密的算法
	GxTfmOpt       opt;               ///<  选择加解密的模式
	GxTfmKey       key;               ///<  选择加解密的密钥来源
	unsigned char  *src_addr;         ///<  需要解密的原始数据的 buffer
	unsigned char  *dst_addr;         ///<  存放解密之后的数据的 buffer
	unsigned int   data_len;          ///<  数据的长度
	unsigned char  soft_key[32];      ///<  若密钥来源于soft key时，需要将密钥填充的此数组
	unsigned int   soft_key_len;      ///<  配置soft key的长度，根据算法决定。AES128:16B; AES192:24B; AES256:32B; DES:8B; TDES:16B
	unsigned char  iv[16];            ///<  根据算法模式配置IV的值
	unsigned int   iv_len;            ///<  根据算法模式配置IV的长度。AES128/AES192/AES256:16B; DES/TDES:8B
} GxTfmCrypto;

struct _m2m_desc {
    unsigned int       start_addr;
    unsigned int       length;
    unsigned int       conf;
    struct _m2m_desc*  next;
};

struct _m2m_channel {
    unsigned int  src_chain_ptr;
    unsigned int  dst_chain_ptr;
    unsigned int  weight;
    unsigned int  ctrl;
    unsigned int  src_update_size;
    unsigned int  dst_update_size;
    unsigned int  src_remainder;
    unsigned int  dst_remainder;
    unsigned int  data_len;
    unsigned int  reserved[55];
};

struct m2m {
    unsigned int         ctrl;
    unsigned int         reserved_0[63];

    unsigned int         rk;                       // 0x100
    unsigned int         reserved_1[19];

    unsigned int         soft_key[8];
    unsigned int         soft_key_update;
    unsigned int         reserved_rk[3];
    unsigned int         record_key[4];
    unsigned int         record_key_update;
    unsigned int         reserved_2[27];

    unsigned int         iv_mask;                   // 0x200
    unsigned int         iv[4];
    unsigned int         iv_update;
    unsigned int         reserved_3[890];

    struct _m2m_channel  channels[8];               //0x1000
    unsigned int         reserved_4[512];

    unsigned int         header_update;            // 0x2000
    unsigned int         desc_update;
    unsigned int         reserved_5[62];

    unsigned int         key_switch_threshold;     // 0x2100
    unsigned int         sync_byte_err_threshold;
    unsigned int         almost_empty_threshold;
    unsigned int         almost_full_threshold;
    unsigned int         reserved_6[60];

    unsigned int         space_err;                // 0x2200
    unsigned int         almost_full;
    unsigned int         almost_empty;
    unsigned int         key_err;
    unsigned int         sync_err;
    unsigned int         dma_done;
    unsigned int         reserved_7[58];

    unsigned int         m2m_int_en;               // 0x2300
    unsigned int         m2m_int;
};

struct mtc {
	unsigned int ctrl1;     // 00
	unsigned int dummy1[(0x24-0x00)/4-1];
	unsigned int len;       // 24
	unsigned int ctrl2;     // 28
	unsigned int dummy2[(0x44-0x28)/4-1];
	unsigned int iv3;       // 44
	unsigned int iv2;       // 48
	unsigned int iv1;       // 4c
	unsigned int iv0;       // 50
	unsigned int dummy3[(0x60-0x50)/4-1];
	unsigned int rst_ctrl;    // 60
	unsigned int keysel;    // 64
	unsigned int dummy4[(0xc0-0x64)/4-1];
	unsigned int status;    // c0
	unsigned int wdata;     // c4
	unsigned int rdata;     // c8
	unsigned int dummy5[(0x160-0xc8)/4-1];
	unsigned int isr_en;    // 160
	unsigned int isr;       // 164
};

/*
 * gx_acpu_mtc_decrypt()
 *
 *  DESCRIPTION
 *    Function for decrypting data using mtc module
 *
 *  PARAMETER
 *  @src_addr
 *      Data address that needs to be decrypted
 *  @length
 *      Data length that needs to be decrypted
 *  @dst_addr
 *      The data address used to store the decrypted result
 *
 *  NOTES
 *
 * */
void gx_acpu_mtc_decrypt(unsigned int *src_addr, unsigned int length, unsigned int *dst_addr);

int gx_acpu_m2m_encrypt(GxTfmCrypto *param);
int gx_acpu_m2m_decrypt(GxTfmCrypto *param);

#ifdef __cplusplus
}
#endif

#endif
