# CmakeLists.txt
#
# This file allows building GXAVDEV with the CMake configuration and build
# tool. Download CMake in source or binary form from http://www.cmake.org/
#
# Original listfile by
#
INCLUDE(../cmake/toolchain.cmake)

PROJECT(gxgui CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Configuration checks
INCLUDE(CheckTypeSize)
INCLUDE(config.cmake)

# User-configurable options
SET(BUILD_SHARED_LIBS OFF CACHE BOOL
	"Build shared libraries instead of static ones.")

SET(TEST "$ENV{GXLIB_PATH}/abcd" CACHE STRING "test")

# Output files
CONFIGURE_FILE(config-cmake.h.in
	${CMAKE_BINARY_DIR}/gui_config.h
	@ONLY)

# Include.
INCLUDE_DIRECTORIES( include             )
IF (NDS STREQUAL YES)
	INCLUDE_DIRECTORIES( ${GOXCEED_PATH}/include/freetype2)
ENDIF()
INCLUDE_DIRECTORIES( ../include/gui_core )

# Source code.
#INCLUDE(cmake/access.cmake)

AUX_SOURCE_DIRECTORY( ../gui_view SRC_GUI)
AUX_SOURCE_DIRECTORY( core        SRC_GUI)
AUX_SOURCE_DIRECTORY( widget      SRC_GUI)
AUX_SOURCE_DIRECTORY( gdi/font    SRC_GUI)
AUX_SOURCE_DIRECTORY( gdi/gal     SRC_GUI)
AUX_SOURCE_DIRECTORY( event       SRC_GUI)
AUX_SOURCE_DIRECTORY( mini/       SRC_GUI)
AUX_SOURCE_DIRECTORY( mini/widget SRC_GUI)

IF (GUI_SCRIPT STREQUAL XML)
	AUX_SOURCE_DIRECTORY( xml SRC_GUI)
	AUX_SOURCE_DIRECTORY( xml/libxml SRC_GUI)
ELSE()
	AUX_SOURCE_DIRECTORY( json SRC_GUI)
ENDIF()

IF (ARCH STREQUAL i386)
	AUX_SOURCE_DIRECTORY( event/hws  SRC_GUI)
	AUX_SOURCE_DIRECTORY( gdi/SDL    SRC_GUI)
	AUX_SOURCE_DIRECTORY( gdi/hds    SRC_GUI)
ELSE()
	AUX_SOURCE_DIRECTORY( event/hwa  SRC_GUI)
	AUX_SOURCE_DIRECTORY( gdi/hda    SRC_GUI)
	AUX_SOURCE_DIRECTORY( gdi/image  SRC_GUI)
ENDIF()

IF (OS STREQUAL LINUX)
	SET (SRC_GUI ${SRC_GUI}
		gdi/SDL/SDL_events.c
		gdi/SDL/SDL_surface.c
		gdi/SDL/SDL_error.c
		gdi/SDL/SDL_pixels.c
		gdi/SDL/SDL_RLEaccel.c
		gdi/SDL/SDL_rwops.c
		gdi/SDL/SDL_blit.c
		gdi/SDL/SDL_video.c
		gdi/SDL/SDL_blit_0.c
		gdi/SDL/SDL_blit_1.c
		gdi/SDL/SDL_blit_A.c
		gdi/SDL/SDL_blit_N.c
		gdi/SDL/SDL_bmp.c
		gdi/SDL/SDL_endian.c
		gdi/SDL/SDL_cpuinfo.c
		gdi/SDL/SDL_sysmutex.c
		gdi/SDL/SDL_cursor.c
		gdi/SDL/SDL_systhread.c
		gdi/SDL/SDL_syssem.c
		gdi/SDL/SDL_keyboard.c
		gdi/SDL/SDL_mouse.c
		gdi/SDL/SDL_resize.c
		gdi/SDL/SDL_systimer.c
		gdi/SDL/SDL_fbvideo.c
		gdi/SDL/SDL_timer.c
		gdi/SDL/SDL_thread.c
		gdi/SDL/SDL_quit.c
		gdi/SDL/SDL.c
		gdi/SDL/SDL_fatal.c
		gdi/SDL/SDL_active.c
		gdi/SDL/SDL_fbevents.c
		gdi/SDL/SDL_fbmatrox.c
		gdi/SDL/SDL_fb3dfx.c
		gdi/SDL/SDL_fbriva.c
		gdi/SDL/SDL_fbevents.c
		gdi/SDL/SDL_fbelo.c
		gdi/SDL/SDL_yuv.c
		gdi/SDL/SDL_yuv_sw.c
		gdi/SDL/SDL_stretch.c
		)
ENDIF()

IF (OS STREQUAL ECOS)
	SET (SRC_GUI ${SRC_GUI}
		gdi/SDL/SDL_surface.c
		gdi/SDL/SDL_error.c
		gdi/SDL/SDL_pixels.c
		gdi/SDL/SDL_RLEaccel.c
		gdi/SDL/SDL_rwops.c
		gdi/SDL/SDL_blit.c
		gdi/SDL/SDL_video.c
		gdi/SDL/SDL_blit_0.c
		gdi/SDL/SDL_blit_1.c
		gdi/SDL/SDL_blit_A.c
		gdi/SDL/SDL_blit_N.c
		gdi/SDL/SDL_bmp.c
		gdi/SDL/SDL_endian.c
		gdi/SDL/SDL_cpuinfo.c
		gdi/SDL/SDL_sysmutex.c
		gdi/SDL/SDL_cursor.c
		gdi/SDL/SDL_systhread.c
		gdi/SDL/SDL_syssem.c
		gdi/SDL/SDL_yuv.c
		gdi/SDL/SDL_yuv_sw.c
		gdi/SDL/SDL_stretch.c
		)
ENDIF()

IF (CONFIG_TTF)
	ADD_DEFINITIONS(-DGUI_TTF)
	AUX_SOURCE_DIRECTORY( gdi/font/ttf SRC_GUI)
	SET (SRC_GUI ${SRC_GUI}
		gdi/SDL/SDL_surface.c
		gdi/SDL/SDL_error.c
		gdi/SDL/SDL_pixels.c
		gdi/SDL/SDL_RLEaccel.c
		gdi/SDL/SDL_rwops.c
		gdi/SDL/SDL_blit.c
		gdi/SDL/SDL_video.c
		gdi/SDL/SDL_blit_0.c
		gdi/SDL/SDL_blit_1.c
		gdi/SDL/SDL_blit_A.c
		gdi/SDL/SDL_blit_N.c
		gdi/SDL/SDL_bmp.c
		gdi/SDL/SDL_endian.c
		gdi/SDL/SDL_cpuinfo.c
		gdi/SDL/SDL_sysmutex.c
		gdi/SDL/SDL_cursor.c
		gdi/SDL/SDL_systhread.c
		gdi/SDL/SDL_syssem.c
		gdi/SDL/SDL_yuv.c
		gdi/SDL/SDL_yuv_sw.c
		gdi/SDL/SDL_stretch.c
		)
ENDIF()

IF(CONFIG_T9)
    ADD_DEFINITIONS(-DGUI_T9)
ENDIF()

IF(CONFIG_JPEG_ENCODING)
    ADD_DEFINITIONS(-DJPEG_ENCODING)
ENDIF()

IF(CONFIG_KEY_RECEIVE)
	ADD_DEFINITIONS(-DKEY_RECEIVE)
ENDIF()


IF (CONFIG_SOFT_JPEG AND (NOT CONFIG_TTF))
	SET (SRC_GUI ${SRC_GUI}
		gdi/SDL/SDL_yuv.c
		gdi/SDL/SDL_yuv_sw.c
		gdi/SDL/SDL_stretch.c
		)
ENDIF()

SET (SRC_GUI ${SRC_GUI}
	${SRC_ACCESS}
	${SRC_AVFORMAT}
	${SRC_AVOUT}
	${SRC_AVUTIL}
	${SRC_DECODER}
	${SRC_DEMUXER}
	)

# Build library
ADD_GX_LIBRARY(ui
	${SRC_GUI}
	)

