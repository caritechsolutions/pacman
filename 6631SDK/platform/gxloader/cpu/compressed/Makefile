-include .config

all: loader_compressed_stage2.bin

LD_FILE_NAME=gxloader_compressed.lds
LD_FILE=cpu/compressed/gxloader_compressed.lds

loader-stage2.bin: loader-flash.bin
	@dd if=$< of=$@ bs=1 skip=$(shell printf "%d" $(STAGE2_LMA_ADDR)) > /dev/null 2>&1

piggy_data: loader-stage2.bin
ifeq ($(CONFIG_STAGE2_COMPRESSED_GZIP), y)
	@cat $< | gzip -n -f -9 > $@
endif
ifeq ($(CONFIG_STAGE2_COMPRESSED_LZMA), y)
	@(cat $< | lzma -9 && printf \\000\\360\\161\\000) > $@
endif
	@rm loader-stage2.bin

cpu/compressed/piggy.o: piggy_data

ifeq ($(CONFIG_ARCH), ARM)
SOBJS-y += cpu/arm/cpu/armv7/v7_dcache.o \
	cpu/arm/cpu/armv7/v7_icache.o \
	cpu/arm/cpu/armv7/mmu_cache.o
endif

SOBJS-y += cpu/compressed/start.o
SOBJS-y += cpu/compressed/piggy.o
COBJS-y += cpu/compressed/decompress.o
COBJS-y += common/decompress/decompress.o
COBJS-$(ENABLE_DECOMPRESS_ZLIB) += common/decompress/zlib_inflate/inffast.o   \
	common/decompress/zlib_inflate/inflate.o                                       \
	common/decompress/zlib_inflate/inftrees.o                                      \
	common/decompress/zlib_inflate/infutil.o                                       \
	common/decompress/zlib/uncompr.o
COBJS-$(ENABLE_DECOMPRESS_LZMA) += common/decompress/decompress_unlzma.o
COBJS-$(ENABLE_DECOMPRESS_GZIP) += common/decompress/decompress_inflate.o

COBJS-y += libc/q_malloc.o    \
	libc/fakelibc.o           \
	libc/string.o             \
	libc/math.o               \
	libc/errno.o              \
	libc/div64.o

COBJS-y += common/kfifo.o

COBJS-y += libc/vsprintf.o
COBJS-y += libc/vsnprintf.o

ifeq ($(CONFIG_UART_TYPE), DW)
COBJS-y += drivers/serial/dw_uart.o
else
COBJS-y += drivers/serial/gx_uart.o
endif

$(sort $(SOBJS-y)):%.o:%.S
	@echo [$(CC) compiling $@]
	@$(CC) $(CFLAGS) -DBOOT_FROM_FLASH -c $(CPPFLAGS) $< -o $@
$(sort $(COBJS-y)):%.o:%.c
	@echo [$(CC) compiling $@]
	@$(CC) $(CFLAGS)  -c $(CPPFLAGS) $< -o $@

loader_compressed.elf: $(COBJS-y) $(SOBJS-y)
	@-rm -f $(LD_FILE_NAME)
	@$(CC) $(CPPFLAGS) -E -x assembler-with-cpp -P -o $(LD_FILE_NAME) $(LD_FILE)
	@sed -i "s/\(.*stage2.*ORIGIN =\).*,/\1 $(LOADER_COMPRESSED_START_ADDR),/g" $(LD_FILE_NAME)
	@sed -i "s/\(.*stage2.*LENGTH =\).*/\1 $(LOADER_COMPRESSED_SIZE)/g" $(LD_FILE_NAME)
	@$(LD) $(LDFLAGS) -T$(LD_FILE_NAME) -o $@ $^ $(LIBS) $(LIBGCC)
	@rm -f $(COBJS-y) $(SOBJS-y)

loader_compressed_stage2.bin: loader_compressed.elf
	@$(OBJCOPY) -Obinary -R .bss $< $@

clean:
	@-rm -f loader-stage2.bin loader_compressed_stage2.bin
	@-rm -f $(COBJS-y) $(SOBJS-y)
	@-rm -f $(LD_FILE_NAME)

.PHONY: clean
