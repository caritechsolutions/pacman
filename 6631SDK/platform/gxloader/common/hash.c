#include "common/hash.h"
#ifdef CONFIG_ENABLE_HARDWARE_HASH
#include "driver/gx_sha256.h"
#else
#include "common/gx_soft_sha.h"
#endif

int gx_sha256(const void *buf, unsigned int len, void *hash)
{
#ifdef CONFIG_ENABLE_HARDWARE_HASH
	return gx_hw_sha256((int *)buf, len, hash, 1);
#else
	return gx_soft_sha256((const unsigned char *)buf, len, hash);
#endif
}

int gx_sm3(const void *buf, unsigned int len, void *hash)
{
#ifdef CONFIG_ENABLE_HARDWARE_HASH
	return gx_hw_sha256((int *)buf, len, hash, 0);
#else
	return -1;
#endif
}
