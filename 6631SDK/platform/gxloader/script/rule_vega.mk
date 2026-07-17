#/*
# * =====================================================================================
# *
# *       Filename:  rule_vega.mk
# *
# *       Compiler:  gcc
# *       Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
# *
# * =====================================================================================
# */

CONFIG_SPI_TYPE = DW
CONFIG_DWSPI_V2 = y
# 暂时关闭 padmux, 等 vega padmux 驱动加好改成 y
CONFIG_PADMUX = y
CONFIG_GPIO_SET_NUM = 4
CONFIG_HDMI_PHY_V200 = y
ENABLE_HARDWARE_HASH = y
CONFIG_SIRIUS_OTP = y
CONFIG_RCC_GX_V2 = y
CONFIG_TIME_GX_V2 = y
CONFIG_GPIO_GX_V2 = y
ENABLE_LOWPOWER = y

ROM_COPY_SIZE = 0x4000
SRAM_RESERVE_SIZE = 0x400 # Storage security information
SRAM_LOWPOWER_USED_SIZE = 0x0

STAGE1_BIN_NAME := vega-nagra-generic-release-stage1.bin
ifneq ($(ENABLE_NAGRA_MODE), y)
STAGE1_BIN_NAME := $(subst nagra,nonnagra,$(STAGE1_BIN_NAME))
endif
ifeq ($(CONFIG_FPGA_BOARD), y)
STAGE1_BIN_NAME := $(subst generic,fpga,$(STAGE1_BIN_NAME))
endif
ifeq ($(ENABLE_BOOT_TOOL), y)
STAGE1_BIN_NAME := $(subst release,boot,$(STAGE1_BIN_NAME))
endif
ifeq ($(ENABLE_MEMORY_TEST), y)
STAGE1_BIN_NAME := vega-nonnagra-generic-memtest-stage1.bin
endif
ifeq ($(ENABLE_MEMORY_PHASE_TEST), y)
STAGE1_BIN_NAME := vega-nonnagra-generic-memphasetest-stage1.bin
endif
ifeq ($(ENABLE_BBT_SLT_TEST), y)
STAGE1_BIN_NAME := $(subst release,bbtslt,$(STAGE1_BIN_NAME))
endif

# All clocks of the Vega chip have been initialized in stage1.
# The memory phase test and memory test do not need to be
# executed again in stage2. The ENABLE_MEMORY_TEST and
# ENABLE_MEMORY_PHASE_TEST options are only used to
# select the stage1 bin of the memory phase test and memory test,
# and then force the option to turn off
ENABLE_MEMORY_TEST = n
ENABLE_MEMORY_PHASE_TEST = n

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

ifeq ($(DDR_PLL_USE_INNO), )
ifeq ($(CONFIG_CHIP_PACKAGE), QFN88)
DDR_PLL_USE_INNO = n
else
DDR_PLL_USE_INNO = y
endif
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
DDR_PAR_INCLUDE_DIR := board/$(CHIP_CORE)/board-generic/$(DDR_TYPE_CH)-sip
DDR_PAR_INCLUDE = "$(DDR_TYPE_CH)_$(DDR_FREQ)_$(DDR_CONFIG)_sip_$(CHIP_PACKAGE).h"
else
DDR_PAR_INCLUDE_DIR := board/$(CHIP_CORE)/board-generic/$(DDR_TYPE_CH)-ext
DDR_PAR_INCLUDE = "$(DDR_TYPE_CH)_$(DDR_FREQ)_$(DDR_CONFIG)_ext_$(CHIP_PACKAGE).h"
endif
##################### DDR config end #########################################

ifeq ($(DDR_SIP), y)
GDBINIT_FILE = board/$(CHIP_CORE)/board-generic/$(CHIP_BOARD)_gdbinit
else
GDBINIT_FILE = board/$(CHIP_CORE)/board-generic/$(CHIP_BOARD)_$(DDR_SIZE)MB_gdbinit
endif

EXTRA-COBJS-y += board/vega/param/vega_clock_param.o

include script/rule.mk

# Redefine LOADER_START_ADDR must come after include script/rule.mk
ifeq ($(ENABLE_BOOT_TOOL), y)
LOADER_START_ADDR := 0x8000
endif

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
CONTROL_DATA_RESEVE='00000000000000000000000000000000'

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
NORMAL_FILE_NAME_NOECC = $(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE)-noecc.$(FILE_POSTFIX)
JBOOT_FILE_NAME=$(FILE_PREFIX)-$(BOOTLOADER_FLASH_TYPE).jboot

ifeq ($(ENABLE_NAGRA_MODE), y)
	UNCHECKED_AREA_SIZE = 4096
else
	UNCHECKED_AREA_SIZE = 0
endif
STAGE2_START := $(shell printf %d $(shell ./script/calc.sh $(ROM_COPY_SIZE) + $(UNCHECKED_AREA_SIZE)))

ifeq ($(ENABLE_SECURE_VERIFY), y)
	SIGN_DIR=tools/secure_tool/digital_signature
	SIGN_LEVEL1_SECTION="level1"
	SIGN_LEVEL2_SECTION="level2"
	SIGN_LEVEL2_FILE_NAME_TEMP=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL2_SECTION)_TEMP.$(FILE_POSTFIX)
	SIGN_LEVEL2_FILE_4K_NAME_TEMP=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL2_SECTION)-4K_TEMP.$(FILE_POSTFIX)
	SIGN_LEVEL2_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL2_SECTION).$(FILE_POSTFIX)
	SIGN_LEVEL2_FILE_4K_NAME=$(basename $(NORMAL_FILE_NAME))-$(SIGN_LEVEL2_SECTION)-4K.$(FILE_POSTFIX)
endif

ifeq ($(ENABLE_DATA_ENCRYPT), y)
	ENCRYPT_SECTION="encrypt"
	ENCRYPT_DIR=tools/secure_tool/data_encryption
	NORMAL_ENCRYPT_FILE_NAME=$(basename $(NORMAL_FILE_NAME))-$(ENCRYPT_SECTION).$(FILE_POSTFIX)
	NORMAL_ENCRYPT_FILE_4K_NAME=$(basename $(NORMAL_FILE_NAME))-$(ENCRYPT_SECTION)-4K.$(FILE_POSTFIX)
ifeq ($(ENABLE_SECURE_VERIFY), y)
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

INSERT_NUM=6144
RESERVE_ROOM1='0000000000000000000000000000000000000000000000000000000000000000'

BOOTLOADER_PAD_TO_SIZE = $(BOOTLOADER_BIN_SIZE)

param.bin :
	@rm -rf tools/generate_param_tool/output/
	@mkdir -p tools/generate_param_tool/output
	@cp board/vega/board-$(CHIP_BOARD)/param/board-param.c tools/generate_param_tool/output
	@cp board/vega/param/*.c tools/generate_param_tool/output
	@cp $(DDR_PAR_INCLUDE_DIR)/$(DDR_PAR_INCLUDE) tools/generate_param_tool/output
	@cp include/version_autogenerated.h tools/generate_param_tool/output
	@cp include/cpu/vega/vega_param.h tools/generate_param_tool/output
ifeq ($(ENABLE_NAGRA_MODE), y)
	@cd tools/generate_param_tool; \
	make; \
	./generate_param -c vega;rm generate_param
else
ifeq ($(CONFIG_REE_LOADER), y)
	@stage2_size=`ls -l loader-stage2.bin | awk '{print $$5}'`; \
	cd tools/generate_param_tool; \
	make; \
	./generate_param -c vega --stage2_size $$stage2_size; \
	rm generate_param
else
	@stage2_size=`ls -l loader-stage2.bin | awk '{print $$5}'`; \
	cd tools/generate_param_tool; \
	make; \
	./generate_param -c vega --stage2_size $$stage2_size; \
	rm generate_param
endif
endif
	@cd $(CUR_PWD)


ifeq ($(CONFIG_REE_LOADER), y)
tee/tee_loader.bin:
	@if [ ! -f $@ ]; then \
		echo "Error: $@ does not exist.You should prepare $@ before compiling"; \
		exit 1; \
	fi

$(TEE_LOADER_BIN) : tee/tee_loader.bin
	@cp $< $@

tee/tee_param.bin:
	@if [ ! -f $@ ]; then \
		echo "Error: $@ does not exist.You should prepare $@ before compiling"; \
		exit 1; \
	fi

tee_param.bin : tee/tee_param.bin
	@cp $< $@
else
tee_param.bin :
	@dd if=/dev/zero of=$@ bs=1 skip=0 count=1028 2>/dev/null
endif

loader-stage2.bin :  loader.elf
	@$(OBJCOPY) -Obinary -R .bss $< $@ $(OBJCOPY_FLAG)
	@dd if=/dev/zero of=pad.bin bs=1 skip=0  count=272 2>/dev/null
	@cat pad.bin >> $@
	@rm pad.bin

loader-flash.bin : loader-stage2.bin param.bin $(TEE_LOADER_BIN) tee_param.bin
	@-rm -rf output/*
	@mkdir -p output
	@script/preprocess_vega_stage1.sh cpu/arm/vega/stage1/$(STAGE1_BIN_NAME) $@
	@xxd -c4 -p $@ > TEMP_FILE1
	@sed -e '1i$(BOOT_HEAD)$(CONTROL_DATA_RESEVE)' TEMP_FILE1 > TEMP_FILE3
	@xxd -r -c4 -p TEMP_FILE3 > $@
	@dd if=/dev/zero of=auxcode_ree_and_tee_sign.bin bs=1 skip=0  count=512 2>/dev/null
	@dd if=/dev/zero of=SCS_NOCS_Certificate.bin bs=1 skip=0 count=580 2>/dev/null
	@dd if=/dev/zero of=key_and_flag.bin bs=1 skip=0 count=20 2>/dev/null
	@dd if=/dev/zero of=control_data.bin bs=1 skip=0 count=16 2>/dev/null
	@dd if=/dev/zero of=sign.bin bs=1 skip=0 count=256 2>/dev/null
	@dd if=/dev/zero of=TEE_Certificate.bin bs=1 skip=0 count=580 2>/dev/null
	@dd if=/dev/zero of=crc32.bin bs=1 skip=0 count=4 2>/dev/null
	@cat control_data.bin >> $@
	@cat auxcode_ree_and_tee_sign.bin >> $@
	@cat SCS_NOCS_Certificate.bin >> $@
	@cat key_and_flag.bin >> $@
	@cat tools/generate_param_tool/output/spl_boot_param.bin >> $@
	@cat tools/generate_param_tool/output/spl_clock_param.bin >> $@
	@cat tools/generate_param_tool/output/spl_dram_param.bin >> $@
	@cat tools/generate_param_tool/output/spl_flash_param.bin >> $@
	@cat tools/generate_param_tool/output/ree_security_cfg.bin >> $@
	@cat control_data.bin >> $@
	@cat sign.bin >> $@
	@cat TEE_Certificate.bin >> $@
	@cat key_and_flag.bin >> $@
	@cat tee_param.bin >> $@
	@cat control_data.bin >> $@
	@cat sign.bin >> $@
	@cat crc32.bin >> $@
	@rm tee_param.bin
	@rm -rf tools/generate_param_tool/output
	@rm SCS_NOCS_Certificate.bin
	@rm key_and_flag.bin
	@rm control_data.bin
	@rm sign.bin
	@rm TEE_Certificate.bin
	@rm crc32.bin
	@rm TEMP_FILE*
	@dd if=/dev/zero of=unchecked_area.bin bs=1 skip=0 count=$(UNCHECKED_AREA_SIZE) 2>/dev/null
	@cat unchecked_area.bin >> $@
	@cat loader-stage2.bin >> $@
	@rm loader-stage2.bin
ifeq ($(ENABLE_NAGRA_MODE), y)
	@./script/fill_bin.sh $@ $(BOOTLOADER_PAD_TO_SIZE)
endif
	@chmod a-x $@
	@mv $@ $(NORMAL_FILE_NAME)
ifeq ($(ENABLE_SPIFLASH), y)
	@gcc tools/loader_write.c -o loader_write
else
ifeq ($(ENABLE_BOOT_TOOL), y)
	@gcc tools/loader_write.c -o loader_write
else
	@gcc tools/nand_ecc_cal.c -o loader_nand_ecc
	@gcc tools/nand_ecc_cal_for_stage2.c -o loader_nand_ecc2
endif
endif
ifeq ($(ENABLE_SECURE_VERIFY), y)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
	@cp $(NORMAL_FILE_NAME) $(SIGN_DIR)/input/
ifeq ($(CONFIG_REE_LOADER), y)
	@cp $(TEE_LOADER_BIN) $(SIGN_DIR)/input/
endif
	@export ENABLE_DATA_ENCRYPT=$(ENABLE_DATA_ENCRYPT); \
	export STAGE1_ENCRYPT=$(STAGE1_ENCRYPT);export STAGE2_ENCRYPT=$(STAGE2_ENCRYPT) \
	export ENABLE_NAGRA_MODE=$(ENABLE_NAGRA_MODE);\
	export CONFIG_REE_LOADER=$(CONFIG_REE_LOADER); \
	cd $(SIGN_DIR);./digital_signature.sh 1 $(NORMAL_FILE_NAME) $(SECURE_VERIFY_TYPE) vega 2 0 0;cd $(CUR_PWD)
	@mv $(SIGN_DIR)/output/$(SIGN_LEVEL2_FILE_NAME) $(SIGN_LEVEL2_FILE_NAME)
ifneq ($(ENABLE_BOOT_TOOL), y)
	@mv $(SIGN_DIR)/output/SCS_NOCS_Certificate.bin output/
	@mv $(SIGN_DIR)/output/SCS_Params_Area.bin output/
	@mv $(SIGN_DIR)/output/SCS_Auxiliary_Code.bin output/
	@mv $(SIGN_DIR)/output/TEE_Certificate.bin output/
	@mv $(SIGN_DIR)/output/TEE_Params_Area.bin output/
endif
ifeq ($(CONFIG_REE_LOADER), y)
	@mv $(SIGN_DIR)/output/tee-loader-level2.bin output/
endif
	@rm $(SIGN_DIR)/input/$(NORMAL_FILE_NAME)
	@cd $(SIGN_DIR);./digital_signature.sh clean;cd $(CUR_PWD)
endif
ifeq ($(ENABLE_DENALI_ELF), y)
	@cp $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME)
else
ifeq ($(ENABLE_SPIFLASH), y)
	@./loader_write $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
else
ifeq ($(ENABLE_BOOT_TOOL), y)
	@./loader_write $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
else
	@dd if=$(NORMAL_FILE_NAME) of=output/loader-stage1-noecc.bin bs=1 skip=0 count=$(shell printf %d $(ROM_COPY_SIZE)) 2>/dev/null
	@dd if=$(NORMAL_FILE_NAME) of=output/loader-stage2-noecc.bin bs=1 skip=$(STAGE2_START) 2>/dev/null
	@cp -f $(NORMAL_FILE_NAME) output/$(NORMAL_FILE_NAME_NOECC)
	@./loader_nand_ecc -i output/loader-stage1-noecc.bin -o output/loader-stage1-ecc.bin -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i output/loader-stage1-noecc.bin -o output/loader-stage1-ecc-4K.bin -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
	@./loader_nand_ecc2 -i output/loader-stage2-noecc.bin -o output/loader-stage2-ecc.bin -s 0 -l 0 -c 1 -p 0x800
	@./loader_nand_ecc2 -i output/loader-stage2-noecc.bin -o output/loader-stage2-ecc-4K.bin -s 0 -l 0 -c 1 -p 0x1000
	@./script/fill_bin.sh output/loader-stage1-ecc.bin 0x20000
	@./script/fill_bin.sh output/loader-stage1-ecc-4K.bin 0x40000
	@cat output/loader-stage1-ecc.bin >  output/$(NORMAL_FILE_NAME)
	@cat output/loader-stage1-ecc.bin >> output/$(NORMAL_FILE_NAME)
	@cat output/loader-stage1-ecc.bin >> output/$(NORMAL_FILE_NAME)
ifeq ($(ENABLE_NAGRA_MODE), y)
	@cp unchecked_area.bin unchecked_area_128K.bin
	@./script/fill_bin.sh unchecked_area_128K.bin 0x20000
	@cat unchecked_area_128K.bin >> output/$(NORMAL_FILE_NAME)
	@rm unchecked_area_128K.bin
endif
	@cat output/loader-stage2-ecc.bin >> output/$(NORMAL_FILE_NAME)
	@cat output/loader-stage1-ecc-4K.bin >  output/$(NORMAL_FILE_4K_NAME)
	@cat output/loader-stage1-ecc-4K.bin >> output/$(NORMAL_FILE_4K_NAME)
	@cat output/loader-stage1-ecc-4K.bin >> output/$(NORMAL_FILE_4K_NAME)
	@ls -l output/$(NORMAL_FILE_4K_NAME)
	@ls -l output/$(NORMAL_FILE_NAME)
ifeq ($(ENABLE_NAGRA_MODE), y)
	@cp unchecked_area.bin unchecked_area_256K.bin
	@./script/fill_bin.sh unchecked_area_256K.bin 0x40000
	@cat unchecked_area_256K.bin >> output/$(NORMAL_FILE_4K_NAME)
	@rm unchecked_area_256K.bin
endif
	@cat output/loader-stage2-ecc-4K.bin >> output/$(NORMAL_FILE_4K_NAME)
	@rm output/loader-stage1-ecc.bin output/loader-stage2-ecc.bin
	@rm output/loader-stage1-ecc-4K.bin output/loader-stage2-ecc-4K.bin
	@rm output/loader-stage1-noecc.bin output/loader-stage2-noecc.bin
endif
endif
endif
ifeq ($(ENABLE_SECURE_VERIFY), y)
ifneq ($(ENABLE_BOOT_TOOL)$(ENABLE_SPIFLASH), nn)
	@./loader_write $(SIGN_LEVEL2_FILE_NAME) output/$(SIGN_LEVEL2_FILE_NAME) $(STAGE1_DATA_START) $(ROM_COPY_SIZE) $(STAGE1_COPY_COUNT)
	@rm $(SIGN_LEVEL2_FILE_NAME)
else
	@dd if=$(SIGN_LEVEL2_FILE_NAME) of=output/loader-stage1-level2-noecc.bin bs=1 count=$(shell printf %d $(ROM_COPY_SIZE)) 2>/dev/null
	@dd if=$(SIGN_LEVEL2_FILE_NAME) of=output/loader-stage2-level2-noecc.bin bs=1 skip=$(STAGE2_START) 2>/dev/null
	@./loader_nand_ecc -i output/loader-stage1-level2-noecc.bin -o output/loader-stage1-level2-ecc.bin -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x800
	@./loader_nand_ecc -i output/loader-stage1-level2-noecc.bin -o output/loader-stage1-level2-ecc-4K.bin -s $(STAGE1_DATA_START) -l $(ROM_COPY_SIZE) -c $(STAGE1_COPY_COUNT) -p 0x1000
	@./loader_nand_ecc2 -i output/loader-stage2-level2-noecc.bin -o output/loader-stage2-level2-ecc.bin -s 0 -l 0 -c 1 -p 0x800
	@./loader_nand_ecc2 -i output/loader-stage2-level2-noecc.bin -o output/loader-stage2-level2-ecc-4K.bin -s 0 -l 0 -c 1 -p 0x1000
	@./script/fill_bin.sh output/loader-stage1-level2-ecc.bin 0x20000
	@./script/fill_bin.sh output/loader-stage1-level2-ecc-4K.bin 0x40000
	@cat output/loader-stage1-level2-ecc.bin >  output/$(SIGN_LEVEL2_FILE_NAME)
	@cat output/loader-stage1-level2-ecc.bin >> output/$(SIGN_LEVEL2_FILE_NAME)
	@cat output/loader-stage1-level2-ecc.bin >> output/$(SIGN_LEVEL2_FILE_NAME)
ifeq ($(ENABLE_NAGRA_MODE), y)
	@cp unchecked_area.bin unchecked_area_128K.bin
	@./script/fill_bin.sh unchecked_area_128K.bin 0x20000
	@cat unchecked_area_128K.bin >> output/$(SIGN_LEVEL2_FILE_NAME)
	@rm unchecked_area_128K.bin
endif
	@cat output/loader-stage2-level2-ecc.bin >> output/$(SIGN_LEVEL2_FILE_NAME)
	@cat output/loader-stage1-level2-ecc-4K.bin >  output/$(SIGN_LEVEL2_FILE_4K_NAME)
	@cat output/loader-stage1-level2-ecc-4K.bin >> output/$(SIGN_LEVEL2_FILE_4K_NAME)
	@cat output/loader-stage1-level2-ecc-4K.bin >> output/$(SIGN_LEVEL2_FILE_4K_NAME)
ifeq ($(ENABLE_NAGRA_MODE), y)
	@cp unchecked_area.bin unchecked_area_256K.bin
	@./script/fill_bin.sh unchecked_area_256K.bin 0x40000
	@cat unchecked_area_256K.bin >> output/$(SIGN_LEVEL2_FILE_4K_NAME)
	@rm unchecked_area_256K.bin
endif
	@cat output/loader-stage2-level2-ecc-4K.bin >> output/$(SIGN_LEVEL2_FILE_4K_NAME)
	@rm output/loader-stage1-level2-ecc.bin output/loader-stage2-level2-ecc.bin
	@rm output/loader-stage1-level2-ecc-4K.bin output/loader-stage2-level2-ecc-4K.bin
	@rm output/loader-stage1-level2-noecc.bin output/loader-stage2-level2-noecc.bin
ifeq ($(CONFIG_REE_LOADER), y)
	@./loader_nand_ecc2 -i output/tee-loader-level2.bin -o output/tee-loader-level2-ecc.bin -s 0  -l 0 -c 1 -p 0x800
	@./loader_nand_ecc2 -i output/tee-loader-level2.bin -o output/tee-loader-level2-ecc-4K.bin -s 0  -l 0 -c 1 -p 0x1000
endif
endif
endif
	@rm unchecked_area.bin
	@rm $(NORMAL_FILE_NAME)
	@rm -f loader_write
	@rm -f loader_nand_ecc
ifeq ($(ENABLE_GDB_DEBUG)$(ENABLE_DENALI_ELF), nn)
#	@./script/insert_stage2_length.sh $(BOOTLOADER_FLASH_TYPE) $(ENABLE_BOOT_TOOL) $(CHIP_CORE)
endif

ifeq ($(ENABLE_ECOS_OTA), y)
include ./script/mkbootlib.mk
endif

libprenos.a: $(COBJS-y) $(SOBJS-y) $(EXTRA-COBJS-y)
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

LD_FILE_NAME=gxloader.lds
LD_FILE=cpu/arm/$(CHIP_CORE)/$(LD_FILE_NAME)

loader.elf: $(filter-out tee_main.o, $(COBJS-y)) $(filter-out cpu/arm/$(CHIP_CORE)/$(CHIP_CORE)_tee_start.o, $(SOBJS-y)) $(EXTRA-OBJS) $(EXTRA-COBJS-y)
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
ifneq ($(CONFIG_REE_LOADER), y)
	@rm -f $(COBJS-y) $(SOBJS-y)
	@rm -f $(stage1_COBJS-y) $(stage1_SOBJS-y)
endif
endif

$(COBJS-y) : include/config.h Makefile
$(SOBJS-y) : include/config.h Makefile
