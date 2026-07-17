#########################################################################
# File Name   : tools/private_handle.sh
# Author      : huyuan
# mail        :
# Created Time: 2018年09月21日 星期五 18时50分56秒
#########################################################################
#!/bin/bash

CUR_DIR=$(cd "$(dirname "$0")"; pwd)
ISV1_DIR=${CUR_DIR}/private_config/irdeto_solution_v1

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
		dd if=${input_file} of=${output_file} bs=1 skip=${addr} count=${len} 2>/dev/null
	else
		if [ ${input_file_len} -lt ${addr} ];then
			echo "read_bin error!" 
			exit 1
		fi
		dd if=${input_file} of=${output_file} bs=1 skip=${addr} 2>/dev/null #不加count就是读到结束
	fi
}

irdeto_solution_v1()
{
	local intput_file=$1
	local dev_prod=${ISV1_DIR}/dev_prod.txt
	local loader_version=${ISV1_DIR}/loader_version.txt

	xxd -c4 -p -r ${dev_prod} TEMP_FILE_DEV_PROD
	xxd -c4 -p -r ${loader_version} TEMP_FILE_LOADER_VERSION

	read_bin ${intput_file} TEMP_FILE_OUTPUT_1 0 16384
	read_bin ${intput_file} TEMP_FILE_OUTPUT_2 16386

	cat TEMP_FILE_OUTPUT_1			>	 TEMP_FILE_OUTPUT
	cat TEMP_FILE_LOADER_VERSION	>>	 TEMP_FILE_OUTPUT
	cat TEMP_FILE_DEV_PROD			>>	 TEMP_FILE_OUTPUT
	cat TEMP_FILE_OUTPUT_2          >>	 TEMP_FILE_OUTPUT

	mv TEMP_FILE_OUTPUT ${intput_file}
	rm -rf TEMP_FILE*
}

main()
{
	local intput_file=$1
	local section=$2

	if [ ! -n "${section}" ];then
		exit
	fi


	case "${section}" in
		"irdeto_solution_v1")
			irdeto_solution_v1 ${intput_file}
			;;
		*)
			echo "error section: ${section}"
			exit;
	esac

}

main $@
