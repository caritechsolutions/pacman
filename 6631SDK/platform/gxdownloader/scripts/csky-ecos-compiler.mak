CROSS_COMPILE=csky-elf-
CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)g++
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
OBJCOPY= $(CROSS_COMPILE)objcopy
NM     = $(CROSS_COMPILE)nm

ECOS_GLOBAL_CFLAGS = -O2 -g -Wall -mcpu=ck610 -Wpointer-arith -Wundef -Wno-write-strings -ffunction-sections -fdata-sections -fno-exceptions
ECOS_GLOBAL_LDFLAGS = -nostdlib -Wl,--gc-sections -Wl,-static,--fatal-warnings

CFLAGS  += $(ECOS_GLOBAL_CFLAGS) -DECOS_OS $(GX_EXT_CFLAGS)
LDFLAGS += $(ECOS_GLOBAL_LDFLAGS) -nostartfiles
LIBS += -ldownloader -lboot -ltarget
ifdef CONFIG_SECURE_SUPPORT
LIBS += --whole-archive -lgxosal -lgxav -lgxsecure --no-whole-archive
else
LIBS += -lgxosal -lgxav -lgxsecure
endif
LIBS += -lgxfrontend -lui -lgxosal -lgxav -lgxbuscore -lgxcore
ifdef CONFIG_WIFI_SUPPORT
LIBS    += -lusbwifi
endif
LIBS += -ljpeg -lfreetype -ltiff
# when gxbus is compiled with O0, must open the following libs
#LIBS    += -lxml -lpng -lmpeg2 -lwebp


