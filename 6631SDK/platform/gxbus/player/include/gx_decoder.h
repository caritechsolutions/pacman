#ifndef __GX_DECODER_H__
#define __GX_DECODER_H__

#include "stheader.h"
#include "gx_mediafilter.h"

#define GX_DECODER_SUPPORT_MAX   (128)

#define GX_VD_PIN_NAME_IN    "vd_in"
#define GX_VD_PIN_NAME_OUT   "vd_out"

#define GX_AD_PIN_NAME_ESA_IN    "esa_in"
#define GX_AD_PIN_NAME_ADESA_IN  "ad_esa_in"
#define GX_AD_PIN_NAME_SPD_IN    "spd_in"
#define GX_AD_PIN_NAME_PCM_OUT   "pcm_out"
#define GX_AD_PIN_NAME_ADPCM_OUT "ad_pcm_out"
#define GX_AD_PIN_NAME_AC3_OUT   "ac3_out"
#define GX_AD_PIN_NAME_EAC3_OUT  "eac3_out"
#define GX_AD_PIN_NAME_DTS_OUT   "dts_out"
#define GX_AD_PIN_NAME_AAC_OUT   "aac_out"

#define GX_SD_PIN_NAME_IN        "sd_in"
#define GX_SD_PIN_NAME_OUT       "sd_out"

typedef enum
{
	GX_VD_CTRL_PAUSE          ,
	GX_VD_CTRL_RESUME         ,
	GX_VD_CTRL_UPDATE         ,
	GX_VD_CTRL_REFRESH        ,
	GX_VD_CTRL_SET_SPEED      ,
	GX_VD_CTRL_GET_STATUS     ,
	GX_VD_CTRL_GET_CUR_PTS    ,
	GX_VD_CTRL_GET_FRAME_INFO ,
	GX_VD_CTRL_CHECK_FRAME_OK ,
	GX_VD_CTRL_CONTINUE       ,
	GX_VD_CTRL_CHECK_DISPLAY  ,
	GX_VD_CTRL_START_DECODE   ,
	GX_VD_CTRL_SET_SYNC       ,
	GX_VD_CTRL_JPEG_RESET     ,
	GX_VD_CTRL_JPEG_END       ,
	GX_VD_CTRL_GET_JPEG_STATE ,
	GX_VD_CTRL_SET_DELAY      ,
	GX_VD_CTRL_READ_SUBCC     ,
	GX_VD_CTRL_JUMP_RESET     ,
	GX_VD_CTRL_ONE_FRAME_OVER ,
	GX_VD_CTRL_GET_DISPLAY_PTS,
	GX_VD_CTRL_CHECK_DYNAMIC_RANGE_CHANGE,
	GX_VD_CTRL_FREEZE_VIDEO   ,
	GX_VD_CTRL_SET_XSPEED     ,
}GxVdCtrl;

typedef enum
{
	GX_AD_CTRL_PAUSE          ,
	GX_AD_CTRL_RESUME         ,
	GX_AD_CTRL_UPDATE         ,
	GX_AD_CTRL_REFRESH        ,
	GX_AD_CTRL_SET_SPEED      ,
	GX_AD_CTRL_GET_STATUS     ,
	GX_AD_CTRL_GET_FRAME_INFO ,
	GX_AD_CTRL_SWITCH_HEADER  ,
	GX_AD_CTRL_GET_PCM_SIZE   ,
	GX_AD_CTRL_RESET          ,
	GX_AD_CTRL_SUPPORT_SPEED  ,
}GxAdCtrl;

typedef enum
{
	GX_SD_CTRL_PAUSE          ,
	GX_SD_CTRL_RESUME         ,
	GX_SD_CTRL_UPDATE         ,
	GX_SD_CTRL_REFRESH        ,
	GX_SD_CTRL_GET_FRAME_INFO ,
	GX_SD_CTRL_SWITCH_HEADER  ,
}GxSdCtrl;

typedef enum {
	AVSTREAM_ES = 0,
	AVSTREAM_TS,
} AVStreamType;

typedef struct {
	unsigned char *data;
	unsigned int  size;
	unsigned int  timeout;
} AVCodecSubCC;

typedef struct gx_avcodec{
	GxMediaFilter       filter;
	void                *header;

	int                 decoder_id;
	char                *name;
	int32_t             run_pthread;
	void                *priv;
}GxAVCodec;

typedef struct gx_audio_codec {
	GxAVCodec           parent;

	int                 out_minsize;
	char*               out_buffer;
	int                 out_buffer_len;
	int                 out_buffer_size;

}GxAudioCodec;

typedef struct gx_video_codec {
	GxAVCodec           parent;

} GxVideoCodec;

typedef struct gx_sub_codec {
	GxAVCodec           parent;

} GxSubCodec;

typedef struct gx_av_codec_class {
	GxMediaFilterClass  _inherit;
	AUTHER;

	const char*         name;
	int                 format[GX_DECODER_SUPPORT_MAX];

	int  (*probe)  (AudioCodecType codec_type, int need_decode);
	int  (*open)   (GxAVCodec *codec);
	int  (*close)  (GxAVCodec *codec);
	int  (*control)(GxAVCodec *codec, int query, void* args);
	int  (*decode) (GxAVCodec *codec, void **outdata, int *outdata_size, uint8_t *buf, int buf_size);
} GxAVCodecClass;

int GxAudioDecoder_CheckReOpen(AudioCodecType codec1, AudioCodecType codec2);

GxVideoCodec *GxVideoDecoder_Open(GxStreamVideoHeader *header);
GxAudioCodec *GxAudioDecoder_Open(GxStreamAudioHeader *header);
GxSubCodec   *GxSubDecoder_Open(GxStreamSubHeader *header);

void GxAudioDecoder_Close(GxAudioCodec * codec);
void GxVideoDecoder_Close(GxVideoCodec * codec);
void GxSubDecoder_Close(GxSubCodec * codec);

int   GxAudioDecoder_Decode(GxAudioCodec *sh_audio, unsigned char *buffer, int minlen, int maxlen);
void* GxVideoDecoder_Decode(GxVideoCodec *sh_video, unsigned char *start,  int in_size, int drop_frame, double pts);
int   GxSubDecoder_Decode(GxSubCodec *sh_sub, unsigned char *buffer, int minlen, int maxlen);

int GxAudioDecoder_Control(GxAudioCodec * sh_audio, int cmd, void *arg);
int GxVideoDecoder_Control(GxVideoCodec * sh_video, int cmd, void *arg);
int GxSubDecoder_Control(GxSubCodec * sh_sub, int cmd, void *arg);


void GxAudioDecoderClass_Init(void);
void GxAudioDecoderClass_Destroy(void);
void GxVideoDecoderClass_Init(void);
void GxVideoDecoderClass_Destroy(void);
void GxSubDecoderClass_Init(void);
void GxSubDecoderClass_Destroy(void);


#define GXDECODEROBJECT(s)                (GxAVCodec*)(s)
#define GxGetDecoderClassFromObject(s)    (GxAVCodecClass*)(s->parent.filter.obj.cls)
#define GXDECODER_CLASS(s)                (GxAVCodecClass *)(s)


#endif

