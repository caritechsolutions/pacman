#########################################################################
# File Name   : script/data_encrypt.sh
# Author      :
# mail        :
# Created Time: 2018年01月31日 星期三 14时07分26秒
#########################################################################
#!/bin/bash

CUR_DIR=$(cd "$(dirname "$0")"; pwd)
INPUT_DIR=${CUR_DIR}/input
OUTPUT_DIR=${CUR_DIR}/output
CHIP_CONFIG_DIR=${CUR_DIR}/config
COMMON_DIR=${CUR_DIR}/../common


IV_VALUE=
ALIGN_VALUE=16

LOADER_STAGE1_ENCRYPT_KEY=
LOADER_STAGE1_ENCRYPT_KEY_START=
LOADER_STAGE1_ENCRYPT_KEY_LENGTH=
LOADER_STAGE1_ENCRYPT_FLAG=
LOADER_STAGE1_ENCRYPT_FLAG_START=
LOADER_STAGE1_ENCRYPT_FLAG_LENGTH=
LOADER_STAGE1_ENCRYPT_DERIVE_KEY=
LOADER_STAGE1_ENCRYPT_DATA_START=
LOADER_STAGE1_ENCRYPT_DATA_SIZE=

LOADER_STAGE2_ENCRYPT_KEY=
LOADER_STAGE2_ENCRYPT_KEY_START=
LOADER_STAGE2_ENCRYPT_KEY_LENGTH=
LOADER_STAGE2_ENCRYPT_FLAG=
LOADER_STAGE2_ENCRYPT_FLAG_START=
LOADER_STAGE2_ENCRYPT_FLAG_LENGTH=
LOADER_STAGE2_ENCRYPT_DERIVE_KEY=
LOADER_STAGE2_ENCRYPT_DATA_START=
LOADER_STAGE2_ENCRYPT_DATA_SIZE=
ENABLE_EXTERN_PARAM=
EXTERN_PARAM_FILE_SIZE=
EXTERN_PARAM_FILE_TOTAL_SIZE=
EXTERN_PARAM_FILE_COUNT=
LOADER_EXTERN_PARAM_DATA_START=

data_encrypt()
{
	local encrypt_type=$1
	local input_file=$2
	local output_file=$3
	local decrypt_key=$4
	local encrypt_mode=$5

	IV="-iv ${IV_VALUE}"

	if [ "${encrypt_mode}" = "ecb" ];then
		IV=
	fi

	case "${encrypt_type}" in
		"sm4")
			gmssl sms4-${encrypt_mode} -in ${input_file} -out ${output_file} -K ${decrypt_key} ${IV} -nopad -nosalt
			;;
		"aes128")
			openssl enc -aes-128-${encrypt_mode} -in ${input_file} -out ${output_file} -K ${decrypt_key} ${IV} -nopad -nosalt
			;;
		*)
			echo "Error data encrypt type: ${encrypt_type}"
			exit 1
	esac
}

help()
{
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	echo "Instructions："
	echo "		./data_encrypt.sh clean"
	echo "		./data_encrypt.sh <bootloader> <input_file> <output_file> <encrypt_type> [chip] [stage1_encrypt] [stage2_encrypt] [stage1_encrypt_derive] [stage2_encrypt_derive] [secure_boot_level]"
	echo "			bootloader		：0 | 1"
	echo "			input_file		：the file to encrypt"
	echo "			output_file		：the encrypted of input_file"
	echo "			encrypt_type		：sm4-cbc | sm4-ecb | aes128-cbc | aes128-ecb"
	echo "			chip			：sirius | taurus | gemini"
	echo "			stage1_encrypt		：0 | 1"
	echo "			stage2_encrypt		：0 | 1"
	echo "			stage1_encrypt_derive	：0 | 1"
	echo "			stage2_encrypt_derive	：0 | 1"
	echo "			secure_boot_level	: 0 | 1 | 2"
	echo ""
	echo "Note："
	echo "		1：<bootloader> option indicate the input_file is bootloader or other file, <bootloader> = 1 is bootloader,<bootloader> = 0 is other file"
	echo "			if <bootloader> = 1, [chip] / [stage1_encrypt_derive] / [stage2_encrypt_derive] / [secure_boot_level]option must be set"
	echo "			if <bootloader> = 0, [chip] / [stage1_encrypt_derive] / [stage2_encrypt_derive] / [secure_boot_level]option don't need to care"
	echo "		2：<stage1_encrypt> option indicate show if stage1 data needs to be encrypted"
	echo "		3：<stage2_encrypt> option indicate show if stage2 data needs to be encrypted"
	echo "		4：<stage1_encrypt_derive> option indicate stage1 whether to use derive"
	echo "		5：<stage2_encrypt_derive> option indicate stage2 whether to use derive"
	echo "		6：<secure_boot_level> option indicate secure boot level ,<secure_boot_level> = 0 is normal boot,<secure_boot_level> = 1 is Level 1 secure boot,<secure_boot_level> = 2 is level 2 secure boot"
	echo "			if [chip] = taurus or gemini, [secure_boot_level] option must be set the correct secure boot level"
	echo "			if [chip] != taurus 0r gemini  , [secure_boot_level] option don't need to care"
	echo "Example："
	echo "		./data_encrypt.sh 1 loader-sflash.bin loader-sflash-encrypt.bin sm4-cbc sirius 1 1 0"
	echo "		./data_encrypt.sh 1 loader-sflash.bin loader-sflash-encrypt.bin aes128-ecb sirius 0 1 0"
	echo "		./data_encrypt.sh 0 test.bin test-encrypt.bin sm4-ecb"
	echo "		./data_encrypt.sh 0 test.bin test-encrypt.bin aes128-cbc"
	echo ""
	echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	exit 1
}

read_bin()
{
	local input_file=$1
	local output_file=$2
	local addr=$3

	if [ ! -f ${input_file} ]; then
		echo "${input_file} not exist !"
		exit 1
	fi

	local input_file_len=`ls -l ${input_file} | awk '{print $5}'`

	if [ -n "$4" ];then
		local len=$4
		if [ ${input_file_len} -lt $(expr "${addr}" + "${len}") ];then
			echo "read_bin error!"
			exit 1
		fi

		${COMMON_DIR}/read_file ${input_file} ${output_file} ${addr} 0 ${len}
	else
		if [ ${input_file_len} -lt ${addr} ];then
			echo "read_bin error!"
			exit 1
		fi

		${COMMON_DIR}/read_file ${input_file} ${output_file} ${addr} 1
	fi
}

data_encrypt_for_bootloader()
{
	if [ $# -lt 9 ];then
		help
	fi

	local bootloader=${1}
	local input_file=${INPUT_DIR}/${2}
	local output_file=${OUTPUT_DIR}/${3}
	local encrypt=$4
	local encrypt_chip=$5
	local stage1_encrypt=$6
	local stage2_encrypt=$7
	local stage1_encrypt_derive=$8
	local stage2_encrypt_derive=$9
	local secure_boot_level=${10}
	ENABLE_EXTERN_PARAM=${11}
	EXTERN_PARAM_FILE_SIZE=${12}
	EXTERN_PARAM_FILE_COUNT=${13}
	EXTERN_PARAM_FILE_TOTAL_SIZE=0

	if [ -z "$EXTERN_PARAM_FILE_COUNT" ]; then
		EXTERN_PARAM_FILE_COUNT=1
	fi
	EXTERN_PARAM_FILE_TOTAL_SIZE=$((EXTERN_PARAM_FILE_COUNT*EXTERN_PARAM_FILE_SIZE))

	local encrypt_type=${encrypt%-*}
	local encrypt_mode=${encrypt##*-}

	local input_file_len=`ls -l ${input_file} | awk '{print $5}'`

	if [ "${encrypt_chip}" = "sirius" ];then
		LOADER_STAGE1_ENCRYPT_KEY_START=16340  #0x3FD4
		LOADER_STAGE1_ENCRYPT_KEY_LENGTH=16
		LOADER_STAGE1_ENCRYPT_FLAG=11111111
		LOADER_STAGE1_ENCRYPT_FLAG_START=16356 #0x3FE4
		LOADER_STAGE1_ENCRYPT_FLAG_LENGTH=4
		LOADER_STAGE1_ENCRYPT_DATA_START=32
		LOADER_STAGE1_ENCRYPT_DATA_SIZE=15536

		LOADER_STAGE2_ENCRYPT_KEY_START=16390  #0x4006
		LOADER_STAGE2_ENCRYPT_KEY_LENGTH=16
		LOADER_STAGE2_ENCRYPT_FLAG=11111111
		LOADER_STAGE2_ENCRYPT_FLAG_START=16386 #0x4002
		LOADER_STAGE2_ENCRYPT_FLAG_LENGTH=4
		LOADER_STAGE2_ENCRYPT_DATA_START=16416 #0x4020
		LOADER_STAGE2_ENCRYPT_DATA_SIZE=`echo "${input_file_len}-${LOADER_STAGE2_ENCRYPT_DATA_START}"|bc`

		RESERVE1_SIZE=32	# 0x0 ~ 0x20
		RESERVE2_SIZE=772	# 0x3cd0 ~ 0x3fd4
		RESERVE3_SIZE=26	# 0x3fe8 ~ 0x4002
		RESERVE4_SIZE=10	# 0x4016 ~ 0x4020

		read_bin ${input_file} reserve1.bin 0 ${RESERVE1_SIZE} # magic
		read_bin ${input_file} stage1_data.bin ${LOADER_STAGE1_ENCRYPT_DATA_START} ${LOADER_STAGE1_ENCRYPT_DATA_SIZE} # stage1 data
		read_bin ${input_file} reserve2.bin $((${LOADER_STAGE1_ENCRYPT_DATA_START} + ${LOADER_STAGE1_ENCRYPT_DATA_SIZE})) ${RESERVE2_SIZE}
		read_bin ${input_file} loader_stage1_decrypt_key.bin ${LOADER_STAGE1_ENCRYPT_KEY_START} ${LOADER_STAGE1_ENCRYPT_KEY_LENGTH} # stage1 key
		read_bin ${input_file} loader_stage1_decrypt_flag.bin ${LOADER_STAGE1_ENCRYPT_FLAG_START} ${LOADER_STAGE1_ENCRYPT_FLAG_LENGTH} # stage1 flag
		read_bin ${input_file} reserve3.bin $((${LOADER_STAGE1_ENCRYPT_FLAG_START} + ${LOADER_STAGE1_ENCRYPT_FLAG_LENGTH})) ${RESERVE3_SIZE}
		read_bin ${input_file} loader_stage2_decrypt_flag.bin ${LOADER_STAGE2_ENCRYPT_FLAG_START} ${LOADER_STAGE2_ENCRYPT_FLAG_LENGTH} # stage2 flag
		read_bin ${input_file} loader_stage2_decrypt_key.bin ${LOADER_STAGE2_ENCRYPT_KEY_START} ${LOADER_STAGE2_ENCRYPT_KEY_LENGTH} # stage2 key
		read_bin ${input_file} reserve4.bin $((${LOADER_STAGE2_ENCRYPT_KEY_START} + ${LOADER_STAGE2_ENCRYPT_KEY_LENGTH})) ${RESERVE4_SIZE}
		read_bin ${input_file} stage2_data.bin ${LOADER_STAGE2_ENCRYPT_DATA_START} # stage1 data

		### stage1
		if [ ${stage1_encrypt} -eq 1 ]; then
			stage_file_len=`ls -l stage1_data.bin | awk '{print $5}'`
			reserve_len=$((${stage_file_len} % ${ALIGN_VALUE}))
			data_len=$((${stage_file_len} - ${reserve_len}))

			read_bin stage1_data.bin stage1_data_to_en 0 ${data_len}
			read_bin stage1_data.bin stage1_data_reserve ${data_len}
			data_encrypt ${encrypt_type} stage1_data_to_en stage1_data.en ${LOADER_STAGE1_ENCRYPT_KEY} ${encrypt_mode}


			cat reserve1.bin > ${output_file}
			cat stage1_data.en >> ${output_file}
			cat stage1_data_reserve >> ${output_file}
			cat reserve2.bin >> ${output_file}

			rm reserve1.bin
			rm stage1_data.bin
			rm reserve2.bin
			rm stage1_data_reserve
			rm stage1_data_to_en
			rm stage1_data.en

			if [ ${stage1_encrypt_derive} -eq 1 ];then
				echo ${LOADER_STAGE1_ENCRYPT_KEY} > KEY_FILE
				xxd -c4 -p -r KEY_FILE TEMP_FILE
				# 对加密数据的KEY进行加密
				data_encrypt ${encrypt_type} TEMP_FILE TEMP_FILE_ENCRYPT ${LOADER_STAGE1_ENCRYPT_DERIVE_KEY} ecb
				mv TEMP_FILE_ENCRYPT loader_stage1_decrypt_key.bin
				rm KEY_FILE
				rm TEMP_FILE
			fi
			echo ${LOADER_STAGE1_ENCRYPT_FLAG} > TEMP_FILE
			xxd -c4 -p -r TEMP_FILE loader_stage1_decrypt_flag.bin
			rm TEMP_FILE

			cat loader_stage1_decrypt_key.bin >> ${output_file}
			cat loader_stage1_decrypt_flag.bin >> ${output_file}
			cat reserve3.bin >> ${output_file}

			rm loader_stage1_decrypt_key.bin
			rm loader_stage1_decrypt_flag.bin
			rm reserve3.bin
		else
			cat reserve1.bin > ${output_file}
			cat stage1_data.bin >> ${output_file}
			cat reserve2.bin >> ${output_file}
			cat loader_stage1_decrypt_key.bin >> ${output_file}
			cat loader_stage1_decrypt_flag.bin >> ${output_file}
			cat reserve3.bin >> ${output_file}

			rm reserve1.bin
			rm stage1_data.bin
			rm reserve2.bin
			rm loader_stage1_decrypt_key.bin
			rm loader_stage1_decrypt_flag.bin
			rm reserve3.bin
		fi

		## stage2
		if [ ${stage2_encrypt} -eq 1 ]; then
			if [ ${stage2_encrypt_derive} -eq 1 ];then
				echo ${LOADER_STAGE2_ENCRYPT_KEY} > KEY_FILE
				xxd -c4 -p -r KEY_FILE TEMP_FILE
				# 对加密数据的KEY进行加密
				data_encrypt ${encrypt_type} TEMP_FILE TEMP_FILE_ENCRYPT ${LOADER_STAGE2_ENCRYPT_DERIVE_KEY} ecb
				mv TEMP_FILE_ENCRYPT loader_stage2_decrypt_key.bin
				rm KEY_FILE
				rm TEMP_FILE
			fi
			echo ${LOADER_STAGE2_ENCRYPT_FLAG} > TEMP_FILE
			xxd -c4 -p -r TEMP_FILE loader_stage2_decrypt_flag.bin
			rm TEMP_FILE

			cat loader_stage2_decrypt_flag.bin >> ${output_file}
			cat loader_stage2_decrypt_key.bin >> ${output_file}
			cat reserve4.bin >> ${output_file}

			rm loader_stage2_decrypt_key.bin
			rm loader_stage2_decrypt_flag.bin
			rm reserve4.bin

			stage_file_len=`ls -l stage2_data.bin | awk '{print $5}'`
			reserve_len=$((${stage_file_len} % ${ALIGN_VALUE}))
			data_len=$((${stage_file_len} - ${reserve_len}))

			read_bin stage2_data.bin stage2_data_to_en 0 ${data_len}
			read_bin stage2_data.bin stage2_data_reserve ${data_len}
			data_encrypt ${encrypt_type} stage2_data_to_en stage2_data.en ${LOADER_STAGE2_ENCRYPT_KEY} ${encrypt_mode}

			cat stage2_data.en >> ${output_file}
			cat stage2_data_reserve >> ${output_file}

			rm stage2_data.bin
			rm stage2_data_to_en
			rm stage2_data_reserve
			rm stage2_data.en
		else
			cat loader_stage2_decrypt_flag.bin >> ${output_file}
			cat loader_stage2_decrypt_key.bin >> ${output_file}
			cat reserve4.bin >> ${output_file}
			cat stage2_data.bin >> ${output_file}

			rm loader_stage2_decrypt_flag.bin
			rm loader_stage2_decrypt_key.bin
			rm reserve4.bin
			rm stage2_data.bin

		fi

	elif [ "${encrypt_chip}" = "taurus" -o "${encrypt_chip}" = "gemini" ];then
		if [ "${input_file##*.}" = "boot" ];then
			LOADER_STAGE1_ENCRYPT_DATA_START=32
			LOADER_STAGE2_ENCRYPT_DATA_START=8252 #0x203C
			RESERVE1_SIZE=32 	# 0x0 ~ 0x20
		else
			LOADER_STAGE1_ENCRYPT_DATA_START=4
			LOADER_STAGE2_ENCRYPT_DATA_START=8224 #0x2020
			RESERVE1_SIZE=4 	# 0x0 ~ 0x4
			if [ "${ENABLE_EXTERN_PARAM}" = "y" ];then
				LOADER_EXTERN_PARAM_DATA_START=8192
				LOADER_STAGE2_ENCRYPT_DATA_START=$((${LOADER_STAGE2_ENCRYPT_DATA_START} + ${EXTERN_PARAM_FILE_TOTAL_SIZE}))
			fi
		fi

		if [ ${secure_boot_level} -eq 0 ];then
			LOADER_STAGE1_ENCRYPT_DATA_SIZE=8184 #0x1ff8
			RESERVE2_SIZE=4	    # 0x1ffc ~ 0x1fff
		elif [ ${secure_boot_level} -eq 1 ];then
			LOADER_STAGE1_ENCRYPT_DATA_SIZE=7796 #0x1e74
			RESERVE2_SIZE=392	# 0x1e78 ~ 0x1fff
		else
			LOADER_STAGE1_ENCRYPT_DATA_SIZE=7284 #0x1c74
			RESERVE2_SIZE=904	# 0x1c78 ~ 0x1fff
		fi

		LOADER_STAGE2_ENCRYPT_DATA_SIZE=`echo "${input_file_len}-${LOADER_STAGE2_ENCRYPT_DATA_START}"|bc`

		RESERVE3_SIZE=32	# 0x2000 ~ 0x2020 or 0x201C ~ 0x203C

		read_bin ${input_file} reserve1.bin 0 ${RESERVE1_SIZE} # magic
		read_bin ${input_file} stage1_data.bin ${LOADER_STAGE1_ENCRYPT_DATA_START} ${LOADER_STAGE1_ENCRYPT_DATA_SIZE} # stage1 data
		read_bin ${input_file} reserve2.bin $((${LOADER_STAGE1_ENCRYPT_DATA_START} + ${LOADER_STAGE1_ENCRYPT_DATA_SIZE})) ${RESERVE2_SIZE}
		if [ "${ENABLE_EXTERN_PARAM}" = "y" ];then
			read_bin ${input_file} reserve3.bin $((${LOADER_STAGE1_ENCRYPT_DATA_START} + ${LOADER_STAGE1_ENCRYPT_DATA_SIZE} + ${RESERVE2_SIZE} + ${EXTERN_PARAM_FILE_TOTAL_SIZE}))  ${RESERVE3_SIZE}
			read_bin ${input_file} extern_param.bin ${LOADER_EXTERN_PARAM_DATA_START} ${EXTERN_PARAM_FILE_TOTAL_SIZE}
		else
			read_bin ${input_file} reserve3.bin $((${LOADER_STAGE1_ENCRYPT_DATA_START} + ${LOADER_STAGE1_ENCRYPT_DATA_SIZE} + ${RESERVE2_SIZE}))  ${RESERVE3_SIZE}
		fi
		read_bin ${input_file} stage2_data.bin ${LOADER_STAGE2_ENCRYPT_DATA_START} # stage2 data

		### stage1
		if [ ${stage1_encrypt} -eq 1 ];then
			stage_file_len=`ls -l stage1_data.bin | awk '{print $5}'`
			reserve_len=$((${stage_file_len} % ${ALIGN_VALUE}))
			data_len=$((${stage_file_len} - ${reserve_len}))

			read_bin stage1_data.bin stage1_data_to_en 0 ${data_len}
			read_bin stage1_data.bin stage1_data_reserve ${data_len}
			data_encrypt ${encrypt_type} stage1_data_to_en stage1_data.en ${LOADER_STAGE1_ENCRYPT_KEY} ${encrypt_mode}


			cat reserve1.bin > ${output_file}
			cat stage1_data.en >> ${output_file}
			cat stage1_data_reserve >> ${output_file}
			cat reserve2.bin >> ${output_file}

			rm reserve1.bin
			rm stage1_data.bin
			rm reserve2.bin
			rm stage1_data_reserve
			rm stage1_data_to_en
			rm stage1_data.en
		else
			cat reserve1.bin > ${output_file}
			cat stage1_data.bin >> ${output_file}
			cat reserve2.bin >> ${output_file}

			rm reserve1.bin
			rm stage1_data.bin
			rm reserve2.bin
		fi

		## extern_param(encrypt key use same as stage2)
		if [ "${ENABLE_EXTERN_PARAM}" = "y" ];then
			if [ ${stage2_encrypt} -eq 1 ]; then
				stage_file_len=`ls -l extern_param.bin | awk '{print $5}'`
				for i in $(seq 0 $((EXTERN_PARAM_FILE_COUNT-1))); do
					reserve_len=$((${EXTERN_PARAM_FILE_SIZE} % ${ALIGN_VALUE}))
					data_len=$((${EXTERN_PARAM_FILE_SIZE} - ${reserve_len}))

					read_bin extern_param.bin extern_param_to_en $((i * EXTERN_PARAM_FILE_SIZE))  ${data_len}
					read_bin extern_param.bin extern_param_reserve $((i * EXTERN_PARAM_FILE_SIZE + ${data_len})) ${reserve_len}
					data_encrypt ${encrypt_type} extern_param_to_en extern_param.en ${LOADER_STAGE2_ENCRYPT_KEY} ${encrypt_mode}

					cat extern_param.en >> ${output_file}
					cat extern_param_reserve >> ${output_file}

					rm extern_param_to_en
					rm extern_param_reserve
					rm extern_param.en
				done
				rm extern_param.bin
			else
				cat extern_param.bin >> ${output_file}

				rm extern_param.bin
			fi
		fi

		cat reserve3.bin >> ${output_file}

		rm reserve3.bin

		## stage2
		if [ ${stage2_encrypt} -eq 1 ]; then

			stage_file_len=`ls -l stage2_data.bin | awk '{print $5}'`
			reserve_len=$((${stage_file_len} % ${ALIGN_VALUE}))
			data_len=$((${stage_file_len} - ${reserve_len}))

			read_bin stage2_data.bin stage2_data_to_en 0 ${data_len}
			read_bin stage2_data.bin stage2_data_reserve ${data_len}
			data_encrypt ${encrypt_type} stage2_data_to_en stage2_data.en ${LOADER_STAGE2_ENCRYPT_KEY} ${encrypt_mode}

			cat stage2_data.en >> ${output_file}
			cat stage2_data_reserve >> ${output_file}

			rm stage2_data.bin
			rm stage2_data_to_en
			rm stage2_data_reserve
			rm stage2_data.en
		else
			cat stage2_data.bin >> ${output_file}

			rm stage2_data.bin

		fi

	elif [ "${encrypt_chip}" = "vega" ];then
		LOADER_STAGE1_ENCRYPT_DATA_START=48
		LOADER_STAGE1_ENCRYPT_DATA_SIZE=18528
		LOADER_STAGE2_ENCRYPT_KEY_START=19684 #0x4CE4
		LOADER_STAGE2_ENCRYPT_KEY_LENGTH=16
		LOADER_STAGE2_ENCRYPT_FLAG_START=19700 #0x4CF4
		LOADER_STAGE2_ENCRYPT_FLAG_LENGTH=4

		TEE_LOADER_STAGE2_ENCRYPT_KEY_START=23252  #0x5AD4
		TEE_LOADER_STAGE2_ENCRYPT_KEY_LENGTH=16
		TEE_LOADER_STAGE2_ENCRYPT_FLAG_START=23268 #0x5AE4
		TEE_LOADER_STAGE2_ENCRYPT_FLAG_LENGTH=4
		LOADER_STAGE2_ENCRYPT_DATA_START=24576 #0x6000
		LOADER_STAGE2_ENCRYPT_DATA_SIZE=`echo "${input_file_len}-${LOADER_STAGE2_ENCRYPT_DATA_START}"|bc`

		RESERVE1_SIZE=32	# 0x0 ~ 0x20
		RESERVE2_SIZE=1108	# 0x4890 ~ 0x4ce4
		RESERVE3_SIZE=3552	# 0x4cf4 ~ 0x5ad4
		RESERVE4_SIZE=1304	# 0x5ae8 ~ 0x6000

		read_bin ${input_file} reserve1.bin 0 ${RESERVE1_SIZE} # magic
		read_bin ${input_file} stage1_data.bin ${LOADER_STAGE1_ENCRYPT_DATA_START} ${LOADER_STAGE1_ENCRYPT_DATA_SIZE} # stage1 data
		read_bin ${input_file} reserve2.bin $((${LOADER_STAGE1_ENCRYPT_DATA_START} + ${LOADER_STAGE1_ENCRYPT_DATA_SIZE})) ${RESERVE2_SIZE}
		read_bin ${input_file} loader_stage2_decrypt_key.bin ${LOADER_STAGE2_ENCRYPT_KEY_START} ${LOADER_STAGE2_ENCRYPT_KEY_LENGTH} # stage1 key
		read_bin ${input_file} loader_stage2_decrypt_flag.bin ${LOADER_STAGE2_ENCRYPT_FLAG_START} ${LOADER_STAGE2_ENCRYPT_FLAG_LENGTH} # stage1 flag
		read_bin ${input_file} reserve3.bin $((${LOADER_STAGE2_ENCRYPT_FLAG_START} + ${LOADER_STAGE2_ENCRYPT_FLAG_LENGTH})) ${RESERVE3_SIZE}
		read_bin ${input_file} tee_loader_stage2_decrypt_key.bin ${TEE_LOADER_STAGE2_ENCRYPT_KEY_START} ${TEE_LOADER_STAGE2_ENCRYPT_KEY_LENGTH} # stage2 key
		read_bin ${input_file} tee_loader_stage2_decrypt_flag.bin ${TEE_LOADER_STAGE2_ENCRYPT_FLAG_START} ${TEE_LOADER_STAGE2_ENCRYPT_FLAG_LENGTH} # stage2 flag
		read_bin ${input_file} reserve4.bin $((${TEE_LOADER_STAGE2_ENCRYPT_KEY_START} + ${TEE_LOADER_STAGE2_ENCRYPT_KEY_LENGTH})) ${RESERVE4_SIZE}
		read_bin ${input_file} stage2_data.bin ${LOADER_STAGE2_ENCRYPT_DATA_START} # stage1 data

		### stage1
		if [ ${stage1_encrypt} -eq 1 ]; then
			stage_file_len=`ls -l stage1_data.bin | awk '{print $5}'`
			reserve_len=$((${stage_file_len} % ${ALIGN_VALUE}))
			data_len=$((${stage_file_len} - ${reserve_len}))

			read_bin stage1_data.bin stage1_data_to_en 0 ${data_len}
			read_bin stage1_data.bin stage1_data_reserve ${data_len}
			data_encrypt ${encrypt_type} stage1_data_to_en stage1_data.en ${LOADER_STAGE1_ENCRYPT_KEY} ${encrypt_mode}


			cat reserve1.bin > ${output_file}
			cat stage1_data.en >> ${output_file}
			cat stage1_data_reserve >> ${output_file}
			cat reserve2.bin >> ${output_file}

			rm reserve1.bin
			rm stage1_data.bin
			rm reserve2.bin
			rm stage1_data_reserve
			rm stage1_data_to_en
			rm stage1_data.en

			if [ ${stage1_encrypt_derive} -eq 1 ];then
				echo ${LOADER_STAGE1_ENCRYPT_KEY} > KEY_FILE
				xxd -c4 -p -r KEY_FILE TEMP_FILE
				# 对加密数据的KEY进行加密
				data_encrypt ${encrypt_type} TEMP_FILE TEMP_FILE_ENCRYPT ${LOADER_STAGE1_ENCRYPT_DERIVE_KEY} ecb
				mv TEMP_FILE_ENCRYPT loader_stage1_decrypt_key.bin
				rm KEY_FILE
				rm TEMP_FILE
			fi
			echo ${LOADER_STAGE1_ENCRYPT_FLAG} > TEMP_FILE
			xxd -c4 -p -r TEMP_FILE loader_stage1_decrypt_flag.bin
			rm TEMP_FILE

			cat loader_stage1_decrypt_key.bin >> ${output_file}
			cat loader_stage1_decrypt_flag.bin >> ${output_file}
			cat reserve3.bin >> ${output_file}

			rm loader_stage1_decrypt_key.bin
			rm loader_stage1_decrypt_flag.bin
			rm reserve3.bin
		else
			cat reserve1.bin > ${output_file}
			cat stage1_data.bin >> ${output_file}
			cat reserve2.bin >> ${output_file}
			cat loader_stage1_decrypt_key.bin >> ${output_file}
			cat loader_stage1_decrypt_flag.bin >> ${output_file}
			cat reserve3.bin >> ${output_file}

			rm reserve1.bin
			rm stage1_data.bin
			rm reserve2.bin
			rm loader_stage1_decrypt_key.bin
			rm loader_stage1_decrypt_flag.bin
			rm reserve3.bin
		fi

		## stage2
		if [ ${stage2_encrypt} -eq 1 ]; then
			if [ ${stage2_encrypt_derive} -eq 1 ];then
				echo ${LOADER_STAGE2_ENCRYPT_KEY} > KEY_FILE
				xxd -c4 -p -r KEY_FILE TEMP_FILE
				# 对加密数据的KEY进行加密
				data_encrypt ${encrypt_type} TEMP_FILE TEMP_FILE_ENCRYPT ${LOADER_STAGE2_ENCRYPT_DERIVE_KEY} ecb
				mv TEMP_FILE_ENCRYPT loader_stage2_decrypt_key.bin
				rm KEY_FILE
				rm TEMP_FILE
			fi
			echo ${LOADER_STAGE2_ENCRYPT_FLAG} > TEMP_FILE
			xxd -c4 -p -r TEMP_FILE loader_stage2_decrypt_flag.bin
			rm TEMP_FILE

			cat loader_stage2_decrypt_flag.bin >> ${output_file}
			cat loader_stage2_decrypt_key.bin >> ${output_file}
			cat reserve4.bin >> ${output_file}

			rm loader_stage2_decrypt_key.bin
			rm loader_stage2_decrypt_flag.bin
			rm reserve4.bin

			stage_file_len=`ls -l stage2_data.bin | awk '{print $5}'`
			reserve_len=$((${stage_file_len} % ${ALIGN_VALUE}))
			data_len=$((${stage_file_len} - ${reserve_len}))

			read_bin stage2_data.bin stage2_data_to_en 0 ${data_len}
			read_bin stage2_data.bin stage2_data_reserve ${data_len}
			data_encrypt ${encrypt_type} stage2_data_to_en stage2_data.en ${LOADER_STAGE2_ENCRYPT_KEY} ${encrypt_mode}

			cat stage2_data.en >> ${output_file}
			cat stage2_data_reserve >> ${output_file}

			rm stage2_data.bin
			rm stage2_data_to_en
			rm stage2_data_reserve
			rm stage2_data.en
		else
			cat loader_stage2_decrypt_flag.bin >> ${output_file}
			cat loader_stage2_decrypt_key.bin >> ${output_file}
			cat reserve4.bin >> ${output_file}
			cat stage2_data.bin >> ${output_file}

			rm loader_stage2_decrypt_flag.bin
			rm loader_stage2_decrypt_key.bin
			rm reserve4.bin
			rm stage2_data.bin

		fi
	else
		echo "error CHIP: ${encrypt_chip}"
		exit 1
	fi
}

data_encrypt_for_other()
{
	if [ $# != 4 ];then
		help
	fi

	local bootloader=${1}
	local input_file=${INPUT_DIR}/${2}
	local output_file=${OUTPUT_DIR}/${3}
	local encrypt=$4

	local encrypt_type=${encrypt%-*}
	local encrypt_mode=${encrypt##*-}

	local input_file_len=`ls -l ${input_file} | awk '{print $5}'`
	reserve_len=$((${input_file_len} % ${ALIGN_VALUE}))
	data_len=$((${input_file_len} - ${reserve_len}))

	read_bin ${input_file} TEMP_FILE1 0 ${data_len}
	read_bin ${input_file} TEMP_FILE2 ${data_len}

	data_encrypt ${encrypt_type} TEMP_FILE1 TEMP_FILE1_ENCRYPT ${LOADER_STAGE1_ENCRYPT_KEY} ${encrypt_mode}

	cat TEMP_FILE1_ENCRYPT		> ${output_file}
	cat TEMP_FILE2			>> ${output_file}

	rm -rf TEMP_FILE*
}

main()
{
	IV_VALUE="`cat ${CHIP_CONFIG_DIR}/iv.txt`"
	LOADER_STAGE1_ENCRYPT_KEY="`cat ${CHIP_CONFIG_DIR}/stage1_data_key.txt`"
	LOADER_STAGE2_ENCRYPT_KEY="`cat ${CHIP_CONFIG_DIR}/stage2_data_key.txt`"
	LOADER_STAGE1_ENCRYPT_DERIVE_KEY="`cat ${CHIP_CONFIG_DIR}/stage1_derive_key.txt`"
	LOADER_STAGE2_ENCRYPT_DERIVE_KEY="`cat ${CHIP_CONFIG_DIR}/stage2_derive_key.txt`"

	local bootloader=${1}
	local input_file=${INPUT_DIR}/${2}
	local output_file=${OUTPUT_DIR}/${3}

	if [ ! -f "${input_file}" ];then
		echo "error: ${input_file} is not exist !!!"
		exit 1
	fi
	if [ -f "${output_file}" ];then
		rm -rf ${output_file}
	fi


	gcc ${COMMON_DIR}/read_file.c -o ${COMMON_DIR}/read_file

	if [ ${bootloader} -eq 1 ];then
		data_encrypt_for_bootloader $@
	else
		data_encrypt_for_other $@
	fi

	rm -rf ${COMMON_DIR}/read_file
}

clean()
{
	rm -rf ${OUTPUT_DIR}/*
}

if [ "$1" = "clean" ];then
	clean
elif [ $# -le 3 ];then
	help
else
	main $@
fi

