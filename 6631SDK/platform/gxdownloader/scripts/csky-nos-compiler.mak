CROSS_COMPILE=csky-elf-
CC     = $(CROSS_COMPILE)gcc
CPP    = $(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE)ld
AR     = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
OBJCOPY= $(CROSS_COMPILE)objcopy
NM     = $(CROSS_COMPILE)nm

CFLAGS =-DNO_OS -DLITTLE_ENDIAN
LDFLAGS +=
CFLAGS += -fno-builtin

LIBS += -ldownloader
ifdef CONFIG_SECURE_SUPPORT
LIBS += --whole-archive -lgxosal -lgxav -lgxsecure --no-whole-archive
else
LIBS += -lgxosal -lgxav -lgxsecure
endif
LIBS += -lgxfrontend -lui -lxml -lgxosal -lgxav -lfreetype -lgxplayer -lgxbuscore -lgxcore -lnos
