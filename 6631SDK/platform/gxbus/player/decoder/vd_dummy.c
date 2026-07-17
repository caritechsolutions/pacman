
#include "vd.h"
#include "gx_avutil.h"
#include "gx_image.h"
#include "vd_es2frame.h"


static int vd_dummy_open(GxAVCodec*  vd)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vd_dummy_control(GxAVCodec*  vd, int cmd, void* args)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vd_dummy_close(GxAVCodec*  vd)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vd_dummy_decode(GxAVCodec* vd, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

GxAVCodecClass gx_decoder_vd_dummy = {
	._inherit = {
		._inherit = {
			.name    = "DummyVd",
			.parent  = (void* )&gx_VideoDecoderBase,
			.size    = sizeof(GxVideoCodec),
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("VideoDecoder","Dummy","No description","L.F","No comment"),

	.name    = "Dummy Video Decoder",
	.format  = {PLAYER_VCODEC_DUMMY, -1},
	.open    = vd_dummy_open,
	.close   = vd_dummy_close,
	.control = vd_dummy_control,
	.decode  = vd_dummy_decode,
};

