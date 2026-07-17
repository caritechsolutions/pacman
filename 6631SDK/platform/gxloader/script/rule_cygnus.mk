#/*
# * =====================================================================================
# *
# *       Filename:  rule_CK_cygnus.mk
# *
# *       Compiler:  gcc
# *       Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
# *
# * =====================================================================================
# */

CONFIG_DWSPI_V2 = y
CONFIG_PADMUX = y
CONFIG_GPIO_SET_NUM = 3
CONFIG_HDMI_PHY_V200 = y
CONFIG_GX3211_OTP = y
CONFIG_STAGE1_USE_MTC = y
ENABLE_SOFT_HASH = y
CONFIG_RCC_GX_V1 = y
CONFIG_TIME_GX_V2 = y
CONFIG_GPIO_GX_V2 = y
ENABLE_LOWPOWER = y

ROM_COPY_SIZE	            = 0x2000
SRAM_RESERVE_SIZE = 0x500 # Storage security information
SRAM_LOWPOWER_USED_SIZE = 0x0

##################### DDR config start #########################################
DDR_FREQUENCY_400M=400000000
DDR_FREQUENCY_533M=533000000
DDR_FREQUENCY_667M=667000000
DDR_FREQUENCY_720M=720000000
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
ifeq ($(shell echo "$(DDR_FREQUENCY) <= $(DDR_FREQUENCY_720M)" | bc), 1)
	DDR_PARAM_FREQUENCY = $(DDR_FREQUENCY_720M)
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
endif

ifneq ($(DDR_SIZE), )
	COMMON_CFG := $(shell printf $(shell ./script/calc_compare.sh $(DDR_SIZE) 128))
ifeq ($(COMMON_CFG), 1)
	DDR_CONFIG =
else
	DDR_CONFIG = "_$(DDR_SIZE)MB"
endif
endif

DDR_TYPE_CH = $(shell echo $(DDR_TYPE) | tr '[A-Z]' '[a-z]')
DDR_FREQ = $(shell expr $(DDR_PARAM_FREQUENCY) / 1000000)MHz

ifeq ($(CONFIG_CHIP_PACKAGE), LQFP)
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_lqfp_$(DDR_FREQ)$(DDR_CONFIG).h"
	DDR_PAR_PATCH_INCLUDE =  $(DDR_TYPE_CH)_lqfp_$(DDR_FREQ)$(DDR_CONFIG)_patch.h
	DDR_PAR_PATCH_ARRAY = $(DDR_TYPE_CH)_lqfp_$(DDR_FREQ)$(DDR_CONFIG)_patch
else
	DDR_PAR_INCLUDE =  "$(DDR_TYPE_CH)_qfn_$(DDR_FREQ)$(DDR_CONFIG).h"
	DDR_PAR_PATCH_INCLUDE =  $(DDR_TYPE_CH)_qfn_$(DDR_FREQ)$(DDR_CONFIG)_patch.h
	DDR_PAR_PATCH_ARRAY = $(DDR_TYPE_CH)_qfn_$(DDR_FREQ)$(DDR_CONFIG)_patch
endif

ifeq ($(wildcard board/$(CHIP_CORE)/board-generic/$(DDR_PAR_PATCH_INCLUDE)), )
	DDR_PAR_PATCH_INCLUDE :=
endif

ifeq ($(CHIP_BOARD), Cygnus-X5)
	DDR_SEC_PAR_INCLUDE =  "$(DDR_TYPE_CH)_qfn_533MHz$(DDR_CONFIG).h"
	DDR_SEC_PAR_PATCH_INCLUDE =  $(DDR_TYPE_CH)_qfn_533MHz$(DDR_CONFIG)_patch.h
	DDR_SEC_PAR_PATCH_ARRAY = $(DDR_TYPE_CH)_qfn_533MHz$(DDR_CONFIG)_patch
endif

ifeq ($(CHIP_BOARD), 6706HX-BBT)
	DDR_SEC_PAR_INCLUDE = "ddr3_qfn_720MHz_128MB.h"
	DDR_SEC_PAR_PATCH_INCLUDE = ddr3_qfn_720MHz_128MB_patch.h
	DDR_SEC_PAR_PATCH_ARRAY = ddr3_qfn_720MHz_128MB_patch
endif

ifeq ($(wildcard board/$(CHIP_CORE)/board-generic/$(DDR_SEC_PAR_PATCH_INCLUDE)), )
	DDR_SEC_PAR_PATCH_INCLUDE :=
endif
##################### DDR config end #########################################

include script/rule.mk

CHIP_ID=0x6705

ifeq ($(ENABLE_EXTERN_PARAM), y)
extern_param.bin : Makefile
	@-rm -f extern_param.txt
	@-rm -f extern_param.bin
	@-rm -f fill.bin
	@printf %08x $(CHIP_ID) > extern_param.txt
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p > $@
	@printf %08x $(DEFAULT_EXT_CLOCK_XTAL) > extern_param.txt
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p >> $@
	@printf %08x 00000000 > extern_param.txt  #current the cpu frequency of all type of cygnus chip is same
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p >> $@
	@printf %08x $(STACK_TOP_ADDR) > extern_param.txt
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p >> $@
	@printf %08x $(LOADER_START_ADDR) > extern_param.txt
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p >> $@
ifeq ($(DDR_TYPE), DDR3)
	@printf %08x 3 > extern_param.txt
else
ifeq ($(DDR_TYPE), DDR2)
	@printf %08x 2 > extern_param.txt
else
ifeq ($(DDR_TYPE), DDR1)
	@printf %08x 1 > extern_param.txt
endif
endif
endif
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p >> $@
	@printf %08x $(DDR_FREQUENCY) > extern_param.txt
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p >> $@
	@./script/ddr_config_create.sh $(CHIP_CORE) $(DDR_PAR_INCLUDE) extern_param.txt
	@xxd -r -p extern_param.txt | od -v -An -t x1 -w4 | awk '{print $$4, $$3, $$2, $$1}' | xxd -r -p >> $@
	@dd if=/dev/zero of=fill.bin bs=1 count=$$((($(EXTERN_PARAM_FILE_SIZE) - $$(ls -l $@ | awk '{print $$5}'))))
	@cat fill.bin >> $@
	@rm fill.bin
	@rm extern_param.txt
endif

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
	BOOTLOADER_MAGIC='01670500'
	FILE_PREFIX="$(CHIP_CORE)-$(CHIP_BOARD)"
	FILE_POSTFIX="boot"

	# BOOT_HEAD
	BOOT_HEAD_CHIP='6705'
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
endif

ifeq ($(ENABLE_DATA_ENCRYPT), y)
	ENCRYPT_SECTION="encrypt"
	ENCRYPT_DIR=tools/secure_tool/data_encryption
	NORMAL_ENCRYPT_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(ENCRYPT_SECTION).$(FILE_POSTFIX)
ifeq ($(ENABLE_SECURE_VERIFY), y)
	SIGN_LEVEL1_ENCRYPT_FILE_NAME=$(basename $(SIGN_LEVEL1_FILE_NAME))-$(ENCRYPT_SECTION).$(FILE_POSTFIX)
	SIGN_LEVEL2_ENCRYPT_FILE_NAME=$(basename $(SIGN_LEVEL2_FILE_NAME))-$(ENCRYPT_SECTION).$(FILE_POSTFIX)
endif
endif


CUR_PWD := $(shell pwd)

INSERT_NUM=2048
RESERVE_ROOM1='0000000000000000000000000000000000000000000000000000000000000000'

ifeq ($(ENABLE_EXTERN_PARAM), y)
EXTERN_PARAM_ASCII=$(shell xxd -c4 -p extern_param.bin)
EXTERN_PARAM_INSERT_LINE=$(INSERT_NUM)
RESERVE_ROOM1_INSERT_LINE=$(shell echo $$(($(INSERT_NUM)+1)))
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
	@xxd -c4 -p $@ > TEMP_FILE1
ifeq ($(ENABLE_EXTERN_PARAM), y)
	@sed -e '$(INSERT_NUM)a$(EXTERN_PARAM_ASCII)' TEMP_FILE1 > TEMP_FILE3
	@sed -e '$(RESERVE_ROOM1_INSERT_LINE)a$(RESERVE_ROOM1)' TEMP_FILE3 > TEMP_FILE5
	@xxd -r -c4 -p TEMP_FILE5 > $@
	@-rm -f extern_param.bin
else
	@sed -e '$(INSERT_NUM)a$(RESERVE_ROOM1)' TEMP_FILE1 > TEMP_FILE3
	@xxd -r -c4 -p TEMP_FILE3 > $@
endif
	@rm TEMP_FILE*

	@chmod a-x $@
	@mv $@ $(NORMAL_FILE_NAME)
	@gcc tools/loader_write.c -o loader_write
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
	@cp $(NORMAL_FILE_NAME) $(SIGN_DIR)/input/
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) cygnus 1 $(ENABLE_EXTERN_PARAM) $(EXTERN_PARAM_FILE_SIZE);cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) cygnus 2 $(ENABLE_EXTERN_PARAM) $(EXTERN_PARAM_FILE_SIZE);cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_FILE_NAME)
ifneq ($(ENABLE_BOOT_TOOL), y)
	@mv $(SIGN_DIR)/output/loader_level1_stage1_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level1_stage2_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level2_stage1_code.bin output/
	@mv $(SIGN_DIR)/output/loader_level2_stage2_code.bin output/
ifeq ($(ENABLE_EXTERN_PARAM), y)
	@mv $(SIGN_DIR)/output/loader_level1_extern_param.bin output/
	@mv $(SIGN_DIR)/output/loader_level2_extern_param.bin output/
endif
	@mv $(SIGN_DIR)/output/market_id.bin output/
endif
	@rm $(SIGN_DIR)/input/$(NORMAL_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
endif
ifeq ($(ENABLE_DENALI_ELF), y)
	@cp $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME)
else
	@./loader_write $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
endif
ifneq ($(ENABLE_DATA_ENCRYPT), y)
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@./loader_write $(SIGN_LEVEL1_FILE_NAME) output/$(SIGN_LEVEL1_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
	@./loader_write $(SIGN_LEVEL2_FILE_NAME) output/$(SIGN_LEVEL2_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
	@rm $(SIGN_LEVEL1_FILE_NAME)
	@rm $(SIGN_LEVEL2_FILE_NAME)
endif
else
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@cp $(NORMAL_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(NORMAL_FILE_NAME) $(NORMAL_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) gemini $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) 0 0 0 $(ENABLE_EXTERN_PARAM) $(EXTERN_PARAM_FILE_SIZE)
	@./loader_write $(ENCRYPT_DIR)/output/$(NORMAL_ENCRYPT_FILE_NAME) output/$(NORMAL_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
	@rm $(ENCRYPT_DIR)/input/$(NORMAL_FILE_NAME)
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@mv $(SIGN_LEVEL1_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(SIGN_LEVEL1_FILE_NAME) $(SIGN_LEVEL1_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) gemini $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) 0 0 1 $(ENABLE_EXTERN_PARAM) $(EXTERN_PARAM_FILE_SIZE)
	@./loader_write $(ENCRYPT_DIR)/output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) output/$(SIGN_LEVEL1_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@rm $(ENCRYPT_DIR)/input/$(SIGN_LEVEL1_FILE_NAME)
	@mv $(SIGN_LEVEL2_FILE_NAME) $(ENCRYPT_DIR)/input/
	@./$(ENCRYPT_DIR)/data_encrypt.sh 1 $(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_ENCRYPT_FILE_NAME) $(DATA_ENCRYPT_TYPE) gemini $(STAGE1_ENCRYPT) $(STAGE2_ENCRYPT) 0 0 2 $(ENABLE_EXTERN_PARAM) $(EXTERN_PARAM_FILE_SIZE)
	@./loader_write $(ENCRYPT_DIR)/output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) output/$(SIGN_LEVEL2_ENCRYPT_FILE_NAME) $(STAGE1_DATA_START) $(STAGE1_SIZE) $(STAGE1_COPY_COUNT)
	@./$(ENCRYPT_DIR)/data_encrypt.sh clean
	@rm $(ENCRYPT_DIR)/input/$(SIGN_LEVEL2_FILE_NAME)
endif
endif
ifneq ($(ENABLE_SECURE_VERIFY), y)
ifeq ($(CONFIG_CHECK_STAGE2_HASH), y)
	@dd if=output/$(NORMAL_FILE_NAME) of=stage2.bin skip=8224 bs=1 >/dev/null 2>&1
	@openssl dgst -binary -sha256 -out stage2_hash.bin stage2.bin
	@cat stage2_hash.bin >> output/$(NORMAL_FILE_NAME)
	@rm stage2.bin stage2_hash.bin
endif
endif
	@rm $(NORMAL_FILE_NAME)
	@rm loader_write
ifeq ($(ENABLE_GDB_DEBUG)$(ENABLE_DENALI_ELF), nn)
	@./script/insert_stage2_length.sh $(BOOTLOADER_FLASH_TYPE) $(ENABLE_BOOT_TOOL) $(CHIP_CORE) $(ENABLE_EXTERN_PARAM) $(EXTERN_PARAM_FILE_SIZE)
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
LD_FILE=cpu/ck/cygnus/$(LD_FILE_NAME)

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

