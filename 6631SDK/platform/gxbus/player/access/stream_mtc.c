#include "gx_demux.h"
#include "gx_options.h"
#include "stream_mtc.h"
#include "devapi/chip_info.h"
#include "gxsecure/gxtfm_api.h"

static unsigned int MTC_BLOCK_SIZE = 4*188;
static unsigned int MTC_READ_BLOCK_COUNT  = 64;
static unsigned int MTC_WRITE_BLOCK_COUNT = 64;

struct mtc_code {
	char *str;
	int  algo;
	int  sub_mode;
};

static struct mtc_code code[] = {
	{"des",          GXMTC_API_DES,    GXMTC_API_ECB},
	{"des-ecb",      GXMTC_API_DES,    GXMTC_API_ECB},
	{"des-cbc",      GXMTC_API_DES,    GXMTC_API_CBC},
	{"des-ofb",      GXMTC_API_DES,    GXMTC_API_OFB},
	{"des-cfb",      GXMTC_API_DES,    GXMTC_API_CFB},
	{"des3",         GXMTC_API_3DES,   GXMTC_API_ECB},
	{"aes",          GXMTC_API_AES128, GXMTC_API_ECB},
	{"aes-128",      GXMTC_API_AES128, GXMTC_API_ECB},
	{"aes-128-ecb",  GXMTC_API_AES128, GXMTC_API_ECB},
	{"aes-128-cbc",  GXMTC_API_AES128, GXMTC_API_CBC},
	{"aes-192",      GXMTC_API_AES192, GXMTC_API_ECB},
	{"aes-192-ecb",  GXMTC_API_AES192, GXMTC_API_ECB},
	{"aes-192-cbc",  GXMTC_API_AES192, GXMTC_API_CBC},
	{"aes-256",      GXMTC_API_AES256, GXMTC_API_ECB},
	{"aes-256-ecb",  GXMTC_API_AES256, GXMTC_API_ECB},
	{"aes-256-cbc",  GXMTC_API_AES256, GXMTC_API_CBC},
	{"custom",       0,                0            },
};

static int mtc_common(GxMtc *mtc, unsigned char* dst, unsigned char* src, unsigned int size, int enc, off_t pos)
{
	int ret;
	GxTfmCrypto mtc_data = {0};

	int alg_array[GXMTC_API_SMS4+1] = {
		TFM_ALG_DES,
		TFM_ALG_TDES,
		TFM_ALG_AES128,
		TFM_ALG_AES192,
		TFM_ALG_AES256,
		TFM_ALG_AES128, // multi2 reserve
		TFM_ALG_SM4,
	};

	if ((int)src%MTC_ADDR_ALIGN || (int)dst%MTC_ADDR_ALIGN || size%MTC_SIZE_ALIGN) {
		gxlogd("%s, not align !!! size = %d\n", __func__, size);
		return -1;
	}

	memset(&mtc_data, 0, sizeof(GxTfmCrypto));
	mtc_data.module = TFM_MOD_M2M;
	mtc_data.alg           = alg_array[mtc->algo];
	mtc_data.opt           = mtc->sub_mode;
	mtc_data.src.id        = TFM_SRC_MEM;
	mtc_data.dst.id        = TFM_DST_MEM;
	mtc_data.input.buf     = src;
	mtc_data.input.length  = size;
	mtc_data.output.buf    = dst;
	mtc_data.output.length = size;
	mtc_data.even_key.id   = TFM_KEY_ACPU_SOFT;
	mtc_data.odd_key.id    = TFM_KEY_ACPU_SOFT;
	mtc_data.flags         = TFM_FLAG_CRYPT_EVEN_KEY_VALID | TFM_FLAG_CRYPT_ODD_KEY_VALID;
	memcpy(mtc_data.iv, mtc->iv_value, 16);
	memcpy(mtc_data.soft_key, mtc->key_value, mtc->key_len);

	if (enc)
		ret = GxTfm_Encrypt(&mtc_data);
	else
		ret = GxTfm_Decrypt(&mtc_data);
	if (ret < 0) {
		gxlogd("%s(%d) ioctl error, ret = %d\n", __func__, __LINE__, ret);
		return -1;
	}
	return 0;
}

static int mtc_encrypt(GxMtc *mtc, unsigned char* dst, unsigned char* src, unsigned int size)
{
	int ret = mtc_common(mtc, dst, src, size, 1, mtc->w_fpos);
	mtc->w_fpos += size;
	return ret;
}

static int mtc_decrypt(GxMtc *mtc, unsigned char* dst, unsigned char* src, unsigned int size)
{
	int ret = mtc_common(mtc, dst, src, size, 0, mtc->r_fpos);

	return ret;
}

static int mtc_open(GxStream* stream, int mode)
{
#define MAX_PROTO_LEN 32
	int i = 0;
	char *key_algo = NULL, *key = NULL, *iv = NULL;
	char proto[MAX_PROTO_LEN];
	GxStreamParam param;
	GxStreamMtc *s  = (GxStreamMtc*)stream;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &stream->interrupt_cbk);
	memset(&s->mtc, 0, sizeof(GxMtc));

	key_algo = GxOptions_Get_By_Id(stream->options, " -H", OPTIONS_ENC);
	if (!key_algo)
		return GX_PLAYER_ERROR;

	key = GxOptions_Get_By_Id(stream->options, " -H", OPTIONS_ENC_KEY);
	iv  = GxOptions_Get_By_Id(stream->options, " -H", OPTIONS_ENC_IV);

	if (key)
		s->mtc.key_len = hex_to_data(s->mtc.key_value, key);

	if (iv)
		s->mtc.iv_len = hex_to_data(s->mtc.iv_value, iv);

	for (i = 0; i < sizeof(code)/sizeof(struct mtc_code); i++) {
		if (strncasecmp(key_algo, code[i].str, strlen(code[i].str)) == 0) {
			s->mtc.algo     = code[i].algo;
			s->mtc.sub_mode = code[i].sub_mode;
			break;
		}
	}

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
		goto mtc_error;
	}

	s->parent.end_pos      = s->children->end_pos;
	s->parent.file_format  = s->children->file_format;
	s->parent.demuxer_type = s->children->demuxer_type;
	return GX_PLAYER_OK;

mtc_error:
	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return GX_PLAYER_ERROR;
}

static int mtc_read(GxStream *stream, uint8_t *buffer, size_t size)
{
	GxStreamMtc *s = (GxStreamMtc*)stream;
	GxMtc *mtc = &s->mtc;
	GxStreamClass *sl = NULL;
	uint8_t *src = NULL, *dst = NULL;
	size_t align = 0;
	int blocks;

	if (s->children)
		sl = GetGxStreamClassFromObject(s->children);

	if (!sl || !sl->read)
		return -1;

	if (!mtc->r_inbuf)
		mtc->r_inbuf = av_malloc(MTC_BLOCK_SIZE*MTC_READ_BLOCK_COUNT+MTC_ADDR_ALIGN);

	if (!mtc->r_outbuf)
		mtc->r_outbuf = av_malloc(MTC_BLOCK_SIZE*MTC_READ_BLOCK_COUNT+MTC_ADDR_ALIGN);

	if (mtc->r_outdata == 0) {
		align = stream->pos % MTC_BLOCK_SIZE;
		if (align) {
			sl->seek(s->children, stream->pos - align);
			mtc->r_indata = 0;
		}
	}

	src = (uint8_t*)((((unsigned int)mtc->r_inbuf  + MTC_ADDR_ALIGN - 1)/MTC_ADDR_ALIGN)*MTC_ADDR_ALIGN);
	dst = (uint8_t*)((((unsigned int)mtc->r_outbuf + MTC_ADDR_ALIGN - 1)/MTC_ADDR_ALIGN)*MTC_ADDR_ALIGN);
retry:
	if (mtc->r_outdata > 0) {
		size = FFMIN(size, (mtc->r_outdata - align));
		memcpy(buffer, mtc->r_outptr + align, size);
		mtc->r_outptr  += (size + align);
		mtc->r_outdata -= (size + align);
		return size;
	}

	while (mtc->r_indata < MTC_BLOCK_SIZE) {
		if (stream->interrupt_cbk && stream->interrupt_cbk())
			return -1;

		int n = sl->read(s->children, src + mtc->r_indata, (MTC_BLOCK_SIZE*MTC_READ_BLOCK_COUNT - mtc->r_indata));
		if (n < 0)
			return -1;

		mtc->r_indata += n;
	}

	blocks = mtc->r_indata/MTC_BLOCK_SIZE;
	if (!blocks)
		return -1;

	mtc->r_fpos = stream->pos - align;
	if (mtc_decrypt(mtc, dst, src, blocks*MTC_BLOCK_SIZE) != 0)
		return -1;

	mtc->r_outdata = MTC_BLOCK_SIZE*blocks;
	mtc->r_outptr  = dst;
	mtc->r_indata -= MTC_BLOCK_SIZE*blocks;
	if (mtc->r_indata > 0)
		memmove(src, src+MTC_BLOCK_SIZE*blocks, mtc->r_indata);

	goto retry;
}

static int mtc_write(GxStream *stream, uint8_t *buffer, size_t size)
{
	GxStreamMtc *s = (GxStreamMtc*)stream;
	GxMtc *mtc = &s->mtc;
	GxStreamClass *sl = NULL;
	int total_size, blocks, pad_len, out_size;
	uint8_t *src = NULL, *dst = NULL, *copy_addr = NULL;

	if (s->children)
		sl = GetGxStreamClassFromObject(s->children);

	if (!sl || !sl->write)
		return -1;

	if (!mtc->w_inbuf)
		mtc->w_inbuf = av_malloc(MTC_BLOCK_SIZE*MTC_WRITE_BLOCK_COUNT+MTC_ADDR_ALIGN);

	if (!mtc->w_outbuf)
		mtc->w_outbuf = av_malloc(MTC_BLOCK_SIZE*MTC_WRITE_BLOCK_COUNT+MTC_ADDR_ALIGN);

	if (!mtc->pad)
		mtc->pad = av_malloc(MTC_BLOCK_SIZE);

	total_size = size + mtc->pad_len;
	pad_len = total_size % MTC_BLOCK_SIZE;
	out_size = total_size - pad_len;
	blocks = out_size / MTC_BLOCK_SIZE;
	src = (uint8_t*)((((unsigned int)mtc->w_inbuf  + MTC_ADDR_ALIGN - 1)/MTC_ADDR_ALIGN)*MTC_ADDR_ALIGN);
	dst = (uint8_t*)((((unsigned int)mtc->w_outbuf + MTC_ADDR_ALIGN - 1)/MTC_ADDR_ALIGN)*MTC_ADDR_ALIGN);
	copy_addr = buffer;
	if (out_size) {
		while (blocks > 0) {
			int res_block = (blocks%MTC_WRITE_BLOCK_COUNT);
			int copy_len  = (res_block==0)?(MTC_WRITE_BLOCK_COUNT*MTC_BLOCK_SIZE):(res_block*MTC_BLOCK_SIZE);

			if (mtc->pad_len) {
				memcpy(src, mtc->pad, mtc->pad_len);
				memcpy(&src[mtc->pad_len], copy_addr, copy_len-mtc->pad_len);
			} else {
				memcpy(src, copy_addr, copy_len);
			}

			if (mtc_encrypt(mtc, dst, src, copy_len) != 0)
				return -1;

			if (sl->write(s->children, dst, copy_len) < 0)
				return -1;

			copy_addr += (copy_len-mtc->pad_len);
			blocks -= copy_len/MTC_BLOCK_SIZE;
			mtc->pad_len = 0;
		}
		memcpy(mtc->pad, &buffer[size-pad_len], pad_len);
	} else
		memcpy(&mtc->pad[mtc->pad_len], buffer, size);

	mtc->pad_len = pad_len;

	return size;
}

static int mtc_seek(GxStream *stream, off_t newpos)
{
	int ret = -1;
	GxStreamMtc *s = (GxStreamMtc*)stream;
	GxStreamClass *sl = NULL;
	GxMtc *mtc = &s->mtc;

	if (s->children) {
		sl = GetGxStreamClassFromObject(s->children);
		ret = (sl && sl->seek)?sl->seek(s->children, newpos):GX_PLAYER_ERROR;
		if (ret == GX_PLAYER_OK) {
			mtc->r_indata  = 0;
			mtc->r_outdata = 0;
			mtc->pad_len   = 0;
			s->parent.pos = s->children->pos;
		}
	}

	return ret;
}

static int mtc_control(GxStream *stream, int cmd, void *arg)
{
	int ret = -1;
	GxStreamMtc *s = (GxStreamMtc*)stream;
	GxStreamClass *sl = NULL;
	if (s->children) {
		sl = GetGxStreamClassFromObject(s->children);
		ret = (sl && sl->control)?sl->control(s->children, cmd, arg):GX_PLAYER_ERROR;
		if (ret == GX_PLAYER_OK) {
			//s->parent.pos = s->children->pos;
			s->parent.end_pos = s->children->end_pos;
		}
	}
	return ret;
}

void mtc_close(GxStream *stream)
{
	GxStreamMtc *s = (GxStreamMtc*)stream;
	GxMtc *mtc = &s->mtc;

	if (mtc->w_inbuf) {
		av_free(mtc->w_inbuf);
		mtc->w_inbuf = NULL;
	}
	if (mtc->w_outbuf) {
		av_free(mtc->w_outbuf);
		mtc->w_outbuf = NULL;
	}
	if (mtc->r_inbuf) {
		av_free(mtc->r_inbuf);
		mtc->r_inbuf = NULL;
	}
	if (mtc->r_outbuf) {
		av_free(mtc->r_outbuf);
		mtc->r_outbuf = NULL;
	}
	if (mtc->pad) {
		av_free(mtc->pad);
		mtc->pad = NULL;
	}
	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return;
}

GxStreamClass gx_stream_mtc = {
	._inherit = {
		._inherit = {
			.name = "StreamMtc",
			.parent = &gx_streambase,
			.size = sizeof(GxStreamMtc),
		},
	},
	.protocols = {"mtc", NULL},
	DEF_AUTHOR("Stream","mtc","No description","L.F","No comment"),

	.flags   = 0,
	.open    = mtc_open,
	.read    = mtc_read,
	.write   = mtc_write,
	.seek    = mtc_seek,
	.control = mtc_control,
	.close   = mtc_close,
};
