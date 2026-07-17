/** \addtogroup  gxplayer_setting
 *  @{
 */
#ifndef __GXPLAYER_SETTING_H__
#define __GXPLAYER_SETTING_H__

__BEGIN_DECLS

/**
 * 音频输出端口
 **/
typedef enum {
	AUDIO_OUTPUT_DAC_OUT    = (0x1) ,  ///< 外置 DAC 端口,例如: 外置喇叭.
	AUDIO_OUTPUT_DAC_IN     = (0x4) ,  ///< 内置 DAC 端口,例如: CVBS.
	AUDIO_OUTPUT_SPDIF      = (0x8) ,  ///< SPDIF 端口.
	AUDIO_OUTPUT_HDMI       = (0x10) ,  ///< HDMI  端口.
} AudioOutputInterface;

/**
 * 音频输出数据模式
 **/
typedef enum {
	AUDIO_OUTPUT_PCM,   ///< 输出 PCM 数据.
	AUDIO_OUTPUT_AC3,   ///< 输出 AC3 数据.
	AUDIO_OUTPUT_EAC3,  ///< 输出 EAC3 数据.
	AUDIO_OUTPUT_SMART, ///< 智能模式输出,优先输出压缩数据,例如: AC3 / EAC3 / DTS.
	AUDIO_OUTPUT_SAFE,  ///< 安全模式输出,优先输出 PCM 数据.
	AUDIO_OUTPUT_RAW,   ///< 原始模式输出,HDMI EDID 未识别时优先输出压缩数据.
	AUDIO_OUTPUT_DTS,   ///< 输出 DTS 数据.
	AUDIO_OUTPUT_AAC,   ///< 输出 AAC 数据.
	AUDIO_OUTPUT_NOTANY,///< 不输出任何数据.
} AudioOutputDataType;

/**
 * 音频倍数播放模式
 **/
typedef enum {
	AUDIO_OUTPUT_NORMAL,
	AUDIO_OUTPUT_FAST,
} AudioOutputSpeedMode;

/**
 * 音频输出端口输出配置参数.
 **/
typedef struct {
	AudioOutputDataType     output_data;  ///< 音频输出数据.
	AudioOutputInterface    output_port;  ///< 音频输出端口.
}AudioOutputConfig;

/**
 * 音频未播放状态,输出端口输出参数.
 **/
typedef struct {
	AudioOutputInterface    output_port; ///< 音频输出端口.
	union {
		struct {
			unsigned int samplefre;     ///< 音频输出采样率.
			unsigned int channels;      ///< 音频输出通道数.
		} hdmi;                         ///< HDMI 音频输出端口.
	};
}AudioOutPortSet;

/**
 * 音频输出端口开关参数.
 **/
typedef struct {
	AudioOutputInterface    output_port; ///< 音频输出端口.
	unsigned int            onoff;       ///< 使能开关.
}AudioOutPortTurn;

/**
 * 智能音量控制配置.
 **/
typedef struct {
	unsigned int            target_timems;   ///< 目标控制时间.
	float                   target_loudness; ///< 目标响度，范围: 0 - 9.
} AudioSmartConfig;

/**
 * 音频声道参数
 **/
typedef enum
{
	AUDIO_TRACK_MONO        ,     ///< 单声道输出.
	AUDIO_TRACK_LEFT        ,     ///< 左右声道都输出左声道数据.
	AUDIO_TRACK_RIGHT       ,     ///< 左右声道都输出右声道数据.
	AUDIO_TRACK_STEREO      ,     ///< 立体声输出.
	AUDIO_TRACK_RIGHT_MUTE_LEFT,  ///< 左声道静音,右声道输出右声道数据.
	AUDIO_TRACK_LEFT_MUTE_RIGHT,  ///< 右声道静音,左声道输出左声道数据.
	AUDIO_TRACK_MONO_MUTE_LEFT,   ///< 左声道静音,右声道输出单声道数据.
	AUDIO_TRACK_MONO_MUTE_RIGHT,  ///< 右声道静音,左声道输出单声道数据.
	AUDIO_TRACK_SPDIF_LR    ,     ///< 未知定义,无效.
	AUDIO_TRACK_SPDIF_LsRs  ,     ///< 左声道输出左环绕声道数据,右声道输出右环绕声道数据.
	AUDIO_TRACK_SPDIF_CS    ,     ///< 未知定义,无效.
	AUDIO_TRACK_SPDIF_X1X2  ,     ///< 未知定义,无效.
}AudioTrack;

/**
 * 主/次路音频混合模式
 **/
typedef enum
{
	AUDIO_PRIMARY_TRACK    ,  ///< 主路音频输出.
	AUDIO_DESCRIPTOR_TRACK ,  ///< 次路音频输出.
	AUDIO_FULL_MIX_TRACK      ///< 主/次路音频混合输出.
}AudioDecriptorTrack;

/**
 * DOLBY / DOLBY-PLUS 音频工作模式
 **/
typedef enum
{
	AUDIO_AC3_DECODE_MODE = (1<<0),                                      ///< DOBLY / DOLBY-PLUS 解码.
	AUDIO_AC3_BYPASS_MODE = (1<<1),                                      ///< DOLBY / DOLBY-PLUS 透传.
	AUDIO_AC3_FULL_MODE   = AUDIO_AC3_DECODE_MODE|AUDIO_AC3_BYPASS_MODE, ///< DOLBY / DOLBY-PLUS 解码/透传.
}AudioAc3Mode;

/**
 * MPEG4-AAC 音频工作模式
 **/
typedef enum
{
	AUDIO_AAC_DECODE_MODE  = (1<<0),  ///< MPEG4-AAC 解码.
	AUDIO_AAC_BYPASS_MODE  = (1<<1),  ///< MPEG4-AAC 透传.
	AUDIO_AAC_CONVERT_MODE = (1<<2),  ///< MPEG4-AAC 转码 DOLBY.
}AudioAACMode;

/**
 * 视频输出端口
 */
typedef enum
{
	GX_VIDEO_OUTPUT_RCA       = (1 << 0) , ///< 视频 RCA    输出端口.
	GX_VIDEO_OUTPUT_RCA1      = (1 << 1) , ///< 视频 RCA1   输出端口.
	GX_VIDEO_OUTPUT_YUV       = (1 << 2) , ///< 视频 YUV    输出端口.
	GX_VIDEO_OUTPUT_SCART     = (1 << 3) , ///< 视频 SCART  输出端口.
	GX_VIDEO_OUTPUT_SVIDEO    = (1 << 4) , ///< 视频 SVIDEO 输出端口.
	GX_VIDEO_OUTPUT_HDMI      = (1 << 5) , ///< 视频 HDMI   输出端口.
} VideoOutputInterface;

/**
 * 视频输出制式
 */
typedef enum
{
	GX_VIDEO_OUTPUT_PAL         = 1 ,  ///< 视频输出 PAL 制式,有效输出端口: RCA / RCA1/ SCART / SVIDEO.
	GX_VIDEO_OUTPUT_PAL_M       = 2 ,  ///< 视频输出 PAL-M 制式,有效输出端口: RCA / RCA1/ SCART / SVIDEO.
	GX_VIDEO_OUTPUT_PAL_N       = 3 ,  ///< 视频输出 PAL-N 制式,有效输出端口: RCA / RCA1/ SCART / SVIDEO.
	GX_VIDEO_OUTPUT_PAL_NC      = 4 ,  ///< 视频输出 PAL-NC 制式,有效输出端口: RCA / RCA1/ SCART / SVIDEO.
	GX_VIDEO_OUTPUT_NTSC_M      = 5 ,  ///< 视频输出 NTSC-M 制式,有效输出端口: RCA / RCA1/ SCART / SVIDEO.
	GX_VIDEO_OUTPUT_NTSC_443    = 6 ,  ///< 视频输出 NTSC-433 制式,有效输出端口: RCA / RCA1/ SCART / SVIDEO.
	GX_VIDEO_OUTPUT_480I        ,      ///< 视频输出 480I 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_480P        ,      ///< 视频输出 480P 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_576I        ,      ///< 视频输出 576I 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_576P        ,      ///< 视频输出 576P 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_720P_50HZ   ,      ///< 视频输出 720P-50HZ 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_720P_60HZ   ,      ///< 视频输出 720P-60HZ 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_1080I_50HZ  ,      ///< 视频输出 1080I-50HZ 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_1080P_50HZ  ,      ///< 视频输出 1080P-50HZ 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_1080I_60HZ  ,      ///< 视频输出 1080I-60HZ 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_1080P_60HZ  ,      ///< 视频输出 1080P-60HZ 制式,有效输出端口: HDMI.
	GX_VIDEO_OUTPUT_MODE_MAX           ///< 视频输出最大值.
} VideoOutputMode;

/**
 * 视频输出端口输出配置参数
 **/
typedef struct {
	VideoOutputInterface    interface; ///< 视频输出端口.
	VideoOutputMode         mode;      ///< 视频输出制式.
} VideoOutputConfig;

/**
 * SCART 源模式选择
 **/
typedef enum {
	GX_SCART_CVBS,          ///< SCART 端口选择 CVBS 源.
	GX_SCART_RGB ,          ///< SCART 端口选择 RGB  源.
} VideoOutputScartMode;

/**
 * 视频解码 userdata 配置参数
 */
typedef struct {
	int cc_enable;   ///< 支持 ATSC-CC 数据输出.
	int cc_display;  ///< ATSC-CC 是否硬件显示.
	int hdr_enable;  ///< 支持 HDR
} VideoUserdataConfig;

/**
 * 视频输出色彩配置参数
 */
typedef struct {
	VideoOutputInterface    interface;   ///< 视频输出端口.
	int                     brightness;  ///< 视频输出亮度.
	int                     saturation;  ///< 视频输出饱和度.
	int                     contrast;    ///< 视频输出对比度.
	int                     sharpness;   ///< 视频输出锐度.
	int                     hue;         ///< 视频输出色度.
} VideoColor;

/**
 * 视频输出彩条控制参数
 */
typedef struct {
	VideoOutputInterface    interface;  ///< 视频输出端口.
	int                     enable;     ///< 彩条开关使能.
} VideoOutDefault;

/**
 * 视频输出端口开关参数
 **/
typedef struct {
	VideoOutputInterface    interface;  ///< 视频输出端口.
	int                     enable;     ///< 彩条开关使能.
} VideoOutPower;

/**
 * 视频显示视角比
 **/
typedef enum {
	ASPECT_NORMAL       = 0 ,    ///< 自适应的视角比.
	ASPECT_PAN_SCAN         ,    ///< PAN-SCAN 的视角比.
	ASPECT_LETTER_BOX       ,    ///< LETTER-BOX 的视角比.
	ASPECT_COMBINED         ,    ///< PAN-SCAN 和 LETTEX-BOX 自适应的视角比.
	ASPECT_RAW_SIZE         ,    ///< 视频流原始大小视角比.
	ASPECT_RAW_RATIO        ,    ///< 视频流原始比率视角比.
	ASPECT_4X3_PULL         ,    ///< 视频 4:3 拉伸视角比.
	ASPECT_4X3_CUT          ,    ///< 视频 4:3 缩小视角比.
	ASPECT_16X9_PULL        ,    ///< 视频 16:9 拉伸视角比.
	ASPECT_16X9_CUT         ,    ///< 视频 16:9 缩小视角比.
	ASPECT_4X3              ,    ///< 视频 4:3 拉伸/缩小视角比.
	ASPECT_16X9             ,    ///< 视频 16:9 拉伸/缩小视角比.
}VideoAspect;

/**
 * 视频显示设备参数
 **/
typedef struct {
	int aspect;                 ///< 视频显示设备视角比 DISPLAY_SCREEN_4X3 4:3 视角比;DISPLAY_SCREEN_16X9 16:9 视角比
#define DISPLAY_SCREEN_4X3   0  ///< 4:3 视角比.
#define DISPLAY_SCREEN_16X9  1  ///< 16:9 视角比.
	int xres;                   ///< 视频显示的 x 轴.
	int yres;                   ///< 视频显示的 y 轴.
}DisplayScreen;


/**
 * 视频输出制式自适应参数
 **/
typedef struct {
	int enable;             ///< 自适应开关: 1 - 开启；0 - 关闭.
	VideoOutputMode pal;    ///< 视频 PAL 制式输出.
	VideoOutputMode ntsc;   ///< 视频 NTSC 制式输出.
}PlayerVideoAutoAdapt;

/**
 * 视频同步后的帧播放模式
 **/
typedef enum {
	VIDEO_SYNCMODE_FREERUN, ///< 视频帧直接输出,与同步状态无关.
	VIDEO_SYNCMODE_NORMAL,  ///< 视频帧等同步完成后再输出.
	VIDEO_SYNCMODE_SLOWLY,  ///< 视频帧缓慢同步输出.
	VIDEO_SYNCMODE_LATER,   ///< 视频帧先播放,等同步完成再输出.
} VideoSyncMode;

/**
 * 视频显示窗口参数
 **/
typedef struct {
	unsigned int        x;      ///< 视频显示的 x 轴.
	unsigned int        y;      ///< 视频显示的 y 轴.
	unsigned int        width;  ///< 视频显示的宽度.
	unsigned int        height; ///< 视频显示的高度.
} PlayerWindow;

/**
 * 视频播放选择帧率模式
 **/
typedef enum {
	SELECT_DECODER,   ///< 播放选择解码的帧率.
	SELECT_FPS    ,   ///< 播放选择 PTS 探测的帧率.
} VideoFpsSource;

/**
 * 音频解码器解 KEY 参数
 **/
typedef struct {
	int timeout_ms;                  ///< 解 KEY 超时时间.
	unsigned char map[64];           ///< 解 KEY 的 map.
	unsigned char map_key[16];       ///< 解 KEY 的 map key.
	unsigned char rand_data[16];     ///< 解 KEY 的 rand data.
	unsigned char chip_id[4];        ///< 解 KEY 的 CHIP ID.
	unsigned char scr_key[16];       ///< 解 KEY 的 加密密文数据.
	unsigned char dec_key[16];       ///< 解 KEY 的 解密明文数据.
} EncryptKey;

/**
 * ".ets" 后缀的录制流的加解密钥参数
 **/
typedef struct {
	int enable;                           ///< 是否开启加解密: 1 - 开启；0 - 关闭.
	int len;                              ///< 加解密钥的长度.
	char key[PLAYER_MAX_ENCRYPT_KEY_LEN]; ///< 加解密钥.
} PlayerEtsEncrypt;

/**
 * 播放器使用内存参数
 **/
typedef struct {
	unsigned int sysmem;   ///< 使用系统内存大小,单位: Byte.
	unsigned int holemem;  ///< 使用洞内存大小,单位: Byte.
} PlayerHeapInfo;

/**
 * 播放器事件类型
 **/
typedef enum
{
	PLAYER_EVENT_SPEED_REPORT         ,  ///< 播放器快进快退速率切换事件.
	PLAYER_EVENT_STATUS_REPORT        ,  ///< 播放器状态切换事件.
	PLAYER_EVENT_AVCODEC_REPORT       ,  ///< 播放器解码器切换事件.
	PLAYER_EVENT_RESOLUTION_REPORT    ,  ///< 播放器制式切换事件.
	PLAYER_EVENT_RECORD_OVERFLOW      ,  ///< 播放器录制缓冲溢出事件.
	PLAYER_EVENT_DYNAMIC_RANGE_CHANGED,  ///< 播放器视频动态范围变化事件.
} PlayerEventID;

/**
 * 视频去隔行功能参数
 **/
typedef enum {
	VD_FLAG_DEINTERLACE_OFF = 0,    ///< 关闭视频去隔行功能
	VD_FLAG_DEINTERLACE_Y   = 1<<1, ///< 开启视频亮度数据去隔行功能
	VD_FLAG_DEINTERLACE_UV  = 1<<2, ///< 开启视频色度数据去隔行功能
} VideoDeinterlaceFlag;

/**
 * 视频去隔行功能参数
 **/
typedef struct {
	VideoDeinterlaceFlag flag; ///< 去隔行功能开关
	unsigned int max_width;    ///< 去隔行功能的视频流宽度上限(资源允许情况下，对同时满足max_width和max_height限制之下的流做去隔行处理）
	unsigned int max_height;   ///< 去隔行功能的视频流高度上限(资源允许情况下，对同时满足max_width和max_height限制之下的流做去隔行处理）
} VideoDeinterlaceParam;

/**
 * 视频动态范围转换器工作模式
 **/
typedef enum {
	GX_VDRC_AUTO  , ///< 自适应最佳视频输出效果
	GX_VDRC_BYPASS, ///< 关闭动态范围转换, hdr/sdr都直接输出给电视机，输出效果由电视机决定
	GX_VDRC_TOSDR , ///< 强制把视频流转成sdr输出
} VideoDrcMode;

/**
 * 网络流多带宽的像素设置
 **/
typedef struct {
	int width;  //设置网络流多带宽的水平像素
	int height; //设置网络流多带宽的垂直像素
}NetworkResolution;

/**
 * 播放器事件回调参数
 **/
typedef void (*PLAYER_EVENT_REPORT)(PlayerEventID event,void* args);

/**
 * 客户自定义SSL加密套件
 **/
typedef int (*CUSTOM_SSL_CHPHER_SUITE_CBK)(char** args);

/**
 * 视频静帧静帧回调参数
 **/
typedef int  (*PLAYER_FREEZE_FRAME)(void);

/**
 * 视频静帧取消回调参数
 **/
typedef int  (*PLAYER_UNFREEZE_FRAME)(void);

/**
 * 跳转回调参数
 **/
typedef int  (*PLAYER_SEEK_CBK)(int timems);

/**
 * \brief   播放器模块初始化.
 *
 * \retval void.
 **/
extern void GxPlayer_ModuleInit(void);

/**
 * \brief   播放器模块去初始化,释放资源.
 *
 * \retval void.
 **/
extern void GxPlayer_ModuleDestroy(void);

/**
 * \brief   设置播放器事件回调函数.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   report [in]  事件回调参数,详见: gxdocref PLAYER_EVENT_REPORT.
 * \retval  void.
 **/
extern void GxPlayer_SetEventCallback(PLAYER_EVENT_REPORT report);

/**
 * \brief   设置视频静帧/取消静帧回调函数.
 * \details 1. 播放器初始化之后调用有效,接口废弃.
 *
 * \param   freeze   [in]  静帧回调参数,详见: gxdocref PLAYER_FREEZE_FRAME.
 * \param   unfreeze [in]  取消静帧回调参数,详见: gxdocref PLAYER_UNFREEZE_FRAME.
 * \retval  void.
 **/
player_attribute_deprecated
extern void GxPlayer_SetFreezeCallback(PLAYER_FREEZE_FRAME freeze,PLAYER_UNFREEZE_FRAME unfreeze);

/**
 * \brief   设置跳转回调函数.
 * \details 1. 播放器初始化之后调用有效,接口废弃.
 *
 * \param   cbk   [in]  静帧回调参数,详见: gxdocref PLAYER_SEEK_CBK.
 * \retval  void.
 **/
player_attribute_deprecated
extern void GxPlayer_SetSeekCallback(PLAYER_SEEK_CBK cbk);

/**
 * \brief   设置音频输出端口.
 * \details 1. 播放器初始化之后调用有效, GxPlayer_SetAudioOutputConfig 时生效.
 *
 * \param   interface [in]  音频输出端口,详见: gxdocref AudioOutputInterface.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_SetAudioOutputSelect(int interface);

/**
 * \brief   设置音频输出端口输出数据类型.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   config             [in]  音频输出端口配置,详见: gxdocref AudioOutputConfig.
 * \param   config.output_port [in]  音频输出端口类型,详见: gxdocref AudioOutputInterface.
 * \param   config.output_data [in]  音频输出数据类型,详见: gxdocref AudioOutputDataType.
 * \retval  GX_PLAYER_OK             成功.
 * \retval  GX_PLAYER_ERROR          失败.
 **/
extern status_t GxPlayer_SetAudioOutputConfig(AudioOutputConfig config);

/**
 * \brief   设置音频输出音量.
 * \details 1. 播放器初始化之后调用有效,播放音频音量为最后一次设置值.
 *
 * \param   vol             [in]  音频输出音量值,取值范围: 0 ~ 100.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioVolume(unsigned int vol);

/**
 * \brief   获取音频输出音量.
 * \details 1. 播放器初始化之后调用有效,获取音频音量为最后一次设置值.
 *
 * \param   vol             [out] 音频输出音量值,取值范围: 0 ~ 100.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_GetAudioVolume(unsigned int *vol);

/**
 * \brief   设置主路音频音量.
 * \details 1. 播放器初始化之后调用有效,设置主路音频音量为最后一次设置值.
 *
 * \param   volume          [in]  主路音频音量值,取值范围: 0 ~ 32.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioBoostVolume(unsigned int volume);

/**
 * \brief   获取是否支持次路音频解码.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   vaild           [out] 是否支持次路音频解码: 0 - 不支持；1 - 支持.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_GetAudioDescriptorVaild(int *vaild);

/**
 * \brief   设置次路音频音量.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   volume          [in]  次路音频音量,取值范围: 0 ~ 32.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioDescriptorBoostVolume(unsigned int volume);

/**
 * \brief   设置音频DAC音量.
 * \details 1. 重新播放节目后调用生效.
 *
 * \param   voldb           [in]  音频DAC音量,取值范围: -36 ~ 6.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioDACVolume(int voldb);


/**
 * \brief   设置主/次路音频输出方式.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   track           [in]  主/次路音频输出方式详见: gxdocref AudioDecriptorTrack.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioDescriptorTrack(AudioDecriptorTrack tarck);

/**
 * \brief   设置音频输出声道模式.
 * \details 1. 播放器初始化之后调用有效,音频播放为最后一次设置的声道模式.
 *
 * \param   track           [in]  声道模式,详见: gxdocref AudioTrack.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioTrack(AudioTrack track);

/**
 * \brief   设置音频输出是否静音.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 开启静音对所有音频输出端口生效.
 *
 * \param   enable          [in]  静音使能: 1 - 开启静音；0 - 关闭静音.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioMute(int enable);

/**
 * \brief   获取音频输出静音状态.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable          [in]  静音状态: 1 - 静音；0 - 非静音.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_GetAudioMute(int *enable);


/**
 * \brief   设置音频输出端口开关,防爆音功能.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 使用场景: 待机功能,防爆音.
 *
 * \param   enable          [in]  防爆音使能: 1 - 关闭音频端口输出；0 - 开启音频端口输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioPowerMute(int enable);

/**
 * \brief   设置音频处于未播放状态时,音频输出端口输出数据.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 只对 HDMI 有效,其他输出端口无效.
 *
 * \param   port            [in]  输出端口设置,详见: gxdocref AudioOutPortSet.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioPort(AudioOutPortSet port);

/**
 * \brief   设置音频输出端口开关.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 只对 HDMI / CVBS 有效,SPDIF 等其他输出端口无效.
 *
 * \param   port            [in]  输出端口开关设置,详见: gxdocref AudioOutPortTurn.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_TurnAudioPort(AudioOutPortTurn port);

/**
 * \brief   设置音频解码 ESA 数据满门限.
 * \details 1. 播放器初始化之后调用有效, DVB 播放有效.
 *
 * \param   port            [in]  门限值,单位 Byte.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioDmxFullGate(int full_gate);

/**
 * \brief   设置音频 DOLBY / DOLBY-PLUS 编码标准的解码模式.
 * \details 1. 播放器初始化之后调用有效, DOLBY / DOLBY-PLUS 音频编码数据有效.
 *
 * \param   mode            [in]  解码模式,详见: gxdocref AudioAc3Mode.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioAC3Mode(AudioAc3Mode mode);

/**
 * \brief   设置音频 MPEG4-AAC 编码标准的解码模式.
 * \details 1. 播放器初始化之后调用有效, MPEG4-AAC 音频编码数据有效.
 *
 * \param   mode            [in]  解码模式,详见: gxdocref AudioAACMode.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioAACMode(AudioAACMode mode);

/**
 * \brief   设置音频解码 Mask 的音频编码标准.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   mask            [in]  音频编码标准,详见: gxdocref AudioCodecType.
 * \retval  void.
 **/
extern void GxPlayer_AudioCodecMask(AudioCodecType mask);

/**
 * \brief   设置音频输出同步使能.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable          [in]  同步使能: 1 - 同步输出；0 - 不同步输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioSyncEnable(int enable);

/**
 * \brief   设置音频输出同步低门限.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   timems          [in]  同步低门限,低于此门限音频同步输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioSyncTime(unsigned int timems);

/**
 * \brief   设置音频输出同步高门限.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   timems          [in]  同步高门限,低于此门限音频不同步输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioSyncHighTime(unsigned int timems);

/**
 * \brief   设置音频解码 Downmix 开关.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable          [in]  开关使能: 1 - 开启音频解码 Downmix；0 - 关闭音频解码 Downmix.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioDownMix(int enable);

/**
 * \brief   设置音频 PTS 恢复系统时钟 STC 开关.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable          [in]  开关使能: 1 - 开启音频 PTS 恢复 STC；0 - 关闭音频 PTS 恢复 STC.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetAudioPureRecoveryMode(int enable);

/**
 * \brief   设置视频同步时的视频帧播放模式.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   mode            [in]  视频帧播放模式,详见: gxdocref VideoSyncMode.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoSyncMode(VideoSyncMode mode);

/**
 * \brief   设置视频同步低门限.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   timems          [in]  视频同步低门限,低于此门限视频同步输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoSyncTime(unsigned int timems);

/**
 * \brief   设置视频同步高门限.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   timems          [in]  视频同步高门限,高于此门限视频不同步输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoSyncHighTime(unsigned int timems);

/**
 * \brief   设置视频输出端口的色彩信息.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 色彩信息包括: 亮度,饱和度,对比度,锐度,色度.
 *
 * \param   color           [in]  视频色彩信息,详见: gxdocref VideoColor.
 * \param   color.interface [in]  视频输出端口,详见: gxdocref VideoOutputInterface.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoColors(VideoColor color);

/**
 * \brief   获取视频输出端口的色彩信息.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 色彩信息包括: 亮度,饱和度,对比度,锐度,色度.
 *
 * \param   color           [in,out] 视频色彩信息,详见: gxdocref VideoColor.
 * \param   color.interface [in]     视频输出端口,详见: gxdocref VideoOutputInterface.
 * \retval  GX_PLAYER_OK             成功.
 * \retval  GX_PLAYER_ERROR          失败.
 **/
extern status_t GxPlayer_GetVideoColors(VideoColor *color);

/**
 * \brief   获取视频输出端口.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   iface           [out] 视频输出端口,详见: gxdocref VideoOutputInterface.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_GetVideoUsableIface(int *iface);

/**
 * \brief   设置视频显示视角比.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   aspect         [in]  视频显示视角比,详见: gxdocref VideoAspect.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoAspect(VideoAspect aspect);

/**
 * \brief   设置视频动态范围转换器工作模式.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   mode         [in]  视频动态范围转换器工作模式,详见: gxdocref VideoDrcMode.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoDrcMode(VideoDrcMode mode);

/**
 * \brief   设置视频输出端口.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   select          [in]  视频输出端口,详见: gxdocref VideoOutputInterface.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoOutputSelect(int select);

/**
 * \brief   设置视频输出端口的输出配置.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   config           [in]  视频输出端口配置,详见: gxdocref VideoOutputConfig.
 * \param   config.interface [in]  视频输出端口,详见: gxdocref VideoOutputInterface.
 * \param   config.mode      [in]  视频输出配置,详见: gxdocref VideoOutputMode.
 * \retval  GX_PLAYER_OK           成功.
 * \retval  GX_PLAYER_ERROR        失败.
 **/
extern status_t GxPlayer_SetVideoOutputConfig(VideoOutputConfig config);

/**
 * \brief   获取视频输出端口的输出配置.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   config           [in,out] 视频输出端口配置,详见: gxdocref VideoOutputConfig.
 * \param   config.interface [in]     视频输出端口,详见: gxdocref VideoOutputInterface.
 * \param   config.mode      [out]    视频输出配置,详见: gxdocref VideoOutputMode.
 * \retval  GX_PLAYER_OK              成功.
 * \retval  GX_PLAYER_ERROR           失败.
 **/
extern status_t GxPlayer_GetVideoOutputConfig(VideoOutputConfig *config);

/**
 * \brief   设置视频输出端口输出彩条开关.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   outdef           [in]  视频输出端口书彩条开关,详见: gxdocref VideoOutDefault.
 * \param   outdef.interface [in]  视频输出端口,详见: gxdocref VideoOutputInterface.
 * \param   outdef.enable    [in]  输出彩条使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK           成功.
 * \retval  GX_PLAYER_ERROR        失败.
 **/
extern status_t GxPlayer_SetVideoOutputDef(VideoOutDefault outdef);

/**
 * \brief   设置所有视频输出端口开关.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 使用场景: 待机功能.
 *
 * \param   enable          [in]  使能开关: 0 - 关闭视频输出端口；1 - 开启视频输出端口.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoOutputPower(int enable);

/**
 * \brief   设置单路视频输出端口开关.
 * \details 1. 播放器初始化之后调用有效.
 * \details 2. 使用场景: 待机功能.
 *
 * \param   power           [in]  视频输出端口参数,详见: gxdocref VideoOutPower.
 * \details 3. power.interface [in]  视频输出端口,详见: gxdocref VideoOutputInterface.
 * \details 4. power.enable    [in]  使能开关: 0 - 关闭视频输出端口；1 - 开启视频输出端口.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoOutputPower2(VideoOutPower power);

/**
 * \brief   设置视频只解码不输出.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   bypass  [in] 开关使能: 1 - 只解码不输出；0 - 解码后输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoPlayBypass(int bypass);

/**
 * \brief   设置视频输出端口 SCART 的输入源模式.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   mode            [in]  视频 SCART 输入源模式,详见: gxdocref VideoOutputScartMode.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetScartMode(VideoOutputScartMode mode);

/**
 * \brief   设置视频输出端口 CVBS 的输出窗口.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   window          [in]  视频输出端口 CVBS 输出窗口,详见: gxdocref PlayerWindow.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetCvbsViewport(PlayerWindow window);

/**
 * \brief   设置视频显示设备信息.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   scr             [in]  视频显示设备信息,详见: gxdocref DisplayScreen.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetDisplayScreen(DisplayScreen scr);

/**
 * \brief   设置视频马赛克是否不输出.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   number          [in]  马赛克输出使能: 0 - 马赛克输出; 1 - 马赛克不输出.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoMosaicDrop(int number);

/**
 * \brief   设置视频马赛克门限百分比,超过该门限被认为马赛克.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   percent         [in]  马赛克门限: 0 ~ 100.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoMosaicGate(int percent);

/**
 * \brief   设置视频解码 ATSC-CC 配置.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   config          [in]  配置参数,详见: gxdocref VideoUserdataConfig.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoUserdataConfig(VideoUserdataConfig *config);

/**
 * \brief   设置视频静帧切台开关.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable          [in]  使能开关: 1 - 静帧切台；0 - 黑屏切台.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetFreezeFrameSwitch(int enable);

/**
 * \brief   设置视频输出制式自适应配置.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   config          [in]  配置参数,详见: gxdocref PlayerVideoAutoAdapt.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_SetVideoAutoAdapt(PlayerVideoAutoAdapt* config);

/**
 * \brief   设置视频标准多分配一个帧存的配置
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   mask             [in]  标识针对哪些编码需要多分配一帧，默认值 0xFFFFFFFF, 表示所有编码都要多申请一帧
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern void GxPlayer_SetVideoExtraFrameBuffer(VideoCodecType mask);

/**
 * \brief   设置视频去隔行开关.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable [in]  使能开关: 1 - 开启去隔行; 0 - 关闭去隔行.
 * \retval  void.
 **/
extern void GxPlayer_SetVideoDeinterlace(int enable);

/**
 * \brief   设置视频去隔行参数.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   param [in]  去隔行功能控制参数，详见gxdocref VideoDeinterlaceParam.
 * \retval  void.
 **/
extern void GxPlayer_SetVideoDeinterlaceParam(VideoDeinterlaceParam *param);

/**
 * \brief   设置视频播放选择帧率模式.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   src [in]  视频播放选择帧率模式,详见: gxdocref VideoFpsSource.
 * \retval  void.
 **/
extern void GxPlayer_SetVideoFpsSource(VideoFpsSource src);

/**
 * \brief   设置视频 Mask 的视频编码标准.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   src [in]  视频编码标准,详见: gxdocref VideoCodecType.
 * \retval  void.
 **/
extern void GxPlayer_VideoCodecMask(VideoCodecType mask);

/**
 * \brief   设置视频 PP 缩放开关.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable [in] 开关使能: 1 - 开启 PP 缩放; 0 - 关闭 PP 缩放.
 * \retval  void.
 **/
extern void GxPlayer_SetVideoPPZoom(int enable);

/**
 * \brief   解密一帧 KEY.
 * \details 1. 音频解码器启动后有效.
 *
 * \param   key             [in,out]  解 KEY 配置参数,详见: gxdocref EncryptKey.
 * \param   key.map         [in]      解 KEY 的 map.
 * \param   key.map_key     [in]      解 KEY 的 map key.
 * \param   key.rand_data   [in]      解 KEY 的 rand data.
 * \param   key.chip_id     [in]      解 KEY 的 CHIP ID.
 * \param   key.scr_key     [in]      解 KEY 的 加密密文数据.
 * \param   key.dec_key     [out]     解 KEY 的 解密明文数据.
 * \retval  GX_PLAYER_OK              成功.
 * \retval  GX_PLAYER_ERROR           失败.
 **/
extern status_t GxPlayer_GetEncrypt(EncryptKey *key);

/**
 * \brief   设置 ".ets" 后缀录制加解密配置.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enc             [in] 加解密配置参数,详见: gxdocref PlayerEtsEncrypt.
 * \retval  GX_PLAYER_OK         成功.
 * \retval  GX_PLAYER_ERROR      失败.
 **/
extern status_t GxPlayer_SetEtsEncrypt(PlayerEtsEncrypt  enc);

/**
 * \brief   获取 ".ets" 后缀录制加解密配置.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enc             [out] 加解密配置参数,详见: gxdocref PlayerEtsEncrypt.
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern status_t GxPlayer_GetEtsEncrypt(PlayerEtsEncrypt *enc);

/**
 * \brief   设置播放器锁频
 * \details 1. 接口废弃.
 *
 * \param   url [in] URL 参数.
 * \retval  void.
 **/
player_attribute_deprecated
extern void GxPlayer_SetTP(const char* url);

/**
 * \brief   设置播放器使用内存.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   info [in] 内存信息,详见: gxdocref PlayerHeapInfo.
 * \retval   0        成功.
 * \retval  -1        失败.
 **/
extern int  GxPlayer_HeapModify(PlayerHeapInfo *info);

/**
 * \brief   注册播放器使用用户的洞内存.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   start [in] 用户的洞内存地址.
 * \param   size  [in] 用户的洞内存大小.
 * \retval   0         成功.
 * \retval  -1         失败.
 **/
extern int  GxPlayer_HeapRegister(unsigned int start, unsigned int size);

/**
 * \brief   卸载播放器使用用户的洞内存.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   start [in] 用户的洞内存地址.
 * \retval   0         成功.
 * \retval  -1         失败.
 **/
extern int  GxPlayer_HeapUnregister(unsigned int start);

/**
 * \brief   开启DOLBY CERTIFY模式.
 * \details 1. 播放器初始化之后调用有效,并且音频停播状态.
 *
 * \param   enable [in] 开启dolby certify.
 * \retval   0          成功.
 * \retval  -1          失败.
 **/
extern status_t GxPlayer_SetAudioDolbyCertify(int enable);

/**
 * \brief   开启音量智能调节到合适音量(db).
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable [in] 1:开启. 0:关闭.
 * \retval   0          成功.
 * \retval  -1          失败.
 **/
extern status_t GxPlayer_SetAudioSmartVolume(unsigned int enable, AudioSmartConfig *config);

/**
 * \brief   设置切台后的初始响度值.
 *
 * \param   loudness [in].
 * \retval   0          成功.
 * \retval  -1          失败.
 **/
extern status_t GxPlayer_SetAudioSmartVolumeLoudness(float  loudness);

/**
 * \brief   获取正在使用的响度值.
 *
 * \param   loudness [in].
 * \retval   0          成功.
 * \retval  -1          失败.
 **/
extern status_t GxPlayer_GetAudioSmartVolumeLoudness(float *loudness);

/**
 * \brief   设置音频倍数播放模式，播放器默认NORMAL.
 *
 * \param   mode [in].
 * \retval   0          成功.
 * \retval  -1          失败.
 **/
extern status_t GxPlayer_SetAudioSpeedMode(AudioOutputSpeedMode mode);

/**
 * \brief   自定义设置SSL加密套件.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \retval  GX_PLAYER_OK          成功.
 * \retval  GX_PLAYER_ERROR       失败.
 **/
extern void GxPlayer_SetCustomSSLCipherSuiteCallback(CUSTOM_SSL_CHPHER_SUITE_CBK report);

/**
 * \brief   自定义设置多带宽网络流的分辨率设置.
 * \details 1. 播放器播放前调用有效.
 *
 **/
extern void GxPlayer_SetNetworkMulitBandwidthResolution(NetworkResolution *config);

/**
 * \brief   设置网络Audio延时.
 * \details 1. 播放器初始化之后调用有效.
 *
 * \param   enable [in] 设置的时长.
 * \retval   0          成功.
 * \retval  -1          失败.
 **/
extern status_t GxPlayer_SetNetAudioDelay(unsigned int vol);

__END_DECLS

#endif
/** @}*/
