# 编程指南

- 使用 player 模块前，必须按照本章描述的顺序对模块进行初始化。

- 初始化的一般流程如下：

## 模块注册
- 如果不太关心 Flash/DDR 消耗，建议选择[全功能注册](#全功能注册)。
- 否则，按需注册。
### 全功能注册
* \ref GxPlayer_ModuleRegisterALL_V2
### DVB 相关
* \ref GxPlayer_ModuleRegisterStreamDVB
* \ref GxPlayer_ModuleRegisterDVBSourceNormal
* \ref GxPlayer_ModuleRegisterDVBSourceTSBuff
* \ref GxPlayer_ModuleRegisterDVBSourceTSCache
* \ref GxPlayer_ModuleRegisterDemuxerHWTS
### 多媒体相关
* \ref GxPlayer_ModuleRegisterStreamFILE
* \ref GxPlayer_ModuleRegisterStreamFILE_V2
* \ref GxPlayer_ModuleRegisterStreamMEM
* \ref GxPlayer_ModuleRegisterStreamFIFO
* \ref GxPlayer_ModuleRegisterStreamRINGMEM
* \ref GxPlayer_ModuleRegisterDemuxerMP3
* \ref GxPlayer_ModuleRegisterDemuxerMP4
* \ref GxPlayer_ModuleRegisterDemuxerFLAC
* \ref GxPlayer_ModuleRegisterDemuxerWAV
* \ref GxPlayer_ModuleRegisterDemuxerAVI
* \ref GxPlayer_ModuleRegisterDemuxerMKV
* \ref GxPlayer_ModuleRegisterDemuxerFLV
* \ref GxPlayer_ModuleRegisterDemuxerAAC
* \ref GxPlayer_ModuleRegisterDemuxerES
* \ref GxPlayer_ModuleRegisterDemuxerLOGO
* \ref GxPlayer_ModuleRegisterDemuxerSWTS
* \ref GxPlayer_ModuleRegisterDemuxerHWTS
* \ref GxPlayer_ModuleRegisterDemuxerREAL
* \ref GxPlayer_ModuleRegisterDemuxerPS
* \ref GxPlayer_ModuleRegisterDemuxerOGG
* \ref GxPlayer_ModuleRegisterDemuxerDOLBY
* \ref GxPlayer_ModuleRegisterDemuxerDTS
* \ref GxPlayer_ModuleRegisterDemuxerAPE
* \ref GxPlayer_ModuleRegisterDecoderSWAPE
### 网络相关
* \ref GxPlayer_ModuleRegisterStreamHTTP_V2
* \ref GxPlayer_ModuleRegisterStreamRTSP_V2
* \ref GxPlayer_ModuleRegisterStreamRTMP_V2
* \ref GxPlayer_ModuleRegisterStreamUDP_V2
* \ref GxPlayer_ModuleRegisterStreamRTP_V2
* \ref GxPlayer_ModuleRegisterStreamRTPSDP_V2
* \ref GxPlayer_ModuleRegisterStreamM3U8_V2
* \ref GxPlayer_ModuleRegisterStreamMPEGDASH_V2
* \ref GxPlayer_ModuleRegisterStreamSWCRYPTO_V2
* \ref GxPlayer_ModuleRegisterStreamConcat_V2
* \ref GxPlayer_ModuleRegisterStreamCompose_V2
* \ref GxPlayer_ModuleRegisterStreamSRTP_V2
* \ref GxPlayer_ModuleRegisterStreamMMS
* \ref GxPlayer_ModuleRegisterStreamData
* \ref GxPlayer_ModuleRegisterStreamSWCRYPTO
* \ref GxPlayer_ModuleRegisterStreamHWCRYPTO
* \ref GxPlayer_ModuleRegisterStreamDRMCRYPTO
### 录制相关
* \ref GxPlayer_ModuleRegisterStreamDVB
* \ref GxPlayer_ModuleRegisterDVBSourceNormal
* \ref GxPlayer_ModuleRegisterDemuxerHWTS
* \ref GxPlayer_ModuleRegisterStreamPVR
* \ref GxPlayer_ModuleRegisterMuxerRAWVIDEO
* \ref GxPlayer_ModuleRegisterMuxerMPEGTS
* \ref GxPlayer_ModuleRegisterRecorderDVB
* \ref GxPlayer_ModuleRegisterRecorderVMX
### 字幕相关
* \ref GxPlayer_ModuleRegisterStreamDVB
* \ref GxPlayer_ModuleRegisterDVBSourceNormal
* \ref GxPlayer_ModuleRegisterSubtitleALL
* \ref GxPlayer_ModuleRegisterSubtitleDVB
* \ref GxPlayer_ModuleRegisterSubtitleScte
* \ref GxPlayer_ModuleRegisterSubtitleAribCC
* \ref GxPlayer_ModuleRegisterSubtitleAtscCC
* \ref GxPlayer_ModuleRegisterSubtitleInside
* \ref GxPlayer_ModuleRegisterSubtitleOutside
* \ref GxPlayer_ModuleRegisterSubtitleTeletext
### FM 相关
* \ref GxPlayer_ModuleRegisterStreamFM
* \ref GxPlayer_ModuleRegisterDemuxerFM

## 回调注册

- 事件回调注册 【**可选**】
  - \ref GxPlayer_SetEventCallback
  - 默认情况下，Player 的事件是发送到 GxBus 总线的；注册了这个回调后，事件会通过这个回调函数发送给用户。

## 模块配置

- 按需修改 GxPlayer 模块的默认配置 【**可选**】
  - \ref GxBUS_ConfigSet
  - \ref GxBUS_ConfigSetInt
- 配置项详见: gxplayer_define.h 中 PLAYER_CONFIG_xxxx

- \ref PLAYER_CONFIG_PLAY_RESERVED_MEMSIZE
- \ref PLAYER_CONFIG_PLAY_RESERVED_HOLESIZE
- \ref PLAYER_CONFIG_INDEX_RESERVED_MEMSIZE
- \ref PLAYER_CONFIG_RECORD_CACHED_MEMSIZE
- \ref PLAYER_CONFIG_RECORD_TSW_MEMSIZE
- \ref PLAYER_CONFIG_NETWORK_CACHED_MEMSIZE
- \ref PLAYER_CONFIG_NETWORK_CACHED_SEEK_SIZE
- \ref PLAYER_CONFIG_NETWORK_TIMEOUT
- \ref PLAYER_CONFIG_NETWORK_RECVBUF
- \ref PLAYER_CONFIG_NETWORK_HLS_MULIPLE_LIST
- \ref PLAYER_CONFIG_NETWORK_FFHLS_MAX_PLAYLIST
- \ref PLAYER_CONFIG_NETWORK_HTTP_PERSISTENT_CONNECTIONS
- \ref PLAYER_CONFIG_NETWORK_DEFAULT_BANDWIDTH_INDEX
- \ref PLAYER_CONFIG_NETWORK_IS_FALSE_LIVE
- \ref PLAYER_CONFIG_DELAY_CACHE_SIZE
- \ref PLAYER_CONFIG_DELAY_CACHE_HW_SIZE
- \ref PLAYER_CONFIG_DELAY_CACHE_TO_MEMORY
- \ref PLAYER_CONFIG_FLAG_ERROR_STOP
- \ref PLAYER_CONFIG_FLAG_SOF_REPLAY
- \ref PLAYER_CONFIG_STREAM_BUFFER_SIZE

- \ref PLAYER_CONFIG_AUTOPLAY_VIDEO_NAME
- \ref PLAYER_CONFIG_AUTOPLAY_VIDEO_URL
- \ref PLAYER_CONFIG_AUTOPLAY_RADIO_NAME
- \ref PLAYER_CONFIG_AUTOPLAY_RADIO_URL
- \ref PLAYER_CONFIG_AUTOPLAY_RADIO_FLAG
- \ref PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG

- \ref PLAYER_CONFIG_VIDEO_INTERFACE
- \ref PLAYER_CONFIG_VIDEO_RESOLUTION_HDMI
- \ref PLAYER_CONFIG_VIDEO_RESOLUTION_YUV
- \ref PLAYER_CONFIG_VIDEO_RESOLUTION_RCA
- \ref PLAYER_CONFIG_VIDEO_RESOLUTION_RCA1
- \ref PLAYER_CONFIG_VIDEO_RESOLUTION_SCART
- \ref PLAYER_CONFIG_VIDEO_RESOLUTION_SVIDEO
- \ref PLAYER_CONFIG_VIDEO_BRIGHTNESS_HDMI
- \ref PLAYER_CONFIG_VIDEO_BRIGHTNESS_YUV
- \ref PLAYER_CONFIG_VIDEO_BRIGHTNESS_RCA
- \ref PLAYER_CONFIG_VIDEO_BRIGHTNESS_RCA1
- \ref PLAYER_CONFIG_VIDEO_BRIGHTNESS_SCART
- \ref PLAYER_CONFIG_VIDEO_BRIGHTNESS_SVIDEO
- \ref PLAYER_CONFIG_VIDEO_SATURATION_HDMI
- \ref PLAYER_CONFIG_VIDEO_SATURATION_YUV
- \ref PLAYER_CONFIG_VIDEO_SATURATION_RCA
- \ref PLAYER_CONFIG_VIDEO_SATURATION_RCA1
- \ref PLAYER_CONFIG_VIDEO_SATURATION_SCART
- \ref PLAYER_CONFIG_VIDEO_SATURATION_SVIDEO
- \ref PLAYER_CONFIG_VIDEO_CONTRAST_HDMI
- \ref PLAYER_CONFIG_VIDEO_CONTRAST_YUV
- \ref PLAYER_CONFIG_VIDEO_CONTRAST_RCA
- \ref PLAYER_CONFIG_VIDEO_CONTRAST_RCA1
- \ref PLAYER_CONFIG_VIDEO_CONTRAST_SCART
- \ref PLAYER_CONFIG_VIDEO_CONTRAST_SVIDEO

- \ref PLAYER_CONFIG_VIDEO_ESV_SIZE
- \ref PLAYER_CONFIG_VIDEO_DISPLAY_SCREEN
- \ref PLAYER_CONFIG_VIDEO_SCREEN_XRES
- \ref PLAYER_CONFIG_VIDEO_SCREEN_YRES
- \ref PLAYER_CONFIG_VIDEO_ASPECT
- \ref PLAYER_CONFIG_VIDEO_FREEZE_SWITCH
- \ref PLAYER_CONFIG_VIDEO_FREEZE_BUFFER
- \ref PLAYER_CONFIG_VIDEO_DEC_TIMEOUT
- \ref PLAYER_CONFIG_VIDEO_DEC_USERDATA_ENABLE
- \ref PLAYER_CONFIG_VIDEO_DEC_USERDATA_LENGTH
- \ref PLAYER_CONFIG_VIDEO_DEC_USERDATA_DISPLAY
- \ref PLAYER_CONFIG_VIDEO_DEC_MOSAIC_DROP
- \ref PLAYER_CONFIG_VIDEO_DEC_MOSAIC_GATE
- \ref PLAYER_CONFIG_VIDEO_DEC_SYNC_FLAG
- \ref PLAYER_CONFIG_VIDEO_DEC_SYNC_TIMEMS
- \ref PLAYER_CONFIG_VIDEO_SUPPORT_DEINTERLACE
- \ref PLAYER_CONFIG_VIDEO_SUPPORT_PPZOOM
- \ref PLAYER_CONFIG_VIDEO_SUPPORT_10BIT
- \ref PLAYER_CONFIG_VIDEO_AUTO_ADAPT
- \ref PLAYER_CONFIG_VIDEO_AUTO_ADAPT1
- \ref PLAYER_CONFIG_VIDEO_SHARPNESS_EN
- \ref PLAYER_CONFIG_VIDEO_HUE_EN
- \ref PLAYER_CONFIG_VIDEO_AUTO_PAL
- \ref PLAYER_CONFIG_VIDEO_AUTO_NTSC
- \ref PLAYER_CONFIG_VIDEO_OUT_DELAY
- \ref PLAYER_CONFIG_HDMI_DELAY
- \ref PLAYER_CONFIG_VIDEO_PP_MAX_WIDTH
- \ref PLAYER_CONFIG_VIDEO_PP_MAX_HEIGHT
- \ref PLAYER_CONFIG_VIDEO_FB_MAX_SIZE
- \ref PLAYER_CONFIG_VIDEO_EXTRA_FB_NUM

- \ref PLAYER_CONFIG_AUDIO_ANTI_ERROR_CODE
- \ref PLAYER_CONFIG_AUDIO_DMX_FULL_GATE
- \ref PLAYER_CONFIG_AUDIO_INTERFACE
- \ref PLAYER_CONFIG_AUDIO_SPDIF_SOURCE
- \ref PLAYER_CONFIG_AUDIO_HDMI_SOURCE
- \ref PLAYER_CONFIG_AUDIO_VOLUME_PRIV
- \ref PLAYER_CONFIG_AUDIO_VOLUME_TABLE
- \ref PLAYER_CONFIG_AUDIO_VOLUME
- \ref PLAYER_CONFIG_AUDIO_TRACK
- \ref PLAYER_CONFIG_AUDIO_AC3_MODE
- \ref PLAYER_CONFIG_AUDIO_AC3_ENABLE
- \ref PLAYER_CONFIG_AUDIO_AAC_MODE
- \ref PLAYER_CONFIG_AUDIO_AAC_ENABLE
- \ref PLAYER_CONFIG_AUDIO_DTS_ENABLE
- \ref PLAYER_CONFIG_AUDIO_DOWNMIX
- \ref PLAYER_CONFIG_AUDIO_SYNC_FLAG
- \ref PLAYER_CONFIG_AUDIO_SYNC_TIMEMS
- \ref PLAYER_CONFIG_AUDIO_MUTE
- \ref PLAYER_CONFIG_AUDIO_SPEED_MUTE
- \ref PLAYER_CONFIG_AUDIO_ESA_SIZE
- \ref PLAYER_CONFIG_AUDIO_PCM_SIZE
- \ref PLAYER_CONFIG_AUDIO_BOOST_VOLUME
- \ref PLAYER_CONFIG_AUDIO_DESCRIPTOR_BOOST_VOLUME
- \ref PLAYER_CONFIG_AUDIO_VOL_RIGHT_NOW
- \ref PLAYER_CONFIG_AUDIO_VOL_STEP

- \ref PLAYER_CONFIG_SUTITLE_ENABLE_SCTE

- \ref PLAYER_CONFIG_PVR_TIME_SHIFT_FLAG
- \ref PLAYER_CONFIG_PVR_TIME_SHIFT_FILE
- \ref PLAYER_CONFIG_PVR_TIME_SHIFT_DMXID
- \ref PLAYER_CONFIG_PVR_PLAYBACK_FORCE_HWTS
- \ref PLAYER_CONFIG_PVR_PLAYBACK_HWTS_SYNCMODE
- \ref PLAYER_CONFIG_PVR_RECORD_VOLUME_SIZE
- \ref PLAYER_CONFIG_PVR_RECORD_VOLUME_MAX
- \ref PLAYER_CONFIG_PVR_RECORD_VOLUME_FULLSTOP
- \ref PLAYER_CONFIG_PVR_RECORD_RESERVE_SIZE
- \ref PLAYER_CONFIG_PVR_PLAY_DEMUX_ID

- \ref PLAYER_CONFIG_ETS_FILE_SHADOW_TS
- \ref PLAYER_CONFIG_ETS_FILE_ENCRYPT_ENABLE
- \ref PLAYER_CONFIG_ETS_FILE_ENCRYPT_ALGO
- \ref PLAYER_CONFIG_ETS_FILE_ENCRYPT_LEN
- \ref PLAYER_CONFIG_ETS_FILE_ENCRYPT_KEY
- \ref PLAYER_CONFIG_ETS_FILE_BLOCK_SIZE
- \ref PLAYER_CONFIG_ETS_FILE_BLOCK_COUNT_DEC
- \ref PLAYER_CONFIG_ETS_FILE_BLOCK_COUNT_ENC

- \ref PLAYER_CONFIG_VOUT_HDCP_ON
- \ref PLAYER_CONFIG_VOUT_MACROVISION_ON

## 模块加载

- 使用 GxPlayer 模块
  - \ref GxPlayerModuleInit

## 模块设置

- 配置视频输出 【**必选**】
  - \ref GxPlayer_SetVideoOutputSelect
  - \ref GxPlayer_GetVideoOutputConfig
- 其他设置【可选】
  - 参考: [音频设置](./feature/audio.md)，[视频设置](./feature/video.md)

