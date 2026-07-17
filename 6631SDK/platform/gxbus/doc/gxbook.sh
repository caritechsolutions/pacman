#! /bin/bash

TargetPath=$(pwd)/_output
TargetFile=${TargetPath}/_build/html/index.html

function build_book() {
	cp tools/gxddkbuild ./
	if [ "$2" = "all" ]; then
		./tools/gxddkbuild -i $1 -p ./tools/conf.py -s tools/scripts -o html
	else
		./tools/gxddkbuild -i $1 -p ./tools/conf.py -s tools/scripts -t md -o html
	fi
	echo "Open File:"
	echo "       >>  ${TargetFile}"
}

function build_help() {
	echo "./gxbook.sh [file] <all>"
	echo ""
	echo "    eg: ./build clean"
	echo "        ./build player/index.rst"
	echo "        ./build player/index.rst all"
	echo ""
	echo "docker install:"
	echo "cat ./tools/readmine.txt"
}

if [ $# -eq 1 ] ;then
	if [ "$1" = "clean" ]; then
		rm ${TargetPath} -rf
		exit 0
	else
		build_book $1
	fi
elif [ $# -eq 2 ]; then
	build_book $1 $2
else
	build_help
fi
