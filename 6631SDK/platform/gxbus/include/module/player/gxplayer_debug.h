/** \addtogroup gxplayer_debug
 **  @{
 **/
#ifndef __GXPLAYER_DEBUG_H__
#define __GXPLAYER_DEBUG_H__

#include "gxcore.h"

__BEGIN_DECLS

/**
 * gxavdev 驱动级别调试
 **/
typedef enum {
	GX_PLAYER_DEBUG_LEVEL_ERROR   = (0x1 << 0),  ///< 错误打印输出.
	GX_PLAYER_DEBUG_LEVEL_INFO    = (0x1 << 1),  ///< 信息打印输出.
	GX_PLAYER_DEBUG_LEVEL_WARNING = (0x1 << 2),  ///< 警告打印输出.
	GX_PLAYER_DEBUG_LEVEL_DEBUG   = (0x1 << 3),  ///< 调试打印输出.
} PlayerDebugAVLevel;

/**
 * 音频解码功能调试
 **/
typedef enum {
	GX_PLAYER_DEBUG_DECODE_ESA       = (0x1 << 0),  ///< 音频解码 ESA 数据量打印.
	GX_PLAYER_DEBUG_DECODE_TASK      = (0x1 << 1),  ///< 音频解码任务流程打印.
	GX_PLAYER_DEBUG_DECODE_FIND_PTS  = (0x1 << 2),  ///< 音频解码查找 PTS 打印.
	GX_PLAYER_DEBUG_DECODE_FIX_PTS   = (0x1 << 3),  ///< 音频解码修复 PTS 打印.
	GX_PLAYER_DEBUG_BYPASS_ESA       = (0x1 << 4),  ///< 音频透传 ESA 数据量打印.
	GX_PLAYER_DEBUG_BYPASS_TASK      = (0x1 << 5),  ///< 音频透传任务流程打印.
	GX_PLAYER_DEBUG_BYPASS_FIND_PTS  = (0x1 << 6),  ///< 音频透传查找 PTS 打印.
	GX_PLAYER_DEBUG_BYPASS_FIX_PTS   = (0x1 << 7),  ///< 音频透传修复 PTS 打印.
	GX_PLAYER_DEBUG_CONVERT_ESA      = (0x1 << 8),  ///< 音频转码 ESA 数据量打印.
	GX_PLAYER_DEBUG_CONVERT_TASK     = (0x1 << 9),  ///< 音频转码任务流程打印.
	GX_PLAYER_DEBUG_CONVERT_FIND_PTS = (0x1 << 10), ///< 音频转码查找 PTS 打印.
	GX_PLAYER_DEBUG_CONVERT_FIX_PTS  = (0x1 << 11), ///< 音频转码修复 PTS 打印.
} PlayerDebugAdecContent;

/**
 * 音频播放功能调试
 **/
typedef enum {
	GX_PLAYER_DEBUG_R0_SYNC_PTS = (0x1 << 0),  ///< 音频播放通路 0 同步打印,包括: PCM 输出.
	GX_PLAYER_DEBUG_R1_SYNC_PTS = (0x1 << 1),  ///< 音频播放通路 1 同步打印,包括: ADPCM 输出.
	GX_PLAYER_DEBUG_R2_SYNC_PTS = (0x1 << 2),  ///< 音频播放通路 2 同步打印,包括: AC3 输出.
	GX_PLAYER_DEBUG_R3_SYNC_PTS = (0x1 << 3),  ///< 音频播放通路 3 同步打印,包括: EAC3, DTS 输出.
} PlayerDebugAoutContent;

/**
 * \brief   网络交互调试打印.
 * \details 1. 播放器初始化后调用,即时生效.
 * \details 2. 使用场景: 网络交互失败,查看与服务器交互信息,定位交互失败原因.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugNetworkStream(int enable);

/**
 * \brief   RTP 协议丢包率统计打印.
 * \details 1. 播放器初始化后调用,即时生效, V2 版本的 RTP Stream 有效.
 * \details 2. 使用场景: 网络交互失败,查看与服务器器交互信息,定位交互失败原因.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugNetworkRTPPacket(int enable);

/**
 * \brief   stream 读写速率统计打印.
 * \details 1. 播放器初始化后调用,即时生效,非 DVB 播放有效.
 * \details 2. 使用场景: 文件播放出现音视频卡顿,查看 U 盘等读写速率,帮助定位卡顿原因.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugStreamSpeed(int enable);

/**
 * \brief   packet 拆包速率统计打印.
 * \details 1. 播放器初始化后调用,即时生效,非 DVB 播放有效.
 * \details 2. 使用场景: 文件播放出现音视频卡顿,查看拆包速率,帮助定位卡顿原因.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugPacketSpeed(int enable);

/**
 * \brief   音视频解码器数据量打印.
 * \details 1. 播放器初始化后调用,即时生效,非 DVB 播放有效.
 * \details 2. 使用场景: 文件播放出现音视频卡顿,查看拆包速率,帮助定位卡顿原因.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugDemuxFillData(int enable);

/**
 * \brief   PVR 录制调试打印.
 * \details 1. 播放器初始化后调用,即时生效.
 * \details 2. 使用场景: 帮助 PVR 录制异常原因.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugStreamPVR    (int enable);

/**
 * \brief   音频解码器一次填包长度以及 PTS 打印.
 * \details 1. 播放器初始化后调用,即时生效,非使用硬件解复用生效.
 * \details 2. 使用场景: 音频解码器解码错误或 PTS 异常时,通过 ffmpeg 拆包对比,判断拆包是否出错.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugDemuxApts(int enable);

/**
 * \brief   视频解码器一次填包长度以及 PTS 打印.
 * \details 1. 播放器初始化后调用,即时生效,非使用硬件解复用生效.
 * \details 2. 使用场景: 视频解码器解码错误或 PTS 异常时,通过 ffmpeg 拆包对比,判断拆包是否出错.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugDemuxVpts(int enable);

/**
 * \brief   音视频 PTS, 差值, 数据量, 以及同步信息打印.
 * \details 1. 播放器初始化后调用,即时生效,非使用硬件解复用生效.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugAVInfo(int enable);

/**
 * \brief   打印播放器各模块使用情况.
 * \details 1. 播放器播放节目之后,通过查看各模块使用情况,方便进一步调试.
 *
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugPlayerMedia(void);

/**
 * \brief   播放器播放流程每个环节耗时打印.
 * \details 1. 播放器播放节目之后,通过该接口查看每个环节耗时,例如:切台时间.
 *
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugPlayerCostTime(void);

/**
 * \brief   字幕开启同步试打印.
 * \details 1. 字幕丢失或没有显示时,可通过该接口查看数据包是否丢失.
 *
 * \param   enable [in]     使能开关: 1 - 开启；0 - 关闭.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugSubtitleSync(int enable);

/**
 * \brief   音频解码器开启级别调试打印.
 * \details 1. 音频解码器模块级别打印控制输出,对单模块进行控制.
 *
 * \param   level [in]      级别调试,详见: gxdocref PlayerDebugAVLevel.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugAdecConfig(PlayerDebugAVLevel level);

/**
 * \brief   音频解码器开启功能调试打印.
 * \details 1. 音频解码器模块功能打印控制输出,对单模块进行控制.
 *
 * \param   content [in]    功能调试,详见: gxdocref PlayerDebugAdecContent.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugAdecContent(PlayerDebugAdecContent content);

/**
 * \brief   音频播放器开启级别调试打印.
 * \details 1. 音频播放器模块级别打印控制输出,对单模块进行控制.
 *
 * \param   level [in]      级别调试,详见: gxdocref PlayerDebugAVLevel.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugAoutConfig (PlayerDebugAVLevel level);

/**
 * \brief   音频播放器开启功能调试打印.
 * \details 1. 音频播放器模块功能打印控制输出,对单模块进行控制.
 *
 * \param   content [in]    功能调试,详见: gxdocref PlayerDebugAoutContent.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DebugAoutContent(PlayerDebugAoutContent content);

/**
 * \brief   Dump 音频解码器 PCM 数据.
 * \details 1. 使用场景: 音频解码异常时,通过 Dump 音频解码器数据,判定解码数据是否正常.
 *
 * \param   enable [in]     使能开启: 1 - 开启 dump; 0 - 关闭 dump.
 * \param   url    [in]     录制存储文件的 url.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DumpAdecPCM(int enable, char *url);

/**
 * \brief   Dump 音频解码器 ESA 数据.
 * \details 1. 使用场景: 音频解码异常时,通过 Dump 音频解码器数据,判定输入数据是否正常.
 *
 * \param   enable [in]     使能开启: 1 - 开启 dump; 0 - 关闭 dump.
 * \param   url    [in]     录制存储文件的 url.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DumpAdecESA(int enable, char *url);

/**
 * \brief   Dump 音频播放器 PCM 数据.
 * \details 1. 使用场景: 音频播放异常时,通过 Dump 音频播放器数据,判定播放数据是否正常.
 *
 * \param   enable [in]     使能开启: 1 - 开启 dump; 0 - 关闭 dump.
 * \param   url    [in]     录制存储文件的 url.
 * \retval  GX_PLAYER_OK    成功.
 * \retval  GX_PLAYER_ERROR 失败.
 **/
extern status_t GxPlayer_DumpAoutPCM(int enable, char *url);

/**
 * \brief   播放器内存使用情况打印.
 * \details 1. 使用场景: 查找播放器的内存泄露,冲内存.
 *
 * \retval  void.
 **/
extern void GxPlayer_HeapInfo(void);

/**
 * \brief   获取播放器使用的洞内存的使用情况
 * \details 1. 使用场景: 调试,查看播放器当前洞内存的使用情况;
 *
 * \param   info    [out]  Memhole 使用情况.
 * \retval  void.
 **/
extern void GxPlayer_MemholeInfo(GxMemholeInfo *info);

__END_DECLS

#endif
/** @}*/
