-include $(GXSRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.conf.mak

ifeq ($(ENABLE_MEMWATCH), yes)
	CFLAGS += -DMEMWATCH
endif

MAKEFLAGS += -s
WARNING += -Wundef -Wall #-Wstrict-prototypes

CFLAGS += -fdata-sections -ffunction-sections
#LDFLAGS += -Wl,--print-gc-sections

ifeq ($(OS), ecos)
#CFLAGS  += -pipe -O0 -g -I$(GXLIB_PATH)/include -I$(GXLIB_PATH)/include/bus  $(TARGET_DEFS) -DARCH=$(ARCH) $(WARNING) -I.
CFLAGS  += -pipe -Os -g -DNDEBUG -I$(GXLIB_PATH)/include -I$(GXLIB_PATH)/include/bus  $(TARGET_DEFS) -DARCH=$(ARCH) $(WARNING) -I.
#CFLAGS  += -pipe -g -O0 -DMEMORY_DEBUG -I$(GXLIB_PATH)/include -I$(GXLIB_PATH)/include/bus  $(TARGET_DEFS) -DARCH=$(ARCH) $(WARNING) -I.
LIBS += -lgxcore
LIBS += -lpanel -lgpio
BIN_PATH =$(GXSRC_PATH)/tools/genflash
endif

ifeq ($(OS), linux)
CFLAGS  += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
CFLAGS  += -Winline -pipe -O2 -I$(GXLIB_PATH)/include -I$(GXLIB_PATH)/include/bus  $(TARGET_DEFS) -DARCH=$(ARCH) $(WARNING) -I.
#CFLAGS  += -Winline -pipe -O0 -g -ggdb -I$(GXLIB_PATH)/include -I$(GXLIB_PATH)/include/bus  $(TARGET_DEFS) -DARCH=$(ARCH) $(WARNING) -I.
#CFLAGS  += -Winline -pipe -O0 -g -ggdb -DMEMORY_DEBUG -DDBG_QM_MALLOC=1 -I$(GXLIB_PATH)/include -I$(GXLIB_PATH)/include/bus  $(TARGET_DEFS) -DARCH=$(ARCH) $(WARNING) -I.
CFLAGS += -I$(GXLIB_PATH)/include/kernel_include/
ifeq ($(BR2_MOD_APP_APPLETS), y)
LIBS += -ldl
LDFLAGS +=  -rdynamic
else
ifeq ($(BR2_MOD_APP_MAIN_MENU_OBJ), y)
LIBS += -ldl
LDFLAGS +=  -rdynamic
else
LDFLAGS += -static
endif
endif
ifeq ($(CHIP), GX3201)
CFLAGS += -mkstq2
LDFLAGS += -mkstq2
endif
ifeq ($(ARCH), csky)
CFLAGS += -Wno-misleading-indentation
endif
LIBS += -lgxcore -lz -lrt
ifeq ($(BR2_MOD_APP_PLUGINSTORE), y)
LIBS += -Wl,--whole-archive -lpluginstore -Wl,--no-whole-archive -lsqlite3 -ldl -lpthread
endif
BIN_PATH =$(GXSRC_PATH)/tools/genflash

ifeq ($(ARCH), arm)
CFLAGS += -fopenmp
LDFLAGS += -fopenmp
LIBS += -lm
endif
endif
ifeq ($(BR2_MOD_APP_FACE_DETECT), y)
LIBS += -lface_detect -lncnn
endif

CFLAGS  += -I$(GXSRC_PATH)/app/richepg/include
CFLAGS  += -I$(GXSRC_PATH)/app/richepg/include/export

CFLAGS  += -I$(GXSRC_PATH)/include/$(ARCH)-$(OS)
LDFLAGS += -L$(GXSRC_PATH)/lib/$(ARCH)-$(OS)

LDFLAGS += -L$(GXLIB_PATH)/lib
LDFLAGS += -L$(GXSRC_PATH)/lib
MAKE += -s

OBJS=$(addprefix $(GXSRC_PATH)/output/objects/, $(addsuffix .o, $(basename $(notdir $(SRC)))))
CXXFLAGS=$(filter-out -Wstrict-prototypes,$(CFLAGS))

all: env $(BEFORE) deps objects $(OBJS) $(LIB) $(BIN)

env:
ifndef GXLIB_PATH
	$(error Error: you must set the GXLIB_PATH environment variable to point to your gxsoft Path.)
endif
	#sh $(GXSRC_PATH)/scripts/create_signal_connect.sh
	find $(GXSRC_PATH) -name "app_sysinfo.o" | xargs rm -f

# automatic generation of all the rules written by vincent by hand.
deps: $(SRC)
	@echo "Generating new dependency file...";
	@-rm -f deps;
	@for f in $(SRC); do \
		OBJ=$(GXSRC_PATH)/output/objects/`basename $$f|sed -e 's/\.c/\.o/'`; \
		echo $$OBJ: $$f>> deps; \
		echo '	@echo -e "compiling \033[032m[$(CC)]\033[0m": ' $$f >> deps; \
		echo '	$(CC) $$(CFLAGS) -c -o $$@ $$^'>> deps; \
	done
	@for f in $(SRC); do \
		OBJ=$(GXSRC_PATH)/output/objects/`basename $$f|sed -e 's/\.cpp/\.o/' -e 's/\.cxx/\.o/' -e 's/\.cc/\.o/'`; \
		echo $$OBJ: $$f>> deps; \
		echo '	@echo -e "compiling \033[032m[$(CPP)]\033[0m": ' $$f >> deps; \
		echo '	$(CPP) $$(CXXFLAGS) -c -o $$@ $$^'>> deps; \
	done

-include ./deps
#-include ./$(ARCH)-$(OS)-compiler.mak

objects:
	@mkdir	-p $(GXSRC_PATH)/output/objects
	@echo "making objects dir"


$(LIB): objects $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@


$(BIN): $(OBJS)
	@echo "LDFLAGS:"$(LDFLAGS)
	$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@ -Wl,-Map,$(GXSRC_PATH)/output/$(BIN).map
	@mv $@ $(GXSRC_PATH)/output

subdirs:
	@list='$(SUBDIRS)'; \
		for subdir in $$list; do \
			echo "Making $$target in $$subdir"; \
			cd $$subdir && $(MAKE) -f Makefile; \
			if [ "$$?" != "0" ]; then\
			exit 1;\
			fi;\
			cd ..; \
		done;

subdirsclean:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		echo "Making $$target in $$subdir"; \
		cd $$subdir && $(MAKE) -f Makefile clean; \
		if [ "$$?" != "0" ]; then\
			exit 1;\
		fi;\
		cd ..; \
	done;

subdirinstall:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		echo "Making $$target in $$subdir"; \
		cd $$subdir && $(MAKE) -f Makefile install; \
		if [ "$$?" != "0" ]; then\
                        exit 1;\
                fi;\
		cd ..; \
	done;

install-dir:env subdirinstall
	install -d "$(GXLIB_PATH)/include"
	install -d "$(GXLIB_PATH)/lib"

clean: subdirsclean
	@rm -rf $(OBJS) *.o .*swp output/objects deps $(CLEANFILE) $(BIN) *.log $(LIB)
	@find -name "*~" -exec rm {} \;
	@rm -f system/configin/Config.in.netapps
	#@echo "clean ...."
	#@echo $(BUILD_DIR)

format:
	@echo "Makeing format...";
	@find -name "*.c" -exec dos2unix -qU 2>d2utmp1 {} \;
	@find -name "*.h" -exec dos2unix -qU 2>d2utmp1 {} \;
#	@find -name "*.c" -exec indent -npro -kr -i8 -sob -l120 -ss -ncs  {} \;
	@find -name "*~" -exec rm {} \;
	@find -name "d2utmp*" -exec rm {} \;
	@find -name "deps*" -exec rm {} \;

.PHONY: bin
bin:
	bash $(BIN_PATH)/mkimg.sh $(CROSS_COMPILE)
