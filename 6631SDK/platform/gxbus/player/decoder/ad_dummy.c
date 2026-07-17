
#include "ad.h"


static int ad_dummy_open(GxAVCodec*  ad)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_dummy_control(GxAVCodec*  ad, int cmd, void* args)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_dummy_close(GxAVCodec*  ad)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int ad_dummy_decode(GxAVCodec* ad, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

GxAVCodecClass gx_audio_decoder_dummy = {
	._inherit = {
		._inherit = {
			.name    = "DummyAd",
			.parent  = (void* )&gx_AudioDecoderBase,
			.size    = sizeof(GxAudioCodec),
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.config = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("AudioDecoder","Dummy","No description","L.F","No comment"),

	.name    = "Dummy Audio Decoder",
	.format  = {PLAYER_ACODEC_DUMMY, -1},
	.open    = ad_dummy_open,
	.close   = ad_dummy_close,
	.control = ad_dummy_control,
	.decode  = ad_dummy_decode,
};


