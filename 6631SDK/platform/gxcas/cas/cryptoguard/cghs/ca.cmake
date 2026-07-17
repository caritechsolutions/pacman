INCLUDE_DIRECTORIES(cas/cryptoguard/cghs/include)
AUX_SOURCE_DIRECTORY(cas/cryptoguard/cghs/src CRYPTOGUARD)

SET(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -DGXCAS_EVENT_MAX_LEN=2048" )
SET(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -DGXCAS_EVENT_MAX_LEN=2048" )

SET(CAS
	${CAS}
    ${CRYPTOGUARD}
	)


INSTALL(FILES cas/cryptoguard/cghs/include/cryptoguard_api.h DESTINATION ${GOXCEED_PATH}/include/gxcas)
#INSTALL(FILES cas/cryptoguard/include/uti_udrm_type.h DESTINATION ${GOXCEED_PATH}/include/gxcas)
#INSTALL(FILES cas/cryptoguard/include/unitend.h DESTINATION ${GOXCEED_PATH}/include/gxcas)
