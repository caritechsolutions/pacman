CROSS_COMPILE=ckcore-elf-
CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)g++
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

-include $(GXLIB_PATH)/include/pkgconf/ecos.mak

CFLAGS  += $(ECOS_GLOBAL_CFLAGS) -DECOS_OS 
LDFLAGS += $(ECOS_GLOBAL_LDFLAGS) -nostartfiles -Ttarget.ld -ltarget -lgxcore -lgxav
