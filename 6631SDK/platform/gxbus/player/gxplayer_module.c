#include "gx_demux.h"
#include "gx_common.h"
#include "gxplayer_decoder.h"
#include "avformat/avformat.h"
#include "gxplayer_internal.h"
#include "audio_resample.h"

#define FF_STREAM_REGISTER(name)   do {      \
	extern void av_stream_register_##name(); \
	av_stream_register_##name();             \
} while(0)

#define GX_STREAM_REGISTER(name)   do {      \
	extern GxStreamClass gx_stream_##name;   \
	GxStreamRegister(&gx_stream_##name);     \
} while(0)

#define FF_DEMUXER_REGISTER(name)   do {     \
	extern void av_demux_register_##name();  \
	av_demux_register_##name();              \
} while(0)

#define GX_DEMUXER_REGISTER(name)   do {     \
	extern GxDemuxerClass gx_demux_##name;   \
	GxDemuxerRegister(&gx_demux_##name);     \
} while(0)

#define FF_MUXER_REGISTER(name)   do {       \
	extern void av_mux_register_##name();    \
	av_mux_register_##name();                \
} while(0)

#define GX_SUBTITLE_REGISTER(name)   do {                 \
	extern PISubCtrl subctrl_##name;                      \
	extern void GxPlayerSubtitleRegister(PISubCtrl* cls); \
	GxPlayerSubtitleRegister(&subctrl_##name);            \
} while(0)

#define GX_SUB_DECODER_REGISTER(name)   do {                 \
	extern GxAVCodecClass gx_sub_decoder_##name;                      \
	extern void GxSubtitleDecoderRegister(GxAVCodecClass* cls); \
	GxSubtitleDecoderRegister(&gx_sub_decoder_##name);            \
} while(0)

void GxPlayer_ModuleRegisterStreamHTTP(void)
{
#if (defined CONFIG_STREAM_HTTP) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(http1);
#endif
}

void GxPlayer_ModuleRegisterStreamHTTP_V2(void)
{
#if (defined CONFIG_STREAM_FFHTTP) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffhttp);
	FF_STREAM_REGISTER(http);
#endif
}

void GxPlayer_ModuleRegisterStreamLIBRIST_V2(void)
{
#if (defined CONFIG_IPTV_ENABLE) && (defined CONFIG_STREAM_FFRIST)
	GX_STREAM_REGISTER(ffrist);
	FF_STREAM_REGISTER(librist);
#endif
}

void GxPlayer_ModuleRegisterStreamRTSP(void)
{
#if (defined CONFIG_STREAM_RTSP) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(udp);
	FF_STREAM_REGISTER(rtp);
	FF_STREAM_REGISTER(rtsp);
	GX_STREAM_REGISTER(rtsp);
#endif
}

void GxPlayer_ModuleRegisterStreamRTSP_V2(void)
{
#if (defined CONFIG_STREAM_FFRTSP) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(rtsp);
	GX_STREAM_REGISTER(ffrtsp);
#endif
}

void GxPlayer_ModuleRegisterStreamRTMP(void)
{
#if (defined CONFIG_STREAM_RTMP) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(rtmp);
#endif
}

void GxPlayer_ModuleRegisterStreamRTMP_V2(void)
{
#if (defined CONFIG_STREAM_FFRTMP) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(ffrtmp);
	GX_STREAM_REGISTER(ffrtmp);
#endif
}

void GxPlayer_ModuleRegisterStreamUDP(void)
{
#if (defined CONFIG_STREAM_UDP) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(udp);
#endif
}

void GxPlayer_ModuleRegisterStreamUDP_V2(void)
{
#if (defined CONFIG_STREAM_FFUDP) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(udp);
	GX_STREAM_REGISTER(ffudp);
#endif
}

void GxPlayer_ModuleRegisterStreamRTP(void)
{
#if (defined CONFIG_STREAM_RTP) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(rtp);
#endif
}

void GxPlayer_ModuleRegisterStreamRTP_V2(void)
{
#if (defined CONFIG_STREAM_FFRTP) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(rtp);
	GX_STREAM_REGISTER(ffrtp);
#endif
}

void GxPlayer_ModuleRegisterStreamRTPSDP_V2(void)
{
#if (defined CONFIG_STREAM_FFRTP) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(rtpsdp);
	GX_STREAM_REGISTER(ffrtpsdp);
#endif
}

void GxPlayer_ModuleRegisterStreamM3U8(void)
{
#if (defined CONFIG_STREAM_M3U8) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(m3u8);
#endif
}

void GxPlayer_ModuleRegisterStreamM3U8_V2(void)
{
#if (defined CONFIG_STREAM_FFHLS) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffhls);
	FF_STREAM_REGISTER(ffhls);
#endif
}

void GxPlayer_ModuleRegisterStreamMMS_V2(void)
{
#if (defined CONFIG_STREAM_FFMMS) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(mms);
	GX_STREAM_REGISTER(ffmms);
#endif
}

void GxPlayer_ModuleRegisterStreamMMS(void)
{
#if (defined CONFIG_STREAM_MMS) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(mms);
#endif
}

void GxPlayer_ModuleRegisterStreamSRTP_V2(void)
{
#if (defined CONFIG_STREAM_FFSRTP) && (defined CONFIG_IPTV_ENABLE)
	FF_STREAM_REGISTER(srtp);
	GX_STREAM_REGISTER(ffsrtp);
#endif
}

void GxPlayer_ModuleRegisterStreamWFD(void)
{
#ifdef CONFIG_STREAM_WFD
	GX_STREAM_REGISTER(wfd);
#endif
}

void GxPlayer_ModuleRegisterStreamDVB(void)
{
#ifdef CONFIG_STREAM_DVB
	GX_STREAM_REGISTER(dvb);
	// 为了和之前版本保持兼容, 下面两个自动注册进去.
	GxPlayer_ModuleRegisterDVBSourceTSBuff();
	GxPlayer_ModuleRegisterDVBSourceNormal();
#endif
}

void GxPlayer_ModuleRegisterStreamMEM(void)
{
#ifdef CONFIG_STREAM_MEM
	GX_STREAM_REGISTER(mem);
#endif
}

void GxPlayer_ModuleRegisterStreamFIFO(void)
{
#ifdef CONFIG_STREAM_FIFO
	GX_STREAM_REGISTER(fifo);
#endif
}

void GxPlayer_ModuleRegisterStreamRINGMEM(void)
{
#ifdef CONFIG_STREAM_RINGMEM
	GX_STREAM_REGISTER(ringmem);
#endif
}

void GxPlayer_ModuleRegisterStreamFILE(void)
{
#ifdef CONFIG_STREAM_FILE
	GX_STREAM_REGISTER(file);
#ifdef CONFIG_FFMPEG_ENABLE
	FF_STREAM_REGISTER(file);
#endif
#endif
	GX_STREAM_REGISTER(dvr);
}

void GxPlayer_ModuleRegisterStreamFILE_V2(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_STREAM_FFFILE)
	GX_STREAM_REGISTER(fffile);
#endif
}

void GxPlayer_ModuleRegisterStreamData(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_STREAM_FFDATA)
	GX_STREAM_REGISTER(ffdata);
	FF_STREAM_REGISTER(ffdata);
#endif
}

void GxPlayer_ModuleRegisterStreamConcat_V2(void)
{
#if (defined CONFIG_STREAM_FFCONCAT) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffconcat);
	FF_STREAM_REGISTER(concat);
#endif
}

void GxPlayer_ModuleRegisterStreamMPEGDASH_V2(void)
{
#if (defined CONFIG_STREAM_MPEGDASH) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffdash);
	FF_STREAM_REGISTER(mpegdash);
#endif
}

void GxPlayer_ModuleRegisterStreamMPEGDASH(void)
{
#if (defined CONFIG_STREAM_MPEGDASH) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffmpegdash);
	FF_STREAM_REGISTER(mpegdash);
#endif
}


void GxPlayer_ModuleRegisterStreamSWCRYPTO(void)
{
#if (defined CONFIG_STREAM_CRYPTO) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(crypto);
#endif
}

void GxPlayer_ModuleRegisterStreamSWCRYPTO_V2(void)
{
#if (defined CONFIG_STREAM_FFCRYPTO) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffcrypto);
	FF_STREAM_REGISTER(ffcrypto);
#endif
}

void GxPlayer_ModuleRegisterStreamCompose_V2(void)
{
#if (defined CONFIG_STREAM_FFCOMPOSE) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffcompose);
	FF_STREAM_REGISTER(compose);
#endif
}

void GxPlayer_ModuleRegisterStreamHWCRYPTO(void)
{
#ifdef CONFIG_STREAM_HWCRYPTO
	GX_STREAM_REGISTER(mtc);
#endif
}

void GxPlayer_ModuleRegisterStreamDRMCRYPTO(void)
{
#if (defined CONFIG_STREAM_CRYPTO) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(drm);
#endif
}

void GxPlayer_ModuleRegisterStreamAssociationLibre_V2(void)
{
#if (defined CONFIG_STREAM_FFASSOCIATIONLIBRE) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(ffassociationlibre);
	FF_STREAM_REGISTER(associationlibre);
#endif
}

void GxPlayer_ModuleRegisterStreamAssociationLibre(void)
{
#if (defined CONFIG_STREAM_ASSOCIATIONLIBRE) && (defined CONFIG_IPTV_ENABLE)
	GX_STREAM_REGISTER(associationlibre);
#endif
}

void GxPlayer_ModuleRegisterStreamPVR(void)
{
	extern void GxFileRegisterPVR(void);
	GxFileRegisterPVR();
	GX_STREAM_REGISTER(pvr);
}

void GxPlayer_ModuleRegisterStreamFM(void)
{
#if defined(CONFIG_STREAM_FM)
	GX_STREAM_REGISTER(fm);
#endif
}

void GxPlayer_ModuleRegisterStreamALL(void)
{
#if defined(CONFIG_IPTV_ENABLE)
	GxPlayer_ModuleRegisterStreamHTTP();
	GxPlayer_ModuleRegisterStreamRTSP();
	GxPlayer_ModuleRegisterStreamRTMP();
	GxPlayer_ModuleRegisterStreamUDP();
	GxPlayer_ModuleRegisterStreamRTP();
	GxPlayer_ModuleRegisterStreamM3U8();
	GxPlayer_ModuleRegisterStreamMPEGDASH();
	GxPlayer_ModuleRegisterStreamWFD();
	GxPlayer_ModuleRegisterStreamSWCRYPTO();
	GxPlayer_ModuleRegisterStreamDRMCRYPTO();
	GxPlayer_ModuleRegisterStreamAssociationLibre();
#endif
	GxPlayer_ModuleRegisterStreamPVR();
	GxPlayer_ModuleRegisterStreamDVB();
	GxPlayer_ModuleRegisterStreamMEM();
	GxPlayer_ModuleRegisterStreamFIFO();
	GxPlayer_ModuleRegisterStreamRINGMEM();
	GxPlayer_ModuleRegisterStreamFILE();
	GxPlayer_ModuleRegisterStreamFM();

	GxPlayer_ModuleRegisterStreamHWCRYPTO();
}

void GxPlayer_ModuleRegisterStreamALL_V2(void)
{
	GxPlayer_ModuleRegisterStreamFILE_V2();
#if defined(CONFIG_IPTV_ENABLE)
	GxPlayer_ModuleRegisterStreamHTTP_V2();
	GxPlayer_ModuleRegisterStreamRTSP_V2();
	GxPlayer_ModuleRegisterStreamRTMP_V2();
	GxPlayer_ModuleRegisterStreamUDP_V2();
	GxPlayer_ModuleRegisterStreamRTP_V2();
	GxPlayer_ModuleRegisterStreamM3U8_V2();
	GxPlayer_ModuleRegisterStreamMPEGDASH_V2();
	GxPlayer_ModuleRegisterStreamRTPSDP_V2();
	GxPlayer_ModuleRegisterStreamCompose_V2();
	GxPlayer_ModuleRegisterStreamConcat_V2();
	GxPlayer_ModuleRegisterStreamAssociationLibre_V2();
	GxPlayer_ModuleRegisterStreamLIBRIST_V2();
	GxPlayer_ModuleRegisterStreamDRMCRYPTO();
	GxPlayer_ModuleRegisterStreamWFD();
#endif
	GxPlayer_ModuleRegisterStreamDVB();
	GxPlayer_ModuleRegisterStreamMEM();
	GxPlayer_ModuleRegisterStreamFIFO();
	GxPlayer_ModuleRegisterStreamRINGMEM();
	GxPlayer_ModuleRegisterStreamFILE();
	GxPlayer_ModuleRegisterStreamFM();
	GxPlayer_ModuleRegisterStreamPVR();

	GxPlayer_ModuleRegisterStreamSWCRYPTO_V2();
	GxPlayer_ModuleRegisterStreamHWCRYPTO();
}

void GxPlayer_ModuleRegisterDemuxerMP3(void)
{
#ifdef CONFIG_DEMUX_MPEG_AUDIO
	GX_DEMUXER_REGISTER(audio);
#ifdef CONFIG_FFMPEG_ENABLE
	FF_DEMUXER_REGISTER(ffmp3);
#endif
#endif
}

void GxPlayer_ModuleRegisterDemuxerMP4(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_MOV)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(mov);
#endif
}

void GxPlayer_ModuleRegisterDemuxerFLAC(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_FLAC)
	FF_DEMUXER_REGISTER(flac);
#endif
}

void GxPlayer_ModuleRegisterDemuxerWAV(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_WAV)
	FF_DEMUXER_REGISTER(wav);
	FF_DEMUXER_REGISTER(w64);
#endif
}

void GxPlayer_ModuleRegisterDemuxerAVI(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_AVI)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(avi);
#endif
}

void GxPlayer_ModuleRegisterDemuxerMKV(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_MKV)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(mkv);
#endif
}

void GxPlayer_ModuleRegisterDemuxerFLV(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_FLV)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(flv);
#endif
}

void GxPlayer_ModuleRegisterDemuxerAAC(void)
{
#ifdef CONFIG_DEMUX_AAC
	GX_DEMUXER_REGISTER(aac);
#ifdef CONFIG_FFMPEG_ENABLE
	FF_DEMUXER_REGISTER(aac);
#endif
#endif
}

void GxPlayer_ModuleRegisterDemuxerES(void)
{
#ifdef CONFIG_DEMUX_ES
	GX_DEMUXER_REGISTER(es);
#endif
}

void GxPlayer_ModuleRegisterDemuxerLOGO(void)
{
#ifdef CONFIG_DEMUX_ES
	GX_DEMUXER_REGISTER(es);
#endif
}

void GxPlayer_ModuleRegisterDemuxerRMVB(void)
{
#ifdef CONFIG_DEMUX_RMVB
	GX_DEMUXER_REGISTER(rmvb);
#endif
}

void GxPlayer_ModuleRegisterDemuxerSWTS(void)
{
#ifdef CONFIG_DEMUX_MPEG_TS
#ifdef CONFIG_FFMPEG_ENABLE
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(mpegts);
#endif
	GX_DEMUXER_REGISTER(ts);
#endif
}

void GxPlayer_ModuleRegisterDemuxerHWTS(void)
{
#ifdef CONFIG_DEMUX_HW
	GX_DEMUXER_REGISTER(hw);
	GX_DEMUXER_REGISTER(hwts);
#endif
}

void GxPlayer_ModuleRegisterDemuxerREAL(void)
{
#ifdef CONFIG_DEMUX_REAL
	GX_DEMUXER_REGISTER(real);
#endif
}

void GxPlayer_ModuleRegisterDemuxerPS(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_MPEG_PS)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(mpegps);
#endif
}

void GxPlayer_ModuleRegisterDemuxerOGG(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_OGG)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(ogg);
#endif
}

void GxPlayer_ModuleRegisterDemuxerDOLBY(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_AC3)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(dolby);
#endif
}

void GxPlayer_ModuleRegisterDemuxerDTS(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_DTS)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(dts);
#endif
}

void GxPlayer_ModuleRegisterDemuxerAPE(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_APE)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(ape);
#endif
}

void GxPlayer_ModuleRegisterDemuxerSBC(void)
{
#if defined(CONFIG_FFMPEG_ENABLE) && defined(CONFIG_DEMUX_SBC)
	GX_DEMUXER_REGISTER(lavf);
	FF_DEMUXER_REGISTER(sbc);
#endif
}

void GxPlayer_ModuleRegisterDemuxerFM(void)
{
#ifdef CONFIG_DEMUX_FM
	GX_DEMUXER_REGISTER(fm);
#endif
}

void GxPlayer_ModuleRegisterDemuxerAssociationLibre(void)
{
#if defined(CONFIG_DEMUX_ASSOCIATIONLIBRE) && defined(CONFIG_FFMPEG_ENABLE)
	GX_DEMUXER_REGISTER(associationlibre);
#endif
}

void GxPlayer_ModuleRegisterDemuxerALL(void)
{
#ifdef CONFIG_DEMUX_MPEG_AUDIO
	GxPlayer_ModuleRegisterDemuxerMP3();
#endif
#ifdef CONFIG_DEMUX_MOV
	GxPlayer_ModuleRegisterDemuxerMP4();
#endif
#ifdef CONFIG_DEMUX_AVI
	GxPlayer_ModuleRegisterDemuxerAVI();
#endif
#ifdef CONFIG_DEMUX_MKV
	GxPlayer_ModuleRegisterDemuxerMKV();
#endif
#ifdef CONFIG_DEMUX_FLV
	GxPlayer_ModuleRegisterDemuxerFLV();
#endif
#ifdef CONFIG_DEMUX_AAC
	GxPlayer_ModuleRegisterDemuxerAAC();
#endif
#ifdef CONFIG_DEMUX_ES
	GxPlayer_ModuleRegisterDemuxerES();
	GxPlayer_ModuleRegisterDemuxerLOGO();
#endif
#ifdef CONFIG_DEMUX_RMVB
	GxPlayer_ModuleRegisterDemuxerRMVB();
#endif
#ifdef CONFIG_DEMUX_MPEG_TS
	GxPlayer_ModuleRegisterDemuxerSWTS();
#endif
#ifdef CONFIG_DEMUX_HW
	GxPlayer_ModuleRegisterDemuxerHWTS();
#endif
#ifdef CONFIG_DEMUX_REAL
	GxPlayer_ModuleRegisterDemuxerREAL();
#endif
#ifdef CONFIG_DEMUX_MPEG_PS
	GxPlayer_ModuleRegisterDemuxerPS();
#endif
#ifdef CONFIG_DEMUX_OGG
	GxPlayer_ModuleRegisterDemuxerOGG();
#endif
#ifdef CONFIG_DEMUX_AC3
	GxPlayer_ModuleRegisterDemuxerDOLBY();
#endif
#ifdef CONFIG_DEMUX_DTS
	GxPlayer_ModuleRegisterDemuxerDTS();
#endif
#ifdef CONFIG_DEMUX_APE
	GxPlayer_ModuleRegisterDemuxerAPE();
#endif
#ifdef CONFIG_DEMUX_SBC
	GxPlayer_ModuleRegisterDemuxerSBC();
#endif
#ifdef CONFIG_DEMUX_FM
	GxPlayer_ModuleRegisterDemuxerFM();
#endif
#ifdef CONFIG_DEMUX_FLAC
	GxPlayer_ModuleRegisterDemuxerFLAC();
#endif
#ifdef CONFIG_DEMUX_WAV
	GxPlayer_ModuleRegisterDemuxerWAV();
#endif
}

void GxPlayer_ModuleRegisterMuxerRAWVIDEO(void)
{
#ifdef CONFIG_MUX_RAWVIDEO
	FF_MUXER_REGISTER(rawvideo);
#endif
}

void GxPlayer_ModuleRegisterMuxerMPEGTS(void)
{
#ifdef CONFIG_MUX_MPEGTS
	FF_MUXER_REGISTER(mpegts);
#endif
}

void GxPlayer_ModuleRegisterMuxerALL(void)
{
#ifdef CONFIG_MUX_RAWVIDEO
	GxPlayer_ModuleRegisterMuxerRAWVIDEO();
#endif
#ifdef CONFIG_MUX_MPEGTS
	GxPlayer_ModuleRegisterMuxerMPEGTS();
#endif
}

void GxPlayer_ModuleRegisterSubtitleDVB(void)
{
#ifdef CONFIG_SUBTITLE_DVB
	GX_SUBTITLE_REGISTER(dvb);
#endif
}

void GxPlayer_ModuleRegisterSubDecoderText(void)
{
#ifdef CONFIG_SUBTITLE_MEDIA
	GX_SUB_DECODER_REGISTER(text);
#endif
}

void GxPlayer_ModuleRegisterSubDecoderVob(void)
{
#ifdef CONFIG_SUBTITLE_MEDIA
	GX_SUB_DECODER_REGISTER(vob);
#endif
}

void GxPlayer_ModuleRegisterSubDecoderTtml(void)
{
#if (defined CONFIG_SUBTITLE_TTML) && (defined CONFIG_IPTV_ENABLE)
	GX_SUB_DECODER_REGISTER(ttml);
#endif
}

void GxPlayer_ModuleRegisterSubtitleAribCC(void)
{
#ifdef CONFIG_SUBTITLE_ARIBCC
	GX_SUBTITLE_REGISTER(aribcc);
#endif
}

void GxPlayer_ModuleRegisterSubtitleAtscCC(void)
{
#ifdef CONFIG_SUBTITLE_ATSCCC
	extern void vbi_caption_register(void);

	vbi_caption_register();
	GX_SUBTITLE_REGISTER(atsccc);
#endif
}

void GxPlayer_ModuleRegisterSubtitleScte(void)
{
#ifdef CONFIG_SUBTITLE_SCTE
	GX_SUBTITLE_REGISTER(scte);
#endif
}

void GxPlayer_ModuleRegisterSubtitleTeletext(void)
{
#ifdef CONFIG_SUBTITLE_TELETEXT
	extern void vbi_teletext_register(void);

	vbi_teletext_register();
	GX_SUBTITLE_REGISTER(teletext);
	GX_SUBTITLE_REGISTER(magazine);
#endif
}

void GxPlayer_ModuleRegisterSubtitleInside(void)
{
#ifdef CONFIG_SUBTITLE_MEDIA
	GX_SUBTITLE_REGISTER(inside);
	GX_SUB_DECODER_REGISTER(text);
	GX_SUB_DECODER_REGISTER(vob);
#endif
}

void GxPlayer_ModuleRegisterSubtitleOutside(void)
{
#ifdef CONFIG_SUBTITLE_MEDIA
	GX_SUBTITLE_REGISTER(outside);
#endif
}

void GxPlayer_ModuleRegisterCENCSchemeDecrypt(void)
{
#if defined(CONFIG_FFMPEG_ENABLE)
	extern void av_cenc_scheme_decrypt_regiseter(void);
	av_cenc_scheme_decrypt_regiseter();
#endif
}

void GxPlayer_ModuleRegisterSubtitleALL(void)
{
#ifdef CONFIG_SUBTITLE_DVB
	GxPlayer_ModuleRegisterSubtitleDVB();
#endif
#ifdef CONFIG_SUBTITLE_ARIBCC
	GxPlayer_ModuleRegisterSubtitleAribCC();
#endif
#ifdef CONFIG_SUBTITLE_ATSCCC
	GxPlayer_ModuleRegisterSubtitleAtscCC();
#endif
#ifdef CONFIG_SUBTITLE_SCTE
	GxPlayer_ModuleRegisterSubtitleScte();
#endif
#ifdef CONFIG_SUBTITLE_TELETEXT
	GxPlayer_ModuleRegisterSubtitleTeletext();
#endif
#ifdef CONFIG_SUBTITLE_MEDIA
	GxPlayer_ModuleRegisterSubtitleInside();
	GxPlayer_ModuleRegisterSubtitleOutside();
#endif
#if defined(CONFIG_IPTV_ENABLE) && defined(CONFIG_STREAM_MPEGDASH) && defined(CONFIG_SUBTITLE_TTML)
	GxPlayer_ModuleRegisterSubDecoderTtml();
#endif

}

void GxPlayer_ModuleRegisterDecoderSWAPE(void)
{
#ifdef CONFIG_AD_SW_APE
	extern GxAudioDecoder ape_decoder;
	extern void GxPlayerSwDecoderRegister(GxAudioDecoder *adec);
	GxPlayerSwDecoderRegister(&ape_decoder);
#endif
}

extern GxDumpFilterClass gx_dumpfilter_dvb;
extern GxDumpFilterClass gx_dumpfilter_fm;
extern void GxDumpFilterRegister(GxDumpFilterClass* cls);
void GxPlayer_ModuleRegisterRecorderDVB(void)
{
	GxDumpFilterRegister(&gx_dumpfilter_dvb);
}

void GxPlayer_ModuleRegisterRecorderFM(void)
{
	GxDumpFilterRegister(&gx_dumpfilter_fm);
}

void GxPlayer_ModuleRegisterALL(void)
{
	GxPlayer_ModuleRegisterStreamALL();
	GxPlayer_ModuleRegisterDemuxerALL();
	GxPlayer_ModuleRegisterMuxerALL();
	GxPlayer_ModuleRegisterSubtitleALL();
	GxPlayer_ModuleRegisterDecoderSWAPE();
	GxPlayer_ModuleRegisterRecorderDVB();
	GxPlayer_ModuleRegisterRecorderFM();
	GxPlayer_MoudleRegisterAudioResampler();
}

void GxPlayer_ModuleRegisterALL_V2(void)
{
	GxPlayer_ModuleRegisterStreamALL_V2();
	GxPlayer_ModuleRegisterDemuxerALL();
	GxPlayer_ModuleRegisterMuxerALL();
	GxPlayer_ModuleRegisterSubtitleALL();
	GxPlayer_ModuleRegisterDecoderSWAPE();
	GxPlayer_ModuleRegisterRecorderDVB();
	GxPlayer_ModuleRegisterRecorderFM();
	GxPlayer_MoudleRegisterAudioResampler();
}

#include "access/dvbsource.h"
extern DVBSourceClass dvbsource_tsbuff;
extern DVBSourceClass dvbsource_tscache;
extern DVBSourceClass dvbsource_normal;
extern void GxDVBSourceRegister(DVBSourceClass * cls);
void GxPlayer_ModuleRegisterDVBSourceTSBuff(void)
{
	GxDVBSourceRegister(&dvbsource_tsbuff);
}

void GxPlayer_ModuleRegisterDVBSourceTSCache(void)
{
	GxDVBSourceRegister(&dvbsource_tscache);
}

void GxPlayer_ModuleRegisterDVBSourceNormal(void)
{
	GxDVBSourceRegister(&dvbsource_normal);
}

void GxPlayer_MoudleRegisterAudioResampler(void)
{
	gx_resample_register();
}
