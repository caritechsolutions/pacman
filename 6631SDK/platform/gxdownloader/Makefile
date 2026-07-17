INC  	= -I$(GXSRC_PATH)/               \
		  -I$(GXSRC_PATH)/include        \
		  -I$(GXSRC_PATH)/include/generated \
		  -I$(GXLIB_PATH)/include        \
		  -I$(GXLIB_PATH)/include/av     \
		  -I$(GXLIB_PATH)/include/gxos   \
		  -I$(GXLIB_PATH)/include/sys    \
		  -I$(GXLIB_PATH)/include/downloader

CURDIR         := $(shell pwd)
obj            := $(CURDIR)/scripts/kconfig
SUBDIR         := $(obj)

.PHONY : clean defconfig menuconfig savedefconfig

savedefconfig: $(obj)/conf
	$< --$@=defconfig Kconfig
	$< --silentoldconfig Kconfig

defconfig: $(obj)/conf
	$< Kconfig
	$< --silentoldconfig Kconfig

menuconfig: $(obj)/mconf $(obj)/conf
	$(obj)/mconf Kconfig
	$(obj)/conf --silentoldconfig Kconfig

silentoldconfig: $(obj)/conf
	$< $(silent) --$@ Kconfig

%_defconfig: $(obj)/conf
	$< $(silent) --defconfig=configs/$@ Kconfig

$(obj)/conf:
	$(Q)$(MAKE) -C $(SUBDIR) prepare
	$(Q)$(MAKE) -C $(SUBDIR)

$(obj)/mconf:
	$(Q)$(MAKE) -C $(SUBDIR) prepare
	$(Q)$(MAKE) -C $(SUBDIR)


-include $(GXSRC_PATH)/scripts/inc.Makefile

clean :
	@find . -name "*.o"|xargs rm -rf
	@rm -f $(COBJS-y) $(SOBJS-y)
	@rm -f gxota.elf gxota.bin ota.img
	@rm -f target.ld
	@rm -rf _build-*
	@echo -e make $@ finished

