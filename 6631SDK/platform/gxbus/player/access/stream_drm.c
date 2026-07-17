#include "gx_demux.h"
#include "gx_options.h"
#include "stream_drm.h"
#include "stream_m3u8.h"

extern GxStreamOptionCallback public_stream_callback;

static int drm_open(GxStream* stream, int mode)
{
	GxStreamParam param;
	GxStreamDrm *s = (GxStreamDrm*)stream;
	GxStreamClass* sl = NULL;
	char *key = NULL, *iv = NULL;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &stream->interrupt_cbk);

	memset(&s->drm, 0, sizeof(GxDrm));
	key = GxOptions_Get_By_Id(stream->options, " -H", OPTIONS_ENC_KEY_URL);
	if (key) {
		s->drm.decrypt_key_url = av_strdup(key);
		s->drm.encrypt_key_url = av_strdup(key);
		s->drm.has_key = 1;
	}

	iv = GxOptions_Get_By_Id(stream->options, " -H", OPTIONS_ENC_IV);
	if (iv) {
		s->drm.encrypt_iv_len = hex_to_data(s->drm.encrypt_iv, iv);
		s->drm.decrypt_iv_len = hex_to_data(s->drm.decrypt_iv, iv);
		s->drm.has_iv = 1;
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
		goto drm_error;
	}

	sl = GetGxStreamClassFromObject(s->children);
	if (!sl) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto drm_error;
	}

	if (!s->drm.has_key) {
		gxlogd("%s %d failed\n", __FUNCTION__, __LINE__);
		goto drm_error;
	}

	sl->control(s->children, GX_STREAM_CTRL_GET_SIZE, &s->drm.con_l);
	s->drm.res_l = s->drm.con_l;
	s->drm.newFlags = 1;
	s->drm.data_url = av_strdup(stream->url);
	sl->control(s->children, GX_STREAM_CTRL_GET_TOTAL_TIME, &s->drm.duration);

	s->parent.file_format  = s->children->file_format;
	s->parent.demuxer_type = s->children->demuxer_type;
	return GX_PLAYER_OK;

drm_error:
	if (s->drm.data_url) {
		av_free(s->drm.data_url);
		s->drm.data_url = NULL;
	}

	if (s->drm.decrypt_key_url) {
		av_free(s->drm.decrypt_key_url);
		s->drm.decrypt_key_url = NULL;
	}

	if (s->drm.encrypt_key_url) {
		av_free(s->drm.encrypt_key_url);
		s->drm.encrypt_key_url = NULL;
	}

	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return GX_PLAYER_ERROR;
}

static void drm_close(GxStream*  stream)
{
	GxStreamDrm* s = (GxStreamDrm*)stream;

	if (s->drm.data_url) {
		av_free(s->drm.data_url);
		s->drm.data_url = NULL;
	}

	if (s->drm.decrypt_key_url) {
		av_free(s->drm.decrypt_key_url);
		s->drm.decrypt_key_url = NULL;
	}

	if (s->drm.encrypt_key_url) {
		av_free(s->drm.encrypt_key_url);
		s->drm.encrypt_key_url = NULL;
	}

	if (s->children) {
		GxStream_Close(s->children);
		s->children = NULL;
	}

	return;
}

static int drm_read(GxStream *stream, uint8_t *buffer, size_t max_len)
{
	GxStreamDrm* s = (GxStreamDrm*)stream;
	GxDrm* drm  = &s->drm;
	GxStreamClass* sl = NULL;
    int blocks;

	if (!s->children)
		return -1;

retry:
	if (drm->outdata > 0) {
		max_len = FFMIN(max_len, drm->outdata);
		memcpy(buffer, drm->outptr, max_len);
		drm->outptr  += max_len;
		drm->outdata -= max_len;
		drm->res_l   -= max_len;
		return max_len;
	}

	while ((drm->indata - drm->indata_used) < 2*DRM_BLOCKSIZE) {
		if (stream->interrupt_cbk && stream->interrupt_cbk())
			return -1;

		sl = GetGxStreamClassFromObject(s->children);
		int n = sl?sl->read(s->children,
				drm->inbuffer + drm->indata, sizeof(drm->inbuffer) - drm->indata):-1;
		if (n < 0) {
			drm->res_l = 0;
			break;
		}
		drm->indata += n;
	}

	blocks = (drm->indata - drm->indata_used)/DRM_BLOCKSIZE;
	if (!blocks) {
		drm->res_l = 0;
		return 0;
	}

	if (public_stream_callback) {
		GxStreamOptionCallbackParam param;
		int ret = 0;

		param.keyUrl     = drm->decrypt_key_url;
		param.dataUrl    = drm->data_url;
		param.dataIn     = drm->inbuffer + drm->indata_used;
		param.dataOut    = drm->outbuffer;
		param.dataInLen  = blocks * DRM_BLOCKSIZE;
		param.dataOutLen = blocks * DRM_BLOCKSIZE;
		param.keyIV      = drm->decrypt_iv;
		param.keyIVSize  = drm->decrypt_iv_len;
		param.newTSFlag  = drm->newFlags;

		ret = public_stream_callback(&param);
		if (ret != 0) {
			gxlogd("error: %s %d (ret %d)\n", __FUNCTION__, __LINE__, ret);
			return -1;
		}

		drm->newFlags = 0;
		if (param.dataInLen != param.dataOutLen) {
			gxlogd("warning: %s %d (in : %d, out: %d)\n", __FUNCTION__, __LINE__, param.dataInLen, param.dataOutLen);
			return 0;
		}
	} else {
		gxlogd("error: %s %d (callback NULL)\n", __FUNCTION__, __LINE__);
		return -1;
	}

	drm->outdata      = DRM_BLOCKSIZE * blocks;
	drm->outptr       = drm->outbuffer;
	drm->indata_used += DRM_BLOCKSIZE * blocks;

	if (drm->indata_used >= sizeof(drm->inbuffer)/2) {
		memmove(drm->inbuffer,
				drm->inbuffer + drm->indata_used, drm->indata - drm->indata_used);
		drm->indata     -= drm->indata_used;
		drm->indata_used = 0;
	}

	goto retry;
}

static int drm_write(GxStream *stream, uint8_t *buffer, size_t size)
{
	return -1;
}

static int drm_seek(GxStream* stream, off_t newpos)
{
	int ret = -1;
	GxStreamDrm *s = (GxStreamDrm*)stream;
	GxStreamClass *sl = NULL;
	GxDrm *drm = &s->drm;

	if (s->children) {
		sl = GetGxStreamClassFromObject(s->children);
		ret = (sl && sl->seek)?sl->seek(s->children, newpos):GX_PLAYER_ERROR;
		if (ret == GX_PLAYER_OK) {
			drm->indata  = 0;
			drm->outdata = 0;
			drm->pad_len = 0;
			drm->res_l   = (drm->con_l - s->children->pos);
			s->parent.pos = s->children->pos;
		}
	}

	return ret;
}

static int drm_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamDrm* s = (GxStreamDrm*)stream;
	GxDrm* drm  = &s->drm;

	if(drm->outptr)
		return GX_PLAYER_ERROR;

	if(drm->res_l >= 0) {
		drm->res_l   += size;
		drm->outptr  -= size;
		drm->outdata += size;
	}

	return GX_PLAYER_OK;
}

static int drm_control(GxStream *stream, int cmd, void *arg)
{
	GxStreamDrm* s = (GxStreamDrm*)stream;
	GxDrm* drm  = &s->drm;

	switch (cmd) {
		case GX_STREAM_CTRL_GET_SIZE:
		{
			*(off_t*)arg = drm->con_l;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_RES_SIZE:
		{
			*(off_t*)arg = drm->res_l;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_ROLL_BACK:
		{
			GxStream_RollBack* rol = arg;
			if (!rol)
				return GX_PLAYER_ERROR;

			drm_rollback(stream, rol->buffer, rol->size);
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_GET_TOTAL_TIME:
		{
			if(drm->duration > 0){
				*(int*)arg = drm->duration;
				return GX_PLAYER_OK;
			}
		}
		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

GxStreamClass gx_stream_drm = {
	._inherit = {
		._inherit = {
			.name = "StreamDrm",
			.parent = &gx_streambase,
			.size = sizeof(GxStreamDrm),
		},
	},
	.protocols = {"drm", NULL},
	DEF_AUTHOR("Stream","drm","No description","L.F","No comment"),

	.flags   = GX_STREAM_NEED_PROBE,
	.open    = drm_open,
	.read    = drm_read,
	.write   = drm_write,
	.seek    = drm_seek,
	.control = drm_control,
	.close   = drm_close,
};
