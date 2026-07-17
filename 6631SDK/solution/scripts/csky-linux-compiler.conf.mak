CROSS_COMPILE ?= csky-linux-
CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)g++
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
STRIP  = $(CROSS_COMPILE)strip

CFLAGS += -DLINUX_OS
LIBS   =  -lgxcore -lm -lrt -lpthread
LDFLAGS += -Wl,--gc-sections
