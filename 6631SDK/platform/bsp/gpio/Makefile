-include $(GXSRC_PATH)/scripts/$(ARCH)-$(OS)-compiler.mak

ifeq ($(OS), ecos)
GPIO_TARGET:= libgpio.a
CFLAGS := -DECOS_OS
ifeq ($(GX_OPTIMIZE), Os)
CFLAGS += -Os -ffunction-sections -fdata-sections
else ifeq ($(GX_OPTIMIZE), O0)
CFLAGS += -O0 -g
else
CFLAGS += -O2 -g -ffunction-sections -fdata-sections
endif
CFLAGS += -I$(GXLIB_PATH)/include
gpio-objs := bsp_gpio.o gpio_mod_ecos.o
all: $(GPIO_TARGET) install

CFLAGS += $(GX_EXT_CFLAGS)

$(GPIO_TARGET): $(gpio-objs)
	@$(AR) -r $@ $^

%.o: %.c
	@echo -e "compiling \033[032m[$(CC)]\033[0m": $<
	@$(CC) -c $(CFLAGS) $< -o $(dir $<)/$(notdir $@)

install:
	@echo "\033[1;31mInstalling: $(GXLIB_PATH)/lib/$(GPIO_TARGET)\033[0m"
	@ if [ -e $(GPIO_TARGET) ]; then install -p -m 644 $(GPIO_TARGET) $(GXLIB_PATH)/lib; fi
	@echo "\033[1;31mInstalling: $(GXLIB_PATH)/include/bsp_dev_gpio.h\033[0m"
	@install -p -m 644 bsp_dev_gpio.h $(GXLIB_PATH)/include
endif

ifeq ($(OS), linux)
GPIO_TARGET = gpio.ko
EXTRA_CFLAGS := -DLINUX_OS
gpio-objs := bsp_gpio.o gpio_mod_linux.o
obj-m += gpio.o
J=-j$(shell nproc)
GXDRIVER_PATH=$(GXLIB_PATH)/lib/drivers

all: $(GPIO_TARGET) install

$(GPIO_TARGET):
	$(MAKE) ${J} ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(GX_KERNEL_PATH) SUBDIRS=`pwd`

install:
	@echo "\033[1;31mInstalling: $(GXDRIVER_PATH)/$(GPIO_TARGET)\033[0m"
	@if [ -e $(GPIO_TARGET) ]; then install -p $(GPIO_TARGET) $(GXDRIVER_PATH); fi
	@echo "\033[1;31mInstalling: $(GXLIB_PATH)/include/bsp_dev_gpio.h\033[0m"
	@install -p -m 644 bsp_dev_gpio.h $(GXLIB_PATH)/include
endif

clean:
	rm -rf $(GPIO_TARGET) *.a *.ko *.o *.mod.c .tmp_versions Module.symvers modules.order .tmp_versions
	find ../ -name "*.cmd" -delete
