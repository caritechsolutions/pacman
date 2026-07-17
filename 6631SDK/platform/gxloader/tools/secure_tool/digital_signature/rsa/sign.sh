#!/bin/bash

source ${SIGN_CUR_DIR}/common_fun.sh
RSA_CUR_PWD=`pwd`
RSA_KEY_DIR=${RSA_CUR_PWD}/key
RSA_CONFIG_DIR=${RSA_CUR_PWD}/config
RESERVE1_NUM=28
RESERVE2_NUM=20
RESERVE3_NUM=32
SWITCH_MARKET_ID=${RSA_CONFIG_DIR}/switch_mark_id.txt

rsa_pkcs()
{
	local input_file=$1
	local private_key=$2
	local output_file=$3

	openssl dgst -sha256 -sign ${private_key} -out ${output_file} ${input_file}
}

rsa_smi()
{
	local input_file=$1
	local private_key=$2
	local output_file=$3
	local FIXED_VALUE=0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000

	echo ${FIXED_VALUE} > Fixed_value
	xxd -c4 -p -r Fixed_value > Fixed_value.bin

	txt_to_bin1 ${SWITCH_MARKET_ID} switch_mark_id.bin
	read_bin switch_mark_id.bin Market_id_addr.bin 0 4
	read_bin switch_mark_id.bin Market_id.bin 4 4

	sha256sum ${input_file} > Input_hash256
	echo `cat Input_hash256 | awk '{print $1}'` > Input_hash256
	txt_to_bin1 Input_hash256 Input_hash256.bin

	cat Fixed_value.bin    >  File_to_sign
	cat Market_id.bin      >> File_to_sign
	cat Market_id_addr.bin >> File_to_sign
	for((i=1;i<8;i++));do
		cat Input_hash256.bin  >> File_to_sign
	done

	openssl rsautl -sign -raw -inkey ${private_key} -in File_to_sign -out ${output_file}

	rm Fixed_value
	rm Fixed_value.bin
	rm Market_id.bin
	rm Market_id_addr.bin
	rm switch_mark_id.bin
	rm Input_hash256
	rm Input_hash256.bin
	rm File_to_sign
}

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

rsa()
{
	local input_file=$1
	local private_key=$2
	local output_file=$3
	local sign_algo=$4

	if [ "${sign_algo}" = "RSA_PKCS" ];then
		rsa_pkcs ${input_file} ${private_key} ${output_file}
	elif [ "${sign_algo}" = "RSA_SMI" ];then
		rsa_smi ${input_file} ${private_key} ${output_file}
	else
		echo "error sign algo: ${sign_algo}"
		exit 1
	fi
}

sign_for_other_file()
{
	local SUFFIX="-sign"
	local input_file=${INPUT_DIR}/${SIGN_FILE}
	local output_file=${OUTPUT_DIR}/${SIGN_FILE}${SUFFIX}
	local sign_algo=${SIGN_ALGO}
	local private_key=${RSA_KEY_DIR}/1nd_key.pri

	if [ ! -f "${private_key}" ];then
		echo "error: ${private_key} is not exist !!!"
		exit 1
	fi

	if [ -f "${output_file}" ];then
		rm -rf ${output_file}
	fi

	rsa ${input_file} ${private_key} ${output_file} ${sign_algo}
}

sign_for_bootloader_file()
{
	local input_file=${INPUT_DIR}/${SIGN_FILE}
	local output_file=TEMP_FILE

	PRIVATE_KEY_FILE=${RSA_KEY_DIR}/1nd_key.pri
	PRIVATE_2NDKEY_FILE=${RSA_KEY_DIR}/2nd_key.pri
	SECOND_KEY_MODULUS=${RSA_KEY_DIR}/2nd_key_modulus.bin

	TEE_PRIVATE_KEY_FILE=${RSA_KEY_DIR}/tee_1nd_key.pri
	TEE_PRIVATE_2NDKEY_FILE=${RSA_KEY_DIR}/tee_2nd_key.pri
	TEE_SECOND_KEY_MODULUS=${RSA_KEY_DIR}/tee_2nd_key_modulus.bin

	case "${SECURE_LEVEL}" in
		1) #一级高安
			if [ "${CHIP}" = "gx3211" -o "${CHIP}" = "gx6605s" -o "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" ];then
				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
					read_bin ${input_file} loader_stage1.bin $((${MAGIC_NUM} + ${RESERVE1_NUM})) 7796 #0x1e77-${MAGIC_NUM}+1
				else
					read_bin ${input_file} loader_stage1.bin ${MAGIC_NUM} 7796 #0x1e77-${MAGIC_NUM}+1
				fi
				txt_to_bin ${REG_CFG_FILE} reg_cfg_file.bin
				txt_to_bin1 ${MARKET_ID_FILE} market_id.bin
				zero_to_bin crc_num.bin ${CRC_NUM}

				cat loader_stage1.bin > TMP_FILE1
				cat reg_cfg_file.bin >> TMP_FILE1
				cat market_id.bin    >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_stage1.sig ${SIGN_ALGO}
				mv TMP_FILE1 ${OUTPUT_DIR}/loader_level1_stage1_code.bin

				cat magic_num.bin     >${output_file}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					cat reserve1.bin  >>${output_file}
					rm reserve1.bin
				fi
				cat loader_stage1.bin >>${output_file}
				cat reg_cfg_file.bin  >>${output_file}
				cat market_id.bin     >>${output_file}
				cat loader_stage1.sig >>${output_file}
				cat crc_num.bin       >>${output_file}

				rm magic_num.bin
				rm loader_stage1.bin
				rm reg_cfg_file.bin
				rm loader_stage1.sig
				rm crc_num.bin

				if [ "${ENABLE_EXTERN_PARAM}" = "y" ];then
					if [ "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" ];then
						if [ -z "$EXTERN_PARAM_FILE_COUNT" ]; then
							EXTERN_PARAM_FILE_COUNT=1
						fi
						for ((i = 0; i < $EXTERN_PARAM_FILE_COUNT; i++)); do
							offset=$((EXTERN_PARAM_START + i * EXTERN_PARAM_FILE_SIZE))
							read_bin ${input_file} loader_extern_param.bin ${offset} $((${EXTERN_PARAM_FILE_SIZE} - 256))
							cat loader_extern_param.bin > TMP_FILE1
							rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_extern_param.sig ${SIGN_ALGO}
							if [ "${i}" -eq 0 ]; then
								mv TMP_FILE1 ${OUTPUT_DIR}/loader_level1_extern_param.bin
							else
								mv TMP_FILE1 ${OUTPUT_DIR}/loader_level1_extern_param_$i.bin
							fi

							cat loader_extern_param.bin           >>${output_file}
							cat loader_extern_param.sig           >>${output_file}

							rm loader_extern_param.bin
							rm loader_extern_param.sig
						done
					fi
				fi

				if [ "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" -o "${BIN_OR_BOOT}" = "BOOT" ];then
					#   stage2_bin
					#   char sig_key[256];
					read_bin ${input_file} reserve3.bin ${LOADER_STAGE2_START} ${RESERVE3_NUM}
					LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE3_NUM}"|bc`
					read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}
					cat loader_stage2.bin > TMP_FILE1
					rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_stage2.sig ${SIGN_ALGO}
					mv TMP_FILE1 ${OUTPUT_DIR}/loader_level1_stage2_code.bin

					cat reserve3.bin				>>${output_file}
					cat loader_stage2.bin           >>${output_file}
					cat loader_stage2.sig           >>${output_file}

					rm reserve3.bin
					rm loader_stage2.bin
					rm loader_stage2.sig
				else
					#   stage2_bin
					#   fill_zero
					#   char sig_key[256];
					read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_START}
					BOOTLOADER_SIZE=`printf '%d' $BOOTLOADER_SIZE`
					INPUT_FILE_SIZE=`ls -l $input_file | awk '{print $5}'`
					FILL_ZERO_SIZE=`echo "$BOOTLOADER_SIZE-$INPUT_FILE_SIZE-256"|bc`
					if [ "$FILL_ZERO_SIZE" -le 0 ];then
						echo "error:region 'stage2' overflowed!"
						rm loader_stage2.bin
						exit 1
					fi
					zero_to_bin loader_stage2_fill_zero.bin ${FILL_ZERO_SIZE}  # fill zero					
					cat loader_stage2.bin > TMP_FILE1
					cat loader_stage2_fill_zero.bin>> TMP_FILE1
					rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_stage2.sig ${SIGN_ALGO}
					mv TMP_FILE1 ${OUTPUT_DIR}/loader_level1_stage2_code.bin

					cat loader_stage2.bin           >>${output_file}
					cat loader_stage2_fill_zero.bin >>${output_file}
					cat loader_stage2.sig           >>${output_file}

					rm loader_stage2.bin
					rm loader_stage2_fill_zero.bin
					rm loader_stage2.sig
				fi

				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					rm ${OUTPUT_DIR}/loader_level1_stage1_code.bin
					rm ${OUTPUT_DIR}/loader_level1_stage2_code.bin
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level1.boot
				else
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level1.bin
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			elif [ "${CHIP}" = "sirius" ];then
				txt_to_bin1 ${MARKET_ID_FILE} market_id.bin
				MARK_ID_NUM=`ls -l market_id.bin | awk '{print $5}'`
				STAGE1_DATA_NUM=`echo "${LOADER_STAGE1_LEN} - ${MAGIC_NUM} - ${RESERVE1_NUM} - ${ND_KEY_NUM} - ${MARK_ID_NUM} - ${SIGN_NUM} - ${SIGN_NUM} - ${LOADER_STAGE1_DECRYPT_KEY_LENGTH} - ${LOADER_STAGE1_DECRYPT_FLAG_LENGTH} - ${RESERVE2_NUM} - ${CRC_NUM}"|bc`

				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM}
				read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
				read_bin ${input_file} loader_stage1.bin ${LOADER_STAGE1_START} ${STAGE1_DATA_NUM}
				zero_to_bin 2nd_key.sig ${SIGN_NUM}
				zero_to_bin 2nd_key.bin ${SIGN_NUM}
				read_bin ${input_file} loader_stage1_decrypt_key.bin ${LOADER_STAGE1_DECRYPT_KEY_START} ${LOADER_STAGE1_DECRYPT_KEY_LENGTH}
				read_bin ${input_file} loader_stage1_decrypt_flag.bin ${LOADER_STAGE1_DECRYPT_FLAG_START} ${LOADER_STAGE1_DECRYPT_FLAG_LENGTH}
				zero_to_bin reserve2.bin ${RESERVE2_NUM}
				zero_to_bin crc_num.bin ${CRC_NUM}

				cat loader_stage1.bin > TMP_FILE1
				cat 2nd_key.bin      >> TMP_FILE1
				cat market_id.bin    >> TMP_FILE1
				cat 2nd_key.sig      >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_stage1.sig ${SIGN_ALGO}
				mv TMP_FILE1 ${OUTPUT_DIR}/loader_level1_stage1_code.bin

				cat magic_num.bin      >${output_file}
				cat reserve1.bin      >>${output_file}
				cat loader_stage1.bin >>${output_file}
				cat 2nd_key.bin       >>${output_file}
				cat market_id.bin     >>${output_file}
				cat 2nd_key.sig       >>${output_file}
				cat loader_stage1.sig >>${output_file}
				cat loader_stage1_decrypt_key.bin  >> ${output_file}
				cat loader_stage1_decrypt_flag.bin >> ${output_file}
				cat reserve2.bin      >>${output_file}
				cat crc_num.bin       >>${output_file}

				rm magic_num.bin
				rm reserve1.bin
				rm loader_stage1.bin
				rm 2nd_key.sig
				rm 2nd_key.bin
				rm loader_stage1_decrypt_key.bin
				rm loader_stage1_decrypt_flag.bin
				rm reserve2.bin
				rm loader_stage1.sig
				rm crc_num.bin

				read_bin ${input_file} reserve3.bin ${LOADER_STAGE2_START} ${RESERVE3_NUM}
				LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE3_NUM}"|bc`
				read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}

				#   stage2_bin
				#   char sig_key[256];
				cat loader_stage2.bin > TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_stage2.sig ${SIGN_ALGO}
				mv TMP_FILE1 ${OUTPUT_DIR}/loader_level1_stage2_code.bin

				cat reserve3.bin				>>${output_file}
				cat loader_stage2.bin           >>${output_file}
				cat loader_stage2.sig           >>${output_file}

				rm reserve3.bin
				rm loader_stage2.bin
				rm loader_stage2.sig

				mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level1.${SIGN_FILE##*.}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					rm ${OUTPUT_DIR}/loader_level1_stage1_code.bin
					rm ${OUTPUT_DIR}/loader_level1_stage2_code.bin
				else
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			elif [ "${CHIP}" = "gx3113c" ];then
				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM} 
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
					read_bin ${input_file} loader_stage1.bin $((${MAGIC_NUM} + ${RESERVE1_NUM})) 3700 #0xe77-${MAGIC_NUM}+1
				else
					read_bin ${input_file} loader_stage1.bin ${MAGIC_NUM} 3700 #0xe77-${MAGIC_NUM}+1
				fi
				txt_to_bin ${REG_CFG_FILE} reg_cfg_file.bin
				txt_to_bin1 ${MARKET_ID_FILE} market_id.bin
				zero_to_bin crc_num.bin ${CRC_NUM}

				cat loader_stage1.bin > TMP_FILE1
				cat reg_cfg_file.bin >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_stage1.sig ${SIGN_ALGO}
				rm TMP_FILE1

				cat magic_num.bin      >${output_file}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					cat reserve1.bin  >>${output_file}
					rm reserve1.bin
				fi
				cat loader_stage1.bin >>${output_file}
				cat reg_cfg_file.bin  >>${output_file}
				cat market_id.bin     >>${output_file}
				cat loader_stage1.sig >>${output_file}
				cat crc_num.bin       >>${output_file}

				rm magic_num.bin
				rm loader_stage1.bin
				rm reg_cfg_file.bin
				rm loader_stage1.sig
				rm crc_num.bin

				read_bin ${input_file} reserve3.bin ${LOADER_STAGE2_START} ${RESERVE3_NUM}
				LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE3_NUM}"|bc`
				read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}

				#   stage2_bin
				#   char sig_key[256];
				cat loader_stage2.bin > TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} loader_stage2.sig ${SIGN_ALGO}
				rm TMP_FILE1

				cat reserve3.bin				>>${output_file}
				cat loader_stage2.bin           >>${output_file}
				cat loader_stage2.sig           >>${output_file}

				rm reserve3.bin
				rm loader_stage2.bin
				rm loader_stage2.sig

				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level1.boot
				else
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level1.bin
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			fi
			;;
		2) #二级高安
			if [ "${CHIP}" = "gx3211" -o "${CHIP}" = "gx6605s" -o "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" ];then
				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM}
				cat ${SECOND_KEY_MODULUS} > 2nd_key.bin

				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
					read_bin ${input_file} loader_stage1.bin $((${MAGIC_NUM} + ${RESERVE1_NUM})) 7284
					else
						read_bin ${input_file} loader_stage1.bin ${MAGIC_NUM} 7284 #0x1c77-${MAGIC_NUM}+1
					fi
				txt_to_bin ${REG_CFG_FILE} reg_cfg_file.bin
				txt_to_bin1 ${MARKET_ID_FILE} market_id.bin
				zero_to_bin crc_num.bin ${CRC_NUM}

				cat 2nd_key.bin       > TMP_FILE1
				cat market_id.bin     >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} 2nd_key.sig ${SIGN_ALGO}
				rm TMP_FILE1

				cat loader_stage1.bin > TMP_FILE1
				cat reg_cfg_file.bin >> TMP_FILE1
				if [ "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" ];then
				    rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage1.sig RSA_PKCS
				else
				    rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage1.sig ${SIGN_ALGO}
				fi
				mv TMP_FILE1 ${OUTPUT_DIR}/loader_level2_stage1_code.bin

				cat magic_num.bin      >${output_file}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					cat reserve1.bin  >>${output_file}
					rm reserve1.bin
				fi
				cat loader_stage1.bin >>${output_file}
				cat reg_cfg_file.bin  >>${output_file}
				cat 2nd_key.bin       >>${output_file}
				cat market_id.bin     >>${output_file}
				cat 2nd_key.sig       >>${output_file}
				cat loader_stage1.sig >>${output_file}
				cat crc_num.bin       >>${output_file}

				rm magic_num.bin
				rm loader_stage1.bin
				rm reg_cfg_file.bin
				rm 2nd_key.bin
				rm 2nd_key.sig
				rm loader_stage1.sig
				rm crc_num.bin

				if [ "${ENABLE_EXTERN_PARAM}" = "y" ];then
					if [ "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" ];then
						if [ -z "$EXTERN_PARAM_FILE_COUNT" ]; then
							EXTERN_PARAM_FILE_COUNT=1
						fi
						for ((i = 0; i < $EXTERN_PARAM_FILE_COUNT; i++)); do
							offset=$((EXTERN_PARAM_START + i * EXTERN_PARAM_FILE_SIZE))
							read_bin ${input_file} loader_extern_param.bin ${offset} $((${EXTERN_PARAM_FILE_SIZE} - 256))
							cat loader_extern_param.bin > TMP_FILE1
							if [ "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" ];then
								rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_extern_param.sig RSA_PKCS
							else
								rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_extern_param.sig ${SIGN_ALGO}
							fi

							if [ "${i}" -eq 0 ]; then
								mv TMP_FILE1 ${OUTPUT_DIR}/loader_level2_extern_param.bin
							else
								mv TMP_FILE1 ${OUTPUT_DIR}/loader_level2_extern_param_${i}.bin
							fi

							cat loader_extern_param.bin           >>${output_file}
							cat loader_extern_param.sig           >>${output_file}

							rm loader_extern_param.bin
							rm loader_extern_param.sig
						done
					fi
				fi

				if [ "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" -o "${BIN_OR_BOOT}" = "BOOT" ];then
					read_bin ${input_file} reserve3.bin ${LOADER_STAGE2_START} ${RESERVE3_NUM}
					LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE3_NUM}"|bc`
					read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}

					#   stage2_bin
					#   char sig_key[256];
					cat loader_stage2.bin > TMP_FILE1
					if [ "${CHIP}" = "taurus" -o "${CHIP}" = "gemini" ];then
						rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage2.sig RSA_PKCS
					else
						rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage2.sig ${SIGN_ALGO}
					fi
					mv TMP_FILE1 ${OUTPUT_DIR}/loader_level2_stage2_code.bin

					cat reserve3.bin				>>${output_file}
					cat loader_stage2.bin           >>${output_file}
					cat loader_stage2.sig           >>${output_file}

					rm reserve3.bin
					rm loader_stage2.bin
					rm loader_stage2.sig
				else
					#   stage2_bin
					#   fill_zero
					#   char sig_key[256];
					read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_START}
					BOOTLOADER_SIZE=`printf '%d' $BOOTLOADER_SIZE`
					INPUT_FILE_SIZE=`ls -l $input_file | awk '{print $5}'`
					FILL_ZERO_SIZE=`echo "$BOOTLOADER_SIZE-$INPUT_FILE_SIZE-256"|bc`
					if [ "$FILL_ZERO_SIZE" -le 0 ];then
						echo "error:region 'stage2' overflowed!"
						rm loader_stage2.bin
						exit 1
					fi
					zero_to_bin loader_stage2_fill_zero.bin ${FILL_ZERO_SIZE}  # fill zero					
					cat loader_stage2.bin > TMP_FILE1
					cat loader_stage2_fill_zero.bin>> TMP_FILE1
					rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage2.sig ${SIGN_ALGO}
					mv TMP_FILE1 ${OUTPUT_DIR}/loader_level2_stage2_code.bin

					cat loader_stage2.bin           >>${output_file}
					cat loader_stage2_fill_zero.bin >>${output_file}
					cat loader_stage2.sig           >>${output_file}

					rm loader_stage2.bin
					rm loader_stage2_fill_zero.bin
					rm loader_stage2.sig
				fi

				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					rm ${OUTPUT_DIR}/loader_level2_stage1_code.bin
					rm ${OUTPUT_DIR}/loader_level2_stage2_code.bin
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level2.boot
				else
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level2.bin
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			elif [ "${CHIP}" = "sirius" ];then
				txt_to_bin1 ${MARKET_ID_FILE} market_id.bin
				cat ${SECOND_KEY_MODULUS} > 2nd_key.bin
				MARK_ID_NUM=`ls -l market_id.bin | awk '{print $5}'`
				ND_KEY_NUM=`ls -l 2nd_key.bin | awk '{print $5}'`
				STAGE1_DATA_NUM=`echo "${LOADER_STAGE1_LEN} - ${MAGIC_NUM} - ${RESERVE1_NUM} - ${ND_KEY_NUM} - ${MARK_ID_NUM} - ${SIGN_NUM} - ${SIGN_NUM} - ${LOADER_STAGE1_DECRYPT_KEY_LENGTH} - ${LOADER_STAGE1_DECRYPT_FLAG_LENGTH} - ${RESERVE2_NUM} - ${CRC_NUM}"|bc`

				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM}
				read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
				read_bin ${input_file} loader_stage1.bin ${LOADER_STAGE1_START} ${STAGE1_DATA_NUM}
				read_bin ${input_file} loader_stage1_decrypt_key.bin ${LOADER_STAGE1_DECRYPT_KEY_START} ${LOADER_STAGE1_DECRYPT_KEY_LENGTH}
				read_bin ${input_file} loader_stage1_decrypt_flag.bin ${LOADER_STAGE1_DECRYPT_FLAG_START} ${LOADER_STAGE1_DECRYPT_FLAG_LENGTH}
				zero_to_bin reserve2.bin ${RESERVE2_NUM}
				zero_to_bin crc_num.bin ${CRC_NUM}

				cat 2nd_key.bin >  TMP_FILE1
				if [ "${SIGN_ALGO}" = "RSA_PKCS" ];then
					cat market_id.bin >> TMP_FILE1
				fi
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} 2nd_key.sig ${SIGN_ALGO}
				rm TMP_FILE1

				cat loader_stage1.bin > TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage1.sig RSA_PKCS
				mv TMP_FILE1 ${OUTPUT_DIR}/loader_level2_stage1_code.bin


				cat magic_num.bin      >${output_file}
				cat reserve1.bin      >>${output_file}
				cat loader_stage1.bin >>${output_file}
				cat 2nd_key.bin       >>${output_file}
				cat market_id.bin     >>${output_file}
				cat 2nd_key.sig       >>${output_file}
				cat loader_stage1.sig >>${output_file}
				cat loader_stage1_decrypt_key.bin  >> ${output_file}
				cat loader_stage1_decrypt_flag.bin >> ${output_file}
				cat reserve2.bin      >>${output_file}
				cat crc_num.bin       >>${output_file}

				rm magic_num.bin
				rm reserve1.bin
				rm loader_stage1.bin
				rm 2nd_key.bin
				rm 2nd_key.sig
				rm loader_stage1.sig
				rm loader_stage1_decrypt_key.bin
				rm loader_stage1_decrypt_flag.bin
				rm reserve2.bin
				rm crc_num.bin

				read_bin ${input_file} reserve3.bin ${LOADER_STAGE2_START} ${RESERVE3_NUM}
				LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE3_NUM}"|bc`
				read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}

				#   stage2_bin
				#   char sig_key[256];
				cat loader_stage2.bin > TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage2.sig RSA_PKCS
				mv TMP_FILE1 ${OUTPUT_DIR}/loader_level2_stage2_code.bin

				cat reserve3.bin				>>${output_file}
				cat loader_stage2.bin           >>${output_file}
				cat loader_stage2.sig           >>${output_file}

				rm reserve3.bin
				rm loader_stage2.bin
				rm loader_stage2.sig

				mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level2.${SIGN_FILE##*.}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					rm ${OUTPUT_DIR}/loader_level2_stage1_code.bin
					rm ${OUTPUT_DIR}/loader_level2_stage2_code.bin
				else
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			elif [ "${CHIP}" = "vega" ];then
				txt_to_bin1 ${DTE_Boot_Code_Area_market_id_FILE} DTE_Boot_Code_Area_market_id.bin
				txt_to_bin1 ${DTE_Boot_Code_Area_market_id_mask_FILE} DTE_Boot_Code_Area_market_id_mask.bin
				txt_to_bin1 ${DTE_Boot_Code_Area_version_FILE} DTE_Boot_Code_Area_version.bin
				txt_to_bin1 ${DTE_Boot_Code_Area_version_mask_FILE} DTE_Boot_Code_Area_version_mask.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_REE_market_id_FILE} SCS_Auxiliary_Code_Area_REE_market_id.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_REE_market_id_mask_FILE} SCS_Auxiliary_Code_Area_REE_market_id_mask.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_REE_version_FILE} SCS_Auxiliary_Code_Area_REE_version.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_REE_version_mask_FILE} SCS_Auxiliary_Code_Area_REE_version_mask.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_TEE_market_id_FILE} SCS_Auxiliary_Code_Area_TEE_market_id.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_TEE_market_id_mask_FILE} SCS_Auxiliary_Code_Area_TEE_market_id_mask.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_TEE_version_FILE} SCS_Auxiliary_Code_Area_TEE_version.bin
				txt_to_bin1 ${SCS_Auxiliary_Code_Area_TEE_version_mask_FILE} SCS_Auxiliary_Code_Area_TEE_version_mask.bin
				txt_to_bin1 ${SCS_NOCS_Certificate_market_id_FILE} SCS_NOCS_Certificate_market_id.bin
				txt_to_bin1 ${SCS_NOCS_Certificate_market_id_mask_FILE} SCS_NOCS_Certificate_market_id_mask.bin
				txt_to_bin1 ${SCS_NOCS_Certificate_version_FILE} SCS_NOCS_Certificate_version.bin
				txt_to_bin1 ${SCS_NOCS_Certificate_version_mask_FILE} SCS_NOCS_Certificate_version_mask.bin
				txt_to_bin1 ${SCS_Params_Area_market_id_FILE} SCS_Params_Area_market_id.bin
				txt_to_bin1 ${SCS_Params_Area_market_id_mask_FILE} SCS_Params_Area_market_id_mask.bin
				txt_to_bin1 ${SCS_Params_Area_version_FILE} SCS_Params_Area_version.bin
				txt_to_bin1 ${SCS_Params_Area_version_mask_FILE} SCS_Params_Area_version_mask.bin
				txt_to_bin1 ${TEE_Certificate_market_id_FILE} TEE_Certificate_market_id.bin
				txt_to_bin1 ${TEE_Certificate_market_id_mask_FILE} TEE_Certificate_market_id_mask.bin
				txt_to_bin1 ${TEE_Certificate_version_FILE} TEE_Certificate_version.bin
				txt_to_bin1 ${TEE_Certificate_version_mask_FILE} TEE_Certificate_version_mask.bin
				txt_to_bin1 ${TEE_Loader_Area_market_id_FILE} TEE_Loader_Area_market_id.bin
				txt_to_bin1 ${TEE_Loader_Area_market_id_mask_FILE} TEE_Loader_Area_market_id_mask.bin
				txt_to_bin1 ${TEE_Loader_Area_version_FILE} TEE_Loader_Area_version.bin
				txt_to_bin1 ${TEE_Loader_Area_version_mask_FILE} TEE_Loader_Area_version_mask.bin
				txt_to_bin1 ${TEE_Params_Area_market_id_FILE} TEE_Params_Area_market_id.bin
				txt_to_bin1 ${TEE_Params_Area_market_id_mask_FILE} TEE_Params_Area_market_id_mask.bin
				txt_to_bin1 ${TEE_Params_Area_version_FILE} TEE_Params_Area_version.bin
				txt_to_bin1 ${TEE_Params_Area_version_mask_FILE} TEE_Params_Area_version_mask.bin
				txt_to_bin ${KEY_RIGHT_FILE} key_right.bin
				txt_to_bin ${TEE_KEY_RIGHT_FILE} tee_key_right.bin
				txt_to_bin1 ${KEY_LADDER_CONSTANT1_FILE} key_ladder_constant1.bin
				txt_to_bin1 ${KEY_LADDER_CONSTANT2_FILE} key_ladder_constant2.bin
				txt_to_bin1 ${KEY_LADDER_CONSTANT3_FILE} key_ladder_constant3.bin
				txt_to_bin1 ${TEE_KEY_LADDER_CONSTANT1_FILE} tee_key_ladder_constant1.bin
				txt_to_bin1 ${TEE_KEY_LADDER_CONSTANT2_FILE} tee_key_ladder_constant2.bin
				txt_to_bin1 ${TEE_KEY_LADDER_CONSTANT3_FILE} tee_key_ladder_constant3.bin
				txt_to_bin1 ${LOADER_STAGE2_ENCRYPT_KEY_FILE} loader_stage2_encrypt_key.bin
				txt_to_bin1 ${TEE_LOADER_STAGE2_ENCRYPT_KEY_FILE} tee_loader_stage2_encrypt_key.bin
				cat ${SECOND_KEY_MODULUS} > 2nd_key.bin
				cat ${TEE_SECOND_KEY_MODULUS} > tee_2nd_key.bin
				STAGE1_DATA_NUM=10336
				LOADER_STAGE1_START=48

				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM}
				read_bin ${input_file} reserve1.bin ${MAGIC_NUM} 28
				read_bin ${input_file} loader_stage1.bin ${LOADER_STAGE1_START} ${STAGE1_DATA_NUM}
				zero_to_bin crc_num.bin ${CRC_NUM}

				# SCS_Auxiliary_Code
				cat loader_stage1.bin > TMP_FILE1
				cat SCS_Auxiliary_Code_Area_REE_market_id.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_REE_market_id_mask.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_REE_version.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_REE_version_mask.bin >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} REE_SCS_Auxiliary_Code.sig RSA_PKCS
				rm TMP_FILE1

				cat SCS_Auxiliary_Code_Area_TEE_market_id.bin > TMP_FILE1
				cat SCS_Auxiliary_Code_Area_TEE_market_id_mask.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_TEE_version.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_TEE_version_mask.bin >> TMP_FILE1
				cat loader_stage1.bin >> TMP_FILE1
				rsa TMP_FILE1 ${TEE_PRIVATE_2NDKEY_FILE} TEE_SCS_Auxiliary_Code.sig RSA_PKCS
				rm TMP_FILE1

				if [ "${STAGE1_ENCRYPT}" = "1" ];then
					stage_file_len=`ls -l loader_stage1.bin | awk '{print $5}'`
					reserve_len=$((${stage_file_len} % 16))
					data_len=$((${stage_file_len} - ${reserve_len}))
					read_bin loader_stage1.bin loader_stage1_to_en.bin 0 ${data_len}
					read_bin loader_stage1.bin loader_stage1_reserve.bin ${data_len}
					data_encrypt aes128 loader_stage1_to_en.bin loader_stage1_en.bin ${LOADER_STAGE1_ENCRYPT_KEY} cbc
				fi

				cat SCS_Auxiliary_Code_Area_TEE_market_id.bin > TMP_FILE1
				cat SCS_Auxiliary_Code_Area_TEE_market_id_mask.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_TEE_version.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_TEE_version_mask.bin >> TMP_FILE1
				if [ "${STAGE1_ENCRYPT}" = "1" ];then
					cat loader_stage1_en.bin >> TMP_FILE1
					cat loader_stage1_reserve.bin >> TMP_FILE1
				else
				       cat loader_stage1.bin >> TMP_FILE1
				fi
				cat SCS_Auxiliary_Code_Area_REE_market_id.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_REE_market_id_mask.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_REE_version.bin >> TMP_FILE1
				cat SCS_Auxiliary_Code_Area_REE_version_mask.bin >> TMP_FILE1

				cat REE_SCS_Auxiliary_Code.sig >> TMP_FILE1
				cat TEE_SCS_Auxiliary_Code.sig >> TMP_FILE1
				rm REE_SCS_Auxiliary_Code.sig
				rm TEE_SCS_Auxiliary_Code.sig
				mv TMP_FILE1 ${OUTPUT_DIR}/SCS_Auxiliary_Code.bin

				# SCS_NOCS_Certificate
				cat 2nd_key.bin >  TMP_FILE1
				cat key_right.bin >> TMP_FILE1
				cat key_ladder_constant1.bin >> TMP_FILE1
				cat key_ladder_constant2.bin >> TMP_FILE1
				cat key_ladder_constant3.bin >> TMP_FILE1
				cat SCS_NOCS_Certificate_market_id.bin >> TMP_FILE1
				cat SCS_NOCS_Certificate_market_id_mask.bin >> TMP_FILE1
				cat SCS_NOCS_Certificate_version.bin >> TMP_FILE1
				cat SCS_NOCS_Certificate_version_mask.bin >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_KEY_FILE} SCS_NOCS_Certificate.sig ${SIGN_ALGO}
				cat SCS_NOCS_Certificate.sig >> TMP_FILE1
				mv TMP_FILE1 ${OUTPUT_DIR}/SCS_NOCS_Certificate.bin
				rm SCS_NOCS_Certificate.sig

				# SCS_Params_Area
				if [ "${STAGE2_ENCRYPT}" = "1" ];then
					echo "11111111" > TMP_FILE1
					xxd -c4 -p -r TMP_FILE1 loader_stage2_decrypt_flag.bin
					rm TMP_FILE1
				else
					zero_to_bin loader_stage2_decrypt_flag.bin 4
					if [ "${ENABLE_NAGRA_MODE}" != "y" ];then
						zero_to_bin loader_stage2_encrypt_key.bin 16
					fi
				fi
				read_bin ${input_file} ree_param.bin 11512 2696

				cat loader_stage2_encrypt_key.bin > TMP_FILE1
				cat loader_stage2_decrypt_flag.bin >> TMP_FILE1
				cat ree_param.bin >> TMP_FILE1
				cat SCS_Params_Area_market_id.bin >> TMP_FILE1
				cat SCS_Params_Area_market_id_mask.bin >> TMP_FILE1
				cat SCS_Params_Area_version.bin >> TMP_FILE1
				cat SCS_Params_Area_version_mask.bin >> TMP_FILE1

				rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} SCS_Params_Area.sig RSA_PKCS

				if [ "${STAGE2_ENCRYPT}" = "1" ] || [ "${ENABLE_NAGRA_MODE}" = "y" ];then
					echo ${LOADER_STAGE2_ENCRYPT_KEY} > KEY_FILE
					xxd -c4 -p -r KEY_FILE TEMP_FILE
					# 对加密数据的KEY进行加密
					data_encrypt aes128 TEMP_FILE TEMP_FILE_ENCRYPT ${LOADER_STAGE2_ENCRYPT_DERIVE_KEY} ecb
					mv TEMP_FILE_ENCRYPT loader_stage2_encrypt_key.bin
					rm KEY_FILE
					rm TEMP_FILE
				fi

				cat loader_stage2_encrypt_key.bin > TMP_FILE1
				cat loader_stage2_decrypt_flag.bin >> TMP_FILE1
				cat ree_param.bin >> TMP_FILE1
				cat SCS_Params_Area_market_id.bin >> TMP_FILE1
				cat SCS_Params_Area_market_id_mask.bin >> TMP_FILE1
				cat SCS_Params_Area_version.bin >> TMP_FILE1
				cat SCS_Params_Area_version_mask.bin >> TMP_FILE1
				cat SCS_Params_Area.sig >> TMP_FILE1
				mv TMP_FILE1 ${OUTPUT_DIR}/SCS_Params_Area.bin
				rm SCS_Params_Area.sig

				# TEE Certificate
				cat tee_2nd_key.bin >  TMP_FILE1
				cat tee_key_right.bin >> TMP_FILE1
				cat tee_key_ladder_constant1.bin >> TMP_FILE1
				cat tee_key_ladder_constant2.bin >> TMP_FILE1
				cat tee_key_ladder_constant3.bin >> TMP_FILE1
				cat TEE_Certificate_market_id.bin >> TMP_FILE1
				cat TEE_Certificate_market_id_mask.bin >> TMP_FILE1
				cat TEE_Certificate_version.bin >> TMP_FILE1
				cat TEE_Certificate_version_mask.bin >> TMP_FILE1
				rsa TMP_FILE1 ${TEE_PRIVATE_KEY_FILE} TEE_Certificate.sig ${SIGN_ALGO}
				cat TEE_Certificate.sig >> TMP_FILE1
				mv TMP_FILE1 ${OUTPUT_DIR}/TEE_Certificate.bin
				rm TEE_Certificate.sig

				# TEE_Params_Area
				if [ "${STAGE2_ENCRYPT}" = "1" ];then
					echo "11111111" > TMP_FILE1
					xxd -c4 -p -r TMP_FILE1 tee_loader_stage2_decrypt_flag.bin
					rm TMP_FILE1
				else
					zero_to_bin tee_loader_stage2_decrypt_flag.bin 4
					if [ "${ENABLE_NAGRA_MODE}" != "y" ];then
						zero_to_bin tee_loader_stage2_encrypt_key.bin 16
					fi
				fi
				read_bin ${input_file} tee_param.bin 15080 1028

				cat tee_loader_stage2_encrypt_key.bin > TMP_FILE1
				cat tee_loader_stage2_decrypt_flag.bin >> TMP_FILE1
				cat tee_param.bin >> TMP_FILE1
				cat TEE_Params_Area_market_id.bin >> TMP_FILE1
				cat TEE_Params_Area_market_id_mask.bin >> TMP_FILE1
				cat TEE_Params_Area_version.bin >> TMP_FILE1
				cat TEE_Params_Area_version_mask.bin >> TMP_FILE1

				rsa TMP_FILE1 ${TEE_PRIVATE_2NDKEY_FILE} TEE_Params_Area.sig RSA_PKCS

				if [ "${STAGE2_ENCRYPT}" = "1" ] || [ "${ENABLE_NAGRA_MODE}" = "y" ];then
					echo ${TEE_LOADER_STAGE2_ENCRYPT_KEY} > KEY_FILE
					xxd -c4 -p -r KEY_FILE TEMP_FILE
					# 对加密数据的KEY进行加密
					data_encrypt aes128 TEMP_FILE TEMP_FILE_ENCRYPT ${TEE_LOADER_STAGE2_ENCRYPT_DERIVE_KEY} ecb
					mv TEMP_FILE_ENCRYPT tee_loader_stage2_encrypt_key.bin
					rm KEY_FILE
					rm TEMP_FILE
				fi
				cat tee_loader_stage2_encrypt_key.bin > TMP_FILE1
				cat tee_loader_stage2_decrypt_flag.bin >> TMP_FILE1
				cat tee_param.bin >> TMP_FILE1
				cat TEE_Params_Area_market_id.bin >> TMP_FILE1
				cat TEE_Params_Area_market_id_mask.bin >> TMP_FILE1
				cat TEE_Params_Area_version.bin >> TMP_FILE1
				cat TEE_Params_Area_version_mask.bin >> TMP_FILE1
				cat TEE_Params_Area.sig >> TMP_FILE1
				mv TMP_FILE1 ${OUTPUT_DIR}/TEE_Params_Area.bin
				rm TEE_Params_Area.sig

				cat magic_num.bin      >${output_file}
				cat reserve1.bin      >>${output_file}
				cat ${OUTPUT_DIR}/SCS_Auxiliary_Code.bin >>${output_file}
				cat ${OUTPUT_DIR}/SCS_NOCS_Certificate.bin >>${output_file}
				cat ${OUTPUT_DIR}/SCS_Params_Area.bin >>${output_file}
				cat ${OUTPUT_DIR}/TEE_Certificate.bin >>${output_file}
				cat ${OUTPUT_DIR}/TEE_Params_Area.bin >>${output_file}
				cat crc_num.bin     >>${output_file}

				rm magic_num.bin
				rm reserve1.bin
				rm loader_stage1.bin
				rm crc_num.bin

				if [ "${ENABLE_NAGRA_MODE}" = "y" ];then
					read_bin ${input_file} unchecked_area.bin 16384 4096
					cat unchecked_area.bin >> ${output_file}
					LOADER_STAGE2_FACT_START=20480 #16384 + 4096
					stage_file_len=`ls -l ${input_file} | awk '{print $5}'`
					data_len=$((${stage_file_len} - ${LOADER_STAGE2_FACT_START} - 256 - 16))
					read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START} ${data_len}
				else
					LOADER_STAGE2_FACT_START=16384
					stage_file_len=`ls -l ${input_file} | awk '{print $5}'`
					data_len=$((${stage_file_len} - ${LOADER_STAGE2_FACT_START} - 256 - 16))
					read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START} ${data_len}
				fi

				#   stage2_bin
				#   char sig_key[256];
				# DTE_Boot_Code_Area
				cat loader_stage2.bin > TMP_FILE1
				cat DTE_Boot_Code_Area_market_id.bin >> TMP_FILE1
				cat DTE_Boot_Code_Area_market_id_mask.bin >> TMP_FILE1
				cat DTE_Boot_Code_Area_version.bin >> TMP_FILE1
				cat DTE_Boot_Code_Area_version_mask.bin >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage2.sig RSA_PKCS

				if [ "${STAGE2_ENCRYPT}" = "1" ];then
					stage_file_len=`ls -l loader_stage2.bin | awk '{print $5}'`
					reserve_len=$((${stage_file_len} % 16))
					data_len=$((${stage_file_len} - ${reserve_len}))
					read_bin loader_stage2.bin loader_stage2_to_en.bin 0 ${data_len}
					read_bin loader_stage2.bin loader_stage2_reserve.bin ${data_len}
					data_encrypt aes128 loader_stage2_to_en.bin loader_stage2_en.bin ${LOADER_STAGE2_ENCRYPT_KEY} cbc
					rm loader_stage2_to_en.bin
				fi

				if [ "${STAGE2_ENCRYPT}" = "1" ];then
					cat loader_stage2_en.bin > TMP_FILE1
					cat loader_stage2_reserve.bin >> TMP_FILE1
					#rm loader_stage2_en.bin
					rm loader_stage2_reserve.bin
				else
					cat loader_stage2.bin > TMP_FILE1
				fi
				cat DTE_Boot_Code_Area_market_id.bin >> TMP_FILE1
				cat DTE_Boot_Code_Area_market_id_mask.bin >> TMP_FILE1
				cat DTE_Boot_Code_Area_version.bin >> TMP_FILE1
				cat DTE_Boot_Code_Area_version_mask.bin >> TMP_FILE1
				cp TMP_FILE1 ${OUTPUT_DIR}/loader_level2_stage2_code.bin
				cat loader_stage2.sig >> TMP_FILE1

				cat TMP_FILE1 >> ${output_file}

				rm TMP_FILE1
				rm loader_stage2.bin
				rm loader_stage2.sig

				# TEE_Loader_Area
				if [ "${CONFIG_REE_LOADER}" = "y" ];then
					stage_file_len=`ls -l ${INPUT_DIR}/tee-loader-flash.bin | awk '{print $5}'`
					data_len=$((${stage_file_len} - 256 - 16))
					read_bin ${INPUT_DIR}/tee-loader-flash.bin tee-loader.bin 0 ${data_len}

					cat tee-loader.bin > TMP_FILE1
					cat TEE_Loader_Area_market_id.bin >> TMP_FILE1
					cat TEE_Loader_Area_market_id_mask.bin >> TMP_FILE1
					cat TEE_Loader_Area_version.bin >> TMP_FILE1
					cat TEE_Loader_Area_version_mask.bin >> TMP_FILE1

					rsa TMP_FILE1 ${TEE_PRIVATE_2NDKEY_FILE} tee-loader.sig RSA_PKCS
					if [ "${STAGE2_ENCRYPT}" = "1" ];then
						stage_file_len=`ls -l tee-loader.bin | awk '{print $5}'`
						reserve_len=$((${stage_file_len} % 16))
						data_len=$((${stage_file_len} - ${reserve_len}))
						read_bin tee-loader.bin tee-loader_to_en.bin 0 ${data_len}
						read_bin tee-loader.bin tee-loader_reserve.bin ${data_len}
						data_encrypt aes128 tee-loader_to_en.bin tee-loader_en.bin ${TEE_LOADER_STAGE2_ENCRYPT_KEY} cbc
						rm tee-loader_to_en.bin
					fi
					if [ "${STAGE2_ENCRYPT}" = "1" ];then
						cat tee-loader_en.bin > TMP_FILE1
						cat tee-loader_reserve.bin >> TMP_FILE1
						rm tee-loader_en.bin
						rm tee-loader_reserve.bin
					else
						cat tee-loader.bin > TMP_FILE1
					fi
					cat TEE_Loader_Area_market_id.bin >> TMP_FILE1
					cat TEE_Loader_Area_market_id_mask.bin >> TMP_FILE1
					cat TEE_Loader_Area_version.bin >> TMP_FILE1
					cat TEE_Loader_Area_version_mask.bin >> TMP_FILE1
					cat tee-loader.sig >> TMP_FILE1
					cp TMP_FILE1 ${OUTPUT_DIR}/tee-loader-level2.bin
					rm TMP_FILE1
					rm tee-loader.bin
					rm tee-loader.sig
				fi

				mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level2.${SIGN_FILE##*.}

				rm *.bin
			elif [ "${CHIP}" = "gx3113c" ];then
				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM} 
				cat ${SECOND_KEY_MODULUS} > 2nd_key.bin
				rsa 2nd_key.bin ${PRIVATE_KEY_FILE} 2nd_key.sig ${SIGN_ALGO}

				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
					read_bin ${input_file} loader_stage1.bin $((${MAGIC_NUM} + ${RESERVE1_NUM})) 3188 #0xc77-${MAGIC_NUM}+1
				else
					read_bin ${input_file} loader_stage1.bin ${MAGIC_NUM} 3188 #0xc77-${MAGIC_NUM}+1
				fi
				txt_to_bin ${REG_CFG_FILE} reg_cfg_file.bin
				txt_to_bin1 ${MARKET_ID_FILE} market_id.bin
				zero_to_bin crc_num.bin ${CRC_NUM}

				cat loader_stage1.bin > TMP_FILE1
				cat reg_cfg_file.bin >> TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage1.sig ${SIGN_ALGO}
				rm TMP_FILE1

				cat magic_num.bin      >${output_file}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					cat reserve1.bin  >>${output_file}
					rm reserve1.bin
				fi
				cat loader_stage1.bin >>${output_file}
				cat reg_cfg_file.bin  >>${output_file}
				cat market_id.bin     >>${output_file}
				cat 2nd_key.bin       >>${output_file}
				cat 2nd_key.sig       >>${output_file}
				cat loader_stage1.sig >>${output_file}
				cat crc_num.bin       >>${output_file}

				rm magic_num.bin
				rm loader_stage1.bin
				rm reg_cfg_file.bin
				rm 2nd_key.bin
				rm 2nd_key.sig
				rm loader_stage1.sig
				rm crc_num.bin


				read_bin ${input_file} reserve3.bin ${LOADER_STAGE2_START} ${RESERVE3_NUM}
				LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE3_NUM}"|bc`
				read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}

				#   stage2_bin
				#   char sig_key[256];
				cat loader_stage2.bin > TMP_FILE1
				rsa TMP_FILE1 ${PRIVATE_2NDKEY_FILE} loader_stage2.sig RSA_PKCS
				rm TMP_FILE1

				cat reserve3.bin				>>${output_file}
				cat loader_stage2.bin           >>${output_file}
				cat loader_stage2.sig           >>${output_file}

				rm reserve3.bin
				rm loader_stage2.bin
				rm loader_stage2.sig

				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level2.boot
				else
					mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level2.bin
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			fi
			;;
		*)
			echo "ERROR parameter : ${SECURE_LEVEL}"
			exit 1
			;;
	esac
}

main()
{
	if [ ${BOOTLOADER} -eq 1 ];then
		sign_for_bootloader_file
	else
		sign_for_other_file
	fi
}

main
