#/*
# * =====================================================================================
# *
# *       Filename:  rule_CK_GX6605s.mk
# *
# *       Compiler:  gcc
# *       Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
# *
# * =====================================================================================
# */

CONFIG_DWSPI_V1 = y
CONFIG_GPIO_SET_NUM = 3
CONFIG_HDMI_PHY_V100 = y
CONFIG_GX3211_OTP = y
CONFIG_STAGE1_USE_MTC = y
ENABLE_SOFT_HASH = y
CONFIG_RCC_GX_V1 = y
CONFIG_TIME_GX_V1 = y
CONFIG_GPIO_GX_V1 = y
ENABLE_LOWPOWER = y

ROM_COPY_SIZE	            = 0x2000
SRAM_RESERVE_SIZE = 0x500 # Storage security information
SRAM_LOWPOWER_USED_SIZE = 0x80

ifeq ($(CONFIG_BGA), y)
	CONFIG_CHIP_PACKAGE = BGA
else
ifeq ($(CONFIG_SIP), y)
	CONFIG_CHIP_PACKAGE = SIP
else
	CONFIG_CHIP_PACKAGE = QFN
endif
endif

##################### DDR config start #########################################
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

DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_$(DDR_FREQ).h"

# gx6605s DDR_SIZE is always 64MB
DDR_SIZE = 64
##################### DDR config end #########################################

include script/rule.mk

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

ifeq ($(ENABLE_BOOT_TOOL), y)
	STAGE1_SIZE= $(shell printf 0x%08x $(shell ./script/calc.sh $(ROM_COPY_SIZE) + 28))
else
	STAGE1_SIZE=$(ROM_COPY_SIZE)
endif

STAGE1_LMA_ADDR = 0x00000000
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
	BOOTLOADER_MAGIC='11320100'
	FILE_PREFIX="$(CHIP_CORE)-$(CHIP_BOARD)"
	FILE_POSTFIX="boot"

	# BOOT_HEAD
	BOOT_HEAD_CHIP='3211'
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
INSERT_NUM=2048
RESERVE_ROOM1='0000000000000000000000000000000000000000000000000000000000000000'
endif

loader-flash.bin : loader.elf
	@-rm -rf output/*
	@$(OBJCOPY) -Obinary -R .bss $< $@ $(OBJCOPY_FLAG)
ifeq ($(CONFIG_STAGE2_COMPRESSED), y)
	$(compressed_stage2)
endif
	@xxd -c4 -p $@ > TEMP_FILE1
	@sed -e '1i$(BOOT_HEAD)' TEMP_FILE1 > TEMP_FILE3
	@xxd -r -c4 -p TEMP_FILE3 > $@
	@rm TEMP_FILE*
ifeq ($(ENABLE_BOOT_TOOL), y)
	@xxd -c4 -p $@ > TEMP_FILE1
	@sed -e '$(INSERT_NUM)i$(RESERVE_ROOM1)' TEMP_FILE1 > TEMP_FILE3
	@xxd -r -c4 -p TEMP_FILE3 > $@
	@rm TEMP_FILE*
endif
	@chmod a-x $@
	@mv $@ $(NORMAL_FILE_NAME)
	@gcc tools/loader_write.c -o loader_write
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
	@cp $(NORMAL_FILE_NAME) $(SIGN_DIR)/input/
ifneq ($(ENABLE_BOOT_TOOL), y)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx6605s 1 $(SIGN_TOTAL_SIZE);cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx6605s 2 $(SIGN_TOTAL_SIZE);cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_FILE_NAME)
	@mv $(SIGN_DIR)/output/loader_level1_stage1_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level1_stage2_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level2_stage1_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level2_stage2_code.bin output/
	@mv $(SIGN_DIR)/output/market_id.bin output/
else
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx6605s 1;cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) gx6605s 2;cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_FILE_NAME)
endif
	@rm $(SIGN_DIR)/input/$(NORMAL_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
endif
ifeq ($(ENABLE_DENALI_ELF), y)
	@cp $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME)
else
	@./loader_write $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
endif
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@./loader_write $(SIGN_LEVEL1_FILE_NAME) output/$(SIGN_LEVEL1_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
	@./loader_write $(SIGN_LEVEL2_FILE_NAME) output/$(SIGN_LEVEL2_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
	@rm $(SIGN_LEVEL1_FILE_NAME)
	@rm $(SIGN_LEVEL2_FILE_NAME)
endif
	@rm $(NORMAL_FILE_NAME)
	@rm loader_write
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

