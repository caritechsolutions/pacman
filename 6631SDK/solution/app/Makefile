##############################################################################
SUBDIRS=

ifeq ($(OS), ecos)
SRC += $(shell find . -iname "*.c"  | grep -v "./opensource/*" | grep -v "./networkmanager/*" | grep -v "./module/app_curl.c")
SRC += $(shell find ./netapps/dlna/httpd/ -iname "*.cpp")
SRC += signal_connect_cplus.cpp
SRC += bsp.cpp
SRC += ./networkmanager/app_wifi.c
SRC += ./networkmanager/app_bluetooth.c
SRC += ./networkmanager/app_network.c
SRC += ./networkmanager/app_3g.c
SRC += ./networkmanager/app_vpn.c
endif

ifeq ($(OS), linux)
SRC += $(shell find . -iname "*.c")
SRC += $(shell find . -iname "*.cpp")
#SRC += $(shell find . -iname "*.c" | grep -v "./opensource/*")
endif



LIB = libapp.a
BIN = out.elf

CFLAGS += -I./include
#CFLAGS += -I./module/cim

CFLAGS += -I./log/include

CFLAGS += -I./networkmanager/include
CFLAGS += -I./player/include
CFLAGS += -I./netapps
CFLAGS += -I./voice_control/include
CFLAGS += -I./netapps/subt_trans/include

CFLAGS  += -I$(GXLIB_PATH)/include/bus
CFLAGS  += -I$(GXLIB_PATH)/include/bus/gui_core
CFLAGS  += -I$(GXLIB_PATH)/include/bus/service
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/player
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/ca
CFLAGS  += -I$(GXLIB_PATH)/include/app/ca
CFLAGS  += -I$(GXSRC_PATH)/include

default: del all

del:
	rm -rf out.elf

##############################################################################
-include  $(GXSRC_PATH)/scripts/inc.Makefile.conf.mak
##############################################################################

# Include library makefile auto generated
# Ref: scripts/factory_makefile_lib.sh
-include Makefile.a
-include Makefile_w.a
-include Makefile_netapps

ifeq ($(OS), ecos)
 LDFLAGS += -lui -Wl,--start-group -lgxplayer -lsmartvolume -lgxdvb -Wl,--end-group -lgxbuscore -lgxcaption -lgxhotplug -lgxfrontend -lgxav -lfreetype -ljpeg -lpng -ldb -lenca -lxml -ljansson -lcjson -lcyassl -lgxcore -lz -lwebp -lcurl -ltiff -lmpeg2 -lstdc++ -lzvbi -liconv -lgxsecure -lqrencode -lisakmp -lopenvpn -llzo -lgxbusmisc
endif

# -lrist is wrapped in --as-needed on purpose. The app does not use librist at
# all: no source here includes a librist header or calls a librist symbol -- the
# flag is left over from the abandoned userspace-decrypt approach. But GNU ld
# records DT_NEEDED for -l flags regardless of whether any symbol is satisfied,
# so the app carried a hard runtime dependency on librist.so.4 for a library it
# never touches. A librist that fails to load would then stop the whole STB app
# from starting -- a reflash-only brick, caused by a component the app does not
# use, and we have just started modifying librist.
#
# --as-needed records DT_NEEDED only if the library actually resolves an
# undefined symbol, so this drops the dependency if it is vestigial (expected)
# and keeps it if something does need it after all. Scoped with --no-as-needed
# immediately after, because applying it to the whole list could drop libraries
# that are only needed at runtime. Confirm with: readelf -d out.elf | grep NEEDED
ifeq ($(OS), linux)
 LDFLAGS += -lui -Wl,--start-group -lgxplayer -lsmartvolume -lgxdvb -Wl,--end-group -lgxbuscore -lgxcaption -lgxhotplug -lfreetype -ljpeg -lpng -ldb -lenca -lxml -ljansson -lcjson -lcyassl -lgxcore -lz  -lwebp -lcurl -ltiff -lmpeg2 -lxml2 -lstdc++ -lzvbi -liconv -lqrencode -ltinyalsa -lopenvpn -llzo -Wl,--as-needed -lrist -Wl,--no-as-needed -lgxbusmisc -lnl-3 -lnl-genl-3
endif

print_ld:
	@echo "--------------------------------------LDFLAGS------"
	@echo $$LDFLAGS

#test:
#	@echo making test begin...
#	$(CC)  main.c  -o test \
#	$(CFLAGS)                       \
#	-L$(GXLIB_PATH)/lib -lservice -L./ -lapp $(LDFLAGS) -lui
