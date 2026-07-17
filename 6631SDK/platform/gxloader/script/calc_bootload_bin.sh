#!/bin/sh

main()
{
	if [ $# -lt 2 ]; then
		exit 1
	fi

	local input_file=$1
	local BOOTLOADER_SIZE=$2

	BOOTLOADER_SIZE=`printf '%d' $BOOTLOADER_SIZE`
	INPUT_FILE_SIZE=`ls -l $input_file | awk '{print $5}'`
	TMP_SIZE=`echo "$BOOTLOADER_SIZE-$INPUT_FILE_SIZE"|bc`
	if [ "$TMP_SIZE" -le 0 ];then
		echo "error:bootloader bin overflowed!"
		rm $input_file
		exit 1
	fi
}

main $@
