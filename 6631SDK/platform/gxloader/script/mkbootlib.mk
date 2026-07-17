boot-COBJS-y = boot.o
boot-COBJS-y += load_kernel/load_kernel.o

boot-COBJS-y += common/decompress/decompress.o
boot-COBJS-$(ENABLE_COMPRESS_ZLIB) += common/decompress/zlib_deflate/deflate.o     \
		common/decompress/zlib_deflate/deftree.o                                         \
			common/decompress/zlib_deflate/bitrev.o                                          \
				common/decompress/zlib/compress.o
boot-COBJS-$(ENABLE_DECOMPRESS_ZLIB) += common/decompress/zlib_inflate/inffast.o   \
		common/decompress/zlib_inflate/inflate.o                                       \
			common/decompress/zlib_inflate/inftrees.o                                      \
				common/decompress/zlib_inflate/infutil.o                                       \
					common/decompress/zlib/uncompr.o
boot-COBJS-$(ENABLE_DECOMPRESS_LZO)  += common/decompress/decompress_unlzo.o
boot-COBJS-$(ENABLE_DECOMPRESS_LZMA) += common/decompress/decompress_unlzma.o
boot-COBJS-$(ENABLE_DECOMPRESS_GZIP) += common/decompress/decompress_inflate.o
boot-COBJS-$(ENABLE_ROOTFS_ROMFS)  += load_kernel/romfs.o
boot-COBJS-$(ENABLE_UIMAGE)  += load_kernel/uImage.o
boot-COBJS-$(ENABLE_ROOTFS_CRAMFS)  += \
		load_kernel/cramfs.o       \
			load_kernel/uncompress.o

boot-COBJS-$(ENABLE_KERNEL_DTB) += fdt/src/fdt.o \
	fdt/src/fdt_ro.o \
	fdt/src/fdt_rw.o \
	fdt/src/fdt_strerror.o \
	fdt/src/fdt_sw.o \
	fdt/src/fdt_wip.o \
	fdt/src/fdt_empty_tree.o \
	fdt/src/fdt_addresses.o \
	fdt/src/fdt_region.o   \
	fdt/fdt_support.o

libboot.a: $(boot-COBJS-y)
	@$(AR) -rcs $@ $^
	@rm -f $(COBJS-y) $(SOBJS-y)
	@mv $@ output/

