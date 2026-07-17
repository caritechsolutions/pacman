# 客户集成
## 开发环境

gxloader 提供 user 目录用于客户开发自定义程序

### 第三方代码集成

- 第三方头文件存放地址
```
user/include
```

- 第三方 lib 库存放地址
```
user/lib
```

-  Makefile 集成
```
user/user.mk
```

添加第三方库链接
```
MY_LIBS += -lxxx
```

添加编译文件
```
COBJS-y += user/xxx.o
```

- 程序入口：user/user_config.c 的 user_init
```
void user_init(void)
{
   // 添加自定义程序
}
```

### kernel 安全启动

- kernel 解密
  - 修改配置项
  ```
	ENABLE_APP_DECRYPT = y
  ```
  - 回调函数开发范例
  ```
	#include "user.h"

	static unsigned char user_IK[16] = {
		// ...
	};

	// 回调函数开发
	static int user_app_dec(struct partition_info *partition, unsigned char *src, unsigned int src_len, unsigned char *dst, unsigned int dst_len)
	{
		GxTfmCrypto param = {0};

		if (partition == NULL || src == NULL || dst == NULL || src_len < dst_len || src_len == 0)
			return -2;

		param.module        = TFM_MOD_M2M;
		param.alg           = TFM_ALG_AES128;
		param.opt           = TFM_OPT_CBC;
		param.even_key.id   = TFM_KEY_ACPU_SOFT;
		param.input.buf     = src;
		param.input.length  = src_len;
		param.output.buf    = dst;
		param.output.length = dst_len;
		memcpy(param.soft_key, user_IK, sizeof(user_IK));

		return GxTfm_Decrypt(&param); // 本接口是国芯微高安库函数，需要添加依赖库和头文件
	}

	int user_init(void)
	{
		// 注册回调函数
		struct crypt_ops_t d = {0};

		d.app_dec = user_app_dec;
		register_decrypt(&d);

		return 0;
	}
  ```

- kernel 验签
  - 修改配置项
  ```
	#SECURE BOOT VERIFY : RSA_PKCS | RSA_SMI
	SECURE_VERIFY_TYPE = RSA_PKCS

	ENABLE_VERIFY = y
  ```
  - 回调函数开发范例
  ```
	#include "user.h"

	static unsigned char user_pub_key[256] = {
		// ...
	};

	// 回调函数开发
	static int user_verify_signature(struct partition_info *partition, unsigned char *signature, unsigned int sign_len, unsigned char *src, unsigned int src_len)
	{
		unsigned char sig[256] = {0};

		if (NULL == partition || NULL == src || 0 == src_len)
			return -1;

		if (partition_read(partition, partition->total_size-256, sig, 256) < 0)
			return -1;

		if (rsa_pubkey_verif_fun(src, src_len-256, user_pub_key, sig) < 0) {
			printf("[USER] verify signature failed\n");
			return -1;
		}

		return 0;
	}

	int user_init(void)
	{
		// 注册回调函数
		struct signature_ops_t s = {0};

		s.get_signature    = NULL;
		s.verify_signature = user_verify_signature;
		register_signature(&s);

		return 0;
	}
  ```
