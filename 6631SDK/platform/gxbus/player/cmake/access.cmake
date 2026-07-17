SET(SRC_ACCESS
	access/stream.c
	access/url.c
	access/options.c
	access/recfs.c
	access/recinode_file.c
	access/recinode_mem.c
	)
	
IF (CONFIG_IPTV_ENABLE)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/tcp.c
		access/network.c
		access/rtp.c
		access/udp.c
		access/stream_drm.c
		access/stream_crypto.c
		)
ENDIF (CONFIG_IPTV_ENABLE)

IF (CONFIG_STREAM_FILE)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_file.c
		access/stream_pvr.c
		access/stream_dvr.c
		access/fileops.c
		access/fileops_volume.c
		access/fileops_posix.c
		access/fileops_stdlib.c
		access/fileops_pvr.c
		access/libpvr/libpvr.c
		access/libpvr/src/pvr_pvr.c
		access/libpvr/src/pvr_seg.c
		access/libpvr/src/pvr_evt.c
		access/libpvr/src/pvr_lst.c
		access/libpvr/src/pvr_parser.c
		access/libpvr/src/pvr_file.c
		access/libpvr/src/pvr_utils.c
		)
ENDIF (CONFIG_STREAM_FILE)

IF (CONFIG_STREAM_HWCRYPTO)
	SET(SRC_ACCESS ${SRC_ACCESS} access/stream_mtc.c)
ENDIF (CONFIG_STREAM_HWCRYPTO)

IF (CONFIG_STREAM_FIFO)
	SET(SRC_ACCESS ${SRC_ACCESS} access/stream_fifo.c)
ENDIF (CONFIG_STREAM_FIFO)

IF (CONFIG_STREAM_RINGMEM)
	SET(SRC_ACCESS ${SRC_ACCESS} access/stream_ringmem.c)
ENDIF (CONFIG_STREAM_RINGMEM)

IF (CONFIG_STREAM_MEM)
	SET(SRC_ACCESS ${SRC_ACCESS} access/stream_mem.c)
ENDIF (CONFIG_STREAM_MEM)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_HTTP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/hls.c
		access/stream_m3u8.c
		access/stream_list.c
		access/stream_http.c
		access/cookie.c)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_HTTP)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_RTP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_rtp.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_RTP)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_UDP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_udp.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_UDP)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_WFD)
	SET(SRC_ACCESS ${SRC_ACCESS} access/stream_wfd.c)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_WFD)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_RTSP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_rtsp.c
		access/freesdp/common_sdp.c
		access/freesdp/errorlist.c
		access/freesdp/parser_sdp.c
		access/librtsp/rtsp.c
		access/librtsp/rtsp_rtp.c
		access/librtsp/rtsp_session.c
		access/realrtsp/asmrp.c
		access/realrtsp/real.c
		access/realrtsp/rmff.c
		access/realrtsp/sdpplin.c
		#access/realrtsp/md5.c
		access/realrtsp/xbuffer.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_RTSP)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_RTMP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_rtmp.c
		access/librtmp/rtmp.c
		access/librtmp/log.c
		access/librtmp/amf.c
		access/librtmp/hashswf.c
		access/librtmp/parseurl.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_RTMP)

IF (CONFIG_FFMPEG_ENABLE AND (CONFIG_STREAM_FFRTSP OR CONFIG_STREAM_FFSRTP OR CONFIG_STREAM_FFCRYPTO OR CONFIG_STREAM_FFRTMP OR CONFIG_STREAM_FFRTMPHTTP OR CONFIG_STREAM_FFMMS OR CONFIG_STREAM_FFDATA OR CONFIG_STREAM_FFHLS OR CONFIG_STREAM_FFHTTP OR (CONFIG_STREAM_MPEGDASH AND CONFIG_STREAM_FFHTTP) OR CONFIG_STREAM_CONCAT OR CONFIG_STREAM_FFRIST))
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_ffmpeg.c
		)
ENDIF (CONFIG_FFMPEG_ENABLE AND (CONFIG_STREAM_FFRTSP OR CONFIG_STREAM_FFSRTP OR CONFIG_STREAM_FFCRYPTO OR CONFIG_STREAM_FFRTMP OR CONFIG_STREAM_FFRTMPHTTP OR CONFIG_STREAM_FFMMS OR CONFIG_STREAM_FFDATA OR CONFIG_STREAM_FFHLS OR CONFIG_STREAM_FFHTTP OR (CONFIG_STREAM_MPEGDASH AND CONFIG_STREAM_FFHTTP) OR CONFIG_STREAM_CONCAT OR CONFIG_STREAM_FFRIST))

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_MMS)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_mms.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_MMS)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_LIVE555)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_live555.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_LIVE555)

IF (CONFIG_STREAM_DVB)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_dvb.c
		access/dvbsource.c
		access/dvbsource_tsbuff.c
		access/dvbsource_tscache.c
		access/dvbsource_normal.c
       )
ENDIF (CONFIG_STREAM_DVB)

IF (CONFIG_STREAM_FM)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_fm.c
       )
ENDIF (CONFIG_STREAM_FM)

IF (CONFIG_STREAM_CACHE)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/cache.c)
ENDIF (CONFIG_STREAM_CACHE)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_MPEGDASH AND CONFIG_STREAM_HTTP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_ffdash.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_MPEGDASH AND CONFIG_STREAM_HTTP)

IF (CONFIG_VOD_SYSTEM)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_vod.c)
ENDIF (CONFIG_VOD_SYSTEM)

IF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_ASSOCIATIONLIBRE)
	SET(SRC_ACCESS ${SRC_ACCESS}
		access/stream_associationlibre.c
		)
ENDIF (CONFIG_IPTV_ENABLE AND CONFIG_STREAM_ASSOCIATIONLIBRE)

