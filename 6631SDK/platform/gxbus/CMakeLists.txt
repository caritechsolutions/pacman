# CmakeLists.txt
#
# This file allows building GXAVDEV with the CMake configuration and build
# tool. Download CMake in source or binary form from http://www.cmake.org/
#
# Original listfile by
#

IF ($ENV{GX_PROJ} STREQUAL ota OR $ENV{ARCH_OS} STREQUAL csky-nos OR $ENV{ARCH_OS} STREQUAL arm-nos)
#SET( CONFIG_TTF 1)
#SET( CONFIG_JPEG_INFO 1)
#SET( CONFIG_SOFT_JPEG 1)
#SET( CONFIG_BMP_INFO 1)
#SET( CONFIG_ICONV 1)
#SET( CONFIG_ROLL_STR 1)
ELSE()
SET( CONFIG_TTF 1)
SET( CONFIG_JPEG_INFO 1)
SET( CONFIG_SOFT_JPEG 1)
SET( CONFIG_BMP_INFO 1)
SET( CONFIG_ICONV 1)
SET( CONFIG_ROLL_STR 1)
ENDIF()

IF (CONFIG_TTF)
	ADD_DEFINITIONS(-DGUI_TTF)
ENDIF()

IF (CONFIG_SOFT_JPEG)
	ADD_DEFINITIONS(-DGUI_SOFT_JPEG)
ENDIF()

IF (CONFIG_JPEG_INFO)
	ADD_DEFINITIONS(-DGUI_JPEG_INFO)
ENDIF()

IF (CONFIG_BMP_INFO)
	ADD_DEFINITIONS(-DGUI_BMP_INFO)
ENDIF()

IF (CONFIG_ICONV)
	ADD_DEFINITIONS(-DGUI_STR_ICONV)
ENDIF()

IF (CONFIG_ROLL_STR)
	ADD_DEFINITIONS(-DGUI_ROLL_STR)
ENDIF()

IF ($ENV{ARCH_OS} STREQUAL csky-nos OR $ENV{ARCH_OS} STREQUAL arm-nos)
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)
INCLUDE_DIRECTORIES( ${GOXCEED_PATH}/include/gxos  )
PROJECT(gxbus C CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET( CONFIG_XML 1)
SET( CONFIG_I18N 1)
#SET( CONFIG_TTF 1)

IF (CONFIG_TTF)
	ADD_DEFINITIONS(-DGUI_TTF)
ENDIF()
IF (CONFIG_I18N)
	ADD_DEFINITIONS(-DGUI_I18N)
ENDIF()
INCLUDE_DIRECTORIES( ${GOXCEED_PATH}/include/gxos  )
INCLUDE_DIRECTORIES( ${GOXCEED_PATH}/include/sys  )
INCLUDE_DIRECTORIES( ${GOXCEED_PATH}/include/freetype  )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/gui_core       )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/gui_core/include       )
PROJECT(gxbus C CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/gui_core/gdi    SRC_GDI )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/gui_core/gdi/font    SRC_FONT )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/gui_core/mini   SRC_MINI )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/gui_core/mini/widget   SRC_WIDGET )

SET(CMAKE_BUILD_TYPE "Release")
SET(SRC_COMMON ${SRC_COMMON}
	${CMAKE_SOURCE_DIR}/common/gxavdev.c
	${CMAKE_SOURCE_DIR}/common/gxconfig.c
	${CMAKE_SOURCE_DIR}/common/gx_videomem.c
	)

SET (SRC_GDI ${SRC_GDI}
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_blit.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_pixel.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_rect.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_char.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_img.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_init.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/gal/gal_init.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/gal/gal_blit.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/gal/gal_pixel.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/gal/gal_rect.c
	${CMAKE_SOURCE_DIR}/gui_core/core/gui_hash.c
	${CMAKE_SOURCE_DIR}/gui_core/core/stack.c
	${CMAKE_SOURCE_DIR}/gui_core/core/linklist.c
	${CMAKE_SOURCE_DIR}/gui_core/core/framebuffer.c
	${CMAKE_SOURCE_DIR}/gui_core/core/color.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/image/IMG.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/image/IMG_BMP.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/image/IMG_JPEG.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_jpeg.c
	${CMAKE_SOURCE_DIR}/gui_core/gdi/hda/hda_color.c
	)

IF (CONFIG_TTF)
	SET (SRC_FONT ${SRC_FONT}
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/font/ttf/gal_ttf.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_surface.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_RLEaccel.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_video.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_error.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_pixels.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_blit.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_blit_0.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_blit_1.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_blit_N.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_blit_A.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/SDL/SDL_cpuinfo.c
	    ${CMAKE_SOURCE_DIR}/gui_core/gdi/image/IMG.c
	    )
ENDIF()

IF (CONFIG_XML)
	SET (SRC_WIDGET ${SRC_WIDGET}
	${CMAKE_SOURCE_DIR}/gui_core/mini/widget/mini_xml.c
	${CMAKE_SOURCE_DIR}/gui_core/mini/widget/frame.c
	${CMAKE_SOURCE_DIR}/gui_core/mini/widget/label.c
	${CMAKE_SOURCE_DIR}/gui_core/mini/widget/progress.c
	${CMAKE_SOURCE_DIR}/gui_core/mini/widget/list.c
	${CMAKE_SOURCE_DIR}/gui_core/widget/widget.c
	)
ENDIF()

ADD_GX_LIBRARY(ui
	${SRC_GDI}
	${SRC_FONT}
	${SRC_MINI}
	${SRC_WIDGET}
	)

ADD_GX_LIBRARY(gxbuscore
	${SRC_COMMON}
	)

INSTALL(DIRECTORY include/gui_core DESTINATION ${GOXCEED_PATH}/include/bus)
INSTALL(FILES include/module/config/gxconfig.h DESTINATION ${GOXCEED_PATH}/include/bus/module/config)

ADD_SUBDIRECTORY(player)
INSTALL(FILES include/module/player/gxmedia_api.h      DESTINATION ${GOXCEED_PATH}/include/player)
INSTALL(FILES include/module/player/gxplayer_decoder.h DESTINATION ${GOXCEED_PATH}/include/player)

ELSE()

# Include.
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include                )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/gui_core       )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/service        )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/module         )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/bus                    )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/subtitle               )

#数据广播
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/sub_system/si_parse_lib )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/sub_system/si_parse_lib/si_parse_lib_des )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/sub_system/si_parse_lib/si_parse_lib_table )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/sub_system/dmx_sub_system )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include/sub_system/si_sub_engine )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/sub_system/si_parse_lib/include )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/sub_system/dmx_sub_system/include )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/sub_system/si_sub_engine/include )

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)

PROJECT(gxbus C CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Configuration checks
INCLUDE(CheckTypeSize)
CHECK_INCLUDE_FILE(byteswap.h       HAVE_BYTESWAP_H)

# User-configurable options
SET(BUILD_SHARED_LIBS OFF CACHE BOOL
	"Build shared libraries instead of static ones.")

SET(TEST "$ENV{GXLIB_PATH}/abcd" CACHE STRING "test")

# Output files
CONFIGURE_FILE(config-cmake.h.in
	${CMAKE_BINARY_DIR}/config.h
	@ONLY)

# Source code.
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/book           SRC_BOOK             )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/bus            SRC_BUS              )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/common         SRC_COMMON           )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/epg            SRC_EPG              )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/advance_epg    SRC_ADVANCE_EPG      )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/extra          SRC_EXTRA            )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/hotplug        SRC_HOTPLUG          )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/serial         SRC_SERIAL           )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/hdmi_hotplug   SRC_HDMI_HOTPLUG     )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/chiptest       SRC_CHIPTEST         )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/chiptest/vsame SRC_VSAME            )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/pm             SRC_PM               )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/search         SRC_SEARCH           )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/si             SRC_SI               )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/si/table       SRC_SI               )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/update         SRC_UPDATE           )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/frontend       SRC_FRONTEND         )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/debug          SRC_DEBUG            )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/misc           SRC_MISC             )

#数据广播
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/sub_system/si_parse_lib     SRC_SI_PARSE_LIB )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/sub_system/dmx_sub_system   SRC_DMX_SUB_SYSTEM )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/sub_system/dmx_sub_system/dmx_sub_driver SRC_DMX_SUB_DRIVER_SYSTEM )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/sub_system/si_sub_engine    SRC_SI_ENGINE_SYSTEM )
AUX_SOURCE_DIRECTORY( ${CMAKE_SOURCE_DIR}/atsc/atsc_search            SRC_ATSC_SEARCH )

# Source code
SET(SRC_GXDVB ${SRC_SI}
	${SRC_PM}
	${SRC_EPG}
	${SRC_BOOK}
	${SRC_EXTRA}
	${SRC_UPDATE}
	${SRC_SEARCH}
	${SRC_FRONTEND}
	${SRC_SI_PARSE_LIB}
	${SRC_SI_ENGINE_SYSTEM}
	${SRC_DMX_SUB_SYSTEM}
	${SRC_DMX_SUB_DRIVER_SYSTEM}
	${SRC_ATSC_SEARCH}
	)

SET(SRC_GXHOTPLUG
	${SRC_HOTPLUG}
	${SRC_HDMI_HOTPLUG}
	)

SET(SRC_GXCHIPTEST
	${SRC_CHIPTEST}
	)

SET(SRC_GXSERIAL
	${SRC_SERIAL}
	)

# SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")

# Build libraries
ADD_GX_LIBRARY(gxdvb
	${SRC_GXDVB}
	)
ADD_GX_LIBRARY(gxbuscore
	${SRC_BUS}
	${SRC_COMMON}
	${SRC_DEBUG}
	)
ADD_GX_LIBRARY(gxhotplug
	${SRC_GXHOTPLUG}
	)

ADD_GX_LIBRARY(gxbusmisc
	${SRC_MISC}
	)

#IF (OS STREQUAL ECOS)
#ADD_GX_LIBRARY(gxchiptest
#	${SRC_GXCHIPTEST}
#	${SRC_VSAME}
#	)
#ENDIF()

ADD_SUBDIRECTORY(subtitle)

ADD_SUBDIRECTORY(player)
ADD_SUBDIRECTORY(gui_core)

IF (OS STREQUAL ECOS)
ADD_GX_LIBRARY(gxserial
	${SRC_GXSERIAL}
	)
ENDIF()

INSTALL(DIRECTORY include/ DESTINATION ${GOXCEED_PATH}/include/bus)
ENDIF()
