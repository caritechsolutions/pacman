#!/bin/bash
##############################################################################
# yanyu, yanyu@nationalchip.com
# Copyright(C) 2013-2021, HangZhou NationalChip Science and Technology Corp.
# Scan cas in solution/app/cas and add cas choose in make menuconfig CAS Type
##############################################################################

BUILD_DIR=$(pwd)
config_proj_path=${BUILD_DIR}/app/ca/cas
config_proj_conf=${BUILD_DIR}/system/configin/Config.in.secure

function scan_dir(){
	for file in ` ls $config_proj_path `
	do
		if [ -e $config_proj_path"/"$file ]; then
			typeset -u pathName
			pathName=$file
			FIND_STR="BR2_MOD_CASCAM_$pathName"
			if [ `grep -c "$FIND_STR" $config_proj_conf` -ne '0' ];then
				echo "$config_proj_conf Has"$FIND_STR
			else
				sed -i '/depends on BR2_MOD_CASCAM/a config '"$FIND_STR"'' $config_proj_conf
				sed -i '/config '"$FIND_STR"'/a	 bool "'"$file"'"' $config_proj_conf
			fi
		fi
	done
}

if [ -e $config_proj_path ]; then
	scan_dir
fi
