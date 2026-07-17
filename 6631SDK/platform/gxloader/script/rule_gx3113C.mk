#/*
# * =====================================================================================
# *
# *       Filename:  rule_CK_GX3113C.mk
# *
# *       Compiler:  gcc
# *       Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
# *
# * =====================================================================================
# */
# Memory layout arithmetic

REG_ADDR_HEAD	    = a
DRAM_ADDR_HEAD	    = 9
DRAMBASE            = 0x$(DRAM_ADDR_HEAD)0000000
SRAMBASE	        = 0x$(REG_ADDR_HEAD)0100000
SRAM_SIZE           = 0x00003000
VIR_PHY_OFFSET      = 0x80000000
DRAM_PHY_ADDR		= 0x10000000

TEMP_CFG_FILE       = .tmp_file
CONF_FILE           = .cfg

CMDLINE_VALUE := $(shell ./tools/cmdline_update .config $(DRAM_PHY_ADDR) $(TEMP_CFG_FILE))

include $(CONF_FILE)

DRAM_SIZE_ALIGN := $(shell printf 0x%x $(shell ./script/calc.sh 1024 \* 1024 \* 16))

ifeq ($(DRAM_SIZE), )
	DRAM_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh \( $(CMDLINE_DRAM_SIZE) + $(DRAM_SIZE_ALIGN) - 1 \) / $(DRAM_SIZE_ALIGN) \* $(DRAM_SIZE_ALIGN)))
endif

ifeq ($(KERNEL_SIZE), )
	KERNEL_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(CMDLINE_MEM_SIZE) / 2))
endif

ifeq ($(STACK_SIZE), )
	STACK_SIZE = 0x200000
endif

ifeq ($(BOOTLOADER_SIZE), )
	BOOTLOADER_SIZE = 0x20000
endif

ifeq ($(BOOTLOADER_BIN_SIZE), )
	BOOTLOADER_BIN_SIZE = $(BOOTLOADER_SIZE)
endif

ifeq ($(BOOTLOADER_STAGE2_SIZE), )
	BOOTLOADER_STAGE2_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(BOOTLOADER_BIN_SIZE) + 0x80000))
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

ALL_SECURE_VERIFY_TYPE = "RSA_PKCS"
ifneq ($(findstring $(SECURE_VERIFY_TYPE),$(ALL_SECURE_VERIFY_TYPE)), )
	ENABLE_SECURE_VERIFY = y
endif

SVPU_BUF_SIZE = 0x0
HEAP_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(DRAM_SIZE) - $(KERNEL_SIZE) - $(STACK_SIZE) - $(SPP_BUF_SIZE) - $(SVPU_BUF_SIZE) - $(OSD_BUF_SIZE) - $(OTA_DRAM_SIZE) - $(BOOTLOADER_STAGE2_SIZE) - $(CMDLINE_FWMEM_SIZE)))

#
# DRAM : KERNEL + HEAP + STACK + SPP_BUF + SVPU_BUF + OTA + LOADER
#
KERNEL_START_ADDR := $(DRAMBASE)
KERNEL_END_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(KERNEL_START_ADDR) + $(KERNEL_SIZE)))
OTA_DRAM_START_ADDR := $(KERNEL_END_ADDR)
OTA_DRAM_END_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(OTA_DRAM_START_ADDR) + $(OTA_DRAM_SIZE)))
HEAP_START_ADDR := $(OTA_DRAM_END_ADDR)
HEAP_END_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(HEAP_START_ADDR) + $(HEAP_SIZE)))
STACK_BOTTOM_ADDR := $(HEAP_END_ADDR)
STACK_TOP_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(STACK_BOTTOM_ADDR) + $(STACK_SIZE)))
LOADER_START_ADDR := $(STACK_TOP_ADDR)
LOADER_END_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(LOADER_START_ADDR) + $(BOOTLOADER_STAGE2_SIZE)))
SPP_BUF_START_ADDR := $(LOADER_END_ADDR)
SPP_BUF_END_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(SPP_BUF_START_ADDR) + $(SPP_BUF_SIZE)))
SVPU_BUF_START_ADDR := $(SPP_BUF_END_ADDR)
SVPU_BUF_END_ADDR := $(shell printf 0x%x $(shell ./script/calc.sh $(SVPU_BUF_START_ADDR) + $(SVPU_BUF_SIZE)))
OSD_BUF_START_ADDR := $(SVPU_BUF_END_ADDR)
OSD_BUF_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(OSD_BUF_START_ADDR) + $(OSD_BUF_SIZE)))

# align OTA_DRAM_START_ADDR
OTA_DRAM_START_ALIGN_ADDR := $(shell printf 0x%x $(shell ./script/align.sh $(OTA_DRAM_START_ADDR) 1024 1))
OTA_DRAM_SIZE := $(shell printf 0x%x $(shell ./script/calc.sh $(OTA_DRAM_SIZE) + $(OTA_DRAM_START_ADDR) - $(OTA_DRAM_START_ALIGN_ADDR)))
OTA_DRAM_START_ADDR := $(OTA_DRAM_START_ALIGN_ADDR)

CHIP_CORE_REV	    = 3
ROM_COPY_SIZE	    = 0x1000

ifeq ($(ENABLE_SECURE_VERIFY), y)
	SRAM_SECURE_USED_SIZE = 0x500 # Storage security information
	SRAM_USED_SIZE = $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAM_SIZE) - $(SRAM_SECURE_USED_SIZE)))
else
	SRAM_LOWPOWER_USED_SIZE = 0x80
	SRAM_USED_SIZE = $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAM_SIZE) - $(SRAM_LOWPOWER_USED_SIZE)))
endif

ifeq ($(GPIO_TABLE_SIZE), )
	GPIO_TABLE_SIZE = 0x400
endif

#
# SRAM : STAGE1 + GPIO_TABLE + STAGE1_STACK
#
STAGE1_START_ADDR := $(SRAMBASE)
STAGE1_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAMBASE) + $(ROM_COPY_SIZE)))
GPIO_TABLE_START_ADDR := $(STAGE1_END_ADDR)
GPIO_TABLE_END_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(GPIO_TABLE_START_ADDR) + $(GPIO_TABLE_SIZE)))
STAGE1_STACK_BOTTOM_ADDR := $(GPIO_TABLE_END_ADDR)
STAGE1_STACK_TOP_ADDR := $(shell printf 0x%08x $(shell ./script/calc.sh $(SRAMBASE) + $(SRAM_USED_SIZE)))

ifeq ($(FLASH_TABLE_SEARCH_START_ADDR), )
	FLASH_TABLE_SEARCH_START_ADDR = 0
endif

ifeq ($(FLASH_TABLE_SEARCH_SKIP_SIZE), )
	FLASH_TABLE_SEARCH_SKIP_SIZE = 0x800
endif

ifeq ($(FLASH_TABLE_SEARCH_SIZE), )
	FLASH_TABLE_SEARCH_SIZE = 0x400000
endif

ifeq ($(CONFIG_24M_XTAL), y)
	DEFAULT_EXT_CLOCK_XTAL = 24000000
else
	DEFAULT_EXT_CLOCK_XTAL = 27000000
endif

ADAPTIVE_FLASH = $(shell echo $(ENABLE_SPINAND)$(ENABLE_NANDFLASH) | grep "yy")

ifneq ($(DDR_SIZE), )
	COMMON_CFG := $(shell printf $(shell ./script/calc_compare.sh $(DDR_SIZE) 256))
ifeq ($(COMMON_CFG), 1)
	DDR_CONFIG =
else
	DDR_CONFIG = "_$(DDR_SIZE)MB"
endif
endif

DDR_TYPE_CH = $(shell echo $(DDR_TYPE) | tr '[A-Z]' '[a-z]')
DDR_FREQ = $(shell expr $(DDR_FREQUENCY) / 1000000)"MHz"

ifeq ($(CONFIG_SIP), y)
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_$(DDR_FREQ)_sip$(DDR_CONFIG).h"
else
ifeq ($(CONFIG_BGA), y)
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_bga_$(DDR_FREQ)$(DDR_CONFIG).h"
else
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_$(DDR_FREQ)$(DDR_CONFIG).h"
endif
endif

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

# Object files
SOBJS-y = cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_start.o
SOBJS-$(ENABLE_LIB) += cpu/ck/setjmp.o
COBJS-y += cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_pll_mini.o \
	cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_pll_full.o \
	cpu/ck/$(CHIP_CORE)/$(CHIP_CORE)_sdram.o \
	cpu/ck/cpu.o

COBJS-y += cpu/copy.o
COBJS-$(ENABLE_MEMORY) += memtest.o
COBJS-$(ENABLE_MEMORY_TEST) += test/memory/mem_test.o
COBJS-$(ENABLE_MEMORY_PHASE_TEST) += test/memory/memphase_test.o

COBJS-y += partition.o     \

COBJS-y += boot.o          \
	bootconfig.o           \
	common/util.o          \
	common/crc32.o         \
	common/meminfo.o       \
	common/get_mem_info.o  \
	load_kernel/load_kernel.o

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

COBJS-$(ENABLE_OTA) += ota/gxota.o \
	ota/libini.o

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

COBJS-$(ENABLE_IRR) += 	drivers/irr/gx_irr.o

COBJS-$(ENABLE_CTR)  += drivers/ctr/ctr.o
COBJS-$(ENABLE_GPIO)  += drivers/gpio/gx_gpio.o
COBJS-$(ENABLE_EEPROM)  += drivers/eeprom/eeprom.o
COBJS-$(ENABLE_CHIP_INFO) += drivers/chip_info/chip_info.o
COBJS-$(ENABLE_WDT) += drivers/watchdog/gx_wdt.o
COBJS-$(ENABLE_TIME)  += drivers/time/time.o
COBJS-$(ENABLE_JTAG_PASSWD)  += drivers/jtag/jtagpasswd.o

ifeq ($(ENABLE_I2C), y)
	COBJS-y += drivers/i2c/i2c-core.o
ifeq ($(CONFIG_I2C_TYPE), DW)
	COBJS-y += drivers/i2c/dw_i2c.o
else
	CONFIG_I2C_TYPE = GX
	COBJS-y += drivers/i2c/gx_i2c.o
endif
endif

COBJS-$(ENABLE_IRQ) += drivers/intc/interrupt.o \
	drivers/intc/nationalchip/nc.o

PANEL_FILES=$(patsubst %c,%o,$(shell ls board/$(CHIP_CORE)/board-$(CHIP_BOARD)/panel*.c 2>/dev/null))
COBJS-y += board/$(CHIP_CORE)/$(CHIP_CORE)_api.o              \
	board/$(CHIP_CORE)/board-$(CHIP_BOARD)/board-init.o                \
	board/$(CHIP_CORE)/board-common.o                \
	$(PANEL_FILES)	\
	drivers/flash/flash.o

COBJS-$(ENABLE_CMD) += bootmenu.o commands.o

COBJS-$(ENABLE_LOGO) += drivers/jpeg/gx3201_jpeg.o \
			drivers/vout/gx3113C_vout.o

COBJS-$(ENABLE_NANDFLASH) += drivers/flash/nand/nand_flash.o

ifeq ($(ENABLE_SPIFLASH), y)
COBJS-y += drivers/platform/device.o
COBJS-y += drivers/spi/spi.o
ifeq ($(SPI_TYPE), DW)
COBJS-y += drivers/spi/dw_spi/dw_spim.o
else
COBJS-y += drivers/spi/gx_spi/gx_spi.o
endif
else
ifeq ($(ENABLE_SPINAND), y)
COBJS-y += drivers/platform/device.o
COBJS-y += drivers/spi/spi.o
ifeq ($(SPI_TYPE), DW)
COBJS-y += drivers/spi/dw_spi/dw_spim.o
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



COBJS-$(ENABLE_NET) += \
	common/net/net.o            \
	common/net/net_ipv4.o       \
	common/net/net_ipv4_tftp.o  \
	common/net/net_ipv4_bootp.o \
	common/net/net_ipv4_dns.o   \
	common/net/synopsis.o       \
	common/net/synopGMAC_network_interface.o \
	common/net/synopGMAC_Dev.o

COBJS-$(ENABLE_GX_OTP) += drivers/gx_otp/gx3113c_otp.o

ifeq ($(ENABLE_MEMORY), y)
EXTRA-OBJS += libc/language_c_libc_stdlib_rand.o
endif

COBJS-$(ENABLE_MTC) += drivers/mtc/gx3113c_mtc_core.o\
	drivers/mtc/gx_mtc.o
COBJS-$(ENABLE_MTC_TEST) += test/mtc/mtc_test.o\
	test/mtc/decrypt_test_bin.o

ifeq ($(ENABLE_SECURE_VERIFY), y)
	COBJS-y += common/secure/common.o
	COBJS-y += common/secure/sha/gx_soft_sha256.o
	COBJS-y += common/secure/rsa/rsa.o           \
		common/secure/rsa/rsa_verify.o
endif

ifeq ($(CONFIG_AUTO_CMD), )
	CONFIG_AUTO_CMD = boot
endif

$(SOBJS-y):%.o:%.S
	@echo [$(CC) compiling $@]
	@$(CC) $(CFLAGS) -DBOOT_FROM_FLASH -c $(CPPFLAGS) $< -o $@
$(COBJS-y):%.o:%.c
	@echo [$(CC) compiling $@]
	@$(CC) $(CFLAGS)  -c $(CPPFLAGS) $< -o $@

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

	@echo "#define CONFIG_ARCH              \"$(CONFIG_ARCH)\"" >> $@
	@echo "#define CONFIG_ARCH_CKMMU"         >> $@
	@echo "#define CONFIG_ARCH_CKMMU_GX3113C" >> $@
	@echo "#define CHIP_CORE                   \"$(CHIP_CORE)\"" >> $@
	@echo "#define CONFIG_REV                   \"$(CHIP_CORE_REV)\"" >> $@
	@echo "#define CONFIG_ARCH_CKMMU_GX3113C_REV   $(CHIP_CORE_REV)" >> $@

	@echo "#define CHIP_BOARD                 \"$(CHIP_BOARD)\"" >> $@
	@echo "#define CHIP_BOARD_$(shell echo $(CHIP_BOARD) | tr "-" "_" )" >> $@
	@echo "" >> $@

ifeq ($(CONFIG_BGA), y)
	@echo "#define CONFIG_BGA" >> $@
else
ifeq ($(CONFIG_SIP), y)
	@echo "#define CONFIG_SIP" >> $@
else
	@echo "#define CONFIG_LQFP" >> $@
endif
endif

##################################################################
#
##II  OS CHOICE and Option
#
##################################################################
ifeq ($(ENABLE_DENALI_ELF), y)

	@echo "#define ENABLE_DENALI_ELF" >> $@

endif

ifeq ($(ENABLE_BOOT_FROM_USB), y)
	@echo "#define CONFIG_ENABLE_BOOT_FROM_USB" >> $@
	@echo "#define BOOT_FROM_USB_IMAGE_NAME            \"$(BOOT_FROM_USB_IMAGE_NAME)\"" >> $@
	@echo "#define BOOT_FROM_USB_DTB_NAME             \"$(BOOT_FROM_USB_DTB_NAME)\"" >> $@
	@echo "#define BOOT_FROM_USB_DTB_LOAD_ADDR          $(BOOT_FROM_USB_DTB_LOAD_ADDR)" >> $@
	@echo "#define BOOT_FROM_USB_MAGIC         $(BOOT_FROM_USB_MAGIC)" >> $@
endif

ifneq ($(CMDLINE_VALUE), "")
	@echo "#define CONFIG_ENABLE_CMDLINE" >> $@
	@echo '#define CONFIG_CMDLINE_VALUE    "$(CMDLINE_VALUE)"' >> $@
endif

ifneq ($(EXTEND_MTDPARTS), )
	@echo '#define CONFIG_EXTEND_MTDPARTS  $(EXTEND_MTDPARTS)' >> $@
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

	@echo "#define DDR_FREQUENCY_CONFIG         $(DDR_FREQUENCY)" >> $@
	@echo "#define DDR_PAR_INCLUDE              \"$(DDR_PAR_INCLUDE)\"" >> $@
ifneq ($(DDR_SIZE), )
	@echo "#define DDR_SIZE                         $(DDR_SIZE)" >> $@
endif

ifeq ($(DVB_CHANNEL_S2), y)
	@echo "#define DVB_CHANNEL_S2" >> $@
else
ifeq ($(DVB_CHANNEL_C), y)
	@echo "#define DVB_CHANNEL_C" >> $@
endif
endif

ifeq ($(CONFIG_24M_XTAL), y)
	@echo "#define CONFIG_24M_XTAL" >> $@
endif
	@echo "#define DEFAULT_EXT_CLOCK_XTAL      $(DEFAULT_EXT_CLOCK_XTAL)" >> $@

ifeq ($(ENABLE_CHIPTEST), y)
	@echo "#define CONFIG_ENABLE_CHIPTEST" >> $@
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

ifeq ($(ENABLE_DMA), y)
	@echo "#define CONFIG_ENABLE_DMA" >> $@
endif

ifeq ($(ENABLE_MEMORY), y)
	@echo "#define CONFIG_ENABLE_MEMORY" >> $@
endif

ifeq ($(ENABLE_MEMORY_TEST), y)
	@echo "#define CONFIG_ENABLE_MEMORY_TEST" >> $@
endif

ifeq ($(ENABLE_MEMORY_PHASE_TEST), y)
	@echo "#define CONFIG_ENABLE_MEMORY_PHASE_TEST" >> $@
endif

ifeq ($(ENABLE_LIB), y)
	@echo "#define CONFIG_ENABLE_LIB" >> $@
endif

	@echo "" >> $@

#################################################################
#
##IV Debug option
#
#################################################################
ifeq ($(ENABLE_RELEASE), n)
	@echo "#define CONFIG_DEVEL" >> $@
else
	@echo "#define CONFIG_RELEASE" >> $@
endif

ifeq ($(ENABLE_GDB_DEBUG), y)
	@echo "#define CONFIG_ENABLE_GDB_DEBUG" >> $@
endif

ifeq ($(ENABLE_IO_FRAMEWORK), y)
	@echo "#define CONFIG_ENABLE_IO_FRAMEWORK" >> $@
endif

	@echo "#define CONFIG_BOOTDELAY             $(CONFIG_BOOTDELAY)" >> $@
	@echo "#define CONFIG_AUTO_CMD         \"$(CONFIG_AUTO_CMD)\"" >> $@
	@echo "#define CONFIG_UART_PORT             $(CONFIG_UART_PORT)" >> $@
	@echo "#define CONFIG_UART_BAUDRATE         $(CONFIG_UART_BAUDRATE)" >> $@
ifeq ($(CONFIG_UART_TYPE), DW)
	@echo "#define CONFIG_UART_TYPE_DW" >> $@
else
	@echo "#define CONFIG_UART_TYPE_GX" >> $@
endif

ifeq ($(ENABLE_DBG_PIN_NO_MULTT), y)
	@echo "#define CONFIG_DBG_PIN_NO_MULTI" >> $@
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

ifeq ($(SPI_TYPE), DW)
	@echo "#define CONFIG_ENABLE_DWSPI" >> $@
else
	@echo "#define CONFIG_ENABLE_GXSPI" >> $@
endif

ifeq ($(ENABLE_SPIFLASH), y)
	@echo "#define CONFIG_ENABLE_SPI1" >> $@
else
ifeq ($(ENABLE_SPINAND), y)
	@echo "#define CONFIG_ENABLE_SPI1" >> $@
endif
endif

ifneq ($(MEMORY_PROTECT_TYPE), )
	@echo "#define CONFIG_MEMORY_PROTECT_TYPE    $(MEMORY_PROTECT_TYPE)" >> $@
else
	@echo "#define CONFIG_MEMORY_PROTECT_TYPE    0" >> $@
endif

ifeq ($(ENABLE_VERIFY), y)
	@echo "#define ENABLE_VERIFY" >> $@
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
	@echo "#define CONFIG_ENABLE_INTC_NC" >> $@
endif

ifeq ($(ENABLE_UART_IRQ), y)
	@echo "#define CONFIG_ENABLE_UART_IRQ" >> $@
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

ifeq ($(NET_PHY), PHY0)
	@echo "#define NET_PHY_SELECT PHY0" >> $@
else
	@echo "#define NET_PHY_SELECT PHY1" >> $@
endif

ifeq ($(ENABLE_I2C), y)
	@echo "#define CONFIG_ENABLE_I2C" >> $@
	@echo '#define CONFIG_I2C_TYPE_$(CONFIG_I2C_TYPE)' >> $@
endif

ifeq ($(ENABLE_GPIO), y)
	@echo "#define CONFIG_ENABLE_GPIO" >> $@
endif
	@echo '#define CONFIG_GPIO_SET_NUM    3'   >> $@
	@echo "#define CONFIG_GPIO_GX_V1" >> $@

ifeq ($(ENABLE_EEPROM), y)
	@echo "#define CONFIG_ENABLE_EEPROM" >> $@
	@echo '#define CONFIG_EEPROM_I2C_BUS_NUM $(EEPROM_I2C_BUS_NUM)' >> $@
	@echo '#define CONFIG_EEPROM_DEVICE_ADDR $(EEPROM_DEVICE_ADDR)' >> $@
	@echo '#define CONFIG_EEPROM_TYPE        $(EEPROM_TYPE)' >> $@
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

ifeq ($(ENABLE_SMARTCARD1), y)
	@echo "#define CONFIG_ENABLE_SMARTCARD1" >> $@
endif

ifeq ($(ENABLE_SMARTCARD2), y)
	@echo "#define CONFIG_ENABLE_SMARTCARD2" >> $@
endif

ifeq ($(ENABLE_CLOSE_PINGPANG), y)
	@echo "#define CONFIG_ENABLE_CLOSE_PINGPANG" >> $@
endif

ifeq ($(ENABLE_SCPU), y)
	@echo "#define CONFIG_ENABLE_SCPU" >> $@
ifeq ($(SCPU_BIN_FLASH_ADDR), )
else
	@echo '#define CONFIG_SCPU_BIN_FLASH_ADDR $(SCPU_BIN_FLASH_ADDR)' >> $@
endif
endif

ifeq ($(ENABLE_CTR_CALLBACK), y)
	@echo "#define CONFIG_ENABLE_CTR_CALLBACK" >> $@
endif

ifeq ($(ENABLE_LOGO), y)
	@echo "#define CONFIG_ENABLE_LOGO" >> $@
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


	@echo "" >> $@

	@echo "#define DRAMBASE                 $(DRAMBASE)" >> $@
	@echo "#define SRAMBASE                 $(SRAMBASE)" >> $@
	@echo "/* DRAM Memory space distribution */"                 >> $@
	@echo "#define KERNEL_START_ADDR        $(KERNEL_START_ADDR)" >> $@
	@echo "#define KERNEL_END_ADDR          $(KERNEL_END_ADDR)" >> $@
	@echo "#define OTA_DRAM_START_ADDR      $(OTA_DRAM_START_ADDR)"   >> $@
	@echo "#define OTA_DRAM_END_ADDR        $(OTA_DRAM_END_ADDR)"     >> $@
	@echo "#define HEAP_START_ADDR          $(HEAP_START_ADDR)"     >> $@
	@echo "#define HEAP_END_ADDR            $(HEAP_END_ADDR)"       >> $@
	@echo "#define STACK_BOTTOM_ADDR        $(STACK_BOTTOM_ADDR)"      >> $@
	@echo "#define STACK_TOP_ADDR           $(STACK_TOP_ADDR)"      >> $@
	@echo "#define LOADER_START_ADDR        $(LOADER_START_ADDR)"   >> $@
	@echo "#define LOADER_END_ADDR          $(LOADER_END_ADDR)"     >> $@
	@echo "#define SPP_BUF_START_ADDR       $(SPP_BUF_START_ADDR)"   >> $@
	@echo "#define SPP_BUF_END_ADDR         $(SPP_BUF_END_ADDR)"   >> $@
	@echo "#define SVPU_BUF_START_ADDR      $(SVPU_BUF_START_ADDR)"   >> $@
	@echo "#define SVPU_BUF_END_ADDR        $(SVPU_BUF_END_ADDR)"   >> $@
	@echo "#define OSD_BUF_START_ADDR       $(OSD_BUF_START_ADDR)"   >> $@
	@echo "#define OSD_BUF_END_ADDR         $(OSD_BUF_END_ADDR)"   >> $@

	@echo "/* SRAM Memory space distribution */"                 >> $@
	@echo "#define STAGE1_START_ADDR                $(STAGE1_START_ADDR)"       >> $@
	@echo "#define STAGE1_END_ADDR                  $(STAGE1_END_ADDR)"         >> $@
	@echo "#define GPIO_TABLE_START_ADDR            $(GPIO_TABLE_START_ADDR)"   >> $@
	@echo "#define GPIO_TABLE_END_ADDR              $(GPIO_TABLE_END_ADDR)"     >> $@
	@echo "#define STAGE1_STACK_BOTTOM_ADDR         $(STAGE1_STACK_BOTTOM_ADDR)">> $@
	@echo "#define STAGE1_STACK_TOP_ADDR            $(STAGE1_STACK_TOP_ADDR)"   >> $@
	@echo "#define STAGE1_STACK_TOP_ADDR_NOMMU      ($(STAGE1_STACK_TOP_ADDR) - 0x$(REG_ADDR_HEAD)0000000)"   >> $@

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
	@echo "#define KERNEL_SIZE                      $(KERNEL_SIZE)" >> $@
	@echo "#define HEAP_SIZE                        $(HEAP_SIZE)" >> $@
	@echo "#define STACK_SIZE                       $(STACK_SIZE)" >> $@
	@echo "#define LOGO_PIXEL_SIZE                  $(LOGO_PIXEL_SIZE)"   >> $@
	@echo "#define SPP_BUF_SIZE                     $(SPP_BUF_SIZE)" >> $@
	@echo "#define SVPU_BUF_SIZE                    $(SVPU_BUF_SIZE)" >> $@
	@echo "#define OSD_BUF_SIZE                     $(OSD_BUF_SIZE)" >> $@
	@echo "#define OTA_DRAM_SIZE                    $(OTA_DRAM_SIZE)" >> $@
	@echo "#define BOOTLOADER_STAGE2_SIZE           $(BOOTLOADER_STAGE2_SIZE)" >> $@
	@echo "#define BOOTLOADER_SIZE                  $(BOOTLOADER_SIZE)" >> $@
	@echo "#define BOOTLOADER_BIN_SIZE              $(BOOTLOADER_BIN_SIZE)" >> $@

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

	@echo "" >> $@

	@echo "/* NOTE: here GX_PAGETABLE_BASE is just one relative address */" >> $@
	@echo "#define GX_PAGETABLE_BASE            0x00030000" >> $@

	@echo "#define GX_REG_VIRTUAL_BASE1         0x$(REG_ADDR_HEAD)0000000" >> $@
	@echo "#define GX_REG_VIRTUAL_BASE2         0x$(REG_ADDR_HEAD)0000000" >> $@
	@echo "#define GX_DRAM_VIRTUAL_OFFSET       $(VIR_PHY_OFFSET)" >> $@

	@echo "" >> $@
	@echo "#define FLASH_TABLE_SEARCH_START_ADDR	$(FLASH_TABLE_SEARCH_START_ADDR)" >> $@
	@echo "#define FLASH_TABLE_SEARCH_SKIP_SIZE	$(FLASH_TABLE_SEARCH_SKIP_SIZE)" >> $@
	@echo "#define FLASH_TABLE_SEARCH_END_ADDR	($(FLASH_TABLE_SEARCH_START_ADDR) + $(FLASH_TABLE_SEARCH_SIZE))" >> $@
	@echo "" >> $@
ifneq ($(OTA_FORCE_FLASH_ADDR), )
	@echo "#define OTA_FORCE_FLASH_ADDR    $(OTA_FORCE_FLASH_ADDR)"       >> $@
endif
ifneq ($(OTA_FORCE_FLASH_SIZE), )
	@echo "#define OTA_FORCE_FLASH_SIZE    $(OTA_FORCE_FLASH_SIZE)"       >> $@
endif
	@echo "" >> $@

ifeq ($(ENABLE_THIRDLIB), y)
	@echo "#define CONFIG_ENABLE_THIRDLIB" >> $@
endif

ifeq ($(ENABLE_BOOT_TOOL), y)
	@echo "#define ENABLE_BOOT_TOOL" >> $@
endif

	@echo "" >> $@
ifeq ($(ENABLE_APP_DECRYPT), y)
	@echo "#define ENABLE_APP_DECRYPT" >> $@
endif
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@echo "#define ENABLE_SECURE_VERIFY" >> $@
	@echo "#define CONFIG_SECURE_VERIFY_TYPE_$(SECURE_VERIFY_TYPE)"    >> $@
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

	@echo "" >> $@

	@echo "#endif" >> $@

loader.bin : loader.elf
	@$(OBJCOPY) -Obinary $< $@
	@chmod a-x $@
	@mv $@ output/

ifeq ($(ENABLE_BOOT_TOOL), y)
STAGE1_COPY_COUNT = 1
STAGE1_DATA_START = 32
else
STAGE1_DATA_START = 4
ifeq ($(ENABLE_SPIFLASH), y)
STAGE1_COPY_COUNT = 1
else
STAGE1_COPY_COUNT = 5
endif
endif

ifeq ($(ENABLE_SECURE_VERIFY), y)
STAGE1_LEN = $(ROM_COPY_SIZE) - 396
else
STAGE1_LEN = $(ROM_COPY_SIZE) - 8
endif

STAGE1_LMA_ADDR = 0x00000000

ifeq ($(ENABLE_BOOT_TOOL), y)
	STAGE1_SIZE= $(shell printf 0x%08x $(shell ./script/calc.sh $(ROM_COPY_SIZE) + 28))
else
	STAGE1_SIZE=$(ROM_COPY_SIZE)
endif

STAGE2_LMA_ADDR = $(shell printf 0x%08x $(shell ./script/calc.sh $(STAGE1_SIZE) - $(STAGE1_DATA_START)))

ifeq ($(ENABLE_SPIFLASH), y)
	BOOTLOADER_FLASH_TYPE = sflash
else
	ifeq ($(ENABLE_SPINAND), y)
	BOOTLOADER_FLASH_TYPE = spinand
else
	BOOTLOADER_FLASH_TYPE = nand
endif
endif

ifneq ($(ADAPTIVE_FLASH), )
	BOOTLOADER_FLASH_TYPE = nand
endif

ifeq ($(ENABLE_BOOT_TOOL), y)
	BOOTLOADER_MAGIC='31610100'
	FILE_PREFIX="$(CHIP_CORE)-$(CHIP_BOARD)"
	FILE_POSTFIX="boot"

	# BOOT_HEAD
	BOOT_HEAD_CHIP='6131'
	BOOT_HEAD=$(shell ./script/boot_head.sh ${BOOT_HEAD_CHIP} ${ROM_SERIAL_BAUDRATE})

else
	FILE_PREFIX="loader"
	FILE_POSTFIX="bin"
ifeq ($(ENABLE_SPINAND), y)
	BOOTLOADER_MAGIC='BB55BB55'
else
	BOOTLOADER_MAGIC='AA55AA55'
endif
	BOOT_HEAD=$(BOOTLOADER_MAGIC)
endif

NORMAL_FILE_NAME=$(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE).$(FILE_POSTFIX)

ifeq ($(ENABLE_SECURE_VERIFY), y)
	SIGN_DIR=tools/secure_tool/digital_signature
	SIGN_LEVEL1_SECTION="level1"
	SIGN_LEVEL2_SECTION="level2"
	SIGN_LEVEL1_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL1_SECTION).$(FILE_POSTFIX)
	SIGN_LEVEL2_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL2_SECTION).$(FILE_POSTFIX)
ifneq ($(ENABLE_BOOT_TOOL), y)
ifeq ($(ENABLE_SPIFLASH), y)
	SIGN_TOTAL_SIZE=$(shell printf 0x%x $(shell ./script/calc.sh $(BOOTLOADER_BIN_SIZE)))
else
	SIGN_TOTAL_SIZE = $(shell printf 0x%x $(shell ./script/calc.sh $(BOOTLOADER_BIN_SIZE) - $(ROM_COPY_SIZE) \* 4))
endif
endif
endif

CUR_PWD := $(shell pwd)

ifeq ($(ENABLE_BOOT_TOOL), y)
INSERT_NUM=1024
RESERVE_ROOM1='0000000000000000000000000000000000000000000000000000000000000000'
endif

#.data lma addr must be 0x60001000-4
loader-flash.bin: loader.elf
	@-rm -rf output/*
	@$(OBJCOPY) -Obinary $< $@ $(OBJCOPY_FLAG)
	@xxd -c4 -p $@ > TEMP_FILE1
	@sed -e '1i$(BOOT_HEAD)' TEMP_FILE1 > TEMP_FILE2
	@xxd -r -c4 -p TEMP_FILE2 > $@
	@rm TEMP_FILE*
ifeq ($(ENABLE_BOOT_TOOL), y)
	@xxd -c4 -p $@ > TEMP_FILE1
	@sed -e '$(INSERT_NUM)i$(RESERVE_ROOM1)' TEMP_FILE1 > TEMP_FILE3
	@xxd -r -c4 -p TEMP_FILE3 > $@
	@rm TEMP_FILE*
endif
	@chmod a-x $@
	@mv $@ $(NORMAL_FILE_NAME)
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
	@cp $(NORMAL_FILE_NAME) $(SIGN_DIR)/input/
ifneq ($(ENABLE_BOOT_TOOL), y)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx3113c 1 $(SIGN_TOTAL_SIZE);cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx3113c 2 $(SIGN_TOTAL_SIZE);cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_FILE_NAME)
else
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx3113c 1;cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx3113c 2;cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_FILE_NAME)
endif
	@rm $(SIGN_DIR)/input/$(NORMAL_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
	@mv $(SIGN_LEVEL1_FILE_NAME) output/$(SIGN_LEVEL1_FILE_NAME)
	@mv $(SIGN_LEVEL2_FILE_NAME) output/$(SIGN_LEVEL2_FILE_NAME)
endif
	@mv $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME)
ifneq ($(ENABLE_BOOT_TOOL), y)
ifneq ($(ENABLE_GDB_DEBUG), y)
	@./script/calc_bootload_bin.sh output/$(NORMAL_FILE_NAME) $(BOOTLOADER_BIN_SIZE)
endif
else
ifeq ($(ENABLE_GDB_DEBUG)$(ENABLE_DENALI_ELF), nn)
	@./script/insert_stage2_length.sh $(BOOTLOADER_FLASH_TYPE) $(ENABLE_BOOT_TOOL) $(CHIP_CORE)
endif
endif

ifeq ($(ENABLE_ECOS_OTA), y)
include ./script/mkbootlib.mk
endif

libprenos.a: $(COBJS-y) $(SOBJS-y)
	@$(AR) -rcs $@ $^
	@rm -f $(COBJS-y) $(SOBJS-y)

MY_DIR_LIBS += ./libprenos.a

libnos.a: libprenos.a
	$(AR_LIB)
	@mv $@ output/
	@rm -f $^

TARGET_LD = ate/target.ld
target_ld:
	@sed -i "s/\(.*sdram.*ORIGIN =\).*,/\1 $(OTA_START_ADDR_TO_LD),/g" $(TARGET_LD)
	@sed -i "s/\(.*sdram.*LENGTH =\).*/\1 $(OTA_SIZE_TO_LD)/g" $(TARGET_LD)

ifeq ($(ENABLE_GDB_DEBUG), y)
LD_FILE_NAME=gxloader_gdb.lds
OBJCOPY_FLAG=--change-section-lma .data=$(STAGE2_LMA_ADDR)
else
ifeq ($(ENABLE_DENALI_ELF), y)
LD_FILE_NAME=gxloader_denali.lds
OBJCOPY_FLAG=--change-section-lma .text=$(STAGE1_LMA_ADDR)
else
LD_FILE_NAME=gxloader.lds
OBJCOPY_FLAG=--change-section-lma .text=$(STAGE1_LMA_ADDR) --change-section-lma .data=$(STAGE2_LMA_ADDR)
endif
endif
LD_FILE=cpu/ck/$(CHIP_CORE)/$(LD_FILE_NAME)

loader.elf: $(COBJS-y) $(SOBJS-y) $(EXTRA-OBJS)
	@-rm -f $(LD_FILE_NAME)
	@$(CC) $(CPPFLAGS) -E -x assembler-with-cpp -P -o $(LD_FILE_NAME) $(LD_FILE)
ifeq ($(ENABLE_DENALI_ELF), y)
	@$(LD) $(LDFLAGS) -T$(LD_FILE_NAME) -o $@ $^ $(LIBS) $(LIBGCC)
	@$(OBJCOPY) -R .data $@
	@rm -f $(COBJS-y) $(SOBJS-y)
else
	@sed -i "s/\(.*stage1.*LENGTH =\).*/\1 $(STAGE1_LEN)/g" $(LD_FILE_NAME)
	@sed -i "s/\(.*stage2.*ORIGIN =\).*,/\1 $(LOADER_START_ADDR),/g" $(LD_FILE_NAME)
	@sed -i "s/\(.*stage2.*LENGTH =\).*/\1 $(BOOTLOADER_STAGE2_SIZE)/g" $(LD_FILE_NAME)
	@sed -i "s/\(.*sdram.*ORIGIN =\).*,/\1 $(LOADER_START_ADDR),/g" $(LD_FILE_NAME)
	@sed -i "s/\(.*sdram.*LENGTH =\).*/\1 $(BOOTLOADER_STAGE2_SIZE)/g" $(LD_FILE_NAME)
	@$(LD) $(LDFLAGS) -T$(LD_FILE_NAME) -o $@ $^ $(LIBS) $(LIBGCC)
	@rm -f $(COBJS-y) $(SOBJS-y)
endif

$(COBJS-y) : include/config.h Makefile
$(SOBJS-y) : include/config.h Makefile

