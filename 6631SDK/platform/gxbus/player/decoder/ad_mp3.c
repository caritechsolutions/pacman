#include"gx_common.h"
#include "ad.h"

#include "mp3lib/mp3.h"

extern int GxAudioDecoder_BufferInit(GxAudioCodec*  AudioCodec);


/**********************************************************/

extern GxFifo* adfifo;

int mp3_read(unsigned char* buf, int size)
{
	if (GxFifo_GetLength(adfifo) >= size)
		return GxFifo_Read(adfifo, (unsigned char* )buf, size);
	else
		return 0;
}

/***********************************************************************/

static int mp3_open(GxAVCodec*  codec)
{
	GxAudioCodec* audio_codec = (GxAudioCodec *) (codec);
	GxStreamAudioHeader* sh = (GxStreamAudioHeader *) codec->header;

	MP3_Init();
	if (GxAudioDecoder_BufferInit(audio_codec) != GX_PLAYER_OK)
		return GX_PLAYER_ERROR;;

	sh->channels = 2;	// hack

	return GX_PLAYER_OK;

}

static int mp3_control(GxAVCodec*  codec, int query, void* arg)
{
	return GX_PLAYER_OK;
}

static int mp3_close(GxAVCodec*  codec)
{
	return GX_PLAYER_OK;
}

static int mp3_decode(GxAVCodec*  codec, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	return 	MP3_DecodeFrame(buf, -1);
}



static void ad_mp3_run_thread(void* filter)
{
	GxAudioCodec* audio_codec = (GxAudioCodec *) (filter);
	GxMediaFilter* mf = GXMEDIAFILTER(filter);
	GxPin* out_pin, *in_pin;
	size_t size;	
	
	out_pin = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_PCM_OUT);
	in_pin = GxMediaFilter_FindPin(filter, GX_AD_PIN_NAME_IN);

	while (mf->status != GX_MFT_STATE_STOPPED && in_pin && out_pin && in_pin->source && in_pin->source->fifo) {

		if (in_pin && out_pin && in_pin->source && in_pin->source->fifo ) {
			do {
				adfifo = in_pin->source->fifo;
				size =  mp3_decode(&audio_codec->parent, NULL,NULL,(unsigned char* )audio_codec->out_buffer, 
							  audio_codec->out_buffer_size);
				if (size > 0) {
				 	unsigned int write_size = 0;
					while(write_size < size){
						write_size += GxFifo_Write(out_pin->fifo, 
							(unsigned char*)audio_codec->out_buffer +write_size,size-write_size);
						GxCore_ThreadDelay(10);
					} 
				}
				GxCore_ThreadDelay(10);
			} while (size > 0 && mf->status == GX_MFT_STATE_RUNNING);

			GxCore_ThreadDelay(10);
			if(in_pin->source->filter->status == GX_MFT_STATE_STOPPED)
				break;
		}
	}
	
	GxMediaFilter_Stop(mf);
	gxlogf("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

}

static int ad_mp3_run(GxMediaFilter*  filter)
{
	GxAVCodec* d = GXDECODEROBJECT(filter);
	if (GxCore_ThreadCreate("ad_mp3_run_thread", &d->run_pthread, ad_mp3_run_thread, filter, 1024*30, GXOS_DEFAULT_PRIORITY) == 0) {
		filter->status = GX_MFT_STATE_RUNNING;
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int ad_mp3_pause(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int ad_mp3_stop(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

static int ad_mp3_resume(GxMediaFilter*  filter)
{
	return GX_PLAYER_OK;
}

GxAVCodecClass gx_audio_decoder_MP3 = {
	._inherit = {
		._inherit = {
			.name    = "AudioDecoderMP3",
			.parent  = (void* )&gx_AudioDecoderBase,
			.size    = sizeof(GxAudioCodec),
			.version = {1, 0}
		},
		.run    = ad_mp3_run,
		.pause  = ad_mp3_pause,
		.resume = ad_mp3_resume,
		.stop   = ad_mp3_stop,
	},
	.author = {
		.info    = "MP3-File decoder",
		.name    = "mp3",
		.desc    = "mp3 files",
		.author  = "L.F",
		.comment = "based on the code from mplayer (probably Arpi)"},

	.name    = "SW MP3 AudioDecoder",
	.format  = {0x50, 0x55, 0x5500736d, -1},
	.open    = mp3_open,
	.close   = mp3_close,
	.control = mp3_control,
	.decode  = mp3_decode,
};
