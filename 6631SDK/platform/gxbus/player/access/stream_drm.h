#ifndef MPLAYER_DRM_STREAMING_H
#define MPLAYER_DRM_STREAMING_H

#include "gx_stream.h"

#define DRM_BLOCKSIZE 16
#define DRM_MAX_BUFFER_BLOCKS 150

typedef struct drm {
	int newFlags;
    int indata, indata_used, outdata;
	int con_l, res_l, duration;
    uint8_t *outptr;
	uint8_t inbuffer [DRM_BLOCKSIZE*DRM_MAX_BUFFER_BLOCKS];
	uint8_t outbuffer[DRM_BLOCKSIZE*DRM_MAX_BUFFER_BLOCKS];
	uint8_t has_iv, has_key;
	char    *data_url;
	char    *decrypt_key_url;
	uint8_t decrypt_iv [16];
	uint8_t decrypt_iv_len;
	char    *encrypt_key_url;
	uint8_t encrypt_iv [16];
	uint8_t encrypt_iv_len;
	uint8_t pad[DRM_BLOCKSIZE];
	int pad_len;
} GxDrm;

typedef struct stream_drm {
	GxStream  parent;
	GxDrm     drm;
	GxStream* children;
} GxStreamDrm;

#endif
