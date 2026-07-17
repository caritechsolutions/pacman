CROSS_COMPILE=arm-none-eabi-
CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)gcc
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
OBJCOPY= $(CROSS_COMPILE)objcopy
NM     = $(CROSS_COMPILE)nm

-include $(GXLIB_PATH)/include/pkgconf/ecos.mak

CFLAGS  += $(ECOS_GLOBAL_CFLAGS) -DECOS_OS $(GX_EXT_CFLAGS)
ECOS_GLOBAL_LDFLAGS = -nostdlib -Wl,--gc-sections -Wl,-static,--fatal-warnings
LDFLAGS += $(ECOS_GLOBAL_LDFLAGS) -nostartfiles
LIBS 	+= -ldownloader -lboot -ltarget -lgxosal -lgxav -lgxsecure -lgxfrontend -lui -lgxav -lgxbuscore -lgxcore
ifdef CONFIG_WIFI_SUPPORT
LIBS    += -lusbwifi
endif
LIBS    += -ljpeg -lfreetype -ltiff


