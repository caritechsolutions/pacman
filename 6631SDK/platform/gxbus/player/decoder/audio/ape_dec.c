#include "gxplayer_decoder.h"
#include "../../avformat/avcodec.h"
#include "../../avformat/avformat.h"
#include "../../avcodec/apedec.h"

#define MAX_APE_BUF_SIZE (16*1024)
typedef struct {
	AVCodecContext                avctx;
	int                           inited;
	int                           update;
	void                          *priv;
	GxAudioDecoderReadCallback    rc;
	GxAudioDecoderWriteCallback   wc;
	GxAudioDecoderControlCallback cc;
	unsigned char                 extradata[6];
	unsigned int                  extradata_size;
	unsigned char                 outdata[MAX_APE_BUF_SIZE];
	AVPacket                      *avpkt;
} GxAudioApeDecoder;

static int _decode_frame(GxAudioApeDecoder *apedec, AVPacket *avpkt)
{
	unsigned int channels = apedec->avctx.channels;
	unsigned int bits = 0, nb_samples  = 0, samples = 0;
	int got_frame_ptr = 0, ret = 0;

	switch (apedec->avctx.bits_per_coded_sample) {
	case 8:
		bits = 1;
		break;
	case 16:
		bits = 2;
		break;
	case 24:
		bits = 4;
	default:
		bits = 4;
		break;
	}
	samples = nb_samples = MAX_APE_BUF_SIZE / (bits * channels);
	ret = ape_decode_frame(&apedec->avctx,
			apedec->outdata,
			&samples,
			&got_frame_ptr,
			avpkt);
	if (ret < 0)
		return -1;

	if (got_frame_ptr && (samples > 0)) {
		apedec->wc(apedec->priv, apedec->outdata, (samples * bits * channels), avpkt->pts);
		avpkt->pts = -1;
	}

	return (ret == avpkt->size) ? 1 : 0;
}

static void* ape_decode_create(GxAudioDecoderParam *param)
{
	GxAudioApeDecoder* apedec = av_mallocz(sizeof(GxAudioApeDecoder));

	apedec->rc = param->rc;
	apedec->wc = param->wc;
	apedec->cc = param->cc;
	apedec->priv = param->priv;

	return (void *)apedec;
}

static GxAudioDecoderStatus ape_decode_decode(void *handle)
{
	int ret = 0;
	unsigned char tags[4];
	unsigned char buff[4];
	GxAudioApeDecoder* apedec = (GxAudioApeDecoder*)handle;
	GxAudioDecoderBufInfo wInfo, rInfo;
	int pts = -1;

	if (!apedec) return GX_AUDIO_DECODER_ERROR;

	apedec->cc(apedec->priv, GX_AUDIO_DECODER_GET_WBUF_INFO, &wInfo);
	if (wInfo.free < ((4 * wInfo.cap) / 5))
		return GX_AUDIO_DECODER_WBUF_FULL;

	if (apedec->avpkt) {
		ret = _decode_frame(apedec, apedec->avpkt);
		if (ret != 0) {
			av_free(apedec->avpkt->data);
			av_free(apedec->avpkt);
			apedec->avpkt = NULL;
		}
		return GX_AUDIO_DECODER_OK;
	}

	apedec->cc(apedec->priv, GX_AUDIO_DECODER_GET_RBUF_INFO, &rInfo);
	if (rInfo.size <= 0) {
		if (apedec->update)
			return GX_AUDIO_DECODER_EOF;
		else
			return GX_AUDIO_DECODER_RBUF_EMPTY;
	}

	ret = apedec->rc(apedec->priv, tags, 4, &pts);
	if (ret != 4) {
		gxlogd("%s %d: header tags error (ret %d)\n", __FUNCTION__, __LINE__, ret);
		return GX_AUDIO_DECODER_ERROR;
	}

	ret = apedec->rc(apedec->priv, buff, 4, ((pts !=- 1) ? &pts : NULL));
	if (ret != 4) {
		gxlogd("%s %d: header size error (ret %d)\n", __FUNCTION__, __LINE__, ret);
		return GX_AUDIO_DECODER_ERROR;
	}

	if ((tags[0] == 'M') && (tags[1] == 'A') && (tags[2] == 'C') && (tags[3] == ' ')) {
		unsigned int   packet_size = ((buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | buff[3]);
		unsigned char  packet[32];

		ret = apedec->rc(apedec->priv, packet, packet_size, NULL);
		if (ret != packet_size) {
			gxlogd("%s %d: header size error (ret %d)\n", __FUNCTION__, __LINE__, ret);
			return GX_AUDIO_DECODER_ERROR;
		}
		apedec->extradata_size = 6;
		memcpy(apedec->extradata, packet, apedec->extradata_size);
		if (!apedec->inited) {
			AVCodecContext *avctx = &apedec->avctx;
			GxAudioDecoderPcmInfo info;

			avctx->codec_type     = CODEC_TYPE_AUDIO;
			avctx->extradata      = apedec->extradata;
			avctx->extradata_size = apedec->extradata_size;
			avctx->bits_per_coded_sample = *((short *)(packet +  6));
			avctx->channels              = *((short *)(packet +  8));
			avctx->sample_rate           = *((int   *)(packet + 12));
			ape_decode_init(avctx);

			info.sample_rate     = avctx->sample_rate;
			info.channel_num     = avctx->channels;
			info.bits_per_sample = avctx->bits_per_coded_sample;
			info.channel_flag    = 0;
			info.interlace       = 1;
			info.endian          = 0;
			info.float_en        = 0;
			apedec->cc(apedec->priv, GX_AUDIO_DECODER_SET_PCM_INFO, (void *)&info);
			apedec->inited = 1;
		}
	} else if ((tags[0] == 'A') && (tags[1] == 'P') && (tags[2] == 'E') && (tags[3] == ' ')) {
		if (apedec->inited) {
			AVPacket *avpkt = av_mallocz(sizeof(AVPacket));

			avpkt->size = ((buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | buff[3]);
			avpkt->data = av_malloc(avpkt->size);
			ret = apedec->rc(apedec->priv, avpkt->data, avpkt->size, ((pts !=- 1) ? &pts : NULL));
			if (ret != avpkt->size) {
				gxlogd("%s %d: header size error (ret %d)\n", __FUNCTION__, __LINE__, ret);
				av_free(avpkt->data);
				return GX_AUDIO_DECODER_ERROR;
			}
			avpkt->pts = pts;

			ret = _decode_frame(apedec, avpkt);
			if (ret != 0) {
				av_free(avpkt->data);
				av_free(avpkt);
			} else
				apedec->avpkt = avpkt;
		}
	}

	return GX_AUDIO_DECODER_OK;
}

static void ape_decode_destory(void *handle)
{
	GxAudioApeDecoder* apedec = (GxAudioApeDecoder*)handle;

	if (apedec) {
		if (apedec->avpkt) {
			if (apedec->avpkt->data)
				av_free(apedec->avpkt->data);
			av_free(apedec->avpkt);
			apedec->avpkt = NULL;
		}

		if (apedec->inited) {
			ape_decode_close(&apedec->avctx);
			apedec->inited = 0;
		}

		av_free(apedec);
	}
}

static void ape_decode_update(void *handle)
{
	GxAudioApeDecoder* apedec = (GxAudioApeDecoder*)handle;

	apedec->update = 1;
	return;
}

GxAudioDecoder ape_decoder = {
	.name      = "APE decoder",
	.score     = 50,
	.codecType = AUDIO_CODEC_APE,
	.create    = ape_decode_create,
	.decode    = ape_decode_decode,
	.destory   = ape_decode_destory,
	.update    = ape_decode_update,
};
