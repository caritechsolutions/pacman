#include "gx_common.h"
#include "gx_system.h"
#include "gxplayer_pvr.h"
#include "pvr_enc.h"

#define PVR_ENCRYPT_COUNT (8)
static PlayerPvrEncryptOps *enc_ops[PVR_ENCRYPT_COUNT] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static PlayerPvrEncryptOps* __find_ops(PlayerPvrEncryptMode mode)
{
	unsigned int i = 0;
	PlayerPvrEncryptOps *ops = NULL;

	for (i = 0; i < PVR_ENCRYPT_COUNT; i++) {
		if (enc_ops[i] && (enc_ops[i]->enc_mode == mode)) {
			ops = enc_ops[i];
			break;
		}
	}

	return ops;
}

static void __add_ops(PlayerPvrEncryptOps *ops)
{
	int pos = -1;
	unsigned int i = 0;

	for (i = 0; i < PVR_ENCRYPT_COUNT; i++) {
		if (enc_ops[i] && (enc_ops[i]->enc_mode == ops->enc_mode))
			break;

		if (enc_ops[i] == NULL) {
			if (pos == -1)
				pos = i;
		}
	}

	if ((i >= PVR_ENCRYPT_COUNT) && (pos != -1))
		enc_ops[pos] = ops;

	return;
}

static void __del_ops(PlayerPvrEncryptOps *ops)
{
	unsigned int i = 0;

	for (i = 0; i < PVR_ENCRYPT_COUNT; i++) {
		if (enc_ops[i] && (enc_ops[i]->enc_mode == ops->enc_mode))
			enc_ops[i] = NULL;
	}

	return;
}

status_t GxPVRPlayer_RegisterEncrypt(PlayerPvrEncryptOps *ops)
{
	__add_ops(ops);
	return 0;
}

status_t GxPVRPlayer_UnRegisterEncrypt(PlayerPvrEncryptOps *ops)
{
	__del_ops(ops);
	return 0;
}

int pvr_encrypt_process(PlayerPvrEncryptMode mode, PVREncryptBuff *buf)
{
	PlayerPvrEncryptOps *ops = __find_ops(mode);
	PlayerPvrEncryptPara para;
	int ret = 0;

	if (ops == NULL)
		return -1;

	if (buf->encrpyt && (ops->encrpyt == NULL)) {
		gxloge("%s %d: not encrpyt\n", __func__, __LINE__);
		return -1;
	}
	if (!buf->encrpyt && (ops->decrpyt == NULL)) {
		gxloge("%s %d: not encrpyt\n", __func__, __LINE__);
		return -1;
	}

	para.enc_mode  = ops->enc_mode;
	para.ibuf_type = buf->ibuf_type;
	para.obuf_type = buf->obuf_type;
	if (buf->encrpyt) {
		switch (para.enc_mode) {
		case PLAYER_PVR_PVR_ENCRYPT:
			para.pvr.i_buf    = buf->ibuf;
			para.pvr.i_size   = buf->isize;
			para.pvr.o_buf    = buf->obuf;
			para.pvr.o_size   = buf->osize;
			para.pvr.key_desc = NULL;
			ret = ops->encrpyt(&para);
			buf->key_desc = para.pvr.key_desc;
			break;
		case PLAYER_PVR_DVR_ENCRYPT:
			para.dvr.i_buf    = buf->ibuf;
			para.dvr.i_size   = buf->isize;
			para.dvr.o_buf    = buf->obuf;
			para.dvr.o_size   = buf->osize;
			para.dvr.fpos     = buf->fpos;
			ret = ops->encrpyt(&para);
			break;
		default:
			return -1;
		}
	} else {
		switch (para.enc_mode) {
		case PLAYER_PVR_PVR_ENCRYPT:
			para.pvr.i_buf    = buf->ibuf;
			para.pvr.i_size   = buf->isize;
			para.pvr.o_buf    = buf->obuf;
			para.pvr.o_size   = buf->osize;
			para.pvr.key_desc = buf->key_desc;
			ret = ops->decrpyt(&para);
			break;
		case PLAYER_PVR_DVR_ENCRYPT:
			para.dvr.i_buf    = buf->ibuf;
			para.dvr.i_size   = buf->isize;
			para.dvr.o_buf    = buf->obuf;
			para.dvr.o_size   = buf->osize;
			para.dvr.fpos     = buf->fpos;
			ret = ops->decrpyt(&para);
			break;
		default:
			return -1;
		}
	}

	return ret;
}
