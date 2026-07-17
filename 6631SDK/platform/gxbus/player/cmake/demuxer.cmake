SET(SRC_DEMUXER demuxer/demux.c
	demuxer/demux_stream.c
	demuxer/demux_dump.c
	demuxer/demux_packet.c
	demuxer/demux_pool.c
	demuxer/video.c
	demuxer/parse_es.c
	demuxer/mpeg_header.c
	demuxer/iframe_probe.c
	)

IF (CONFIG_DEMUX_MPEG_AUDIO)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/demux_audio.c
		demuxer/bluray_pcm.c
		demuxer/adpcm.c
		demuxer/adpcm_data.c
		)
ENDIF (CONFIG_DEMUX_MPEG_AUDIO)

IF (CONFIG_DEMUX_HW)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/demux_hw.c
		demuxer/demux_pes.c
		demuxer/hw_demux.c
		demuxer/ad_demux.c
		)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_hwts.c)
ENDIF (CONFIG_DEMUX_HW)

IF (CONFIG_DEMUX_MOV)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/demux_mov.c
		demuxer/parse_mp4.c
		)
ENDIF (CONFIG_DEMUX_MOV)

IF (CONFIG_DEMUX_MKV)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		#demuxer/demux_mkv.c
		#demuxer/ebml.c
		#demuxer/xtr_avc.c
		)
ENDIF (CONFIG_DEMUX_MKV)

IF (CONFIG_DEMUX_ASSOCIATIONLIBRE)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/demux_associationlibre.c
		)
ENDIF (CONFIG_DEMUX_ASSOCIATIONLIBRE)

IF (CONFIG_DEMUX_MPEG_TS)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_ts.c)
ENDIF (CONFIG_DEMUX_MPEG_TS)

IF (CONFIG_DEMUX_AAC)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_aac.c)
ENDIF (CONFIG_DEMUX_AAC)

IF (CONFIG_DEMUX_DRA)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_dra.c)
ENDIF (CONFIG_DEMUX_DRA)

IF (CONFIG_DEMUX_ES)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_mpg.c)
ENDIF (CONFIG_DEMUX_ES)

IF (CONFIG_DEMUX_REAL)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_real.c)
ENDIF (CONFIG_DEMUX_REAL)

IF (CONFIG_DEMUX_LIVE555)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/demux_live555_rtp.c
		demuxer/demux_rtp.cpp
		demuxer/demux_rtp_codec.cpp
		)
ENDIF (CONFIG_DEMUX_LIVE555)

IF (CONFIG_DEMUX_RMVB)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/demux_rmvb.c
		)
	AUX_SOURCE_DIRECTORY( demuxer/rmvb/rm_parser  SRC_DEMUXER )
	AUX_SOURCE_DIRECTORY( demuxer/rmvb/rm_common  SRC_DEMUXER )
	AUX_SOURCE_DIRECTORY( demuxer/rmvb/rv_depack  SRC_DEMUXER )
	AUX_SOURCE_DIRECTORY( demuxer/rmvb/ra_depack  SRC_DEMUXER )
	INCLUDE_DIRECTORIES ( demuxer/rmvb/rm_include             )
ENDIF (CONFIG_DEMUX_RMVB)

IF (CONFIG_DEMUX_ASF)
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/demux_asf.c
		demuxer/asf_header.c
		)
ENDIF (CONFIG_DEMUX_ASF)

IF (CONFIG_DEMUX_DOLBY)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_dolby.c)
ENDIF (CONFIG_DEMUX_DOLBY)

IF (CONFIG_DEMUX_ES)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_es.c)
ENDIF (CONFIG_DEMUX_ES)

IF (CONFIG_FFMPEG_ENABLE AND (CONFIG_DEMUX_FLV OR CONFIG_DEMUX_AVI OR CONFIG_DEMUX_APE OR CONFIG_DEMUX_OGG))
	SET(SRC_DEMUXER ${SRC_DEMUXER}
		demuxer/xtr_mpeg4.c
		demuxer/demux_lavf.c)
ENDIF ()

IF (CONFIG_DEMUX_FM)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_fm.c)
ENDIF (CONFIG_DEMUX_FM)


IF (CONFIG_VOD_SYSTEM)
	SET(SRC_DEMUXER ${SRC_DEMUXER} demuxer/demux_vod.c)
ENDIF (CONFIG_VOD_SYSTEM)

