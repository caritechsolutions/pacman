#ifndef _GX_SHA256_H
#define _GX_SHA256_H
/*
 * data		:	src data
 * len		:	the total bytes length of data
 * hash		:	the result buf of hash value
 * sha_sm_n	:	sha256 or sm3
 *			1 is sha256, 0 is sm3
 * */
int gx_hw_sha256(int *data, unsigned int len, int *hash, int sha_sm_n);

#endif /* !_GX_SHA256_H */
