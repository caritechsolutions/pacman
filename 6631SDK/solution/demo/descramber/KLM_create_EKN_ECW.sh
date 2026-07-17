#! /bin/sh

############################### EXPORT ################################

FILE_TEMP_SRC=klm_temp_src.bin
FILE_TEMP_DST=klm_temp_dst.bin
ABS_PATH=./

VAL_BLOCK_LEN=0
VAL_ALG=0
STRING_ROOTKEY=0
STRING_CW=0

ALG_TYPE_AES128=0
ALG_TYPE_TDES=1

############################## FUNCTION ################################

#######################################################
#
# @ Description: check the parameters
#
# @ Params
#       $1 : KLM_ALG
#       $2 : KLM_STAGE
#       $3 : ROOTKEY_STR
#       $4 : CW_STR
#
func_check_param()
{
	export VAL_ALG=$1
	export STRING_ROOTKEY=$3
	export STRING_CW=$4

	if [ $1 = $ALG_TYPE_AES128 ]; then
		export BLOCK_LEN=16
		export ROOTKEY_LEN=32
		export CW_LEN=32
	elif [ $1 = $ALG_TYPE_TDES ]; then
		export BLOCK_LEN=8
		export ROOTKEY_LEN=32
		export CW_LEN=16
	else
		echo "Don't support the KLM_Alg parameter"
		exit 1
	fi

	if [ $2 -lt 2 ] || [ $2 -ge 8 ]; then
		echo "Don't support the KLM_stage"
		exit 1
	fi

	if [ ${#3} != $ROOTKEY_LEN ]; then
		echo "Can't solve the rootkey_string! The string length should be "$ROOTKEY_LEN
		exit 1
	fi

	if [ ${#4} != $CW_LEN ]; then
		echo "Can't solve the CW_string! The string length shoule be "$CW_LEN
		exit 1
	fi
}

#######################################################
#
# @ Description: Create the KN
#
# @ Params
#       $1 : stage pos
#
func_create_KN()
{
	STRING_EKN=000102030405060708090a0b0c0d0e0f

	if [ $VAL_ALG = $ALG_TYPE_AES128 ]; then
		STRING_ALG=-aes-128-ecb
	else
		STRING_ALG=-des-ede
	fi

	echo $STRING_EKN | xxd -r -ps > $FILE_TEMP_SRC
	#xxd $FILE_TEMP_SRC

	if [ $1 = '2' ]; then
		openssl enc -d $STRING_ALG -in $FILE_TEMP_SRC -out $FILE_TEMP_DST -K $STRING_ROOTKEY -nopad
		#echo openssl enc -d $STRING_ALG -in $FILE_TEMP_SRC -out $FILE_TEMP_DST -K $STRING_ROOTKEY -nopad
	else
		STRING_KN=`xxd -p $FILE_TEMP_DST`
		openssl enc -d $STRING_ALG -in $FILE_TEMP_SRC -out $FILE_TEMP_DST -K $STRING_KN -nopad
		#echo openssl enc -d $STRING_ALG -in $FILE_TEMP_SRC -out $FILE_TEMP_DST -K $STRING_KN -nopad
	fi

	echo "\n[EKN] "$(($1-2))
	xxd -i $FILE_TEMP_SRC
}

#######################################################
#
# @ Description: Create the ECW
#
#
func_create_ECW()
{
	if [ $VAL_ALG = $ALG_TYPE_AES128 ]; then
		STRING_ALG=-aes-128-ecb
	else
		STRING_ALG=-des-ede
	fi

	echo $STRING_CW | xxd -r -ps > $FILE_TEMP_SRC
	#xxd $FILE_TEMP_SRC

	STRING_KN=`xxd -p $FILE_TEMP_DST`
	openssl enc -e $STRING_ALG -in $FILE_TEMP_SRC -out $FILE_TEMP_DST -K $STRING_KN -nopad
	#echo openssl enc -e $STRING_ALG -in $FILE_TEMP_SRC -out $FILE_TEMP_DST -K $STRING_KN -nopad

	echo "\n[ECW]"
	xxd -i $FILE_TEMP_DST
}

help()
{
	echo "./KLM_create_EKN_ECW.sh [KLM_Alg] [KLM_stage] [rootkey_string] [CW_string]"
	echo "[KLM_Alg]       : 0 AES128"
	echo "                  1 TDES"
}

############################## MAIN ################################

if [ $# -eq 4 ]; then

	func_check_param $1 $2 $3 $4

	for i in $(seq 2 $2)
	do
		func_create_KN $i
	done

	func_create_ECW

	if [ -f $FILE_TEMP_SRC ]; then
		rm  $FILE_TEMP_SRC
	fi

	if [ -f $FILE_TEMP_DST ]; then
		rm  $FILE_TEMP_DST
	fi

else
	help
	exit 1
fi

