#!/bin/bash

CUR_DIR=$(cd "$(dirname "$0")"; pwd)
KEY_BITS=2048
KEY_DIR=${CUR_DIR}/rsa_new_key
PRIVATE_KEY_FILE=
PUBLIC_KEY_FILE=
PUBLIC_KEY_BIN_FILE=
PUBLIC_KEY_TEXT_FILE=

txt_to_bin()
{
    local txt_file=$1
    local bin_file=$2
    xxd -r -p ${txt_file} | od -v -An -t x1 -w4  | awk '{print $1, $2, $3, $4}' | xxd -r -p > ${bin_file}
}

generate_private_key()
{
	openssl genrsa -out ${PRIVATE_KEY_FILE} -f4 ${KEY_BITS}
}

generate_public_key()
{
	if [ ! -f ${PRIVATE_KEY_FILE} ];then
		echo "generate private key error !"
		exit 1
	fi

	openssl rsa -in ${PRIVATE_KEY_FILE} -out ${PUBLIC_KEY_FILE} -pubout

	if [ ! -f ${PUBLIC_KEY_FILE} ];then
		echo "generate public key error !"
		exit 1
	fi

	openssl rsa -in ${PUBLIC_KEY_FILE} -pubin -modulus -noout -out ${PUBLIC_KEY_TEXT_FILE}
	sed -i 's/Modulus=//g' ${PUBLIC_KEY_TEXT_FILE}
	txt_to_bin ${PUBLIC_KEY_TEXT_FILE} ${PUBLIC_KEY_BIN_FILE}

	xxd -c1 -g1 ${PUBLIC_KEY_BIN_FILE} > TMP1
	cat TMP1 | awk '{print $2}' > ${PUBLIC_KEY_TEXT_FILE}

	if [ ! -d "${KEY_DIR}" ];then
		mkdir ${KEY_DIR}
	fi
	rm TMP1
	mv ${PRIVATE_KEY_FILE} ${KEY_DIR}/
	mv ${PUBLIC_KEY_FILE} ${KEY_DIR}/
	mv ${PUBLIC_KEY_BIN_FILE} ${KEY_DIR}/
	mv ${PUBLIC_KEY_TEXT_FILE} ${KEY_DIR}/
}

generate_key()
{
	PRIVATE_KEY_FILE=${1}.pri
	PUBLIC_KEY_FILE=${1}.pub
	PUBLIC_KEY_BIN_FILE=${1}_modulus.bin
	PUBLIC_KEY_TEXT_FILE=${1}_modulus.txt

	generate_private_key
	generate_public_key
}

help()
{
	echo ""
	echo "Instructions:"
	echo "		./rsa_generate_key.sh <key_file | clean>"
	echo "			key_file ：the name of the generated key file, these generated files are located in the rsa_new_key directory"
	echo "			clean    ：delete the rsa_new_key directory"
	echo ""
	echo "example:"
	echo "		step1：Executing the \"./rsa_generate_key.sh test\" command"
	echo "		step2：View the files in the rsa_new_key directory，as follows："
	echo "				rsa_new_key/"
	echo "				├── test_modulus.bin        // public key modulus, binary file"
	echo "				├── test_modulus.txt        // public key modulus, hex file"
	echo "				├── test.pri                // private key"
	echo "				└── test.pub                // public key"

	exit 1
}

clean()
{
	rm -rf ${KEY_DIR}
	exit 0
}

if [ $# != 1 ];then
	help
elif [ "$1" == "clean" ];then
	clean
else
	generate_key $@
fi
