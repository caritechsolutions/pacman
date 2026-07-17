#ifndef __PVR_ENC_H__
#define __PVR_ENC_H__

typedef struct {
	int                    encrpyt;
	PlayerPvrBufSecureType ibuf_type;
	PlayerPvrBufSecureType obuf_type;
	unsigned char         *ibuf;
	unsigned int           isize;
	unsigned char         *obuf;
	unsigned int           osize;
	char                  *key_desc;
	long long              fpos;
} PVREncryptBuff;

int pvr_encrypt_process(PlayerPvrEncryptMode mode, PVREncryptBuff *buf);

#endif
