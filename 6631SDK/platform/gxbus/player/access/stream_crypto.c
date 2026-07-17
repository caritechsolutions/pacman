#include "gx_demux.h"
#include "gx_options.h"
#include "stream_crypto.h"
#include "stream_m3u8.h"
#include "../avutil/aes.h"

handle_t Gx_AES_Create(const uint8_t *key, int key_bits, int decrypt)
{
	struct AVAES *aes = NULL;

	aes = av_aes_alloc();
	if (aes == NULL)
		return 0;

	if (av_aes_init(aes, key, key_bits, decrypt) < 0)
		return 0;

	return (handle_t)aes;
}


void Gx_AES_Crypt(handle_t aes, uint8_t *dst, const uint8_t *src, int count, uint8_t *iv, int decrypt)
{
	return av_aes_crypt((struct AVAES *)aes, dst, src, count, iv, decrypt);
}

void Gx_AES_Destroy(handle_t aes)
{
	av_free((struct AVAES*)aes);
	return;
}


static int crypto_open(GxStream* stream, int mode)
{
	GxStreamParam param;
	GxStreamCrypto *s = (GxStreamCrypto*)stream;
	GxStreamClass* sl = NULL;
	char* key = NULL, *iv = NULL;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &stream->interrupt_cbk);

	memset(&s->crypto, 0, sizeof(GxCrypto));
	key = GxOptions_Get_By_Id(stream->options, " -H", OPTIONS_ENC_KEY);
	if (key) {
		hex_to_data(s->crypto.encrypt_key, key);
		hex_to_data(s->crypto.decrypt_key, key);
		s->crypto.has_key = 1;
	}
	iv = GxOptions_Get_By_Id(stream->options, " -H", OPTIONS_ENC_IV);
	if (iv) {
		hex_to_data(s->crypto.encrypt_iv, iv);
		hex_to_data(s->crypto.decrypt_iv, iv);
		s->crypto.has_iv = 1;
	}

	param.mode     = mode;
	param.prog     = 0;
	param.priv     = NULL;
	param.flags    = stream->flags;
	param.options  = stream->options;
	param.protocol = NULL;

	s->children = GxStream_OpenByParam(stream->url, &param);
	if (s->children == NULL) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto crypto_error;
	}

	sl = GetGxStreamClassFromObject(s->children);
	if (!sl) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto crypto_error;
	}

	if (!s->crypto.has_key) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto crypto_error;
	}

	sl->control(s->children, GX_STREAM_CTRL_GET_SIZE, &s->crypto.con_l);
	s->crypto.res_l = s->crypto.con_l;
	sl->control(s->children, GX_STREAM_CTRL_GET_TOTAL_TIME, &s->crypto.duration);

	s->crypto.aes_decrypt = Gx_AES_Create(s->crypto.decrypt_key, 8*CRYPTO_BLOCKSIZE, 1);
	if (!s->crypto.aes_decrypt) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto crypto_error;
	}

	s->crypto.aes_encrypt = Gx_AES_Create(s->crypto.encrypt_key, 8*CRYPTO_BLOCKSIZE, 0);
	if (!s->crypto.aes_encrypt) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto crypto_error;
	}

	s->parent.file_format  = s->children->file_format;
	s->parent.demuxer_type = s->children->demuxer_type;
	return GX_PLAYER_OK;

crypto_error:
	if (s->crypto.aes_decrypt) {
		Gx_AES_Destroy(s->crypto.aes_decrypt);
		s->crypto.aes_decrypt = 0;
	}

	if (s->crypto.aes_encrypt) {
		Gx_AES_Destroy(s->crypto.aes_encrypt);
		s->crypto.aes_encrypt = 0;
	}

	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return GX_PLAYER_ERROR;
}

static int crypto_read(GxStream *stream, uint8_t *buffer, size_t max_len)
{
	GxStreamCrypto* s = (GxStreamCrypto*)stream;
	GxCrypto* crypto  = &s->crypto;
	GxStreamClass* sl = NULL;
    int blocks;

	if (!s->children)
		return -1;

retry:
	if (crypto->outdata > 0) {
		max_len = FFMIN(max_len, crypto->outdata);
		memcpy(buffer, crypto->outptr, max_len);
		crypto->outptr  += max_len;
		crypto->outdata -= max_len;
		crypto->res_l   -= max_len;
		return max_len;
	}

	while ((crypto->indata - crypto->indata_used) < 2*CRYPTO_BLOCKSIZE) {
		if (stream->interrupt_cbk && stream->interrupt_cbk())
			return -1;

		sl = GetGxStreamClassFromObject(s->children);
		int n = sl?sl->read(s->children,
				crypto->inbuffer + crypto->indata, sizeof(crypto->inbuffer) - crypto->indata):-1;
		if (n < 0) {
			crypto->res_l = 0;
			break;
		}
		crypto->indata += n;
	}

	blocks = (crypto->indata - crypto->indata_used)/CRYPTO_BLOCKSIZE;
	if (!blocks) {
		crypto->res_l = 0;
		return 0;
	}

	Gx_AES_Crypt(crypto->aes_decrypt, crypto->outbuffer,
			crypto->inbuffer + crypto->indata_used, blocks, crypto->has_iv?crypto->decrypt_iv:NULL, 1);
	crypto->outdata      = CRYPTO_BLOCKSIZE * blocks;
	crypto->outptr       = crypto->outbuffer;
	crypto->indata_used += CRYPTO_BLOCKSIZE * blocks;

	if (crypto->indata_used >= sizeof(crypto->inbuffer)/2) {
		memmove(crypto->inbuffer,
				crypto->inbuffer + crypto->indata_used, crypto->indata - crypto->indata_used);
		crypto->indata     -= crypto->indata_used;
		crypto->indata_used = 0;
	}
	goto retry;
}

static int crypto_write(GxStream *stream, uint8_t *buffer, size_t size)
{
	GxStreamCrypto* s = (GxStreamCrypto*)stream;
	GxCrypto* crypto  = &s->crypto;
	GxStreamClass* sl = NULL;
	int total_size, blocks, pad_len, out_size;
	uint8_t *out_buf;
	int ret = 0;

	total_size = size + crypto->pad_len;
	pad_len = total_size % CRYPTO_BLOCKSIZE;
	out_size = total_size - pad_len;
	blocks = out_size / CRYPTO_BLOCKSIZE;

	if (out_size) {
		out_buf = av_malloc(out_size);
		if (!out_buf)
			return -1;

		if (crypto->pad_len) {
			memcpy(&crypto->pad[crypto->pad_len], buffer, CRYPTO_BLOCKSIZE - crypto->pad_len);
			Gx_AES_Crypt(crypto->aes_encrypt, out_buf, crypto->pad, 1, crypto->has_iv?crypto->encrypt_iv:NULL, 0);
			blocks--;
		}

		Gx_AES_Crypt(crypto->aes_encrypt, &out_buf[crypto->pad_len ? CRYPTO_BLOCKSIZE : 0],
				&buffer[crypto->pad_len ? CRYPTO_BLOCKSIZE - crypto->pad_len: 0],
				blocks, crypto->has_iv?crypto->encrypt_iv:NULL, 0);

		sl = GetGxStreamClassFromObject(s->children);
		ret = sl?sl->write(s->children, out_buf, out_size):-1;
		av_free(out_buf);
		if (ret < 0)
			return ret;

		memcpy(crypto->pad, &buffer[size - pad_len], pad_len);
	} else
		memcpy(&crypto->pad[crypto->pad_len], buffer, size);

	crypto->pad_len = pad_len;

	return size;
}

static int crypto_seek(GxStream* stream, off_t newpos)
{
	int ret = -1;
	GxStreamCrypto *s = (GxStreamCrypto*)stream;
	GxStreamClass *sl = NULL;
	GxCrypto *crypto = &s->crypto;

	if (s->children) {
		sl = GetGxStreamClassFromObject(s->children);
		ret = (sl && sl->seek)?sl->seek(s->children, newpos):GX_PLAYER_ERROR;
		if (ret == GX_PLAYER_OK) {
			crypto->indata  = 0;
			crypto->outdata = 0;
			crypto->pad_len = 0;
			crypto->res_l   = (crypto->con_l - s->children->pos);
			s->parent.pos = s->children->pos;
		}
	}

	return ret;
}

static int crypto_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamCrypto* s = (GxStreamCrypto*)stream;
	GxCrypto* crypto  = &s->crypto;

	if(crypto->outptr)
		return GX_PLAYER_ERROR;

	if(crypto->res_l >= 0) {
		crypto->res_l   += size;
		crypto->outptr  -= size;
		crypto->outdata += size;
	}

	return GX_PLAYER_OK;
}

static int crypto_control(GxStream *stream, int cmd, void *arg)
{
	GxStreamCrypto* s = (GxStreamCrypto*)stream;
	GxCrypto* crypto  = &s->crypto;

	switch (cmd) {
		case GX_STREAM_CTRL_GET_SIZE:
		{
			*(off_t*)arg = crypto->con_l;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_RES_SIZE:
		{
			*(off_t*)arg = crypto->res_l;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_ROLL_BACK:
		{
			GxStream_RollBack* rol = arg;
			if (!rol)
				return GX_PLAYER_ERROR;

			crypto_rollback(stream, rol->buffer, rol->size);
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_TOTAL_TIME:
		{
			if(crypto->duration > 0){
				*(int*)arg = crypto->duration;
				return GX_PLAYER_OK;
			}
		}
		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

void crypto_close(GxStream*  stream)
{
	GxStreamCrypto* s = (GxStreamCrypto*)stream;

	if (s->crypto.aes_decrypt) {
		Gx_AES_Destroy(s->crypto.aes_decrypt);
		s->crypto.aes_decrypt = 0;
	}

	if (s->crypto.aes_encrypt) {
		Gx_AES_Destroy(s->crypto.aes_encrypt);
		s->crypto.aes_encrypt = 0;
	}

	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return;
}

GxStreamClass gx_stream_crypto = {
	._inherit = {
		._inherit = {
			.name = "StreamCrypto",
			.parent = &gx_streambase,
			.size = sizeof(GxStreamCrypto),
		},
	},
	.protocols = {"crypto", NULL},
	DEF_AUTHOR("Stream","crypto","No description","L.F","No comment"),

	.flags   = GX_STREAM_NEED_PROBE,
	.open    = crypto_open,
	.read    = crypto_read,
	.write   = crypto_write,
	.seek    = crypto_seek,
	.control = crypto_control,
	.close   = crypto_close,
};
