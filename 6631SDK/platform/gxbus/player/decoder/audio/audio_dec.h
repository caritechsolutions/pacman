#ifndef __AUDIO_DEC_H__
#define __AUDIO_DEC_H__

typedef struct {
	GxAudioDecoder *dec;
	void           *priv;
} GxAudioDecoderContext;

int                    GxAudioDecoderProbe  (AudioCodecType codecType);
GxAudioDecoderContext* GxAudioDecoderCreate (AudioCodecType type, GxAudioDecoderParam *param);
GxAudioDecoderStatus   GxAudioDecoderDecode (GxAudioDecoderContext *ctx);
void                   GxAudioDecoderDestory(GxAudioDecoderContext *ctx);
void                   GxAudioDecoderUpdate (GxAudioDecoderContext *ctx);
#endif
