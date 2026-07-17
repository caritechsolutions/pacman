#!/bin/sh 
#ex. ./mv_file_name.sh 6131 3113C
SRC_KEY_WORD=$1
DST_KEY_WORD=$2
while :;
do
	src_list=`find * -name "*${SRC_KEY_WORD}*"`
	echo $src_list
	if [ -z "$src_list" ];then
		echo "operate finished"
		exit
	fi
	for unit in ${src_list}
	do 
		echo "$unit"
		dst_unit=`echo ${unit}|sed "s/${SRC_KEY_WORD}/${DST_KEY_WORD}/g"`
		echo "$dst_unit"
		echo "mv $unit $dst_unit"
		mv $unit $dst_unit
	done
done
