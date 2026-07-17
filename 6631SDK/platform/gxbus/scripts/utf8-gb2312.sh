#! /bin/sh

FILE=`find -name "*.h"`

for i in $FILE ; do
	iconv -t=gbk -f=utf8 $i -o "$i"_1
	mv "$i"_1 $i
done
