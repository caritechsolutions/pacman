#!/bin/bash
# Read GX6601/GX6602/GX3231/GX6601E/GX6605 Chip S/N on Linux OS
# Copyleft (C), NationalChip Science and Technology Inc.
# By LiuJianhua / BlackerLiu, liujh@nationalchip.com
# On 2013-12-11

otp_addr=0
otp_num=0

usage()
{
	echo "Usage: $0 -a addr -n num"
	echo "-a  addr           set otp addr"
	echo "-n  num            read num bytes otp"
	echo "-h                 display this help message"
}

if [[ $# -lt 2 ]];then
	usage
	exit 1
fi


while getopts "a:n:h" arg 
do
	case $arg in
             a)
		otp_addr=$OPTARG
                ;;
             n)
		otp_num=$OPTARG
                ;;
             h)
                usage
		exit 0
                ;;
             ?)  
		echo "unkonw argument"
       		exit 1
        	;;
        esac
done

# check input param
if [[ $otp_addr = "0" ]];then
	echo "otp addr error!"
	exit 1
fi

# check input num
if [[ $otp_num -lt 0 || $otp_num -gt 256 ]];then
	echo "otp num error!"
	exit 1
fi

if [ -e /proc/gx_otp ];then
	echo $otp_addr@$otp_num > /proc/gx_otp
	busybox hexdump -C /proc/gx_otp
else
	echo "Cannot find /proc/gx_otp"
fi
