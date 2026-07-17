#########################################################################
# File Name   : digital_signature.sh
# Author      :
# mail        :
# Created Time: 2018年03月16日 星期五 13时07分16秒
#########################################################################
#!/bin/bash

CHIP_LIST="gx3211 | gx6605s | gx3113c | sirius | taurus | gemini | cygnus | canopus | scorpio | scorpio_taurus | vega"
SECURE_LEVEL_LIST="1 | 2"
SIGN_ALGO_LIST="SM2 | RSA_PKCS | RSA_SMI"

# incoming parameters
export CHIP=
export SECURE_LEVEL=
export SIGN_ALGO=
export SIGN_FILE=
export BOOTLOADER=
# generic config
export SIGN_CUR_DIR=$(cd "$(dirname "$0")"; pwd)
export ENCRYPT_CONFIG_DIR=${SIGN_CUR_DIR}/../data_encryption/config
export MAGIC_NUM=4
export CRC_NUM=4
export ND_KEY_NUM=256
export SIGN_NUM=256
export MARK_ID_NUM=4
export LOADER_STAGE1_START=
export LOADER_STAGE2_START=
export LOADER_STAGE1_LEN=
export STAGE1_DATA_NUM=
export BOOTLOADER_SIZE=0x20000
export INPUT_DIR=${SIGN_CUR_DIR}/input
export OUTPUT_DIR=${SIGN_CUR_DIR}/output
export CHIP_CONFIG_DIR=${SIGN_CUR_DIR}/chip_config
export COMMON_CONFIG_DIR=${SIGN_CUR_DIR}/common_config
export BIN_OR_BOOT=
export REG_CFG_FILE=
export MARKET_ID_FILE=${COMMON_CONFIG_DIR}/market_id.txt
export VERSION_FILE=${COMMON_CONFIG_DIR}/version.txt
export DTE_Boot_Code_Area_market_id_FILE=${COMMON_CONFIG_DIR}/vega/DTE_Boot_Code_Area_market_id.txt
export DTE_Boot_Code_Area_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/DTE_Boot_Code_Area_market_id_mask.txt
export DTE_Boot_Code_Area_version_FILE=${COMMON_CONFIG_DIR}/vega/DTE_Boot_Code_Area_version.txt
export DTE_Boot_Code_Area_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/DTE_Boot_Code_Area_version_mask.txt
export SCS_Auxiliary_Code_Area_REE_market_id_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_REE_market_id.txt
export SCS_Auxiliary_Code_Area_REE_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_REE_market_id_mask.txt
export SCS_Auxiliary_Code_Area_REE_version_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_REE_version.txt
export SCS_Auxiliary_Code_Area_REE_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_REE_version_mask.txt
export SCS_Auxiliary_Code_Area_TEE_market_id_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_TEE_market_id.txt
export SCS_Auxiliary_Code_Area_TEE_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_TEE_market_id_mask.txt
export SCS_Auxiliary_Code_Area_TEE_version_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_TEE_version.txt
export SCS_Auxiliary_Code_Area_TEE_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Auxiliary_Code_Area_TEE_version_mask.txt
export SCS_NOCS_Certificate_market_id_FILE=${COMMON_CONFIG_DIR}/vega/SCS_NOCS_Certificate_market_id.txt
export SCS_NOCS_Certificate_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_NOCS_Certificate_market_id_mask.txt
export SCS_NOCS_Certificate_version_FILE=${COMMON_CONFIG_DIR}/vega/SCS_NOCS_Certificate_version.txt
export SCS_NOCS_Certificate_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_NOCS_Certificate_version_mask.txt
export SCS_Params_Area_market_id_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Params_Area_market_id.txt
export SCS_Params_Area_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Params_Area_market_id_mask.txt
export SCS_Params_Area_version_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Params_Area_version.txt
export SCS_Params_Area_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/SCS_Params_Area_version_mask.txt
export TEE_Certificate_market_id_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Certificate_market_id.txt
export TEE_Certificate_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Certificate_market_id_mask.txt
export TEE_Certificate_version_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Certificate_version.txt
export TEE_Certificate_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Certificate_version_mask.txt
export TEE_Loader_Area_market_id_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Loader_Area_market_id.txt
export TEE_Loader_Area_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Loader_Area_market_id_mask.txt
export TEE_Loader_Area_version_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Loader_Area_version.txt
export TEE_Loader_Area_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Loader_Area_version_mask.txt
export TEE_Params_Area_market_id_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Params_Area_market_id.txt
export TEE_Params_Area_market_id_mask_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Params_Area_market_id_mask.txt
export TEE_Params_Area_version_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Params_Area_version.txt
export TEE_Params_Area_version_mask_FILE=${COMMON_CONFIG_DIR}/vega/TEE_Params_Area_version_mask.txt
export KEY_RIGHT_FILE=${COMMON_CONFIG_DIR}/key_right.txt
export TEE_KEY_RIGHT_FILE=${COMMON_CONFIG_DIR}/tee_key_right.txt
export KEY_LADDER_CONSTANT1_FILE=${ENCRYPT_CONFIG_DIR}/key_ladder_constant1.txt
export KEY_LADDER_CONSTANT2_FILE=${ENCRYPT_CONFIG_DIR}/key_ladder_constant2.txt
export KEY_LADDER_CONSTANT3_FILE=${ENCRYPT_CONFIG_DIR}/key_ladder_constant3.txt
export TEE_KEY_LADDER_CONSTANT1_FILE=${ENCRYPT_CONFIG_DIR}/tee_key_ladder_constant1.txt
export TEE_KEY_LADDER_CONSTANT2_FILE=${ENCRYPT_CONFIG_DIR}/tee_key_ladder_constant2.txt
export TEE_KEY_LADDER_CONSTANT3_FILE=${ENCRYPT_CONFIG_DIR}/tee_key_ladder_constant3.txt
export LOADER_STAGE1_ENCRYPT_KEY_FILE="${ENCRYPT_CONFIG_DIR}/stage1_data_key.txt"
export LOADER_STAGE1_ENCRYPT_KEY="`cat ${ENCRYPT_CONFIG_DIR}/stage1_data_key.txt`"
export LOADER_STAGE2_ENCRYPT_KEY_FILE="${ENCRYPT_CONFIG_DIR}/stage2_data_key.txt"
export LOADER_STAGE2_ENCRYPT_KEY="`cat ${ENCRYPT_CONFIG_DIR}/stage2_data_key.txt`"
export TEE_LOADER_STAGE2_ENCRYPT_KEY_FILE="${ENCRYPT_CONFIG_DIR}/tee_stage2_data_key.txt"
export TEE_LOADER_STAGE2_ENCRYPT_KEY="`cat ${ENCRYPT_CONFIG_DIR}/tee_stage2_data_key.txt`"
export LOADER_STAGE2_ENCRYPT_DERIVE_KEY_FILE="${ENCRYPT_CONFIG_DIR}/stage2_derive_key.txt"
export LOADER_STAGE2_ENCRYPT_DERIVE_KEY="`cat ${ENCRYPT_CONFIG_DIR}/stage2_derive_key.txt`"
export TEE_LOADER_STAGE2_ENCRYPT_DERIVE_KEY_FILE="${ENCRYPT_CONFIG_DIR}/tee_stage2_derive_key.txt"
export TEE_LOADER_STAGE2_ENCRYPT_DERIVE_KEY="`cat ${ENCRYPT_CONFIG_DIR}/tee_stage2_derive_key.txt`"
export IV_VALUE="`cat ${ENCRYPT_CONFIG_DIR}/iv.txt`"
SM2_DIR=${SIGN_CUR_DIR}/sm2
RSA_DIR=${SIGN_CUR_DIR}/rsa
export COMMON_DIR=${SIGN_CUR_DIR}/../common

# config for gx3211/gx6605s
export REG_CFG_FILE_NUM=
# config for sirius
export LOADER_STAGE1_DECRYPT_KEY_START=16340  #0x3FD4
export LOADER_STAGE1_DECRYPT_KEY_LENGTH=16
export LOADER_STAGE1_DECRYPT_FLAG_START=16356 #0x3FE4
export LOADER_STAGE1_DECRYPT_FLAG_LENGTH=4
export ENABLE_EXTERN_PARAM=
export EXTERN_PARAM_FILE_SIZE=
export EXTERN_PARAM_FILE_COUNT=1
export EXTERN_PARAM_START=

VERSION_INFO=1.0.0

stage1_code_len_check()
{
	local len=0x
	local stage1_code_len=0

	if [ "${CHIP}" = "vega" ];then
		return
	fi
	len=0x$(readelf -S ../../../loader.elf | grep ".text" | awk -F ' ' '{print $7}')
	len=$(printf %d ${len})


	if [ "${CHIP}" = "gx3113c" ];then
		if [ "${SECURE_LEVEL}" = "2" ];then
			stage1_code_len=3188 #4096-908
		else
			stage1_code_len=3700 #4096-396
		fi
	elif [ "${CHIP}" = "gx6605s" ];then
		if [ "${SECURE_LEVEL}" = "2" ];then
			stage1_code_len=7284 #8192-908
		else
			stage1_code_len=7796 #8192-396
		fi
	elif [ "${CHIP}" = "gx3211" ];then
		if [ "${SECURE_LEVEL}" = "2" ];then
			stage1_code_len=7284 #8192-908
		else
			stage1_code_len=7796 #8192-396
		fi
	elif [ "${CHIP}" = "sirius" ];then
		stage1_code_len=15536 #16384-848 leve1 == level2
	elif [ "${CHIP}" = "taurus" ];then
		if [ "${SECURE_LEVEL}" = "2" ];then
			stage1_code_len=7284 #8192-908
		else
			stage1_code_len=7796 #8192-396
		fi
	elif [ "${CHIP}" = "gemini" ];then
		if [ "${SECURE_LEVEL}" = "2" ];then
			stage1_code_len=7284 #8192-908
		else
			stage1_code_len=7796 #8192-396
		fi
	else
		echo "error CHIP: ${CHIP}"
		exit 1
	fi

	if [ $len -gt $stage1_code_len ] ;then
		echo "error:region 'stage1' overflowed by" $((${len}-${stage1_code_len})) "bytes" "when secure level == "${SECURE_LEVEL}
		exit 1
	fi
}

main()
{
	BOOTLOADER=$1
	SIGN_FILE=$2
	SIGN_ALGO=$3
	CHIP=$4
	SECURE_LEVEL=$5
	ENABLE_EXTERN_PARAM=$6
	EXTERN_PARAM_FILE_SIZE=$7
	EXTERN_PARAM_FILE_COUNT=$8

	if [ -n "$6" ];then
		BOOTLOADER_SIZE=$6
	fi

	if [ ${BOOTLOADER} -eq 0 ];then
		if [ "`echo ${SIGN_ALGO_LIST} | grep "${SIGN_ALGO}"`" ];then 
			echo
		else
			echo "error: ${LINENO}"
			help
		fi
	else
		if [ "`echo ${CHIP_LIST} | grep "${CHIP}"`" -a "`echo ${SECURE_LEVEL_LIST} | grep "${SECURE_LEVEL}"`" -a "`echo ${SIGN_ALGO_LIST} | grep "${SIGN_ALGO}"`" ];then 
			echo
		else
			echo "error: ${LINENO}"
			help
		fi

		if [ "${SIGN_FILE##*.}" = "boot" ];then
			BIN_OR_BOOT=BOOT
		fi

		if [ "${CHIP}" = "gx3113c" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=4124   #4KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=4096   #4KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_gx3113c.txt
		elif [ "${CHIP}" = "gx6605s" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=8220  #8KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=8192   #8KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_gx6605s.txt
		elif [ "${CHIP}" = "gx3211" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=8220  #8KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=8192   #8KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_gx3211.txt
		elif [ "${CHIP}" = "sirius" ];then
			LOADER_STAGE1_LEN=16384  #16KB
			LOADER_STAGE1_START=32
		elif [ "${CHIP}" = "vega" ];then
			LOADER_STAGE1_LEN=16384  #16KB
			LOADER_STAGE1_START=48
		elif [ "${CHIP}" = "taurus" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=8220  #8KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=8192   #8KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_taurus.txt
		elif [ "${CHIP}" = "gemini" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=8220  #8KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=8192   #8KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_gemini.txt
		elif [ "${CHIP}" = "cygnus" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=8220  #8KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=8192   #8KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_cygnus.txt
			CHIP=gemini
		elif [ "${CHIP}" = "scorpio" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=8220  #8KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=8192   #8KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_scorpio.txt
			CHIP=gemini
		elif [ "${CHIP}" = "scorpio_taurus" ];then
			if [ "${BIN_OR_BOOT}" = "BOOT" ];then
				LOADER_STAGE1_LEN=8220  #8KB + 28B
				LOADER_STAGE1_START=32
			else
				LOADER_STAGE1_LEN=8192   #8KB
				LOADER_STAGE1_START=4
			fi
			REG_CFG_FILE=${CHIP_CONFIG_DIR}/reg_config_scorpio_taurus.txt
			CHIP=gemini
		else
			echo "error CHIP: ${CHIP}"
			exit 1
		fi

		LOADER_STAGE2_START=$((${LOADER_STAGE1_LEN}))
		if [ "${ENABLE_EXTERN_PARAM}" = "y" ];then
			EXTERN_PARAM_START=${LOADER_STAGE2_START}
			LOADER_STAGE2_START=$((${LOADER_STAGE2_START}+${EXTERN_PARAM_FILE_SIZE}*${EXTERN_PARAM_FILE_COUNT}))
		fi
	fi

	if [ ! -f "${INPUT_DIR}/${SIGN_FILE}" ];then
		echo "error: ${INPUT_DIR}/${SIGN_FILE} is not exist !!!"
		exit 1
	fi

	if [ ${BOOTLOADER} -eq 1 ];then
		stage1_code_len_check
	fi

	gcc ${COMMON_DIR}/read_file.c -o ${COMMON_DIR}/read_file

	local PWD=`pwd`
	case "${SIGN_ALGO}" in
		"SM2")
			cd ${SM2_DIR}
			./sign.sh
			cd ${PWD}
			;;
		"RSA_PKCS")
			cd ${RSA_DIR}
			./sign.sh
			cd ${PWD}
			;;
		"RSA_SMI")
			cd ${RSA_DIR}
			./sign.sh
			cd ${PWD}
			;;
		*)
			echo "error: ${LINENO}"
			help
			;;
	esac

	rm -rf ${COMMON_DIR}/read_file
}

help()
{

	echo "-------------------------------------------------------------------------------------------------------------------------------"
	echo "Instructions："
	echo "		./digital_signature.sh clean"
	echo "		./digital_signature.sh <bootloader> <sign_file> <sign_algo> [chip] [secure_level] [length]"
	echo "			bootloader  : 0 | 1"
	echo "			sign_file   : the sign file"
	echo "			sign_algo   : ${SIGN_ALGO_LIST}"
	echo "			chip        : ${CHIP_LIST}"
	echo "			secure_level: ${SECURE_LEVEL_LIST}"
	echo "			length      : the size of the outputsize"
	echo ""
	echo "Note："
	echo "		1：<bootloader> option indicate the sign file is bootloader or other file, bootloader = 1 is bootloader, bootloader = 0 is other file"
	echo "		   if bootloader = 1, [chip] / [secure_level] option must be set,[length] option gx3211 | gx6605s | gx3113c must be set"
	echo "		   if bootloader = 0, [chip] / [secure_level] / [length] option don't need to care"
	echo "		2：[length] option indicate the signed file length, if you use this script outside of the gxloader project, you should ask the gxloader compiler for this length"
	echo ""
	echo ""
	echo "Example："
	echo "		./digital_signature.sh 1 loader-sflash.bin RSA_PKCS sirius 1"
	echo "		./digital_signature.sh 1 loader-sflash.bin SM2 sirius 1"
	echo "		./digital_signature.sh 0 test.bin SM2"
	echo "		./digital_signature.sh 0 test.bin RSA_SMI"
	echo "		./digital_signature.sh 1 loader-sflash.bin RSA_PKCS gx3211 2 0x20000"
	echo ""
	echo "Other info："
	echo "		version info ：${VERSION_INFO}"
	echo "		if there are any questions, please send mail to huyuan@nationalchip.com"
	echo "-------------------------------------------------------------------------------------------------------------------------------"
	exit
}

clean()
{
	rm -rf ${OUTPUT_DIR}/*
}

if [ "$1" = "clean" ];then
	clean
elif [ $# -le 2 ];then
	help
else
	main $@
fi
