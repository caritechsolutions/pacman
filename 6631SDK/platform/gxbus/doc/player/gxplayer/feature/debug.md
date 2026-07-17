# 调试指南

  > 利用播放器的调试手段, 加快分析定位问题, 从而解决问题.

## 调试手段

  - [编译调试](#编译调试):

    > 编译时, 通过打优化级别和印级别编译来控制打印来分析定位问题.

    > 通常情况下, 用于分析调用流程是否合理.

  - [功能调试](#功能调试):

    > 应用调用调试接口, 开启或输出打印来分析定位问题.

    > 通常情况下, 用于分析模块是否正常运行.

  - [内存调试](#内存调试):

    > 播放器内存异常时, 通过调试接口, 可以定位内存异常的原因.

    > 通常情况下, 用于检查播放器内存是否泄露, 越界等异常等行为.

  - [其他调试](#其他调试):

    > 除编译调试、功能调试、内存调试之外, 分析定位问题的手段汇总.

## 编译调试

### 优化级别编译

  - 示例:

    > optimize=O0 ./build csky ecos

    > optimize=O2 ./build csky ecos

    > optimize=Os ./build csky ecos

  - 注意:

    > 没有编译选项时, 默认为 optimize=O2 编译.

| 选项              | 说明                                         |
| ----------------  | -------------------------------------------- |
| optimize=O0       | 开启播放器内存调试, gdb调试, 统计切台时间等  |
| optimize=O2       | 开启播放器统计切台时间, gdb调试等            |
| optimize=Os       | 不开启调试信息                               |

### 打印级别编译

  - 示例:

    > loglevel=0 ./build csky ecos

    > loglevel=1 ./build csky ecos

    > loglevel=2 ./build csky ecos

    > loglevel=3 ./build csky ecos

    > loglevel=4 ./build csky ecos

    > loglevel=flow ./build csky ecos

  - 注意:

    > 没有编译选项时, 默认为 loglevel=2 编译.

| 选项              | 说明                                                       |
| ----------------  | ---------------------------------------------------------- |
| loglevel=0        | 关闭所有打印                                               |
| loglevel=1        | 开启播放器运行时的错误打印                                 |
| loglevel=2        | 开启播放器运行时的错误打印、信息打印                       |
| loglevel=3        | 开启播放器运行时的错误打印、信息打印、信警告打印           |
| loglevel=4        | 开启播放器运行时的错误打印、信息打印、信警告打印、调试打印 |
| loglevel=flow     | 开启应用调用播放器的接口打印                               |

### 洞内存调试编译

  - 示例:

    > debughole=0 ./build csky ecos

    > debughole=1 ./build csky ecos

  - 注意:

    > 没有编译选项时, 默认为debughole=0 编译.

    > 需要开启洞内存调试时, 需要开启并重新编译 gxapi、gxbus、应用等使用洞内存的工程.

| 选项              | 说明                                                       |
| ----------------  | ---------------------------------------------------------- |
| debughole=0       | 关闭洞内存调试                                             |
| debughole=1       | 开启洞内存调试                                             |

### Example

  - 播放器打印级别调试:

    - 编译:

      > loglevel=flow ./build csky ecos

    - 打印如下:
      ```
      [2020-03-13 10:37:12] I/ [FLOW] - [GxPlayer_MediaRecordConfig 212]: player_DVB
      [2020-03-13 10:37:12] I/ [FLOW] - [GxPlayer_MediaRecordConfig 213]: player_DVB
      ...
      [2020-03-13 10:37:16] I/ [FLOW] - [GxPlayer_MediaPause 218]: player_DVB
      ...
      [2020-03-13 10:37:17] I/ [FLOW] - [GxPlayer_MediaPause 219]: player_DVB
      ...
      [2020-03-13 10:37:17] I/ [FLOW] - [GxPlayer_MediaGetTime 488]: player_DVB
      [2020-03-13 10:37:17] I/ [FLOW] - [GxPlayer_MediaGetTime 507]: player_DVB, 0, 0, 0
      ```

    - [实例1](http://git.nationalchip.com/redmine/issues/250591)

## 功能调试

  - **头文件**: gxplayer_debug.h.

| 功能                        | 接口调试                                | Example |
| --------------------------- | --------------------------------------- | --------------------------- |
| [网络交互调试开关](#func-1)            | gxdocref GxPlayer_DebugNetworkStream    |  |
| [RTP 丢包率统计调试开关](#func-2)      | gxdocref GxPlayer_DebugNetworkRTPPacket |  |
| [Stream 读写速率统计调试开关](#func-3) | gxdocref GxPlayer_DebugStreamSpeed      | [实例1](http://git.nationalchip.com/redmine/issues/247156) |
| [PVR 录制调试开关](#func-5)            | gxdocref GxPlayer_DebugStreamPVR        |  |
| [Packet 拆包速率统计调试开关](#func-4) | gxdocref GxPlayer_DebugPacketSpeed      |  |
| [音视频解码数据量调试开关](#func-6)    | gxdocref GxPlayer_DebugDemuxFillData    |  |
| [音频拆包时戳大小调试开关](#func-7)    | gxdocref GxPlayer_DebugDemuxApts        |  |
| [视频拆包时戳大小调试开关](#func-8)    | gxdocref GxPlayer_DebugDemuxVpts        |  |
| [播放器字幕同步调试开关](#func-9)      | gxdocref GxPlayer_DebugSubtitleSync     |  |
| [播放器音视频信息调试开关](#func-18)    | gxdocref GxPlayer_DebugAVInfo           |  |
| [播放器使用模块情况调试](#func-10)      | gxdocref GxPlayer_DebugPlayerMedia      |  |
| [播放器切台时间统计调试](#func-11)      | gxdocref GxPlayer_DebugPlayerCostTime   |  |
| [音频解码打印级别调试设置](#func-12)    | gxdocref GxPlayer_DebugAdecConfig       |  |
| [音频解码打印内容调试设置](#func-13)    | gxdocref GxPlayer_DebugAdecContent      |  |
| [音频播放打印级别调试设置](#func-14)    | gxdocref GxPlayer_DebugAoutConfig       |  |
| [音频播放打印级别调试设置](#func-15)    | gxdocref GxPlayer_DebugAoutConfig       |  |
| [音频解码 pcm 数据 dump 调试](#func-16) | gxdocref GxPlayer_DumpAdecPCM         | [实例1](http://git.nationalchip.com/redmine/issues/249437) |
| [音频解码  es 数据 dump 调试](#func-18) | gxdocref GxPlayer_DumpAdecESA         |  |
| [音频播放 pcm 数据 dump 调试](#func-17) | gxdocref GxPlayer_DumpAoutPCM         |  |

### 网络交互调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.network_stream = 1
或者调用函数:
    GxPlayer_DebugNetWorkStream(1)
打印说明:
    //请求命令
    I/ >>>>> GET /fvod/avi_stream/chenpeng.avi HTTP/1.1
    Host: 192.168.1.189:10080
    Accept: */*
    User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.131
    Safari/537.36
    Range: bytes=0-
    Connection: keep-alive

    //接收回复
    I/ <<<<< HTTP/1.1 206 Partial Content
    Accept-Ranges: bytes
    Access-Control-Allow-Origin: *
    Content-Length: 612796760
    Content-Range: bytes 0-612796759/612796760
    Content-Type: video/x-msvideo
    Last-Modified: Mon, 29 Mar 2021 08:19:03 GMT
    Set-Cookie: sid=Z2g9DNXGg; Path=/; Expires=Wed, 14 Apr 2021 11:59:33 GMT; Max-Age=86400; HttpOnly
    Date: Tue, 13 Apr 2021 11:59:33 GMT
用途说明:
    网络交互是否出现异常, 只限于 http / https 的网络交互.
```

### RTP 丢包率统计调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.rtp_packet = 1
或者调用函数:
    GxPlayer_DebugNetworkRTPPacket(1)
打印说明:
    略
用途说明:
    播放网络 RTP 视频出现马赛克时, 通过该调试打印来判断网络端是否出现丢包.
```

### Stream 读写速率统计调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.stream_speed = 1
或者调用函数:
    GxPlayer_DebugStreamSpeed(1)
打印说明:
    //  文件链接地址 读写操作 速率
    I/ /mnt/usb01/tegong.Inside.Out.2015.1080p_hevc_10bit.mkv, r 18376.24 KB/s
    I/ /mnt/usb01/tegong.Inside.Out.2015.1080p_hevc_10bit.mkv, r 17299.01 KB/s
用途说明:
    播放大码率文件, 通过打印分析读写速率与码流码率的对比, 判断是否满足码流播放速率.
```

### PVR 录制调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.stream_pvr = 1
或者调用函数:
    GxPlayer_DebugStreamPVR(1)
打印说明:
    略
用途说明:
    PVR 录制流程打印, 分析其是否符合预期.
```

### Packet 拆包速率统计调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.packet_speed = 1
或者调用函数:
    GxPlayer_DebugPacketSpeed(1)
打印说明:
    //  音频或者视频         速率
    I/ audio packet speed   91.15 KB/s
    I/ audio packet speed  111.23 KB/s
    I/ video packet speed 9464.12 KB/s
用途说明:
    播放大码率文件, 通过音视频拆包速率与音视频码率对比, 来判断卡顿是否是拆包导致的.
```

### 音视频解码数据量调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.demux_fill_data = 1
或者调用函数:
    GxPlayer_DebugDemuxFillData(1)
打印说明:
    //音频解码器0数据量   音频解码器1数据量  视频解码器数据量  音频解复用0数据量 音频解复用1数据量 视频解复用数据量 stream_buffer数据量
    I/ fill - ea0:62.83K    ea1:0.00K       ev:107.43K,     da0:1.21(K)     da1:0.00(K)     dv0.00(K),  sc:16.52(K)
    I/ fill - ea0:62.83K    ea1:0.00K       ev:107.43K,     da0:1.21(K)     da1:0.00(K)     dv0.00(K),  sc:16.52(K)
    I/ fill - ea0:63.48K    ea1:0.00K       ev:117.25K,     da0:5.89(K)     da1:0.00(K)     dv0.00(K),  sc:16.52(K)
    I/ fill - ea0:63.70K    ea1:0.00K       ev:117.25K,     da0:3.89(K)     da1:0.00(K)     dv0.00(K),  sc:16.52(K)
    I/ fill - ea0:63.70K    ea1:0.00K       ev:117.25K,     da0:3.89(K)     da1:0.00(K)     dv0.00(K),  sc:16.52(K)
用途说明:
    播放文件, 音频或者视频发生卡顿, 通过打印判断音视频卡顿是否是数据不够导致的.
```

### 音频拆包时戳大小调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.demux_apts = 1
或者调用函数:
    GxPlayer_DebugDemuxApts(1)
打印说明:
    I/ audio: pts 1464212, size 24379     //音频 时戳 包大小
    I/ audio: pts 1464046, size 2202
    I/ audio: pts 1463962, size 691
用途说明:
    播放文件, 通过和 ffmpeg 拆包对比, 来判断音频拆包是否异常.
```

### 视频拆包时戳大小调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.demux_apts = 1
或者调用函数:
    GxPlayer_DebugDemuxVpts(1)
打印说明:
    I/ video: pts 1464212, size 24379     //视频 时戳 包大小
    I/ video: pts 1464046, size 2202
    I/ video: pts 1463962, size 691
用途说明:
    播放文件, 通过和 ffmpeg 拆包对比, 来判断视频拆包是否异常.
```

### 播放器字幕同步调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.sub_sync = 1
或者调用函数:
    GxPlayer_DebugSubtitleSync(1)
打印说明:
    I/ [subtitle]-[RP] start:-17270647 cur:-17271303 start-current:656
    I/ [subtitle]-[RP] start:-17270647 cur:-17271207 start-current:560
    I/ [subtitle]-[RP] start:-17270647 cur:-17271111 start-current:464
    I/ [subtitle]-[RP] start:-17270647 cur:-17271015 start-current:368
    I/ [subtitle]-[RP] start:-17270647 cur:-17270919 start-current:272
    I/ [subtitle]-[CM] start:-17270647 cur:-17270823 start-current:176
用途说明:
    定位是否有字幕数据,以及字幕是否是同步丢失.
```

### 播放器音视频信息调试开关

```
调试开关:
    (cskygdb) p PSystem.debug.av_info = 1
或者调用函数:
    GxPlayer_DebugAVInfo(1)
打印说明:
    //          同步方式      音频播放时辍 视频播放时辍 音视频差     视频同步时辍  音频待解码数据量 视频待解码数据量
    I/ [av]: stc - avpts, pts - 4075438423 4075442861 offset -98  vdec: 4075444661, size - 20870  641276
    I/ [av]: stc - avpts, pts - 4075438423 4075442861 offset -98  vdec: 4075446461, size - 20870  641276
    I/ [av]: stc - avpts, pts - 4075439503 4075444661 offset -114 vdec: 4075448261, size - 19886  609244
    I/ [av]: stc - avpts, pts - 4075440583 4075444661 offset -90  vdec: 4075448261, size - 19886  609244
    I/ [av]: stc - avpts, pts - 4075441663 4075446461 offset -106 vdec: 4075450061, size - 18350  593884
用途说明:
    用于定位音视频是否同步, 以及是否缺数据导致卡顿等.
```

### 播放器使用模块情况调试

```
调用函数:
    GxPlayer_DebugPlayerMedia()
打印说明:
    I/ ==================== Player Use Module ====================
    I/ ===========================================================
    I/ Player Name: player_av
    I/   media play url: /mnt/usb01/1.mkv   // 播放url
    I/     stream  name: StreamFile         // stream 模块
    I/     demuxer name: Demux LAVF         // demuxer 模块
    I/     adec0   name: HW  AudioDecoder   // 音频解码模块
    I/             type: ac3                // 音频编码
    I/     vdec    name: HW  DECODER        // 视频解码模块
    I/             type: h264               // 视频编码
    I/     cdec    name: Text Sub Decoder   // 字幕解码模块
    I/     aplay   name: HW AudioPlayer     // 音频播放模块
    I/     vplay   name: HW VideoDecoder    // 视频播放模块
    I/     splay   name: SW Sub Text Out    // 字幕播放模块
    I/ ===========================================================
用途说明:
    播放器运行模块基本定位.
```

### 播放器切台时间统计调试

```
调用函数:
    GxPlayer_DebugPlayerCostTime()
打印说明:
    I/ ==================== Player Cost Time  ====================
    I/ ===========================================================
    I/ Player Name: player_av
    I/        total play   func:         300  ms    ## 播放接口调用花费总时间
    I/              player prep:         50   ms
    I/              player zoom:         40   ms
    I/              media  stop:         0    ms      ## 停止上一次media花费总时间，以下是具体时间
    I/                     stream:       0    ms
    I/                     demuxer:      0    ms
    I/                     video:        0    ms
    I/                     audio:        0    ms
    I/                     subtitle:     0    ms
    I/                     alive:        0    ms
    I/              media  open:         20   ms     ## 打开media花费总时间，以下是具体时间
    I/                     stream:       0    ms
    I/                     demuxer:      20   ms
    I/                     video:        0    ms
    I/                     audio:        0    ms
    I/                     subtitle:     0    ms
    I/              media  config:       10   ms     ## 配置media花费总时间，以下是具体时间
    I/                     stream:       0    ms
    I/                     demuxer:      0    ms
    I/                     video:        0    ms
    I/                     audio:        10   ms
    I/                     subtitle:     0    ms
    I/              media  run:          180  ms    ## 运行media花费总时间，以下是具体时间
    I/                     stream:       0    ms
    I/                     demuxer:      150  ms
    I/                     video:        0    ms
    I/                     audio:        20   ms
    I/                     subtitle:     10   ms
    I/              media  seek:         0    ms
    I/                     demuxer:      0    ms    ## 播放器解复用 SEEK 花费的时间
    I/        audio frame  decode:(20)   370  ms    ## 开始调用播放接口到视频解码到(20)帧存时花费时间
    I/        video frame  decode:(02)   390  ms    ## 开始调用播放接口到音频解码到(02)帧存时花费时间
    I/        video start  decode:       360  ms    ## 视频解码探测到帧存开始启动解码花费时间
    I/        video first  show:         390  ms    ## 开始调用播放接口到视频显示首个I帧花费时间
    I/ ===========================================================
用途说明:
    统计切台时间, 定位耗时模块.
```

### 音频解码打印级别调试设置

```
调用函数:
   GxPlayer_DebugAdecConfig(PlayerDebugAVLevel level)
参数说明:
   typedef enum {
   　　GX_PLAYER_DEBUG_LEVEL_ERROR   = (0x1 << 0),　//错误打印
   　　GX_PLAYER_DEBUG_LEVEL_INFO    = (0x1 << 1),　//信息打印
   　　GX_PLAYER_DEBUG_LEVEL_WARNING = (0x1 << 2),　//警告打印
   　　GX_PLAYER_DEBUG_LEVEL_DEBUG   = (0x1 << 3),　//调试打印
   } PlayerDebugAVLevel;
```

### 音频解码打印内容调试设置

```
调用函数:
    GxPlayer_DebugAdecContent(PlayerDebugAdecContent content)
参数说明:
    typedef enum {
    　　GX_PLAYER_DEBUG_DECODE_ESA       = (0x1 << 0),  //解码esa数据量打印
    　　GX_PLAYER_DEBUG_DECODE_TASK      = (0x1 << 1),  //解码配置任务流程打印
    　　GX_PLAYER_DEBUG_DECODE_FIND_PTS  = (0x1 << 2),  //解码查找pts打印
    　　GX_PLAYER_DEBUG_DECODE_FIX_PTS   = (0x1 << 3),  //解码修复pts打印
    　　GX_PLAYER_DEBUG_BYPASS_ESA       = (0x1 << 4),  //透传esa数据量打印
    　　GX_PLAYER_DEBUG_BYPASS_TASK      = (0x1 << 5),  //透传配置任务流程打印
    　　GX_PLAYER_DEBUG_BYPASS_FIND_PTS  = (0x1 << 6),  //透传查找pts打印
    　　GX_PLAYER_DEBUG_BYPASS_FIX_PTS   = (0x1 << 7),  //透传修复pts打印
    　　GX_PLAYER_DEBUG_CONVERT_ESA      = (0x1 << 8),  //转码esa数据量打印
    　　GX_PLAYER_DEBUG_CONVERT_TASK     = (0x1 << 9),  //转码配置任务流程打印
    　　GX_PLAYER_DEBUG_CONVERT_FIND_PTS = (0x1 << 10), //转码查找pts打印
    　　GX_PLAYER_DEBUG_CONVERT_FIX_PTS  = (0x1 << 11), //转码修复pts打印
    } PlayerDebugAdecContent;
打印说明:
    1. 解码esa数据量打印
       (cskygdb) p GxPlayer_DebugAdecContent(0x1<<0)
       $13 = 0
       //             任务类型　数据是否溢出　esa的数据量
       [adec]-[info]: [decode] overflow 0, esa 390662
       [adec]-[info]: [decode] overflow 0, esa 391182
       [adec]-[info]: [decode] overflow 0, esa 390598
       [adec]-[info]: [decode] overflow 0, esa 391118
    2. 解码配置任务流程打印
       (cskygdb) p GxPlayer_DebugAdecContent(0x1<<1)
       $16 = 0
       // 　　　　　　任务类型　运行函数：start - 0: 不满足解码条件, 不启动解码, １: 满足解码条件, 启动解码, complete isr：解码上报终断
       [adec]-[info]: [decode] start:0
       [adec]-[info]: [decode] start:1
       [adec]-[info]: [decode] complete isr
    3. 透传查找pts打印:
       (cskygdb) p GxPlayer_DebugAdecContent(0x1<<2)
       $15 = 0
       //　 　　　　 错误码      pts目标地址  pts起始地址　pts结束地址　 错误码: 0 - 正常解码, 其余 - 解码异常
       [adec]-[info]: err 0 - 1, 0x00000000, 0x14ca9f11, 0x14caa211, ( 0  0), (0 0)
       [adec]-[info]: err 0 - 1, 0x14caa480, 0x14caa211, 0x14caa511, ( 0  0), (1 0)
       [adec]-[info]: err 0 - 1, 0x00000000, 0x14caa511, 0x14caa811, ( 0  0), (0 0)
    4. 解码修复pts打印:
       (cskygdb) p GxPlayer_DebugAdecContent(0x1<<3)
       $17 = 0
       //            错误码        样点数 　　　修复的pts    查找的pts   每帧播放时长(时长发生跳变说明解码有异常)
       [adec]-[info]: err 0 - 1, 3, 1152,    0, 3955343869,          0, 1080
       [adec]-[info]: err 0 - 1, 3, 1152,    0, 3955344949,          0, 1080
       [adec]-[info]: err 0 - 1, 3, 1152,    0, 3955346029,          0, 1080
       [adec]-[info]: err 0 - 1, 2, 1152,    0, 3955347109, 3955347109, 1080
```

### 音频播放打印级别调试设置

```
调用函数:
   GxPlayer_DebugAoutConfig(PlayerDebugAVLevel level)
参数说明:
   typedef enum {
   　　GX_PLAYER_DEBUG_LEVEL_ERROR   = (0x1 << 0),　//错误打印
   　　GX_PLAYER_DEBUG_LEVEL_INFO    = (0x1 << 1),　//信息打印
   　　GX_PLAYER_DEBUG_LEVEL_WARNING = (0x1 << 2),　//警告打印
   　　GX_PLAYER_DEBUG_LEVEL_DEBUG   = (0x1 << 3),　//调试打印
   } PlayerDebugAVLevel;
```

### 音频播放打印级别调试设置

```
调用函数:
    status_t GxPlayer_DebugAoutContent(PlayerDebugAoutContent content)
参数说明:
　　typedef enum {
　　  GX_PLAYER_DEBUG_R0_SYNC_PTS = (0x1 << 0), //R0同步信息：PCM数据播放时的同步打印信息
　　  GX_PLAYER_DEBUG_R1_SYNC_PTS = (0x1 << 1), //R1同步信息：AD PCM数据播放时的同步打印信息
　　  GX_PLAYER_DEBUG_R2_SYNC_PTS = (0x1 << 2), //R2同步信息：ac3数据播放时的同步打印信息
　　  GX_PLAYER_DEBUG_R3_SYNC_PTS = (0x1 << 3), //R0同步信息：eac3/dts/aac数据播放时的同步打印信息
　　} PlayerDebugAoutContent;
打印说明:
    1. R0同步信息-PCM数据播放时的同步打印信息:
       (cskygdb) p GxPlayer_DebugAoutContent(0x1<<0)
       $19 = 0
       //        R0播放　同步状态　该播放帧的pts   播放时对应的stc 　pts与stc差值(MS) 配置的低门限　配置的高门限
       [aout]-[info]: R0 - [CM]: pts.4287608653 stc.4287608650 offset.  2 low.100, high.4000 (sk: 0)
       [aout]-[info]: R0 - [CM]: pts.4287608677 stc.4287608656 offset. 21 low.100, high.4000 (sk: 0)
       [aout]-[info]: R0 - [CM]: pts.4287608701 stc.4287608690 offset. 10 low.100, high.4000 (sk: 0)
       [aout]-[info]: R0 - [CM]: pts.4287608725 stc.4287608703 offset. 22 low.100, high.4000 (sk: 0)
       [aout]-[info]: R0 - [CM]: pts.4287608749 stc.4287608730 offset. 18 low.100, high.4000 (sk: 0)
```

### 音频解码 pcm 数据 dump 调试

```
调用函数:
    GxPlayer_DumpAdecPCM(int enable, char *url)
用途说明:
    音频声音异常时, 通过 dump 音频解码 pcm 数据来判断解码数据是否正常.
```

### 音频解码  es 数据 dump 调试

```
调用函数:
    GxPlayer_DumpAdecESA(int enable, char *url)
用途说明:
    音频声音异常时, 通过 dump 音频解码  es 数据来判断输入数据是否正常.
```

### 音频播放 pcm 数据 dump 调试

```
调用函数:
    GxPlayer_DumpAoutPCM(int enable, char *url)
用途说明:
    音频声音异常, 通过 dump 音频播放 pcm 数据来判断播放数据是否正常.
```

## 内存调试

### 堆内存调试

    - 编译:

      > optimize=00 ./build csky ecos

    - 打印:

      ```
      调试开关:
          (cskygdb) p gxplayer_heap_info ()
          或者调用函数：
              GxPlayer_HeapInfo()
      打印说明:
          I/ ============================================
          I/ -[sys]                                                       //系统内存使用信息
          I/        maxsiz = 0x400000, maxuse = 0x51020, nowuse = 0x0     //maxsiz: 申请系统最大内存大小，maxuse: 使用系统内存的峰值大小，nowuse: 现使用系统内存的大小
          I/ -[user]                                                      //用户配置内存使用信息，例如: femem地址等
          I/    [0] maxsiz = 0x0, maxuse = 0x0, nowuse = 0x0
          I/ -[hole]                                                      //共用洞内存使用信息:
          I/    [0] maxsiz = 0x500000, maxuse = 0x0, nowuse = 0x0         //第一块洞的内存使用信息
          I/    [1] maxsiz = 0x500000, maxuse = 0x0, nowuse = 0x0         //第二块洞的内存使用信息
          I/ -------------------------------------------
          I/ [not free]                                                   //未被释放内存函数说明
          I/      GxPlayer_Create  391     0x93da7090      16424          //函数 行数 内存地址 大小
          I/ ============================================
     内存异常情况打印说明:
         1. 内存申请释放函数不配对或野指针，例如:申请GxCore_Malloc,释放av_free：
            E/ __del_dp_list:144 [not exit]: GxPlayer_Create  391     0x93da7090      16424  //检测函数 行数 调用函数行数 内存地址 大小
         2. 内存地址被重复释放：
            E/ __del_dp_list:149 [has free]: GxPlayer_Create  391     0x93da7090      16424  //检测函数 行数 调用函数行数 内存地址 大小
         3. 内存地址发生写越界:
            E/ __del_dp_list:158 [mem bound]: GxPlayer_Create  391     0x93da7090      16424 //检测函数 行数 调用函数行数 内存地址 大小
      ```

    - [实例1](git.nationalchip.com/redmine/issues/247273)

    - [实例2](git.nationalchip.com/redmine/issues/248246)

### 洞内存调试

    - 编译:

      > gxapi: debughole=1 ./build csky ecos

      > gxbus: debughole=1 ./build csky ecos

      > 应用方案：增加-DDBG_QM_MALLOC编译选项，通常应用未使用到洞操作，此选项不需要开启．

    - 打印:
    ```
    调试开关:
        (cskygdb) p GxCore_HwCheckDebug()
    打印说明:
        id 热度 分配kernel地址 分配大小 分配时调用的文件:行数
        =======================================================
        check malloc list !!
        444 0 0x9486b8b0    27824 /home/linxsh/git-code/v1.9-dev/gxbus/gui_core/core/framebuffer.c:45!!
        379 0 0x94eef8e0  3658020 common/memhole.c:32!!
        307 0 0x94c978c0   528404 common/memhole.c:32!!
        305 0 0x948778b8  4198420 common/memhole.c:32!!
        3 0 0x94694ca0  1843200 /home/linxsh/git-code/v1.9-dev/gxbus/gui_core/core/framebuffer.c:57!!
        2 0 0x944ccc98  1843200
        /home/linxsh/git-code/v1.9-dev/gxbus/gui_core/core/framebuffer.c:57!!
        1 0 0x94400c90   833556 common/memhole.c:32!!
        check malloc list finish count = 7 total size = 12932624 !!
        check free list !!
        check free list finish count = 0 total size = 0!!
        err memory count = 0!!
        =======================================================
    ```

   - 注意:

     - 播放器stop有相应4块内存不被释放,分别为: freeze/svpu buffer 以及 esa/esv 内容保护buffer.

     - esa/esv buffer - 防止denmux硬件冲内存．(内容保护)

     - freeze buffer  - 静帧播放使用.

     - svpu buffer    - cvbs显示使用．

   - [实例1](http://git.nationalchip.com/redmine/issues/247130)

   - [实例2](http://git.nationalchip.com/redmine/issues/282464)

## 其他调试

### ecos线程堆栈调试

   ```
   调试开关:
       (cskygdb) p gxthread_info ()
   打印说明:
       -------  -- ------ ---------------------------------------- -- -- ---------- ---------- ---------- ----------
       Handle   ID State  Name                                     SP CP Stack Base Stack Size Stack Used Stack Ptr
       -------  -- ------ ---------------------------------------- -- -- ---------- ---------- ---------- ----------
       90bc5a90 23 SLEEP  FrontendMsgScheduler                     14 14 0x90bc5b94 0x00001eac 0x0000011c 0x90bc7900
       90bc7b50 24 SLEEP  FrontendConsoleScheduler                 15 15 0x90bc7c54 0x00001eac 0x00000a50 0x90bc98e0
       90bc9cb0 25 SLEEP  HotPlugConsoleScheduler                  14 14 0x90bc9db4 0x00001eac 0x00000448 0x90bcbb20
       90bcbf10 26 RUN    HdmiHotPlugConsoleScheduler              15 15 0x90bcc014 0x00001eac 0x00000a70 0x90bcdbc8
       90bce010 27 SLEEP  HdmiHotPlugMsgScheduler                  15 15 0x90bce114 0x00001eac 0x0000011c 0x90bcfe80
       90bd0170 28 SLEEP  DebugConsoleScheduler                    15 15 0x90bd0274 0x00001eac 0x00000998 0x90bd1ff8
       90a0e068 31 SLEEP  usb_storage                              13 13 0x90e01b80 0x00001000 0x00000628 0x90e02aa8
       90e02eb0 32 RUN    case_manage                              15 15 0x90e02fb4 0x00018eac 0x00000900 0x90e1bd18
       90e1bef0 33 RUN    frontend_info                            15 15 0x90e1bff4 0x00018eac 0x000005d0 0x90e34da8
       909d1b68 1  RUN    Idle Thread                              31 31 0x909c1b68 0x00010000 0x000005c0 0x909d1a80
       909beb6c 2  SUSP   main                                     15 15 0x909bec70 0x00001eac 0x0000118c 0x909c0a48
       90a0deb8 3  SLEEP  khubd                                    10 10 0x90a09eb8 0x00004000 0x00000a58 0x90a0ddb0
       90aa4fd0 4  SLEEP  do_mount                                 15 15 0x90aa50d4 0x00001eac 0x000007ec 0x90aa6d90
       90aa7010 5  SLEEP  auto_unmount                             20 20 0x90aa7114 0x00001eac 0x000000cc 0x90aa8ed0
       909a1ec0 6  SLEEP  gx3211_hdmi_thread                       9  9  0x90aad4a0 0x00004000 0x00000480 0x90ab1390
       90a58980 7  SLEEP  thread-demux0                            9  9  0x90ab1520 0x00004000 0x00000440 0x90ab5420
       90ae8490 8  SLEEP  init                                     15 15 0x90ae8594 0x00018eac 0x00001008 0x90b00e60
       909a5570 9  SLEEP  gx3201_jpeg_thread                       9  9  0x90b11680 0x00004000 0x000004c0 0x90b154f8
       90b15710 10 SLEEP  pMonitor                                 14 14 0x90b15814 0x0000feac 0x00001f40 0x90b255c0
       90b25750 11 SLEEP  PlayerMsgScheduler                       14 14 0x90b25854 0x00007eac 0x00001458 0x90b2d5c0
       90b93bb0 12 SLEEP  SiMsgScheduler                           15 15 0x90b93cb4 0x00001eac 0x000006b0 0x90b95a20
       90b95bf0 13 SLEEP  SiConsoleScheduler                       16 16 0x90b95cf4 0x00001eac 0x0000017c 0x90b97a00
       90b98030 14 SLEEP  parse read                               15 15 0x90b98134 0x00003eac 0x000000f4 0x90b9bec8
       90b9c130 15 SLEEP  EpgMsgScheduler                          15 15 0x90b9c234 0x00001eac 0x0000011c 0x90b9dfa0
       90b9e1f0 16 RUN    EpgConsoleScheduler                      15 15 0x90b9e2f4 0x00001eac 0x00000c18 0x90ba0068
       90ba0690 17 SLEEP  SearchMsgScheduler                       15 15 0x90ba0794 0x00001eac 0x0000011c 0x90ba2500
       90ba29f0 18 SLEEP  BlindSearchMsgScheduler                  15 15 0x90ba2af4 0x00007eac 0x0000011c 0x90baa860
       90baaab0 19 SLEEP  BlindSearchConsoleScheduler              15 15 0x90baabb4 0x00007eac 0x00000114 0x90bb2928
       90bb2d10 20 SLEEP  ExtraMsgScheduler                        15 15 0x90bb2e14 0x00001eac 0x0000011c 0x90bb4b80
       90bb5730 21 SLEEP  GuiViewMsgScheduler                      14 14 0x90bb5834 0x00007eac 0x00004700 0x90bbd5a0
       90bbd7f0 22 RUN    GuiViewConsoleScheduler                  14 14 0x90bbd8f4 0x00007eac 0x00006200 0x90bc5508
       90f35330 87 SLEEP  play step2                               14 14 0x90f35434 0x0000feac 0x000011b8 0x90f451b8
       90ea78f0 88 RUN    demux_depack                             15 15 0x90ea79f4 0x0000feac 0x000014b0 0x90eb7750
       9102d1f0 89 EXIT   so_text_thread                           15 15 0x9102d2f4 0x000026ac 0x000002e0 0x9102f8c0
       910023b0 90 RUN    demux_check                              15 15 0x910024b4 0x00001eac 0x00000d58 0x91004248
       90fdaaf0 91 RUN    sd_text_run_thread                       15 15 0x90fdabf4 0x00001eac 0x000010e8 0x90fdc968
       Total Stack Size:  958504
       Total Stack Used:  114832
   查看线程的栈信息:
       (cskygdb)  p gxthread_load(0x90eb7750) //Stack Ptr对应值

       Program received signal SIGTRAP, Trace/breakpoint trap.
       0x9012352c in gxthread_load ()
       The program being debugged was signaled while in a function called from GDB.
       GDB remains in the frame where the signal was received.
       To change this behavior use "set unwindonsignal on".
       Evaluation of the expression containing the function
       (gxthread_load) will be abandoned.
       When the function is done executing, GDB will silently stop.
       (cskygdb) bt
       #0  0x9012352c in gxthread_load ()
       #1  0x90117272 in Cyg_Scheduler::unlock_inner (new_lock=0)
           at /home/linxsh/git-code/v1.9-dev/ecos3.0/ecos3.0/packages/kernel/v3_0/src/sched/sched.cxx:203
       #2  0x901167d2 in Cyg_Thread::delay(unsigned long long) ()
       #3  0x901156da in cyg_thread_delay (delay=<value optimized out>)
           at /home/linxsh/git-code/v1.9-dev/ecos3.0/ecos3.0/packages/kernel/v3_0/src/common/kapi.cxx:306
       #4  0x9002e5a4 in GxCore_ThreadDelay (millisecond=10) at os/ecos/osapi.c:628
       #5  0x90299890 in GxDemuxStream_PushSub (ds=0x90f6de44)
           at /home/linxsh/git-code/v1.9-dev/gxbus/player/demuxer/demux_stream.c:1004
       #6  0x90290a1c in demuxerbase_run_thread (data=0x90f6b280)
           at /home/linxsh/git-code/v1.9-dev/gxbus/player/demuxer/demux.c:494
       #7  0x9002e1bc in default_thread_function (arg=0x90aa3c98) at os/ecos/osapi.c:411
       #8  0x90112f8a in pthread_entry(unsigned int) ()
       #9  0x90116534 in Cyg_HardwareThread::thread_entry(Cyg_Thread*) ()
       #10 0x90116520 in ?? ()
   ```
   - [实例1](http://git.nationalchip.com/redmine/issues/250995)
