/** \addtogroup gxmediaapi
 **  @{
 **/
#ifndef __GX_MEDIA_API_H__
#define __GX_MEDIA_API_H__

#include "gxcore.h"
#include "gxplayer_decoder.h"

typedef enum {
	GXMEDIA_SOURCE_TS,
	GXMEDIA_SOURCE_ES,
	GXMEDIA_SOURCE_PES,
	GXMEDIA_SOURCE_DVBTS,
} GxMediaSourceType;

typedef enum {
	GXMEDIA_AVSTREAM_ES,
	GXMEDIA_AVSTREAM_TS,
} GxMediaAVStreamType;

typedef enum {
	GXMEDIA_OUTPUT_NORMAL  = 0,
	GXMEDIA_OUTPUT_PROTECT = 1,
} GxMediaProtectMode;

typedef enum {
	GXMEDIA_STREAM_VIDEO,
	GXMEDIA_STREAM_AUDIO,
	GXMEDIA_STREAM_SUBTITLE
} GxMediaStreamType;

typedef enum {
	GXMEDIA_VIDEO_DECODE_ALL,
	GXMEDIA_VIDEO_DECODE_I_ONLY,
	GXMEDIA_VIDEO_DECODE_IP,
} GxMediaVideoDecodeMode;

typedef enum {
	GXMEDIA_VIDEO_NORMAL_SPEED, /* 正常 */
	GXMEDIA_VIDEO_FAST_SPEED,   /* 快进 */
	GXMEDIA_VIDEO_SLOW_SPEED,   /* 慢放 */
	GXMEDIA_VIDEO_STEP_SPEED,   /* 快退 */
} GxMediaVideoDecodeSpeedmode;

typedef enum {
	GXMEDIA_ERR_NO_ERROR,
	GXMEDIA_ERR_UNKOWN_ERROR,
	GXMEDIA_ERR_ES_OVERFLOW,
	GXMEDIA_ERR_DEC_UNSUPPORT,
	GXMEDIA_ERR_SIZE_UNSPPORT,
	GXMEDIA_ERR_SIZE_CHANGED,
	GXMEDIA_ERR_FB_OVERFLOW
} GxMediaDecErrCode;

typedef enum {
	GXMEDIA_STATE_NO_DEC,
	GXMEDIA_STATE_RUNNING,
	GXMEDIA_STATE_STOPPED,
	GXMEDIA_STATE_ERROR
} GxMediaDecState;

typedef struct {
	int              enable;
	unsigned int     low_gate;
	unsigned int     high_gate;
} GxMediaSyncParam;

typedef struct {
	int right_now;
	int target_value;
	int step;
} GxMediaAudioVolume;

typedef struct {
	AudioCodecType      codectype;
	int                 sample_rate;
	int                 sample_bits;
	int                 channel_num;
	int                 big_endian;
	GxMediaSyncParam    sync;
	int                 enough_data;
	GxMediaAudioVolume  volume;
	int                 bypass; //0:decode, 1:bypas
	GxMediaAVStreamType stream_data; //0:es, 1:ts
	int                 stream_pid;  //stream_data 为 ts 有效
} GxMediaAudioPara;

typedef struct {
	int       width;
	int       height;
} GxMediaVideoFrame;

typedef int (*GxMediaVideoFrameSwitchCB)(GxMediaVideoFrame *frame);

typedef struct {
	unsigned int frame_id;
	unsigned char hash[32];
} GxMediaVideoFrameHash;

typedef int (*GxMediaVideoEachFrameCB)(int id);

typedef struct {
	VideoCodecType              codectype;
	int                         width;
	int                         height;
	int                         mosaic_gate;
	int                         frame_rate;
	GxMediaVideoDecodeMode      decode_mode;
	GxMediaVideoDecodeSpeedmode speed_mode;
	GxMediaSyncParam            sync;
	GxMediaVideoFrameSwitchCB   vf_cb;
	GxMediaVideoEachFrameCB     each_frame_cb;
	GxMediaVideoEachFrameCB     stream_finish_cb;
} GxMediaVideoPara;

typedef struct {
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
} GxMediaVideoWindow;

typedef struct {
	unsigned int adsp_modid;
	unsigned int aplay_modid;
} GxMediaAudioMod;

typedef struct {
	unsigned int vdec_modid;
	unsigned int vout_modid;
} GxMediaVideoMod;

typedef struct {
	unsigned int demux_modid;
} GxMediaDemuxMod;

typedef struct {
	unsigned int stc_modid;
} GxMediaStcMod;

typedef struct {
	GxMediaAudioMod audio;
	GxMediaVideoMod video;
	GxMediaDemuxMod demux;
	GxMediaStcMod   stc;
} GxMediaModuleMod;

typedef struct {
	GxMediaSourceType   source_type;
	GxMediaProtectMode  protect_mode;
	GxMediaAVStreamType a_stream_type;
} GxMediaModuleModPara;

typedef struct {
	GxMediaAudioPara aparam;
	GxMediaVideoPara vparam;
	int tsid;
	int audPid;
	int vidPid;
	int pcrPid;
	int freq_HZ;
	int sync_mode;
} GxMediaModulePara;

typedef int (*GxMediaDecryptCB)(void *priv, unsigned char *src, unsigned char *dst, unsigned int len);

typedef struct {
	GxMediaSourceType source_type;
	GxMediaStreamType stream_type;
	uint8_t* buf;
	int len;
	int pts;
	void             *priv;
	GxMediaDecryptCB  decrypt_cb;
} GxMediaModuleInject;

typedef struct {
	int esa_free_size;
	int esa_used_size;
	int pcm_free_size;
	int pcm_used_size;
} GxMediaModuleAudioFifo;

typedef struct {
	int esv_free_size;
	int esv_used_size;
} GxMediaModuleVideoFifo;

typedef struct {
	int ts_free_size;
	int ts_used_size;
} GxMediaModuleDemuxFifo;

typedef struct {
	int64_t                apts;
	int64_t                vpts;
	int64_t                stc;
	GxMediaModuleAudioFifo audio;
	GxMediaModuleVideoFifo video;
	GxMediaModuleDemuxFifo demux;
} GxMediaModuleInfo;

typedef struct {
	GxMediaDecState   state;
	GxMediaDecErrCode err_code;
} GxMediaModuleDecState;

typedef struct {
	unsigned width;
	unsigned height;

	unsigned long long dec_frame_cnt;
	unsigned long long disp_frame_cnt;
} GxMediaVideoFrameInfo;

typedef struct {
	GxMediaModuleDecState video;
	GxMediaModuleDecState audio;
} GxMediaModuleState;

handle_t GxMediaApi_ModuleOpen      (GxMediaSourceType type);
handle_t GxMediaApi_ModuleOpen2     (GxMediaSourceType type, GxMediaProtectMode mode);
handle_t GxMediaApi_ModuleOpen3     (GxMediaModuleMod mod, GxMediaModuleModPara mpara);
status_t GxMediaApi_ModuleClose     (handle_t handle);
status_t GxMediaApi_ModuleConfig    (handle_t handle, GxMediaModulePara para);
status_t GxMediaApi_ModuleStart     (handle_t handle);
status_t GxMediaApi_ModuleStop      (handle_t handle, int freeze);
status_t GxMediaApi_ModulePause     (handle_t handle);
status_t GxMediaApi_ModuleResume    (handle_t handle);
status_t GxMediaApi_ModuleFinished  (handle_t handle);
status_t GxMediaApi_ModuleInfo      (handle_t handle, GxMediaModuleInfo  *info);
status_t GxMediaApi_ModuleState     (handle_t handle, GxMediaModuleState *state);
int      GxMediaApi_ModuleInjectData(handle_t handle, GxMediaModuleInject inject);
status_t GxMediaApi_ModuleSpeed     (handle_t handle, float speed);

handle_t GxMediaApi_AudioOpen       (handle_t demux_handle);
handle_t GxMediaApi_AudioOpen3      (GxMediaAudioMod mod);
status_t GxMediaApi_AudioBindDemux  (handle_t handle, handle_t demux_handle);
status_t GxMediaApi_AudioClose      (handle_t handle);
status_t GxMediaApi_AudioConfig     (handle_t handle, GxMediaAudioPara param);
status_t GxMediaApi_AudioStart      (handle_t handle);
status_t GxMediaApi_AudioStop       (handle_t handle);
status_t GxMediaApi_AudioPause      (handle_t handle);
status_t GxMediaApi_AudioResume     (handle_t handle);
status_t GxMediaApi_AudioGetFifoInfo(handle_t handle, GxMediaModuleAudioFifo *info);
status_t GxMediaApi_AudioGetDecState(handle_t handle, GxMediaModuleDecState  *state);
status_t GxMediaApi_AudioFinished   (handle_t handle);
status_t GxMediaApi_AudioEos        (handle_t handle);
status_t GxMediaApi_AudioGetPts     (handle_t handle, int64_t* apts);
int      GxMediaApi_AudioInjectData (handle_t handle, GxMediaSourceType type, uint8_t* buffer, int len, int pts);
status_t GxMediaApi_AudioSpeed      (handle_t handle, float speed);

handle_t GxMediaApi_VideoOpen       (handle_t demux_handle);
handle_t GxMediaApi_VideoOpen3      (GxMediaVideoMod mod);
status_t GxMediaApi_videoBindDemux  (handle_t handle, handle_t demux_handle);
status_t GxMediaApi_VideoClose      (handle_t handle);
status_t GxMediaApi_VideoConfig     (handle_t handle, GxMediaVideoPara param);
status_t GxMediaApi_VideoSetWindow  (handle_t handle, GxMediaVideoWindow win);
status_t GxMediaApi_VideoStart      (handle_t handle);
status_t GxMediaApi_VideoStop       (handle_t handle, int freeze);
status_t GxMediaApi_VideoPause      (handle_t handle);
status_t GxMediaApi_VideoResume     (handle_t handle);
status_t GxMediaApi_VideoGetFifoInfo(handle_t handle, GxMediaModuleVideoFifo *info);
status_t GxMediaApi_VideoGetDecState(handle_t handle, GxMediaModuleDecState  *state);
status_t GxMediaApi_VideoGetFrameInfo(handle_t handle, GxMediaVideoFrameInfo *info);
status_t GxMediaApi_VideoGetPts     (handle_t handle, int64_t* vpts);
status_t GxMediaApi_VideoFinished   (handle_t handle);
status_t GxMediaApi_VideoEos        (handle_t handle);
status_t GxMediaApi_VideoGetHash    (handle_t handle, GxMediaVideoFrameHash *hash);
status_t GxMediaApi_VideoSpeed      (handle_t handle, float speed);

int GxMediaApi_VideoInjectData(handle_t handle, GxMediaSourceType type, uint8_t* buffer, int len, int pts);

handle_t GxMediaApi_DemuxOpen       (GxMediaSourceType SourceType);
handle_t GxMediaApi_DemuxOpen3      (GxMediaDemuxMod mod, GxMediaSourceType SourceType);
status_t GxMediaApi_DemuxConfig     (handle_t handle, int Tsid, int AudPid, int VidPid, int PcrPid);
status_t GxMediaApi_DemuxStart      (handle_t handle);
status_t GxMediaApi_DemuxStop       (handle_t handle);
status_t GxMediaApi_DemuxClose      (handle_t handle);
status_t GxMediaApi_DemuxGetAfifo   (handle_t handle);
status_t GxMediaApi_DemuxGetVfifo   (handle_t handle);
status_t GxMediaApi_DemuxGetFifoInfo(handle_t handle, GxMediaModuleDemuxFifo *info);
int      GxMediaApi_DemuxInjectData (handle_t handle, uint8_t* buffer, int len);

handle_t GxMediaApi_StcOpen   (void);
handle_t GxMediaApi_StcOpen3  (GxMediaStcMod mod);
status_t GxMediaApi_StcConfig (handle_t handle, int freq_HZ, int sync_mode);
status_t GxMediaApi_StcStart  (handle_t handle);
status_t GxMediaApi_StcStop   (handle_t handle);
status_t GxMediaApi_StcPause  (handle_t handle);
status_t GxMediaApi_StcResume (handle_t handle);
status_t GxMediaApi_StcClose  (handle_t handle);
status_t GxMediaApi_StcGetTime(handle_t handle, int64_t* stc);
status_t GxMediaApi_StcSpeed  (handle_t handle, float speed);

status_t GxMediaApi_Init(void);


/**
 * 帧信息
 */
typedef struct {
	unsigned int   samplefre;      ///<采样率: 例如32000Hz/44100Hz/48000Hz
	unsigned int   channels;       ///<通道数: 支持小于等于2声道
	unsigned int   bitwidth;       ///<位宽: 固定16bit
	unsigned int   endian;         ///<大小端: 固定大端模式
	unsigned int   interlace;      ///<交织: 固件交织储存
	unsigned char *user_frame_buf; ///<用户操作帧内存
	unsigned char *kern_frame_buf; ///<内核操作帧内存
	unsigned int   frame_size;     ///<帧大小
	unsigned int   frame_no;       ///<帧号:从0开始累加
	AudioCodecType codectype;      ///<编码:数据帧编码类型
	int            sample_val;     ///<解码帧样点平均值
	int            down_mix;       ///<解码帧解码downmix配置
	int            volume_db;      ///<解码帧解码音量配置
	int            ch_num;         ///<播放声道个数.
	int            ch_size;        ///<播放每个声道大小.
} GxMediaApiOneFrame;

/**
 * 功能描述：
 * 　　打开音频播放器接口
 * 使用场景：
 *     该接口只支持音频播放器获取播放数据功能，不支持直接给音频播放器播放数据
 * \retval   0 成功
 * \retval  -1 失败
 */
handle_t GxMediaApi_AoutOpen(void);

/**
 * 功能描述：
 * 　　从打开音频播放器读取一帧PCM数据
 * 使用场景：
 * 　　该接口只对PCM数据，不使用在AC3/EAC3等压缩数据
 * 使用注意事项:
 *     该接口frame参数是输出参数
 * \param handle 打开播放器句柄
 * \param frame  获取到的帧信息
 * \param timeout_ms 超时时间.
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AoutPCMFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms);

/**
 * 功能描述：
 * 　　释放获取音频播放器的帧数据
 * 使用场景：
 * 　　该接口只对PCM数据，不使用在AC3/EAC3等压缩数据
 * 使用注意事项:
 *     该接口frame参数是输入参数，通常每一次对应alloc，必定会有相应的free
 * \param handle 打开播放器句柄
 * \param frame  输入释放的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AoutPCMFrameFree (handle_t handle, GxMediaApiOneFrame *frame);

/**
 * 功能描述：
 * 　　关闭频播放器接口
 * 使用场景：
 *     该接口只支持音频播放器获取播放数据功能，不支持直接给音频播放器播放数据
 * \param handle 打开播放器句柄
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AoutClose(handle_t handle);

/**
 * 功能描述：
 * 　　打开第一路音频解码器
 * 使用场景：
 *     该接口只支持音频解码器获取播放数据功能，不支持直接给音频播放器播放数据
 * \retval   0 成功
 * \retval  -1 失败
 */
handle_t GxMediaApi_AdecOpen(void);

/**
 * 功能描述：
 * 　　从音频解码器读取一帧 PCM 数据.
 * \param handle 打开播放器句柄
 * \param frame  获取到的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecPCMFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms);

/**
 * 功能描述：
 * 　　释放获取音频解码器的帧数据.
 * 注意事项:
 *     需要按照获取 PCM 帧数据顺序依次释放.
 * \param handle 打开播放器句柄
 * \param frame  输入释放的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecPCMFrameFree(handle_t handle, GxMediaApiOneFrame *frame);

/**
 * 功能描述：
 * 　　从音频解码器读取一帧 ESA 数据.
 * \param handle 打开播放器句柄
 * \param frame  获取到的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecESAFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms);

/**
 * 功能描述：
 * 　　释放获取音频解码器的帧数据
 * 注意事项:
 *     需要按照获取 PCM 帧数据顺序依次释放.
 * \param handle 打开播放器句柄
 * \param frame  输入释放的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecESAFrameFree(handle_t handle, GxMediaApiOneFrame *frame);


/**
 * 功能描述：
 * 　　关闭频解码器接口
 * 使用场景：
 *     该接口只支持音频播放器获取播放数据功能，不支持直接给音频播放器播放数据
 * \param handle 打开播放器句柄
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecClose(handle_t handle);

/**
 * 功能描述：
 * 　　注册给音频解码器重采样的参数,目前只支持44.1kHz的重采样.
 *     支持6605s/taurus/sirius/gemini/cyguns,不支持芯片注册时打印错误.
 */
void GxMediaApi_AdecResampleRegister(void);

/**
 * 功能描述：
 * 　　注册给音频解码器重采样的参数,支持所有采样率的重采样.
 *     支持canopus/vega/..,不支持芯片注册时打印错误.
 */
void GxMediaApi_AdecResampleAllRegister(void);

/**
 * 功能描述：
 * 　　注册给音频播放器重采样的参数,支持16kHz的重采样.
 */
void GxMediaApi_AoutResampleRegister(void);

/**
 * 功能描述：
 * 　　从音频解码器读取一帧重采样 PCM 数据, 固定输出44.1khz/2声道 PCM.
 * \param handle 打开播放器句柄
 * \param frame  获取到的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecResamplePCMFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms);

/**
 * 功能描述：
 * 　　释放获取音频解码器的帧重采样数据.
 * 注意事项:
 *     需要按照获取 PCM 帧数据顺序依次释放.
 * \param handle 打开播放器句柄
 * \param frame  输入释放的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecResamplePCMFrameFree(handle_t handle, GxMediaApiOneFrame *frame);

/**
 * 功能描述：
 * 　　设置音频解码器的帧重采样数据的通道数.
 * \param handle    打开播放器句柄
 * \param channels  通道数
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecResampleSetChannels(handle_t handle, int channels);

/**
 * 功能描述：
 * 　　设置音频解码器的帧重采样数据的音量.
 * \param handle    打开播放器句柄
 * \param channels  音量,默认值 100, 范围 0 - 100
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecResampleSetVolume(handle_t handle, int volume);

/**
 * 功能描述：
 * 　　设置音频解码器的帧重采样数据的静音状态.
 * \param handle    打开播放器句柄
 * \param mute      静音时能,默认值 0, 范围 0 或者 1
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecResampleSetMute(handle_t handle, int mute);

/**
 * 功能描述：
 * 　　从音频解码器读取一帧解码的 ESA 数据.
 * \param handle 打开播放器句柄
 * \param frame  获取到的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecDecESAFrameAlloc(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms);

/**
 * 功能描述：
 * 　　释放获取音频解码器的帧重采样数据.
 * 注意事项:
 *     需要按照获取 PCM 帧数据顺序依次释放.
 * \param handle 打开播放器句柄
 * \param frame  输入释放的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_AdecDecESAFrameFree(handle_t handle, GxMediaApiOneFrame *frame);

typedef enum {
	GXMEDIA_APLAY_STERO = 0,
    GXMEDIA_APLAY_RIGHT = 1,
    GXMEDIA_APLAY_LEFT  = 2,
    GXMEDIA_APLAY_MULTI = 3,
} GxMediaApiAPlayChannel;

/**
 * 功能描述：
 * 　　打开音频播放器接口
 * 使用场景：
 *     该接口支持直接给音频播放器播放数据
 * \retval   0 成功
 * \retval  -1 失败
 */
handle_t GxMediaApi_APlayOpen(int modid);

/**
 * 功能描述：
 * 　　关闭音频播放器接口
 * 使用场景：
 *     该接口只支持直接给音频播放器播放数据
 * \param handle 打开播放器句柄
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_APlayClose(handle_t handle);

/**
 * 功能描述：
 * 　　从音频播放器获取一帧空闲的 PCM 数据帧.
 * \param handle 打开播放器句柄
 * \param frame  获取到的帧信息
 * \param timeout_ms 暂时不可用
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_APlayGetPCMFrame(handle_t handle, GxMediaApiOneFrame *frame, int timeout_ms);

/**
 * 功能描述：
 * 　　注入音频播放器一帧填充的 PCM 数据帧.
 * \param handle 打开播放器句柄
 * \param frame  获取到的帧信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_APlayPushPCMFrame(handle_t handle, GxMediaApiOneFrame *frame);

/**
 * 功能描述：
 * 　　设置音频播放器输出声道.
 * \param handle  打开播放器句柄
 * \param channel 设置的声道信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_APlaySetChannel(handle_t handle, GxMediaApiAPlayChannel channel);

/**
 * 功能描述：
 * 　　设置音频播放器是否静音.
 * \param handle  打开播放器句柄
 * \param mute    设置的静音信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_APlaySetMute(handle_t handle, int mute);

/**
 * 功能描述：
 * 　　设置音频播放器输出音量.
 * \param handle  打开播放器句柄
 * \param volume  设置的音量信息
 * \retval   0 成功
 * \retval  -1 失败
 */
status_t GxMediaApi_APlaySetVolume(handle_t handle, int volume);
#endif
/** @}*/
