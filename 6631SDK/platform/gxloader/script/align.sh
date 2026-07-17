#!/bin/sh

num=$1
size=$2
up=$3

if [ ${up} -eq 0 ]; then
	echo $(( ${num} & ~(${size} - 1) ))
else
	echo $(( (${num} + (${size} - 1)) & ~(${size} - 1) ))
fi
