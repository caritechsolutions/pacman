#ifndef __GX_AUDIO_DONGLE_H__
#define __GX_AUDIO_DONGLE_H__

typedef enum {
	GXAUDIO_DONGLE_PCM     = 0, //音频编码，PCM编码
	GXAUDIO_DONGLE_SBC     = 1, //音频编码，SBC编码
} GxAudioDongleFormat;

typedef struct {
	unsigned int sample_freq; //PCM编码音频的采样率
	unsigned int channel_num; //PCM编码音频的通道数
	unsigned int bits;        //PCM编码音频的位宽
	unsigned int interlace;   //PCM编码音频的交织/非交织
	unsigned int endian;      //PCM编码音频的大小端,1:大端，0:小端
} GxAudioDonglePCM;

typedef struct {
	GxAudioDongleFormat  format;      //音频编码
	union {
		GxAudioDonglePCM pcm;         //PCM编码音频信息
	};
	unsigned char         keep_pulse; //持续性输出,例如: 静音或获取不到音频时, 输出 0 数据.
} GxAudioDongleCap;

typedef struct {
	unsigned char       *buf_ptr;     //传输数据buf指针
	unsigned int         buf_size;    //传输数据buf大小
	AudioCodecType       codec;       //传输数据编码
	unsigned int         frame_no;    //传输数据帧号
	int                  sample_val;  //传输数据解码帧平均样点值
	int                  down_mix;    //传输数据解码是否downmix
	int                  volume_db;   //传输数据解码音量值
} GxAudioDongleData;

typedef struct {
	unsigned int datalen;             //已缓冲数据大小
	unsigned int freelen;             //可缓冲数据大小
} GxAudioDongleBufInfo;

typedef struct {
	const char           *name;                                     ///< 音频 dongle ops 名称, 唯一标识, 用于多个 dongle 匹配, 必选.
#define GX_AUDIO_DONGLE_DEC_CDD_FRAME_NAME  "dec.cdd.frame"         ///< 音频解码前的CDD数据.
#define GX_AUDIO_DONGLE_DEC_PCM_FRAME_NAME  "dec.pcm.frame"         ///< 音频解码后的PCM数据.
#define GX_AUDIO_DONGLE_OUT_PCM_FRAME_NAME  "out.pcm.frame"         ///< 音频播放前的PCM数据.
	const char           *private_name;                             ///< 选择名称为 NULL, 选择"dec.pcm.frame"音频解码原始帧.
	const char           *author;                                   ///< 音频 dongle ops 作者.
	unsigned int         (*get_cap)    (GxAudioDongleCap  *cap);    ///< 音频 dongle ops 可以支持音频数据信息.
	unsigned int         (*get_bufinfo)(GxAudioDongleBufInfo *buf); ///< 音频 dongle ops 缓冲信息.
	unsigned int         (*push)       (GxAudioDongleData *data);   ///< 音频 dongle ops 推送数据.
	int                  (*pull)       (GxAudioDongleData *data);   ///< 音频 dongle ops 拉取数据.
} GxAudioDongleOps;

/**
 * \brief   注册音频 dongle.
 *
 * \param   ops       [in] 音频 dongle 操作类，需要应用实现.
 * \retval  0    成功.
 * \retval  非0 失败.
 **/
int GxAudioRegisterDongle(GxAudioDongleOps *ops);

/**
 * \brief   取消注册音频 dongle.
 *
 * \param   ops       [in] 音频 dongle 操作类.
 * \retval  0    成功.
 * \retval  非0 失败.
 **/
int GxAudioUnRegisterDongle(GxAudioDongleOps *ops);

typedef enum {
	GXAUDIO_DONGLE_STEREO_CHANNEL = 0, //立体声音频输出
	GXAUDIO_DONGLE_LEFT_CHANNEL,       //左声道音频输出
	GXAUDIO_DONGLE_RIGHT_CHANNEL,      //右声道音频输出
} GxAudioDongleChannel;

typedef struct {
	GxAudioDongleChannel channel;      //音频声道信息
	int                  volume;       //音频音量大小: 0 - 100
	int                  mute;         //音频静音使能
} GxAudioDongleParam;

/**
 * \brief   开启音频 dongle.
 *
 * \param   name   [in] 音频 dongle 名称，需要与注册的音频 dongle 名称一致.
 * \param   param  [in] 音频 dongle 播放参数.
 *
 * \retval  0    成功.
 * \retval  非0 失败.
 **/
int GxAudioDongleEnable    (const char *name, GxAudioDongleParam *param);

/**
 * \brief   关闭音频 dongle.
 *
 * \param   name    [in] 音频 dongle 名称，需要与注册的音频 dongle 名称一致.
 *
 * \retval  0    成功.
 * \retval  非0 失败.
 **/
int GxAudioDongleDisable   (const char *name);

/**
 * \brief   设置音频 dongle 的音量.
 *
 * \param   name   [in] 音频 dongle 名称，需要与注册的音频 dongle 名称一致.
 * \param   volume [in] 音频 dongle 音量，取值范围0~100.
 *
 * \retval  0    成功.
 * \retval  非0 失败.
 **/
int GxAudioDongleSetVolume (const char *name, int volume);

/**
 * \brief   设置音频 dongle 的静音状态.
 *
 * \param   name   [in] 音频 dongle 名称，需要与注册的音频 dongle 名称一致.
 * \param   mute   [in] 音频 dongle 静音开关.
 *
 * \retval  0    成功.
 * \retval  非0 失败.
 **/
int GxAudioDongleSetMute   (const char *name, int mute);

/**
 * \brief   设置音频 dongle 的声道.
 *
 * \param   name     [in] 音频 dongle 名称，需要与注册的音频 dongle 名称一致.
 * \param   channel  [in] 音频 dongle 声道信息.
 *
 * \retval  0    成功.
 * \retval  非0 失败.
 **/
int GxAudioDongleSetChannel(const char *name, GxAudioDongleChannel channel);

#endif
