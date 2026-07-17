# CmakeLists.txt
#
# This file allows building GXAVDEV with the CMake configuration and build
# tool. Download CMake in source or binary form from http://www.cmake.org/
#
# Original listfile by
#

PROJECT(gxplayer C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Configuration checks
INCLUDE(../../cmake/arm-ecos-toolchain.cmake)
INCLUDE(CheckTypeSize)
INCLUDE(../config.cmake)

SET(SRC_ACCESS
	stream.c
	url.c
	options.c
	)

IF (CONFIG_STREAM_FILE)
	SET(SRC_ACCESS ${SRC_ACCESS}
		stream_file.c
		fileops.c
		fileops_volume.c
		fileops_posix.c
		fileops_stdlib.c
		)
ENDIF (CONFIG_STREAM_FILE)

IF (CONFIG_STREAM_FIFO)
	SET(SRC_ACCESS ${SRC_ACCESS} stream_fifo.c)
ENDIF (CONFIG_STREAM_FIFO)

IF (CONFIG_STREAM_RINGMEM)
	SET(SRC_ACCESS ${SRC_ACCESS} access/stream_ringmem.c)
ENDIF (CONFIG_STREAM_RINGMEM)

IF (CONFIG_STREAM_MEM)
	SET(SRC_ACCESS ${SRC_ACCESS} stream_mem.c)
ENDIF (CONFIG_STREAM_MEM)

IF (CONFIG_STREAM_HTTP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		stream_m3u8.c
		stream_http.c
		tcp.c
		network.c
		cookie.c)
ENDIF (CONFIG_STREAM_HTTP)

IF (CONFIG_STREAM_RTSP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		rtp.c
		stream_rtp.c
		stream_rtsp.c
		stream_udp.c
		udp.c
		freesdp/common_sdp.c
		freesdp/errorlist.c
		freesdp/parser_sdp.c
		librtsp/rtsp.c
		librtsp/rtsp_rtp.c
		librtsp/rtsp_session.c
		realrtsp/asmrp.c
		realrtsp/real.c
		realrtsp/rmff.c
		realrtsp/sdpplin.c
		realrtsp/md5.c
		realrtsp/xbuffer.c
		)
ENDIF (CONFIG_STREAM_RTSP)

IF (CONFIG_STREAM_RTMP)
	SET(SRC_ACCESS ${SRC_ACCESS}
		stream_rtmp.c
		librtmp/rtmp.c
		librtmp/log.c
		librtmp/amf.c
		librtmp/hashswf.c
		librtmp/parseurl.c
		)
ENDIF (CONFIG_STREAM_RTMP)

IF (CONFIG_STREAM_LIVE555)
	SET(SRC_ACCESS ${SRC_ACCESS}
		stream_live555.c
		)
ENDIF (CONFIG_STREAM_LIVE555)

IF (CONFIG_STREAM_DVB)
	SET(SRC_ACCESS ${SRC_ACCESS}
		stream_dvb.c)
ENDIF (CONFIG_STREAM_DVB)

IF (CONFIG_STREAM_CACHE)
	SET(SRC_ACCESS ${SRC_ACCESS}
		cache.c)
ENDIF (CONFIG_STREAM_CACHE)


ADD_LIBRARY( gxplayer_access ${SRC_ACCESS} )

