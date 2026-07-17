#include "gx_config.h"
#include "gxtype.h"
#include "gxplayer_decoder.h"
#include "audio_dec.h"
#include "gx_mem.h"

#define MAX_AUDIO_DECODER_NUM (16)

static GxAudioDecoder *dec_list[MAX_AUDIO_DECODER_NUM] = {
	NULL,
};

static GxAudioDecoder *find_audio_dec(AudioCodecType type)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_AUDIO_DECODER_NUM; i++) {
		GxAudioDecoder *tmpDec = dec_list[i];
		if (tmpDec && (tmpDec->codecType == type))
			return tmpDec;
	}

	return NULL;
}

static int add_audio_dec(GxAudioDecoder *dec)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_AUDIO_DECODER_NUM; i++) {
		GxAudioDecoder *tmpDec = dec_list[i];
		if ((!tmpDec) || (tmpDec->codecType == dec->codecType))
			break;
	}

	if (i < MAX_AUDIO_DECODER_NUM) {
		dec_list[i] = dec;
		return 0;
	}

	return -1;
}

int GxAudioRegisterDecoder(GxAudioDecoder *dec)
{
	return add_audio_dec(dec);
}

GxAudioDecoderContext* GxAudioDecoderCreate(AudioCodecType type, GxAudioDecoderParam *param)
{
	GxAudioDecoder *dec = NULL;
	GxAudioDecoderContext *ctx = NULL;

	dec = find_audio_dec(type);
	if (!dec)
		return NULL;

	ctx = av_mallocz(sizeof(GxAudioDecoderContext));
	if (!ctx)
		return NULL;

	ctx->dec  = dec;
	if (ctx->dec->create) {
		ctx->priv = ctx->dec->create(param);
		if (!ctx->priv) {
			av_free(ctx);
			return NULL;
		}
	}

	return ctx;
}

GxAudioDecoderStatus GxAudioDecoderDecode(GxAudioDecoderContext *ctx)
{
	if (ctx) {
		if (ctx->dec && ctx->dec->decode)
			return ctx->dec->decode(ctx->priv);
	}

	return GX_AUDIO_DECODER_ERROR;
}

void GxAudioDecoderDestory(GxAudioDecoderContext *ctx)
{
	if (ctx) {
		if (ctx->dec && ctx->dec->destory)
			ctx->dec->destory(ctx->priv);

		ctx->dec  = NULL;
		ctx->priv = NULL;
		av_free(ctx);
	}
}

void GxAudioDecoderUpdate(GxAudioDecoderContext *ctx)
{
	if (ctx) {
		if (ctx->dec && ctx->dec->update)
			ctx->dec->update(ctx->priv);
	}
}

int GxAudioDecoderProbe(AudioCodecType codecType)
{
	GxAudioDecoder *dec = find_audio_dec(codecType);

	return (dec ? dec->score : 0);
}
