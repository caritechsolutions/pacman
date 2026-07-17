#include "gx_stream.h"
#include "gx_demux.h"
#include "gx_options.h"
#include "stream_pvr.h"
#include "devapi/chip_info.h"
#include "gxsecure/gxtfm_api.h"
#include "pvr_enc.h"

static int _pvr_tfm_copy(unsigned char* src, unsigned char* dst, unsigned int size, int encrpyt)
{
	int ret = -1;
	GxTfmCrypto param = {0};

	memset(&param, 0, sizeof(GxTfmCrypto));
	param.module        = TFM_MOD_M2M;
	param.alg           = TFM_ALG_AES128;
	param.opt           = TFM_OPT_ECB;
	param.src.id        = TFM_SRC_MEM;
	param.dst.id        = TFM_DST_MEM;
	param.input.buf     = src;
	param.input.length  = size;
	param.output.buf    = dst;
	param.output.length = size;
	param.even_key.id   = TFM_KEY_SSUK;
	param.odd_key.id    = TFM_KEY_SSUK;
	param.flags         = TFM_FLAG_CRYPT_EVEN_KEY_VALID | TFM_FLAG_CRYPT_ODD_KEY_VALID |
		TFM_FLAG_CRYPT_TS_PACKET_MODE | TFM_FLAG_CRYPT_SWITCH_CLR | TFM_FLAG_CRYPT_HW_PVR_MODE;

	if (encrpyt) {
		param.input_paddr = (unsigned int)src;
		ret = GxTfm_Encrypt(&param);
	} else {
		param.output_paddr = (unsigned int)dst;
		ret = GxTfm_Decrypt(&param);
	}
	if (ret < 0) {
		gxloge("PVR %s Copy error, ret = %d\n", encrpyt ? "encrpyt" : "decrpyt", ret);
		return -1;
	}

	return 0;
}

static int _pvr_encrypt(GxStreamPVR *s,
		unsigned char* dst,
		unsigned char* src,
		unsigned int size,
		char **desc_key)
{
	int ret = -1;
	PVREncryptBuff buf;

	buf.encrpyt   = 1;
	buf.ibuf      = src;
	buf.ibuf_type = s->security_flag ? PLAYER_PVR_BUF_SECURE : PLAYER_PVR_BUF_NORMAL;
	buf.isize     = size;
	buf.obuf      = dst;
	buf.obuf_type = PLAYER_PVR_BUF_NORMAL;
	buf.osize     = size;
	buf.key_desc  = NULL;
	ret = pvr_encrypt_process(s->enc_mode, &buf);
	if (ret < 0) {
		gxloge("PVR Encrypt Process error, ret = %d\n", ret);
		return -1;
	}
	*desc_key = buf.key_desc;

	return ret;
}

static int _pvr_decrypt(GxStreamPVR *s,
		unsigned char* dst,
		unsigned char* src,
		unsigned int size,
		char *desc_key)
{
	int ret = -1;
	PVREncryptBuff buf;

	buf.encrpyt   = 0;
	buf.ibuf      = src;
	buf.ibuf_type = PLAYER_PVR_BUF_NORMAL;
	buf.isize     = size;
	buf.obuf      = dst;
	buf.obuf_type = s->security_flag ? PLAYER_PVR_BUF_SECURE : PLAYER_PVR_BUF_NORMAL;
	buf.osize     = size;
	buf.key_desc  = desc_key;
	ret = pvr_encrypt_process(s->enc_mode, &buf);
	if (ret < 0) {
		gxloge("PVR Decrypt Process error, ret = %d\n", ret);
		return -1;
	}

	return ret;
}

static void _pvr_update_fpos(GxStreamPVR *s)
{
	s->parent.pos     = s->fpos;
	s->parent.end_pos = s->children->end_pos;
}

static int stream_pvr_open(GxStream* stream, int mode)
{
#define MAX_PROTO_LEN 32
	char proto[MAX_PROTO_LEN];
	GxStreamParam param;
	GxStreamPVR *s   = (GxStreamPVR*)stream;

	protocol_by_url(stream->original_url, proto, MAX_PROTO_LEN);
	param.mode     = mode;
	param.prog     = 0;
	param.flags    = stream->flags;
	param.options  = stream->options;
	param.protocol = proto;
	param.priv     = NULL;

	s->children = GxStream_OpenByParam(stream->original_url, &param);
	if (s->children == NULL) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto pvr_error;
	}
	s->sl = GetGxStreamClassFromObject(s->children);
	s->fpos = 0;
	s->data_size = 0;
	s->process_flag = 0;

	s->parent.file_format  = s->children->file_format;
	s->parent.demuxer_type = GX_DEMUXER_TYPE_HW_TS;
	s->parent.ispvr        = 1;
	_pvr_update_fpos(s);
	return GX_PLAYER_OK;

pvr_error:
	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return GX_PLAYER_ERROR;
}

static int stream_pvr_read(GxStream *stream, uint8_t *buffer, size_t size)
{
	GxStreamPVR    *s = (GxStreamPVR*)stream;
	GxStreamClass *sl = s->sl;
	int ret = 0;
	int rsize = s->data_size;
	unsigned char *rbuf = NULL;

	if (!sl->read || !s->children)
		return -1;

	if (s->security_flag) {
		if ((s->block_size <= 0) || (size != s->block_size)) {
			gxloge("%s %d: %d %d\n", __func__, __LINE__, size, s->block_size);
			return -1;
		}

		if (!s->data_buf) {
			s->data_buf = av_malloc(s->block_size);
			if (s->data_buf == NULL) {
				gxloge("%s %d\n", __func__, __LINE__);
				return -1;
			}
		}
		rbuf = s->data_buf;
	} else {
		if (s->crypt_enable) {
			if ((s->block_size <= 0) || (size != s->block_size)) {
				gxloge("%s %d: %d %d\n", __func__, __LINE__, size, s->block_size);
				return -1;
			}
		}
		rbuf = buffer;
	}

	while (rsize < size) {
		int n = 0;

		if (stream->interrupt_cbk && stream->interrupt_cbk())
			return -1;
		n = sl->read(s->children, rbuf + rsize, size - rsize);
		if (n < 0)
			return -1;
		rsize += n;
	}

	if (s->crypt_enable) {
		char *desc_key = NULL;
		GxRecordPVRControl pvr_ctrl;

		pvr_ctrl.opt = GX_RECORD_PVR_GET_KEY_DESC;
		pvr_ctrl.arg = NULL;
		GxStream_Control(s->children, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&pvr_ctrl);
		desc_key = (char *)pvr_ctrl.arg;
		if ((ret = _pvr_decrypt(s, buffer, rbuf, rsize, desc_key)) != 0) {
			if (ret == -2) {
				s->data_size = rsize;
				s->process_flag = 1;
				return 0;
			}
			s->process_flag = 0;
			s->data_size = 0;
			return -1;
		}
	} else if (s->security_flag) {
		if ((ret = _pvr_tfm_copy(rbuf, buffer, rsize, 0)) != 0) {
			return -1;
		}
	}

	s->process_flag = 0;
	s->data_size = 0;
	s->fpos += rsize;
	_pvr_update_fpos(s);
	return rsize;
}

static int stream_pvr_write(GxStream *stream, uint8_t *buffer, size_t size)
{
	GxStreamPVR    *s = (GxStreamPVR*)stream;
	GxStreamClass *sl = s->sl;
	int wsize = 0;
	unsigned char *wbuf = NULL;

	if (!sl->write || !s->children)
		return -1;

	if (s->security_flag) {
		if ((s->block_size <= 0) || (size != s->block_size)) {
			gxloge("%s %d: %d %d\n", __func__, __LINE__, size, s->block_size);
			return -1;
		}

		if (!s->data_buf) {
			s->data_buf = av_malloc(s->block_size);
			if (s->data_buf == NULL) {
				gxloge("%s %d\n", __func__, __LINE__);
				return -1;
			}
		}
		wbuf = s->data_buf;
	} else {
		if (s->crypt_enable) {
			if ((s->block_size <= 0) || ((size % s->block_size) != 0)) {
				gxloge("%s %d: %d %d\n", __func__, __LINE__, size, s->block_size);
				return -1;
			}
		}
		wbuf = buffer;
	}

	if (s->crypt_enable) {
		char *desc_key = NULL;
		GxRecordPVRControl pvr_ctrl;

		if (_pvr_encrypt(s, wbuf, buffer, size, &desc_key) != 0)
			return -1;

		pvr_ctrl.opt = GX_RECORD_PVR_SET_KEY_DESC;
		pvr_ctrl.arg = (void *)desc_key;
		GxStream_Control(s->children, GX_STREAM_CTRL_PVR_SET_CONTROL, (void *)&pvr_ctrl);
	} else if (s->security_flag) {
		if (_pvr_tfm_copy(buffer, wbuf, size, 1) != 0)
			return -1;
	}

	while (wsize < size) {
		int n = 0;

		if (stream->interrupt_cbk && stream->interrupt_cbk())
			return -1;
		n = sl->write(s->children, wbuf + wsize, size - wsize);
		if (n < 0)
			return -1;
		wsize += n;
	}

	s->fpos += wsize;
	_pvr_update_fpos(s);
	return wsize;
}

static int stream_pvr_seek(GxStream *stream, off_t newpos)
{
	int ret = -1;
	GxStreamPVR    *s = (GxStreamPVR*)stream;
	GxStreamClass *sl = s->sl;

	if (!sl->seek || !s->children)
		return -1;

	if (s->block_size > 0)
		newpos = newpos / s->block_size * s->block_size;

	ret = (sl && sl->seek) ? sl->seek(s->children, newpos) : GX_PLAYER_ERROR;
	if (ret == GX_PLAYER_OK) {
		s->fpos = newpos;
		s->data_size = 0;
		_pvr_update_fpos(s);
	}

	return ret;
}

static int stream_pvr_control(GxStream *stream, int cmd, void *arg)
{
	int ret = -1;
	GxStreamPVR    *s = (GxStreamPVR*)stream;
	GxStreamClass *sl = s->sl;

	if (!sl->control || !s->children)
		return -1;

	switch (cmd) {
	case GX_STREAM_CTRL_READ_BLOCK:
		{
			int ret = stream_pvr_read(stream, (uint8_t *)arg, s->block_size);

			if (ret <= 0) {
				int errorcode = 0;

				sl->control(s->children, GX_STREAM_CTRL_GET_ERROR_CODE, &errorcode);
				return ((errorcode == 0) ? 0 : -1);
			}
			return ret;
		}
	case GX_STREAM_CTRL_SET_SECURITY:
		{
			GxStreamSecurity *sec = (GxStreamSecurity *)arg;

			s->security_flag = sec->security_flag;
			s->block_size    = sec->block_size;
			s->crypt_enable  = sec->crypt_enable;
			s->enc_mode      = PLAYER_PVR_PVR_ENCRYPT;
			break;
		}
	case GX_STREAM_CTRL_GET_PROCESS_FLAG:
		{
			*(unsigned int *)arg = s->process_flag;
			return 0;
		}
	default:
		break;
	}

	return sl->control(s->children, cmd, arg);
}

static void stream_pvr_close(GxStream *stream)
{
	GxStreamPVR *s = (GxStreamPVR*)stream;

	if (s->data_buf) {
		av_free(s->data_buf);
		s->data_buf = NULL;
	}

	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return;
}

GxStreamClass gx_stream_pvr = {
	._inherit = {
		._inherit = {
			.name = "StreamPVR",
			.parent = &gx_streambase,
			.size = sizeof(GxStreamPVR),
		},
	},
	.protocols = {"pvr", NULL},
	DEF_AUTHOR("Stream","PVR","No description","L.F","No comment"),

	.flags   = GX_STREAM_NOT_CACHE,
	.open    = stream_pvr_open,
	.read    = stream_pvr_read,
	.write   = stream_pvr_write,
	.seek    = stream_pvr_seek,
	.control = stream_pvr_control,
	.close   = stream_pvr_close,
};
