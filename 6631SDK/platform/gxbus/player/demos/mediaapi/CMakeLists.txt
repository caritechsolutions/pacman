PROJECT(demos-mediaapi C CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(GXTARGETS demos-mediaapi)

INCLUDE(../../../cmake/toolchain.cmake)

INCLUDE_DIRECTORIES( ../include )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include )

LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/../..
	${PROJECT_BINARY_DIR}/../../../subtitle
	${PROJECT_BINARY_DIR}/../../../gui_core
	${PROJECT_BINARY_DIR}/../../..
	${PROJECT_BINARY_DIR}/../common
	${GOXCEED_PATH}/lib)

AUX_SOURCE_DIRECTORY(.  SRCS)

IF (GX_SSLLIB STREQUAL openssl)
	SET(LIB_SSL
		libopenssl.a
		)
ENDIF()

IF (GX_SSLLIB STREQUAL wolfssl)
	SET(LIB_SSL
		libcyassl.a
		)
ENDIF()

IF (OS STREQUAL ECOS)
	SET(SRCS ${SRCS}
		bsp.cpp
		)
	SET(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,--gc-sections -Wl,-static,--fatal-warnings -nostartfiles -Ttarget.ld -ltarget")
	ADD_EXECUTABLE(${GXTARGETS} ${SRCS})
	TARGET_LINK_LIBRARIES(${GXTARGETS}
		libdemos-common.a
		libgxplayer.a
		libgxdvb.a
		libgxplayer.a
		libsmartvolume.a
		libenca.a
		libgxcaption.a
		libzvbi.a
		${LIB_SSL}
		libui.a
		libgxbuscore.a
		libfreetype.a
		libxml.a
		libxml2.a
		libgxav.a
		libgxsecure.a
		libjpeg.a
		libmpeg2.a
		libtiff.a
		libpng.a
		libz.a
		libcurl.a
		libgxcore.a
		libusbwifi.a
		libgxhotplug.a
		libwebp.a
		libiconv.a
		)
ENDIF()

IF (OS STREQUAL LINUX)
	SET(CMAKE_EXE_LINKER_FLAGS "-static")
	ADD_EXECUTABLE(${GXTARGETS} ${SRCS})
	TARGET_LINK_LIBRARIES(${GXTARGETS}
		libdemos-common.a
		libgxplayer.a
		libsmartvolume.a
		libgxcaption.a
		libui.a
		libgxbuscore.a
		libgxhotplug.a
		libgxdvb.a
		libgxcore.a
		libjpeg.a
		libwebp.a
		libpng.a
		libtiff.a
		libz.a
		libmpeg2.a
		libcurl.a
		libzvbi.a
		${LIB_SSL}
		libenca.a
		libgxcore.a
		libiconv.a
		liblzo.a
		libfreetype.a
		libgxdvb.a
		libxml.a
		libxml2.a
		libm.a
		libstdc++.a
		libgcc.a
		libc.a
		libdl.a
		libcjson.a
		librist.a
		libpthread.a
		)
ENDIF()

ADD_DEPENDENCIES(${GXTARGETS}
	demos-common
	gxplayer_static
	gxbuscore_static
	gxcaption_static
	ui_static
	gxhotplug_static
	gxdvb_static
	)

SET_TARGET_PROPERTIES(${GXTARGETS} PROPERTIES OUTPUT_NAME mediaapi)
INSTALL( TARGETS ${GXTARGETS} RUNTIME DESTINATION ${GOXCEED_PATH}/bin )
