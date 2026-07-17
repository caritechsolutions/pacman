/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"

#if SKIN_SUPPORT
#include "app_module.h"

//#include "netapps/app_netapps_service.h"
#include <sys/stat.h>
#include <gxos/gxcore_os_core.h>
#include <gx_apps.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "gx_apps.h"
#include <cyassl/version.h>
#include <cyassl/openssl/md5.h>
#include <cyassl/ctaocrypt/aes.h>
#include <cyassl/ctaocrypt/des3.h>
#include <cyassl/ctaocrypt/hmac.h>
#include <cyassl/ctaocrypt/sha.h>

#include "app_skin_secure.h"


#define SKIN_SECURE_AES_BLOCK_SIZE 16

static unsigned char s_skin_secure_base_key[SKIN_SECURE_AES_BLOCK_SIZE]={0x11,0x22,0x33,};
static unsigned char s_skin_secure_base_iv[SKIN_SECURE_AES_BLOCK_SIZE]={};

//2A3D08231B2E0103894C7A9E249A1B36
static unsigned char *skin_secury_key(void)
{
    s_skin_secure_base_key[0] = 0x2A;
    s_skin_secure_base_key[1] = 0x3D;
	s_skin_secure_base_key[2] = 0x08;
    s_skin_secure_base_key[3] = 0x23;   

    s_skin_secure_base_key[4] = 0x1B;
    s_skin_secure_base_key[5] = 0x2E;
    s_skin_secure_base_key[6] = 0x01;
    s_skin_secure_base_key[7] = 0x03;

    s_skin_secure_base_key[8] = 0x89;
    s_skin_secure_base_key[9] = 0x4c;
	s_skin_secure_base_key[10] = 0x7A;
	s_skin_secure_base_key[11] = 0x9E;

    s_skin_secure_base_key[12] = 0x24;
    s_skin_secure_base_key[13] = 0x9A;
	s_skin_secure_base_key[14] = 0x1B;
	s_skin_secure_base_key[15] = 0x36;

    return s_skin_secure_base_key;
}

//0D1C0717FA8B091DE43D271B057E9F47
static unsigned char *skin_secury_iv(void)
{
    s_skin_secure_base_iv[0] = 0x0D;   
    s_skin_secure_base_iv[1] = 0x1C;
	s_skin_secure_base_iv[2] = 0x07;
	s_skin_secure_base_iv[3] = 0x17;

	s_skin_secure_base_iv[4] = 0xFA;
    s_skin_secure_base_iv[5] = 0x8B;
	s_skin_secure_base_iv[6] = 0x09;
    s_skin_secure_base_iv[7] = 0x1D;

    s_skin_secure_base_iv[8] = 0xE4;
    s_skin_secure_base_iv[9] = 0x3D;
	s_skin_secure_base_iv[10] = 0x27;
	s_skin_secure_base_iv[11] = 0x1B;

    s_skin_secure_base_iv[12] = 0x05;
    s_skin_secure_base_iv[13] = 0x7E;
	s_skin_secure_base_iv[14] = 0x9F;
	s_skin_secure_base_iv[15] = 0x47;

    return s_skin_secure_base_iv;
}

static int _skin_secure_aes_cbc_decrypt(const unsigned char* key, unsigned int keylen, 
		const unsigned char* iv, const unsigned char *in, unsigned char* out, unsigned int sz)
{
	Aes dec;

    AesSetKey(&dec, key, keylen, iv, AES_DECRYPTION);
    AesCbcDecrypt(&dec, out, in, sz);

	return 0 ;
}

int gxapp_skin_secure_decrypt(char *src,  int srclen, char *dst)
{
    if ( (dst == NULL) || (src == NULL ) || (srclen <=0) )
		return -1 ;

    _skin_secure_aes_cbc_decrypt(skin_secury_key(), SKIN_SECURE_AES_BLOCK_SIZE, 
				skin_secury_iv(), (unsigned char *)src, (unsigned char *)dst, srclen);

    return 0;
}

//md5

#define MD5_DATA_LEN (16)

int gxapp_skin_secure_buff_md5_check(char *src,int srclen,char *verify_md5)
{
	GX_MD5_CTX md5_ctx;
	unsigned char md5_result[MD5_DATA_LEN];
	char *dst = NULL;

	dst = (char *)malloc(2*MD5_DATA_LEN+1);
	if(dst)
	{
		memset(dst, 0, 2*MD5_DATA_LEN+1);
		GX_MD5_Init(&md5_ctx);
		GX_MD5_Update(&md5_ctx,src, srclen);
		GX_MD5_Final(md5_result,&md5_ctx);
		gx_hex2str(md5_result, dst, sizeof(md5_result));

		if(strcmp(verify_md5, dst) != 0)
		{
			free(dst);
			return -41 ;
		}

		free(dst);
		return 0 ;
	}

	return -42;
}

#endif
