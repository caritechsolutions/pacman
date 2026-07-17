# 音频控制功能

## 概述
  - [音频背景知识](../../background/background.md#音频)
    - 音频可以分成[音频解码](#音频解码)、[音频播放](#音频播放)两大模块.
    - 音频解码: 将压缩的编码数据解码成PCM数据,包括PCM过滤,AC3/DTS过滤,AAC转码AC3等功能.
    - 音频播放: 将PCM/AC3/DTS等数据传输给相应端口,同时可以控制音量，声道等功能.
    - AD功能: 在主路播放基础上,开关次路音频.

## 音频解复用
  - 音频容器过滤成解码的es流数据

| 功能                         | 接口                                       | 说明                            |
| ---------------------------- | ------------------------------------------ | ------------------------------- |
| 注册MP3编码对应容器解复用    | GxPlayer_ModuleRegisterDemuxerMP3          | 一般用于单路MP3编码文件         |
| 注册OGG容器解复用            | GxPlayer_ModuleRegisterDemuxerOGG          | 一般用于单路VORBIS编码文件      |
| 注册AAC容器解复用            | GxPlayer_ModuleRegisterDemuxerAAC          | 一般用于单路AAC编码文件         |
| 注册AC3/EAC3容器解复用       | GxPlayer_ModuleRegisterDemuxerDOLBY        | 一般用于单路AC3/EAC3编码文件    |
| 注册DTS容器解复用            | GxPlayer_ModuleRegisterDemuxerDTS          | 一般用于单路DTS编码文件         |
| 注册APE容器解复用            | GxPlayer_ModuleRegisterDemuxerAPE          | 一般用于单路APE编码文件         |

## 音频解码
  - 使用Audio DSP进行音频解码,需要注册相应的音频固件.

| 方案  | 芯片      | 支持的编码音频解码                                           | 接口                          |
| ----- | --------- | ------------------------------------------------------------ | ----------------------------- |
| ecos  | NNE芯片   | 解码: MPEG12A,MPEG4-AAC,DRA,OPUS,VORBIS,FLAC,SBC.                                                     过滤: PCM,AAC,AC3,EAC3,DTS.                                                               转码AC3: AAC.                                                                                          双路解码:MPEG12A,MPEG4-AAC. | CODEC_AUDIO(chip)             |
| ecos  | 非NNE芯片 | 解码: MPEG12A,MPEG4-AAC.                                                                 过滤: PCM,AAC,AC3,EAC3,DTS.                                                              转码AC3: AAC.                                                                                          双路解码:MPEG12A,MPEG4-AAC. | CODEC_MPEGA(chip)             |
| ecos  | 非NNE芯片 | 解码: MPEG12A,MPEG4-AAC.                                                                 过滤: PCM,AAC,AC3,EAC3,DTS. | CODEC_MPEGA_MINI(chip)        |
| ecos  | 非NNE芯片 | 解码: DRA.                                                   | CODEC_DRA(chip)               |
| ecos  | 非NNE芯片 | 解码: FLAC,OPUS,VORBIS.                                      | CODEC_FOV(chip)               |
| ecos  | 非NNE芯片 | 解码: FLAC.                                                  | CODEC_FLAC(chip)              |
| ecos  | 非NNE芯片 | 解码: OPUS.                                                  | CODEC_OPUS(chip)              |
| ecos  | 非NNE芯片 | 解码: VORBIS.                                                | CODEC_VORBIS(chip)            |
| ecos  | 非NNE芯片 | 解码: SBC.                                                   | CODEC_SBC(chip)               |
| linux | NNE芯片   | 解码: MPEG12A,MPEG4-AAC,DRA,OPUS,VORBIS,FLAC,SBC.                                                     过滤: PCM,AAC,AC3,EAC3,DTS.                                                               转码AC3: AAC.                                                                                          双路解码:MPEG12A,MPEG4-AAC. | gxavdevb编译自动注册ALL固件.  |
| linux | 非NNE芯片 | 解码: MPEG12A,MPEG4-AAC,DRA,OPUS,VORBIS,FLAC,SBC.                                                     过滤: PCM,AAC,AC3,EAC3,DTS.                                                               转码AC3: AAC.                                                                                          双路解码:MPEG12A,MPEG4-AAC. | gxavdevb编译自动注册单独固件. |

  - 特性
    - [MPEG12A](../../background/background.md#MPEG12A): 支持解码.
    - [MPEG4-AAC](../../background/background.md#MPEG4-AAC): 支持解码,过滤,转码AC3.
    - [AC3/EAC3](../../background/background.md#DOLBY): 支持解码,过滤.解码需要授权芯片支持解码(-D芯片),非授权芯片只支持过滤.
    - [DRA](../../background/background.md#DRA): 支持解码.
    - [FLAC](../../background/background.md#FLAC): 支持解码.
    - [OPUS](../../background/background.md#OPUS): 支持解码.
    - [VORBIS](../../background/background.md#VORBIS): 支持解码.
    - [DTS](../../background/background.md#DTS): 只支持过滤.

  - 配置

| 功能                         | 接口                                       |
| ---------------------------- | ------------------------------------------ |
| 注册音频解码器               | [**GxAudioRegisterDecoder**](gxdocref)                 |
| 配置音频编码AC3/EAC3工作模式 | [**GxPlayer_SetAudioAC3Mode**](gxdocref)               |
| 配置音频编码AAC工作模式      | [**GxPlayer_SetAudioAACMode**](gxdocref)               |
| 配置不处理的音频编码         | [**GxPlayer_AudioCodecMask**](gxdocref)                |

  - 控制

| 功能                         | 接口                                           |
| ---------------------------- | ---------------------------------------------- |
| 音频切换音轨                 | [**GxPlayer_MediaAudioSwitch**](gxdocref)                  |

## 音频AD
  - 全称audio description,音频的次路音频需要和主路音频有相同的编码以及采样率.

  - 控制

| 功能               | 接口                                       |
| ------------------ | ------------------------------------------ |
| 设置次路音频声道   | [**GxPlayer_SetAudioDescriptorTrack**]()       |
| 设置次路音频音量   | [**GxPlayer_SetAudioDescriptorBoostVolume**]() |
| 开启次路音频       | [**GxPlayer_MediaAdAudioEnable**]()            |
| 关闭次路音频       | [**GxPlayer_MediaAdAudioDisable**]()           | 
| 设置主次路音频混合 | [**GxPlayer_SetAudioDownMix**]()               |

## 音频播放
  - 特性
    - PCM
      - 支持16kHz,22.05kHz,24kHz,32kHz,44.1kHz,48kHz的采样率播放.
      - 支持最大7.1声道.
      - 支持两路PCM数据混合,要求相同采样率.
      - 支持静音,音量,声道控制.
      - 内置DAC.
    - SPDIF
      - 支持AC3/EAC3播放.
      - 支持DTS播放,仅限于sirius/taurus/gemini芯片.
      - 支持AAC播放.
      - 支持两声道PCM播放.
  - 配置

| 功能                         | 接口                                       |
| ---------------------------- | ------------------------------------------ |
| 选择音频输出端口             | [**GxPlayer_SetAudioOutputSelect**](gxdocref)          |
| 配置音频输出端口输出数据类型 | [**GxPlayer_SetAudioOutputConfig**](gxdocref)          |
| 配置音频同步开关             | [**GxPlayer_SetAudioSyncEnable**](gxdocref)            |
| 配置音频为STC时钟            | [**GxPlayer_SetAudioPureRecoveryMode**](gxdocref)      |
| 配置音频提前/延时播放        | [**GxPlayer_MediaAudioDelayMs**](gxdocref)             |
| 配置HWTS音频pts延时          | [**GxPlayer_MediaAudioSync**](gxdocref)                |

  - 控制

| 功能                         | 接口                                       |
| ---------------------------- | ------------------------------------------ |
| 设置音量                     | [**GxPlayer_SetAudioVolume**](gxdocref)                |
| 设置静音                     | [**GxPlayer_SetAudioMute**](gxdocref)                  |
| 切换声道                     | [**GxPlayer_SetAudioTrack**](gxdocref)                 |
| 进入低功耗时防爆音           | [**GxPlayer_SetAudioPowerMute**](gxdocref)             |
| 不播放音频时HDMI输出音频     | [**GxPlayer_SetAudioPort**](gxdocref)                  |
| 开关接口输出                 | [**GxPlayer_TurnAudioPort**](gxdocref)                 |

  - 获取

| 功能                         | 接口                                       |
| ---------------------------- | ------------------------------------------ |
| 获取音量                     | [**GxPlayer_GetAudioVolume**](gxdocref)                |

