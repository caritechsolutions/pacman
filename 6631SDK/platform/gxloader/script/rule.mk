#/*
# * =====================================================================================
# *
# *       Filename:  rule.mk
# *
# *       Compiler:  gcc
# *       Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
# *
# * =====================================================================================
# */
# Memory layout arithmetic

ifeq ($(CONFIG_ARCH), ARM)
SRAM_ADDR_HEAD	    = 8
DRAM_ADDR_HEAD	    = 0
DRAMBASE            = 0x$(DRAM_ADDR_HEAD)0000000
SRAMBASE	    = 0x$(SRAM_ADDR_HEAD)0400000
SRAM_SIZE           = 0x00008000
VIR_PHY_OFFSET      = 0x00000000
DRAM_PHY_ADDR	    = 0x00000000
else
# CSKY
REG_ADDR_HEAD	    = a
DRAM_ADDR_HEAD	    = 9
DRAMBASE            = 0x$(DRAM_ADDR_HEAD)0000000
SRAMBASE	    = 0x$(REG_ADDR_HEAD)0100000
SRAM_SIZE           = 0x00004000
VIR_PHY_OFFSET      = 0x80000000
DRAM_PHY_ADDR	    = 0x10000000
endif

TEMP_CFG_FILE       = .tmp_file
CONF_FILE           = .cfg

CMDLINE_VALUE :=	$(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE))

ifneq ($(CONFIG_MULTI_CMDLINE), )
TEMP_CFG_FILE1      = .tmp_file1
CONF_FILE1          = .cfg1
TEMP_CFG_FILE2      = .tmp_file2
CONF_FILE2          = .cfg2
TEMP_CFG_FILE3      = .tmp_file3
CONF_FILE3          = .cfg3
TEMP_CFG_FILE4      = .tmp_file4
CONF_FILE4          = .cfg4
TEMP_CFG_FILE5      = .tmp_file5
CONF_FILE5          = .cfg5
TEMP_CFG_FILE6      = .tmp_file6
CONF_FILE6          = .cfg6
CMDLINE_VALUE1 := $(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE1) $(CONF_FILE1) CMDLINE_VALUE1)
CMDLINE_VALUE2 := $(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE2) $(CONF_FILE2) CMDLINE_VALUE2)
CMDLINE_VALUE3 := $(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE3) $(CONF_FILE3) CMDLINE_VALUE3)
CMDLINE_VALUE4 := $(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE4) $(CONF_FILE4) CMDLINE_VALUE4)
CMDLINE_VALUE5 := $(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE5) $(CONF_FILE5) CMDLINE_VALUE5)
CMDLINE_VALUE6 := $(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE6) $(CONF_FILE6) CMDLINE_VALUE6)
endif
include $(CONF_FILE)

ifdef CMDLINE_MEM_END
ifdef CMDLINE_VIDEOMEM_START
ifneq ($(CMDLINE_MEM_END), $(CMDLINE_VIDEOMEM_START))
$(error "Please check value of CMDLINE_VALUE, no other mem fields can be defined between "mem" and "videomem"; other mem fields need to be defined after "videomem".")
endif
endif
endif

CHIP_CORE_IN_CAPTIAL = $(shell echo $(CHIP_CORE) | tr '[:lower:]' '[:upper:]')

DRAM_SIZE_ALIGN := $(shell printf 0x%x $(shell ./script/calc.sh 1024 \* 1024 \* 16))

ifeq ($(DRAM_SIZE), )
	DRAM_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh \( $(CMDLINE_DRAM_SIZE) + $(DRAM_SIZE_ALIGN) - 1 \) / $(DRAM_SIZE_ALIGN) \* $(DRAM_SIZE_ALIGN)))
endif

ifeq ($(ENABLE_KERNEL_DTB), y)
ifeq ($(CONFIG_ARCH), ARM)
	KERNEL_DTB_OFFSET = 0x100
	KERNEL_DTB_SIZE = 0x3f00
else
	# CSKY
	KERNEL_DTB_OFFSET = 0x0
	KERNEL_DTB_SIZE = 0x4000
endif
else
	KERNEL_DTB_OFFSET = 0x0
	KERNEL_DTB_SIZE = 0x0
endif

ifeq ($(CONFIG_REE_LOADER), y)
	TEE_LOADER_BIN = tee-loader-flash.bin
endif

ifeq ($(KERNEL_SIZE), )
	KERNEL_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(CMDLINE_MEM_SIZE) / 2))
endif

ifeq ($(STACK_SIZE), )
	STACK_SIZE = 0x200000
endif

ifeq ($(ENABLE_IRQ), y)
	IRQ_STACK_SIZE = 0x8000
	CONFIG_STACKSIZE_IRQ = $(shell printf 0x%x $(shell ./script/calc.sh $(IRQ_STACK_SIZE) / 2))
else
	IRQ_STACK_SIZE = 0x0
endif

ifeq ($(BOOTLOADER_SIZE), )
	BOOTLOADER_SIZE = 0x20000
endif

ifeq ($(BOOTLOADER_BIN_SIZE), )
ifeq ($(ENABLE_BOOT_TOOL), y)
	BOOTLOADER_BIN_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(BOOTLOADER_SIZE) + $(BOOTLOADER_SIZE)))
else
	BOOTLOADER_BIN_SIZE = $(BOOTLOADER_SIZE)
endif
endif

ifeq ($(CONFIG_EMBED_KERNEL_IMG), y)
	CONFIG_EMBED_KERNEL_IMG_FILE := $(wildcard $(CONFIG_EMBED_KERNEL_IMG_FILE))
ifneq ($(CONFIG_EMBED_KERNEL_IMG_FILE), )
	EMBED_KERNEL_IMG_SIZE := $(shell ./script/align.sh $(shell stat -c%s ecos_romfs.img) 0x10000 1)
endif
endif

ifeq ($(BOOTLOADER_STAGE2_SIZE), )
ifeq ($(CONFIG_EMBED_KERNEL_IMG), y)
	BOOTLOADER_STAGE2_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(BOOTLOADER_BIN_SIZE) + $(EMBED_KERNEL_IMG_SIZE)))
else
	BOOTLOADER_STAGE2_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(BOOTLOADER_BIN_SIZE) + 0x80000))
endif
endif

ifeq ($(CONFIG_REE_LOADER), y)
	TEE_BOOTLOADER_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(BOOTLOADER_BIN_SIZE) + 0x80000))
else
	TEE_BOOTLOADER_SIZE = 0x0
endif

ifeq ($(OTA_DRAM_SIZE), )
	OTA_DRAM_SIZE = 0x0
endif

ifeq ($(ENABLE_OTA), y)
ifeq ($(ENABLE_OTA_FORCE_DRAM_ADDR), y)
ifeq ($(OTA_DRAM_SIZE), 0x0)
	OTA_DRAM_SIZE = 0x400000
endif
endif
endif

ifeq ($(ENABLE_LIB), y)
ifeq ($(ENABLE_OTA_FORCE_DRAM_ADDR), y)
ifeq ($(OTA_DRAM_SIZE), 0x0)
	OTA_DRAM_SIZE = 0x400000
endif
endif
endif

ifeq ($(LOGO_PIXEL_SIZE), )
ifeq ($(ENABLE_GDI), y)
	#default logo pixel size: 1920 * 1080 = 0x1fa400
	LOGO_PIXEL_SIZE = 0x1fe000
else
ifeq ($(ENABLE_LOGO), y)
	#default logo pixel size: 1920 * 1080 = 0x1fa400
	LOGO_PIXEL_SIZE = 0x1fe000
else
	LOGO_PIXEL_SIZE = 0x0
endif
endif
endif

ifeq ($(SPP_BUF_SIZE), )
ifeq ($(LOGO_PIXEL_SIZE), 0x0)
	SPP_BUF_SIZE = 0x0
else
	SPP_BUF_SIZE_ALIGN = 0x100000
	SPP_BUF_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh \( $(LOGO_PIXEL_SIZE) \* 3 + $(SPP_BUF_SIZE_ALIGN) \) / $(SPP_BUF_SIZE_ALIGN) \* $(SPP_BUF_SIZE_ALIGN)))
endif
endif

ifeq ($(SVPU_BUF_SIZE), )
	#default svpu buf size: 720 * 576 * 3 = 1244160 = 0x12fc00
ifeq ($(ENABLE_GDI), y)
	SVPU_BUF_SIZE = 0x130000
else
ifeq ($(ENABLE_LOGO), y)
	SVPU_BUF_SIZE = 0x130000
else
	SVPU_BUF_SIZE = 0x0
endif
endif
endif

ifeq ($(CMDLINE_SVPUMEM_SIZE), )
ifeq ($(ENABLE_LOGO)$(ENABLE_FIREWALL), yy)
  $(error "when ENABLE_LOGO = y and ENABLE_FIREWALL = y, CMDLINE_VALUE must involve svpumem. e.g: svpumem=2M ")
endif
else
	SVPU_BUF_SIZE = 0x0
endif

ifeq ($(OSD_BUF_SIZE), )
	OSD_BUF_SIZE = 0x0
endif

ifeq ($(ENABLE_TEE), y)
	TEE_DRAM_SIZE = $(CMDLINE_TEEMEM_SIZE)
else
	TEE_DRAM_SIZE = 0x0
endif

ifeq ($(ENABLE_ROOTFS_INITRD), y)
ifeq ($(INITRD_DRAM_SIZE), 0)
	INITRD_DRAM_SIZE = 0x800000
endif
ifeq ($(INITRD_DRAM_SIZE), )
	INITRD_DRAM_SIZE = 0x800000
endif
else
	INITRD_DRAM_SIZE = 0x0
endif

ALL_SECURE_VERIFY_TYPE = "SM2 RSA_PKCS RSA_SMI"
ALL_DATA_ENCRYPT_TYPE = "sm4-cbc sm4-ecb aes128-cbc aes128-ecb"
ifneq ($(findstring $(SECURE_VERIFY_TYPE),$(ALL_SECURE_VERIFY_TYPE)), )
	ENABLE_SECURE_VERIFY = y
	CONFIG_HASH = y
ifeq ($(CHIP_CORE), canopus)
# canopus
ifeq ($(SECURE_VERIFY_TYPE), SM2)
	CONFIG_PKA = y
else
	CONFIG_PKA = n
endif
endif # canopus
endif

ifeq ($(CONFIG_CHECK_STAGE2_HASH), y)
CONFIG_HASH = y
endif

ifneq ($(findstring $(DATA_ENCRYPT_TYPE),$(ALL_DATA_ENCRYPT_TYPE)), )
	ENABLE_DATA_ENCRYPT = y
	DATA_ENCRYPT_ALG = $(shell echo $(DATA_ENCRYPT_TYPE) | awk -F '-' '{print $$1}' | tr a-z A-Z)
	DATA_ENCRYPT_OPT = $(shell echo $(DATA_ENCRYPT_TYPE) | awk -F '-' '{print $$2}' | tr a-z A-Z)

ifeq ($(DATA_ENCRYPT_DERIVE), 0x01)
	STAGE1_ENCRYPT_DERIVE=1
	STAGE2_ENCRYPT_DERIVE=0
else
ifeq ($(DATA_ENCRYPT_DERIVE), 0x10)
	STAGE1_ENCRYPT_DERIVE=0
	STAGE2_ENCRYPT_DERIVE=1
else
ifeq ($(DATA_ENCRYPT_DERIVE), 0x11)
	STAGE1_ENCRYPT_DERIVE=1
	STAGE2_ENCRYPT_DERIVE=1
else
	STAGE1_ENCRYPT_DERIVE=0
	STAGE2_ENCRYPT_DERIVE=0
endif
endif
endif

ifeq ($(DATA_ENCRYPT), 0x01)
	STAGE1_ENCRYPT=1
	STAGE2_ENCRYPT=0
else
ifeq ($(DATA_ENCRYPT), 0x10)
	STAGE1_ENCRYPT=0
	STAGE2_ENCRYPT=1
else
ifeq ($(DATA_ENCRYPT), 0x11)
	STAGE1_ENCRYPT=1
	STAGE2_ENCRYPT=1
else
	STAGE1_ENCRYPT=0
	STAGE2_ENCRYPT=0
	ENABLE_DATA_ENCRYPT = n
endif
endif
endif

ifneq ($(STAGE1_ENCRYPT), 1)
	STAGE1_ENCRYPT_DERIVE=0
endif
ifneq ($(STAGE2_ENCRYPT), 1)
	STAGE2_ENCRYPT_DERIVE=0
endif

endif

ifeq ($(SECURE_VERIFY_TYPE), SM2)
	EXTRA_VERIFY_USED_SIZE = 0x200
	LOADER_EXTRA_VERIFY_USED_SIZE = $(EXTRA_VERIFY_USED_SIZE)
ifeq ($(ENABLE_TEE)$(ENABLE_VERIFY), yy)
	TEE_EXTRA_VERIFY_USED_SIZE = $(EXTRA_VERIFY_USED_SIZE)
else
	TEE_EXTRA_VERIFY_USED_SIZE = 0x0
endif
else
	EXTRA_VERIFY_USED_SIZE = 0x0
	LOADER_EXTRA_VERIFY_USED_SIZE = 0x0
	TEE_EXTRA_VERIFY_USED_SIZE = 0x0
endif

ifeq ($(ENABLE_RCC), y)
ifeq ($(findstring loadermem, $(CMDLINE_VALUE)), loadermem)
	ENABLE_LOADER_STAGE2_RCC = y
	BOOTLOADER_STAGE2_SIZE = $(CMDLINE_LOADERMEM_SIZE)
endif
endif

ifeq ($(ENABLE_LOADER_STAGE2_RCC), y)
HEAP_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(DRAM_SIZE) - \
	                                                 $(KERNEL_DTB_OFFSET) - \
	                                                 $(KERNEL_DTB_SIZE) - \
	                                                 $(KERNEL_SIZE) - \
	                                                 $(INITRD_DRAM_SIZE) - \
	                                                 $(STACK_SIZE) - \
	                                                 $(IRQ_STACK_SIZE) - \
	                                                 $(SPP_BUF_SIZE) - \
	                                                 $(SVPU_BUF_SIZE) - \
	                                                 $(OSD_BUF_SIZE) - \
	                                                 $(OTA_DRAM_SIZE) - \
	                                                 $(LOADER_EXTRA_VERIFY_USED_SIZE) - \
	                                                 $(CMDLINE_FWMEM_SIZE) - \
	                                                 $(TEE_EXTRA_VERIFY_USED_SIZE)))
else
HEAP_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(DRAM_SIZE) - \
	                                                 $(KERNEL_DTB_OFFSET) - \
	                                                 $(KERNEL_DTB_SIZE) - \
	                                                 $(KERNEL_SIZE) - \
	                                                 $(INITRD_DRAM_SIZE) - \
	                                                 $(STACK_SIZE) - \
	                                                 $(IRQ_STACK_SIZE) - \
	                                                 $(SPP_BUF_SIZE) - \
	                                                 $(SVPU_BUF_SIZE) - \
	                                                 $(OSD_BUF_SIZE) - \
	                                                 $(OTA_DRAM_SIZE) - \
	                                                 $(LOADER_EXTRA_VERIFY_USED_SIZE) - \
	                                                 $(BOOTLOADER_STAGE2_SIZE) - \
							 $(TEE_BOOTLOADER_SIZE) - \
	                                                 $(CMDLINE_FWMEM_SIZE) - \
	                                                 $(TEE_EXTRA_VERIFY_USED_SIZE)))
endif

#
# ARM DRAM : DTB + KERNEL + HEAP + STACK + SPP + SVPU + OTA + LOADER
# CSKY DRAM : KERNEL + DTB + HEAP + STACK + SPP + SVPU + OSD + OTA + LOADER
#
ifeq ($(CONFIG_ARCH), ARM)
KERNEL_DTB_START_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(DRAMBASE) + $(KERNEL_DTB_OFFSET)))
KERNEL_DTB_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(KERNEL_DTB_START_ADDR) + $(KERNEL_DTB_SIZE)))
KERNEL_START_ADDR := $(KERNEL_DTB_END_ADDR)
KERNEL_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(KERNEL_START_ADDR) + $(KERNEL_SIZE)))
INITRD_DRAM_START_ADDR := $(KERNEL_END_ADDR)
else
# CSKY
KERNEL_START_ADDR := $(DRAMBASE)
KERNEL_END_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(DRAMBASE) + $(KERNEL_SIZE)))
KERNEL_DTB_START_ADDR := $(KERNEL_END_ADDR)
KERNEL_DTB_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(KERNEL_DTB_START_ADDR) + $(KERNEL_DTB_SIZE)))
INITRD_DRAM_START_ADDR := $(KERNEL_DTB_END_ADDR)
endif
INITRD_DRAM_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(INITRD_DRAM_START_ADDR) + $(INITRD_DRAM_SIZE)))
OTA_DRAM_START_ADDR := $(INITRD_DRAM_END_ADDR)
OTA_DRAM_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(OTA_DRAM_START_ADDR) + $(OTA_DRAM_SIZE)))
HEAP_START_ADDR := $(OTA_DRAM_END_ADDR)
HEAP_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(HEAP_START_ADDR) + $(HEAP_SIZE)))
STACK_BOTTOM_ADDR := $(HEAP_END_ADDR)
STACK_TOP_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(STACK_BOTTOM_ADDR) + $(STACK_SIZE)))
IRQ_STACK_BOTTOM_ADDR := $(STACK_TOP_ADDR)
IRQ_STACK_TOP_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(IRQ_STACK_BOTTOM_ADDR) + $(IRQ_STACK_SIZE)))
ifeq ($(ENABLE_LOADER_STAGE2_RCC), y)
SPP_BUF_START_ADDR := $(IRQ_STACK_TOP_ADDR)
SPP_BUF_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(SPP_BUF_START_ADDR) + $(SPP_BUF_SIZE)))
SVPU_BUF_START_ADDR := $(SPP_BUF_END_ADDR)
SVPU_BUF_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(SVPU_BUF_START_ADDR) + $(SVPU_BUF_SIZE)))
OSD_BUF_START_ADDR := $(SVPU_BUF_END_ADDR)
OSD_BUF_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(OSD_BUF_START_ADDR) + $(OSD_BUF_SIZE)))
LOADER_EXTRA_VERIFY_START_ADDR := $(OSD_BUF_END_ADDR)
LOADER_EXTRA_VERIFY_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(LOADER_EXTRA_VERIFY_START_ADDR) + $(LOADER_EXTRA_VERIFY_USED_SIZE)))
LOADER_START_ADDR := $(LOADER_EXTRA_VERIFY_END_ADDR)
LOADER_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(LOADER_START_ADDR) + $(BOOTLOADER_STAGE2_SIZE)))
TEE_LOADER_START_ADDR := $(LOADER_END_ADDR)
TEE_LOADER_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(TEE_LOADER_START_ADDR) + $(TEE_BOOTLOADER_SIZE)))
else
LOADER_EXTRA_VERIFY_START_ADDR := $(IRQ_STACK_TOP_ADDR)
LOADER_EXTRA_VERIFY_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(LOADER_EXTRA_VERIFY_START_ADDR) + $(LOADER_EXTRA_VERIFY_USED_SIZE)))
LOADER_START_ADDR := $(LOADER_EXTRA_VERIFY_END_ADDR)
LOADER_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(LOADER_START_ADDR) + $(BOOTLOADER_STAGE2_SIZE)))
TEE_LOADER_START_ADDR := $(LOADER_END_ADDR)
TEE_LOADER_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(TEE_LOADER_START_ADDR) + $(TEE_BOOTLOADER_SIZE)))
SPP_BUF_START_ADDR := $(TEE_LOADER_END_ADDR)
SPP_BUF_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(SPP_BUF_START_ADDR) + $(SPP_BUF_SIZE)))
SVPU_BUF_START_ADDR := $(SPP_BUF_END_ADDR)
SVPU_BUF_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(SVPU_BUF_START_ADDR) + $(SVPU_BUF_SIZE)))
OSD_BUF_START_ADDR := $(SVPU_BUF_END_ADDR)
OSD_BUF_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(OSD_BUF_START_ADDR) + $(OSD_BUF_SIZE)))
endif

ifeq ($(CONFIG_ARCH), CSKY)
# align OTA_DRAM_START_ADDR
# Because csky's interrupt vector base register VBR must be 1024 byte aligned, please see redmine issue 262660
OTA_DRAM_START_ALIGN_ADDR := $(shell printf 0x%x $(shell ./script/align.sh $(OTA_DRAM_START_ADDR) 1024 1))
OTA_DRAM_SIZE := $(shell printf 0x%x $(shell ./script/calc.sh $(OTA_DRAM_SIZE) + $(OTA_DRAM_START_ADDR) - $(OTA_DRAM_START_ALIGN_ADDR)))
OTA_DRAM_START_ADDR := $(OTA_DRAM_START_ALIGN_ADDR)
endif

CHIP_CORE_REV	            = 3

ifeq ($(ENABLE_SECURE_VERIFY), y)
	SRAM_USED_SIZE = $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAM_SIZE) - $(SRAM_RESERVE_SIZE)))
else
	SRAM_USED_SIZE = $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAM_SIZE) - $(SRAM_LOWPOWER_USED_SIZE)))
endif

ifeq ($(GPIO_TABLE_SIZE), )
	GPIO_TABLE_SIZE = 0x400
endif

#
# SRAM : STAGE1 + GPIO TABLE + STAGE1 STACK
#
STAGE1_START_ADDR := $(SRAMBASE)
STAGE1_END_ADDR = $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAMBASE) + $(ROM_COPY_SIZE)))
GPIO_TABLE_START_ADDR = $(STAGE1_END_ADDR)
GPIO_TABLE_END_ADDR = $(shell printf 0x%08x $(shell ./script/calc.sh $(GPIO_TABLE_START_ADDR) + $(GPIO_TABLE_SIZE)))
STAGE1_STACK_BOTTOM_ADDR = $(GPIO_TABLE_END_ADDR)
STAGE1_STACK_TOP_ADDR = $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAMBASE) + $(SRAM_USED_SIZE)))
ifeq ($(CONFIG_ARCH), CSKY)
STAGE1_STACK_TOP_ADDR_NOMMU = $(shell printf 0x%08x $(shell ./script/calc.sh $(STAGE1_STACK_TOP_ADDR) - 0x$(REG_ADDR_HEAD)0000000))
endif

ifeq ($(FLASH_TABLE_SEARCH_START_ADDR), )
	FLASH_TABLE_SEARCH_START_ADDR = 0
endif

ifeq ($(FLASH_TABLE_SEARCH_SKIP_SIZE), )
	FLASH_TABLE_SEARCH_SKIP_SIZE = 0x800
endif

ifeq ($(FLASH_TABLE_SEARCH_SIZE), )
	FLASH_TABLE_SEARCH_SIZE = 0x400000
endif

ifeq ($(CONFIG_DEMOD), dvbs)
	DVB_CHANNEL_S2 = y
else
ifeq ($(CONFIG_DEMOD), dvbs2)
	DVB_CHANNEL_S2 = y
else
ifeq ($(CONFIG_DEMOD), dvbc)
	DVB_CHANNEL_C = y
else
ifeq ($(CONFIG_DEMOD), abs)
	ABS_CHANNEL = y
endif
endif
endif
endif

ifeq ($(CONFIG_24M_XTAL), y)
	DEFAULT_EXT_CLOCK_XTAL = 24000000
else
	DEFAULT_EXT_CLOCK_XTAL = 27000000
endif

ADAPTIVE_FLASH = $(shell echo $(ENABLE_SPINAND)$(ENABLE_NANDFLASH) | grep "yy")

ifeq ($(ENABLE_LIB), y)
ifeq ($(ENABLE_OTA_FORCE_DRAM_ADDR), y)
	OTA_START_ADDR_TO_LD = $(OTA_DRAM_START_ADDR)
	OTA_SIZE_TO_LD = $(OTA_DRAM_SIZE)
else
	OTA_START_ADDR_TO_LD = $(KERNEL_START_ADDR)
	OTA_SIZE_TO_LD = $(shell printf 0x%x $(shell ./script/calc.sh $(KERNEL_END_ADDR) - $(KERNEL_START_ADDR)))
endif
endif

ifeq ($(ENABLE_BOOT_FROM_USB), y)
    ifeq ($(BOOT_FROM_USB_IMAGE_NAME), )
        BOOT_FROM_USB_IMAGE_NAME = "uImage"
    endif
    ifeq ($(BOOT_FROM_USB_DTB_NAME), )
        BOOT_FROM_USB_DTB_NAME = $(CHIP_CORE)".dtb"
    endif
    ifeq ($(BOOT_FROM_USB_DTB_LOAD_ADDR), )
        BOOT_FROM_USB_DTB_LOAD_ADDR = 0x92000000
    endif
    ifeq ($(BOOT_FROM_USB_MAGIC), )
        BOOT_FROM_USB_MAGIC = 0x20150401
    endif
endif

ifeq ($(ROM_SERIAL_BAUDRATE), )
	ROM_SERIAL_BAUDRATE = 115200
endif

ifeq ($(CONFIG_PRINT_LEVEL), )
	CONFIG_PRINT_LEVEL = 1
endif

ifeq ($(ENABLE_ECOS_OTA), y)
	ENABLE_ROOTFS_CRAMFS = n
endif

ifeq ($(CONFIG_PANEL), )
	CONFIG_PANEL = y
endif

ifeq ($(ENABLE_BBT_SLT_TEST), y)
	CONFIG_I2C_BUS_FREQ = 400000UL

ifeq ($(CONFIG_ARCH), ARM)
	OTA_START_ADDR_TO_LD = $(shell printf 0x%x $(shell ./script/calc.sh $(DRAMBASE) + 0x8000))
	OTA_SIZE_TO_LD = $(shell printf 0x%x $(shell ./script/calc.sh $(KERNEL_END_ADDR) - $(KERNEL_START_ADDR)))
else
	OTA_START_ADDR_TO_LD = $(shell printf 0x%x $(shell ./script/calc.sh $(DRAMBASE) + 0x0000))
	OTA_SIZE_TO_LD = $(shell printf 0x%x $(shell ./script/calc.sh $(KERNEL_END_ADDR) - $(KERNEL_START_ADDR)))
endif
endif

# Object files
ifeq ($(CONFIG_ARCH), ARM)
SOBJS-y += cpu/arm/$(CHIP_CORE)/$(CHIP_CORE)_start.o \
	cpu/arm/cpu/armv7/v7_dcache.o \
	cpu/arm/cpu/armv7/v7_icache.o \
	cpu/arm/cpu/armv7/mmu_cache.o \
	cpu/arm/cpu/armv7/intc.o
SOBJS-$(ENABLE_LIB) += cpu/arm/setjmp.o
SOBJS-y += cpu/arm/cpu/armv7/return_from_teeos.o
SOBJS-y += cpu/arm/cpu/armv7/smccc-call.o
COBJS-y += cpu/arm/cpu/armv7/cache-cp15.o
COBJS-y += cpu/arm/$(CHIP_CORE)/$(CHIP_CORE)_pll_full.o \
	cpu/arm/cpu.o
ifneq ($(CHIP_CORE), vega)
COBJS-y += cpu/arm/$(CHIP_CORE)/$(CHIP_CORE)_pll_mini.o \
	cpu/arm/$(CHIP_CORE)/$(CHIP_CORE)_sdram.o
endif

COBJS-$(CONFIG_DDR_SOFT_TRAINING) += cpu/arm/$(CHIP_CORE)/ddr_soft_training.o
else
	# CSKY
ifneq ($(filter $(CHIP_CORE), taurus gemini),)
SOBJS-y += cpu/ck/gemini_taurus/gemini_taurus_start.o
SOBJS-$(ENABLE_LIB) += cpu/ck/setjmp.o
COBJS-y += cpu/ck/gemini_taurus/gemini_taurus_pll_mini.o \
	cpu/ck/gemini_taurus/gemini_taurus_pll_full.o \
	cpu/ck/gemini_taurus/gemini_taurus_sdram.o \
	cpu/ck/cpu.o
else
SOBJS-y += cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_start.o
SOBJS-$(ENABLE_LIB) += cpu/ck/setjmp.o
COBJS-y += cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_pll_mini.o \
	cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_pll_full.o \
	cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_sdram.o \
	cpu/ck/cpu.o
endif

endif

ifneq ($(CHIP_CORE), vega)
COBJS-y += cpu/copy.o
endif
COBJS-y += common/secure/common.o
COBJS-$(ENABLE_MEMORY) += memtest.o
COBJS-$(ENABLE_MEMORY_TEST) += test/memory/mem_test.o
COBJS-$(ENABLE_MEMORY_PHASE_TEST) += test/memory/memphase_test.o
ifeq ($(ENABLE_MEMORY), y)
ifeq ($(CONFIG_ARCH), CSKY)
EXTRA-OBJS += libc/language_c_libc_stdlib_rand.o
endif
endif

ifeq ($(CONFIG_EMBED_FLASH_TABLE), y)
COBJS-y += embed_flash_table.o
endif

ifeq ($(CONFIG_EMBED_KERNEL_IMG), y)
SOBJS-y += boot/embed_kernel_img.o
endif

COBJS-y += partition.o     \
	boot.o                 \
	bootconfig.o           \
	common/util.o          \
	common/crc32.o         \
	common/meminfo.o       \
	common/get_mem_info.o  \
	load_kernel/load_kernel.o

COBJS-$(ENABLE_KERNEL_DTB) += fdt/src/fdt.o \
	fdt/src/fdt_ro.o \
	fdt/src/fdt_rw.o \
	fdt/src/fdt_strerror.o \
	fdt/src/fdt_sw.o \
	fdt/src/fdt_wip.o \
	fdt/src/fdt_empty_tree.o \
	fdt/src/fdt_addresses.o \
	fdt/src/fdt_region.o   \
	fdt/fdt_support.o

COBJS-y += drivers/clock/common/reg_bits.o
COBJS-$(CONFIG_CLOCK_GX_V31) += drivers/clock/clock_v31.o \
	drivers/clock/v31/clk_reg.o

COBJS-$(ENABLE_FIREWALL) += drivers/firewall/firewall.o
ifeq ($(CONFIG_ARCH), ARM)
COBJS-$(ENABLE_FIREWALL) += drivers/firewall/$(CHIP_CORE)_firewall.o
else
COBJS-$(ENABLE_FIREWALL) += drivers/audio/audio_smp.o \
	drivers/video/video_smp.o \
	drivers/demux/demux_smp.o \
	drivers/scpu/gx_scpu.o
endif

COBJS-$(ENABLE_TEE) += drivers/permit/permit.o
COBJS-$(ENABLE_TEE) += drivers/permit/canopus_permit.o
COBJS-$(ENABLE_TEE) += drivers/permit/sirius_permit.o
COBJS-$(ENABLE_TEE) += drivers/permit/vega_permit.o

COBJS-$(ENABLE_IRQ) += drivers/intc/interrupt.o
ifeq ($(CONFIG_ARCH), ARM)
COBJS-$(ENABLE_IRQ) += drivers/intc/gic/gic.o
else
COBJS-$(ENABLE_IRQ) += drivers/intc/nationalchip/nc.o
endif

COBJS-$(CONFIG_PADMUX) += drivers/padmux/$(CHIP_CORE)_padmux.o

COBJS-y += libc/q_malloc.o    \
	libc/fakelibc.o           \
	libc/string.o             \
	libc/math.o               \
	libc/errno.o              \
	libc/div64.o

COBJS-y += common/kfifo.o

COBJS-y += libc/vsprintf.o
COBJS-y += libc/vsnprintf.o

COBJS-y += common/decompress/decompress.o
COBJS-$(ENABLE_COMPRESS_ZLIB) += common/decompress/zlib_deflate/deflate.o     \
	common/decompress/zlib_deflate/deftree.o                                         \
	common/decompress/zlib_deflate/bitrev.o                                          \
	common/decompress/zlib/compress.o
COBJS-$(ENABLE_DECOMPRESS_ZLIB) += common/decompress/zlib_inflate/inffast.o   \
	common/decompress/zlib_inflate/inflate.o                                       \
	common/decompress/zlib_inflate/inftrees.o                                      \
	common/decompress/zlib_inflate/infutil.o                                       \
	common/decompress/zlib/uncompr.o
COBJS-$(ENABLE_DECOMPRESS_LZO)  += common/decompress/decompress_unlzo.o
COBJS-$(ENABLE_DECOMPRESS_LZMA) += common/decompress/decompress_unlzma.o
COBJS-$(ENABLE_DECOMPRESS_GZIP) += common/decompress/decompress_inflate.o

ifeq ($(CONFIG_UART_TYPE), DW)
COBJS-y += drivers/serial/dw_uart.o
else
COBJS-y += drivers/serial/gx_uart.o
endif

COBJS-$(ENABLE_TEST) += test/gxloader_test.o

COBJS-$(ENABLE_OTA) += ota/gxota.o  \
	ota/libini.o

COBJS-$(CONFIG_SIMPLE_DLP_SUPPORT) += ota/simple_dlp.o ota/simple_ini.o

ifeq ($(ENABLE_IO_FRAMEWORK), y)
	COBJS-y += ate/gxdev.o
	COBJS-y += ate/fake_ramfs.o
	COBJS-y += ate/file_io.o
	COBJS-y += ate/io.o
	COBJS-y += libc/vfprintf.o
	COBJS-y += common/stdio.o
endif

ifeq ($(ENABLE_LIB), y)
	COBJS-y += ate/lib/common.o
else
	COBJS-y += main.o
endif

ifeq ($(CONFIG_REE_LOADER), y)
	COBJS-y += drivers/firmware/gxteeloader.o
endif

ifeq ($(ENABLE_IRR), y)
ifeq ($(ENABLE_IRR_V2), y)
COBJS-y += drivers/irr/gx_irr_v2.o
else
COBJS-y += drivers/irr/gx_irr.o
endif
endif

COBJS-$(ENABLE_SCPU) += drivers/scpu/gx_scpu.o

COBJS-$(ENABLE_RCC) += 	drivers/rcc/gx_rcc.o

COBJS-$(ENABLE_CTR)  += drivers/ctr/ctr.o
COBJS-$(ENABLE_GPIO)  += drivers/gpio/gx_gpio.o
COBJS-$(ENABLE_EEPROM)  += drivers/eeprom/eeprom.o
COBJS-$(ENABLE_CHIP_INFO) += drivers/chip_info/chip_info.o
COBJS-$(ENABLE_WDT) += drivers/watchdog/gx_wdt.o
ifeq ($(ENABLE_TIME), y)
COBJS-y  += drivers/time/time_interface.o
COBJS-$(CONFIG_TIME_GX_V1)  += drivers/time/time.o
COBJS-$(CONFIG_TIME_GX_V2)  += drivers/time/time_v2.o
endif
COBJS-$(ENABLE_JTAG_PASSWD)  += drivers/jtag/jtagpasswd.o
COBJS-$(ENABLE_LOWPOWER)  += drivers/lowpower/gxlowpower.o
COBJS-$(ENABLE_SMARTCARD)  += drivers/sci/sci.o

ifeq ($(ENABLE_I2C), y)
	COBJS-y += drivers/i2c/i2c-core.o
ifeq ($(CONFIG_I2C_TYPE), DW)
	COBJS-y += drivers/i2c/dw_i2c.o
else
	CONFIG_I2C_TYPE = GX
	COBJS-y += drivers/i2c/gx_i2c.o
endif
endif

ifeq ($(ENABLE_TDCXO), y)
	COBJS-y += drivers/tdcxo/tdcxo.o
endif

COBJS-$(ENABLE_UBI) += drivers/mtd/mtdcore.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/attach.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/build.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/vtbl.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/vmt.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/upd.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/kapi.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/eba.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/wl.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/crc32.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/io.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/misc.o
COBJS-$(ENABLE_UBI) += drivers/mtd/ubi/debug.o
COBJS-$(ENABLE_UBI) += common/ubi_user.o
COBJS-$(ENABLE_UBI) += libc/rbtree.o
COBJS-$(ENABLE_UBI) += libc/linux_compat.o

PANEL_FILES=$(patsubst %c,%o,$(shell ls board/$(CHIP_CORE)/board-$(CHIP_BOARD)/panel*.c 2>/dev/null))
COBJS-y += board/$(CHIP_CORE)/$(CHIP_CORE)_api.o              \
	board/$(CHIP_CORE)/board-common.o                \
	$(PANEL_FILES)	\
	drivers/flash/flash.o
COBJS-y += $(patsubst %.c, %.o, $(wildcard board/$(CHIP_CORE)/ddr_sec_param.c))
COBJS-y += $(patsubst %.c, %.o, $(wildcard board/$(CHIP_CORE)/ddr_thr_param.c))

ifeq ($(CONFIG_JZ_TAURUS), y)
COBJS-y += board/taurus/board-$(CHIP_BOARD)/board-init.o
else
COBJS-y += board/$(CHIP_CORE)/board-$(CHIP_BOARD)/board-init.o
endif

COBJS-$(ENABLE_CMD) += bootmenu.o commands.o common/cli_readline.o

ifeq ($(ENABLE_RELEASE_CHECK), )
	ENABLE_RELEASE_CHECK = n
endif
ifeq ($(ENABLE_RELEASE_CHECK), y)
	ENABLE_GDI = y
endif

COBJS-$(ENABLE_LOGO) += drivers/jpeg/gx3201_jpeg.o
COBJS-$(ENABLE_GDI) += drivers/gdi/gx3201_gdi.o
ifeq ($(ENABLE_GDI), y)
COBJS-y += drivers/vout/gx3211_vout.o \
	drivers/audio/aout.o \
	drivers/vout/dw_hdmi/dw_hdmi.o \
	drivers/vout/cygnus_vpu_vout.o
GDI_FILES=$(patsubst %c,%o,$(shell ls common/gdi*.c 2>/dev/null))
COBJS-y += $(GDI_FILES)
else
ifeq ($(ENABLE_LOGO), y)
COBJS-y += drivers/vout/gx3211_vout.o \
	drivers/audio/aout.o \
	drivers/vout/dw_hdmi/dw_hdmi.o \
	drivers/vout/cygnus_vpu_vout.o
endif
endif

COBJS-y += drivers/audio/spdif.o

COBJS-$(ENABLE_NANDFLASH) += drivers/flash/nand/nand_flash.o


ifeq ($(ENABLE_SPIFLASH), y)
COBJS-y += drivers/platform/device.o
COBJS-y += drivers/spi/spi.o
ifeq ($(CONFIG_SPI_TYPE), DW)
COBJS-$(CONFIG_DWSPI_V1) += drivers/spi/dw_spi/dw_spim.o
COBJS-$(CONFIG_DWSPI_V2) += drivers/spi/dw_spi/dw_spi_v2.o
else
COBJS-y += drivers/spi/gx_spi/gx_spi.o
endif
else
ifeq ($(ENABLE_SPINAND), y)
COBJS-y += drivers/platform/device.o
COBJS-y += drivers/spi/spi.o
ifeq ($(CONFIG_SPI_TYPE), DW)
COBJS-$(CONFIG_DWSPI_V1) += drivers/spi/dw_spi/dw_spim.o
COBJS-$(CONFIG_DWSPI_V2) += drivers/spi/dw_spi/dw_spi_v2.o
else
COBJS-y += drivers/spi/gx_spi/gx_spi.o
endif
endif
endif

COBJS-$(ENABLE_SPIFLASH) += drivers/flash/spinor/sflash.o

COBJS-$(ENABLE_SPINAND) += drivers/flash/spinand/spinand_flash.o

COBJS-$(ENABLE_FLASH_TEST) += drivers/flash/flash_test.o

COBJS-$(ENABLE_ROOTFS_ROMFS)  += load_kernel/romfs.o

COBJS-$(ENABLE_UIMAGE)  += load_kernel/uImage.o

COBJS-$(ENABLE_ELF)  += load_kernel/elf.o

COBJS-$(ENABLE_ROOTFS_CRAMFS)  += \
   load_kernel/cramfs.o       \
   load_kernel/uncompress.o   \

COBJS-$(ENABLE_ROOTFS_YAFFS2) += fs/yaffs2/yaffscfg.o           \
   fs/yaffs2/yaffs_checkptrw.o    \
   fs/yaffs2/yaffs_ecc.o          \
   fs/yaffs2/yaffs_guts.o         \
   fs/yaffs2/yaffs_mtdif.o        \
   fs/yaffs2/yaffs_mtdif2.o       \
   fs/yaffs2/yaffs_nand.o         \
   fs/yaffs2/yaffs_packedtags1.o  \
   fs/yaffs2/yaffs_packedtags2.o  \
   fs/yaffs2/yaffs_qsort.o        \
   fs/yaffs2/yaffs_tagscompat.o   \
   fs/yaffs2/yaffs_tagsvalidity.o \

COBJS-$(ENABLE_MINIFS) += fs/minifs/compr.o      \
	fs/minifs/compr_zlib.o                       \
	fs/minifs/device.o                           \
	fs/minifs/file.o                             \
	fs/minifs/minifs_spiflash_nos.o              \
	fs/minifs/node.o                             \
	fs/minifs/object.o
COBJS-$(ENABLE_MINIFS_TEST) += fs/minifs/test_minifs.o
COBJS-$(ENABLE_USB) += \
   drivers/usb/host/common.o \
   drivers/usb/host/ehci-hcd.o \
   drivers/usb/host/ehci-gx.o \
   common/usb.o \
   common/usb_hub.o \
   common/usb_storage.o \
   common/usb_update.o \
   fs/fat/file.o \
   fs/fat/part_dos.o \
   fs/fat/part_efi.o
ifeq ($(ENABLE_FAT_WRITE), y)
COBJS-$(ENABLE_USB) += fs/fat/fat_write.o
else
COBJS-$(ENABLE_USB) += fs/fat/fat.o
endif

ifeq ($(USB_EYE_DIAGRAM_TEST), y)
ifeq ($(ENABLE_USB), n)
	COBJS-y += drivers/usb/host/common.o
endif
endif


COBJS-$(ENABLE_NET) += \
   common/net/net.o            \
   common/net/net_ipv4.o       \
   common/net/net_ipv4_tftp.o  \
   common/net/net_ipv4_bootp.o \
   common/net/net_ipv4_dns.o   \
   common/net/synopsis.o       \
   common/net/synopGMAC_network_interface.o \
   common/net/synopGMAC_Dev.o

COBJS-$(ENABLE_USB_NETWORK_ASIX88179) += \
   common/net/asix88179.o \
   common/net/usb_ether.o

COBJS-$(CONFIG_SIRIUS_OTP) += drivers/gx_otp/sirius_otp.o
COBJS-$(CONFIG_GX3211_OTP) += drivers/gx_otp/gx3211_otp.o \
			     common/sec_rw.o

COBJS-$(ENABLE_MTC) += drivers/mtc/gx_mtc_core.o\
	drivers/mtc/gx_mtc.o
COBJS-$(ENABLE_MTC_TEST) += test/mtc/mtc_test.o\
	test/mtc/decrypt_test_bin.o

ifeq ($(ENABLE_SECURE_VERIFY), y)
ifeq ($(CONFIG_PKA), y)
COBJS-y += common/secure/pka/dw_pka.o
endif
ifeq ($(SECURE_VERIFY_TYPE), SM2)
COBJS-y += common/secure/sm2/sm2_verify.o
ifeq ($(CONFIG_PKA), n)
EXTRA-OBJS += common/secure/sm2/lib/acpu_driver.o	\
	common/secure/sm2/lib/ecc_shared.o				\
	common/secure/sm2/lib/ecsm.o					\
	common/secure/sm2/lib/sm2.o						\
	common/secure/sm2/lib/tools.o
endif
else
ifeq ($(CONFIG_STAGE1_VERIFY_USE_RSA), y)
COBJS-y += common/secure/rsa/rsa.o					\
	common/secure/rsa/rsa_verify.o
endif
endif
endif

ifeq ($(ENABLE_DATA_ENCRYPT), y)
COBJS-y += common/secure/decrypt/decrypt.o
COBJS-$(CONFIG_STAGE1_USE_ACPU_CRYPTO) += common/secure/decrypt/gx_acpu_decrypt.o
COBJS-$(CONFIG_STAGE1_USE_MTC) += common/secure/decrypt/gx_acpu_mtc.o
endif

ifeq ($(ENABLE_APP_DECRYPT), y)
COBJS-y += common/secure/decrypt/gx_acpu_mtc.o
endif

ifeq ($(CONFIG_HASH), y)
COBJS-$(ENABLE_HARDWARE_HASH) += drivers/hash/gx_sha256.o
COBJS-$(ENABLE_SOFT_HASH) += common/secure/sha/gx_soft_sha256.o
COBJS-y += common/hash.o
endif

ifeq ($(ENABLE_VERIFY), y)
	COBJS-y += common/secure/rsa/rsa.o \
	common/secure/rsa/rsa_verify.o
endif

COBJS-y += drivers/ddr/ddr_info.o

ifeq ($(CONFIG_AUTO_CMD), )
	CONFIG_AUTO_CMD = boot
endif

$(sort $(SOBJS-y)):%.o:%.S
	@echo [$(CC) compiling $@]
	@$(CC) $(CFLAGS) -DBOOT_FROM_FLASH -c $(CPPFLAGS) $< -o $@
$(sort $(COBJS-y)):%.o:%.c
	@echo [$(CC) compiling $@]
	@$(CC) $(CFLAGS)  -c $(CPPFLAGS) $< -o $@
$(sort $(EXTRA-COBJS-y)):%.o:%.c
	@echo [$(CC) compiling $@]
	@$(CC) $(CFLAGS)  -c $(CPPFLAGS) $< -o $@

ifeq ($(CONFIG_EMBED_FLASH_TABLE), y)
embed_flash_table.c:$(CONFIG_EMBED_FLASH_TABLE_BIN_FILE)
	@echo [bin_to_c $@]
	@script/bin_to_c.py --bin $(CONFIG_EMBED_FLASH_TABLE_BIN_FILE) \
		            --vname embed_flash_table \
		            --out $@
endif

ifeq ($(CONFIG_STAGE2_COMPRESSED), y)
# export variable to cpu/<ARCH>/compressed/Makefile
export CC CPPFLAGS CFLAGS LD LDFLAGS LIBS LIBGCC OBJCOPY OBJCOPY_FLAG
export LOADER_COMPRESSED_START_ADDR = $(shell printf 0x%x $(shell ./script/calc.sh $(KERNEL_START_ADDR) + 0x4000))
export LOADER_COMPRESSED_SIZE = $(KERNEL_SIZE)
export STAGE2_LMA_ADDR

define compressed_stage2
	@make -f cpu/compressed/Makefile
	@dd if=$@ of=TEMP_FILE1 bs=1 count=$(shell printf "%d" $(STAGE2_LMA_ADDR)) > /dev/null 2>&1
	@mv TEMP_FILE1 $@
	@cat loader_compressed_stage2.bin >> $@
	@rm loader_compressed_stage2.bin
endef
endif

# check config
checkcfg: Makefile
ifeq ($(ENABLE_SCPU), y)
ifeq ($(CMDLINE_SCPUMEM_SIZE), 0x0)
	$(error error is not config scpumem in CMDLINE_VALUE )
endif
ifeq ($(SCPU_BIN_FLASH_ADDR), )
	$(error error is not config SCPU_BIN_FLASH_ADDR )
endif
endif
ifeq ($(DDR_SIZE), )
	$(error option 'DDR_SIZE' must be set )
endif

LOADER_BANNER := $(shell ./script/mkversion.sh)

# automatically generated file
include/version_autogenerated.h:
	@echo "/*" > $@
	@echo " * Boot Loader Version" >> $@
	@echo " *" >> $@
	@echo " * This file is automatically generated from Makefile" >> $@
	@echo " */" >> $@
	@echo "" >> $@
	@echo "#ifndef __BOOTLOADER_VERSION_AUTOGENERATED_H" >> $@
	@echo "#define __BOOTLOADER_VERSION_AUTOGENERATED_H" >> $@
	@echo "" >> $@
	@echo "#define LOADER_BANNER    \"$(LOADER_BANNER)\"" >> $@

##############################################################
#
## I   SOC architechture
#
##############################################################

	@echo "#define CONFIG_ARCH                    \"$(CONFIG_ARCH)\"" >> $@
ifeq ($(CONFIG_ARCH), ARM)
	@echo "#define CONFIG_ARCH_ARM" >> $@
	@echo "#define CONFIG_ARCH_ARMV7" >> $@
	@echo "#define CONFIG_ARCH_ARM_$(CHIP_CORE_IN_CAPTIAL)" >> $@
	@echo "#define CONFIG_ARCH_ARM_$(CHIP_CORE_IN_CAPTIAL)_REV     \"$(CHIP_CORE_REV)\"" >> $@
else
# CSKY
	@echo "#define CONFIG_ARCH_CKMMU" >> $@
ifneq ($(CONFIG_JZ_TAURUS), y)
	@echo "#define CONFIG_ARCH_CKMMU_$(CHIP_CORE_IN_CAPTIAL)" >> $@
	@echo "#define CONFIG_ARCH_CKMMU_$(CHIP_CORE_IN_CAPTIAL)_REV     \"$(CHIP_CORE_REV)\"" >> $@
endif
endif
ifeq ($(CONFIG_JZ_TAURUS), y)
	@echo "#define CHIP_CORE                   \"taurus\"" >> $@
	@echo "#define CONFIG_REV                   \"$(CHIP_CORE_REV)\"" >> $@
else
	@echo "#define CHIP_CORE                      \"$(CHIP_CORE)\"" >> $@
	@echo "#define CONFIG_REV                     \"$(CHIP_CORE_REV)\"" >> $@
endif

	@echo "#define CHIP_BOARD                     \"$(CHIP_BOARD)\"" >> $@
	@echo "#define CHIP_BOARD_$(shell echo $(CHIP_BOARD) | tr "-" "_" )"                         >> $@
	@echo "" >> $@

	@echo "#define CONFIG_$(CONFIG_CHIP_PACKAGE)" >> $@

ifeq ($(CONFIG_FPGA_BOARD), y)
	@echo "#define CONFIG_FPGA_BOARD" >> $@
endif

ifeq ($(CONFIG_ACCELERATOR), y)
	@echo "#define CONFIG_ACCELERATOR" >> $@
endif

ifeq ($(CONFIG_REE_LOADER), y)
	@echo "#define CONFIG_REE_LOADER" >> $@
endif


##################################################################
#
##II  OS CHOICE and Option
#
##################################################################
ifeq ($(ENABLE_DENALI_ELF), y)

	@echo "#define ENABLE_DENALI_ELF" >> $@

endif

ifneq ($(CMDLINE_VALUE), "")
	@echo "#define CONFIG_ENABLE_CMDLINE" >> $@
	@echo '#define CONFIG_CMDLINE_VALUE     "$(CMDLINE_VALUE)"' >> $@
ifneq ($(CONFIG_MULTI_CMDLINE), )
	@echo "#define CONFIG_MULTI_CMDLINE" >> $@
	@echo '#define CONFIG_CMDLINE_VALUE1    "$(CMDLINE_VALUE1)"' >> $@
	@echo '#define CONFIG_CMDLINE_VALUE2    "$(CMDLINE_VALUE2)"' >> $@
	@echo '#define CONFIG_CMDLINE_VALUE3    "$(CMDLINE_VALUE3)"' >> $@
	@echo '#define CONFIG_CMDLINE_VALUE4    "$(CMDLINE_VALUE4)"' >> $@
	@echo '#define CONFIG_CMDLINE_VALUE5    "$(CMDLINE_VALUE5)"' >> $@
	@echo '#define CONFIG_CMDLINE_VALUE6    "$(CMDLINE_VALUE6)"' >> $@
endif
endif

ifneq ($(EXTEND_MTDPARTS), )
	@echo '#define CONFIG_EXTEND_MTDPARTS           $(EXTEND_MTDPARTS)' >> $@
endif

ifeq ($(DDR_SIP), y)
	@echo "#define DDR_SIP" >> $@
endif

ifeq ($(DDR_TYPE), DDR3)
	@echo "#define CONFIG_DDR3" >> $@
else
ifeq ($(DDR_TYPE), DDR2)
	@echo "#define CONFIG_DDR2" >> $@
else
ifeq ($(DDR_TYPE), DDR1)
	@echo "#define CONFIG_DDR1" >> $@
endif
endif
endif

ifdef DDR_POS
	@echo "#define CONFIG_$(DDR_TYPE)_$(DDR_POS)" >> $@
endif

ifeq ($(DDR_PLL_USE_INNO), y)
	@echo "#define DDR_PLL_USE_INNO" >> $@
endif

ifdef DDR_FREQUENCY_400M
	@echo "#define DDR_FREQUENCY_400M            $(DDR_FREQUENCY_400M)" >> $@
endif
ifdef DDR_FREQUENCY_533M
	@echo "#define DDR_FREQUENCY_533M            $(DDR_FREQUENCY_533M)" >> $@
endif
ifdef DDR_FREQUENCY_667M
	@echo "#define DDR_FREQUENCY_667M            $(DDR_FREQUENCY_667M)" >> $@
endif
ifdef DDR_FREQUENCY_720M
	@echo "#define DDR_FREQUENCY_720M            $(DDR_FREQUENCY_720M)" >> $@
endif
ifdef DDR_FREQUENCY_783M
	@echo "#define DDR_FREQUENCY_783M            $(DDR_FREQUENCY_783M)" >> $@
endif
ifdef DDR_FREQUENCY_800M
	@echo "#define DDR_FREQUENCY_800M            $(DDR_FREQUENCY_800M)" >> $@
endif
ifdef DDR_FREQUENCY_933M
	@echo "#define DDR_FREQUENCY_933M            $(DDR_FREQUENCY_933M)" >> $@
endif
ifdef DDR_PARAM_FREQUENCY
	@echo "#define DDR_PARAM_FREQUENCY           $(DDR_PARAM_FREQUENCY)" >> $@
endif
ifdef DDR_FREQUENCY
	@echo "#define DDR_FREQUENCY_CONFIG             $(DDR_FREQUENCY)" >> $@
endif
ifdef CPU_FREQUENCY
	@echo "#define CPU_FREQUENCY_CONFIG             $(CPU_FREQUENCY)" >> $@
endif
	@echo "#define DDR_PAR_INCLUDE                \"$(DDR_PAR_INCLUDE)\"" >> $@
ifneq ($(DDR_PAR_PATCH_INCLUDE), )
	@echo "#define DDR_PAR_PATCH_INCLUDE              \"$(DDR_PAR_PATCH_INCLUDE)\"" >> $@
	@echo "#define DDR_PAR_PATCH_ARRAY              $(DDR_PAR_PATCH_ARRAY)" >> $@
endif
ifneq ($(DDR_SEC_PAR_INCLUDE), )
	@echo "#define DDR_SEC_PAR_INCLUDE          \"$(DDR_SEC_PAR_INCLUDE)\"" >> $@
endif
ifneq ($(DDR_THR_PAR_INCLUDE), )
	@echo "#define DDR_THR_PAR_INCLUDE          \"$(DDR_THR_PAR_INCLUDE)\"" >> $@
endif
ifneq ($(DDR_SEC_PAR_PATCH_INCLUDE), )
	@echo "#define DDR_SEC_PAR_PATCH_INCLUDE              \"$(DDR_SEC_PAR_PATCH_INCLUDE)\"" >> $@
	@echo "#define DDR_SEC_PAR_PATCH_ARRAY              $(DDR_SEC_PAR_PATCH_ARRAY)" >> $@
endif
ifneq ($(DDR_SIZE), )
	@echo "#define DDR_SIZE                         $(DDR_SIZE)" >> $@
endif

ifeq ($(CONFIG_DDR_SOFT_TRAINING), y)
	@echo "#define CONFIG_DDR_SOFT_TRAINING" >> $@
endif

ifeq ($(USB_RESISTANCE), 2.2)
	@echo "#define USB_RESISTANCE_2_2" >> $@
else
ifeq ($(USB_RESISTANCE), 10)
	@echo "#define USB_RESISTANCE_10" >> $@
else
ifeq ($(USB_RESISTANCE), 0)
	@echo "#define USB_RESISTANCE_0" >> $@
endif
endif
endif

ifeq ($(DVB_CHANNEL_S2), y)
	@echo "#define DVB_CHANNEL_S2" >> $@
else
ifeq ($(DVB_CHANNEL_C), y)
	@echo "#define DVB_CHANNEL_C" >> $@
else
ifeq ($(ABS_CHANNEL), y)
	@echo "#define ABS_CHANNEL" >> $@
endif
endif
endif

ifeq ($(CONFIG_24M_XTAL), y)
	@echo "#define CONFIG_24M_XTAL" >> $@
endif
	@echo "#define DEFAULT_EXT_CLOCK_XTAL           $(DEFAULT_EXT_CLOCK_XTAL)" >> $@

ifeq ($(ENABLE_KERNEL_DTB), y)
	@echo "#define CONFIG_ENABLE_KERNEL_DTB" >> $@
endif

ifeq ($(ENABLE_TEE), y)
	@echo "#define CONFIG_ENABLE_TEE" >> $@
endif

ifeq ($(ENABLE_FIREWALL), y)
	@echo "#define CONFIG_ENABLE_FIREWALL" >> $@
endif

ifeq ($(USB_EYE_DIAGRAM_TEST), y)
	@echo "#define CONFIG_USB_EYE_DIAGRAM_TEST" >> $@
endif

	@echo "" >> $@

##################################################################
#
##III  Function module DMA and ATE will enable interrupts and MMU
#
##################################################################

ifeq ($(ENABLE_TEST), y)
	@echo "#define CONFIG_ENABLE_TEST" >> $@
endif

ifeq ($(ENABLE_FLASH_TEST), y)
	@echo "#define CONFIG_ENABLE_FLASH_TEST" >> $@
endif

ifeq ($(ENABLE_OTA), y)
	@echo "#define CONFIG_ENABLE_OTA" >> $@
endif

ifeq ($(ENABLE_ECOS_OTA), y)
	@echo "#define CONFIG_ECOS_OTA" >> $@
endif

ifeq ($(CONFIG_SIMPLE_DLP_SUPPORT), y)
	@echo "#define CONFIG_SIMPLE_DLP_SUPPORT" >> $@
	@echo '#define CONFIG_DLP_PATH           $(CONFIG_DLP_PATH)'       >> $@
	@echo '#define CONFIG_BACKUP_DLP_PATH    $(CONFIG_BACKUP_DLP_PATH)'       >> $@
endif

ifeq ($(ENABLE_DMA), y)
	@echo "#define CONFIG_ENABLE_DMA" >> $@
endif

ifeq ($(ENABLE_MEMORY), y)
	@echo "#define CONFIG_ENABLE_MEMORY" >> $@
endif

ifeq ($(ENABLE_MEMORY_TEST), y)
	@echo "#define CONFIG_ENABLE_MEMORY_TEST" >> $@
endif
ifneq ($(CONFIG_MEMORY_TEST_SIZE), )
	@echo "#define CONFIG_MEMORY_TEST_SIZE $(CONFIG_MEMORY_TEST_SIZE)" >> $@
endif

ifeq ($(ENABLE_MEMORY_PHASE_TEST), y)
	@echo "#define CONFIG_ENABLE_MEMORY_PHASE_TEST" >> $@
endif

ifeq ($(ENABLE_LIB), y)
	@echo "#define CONFIG_ENABLE_LIB" >> $@
endif

ifeq ($(ENABLE_CHIPTEST), y)
	@echo "#define CONFIG_ENABLE_CHIPTEST" >> $@
endif

	@echo "" >> $@

#################################################################
#
##IV Debug option
#
#################################################################
ifeq ($(ENABLE_RELEASE_CHECK), y)
	@echo "#define CONFIG_ENABLE_RELEASE_CHECK" >> $@
endif

ifeq ($(ENABLE_GDB_DEBUG), y)
	@echo "#define CONFIG_ENABLE_GDB_DEBUG" >> $@
endif

ifeq ($(ENABLE_IO_FRAMEWORK), y)
	@echo "#define CONFIG_ENABLE_IO_FRAMEWORK" >> $@
endif



	@echo "#define CONFIG_BOOTDELAY                 $(CONFIG_BOOTDELAY)" >> $@
	@echo "#define CONFIG_AUTO_CMD                \"$(CONFIG_AUTO_CMD)\"" >> $@
	@echo "#define CONFIG_UART_PORT                 $(CONFIG_UART_PORT)" >> $@
	@echo "#define CONFIG_UART_BAUDRATE             $(CONFIG_UART_BAUDRATE)" >> $@
ifeq ($(CONFIG_UART_TYPE), DW)
	@echo "#define CONFIG_UART_TYPE_DW" >> $@
else
	@echo "#define CONFIG_UART_TYPE_GX" >> $@
endif

ifeq ($(ENABLE_DBG_PIN_NO_MULTT), y)
	@echo "#define CONFIG_DBG_PIN_NO_MULTI" >> $@
endif
ifeq ($(CONFIG_MULPIN_VERIFY), y)
	@echo "#define CONFIG_MULPIN_VERIFY" >> $@
endif

	@echo "" >> $@
	@echo "" >> $@

#############################################################
#
##VI FS/Flash support
#
#############################################################

ifeq ($(ENABLE_ROOTFS_ROMFS), y)
	@echo "#define CONFIG_ENABLE_ROOTFS_ROMFS" >> $@
	@echo "#define ROMFS_LOAD" >> $@
endif

ifeq ($(ENABLE_UIMAGE), y)
	@echo "#define UIMAGE_LOAD" >> $@
endif

ifeq ($(ENABLE_ELF), y)
	@echo "#define ELF_LOAD" >> $@
endif

ifeq ($(ENABLE_ROOTFS_CRAMFS), y)
	@echo "#define CONFIG_ENABLE_ROOTFS_CRAMFS" >> $@
	@echo "#define CRAMFS_LOAD" >> $@
endif

ifeq ($(ENABLE_ROOTFS_YAFFS2), y)
	@echo "#define CONFIG_ENABLE_ROOTFS_YAFFS2" >> $@
endif

ifeq ($(ENABLE_MINIFS), y)
ifneq ($(ENABLE_SPIFLASH), y)
	@echo "set 'ENABLE_MINIFS = y' must set 'ENABLE_SPIFLASH = y' first!"
	@exit 1
endif
ifneq ($(ENABLE_COMPRESS_ZLIB), y)
	@echo "set 'ENABLE_MINIFS = y' must set 'ENABLE_COMPRESS_ZLIB = y' first!"
	@exit 1
endif
ifneq ($(ENABLE_DECOMPRESS_ZLIB), y)
	@echo "set 'ENABLE_MINIFS = y' must set 'ENABLE_DECOMPRESS_ZLIB = y' first!"
	@exit 1
endif
	@echo "#define CONFIG_ENABLE_MINIFS" >> $@
endif

ifeq ($(ENABLE_ROOTFS_UBIFS), y)
	@echo "#define CONFIG_ENABLE_ROOTFS_UBIFS" >> $@
endif

ifeq ($(ENABLE_ROOTFS_INITRD), y)
	@echo "#define CONFIG_ENABLE_ROOTFS_INITRD" >> $@
endif

ifeq ($(ENABLE_UBI), y)
	@echo "#define CONFIG_ENABLE_UBI" >> $@
endif

ifeq ($(ENABLE_UBI_TEST), y)
	@echo "#define CONFIG_ENABLE_UBI_TEST" >> $@
endif

ifeq ($(ENABLE_LOAD_IMAGE_LEN), y)
	@echo "#define CONFIG_LOAD_IMAGE_LEN" >> $@
endif

ifeq ($(ENABLE_NANDFLASH), y)
	@echo "#define CONFIG_ENABLE_NANDFLASH" >> $@
endif

ifeq ($(ENABLE_SPIFLASH), y)
	@echo "#define CONFIG_ENABLE_SFLASH" >>$@
endif

ifeq ($(ENABLE_SPINAND), y)
	@echo "#define CONFIG_ENABLE_SPINAND" >> $@
endif

ifneq ($(ADAPTIVE_FLASH), )
	@echo "#define CONFIG_ENABLE_ADAPTIVE_FLASH" >> $@
endif

ifeq ($(ENABLE_FLASH_FULLFUNCTION), y)
	@echo "#define CONFIG_ENABLE_FLASH_FULLFUNCTION" >> $@
endif

ifeq ($(CONFIG_SPI_TYPE), DW)
	@echo "#define CONFIG_ENABLE_DWSPI" >> $@
else
	@echo "#define CONFIG_ENABLE_GXSPI" >> $@
endif

ifeq ($(ENABLE_SPI_QUAD), y)
	@echo "#define CONFIG_ENABLE_SPI_QUAD" >> $@
endif

ifeq ($(ENABLE_SPIFLASH), y)
	@echo "#define CONFIG_ENABLE_SPI1" >> $@
else
ifeq ($(ENABLE_SPINAND), y)
	@echo "#define CONFIG_ENABLE_SPI1" >> $@
endif
endif

	@echo "" >> $@

#############################################################
#
##VII Component configurations, usually you needn't modify them
#
#############################################################
ifeq ($(ENABLE_VEDIO), y)
	@echo "#define CONFIG_ENABLE_VEDIO" >> $@
endif

ifeq ($(ENABLE_IRQ), y)
	@echo "#define CONFIG_ENABLE_IRQ" >> $@
ifeq ($(CONFIG_ARCH), ARM)
	@echo "#define CONFIG_ENABLE_INTC_GIC" >> $@
else
	@echo "#define CONFIG_ENABLE_INTC_NC" >> $@
endif
endif

ifeq ($(ENABLE_UART_IRQ), y)
	@echo "#define CONFIG_ENABLE_UART_IRQ" >> $@
endif

ifeq ($(ENABLE_RCC), y)
	@echo "#define CONFIG_ENABLE_RCC" >> $@
ifeq ($(CONFIG_RCC_GX_V1), y)
	@echo "#define CONFIG_RCC_GX_V1"  >> $@
endif
ifeq ($(CONFIG_RCC_GX_V2), y)
	@echo "#define CONFIG_RCC_GX_V2"  >> $@
endif
ifeq ($(ENABLE_LOADER_STAGE2_RCC), y)
	@echo "#define CONFIG_LOADER_STAGE2_RCC" >> $@
endif
endif

ifeq ($(ENABLE_CTR), y)
	@echo "#define CONFIG_ENABLE_CTR" >> $@
endif

ifeq ($(ENABLE_TIME), y)
	@echo "#define CONFIG_ENABLE_TIME" >> $@
endif

ifeq ($(ENABLE_JTAG_PASSWD), y)
	@echo "#define CONFIG_ENABLE_JTAG_PASSWD" >> $@
endif

ifeq ($(ENABLE_USB), y)
	@echo "#define CONFIG_ENABLE_USB" >> $@

ifeq ($(ENABLE_FAT_WRITE), y)
	@echo "#define CONFIG_ENABLE_FAT_WRITE" >> $@
endif

ifeq ($(USB_WORKMODE), speed)
	@echo "#define CONFIG_USB_WORKMODE_SPEED" >> $@
else
	@echo "#define CONFIG_USB_WORKMODE_COMPATIBLE" >> $@
endif
endif

ifeq ($(ENABLE_GX_OTP), y)
	@echo "#define CONFIG_ENABLE_GX_OTP" >> $@
endif

ifeq ($(ENABLE_OTP_FULLFUNCTION), y)
	@echo "#define CONFIG_ENABLE_OTP_FULLFUNCTION" >> $@
endif

ifeq ($(ENABLE_NET), y)
	@echo "#define CONFIG_ENABLE_NET" >> $@
endif

ifeq ($(ENABLE_USB_NETWORK_ASIX88179), y)
	@echo "#define CONFIG_ENABLE_USB_NETWORK_ASIX88179" >> $@
endif

ifeq ($(NET_PHY), PHY0)
	@echo "#define NET_PHY_SELECT PHY0" >> $@
else
	@echo "#define NET_PHY_SELECT PHY1" >> $@
endif

ifeq ($(ENABLE_I2C), y)
	@echo "#define CONFIG_ENABLE_I2C" >> $@
	@echo '#define CONFIG_I2C_TYPE_$(CONFIG_I2C_TYPE)' >> $@
ifneq ($(CONFIG_I2C_BUS_FREQ), )
	@echo '#define CONFIG_I2C_BUS_FREQ $(CONFIG_I2C_BUS_FREQ)' >> $@
endif
endif

ifeq ($(ENABLE_GPIO), y)
	@echo "#define CONFIG_ENABLE_GPIO" >> $@
endif
	@echo '#define CONFIG_GPIO_SET_NUM    $(CONFIG_GPIO_SET_NUM)'   >> $@
ifeq ($(CONFIG_GPIO_GX_V1), y)
	@echo "#define CONFIG_GPIO_GX_V1" >> $@
endif
ifeq ($(CONFIG_GPIO_GX_V2), y)
	@echo "#define CONFIG_GPIO_GX_V2" >> $@
endif

ifeq ($(CONFIG_TIME_GX_V1), y)
	@echo "#define CONFIG_TIME_GX_V1" >> $@
endif

ifeq ($(CONFIG_TIME_GX_V2), y)
	@echo "#define CONFIG_TIME_GX_V2" >> $@
endif

ifeq ($(ENABLE_EEPROM), y)
	@echo "#define CONFIG_ENABLE_EEPROM" >>  $@
	@echo '#define CONFIG_EEPROM_I2C_BUS_NUM        $(EEPROM_I2C_BUS_NUM)' >> $@
	@echo '#define CONFIG_EEPROM_DEVICE_ADDR        $(EEPROM_DEVICE_ADDR)' >> $@
	@echo '#define CONFIG_EEPROM_TYPE               $(EEPROM_TYPE)' >> $@
endif

ifeq ($(ENABLE_IRR), y)
	@echo "#define CONFIG_ENABLE_IRR" >> $@
endif
ifeq ($(ENABLE_CHIP_INFO), y)
	@echo "#define CONFIG_ENABLE_CHIP_INFO" >> $@
endif
ifeq ($(ENABLE_WDT), y)
	@echo "#define CONFIG_ENABLE_WDT" >> $@
endif

ifeq ($(ENABLE_LOWPOWER), y)
	@echo "#define CONFIG_ENABLE_LOWPOWER" >> $@
endif

ifeq ($(ENABLE_HARDWARE_HASH), y)
	@echo "#define CONFIG_ENABLE_HARDWARE_HASH" >> $@
endif

ifeq ($(ENABLE_CLOSE_PINGPANG), y)
	@echo "#define CONFIG_ENABLE_CLOSE_PINGPANG" >> $@
endif

ifeq ($(ENABLE_SCPU), y)
	@echo "#define CONFIG_ENABLE_SCPU" >> $@
ifeq ($(SCPU_BIN_FLASH_ADDR), )
else
	@echo '#define CONFIG_SCPU_BIN_FLASH_ADDR       $(SCPU_BIN_FLASH_ADDR)' >> $@
endif
endif

ifeq ($(ENABLE_CTR_CALLBACK), y)
	@echo "#define CONFIG_ENABLE_CTR_CALLBACK" >> $@
endif

ifeq ($(ENABLE_LOGO), y)
	@echo "#define CONFIG_ENABLE_LOGO" >> $@
endif
ifeq ($(CONFIG_HDMI_PHY_V100), y)
	@echo "#define ENABLE_HDMI_PHY_V100" >> $@
endif
ifeq ($(CONFIG_HDMI_PHY_V200), y)
	@echo "#define ENABLE_HDMI_PHY_V200" >> $@
endif

ifeq ($(ENABLE_PAL), y)
	@echo "#define ENABLE_PAL" >> $@
endif
ifeq ($(ENABLE_PAL_M), y)
	@echo "#define ENABLE_PAL_M" >> $@
endif
ifeq ($(ENABLE_PAL_N), y)
	@echo "#define ENABLE_PAL_N" >> $@
endif
ifeq ($(ENABLE_PAL_NC), y)
	@echo "#define ENABLE_PAL_NC" >> $@
endif
ifeq ($(ENABLE_NTSC_M), y)
	@echo "#define ENABLE_NTSC_M" >> $@
endif
ifeq ($(ENABLE_NTSC_443), y)
	@echo "#define ENABLE_NTSC_443" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_480I), y)
	@echo "#define ENABLE_YPBPR_HDMI_480I" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_480P), y)
	@echo "#define ENABLE_YPBPR_HDMI_480P" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_576I), y)
	@echo "#define ENABLE_YPBPR_HDMI_576I" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_576P), y)
	@echo "#define ENABLE_YPBPR_HDMI_576P" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_720P_50HZ), y)
	@echo "#define ENABLE_YPBPR_HDMI_720P_50HZ" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_720P_60HZ), y)
	@echo "#define ENABLE_YPBPR_HDMI_720P_60HZ" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_1080I_50HZ), y)
	@echo "#define ENABLE_YPBPR_HDMI_1080I_50HZ" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_1080I_60HZ), y)
	@echo "#define ENABLE_YPBPR_HDMI_1080I_60HZ" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_1080P_50HZ), y)
	@echo "#define ENABLE_YPBPR_HDMI_1080P_50HZ" >> $@
endif
ifeq ($(ENABLE_YPBPR_HDMI_1080P_60HZ), y)
	@echo "#define ENABLE_YPBPR_HDMI_1080P_60HZ" >> $@
endif


ifeq ($(CONFIG_NO_PRINT), y)
	@echo "#define CONFIG_NO_PRINT" >> $@
endif

	@echo "#define CONFIG_PRINT_LEVEL  $(CONFIG_PRINT_LEVEL)" >> $@

ifeq ($(CONFIG_NO_GETC), y)
	@echo "#define CONFIG_NO_GETC" >> $@
endif

ifeq ($(ENABLE_CMD), y)
	@echo "#define CONFIG_ENABLE_CMD" >> $@
endif
ifeq ($(ENABLE_MTC), y)
	@echo "#define CONFIG_ENABLE_MTC" >> $@
ifeq ($(ENABLE_MTC_TEST), y)
	@echo "#define CONFIG_ENABLE_MTC_TEST" >> $@
endif
endif

ifeq ($(CONFIG_PANEL), y)
	@echo "#define CONFIG_PANEL" >> $@
endif

ifneq ($(CONFIG_CRYPT_PART_INFO), )
	@echo "" >> $@
	@echo "#define CONFIG_CRYPT_PART_INFO \"$(CONFIG_CRYPT_PART_INFO)\"" >> $@
endif

#############################################################
#
##customer define
#
#############################################################
ifeq ($(ENABLE_USB_RECOVER), y)
	@echo "#define ENABLE_USB_RECOVER" >> $@
	@echo "#define CONFIG_RECOVERY_IMAGE_SIZE    $(CONFIG_RECOVERY_IMAGE_SIZE)" >> $@
	@echo "#define CONFIG_GDI_BPP          $(CONFIG_GDI_BPP)" >> $@
ifneq ($(CONFIG_RECOVERY_IMAGE),)
	@echo "#define CONFIG_RECOVERY_IMAGE    \"$(CONFIG_RECOVERY_IMAGE)\"" >> $@
endif
ifeq ($(USB_RECOVER_WHOLE_IMAGE), y)
	@echo "#define USB_RECOVER_WHOLE_IMAGE" >> $@
endif
endif

	@echo "" >> $@

	@echo "#define DRAMBASE                         $(DRAMBASE)" >> $@
	@echo "#define SRAMBASE                         $(SRAMBASE)" >> $@
	@echo "/* DRAM Memory space distribution */"                 >> $@
ifeq ($(CONFIG_ARCH), ARM)
	@echo "#define KERNEL_DTB_START_ADDR            $(KERNEL_DTB_START_ADDR)" >> $@
	@echo "#define KERNEL_DTB_END_ADDR              $(KERNEL_DTB_END_ADDR)" >> $@
	@echo "#define KERNEL_START_ADDR                $(KERNEL_START_ADDR)" >> $@
	@echo "#define KERNEL_END_ADDR                  $(KERNEL_END_ADDR)" >> $@
else
# CSKY
	@echo "#define KERNEL_START_ADDR                $(KERNEL_START_ADDR)" >> $@
	@echo "#define KERNEL_END_ADDR                  $(KERNEL_END_ADDR)" >> $@
	@echo "#define KERNEL_DTB_START_ADDR            $(KERNEL_DTB_START_ADDR)" >> $@
	@echo "#define KERNEL_DTB_END_ADDR              $(KERNEL_DTB_END_ADDR)" >> $@
endif
	@echo "#define INITRD_DRAM_START_ADDR           $(INITRD_DRAM_START_ADDR)"   >> $@
	@echo "#define INITRD_DRAM_END_ADDR             $(INITRD_DRAM_END_ADDR)"     >> $@
	@echo "#define OTA_DRAM_START_ADDR              $(OTA_DRAM_START_ADDR)"   >> $@
	@echo "#define OTA_DRAM_END_ADDR                $(OTA_DRAM_END_ADDR)"     >> $@
	@echo "#define HEAP_START_ADDR                  $(HEAP_START_ADDR)"     >> $@
	@echo "#define HEAP_END_ADDR                    $(HEAP_END_ADDR)"       >> $@
	@echo "#define STACK_BOTTOM_ADDR                $(STACK_BOTTOM_ADDR)"      >> $@
	@echo "#define STACK_TOP_ADDR                   $(STACK_TOP_ADDR)"      >> $@
	@echo "#define IRQ_STACK_BOTTOM_ADDR            $(IRQ_STACK_BOTTOM_ADDR)"      >> $@
	@echo "#define IRQ_STACK_TOP_ADDR               $(IRQ_STACK_TOP_ADDR)"      >> $@
ifeq ($(ENABLE_LOADER_STAGE2_RCC), y)
	@echo "#define SPP_BUF_START_ADDR               $(SPP_BUF_START_ADDR)"   >> $@
	@echo "#define SPP_BUF_END_ADDR                 $(SPP_BUF_END_ADDR)"   >> $@
	@echo "#define SVPU_BUF_START_ADDR              $(SVPU_BUF_START_ADDR)"   >> $@
	@echo "#define SVPU_BUF_END_ADDR                $(SVPU_BUF_END_ADDR)"   >> $@
	@echo "#define OSD_BUF_START_ADDR               $(OSD_BUF_START_ADDR)"   >> $@
	@echo "#define OSD_BUF_END_ADDR                 $(OSD_BUF_END_ADDR)"   >> $@
ifeq ($(SECURE_VERIFY_TYPE), SM2)
	@echo "#define LOADER_EXTRA_VERIFY_START_ADDR   $(LOADER_EXTRA_VERIFY_START_ADDR)"     >> $@
	@echo "#define LOADER_EXTRA_VERIFY_END_ADDR     $(LOADER_EXTRA_VERIFY_END_ADDR)"     >> $@
endif
	@echo "#define LOADER_START_ADDR                $(LOADER_START_ADDR)"   >> $@
	@echo "#define LOADER_END_ADDR                  $(LOADER_END_ADDR)"     >> $@
	@echo "#define TEE_LOADER_START_ADDR            $(TEE_LOADER_START_ADDR)"   >> $@
	@echo "#define TEE_LOADER_END_ADDR              $(TEE_LOADER_END_ADDR)"     >> $@
else
ifeq ($(SECURE_VERIFY_TYPE), SM2)
	@echo "#define LOADER_EXTRA_VERIFY_START_ADDR   $(LOADER_EXTRA_VERIFY_START_ADDR)"     >> $@
	@echo "#define LOADER_EXTRA_VERIFY_END_ADDR     $(LOADER_EXTRA_VERIFY_END_ADDR)"     >> $@
endif
	@echo "#define LOADER_START_ADDR                $(LOADER_START_ADDR)"   >> $@
	@echo "#define LOADER_END_ADDR                  $(LOADER_END_ADDR)"     >> $@
	@echo "#define TEE_LOADER_START_ADDR            $(TEE_LOADER_START_ADDR)"   >> $@
	@echo "#define TEE_LOADER_END_ADDR              $(TEE_LOADER_END_ADDR)"     >> $@
	@echo "#define SPP_BUF_START_ADDR               $(SPP_BUF_START_ADDR)"   >> $@
	@echo "#define SPP_BUF_END_ADDR                 $(SPP_BUF_END_ADDR)"   >> $@
	@echo "#define SVPU_BUF_START_ADDR              $(SVPU_BUF_START_ADDR)"   >> $@
	@echo "#define SVPU_BUF_END_ADDR                $(SVPU_BUF_END_ADDR)"   >> $@
	@echo "#define OSD_BUF_START_ADDR               $(OSD_BUF_START_ADDR)"   >> $@
	@echo "#define OSD_BUF_END_ADDR                 $(OSD_BUF_END_ADDR)"   >> $@
endif

	@echo "/* SRAM Memory space distribution */"                 >> $@
	@echo "#define STAGE1_START_ADDR                $(STAGE1_START_ADDR)"       >> $@
	@echo "#define STAGE1_END_ADDR                  $(STAGE1_END_ADDR)"         >> $@
	@echo "#define GPIO_TABLE_START_ADDR            $(GPIO_TABLE_START_ADDR)"   >> $@
	@echo "#define GPIO_TABLE_END_ADDR              $(GPIO_TABLE_END_ADDR)"     >> $@
	@echo "#define STAGE1_STACK_BOTTOM_ADDR         $(STAGE1_STACK_BOTTOM_ADDR)">> $@
	@echo "#define STAGE1_STACK_TOP_ADDR            $(STAGE1_STACK_TOP_ADDR)"   >> $@
ifeq ($(CONFIG_ARCH), ARM)
	@echo "#define STAGE1_STACK_TOP_ADDR_NOMMU      ($(STAGE1_STACK_TOP_ADDR) - $(VIR_PHY_OFFSET))"   >> $@
else
# CSKY
	@echo "#define STAGE1_STACK_TOP_ADDR_NOMMU      ($(STAGE1_STACK_TOP_ADDR) - 0x$(REG_ADDR_HEAD)0000000)"   >> $@
endif


	@echo ""                                                     >>$@
	@echo "#define DRAM_SIZE                        $(DRAM_SIZE)" >> $@
	@echo "#define SRAM_SIZE                        $(SRAM_SIZE)" >> $@

	@echo ""                                                     >>$@
	@echo "#define ROM_COPY_SIZE                    $(ROM_COPY_SIZE)" >> $@
	@echo "#define CMDLINE_DRAM_SIZE                $(CMDLINE_DRAM_SIZE)" >> $@

	@echo ""                                                     >>$@
	@echo "/* command line argument */"                          >>$@
ifneq ($(CMDLINE_VALUE), )
	@cat $(TEMP_CFG_FILE)                                        >>$@
	@rm -f $(TEMP_CFG_FILE)
	@rm -f $(CONF_FILE)
endif

	@echo ""                                                     >>$@
	@echo "#define DRAM_SIZE_ALIGN                  $(DRAM_SIZE_ALIGN)" >> $@
	@echo "#define KERNEL_DTB_SIZE                  $(KERNEL_DTB_SIZE)" >> $@
	@echo "#define KERNEL_SIZE                      $(KERNEL_SIZE)" >> $@
	@echo "#define INITRD_DRAM_SIZE                 $(INITRD_DRAM_SIZE)" >> $@
	@echo "#define HEAP_SIZE                        $(HEAP_SIZE)" >> $@
	@echo "#define STACK_SIZE                       $(STACK_SIZE)" >> $@
	@echo "#define IRQ_STACK_SIZE                   $(IRQ_STACK_SIZE)" >> $@
	@echo "#define LOGO_PIXEL_SIZE                  $(LOGO_PIXEL_SIZE)"   >> $@
	@echo "#define SPP_BUF_SIZE                     $(SPP_BUF_SIZE)" >> $@
	@echo "#define SVPU_BUF_SIZE                    $(SVPU_BUF_SIZE)" >> $@
	@echo "#define OSD_BUF_SIZE                     $(OSD_BUF_SIZE)" >> $@
	@echo "#define OTA_DRAM_SIZE                    $(OTA_DRAM_SIZE)" >> $@
	@echo "#define BOOTLOADER_STAGE2_SIZE           $(BOOTLOADER_STAGE2_SIZE)" >> $@
	@echo "#define BOOTLOADER_SIZE                  $(BOOTLOADER_SIZE)" >> $@
	@echo "#define BOOTLOADER_BIN_SIZE              $(BOOTLOADER_BIN_SIZE)" >> $@
	@echo "#define TEE_BOOTLOADER_SIZE              $(TEE_BOOTLOADER_SIZE)" >> $@


ifeq ($(ENABLE_IRQ), y)
	@echo "#define CONFIG_STACKSIZE_IRQ             $(CONFIG_STACKSIZE_IRQ)"      >> $@
endif
	@echo "" >> $@

ifeq ($(ENABLE_OTA), y)
ifeq ($(ENABLE_OTA_FORCE_DRAM_ADDR), y)
	@echo "#define CONFIG_ENABLE_OTA_FORCE_DRAM_ADDR" >> $@
endif
endif
ifeq ($(ENABLE_LIB), y)
ifeq ($(ENABLE_OTA_FORCE_DRAM_ADDR), y)
ifneq ($(ENABLE_OTA), y)
	@echo "#define CONFIG_ENABLE_OTA_FORCE_DRAM_ADDR" >> $@
endif
endif
endif


	@echo "/* NOTE: here GX_PAGETABLE_BASE is just one relative address */" >> $@
	@echo "#define GX_PAGETABLE_BASE                0x00030000" >> $@

ifeq ($(CONFIG_ARCH), ARM)
	@echo "#define GX_REG_VIRTUAL_BASE1             $(VIR_PHY_OFFSET)" >> $@
	@echo "#define GX_REG_VIRTUAL_BASE2             $(VIR_PHY_OFFSET)" >> $@
else
# CSKY
	@echo "#define GX_REG_VIRTUAL_BASE1         0x$(REG_ADDR_HEAD)0000000" >> $@
	@echo "#define GX_REG_VIRTUAL_BASE2         0x$(REG_ADDR_HEAD)0000000" >> $@
endif
	@echo "#define GX_DRAM_VIRTUAL_OFFSET           $(VIR_PHY_OFFSET)" >> $@

	@echo "" >> $@
ifeq ($(CONFIG_EMBED_FLASH_TABLE), y)
	@echo "#define CONFIG_EMBED_FLASH_TABLE" >> $@
endif
	@echo "#define FLASH_TABLE_SEARCH_START_ADDR    $(FLASH_TABLE_SEARCH_START_ADDR)" >> $@
	@echo "#define FLASH_TABLE_SEARCH_SKIP_SIZE	    $(FLASH_TABLE_SEARCH_SKIP_SIZE)" >> $@
	@echo "#define FLASH_TABLE_SEARCH_END_ADDR	   ($(FLASH_TABLE_SEARCH_START_ADDR) + $(FLASH_TABLE_SEARCH_SIZE))" >> $@
ifeq ($(CONFIG_PARTITION_V5), y)
	@echo "#define CONFIG_PARTITION_V5" >> $@
endif
	@echo "" >> $@
ifneq ($(OTA_FORCE_FLASH_ADDR), )
	@echo "#define OTA_FORCE_FLASH_ADDR    $(OTA_FORCE_FLASH_ADDR)"       >> $@
endif
ifneq ($(OTA_FORCE_FLASH_SIZE), )
	@echo "#define OTA_FORCE_FLASH_SIZE    $(OTA_FORCE_FLASH_SIZE)"       >> $@
endif
ifneq ($(OTA_FORCE_MULTI_SECTION_FLASH), )
	@echo '#define OTA_FORCE_MULTI_SECTION_FLASH    $(OTA_FORCE_MULTI_SECTION_FLASH)'       >> $@
endif
	@echo "" >> $@

ifeq ($(ENABLE_THIRDLIB), y)
	@echo "#define CONFIG_ENABLE_THIRDLIB" >> $@
endif

ifeq ($(ENABLE_BOOT_TOOL), y)
	@echo "#define ENABLE_BOOT_TOOL" >> $@
endif
ifeq ($(CONFIG_CHECK_STAGE2_HASH), y)
	@echo "#define CONFIG_CHECK_STAGE2_HASH" >> $@
endif

	@echo "" >> $@
ifeq ($(ENABLE_APP_DECRYPT), y)
	@echo "#define ENABLE_APP_DECRYPT" >> $@
endif
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@echo "#define ENABLE_SECURE_VERIFY" >> $@
	@echo "#define CONFIG_SECURE_VERIFY_TYPE_$(SECURE_VERIFY_TYPE)"    >> $@
ifeq ($(CONFIG_PKA), y)
	@echo "#define CONFIG_PKA" >> $@
endif
endif
	@echo "#define EXTRA_VERIFY_USED_SIZE    $(EXTRA_VERIFY_USED_SIZE)" >> $@

ifeq ($(ENABLE_NAGRA_MODE), y)
	@echo "#define CONFIG_NAGRA_MODE" >> $@
endif

	@echo "" >> $@
ifeq ($(ENABLE_DATA_ENCRYPT), y)
	@echo "#define ENABLE_DATA_ENCRYPT" >> $@
	@echo "#define CONFIG_DATA_ENCRYPT_TYPE           \"$(DATA_ENCRYPT_TYPE)\""     >> $@
	@echo "#define CONFIG_DATA_ENCRYPT_TYPE_$(DATA_ENCRYPT_ALG)"     >> $@
	@echo "#define CONFIG_DATA_ENCRYPT_TYPE_$(DATA_ENCRYPT_OPT)"     >> $@
ifeq ($(STAGE1_ENCRYPT), 1)
	@echo "#define ENABLE_STAGE1_ENCRYPT" >> $@
ifeq ($(STAGE1_ENCRYPT_DERIVE), 1)
	@echo "#define ENABLE_STAGE1_ENCRYPT_DERIVE" >> $@
endif
endif
ifeq ($(STAGE2_ENCRYPT), 1)
	@echo "#define ENABLE_STAGE2_ENCRYPT" >> $@
ifeq ($(STAGE2_ENCRYPT_DERIVE), 1)
	@echo "#define ENABLE_STAGE2_ENCRYPT_DERIVE" >> $@
endif
endif
endif
	@echo "" >> $@

ifeq ($(ENABLE_SECURE_ALIGN), y)
	@echo "#define ENABLE_SECURE_ALIGN" >> $@
endif

ifneq ($(MEMORY_PROTECT_TYPE), )
	@echo "#define CONFIG_MEMORY_PROTECT_TYPE    $(MEMORY_PROTECT_TYPE)" >> $@
else
	@echo "#define CONFIG_MEMORY_PROTECT_TYPE    0" >> $@
endif

ifeq ($(SECURE_FIRMWARE_LOAD), y)
	@echo "#define CONFIG_SECURE_FIRMWARE_LOAD" >> $@
endif

ifeq ($(ENABLE_VERIFY), y)
	@echo "#define ENABLE_VERIFY" >> $@
endif

ifeq ($(ENABLE_DVTCAS_VERIFY), y)
	@echo "#define ENABLE_DVTCAS_VERIFY" >> $@
endif

ifeq ($(ENABLE_BOOT_FROM_USB), y)
	@echo "#define CONFIG_ENABLE_BOOT_FROM_USB" >> $@
	@echo "#define BOOT_FROM_USB_IMAGE_NAME            \"$(BOOT_FROM_USB_IMAGE_NAME)\"" >> $@
	@echo "#define BOOT_FROM_USB_DTB_NAME             \"$(BOOT_FROM_USB_DTB_NAME)\"" >> $@
	@echo "#define BOOT_FROM_USB_DTB_LOAD_ADDR          $(BOOT_FROM_USB_DTB_LOAD_ADDR)" >> $@
	@echo "#define BOOT_FROM_USB_MAGIC         $(BOOT_FROM_USB_MAGIC)" >> $@
endif

ifeq ($(CONFIG_BOOT_KERNEL_FROM_FIXED_ADDR), y)
	@echo "#define CONFIG_BOOT_KERNEL_FROM_FIXED_ADDR  $(CONFIG_BOOT_KERNEL_FROM_FIXED_ADDR)" >> $@
	@echo "#define CONFIG_KERNEL_IMAGE_ADDR            $(CONFIG_KERNEL_IMAGE_ADDR)" >> $@
	@echo "#define CONFIG_KERNEL_IMAGE_SIZE            $(CONFIG_KERNEL_IMAGE_SIZE)" >> $@
endif

ifeq ($(CONFIG_EMBED_KERNEL_IMG), y)
	@echo "#define CONFIG_EMBED_KERNEL_IMG" >> $@
	@echo "#define CONFIG_EMBED_KERNEL_IMG_FILE      \"$(CONFIG_EMBED_KERNEL_IMG_FILE)\"" >> $@
endif

ifeq ($(CONFIG_LOAD_GX_RECOVERY_OS), y)
	@echo "#define CONFIG_LOAD_GX_RECOVERY_OS" >> $@
endif

#############################################################
#
## Compression/Decompression algorithm support
#
#############################################################
ifeq ($(ENABLE_COMPRESS_ZLIB), y)
	@echo "#define CONFIG_ENABLE_COMPRESS_ZLIB" >> $@
endif

ifeq ($(ENABLE_DECOMPRESS_ZLIB), y)
	@echo "#define CONFIG_ENABLE_DECOMPRESS_ZLIB" >> $@
endif

ifeq ($(ENABLE_DECOMPRESS_LZO), y)
	@echo "#define CONFIG_ENABLE_DECOMPRESS_LZO" >> $@
endif

ifeq ($(ENABLE_DECOMPRESS_LZMA), y)
	@echo "#define CONFIG_ENABLE_DECOMPRESS_LZMA" >> $@
endif

ifeq ($(ENABLE_DECOMPRESS_GZIP), y)
ifneq ($(ENABLE_DECOMPRESS_ZLIB), y)
	@echo "set 'ENABLE_DECOMPRESS_GZIP = y' must set 'ENABLE_DECOMPRESS_ZLIB = y' first!"
	@exit 1
endif
	@echo "#define CONFIG_ENABLE_DECOMPRESS_GZIP" >> $@
endif
	@echo "#define CONFIG_ROM_SERIAL_BAUDRATE            $(ROM_SERIAL_BAUDRATE)"   >> $@

ifeq ($(ENABLE_EXTERN_PARAM), y)
	@echo "#define CONFIG_EXTERN_PARAM"           >> $@
	@echo "#define EXTERN_PARAM_FILE_SIZE                $(EXTERN_PARAM_FILE_SIZE)"   >> $@
	@echo "#define EXTERN_PARAM_FILE_COUNT                $(EXTERN_PARAM_FILE_COUNT)"   >> $@
endif

ifeq ($(CONFIG_STAGE2_COMPRESSED), y)
	@echo "#define CONFIG_STAGE2_COMPRESSED" >> $@
endif

#############################################################
#
## clock control
#
#############################################################
ifeq ($(CONFIG_CLOCK_GP_DISABLE), y)
	@echo "#define CONFIG_CLOCK_GP_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_GSE_DISABLE), y)
	@echo "#define CONFIG_CLOCK_GSE_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_PIDFILTER_DISABLE), y)
	@echo "#define CONFIG_CLOCK_PIDFILTER_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_ETH_DISABLE), y)
	@echo "#define CONFIG_CLOCK_ETH_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_SDIO_DISABLE), y)
	@echo "#define CONFIG_CLOCK_SDIO_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_SMARTCARD_DISABLE), y)
	@echo "#define CONFIG_CLOCK_SMARTCARD_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_DVB_DISABLE), y)
	@echo "#define CONFIG_CLOCK_DVB_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_HDMI_DISABLE), y)
	@echo "#define CONFIG_CLOCK_HDMI_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_VDEC_LOWPOWER), y)
	@echo "#define CONFIG_CLOCK_VDEC_LOWPOWER" >> $@
endif
ifeq ($(CONFIG_CLOCK_SCPU_DISABLE), y)
	@echo "#define CONFIG_CLOCK_SCPU_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_FM_DISABLE), y)
	@echo "#define CONFIG_CLOCK_FM_DISABLE" >> $@
endif
ifeq ($(CONFIG_CLOCK_TSIO_DISABLE), y)
	@echo "#define CONFIG_CLOCK_TSIO_DISABLE" >> $@
endif
#############################################################
#
## bbt test
#
#############################################################
ifeq ($(ENABLE_BBT_SLT_TEST), y)
	@echo "#define CONFIG_ENABLE_BBT_SLT_TEST" >> $@
endif


	@echo "" >> $@
	@echo "#endif" >> $@
