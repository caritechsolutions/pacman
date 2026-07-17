INCLUDE(../../../cmake/toolchain.cmake)

INCLUDE_DIRECTORIES(./include)

INCLUDE_DIRECTORIES(../include)

AUX_SOURCE_DIRECTORY(.  SRC)

ADD_LIBRARY(demos-common ${SRC})
