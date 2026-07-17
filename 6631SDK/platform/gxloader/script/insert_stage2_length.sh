#!/bin/bash


DIR=output
SKIP_ADDR=
FILE_SKIP_ADDR=
LENGTH_ADDR=28
RESERVE_ADDR=32
CUR_DIR=$(cd "$(dirname "$0")"; pwd)
COMMON_DIR=${CUR_DIR}/../tools/secure_tool/common

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


txt_to_bin()
{
        local txt_file=$1
        local bin_file=$2
        #xxd -r -p ${txt_file} > ${bin_file}
        xxd -r -p ${txt_file} | od -v -An -t x1 -w4  | awk '{print $4, $3, $2, $1}' | xxd -r -p > ${bin_file}
}

insert_length()
{
	local file_name=$1
	local file_length=`ls -l ${file_name} | awk '{print $5}'`
	local skip_addr=$[${FILE_SKIP_ADDR}+${LENGTH_ADDR}]
	local last_addr=$[${FILE_SKIP_ADDR}+${RESERVE_ADDR}]

#	echo "$file_name"
#	echo "$FILE_SKIP_ADDR"
	file_length=$[${file_length}-${last_addr}]
	file_length=`printf '%08x' ${file_length}`

	rm -rf TEMP_FILE*

	echo ${file_length} > TEMP_FILE1
	txt_to_bin TEMP_FILE1 TEMP_FILE2

	read_bin ${file_name} TEMP_FILE3 0 ${skip_addr}
	read_bin ${file_name} TEMP_FILE4 ${last_addr}

	cat TEMP_FILE3 > ${file_name}
	cat TEMP_FILE2 >> ${file_name}
	cat TEMP_FILE4 >> ${file_name}

	rm -rf TEMP_FILE*
}

main()
{
	local flash_type=$1
	local boot_tool=$2
	local chip_type=$3
	local extern_param=$4
	local extern_param_file_size=$5
	local extern_param_file_count=$6
	local extern_param_file_total_size=0

	if [ -z "$extern_param_file_count" ]; then
		extern_param_file_count=1
	fi

	extern_param_file_total_size=$extern_param_file_size*$extern_param_file_count

	if [ "${chip_type}" == "sirius" -o "${chip_type}" == "canopus" ];then
		SKIP_ADDR=0x4000
	elif [ "${chip_type}" == "gx3211" -o "${chip_type}" == "gx6605s" \
		-o "${chip_type}" == "taurus" -o "${chip_type}" == "gemini" \
		-o "${chip_type}" == "scorpio" \
		-o "${chip_type}" == "scorpio_taurus" \
		-o "${chip_type}" == "cygnus" ];then
		if [ "${boot_tool}" = "y" ];then
			SKIP_ADDR=0x201C
		else
			SKIP_ADDR=0x2000
		fi
	elif [ "${chip_type}" == "gx3113C" -o "${chip_type}" == "gx3201" ];then
		if [ "${boot_tool}" = "y" ];then
			SKIP_ADDR=0x101C
		else
			SKIP_ADDR=0x1000
		fi
	else
		echo "$0 : error chip!"
		exit
	fi

	gcc ${COMMON_DIR}/read_file.c -o ${COMMON_DIR}/read_file
	for file_name in ${DIR}/*
	do
		if [ "${boot_tool}" = "y" ];then
			# boot file
			file_mark=".boot"
			FILE_SKIP_ADDR=$SKIP_ADDR
		else
			file_mark=${flash_type}
			# flash file
			if [[ ${flash_type} =~ "sflash" ]];then
				echo
				FILE_SKIP_ADDR=$SKIP_ADDR
				# nor flash
			else
				# nand flash 4K page size
				if [[ ${file_name} =~ "-4K" ]];then
					if [ ${chip_type} = "sirius" ];then
						FILE_SKIP_ADDR=$[SKIP_ADDR/2048*4096*5]
					elif [ ${chip_type} = "canopus" ];then
						FILE_SKIP_ADDR=$[SKIP_ADDR/1024*4096*1]
					fi
				else
					# nand flash 2K page size
					if [ ${chip_type} = "canopus" ];then
						FILE_SKIP_ADDR=$[SKIP_ADDR/1024*2048*1]
					else
						FILE_SKIP_ADDR=$[SKIP_ADDR*5]
					fi
				fi
			fi
		fi

		if [ "${extern_param}" = "y" ];then
			FILE_SKIP_ADDR=$((${FILE_SKIP_ADDR}+${extern_param_file_total_size}))
		fi

		if [[ ${file_name} =~ ${file_mark} ]];then
			insert_length ${file_name}
		fi
	done
	rm -rf ${COMMON_DIR}/read_file
}

help()
{
	echo "------------------------------------------------------------------------------------------------------------------"
	echo "Instructions: "
	echo "			./insert_stage2_length.sh <flash_type> <boot_file> <chip_type>"
	echo "				flash_type : sflash | spinand | nand"
	echo "				boot_file  : 0 | 1"
	echo "				chip_type  : gx3211 | gx3201 | gx3113C | gx6605s | sirius | taurus | gemini | cygnus"
	echo "Note："
	echo "			1: <boot_file> option indicate the modfiy file is a .boot file or a .bin file"
	echo "------------------------------------------------------------------------------------------------------------------"
}

if [ $# -le 2 ];then
        help
else
        main $@
fi
