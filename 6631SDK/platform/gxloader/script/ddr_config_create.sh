#!/bin/bash

FLAG='#define'
EXTERN_PARAM_TXT=
get_denali_ctl_phy()
{
	local file_name=${1}
	local i=1

	cat ${file_name} | while read line;
	do
		FIRST_ID=$(awk 'NR=='$i' {print $1}' ${file_name})
		THIRD_ID=$(awk 'NR=='$i' {print $3}' ${file_name})
		if [ "${FIRST_ID}" != "" ]; then
			if [ "${FIRST_ID}" == "${FLAG}" ]; then
				printf %08x $THIRD_ID >> ${EXTERN_PARAM_TXT}
			fi
		fi
		((i++))
	done
}

get_ddr_config()
{
	local chip_core=$1
	local ddr_inclue=$2
	EXTERN_PARAM_TXT=$3
	local ddr_para_file1="./board/${chip_core}/board-generic/${ddr_inclue}"
	local ddr_para_file2="./board/${chip_core}/${ddr_inclue}"

	if [ -e $ddr_para_file1 ]; then
		rm -f $EXTERN_PARAM_TXT
		get_denali_ctl_phy $ddr_para_file1
	elif [ -e $ddr_para_file2 ]; then
		rm -f $EXTERN_PARAM_TXT
		get_denali_ctl_phy $ddr_para_file2
	fi
}

get_ddr_config $@
