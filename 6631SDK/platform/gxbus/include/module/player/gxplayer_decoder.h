/** \addtogroup gxplayer_decoder
 **  @{
 **/
#ifndef __GXPLAYER_DECODER_H__
#define __GXPLAYER_DECODER_H__

/**
 * 音视频解码器状态码
 **/
typedef enum {
	AVCODEC_STOPPED = 0x1000000,  ///< 音视频解码器停止状态.
	AVCODEC_RUNNING,              ///< 音视频解码器运行状态,已解码出解码帧的状态.
	AVCODEC_PAUSED,               ///< 音视频解码器暂停状态.
	AVCODEC_FINISHED,             ///< 音视频解码器完成状态,填写的音视频数据完全被消耗.
	AVCODEC_UNSUPPORT,            ///< 音视频解码器编码不支持,该状态不可用.
	AVCODEC_BUSY,                 ///< 音视频解码器被占用状态.
	AVCODEC_ERROR,                ///< 音视频解码器解码错误状态,错误信息需要看错误码.
	AVCODEC_READY,                ///< 音视频解码器待解码状态.
	AVCODEC_STARTED,              ///< 音视频解码器启动状态.
	AVCODEC_TIMEOUT,              ///< 音视频解码器解码超时状态.
} AVCodecState;

/**
 * 音视频解码器错误码
 **/
typedef enum {
	AVCODEC_ERR_NONE,                ///< 音视频解码器正常.
	AVCODEC_ERR_ESBUF_OVERFLOW,      ///< 音视频解码器数据上溢.
	AVCODEC_ERR_CODECTYPE_UNSUPPORT, ///< 音视频解码器不支持编码.
	AVCODEC_ERR_SIZE_UNSUPPORT,      ///< 视频解码器不支持视频的宽或高.
	AVCODEC_ERR_SIZE_CHANGED,        ///< 视频解码器的视频宽或高发生变化.
	AVCODEC_ERR_FB_OVERFLOW,         ///< 视频解码器的帧存上溢.
	AVCODEC_ERR_CODEC_ERROR,         ///< 音视频解码器内部错误, 需要外部对其复位.
} AVCodecErrcode;

/**
 * 音视频解码器状态参数
 **/
typedef struct {
	AVCodecState   state;     ///< 音视频解码器状态码.
	AVCodecErrcode err_code;  ///< 音视频解码器错误码,需要解码器处于错误状态.
} AVCodecStatus;

/**
 * 音视频播放器同步状态码
 **/
typedef enum {
	AVCODEC_SYNC_NORMAL,     ///< 音视频处于同步状态
	AVCODEC_SYNC_REPEAT,     ///< 音视频处于重复播放帧状态
	AVCODEC_SYNC_SKIP,       ///< 音视频处于跳帧状态
	AVCODEC_SYNC_FREERUN,    ///< 音视频无法同步处于自由播放状态.
} AVCodecSyncState;

/**
 * 音视频播放器同步状态
 **/
typedef struct {
	unsigned int     sync_flag;  ///< 音视频是否同步,1:同步,0:不同步
	AVCodecSyncState sync_state; ///< 音视频同步状态
} AVCodecSyncStatus;

/**
 * 视频流的动态范围类型
 **/
typedef enum {
	VIDEO_DRT_SDR        , ///< Standard-Dynamic-Range标准动态范围
	VIDEO_DRT_HLG        , ///< Hybrid-Log-Gamma
	VIDEO_DRT_HDR10      , ///< High-Dynamic-Range-10
	VIDEO_DRT_HDR10_PLUS , ///< High-Dynamic-Range-10+
	VIDEO_DRT_DOLBYVISION, ///< Dolby-Vision
} VideoDynamicRangeType;

/**
 * 视频编码类型
 **/
typedef enum {
	VIDEO_CODEC_MPEG12  = (1 << 0),     ///< MPEG12 编码
	VIDEO_CODEC_MPEG4   = (1 << 1),     ///< MPEG4  编码
	VIDEO_CODEC_H263    = (1 << 2),     ///< H263 编码
	VIDEO_CODEC_H264    = (1 << 3),     ///< H264 编码
	VIDEO_CODEC_REAL    = (1 << 4),     ///< REAL 编码
	VIDEO_CODEC_AVS     = (1 << 5),     ///< AVS 编码
	VIDEO_CODEC_H265    = (1 << 6),     ///< h265 编码
	VIDEO_CODEC_VC1     = (1 << 7),     ///< VC1 编码
	VIDEO_CODEC_MJPEG   = (1 << 8),     ///< MJPEG 编码
	VIDEO_CODEC_DIV3    = (1 << 9),     ///< DIV3 编码
	VIDEO_CODEC_UNKNOWN = (0xFFFFFFFF), ///< 预留编码
} VideoCodecType;

/**
 * 音频编码类型
 **/
typedef enum {
	AUDIO_CODEC_MPEG1     = (1 << 0),     ///< MPEG1 编码, MP3 LayerI.
	AUDIO_CODEC_MPEG2     = (1 << 1),     ///< MPEG2 编码, MP3 LayerII.
	AUDIO_CODEC_AAC_LATM  = (1 << 2),     ///< MPEG4 AAC, LATM 封装.
	AUDIO_CODEC_AAC_ADTS  = (1 << 3),     ///< MPEG4 AAC, ADTS 封装.
	AUDIO_CODEC_VORBIS    = (1 << 4),     ///< VORBIS 编码.
	AUDIO_CODEC_AC3       = (1 << 5),     ///< DOLBY 编码.
	AUDIO_CODEC_AVS       = (1 << 6),     ///< AVS 编码.
	AUDIO_CODEC_PCM       = (1 << 7),     ///< PCM 编码.
	AUDIO_CODEC_EAC3      = (1 << 8),     ///< DOLBY-PLUS 编码.
	AUDIO_CODEC_DTS       = (1 << 9),     ///< DTS 编码.
	AUDIO_CODEC_DRA1      = (1 << 10),    ///< DRA 编码.
	AUDIO_CODEC_DTS_HD    = (1 << 11),    ///< DTS-HD 编码.
	AUDIO_CODEC_REAL      = (1 << 12),    ///< REAL 编码.
	AUDIO_CODEC_RA_AAC    = (1 << 13),    ///< RA-AAC 编码.
	AUDIO_CODEC_RA_RA8LBR = (1 << 14),    ///< RA-RA8LBR 编码.
	AUDIO_CODEC_FLAC      = (1 << 15),    ///< FLAC 编码.
	AUDIO_CODEC_OPUS      = (1 << 16),    ///< OPUS 编码.
	AUDIO_CODEC_AMR_NB    = (1 << 17),    ///< AMR-NB 编码.
	AUDIO_CODEC_AMR_WB    = (1 << 18),    ///< AMR-WB 编码.
	AUDIO_CODEC_APE       = (1 << 19),    ///< APE 编码.
	AUDIO_CODEC_SBC       = (1 << 20),    ///< SBC 编码.
	AUDIO_CODEC_DRA       = (1 << 21),    ///< SBC 编码.
	AUDIO_CODEC_UNKNOWN   = (0xFFFFFFFF), ///< 预留编码.
} AudioCodecType;

/**
 * 字幕编码类型
 **/
typedef enum {
	SUBTITLE_CODEC_DVB       = 0,         ///< DVB      Subtitle 编码.
	SUBTITLE_CODEC_TELETEXT,              ///< TELETEXT Subtitle 编码, 包含 text , magazine 类型的字幕．
	SUBTITLE_CODEC_ARIB_CC,               ///< ARIB-CC  Subtitle 编码.
	SUBTITLE_CODEC_SCTE_CC,               ///< SCTE-CC  Subtitle 编码.
	SUBTITLE_CODEC_ATSC_CC,               ///< ATSC-CC  Subtitle 编码.
	SUBTITLE_CODEC_SRT,                   ///< SRT      Subtitle 编码.
	SUBTITLE_CODEC_SSA,                   ///< SSA      Subtitle 编码.
	SUBTITLE_CODEC_TEXT,                  ///< TEXT     Subtitle 编码.
	SUBTITLE_CODEC_VOB,                   ///< VOB      Subtitle 编码.
	SUBTITLE_CODEC_XSUB,                  ///< XSUB     Subtitle 编码.
} SubtitleCodecType;

/**
 * 外部音频解码器解码 PCM 参数
 **/
typedef struct {
	unsigned int sample_rate;     ///< PCM 音频采样率.
	unsigned int channel_num;     ///< PCM 音频声道数.
	unsigned int bits_per_sample; ///< PCM 音频位宽.
	unsigned int interlace;       ///< PCM 音频是否交织存储,只支持交织储存的音频.
	unsigned int endian;          ///< PCM 音频大小端: 1 - 大端; 其他 - 小端.
	unsigned int float_en;        ///< PCM 音频是否时浮点型: 1 - 浮点型; 其他 - 定点型.
	unsigned int channel_flag;    ///< PCM 音频声道标识，gxdocref 声道阵列.
} GxAudioDecoderPcmInfo;

/**
 * 外部音频解码器解码 CDD 参数
 **/
typedef struct {
	unsigned int bit_rate;     ///< CDD 音频比特率bps.
} GxAudioDecoderCddInfo;

/**
 * 外部音频解码器解码输入/输出 FIFO 参数
 **/
typedef struct {
	unsigned int size;          ///< 可解码的 FIFO 数据大小.
	unsigned int free;          ///< 可填充的 FIFO 数据大小.
	unsigned int cap;           ///< FIFO 容量大小.
} GxAudioDecoderBufInfo;

/**
 * 外部音解码器控制交互类型
 **/
typedef enum {
	GX_AUDIO_DECODER_SET_PCM_INFO,       ///< 外部音解码器设置 PCM 信息,参数详见: gxdocref GxAudioDecoderPcmInfo.
	GX_AUDIO_DECODER_GET_RBUF_INFO,      ///< 外部音解码器获取 READ  FIFO 信息, 参数详见: gxdocref GxAudioDecoderBufInfo.
	GX_AUDIO_DECODER_GET_RBUF_PTS_INFO,  ///< 外部音解码器获取 PTS   FIFO 信息, 参数详见: gxdocref GxAudioDecoderBufInfo.
	GX_AUDIO_DECODER_GET_WBUF_INFO,      ///< 外部音解码器获取 WRITE FIFO 信息, 参数详见: gxdocref GxAudioDecoderBufInfo.
	GX_AUDIO_DECODER_SET_CDD_INFO,       ///< 外部音解码器设置 BITRATE 信息,参数详见: gxdocref GxAudioDecoderCddInfo.
} GxAudioDecoderCommand;

/**
 * 外部音解码器解码状态
 **/
typedef enum {
	GX_AUDIO_DECODER_OK         = 0,   ///< 正常解码.
	GX_AUDIO_DECODER_EOF        = 1,   ///< 结束解码.
	GX_AUDIO_DECODER_ERROR      = 2,   ///< 错误解码.
	GX_AUDIO_DECODER_RBUF_EMPTY = 3,   ///< 解码读 FIFO 数据不足.
	GX_AUDIO_DECODER_WBUF_FULL  = 4,   ///< 解码写 FIFO 数据满.
} GxAudioDecoderStatus;

/**
 * 外部音解码器读取 FIFO 数据参数
 **/
typedef int (*GxAudioDecoderReadCallback)   (void *priv, unsigned char* buf, unsigned int size, int *pts);

/**
 * 外部音解码器探测 FIFO 数据参数
 **/
typedef int (*GxAudioDecoderPeekCallback)   (void *priv, unsigned char* buf, unsigned int size);

/**
 * 外部音解码器跳转 FIFO 数据参数
 **/
typedef int (*GxAudioDecoderSkipCallback)   (void *priv, unsigned int size);

/**
 * 外部音解码器写入 FIFO 数据参数
 **/
typedef int (*GxAudioDecoderWriteCallback)  (void *priv, unsigned char* buf, unsigned int size, int  pts);

/**
 * 外部音解码器交互控制参数
 **/
typedef int (*GxAudioDecoderControlCallback)(void *priv, GxAudioDecoderCommand cmd, void *args);

/**
 * 外部音频解码器解码参数
 **/
typedef struct {
	void                         *priv; ///< 私有信息,外部音频解码器调用回调时,需要回传该参数.
	GxAudioDecoderReadCallback    rc;   ///< 读取 FIFO 数据回调.
	GxAudioDecoderPeekCallback    pc;   ///< 探测 FIFO 数据回调.
	GxAudioDecoderSkipCallback    sc;   ///< 跳转 FIFO 数据回调.
	GxAudioDecoderWriteCallback   wc;   ///< 写入 FIFO 数据回到.
	GxAudioDecoderControlCallback cc;   ///< 交互控制回调.
} GxAudioDecoderParam;

/**
 * 外部音频解码器实例参数
 **/
typedef struct {
	const char           *name;     ///< 外部音频解码器名称.
	const char           *author;   ///< 外部音频解码器作者.
	int                   score;    ///< 外部音频解码器命中得分,分数越高,解码优先被选择: 最低0分,最高100分,硬件解码器默认50分.
	AudioCodecType       codecType; ///< 数据编码类型: 兼容流式数据,即每帧数据增加各种编码头数据,解码器解码判断的依据.
	void*                (*create) (GxAudioDecoderParam *param); ///< 创建一个音频解码器.
	void                 (*destory)(void *handle);               ///< 销毁一个音频解码器.
	GxAudioDecoderStatus (*decode) (void *handle);               ///< 启动音频解码器解码一帧音频: 通过回调探测\读取\跳转\写入 FIFO 数据,以及交互解码信息.
	void                 (*update) (void *handle);               ///< 完成所有数据写入,外部音频解码完成所有数据解码,可以返回解码结束状态.
} GxAudioDecoder;

/**
 * \brief   注册外部音频解码器.
 *
 * \param   dec [in]  外部音频解码器实例参数.
 * \retval   0        成功.
 * \retval  -1        失败.
 **/
int  GxAudioRegisterDecoder(GxAudioDecoder *dec);

/**
 * 视频解码器事件类型
 **/
typedef enum
{
	GX_VIDEO_EVENT_DECODE_ONE_FRAME = (1 << 0),  ///< 视频解码一帧事件.
} GxVideoDecoderEvent;

/**
 * 视频解码器事件回调参数
 * id 标识第几路解码器的事件
 **/
typedef void (*GxVideoDeocderEventCallback)(int id, GxVideoDecoderEvent event);

/**
 * \brief   注册视频解码器事件回调
 *
 * \param   cbk [in]  视频解码器事件回调函数.
 * \retval   0        成功.
 * \retval  -1        失败.
 **/
int  GxVideoDecoder_RegisterEventCallback(GxVideoDeocderEventCallback cbk);


#endif
/** @}*/
