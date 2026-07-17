/** \addtogroup  gxplayer_define
 *  @{
 */
#ifndef __GX_PLAYER__DEFINE_H__
#define __GX_PLAYER__DEFINE_H__

__BEGIN_DECLS

#define player_attribute_deprecated __attribute__((deprecated))

#define PLAYER_MAX_SUB_LINE                     (12)   ///< 播放器字幕最大行数.
#define GXSTREAM_MAX_TRACK                      (16)   ///< 播放器增加或删减流的最大个数.
#define GXPLAYER_MAX_BAND_WIDTH                 (16)   ///< 播放器播放 M3U8 的节目切换的最大个数,包括: 不同分辨率下的高标清.
#define GXPLAYER_PVR_EVENT_COUNT                (32)   ///< 播放器支持 PVR 同时开启事件的最大个数．
#define GXPLAYER_PVR_LIST_COUNT                 (32)   ///< 播放器支持 PVR 同时开启事件集的最大个数．

#define PLAYER_NAME_LONG                        (31)   ///< 播放器名称的最大长度.
#define PLAYER_URL_LONG                         (3072) ///< 播放器 URL 的最大长度.

#define PLAYER_TARCK_LANG_LONG                  (8)    ///< 播放器流的语言描述最大长度.
#define PLAYER_TARCK_NAME_LONG                  (32)   ///< 播放器流的名称描述最大长度.
#define PLAYER_TARCK_CODEC_LONG                 (32)   ///< 播放器流的编码描述最大长度.
#define PLAYER_MIME_TYPE_LONG                   (32)   ///< 播放器流的类型描述最大长度.

#define PLAYER_PROG_DESC_LONG                   (32)   ///< 播放器节目描述信息最大长度.
#define PLAYER_MAX_PROG_COUNT                   (32)   ///< 播放器节目最大个数.
#define PLAYER_MAX_PROG_STREAM_COUNT            (16)   ///< 播放器每个节目的音频/视频/字幕流最大个数.

#define PLAYER_MAX_TRACK_AUDIO                  (16)   ///< 播放器音频流的最多个数.
#define PLAYER_MAX_TRACK_SUB                    (32)   ///< 播放器字幕流的最多个数.
#define PLAYER_MAX_TRACK_ADD                    (32)   ///< 播放器 TS-BUFF 追加流的最多个数.

#define PLAYER_MAX_SUB_LOAD                     (1)    ///< 播放器同时加载字幕的最大个数.
#define PLAYER_MAX_ENCRYPT_KEY_LEN              (32)   ///< 播放器 ETS 流录制/播放加密密钥的最大长度.

#define PLAYER_CONFIG_PLAY_RESERVED_MEMSIZE         "Play>resv_mem_size"            ///< 播放器堆内存使用的最大大小，单位 Byte.
#define PLAYER_CONFIG_PLAY_RESERVED_HOLESIZE        "Play>resv_hole_size"           ///< 播放器洞内存使用的最大大小，单位 Byte.
#define PLAYER_CONFIG_INDEX_RESERVED_MEMSIZE        "Play>index_cache_size"         ///< 播放器多媒体文件索引解析内存使用的最大大小，单位 Byte.
#define PLAYER_CONFIG_RECORD_CACHED_MEMSIZE         "Play>record_sw_cache_size"     ///< 播放器录制时 TS SW-Cache 的大小，单位 Byte, 录制时 demux 会从 HW-Cache 拷贝到 SW-Cache.
#define PLAYER_CONFIG_RECORD_TSW_MEMSIZE            "Play>record_hw_cache_size"     ///< 播放器录制时 TS HW-Cache 的大小，单位 Byte.
#define PLAYER_CONFIG_NETWORK_CACHED_MEMSIZE        "Play>network_cache_size"       ///< 网络播放器数据缓冲区的大小，单位 Byte.
#define PLAYER_CONFIG_NETWORK_CACHED_SEEK_SIZE      "Play>network_cache_seek_size"  ///< 网络播放器数据缓冲区内可 seek back 的大小,单位 Byte.
#define PLAYER_CONFIG_NETWORK_TIMEOUT               "Play>network_timeout"          ///< 网络播放器超时时长，单位毫秒.
#define PLAYER_CONFIG_NETWORK_RESOLUTION_WIDTH      "Play>network_reslution_width"  ///< 设置HLS/DASH支持指定分辨率的水平方向上的像素数.
#define PLAYER_CONFIG_NETWORK_RESOLUTION_HEIGHT     "Play>network_reslution_height" ///< 设置HLS/DASH支持指定设置分辨率的垂直方向上的像素数.
#define PLAYER_CONFIG_NETWORK_AUDIO_DELAY           "Play>network_audio_delay"      ///< 网络音频流延时时长，单位毫秒.
#define PLAYER_CONFIG_NETWORK_RECVBUF               "Play>network_recvbuf"          ///< 网络播放器协议栈接收缓冲区大小，单位 Byte.
#define PLAYER_CONFIG_NETWORK_HLS_MULIPLE_LIST      "Play>network_hls_muliple_list" ///< 网络播放器协议 hls 是否开启 多http下载， 1：开启 0：关闭.
#define PLAYER_CONFIG_NETWORK_FFHLS_MAX_PLAYLIST    "Play>network_ffhls_max_playlist" ///< 网络播放器协议 hls 最大支持几路链接(用于:多音轨多带宽的流).
#define PLAYER_CONFIG_NETWORK_HTTP_PERSISTENT_CONNECTIONS    "Play>network_http_persistent_connections" ///< 使用HTTP持久连接
#define PLAYER_CONFIG_NETWORK_DEFAULT_BANDWIDTH_INDEX    "Play>network_default_bandwidth_index" ///< 设置默认带宽的清晰度.(正数表示从最高带宽的第几个，负数表示从最低带宽的第几个(仅支持前三带宽，如:1 2 3：最大 第二大 第三大;-1 -2 -3：最小 第二小 第三小))
#define PLAYER_CONFIG_NETWORK_IS_FALSE_LIVE         "Play>network_is_falselive"     ///< 直播模式改为点播模式(用于dash hls的高延时直播协议上)
#define PLAYER_CONFIG_NETWORK_HTTP_AUTO_ADAPABILITY  "Play>network_http_auto_adapability"     ///< http 自适应(abr)
#define PLAYER_CONFIG_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION  "Play>network_segment_func_is_full_detection"     ///< hls/dash 针对多AV源时全探测
#define PLAYER_CONFIG_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE  "Play>network_screen_to_demux_pts_difference"     ///< 当video master时，显示到拆包的PTS差值(常用Miracast同步时)
#define PLAYER_CONFIG_MAX_URL_SIZE             "Play>max_url_size"                  ///< 设置播放器支持url最大值(byte).
#define PLAYER_CONFIG_MEDIA_MAX_STREAMS             "Play>media_max_streams"        ///< 网络播放或者本地播放多媒体文件时，最大支持流个数.
#define PLAYER_CONFIG_MPEGDASH_MAX_PERIODS          "Play>mpegdash_max_periods"        ///< 网络播放dash流时，支持最大的periods个数.
#define PLAYER_CONFIG_DELAY_CACHE_SIZE              "Play>delay_cache_size"         ///< 播放器 TS-BUFF 软件缓冲区大小.
#define PLAYER_CONFIG_DELAY_CACHE_HW_SIZE           "Play>delay_hw_cache_size"      ///< 播放器 TS-BUFF 硬件缓冲区大小.
#define PLAYER_CONFIG_DELAY_CACHE_TO_MEMORY         "Play>delay_cache2memory"       ///< 播放器 TS-BUFF 是否开启 cache to memory.
#define PLAYER_CONFIG_FLAG_ERROR_STOP               "Play>play_error_stop"          ///< 播放器音频或视频发生异常时是否停止播放.
#define PLAYER_CONFIG_FLAG_SOF_REPLAY               "Play>play_sof_replay"          ///< 播放器文件快退结束时是否从头重新播放.
#define PLAYER_CONFIG_STREAM_BUFFER_SIZE            "Play>stream_buffer_size"       ///< 播放器流数据读写的缓冲区大小, 单位 Byte.

#define PLAYER_CONFIG_AUTOPLAY_VIDEO_NAME           "AutoPlay>video_name"           ///< 播放器服务初始化自动播放功能, 视频播放器的名称.
#define PLAYER_CONFIG_AUTOPLAY_VIDEO_URL            "AutoPlay>video_url"            ///< 播放器服务初始化自动播放功能, 视频的 URL.
#define PLAYER_CONFIG_AUTOPLAY_RADIO_NAME           "AutoPlay>radio_name"           ///< 播放器服务初始化自动播放功能, 音频播放器的名称.
#define PLAYER_CONFIG_AUTOPLAY_RADIO_URL            "AutoPlay>radio_url"            ///< 播放器服务初始化自动播放功能, 音频的 URL.
#define PLAYER_CONFIG_AUTOPLAY_RADIO_FLAG           "AutoPlay>radio_flag"           ///< 播放器服务初始化自动播放功能, 播放的 URL 是否是 Radio: 1 - 播放音频.
#define PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG            "AutoPlay>play_flag"            ///< 播放器服务初始化自动播放功能, 是否启动自动播放: 1 - 启用自动播放功能.

#define PLAYER_CONFIG_VIDEO_INTERFACE               "Video>interface"               ///< 视频输出接口,定义详见: gxdocref VideoOutputInterface.
#define PLAYER_CONFIG_VIDEO_RESOLUTION_HDMI         "Video>resoluton_hdmi"          ///< 视频接口 HDMI   输出制式,定义详见: gxdocref VideoOutputMode.
#define PLAYER_CONFIG_VIDEO_RESOLUTION_YUV          "Video>resoluton_yuv"           ///< 视频接口 YUV    输出制式,定义详见: gxdocref VideoOutputMode.
#define PLAYER_CONFIG_VIDEO_RESOLUTION_RCA          "Video>resoluton_rca"           ///< 视频接口 RCA    输出制式,定义详见: gxdocref VideoOutputMode.
#define PLAYER_CONFIG_VIDEO_RESOLUTION_RCA1         "Video>resoluton_rca1"          ///< 视频接口 RCA1   输出制式,定义详见: gxdocref VideoOutputMode.
#define PLAYER_CONFIG_VIDEO_RESOLUTION_SCART        "Video>resoluton_scart"         ///< 视频接口 SCART  输出制式,定义详见: gxdocref VideoOutputMode.
#define PLAYER_CONFIG_VIDEO_RESOLUTION_SVIDEO       "Video>resoluton_svideo"        ///< 视频接口 SVIDEO 输出制式,定义详见: gxdocref VideoOutputMode.
#define PLAYER_CONFIG_VIDEO_BRIGHTNESS_HDMI         "Video>brightness_hdmi"         ///< 视频接口 HDMI   输出亮度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_BRIGHTNESS_YUV          "Video>brightness_yuv"          ///< 视频接口 YUV    输出亮度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_BRIGHTNESS_RCA          "Video>brightness_rca"          ///< 视频接口 RCA    输出亮度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_BRIGHTNESS_RCA1         "Video>brightness_rca1"         ///< 视频接口 RCA1   输出亮度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_BRIGHTNESS_SCART        "Video>brightness_scart"        ///< 视频接口 SCART  输出亮度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_BRIGHTNESS_SVIDEO       "Video>brightness_svideo"       ///< 视频接口 SVIDEO 输出亮度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_SATURATION_HDMI         "Video>saturation_hdmi"         ///< 视频接口 HDMI   输出色度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_SATURATION_YUV          "Video>saturation_yuv"          ///< 视频接口 YUV    输出色度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_SATURATION_RCA          "Video>saturation_rca"          ///< 视频接口 RCA    输出色度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_SATURATION_RCA1         "Video>saturation_rca1"         ///< 视频接口 RCA1   输出色度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_SATURATION_SCART        "Video>saturation_scart"        ///< 视频接口 SCART  输出色度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_SATURATION_SVIDEO       "Video>saturation_svideo"       ///< 视频接口 SVIDEO 输出色度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_CONTRAST_HDMI           "Video>contrast_hdmi"           ///< 视频接口 HDMI   输出对比度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_CONTRAST_YUV            "Video>contrast_yuv"            ///< 视频接口 YUV    输出对比度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_CONTRAST_RCA            "Video>contrast_rca"            ///< 视频接口 RCA    输出对比度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_CONTRAST_RCA1           "Video>contrast_rca1"           ///< 视频接口 RCA1   输出对比度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_CONTRAST_SCART          "Video>contrast_scart"          ///< 视频接口 SCART  输出对比度,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_CONTRAST_SVIDEO         "Video>contrast_svideo"         ///< 视频接口 SVIDEO 输出对比度,取值范围: 0 ~ 100.

#define PLAYER_CONFIG_VIDEO_ESV_SIZE                "Video>esv_size"                ///< 视频解码器 ESV 大小, 单位Byte, 需 1KByte 对齐.
#define PLAYER_CONFIG_VIDEO_DISPLAY_SCREEN          "Video>screen"                  ///< 视频显示比例: 0 - 4:3；1 - 16:9.
#define PLAYER_CONFIG_VIDEO_SCREEN_XRES             "Video>xres"                    ///< 视频显示 x 轴坐标.
#define PLAYER_CONFIG_VIDEO_SCREEN_YRES             "Video>yres"                    ///< 视频显示 y 轴坐标.
#define PLAYER_CONFIG_VIDEO_ASPECT                  "Video>aspect"                  ///< 视频显示视角比,定义详见: gxdocref VideoAspect.
#define PLAYER_CONFIG_VIDEO_DRCMODE                 "Video>drc_mode"                ///< 视频动态范围转换器工作模式,详见: gxdocref VideoDrcMode.
#define PLAYER_CONFIG_VIDEO_FREEZE_SWITCH           "Video>freeze_switch"           ///< 视频是否静帧切台: 1 - 静帧切台；0 - 黑屏切台.
#define PLAYER_CONFIG_VIDEO_FREEZE_BUFFER           "Video>freeze_buffer"           ///< 视频静帧切台内存大小,单位 Byte.
#define PLAYER_CONFIG_VIDEO_DEC_TIMEOUT             "Video>dec_timeout"             ///< 视频解码超时时长.
#define PLAYER_CONFIG_VIDEO_DEC_HDR_ENABLE          "Video>hdr_enable"               ///< 视频是否则支持HDR
#define PLAYER_CONFIG_VIDEO_DEC_CC_ENABLE           "Video>cc_enable"               ///< 视频是否可输出 ATSC-CC 数据.
#define PLAYER_CONFIG_VIDEO_DEC_CC_DISPLAY          "Video>cc_display"              ///< 视频 ATSC-CC 数据是否硬件处理: 1 - 硬件处理; 0 - 软件处理.
#define PLAYER_CONFIG_VIDEO_DEC_USERDATA_LENGTH     "Video>userdata_length"         ///< 视频存储 userdata 的 FIFO 大小.
#define PLAYER_CONFIG_VIDEO_DEC_MOSAIC_DROP         "Video>mosaic_drop"             ///< 取值 0 ~ 0xFFFFFFFF. 0:关闭马赛克屏蔽功能，所有马赛克帧都播出。非0: 如果出现连续 N 帧马赛克帧，则强制播出一帧, 防止画面一直不动。
#define PLAYER_CONFIG_VIDEO_DEC_MOSAIC_GATE         "Video>mosaic_gate"             ///< 视频误码率达到该门限,被认为马赛克帧: 门限取值范围: 0 ~ 100.
#define PLAYER_CONFIG_VIDEO_DEC_SYNC_FLAG           "Video>sync_flag"               ///< 视频是否同步输出.
#define PLAYER_CONFIG_VIDEO_DEC_SYNC_TIMEMS         "Video>sync_timems"             ///< 视频同步门限,低于此门限可以视频可以正常输出.
#define PLAYER_CONFIG_VIDEO_DEC_SYNC_HIGH_TIMEMS    "Video>sync_high_timems"        ///< 视频同步门限,高于此门限可以视频不做同步输出.
#define PLAYER_CONFIG_VIDEO_SUPPORT_DEINTERLACE     "Video>support_deinterlace"     ///< 视频是否开启去隔行.
#define PLAYER_CONFIG_VIDEO_SUPPORT_PPZOOM          "Video>support_ppzoom"          ///< 视频是否开启 PP 缩放.
#define PLAYER_CONFIG_VIDEO_SUPPORT_10BIT           "Video>support_10bit"           ///< 视频是否支持 H265 10Bit 解码.
#define PLAYER_CONFIG_VIDEO_AUTO_ADAPT              "Video>auto_adapt"              ///< 高清通路输出制式跟随当前播放节目帧率自适应.
#define PLAYER_CONFIG_VIDEO_AUTO_ADAPT1             "Video>auto_adapt1"             ///< 标清通路输出制式跟随当前高清通路制式自适应.
#define PLAYER_CONFIG_VIDEO_SHARPNESS_EN            "Video>sharpness_en"            ///< 视频输出清晰度.
#define PLAYER_CONFIG_VIDEO_HUE_EN                  "Video>hue_en"                  ///< 视频输出色调.
#define PLAYER_CONFIG_VIDEO_AUTO_PAL                "Video>auto_pal"                ///< 视频输出 PAL 制式.
#define PLAYER_CONFIG_VIDEO_AUTO_NTSC               "Video>auto_ntsc"               ///< 视频输出 NTSC 制式.
#define PLAYER_CONFIG_VIDEO_OUT_DELAY               "Video>vout_delay"              ///< 视频延时输出.
#define PLAYER_CONFIG_HDMI_DELAY                    "Video>hdmi_delay"              ///< 视频 HDMI 接口延时输出.
#define PLAYER_CONFIG_VIDEO_PP_MAX_WIDTH            "Video>pp_max_width"            ///< 视频开启 PP 操作,支持最大分辨率的宽.
#define PLAYER_CONFIG_VIDEO_PP_MAX_HEIGHT           "Video>pp_max_height"           ///< 视频开启 PP 操作,支持最大分辨率的高.
#define PLAYER_CONFIG_VIDEO_FB_MAX_SIZE             "Video>fb_max_size"             ///< 视频帧存最大大小,单位 Byte.
#define PLAYER_CONFIG_VIDEO_EXTRA_FB                "Video>extra_fb"                ///< 标识针对哪些编码需要多分配一帧，默认值 0xFFFFFFFF, 表示所有编码都要多申请一帧. gxdocref VideoCodecType.
#define PLAYER_CONFIG_VIDEO_SPEED_MODE              "Video>speed_mode"              ///< 视频解码速度模式.

#define PLAYER_CONFIG_AUDIO_ANTI_ERROR_CODE         "Audio>anti_error_code"         ///< 音频解码抗误码级别: 0 - 不开启；1 - 级别1; 2 - 级别2; 3 - 防水印.
#define PLAYER_CONFIG_AUDIO_DMX_FULL_GATE           "Audio>dmx_full_gate"           ///< 音频解码 ESA 满门限.
#define PLAYER_CONFIG_AUDIO_INTERFACE               "Audio>interface"               ///< 音频输出接口,定义详见: gxdocref AudioOutputInterface.
#define PLAYER_CONFIG_AUDIO_SPDIF_SOURCE            "Audio>spd_source"              ///< 音频 SPDIF 输出模式数据.
#define PLAYER_CONFIG_AUDIO_HDMI_SOURCE             "Audio>hdmi_source"             ///< 音频 HDMI 输出模式数据.
#define PLAYER_CONFIG_AUDIO_VOLUME_PRIV             "Audio>volume_priv"             ///< 音频输出音量是否使用Play接口传入的音量
#define PLAYER_CONFIG_AUDIO_VOLUME_TABLE            "Audio>volume_table"            ///< 音频输出用户设置音量表.
#define PLAYER_CONFIG_AUDIO_VOLUME                  "Audio>volume"                  ///< 音频输出音量值,取值范围: 0 ~ 100.
#define PLAYER_CONFIG_AUDIO_TRACK                   "Audio>track"                   ///< 音频输出声道, 详见: gxdocref AudioTrack.
#define PLAYER_CONFIG_AUDIO_AC3_MODE                "Audio>ac3mode"                 ///< 音频 DOLBY / DOLBY-PLUS 解码模式,详见: gxdocref AudioOutputDataType.
#define PLAYER_CONFIG_AUDIO_AC3_ENABLE              "Audio>ac3enable"               ///< 音频 DOLBY / DOLBY-PLUS 数据处理是否开启.
#define PLAYER_CONFIG_AUDIO_AAC_MODE                "Audio>aacmode"                 ///< 音频 MPEG4-AAC 解码模式,详见: gxdocref AudioAACMode.
#define PLAYER_CONFIG_AUDIO_AAC_ENABLE              "Audio>aacenable"               ///< 音频 MPEG4-AAC 数据处理是否开启.
#define PLAYER_CONFIG_AUDIO_DTS_ENABLE              "Audio>dtsenable"               ///< 音频 DTS 数据处理是否开启.
#define PLAYER_CONFIG_AUDIO_AD_ENABLE               "Audio>adenable"                ///< 音频 AD 功能是否开启.
#define PLAYER_CONFIG_AUDIO_DOWNMIX                 "Audio>downmix"                 ///< 音频是否开启 DOWNMIX 功能.
#define PLAYER_CONFIG_AUDIO_SYNC_FLAG               "Audio>sync_flag"               ///< 音频是否同步输出.
#define PLAYER_CONFIG_AUDIO_SYNC_TIMEMS             "Audio>sync_timems"             ///< 音频同步门限,低于此门限音频可以正常输出.
#define PLAYER_CONFIG_AUDIO_SYNC_HIGH_TIMEMS        "Audio>sync_high_timems"        ///< 音频同步门限,高于此门限音频不做同步输出.
#define PLAYER_CONFIG_AUDIO_MUTE                    "Audio>mute"                    ///< 音频是否静音输出: 1 - 静音；0 - 非静音.
#define PLAYER_CONFIG_AUDIO_SPEED_MUTE              "Audio>speed_mute"              ///< 音频快播是否静音,无效功能.
#define PLAYER_CONFIG_AUDIO_ESA_SIZE                "Audio>esa_size"                ///< 音频解码 ESA 大小, 需 1KByte 对齐.
#define PLAYER_CONFIG_AUDIO_PCM_SIZE                "Audio>pcm_size"                ///< 音频解码 PCM 大小, 需 1KByte 对齐.
#define PLAYER_CONFIG_AUDIO_BOOST_VOLUME            "Audio>boost_volume"            ///< 音频输出主路音量值.
#define PLAYER_CONFIG_AUDIO_DESCRIPTOR_BOOST_VOLUME "Audio>descriptor_boost_volume" ///< 音频输出此路音量值.
#define PLAYER_CONFIG_AUDIO_VOL_RIGHT_NOW           "Audio>right_now"               ///< 音频输出音量是否立即变化: 1 - 立即变化；0 - 缓慢变化.
#define PLAYER_CONFIG_AUDIO_VOL_STEP                "Audio>step"                    ///< 音频输出音量缓慢变化步长.

#define PLAYER_CONFIG_SUTITLE_ENABLE_SCTE           "Subtitle>enable_scte"          ///< 是否开启 SCTE 字幕.

#define PLAYER_CONFIG_PVR_TIME_SHIFT_FLAG           "PVR>shift_flag"                ///< 是否开启 PVR 时移.
#define PLAYER_CONFIG_PVR_TIME_SHIFT_FILE           "PVR>shift_file"                ///< PVR 时移的储存文件.
#define PLAYER_CONFIG_PVR_TIME_SHIFT_DMXID          "PVR>shift_dmxid"               ///< PVR 时移的硬件解复用 ID.
#define PLAYER_CONFIG_PVR_PLAYBACK_FORCE_HWTS       "PVR>playback_force_hwts"       ///< PVR 回放是否强制使用硬件解复用.
#define PLAYER_CONFIG_PVR_PLAYBACK_HWTS_SYNCMODE    "PVR>playback_hwts_syncmode"    ///< PVR HWTS 回放时 STC 的恢复模式.  取值参考 GxSTCProperty_Config, 默认:PURE_APTS_RECOVER
#define PLAYER_CONFIG_PVR_RECORD_VOLUME_SIZE        "PVR>record_volume_sizemb"      ///< PVR 每卷录制默认大小.
#define PLAYER_CONFIG_PVR_RECORD_VOLUME_MAX         "PVR>record_volume_maxnum"      ///< PVR 每卷录制默认个数.
#define PLAYER_CONFIG_PVR_RECORD_VOLUME_FULLSTOP    "PVR>record_volume_fullstop"    ///< PVR 录制满是否结束录制: 1 - 结束录制；0 - 循环录制.
#define PLAYER_CONFIG_PVR_RECORD_RESERVE_SIZE       "PVR>record_reserve_sizemb"     ///< PVR 录制预留空间大小,单位 Byte.
#define PLAYER_CONFIG_PVR_PLAY_DEMUX_ID             "PVR>play_dmxid"                ///< PVR 回放的硬件解复用 ID.

#define PLAYER_CONFIG_VOUT_HDCP_ON                  "Hdmi>vout_hdcp_on"             ///< 是否开启 HDMI 的 HDCP 保护功能.
#define PLAYER_CONFIG_VOUT_MACROVISION_ON           "Cvbs>vout_macrovision_on"      ///< 是否开启 CVBS 的 MACROVISION 功能.
__END_DECLS

#endif

/** @}*/
