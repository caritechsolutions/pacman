#!/bin/bash

INPUT_FILE=${1}
BOOTLOADER_SIZE=${2}
BOOTLOADER_SIZE=`printf '%d' ${BOOTLOADER_SIZE}`
INPUT_FILE_SIZE=`ls -l ${INPUT_FILE} | awk '{print $5}'`
FILL_ZERO_SIZE=`echo "$BOOTLOADER_SIZE-$INPUT_FILE_SIZE"|bc`
if [ "$FILL_ZERO_SIZE" -le 0 ];then
	echo "error: overflowed!"
	rm ${INPUT_FILE}
	exit 1
fi
dd if=/dev/zero of=zero.bin bs=1 skip=0 count=${FILL_ZERO_SIZE} 2>/dev/null
cat zero.bin >> ${INPUT_FILE}
