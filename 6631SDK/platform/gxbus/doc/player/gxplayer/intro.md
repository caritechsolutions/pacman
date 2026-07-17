# 模块概述

GxPlayer 是基于 [GxAVDEV 驱动](../../../gxavdev/intro.md) 封装的 API 接口。对外提供 gxplayer_module.h 头文件，安装到 GXLIBPATH/include/bus/module/player/。提供 libgxplayer.a 库，安装到GXLIBPATH/lib。

> **注意**：使用 API 之前需要确认驱动已经加载成功，并且 /dev/ 下生成了正确的设备结点文件，具体参考 [驱动加载](../../../gxavdev/intro.md)。在使用本模块之前你需要了解[背景知识](../../../gxavdev/basis.md)。

[GxPlayer](./feature.md): 除了音视频的播放功能外,  还包括 PVR、AD、Subtitle、音视频的输出控制等功能。

- [播放功能](./feature/play.md): 本地文件、IPTV、DVB 等媒体的播放。
- [录制功能](2-1.录制.md): DVB、IPTV 等媒体的录制功能。
- [时移功能](2-3.时移.md): DVB、IPTV 等媒体的时移功能。
- [字幕功能](./feature/subtitle.md): 各种形式的字幕相关的功能, 包含 DVB、ATST、CC 以及 其他常见的媒体内嵌或外挂字幕。
- [AD 功能 ](./feature/ad.md): AudioDescription 相关的功能。
- [音频控制](./feature/audio.md): 音频播放、输出等相关的设置。
- [视频控制](./feature/video.md): 视频播放、输出等相关的设置。
- [信息获取](./feature/info.md): 系统、媒体相关的信息获取的功能。
- [其他功能](./feature/other.md): 其他杂项功能。

> **注意**：使用模块功能之前需要确认模块已经初始化成功，具体参考 [系统初始化流程](./progguide.md)。

