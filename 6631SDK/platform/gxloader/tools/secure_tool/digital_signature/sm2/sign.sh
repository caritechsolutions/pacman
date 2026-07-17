#!/bin/bash

source ${SIGN_CUR_DIR}/common_fun.sh

TOOL_TYPE=tool
PROGRAM_TYPE=program
SM2_CUR_PWD=`pwd`
SM2_KEY_DIR=${SM2_CUR_PWD}/key
SM2_CONFIG_DIR=${SM2_CUR_PWD}/config
SM2_FIXED_VALUE=fixed_to_sign.bin
SM2_IDA_VALUE=IDA.bin
SM2_IDA_RESERVE_VALUE=IDA_RESERVE.bin
SM2_ENTLA_VALUE=ENTLA.bin
GMSSL_TOOL=gmssl

RESERVE1_NUM=28
RESERVE2_NUM=32
RESERVE3_NUM=192
RESERVE4_NUM=192
RESERVE5_NUM=20
RESERVE6_NUM=192
RESERVE7_NUM=32
SM2_IDA_NUM=158
SM2_ENTLA_NUM=2
SM2_COORDINATE_NUM=32
SM2_SIGN_NUM=32

sign_fill()
{
    sign_file=$1
    sign_len=`ls -l ${sign_file} | awk '{print $5}'`
    fill_len=`expr 32 - $sign_len`

    for((i=1;i<=$fill_len;i++));
    do
        xxd -p -c32 ${sign_file} > temp
        sed -e '1i00' temp > temp_1
        xxd -p -r temp_1 > ${sign_file}
        rm temp
        rm temp_1
    done

}

sm2_r_s_program()
{
	local private_key=$1
	local input_file=$2
	local output_file_r=$3
	local output_file_s=$4
	local output_file=${LINENO}

	${GMSSL_TOOL} pkeyutl -sign -pkeyopt ec_scheme:sm2 -inkey ${private_key} -in ${input_file} -out ${output_file}
	gcc ${SM2_CUR_PWD}/create_r_s.c -L/usr/local/lib -lcrypto -o create_r_s
	./create_r_s ${output_file} ${output_file}_R ${output_file}_S
	txt_to_bin1 ${output_file}_R ${output_file_r}
	txt_to_bin1 ${output_file}_S ${output_file_s}

	rm create_r_s
	rm ${output_file}
	rm ${output_file}_R
	rm ${output_file}_S
}

sm2_r_s_tool()
{
	local private_key=$1
	local input_file=$2
	local output_file_r=$3
	local output_file_s=$4
	local output_file=${LINENO}

	${GMSSL_TOOL} pkeyutl -sign -asn1parse -pkeyopt ec_scheme:sm2 -inkey ${private_key} -in ${input_file} -out ${output_file}
	sed -n "2p" ${output_file} | cut -d : -f 4 > ${output_file}_R
	sed -n "3p" ${output_file} | cut -d : -f 4 > ${output_file}_S
	txt_to_bin1 ${output_file}_R ${output_file_r}
	txt_to_bin1 ${output_file}_S ${output_file_s}
	sign_fill ${output_file_r}
	sign_fill ${output_file_s}

	rm ${output_file}
	rm ${output_file}_R
	rm ${output_file}_S
}

delete_ec_param()
{
	rm -rf ${SM2_IDA_VALUE}
	rm -rf ${SM2_IDA_RESERVE_VALUE}
	rm -rf ${SM2_ENTLA_VALUE}
	rm -rf ${SM2_FIXED_VALUE}
}

create_ec_param()
{
	delete_ec_param

	txt_to_bin1 ${SM2_CONFIG_DIR}/IDA.txt ${SM2_IDA_VALUE}
	local ida_bytes_len=`ls -l ${SM2_IDA_VALUE} | awk '{print $5}'`
	local ida_reserve_bytes_len=`echo "${SM2_IDA_NUM}-${ida_bytes_len}"|bc`
	local ida_bits_len=$[${ida_bytes_len}*8]
	ida_bits_len=`printf '%04x' ${ida_bits_len}`
	zero_to_bin ${SM2_IDA_RESERVE_VALUE} ${ida_reserve_bytes_len}
	str_to_bin ${ida_bits_len} entla.txt
	txt_to_bin1 entla.txt ${SM2_ENTLA_VALUE}
	txt_to_bin1 ${SM2_CONFIG_DIR}/a.txt A.bin
	txt_to_bin1 ${SM2_CONFIG_DIR}/b.txt B.bin
	txt_to_bin1 ${SM2_CONFIG_DIR}/Gx.txt GX.bin
	txt_to_bin1 ${SM2_CONFIG_DIR}/Gy.txt GY.bin

	cat ${SM2_ENTLA_VALUE}		>   ${SM2_FIXED_VALUE}
	cat ${SM2_IDA_VALUE}		>>  ${SM2_FIXED_VALUE}
	cat A.bin			>>  ${SM2_FIXED_VALUE}
	cat B.bin			>>  ${SM2_FIXED_VALUE}
	cat GX.bin			>>  ${SM2_FIXED_VALUE}
	cat GY.bin			>>  ${SM2_FIXED_VALUE}

	rm entla.txt
	rm -rf A.bin
	rm -rf B.bin
	rm -rf GX.bin
	rm -rf GY.bin
}

sm2()
{
	local private_key=$1
	local input_file=$2
	local output_file_r=$3
	local output_file_s=$4
	local get_r_s_type=$5

	if [ "${get_r_s_type}" = "${TOOL_TYPE}" ];then
		sm2_r_s_tool ${private_key} ${input_file} ${output_file_r} ${output_file_s}
	else
		sm2_r_s_program ${private_key} ${input_file} ${output_file_r} ${output_file_s}
	fi
}

sm3()
{
	local output_hash_file=$1
	local file1_to_hash=${2}
	local file2_to_hash=${3}
	local file3_to_hash=${4}
	local file_to_hash=${LINENO}
	local hash_txt=${LINENO}

	cat ${file1_to_hash}   >  ${file_to_hash}
	cat ${file2_to_hash}   >> ${file_to_hash}
	if [ -n "${file3_to_hash}" ];then
		cat ${file3_to_hash}   >> ${file_to_hash}
	fi

	${GMSSL_TOOL} sm3 -r ${file_to_hash} > ${hash_txt}
	echo `cat ${hash_txt} | awk '{print $1}'` > ${hash_txt}
	txt_to_bin1 ${hash_txt} ${output_hash_file}

	rm -rf ${file_to_hash}
	rm -rf ${hash_txt}
}

sign_for_other_file()
{
	local SUFFIX_R="-sign_r"
	local SUFFIX_S="-sign_s"
	local input_file=${INPUT_DIR}/${SIGN_FILE}
	local output_file_r=${OUTPUT_DIR}/${SIGN_FILE}${SUFFIX_R}
	local output_file_s=${OUTPUT_DIR}/${SIGN_FILE}${SUFFIX_S}

	if [ -f "${output_file_r}" ];then
		rm -rf ${output_file_r}
	fi

	if [ -f "${output_file_s}" ];then
		rm -rf ${output_file_s}
	fi

	local tmp_hash=${LINENO}
	local SM2_ZA_VALUE=${LINENO}
	local PRIVATE_KEY_FILE=${SM2_KEY_DIR}/1nd_key.pri
	local KEY_PUBLIC_X=${SM2_KEY_DIR}/1nd_key_public_x.bin
	local KEY_PUBLIC_Y=${SM2_KEY_DIR}/1nd_key_public_y.bin

	create_ec_param

	sm3 ${SM2_ZA_VALUE} ${SM2_FIXED_VALUE} ${KEY_PUBLIC_X} ${KEY_PUBLIC_Y}
	sm3 ${tmp_hash} ${SM2_ZA_VALUE} ${input_file}
	sm2 ${PRIVATE_KEY_FILE} ${tmp_hash} ${output_file_r} ${output_file_s} ${TOOL_TYPE}

	rm ${SM2_ZA_VALUE}
	rm ${tmp_hash}

	delete_ec_param
}

sign_for_bootloader_file()
{
	local input_file=${INPUT_DIR}/${SIGN_FILE}
	local output_file=${LINENO}

	SM2_ZA_VALUE=za.bin
	KEY1_PRI=1nd_key.pri
	KEY2_PRI=2nd_key.pri
	KEY1_PUBLIC_X=1nd_key_public_x.bin
	KEY1_PUBLIC_Y=1nd_key_public_y.bin
	KEY2_PUBLIC_X=2nd_key_public_x.bin
	KEY2_PUBLIC_Y=2nd_key_public_y.bin
	STAGE1_SIGN_R=stage1_sign_r
	STAGE1_SIGN_S=stage1_sign_s
	STAGE2_SIGN_R=stage2_sign_r
	STAGE2_SIGN_S=stage2_sign_s
	KEY2_PUBLIC_SIGN_R=2nd_key_sign_r
	KEY2_PUBLIC_SIGN_S=2nd_key_sign_s
	PRIVATE_KEY_FILE=${SM2_KEY_DIR}/${KEY1_PRI}
	PRIVATE_2NDKEY_FILE=${SM2_KEY_DIR}/${KEY2_PRI}
	FIRST_KEY_PUBLIC_X=${SM2_KEY_DIR}/${KEY1_PUBLIC_X}
	FIRST_KEY_PUBLIC_Y=${SM2_KEY_DIR}/${KEY1_PUBLIC_Y}
	SECOND_KEY_PUBLIC_X=${SM2_KEY_DIR}/${KEY2_PUBLIC_X}
	SECOND_KEY_PUBLIC_Y=${SM2_KEY_DIR}/${KEY2_PUBLIC_Y}

	create_ec_param
	txt_to_bin1 ${MARKET_ID_FILE} market_id.bin
	MARK_ID_NUM=`ls -l market_id.bin | awk '{print $5}'`
	STAGE1_DATA_NUM=`echo "${LOADER_STAGE1_LEN} - ${MAGIC_NUM} - ${RESERVE1_NUM} - ${SM2_ENTLA_NUM} - ${SM2_IDA_NUM} - ${RESERVE2_NUM} - ${SM2_COORDINATE_NUM} - ${SM2_COORDINATE_NUM} - ${MARK_ID_NUM} - ${SM2_SIGN_NUM} - ${SM2_SIGN_NUM} - ${RESERVE3_NUM} - ${SM2_SIGN_NUM} -${SM2_SIGN_NUM} - ${RESERVE4_NUM} - ${LOADER_STAGE1_DECRYPT_KEY_LENGTH} - ${LOADER_STAGE1_DECRYPT_FLAG_LENGTH} - ${RESERVE5_NUM} - ${CRC_NUM}"|bc`

	case "${SECURE_LEVEL}" in
		1) #一级高安
			if [ "${CHIP}" = "sirius" ];then
				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM}
				read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
				read_bin ${input_file} loader_stage1.bin ${LOADER_STAGE1_START} ${STAGE1_DATA_NUM}
				read_bin ${input_file} loader_stage1_decrypt_key.bin ${LOADER_STAGE1_DECRYPT_KEY_START} ${LOADER_STAGE1_DECRYPT_KEY_LENGTH}
				read_bin ${input_file} loader_stage1_decrypt_flag.bin ${LOADER_STAGE1_DECRYPT_FLAG_START} ${LOADER_STAGE1_DECRYPT_FLAG_LENGTH}
				zero_to_bin ${KEY2_PUBLIC_X} ${SM2_COORDINATE_NUM}
				zero_to_bin ${KEY2_PUBLIC_Y} ${SM2_COORDINATE_NUM}
				zero_to_bin ${KEY2_PUBLIC_SIGN_R} ${SM2_SIGN_NUM}
				zero_to_bin ${KEY2_PUBLIC_SIGN_S} ${SM2_SIGN_NUM}
				zero_to_bin reserve2.bin ${RESERVE2_NUM}
				zero_to_bin reserve3.bin ${RESERVE3_NUM}
				zero_to_bin reserve4.bin ${RESERVE4_NUM}
				zero_to_bin reserve5.bin ${RESERVE5_NUM}
				zero_to_bin crc_num.bin ${CRC_NUM}

				local stage1_plain=${LINENO}
				local stage1_plain_hash=${LINENO}
				sm3 ${SM2_ZA_VALUE} ${SM2_FIXED_VALUE} ${FIRST_KEY_PUBLIC_X} ${FIRST_KEY_PUBLIC_Y}
				cat loader_stage1.bin			>> ${stage1_plain}
				cat ${SM2_ENTLA_VALUE}			>> ${stage1_plain}
				cat ${SM2_IDA_VALUE}			>> ${stage1_plain}
				cat ${SM2_IDA_RESERVE_VALUE}	>> ${stage1_plain}
				cat reserve2.bin				>> ${stage1_plain}
				cat ${KEY2_PUBLIC_X}			>> ${stage1_plain}
				cat ${KEY2_PUBLIC_Y}			>> ${stage1_plain}
				cat market_id.bin				>> ${stage1_plain}
				cat ${KEY2_PUBLIC_SIGN_R}		>> ${stage1_plain}
				cat ${KEY2_PUBLIC_SIGN_S}		>> ${stage1_plain}
				cat reserve3.bin				>> ${stage1_plain}
				sm3 ${stage1_plain_hash} ${SM2_ZA_VALUE} ${stage1_plain}
				sm2 ${PRIVATE_KEY_FILE} ${stage1_plain_hash} ${STAGE1_SIGN_R} ${STAGE1_SIGN_S} ${TOOL_TYPE} 
				mv ${stage1_plain} ${OUTPUT_DIR}/loader_level1_stage1_code.bin
				rm ${stage1_plain_hash}
				rm ${SM2_ZA_VALUE}

				cat magic_num.bin					>  ${output_file}
				cat reserve1.bin					>> ${output_file}
				cat loader_stage1.bin				>> ${output_file}
				cat ${SM2_ENTLA_VALUE}				>> ${output_file}
				cat ${SM2_IDA_VALUE}				>> ${output_file}
				cat ${SM2_IDA_RESERVE_VALUE}		>> ${output_file}
				cat reserve2.bin					>> ${output_file}
				cat ${KEY2_PUBLIC_X}				>> ${output_file}
				cat ${KEY2_PUBLIC_Y}				>> ${output_file}
				cat market_id.bin					>> ${output_file}
				cat ${KEY2_PUBLIC_SIGN_R}			>> ${output_file}
				cat ${KEY2_PUBLIC_SIGN_S}			>> ${output_file}
				cat reserve3.bin					>> ${output_file}
				cat ${STAGE1_SIGN_R}				>> ${output_file}
				cat ${STAGE1_SIGN_S}				>> ${output_file}
				cat reserve4.bin					>> ${output_file}
				cat loader_stage1_decrypt_key.bin   >> ${output_file}
				cat loader_stage1_decrypt_flag.bin  >> ${output_file}
				cat reserve5.bin					>> ${output_file}
				cat crc_num.bin						>> ${output_file}

				rm magic_num.bin
				rm reserve1.bin
				rm reserve2.bin
				rm loader_stage1.bin
				rm ${KEY2_PUBLIC_X}
				rm ${KEY2_PUBLIC_Y}
				rm ${KEY2_PUBLIC_SIGN_R}
				rm ${KEY2_PUBLIC_SIGN_S}
				rm reserve3.bin
				rm ${STAGE1_SIGN_R}
				rm ${STAGE1_SIGN_S}
				rm reserve4.bin
				rm loader_stage1_decrypt_key.bin
				rm loader_stage1_decrypt_flag.bin
				rm reserve5.bin
				rm crc_num.bin

				read_bin ${input_file} reserve7.bin ${LOADER_STAGE2_START} ${RESERVE7_NUM}
				LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE7_NUM}"|bc`
				read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}

				#   stage2_bin
				#   char sig_key[256];

				local stage2_plain=${LINENO}
				local stage2_plain_hash=${LINENO}
				sm3 ${SM2_ZA_VALUE} ${SM2_FIXED_VALUE} ${FIRST_KEY_PUBLIC_X} ${FIRST_KEY_PUBLIC_Y}
				cat loader_stage2.bin           >  ${stage2_plain}
				sm3 ${stage2_plain_hash} ${SM2_ZA_VALUE} ${stage2_plain}
				sm2 ${PRIVATE_KEY_FILE} ${stage2_plain_hash} ${STAGE2_SIGN_R} ${STAGE2_SIGN_S} ${TOOL_TYPE} 
				mv ${stage2_plain} ${OUTPUT_DIR}/loader_level1_stage2_code.bin
				rm ${SM2_ZA_VALUE}
				rm ${stage2_plain_hash}

				cat reserve7.bin				>>${output_file}
				cat loader_stage2.bin           >>${output_file}
				cat ${STAGE2_SIGN_R}            >>${output_file}
				cat ${STAGE2_SIGN_S}            >>${output_file}

				rm reserve7.bin
				rm loader_stage2.bin
				rm ${STAGE2_SIGN_R}
				rm ${STAGE2_SIGN_S}

				mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level1.${SIGN_FILE##*.}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					rm ${OUTPUT_DIR}/loader_level1_stage1_code.bin
					rm ${OUTPUT_DIR}/loader_level1_stage2_code.bin
				else
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			fi
			;;
		2) #二级高安
			if [ "${CHIP}" = "sirius" ];then
				read_bin ${input_file} magic_num.bin 0 ${MAGIC_NUM}
				read_bin ${input_file} reserve1.bin ${MAGIC_NUM} ${RESERVE1_NUM}
				read_bin ${input_file} loader_stage1.bin ${LOADER_STAGE1_START} ${STAGE1_DATA_NUM}
				read_bin ${input_file} loader_stage1_decrypt_key.bin ${LOADER_STAGE1_DECRYPT_KEY_START} ${LOADER_STAGE1_DECRYPT_KEY_LENGTH}
				read_bin ${input_file} loader_stage1_decrypt_flag.bin ${LOADER_STAGE1_DECRYPT_FLAG_START} ${LOADER_STAGE1_DECRYPT_FLAG_LENGTH}
				zero_to_bin reserve2.bin ${RESERVE2_NUM}
				zero_to_bin reserve3.bin ${RESERVE3_NUM}
				zero_to_bin reserve4.bin ${RESERVE4_NUM}
				zero_to_bin reserve5.bin ${RESERVE5_NUM}
				zero_to_bin crc_num.bin ${CRC_NUM}

				local key2_plain=${LINENO}
				local key2_plain_hash=${LINENO}
				sm3 ${SM2_ZA_VALUE} ${SM2_FIXED_VALUE} ${FIRST_KEY_PUBLIC_X} ${FIRST_KEY_PUBLIC_Y}
				cat ${SECOND_KEY_PUBLIC_X}		>> ${key2_plain}
				cat ${SECOND_KEY_PUBLIC_Y}		>> ${key2_plain}
				cat market_id.bin				>> ${key2_plain}
				sm3 ${key2_plain_hash} ${SM2_ZA_VALUE} ${key2_plain}
				sm2 ${PRIVATE_KEY_FILE} ${key2_plain_hash} ${KEY2_PUBLIC_SIGN_R} ${KEY2_PUBLIC_SIGN_S} ${TOOL_TYPE}
				rm ${key2_plain}
				rm ${key2_plain_hash}
				rm ${SM2_ZA_VALUE}

				local stage1_plain=${LINENO}
				local stage1_plain_hash=${LINENO}
				sm3 ${SM2_ZA_VALUE} ${SM2_FIXED_VALUE} ${SECOND_KEY_PUBLIC_X} ${SECOND_KEY_PUBLIC_Y}
				cat loader_stage1.bin > ${stage1_plain}
				sm3 ${stage1_plain_hash} ${SM2_ZA_VALUE} ${stage1_plain}
				sm2 ${PRIVATE_2NDKEY_FILE} ${stage1_plain_hash} ${STAGE1_SIGN_R} ${STAGE1_SIGN_S} ${TOOL_TYPE}
				rm ${stage1_plain_hash}
				rm ${SM2_ZA_VALUE}
				mv ${stage1_plain} ${OUTPUT_DIR}/loader_level2_stage1_code.bin

				cat magic_num.bin					>  ${output_file}
				cat reserve1.bin					>> ${output_file}
				cat loader_stage1.bin				>> ${output_file}
				cat ${SM2_ENTLA_VALUE}				>> ${output_file}
				cat ${SM2_IDA_VALUE}				>> ${output_file}
				cat ${SM2_IDA_RESERVE_VALUE}		>> ${output_file}
				cat reserve2.bin					>> ${output_file}
				cat ${SECOND_KEY_PUBLIC_X}			>> ${output_file}
				cat ${SECOND_KEY_PUBLIC_Y}			>> ${output_file}
				cat market_id.bin					>> ${output_file}
				cat ${KEY2_PUBLIC_SIGN_R}			>> ${output_file}
				cat ${KEY2_PUBLIC_SIGN_S}			>> ${output_file}
				cat reserve3.bin					>> ${output_file}
				cat ${STAGE1_SIGN_R}				>> ${output_file}
				cat ${STAGE1_SIGN_S}				>> ${output_file}
				cat reserve4.bin					>> ${output_file}
				cat loader_stage1_decrypt_key.bin   >> ${output_file}
				cat loader_stage1_decrypt_flag.bin  >> ${output_file}
				cat reserve5.bin					>> ${output_file}
				cat crc_num.bin						>> ${output_file}

				rm magic_num.bin
				rm reserve1.bin
				rm loader_stage1.bin
				rm reserve2.bin
				rm ${KEY2_PUBLIC_SIGN_R}
				rm ${KEY2_PUBLIC_SIGN_S}
				rm reserve3.bin
				rm ${STAGE1_SIGN_R}
				rm ${STAGE1_SIGN_S}
				rm reserve4.bin
				rm loader_stage1_decrypt_key.bin
				rm loader_stage1_decrypt_flag.bin
				rm reserve5.bin
				rm crc_num.bin

				read_bin ${input_file} reserve7.bin ${LOADER_STAGE2_START} ${RESERVE7_NUM}
				LOADER_STAGE2_FACT_START=`echo "${LOADER_STAGE2_START}+${RESERVE7_NUM}"|bc`
				read_bin ${input_file} loader_stage2.bin ${LOADER_STAGE2_FACT_START}

				#   stage2_bin
				#   char sig_key[256];

				local stage2_plain=${LINENO}
				local stage2_plain_hash=${LINENO}
				sm3 ${SM2_ZA_VALUE} ${SM2_FIXED_VALUE} ${SECOND_KEY_PUBLIC_X} ${SECOND_KEY_PUBLIC_Y}
				cat loader_stage2.bin           >  ${stage2_plain}
				sm3 ${stage2_plain_hash} ${SM2_ZA_VALUE} ${stage2_plain}
				sm2 ${PRIVATE_2NDKEY_FILE} ${stage2_plain_hash} ${STAGE2_SIGN_R} ${STAGE2_SIGN_S} ${TOOL_TYPE}
				mv ${stage2_plain} ${OUTPUT_DIR}/loader_level2_stage2_code.bin
				rm ${SM2_ZA_VALUE}
				rm ${stage2_plain_hash}

				cat reserve7.bin				>>${output_file}
				cat loader_stage2.bin           >>${output_file}
				cat ${STAGE2_SIGN_R}            >>${output_file}
				cat ${STAGE2_SIGN_S}            >>${output_file}

				rm reserve7.bin
				rm loader_stage2.bin
				rm ${STAGE2_SIGN_R}
				rm ${STAGE2_SIGN_S}

				mv ${output_file} ${OUTPUT_DIR}/${SIGN_FILE%.*}-level2.${SIGN_FILE##*.}
				if [ "${BIN_OR_BOOT}" = "BOOT" ];then
					rm market_id.bin
					rm ${OUTPUT_DIR}/loader_level2_stage1_code.bin
					rm ${OUTPUT_DIR}/loader_level2_stage2_code.bin
				else
					mv market_id.bin ${OUTPUT_DIR}/market_id.bin
				fi
			fi
			;;
		*)
			echo "ERROR parameter : ${SECURE_LEVEL}"
			delete_ec_param
			rm -rf market_id.bin
			exit 1
			;;
	esac

	delete_ec_param
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
