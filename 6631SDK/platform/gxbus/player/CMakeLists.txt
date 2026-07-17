# CmakeLists.txt
#
# This file allows building GXAVDEV with the CMake configuration and build
# tool. Download CMake in source or binary form from http://www.cmake.org/
#
# Original listfile by
#
INCLUDE(../cmake/toolchain.cmake)

PROJECT(gxplayer CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Configuration checks
INCLUDE(CheckTypeSize)
INCLUDE(config.cmake)

# User-configurable options
SET(BUILD_SHARED_LIBS OFF CACHE BOOL
	"Build shared libraries instead of static ones.")

# Output files
CONFIGURE_FILE(config-cmake.h.in
	${CMAKE_BINARY_DIR}/gx_config.h
	@ONLY)

CHECK_INCLUDE_FILE(byteswap.h       HAVE_BYTESWAP_H)

# Include.
INCLUDE_DIRECTORIES( include                )
INCLUDE_DIRECTORIES( avutil                 )
INCLUDE_DIRECTORIES( avcodec                )
INCLUDE_DIRECTORIES( avformat               )
INCLUDE_DIRECTORIES( vodsystem              )
INCLUDE_DIRECTORIES( interface              )
INCLUDE_DIRECTORIES( mediaapi               )
INCLUDE_DIRECTORIES( ../include             )
INCLUDE_DIRECTORIES( ../subtitle/include    )
INCLUDE_DIRECTORIES( ../include/module/player)

# Source code.
INCLUDE(cmake/access.cmake)
INCLUDE(cmake/avformat.cmake)
INCLUDE(cmake/vodsystem.cmake)
INCLUDE(cmake/avout.cmake)
INCLUDE(cmake/avutil.cmake)
INCLUDE(cmake/avcodec.cmake)
INCLUDE(cmake/decoder.cmake)
INCLUDE(cmake/demuxer.cmake)
INCLUDE(cmake/muxer.cmake)
INCLUDE(cmake/mediaapi.cmake)
INCLUDE(cmake/subtitle.cmake)
INCLUDE(cmake/avmemhole.cmake)

AUX_SOURCE_DIRECTORY( .       SRC_PLAYER)
AUX_SOURCE_DIRECTORY( libass  SRC_PLAYER)
AUX_SOURCE_DIRECTORY( libtag  SRC_PLAYER)
AUX_SOURCE_DIRECTORY( system  SRC_PLAYER)

SET (SRC_PLAYER ${SRC_PLAYER}
	interface/dumpfilter.c
	interface/dumpfilter_dvb.c
	interface/dumpfilter_raw.c
	interface/dumpfilter_fm.c
	interface/avfifo.c
	interface/gx_fifo.c
	interface/gx_fifo_sw.c
	interface/gx_fifo_hwts.c
	interface/gx_fifo_sdc.c
	interface/gx_fifo_muxts.c
	interface/gx_fifo_muxer.c
	interface/media.c
	interface/utils.c
	interface/mediafilter.c
	interface/mediamanager.c
	interface/page_cache.c
	interface/pack_cache.c
	interface/file_cache.c
	interface/speed_detect.c
	interface/gx_audio_control.c
	avutil/mmv.c
	avutil/mma.c
	avutil/memhole.c	)


IF (CONFIG_MUXER_TS_HW)
	SET (SRC_PLAYER ${SRC_PLAYER} muxer/muxer_ts_hw.c)
ENDIF()

IF (CONFIG_MUXER_MPEG_SW)
	SET (SRC_PLAYER ${SRC_PLAYER} muxer/muxer_mpeg.c )
ENDIF()

SET (SRC_PLAYER ${SRC_PLAYER}
	${SRC_ACCESS}
	${SRC_AVFORMAT}
	${SRC_AVOUT}
	${SRC_AVUTIL}
	${SRC_AVCODEC}
	${SRC_DECODER}
	${SRC_DEMUXER}
	${SRC_MUXER}
	${SRC_VODSYSTEM}
	${SRC_MEDIAAPI}
	${SRC_SUBTITLE}
	)

# Build library
IF ($ENV{ARCH_OS} STREQUAL csky-nos OR $ENV{ARCH_OS} STREQUAL arm-nos)
	AUX_SOURCE_DIRECTORY( system  SRC_AVMEMHOLE)
ADD_GX_LIBRARY(gxplayer
	${SRC_MEDIAAPI}
	${SRC_AVMEMHOLE}
	)
ELSE()
ADD_GX_LIBRARY(gxplayer
	${SRC_PLAYER}
	)

IF ($ENV{GX_PROJ} STREQUAL generic)
	ADD_SUBDIRECTORY(demos)
ENDIF()

ENDIF()
