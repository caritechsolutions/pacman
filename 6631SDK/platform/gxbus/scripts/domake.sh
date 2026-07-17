#!/bin/bash

CURRENT_PATH=$(cd `dirname $0`; pwd)

function usage() {
	echo "[brmode=<n>] [build_boot=<n>] ./make.sh <SUBMODE> <COMMANDS>"
	echo ""
	echo "  SUBMODE:"
	echo "    release:    release goxceed library"
	echo "    solution:   compile goxceed and solution"
	echo "    gxtest-api: compile goxceed and gxtest/api"
	echo "    gxtest-bus: compile goxceed and gxtest/gxbus"
	echo "    gxtest: compile goxceed and gxtest/gxbus (debug version)"
	echo ""
	echo "  COMMANDS:"
	echo "    help: show details for every submode"
	echo "    ...:  details"
	echo ""
	echo "  [brmode=<n>]: buildroot compile mode for gxtest-*"
	echo "    release: default"
	echo "    debug:   enable kernel debug & gdbserver"
	echo "    nfs:     enable kernel nfs & gdbserver"
	echo "    jenkins: enable buildin nationalchip devices"
	echo ""
	echo "  [build_boot=<n>]: build boot(and boot file) or not for solution|gxtest-*"
	echo "    no: not build boot and boot file"
	echo "    yes: build boot and boot file"
	echo ""
	echo "  eg: ./make.sh release help"
	echo "      ./make.sh release armecos"
	echo "      ./make.sh solution csky ecos"
	echo "      ./make.sh solution arm linux"
	echo "      ./make.sh gxtest-bus help"
	echo "      ./make.sh gxtest-api csky linux taurus 6605H1 dvbs"
	echo "      ./make.sh gxtest-goxceed csky linux taurus 6605H1 dvbs bin"
	echo "      ./make.sh gxtest-chiptest csky linux taurus 6605H1 dvbs bin"
	echo "      ./make.sh gxtest help"
	echo "      brmode=debug ./make.sh solution csky linux"
	echo "      build_boot=yes ./make.sh gxtest-goxceed csky linux taurus 6605H1 dvbs bin"

	exit 1
}

if [ $# -ge 1 ]; then
	case $1 in
		release)
			shift 1
			bash $CURRENT_PATH/goxcced_build $@
			;;
		solution | gxtest-bus | gxtest-api | gxtest-goxceed | gxtest-chiptest)
			bash $CURRENT_PATH/solution_build $@ || { echo "【!!!compiled failed】【${BASH_SOURCE[0]}: $LINENO】Command: bash $CURRENT_PATH/solution_build $@"; exit 1; }
			;;
		gxtest)
			shift 1
			python3 $CURRENT_PATH/gxtest_build.py $@ || { echo "【!!!compiled failed】【${BASH_SOURCE[0]}: $LINENO】Command: python3 $CURRENT_PATH/gxtest_build.py"; exit 1; }
			;;
		*)
			usage
	esac
else
	usage
fi
