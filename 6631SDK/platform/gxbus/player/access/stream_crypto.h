#ifndef MPLAYER_CRYPTO_STREAMING_H
#define MPLAYER_CRYPTO_STREAMING_H

#include "gx_stream.h"

#define CRYPTO_BLOCKSIZE 16
#define CRYPTO_MAX_BUFFER_BLOCKS 150

typedef struct crypto {
    handle_t aes_decrypt;
    handle_t aes_encrypt;
    int indata, indata_used, outdata;
	int con_l, res_l, duration;
    uint8_t *outptr;
	uint8_t inbuffer [CRYPTO_BLOCKSIZE*CRYPTO_MAX_BUFFER_BLOCKS];
	uint8_t outbuffer[CRYPTO_BLOCKSIZE*CRYPTO_MAX_BUFFER_BLOCKS];
	uint8_t has_iv, has_key;
	uint8_t decrypt_key[16];
	uint8_t decrypt_iv [16];
	uint8_t encrypt_key[16];
	uint8_t encrypt_iv [16];
	uint8_t pad[CRYPTO_BLOCKSIZE];
	int pad_len;
} GxCrypto;

typedef struct stream_crypto {
	GxStream  parent;
	GxCrypto  crypto;
	GxStream* children;
} GxStreamCrypto;

#endif
