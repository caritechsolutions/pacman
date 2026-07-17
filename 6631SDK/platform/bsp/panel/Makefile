-include $(GXSRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mak

ifeq ($(OS), ecos)
PANEL_TARGET:= libpanel.a
CFLAGS := -DECOS_OS
ifeq ($(GX_OPTIMIZE), Os)
CFLAGS += -Os -ffunction-sections -fdata-sections
else ifeq ($(GX_OPTIMIZE), O0)
CFLAGS += -O0 -g
else
CFLAGS += -O2 -g -ffunction-sections -fdata-sections
endif
CFLAGS += -I$(GXLIB_PATH)/include
CFLAGS += -I$(GXSRC_PATH)/include
panel-objs := bsp_panel.o panel_mod_ecos.o
all: $(PANEL_TARGET) install

CFLAGS += $(GX_EXT_CFLAGS)

$(PANEL_TARGET): $(panel-objs)
	@$(AR) -r $@ $^

%.o: %.c
	@echo -e "compiling \033[032m[$(CC)]\033[0m": $<
	@$(CC) -c $(CFLAGS) $< -o $(dir $<)/$(notdir $@)

install:
	@echo "\033[1;31mInstalling: $(GXLIB_PATH)/lib/$(PANEL_TARGET)\033[0m"
	@if [ -e $(PANEL_TARGET) ]; then install -p -m 644 $(PANEL_TARGET) $(GXLIB_PATH)/lib; fi
	@echo "\033[1;31mInstalling: $(GXLIB_PATH)/include/bsp_dev_panel.h\033[0m"
	@install -p -m 644 bsp_dev_panel.h $(GXLIB_PATH)/include
endif

ifeq ($(OS), linux)
PANEL_TARGET:= panel.ko
EXTRA_CFLAGS := -DLINUX_OS
EXTRA_CFLAGS += -I$(GXSRC_PATH)/include
panel-objs := bsp_panel.o panel_mod_linux.o
obj-m += panel.o
J=-j$(shell nproc)
GXDRIVER_PATH=$(GXLIB_PATH)/lib/drivers

all: $(PANEL_TARGET) install

$(PANEL_TARGET):
	$(MAKE) ${J} ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(GX_KERNEL_PATH) SUBDIRS=`pwd`

install:
	@echo "\033[1;31mInstalling: $(GXDRIVER_PATH)/$(PANEL_TARGET)\033[0m"
	@if [ -e $(PANEL_TARGET) ]; then install -p $(PANEL_TARGET) $(GXDRIVER_PATH); fi
	@echo "\033[1;31mInstalling: $(GXLIB_PATH)/include/bsp_dev_panel.h\033[0m"
	@install -p -m 644 bsp_dev_panel.h $(GXLIB_PATH)/include

endif

clean:
	rm -rf $(PANEL_TARGET) *.o *.ko *.mod.c .tmp_versions Module.symvers modules.order .tmp_versions
	find ../ -name "*.cmd" -delete


