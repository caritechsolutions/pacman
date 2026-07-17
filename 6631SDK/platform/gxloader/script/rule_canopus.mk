#/*
# * =====================================================================================
# *
# *       Filename:  rule_canopus.mk
# *
# *       Compiler:  gcc
# *       Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
# *
# * =====================================================================================
# */

CONFIG_SPI_TYPE = DW
CONFIG_DWSPI_V2 = y
CONFIG_PADMUX = y
CONFIG_GPIO_SET_NUM = 4
CONFIG_HDMI_PHY_V200 = y
ENABLE_HARDWARE_HASH = y
CONFIG_SIRIUS_OTP = y
CONFIG_STAGE1_USE_ACPU_CRYPTO = y
CONFIG_STAGE1_VERIFY_USE_RSA = y
CONFIG_RCC_GX_V2 = y
CONFIG_TIME_GX_V2 = y
CONFIG_GPIO_GX_V2 = y
ENABLE_LOWPOWER = y

ROM_COPY_SIZE	            = 0x4000
SRAM_RESERVE_SIZE = 0x400 # Storage security information
SRAM_LOWPOWER_USED_SIZE = 0x0

##################### DDR config start #########################################
DDR_FREQUENCY_400M=400000000
DDR_FREQUENCY_533M=533000000
DDR_FREQUENCY_667M=667000000
DDR_FREQUENCY_800M=800000000
DDR_FREQUENCY_933M=933000000

ifeq ($(shell echo "$(DDR_FREQUENCY) <= $(DDR_FREQUENCY_400M)" | bc), 1)
	DDR_PARAM_FREQUENCY = $(DDR_FREQUENCY_400M)
else
ifeq ($(shell echo "$(DDR_FREQUENCY) <= $(DDR_FREQUENCY_533M)" | bc), 1)
	DDR_PARAM_FREQUENCY = $(DDR_FREQUENCY_533M)
else
ifeq ($(shell echo "$(DDR_FREQUENCY) <= $(DDR_FREQUENCY_667M)" | bc), 1)
	DDR_PARAM_FREQUENCY = $(DDR_FREQUENCY_667M)
else
ifeq ($(shell echo "$(DDR_FREQUENCY) <= $(DDR_FREQUENCY_800M)" | bc), 1)
	DDR_PARAM_FREQUENCY = $(DDR_FREQUENCY_800M)
else
ifeq ($(shell echo "$(DDR_FREQUENCY) <= $(DDR_FREQUENCY_933M)" | bc), 1)
	DDR_PARAM_FREQUENCY = $(DDR_FREQUENCY_933M)
endif
endif
endif
endif
endif

CHIP_PACKAGE_LIST = QFN88 LQFP156 BGA336 LQFP128 LQFP176
ifeq ($(findstring $(CONFIG_CHIP_PACKAGE), $(CHIP_PACKAGE_LIST)), )
$(error chip package has no $(CONFIG_CHIP_PACKAGE) )
endif

DDR_TYPE_CH = $(shell echo $(DDR_TYPE) | tr '[A-Z]' '[a-z]')
DDR_FREQ = $(shell expr $(DDR_PARAM_FREQUENCY) / 1000000)"MHz"
DDR_CONFIG = $(DDR_SIZE)"MB"
CHIP_PACKAGE = $(shell echo $(CONFIG_CHIP_PACKAGE) | tr '[A-Z]' '[a-z]')

ifeq ($(DDR_SIP), y)
ifeq ($(CONFIG_DDR_SOFT_TRAINING), y)
CONFIG_DDR_SOFT_TRAINING = n
$(error The DDR_SIP chip is not allowed to enable the ddr soft training function)
endif
CPPFLAGS += -I ./board/$(CHIP_CORE)/board-generic/$(DDR_TYPE_CH)-sip
DDR_PAR_INCLUDE = "$(DDR_TYPE_CH)_$(DDR_FREQ)_$(DDR_CONFIG)_sip_$(CHIP_PACKAGE).h"
ifeq ($(CHIP_BOARD), 6631SHXD)
CPPFLAGS += -I ./board/$(CHIP_CORE)/board-generic/ddr3-sip
DDR_SEC_PAR_INCLUDE = "ddr3_800MHz_128MB_sip_qfn88.h"
DDR_THR_PAR_INCLUDE = "ddr3_800MHz_256MB_sip_qfn88.h"
endif
else
CPPFLAGS += -I ./board/$(CHIP_CORE)/board-generic/$(DDR_TYPE_CH)-ext
DDR_PAR_INCLUDE = "$(DDR_TYPE_CH)_$(DDR_FREQ)_$(DDR_CONFIG)_ext_$(CHIP_PACKAGE).h"
endif
##################### DDR config end #########################################

include script/rule.mk

loader.bin : loader.elf
	@$(OBJCOPY) -Obinary $< $@
	@chmod a-x $@
	@mv $@ output/

STAGE1_COPY_COUNT = 1

ifneq ($(ENABLE_SECURE_VERIFY), y)
ENABLE_SECURE_VERIFY = n
endif

ifneq ($(ENABLE_DATA_ENCRYPT), y)
ENABLE_DATA_ENCRYPT = n
endif

ifeq ($(ENABLE_SECURE_VERIFY)$(ENABLE_DATA_ENCRYPT), nn)
STAGE1_LEN = $(ROM_COPY_SIZE) - 32
else
STAGE1_LEN = $(ROM_COPY_SIZE) - 848
endif

MAGIC_NUM = 4
STAGE1_DATA_START = 32
STAGE1_LMA_ADDR = 0x00000000
STAGE2_LMA_ADDR = $(shell printf 0x%08x $(shell ./script/calc.sh $(ROM_COPY_SIZE) - $(STAGE1_DATA_START)))
RESERVE_ROOM='00000000000000000000000000000000000000000000000000000000' #STAGE1_DATA_START - MAGIC_NUM

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
	BOOTLOADER_MAGIC='12660100'
	FILE_PREFIX="$(CHIP_CORE)-$(CHIP_BOARD)"
	FILE_POSTFIX="boot"

	# BOOT_HEAD
	BOOT_HEAD_CHIP='6612'
	BOOT_HEAD=$(shell ./script/boot_head.sh ${BOOT_HEAD_CHIP} ${ROM_SERIAL_BAUDRATE})

else
	FILE_PREFIX="loader"
	FILE_POSTFIX="bin"
ifeq ($(ENABLE_SPIFLASH), y)
	BOOTLOADER_MAGIC='AA55AA55'
else
	BOOTLOADER_MAGIC='BB55BB55'
endif
	BOOT_HEAD=$(BOOTLOADER_MAGIC)$(RESERVE_ROOM)
endif

NORMAL_FILE_NAME=$(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE).$(FILE_POSTFIX)
NORMAL_FILE_4K_NAME=$(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE)-4K.$(FILE_POSTFIX)
NORMAL_FILE_NAME_NOECC = $(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE)-noecc.$(FILE_POSTFIX)

ifeq ($(ENABLE_SECURE_VERIFY), y)
	SIGN_DIR=tools/secure_tool/digital_signature
	SIGN_LEVEL1_SECTION="level1"
	SIGN_LEVEL2_SECTION="level2"
	SIGN_LEVEL1_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL1_SECTION).$(FILE_POSTFIX)
	SIGN_LEVEL1_FILE_4K_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL1_SECTION)-4K.$(FILE_POSTFIX)
	SIGN_LEVEL2_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL2_SECTION).$(FILE_POSTFIX)
	SIGN_LEVEL2_FILE_4K_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL2_SECTION)-4K.$(FILE_POSTFIX)
endif

ifeq ($(ENABLE_DATA_ENCRYPT), y)
	ENCRYPT_SECTION="encrypt"
	ENCRYPT_DIR=tools/secure_tool/data_encryption
	NORMAL_ENCRYPT_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(ENCRYPT_SECTION).$(FILE_POSTFIX)
	NORMAL_ENCRYPT_FILE_4K_NAME=$(basename $(NORMAL_FILE_NAME))-$(ENCRYPT_SECTION)-4K.$(FILE_POSTFIX)
ifeq ($(ENABLE_SECURE_VERIFY), y)
	SIGN_LEVEL1_ENCRYPT_FILE_NAME=$(basename $(SIGN_LEVEL1_FILE_NAME))-$(ENCRYPT_SECTION).$(FILE_POSTFIX)
	SIGN_LEVEL1_ENCRYPT_FILE_4K_NAME=$(basename $(SIGN_LEVEL1_FILE_NAME))-$(ENCRYPT_SECTION)-4K.$(FILE_POSTFIX)
	SIGN_LEVEL2_ENCRYPT_FILE_NAME=$(basename $(SIGN_LEVEL2_FILE_NAME))-$(ENCRYPT_SECTION).$(FILE_POSTFIX)
	SIGN_LEVEL2_ENCRYPT_FILE_4K_NAME=$(basename $(SIGN_LEVEL2_FILE_NAME))-$(ENCRYPT_SECTION)-4K.$(FILE_POSTFIX)
endif
endif

ifeq ($(ENABLE_IRDETO_SOLUTION_V1), y)
	PRIVATE_HANDLE_SECTION="irdeto_solution_v1"
else
	PRIVATE_HANDLE_SECTION=
endif

CUR_PWD := $(shell pwd)

INSERT_NUM=4096
RESERVE_ROOM1='0000000000000000000000000000000000000000000000000000000000000000'

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
	@xxd -c4 -p $@ > TEMP_FILE1
	@sed -e '$(INSERT_NUM)i$(RESERVE_ROOM1)' TEMP_FILE1 > TEMP_FILE3
	@xxd -r -c4 -p TEMP_FILE3 > $@
	@rm TEMP_FILE*
	@./tools/private_handle.sh $@ $(PRIVATE_HANDLE_SECTION)
	@chmod a-x $@
	@mv $@ $(NORMAL_FILE_NAME)
ifeq ($(ENABLE_SPIFLASH), y)
	@gcc tools/loader_write.c -o loader_write
else
ifeq ($(ENABLE_BOOT_TOOL), y)
	@gcc tools/loader_write.c -o loader_write
else
	@gcc tools/nand_ecc_cal.c -o loader_nand_ecc
endif
endif
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
	@cp $(NORMAL_FILE_NAME) $(SIGN_DIR)/input/
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) sirius 1;cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) sirius 2;cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_FILE_NAME)
ifneq ($(ENABLE_BOOT_TOOL), y)
	@mv $(SIGN_DIR)/output/loader_level1_stage1_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level1_stage2_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level2_stage1_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level2_stage2_code.bin output/
	@mv $(SIGN_DIR)/output/market_id.bin output/
endif
	@rm $(SIGN_DIR)/input/$(NORMAL_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
endif
ifeq ($(ENABLE_DENALI_ELF), y)
	@cp $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME)
else
ifneq ($(ENABLE_SPIFLASH)$(ENABLE_BOOT_TOOL), nn)
	@./loader_write $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
else
	@cp -f $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME_NOECC)
	@./loader_nand_ecc -i $(NORMAL_FILE_NAME) -o output/$(NORMAL_FILE_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i $(NORMAL_FILE_NAME) -o output/$(NORMAL_FILE_4K_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
endif
endif
ifneq ($(ENABLE_DATA_ENCRYPT), y)
ifeq ($(ENABLE_SECURE_VERIFY), y)
ifneq ($(ENABLE_SPIFLASH)$(ENABLE_BOOT_TOOL), nn)
	@./loader_write $(SIGN_LEVEL1_FILE_NAME) output/$(SIGN_LEVEL1_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@./loader_write $(SIGN_LEVEL2_FILE_NAME) output/$(SIGN_LEVEL2_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
else
	@./loader_nand_ecc -i $(SIGN_LEVEL1_FILE_NAME) -o output/$(SIGN_LEVEL1_FILE_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i $(SIGN_LEVEL1_FILE_NAME) -o output/$(SIGN_LEVEL1_FILE_4K_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
	@./loader_nand_ecc -i $(SIGN_LEVEL2_FILE_NAME) -o output/$(SIGN_LEVEL2_FILE_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i $(SIGN_LEVEL2_FILE_NAME) -o output/$(SIGN_LEVEL2_FILE_4K_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
endif
	@rm $(SIGN_LEVEL1_FILE_NAME)
	@rm $(SIGN_LEVEL2_FILE_NAME)
endif
else
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@cp $(NORMAL_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(NORMAL_FILE_NAME) $(NORMAL_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) sirius $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) $(STAGE1_ENCRYPT_DERIVE) $(STAGE2_ENCRYPT_DERIVE)
ifneq ($(ENABLE_SPIFLASH)$(ENABLE_BOOT_TOOL), nn)
	@./loader_write $(ENCRYPT_DIR)/output/$(NORMAL_ENCRYPT_FILE_NAME) output/$(NORMAL_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
else
	@./loader_nand_ecc -i $(ENCRYPT_DIR)/output/$(NORMAL_ENCRYPT_FILE_NAME) -o output/$(NORMAL_ENCRYPT_FILE_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i $(ENCRYPT_DIR)/output/$(NORMAL_ENCRYPT_FILE_NAME) -o output/$(NORMAL_ENCRYPT_FILE_4K_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
endif
	@rm $(ENCRYPT_DIR)/input/$(NORMAL_FILE_NAME)
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@mv $(SIGN_LEVEL1_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) sirius $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) $(STAGE1_ENCRYPT_DERIVE) $(STAGE2_ENCRYPT_DERIVE)
ifneq ($(ENABLE_SPIFLASH)$(ENABLE_BOOT_TOOL), nn)
	@./loader_write $(ENCRYPT_DIR)/output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
else
	@./loader_nand_ecc -i $(ENCRYPT_DIR)/output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) -o output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i $(ENCRYPT_DIR)/output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) -o output/$(SIGN_LEVEL1_ENCRYPT_FILE_4K_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
endif
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@rm $(ENCRYPT_DIR)/input/$(SIGN_LEVEL1_FILE_NAME)
	@mv $(SIGN_LEVEL2_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) sirius $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) $(STAGE1_ENCRYPT_DERIVE) $(STAGE2_ENCRYPT_DERIVE)
ifneq ($(ENABLE_SPIFLASH)$(ENABLE_BOOT_TOOL), nn)
	@./loader_write $(ENCRYPT_DIR)/output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
else
	@./loader_nand_ecc -i $(ENCRYPT_DIR)/output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) -o output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i $(ENCRYPT_DIR)/output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) -o output/$(SIGN_LEVEL2_ENCRYPT_FILE_4K_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
endif
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@rm $(ENCRYPT_DIR)/input/$(SIGN_LEVEL2_FILE_NAME)
endif
endif
	@rm $(NORMAL_FILE_NAME)
	@rm -f loader_write
	@rm -f loader_nand_ecc
ifeq ($(ENABLE_GDB_DEBUG)$(ENABLE_DENALI_ELF), nn)
ifeq ($(ENABLE_BOOT_TOOL), y)
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
LD_FILE=cpu/arm/$(CHIP_CORE)/$(LD_FILE_NAME)

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

