SRC += $(shell find ./ -maxdepth 1 -name "*.c")
SRC += $(shell find ./$(BUILD_CAS) -name "*.c")

ifeq ($(OS),ecos)
SRC += bsp.cpp
endif

LIB = libapp.a
BIN = out.elf

CFLAGS  += -I$(GXLIB_PATH)/include/frontend
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/player
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/frontend
CFLAGS  += -I$(GXLIB_PATH)/include/gx_gui

LDFLAGS += -lgxcore -lgxfrontend -lgxav -Ttarget.ld -ltarget -lgxcore
default: del all
	
del:
	rm -rf out.elf
	
##############################################################################
LDFLAGS += -lgxplayer -lgxcaption -lgxdvb -lui -lwebp -lgxbuscore -lgxcore -lfreetype -ljpeg  -lpng -lxml -lcyassl -ltiff -lz -lmpeg2 -lgxdvb -lgxcas -lenca
#LDFLAGS += -lgxplayer -ldvbs -lui -lgxcore -lfreetype -ljpeg  -lpng -lxml -lcyassl -ltiff -lz -lmpeg2 -ldvbs -lgxcas -lenca -lgxfrontend_ex
-include  ./$(BUILD_CAS)/Makefile.a
-include  $(GXSRC_PATH)/scripts/inc.Makefile
##############################################################################
