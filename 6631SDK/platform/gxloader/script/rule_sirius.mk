#/*
# * =====================================================================================
# *
# *       Filename:  rule_sirius.mk
# *
# *       Compiler:  gcc
# *       Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
# *
# * =====================================================================================
# */

CONFIG_DWSPI_V1 = y
CONFIG_GPIO_SET_NUM = 3
CONFIG_HDMI_PHY_V100 = y
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

ifeq ($(CONFIG_BGA), y)
	CONFIG_CHIP_PACKAGE = BGA
else
ifeq ($(CONFIG_SIP), y)
	CONFIG_CHIP_PACKAGE = SIP
else
	CONFIG_CHIP_PACKAGE = LQFP
ifeq ($(CONFIG_LQFP_176), y)
	CONFIG_CHIP_PACKAGE = LQFP_176
endif
endif
endif

DDR_FREQUENCY_400M=400000000
DDR_FREQUENCY_533M=533000000
DDR_FREQUENCY_667M=667000000
DDR_FREQUENCY_800M=800000000

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
endif
endif
endif
endif

DDR_TYPE_CH = $(shell echo $(DDR_TYPE) | tr '[A-Z]' '[a-z]')
DDR_FREQ = $(shell expr $(DDR_PARAM_FREQUENCY) / 1000000)"MHz"
DDR_CONFIG = $(DDR_SIZE)"MB"


ifeq ($(CONFIG_SIP), y)
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_$(DDR_FREQ)_sip_$(DDR_CONFIG).h"
else
ifeq ($(CONFIG_BGA), y)
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_bga_$(DDR_FREQ)_$(DDR_CONFIG).h"
else
ifeq ($(CONFIG_LQFP_176), y)
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_lqfp_$(DDR_FREQ)_$(DDR_CONFIG).h"
else
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_$(DDR_FREQ)_$(DDR_CONFIG).h"
endif
endif
endif

include script/rule.mk

loader.bin : loader.elf
	@$(OBJCOPY) -Obinary $< $@
	@chmod a-x $@
	@mv $@ output/

ifeq ($(ENABLE_BOOT_TOOL), y)
STAGE1_COPY_COUNT = 1
else
ifeq ($(ENABLE_SPIFLASH), y)
STAGE1_COPY_COUNT = 1
else
STAGE1_COPY_COUNT = 5
endif
endif

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
ifeq ($(ENABLE_SPINAND), y)
	BOOTLOADER_MAGIC='BB55BB55'
else
	BOOTLOADER_MAGIC='AA55AA55'
endif
	BOOT_HEAD=$(BOOTLOADER_MAGIC)$(RESERVE_ROOM)
endif

NORMAL_FILE_NAME=$(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE).$(FILE_POSTFIX)
NORMAL_FILE_4K_NAME=$(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE)-4K.$(FILE_POSTFIX)

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
	@$(OBJCOPY) -Obinary -R .bss -R .mmu_tables $< $@ $(OBJCOPY_FLAG)
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
	@gcc tools/loader_write.c -o loader_write
	@gcc tools/nand_bin_pack.c -o nand_bin_pack
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
	@cp -f $(NORMAL_FILE_NAME) ori.bin
	@./loader_write $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@./nand_bin_pack -i $(NORMAL_FILE_NAME) -o output/$(NORMAL_FILE_4K_NAME) -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
endif
endif
ifneq ($(ENABLE_DATA_ENCRYPT), y)
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@./loader_write $(SIGN_LEVEL1_FILE_NAME) output/$(SIGN_LEVEL1_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@./loader_write $(SIGN_LEVEL2_FILE_NAME) output/$(SIGN_LEVEL2_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@rm $(SIGN_LEVEL1_FILE_NAME)
	@rm $(SIGN_LEVEL2_FILE_NAME)
endif
else
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@cp $(NORMAL_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(NORMAL_FILE_NAME) $(NORMAL_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) sirius $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) $(STAGE1_ENCRYPT_DERIVE) $(STAGE2_ENCRYPT_DERIVE)
	@./loader_write $(ENCRYPT_DIR)/output/$(NORMAL_ENCRYPT_FILE_NAME) output/$(NORMAL_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@rm $(ENCRYPT_DIR)/input/$(NORMAL_FILE_NAME)
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@mv $(SIGN_LEVEL1_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) sirius $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) $(STAGE1_ENCRYPT_DERIVE) $(STAGE2_ENCRYPT_DERIVE)
	@./loader_write $(ENCRYPT_DIR)/output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@rm $(ENCRYPT_DIR)/input/$(SIGN_LEVEL1_FILE_NAME)
	@mv $(SIGN_LEVEL2_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) sirius $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) $(STAGE1_ENCRYPT_DERIVE) $(STAGE2_ENCRYPT_DERIVE)
	@./loader_write $(ENCRYPT_DIR)/output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@rm $(ENCRYPT_DIR)/input/$(SIGN_LEVEL2_FILE_NAME)
endif
endif
ifneq ($(ENABLE_SECURE_VERIFY), y)
ifeq ($(CONFIG_CHECK_STAGE2_HASH), y)
	@dd if=output/$(NORMAL_FILE_NAME) of=stage2.bin skip=16416 bs=1 >/dev/null 2>&1
	@openssl dgst -binary -sha256 -out stage2_hash.bin stage2.bin
	@cat stage2_hash.bin >> output/$(NORMAL_FILE_NAME)
	@rm stage2.bin stage2_hash.bin
endif
endif
	@rm $(NORMAL_FILE_NAME)
	@rm -f loader_write
	@rm -f nand_bin_pack
ifeq ($(ENABLE_GDB_DEBUG)$(ENABLE_DENALI_ELF), nn)
	@./script/insert_stage2_length.sh $(BOOTLOADER_FLASH_TYPE) $(ENABLE_BOOT_TOOL) $(CHIP_CORE)
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

