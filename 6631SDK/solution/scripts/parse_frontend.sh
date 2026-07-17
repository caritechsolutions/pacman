echo "Enter Tuner Config"

source .config

FESETTING=""

CONFIG_TUNER_TOTAL=""
CONFIG_TUNER_SEQUENCE=""
CONFIG_TUNER_TYPE=""
CONFIG_TUNER_I2C_ADDR=""
CONFIG_TUNER_I2C_MASTER=""
CONFIG_DEMOD_TYPE=""
CONFIG_DEMOD_I2C_ADDR=""
CONFIG_DEMOD_I2C_MASTER=""
CONFIG_DEMOD_TS_OUTPUT=""
CONFIG_IQ_SWAP=""
CONFIG_HV_SWAP=""
CONFIG_LNB_POWER=""

# set total tuner number
CONFIG_TUNER_TOTAL=$(grep "BR2_BOARD_TUNER_" .config | awk -F "_" '/^[^#]/{print $NF}' | awk -F "=" '{print $1}')
CONFIG_TUNER_TOTAL=${CONFIG_TUNER_TOTAL%%TUNER}
FESETTING=${FESETTING}${CONFIG_TUNER_TOTAL}",\""

# param 1: tuner number , value is 1/2/3/4
function config_get_tuner_type(){
	tuner_index=$1

	# get tuner
	CONFIG_TUNER_TYPE=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_TUNER  | grep =y | awk  -F "_" '{print $NF}' |awk -F "=" '{print $1}')

	# see demod/include/tuners.h
	case $CONFIG_TUNER_TYPE in
		SHARP1302)
			CONFIG_TUNER_TYPE=31
			;;
		CE5039)
			CONFIG_TUNER_TYPE=32
			;;
		AV2020)
			CONFIG_TUNER_TYPE=33
			;;
		TS2808)
			CONFIG_TUNER_TYPE=34
			;;
		WZ5001)
			CONFIG_TUNER_TYPE=35
			;;
		MAX2117)
			CONFIG_TUNER_TYPE=36
			;;
		MAX2118)
			CONFIG_TUNER_TYPE=37
			;;
		MAX2120)
			CONFIG_TUNER_TYPE=38
			;;
		STB6000)
			CONFIG_TUNER_TYPE=39
			;;
		LW10039)
			CONFIG_TUNER_TYPE=40
			;;
		AV2011)
			CONFIG_TUNER_TYPE=41
			;;
		STV6110)
			CONFIG_TUNER_TYPE=42
			;;
		RDA5812)
			CONFIG_TUNER_TYPE=43
			;;
		SHARP7306)
			CONFIG_TUNER_TYPE=44
			;;	
		STV6110A)
			CONFIG_TUNER_TYPE=45
			;;
		LW37)
			CONFIG_TUNER_TYPE=46
			;;
		ZL10037)
			CONFIG_TUNER_TYPE=47
			;;	
		MTV600)
			CONFIG_TUNER_TYPE=54
			;;
		MJ0914S)
			CONFIG_TUNER_TYPE=54
			;;
		*)
			CONFIG_TUNER_TYPE=error
	esac
#	echo $CONFIG_TUNER_TYPE
}

function config_get_demod_type(){
	tuner_index=$1
	
	CONFIG_DEMOD_TYPE=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_DEMOD_GX  | awk -F "_" '{print $NF}' | awk -F "=" '{print $1}')

	# see demod/dvb_common.c - default_attach
	# GX1001  - array index 0
	# GX1101  - array index 1
	# GX1201  - array index 2
	# GX1501  - array index 3
	# GX113X  - array index 4
	
	case $CONFIG_DEMOD_TYPE in
		GX1132)
			CONFIG_DEMOD_TYPE=4
			;;
		GX1131)
			CONFIG_DEMOD_TYPE=4
			;;
		GX1101)
			CONFIG_DEMOD_TYPE=1
			;;
		GX1503)
			CONFIG_DEMOD_TYPE=3
			;;
		GX1001)
			CONFIG_DEMOD_TYPE=0
			;;
		GX1201)
			CONFIG_DEMOD_TYPE=2
			;;
		*)
			CONFIG_DEMOD_TYPE=error
	esac	
#	echo $CONFIG_DEMOD_TYPE

}

function config_get_demod_i2c_addr(){
	tuner_index=$1

	CONFIG_DEMOD_I2C_ADDR=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_DEMOD_I2C_ADDR  | awk -F "=" '{print $NF}') 

#	echo $CONFIG_DEMOD_I2C_ADDR
}

function config_get_demod_i2c_master(){
	tuner_index=$1

	CONFIG_DEMOD_I2C_MASTER=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_DEMOD_I2C_MASTER  | awk -F "=" '{print $NF}') 

#	echo $CONFIG_DEMOD_I2C_MASTER
}


function config_get_tuner_i2c_addr(){
	tuner_index=$1

	CONFIG_TUNER_I2C_ADDR=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_TUNER_I2C_ADDR  | awk -F "=" '{print $NF}') 

#	echo $CONFIG_TUNER_I2C_ADDR
}

function config_get_tuner_i2c_master(){
	tuner_index=$1

	CONFIG_TUNER_I2C_MASTER=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_TUNER_I2C_MASTER  | awk -F "=" '{print $NF}') 

#	echo $CONFIG_TUNER_I2C_MASTER
}


function config_get_iq_swap(){
	tuner_index=$1

	CONFIG_IQ_SWAP=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_IQ_SWAP  | awk -F "=" '{print $NF}') 

#	echo $CONFIG_IQ_SWAP
}


function config_get_hv_swap(){
	tuner_index=$1

	CONFIG_HV_SWAP=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_HV_SWAP | awk -F "=" '{print $NF}') 

#	echo $CONFIG_HV_SWAP
}

function config_get_lnb_power(){
	tuner_index=$1

	CONFIG_LNB_POWER=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_LNB_POWER_CONTROL= | awk -F "=" '{print $NF}') 

	CONFIG_LNB_POWER=${CONFIG_LNB_POWER##\"}
	CONFIG_LNB_POWER=${CONFIG_LNB_POWER%%\"}
	#echo $CONFIG_LNB_POWER
	
	case $CONFIG_LNB_POWER in
		GPIO)
			CONFIG_LNB_POWER=0
			;;
		A8293)
			CONFIG_LNB_POWER=1
			;;
		SY7208)
			CONFIG_LNB_POWER=0
			;;
		*)
			CONFIG_LNB_POWER=error
	esac
	#echo $CONFIG_LNB_POWER
}

function config_get_demod_ts_output(){
	tuner_index=$1

	ts_data_line=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep -c TUNER[${tuner_index}]_DEMOD_TS_OUTPUT_DATA_LINES | awk -F "=" '{print $NF}')

	ts_data_order=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk '/^[^#]/' |grep TUNER[${tuner_index}]_DEMOD_TS_OUTPUT_DATA_ORDER | awk -F "=" '{print $NF}')

	ts_pin1=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk -F "_" '/^[^#]*PIN1=y/{print $(NF-1)}') 
	ts_pin2=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk -F "_" '/^[^#]*PIN2=y/{print $(NF-1)}') 
	ts_pin3=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk -F "_" '/^[^#]*PIN3=y/{print $(NF-1)}') 
	ts_pin4=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk -F "_" '/^[^#]*PIN4=y/{print $(NF-1)}') 
	ts_pin5=$(grep "BR2_BOARD_TUNER[${tuner_index}]" .config | awk -F "_" '/^[^#]*PIN5=y/{print $(NF-1)}') 
	
	ts_data_order=${ts_data_order##\"}
	ts_data_order=${ts_data_order%%\"}

	
	CONFIG_DEMOD_TS_OUTPUT=${ts_data_order}_${ts_pin1}_${ts_pin2}_${ts_pin3}_${ts_pin4}_${ts_pin5}

	#echo $CONFIG_DEMOD_TS_OUTPUT
	#see demod/fronted/gx113x.c - gx113x_tspin_tab
	# 0 ~7 DATA 
	# 8 SYNC
	# 9 ERROR
	# A VALID
	# B CLK
	case $CONFIG_DEMOD_TS_OUTPUT in
		SWAP_ERROR_SYNC_VALID_CLK_DATA)
			CONFIG_DEMOD_TS_OUTPUT=0
			;;
		NORMAL_DATA_CLK_VALID_SYNC_ERROR)
			CONFIG_DEMOD_TS_OUTPUT=1
			;;
		SWAP_SYNC_CLK_DATA_VALID_ERROR)
			CONFIG_DEMOD_TS_OUTPUT=2
			;;
		SWAP_SYNC_VALID_CLK_DATA_ERROR)
			CONFIG_DEMOD_TS_OUTPUT=3
			;;
		NORMAL_CLK_DATA_SYNC_VALID_ERROR)
			CONFIG_DEMOD_TS_OUTPUT=4
			;;
		NORMAL_DATA_SYNC_VALID_CLK_ERROR)
			CONFIG_DEMOD_TS_OUTPUT=6
			;;
		NORMAL_DATA_SYNC_ERROR_VALID_CLK)
			CONFIG_DEMOD_TS_OUTPUT=7
			;;
		*)
			CONFIG_DEMOD_TS_OUTPUT=error
			;;
	esac
}

function tuner_setting(){
	CONFIG_TUNER_SEQUENCE=$1
	config_get_demod_type $CONFIG_TUNER_SEQUENCE
	config_get_demod_i2c_master $CONFIG_TUNER_SEQUENCE
	config_get_demod_i2c_addr $CONFIG_TUNER_SEQUENCE
	config_get_tuner_type $CONFIG_TUNER_SEQUENCE
	config_get_tuner_i2c_master $CONFIG_TUNER_SEQUENCE
	config_get_tuner_i2c_addr $CONFIG_TUNER_SEQUENCE
	config_get_iq_swap $CONFIG_TUNER_SEQUENCE
	config_get_hv_swap $CONFIG_TUNER_SEQUENCE
	config_get_demod_ts_output $CONFIG_TUNER_SEQUENCE
	config_get_lnb_power $CONFIG_TUNER_SEQUENCE
	
	a_tuner_setting="|"${CONFIG_DEMOD_TYPE}:${CONFIG_DEMOD_I2C_MASTER}:${CONFIG_DEMOD_I2C_ADDR}:${CONFIG_TUNER_TYPE}:${CONFIG_TUNER_I2C_MASTER}:${CONFIG_TUNER_I2C_ADDR}:"&"${CONFIG_IQ_SWAP}:${CONFIG_HV_SWAP}:${CONFIG_DEMOD_TS_OUTPUT}:${CONFIG_LNB_POWER}
	#echo $a_tuner_setting
	FESETTING=${FESETTING}${a_tuner_setting}
}

tuner_index=1
while [ $tuner_index -le $CONFIG_TUNER_TOTAL ]
do
	#echo "Tuner Index="$tuner_index
	tuner_setting $tuner_index
	((tuner_index=tuner_index+1))
done

FESETTING=${FESETTING}"\""

return=$(echo ${FESETTING} | grep -c error)

if [[ $return -gt 0 ]];then
	echo "WARNING: parse tuner config error, please check your configration"
fi

echo "#define FESETTING ("${FESETTING}")"
