-include include/config/auto.conf
-include $(GXSRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mak

ifeq ($(OS), nos)
SOBJS-y = cpu/$(ARCH)/nos/vectors.o
COBJS-y = init.o         \
          main.o         \
          cmdline.o
else
COBJS-y = main.o
CPPOBJS-y = bsp.o
COBJS-y += $(patsubst %.c, %.o, $(wildcard gx/src/net/*.c))
INC  += -I$(GXSRC_PATH)/gx/src
INC  += -I$(GXSRC_PATH)/gx/src/menu
INC  += -I$(GXSRC_PATH)/gx/src/net
endif

CFLAGS += $(INC)
ifeq ($(GX_OPTIMIZE), Os)
	CFLAGS += -Os -ffunction-sections -fdata-sections
else ifeq ($(GX_OPTIMIZE), O0)
	CFLAGS += -O0 -g -DDEBUG
else
	CFLAGS += -O2 -ffunction-sections -fdata-sections
endif
ifeq ($(GX_DEBUG_HOLE), 1)
	CFLAGS += -DDBG_QM_MALLOC
endif

CFLAGS += -fomit-frame-pointer -fno-builtin -nostdlib
ifeq ($(OS), nos)
CFLAGS += -nostdinc
endif
CFLAGS += -pipe
CFLAGS += ${CT} #used for chiptest
CFLAGS += -DGXLOG_LEVEL=$(GX_LOGLEVEL)

ifeq ($(OS), nos)
LIBGCC_PATH = $(shell dirname `$(CC) -print-libgcc-file-name`)
LIBGCC = -L$(LIBGCC_PATH) -lgcc
endif

ifeq ($(ARCH), csky)
LDFLAGS += --gc-sections -EL
else
ifeq ($(OS), nos)
LDFLAGS += --gc-sections -EL
endif
endif
LDFLAGS += -L$(GXLIB_PATH)/lib
LDFLAGS += $(GX_EXT_CFLAGS)
LD_FILE = target.ld
ifeq ($(OS), ecos)
LD_FILE_NAME = cpu/$(ARCH)/ecos/target.ld
ifeq ($(IMAGE), image)
$(shell $(CC) $(CFLAGS) -E -x assembler-with-cpp -P -o $(LD_FILE) $(LD_FILE_NAME))
endif
else
ifeq ($(OS), nos)
$(shell cp $(GXLIB_PATH)/lib/target.ld $(LD_FILE))
endif
endif

gxota.bin :gxota.elf
	@$(OBJCOPY) -S -g -O binary $< $@
	@rm -rf bin
	@lzma $@   #@lzma $@  在slt芯片测试中, gzip压缩的bin, 在内核启动时解压速度更快, 并且压缩大小和lzma压缩的大小接近
	@mkdir bin
	@mv $@.lzma bin/ota.bin.lzma #@mv $@.lzma bin/ota.bin.lzma
	@start_addr=`$(NM) gxota.elf|grep __text_start__ -w|sed -e 's/ .*//'|sed -e 's/^/0x/'`;\
	genromfs -f ota.img -d bin/ -V OTA$$start_addr
	@rm -rf bin

gxota.elf:$(SOBJS-y) $(COBJS-y) $(CPPOBJS-y)
	@echo "\033[035mlinking\033[0m : \033[032m$^ > $@\033[0m"
	@$(LD) $(LDFLAGS) -T$(LD_FILE) -o $@ $^ $(LIBS) $(LIBGCC)
	@rm -f $(COBJS-y) $(SOBJS-y)

$(SOBJS-y):%.o:%.S
	@echo "\033[035mcompiling\033[0m \033[033m[$(CC)]\033[0m: \033[032m$<\033[0m"
	@$(CC) $(CFLAGS)  -c $(CPPFLAGS) $< -o $@

$(COBJS-y):%.o:%.c
	@echo "\033[035mcompiling\033[0m \033[033m[$(CC)]\033[0m: \033[032m$<\033[0m"
	@$(CC) $(CFLAGS)  -c $(CPPFLAGS) $< -o $@

$(CPPOBJS-y): %.o: %.cpp
	@echo "\033[035mcompiling\033[0m \033[033m[$(CPP)]\033[0m: \033[032m$<\033[0m"
	@$(CPP) $(CFLAGS) -c $(CPPFLAGS) $< -o $@

$(OTA) : $(O_OBJS)
	@$(AR) -rcs $@ $^

