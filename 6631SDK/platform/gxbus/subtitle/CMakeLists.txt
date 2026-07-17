INCLUDE(../cmake/toolchain.cmake)

PROJECT(gxcaption CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

INCLUDE(config.cmake)
CONFIGURE_FILE(config-cmake.h.in
	${CMAKE_BINARY_DIR}/subtitle_config.h
	@ONLY)

INCLUDE_DIRECTORIES(./common        )
INCLUDE_DIRECTORIES(./include       )
INCLUDE_DIRECTORIES(../player/avutil)
INCLUDE_DIRECTORIES(../player/include)
INCLUDE_DIRECTORIES( ../include/module/subtitle)
INCLUDE_DIRECTORIES( ../include/module/player)

AUX_SOURCE_DIRECTORY(dvb2      SRC_SUBTITLE)
AUX_SOURCE_DIRECTORY(aribcc    SRC_SUBTITLE)
AUX_SOURCE_DIRECTORY(atsccc    SRC_SUBTITLE)
#AUX_SOURCE_DIRECTORY(scte      SRC_SUBTITLE)
AUX_SOURCE_DIRECTORY(text      SRC_SUBTITLE)
AUX_SOURCE_DIRECTORY(pixel     SRC_SUBTITLE)
AUX_SOURCE_DIRECTORY(teletext  SRC_SUBTITLE)

IF (CONFIG_SUBTITLE_GDI_DRAW)
	SET (SRC_SUBTITLE ${SRC_SUBTITLE}
		common/gx_gdi_show_text.c
		common/gx_gdi_show_pixel.c
		common/gx_gdi_layer_spp.c
	)
ENDIF (CONFIG_SUBTITLE_GDI_DRAW)

SET (SRC_SUBTITLE ${SRC_SUBTITLE}
	gx_subtitle_setting.c
	gx_subtitle_magazine.c
	gx_subtitle_module.c
	gx_subtitle_atsccc.c
	common/gx_subtitle_show_text.c
	common/gx_subtitle_show_pixel.c
)

####历史原因，还需保留下来
INCLUDE_DIRECTORIES(./old           )
IF (CONFIG_SUBTITLE_GDI_DRAW)
	#AUX_SOURCE_DIRECTORY(old/astccc    SRC_OLD_SUBTITLE)
	AUX_SOURCE_DIRECTORY(old/scte      SRC_OLD_SUBTITLE)
ELSE()
	AUX_SOURCE_DIRECTORY(scte      SRC_SUBTITLE)
ENDIF (CONFIG_SUBTITLE_GDI_DRAW)
AUX_SOURCE_DIRECTORY(old/dvb       SRC_OLD_SUBTITLE)
AUX_SOURCE_DIRECTORY(old/ttx       SRC_OLD_SUBTITLE)

ADD_GX_LIBRARY(gxcaption
	${SRC_OLD_SUBTITLE}
	${SRC_SUBTITLE}
	)

