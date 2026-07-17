if (CONFIG_FFMPEG_ENABLE)

	list(APPEND SRC_AVFORMAT
		avformat/codec_desc.c
		avformat/allformats.c
		avformat/avcodec.c
		avformat/avio.c
		avformat/aviobuf.c
		avformat/parser.c
		avformat/raw.c
		avformat/riff.c
		avformat/cutils.c
		avformat/utils.c
		avformat/avpacket.c
		avformat/options.c
		avformat/aacdec.c
		avformat/sbcdec.c
		avformat/h264dec.c
		avformat/abr.c
	)

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFCONCAT)
        list(APPEND SRC_AVFORMAT
			avformat/concat.c
			avformat/concatdec.c
        )
	endif()

	if (CONFIG_DEMUX_FLV)
        list(APPEND SRC_AVFORMAT
			avformat/flv.c
        )
	endif()

	if (CONFIG_STREAM_FILE)
        list(APPEND SRC_AVFORMAT
			avformat/file.c
        )
	endif()

	if (CONFIG_DEMUX_APE)
        list(APPEND SRC_AVFORMAT
			avformat/ape.c
        )
	endif()

	if (CONFIG_DEMUX_MKV)
        list(APPEND SRC_AVFORMAT
			avformat/rm.c
			avformat/rmdec.c
			avformat/isom.c
			avformat/matroska.c
			avformat/matroskadec.c
        )
	endif()

	if (CONFIG_DEMUX_AVI)
        list(APPEND SRC_AVFORMAT
			avformat/avidec.c
			avformat/metadata.c
        )
	endif()

	if (CONFIG_DEMUX_OGG)
        list(APPEND SRC_AVFORMAT
			avformat/oggdec.c
			#avformat/oggenc.c
			#avformat/oggparsedirac.c
			#avformat/oggparseogm.c
			#avformat/oggparsespeex.c
			avformat/oggparsevorbis.c
			avformat/oggparseopus.c
			#avformat/oggparseflac.c
			#avformat/oggparseskeleton.c
			#avformat/oggparsetheora.c
			avformat/vorbiscomment.c
			avformat/vorbis_parser.c
			avformat/opus_parser.c
			avformat/xiph.c
			avformat/replaygain.c
			#avformat/flac.c
			#avformat/metadata.c
			#avformat/riff.c
        )
	endif()

	if (CONFIG_DEMUX_MPEG_PS)
        list(APPEND SRC_AVFORMAT
			avformat/mpeg.c
			avformat/mpegvideodec.c
			avformat/rawdec.c
			avformat/mpegvideo_parser.c
			avformat/dvd_nav_parser.c
        )
	endif()

	if (CONFIG_DEMUX_MOV)
        list(APPEND SRC_AVFORMAT
			avformat/mov.c
			avformat/dovi_isom.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE)
        list(APPEND SRC_AVFORMAT
			avformat/rdt.c
			avformat/udp.c
			avformat/tcp.c
			avformat/network.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND (CONFIG_STREAM_HTTP OR CONFIG_STREAM_FFHTTP))
        list(APPEND SRC_AVFORMAT
			avformat/http.c
			avformat/httpauth.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFRIST)
        list(APPEND SRC_AVFORMAT
			avformat/librist.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND (CONFIG_STREAM_FFRTSP OR CONFIG_STREAM_FFRTP OR CONFIG_STREAM_FFSRTP OR CONFIG_STREAM_RTSP))
        list(APPEND SRC_AVFORMAT
			avformat/rtspdec.c
			avformat/rtsp.c
			avformat/rtp.c
			avformat/rtpdec.c
			#avformat/rtpdec_ac3.c
			#avformat/rtpdec_amr.c
			#aavformat/rtpdec_asf.c
			#avformat/rtpdec_dv.c
			avformat/rtpdec_h263.c
			avformat/rtpdec_h263_rfc2190.c
			avformat/rtpdec_h264.c
			avformat/rtpdec_hevc.c
			#avformat/rtpdec_ilbc.c
			avformat/rtpdec_latm.c
			#avformat/rtpdec_mpa_robust.c
			avformat/rtpdec_mpeg4.c
			avformat/rtpdec_mpeg12.c
			avformat/rtpdec_mpegts.c
			#avformat/rtpdec_qcelp.c
			#avformat/rtpdec_qdm2.c
			#avformat/rtpdec_rfc4175.c
			#avformat/rtpdec_svq3.c
			avformat/rtpproto.c
			avformat/rtpsdp.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_MPEGDASH)
        list(APPEND SRC_AVFORMAT
			avformat/dashdec.c
			avformat/dash.c
			avformat/ttml_parse.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFHLS)
        list(APPEND SRC_AVFORMAT
			avformat/hls.c
			avformat/hls_sample_encryption.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFMMS)
        list(APPEND SRC_AVFORMAT
			avformat/asf.c
			avformat/asfdec.c
			avformat/asfcrypt.c
			avformat/mms.c
			avformat/mmsh.c
			avformat/mmst.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFRTMP)
        list(APPEND SRC_AVFORMAT
			avformat/rtmpproto.c
			avformat/rtmpdigest.c
			avformat/rtmppkt.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFSRTP)
        list(APPEND SRC_AVFORMAT
			avformat/srtp.c
			avformat/srtpproto.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFRTMPHTTP AND CONFIG_STREAM_FFRTMP)
        list(APPEND SRC_AVFORMAT
			avformat/rtmphttp.c
        )
	endif()

	if (CONFIG_STREAM_FFDATA)
        list(APPEND SRC_AVFORMAT
			avformat/data_uri.c
        )
	endif()

	if (CONFIG_STREAM_FFCRYPTO OR CONFIG_STREAM_FFHLS)
        list(APPEND SRC_AVFORMAT
			avformat/crypto.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFCOMPOSE)
        list(APPEND SRC_AVFORMAT
			avformat/compose.c
        )
	endif()

	if (CONFIG_DEMUX_TS)
        list(APPEND SRC_AVFORMAT
			avformat/mpegts.c
			avformat/seek.c
			avformat/mpegtsenc.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_DEMUX_TLS)
        list(APPEND SRC_AVFORMAT
			avformat/tls.c
        )
	endif()

	if (CONFIG_DEMUX_MP3)
        list(APPEND SRC_AVFORMAT
			avformat/mp3dec.c
			avformat/mpegaudio_parser.c
			avformat/id3v1.c
			avformat/id3v2.c
        )
	endif()

	if (CONFIG_DEMUX_AC3)
        list(APPEND SRC_AVFORMAT
			avformat/ac3dec.c
        )
	endif()

	if (CONFIG_DEMUX_DTS)
        list(APPEND SRC_AVFORMAT
			avformat/dtsdec.c
			avformat/dtshddec.c
        )
	endif()

	if (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_FFASSOCIATIONLIBRE)
        list(APPEND SRC_AVFORMAT
			avformat/associationlibre.c
        )
	endif()

	if (CONFIG_DEMUX_FLAC)
        list(APPEND SRC_AVFORMAT
			avformat/flacdec.c
			avformat/flac_picture.c
        )
	endif()

	if (CONFIG_DEMUX_WAV)
        list(APPEND SRC_AVFORMAT
			avformat/wavdec.c
			avformat/w64.c
        )
	endif()
endif()
