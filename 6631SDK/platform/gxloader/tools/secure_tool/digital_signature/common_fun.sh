#########################################################################
# File Name   : common_fun.sh
# Author      :
# mail        :
# Created Time: 2018年03月16日 星期五 16时52分38秒
#########################################################################
#!/bin/bash

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

zero_to_bin()
{
	local bin_file=$1
	local len=$2
	local DEV="/dev/zero"

	if [ ! -c ${DEV} ]; then
		echo "${DEV} not exist!"
		exit 1
	fi
	dd if=${DEV} of=${bin_file} bs=1 skip=0 count=${len} 2>/dev/null
}

txt_to_bin()
{
	local txt_file=$1
	local bin_file=$2
	#xxd -r -p ${txt_file} > ${bin_file}
	xxd -r -p ${txt_file} | od -v -An -t x1 -w4  | awk '{print $4, $3, $2, $1}' | xxd -r -p > ${bin_file}
}

txt_to_bin1()
{
	local txt_file=$1
	local bin_file=$2
	xxd -r -p ${txt_file} > ${bin_file}
}

str_to_bin()
{
	local str=$1
	local bin_file=$2
	echo -n "$str" > ${bin_file}
}
