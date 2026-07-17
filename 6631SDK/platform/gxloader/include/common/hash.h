#ifndef __HASH_H__
#define __HASH_H__

/*
 * data		:	src data
 * len		:	the total bytes length of data
 * hash		:	the result buf of hash value
 * */
int gx_sha256(const void *buf, unsigned int len, void *hash);

#endif
