-include $(GXSRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mak

ifeq ($(ENABLE_MEMWATCH), yes)
	CFLAGS += -DMEMWATCH
endif
MAKEFLAGS += -s
#WARNING += -Wundef -Wall -Wno-unused-function #-Wstrict-prototypes
CFLAGS  += -pipe -O0 -g -I$(GXLIB_PATH)/include -I$(GXLIB_PATH)/include/bus  $(TARGET_DEFS) -DARCH=$(ARCH) $(WARNING) -I.
CFLAGS += -I$(GXLIB_PATH)/include/bus/gui_core
LIBS += -lgxcore
LDFLAGS += -L$(GXLIB_PATH)/lib -L$(GXLIB_PATH)/lib/gxcas -static
LDFLAGS += -lgxcore
MAKE += -s

OBJS=$(addprefix objects/, $(addsuffix .o, $(basename $(notdir $(SRC)))))

all: env $(BEFORE) deps objects $(OBJS) $(LIB) $(BIN)

env:
ifndef GXLIB_PATH
	$(error Error: you must set the GXLIB_PATH environment variable to point to your gxsoft Path.)
endif
#	sh $(GXSRC_PATH)/scripts/create_signal_connect.sh
#	find $(GXSRC_PATH) -name "app_sysinfo.o" | xargs rm -f

# automatic generation of all the rules written by vincent by hand.
deps: $(SRC) Makefile
	@echo "Generating new dependency file...";
	@-rm -f deps;
	@for f in $(SRC); do \
		OBJ=objects/`basename $$f|sed -e 's/\.cpp/\.o/' -e 's/\.cxx/\.o/' -e 's/\.cc/\.o/' -e 's/\.c/\.o/'`; \
		echo $$OBJ: $$f>> deps; \
		echo '	@echo -e "compiling \033[032m[$(CC)]\033[0m": ' $$f >> deps; \
		echo '	$(CC) $$(CFLAGS) -c -o $$@ $$^'>> deps; \
	done

-include ./deps
#-include ./$(ARCH)-$(OS)-compiler.mak

objects:
	@echo $(CFLAGS)
	@echo $(LDFLAGS)
	@mkdir objects
.PHONY: madlib

$(LIB): objects $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@


$(BIN): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

subdirs:
	@list='$(SUBDIRS)'; \
		for subdir in $$list; do \
			echo "Making $$target in $$subdir"; \
			cd $$subdir && $(MAKE); \
			cd ..; \
		done;

subdirsclean:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		echo "Making $$target in $$subdir"; \
		cd $$subdir && $(MAKE) clean; \
		cd ..; \
	done;

subdirinstall:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		echo "Making $$target in $$subdir"; \
		cd $$subdir && $(MAKE) install; \
		cd ..; \
	done;

install-dir:env subdirinstall
	install -d "$(GXLIB_PATH)/include"
	install -d "$(GXLIB_PATH)/lib"

clean: subdirsclean
	@rm -rf $(OBJS) *.o .*swp objects deps $(CLEANFILE) $(BIN) *.log $(LIB) cscope.* tags
	@find -name "*~" -exec rm {} \;


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
	sh $(GXSRC_PATH)/bin/mkimg.sh
