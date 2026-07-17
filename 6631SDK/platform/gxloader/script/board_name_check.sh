#!/bin/sh

ver="v0.2 -- 20170210"

configlist="release debug boot lib"

help()
{

cat << HELP
	./board_name_verify.sh
HELP
	exit 0
}

error()
{
	echo "$1"
	exit 1
}

echo_exec()
{
	echo $1
	$1
}

par_check()
{
	if [ $# -lt 1 ]; then
		echo "parameters not enough."
		help
	fi
}

name_check()
{
	boardlist=`ls conf/$1/`
	for board in ${boardlist}
	do
		for config in ${configlist}
		do
			configboard=`grep "CHIP_BOARD" conf/$1/${board}/${config}.config`
			configboard=${configboard#*= }
			#echo "CHIP_BOARD =${configboard}"
			if [ "${configboard}" != "${board}" ]; then
				echo "warning: conf/$1/${board}/${config}.config CHIP_BOARD is ${configboard}"
			fi
		done
	done
}

main()
{
	name_check gx3201
	name_check gx3113C
}

main "$@"
