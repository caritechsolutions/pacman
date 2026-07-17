#!/bin/bash

KEY_DIR=sm2_new_key
PRIVATE_KEY_FILE=
PUBLIC_KEY_FILE=
PRIVATE_KEY_BIN_FILE=
PUBLIC_KEY_BIN_FILE_X=
PUBLIC_KEY_BIN_FILE_Y=
PUBLIC_KEY_BIN_FILE_X_START=1
PUBLIC_KEY_BIN_FILE_Y_START=33
PUBLIC_KEY_BIN_LENGTH=32
PUBLIC_KEY_BIN_START_FLAGS="pub"
PUBLIC_KEY_BIN_END_FLAGS="ASN1 OID"
PRIVATE_KEY_BIN_START_FLAGS="priv"
PRIVATE_KEY_BIN_END_FLAGS="pub"

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

txt_to_bin()
{
    local txt_file=$1
    local bin_file=$2
    xxd -r -p ${txt_file} | od -v -An -t x1 -w4  | awk '{print $1, $2, $3, $4}' | xxd -r -p > ${bin_file}
}

generate_private_key()
{
	gmssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:sm2p256v1 -pkeyopt ec_param_enc:named_curve -out ${PRIVATE_KEY_FILE}

	if [ ! -f "${PRIVATE_KEY_FILE}" ];then
		echo "generate private key error !"
		exit 1
	fi

	gmssl pkey -in ${PRIVATE_KEY_FILE} -text -noout > TEMP_FILE1
	START_LINE=$(($(grep -n "${PRIVATE_KEY_BIN_START_FLAGS}" TEMP_FILE1 | cut -d : -f 1) + 1))
	END_LINE=$(($(grep -n "${PRIVATE_KEY_BIN_END_FLAGS}" TEMP_FILE1 | cut -d : -f 1) - 1))
	sed -n "${START_LINE},${END_LINE}p" TEMP_FILE1 > TEMP_FILE2
	sed -i "s/:/ /g" TEMP_FILE2
	xxd -c4 -p -r TEMP_FILE2 ${PRIVATE_KEY_BIN_FILE}
	rm TEMP_FILE*
}

generate_public_key()
{
	gmssl pkey -pubout -in ${PRIVATE_KEY_FILE} -out ${PUBLIC_KEY_FILE}

	if [ ! -f "${PUBLIC_KEY_FILE}" ];then
		echo "generate public key error !"
		exit 1
	fi

	gmssl pkey -in ${PRIVATE_KEY_FILE} -text_pub -noout > TEMP_FILE1

	START_LINE=$(($(grep -n "${PUBLIC_KEY_BIN_START_FLAGS}" TEMP_FILE1 | cut -d : -f 1) + 1))
	END_LINE=$(($(grep -n "${PUBLIC_KEY_BIN_END_FLAGS}" TEMP_FILE1 | cut -d : -f 1) - 1))
	sed -n "${START_LINE},${END_LINE}p" TEMP_FILE1 > TEMP_FILE2
	sed -i "s/:/ /g" TEMP_FILE2
	xxd -c4 -p -r TEMP_FILE2 TEMP_FILE3

	read_bin TEMP_FILE3 ${PUBLIC_KEY_BIN_FILE_X} ${PUBLIC_KEY_BIN_FILE_X_START} ${PUBLIC_KEY_BIN_LENGTH}
	read_bin TEMP_FILE3 ${PUBLIC_KEY_BIN_FILE_Y} ${PUBLIC_KEY_BIN_FILE_Y_START} ${PUBLIC_KEY_BIN_LENGTH}
	
	if [ ! -d "${KEY_DIR}" ];then
		mkdir ${KEY_DIR}
	fi

	mv ${PRIVATE_KEY_FILE} ${KEY_DIR}/
	mv ${PUBLIC_KEY_FILE} ${KEY_DIR}/
	mv ${PRIVATE_KEY_BIN_FILE} ${KEY_DIR}/
	mv ${PUBLIC_KEY_BIN_FILE_X} ${KEY_DIR}/
	mv ${PUBLIC_KEY_BIN_FILE_Y} ${KEY_DIR}/

	rm TEMP_FILE*
}

generate_key()
{
	PRIVATE_KEY_FILE=${1}.pri
	PUBLIC_KEY_FILE=${1}.pub
	PRIVATE_KEY_BIN_FILE=${1}_private.bin
	PUBLIC_KEY_BIN_FILE_X=${1}_public_x.bin
	PUBLIC_KEY_BIN_FILE_Y=${1}_public_y.bin

	generate_private_key
	generate_public_key
}

help()
{
	echo ""
	echo "Instructions:"
	echo "		./sm2_generate_key.sh <key_file | clean>"
	echo "			key_file ：the name of the generated key file, these generated files are located in the sm2_new_key directory"
	echo "			clean    ：delete the sm2_new_key directory"
	echo ""
	echo "example:"
	echo "		step1：Executing the \"./sm2_generate_key.sh test\" command"
	echo "		step2：View the files in the sm2_new_key directory，as follows："
	echo "				sm2_new_key/"
	echo "				├── test.pri                // private key"
	echo "				├── test_private.bin        // private key, binary file"
	echo "				├── test.pub                // public key"
	echo "				├── test_public_x.bin       // the x coordinate of public key, binary file,"
	echo "				└── test_public_y.bin       // the y coordinate of public key, binary file,"

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
