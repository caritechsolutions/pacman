#!/bin/bash

#######################################################
# check_openvpn_conf.sh (C) 2024 Bert
#
# This script will parse clien.ovpn file to insure
# key/crt file path match box
#
#######################################################


CONF_FILE_NAME="client.ovpn"
LOGIN_FILE_NAME="login.conf"

TAG_USER_PASS="auth-user-pass"

OPENVPN_CUR_DIR=$(pwd)
OPENVPN_DIR=${GXSRC_PATH}"/output/theme/openvpn/"
OPENVPN_DST_DIR=${GXSRC_PATH}"/output/theme/"
OPENVPN_DST_DIR_FOLDER=${GXSRC_PATH}"/output/theme/openvpn/"

DEFAULt_DIR="/dvb/theme/openvpn/"
USERDB_DIR="/home/gx/openvpn/"

OVPN_CONF_FOLDER_NAME="ovpn_conf"
OVPN_CONF_DST_FOLDER_NAME="openvpn"
OVPN_CONF_DIR=${OPENVPN_CUR_DIR}/${OVPN_CONF_FOLDER_NAME}
OVPN_CONF_TMP_DIR=${OPENVPN_CUR_DIR}/${OVPN_CONF_DST_FOLDER_NAME}
OVPN_CONF_DST_DIR=${OPENVPN_DST_DIR}/${OVPN_CONF_DST_FOLDER_NAME}

VALID_DIR="$DEFAULt_DIR"

OPENVPN_CONF_FILE="$OVPN_CONF_TMP_DIR/$CONF_FILE_NAME"

echo "check openvpn conf file $CONF_FILE_NAME start... "

# del old openvpn config folder
## del output/theme/openvpn
if [[ -e ${OPENVPN_DST_DIR_FOLDER} ]];then
	rm ${OPENVPN_DST_DIR_FOLDER} -rf;
fi
## del tmp dir  app/netapps/openvpn/openvpn
if [[ -e ${OVPN_CONF_TMP_DIR} ]];then
	rm ${OVPN_CONF_TMP_DIR} -rf;
fi

VPN=$(cat ${GXSRC_PATH}/app/include/app_config.h| grep "VPN_SUPPORT" | grep "1" | wc -l)
if [ $VPN -eq 1 ]; then
	# 检查文件夹是否存在
	if [ -d "$OVPN_CONF_DIR" ]; then
		# 列出文件夹中的所有文件（不包括子文件夹）
		cp ${OVPN_CONF_DIR} ${OVPN_CONF_TMP_DIR}  -rf
		if [ -f "$OPENVPN_CONF_FILE" ]; then
			files=`ls "$OVPN_CONF_TMP_DIR"`
			for file in $files
			do
				if [ "$file" == "$LOGIN_FILE_NAME" ]; then
					VALID_DIR="$USERDB_DIR"
				else
					VALID_DIR="$DEFAULt_DIR"
				fi

				sed -i -E '/^[^#]/ s| .*'$file'| '$VALID_DIR''$file'|' "$OPENVPN_CONF_FILE"
			done
			# 密码文件login.conf 特别处理
			# 由于DIR变量中有 / 字符, sed中采用 | 分隔符来避免与路径字符 / 冲突
			sed -i -E '/^[^#]*'$TAG_USER_PASS'.*/ s|'$TAG_USER_PASS'.*|'$TAG_USER_PASS' '$USERDB_DIR''$LOGIN_FILE_NAME'|g' $OPENVPN_CONF_FILE 

			# mv openvpn to output
			mv ${OVPN_CONF_TMP_DIR} ${OPENVPN_DST_DIR} 

		else
			echo "The openvpn config file $OVPN_CONF_DIR/$CONF_FILE_NAME not exist !!!!"
		fi
	else
		echo "The openvpn_dir $OVPN_CONF_DIR not exist !!!!"
	fi
fi
