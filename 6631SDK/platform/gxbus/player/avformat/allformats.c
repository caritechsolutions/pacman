/*
*  Register all the formats and protocols
*  Copyright (c) 2000, 2001, 2002 Fabrice Bellard
*
*  This file is part of FFmpeg.
*
*  FFmpeg is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  FFmpeg is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with FFmpeg; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include "avformat.h"
#include"gx_common.h"

#define REGISTER_MUXER(X,x) {                            \
          extern AVOutputFormat ff_##x##_muxer;          \
          av_register_output_format(&ff_##x##_muxer); }

#define REGISTER_DEMUXER(X,x) {                          \
          extern AVInputFormat x##_demuxer;              \
         av_register_input_format(&x##_demuxer); }

#define REGISTER_PROTOCOL(X,x) {                         \
          extern URLProtocol gx_##x##_protocol;          \
          register_protocol(&gx_##x##_protocol); }

#define REGISTER_PARSER(X, x) {                          \
        extern AVCodecParser ff_##x##_parser;            \
        if (CONFIG_##X##_PARSER)                         \
            av_register_codec_parser(&ff_##x##_parser); }

void avcodec_register_all(void)
{
#define CONFIG_MPEGVIDEO_PARSER (1)
#define CONFIG_MPEGAUDIO_PARSER (1)
#define CONFIG_DVD_NAV_PARSER (1)
#define CONFIG_OPUS_PARSER (1)
#define CONFIG_VORBIS_PARSER (1)
    static int initialized;

    if (initialized)
        return;
    initialized = 1;

	REGISTER_PARSER(MPEGVIDEO,mpegvideo);
	REGISTER_PARSER(MPEGAUDIO,mpegaudio);
	REGISTER_PARSER(DVD_NAV,dvd_nav);
	REGISTER_PARSER(OPUS,opus);
	REGISTER_PARSER(VORBIS,vorbis);
}


/**
*  Initialize libavformat and register all the (de)muxers and protocols.
*/
#define AV_STREAM_REGISTER(X,x)  do {            \
	static unsigned char av_stream_##name = 0;   \
	if (!av_stream_##name) {                     \
		avcodec_register_all();                  \
		REGISTER_PROTOCOL(X, x);                 \
		av_stream_##name = 1;                    \
	}                                            \
} while(0)

#define AV_DEMUX_REGISTER(X,x)  do {             \
	static unsigned char av_demuxer_##name = 0;  \
	if (!av_demuxer_##name) {                    \
		avcodec_register_all();                  \
		REGISTER_DEMUXER(X, x);                  \
		av_demuxer_##name = 1;                   \
	}                                            \
} while(0)

#define AV_MUX_REGISTER(X,x)  do {               \
	static unsigned char av_muxer_##name = 0;    \
	if (!av_muxer_##name) {                      \
		avcodec_register_all();                  \
		REGISTER_MUXER(X, x);                    \
		av_muxer_##name = 1;                     \
	}                                            \
} while(0)

void av_stream_register_file(void)
{
#ifdef CONFIG_STREAM_FILE
	AV_STREAM_REGISTER(FILE, file);
#endif
}

void av_stream_register_rtp(void)
{
#if ((defined CONFIG_STREAM_FFRTP) || (defined CONFIG_STREAM_FFRTSP) || (defined CONFIG_STREAM_SRTP) || (defined CONFIG_STREAM_RTP)) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(RTP, rtp);
	AV_DEMUX_REGISTER(RTP, rtp);
#endif
}

void av_stream_register_srtp(void)
{
#if (defined CONFIG_STREAM_SRTP) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(SRTP, srtp);
	av_stream_register_rtp();
#endif
}

void av_stream_register_tcp(void)
{
#if ((defined CONFIG_STREAM_TCP) || (defined CONFIG_STREAM_FFRTSP)) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(TCP, tcp);
#endif
}

void av_stream_register_udp(void)
{
#if ((defined CONFIG_STREAM_UDP) || (defined CONFIG_STREAM_FFUDP) || (defined CONFIG_STREAM_FFRTSP)) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(UDP, udp);
	AV_STREAM_REGISTER(UDPLITE, udplite);
#endif
}

void av_stream_register_rtsp(void)
{
#if ((defined CONFIG_STREAM_RTSP) || (defined CONFIG_STREAM_FFRTSP)) && (defined CONFIG_IPTV_ENABLE)
	AV_DEMUX_REGISTER(RTSP, rtsp);
	av_stream_register_rtp();
	av_stream_register_udp();
	av_stream_register_tcp();
#endif
}

void av_stream_register_rtpsdp(void)
{
#if ((defined CONFIG_STREAM_RTSP) || (defined CONFIG_STREAM_FFRTSP)) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(RTSPSDP, rtpsdp);
	AV_DEMUX_REGISTER(SDP, sdp);
#endif
}

void av_stream_register_compose(void)
{
#if (defined CONFIG_STREAM_FFCOMPOSE) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(COMPOSE, compose);
#endif
}

void av_stream_register_http(void)
{
#if (defined CONFIG_IPTV_ENABLE)
	av_stream_register_tcp();
#ifdef CONFIG_DEMUX_TLS
	AV_STREAM_REGISTER(TLS, tls);
#endif
#if (defined CONFIG_STREAM_HTTP) || (defined CONFIG_STREAM_FFHTTP)
	AV_STREAM_REGISTER(HTTP,      http);
	AV_STREAM_REGISTER(HTTPS,     https);
	AV_STREAM_REGISTER(HTTPPROXY, httpproxy);
#endif
#endif
}

void av_stream_register_librist(void)
{
#if (defined CONFIG_IPTV_ENABLE) && (defined CONFIG_STREAM_FFRIST)
	AV_STREAM_REGISTER(LIBRIST, librist);
#endif
}

void av_stream_register_mpegdash(void)
{
#ifdef CONFIG_IPTV_ENABLE
	av_stream_register_http();
#ifdef CONFIG_STREAM_MPEGDASH
	AV_DEMUX_REGISTER(DASH, dash);
#endif
#endif
}

void av_stream_register_mms(void)
{
#if (defined CONFIG_STREAM_FFMMS) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(MMSH, mmsh);
	AV_STREAM_REGISTER(MMST, mmst);
	AV_DEMUX_REGISTER(ASF,   asf);
#endif
}

void av_stream_register_ffcrypto(void)
{
#if (defined CONFIG_STREAM_FFCRYPTO) || (defined CONFIG_STREAM_FFHLS)
	AV_STREAM_REGISTER(CRYPTO,     crypto);
#endif
}

void av_stream_register_ffdata(void)
{
#ifdef CONFIG_STREAM_FFDATA
	AV_STREAM_REGISTER(DATA,     data);
#endif
}

void av_stream_register_ffhls(void)
{
#if (defined CONFIG_STREAM_FFHLS) && (defined CONFIG_IPTV_ENABLE)
	av_stream_register_ffcrypto();
	AV_DEMUX_REGISTER(HLS, hls);
#endif
}

void av_stream_register_concat(void)
{
#if (defined CONFIG_STREAM_FFCONCAT) && (defined CONFIG_IPTV_ENABLE)
	AV_STREAM_REGISTER(CONCAT,      concat);
	AV_DEMUX_REGISTER(CONCAT,       concat);
#endif
}

void av_stream_register_associationlibre(void)
{
#if (defined CONFIG_STREAM_FFASSOCIATIONLIBRE) && (defined CONFIG_IPTV_ENABLE)
	AV_DEMUX_REGISTER(ASSOCIATIONLIBRE, associationlibre);
#endif
}

void av_stream_register_ffrtmp(void)
{
#ifdef CONFIG_IPTV_ENABLE
#ifdef CONFIG_STREAM_FFRTMP
	//AV_STREAM_REGISTER(TCP,      tcp);
	AV_STREAM_REGISTER(RTMP,     rtmp);
#ifdef CONFIG_DEMUX_TLS
	AV_STREAM_REGISTER(RTMP,     rtmps);
#endif
#ifdef CONFIG_STREAM_FFRTMPHTTP
	AV_STREAM_REGISTER(RTMPT,     rtmpt);
	AV_STREAM_REGISTER(FFRTMPHTTP,     ffrtmphttp);
#ifdef CONFIG_DEMUX_TLS
	AV_STREAM_REGISTER(RTMPTS,     rtmpts);
#endif
#endif
#endif
#endif
}

void av_demux_register_mov(void)
{
#ifdef CONFIG_DEMUX_MOV
	AV_DEMUX_REGISTER(MOV, mov);
#endif
}

void av_demux_register_flv(void)
{
#ifdef CONFIG_DEMUX_FLV
	AV_DEMUX_REGISTER(FLV, flv);
#endif
}

void av_demux_register_avi(void)
{
#ifdef CONFIG_DEMUX_AVI
	AV_DEMUX_REGISTER(AVI, avi);
#endif
}

void av_demux_register_ape(void)
{
#ifdef CONFIG_DEMUX_APE
	AV_DEMUX_REGISTER(APE, ape);
#endif
}

void av_demux_register_mkv(void)
{
#ifdef CONFIG_DEMUX_MKV
	AV_DEMUX_REGISTER(MKV, mkv);
#endif
}

void av_demux_register_aac(void)
{
#ifdef CONFIG_DEMUX_AAC
	AV_DEMUX_REGISTER(AAC, aac);
#endif
}

void av_demux_register_mpegps(void)
{
#ifdef CONFIG_DEMUX_MPEG_PS
	AV_DEMUX_REGISTER(MPEGPS,    mpegps);
	AV_DEMUX_REGISTER(MPEGVIDEO, mpegvideo);
#endif
}

void av_demux_register_mpegts(void)
{
#if (defined CONFIG_DEMUX_TS) && (defined CONFIG_DEMUX_MPEG_TS)
	AV_DEMUX_REGISTER(MPEGTS,    mpegts);
	AV_DEMUX_REGISTER(MPEGTSRAW, mpegtsraw);
	AV_DEMUX_REGISTER(H264, h264);
#endif
}

void av_demux_register_ogg(void)
{
#ifdef CONFIG_DEMUX_OGG
	AV_DEMUX_REGISTER(OGG, ogg);
#endif
}

void av_demux_register_dolby(void)
{
#ifdef CONFIG_DEMUX_AC3
	AV_DEMUX_REGISTER(DOBLY, ac3);
	AV_DEMUX_REGISTER(DOBLY, eac3);
#endif
}

void av_demux_register_dts(void)
{
#ifdef CONFIG_DEMUX_DTS
	AV_DEMUX_REGISTER(DTS, dts);
	AV_DEMUX_REGISTER(DTS, dtshd);
#endif
}

void av_demux_register_ffmp3(void)
{
#ifdef CONFIG_DEMUX_MP3
	AV_DEMUX_REGISTER(MP3, ffmp3);
#endif
}

void av_demux_register_sbc(void)
{
#ifdef CONFIG_DEMUX_SBC
	AV_DEMUX_REGISTER(SBC, sbc);
#endif
}

void av_demux_register_flac(void)
{
#ifdef CONFIG_DEMUX_FLAC
	AV_DEMUX_REGISTER(FLAC, flac);
#endif
}

void av_demux_register_wav(void)
{
#ifdef CONFIG_DEMUX_WAV
	AV_DEMUX_REGISTER(WAV, wav);
#endif
}

void av_demux_register_w64(void)
{
#ifdef CONFIG_DEMUX_WAV
	AV_DEMUX_REGISTER(W64, w64);
#endif
}

void av_mux_register_rawvideo(void)
{
#ifdef CONFIG_MUX_RAWVIDEO
	AV_MUX_REGISTER(RAWVIDEO, rawvideo);
#endif
}

void av_mux_register_mpegts(void)
{
#ifdef CONFIG_MUX_MPEGTS
	AV_MUX_REGISTER(MPEGTS, mpegts);
#endif
}

