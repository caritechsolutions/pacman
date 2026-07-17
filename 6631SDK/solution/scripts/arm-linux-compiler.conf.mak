CROSS_COMPILE ?= arm-linux-uclibcgnueabihf-
CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
STRIP  = $(CROSS_COMPILE)strip

CFLAGS += -DLINUX_OS
LIBS   =  -lpthread -lgxcore -lm
LDFLAGS += -Wl,--gc-sections
