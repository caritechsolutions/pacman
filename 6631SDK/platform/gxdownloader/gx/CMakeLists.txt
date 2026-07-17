PROJECT(gxota C CXX)
INCLUDE(../scripts/toolchain.cmake)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(CMAKE_BUILD_TYPE $ENV{BUILD_MODE})

INCLUDE_DIRECTORIES(../)
INCLUDE_DIRECTORIES(../include/generated)
INCLUDE_DIRECTORIES(./src)
INCLUDE_DIRECTORIES(../include)
INCLUDE_DIRECTORIES(./src/net)
INCLUDE_DIRECTORIES(./lib/miniupnpc-2.1.20191224)

IF (ARCH_OS STREQUAL csky-nos OR ARCH_OS STREQUAL arm-nos)
	INCLUDE_DIRECTORIES(./src/menu)
        AUX_SOURCE_DIRECTORY(./ ENTRY)
        AUX_SOURCE_DIRECTORY(./src/keyboard/ KEYBOARD)
        AUX_SOURCE_DIRECTORY(./src/menu/ MENU)
        AUX_SOURCE_DIRECTORY(./src/panel/ PANEL)
        AUX_SOURCE_DIRECTORY(./src/ GXOTA)
ELSEIF (ARCH_OS STREQUAL csky-ecos OR ARCH_OS STREQUAL arm-ecos)
	INCLUDE_DIRECTORIES(./src/menu)
	INCLUDE_DIRECTORIES(./src/miniwget)
	AUX_SOURCE_DIRECTORY(./ ENTRY)
	AUX_SOURCE_DIRECTORY(./src/menu/ MENU)
	AUX_SOURCE_DIRECTORY(./src/ GXOTA)
	SET(MINIWGET lib/miniupnpc-2.1.20191224/miniwget.c
		lib/miniupnpc-2.1.20191224/receivedata.c
		lib/miniupnpc-2.1.20191224/connecthostport.c
		)
ELSE()
        SET(GXOTA ./src/dlp.c
                ./src/ini.c
                ./src/dsmcc.c
		./src/installer.c
		./src/ota_mem.c
		./src/downloader.c
		./src/progress.c
		./src/http_download.c
		./src/parser.c)
ENDIF()

ADD_GX_LIBRARY(downloader
        ${ENTRY}
        ${KEYBOARD}
        ${MENU}
        ${PANEL}
        ${GXOTA}
	${MINIWGET}
        )

INSTALL(FILES src/gxdlp_api.h DESTINATION ${GOXCEED_PATH}/include/downloader)
INSTALL(FILES src/gxota_api.h DESTINATION ${GOXCEED_PATH}/include/downloader)
INSTALL(FILES src/gxota_msg.h DESTINATION ${GOXCEED_PATH}/include/downloader)
INSTALL(FILES src/gxupdate_api.h DESTINATION ${GOXCEED_PATH}/include/downloader)
INSTALL(FILES src/gxupdate_msg.h DESTINATION ${GOXCEED_PATH}/include/downloader)
