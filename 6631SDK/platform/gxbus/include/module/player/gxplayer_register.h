/** \addtogroup  gxplayer_register
 *  @{
 */
#ifndef __GXPLAYER_REGISTER_H__
#define __GXPLAYER_REGISTER_H__

__BEGIN_DECLS

/**
 * \brief   注册播放器所有模块功能.
 * \details 1. 注册播放器所有模块的所有功能.
 * \details 2. 模块包括: Stream, Demuxer, Muxer, Decoder, Recoder, Subtitle, DVBSource.
 * \details 3. IPTV 功能采用 V1 版本.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterALL(void);

/**
 * \brief   注册播放器 Stream 模块的 HTTP 功能.
 * \details 1. V1 版本.
 * \details 2. 协议介绍请参考: [HTTP](../background/background.md#httphttps)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamHTTP(void);

/**
 * \brief   注册播放器 Stream 模块的 RTSP 功能.
 * \details 1. V1 版本.
 * \details 2. 该版本用到第三方库+ffmpeg rtsp功能，对flash要求比较高.
 * \details 3. 先使用librtsp,fail后再走ffmpeg rtsp.
 * \details 4. 协议介绍请参考: [RTSP](../background/background.md#rtsprtcp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRTSP(void);

/**
 * \brief   注册播放器 Stream 模块的 RTMP 功能.
 * \details 1. V1 版本.
 * \details 2. 使用第三方库librtmp
 * \details 3. 协议介绍请参考: [RTMP](../background/background.md#rtmp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRTMP(void);

/**
 * \brief   注册播放器 Stream 模块的 UDP 功能.
 * \details 1. V1 版本.
 * \details 2. 协议介绍请参考: [UDP](../background/background.md#udp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamUDP(void);

/**
 * \brief   注册播放器 Stream 模块的 RTP 功能.
 * \details 1. V1 版本.
 * \details 2. only support rtp mpegts mode.
 * \details 3. 协议介绍请参考: [RTP](../background/background.md#rtp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRTP(void);

/**
 * \brief   注册播放器 Stream 模块的 M3U8 功能.
 * \details 1. V1 版本.
 * \details 2. 协议介绍请参考: [hls](../background/background.md#hlsll-hls)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamM3U8(void);

/**
 * \brief   注册播放器 Stream 模块的 MPEG-DASH 功能.
 * \details 1. V1 版本.该版本已丢弃,因为mpegdah现统一使用ffmpeg dash
 * \details 2. 协议介绍请参考: [mpegdash](../background/background.md#mpegdash)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamMPEGDASH(void);

/**
 * \brief   注册播放器所有模块功能.
 * \details 1. 注册播放器所有模块的所有功能.
 * \details 2. 模块包括: Stream, Demuxer, Muxer, Decoder, Recoder, Subtitle, DVBSource.
 * \details 3. IPTV 功能采用 V2 版本.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterALL_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 HTTP 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg http protocol.
 * \details 3. 协议介绍请参考: [http](../background/background.md#httphttps)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamHTTP_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 RTSP 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg rtsp protocol.
 * \details 3. 协议介绍请参考: [rtsp](../background/background.md#rtsp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRTSP_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 RTMP 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg rtmp protocol.
 * \details 3. 协议介绍请参考: [rtmp](../background/background.md#rtmp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRTMP_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 UDP 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg udp protocol.在V1版本时不注册.方便协议间的自由组合.
 * \details 3. 协议介绍请参考: [udp](../background/background.md#udp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamUDP_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 RTP 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg rtp protocol.在V1版本时不注册.
 * \details 3. 支持ffmpeg rtp支援的所有特性(rtp mpegts or 一路rtp mode).
 * \details 4. 协议介绍请参考: [rtp](../background/background.md#rtp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRTP_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 RTPSDP 功能.
 * \details 1. V2 版本.
 * \details 2. (功能:rtp的单路,player需要推流时的SDP信息).
 * \details 3. 依赖于GxPlayer_ModuleRegisterStreamRTP_V2,注册时,需要依赖rtp_V2的注册.
 * \details 4. 协议介绍请参考: [rtpsdp](../background/background.md#rtpsdp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRTPSDP_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 HLS 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg hls protocol.在V1版本时不注册.
 * \details 3. 协议介绍请参考: [hls](../background/background.md#hlsll-hls)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamM3U8_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 MPEG-DASH 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg mpegdash protocol.V1/V2均使用ffmpeg mpegdash.
 * \details 3. 协议介绍请参考: [mpegdash](../background/background.md#mpegdash)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamMPEGDASH_V2(void);

/**
 * \brief   注册播放器 Stream 模块的软加解密功能.
 * \details 1. V1 版本.
 * \details 2. use hls aes.
 * \details 4. 区别:(
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     用于pvr录制加密数据解密使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO:     仅iptv V1 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO_V2:  仅iptv V2 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     仅 ABV DRM 加密流解密(客户定制);
 *)
 * \details 5. 协议介绍请参考: [hls](../background/background.md#hlsll-hls)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamSWCRYPTO(void);

/**
 * \brief   注册播放器 Stream 模块的软加解密功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg crypto protocol.
 * \details 3. 常用于hls aes解密.
 * \details 4. 区别:(
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     用于pvr录制加密数据解密使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO:     仅iptv V1 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO_V2:  仅iptv V2 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     仅 ABV DRM 加密流解密(客户定制);
 *)
 * \details 5. 协议介绍请参考: [hls](../background/background.md#hlsll-hls)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamSWCRYPTO_V2(void);

/**
 * \brief   注册播放器 Stream 模块的硬件加解密功能.
 * \details 1. V1 / V2 版本兼容.
 * \details 2. 使用于:PVR录制加密数据，加密密钥应用提供，回放配置密钥可播放.#50687
 * \details 3. 该功能只用于pvr录制加密数据的处理,并不用于iptv.
 * \details 4. 区别:(
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     用于pvr录制加密数据解密使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO:     仅iptv V1 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO_V2:  仅iptv V2 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     仅 ABV DRM 加密流解密(客户定制);
 *)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamHWCRYPTO(void);

/**
 * \brief   注册播放器 Stream 模块的 DRM 加解密功能.
 * \details 1. V1版本兼容.V2暂时未调试过
 * \details 2. use [#113233: ABV DRM 加密流播放]
 * \details 3. 区别:(
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     用于pvr录制加密数据解密使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO:     仅iptv V1 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamSWCRYPTO_V2:  仅iptv V2 hls aes 使用;
 ******     GxPlayer_ModuleRegisterStreamHWCRYPTO:     仅 ABV DRM 加密流解密(客户定制);
 *)
 * \param   void.
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamDRMCRYPTO(void);

/**
 * \brief   注册播放器 Stream 模块的 concat 功能.
 * \details 1. V2 版本.
 * \details 2. 私有协议;用于url拼接,类似于播完当前源后再去播下个源.
 * \details 3. 协议介绍请参考: [concat](../background/background.md#concat)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamConcat_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 compose 功能.
 * \details 1. V2 版本.
 * \details 2. 私有协议;合成mpegdash spd文件,常用于youtube url av分离.
 * \details 3. GxPlayer_ModuleRegisterStreamMPEGDASH_V2(需要确认是否注册dash模块).
 * \details 4. 协议介绍请参考: [compose](../background/background.md#compose)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamCompose_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 SRTP 功能.
 * \details 1. V2 版本兼容.
 * \details 2. 注册ffmpeg srtp protocol(Secure Real-time Transport Protocol).
 * \details 3. 该模块暂时未见客户使用过,在一些视频会议模块上使用.
 * \details 4. 协议介绍请参考: [rtp](../background/background.md#rtp)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamSRTP_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 MMS 功能.
 * \details 1. V1 版本.
 * \details 2. 该协议使用非常少.
 * \details 3. 协议介绍请参考: [mms](../background/background.md#mms)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamMMS(void);

/**
 * \brief   注册播放器 Stream 模块的 MMS 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg mms协议
 * \details 3. 协议介绍请参考: [mms](../background/background.md#mms)
 *
 * \param   void.
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamMMS_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 RAW-DATA 功能.
 * \details 1. V2 版本兼容.
 * \details 2. 该协议从ffmpeg移植过来的，目前未使用过.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamData(void);

/**
 * \brief   注册播放器 Stream 模块的 WIFI-DISPLAY 功能.
 * \details 1. V1 / V2 版本兼容.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamWFD(void);

/**
 * \brief   注册播放器 Stream 模块的 DVB 功能.
 * \details 1. V1 / V2 版本兼容.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamDVB(void);

/**
 * \brief   注册播放器 Stream 模块的 FILE 功能.
 * \details 1. V1 / V2 版本兼容.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamFILE(void);

/**
 * \brief   注册播放器 Stream 模块的 FILE m3u8 and FILE mpegdash功能.
 * \details 1. V2 版本兼容.
 * \details 2. 仅用于本地的m3u8 and 本地的dash文件播放.
 * \details 3. FILE mpegdash 依赖 GxPlayer_ModuleRegisterStreamMPEGDASH_V2(确认是否注册dash模块).
 * \details 3. FILE m3u8 依赖 GxPlayer_ModuleRegisterStreamM3U8_V2 or GxPlayer_ModuleRegisterStreamM3U8(确认是否注册m3u8模块).
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamFILE_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 MEM 功能.
 * \details 1. 直接播放内存数据的功能,#72862.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamMEM(void);

/**
 * \brief   注册播放器 Stream 模块的 FIFO 功能.
 * \details 1. 读取/播放 FIFO 数据的功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamFIFO(void);

/**
 * \brief   注册播放器 Stream 模块的 RINGMEM 功能.
 * \details 1. V1 / V2 版本兼容.
 * \details 2. 播放循环内存数据的功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamRINGMEM(void);

/**
 * \brief   注册播放器 Stream 模块的 PVR 录制功能.
 * \details 1. V1 / V2 版本兼容.
 * \details 2. 针对录制文件后缀为 ".pvr".
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamPVR(void);

/**
 * \brief   注册播放器 Stream 模块的 FM 功能.
 * \details 1. 播放 "fm://" 协议的 URL.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamFM(void);

/**
 * \brief   注册播放器 Stream 模块的 associationlibre 功能.
 * \details 1. V2 版本.
 * \details 2. 私有协议;处理AV分离的源,进行组合处理(不依赖于libxml),或者上层传入二路URL时，各取其中一路AV进行自由组合.
 * \details 3. 协议介绍请参考: [associationlibre](../background/background.md#associationlibre)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamAssociationLibre_V2(void);

/**
 * \brief   注册播放器 Stream 模块的 associationlibre 功能.
 * \details 1. V1 版本.
 * \details 2. 私有协议;处理AV分离的源,进行组合处理(不依赖于libxml),或者上层传入二路URL时,进行组合成.
 * \details 3. 协议介绍请参考: [associationlibre](../background/background.md#associationlibre)
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamAssociationLibre(void);

/**
 * \brief   注册播放器 Demuxer 模块 MP3 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerMP3(void);

/**
 * \brief   注册播放器 Demuxer 模块 MP4 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerMP4(void);

/**
 * \brief   注册播放器 Demuxer 模块 flac 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerFLAC(void);

/**
 * \brief   注册播放器 Demuxer 模块 wav 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerWAV(void);

/**
 * \brief   注册播放器 支持 CENC 软解密功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterCENCSchemeDecrypt(void);

/**
 * \brief   注册播放器 Demuxer 模块 AVI 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerAVI(void);

/**
 * \brief   注册播放器 Demuxer 模块 MKV 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerMKV(void);

/**
 * \brief   注册播放器 Demuxer 模块 FLV 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerFLV(void);

/**
 * \brief   注册播放器 Demuxer 模块 AAC 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerAAC(void);

/**
 * \brief   注册播放器 Demuxer 模块 AssociationLibre 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerAssociationLibre(void);

/**
 * \brief   注册播放器 Demuxer 模块 ES 流解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerES(void);

/**
 * \brief   注册播放器 Demuxer 模块 LOGO 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerLOGO(void);

/**
 * \brief   注册播放器 Demuxer 模块 RMVB 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerRMVB(void);

/**
 * \brief   注册播放器 Demuxer 模块 TS 流软解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerSWTS(void);

/**
 * \brief   注册播放器 Demuxer 模块 TS 流硬解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerHWTS(void);

/**
 * \brief   注册播放器 Demuxer 模块 REAL 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerREAL(void);

/**
 * \brief   注册播放器 Demuxer 模块 PS 流解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerPS(void);

/**
 * \brief   注册播放器 Demuxer 模块 OGG 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerOGG(void);

/**
 * \brief   注册播放器 Demuxer 模块 DOLBY / DOLBY-PLUS 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerDOLBY(void);

/**
 * \brief   注册播放器 Demuxer 模块 DTS 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerDTS(void);

/**
 * \brief   注册播放器 Demuxer 模块 APE 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerAPE(void);

/**
 * \brief   注册播放器 Demuxer 模块 FM 解复用功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDemuxerFM(void);

/**
 * \brief   注册播放器 Muxer 模块 RAW-VIDEO 混合流功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterMuxerRAWVIDEO(void);

/**
 * \brief   注册播放器 Muxer 模块 MPEG-TS 混合流功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterMuxerMPEGTS  (void);

/**
 * \brief   注册播放器 Subtitle 模块所有功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleALL     (void);

/**
 * \brief   注册播放器 Subtitle 模块 DVB 字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleDVB     (void);

/**
 * \brief   注册播放器 Subtitle 模块 SCTE-CC 字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleScte    (void);

/**
 * \brief   注册播放器 Subtitle 模块 ARIB-CC 字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleAribCC  (void);

/**
 * \brief   注册播放器 Subtitle 模块 ATSC-CC 字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleAtscCC  (void);

/**
 * \brief   注册播放器 Subtitle 模块内置字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleInside  (void);

/**
 * \brief   注册播放器 Subtitle 模块外挂字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleOutside (void);

/**
 * \brief   注册播放器 Subtitle 解码 Text 模块内置字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubDecoderText  (void);

/**
 * \brief   注册播放器 Subtitle 解码 VOD 模块内置字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubDecoderVob  (void);


/**
 * \brief   注册播放器 Subtitle 解码 TTML模块内置字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubDecoderTtml  (void);

/**
 * \brief   注册播放器 Subtitle 模块 Teletext 字幕功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterSubtitleTeletext(void);

/**
 * \brief   注册播放器 Decoder 模块 APE 音频软解功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDecoderSWAPE(void);

/**
 * \brief   注册播放器 Recoder 模块 DVB 录制功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterRecorderDVB(void);

/**
 * \brief   注册播放器 Recoder 模块 FM 录制功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterRecorderFM(void);

/**
 * \brief   注册播放器 DVBSource 模块 TS Buffer Delay 功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDVBSourceTSBuff(void);

/**
 * \brief   注册播放器 DVBSource 模块 TS Cache Delay 功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDVBSourceTSCache(void);

/**
 * \brief   注册播放器 DVBSource 模块普通功能.
 *
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterDVBSourceNormal(void);

/**
 * \brief   注册播放器音频采样系数.
 *
 * \retval  void.
 */
extern void GxPlayer_MoudleRegisterAudioResampler(void);

/**
 * \brief   注册播放器 Stream 模块的 rist 功能.
 * \details 1. V2 版本.
 * \details 2. 注册ffmpeg rist 协议
 * \details 3. 协议介绍请参考: [rist](../background/background.md#rist)
 *
 * \param   void.
 * \retval  void.
 */
extern void GxPlayer_ModuleRegisterStreamLIBRIST_V2(void);

__END_DECLS

#endif
/** @}*/
