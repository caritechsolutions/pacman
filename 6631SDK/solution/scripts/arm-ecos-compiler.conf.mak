CROSS_COMPILE=arm-none-eabi-
CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
STRIP  = $(CROSS_COMPILE)strip

TARGET_DEFS=-DECOS_OS

-include $(GXLIB_PATH)/include/pkgconf/ecos.mak

CFLAGS  += $(ECOS_GLOBAL_CFLAGS) -D$(ARCH)
CFLAGS += -mfloat-abi=hard
ECOS_GLOBAL_LDFLAGS = -nostdlib -Wl,--gc-sections -Wl,-static,--fatal-warnings
LDFLAGS += $(ECOS_GLOBAL_LDFLAGS) -nostartfiles -mfloat-abi=hard
LDFLAGS += -Wl,--whole-archive -lgxsecure -lgxav -Wl,--no-whole-archive -lgxcore -lgxfrontend -Ttarget.ld -ltarget -lgxcore

