SRC += $(shell find ./ -maxdepth 1 -name "*.c")

ifeq ($(OS),ecos)
SRC += bsp.cpp
endif

LIB = libapp.a
BIN = out.elf

CFLAGS  += -I$(GXLIB_PATH)/include/frontend
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/player
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/frontend
CFLAGS  += -I$(GXLIB_PATH)/include/gx_gui

LDFLAGS += -lgxcore -lgxav -Ttarget.ld -ltarget -lgxcore
default: del all
	
del:
	rm -rf out.elf
	
##############################################################################
LDFLAGS +=  -lgxcas
-include  ./$(BUILD_CAS)/Makefile.a
-include  $(GXSRC_PATH)/scripts/inc.Makefile
##############################################################################
